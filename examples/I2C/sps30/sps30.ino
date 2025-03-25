#include <Wire.h>
#include <sps30.h>

#define PIN_QWIIC_SDA 2
#define PIN_QWIIC_SCL 1

struct sps30_measurement m;
uint32_t auto_clean_days = 4;
const long intervalsps = 1000;
unsigned long time_startsps = 0;
unsigned long time_actualsps = 0;

void getSPS30Data(){

  uint16_t data_ready;
  int16_t ret;

  do {
    ret = sps30_read_data_ready(&data_ready);
    if (ret < 0) {
    } else if (!data_ready)  {}
    else
    break;
    delay(100); /* retry in 100ms */
  } while (1);
  ret = sps30_read_measurement(&m);
}

void setup() {
  Wire.begin(PIN_QWIIC_SDA,PIN_QWIIC_SCL);
  Serial.begin(300);
  sensirion_i2c_init();
  sps30_set_fan_auto_cleaning_interval_days(auto_clean_days);
  sps30_start_measurement();

}

void loop() {

  time_startsps = millis();
  if (time_startsps > time_actualsps + intervalsps) {
    time_actualsps = millis();
    getSPS30Data();
  }
  Serial.print("PM1.0: ");
  Serial.print(m.mc_1p0);
  Serial.print(" PM2.5: ");
  Serial.print(m.mc_2p5);
  Serial.print(" PM4.0: ");
  Serial.print(m.mc_4p0);
  Serial.print(" PM10: ");
  Serial.println(m.mc_10p0);
  delay(100);

}