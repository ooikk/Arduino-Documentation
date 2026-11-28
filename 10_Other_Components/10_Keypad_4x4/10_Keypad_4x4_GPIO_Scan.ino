
// Keypad dimensions
const byte ROWS = 4;
const byte COLS = 4;

const byte NO_KEY = '\0';  // use this char instead of NULL
unsigned long lastKeyPressTime = 0;
char lastKeyPress = NO_KEY;

const unsigned long DEBOUNCE_TIME = 20;  // ms
const unsigned long HOLD_TIME = 1000;    // ms


// Key layout
char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};

// ESP32-S3 GPIO assignments
byte rowPins[ROWS] = { 15, 16, 17, 18 };
byte colPins[COLS] = { 19, 20, 21, 47 };

enum KeyState { IDLE,
                PRESSED,
                HOLD,
                RELEASED };

KeyState keyState = IDLE;
/*
// Keypad event message
struct KeypadMessage {
  char key;
  KeyState state;
};

KeypadMessage keypadMessage;
*/

char scanKeyPad() {
  for (int r = 0; r < ROWS; r++) {
    digitalWrite(rowPins[r], LOW);
    for (int c = 0; c < COLS; c++) {
      if (digitalRead(colPins[c]) == LOW) {
        // Detect key press
        digitalWrite(rowPins[r], HIGH);
        //Serial.printf("Check: %c\n", keys[r][c]);
        if (lastKeyPress != keys[r][c]) {
          lastKeyPress = keys[r][c];
          lastKeyPressTime = millis();
          keyState = IDLE;
          //Serial.printf("Check: key=%c, lastKeyPress=%c\n", keys[r][c], lastKeyPress);
          return NO_KEY;
        }
        //Serial.printf("Check: key=%c, lastKeyPress=%c, keyState=%d\n", keys[r][c], lastKeyPress, keyState);
        if (lastKeyPress == keys[r][c]) {
          if (millis() - lastKeyPressTime > DEBOUNCE_TIME) {
            if (keyState == IDLE) {
              keyState = PRESSED;
              return lastKeyPress;
            }
          }
          if (millis() - lastKeyPressTime > HOLD_TIME) {
            if (keyState == PRESSED) {
              keyState = HOLD;
              return lastKeyPress;
              //return NO_KEY;
            }
          }
        }
        return NO_KEY;
      }
    }
    digitalWrite(rowPins[r], HIGH);
  }
  // No key press
  if ((keyState == PRESSED) || (keyState == HOLD)) {
    keyState = RELEASED;
    return lastKeyPress;
    //return NO_KEY;
  }
  keyState = IDLE;
  lastKeyPress = NO_KEY;
  return NO_KEY;  //NULL;
}


void setup() {
  Serial.begin(115200);

  for (int r = 0; r < ROWS; r++) {
    pinMode(rowPins[r], OUTPUT);
    digitalWrite(rowPins[r], HIGH);
  }
  for (int c = 0; c < COLS; c++)
    pinMode(colPins[c], INPUT_PULLUP);

  Serial.println("ESP32-S3 4x4 Keypad Ready");
}

void loop() {

  // Read keypad

  char key = scanKeyPad();

  // Detect a new keypress
  if (key != NO_KEY) {
    Serial.print("Key pressed: ");
    Serial.println(key);


    switch (keyState) {

      case PRESSED:
        Serial.print("[STATE] Key PRESSED: ");
        Serial.println(lastKeyPress);
        break;

      case HOLD:
        Serial.print("[STATE] Key HOLD: ");
        Serial.println(lastKeyPress);
        break;

      case RELEASED:
        Serial.print("[STATE] Key RELEASED: ");
        Serial.println(lastKeyPress);
        break;
      case IDLE:
        break;
    }
    /*
      // Wait for key release
      while (scanKeyPad() != NO_KEY) {
        delay(10);
      }
      delay(30);
*/
  }
}