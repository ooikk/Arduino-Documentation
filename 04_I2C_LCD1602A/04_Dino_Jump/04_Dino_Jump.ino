// By QWEN 3.7 Plus
/**
The Player (A or ^): Stays at column 3. Pressing the button makes them "jump" to the top row for 700ms.
The Obstacle (M): Spawns on the right and moves left.
The Ground (-): Fills the bottom row. If you are on the bottom row when the M hits your column, it's Game Over!
Difficulty: The obstacle moves faster every time you successfully dodge it.
**/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Initialize LCD (Change 0x27 to 0x3F if your display doesn't work)
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int buttonPin = 4;

// Game States
enum GameState { MENU, PLAYING, GAMEOVER };
GameState currentState = MENU;

// Player variables
const int playerCol = 3;
int playerRow = 1;          // 1 = ground, 0 = air
bool isJumping = false;
unsigned long jumpStartTime = 0;
const unsigned long jumpDuration = 700; // Jump lasts 700ms

// Obstacle variables
int obstacleCol = 15;
unsigned long lastObstacleMove = 0;
unsigned long gameSpeed = 400; // Starting speed in ms (lower = faster)

// Score variables
int score = 0;
int highScore = 0;

// Button debounce variables
bool lastButtonState = HIGH;
bool currentButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// Screen buffers to prevent flickering
char row0[17];
char row1[17];

void setup() {
  pinMode(buttonPin, INPUT_PULLUP); // Internal pull-up resistor
  
  lcd.init();
  lcd.backlight();
  lcd.clear();
  
  drawMenu();
}

void loop() {
  // 1. Read button with debouncing
  bool reading = digitalRead(buttonPin);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != currentButtonState) {
      currentButtonState = reading;
      
      // Trigger ONLY on the initial press (HIGH to LOW transition)
      if (currentButtonState == LOW) {
        handleButtonPress();
      }
    }
  }
  lastButtonState = reading;

  // 2. Game Logic
  if (currentState == PLAYING) {
    updateGame();
    drawGame();
  } 
  else if (currentState == GAMEOVER) {
    drawGameOver();
  }
}

void handleButtonPress() {
  if (currentState == MENU || currentState == GAMEOVER) {
    // Start or restart the game
    resetGame();
    currentState = PLAYING;
  } 
  else if (currentState == PLAYING) {
    // Jump if not already in the air
    if (!isJumping) {
      isJumping = true;
      playerRow = 0;
      jumpStartTime = millis();
    }
  }
}

void resetGame() {
  playerRow = 1;
  isJumping = false;
  obstacleCol = 15;
  score = 0;
  gameSpeed = 400;
  lastObstacleMove = millis();
}

void updateGame() {
  // Check if jump duration has ended
  if (isJumping && (millis() - jumpStartTime > jumpDuration)) {
    isJumping = false;
    playerRow = 1; // Fall back to ground
  }

  // Move obstacle based on gameSpeed
  if (millis() - lastObstacleMove > gameSpeed) {
    lastObstacleMove = millis();
    obstacleCol--;

    // Obstacle passed the screen
    if (obstacleCol < 0) {
      obstacleCol = 15;
      score++;
      // Increase difficulty: speed up, but cap at 150ms
      if (gameSpeed > 150) {
        gameSpeed -= 15; 
      }
    }

    // Collision Detection
    // Hitbox: if obstacle is at player's column (or 1 step ahead) AND player is on the ground
    if ((obstacleCol == playerCol || obstacleCol == playerCol + 1) && playerRow == 1) {
      currentState = GAMEOVER;
      if (score > highScore) {
        highScore = score;
      }
    }
  }
}

void drawGame() {
  // Clear buffers
  for (int i = 0; i < 16; i++) {
    row0[i] = ' ';   // Sky is empty
    row1[i] = '-';   // Ground is dashes
  }
  row0[16] = '\0';
  row1[16] = '\0';

  // Draw Player
  if (playerRow == 0) {
    row0[playerCol] = '^'; // Jumping
  } else {
    row1[playerCol] = 'A'; // Running on ground
  }

  // Draw Obstacle (Cactus)
  if (obstacleCol >= 0 && obstacleCol < 16) {
    row1[obstacleCol] = 'M'; 
  }

  // Draw Score on top right
  String scoreStr = "Sc:" + String(score);
  int startCol = 16 - scoreStr.length();
  for (int i = 0; i < scoreStr.length(); i++) {
    row0[startCol + i] = scoreStr.charAt(i);
  }

  // Print buffers to LCD (prevents flicker)
  lcd.setCursor(0, 0);
  lcd.print(row0);
  lcd.setCursor(0, 1);
  lcd.print(row1);
}

void drawMenu() {
  lcd.setCursor(0, 0);
  lcd.print("  DINO JUMP   ");
  lcd.setCursor(0, 1);
  lcd.print("Press to Start");
}

void drawGameOver() {
  lcd.setCursor(0, 0);
  lcd.print("  GAME OVER!  ");
  
  String msg = "Sc:" + String(score) + " Hi:" + String(highScore);
  // Pad with spaces to ensure old characters are overwritten
  while (msg.length() < 16) {
    msg += " ";
  }
  
  lcd.setCursor(0, 1);
  lcd.print(msg);
}
