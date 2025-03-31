#define LORA_TX 43
#define LORA_RX 44

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial1.begin(19200, SERIAL_8N1, LORA_RX, LORA_TX);
  while (!Serial1);

  Serial.println("Sending AT+XXX");
  Serial1.print("AT+XXX\r\n");
}

int i = 0;
void loop() {
  while (Serial1.available()) {
    Serial.write(Serial1.read());
  }

  delay(100);
  i++;
  if(i>10) {
    i=0;
    Serial.println("Sending AT+XXX");
    Serial1.print("AT+XXX\r");
  }
}