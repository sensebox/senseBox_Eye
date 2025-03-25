#include <Wire.h>
#include <Adafruit_MPU6050.h>

#define PIN_QWIIC_SDA 2
#define PIN_QWIIC_SCL 1

Adafruit_MPU6050 mpu;
sensors_event_t a, g, temp;

void setup() {
  Wire.begin(PIN_QWIIC_SDA,PIN_QWIIC_SCL);
  Serial.begin(115200);
  mpu.begin(0x68, &Wire);
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
}

void loop() {
  mpu.getEvent(&a, &g, &temp);
  Serial.println("Acc:");
  Serial.print("X");
  Serial.print(a.acceleration.x);
  Serial.print(" Y");
  Serial.print(a.acceleration.y);
  Serial.print(" Z");
  Serial.println(a.acceleration.z);
  Serial.println("Gyro:");
  Serial.print("X");
  Serial.print(g.gyro.x);
  Serial.print(" Y");
  Serial.print(g.gyro.y);
  Serial.print(" Z");
  Serial.println(g.gyro.z);
  Serial.print("temp: ");
  Serial.println(temp.temperature);
  delay(100);

}