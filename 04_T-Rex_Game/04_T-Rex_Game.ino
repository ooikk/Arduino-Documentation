// use hd44780 library by Bill Perry, comment off to use standard LiquidCrystal_I2C library by Frank de Brabander
#define HD44780

// set the LCD number of columns and rows
int lcdColumns = 16;
int lcdRows = 2;


#ifdef HD44780
#include <Wire.h>
#include <hd44780.h>                        // Main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h>  // I2C expander I/O class header
hd44780_I2Cexp lcd;                         // it can usually auto-detect your I2C address and pin mappings
#else
#include <LiquidCrystal_I2C.h>

// set LCD address, number of columns and rows
// if you don't know your display address, run an I2C scanner sketch
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);
#endif

/**************************
Display Custom Characters
***************************/
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


const int togglePin = 4;     // switch to trigger jump action
int switchState = HIGH;      // read switch status
bool actionJump = LOW;       // initiate a jump action after switch is pressed and released
bool switchStateCur = HIGH;  // current switch status for checking switch pressed and released function

int t_RexPos = 3;
bool t_RexStatus = LOW;       // at track position
int t_RexPosChar = BASECHAR;  // Char at t_Rex position
byte arrayTrack[16] = { BASECHAR, BASECHAR, WALLCHAR, BASECHAR, BASECHAR, WALLCHAR, BASECHAR, WALLCHAR,
                        BASECHAR, BASECHAR, BASECHAR, WALLCHAR, BASECHAR, BASECHAR, BASECHAR, WALLCHAR };
int scrollTrackIndex = 0;
unsigned long timeInterval = 1000;  // seconds
unsigned long timePrevious = 0;
unsigned long jumptimeInterval = timeInterval * 1.5;  // seconds
unsigned long jumptimePrevious = 0;

const int NUMLIFE = 3;  // allow trial time
int numLife = NUMLIFE;
int Score = 0;  // Score line

void resetGame() {
  // Reset variables
  t_RexStatus = LOW;
  Score = 0;
  numLife = NUMLIFE;
  timePrevious = 0;
  jumptimePrevious = 0;
  // Draw the T-Rex exactly ONCE for the start of the game
  lcd.clear();
 // lcd.setCursor(t_RexPos, 1);
 // lcd.write(REXCHAR);
}

void writeTrack() {
  int tempInd = 0;
  // move at fix interval
  if ((millis() - timePrevious) > timeInterval) {
    //unsigned long currentMillis = millis();  // Capture the time once
    //Serial.printf("Track== Prev:%lu  Cur:%lu  Diff:%lu\n", timePrevious, currentMillis, (currentMillis - timePrevious));

    timePrevious = millis();
    lcd.setCursor(0, 1);  // Go to start of second line
    for (int i = 0; i < 16; i++) {
      tempInd = (i + scrollTrackIndex) % 16;                               // find the mod 16
      if (i == t_RexPos) {                                                 // keep a copy of Char at t_Rex position for hit checking
        t_RexPosChar = arrayTrack[tempInd];                                // keep a copy of status at T-Rex position: wall or space
        if ((t_RexPosChar == WALLCHAR) && (t_RexStatus == HIGH)) Score++;  // gain 1 point each time passing a wall
        if (t_RexStatus == LOW) lcd.write(REXCHAR);                        // re-draw the t_REX at base location
        else lcd.write(arrayTrack[tempInd]);                               // If at jump position, update the track as ussual
      } else
        lcd.write(arrayTrack[tempInd]);                                    // update the track at none t_RexPos
    }
    scrollTrackIndex++;  // prepare for next, shift left
  }
}

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
    // debug
    //unsigned long jumpcurrentMillis = millis();  // Capture the time once
    //Serial.printf("Jump== Prev:%lu  Cur:%lu  Diff:%lu\n", jumptimePrevious, jumpcurrentMillis, (jumpcurrentMillis - jumptimePrevious));
  }
}

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


void setup() {
  // Initialize serial communication at 115200 baud rate
  Serial.begin(115200);

  // Configure the switch pin as INPUT
  pinMode(togglePin, INPUT);

#ifdef HD44780
  lcd.begin(16, 2);
#else
  // initialize LCD
  lcd.init();
#endif

  // Store special character at LCD's memory 0 to 4

  lcd.createChar(CLEARCHAR, clearChar);
  lcd.createChar(BASECHAR, baseChar);
  lcd.createChar(WALLCHAR, wallChar);
  lcd.createChar(REXCHAR, rexChar);
  lcd.createChar(HITCHAR, hitChar);

  lcd.clear();
  lcd.setCursor(0, 0);
  // print message
  //lcd.cursor();
  //lcd.blink();
  lcd.print("Welcome to T-Rex Game");
  lcd.setCursor(0, 1);
  lcd.print("Press Sw 2 start");

  switchState = digitalRead(togglePin);

  // Wait for switch press, LOW status
  while (switchState == HIGH) {
    switchState = digitalRead(togglePin);
  }
  // Wait for switch release, HIGH status
  while (switchState == LOW) {
    switchState = digitalRead(togglePin);
  }

  //lcd.clear();
  resetGame();
}

void loop() {
  // When T-Rex at rest position, read the digital state of the switch pin.
  // Otherwise, continuous pressing the switch will keep T-Rex at jump position
  if (t_RexStatus == LOW) {
    switchState = digitalRead(togglePin);
    if (switchState == LOW) {
      //Serial.println("Switch Status: ON");
      //    lcd.setCursor(0, 0);
      //    lcd.print("Swicth Low");
      switchStateCur = LOW;
    }
    if ((switchStateCur == LOW) && (switchState == HIGH)) {
      switchStateCur = HIGH;  // confirm user press and release switch
      actionJump = HIGH;      // trigger jump action
      //Serial.println("Switch Status: ON");
      //lcd.clear();
      //lcd.setCursor(0, 0);
      //lcd.print("Switch Toggled");
    }
  }

  writeTrack();
  jumpWall();

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
  }
  updateScoreboard();
}
