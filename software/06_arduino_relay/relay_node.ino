/*
 * PECRN - LoRa Relay Node
 *
 * Hardware:
 * Arduino UNO R4 WiFi
 * RYLR998 LoRa Module
 *
 * Relay LoRa Address: 0
 * Field Node Address: 1
 * Base Station Address: 2
 *
 * Network ID: 18
 * Band: 915 MHz
 * UART: 115200
 */

String loraLine = "";

const int FIELD_NODE_ADDRESS = 1;
const int BASE_STATION_ADDRESS = 2;

void setup()
{
  Serial.begin(115200);
  Serial1.begin(115200);

  delay(1000);

  Serial.println("PECRN Relay Node Started");
  Serial.println("Waiting for Field Node...");
}

void loop()
{
  while (Serial1.available())
  {
    char c = Serial1.read();

    if (c == '\n')
    {
      processLoRaLine(loraLine);
      loraLine = "";
    }
    else if (c != '\r')
    {
      loraLine += c;
    }
  }
}

void processLoRaLine(String line)
{
  if (!line.startsWith("+RCV="))
  {
    return;
  }

  int firstComma = line.indexOf(',');

  if (firstComma == -1)
  {
    return;
  }

  int sourceAddress =
      line.substring(5, firstComma).toInt();

  if (sourceAddress != FIELD_NODE_ADDRESS)
  {
    return;
  }

  int secondComma =
      line.indexOf(',', firstComma + 1);

  if (secondComma == -1)
  {
    return;
  }

  int payloadLength =
      line.substring(
          firstComma + 1,
          secondComma
      ).toInt();

  if (payloadLength <= 0)
  {
    return;
  }

  int payloadStart = secondComma + 1;
  int payloadEnd = payloadStart + payloadLength;

  if (payloadEnd > line.length())
  {
    return;
  }

  String payload =
      line.substring(
          payloadStart,
          payloadEnd
      );

  Serial.print("Received from Node ");
  Serial.print(sourceAddress);
  Serial.print(": ");
  Serial.println(payload);

  String forwardCommand =
      "AT+SEND=" +
      String(BASE_STATION_ADDRESS) +
      "," +
      String(payload.length()) +
      "," +
      payload;

  Serial.print("Forwarding to Node ");
  Serial.print(BASE_STATION_ADDRESS);
  Serial.print(": ");
  Serial.println(payload);

  Serial1.print(forwardCommand);
  Serial1.print("\r\n");
}
