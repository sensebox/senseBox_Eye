#include "esp_camera.h"
#include "Arduino.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "driver/rtc_io.h"
#include <EEPROM.h>
#include "camera_pins.h"

// define the number of bytes you want to access
#define EEPROM_SIZE 1

// Pin definition for SD Card
#define VSPI_MISO   40 // DAT
#define VSPI_MOSI   38 // CMD
#define VSPI_SCLK   39 // SCK/CLK
#define VSPI_SS     41 // CS
#define SD_ENABLE   48
SPIClass sdspi = SPIClass();

int pictureNumber = 0;

void setup() {
 
  Serial.begin(115200);
  while(!Serial);
  
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
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_UXGA;
  config.jpeg_quality = 10;
  config.fb_count = 2;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  
  // Init Camera
  Serial.println("init camera");
  delay(3);
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x", err);
    return;
  }

  // Init SD Card
  Serial.println("Starting SD Card");
  delay(3);
  pinMode(SD_ENABLE,OUTPUT);
  digitalWrite(SD_ENABLE,LOW);
  
  sdspi.begin(VSPI_SCLK,VSPI_MISO,VSPI_MOSI,VSPI_SS);
  if(!SD.begin(VSPI_SS,sdspi)){
      Serial.println("Card Mount Failed");
      return;
  }
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
      Serial.println("No SD card attached");
      return;
  }
    
  camera_fb_t * fb = NULL;
  
  // capture to "warm up" camera
  fb = esp_camera_fb_get();  
  esp_camera_fb_return(fb); 
  // capture the image to be saved on the SD Card
  Serial.println("capture image");
  fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Camera capture failed");
    return;
  }
  // initialize EEPROM with predefined size
  EEPROM.begin(EEPROM_SIZE);
  pictureNumber = EEPROM.read(0) + 1;

  // Path where new picture will be saved in SD Card
  String path = "/picture" + String(pictureNumber) +".jpg";

  Serial.printf("Picture file name: %s\n", path.c_str());
  
  File file = SD.open(path.c_str(), FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file in writing mode");
  } 
  else {
    file.write(fb->buf, fb->len); // payload (image), payload length
    Serial.printf("Saved file to path: %s\n", path.c_str());
    EEPROM.write(0, pictureNumber);
    EEPROM.commit();
  }
  file.close();
  esp_camera_fb_return(fb); 
}

void loop() {
  
}
