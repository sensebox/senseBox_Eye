void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Setup done.");
}

void loop() {
  // Normal operation (awake for 2 seconds)
  Serial.println("Awake!");
  delay(1998);

  // Prepare for light sleep
  Serial.println("sleeping...");
  delay(2);
  Serial.flush(); 

  // Light sleep for 2 seconds
  esp_sleep_enable_timer_wakeup(2000000); // Time in microseconds (2 seconds)
  esp_light_sleep_start();
}
