/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "Arduino.h"
#include "Adafruit_NeoPixel.h"
extern "C" {
#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
}

extern "C" {
    void app_main(void);

    Adafruit_NeoPixel pixels(1, 14, NEO_GRB + NEO_KHZ800);
    uint16_t hue = 0;

    void updateLED();
    uint32_t wheel(uint8_t WheelPos);
}

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

void app_main(void)
{
    pixels.begin();
    pixels.setBrightness(30);  
    while(true)
    {
        updateLED();
        vTaskDelay(5 / portTICK_PERIOD_MS);  // Delay for 50 milliseconds
    }
}
