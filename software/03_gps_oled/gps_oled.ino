#include <TinyGPSPlus.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

TinyGPSPlus gps;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup()
{
  Serial.begin(115200);
  Serial1.begin(9600);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println("OLED failed");
    while (true);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
}

void loop()
{
  while (Serial1.available())
  {
    gps.encode(Serial1.read());
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);

  display.println("GPS STATUS");

  display.print("SAT: ");
  display.println(gps.satellites.value());

  display.print("FIX: ");
  display.println(gps.location.isValid() ? "YES" : "NO");

  if (gps.location.isValid())
  {
    display.print("LAT: ");
    display.println(gps.location.lat(), 6);

    display.print("LON: ");
    display.println(gps.location.lng(), 6);
  }
  else
  {
    display.println("Waiting for GPS...");
  }

  display.display();
  delay(1000);
}
