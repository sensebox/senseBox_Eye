#define SW_BUTTON 47

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("ready");
  pinMode(SW_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(SW_BUTTON) == LOW) {
    Serial.println("Button pressed");
    delay(50);
  }
}