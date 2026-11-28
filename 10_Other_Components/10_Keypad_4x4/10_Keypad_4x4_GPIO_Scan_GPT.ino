
// =====================================================
// ESP32-S3 4x4 Matrix Keypad - Direct GPIO Scanning
// Supports PRESSED, HOLD, RELEASED and IDLE states
// =====================================================

// Keypad dimensions
const byte ROWS = 4;
const byte COLS = 4;

const char NO_KEY = '\0';

const unsigned long DEBOUNCE_TIME = 20;
const unsigned long HOLD_TIME = 1000;

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

// Key states
enum KeyState {
  IDLE,
  PRESSED,
  HOLD,
  RELEASED
};

KeyState keyState = IDLE;
KeyState previousKeyState = IDLE;

// Key tracking
char lastKeyPress = NO_KEY;
char candidateKey = NO_KEY;

unsigned long lastKeyPressTime = 0;
unsigned long candidateChangeTime = 0;

// =====================================================
// Scan raw keypad
// Returns the currently detected key, or NO_KEY
// =====================================================
char scanRawKeyPad() {

  char detectedKey = NO_KEY;

  // Ensure all rows are inactive before scanning
  for (int r = 0; r < ROWS; r++) {
    digitalWrite(rowPins[r], HIGH);
  }

  // Scan one row at a time
  for (int r = 0; r < ROWS; r++) {

    digitalWrite(rowPins[r], LOW);
    delayMicroseconds(10);

    for (int c = 0; c < COLS; c++) {

      if (digitalRead(colPins[c]) == LOW) {
        detectedKey = keys[r][c];
        break;
      }
    }

    // Always restore the row to HIGH
    digitalWrite(rowPins[r], HIGH);

    if (detectedKey != NO_KEY) {
      break;
    }
  }

  return detectedKey;
}

// =====================================================
// Keypad scanning and state detection
// =====================================================
void scanKeyPad() {

  unsigned long currentTime = millis();

  // Read the raw keypad
  char currentKey = scanRawKeyPad();

  // Detect a change in the raw key
  if (currentKey != candidateKey) {
    candidateKey = currentKey;
    candidateChangeTime = currentTime;
  }

  // Wait until the input remains stable
  // for the debounce period
  if (currentTime - candidateChangeTime < DEBOUNCE_TIME) {
    return;
  }

  // A new stable key has been detected
  if (candidateKey != lastKeyPress) {

    lastKeyPress = candidateKey;

    if (lastKeyPress != NO_KEY) {

      // New key pressed
      lastKeyPressTime = currentTime;
      keyState = PRESSED;

    } else {

      // Key released
      keyState = RELEASED;
    }

    return;
  }

  // Check whether the key is being held
  if (lastKeyPress != NO_KEY && keyState == PRESSED) {

    if (currentTime - lastKeyPressTime >= HOLD_TIME) {
      keyState = HOLD;
    }
  }
}

// =====================================================
// Arduino setup
// =====================================================
void setup() {

  Serial.begin(115200);

  // Configure row pins as OUTPUT
  for (int r = 0; r < ROWS; r++) {
    pinMode(rowPins[r], OUTPUT);
    digitalWrite(rowPins[r], HIGH);
  }

  // Configure column pins as INPUT_PULLUP
  for (int c = 0; c < COLS; c++) {
    pinMode(colPins[c], INPUT_PULLUP);
  }

  Serial.println();
  Serial.println("ESP32-S3 4x4 Keypad Ready");
}

// =====================================================
// Arduino loop
// =====================================================
void loop() {

  scanKeyPad();

  // Only process a state when it changes
  if (keyState != previousKeyState) {

    switch (keyState) {

      case PRESSED:
        Serial.print("Key PRESSED: ");
        Serial.println(lastKeyPress);
        break;

      case HOLD:
        Serial.print("Key HOLD: ");
        Serial.println(lastKeyPress);
        break;

      case RELEASED:
        Serial.println("Key RELEASED");
        break;

      case IDLE:
        break;
    }

    previousKeyState = keyState;
  }

  // Reset RELEASED to IDLE after reporting it
  if (keyState == RELEASED) {
    keyState = IDLE;
  }

  delay(5);
}
