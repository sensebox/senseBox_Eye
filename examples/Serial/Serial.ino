void setup() {
  Serial.begin(115200);
  while(!Serial);
}

long l = 0;
const int string_length = 256;
char str[string_length + 1]; // +1 for null terminator

void loop() {
  Serial.print("Loop ");
  Serial.print(l);
  Serial.print(": ");

  for (int i = 0; i < string_length; i++) {
    str[i] = random(32, 127); // ASCII printable characters
  }
  str[string_length] = '\0'; // Null-terminate the string

  Serial.println(str);
  l++;
}
