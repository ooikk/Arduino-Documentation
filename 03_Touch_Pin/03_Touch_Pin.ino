// Choose a Touch-enabled pin from your diagram
// GPIO 1 is labeled as TOUCH1
const int touchPin = 4; 
int touchValue = 0;

void setup() {
  Serial.begin(115200);
  delay(1000); // Give the Serial Monitor time to connect
  Serial.println("ESP32-S3 Touch Test Initialized");
}

void loop() {
  // Read the touch value (0 to 100 on S3)
  touchValue = touchRead(touchPin);

  Serial.print("Touch Value: ");
  Serial.println(touchValue);

  delay(100);
}