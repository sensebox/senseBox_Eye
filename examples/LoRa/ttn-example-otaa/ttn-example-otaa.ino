#define LORA_TX 43
#define LORA_RX 44

bool join_success = false;

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

  // Check modem is alive
  sendAT("AT");

  // Get DevEUI (for registration on TTN)
  sendAT("AT+DEVEUI?");

  // Set JoinEUI (AppEUI)
  sendAT("AT+APPEUI=XXX");

  // Set AppKey
  sendAT("AT+APPKEY=XXX");

  // Start Join process
  sendAT("AT+JOIN", 10000);  // Wait up to 10s

  // Wait until join is accepted
  Serial.println("Waiting for JOIN ACCEPT...");
  unsigned long joinStart = millis();
  while (millis() - joinStart < 15000) {
    while (Serial1.available()) {
      String resp = Serial1.readStringUntil('\n');
      Serial.println(resp);
      if (resp.indexOf("Join succeeded") >= 0) {
        Serial.println("JOIN SUCCESSFUL!");
        join_success = true;
      }
    }
  }

  if (!join_success) {
    Serial.println("Join failed or timeout.");
    return;
  }
  else {
    delay(2000);  // Short wait before sending message

    // Send unconfirmed message
    sendAT("AT+UTX=10\r1234567890", 5000);  // send some numbers

    // Send confirmed message
    sendAT("AT+CTX=10\r0987654321", 15000);  // send some numbers
    unsigned long msgStart = millis();
    bool message_received = false;
    while (millis() - msgStart < 15000) {
      while (Serial1.available()) {
        String resp = Serial1.readStringUntil('\n');
        Serial.println(resp);
        if (resp.indexOf("ACK") >= 0) {
          Serial.println("message was received!");
          message_received = true;
        }
      }
    }
    if (!message_received) {
      Serial.println("message send failed or timeout.");
      return;
    }
  }
}

int i = 0;
void loop() {
  if (join_success) {
    while (Serial1.available()) {
      Serial.write(Serial1.read());
    }

    delay(100);
    i++;
    if(i>10) {
      i=0;
      
      // Send unconfirmed message
      sendAT("AT+UTX=10\r1234567890", 5000);  // send some numbers

      // Send confirmed message
      sendAT("AT+CTX=10\r0987654321", 15000);  // send some numbers
      unsigned long msgStart = millis();
      bool message_received = false;
      while (millis() - msgStart < 15000) {
        while (Serial1.available()) {
          String resp = Serial1.readStringUntil('\n');
          Serial.println(resp);
          if (resp.indexOf("ACK") >= 0) {
            Serial.println("message was received!");
            message_received = true;
          }
        }
      }
      if (!message_received) {
        Serial.println("message send failed or timeout.");
        return;
      }
    }
  }
}
