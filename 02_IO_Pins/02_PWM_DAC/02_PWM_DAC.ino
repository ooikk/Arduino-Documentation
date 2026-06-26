// Define the GPIO pin
const int LED_PIN_R = 15;
const int LED_PIN_DAC = 16;

// PWM Properties
const int PWM_FREQ = 5000;  // 5 kHz frequency
/*  Old API
const int PWM_CHANNEL = 0;   // Use PWM channel 0 (0-7 available)
*/
const int PWM_RES = 8;  // 8-bit resolution (0 - 255 values)

unsigned int dutyCycle;

void setup() {

  Serial.begin(115200);
  // New API: Automatically configures an underlying channel and attaches the pin
  ledcAttach(LED_PIN_R, PWM_FREQ, PWM_RES);
  ledcAttach(LED_PIN_DAC, PWM_FREQ, PWM_RES);
  dutyCycle = 255 / 2;
}

void loop() {

  //waitForSerial();
  // Pass the PIN directly to ledcWrite instead of the channel
  dutyCycle = waitForNumber();
  ledcWrite(LED_PIN_R, dutyCycle);
  ledcWrite(LED_PIN_DAC, dutyCycle);
  waitForSerial();
  ledcWrite(LED_PIN_R, 0);
  ledcWrite(LED_PIN_DAC, 0);
}

int waitForNumber() {
  int myNumber = 0;
  while (Serial.available())
    Serial.read();
  Serial.println("Input a number from 0 to 255");
  while (!Serial.available())
    ;

  if (Serial.available() > 0) {
    // Read everything until the user hits 'Enter' (\n)
    String input = Serial.readStringUntil('\n');

    // Convert the text string to an integer
    myNumber = input.toInt();

    // (Use input.toFloat() if you are expecting decimals)

    Serial.print("Converted Number: ");
    Serial.println(myNumber);
  }
  if (myNumber > 255) myNumber = 255;
  else if (myNumber < 0) myNumber = 0;

  Serial.print("Final Number: ");
  Serial.println(myNumber);
  return myNumber;
}

void waitForSerial() {

  // Clear the serial buffer so it doesn't instantly loop again
  while (Serial.available())
    Serial.read();
  Serial.println("Press <SEND> (or type a character) to advance...");
  // Wait here until a character is received over Serial
  while (!Serial.available())
    ;
}