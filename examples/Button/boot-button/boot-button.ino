#define BOOT_BUTTON 0

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("ready");
  pinMode(BOOT_BUTTON, INPUT_PULLUP);  // GPIO0 with internal pull-up resistor
}

void loop() {
  if (digitalRead(BOOT_BUTTON) == LOW) {
    Serial.println("Button pressed");
    delay(50);
  }
}