/*
 * ============================================================
 * PECRN - RELAY NODE
 * ============================================================
 *
 * Hardware:
 *   Arduino UNO R4 WiFi
 *   RYLR998 LoRa
 *
 * Addresses:
 *   Relay = 0
 *   Field = 1
 *   Base  = 2
 *
 * Protocol:
 *
 * Field telemetry:
 *   P1,STATUS,1,<packetID>,NO_GPS
 *   P1,GPS,1,<packetID>,<lat>,<lon>,<sat>
 *   P1,SOS,1,<packetID>,...
 *
 * Relay acknowledgement:
 *   P1,RACK,1,<packetID>
 *
 * Base acknowledgement:
 *   P1,BACK,1,<packetID>
 *
 * ============================================================
 * FEATURES
 * ============================================================
 *
 * 1. Field -> Relay acknowledgement (RACK)
 * 2. Relay -> Base acknowledgement (BACK)
 * 3. Relay retry
 * 4. Retry backoff
 * 5. 10-packet relay queue
 * 6. Strict FIFO for STATUS/GPS
 * 7. SOS priority
 * 8. Active queue duplicate detection
 * 9. Completed packet duplicate history
 * 10. Radio TX pacing
 * 11. Queue-full protection
 * 12. Field traffic priority / quiet period
 *
 * ============================================================
 */


// ============================================================
// NETWORK CONFIGURATION
// ============================================================

const int RELAY_ADDRESS        = 0;
const int FIELD_NODE_ADDRESS   = 1;
const int BASE_STATION_ADDRESS = 2;

const char *PROTOCOL_VERSION = "P1";


// ============================================================
// QUEUE CONFIGURATION
// ============================================================

const int QUEUE_SIZE = 10;


// ============================================================
// COMPLETED PACKET HISTORY
// ============================================================

const int COMPLETED_CACHE_SIZE = 32;

struct CompletedPacket
{
    bool valid;
    int nodeID;
    unsigned long packetID;
};

CompletedPacket completedCache[COMPLETED_CACHE_SIZE];

int completedCacheWriteIndex = 0;


// ============================================================
// RETRY CONFIGURATION
// ============================================================

const int MAX_BASE_ATTEMPTS = 3;

const unsigned long BACK_TIMEOUT_MS = 2000;

const unsigned long RETRY_BACKOFF_MS = 5000;


// ============================================================
// RADIO PACING
// ============================================================

const unsigned long MIN_RADIO_TX_GAP_MS = 350;

const unsigned long RX_TO_TX_GUARD_MS = 150;

/*
 * NEW:
 *
 * After receiving something from Field, don't immediately
 * start transmitting queued telemetry toward Base.
 *
 * This gives Field <-> Relay ACK traffic priority.
 */
const unsigned long FIELD_QUIET_PERIOD_MS = 1200;


// ============================================================
// PACKET STRUCTURE
// ============================================================

struct QueuePacket
{
    bool used;

    int nodeID;

    unsigned long packetID;

    String type;

    String payload;

    bool sos;

    int attempts;

    unsigned long nextAttemptTime;

    bool waitingForBack;

    unsigned long lastSendTime;

    unsigned long sequence;
};


// ============================================================
// GLOBAL QUEUE
// ============================================================

QueuePacket packetQueue[QUEUE_SIZE];

unsigned long nextSequence = 1;


// ============================================================
// LORA UART BUFFER
// ============================================================

String loraLine = "";


// ============================================================
// RADIO TIMING
// ============================================================

unsigned long lastRadioTransmitTime = 0;

unsigned long lastRadioReceiveTime = 0;

/*
 * NEW:
 *
 * Used to prevent Relay -> Base transmissions immediately
 * after Field traffic.
 */
unsigned long lastFieldPacketTime = 0;


// ============================================================
// FUNCTION PROTOTYPES
// ============================================================

void processLoRaLine(String line);

bool parseReceivedPacket(
    String line,
    int &sourceAddress,
    String &payload
);

bool parseApplicationHeader(
    String payload,
    String &type,
    int &nodeID,
    unsigned long &packetID
);

void processFieldPacket(String payload);

void processBasePacket(String payload);

int findActiveDuplicate(
    int nodeID,
    unsigned long packetID
);

bool wasPacketCompleted(
    int nodeID,
    unsigned long packetID
);

void rememberCompletedPacket(
    int nodeID,
    unsigned long packetID
);

int findFreeQueueSlot();

bool enqueuePacket(
    int nodeID,
    unsigned long packetID,
    String type,
    String payload
);

void sendRack(
    int nodeID,
    unsigned long packetID
);

void processQueue();

int selectNextPacket();

void transmitQueuePacket(int index);

void handleBack(
    int nodeID,
    unsigned long packetID
);

void removeQueuePacket(int index);

bool radioReadyToTransmit();

void sendLoRa(
    int destination,
    String payload
);

int queueCount();

void printQueueUsage();

void readLoRaUART();


// ============================================================
// SETUP
// ============================================================

void setup()
{
    Serial.begin(115200);

    Serial1.begin(115200);

    delay(1000);


    // --------------------------------------------------------
    // INITIALIZE QUEUE
    // --------------------------------------------------------

    for (int i = 0; i < QUEUE_SIZE; i++)
    {
        packetQueue[i].used = false;

        packetQueue[i].waitingForBack = false;
    }


    // --------------------------------------------------------
    // INITIALIZE COMPLETED HISTORY
    // --------------------------------------------------------

    for (int i = 0; i < COMPLETED_CACHE_SIZE; i++)
    {
        completedCache[i].valid = false;

        completedCache[i].nodeID = 0;

        completedCache[i].packetID = 0;
    }


    Serial.println();

    Serial.println(
        "================================================"
    );

    Serial.println(
        "PECRN RELAY NODE"
    );

    Serial.println(
        "================================================"
    );

    Serial.println(
        "Protocol: P1"
    );

    Serial.println(
        "Relay Address: 0"
    );

    Serial.println(
        "Field Address: 1"
    );

    Serial.println(
        "Base Address: 2"
    );

    Serial.println(
        "Queue Size: 10"
    );

    Serial.println(
        "Completed History: 32 packets"
    );

    Serial.println(
        "Normal FIFO: ENABLED"
    );

    Serial.println(
        "SOS Priority: ENABLED"
    );

    Serial.println(
        "Duplicate Detection: ENABLED"
    );

    Serial.println(
        "Retry: ENABLED"
    );

    Serial.println(
        "Radio Pacing: ENABLED"
    );

    Serial.println(
        "Field Traffic Priority: ENABLED"
    );

    Serial.print(
        "Field Quiet Period: "
    );

    Serial.print(
        FIELD_QUIET_PERIOD_MS
    );

    Serial.println(
        " ms"
    );

    Serial.println(
        "================================================"
    );

    Serial.println();

    Serial.println(
        "[RELAY] Waiting for packets..."
    );
}


// ============================================================
// MAIN LOOP
// ============================================================

void loop()
{
    /*
     * Read everything currently available from LoRa.
     */

    readLoRaUART();


    /*
     * Service delivery queue.
     */

    processQueue();
}


// ============================================================
// READ LORA UART
// ============================================================

void readLoRaUART()
{
    while (Serial1.available())
    {
        char c = Serial1.read();


        if (c == '\n')
        {
            if (loraLine.length() > 0)
            {
                processLoRaLine(loraLine);
            }

            loraLine = "";
        }

        else if (c != '\r')
        {
            if (loraLine.length() < 250)
            {
                loraLine += c;
            }

            else
            {
                Serial.println(
                    "[ERROR] LoRa line overflow"
                );

                loraLine = "";
            }
        }
    }
}


// ============================================================
// PROCESS RYLR998 UART LINE
// ============================================================

void processLoRaLine(String line)
{
    line.trim();


    if (line.length() == 0)
    {
        return;
    }


    // --------------------------------------------------------
    // AT COMMAND SUCCESS
    // --------------------------------------------------------

    if (line == "+OK")
    {
        return;
    }


    // --------------------------------------------------------
    // RADIO ERROR
    // --------------------------------------------------------

    if (line.startsWith("+ERR="))
    {
        Serial.print(
            "[LORA ERROR] "
        );

        Serial.println(line);

        return;
    }


    // --------------------------------------------------------
    // NOT A RADIO RECEIVE PACKET
    // --------------------------------------------------------

    if (!line.startsWith("+RCV="))
    {
        Serial.print(
            "[LORA] "
        );

        Serial.println(line);

        return;
    }


    int sourceAddress = -1;

    String payload;


    if (!parseReceivedPacket(
            line,
            sourceAddress,
            payload))
    {
        Serial.print(
            "[DROP] Invalid RYLR998 packet: "
        );

        Serial.println(line);

        return;
    }


    lastRadioReceiveTime = millis();


    Serial.print(
        "[RX] LoRa source: "
    );

    Serial.println(
        sourceAddress
    );


    Serial.print(
        "[RX] "
    );

    Serial.println(
        payload
    );


    // --------------------------------------------------------
    // FIELD -> RELAY
    // --------------------------------------------------------

    if (sourceAddress ==
        FIELD_NODE_ADDRESS)
    {
        /*
         * NEW:
         *
         * Record the time of Field traffic.
         *
         * processQueue() will use this to delay Base
         * transmissions.
         */
        lastFieldPacketTime =
            millis();


        processFieldPacket(
            payload
        );


        return;
    }


    // --------------------------------------------------------
    // BASE -> RELAY
    // --------------------------------------------------------

    if (sourceAddress ==
        BASE_STATION_ADDRESS)
    {
        processBasePacket(
            payload
        );

        return;
    }


    Serial.print(
        "[DROP] Unexpected LoRa source "
    );

    Serial.println(
        sourceAddress
    );
}


// ============================================================
// PARSE RYLR998 RECEIVE PACKET
//
// Format:
//
// +RCV=<address>,<length>,<data>,<RSSI>,<SNR>
//
// Payload contains commas, therefore payload LENGTH is used.
// ============================================================

bool parseReceivedPacket(
    String line,
    int &sourceAddress,
    String &payload
)
{
    int firstComma =
        line.indexOf(',');


    if (firstComma < 0)
    {
        return false;
    }


    sourceAddress =
        line.substring(
            5,
            firstComma
        ).toInt();


    int secondComma =
        line.indexOf(
            ',',
            firstComma + 1
        );


    if (secondComma < 0)
    {
        return false;
    }


    int payloadLength =
        line.substring(
            firstComma + 1,
            secondComma
        ).toInt();


    if (payloadLength <= 0)
    {
        return false;
    }


    int payloadStart =
        secondComma + 1;


    int payloadEnd =
        payloadStart +
        payloadLength;


    if (payloadEnd >
        line.length())
    {
        return false;
    }


    payload =
        line.substring(
            payloadStart,
            payloadEnd
        );


    return true;
}


// ============================================================
// PARSE PECRN APPLICATION HEADER
//
// P1,<TYPE>,<NODE>,<PACKET_ID>,...
// ============================================================

bool parseApplicationHeader(
    String payload,
    String &type,
    int &nodeID,
    unsigned long &packetID
)
{
    int comma1 =
        payload.indexOf(',');


    if (comma1 < 0)
    {
        return false;
    }


    int comma2 =
        payload.indexOf(
            ',',
            comma1 + 1
        );


    if (comma2 < 0)
    {
        return false;
    }


    int comma3 =
        payload.indexOf(
            ',',
            comma2 + 1
        );


    if (comma3 < 0)
    {
        return false;
    }


    int comma4 =
        payload.indexOf(
            ',',
            comma3 + 1
        );


    String version =
        payload.substring(
            0,
            comma1
        );


    if (version !=
        PROTOCOL_VERSION)
    {
        return false;
    }


    type =
        payload.substring(
            comma1 + 1,
            comma2
        );


    nodeID =
        payload.substring(
            comma2 + 1,
            comma3
        ).toInt();


    String packetString;


    if (comma4 < 0)
    {
        packetString =
            payload.substring(
                comma3 + 1
            );
    }

    else
    {
        packetString =
            payload.substring(
                comma3 + 1,
                comma4
            );
    }


    packetID =
        strtoul(
            packetString.c_str(),
            NULL,
            10
        );


    if (nodeID <= 0)
    {
        return false;
    }


    if (packetID == 0)
    {
        return false;
    }


    return true;
}


// ============================================================
// PROCESS FIELD PACKET
// ============================================================

void processFieldPacket(String payload)
{
    String type;

    int nodeID = 0;

    unsigned long packetID = 0;


    if (!parseApplicationHeader(
            payload,
            type,
            nodeID,
            packetID))
    {
        Serial.println(
            "[DROP] Invalid Field packet"
        );

        return;
    }


    if (nodeID !=
        FIELD_NODE_ADDRESS)
    {
        Serial.println(
            "[DROP] Wrong Field node ID"
        );

        return;
    }


    if (type != "STATUS" &&
        type != "GPS" &&
        type != "SOS")
    {
        Serial.print(
            "[DROP] Unknown Field type: "
        );

        Serial.println(
            type
        );

        return;
    }


    // ========================================================
    // DUPLICATE CHECK #1 - ACTIVE QUEUE
    // ========================================================

    int activeDuplicate =
        findActiveDuplicate(
            nodeID,
            packetID
        );


    if (activeDuplicate >= 0)
    {
        Serial.print(
            "[DUPLICATE ACTIVE] Node "
        );

        Serial.print(
            nodeID
        );

        Serial.print(
            " Packet "
        );

        Serial.println(
            packetID
        );


        /*
         * Packet is already stored.
         * Field probably missed the previous RACK.
         */

        sendRack(
            nodeID,
            packetID
        );


        return;
    }


    // ========================================================
    // DUPLICATE CHECK #2 - COMPLETED HISTORY
    // ========================================================

    if (wasPacketCompleted(
            nodeID,
            packetID))
    {
        Serial.print(
            "[DUPLICATE COMPLETED] Node "
        );

        Serial.print(
            nodeID
        );

        Serial.print(
            " Packet "
        );

        Serial.println(
            packetID
        );


        /*
         * Packet already reached Base.
         *
         * Do not enqueue or forward it again.
         * Repeat the RACK only.
         */

        sendRack(
            nodeID,
            packetID
        );


        return;
    }


    // ========================================================
    // NEW PACKET
    // ========================================================

    bool accepted =
        enqueuePacket(
            nodeID,
            packetID,
            type,
            payload
        );


    if (!accepted)
    {
        /*
         * Queue full.
         *
         * Do NOT RACK because the packet was not safely stored.
         */

        Serial.print(
            "[QUEUE FULL] Packet "
        );

        Serial.print(
            packetID
        );

        Serial.println(
            " NOT acknowledged"
        );


        return;
    }


    /*
     * Packet is safely stored.
     */

    sendRack(
        nodeID,
        packetID
    );
}


// ============================================================
// PROCESS BASE PACKET
// ============================================================

void processBasePacket(String payload)
{
    String type;

    int nodeID = 0;

    unsigned long packetID = 0;


    if (!parseApplicationHeader(
            payload,
            type,
            nodeID,
            packetID))
    {
        Serial.println(
            "[DROP] Invalid Base packet"
        );

        return;
    }


    if (type != "BACK")
    {
        Serial.print(
            "[DROP] Unknown Base packet type: "
        );

        Serial.println(
            type
        );

        return;
    }


    handleBack(
        nodeID,
        packetID
    );
}


// ============================================================
// FIND ACTIVE DUPLICATE
// ============================================================

int findActiveDuplicate(
    int nodeID,
    unsigned long packetID
)
{
    for (int i = 0;
         i < QUEUE_SIZE;
         i++)
    {
        if (!packetQueue[i].used)
        {
            continue;
        }


        if (packetQueue[i].nodeID ==
                nodeID &&
            packetQueue[i].packetID ==
                packetID)
        {
            return i;
        }
    }


    return -1;
}


// ============================================================
// CHECK COMPLETED HISTORY
// ============================================================

bool wasPacketCompleted(
    int nodeID,
    unsigned long packetID
)
{
    for (int i = 0;
         i < COMPLETED_CACHE_SIZE;
         i++)
    {
        if (!completedCache[i].valid)
        {
            continue;
        }


        if (completedCache[i].nodeID ==
                nodeID &&
            completedCache[i].packetID ==
                packetID)
        {
            return true;
        }
    }


    return false;
}


// ============================================================
// REMEMBER COMPLETED PACKET
// ============================================================

void rememberCompletedPacket(
    int nodeID,
    unsigned long packetID
)
{
    if (wasPacketCompleted(
            nodeID,
            packetID))
    {
        return;
    }


    completedCache[
        completedCacheWriteIndex
    ].valid = true;


    completedCache[
        completedCacheWriteIndex
    ].nodeID = nodeID;


    completedCache[
        completedCacheWriteIndex
    ].packetID = packetID;


    Serial.print(
        "[HISTORY] Remembered Node "
    );

    Serial.print(
        nodeID
    );

    Serial.print(
        " Packet "
    );

    Serial.println(
        packetID
    );


    completedCacheWriteIndex++;


    if (completedCacheWriteIndex >=
        COMPLETED_CACHE_SIZE)
    {
        completedCacheWriteIndex = 0;
    }
}


// ============================================================
// FIND FREE QUEUE SLOT
// ============================================================

int findFreeQueueSlot()
{
    for (int i = 0;
         i < QUEUE_SIZE;
         i++)
    {
        if (!packetQueue[i].used)
        {
            return i;
        }
    }


    return -1;
}


// ============================================================
// ADD PACKET TO QUEUE
// ============================================================

bool enqueuePacket(
    int nodeID,
    unsigned long packetID,
    String type,
    String payload
)
{
    int slot =
        findFreeQueueSlot();


    if (slot < 0)
    {
        return false;
    }


    packetQueue[slot].used =
        true;


    packetQueue[slot].nodeID =
        nodeID;


    packetQueue[slot].packetID =
        packetID;


    packetQueue[slot].type =
        type;


    packetQueue[slot].payload =
        payload;


    packetQueue[slot].sos =
        (type == "SOS");


    packetQueue[slot].attempts =
        0;


    packetQueue[slot].nextAttemptTime =
        millis();


    packetQueue[slot].waitingForBack =
        false;


    packetQueue[slot].lastSendTime =
        0;


    packetQueue[slot].sequence =
        nextSequence++;


    if (nextSequence == 0)
    {
        nextSequence = 1;
    }


    Serial.print(
        "[QUEUE] Accepted packet "
    );

    Serial.println(
        packetID
    );


    printQueueUsage();


    if (type == "SOS")
    {
        Serial.println(
            "[QUEUE] *** SOS PRIORITY ***"
        );
    }


    return true;
}


// ============================================================
// RADIO READY CHECK
// ============================================================

bool radioReadyToTransmit()
{
    unsigned long now =
        millis();


    if ((now -
         lastRadioTransmitTime) <
        MIN_RADIO_TX_GAP_MS)
    {
        return false;
    }


    if ((now -
         lastRadioReceiveTime) <
        RX_TO_TX_GUARD_MS)
    {
        return false;
    }


    return true;
}


// ============================================================
// RAW LORA SEND
// ============================================================

void sendLoRa(
    int destination,
    String payload
)
{
    String command =
        "AT+SEND=" +
        String(destination) +
        "," +
        String(payload.length()) +
        "," +
        payload;


    Serial1.print(
        command
    );


    Serial1.print(
        "\r\n"
    );


    lastRadioTransmitTime =
        millis();
}


// ============================================================
// SEND RACK TO FIELD
// ============================================================

void sendRack(
    int nodeID,
    unsigned long packetID
)
{
    String rack =
        String(PROTOCOL_VERSION) +
        ",RACK," +
        String(nodeID) +
        "," +
        String(packetID);


    unsigned long start =
        millis();


    /*
     * Wait for radio pacing.
     *
     * IMPORTANT:
     * Unlike the previous version, we do NOT transmit anyway
     * if the radio never becomes ready.
     */
    while (!radioReadyToTransmit())
    {
        readLoRaUART();


        if ((millis() - start) >
            1500)
        {
            Serial.print(
                "[RACK ERROR] Radio busy, could not ACK packet "
            );

            Serial.println(
                packetID
            );


            return;
        }
    }


    sendLoRa(
        FIELD_NODE_ADDRESS,
        rack
    );


    Serial.print(
        "[RACK -> FIELD] "
    );

    Serial.println(
        rack
    );
}


// ============================================================
// QUEUE PROCESSOR
// ============================================================

void processQueue()
{
    unsigned long now =
        millis();


    // ========================================================
    // HANDLE BACK TIMEOUTS
    // ========================================================

    for (int i = 0;
         i < QUEUE_SIZE;
         i++)
    {
        if (!packetQueue[i].used)
        {
            continue;
        }


        if (!packetQueue[i].waitingForBack)
        {
            continue;
        }


        if ((now -
             packetQueue[i].lastSendTime) <
            BACK_TIMEOUT_MS)
        {
            continue;
        }


        packetQueue[i].waitingForBack =
            false;


        Serial.print(
            "[BACK TIMEOUT] Packet "
        );

        Serial.println(
            packetQueue[i].packetID
        );


        // ----------------------------------------------------
        // MAX ATTEMPTS REACHED
        // ----------------------------------------------------

        if (packetQueue[i].attempts >=
            MAX_BASE_ATTEMPTS)
        {
            Serial.print(
                "[BACKOFF] Packet "
            );

            Serial.print(
                packetQueue[i].packetID
            );

            Serial.println(
                " retained for later retry"
            );


            packetQueue[i].attempts =
                0;


            packetQueue[i].nextAttemptTime =
                now +
                RETRY_BACKOFF_MS;
        }

        else
        {
            packetQueue[i].nextAttemptTime =
                now +
                MIN_RADIO_TX_GAP_MS;
        }
    }


    // ========================================================
    // NEW: FIELD TRAFFIC PRIORITY
    // ========================================================

    /*
     * Do not start a Relay -> Base transmission immediately
     * after receiving Field traffic.
     *
     * This leaves a quiet window for Field -> Relay and
     * Relay -> Field RACK exchanges.
     */
    if ((now -
         lastFieldPacketTime) <
        FIELD_QUIET_PERIOD_MS)
    {
        return;
    }


    // ========================================================
    // RADIO PACING
    // ========================================================

    if (!radioReadyToTransmit())
    {
        return;
    }


    int selected =
        selectNextPacket();


    if (selected < 0)
    {
        return;
    }


    transmitQueuePacket(
        selected
    );
}


// ============================================================
// SELECT NEXT PACKET
//
// SOS:
//     Priority over normal telemetry.
//
// NORMAL:
//     Strict FIFO.
// ============================================================

int selectNextPacket()
{
    unsigned long now =
        millis();


    // ========================================================
    // FIRST: FIND OLDEST ELIGIBLE SOS
    // ========================================================

    int oldestSOS =
        -1;


    unsigned long oldestSOSSequence =
        0;


    for (int i = 0;
         i < QUEUE_SIZE;
         i++)
    {
        if (!packetQueue[i].used)
        {
            continue;
        }


        if (!packetQueue[i].sos)
        {
            continue;
        }


        if (packetQueue[i].waitingForBack)
        {
            continue;
        }


        if (now <
            packetQueue[i].nextAttemptTime)
        {
            continue;
        }


        if (oldestSOS < 0 ||
            packetQueue[i].sequence <
                oldestSOSSequence)
        {
            oldestSOS =
                i;


            oldestSOSSequence =
                packetQueue[i].sequence;
        }
    }


    if (oldestSOS >= 0)
    {
        return oldestSOS;
    }


    // ========================================================
    // NORMAL FIFO
    // ========================================================

    int oldestNormal =
        -1;


    unsigned long oldestNormalSequence =
        0;


    for (int i = 0;
         i < QUEUE_SIZE;
         i++)
    {
        if (!packetQueue[i].used)
        {
            continue;
        }


        if (packetQueue[i].sos)
        {
            continue;
        }


        if (oldestNormal < 0 ||
            packetQueue[i].sequence <
                oldestNormalSequence)
        {
            oldestNormal =
                i;


            oldestNormalSequence =
                packetQueue[i].sequence;
        }
    }


    if (oldestNormal < 0)
    {
        return -1;
    }


    /*
     * Don't allow newer normal packets to jump ahead while
     * the oldest packet is waiting for BACK.
     */
    if (packetQueue[
            oldestNormal
        ].waitingForBack)
    {
        return -1;
    }


    /*
     * Oldest packet is currently backing off.
     *
     * Preserve FIFO.
     */
    if (now <
        packetQueue[
            oldestNormal
        ].nextAttemptTime)
    {
        return -1;
    }


    return oldestNormal;
}


// ============================================================
// SEND QUEUED PACKET TO BASE
// ============================================================

void transmitQueuePacket(int index)
{
    if (index < 0 ||
        index >= QUEUE_SIZE)
    {
        return;
    }


    if (!packetQueue[index].used)
    {
        return;
    }


    packetQueue[index].attempts++;


    Serial.print(
        "[BASE TX] "
    );

    Serial.print(
        packetQueue[index].type
    );

    Serial.print(
        " Node "
    );

    Serial.print(
        packetQueue[index].nodeID
    );

    Serial.print(
        " Packet "
    );

    Serial.print(
        packetQueue[index].packetID
    );

    Serial.print(
        " Attempt "
    );

    Serial.print(
        packetQueue[index].attempts
    );

    Serial.print(
        "/"
    );

    Serial.println(
        MAX_BASE_ATTEMPTS
    );


    Serial.print(
        "[BASE TX] "
    );

    Serial.println(
        packetQueue[index].payload
    );


    sendLoRa(
        BASE_STATION_ADDRESS,
        packetQueue[index].payload
    );


    packetQueue[index].lastSendTime =
        millis();


    packetQueue[index].waitingForBack =
        true;
}


// ============================================================
// HANDLE BASE ACKNOWLEDGEMENT
// ============================================================

void handleBack(
    int nodeID,
    unsigned long packetID
)
{
    Serial.print(
        "[BACK] Node "
    );

    Serial.print(
        nodeID
    );

    Serial.print(
        " packet "
    );

    Serial.println(
        packetID
    );


    for (int i = 0;
         i < QUEUE_SIZE;
         i++)
    {
        if (!packetQueue[i].used)
        {
            continue;
        }


        if (packetQueue[i].nodeID ==
                nodeID &&
            packetQueue[i].packetID ==
                packetID)
        {
            Serial.print(
                "[DELIVERED] Base confirmed packet "
            );

            Serial.println(
                packetID
            );


            /*
             * Remember BEFORE removing from active queue.
             */
            rememberCompletedPacket(
                nodeID,
                packetID
            );


            removeQueuePacket(
                i
            );


            printQueueUsage();


            return;
        }
    }


    /*
     * Duplicate BACK is harmless.
     */
    if (wasPacketCompleted(
            nodeID,
            packetID))
    {
        Serial.print(
            "[BACK DUPLICATE] Packet "
        );

        Serial.print(
            packetID
        );

        Serial.println(
            " already completed"
        );


        return;
    }


    Serial.print(
        "[BACK UNKNOWN] Packet "
    );

    Serial.println(
        packetID
    );
}


// ============================================================
// REMOVE QUEUE ENTRY
// ============================================================

void removeQueuePacket(int index)
{
    packetQueue[index].used =
        false;


    packetQueue[index].nodeID =
        0;


    packetQueue[index].packetID =
        0;


    packetQueue[index].type =
        "";


    packetQueue[index].payload =
        "";


    packetQueue[index].sos =
        false;


    packetQueue[index].attempts =
        0;


    packetQueue[index].nextAttemptTime =
        0;


    packetQueue[index].waitingForBack =
        false;


    packetQueue[index].lastSendTime =
        0;


    packetQueue[index].sequence =
        0;
}


// ============================================================
// COUNT QUEUE
// ============================================================

int queueCount()
{
    int count = 0;


    for (int i = 0;
         i < QUEUE_SIZE;
         i++)
    {
        if (packetQueue[i].used)
        {
            count++;
        }
    }


    return count;
}


// ============================================================
// PRINT QUEUE
// ============================================================

void printQueueUsage()
{
    Serial.print(
        "[QUEUE] Usage "
    );

    Serial.print(
        queueCount()
    );

    Serial.print(
        "/"
    );

    Serial.println(
        QUEUE_SIZE
    );
}
