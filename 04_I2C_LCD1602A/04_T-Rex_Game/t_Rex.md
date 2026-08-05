# T_Rex Game

The T-Rex Game (also known as the Chrome Dino Game) is a simple, built-in side-scrolling runner game created by Google.

It famously appears in the Google Chrome web browser whenever you try to visit a webpage but lose your internet connection. The character design is a nod to the "prehistoric ages" of being offline.

## How It Works

The gameplay is incredibly straightforward but gets addictive as the speed increases:

The Goal: Guide a pixelated Tyrannosaurus rex across a desert landscape for as long as possible to get a high score.

Obstacles: You must dodge oncoming obstacles like cacti and flying pterodactyls.

## Controls

Spacebar or Up Arrow: Jump over obstacles.

Down Arrow: Duck under flying obstacles (on desktop).

Tap Screen: Jump (on mobile devices).

As you advance, the game transitions between day mode (light background) and night mode (dark background) every 700 points.

## How to Play It (Even When Online)

You don't actually have to pull your internet plug to play it. You can launch it anytime by opening a new tab in Google Chrome and typing this into the address bar:

chrome://dino

Fun Fact: The game is theoretically endless. The developers capped the maximum playtime at approximately 17 million years—a playful reference to how long the T-Rex was alive on Earth before extinction.

<img alt="image" style="width: 75%; height: auto;" src="https://github.com/user-attachments/assets/f703984a-98c2-4fa8-af7e-9a35bc16a68d" />

## The Hardware Setup

Follow the link for LCD1602A setup:

```
I2C LCD          ESP32 S3
GND              GND
VCC              VIN (2.5V to 6V)  // It's recommended connect to >5V to ensure enough contrast.
SDA              GPIO 8
SCL              GPIO 9
```

https://github.com/ooikk/Arduino-Documentation/blob/main/04_I2C_LCD1602A/I2C_LCD1602A.md

Below final circuit with push button switch.

```cpp
// Toggle switch connects to GPIO 4 of ESP32
const int togglePin = 4;     // switch to trigger jump action
```

<img alt="image" style="width: 75%; height: auto;" src="https://github.com/user-attachments/assets/4f50c7ff-d5d9-4342-916f-aa5fc1925757" />

**NOTE:** Omit external pull-up 3.3k ohm resistor if use internal pull up configuration.

```cpp
pinMode(togglePin, INPUT_PULLUP);
```

## The Game

### Toggle Switch

To prevent abnormal action when player presses the switch continuosly, the action only start with "Press" and "Released" sequence.

This code will wait until player Press and Release. Use in none playing mode.

```cpp
// Wait user to continue
switchState = digitalRead(togglePin);
while (switchState == HIGH) {
  switchState = digitalRead(togglePin);
}
while (switchState == LOW) {
  switchState = digitalRead(togglePin);
}
```

Under play mode, the code is broken into two parts in order not to block the CPU processing for other on going tasks. First part is to detect "press" down and let the code flows to next step. With a **switchStateCur** flag to indicate the switch state, on next loop second part of the code will be able to detect "release" button. Once confirm Press and Release, start the jump action.

```cpp
switchState = digitalRead(togglePin);
if (switchState == LOW) {
  switchStateCur = LOW;
}
if ((switchStateCur == LOW) && (switchState == HIGH)) {
  switchStateCur = HIGH;  // confirm user press and release switch
  actionJump = HIGH;      // trigger jump action
}
```

### Reset Game

```cpp
void resetGame() {
  // Reset variables
  t_RexStatus = LOW;
  Score = 0;
  numLife = NUMLIFE;
  timePrevious = 0;
  jumptimePrevious = 0;
  // Draw the T-Rex exactly ONCE for the start of the game
  lcd.clear();
}
```

### Moving Track

For the playing scene, defined custom characters. Use arrayTrack[16] for characters to be displayed on LCD screen. For the running track, only baseChar and wallChar to be displayed.

```cpp
byte clearChar[] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};

byte baseChar[] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B11111
};

byte wallChar[] = {
  B00100,
  B01110,
  B01110,
  B01110,
  B01110,
  B01110,
  B01110,
  B11111
};

byte rexChar[] = {
  B00110,
  B00111,
  B00111,
  B01110,
  B11111,
  B11110,
  B01010,
  B01001
};

byte hitChar[] = {
  B10101,
  B01110,
  B01110,
  B11111,
  B01110,
  B01110,
  B01010,
  B10001
};

const int CLEARCHAR = 0;
const int BASECHAR = 1;
const int WALLCHAR = 2;
const int REXCHAR = 3;
const int HITCHAR = 4;

byte arrayTrack[16] = { BASECHAR, BASECHAR, WALLCHAR, BASECHAR, BASECHAR, WALLCHAR, BASECHAR, WALLCHAR,
                        BASECHAR, BASECHAR, BASECHAR, WALLCHAR, BASECHAR, BASECHAR, BASECHAR, WALLCHAR };

// Store special character at LCD's memory 0 to 4
lcd.createChar(CLEARCHAR, clearChar);
lcd.createChar(BASECHAR, baseChar);
lcd.createChar(WALLCHAR, wallChar);
lcd.createChar(REXCHAR, rexChar);
lcd.createChar(HITCHAR, hitChar);
```

Next step is to have a function to display the scrolling track, it will update at each "timeInterval". It will record the characters a Wall or Base at t_Rex position.

```cpp
void writeTrack() {
  int tempInd = 0;
  // move at fix interval
  if ((millis() - timePrevious) > timeInterval) {
    lcd.setCursor(0, 1);  // Go to start of second line
    for (int i = 0; i < 16; i++) {
      tempInd = (i + scrollTrackIndex) % 16;                               // find the mod 16
      if (i == t_RexPos) {                                                // keep a copy of Char at t_Rex position for hit checking
        t_RexPosChar = arrayTrack[tempInd];                                // keep a copy of status at T-Rex position: wall or space
        if ((t_RexPosChar == WALLCHAR) && (t_RexStatus == HIGH)) Score++;  // gain 1 point each time passing a wall
        if (t_RexStatus == LOW) lcd.write(REXCHAR);                        // re-draw the t_REX at base location
        else lcd.write(arrayTrack[tempInd]);                               // If at jump position, update the track as ussual
      } else
        lcd.write(arrayTrack[tempInd]);                                    // update the track at none t_RexPos
    }
    scrollTrackIndex++;  // prepare for next, shift left
    timePrevious = millis();
  }
}
```

### t_Rex

If an "actionJump" is initiated, it will update the t_Rex to jump from lcd.setCursor(t_RexPos, 1) to lcd.setCursor(t_RexPos, 0).  
t_Rex only come down after "jumptimeInterval".

```cpp
void jumpWall() {
  if (actionJump == HIGH) {      // Action to jump
    actionJump = LOW;            // Set to low for next command
    lcd.setCursor(t_RexPos, 0);  // jump
    lcd.write(REXCHAR);
    t_RexStatus = HIGH;  // Update t_Rex position

    lcd.setCursor(t_RexPos, 1);
    lcd.write(BASECHAR);
    jumptimePrevious = millis();  // keep time it jumps

    // debug
    //Serial.printf("Jmp:%lu ", jumptimePrevious);

  } else if (((millis() - jumptimePrevious) > jumptimeInterval) && (t_RexStatus == HIGH)) {  // check if time to drop down
    t_RexStatus = LOW;                                                                       // update t_Rex position
    lcd.setCursor(t_RexPos, 0);                                                              // yes, clear the jump position
    lcd.write(CLEARCHAR);
    lcd.setCursor(t_RexPos, 1);  // down
    lcd.write(REXCHAR);
  }
}
```

### Checking the Hit

Only check if t_Rex is at the track (non-jump state), got hit if the track character at t_RexPos is a "WALL". If Hit, minus Score and numLife.

```cpp
// check whether t_Rex hitting the Wall
if ((t_RexStatus == LOW) && (t_RexPosChar == WALLCHAR)) {
  numLife--;                   // Hiting the wall, minus one life
  lcd.setCursor(t_RexPos, 1);  //
  lcd.write(HITCHAR);          // show hit character
  lcd.setCursor(t_RexPos, 1);  // ensure blinking on HITChar
  lcd.blink();
  delay(3000);
  lcd.noBlink();
  // Serial.println("Got HIT");
  actionJump = LOW;       // initiate a jump action after switch is pressed and released
  switchStateCur = HIGH;  // current switch status for checking switch pressed and released function
}
```

### Scoreboard

Update the Score and numLife on LCD row 0. Check for Game over if numLife = 0, and wait to start new game.

```cpp
void updateScoreboard() {

  // Print the game score and life
  lcd.setCursor(5, 0);
  lcd.print("S:");
  lcd.print(Score);

  //lcd.setCursor(15, 0);
  lcd.print(" L:");
  lcd.print(numLife, DEC);

  // check if any more Life to continue
  if (numLife == 0) {
    lcd.setCursor(0, 0);
    lcd.print("   Game Over!   ");

    lcd.setCursor(0, 1);
    lcd.print("Press Sw 2 start");

    // Wait user to continue
    switchState = digitalRead(togglePin);
    while (switchState == HIGH) {
      switchState = digitalRead(togglePin);
    }
    while (switchState == LOW) {
      switchState = digitalRead(togglePin);
    }
    // Clear the status and continue new game
    resetGame();
  }
}
```

### Random Wall Generator

May inject random WALL in the track instead of using default fix sequence.

```cpp
// Generate random WALL position
arrayTrack[0] == BASECHAR;
if (arrayTrack[15] == BASECHAR) {  // Avoid 2 consecutive WALLs
  tempInd = millis() % 2;
  if (tempInd == 1)
    arrayTrack[0] == WALLCHAR;
}
```

## 🎮 Other Top Game Ideas for 1 Button + LCD

### Reaction Time Tester (Best for beginners)
The LCD says "Get Ready...". After a random delay (2–5 seconds), it flashes "PRESS NOW!". The ESP32 measures exactly how many milliseconds it took you to press the button.

### 1-Button "Dino Jump" (Chrome Offline Game style)
A character (^) sits on the bottom row of the LCD. An obstacle (M) scrolls from right to left. Pressing the button makes the character "jump" to the top row for a second to dodge the obstacle.

### Speed Clicker Challenge
The game gives you exactly 10 seconds. The LCD displays a countdown. Your goal is to mash the button as many times as possible. It displays your final score and a rank (e.g., "Speed Demon!").

### Higher or Lower (Number Guessing)
The ESP32 picks a number from 1 to 100. You press the button to cycle through numbers. A short press adds 1, a long press (hold > 1 sec) adds 10. The LCD tells you "Higher!" or "Lower!" until you guess it.
