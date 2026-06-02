# T_Rex Game
The T-Rex Game (also known as the Chrome Dino Game) is a simple, built-in side-scrolling runner game created by Google.

It famously appears in the Google Chrome web browser whenever you try to visit a webpage but lose your internet connection. The character design is a nod to the "prehistoric ages" of being offline.

**How It Works**
The gameplay is incredibly straightforward but gets addictive as the speed increases:

The Goal: Guide a pixelated Tyrannosaurus rex across a desert landscape for as long as possible to get a high score.

Obstacles: You must dodge oncoming obstacles like cacti and flying pterodactyls.

**Controls:**

Spacebar or Up Arrow: Jump over obstacles.

Down Arrow: Duck under flying obstacles (on desktop).

Tap Screen: Jump (on mobile devices).

As you advance, the game transitions between day mode (light background) and night mode (dark background) every 700 points.

**How to Play It (Even When Online)**
You don't actually have to pull your internet plug to play it. You can launch it anytime by opening a new tab in Google Chrome and typing this into the address bar:

chrome://dino

Fun Fact: The game is theoretically endless. The developers capped the maximum playtime at approximately 17 million years—a playful reference to how long the T-Rex was alive on Earth before extinction.

<img alt="image" style="width: 75%; height: auto;"  src="https://github.com/user-attachments/assets/f703984a-98c2-4fa8-af7e-9a35bc16a68d" />


# The hardware setup

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

```
// Toggle switch connects to GPIO 4 of ESP32
const int togglePin = 4;     // switch to trigger jump action
```

<img alt="image" style="width: 75%; height: auto;" src="https://github.com/user-attachments/assets/4f50c7ff-d5d9-4342-916f-aa5fc1925757" />

# The Game

## Toggle Switch

To prevent abnormal action if someone press the switch for very long, the action only start with "Press" and "Released" sequence.

This code will wait until user Press and Release. Use in none playing mode.
```
    // Wait user to continue
    switchState = digitalRead(togglePin);
    while (switchState == HIGH) {
      switchState = digitalRead(togglePin);
    }
    while (switchState == LOW) {
      switchState = digitalRead(togglePin);
    }
```

Under Play mode, need to release CPU for other tasks. Use a flag **switchStateCur** to indicate switch state. Once confirm Press and Release, start the
jump action.

```
    switchState = digitalRead(togglePin);
    if (switchState == LOW) {
      switchStateCur = LOW;
    }
    if ((switchStateCur == LOW) && (switchState == HIGH)) {
      switchStateCur = HIGH;  // confirm user press and release switch
      actionJump = HIGH;      // trigger jump action
    }

```
## Moving Track
For the playing scene, defined custom characters. Use arrayTrack[16] for characters to be displayed on LCD screen. For the running track, only baseChar and wallChar to be displayed.

```
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
const int CLEARCHAR = 0;

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

const int BASECHAR = 1;

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
const int WALLCHAR = 2;

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
const int REXCHAR = 3;

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
const int HITCHAR = 4;

byte arrayTrack[16] = { BASECHAR, BASECHAR, WALLCHAR, BASECHAR, BASECHAR, WALLCHAR, BASECHAR, WALLCHAR,
                        BASECHAR, BASECHAR, BASECHAR, WALLCHAR, BASECHAR, BASECHAR, BASECHAR, WALLCHAR };


  // Store special character at LCD's memory 0 to 4
  lcd.createChar(0, clearChar);
  lcd.createChar(1, baseChar);
  lcd.createChar(2, wallChar);
  lcd.createChar(3, rexChar);
  lcd.createChar(4, hitChar);

```

Next step is to have a function to display the scrolling track, it will update at each "timeInterval". It will record the characters a Wall or Base t_Rex position.

```
void writeTrack() {
  int tempInd = 0;
  // move at fix interval
  if ((millis() - timePrevious) > timeInterval) {
    timePrevious = millis();
    lcd.setCursor(0, 1);  // Go to start of second line
    for (int i = 0; i < 16; i++) {
      tempInd = (i + scrollTrackIndex) % 16;    // find the mod 16
      if (i == t_RexPos) {                      // keep a copy of Char at t_Rex position for hit checking
        t_RexPosChar = arrayTrack[tempInd];     // keep a copy of status at T-Rex position: wall or space
        if (t_RexPosChar == WALLCHAR) Score++;  // gain 1 point each time passing a wall
      }
      lcd.write(arrayTrack[tempInd]);  // update the track
    }
    scrollTrackIndex++;  // prepare for next, shift left
  }
}
```

## t_Rex

If an "actionJump" is initiated, it will update the t_Rex to jump from lcd.setCursor(t_RexPos, 1) to lcd.setCursor(t_RexPos, 0). 
t_Rex only come down after "jumptimeInterval".

```
void jumpWall() {
  if (actionJump == HIGH) {      // Action to jump
    actionJump = LOW;            // Set to low for next command
    lcd.setCursor(t_RexPos, 0);  // jump
    lcd.write(REXCHAR);
    t_RexStatus = HIGH;  // Update t_Rex position

    lcd.setCursor(t_RexPos, 1);
    lcd.write(BASECHAR);
    jumptimePrevious = millis();  // keep time it jumps

  } else if ((millis() - jumptimePrevious) > jumptimeInterval) {  // check if time to drop down
    lcd.setCursor(t_RexPos, 0);                                   // yes, clear the jump position
    lcd.write(CLEARCHAR);

    lcd.setCursor(t_RexPos, 1);  // down
    lcd.write(REXCHAR);
    t_RexStatus = LOW;  // update t_Rex position
  }
}
```
## Checking the hit

Only check if t_Rex is at the track (non-jump state), got hit if the track character at t_RexPos is a "WALL". If Hit, minus Score and numLife.
```
  // check whether t_Rex hitting the Wall
  if ((t_RexStatus == LOW) && (t_RexPosChar == WALLCHAR)) {
    numLife--;                   // Hiting the wall, minus one life
    Score--;                     // minus one point as it was added in writeTrack
    lcd.setCursor(t_RexPos, 1);  //
    lcd.write(HITCHAR);          // show hit character
    lcd.setCursor(t_RexPos, 1);  // ensure blinking on HITChar
    lcd.blink();
    delay(3000);
    lcd.noBlink();
    // Serial.println("Got HIT");
  }
```

## Scoreboard
Update the Score and numLife on LCD row 0. Check for Game over if numLife = 0, and wait to start new game.
```
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
    Score = 0;
    numLife = NUMLIFE;
    lcd.clear();
  }
}
```
