/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : PECRN Field Node - Protocol V1
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

#include "ssd1306.h"
#include "ssd1306_fonts.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef enum
{
    GPS_STATE_SEARCHING = 0,
    GPS_STATE_FIX,
    GPS_STATE_LOST
} GPS_State_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define PROTOCOL_VERSION        "P1"

#define NODE_ID                 1
#define RELAY_ADDRESS           0

#define GPS_SEND_INTERVAL_MS    5000U
#define STATUS_INTERVAL_MS      10000U
#define GPS_FIX_TIMEOUT_MS      5000U

#define RACK_TIMEOUT_MS         2000U
#define MAX_RELAY_RETRIES       3
#define RETRY_DELAY_MS          100U

#define SOS_DEBOUNCE_MS         300U

#define GPS_MIN_FIX_QUALITY     1
#define GPS_MIN_SATELLITES      1
#define GPS_MAX_SATELLITES      64

#define GPS_LINE_SIZE           128
#define LORA_RX_SIZE            180
#define PACKET_SIZE             120
#define LORA_COMMAND_SIZE       160
#define DEBUG_BUFFER_SIZE       220

/*
 * Interrupt-driven LoRa RX ring buffer.
 */
#define LORA_RING_SIZE          512

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart4;
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* ============================================================
 * GPS
 * ============================================================ */

char gps_line[GPS_LINE_SIZE];
uint8_t gps_index = 0;

float latitude = 0.0f;
float longitude = 0.0f;

float last_latitude = 0.0f;
float last_longitude = 0.0f;

int satellites = 0;
int gps_fix = 0;

uint8_t has_last_position = 0;

GPS_State_t gps_state = GPS_STATE_SEARCHING;

uint32_t last_valid_fix_time = 0;


/* ============================================================
 * PACKET COUNTER
 * ============================================================ */

uint32_t packet_id = 0;


/* ============================================================
 * SOS
 * ============================================================ */

volatile uint8_t sos_requested = 0;
volatile uint32_t last_sos_interrupt_time = 0;


/* ============================================================
 * PERIODIC TIMERS
 * ============================================================ */

uint32_t last_gps_send = 0;
uint32_t last_status_send = 0;


/* ============================================================
 * LORA
 * ============================================================ */

/*
 * USART1 receives one byte at a time using interrupts.
 */
volatile uint8_t lora_uart_rx_byte;

/*
 * Software ring buffer.
 */
volatile uint8_t lora_ring[LORA_RING_SIZE];

volatile uint16_t lora_ring_head = 0;
volatile uint16_t lora_ring_tail = 0;

volatile uint8_t lora_ring_overflow = 0;


/*
 * Used when reconstructing unsolicited RYLR998 lines.
 */
char lora_rx_line[LORA_RX_SIZE];
uint16_t lora_rx_index = 0;


/* ============================================================
 * OLED
 * ============================================================ */

uint32_t last_oled_search_update = 0;

/* USER CODE END PV */


/* Private function prototypes -----------------------------------------------*/

void SystemClock_Config(void);

static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_UART4_Init(void);
static void MX_I2C1_Init(void);


/* USER CODE BEGIN PFP */

static void Debug_Print(const char *format, ...);

static void OLED_ShowSearching(void);
static void OLED_ShowGPS(void);
static void OLED_ShowLost(void);
static void OLED_ShowSOS(const char *status);
static void OLED_ShowCommunicationFailure(void);

static void ProcessGPSByte(uint8_t byte);
static void ProcessGPSLine(void);
static void UpdateGPSState(void);
static uint8_t GPSPositionIsValid(void);

static void ProcessUnsolicitedLoRaByte(uint8_t byte);

static uint8_t LoRa_GetByte(uint8_t *byte);
static void FlushLoRaUART(void);

static uint8_t WaitForRelayACK(uint32_t expected_packet_id,
                               uint32_t timeout_ms);

static uint8_t SendPacketToRelayReliable(const char *packet,
                                         uint32_t id,
                                         uint8_t is_sos);

static void SendStatusPacket(void);
static void SendGPSPacket(void);
static void SendSOSPacket(void);

static void RestoreOLEDState(void);

static uint32_t NextPacketID(void);

/* USER CODE END PFP */


/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


/* ============================================================
 * DEBUG
 * ============================================================ */

static void Debug_Print(const char *format, ...)
{
    char buffer[DEBUG_BUFFER_SIZE];

    va_list args;

    va_start(args, format);

    vsnprintf(buffer,
              sizeof(buffer),
              format,
              args);

    va_end(args);

    HAL_UART_Transmit(&huart2,
                      (uint8_t *)buffer,
                      strlen(buffer),
                      100);
}


/* ============================================================
 * PACKET ID
 * ============================================================ */

static uint32_t NextPacketID(void)
{
    packet_id++;

    if (packet_id == 0)
    {
        packet_id = 1;
    }

    return packet_id;
}


/* ============================================================
 * GPS VALIDATION
 * ============================================================ */

static uint8_t GPSPositionIsValid(void)
{
    if (gps_fix < GPS_MIN_FIX_QUALITY)
    {
        return 0;
    }

    if (satellites < GPS_MIN_SATELLITES ||
        satellites > GPS_MAX_SATELLITES)
    {
        return 0;
    }

    if (!isfinite(latitude) ||
        !isfinite(longitude))
    {
        return 0;
    }

    if (latitude < -90.0f ||
        latitude > 90.0f)
    {
        return 0;
    }

    if (longitude < -180.0f ||
        longitude > 180.0f)
    {
        return 0;
    }

    if (fabsf(latitude) < 0.00001f &&
        fabsf(longitude) < 0.00001f)
    {
        return 0;
    }

    return 1;
}


/* ============================================================
 * OLED
 * ============================================================ */

static void OLED_ShowSearching(void)
{
    char buffer[32];

    ssd1306_Fill(Black);

    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString("PECRN FIELD",
                        Font_7x10,
                        White);

    ssd1306_SetCursor(0, 18);
    ssd1306_WriteString("GPS SEARCHING",
                        Font_6x8,
                        White);

    ssd1306_SetCursor(0, 32);

    snprintf(buffer,
             sizeof(buffer),
             "SAT %d",
             satellites);

    ssd1306_WriteString(buffer,
                        Font_6x8,
                        White);

    ssd1306_SetCursor(0, 46);
    ssd1306_WriteString("NODE 1",
                        Font_6x8,
                        White);

    ssd1306_UpdateScreen();
}


static void OLED_ShowGPS(void)
{
    char buffer[32];

    ssd1306_Fill(Black);

    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString("GPS FIX",
                        Font_7x10,
                        White);

    ssd1306_SetCursor(0, 16);

    snprintf(buffer,
             sizeof(buffer),
             "LAT %.5f",
             latitude);

    ssd1306_WriteString(buffer,
                        Font_6x8,
                        White);

    ssd1306_SetCursor(0, 28);

    snprintf(buffer,
             sizeof(buffer),
             "LON %.5f",
             longitude);

    ssd1306_WriteString(buffer,
                        Font_6x8,
                        White);

    ssd1306_SetCursor(0, 40);

    snprintf(buffer,
             sizeof(buffer),
             "SAT %d",
             satellites);

    ssd1306_WriteString(buffer,
                        Font_6x8,
                        White);

    ssd1306_SetCursor(0, 52);

    ssd1306_WriteString("NODE 1",
                        Font_6x8,
                        White);

    ssd1306_UpdateScreen();
}


static void OLED_ShowLost(void)
{
    ssd1306_Fill(Black);

    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString("PECRN FIELD",
                        Font_7x10,
                        White);

    ssd1306_SetCursor(0, 18);
    ssd1306_WriteString("GPS LOST",
                        Font_7x10,
                        White);

    ssd1306_SetCursor(0, 34);

    if (has_last_position)
    {
        ssd1306_WriteString("LAST POS SAVED",
                            Font_6x8,
                            White);
    }
    else
    {
        ssd1306_WriteString("NO POSITION",
                            Font_6x8,
                            White);
    }

    ssd1306_SetCursor(0, 48);

    ssd1306_WriteString("SEARCHING...",
                        Font_6x8,
                        White);

    ssd1306_UpdateScreen();
}


static void OLED_ShowSOS(const char *status)
{
    ssd1306_Fill(Black);

    ssd1306_SetCursor(0, 0);

    ssd1306_WriteString("!!! SOS !!!",
                        Font_11x18,
                        White);

    ssd1306_SetCursor(0, 26);

    ssd1306_WriteString("EMERGENCY",
                        Font_7x10,
                        White);

    ssd1306_SetCursor(0, 42);

    ssd1306_WriteString((char *)status,
                        Font_6x8,
                        White);

    ssd1306_UpdateScreen();
}


static void OLED_ShowCommunicationFailure(void)
{
    ssd1306_Fill(Black);

    ssd1306_SetCursor(0, 0);

    ssd1306_WriteString("PECRN FIELD",
                        Font_7x10,
                        White);

    ssd1306_SetCursor(0, 18);

    ssd1306_WriteString("RADIO ERROR",
                        Font_7x10,
                        White);

    ssd1306_SetCursor(0, 36);

    ssd1306_WriteString("RELAY NO ACK",
                        Font_6x8,
                        White);

    ssd1306_UpdateScreen();
}


static void RestoreOLEDState(void)
{
    if (gps_state == GPS_STATE_FIX)
    {
        OLED_ShowGPS();
    }
    else if (gps_state == GPS_STATE_LOST)
    {
        OLED_ShowLost();
    }
    else
    {
        OLED_ShowSearching();
    }
}


/* ============================================================
 * GPS BYTE PROCESSING
 * ============================================================ */

static void ProcessGPSByte(uint8_t byte)
{
    if (byte == '\n')
    {
        gps_line[gps_index] = '\0';

        ProcessGPSLine();

        gps_index = 0;
    }
    else if (byte != '\r')
    {
        if (gps_index < GPS_LINE_SIZE - 1)
        {
            gps_line[gps_index++] = byte;
        }
        else
        {
            gps_index = 0;
        }
    }
}


/* ============================================================
 * GPS NMEA GGA PARSER
 * ============================================================ */

static void ProcessGPSLine(void)
{
    if ((strncmp(gps_line, "$GPGGA,", 7) != 0) &&
        (strncmp(gps_line, "$GNGGA,", 7) != 0))
    {
        return;
    }

    char copy[GPS_LINE_SIZE];

    strncpy(copy,
            gps_line,
            sizeof(copy) - 1);

    copy[sizeof(copy) - 1] = '\0';

    char *fields[15];

    int fieldCount = 0;

    fields[fieldCount++] = copy;

    for (char *p = copy;
         *p != '\0' && fieldCount < 15;
         p++)
    {
        if (*p == ',')
        {
            *p = '\0';

            fields[fieldCount++] =
                p + 1;
        }
    }

    if (fieldCount < 8)
    {
        Debug_Print(
            "[GPS] Reject: incomplete GGA\r\n");

        return;
    }

    const char *lat_str = fields[2];
    const char *ns_str  = fields[3];
    const char *lon_str = fields[4];
    const char *ew_str  = fields[5];
    const char *fix_str = fields[6];
    const char *sat_str = fields[7];

    if (strlen(lat_str) == 0 ||
        strlen(ns_str) == 0 ||
        strlen(lon_str) == 0 ||
        strlen(ew_str) == 0 ||
        strlen(fix_str) == 0 ||
        strlen(sat_str) == 0)
    {
        gps_fix = 0;
        return;
    }

    char ns = ns_str[0];
    char ew = ew_str[0];

    if (ns != 'N' && ns != 'S')
    {
        gps_fix = 0;
        return;
    }

    if (ew != 'E' && ew != 'W')
    {
        gps_fix = 0;
        return;
    }

    char *fix_end;
    char *sat_end;

    long new_fix_long =
        strtol(fix_str,
               &fix_end,
               10);

    long new_sat_long =
        strtol(sat_str,
               &sat_end,
               10);

    if (fix_end == fix_str ||
        *fix_end != '\0')
    {
        gps_fix = 0;
        return;
    }

    if (sat_end == sat_str ||
        *sat_end != '\0')
    {
        gps_fix = 0;
        return;
    }

    int new_fix =
        (int)new_fix_long;

    int new_satellites =
        (int)new_sat_long;

    if (new_satellites >= 0 &&
        new_satellites <= GPS_MAX_SATELLITES)
    {
        satellites =
            new_satellites;
    }

    if (new_fix < GPS_MIN_FIX_QUALITY)
    {
        gps_fix = 0;
        return;
    }

    if (new_satellites < GPS_MIN_SATELLITES ||
        new_satellites > GPS_MAX_SATELLITES)
    {
        gps_fix = 0;
        return;
    }

    char *lat_end;
    char *lon_end;

    float raw_lat =
        strtof(lat_str,
               &lat_end);

    float raw_lon =
        strtof(lon_str,
               &lon_end);

    if (lat_end == lat_str ||
        *lat_end != '\0')
    {
        gps_fix = 0;
        return;
    }

    if (lon_end == lon_str ||
        *lon_end != '\0')
    {
        gps_fix = 0;
        return;
    }

    if (!isfinite(raw_lat) ||
        !isfinite(raw_lon))
    {
        gps_fix = 0;
        return;
    }

    if (raw_lat <= 0.0f ||
        raw_lon <= 0.0f)
    {
        gps_fix = 0;
        return;
    }

    int lat_deg =
        (int)(raw_lat / 100.0f);

    int lon_deg =
        (int)(raw_lon / 100.0f);

    float lat_minutes =
        raw_lat -
        ((float)lat_deg * 100.0f);

    float lon_minutes =
        raw_lon -
        ((float)lon_deg * 100.0f);

    if (lat_minutes < 0.0f ||
        lat_minutes >= 60.0f ||
        lon_minutes < 0.0f ||
        lon_minutes >= 60.0f)
    {
        gps_fix = 0;
        return;
    }

    if (lat_deg < 0 ||
        lat_deg > 90 ||
        lon_deg < 0 ||
        lon_deg > 180)
    {
        gps_fix = 0;
        return;
    }

    float new_latitude =
        (float)lat_deg +
        lat_minutes / 60.0f;

    float new_longitude =
        (float)lon_deg +
        lon_minutes / 60.0f;

    if (ns == 'S')
    {
        new_latitude =
            -new_latitude;
    }

    if (ew == 'W')
    {
        new_longitude =
            -new_longitude;
    }

    if (!isfinite(new_latitude) ||
        !isfinite(new_longitude))
    {
        gps_fix = 0;
        return;
    }

    if (new_latitude < -90.0f ||
        new_latitude > 90.0f ||
        new_longitude < -180.0f ||
        new_longitude > 180.0f)
    {
        gps_fix = 0;
        return;
    }

    if (fabsf(new_latitude) < 0.00001f &&
        fabsf(new_longitude) < 0.00001f)
    {
        gps_fix = 0;
        return;
    }

    latitude =
        new_latitude;

    longitude =
        new_longitude;

    gps_fix =
        new_fix;

    satellites =
        new_satellites;

    last_latitude =
        latitude;

    last_longitude =
        longitude;

    has_last_position =
        1;

    last_valid_fix_time =
        HAL_GetTick();

    if (gps_state != GPS_STATE_FIX)
    {
        Debug_Print(
            "[GPS] VALID FIX ACQUIRED\r\n");

        Debug_Print(
            "[GPS] LAT %.5f LON %.5f SAT %d FIX %d\r\n",
            latitude,
            longitude,
            satellites,
            gps_fix);
    }

    gps_state =
        GPS_STATE_FIX;

    OLED_ShowGPS();
}


/* ============================================================
 * GPS LOST DETECTION
 * ============================================================ */

static void UpdateGPSState(void)
{
    uint32_t now =
        HAL_GetTick();

    if (gps_state == GPS_STATE_FIX &&
        (now - last_valid_fix_time >
         GPS_FIX_TIMEOUT_MS))
    {
        gps_state =
            GPS_STATE_LOST;

        gps_fix =
            0;

        Debug_Print(
            "[GPS] FIX LOST\r\n");

        OLED_ShowLost();
    }
}


/* ============================================================
 * UNSOLICITED LORA DATA
 * ============================================================ */

static void ProcessUnsolicitedLoRaByte(uint8_t byte)
{
    if (byte == '\n')
    {
        lora_rx_line[lora_rx_index] =
            '\0';

        if (lora_rx_index > 0)
        {
            Debug_Print(
                "[LORA] %s\r\n",
                lora_rx_line);
        }

        lora_rx_index = 0;

        memset(lora_rx_line,
               0,
               sizeof(lora_rx_line));
    }
    else if (byte != '\r')
    {
        if (lora_rx_index <
            LORA_RX_SIZE - 1)
        {
            lora_rx_line[lora_rx_index++] =
                (char)byte;
        }
        else
        {
            lora_rx_index = 0;

            memset(lora_rx_line,
                   0,
                   sizeof(lora_rx_line));
        }
    }
}


/* ============================================================
 * LORA INTERRUPT RING BUFFER
 * ============================================================ */

static uint8_t LoRa_GetByte(uint8_t *byte)
{
    if (lora_ring_tail ==
        lora_ring_head)
    {
        return 0;
    }

    *byte =
        lora_ring[lora_ring_tail];

    lora_ring_tail =
        (uint16_t)(
            (lora_ring_tail + 1U) %
            LORA_RING_SIZE);

    return 1;
}


/* ============================================================
 * FLUSH OLD LORA DATA
 * ============================================================ */

static void FlushLoRaUART(void)
{
    /*
     * USART1 itself remains interrupt-driven.
     *
     * We only discard bytes currently waiting in
     * the software ring buffer.
     */

    __disable_irq();

    lora_ring_tail =
        lora_ring_head;

    lora_ring_overflow =
        0;

    __enable_irq();

    lora_rx_index =
        0;

    memset(lora_rx_line,
           0,
           sizeof(lora_rx_line));
}


/* ============================================================
 * WAIT FOR RELAY RACK
 *
 * Expected RYLR998 response:
 *
 * +RCV=0,<length>,P1,RACK,1,<packetID>,RSSI,SNR
 *
 * USART1 reception itself is interrupt-driven.
 * ============================================================ */

static uint8_t WaitForRelayACK(uint32_t expected_packet_id,
                               uint32_t timeout_ms)
{
    uint32_t start =
        HAL_GetTick();

    char line[LORA_RX_SIZE];

    uint16_t line_index =
        0;

    memset(line,
           0,
           sizeof(line));

    char expected_ack[48];

    snprintf(expected_ack,
             sizeof(expected_ack),
             "%s,RACK,%d,%lu",
             PROTOCOL_VERSION,
             NODE_ID,
             (unsigned long)
             expected_packet_id);

    Debug_Print(
        "[RACK WAIT] Expecting %s\r\n",
        expected_ack);

    while ((HAL_GetTick() - start) <
           timeout_ms)
    {
        uint8_t byte;

        /*
         * Keep GPS alive while waiting.
         */
        while (HAL_UART_Receive(&huart4,
                                &byte,
                                1,
                                1) == HAL_OK)
        {
            ProcessGPSByte(byte);
        }

        /*
         * USART1 interrupt places bytes into
         * the ring buffer.
         */
        if (!LoRa_GetByte(&byte))
        {
            UpdateGPSState();
            continue;
        }

        /*
         * Ignore carriage return.
         */
        if (byte == '\r')
        {
            continue;
        }

        /*
         * Newline = complete RYLR998 response.
         */
        if (byte == '\n')
        {
            if (line_index == 0)
            {
                continue;
            }

            line[line_index] =
                '\0';

            Debug_Print(
                "[LORA RX LINE] %s\r\n",
                line);

            /*
             * Local RYLR998 accepted AT+SEND.
             *
             * This is NOT the relay RACK.
             */
            if (strcmp(line,
                       "+OK") == 0)
            {
                Debug_Print(
                    "[LORA] TX command accepted\r\n");

                line_index = 0;

                memset(line,
                       0,
                       sizeof(line));

                continue;
            }

            /*
             * RYLR998 error.
             */
            if (strncmp(line,
                        "+ERR=",
                        5) == 0)
            {
                Debug_Print(
                    "[LORA ERROR] %s\r\n",
                    line);

                line_index = 0;

                memset(line,
                       0,
                       sizeof(line));

                continue;
            }

            /*
             * Incoming LoRa packet.
             */
            if (strncmp(line,
                        "+RCV=",
                        5) == 0)
            {
                char *first_comma =
                    strchr(line, ',');

                if (first_comma != NULL)
                {
                    char address_buffer[12];

                    size_t address_length =
                        (size_t)(
                            first_comma -
                            (line + 5));

                    if (address_length > 0 &&
                        address_length <
                        sizeof(address_buffer))
                    {
                        memcpy(
                            address_buffer,
                            line + 5,
                            address_length);

                        address_buffer[
                            address_length] =
                            '\0';

                        int source_address =
                            atoi(address_buffer);

                        Debug_Print(
                            "[LORA RX] Source %d\r\n",
                            source_address);

                        /*
                         * RACK must come from relay 0.
                         */
                        if (source_address ==
                            RELAY_ADDRESS)
                        {
                            if (strstr(
                                    line,
                                    expected_ack)
                                != NULL)
                            {
                                Debug_Print(
                                    "[RACK] Packet %lu accepted by relay\r\n",
                                    (unsigned long)
                                    expected_packet_id);

                                return 1;
                            }

                            Debug_Print(
                                "[RACK] Relay packet does not match expected ACK\r\n");
                        }
                        else
                        {
                            Debug_Print(
                                "[RACK] Ignoring LoRa source %d\r\n",
                                source_address);
                        }
                    }
                }
            }

            line_index =
                0;

            memset(line,
                   0,
                   sizeof(line));

            continue;
        }

        /*
         * Normal UART character.
         */
        if (line_index <
            sizeof(line) - 1)
        {
            line[line_index++] =
                (char)byte;
        }
        else
        {
            Debug_Print(
                "[LORA] RX line overflow - discarded\r\n");

            line_index =
                0;

            memset(line,
                   0,
                   sizeof(line));
        }

        UpdateGPSState();
    }

    Debug_Print(
        "[RACK TIMEOUT] Packet %lu\r\n",
        (unsigned long)
        expected_packet_id);

    return 0;
}


/* ============================================================
 * RELIABLE FIELD -> RELAY SEND
 * ============================================================ */

static uint8_t SendPacketToRelayReliable(const char *packet,
                                         uint32_t id,
                                         uint8_t is_sos)
{
    char command[LORA_COMMAND_SIZE];

    size_t payload_length =
        strlen(packet);

    snprintf(command,
             sizeof(command),
             "AT+SEND=%d,%u,%s\r\n",
             RELAY_ADDRESS,
             (unsigned int)
             payload_length,
             packet);

    for (int attempt = 1;
         attempt <= MAX_RELAY_RETRIES;
         attempt++)
    {
        /*
         * SOS has priority over ordinary telemetry.
         */
        if (!is_sos &&
            sos_requested)
        {
            Debug_Print(
                "[TX] Normal packet interrupted by SOS\r\n");

            return 0;
        }

        /*
         * Remove old responses before sending.
         */
        FlushLoRaUART();

        Debug_Print(
            "[TX] Packet %lu attempt %d/%d\r\n",
            (unsigned long)id,
            attempt,
            MAX_RELAY_RETRIES);

        Debug_Print(
            "[TX] %s\r\n",
            packet);

        HAL_UART_Transmit(
            &huart1,
            (uint8_t *)command,
            strlen(command),
            HAL_MAX_DELAY);

        if (WaitForRelayACK(
                id,
                RACK_TIMEOUT_MS))
        {
            return 1;
        }

        /*
         * Short retry delay while still processing GPS.
         */
        uint32_t delay_start =
            HAL_GetTick();

        while ((HAL_GetTick() -
                delay_start) <
               RETRY_DELAY_MS)
        {
            uint8_t gps_byte;

            if (HAL_UART_Receive(
                    &huart4,
                    &gps_byte,
                    1,
                    1) == HAL_OK)
            {
                ProcessGPSByte(
                    gps_byte);
            }

            if (!is_sos &&
                sos_requested)
            {
                return 0;
            }
        }
    }

    Debug_Print(
        "[ERROR] Packet %lu not accepted by relay\r\n",
        (unsigned long)id);

    return 0;
}


/* ============================================================
 * STATUS PACKET
 * ============================================================ */

static void SendStatusPacket(void)
{
    char packet[PACKET_SIZE];

    uint32_t id =
        NextPacketID();

    if (gps_state == GPS_STATE_FIX &&
        GPSPositionIsValid())
    {
        snprintf(
            packet,
            sizeof(packet),
            "%s,STATUS,%d,%lu,GPS_FIX,%d",
            PROTOCOL_VERSION,
            NODE_ID,
            (unsigned long)id,
            satellites);
    }
    else if (gps_state ==
             GPS_STATE_LOST &&
             has_last_position)
    {
        snprintf(
            packet,
            sizeof(packet),
            "%s,STATUS,%d,%lu,GPS_LOST",
            PROTOCOL_VERSION,
            NODE_ID,
            (unsigned long)id);
    }
    else
    {
        snprintf(
            packet,
            sizeof(packet),
            "%s,STATUS,%d,%lu,NO_GPS",
            PROTOCOL_VERSION,
            NODE_ID,
            (unsigned long)id);
    }

    uint8_t success =
        SendPacketToRelayReliable(
            packet,
            id,
            0);

    if (!success &&
        !sos_requested)
    {
        Debug_Print(
            "[STATUS] Relay unavailable\r\n");
    }
}


/* ============================================================
 * GPS PACKET
 * ============================================================ */

static void SendGPSPacket(void)
{
    if (gps_state !=
        GPS_STATE_FIX)
    {
        return;
    }

    if (!GPSPositionIsValid())
    {
        Debug_Print(
            "[GPS] TX BLOCKED: invalid GPS state\r\n");

        gps_fix =
            0;

        if (has_last_position)
        {
            gps_state =
                GPS_STATE_LOST;

            OLED_ShowLost();
        }
        else
        {
            gps_state =
                GPS_STATE_SEARCHING;

            OLED_ShowSearching();
        }

        return;
    }

    char packet[PACKET_SIZE];

    uint32_t id =
        NextPacketID();

    snprintf(
        packet,
        sizeof(packet),
        "%s,GPS,%d,%lu,%.5f,%.5f,%d",
        PROTOCOL_VERSION,
        NODE_ID,
        (unsigned long)id,
        latitude,
        longitude,
        satellites);

    uint8_t success =
        SendPacketToRelayReliable(
            packet,
            id,
            0);

    if (!success &&
        !sos_requested)
    {
        Debug_Print(
            "[GPS] Relay unavailable - telemetry dropped\r\n");
    }
}


/* ============================================================
 * SOS PACKET
 * ============================================================ */

static void SendSOSPacket(void)
{
    char packet[PACKET_SIZE];

    uint32_t id =
        NextPacketID();

    if (gps_state == GPS_STATE_FIX &&
        GPSPositionIsValid())
    {
        snprintf(
            packet,
            sizeof(packet),
            "%s,SOS,%d,%lu,%.5f,%.5f,FIX",
            PROTOCOL_VERSION,
            NODE_ID,
            (unsigned long)id,
            latitude,
            longitude);

        OLED_ShowSOS(
            "GPS CURRENT");
    }
    else if (has_last_position &&
             isfinite(last_latitude) &&
             isfinite(last_longitude) &&
             last_latitude >= -90.0f &&
             last_latitude <= 90.0f &&
             last_longitude >= -180.0f &&
             last_longitude <= 180.0f &&
             !(fabsf(last_latitude) <
                   0.00001f &&
               fabsf(last_longitude) <
                   0.00001f))
    {
        snprintf(
            packet,
            sizeof(packet),
            "%s,SOS,%d,%lu,%.5f,%.5f,LAST_KNOWN",
            PROTOCOL_VERSION,
            NODE_ID,
            (unsigned long)id,
            last_latitude,
            last_longitude);

        OLED_ShowSOS(
            "LAST GPS POS");
    }
    else
    {
        has_last_position =
            0;

        last_latitude =
            0.0f;

        last_longitude =
            0.0f;

        snprintf(
            packet,
            sizeof(packet),
            "%s,SOS,%d,%lu,NO_GPS",
            PROTOCOL_VERSION,
            NODE_ID,
            (unsigned long)id);

        OLED_ShowSOS(
            "NO GPS FIX");
    }

    Debug_Print("\r\n");

    Debug_Print(
        "********************************\r\n");

    Debug_Print(
        "*** SOS BUTTON PRESSED      ***\r\n");

    Debug_Print(
        "*** Packet ID: %-10lu ***\r\n",
        (unsigned long)id);

    Debug_Print(
        "********************************\r\n");

    Debug_Print(
        "[SOS] %s\r\n",
        packet);

    uint8_t accepted =
        SendPacketToRelayReliable(
            packet,
            id,
            1);

    if (accepted)
    {
        Debug_Print(
            "[SOS] Relay accepted emergency packet %lu\r\n",
            (unsigned long)id);

        OLED_ShowSOS(
            "RELAY ACCEPTED");

        uint32_t start =
            HAL_GetTick();

        while ((HAL_GetTick() - start) <
               1000U)
        {
            uint8_t gps_byte;

            if (HAL_UART_Receive(
                    &huart4,
                    &gps_byte,
                    1,
                    1) == HAL_OK)
            {
                ProcessGPSByte(
                    gps_byte);
            }
        }
    }
    else
    {
        Debug_Print(
            "[SOS] CRITICAL: Relay did not accept packet %lu\r\n",
            (unsigned long)id);

        OLED_ShowCommunicationFailure();

        uint32_t start =
            HAL_GetTick();

        while ((HAL_GetTick() - start) <
               2000U)
        {
            uint8_t gps_byte;

            if (HAL_UART_Receive(
                    &huart4,
                    &gps_byte,
                    1,
                    1) == HAL_OK)
            {
                ProcessGPSByte(
                    gps_byte);
            }
        }
    }

    RestoreOLEDState();
}

/* USER CODE END 0 */


/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */


    /* MCU Configuration------------------------------------------------------*/

    HAL_Init();


    /* USER CODE BEGIN Init */

    /* USER CODE END Init */


    SystemClock_Config();


    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */


    /* Initialize all configured peripherals */

    MX_GPIO_Init();

    MX_USART2_UART_Init();

    MX_USART1_UART_Init();

    MX_UART4_Init();

    MX_I2C1_Init();


    /* USER CODE BEGIN 2 */

    ssd1306_Init();

    OLED_ShowSearching();


    /*
     * ========================================================
     * START INTERRUPT-DRIVEN RYLR998 RECEPTION
     * ========================================================
     *
     * Receive one USART1 byte.
     *
     * HAL_UART_RxCpltCallback() will store it and immediately
     * arm reception for the next byte.
     */

    if (HAL_UART_Receive_IT(
            &huart1,
            (uint8_t *)&lora_uart_rx_byte,
            1) != HAL_OK)
    {
        Error_Handler();
    }


    Debug_Print("\r\n");

    Debug_Print(
        "========================================\r\n");

    Debug_Print(
        "PECRN FIELD NODE\r\n");

    Debug_Print(
        "Protocol: %s\r\n",
        PROTOCOL_VERSION);

    Debug_Print(
        "Node ID: %d\r\n",
        NODE_ID);

    Debug_Print(
        "Relay LoRa Address: %d\r\n",
        RELAY_ADDRESS);

    Debug_Print(
        "GPS UART: 9600\r\n");

    Debug_Print(
        "LoRa UART: 115200 INTERRUPT RX\r\n");

    Debug_Print(
        "SOS Button: PC13 / B1\r\n");

    Debug_Print(
        "GPS validation: ENABLED\r\n");

    Debug_Print(
        "RACK timeout: %lu ms\r\n",
        (unsigned long)
        RACK_TIMEOUT_MS);

    Debug_Print(
        "Max relay attempts: %d\r\n",
        MAX_RELAY_RETRIES);

    Debug_Print(
        "========================================\r\n\r\n");


    uint32_t now =
        HAL_GetTick();

    last_gps_send =
        now;

    last_status_send =
        now;

    last_oled_search_update =
        now;

    /* USER CODE END 2 */


    /* Infinite loop */

    /* USER CODE BEGIN WHILE */

    while (1)
    {
        uint8_t byte;


        /*
         * ====================================================
         * GPS
         * ====================================================
         */

        while (HAL_UART_Receive(
                   &huart4,
                   &byte,
                   1,
                   1) == HAL_OK)
        {
            ProcessGPSByte(
                byte);
        }


        /*
         * ====================================================
         * LORA
         * ====================================================
         *
         * Hardware reception is done by USART1 interrupt.
         *
         * Here we process bytes already captured by the ISR.
         */

        while (LoRa_GetByte(&byte))
        {
            ProcessUnsolicitedLoRaByte(
                byte);
        }


        /*
         * Detect software RX buffer overflow.
         */

        if (lora_ring_overflow)
        {
            lora_ring_overflow =
                0;

            Debug_Print(
                "[LORA] WARNING: RX ring buffer overflow\r\n");
        }


        UpdateGPSState();


        /*
         * ====================================================
         * SOS PRIORITY
         * ====================================================
         */

        if (sos_requested)
        {
            sos_requested =
                0;

            SendSOSPacket();

            uint32_t current =
                HAL_GetTick();

            last_gps_send =
                current;

            last_status_send =
                current;

            continue;
        }


        uint32_t current =
            HAL_GetTick();


        /*
         * ====================================================
         * GPS TELEMETRY
         * ====================================================
         */

        if ((gps_state ==
             GPS_STATE_FIX) &&
            (current -
             last_gps_send >=
             GPS_SEND_INTERVAL_MS))
        {
            last_gps_send =
                current;

            SendGPSPacket();

            if (sos_requested)
            {
                continue;
            }
        }


        /*
         * ====================================================
         * STATUS
         * ====================================================
         */

        current =
            HAL_GetTick();

        if (current -
            last_status_send >=
            STATUS_INTERVAL_MS)
        {
            last_status_send =
                current;

            SendStatusPacket();
        }


        /*
         * ====================================================
         * OLED SEARCH UPDATE
         * ====================================================
         */

        current =
            HAL_GetTick();

        if ((gps_state ==
             GPS_STATE_SEARCHING) &&
            (current -
             last_oled_search_update >=
             1000U))
        {
            last_oled_search_update =
                current;

            OLED_ShowSearching();
        }
    }

    /* USER CODE END WHILE */


    /* USER CODE BEGIN 3 */

    /* USER CODE END 3 */
}


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct =
        {0};

    RCC_ClkInitTypeDef RCC_ClkInitStruct =
        {0};


    __HAL_RCC_PWR_CLK_ENABLE();

    __HAL_PWR_VOLTAGESCALING_CONFIG(
        PWR_REGULATOR_VOLTAGE_SCALE3);


    RCC_OscInitStruct.OscillatorType =
        RCC_OSCILLATORTYPE_HSI;

    RCC_OscInitStruct.HSIState =
        RCC_HSI_ON;

    RCC_OscInitStruct.HSICalibrationValue =
        RCC_HSICALIBRATION_DEFAULT;

    RCC_OscInitStruct.PLL.PLLState =
        RCC_PLL_ON;

    RCC_OscInitStruct.PLL.PLLSource =
        RCC_PLLSOURCE_HSI;

    RCC_OscInitStruct.PLL.PLLM =
        16;

    RCC_OscInitStruct.PLL.PLLN =
        336;

    RCC_OscInitStruct.PLL.PLLP =
        RCC_PLLP_DIV4;

    RCC_OscInitStruct.PLL.PLLQ =
        2;

    RCC_OscInitStruct.PLL.PLLR =
        2;


    if (HAL_RCC_OscConfig(
            &RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }


    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1 |
        RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource =
        RCC_SYSCLKSOURCE_PLLCLK;

    RCC_ClkInitStruct.AHBCLKDivider =
        RCC_SYSCLK_DIV1;

    RCC_ClkInitStruct.APB1CLKDivider =
        RCC_HCLK_DIV2;

    RCC_ClkInitStruct.APB2CLKDivider =
        RCC_HCLK_DIV1;


    if (HAL_RCC_ClockConfig(
            &RCC_ClkInitStruct,
            FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}


/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{
    /* USER CODE BEGIN I2C1_Init 0 */

    /* USER CODE END I2C1_Init 0 */


    /* USER CODE BEGIN I2C1_Init 1 */

    /* USER CODE END I2C1_Init 1 */


    hi2c1.Instance =
        I2C1;

    hi2c1.Init.ClockSpeed =
        100000;

    hi2c1.Init.DutyCycle =
        I2C_DUTYCYCLE_2;

    hi2c1.Init.OwnAddress1 =
        0;

    hi2c1.Init.AddressingMode =
        I2C_ADDRESSINGMODE_7BIT;

    hi2c1.Init.DualAddressMode =
        I2C_DUALADDRESS_DISABLE;

    hi2c1.Init.OwnAddress2 =
        0;

    hi2c1.Init.GeneralCallMode =
        I2C_GENERALCALL_DISABLE;

    hi2c1.Init.NoStretchMode =
        I2C_NOSTRETCH_DISABLE;


    if (HAL_I2C_Init(
            &hi2c1) != HAL_OK)
    {
        Error_Handler();
    }


    /* USER CODE BEGIN I2C1_Init 2 */

    /* USER CODE END I2C1_Init 2 */
}


/**
  * @brief UART4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART4_Init(void)
{
    /* USER CODE BEGIN UART4_Init 0 */

    /* USER CODE END UART4_Init 0 */


    /* USER CODE BEGIN UART4_Init 1 */

    /* USER CODE END UART4_Init 1 */


    huart4.Instance =
        UART4;

    huart4.Init.BaudRate =
        9600;

    huart4.Init.WordLength =
        UART_WORDLENGTH_8B;

    huart4.Init.StopBits =
        UART_STOPBITS_1;

    huart4.Init.Parity =
        UART_PARITY_NONE;

    huart4.Init.Mode =
        UART_MODE_TX_RX;

    huart4.Init.HwFlowCtl =
        UART_HWCONTROL_NONE;

    huart4.Init.OverSampling =
        UART_OVERSAMPLING_16;


    if (HAL_UART_Init(
            &huart4) != HAL_OK)
    {
        Error_Handler();
    }


    /* USER CODE BEGIN UART4_Init 2 */

    /* USER CODE END UART4_Init 2 */
}


/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{
    /* USER CODE BEGIN USART1_Init 0 */

    /* USER CODE END USART1_Init 0 */


    /* USER CODE BEGIN USART1_Init 1 */

    /* USER CODE END USART1_Init 1 */


    huart1.Instance =
        USART1;

    huart1.Init.BaudRate =
        115200;

    huart1.Init.WordLength =
        UART_WORDLENGTH_8B;

    huart1.Init.StopBits =
        UART_STOPBITS_1;

    huart1.Init.Parity =
        UART_PARITY_NONE;

    huart1.Init.Mode =
        UART_MODE_TX_RX;

    huart1.Init.HwFlowCtl =
        UART_HWCONTROL_NONE;

    huart1.Init.OverSampling =
        UART_OVERSAMPLING_16;


    if (HAL_UART_Init(
            &huart1) != HAL_OK)
    {
        Error_Handler();
    }


    /* USER CODE BEGIN USART1_Init 2 */

    /* USER CODE END USART1_Init 2 */
}


/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{
    /* USER CODE BEGIN USART2_Init 0 */

    /* USER CODE END USART2_Init 0 */


    /* USER CODE BEGIN USART2_Init 1 */

    /* USER CODE END USART2_Init 1 */


    huart2.Instance =
        USART2;

    huart2.Init.BaudRate =
        115200;

    huart2.Init.WordLength =
        UART_WORDLENGTH_8B;

    huart2.Init.StopBits =
        UART_STOPBITS_1;

    huart2.Init.Parity =
        UART_PARITY_NONE;

    huart2.Init.Mode =
        UART_MODE_TX_RX;

    huart2.Init.HwFlowCtl =
        UART_HWCONTROL_NONE;

    huart2.Init.OverSampling =
        UART_OVERSAMPLING_16;


    if (HAL_UART_Init(
            &huart2) != HAL_OK)
    {
        Error_Handler();
    }


    /* USER CODE BEGIN USART2_Init 2 */

    /* USER CODE END USART2_Init 2 */
}


/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct =
        {0};


    /* USER CODE BEGIN MX_GPIO_Init_1 */

    /* USER CODE END MX_GPIO_Init_1 */


    __HAL_RCC_GPIOC_CLK_ENABLE();

    __HAL_RCC_GPIOH_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();


    HAL_GPIO_WritePin(
        LD2_GPIO_Port,
        LD2_Pin,
        GPIO_PIN_RESET);


    /*
     * SOS BUTTON
     */

    GPIO_InitStruct.Pin =
        B1_Pin;

    GPIO_InitStruct.Mode =
        GPIO_MODE_IT_FALLING;

    GPIO_InitStruct.Pull =
        GPIO_NOPULL;

    HAL_GPIO_Init(
        B1_GPIO_Port,
        &GPIO_InitStruct);


    /*
     * LED
     */

    GPIO_InitStruct.Pin =
        LD2_Pin;

    GPIO_InitStruct.Mode =
        GPIO_MODE_OUTPUT_PP;

    GPIO_InitStruct.Pull =
        GPIO_NOPULL;

    GPIO_InitStruct.Speed =
        GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(
        LD2_GPIO_Port,
        &GPIO_InitStruct);


    /*
     * EXTI interrupt
     */

    HAL_NVIC_SetPriority(
        EXTI15_10_IRQn,
        0,
        0);

    HAL_NVIC_EnableIRQ(
        EXTI15_10_IRQn);


    /* USER CODE BEGIN MX_GPIO_Init_2 */

    /* USER CODE END MX_GPIO_Init_2 */
}


/* USER CODE BEGIN 4 */


/* ============================================================
 * SOS BUTTON INTERRUPT
 * ============================================================ */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == B1_Pin)
    {
        uint32_t now =
            HAL_GetTick();

        if ((now -
             last_sos_interrupt_time) >=
            SOS_DEBOUNCE_MS)
        {
            last_sos_interrupt_time =
                now;

            sos_requested =
                1;

            HAL_GPIO_TogglePin(
                LD2_GPIO_Port,
                LD2_Pin);
        }
    }
}


/* ============================================================
 * USART1 RECEIVE COMPLETE INTERRUPT CALLBACK
 *
 * USART1 = RYLR998
 *
 * Keep this ISR callback short:
 *
 * 1. Store received byte.
 * 2. Advance ring buffer.
 * 3. Re-arm USART1 RX interrupt.
 *
 * Do NOT parse packets or print debug information here.
 * ============================================================ */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        uint16_t next_head =
            (uint16_t)(
                (lora_ring_head + 1U) %
                LORA_RING_SIZE);

        /*
         * Check whether the ring buffer has space.
         */
        if (next_head !=
            lora_ring_tail)
        {
            lora_ring[
                lora_ring_head] =
                lora_uart_rx_byte;

            lora_ring_head =
                next_head;
        }
        else
        {
            /*
             * Ring buffer full.
             *
             * Do not overwrite unread data.
             */
            lora_ring_overflow =
                1;
        }


        /*
         * Re-arm reception immediately for the next byte.
         */
        HAL_UART_Receive_IT(
            &huart1,
            (uint8_t *)&lora_uart_rx_byte,
            1);
    }
}


/* USER CODE END 4 */


/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */

    __disable_irq();

    while (1)
    {
    }

    /* USER CODE END Error_Handler_Debug */
}


#ifdef USE_FULL_ASSERT

/**
  * @brief Reports the name of the source file and source line number
  * @param file pointer to source file name
  * @param line line number
  * @retval None
  */
void assert_failed(uint8_t *file,
                   uint32_t line)
{
    /* USER CODE BEGIN 6 */

    /* USER CODE END 6 */
}

#endif /* USE_FULL_ASSERT */
