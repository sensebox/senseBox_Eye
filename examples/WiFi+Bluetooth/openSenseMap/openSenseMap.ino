#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <Adafruit_HDC1000.h>

const char* ssid = "";
const char* password = "";

const unsigned long intervalInterval = 60000;
unsigned long lastSend = 0;

static const uint8_t NUM_SENSORS = 2;

const char* SENSEBOX_ID = "";
const char* SENSOR_TEMP = "";
const char* SENSOR_HUMI = "";

const char* server = "ingress.opensensemap.org";

WiFiClientSecure client;
Adafruit_HDC1000 hdc;

// SHA1 fingerprint is broken. using root SDRG Root X1 valid until 04 Jun 2035 11:04:38 GMT
// ISRGRootX1.crt
const char* root_ca = \
                      "-----BEGIN CERTIFICATE-----\n" \
                      "MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw\n" \
                      "TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n" \
                      "cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4\n" \
                      "WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu\n" \
                      "ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY\n" \
                      "MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc\n" \
                      "h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+\n" \
                      "0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U\n" \
                      "A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW\n" \
                      "T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH\n" \
                      "B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC\n" \
                      "B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv\n" \
                      "KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn\n" \
                      "OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn\n" \
                      "jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw\n" \
                      "qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI\n" \
                      "rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV\n" \
                      "HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq\n" \
                      "hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL\n" \
                      "ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ\n" \
                      "3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK\n" \
                      "NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5\n" \
                      "ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur\n" \
                      "TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC\n" \
                      "jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc\n" \
                      "oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq\n" \
                      "4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA\n" \
                      "mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d\n" \
                      "emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=\n" \
                      "-----END CERTIFICATE-----\n" ;

// ================= WIFI =================
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected");
  Serial.println(WiFi.localIP());
}

// ================= MEASUREMENTS =================
typedef struct {
  const char *sensorId;
  float value;
} measurement;

measurement measurements[NUM_SENSORS];
uint8_t num_measurements = 0;

void addMeasurement(const char *sensorId, float value) {
  if (num_measurements < NUM_SENSORS) {
    measurements[num_measurements++] = {sensorId, value};
  }
}

void writeMeasurementsToClient() {
  for (uint8_t i = 0; i < num_measurements; i++) {
    client.printf("%s,%.2f\n", measurements[i].sensorId, measurements[i].value);
  }
}

// ================= SUBMIT =================
void submitValues() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost -> reconnect");
    WiFi.reconnect();
    delay(3000);
    return;
  }

  if (client.connected()) client.stop();

  if (!client.connect(server, 443)) {
    Serial.println("SSL connect failed");
    return;
  }

  // Build CSV body first
  String body = "";
  for (uint8_t i = 0; i < num_measurements; i++) {
    body += measurements[i].sensorId;
    body += ",";
    body += String(measurements[i].value, 2);
    if (i < num_measurements - 1) body += "\n";
  }

  // HTTP Header
  client.printf(
    "POST /boxes/%s/data HTTP/1.1\r\n"
    "Host: %s\r\n"
    "Content-Type: text/csv\r\n"
    "Connection: close\r\n"
    "Content-Length: %d\r\n\r\n",
    SENSEBOX_ID, server, body.length()
  );

  client.print(body);

  // Response
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break;
  }

  while (client.available()) {
    Serial.write(client.read());
  }

  client.stop();
  num_measurements = 0;
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  initWiFi();
  client.setCACert(root_ca);

  Wire.begin();
  hdc.begin(0x40);
}

// ================= LOOP =================
void loop() {
  if (millis() - lastSend > intervalInterval) {
    lastSend = millis();

    addMeasurement(SENSOR_TEMP, hdc.readTemperature());
    addMeasurement(SENSOR_HUMI, hdc.readHumidity());

    submitValues();
  }
}
