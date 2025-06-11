#include <Wire.h>
#include <Adafruit_MAX1704X.h>
#include <Adafruit_GFX.h> // http://librarymanager/All#Adafruit_GFX_Library
#include <Adafruit_SSD1306.h> // http://librarymanager/All#Adafruit_SSD1306
#include <Adafruit_HDC1000.h> // http://librarymanager/All#Adafruit_HDC1000_Library

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define PIN_QWIIC_SDA 2
#define PIN_QWIIC_SCL 1

Adafruit_MAX17048 maxlipo;

void setup() {
  Serial.begin(115200);
  Wire.begin(PIN_QWIIC_SDA,PIN_QWIIC_SCL);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3D);
  display.setRotation(2);
  display.setCursor(0,0);
  display.setTextSize(1);
  display.setTextColor(WHITE,BLACK);
  display.display();
  delay(100);
  display.clearDisplay();
  while (!maxlipo.begin())
  {
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Couldnt find Adafruit MAX17048?\nMake sure a battery is plugged in!");
    display.display();
    Serial.println(F("Couldnt find Adafruit MAX17048?\nMake sure a battery is plugged in!"));
    delay(2000);
  }
  Serial.print(F("Found MAX17048"));
  Serial.print(F(" with Chip ID: 0x")); 
  Serial.println(maxlipo.getChipID(), HEX);

  Serial.print(F("Reset voltage = ")); 
  Serial.print(maxlipo.getResetVoltage());
  Serial.println(" V");

  delay(1000);
}

void loop() {
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Level: " + String(maxlipo.cellPercent()) + "%");
  display.println("Voltage: " + String(maxlipo.cellVoltage()) + " V");
  display.println("Charge: " + String(maxlipo.chargeRate()) + " %/hr");
  display.display();
  Serial.println("Battery Level: " + String(maxlipo.cellPercent()) + "%");
  Serial.println("Battery Voltage: " + String(maxlipo.cellVoltage()) + " V");
  Serial.print(F("(Dis)Charge rate : ")); 
  Serial.print(maxlipo.chargeRate(), 1); 
  Serial.println(" %/hr");
  delay(1000);
}