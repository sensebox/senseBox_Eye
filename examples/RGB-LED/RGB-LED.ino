// rainbow cycle

#include <Adafruit_NeoPixel.h>

#define LED_PIN 14

Adafruit_NeoPixel pixels(1, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  pixels.begin();
  pixels.setBrightness(30);  
}

uint16_t hue = 0;

void updateLED()
{
    pixels.setPixelColor(0, wheel(hue));
    pixels.show();

    hue += 1;
    if (hue > 255)
    {
        hue = 0;
    }
}

uint32_t wheel(byte WheelPos)
{
    WheelPos = 255 - WheelPos;
    if (WheelPos < 85)
    {
        return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    else if (WheelPos < 170)
    {
        WheelPos -= 85;
        return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
    else
    {
        WheelPos -= 170;
        return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
    }
}


void loop() {
  updateLED();
  delay(5);
}
