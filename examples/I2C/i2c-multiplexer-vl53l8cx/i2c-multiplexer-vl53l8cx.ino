#include <Wire.h>
#include <vl53l8cx.h>

#define PIN_QWIIC_SDA 2
#define PIN_QWIIC_SCL 1

#define TCAADDR 0x77

VL53L8CX sensor_vl53l8cx(&Wire, -1, -1);

void tcaselect(uint8_t i) {
  if (i > 7) return;
 
  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << i);
  Wire.endTransmission();  
}

float oldVl53l8cxMin = -1.0;
float getVl53l8cxMin() {
  VL53L8CX_ResultsData Results;
  uint8_t NewDataReady = 0;
  uint8_t status;

  Wire.setClock(1000000); // vl53l8cx can operate at 1MHz
  status = sensor_vl53l8cx.check_data_ready(&NewDataReady);

  if ((!status) && (NewDataReady != 0)) {
    sensor_vl53l8cx.get_ranging_data(&Results);
    Wire.setClock(100000); // lower the I2C clock to 0.1MHz again for compatibility with other sensors
    float minStatus5 = 10000.0;
    float minStatus69 = 10000.0;
    for(int i = 0; i < VL53L8CX_RESOLUTION_8X8*VL53L8CX_NB_TARGET_PER_ZONE; i++) {
      float distance = ((&Results)->distance_mm[i])/10;
      float target_status = (&Results)->target_status[i];
      if(target_status == 5 && minStatus5 > distance) {
        minStatus5 = distance;
      } else if((target_status == 6 || target_status == 9) && minStatus69 > distance) {
        minStatus69 = distance;
      }
    }
    if (minStatus5 < 10000.0 && minStatus5 >=0) {
      oldVl53l8cxMin = minStatus5;
    } else if (minStatus69 < 10000.0 && minStatus69 >=0) {
      oldVl53l8cxMin = minStatus69;
    } else {
      oldVl53l8cxMin = 0.0;
    }
  }
  return oldVl53l8cxMin;
}

void setup() {
  Wire.begin(PIN_QWIIC_SDA,PIN_QWIIC_SCL);
  Serial.begin(9600);
  delay(1000);
  tcaselect(0);

  Wire.setClock(1000000); // Sensor has max I2C freq of 1MHz
  sensor_vl53l8cx.set_i2c_address(0x52);
  Serial.println("0) begin...");
  sensor_vl53l8cx.begin();
  Serial.println("0) init...");
  sensor_vl53l8cx.init();
  Serial.println("0) set range...");
  sensor_vl53l8cx.set_ranging_frequency_hz(30);
  Serial.println("0) set res...");
  sensor_vl53l8cx.set_resolution(VL53L8CX_RESOLUTION_8X8);
  Serial.println("0) start...");
  sensor_vl53l8cx.start_ranging();

  tcaselect(1);

  Wire.setClock(1000000); // Sensor has max I2C freq of 1MHz
  sensor_vl53l8cx.set_i2c_address(0x51);
  Serial.println("1) begin...");
  sensor_vl53l8cx.begin();
  Serial.println("1) init...");
  sensor_vl53l8cx.init();
  Serial.println("1) set range...");
  sensor_vl53l8cx.set_ranging_frequency_hz(30);
  Serial.println("1) set res...");
  sensor_vl53l8cx.set_resolution(VL53L8CX_RESOLUTION_8X8);
  Serial.println("1) start...");
  sensor_vl53l8cx.start_ranging();

}

void loop() {
  tcaselect(0);
  delay(100);
  sensor_vl53l8cx.set_i2c_address(0x52);
  Serial.print("0) ");
  Serial.println(getVl53l8cxMin());

  tcaselect(1);
  delay(100);
  sensor_vl53l8cx.set_i2c_address(0x51);
  Serial.print("1) ");
  Serial.println(getVl53l8cxMin());

}