#define LORA_TX 43
#define LORA_RX 44

void sendAT(String cmd, unsigned long timeout = 2000) {
  Serial.print("Sending: ");
  Serial.println(cmd);
  Serial1.print(cmd + "\r\n");

  unsigned long start = millis();
  while (millis() - start < timeout) {
    while (Serial1.available()) {
      Serial.write(Serial1.read());
    }
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial1.begin(19200, SERIAL_8N1, LORA_RX, LORA_TX);
  while (!Serial1);

  // 1. Check modem is alive
  sendAT("AT");

  // 2. Get DevEUI (for registration on TTN)
  sendAT("AT+DEVEUI?"); // 70741400000F467C

  // 3. Set JoinEUI (AppEUI)
  sendAT("AT+APPEUI=XXX");

  // 4. Set AppKey
  sendAT("AT+APPKEY=XXX");

  // 5. Start Join process
  sendAT("AT+JOIN", 10000);  // Wait up to 10s

  // 6. Wait until join is accepted
  Serial.println("Waiting for JOIN ACCEPT...");
  unsigned long joinStart = millis();
  while (millis() - joinStart < 15000) {
    while (Serial1.available()) {
      String resp = Serial1.readStringUntil('\n');
      Serial.println(resp);
      if (resp.indexOf("Join succeeded") >= 0) {
        Serial.println("JOIN SUCCESSFUL!");
        goto send_msg;
      }
    }
  }

  Serial.println("Join failed or timeout.");
  return;

send_msg:
  delay(2000);  // Short wait before sending message

  // 7. Send hex message ("Hello")
  sendAT("AT+UTX=10\r1234567890", 5000);  // send some numbers
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
    Serial.println("Sending AT+UTX=10\r1234567890");
    sendAT("AT+UTX=10\r1234567890", 5000);  // send some numbers
  }
}
