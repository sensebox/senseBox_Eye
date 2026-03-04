#define LORA_TX 43
#define LORA_RX 44

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial1.begin(19200, SERIAL_8N1, LORA_RX, LORA_TX);
  while (!Serial1);
  delay(500);

  Serial.println("Sending AT+REBOOT");
  Serial1.print("AT+REBOOT\r\n");
}

int i = 5;
void loop() {
  while (Serial1.available()) {
    Serial.print(".");
    Serial.write(Serial1.read());
  }

  delay(500);
  i++;
  if(i>10) {
    i=0;
    Serial.println("Sending AT+VER?\r");
    Serial1.print("AT+VER?\r");
  }
}