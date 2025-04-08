#include <WiFi.h>
#include <WebServer.h>

// Set your hotspot (AP) credentials
const char* ssid = "senseBox Eye";
const char* password = "12345678";

// Create a web server on port 80
WebServer server(80);

void handleRoot() {
  server.send(200, "text/html", "<h1>Hello from senseBox-eye!</h1><p>You are connected :)</p>");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Start the Access Point
  WiFi.softAP(ssid, password);

  // Print IP address of the AP
  IPAddress IP = WiFi.softAPIP();
  Serial.println("Access Point started");
  Serial.print("IP address: ");
  Serial.println(IP);

  // Define routes
  server.on("/", handleRoot);

  // Start the server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();  // Handle incoming client requests
}
