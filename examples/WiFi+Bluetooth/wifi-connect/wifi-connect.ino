#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP32Time.h>

#define WIFI_SSID "{{SSID_PLACEHOLDER}}"
#define WIFI_PASSWORD "{{PASSWORD_PLACEHOLDER}}"

// Initialize UDP and NTP Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // 0 = UTC, 60000 ms = update every 60s
ESP32Time rtc; // internal RTC

void connectToWiFi() {
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int retryCount = 0;
  while (WiFi.status() != WL_CONNECTED && retryCount < 30) {
    delay(500);
    Serial.print(".");
    retryCount++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected.");
  } else {
    Serial.println("\nFailed to connect to WiFi.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println();

  connectToWiFi();

  // Setup NTP
  Serial.println("Setting up NTP...");
  timeClient.begin();

  // Try to get initial time
  if (WiFi.status() == WL_CONNECTED) {
    if (timeClient.forceUpdate()) {
      rtc.setTime(timeClient.getEpochTime());
      Serial.println("Initial time synchronized.");
    } else {
      Serial.println("Failed to get initial NTP time.");
    }
  }
}

void loop() {
  // Reconnect to WiFi if needed
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Attempting to reconnect...");
    connectToWiFi();
  }

  // Try to update time from NTP server
  if (WiFi.status() == WL_CONNECTED) {
    if (timeClient.forceUpdate()) {
      rtc.setTime(timeClient.getEpochTime());
      Serial.println("Time updated from NTP.");
    } else {
      Serial.println("NTP update failed.");
      WiFi.disconnect();
      connectToWiFi();
    }
  } else {
    Serial.println("Cannot update time, no WiFi.");
    delay(100000);
  }

  // Print human-readable time from RTC
  Serial.println("Current RTC time: " + rtc.getTime("%A, %B %d %Y %H:%M:%S"));
  delay(1000); // every second
}