#include <TinyGPSPlus.h>

TinyGPSPlus gps;

void setup()
{
  Serial.begin(115200);
  Serial1.begin(9600);
}

void loop()
{
  while (Serial1.available())
  {
    gps.encode(Serial1.read());
  }

  if (gps.location.isUpdated())
  {
    Serial.print("Latitude: ");
    Serial.println(gps.location.lat(), 6);

    Serial.print("Longitude: ");
    Serial.println(gps.location.lng(), 6);

    Serial.print("Satellites: ");
    Serial.println(gps.satellites.value());

    Serial.println("----------------");
  }
}
