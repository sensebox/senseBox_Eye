#include <vl53l8cx.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h> // http://librarymanager/All#Adafruit_GFX_Library
#include <Adafruit_SSD1306.h> // http://librarymanager/All#Adafruit_SSD1306
#include <Adafruit_MPU6050.h>
#include <Adafruit_HDC1000.h>
#include <sps30.h>

#define PIN_QWIIC_SDA 2
#define PIN_QWIIC_SCL 1

VL53L8CX sensor_vl53l8cx(&Wire, -1, -1);

Adafruit_MPU6050 mpu;
sensors_event_t a, g, temp;

Adafruit_HDC1000 hdc = Adafruit_HDC1000();

struct sps30_measurement m;
uint32_t auto_clean_days = 4;
const long intervalsps = 1000;
unsigned long time_startsps = 0;
unsigned long time_actualsps = 0;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SENSOR_WIDTH 8
#define SENSOR_HEIGHT 8
#define DISPLAY_WIDTH 64  // Expanded display width
#define DISPLAY_HEIGHT 64
#define SCALE_FACTOR 8    // Scale 8x8 to 64x64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

uint16_t sensorBitmap[SENSOR_WIDTH * SENSOR_HEIGHT];  // Original sensor data
uint8_t scaledBitmap[DISPLAY_WIDTH * DISPLAY_HEIGHT / 8];  // Scaled bitmap for SSD1306

// Grayscale patterns for 4 levels
uint8_t grayscalePatterns[6][8] = {
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},  // White
    {0xEE, 0xDD, 0xEE, 0xDD, 0xEE, 0xDD, 0xEE, 0xDD},  // Very Light Gray
    {0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55},  // Medium Gray
    {0xAA, 0x00, 0xAA, 0x00, 0xAA, 0x00, 0xAA, 0x00},  // Dark Gray
    {0x88, 0x00, 0x88, 0x00, 0x88, 0x00, 0x88, 0x00},  // Very Dark Gray
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}   // Black
};

uint16_t* getVl53l8cxBitmap() {
  VL53L8CX_ResultsData Result;
  uint8_t NewDataReady = 0;
  uint8_t status;

  Wire.setClock(1000000); // Sensor has max I2C freq of 1MHz
  status = sensor_vl53l8cx.check_data_ready(&NewDataReady);

  if ((!status) && (NewDataReady != 0)) {
    sensor_vl53l8cx.get_ranging_data(&Result);

    for (int j = 0; j < SENSOR_HEIGHT; j++) {
      for (int k = 0; k < SENSOR_WIDTH; k++) {
        int index = j * SENSOR_WIDTH + k;
        long distance = (long)(&Result)->distance_mm[VL53L8CX_NB_TARGET_PER_ZONE * index];
        if (distance > 2000) distance = 2000;  // Cap distance at 2 meters
        sensorBitmap[index] = map(distance, 0, 2000, 0, 5);  // Map distance to levels 0-3
      }
    }
  }
  Wire.setClock(100000); // reduce clock speed again
  return sensorBitmap;
}

void scaleBitmapWithShades(uint16_t* inputBitmap, uint8_t* outputBitmap) {
  memset(outputBitmap, 0, DISPLAY_WIDTH * DISPLAY_HEIGHT / 8);  // Clear the output bitmap

  for (int y = 0; y < SENSOR_HEIGHT; y++) {
    for (int x = 0; x < SENSOR_WIDTH; x++) {
      int inputIndex = y * SENSOR_WIDTH + x;
      uint8_t* pattern = grayscalePatterns[inputBitmap[inputIndex]];  // Get pattern for shade

      // Scale each sensor pixel to an 8x8 block in the 64x64 bitmap
      for (int dy = 0; dy < SCALE_FACTOR; dy++) {
        for (int dx = 0; dx < SCALE_FACTOR; dx++) {
          int scaledX = x * SCALE_FACTOR + dx;
          int scaledY = y * SCALE_FACTOR + dy;
          int outputIndex = (scaledY * DISPLAY_WIDTH + scaledX) / 8;
          int bitPosition = 7 - (scaledX % 8);  // Bit position in the byte (MSB to LSB)

          // Get the corresponding bit from the pattern
          uint8_t patternRow = pattern[dy % 8];
          if (patternRow & (1 << (7 - (dx % 8)))) {
            outputBitmap[outputIndex] |= (1 << bitPosition);  // Set the bit
          }
        }
      }
    }
  }
}

void getSPS30Data(){
  uint16_t data_ready;
  int16_t ret;
  ret = sps30_read_data_ready(&data_ready);
  if (ret>=0 && data_ready) {
    ret = sps30_read_measurement(&m);
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(PIN_QWIIC_SDA,PIN_QWIIC_SCL);    // Initialize I2C
  Wire.setClock(1000000); //Sensor has max I2C freq of 1MHz
  sensor_vl53l8cx.begin();
  sensor_vl53l8cx.init();
  sensor_vl53l8cx.set_ranging_frequency_hz(30);
  sensor_vl53l8cx.set_resolution(VL53L8CX_RESOLUTION_8X8);
  sensor_vl53l8cx.start_ranging();
  Wire.setClock(100000); // reduce clock speed again
  Serial.println("VL53L8CX Initialized!");

  mpu.begin(0x68, &Wire);
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.println("MPU Initialized!");

  hdc.begin();
  Serial.println("HDC Initialized!");

  sensirion_i2c_init();
  sps30_set_fan_auto_cleaning_interval_days(auto_clean_days);
  sps30_start_measurement();
  Serial.println("SPS Initialized!");

  display.begin(SSD1306_SWITCHCAPVCC, 0x3D);
  display.display();
  Serial.println("Display Initialized!");
  delay(100);
  display.clearDisplay();

}

long lastLoopTime = 0;
void loop() {
  Serial.println(" || Total Loop duration: " + String(millis() - lastLoopTime) + "ms");
  lastLoopTime = millis();
  display.clearDisplay();
  // --- Get sensor data ---
  hdc.readTemperature();
  Serial.print("HDC temp after: " + String(millis() - lastLoopTime) + "ms");
  hdc.readHumidity();
  Serial.print(" || HDC humid after: " + String(millis() - lastLoopTime) + "ms");
  mpu.getEvent(&a, &g, &temp);
  Serial.print(" || MPU after: " + String(millis() - lastLoopTime) + "ms");
  getSPS30Data();  // fills global 'm'
  Serial.print(" || SPS after: " + String(millis() - lastLoopTime) + "ms");
  uint16_t* vl53Bitmap = getVl53l8cxBitmap();
  Serial.print(" || vl53l8cx after: " + String(millis() - lastLoopTime) + "ms");

  // --- Draw Bitmap ---
  scaleBitmapWithShades(vl53Bitmap, scaledBitmap);
  display.drawBitmap(0, 0, scaledBitmap, DISPLAY_WIDTH, DISPLAY_HEIGHT, SSD1306_WHITE);
  Serial.print(" || draw bitmap after: " + String(millis() - lastLoopTime) + "ms");

  // --- Draw sensor data on the right side ---
  int xOffset = 66; // Start after bitmap
  int yOffset = 0;
  int lineHeight = 9;

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(xOffset, yOffset); display.print("T:"); display.print(hdc.readTemperature(), 1); display.println("C");
  yOffset += lineHeight;

  display.setCursor(xOffset, yOffset); display.print("H:"); display.print(hdc.readHumidity(), 1); display.println("%");
  yOffset += lineHeight;

  display.setCursor(xOffset, yOffset); display.print("X:"); display.print(a.acceleration.x, 1);
  yOffset += lineHeight;

  display.setCursor(xOffset, yOffset); display.print("Y:"); display.print(a.acceleration.y, 1);
  yOffset += lineHeight;

  display.setCursor(xOffset, yOffset); display.print("Z:"); display.print(a.acceleration.z, 1);
  yOffset += lineHeight;

  display.setCursor(xOffset, yOffset); display.print("PM1:"); display.print(m.mc_1p0, 1);
  yOffset += lineHeight;

  display.setCursor(xOffset, yOffset); display.print("PM2.5:"); display.print(m.mc_2p5, 1);
  yOffset += lineHeight;
  Serial.print(" || draw measurements after: " + String(millis() - lastLoopTime) + "ms");

  display.display();
}