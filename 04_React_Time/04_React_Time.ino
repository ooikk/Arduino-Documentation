#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Initialize the LCD (address 0x27 or 0x3F, 16 columns, 2 rows)
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int buttonPin = 4;

// Game States
enum GameState { IDLE, WAITING, READY, RESULT };
GameState currentState = IDLE;

unsigned long waitStartTime = 0;
unsigned long reactionStartTime = 0;
unsigned long reactionTime = 0;
unsigned long randomDelay = 0;

// Debounce variables
bool lastButtonState = HIGH;
bool currentButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

void setup() {
  pinMode(buttonPin, INPUT_PULLUP); // Use internal pull-up resistor
  
  lcd.init();
  lcd.backlight();
  lcd.clear();
  
  showIdleScreen();
}

void loop() {
  // Read button with debouncing
  bool reading = digitalRead(buttonPin);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != currentButtonState) {
      currentButtonState = reading;
      
      // Trigger on button PRESS (transition from HIGH to LOW)
      if (currentButtonState == LOW) {
        handleButtonPress();
      }
    }
  }
  lastButtonState = reading;

  // Game Logic: Check if it's time to show "PRESS NOW!"
  if (currentState == WAITING) {
    if (millis() - waitStartTime > randomDelay) {
      currentState = READY;
      reactionStartTime = millis();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("PRESS NOW!");
    }
  }
}

void handleButtonPress() {
  switch (currentState) {
    case IDLE:
      // Start the game
      currentState = WAITING;
      randomDelay = random(2000, 5000); // Random wait between 2 and 5 seconds
      waitStartTime = millis();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Get Ready...");
      break;
      
    case WAITING:
      // Player pressed too early!
      currentState = RESULT;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Too Early!");
      lcd.setCursor(0, 1);
      lcd.print("Press to retry");
      break;
      
    case READY:
      // Successful reaction!
      currentState = RESULT;
      reactionTime = millis() - reactionStartTime;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Time: ");
      lcd.print(reactionTime);
      lcd.print(" ms");
      lcd.setCursor(0, 1);
      lcd.print("Press to retry");
      break;
      
    case RESULT:
      // Reset to idle
      currentState = IDLE;
      showIdleScreen();
      break;
  }
}

void showIdleScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Reaction Game");
  lcd.setCursor(0, 1);
  lcd.print("Press to Start");
}
