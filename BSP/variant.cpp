/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Ha Thach (tinyusb.org) for Adafruit Industries
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "esp32-hal-gpio.h"
#include "pins_arduino.h"
#include "driver/rmt.h"

extern "C" {

// Initialize variant/board, called before setup()
void initVariant(void) {
  // *************** blink LED ***************
  rmt_config_t config = RMT_DEFAULT_CONFIG_TX((gpio_num_t)PIN_LED, RMT_CHANNEL_0);
  config.clk_div = 2;
  rmt_config(&config);
  rmt_driver_install(RMT_CHANNEL_0, 0, 0);

  rmt_item32_t rmt_green[24] = {
    // G
    {14, 1, 34, 0},{14, 1, 34, 0},
    {14, 1, 34, 0},{14, 1, 34, 0},
    {34, 1, 34, 0},{14, 1, 14, 0},
    {34, 1, 34, 0},{14, 1, 34, 0},
    // R
    {14, 1, 34, 0},{14, 1, 34, 0},
    {14, 1, 34, 0},{14, 1, 34, 0},
    {14, 1, 34, 0},{14, 1, 14, 0},
    {14, 1, 34, 0},{14, 1, 34, 0},
    // B
    {14, 1, 34, 0},{14, 1, 34, 0},
    {14, 1, 34, 0},{14, 1, 34, 0},
    {14, 1, 14, 0},{34, 1, 34, 0},
    {14, 1, 34, 0},{14, 1, 34, 0}
  };
  rmt_write_items(RMT_CHANNEL_0, rmt_green, 24, true);
  rmt_wait_tx_done(RMT_CHANNEL_0, pdMS_TO_TICKS(10));
  delay(80);
  rmt_item32_t rmt_black[24] = {
    // G
    {14, 1, 34, 0},{14, 1, 34, 0},
    {14, 1, 34, 0},{14, 1, 34, 0},
    {14, 1, 34, 0},{14, 1, 34, 0},
    {14, 1, 34, 0},{14, 1, 34, 0},
    // R
    {14, 1, 34, 0},{14, 1, 34, 0},
    {14, 1, 34, 0},{14, 1, 34, 0},
    {14, 1, 34, 0},{14, 1, 34, 0},
    {14, 1, 34, 0},{14, 1, 34, 0},
    // B
    {14, 1, 34, 0},{14, 1, 34, 0},
    {14, 1, 34, 0},{14, 1, 34, 0},
    {14, 1, 34, 0},{14, 1, 34, 0},
    {14, 1, 34, 0},{14, 1, 34, 0}
  };
  rmt_write_items(RMT_CHANNEL_0, rmt_black, 24, true);
  rmt_wait_tx_done(RMT_CHANNEL_0, pdMS_TO_TICKS(10));
  // **************************************
  // TODO: do we want to power the SD card by default?
}
}
