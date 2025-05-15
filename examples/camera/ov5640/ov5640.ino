#include "esp_camera.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "driver/rtc_io.h"
#include "camera_pins.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "driver/rtc_io.h"
#include <EEPROM.h>

#define SW_BUTTON 47

#define EEPROM_SIZE 1

// Pin definition for SD Card
#define VSPI_MISO   40
#define VSPI_MOSI   38
#define VSPI_SCLK   39
#define VSPI_SS     41
#define SD_ENABLE   48
SPIClass sdspi = SPIClass();

int pictureNumber = 0;
camera_fb_t * fb = NULL;

#define PIN_QWIIC_SDA 2
#define PIN_QWIIC_SCL 1

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

uint8_t* jpegBuf = nullptr;
size_t jpegLen = 0;

void deleteAllFromSD(fs::FS &fs, const char * dirname) {
  File root = fs.open(dirname);
  if (!root || !root.isDirectory()) {
    Serial.println("Failed to open directory for deletion");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    String path = String(dirname) + "/" + file.name();
    if (file.isDirectory()) {
      file.close();
      deleteAllFromSD(fs, path.c_str());
      fs.rmdir(path.c_str());
      Serial.printf("Removed folder: %s\n", path.c_str());
    } else {
      file.close();
      fs.remove(path.c_str());
      Serial.printf("Removed file: %s\n", path.c_str());
    }
    file = root.openNextFile();
  }
}

void setup() {
  pinMode(SW_BUTTON, INPUT_PULLUP);
  Wire.begin(PIN_QWIIC_SDA,PIN_QWIIC_SCL);
  Serial.begin(115200);
  delay(1500);

  // Init OLED
  display.begin(SSD1306_SWITCHCAPVCC, 0x3D);
  display.setRotation(2);
  display.display();
  delay(100);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Initializing...");
  display.display();

  // Camera config
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_GRAYSCALE;
  config.frame_size = FRAMESIZE_96X96; // Small grayscale image
  config.jpeg_quality = 12;
  config.fb_count = 1;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

  // Initialize camera
  Serial.println("Init camera...");
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x", err);
    display.println("Camera init failed!");
    display.display();
    return;
  }

  Init SD Card
  Serial.println("Starting SD Card");
  delay(3);
  pinMode(SD_ENABLE, OUTPUT);
  digitalWrite(SD_ENABLE, LOW);

  sdspi.begin(VSPI_SCLK, VSPI_MISO, VSPI_MOSI, VSPI_SS);
  if (!SD.begin(VSPI_SS, sdspi)) {
    Serial.println("Card Mount Failed");
    return;
  }

  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  EEPROM.begin(EEPROM_SIZE);
  pictureNumber = EEPROM.read(0) + 1;

  Serial.println("Wiping SD card...");
  deleteAllFromSD(SD,"/");
}

void loop() {
  // Capture image
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Capture failed");
    display.println("Capture failed!");
    display.display();
    return;
  }

  // Prepare to draw
  display.clearDisplay();

  const int srcWidth = 96;
  const int srcHeight = 96;
  const int targetHeight = 64;
  const int targetWidth = 64;
  const int xOffset = (SCREEN_WIDTH - targetWidth) / 2;  // 32

  // Simple nearest-neighbor downscaling from 96x96 -> 64x64
  for (int y = 0; y < targetHeight; y++) {
    for (int x = 0; x < targetWidth; x++) {
      // Map target (x, y) to source (sx, sy)
      int sx = x * srcWidth / targetWidth;
      int sy = y * srcHeight / targetHeight;
      uint8_t pixel = fb->buf[sy * srcWidth + sx];

      if (pixel > 128) {
        display.drawPixel(x + xOffset, y, WHITE);
      } else {
        display.drawPixel(x + xOffset, y, BLACK);
      }
    }
  }

  display.display();
  esp_camera_fb_return(fb);

  // Button press: switch to RGB565, capture, save, switch back
  if (digitalRead(SW_BUTTON) == LOW) {
    if (!switchCameraFormat(PIXFORMAT_RGB565, FRAMESIZE_WQXGA)) { // 2560x1600
      Serial.println("Failed to switch to RGB565");
      return;
    }

    // init camera
    fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);
    delay(50);

    // take actual picture
    fb = esp_camera_fb_get();

    if (fb && fb->format == PIXFORMAT_RGB565) {
      String filename = "/image" + String(pictureNumber) + ".jpg";
      // convert RGB565 to jpg
      if (frame2jpg(fb, 90, &jpegBuf, &jpegLen)) {
        File file = SD.open(filename, FILE_WRITE);
        if (file) {
          file.write(jpegBuf, jpegLen);
          file.close();
          Serial.println("Saved JPEG to SD: " + filename);
          pictureNumber++;
          EEPROM.write(0, pictureNumber);
          EEPROM.commit();
        }
        free(jpegBuf);
        jpegBuf = nullptr;
      }

      esp_camera_fb_return(fb);
    }

    // Reinitialize back to grayscale for OLED
    if (!switchCameraFormat(PIXFORMAT_GRAYSCALE, FRAMESIZE_96X96)) {
      Serial.println("Failed to switch back to GRAYSCALE");
      return;
    }

    delay(500); // prevent repeated triggers
  }
}

bool switchCameraFormat(pixformat_t format, framesize_t frame_size) {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = format;
  config.frame_size = frame_size;
  config.jpeg_quality = 10;
  config.fb_count = 1; // 2 would take up more space in the RAM
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_LATEST;

  // to suppress error messages when switching camera profile
  esp_log_level_set("gdma", ESP_LOG_NONE);
  
  esp_camera_deinit();
  esp_log_level_set("gdma", ESP_LOG_WARN);
  return esp_camera_init(&config) == ESP_OK;
}
