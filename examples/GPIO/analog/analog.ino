#define GPIO_PIN 3

void setup() {
  Serial.begin(115200);
  pinMode(gpio_pin,INPUT);
  analogReadResolution(13);
  analogSetAttenuation(ADC_11db);
  while (!Serial) ;
}

void loop() {
  int analogValue = analogReadMilliVolts(gpio_pin);
  Serial.println(analogValue);
  delay(100);
}