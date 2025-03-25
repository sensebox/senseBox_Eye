#define BUTTON_PIN 47

void setup() {
  Serial.begin(115200);
  while (!Serial);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    Serial.println("Button pressed");
    delay(50);
  }
}