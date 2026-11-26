#include <Keypad.h>

#define KEYPAD_EVENT

// Keypad dimensions
const byte ROWS = 4;
const byte COLS = 4;

// Key layout
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// ESP32-S3 GPIO assignments
byte rowPins[ROWS] = {15, 16, 17, 18};
byte colPins[COLS] = {19, 20, 21, 47};

// Create keypad object
Keypad keypad = Keypad(
  makeKeymap(keys),
  rowPins,
  colPins,
  ROWS,
  COLS
);

#ifdef KEYPAD_EVENT
void keypadEvent(KeypadEvent key) {

    switch (keypad.getState()) {

        case PRESSED:
            Serial.println("[Event]: Key Pressed");
            break;

        case HOLD:
            Serial.println("[Event]: Key Holding");
            break;

        case RELEASED:
            Serial.println("[Event]: Key Released");
            break;
    }
}
#endif

void setup() {
  Serial.begin(115200);

  keypad.setDebounceTime(20);
  keypad.setHoldTime(1000);

#ifdef KEYPAD_EVENT
    keypad.addEventListener(keypadEvent);
#endif

  Serial.println("ESP32-S3 4x4 Keypad Ready");
}

void loop() {

  // Read keypad
  char key = keypad.getKey();

  // Detect a new keypress
  if (key != NO_KEY) {
    Serial.print("Key pressed: ");
    Serial.println(key);
  }
}