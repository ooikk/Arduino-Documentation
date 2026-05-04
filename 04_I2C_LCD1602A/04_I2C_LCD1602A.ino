

// use hd44780 library by Bill Perry, comment off to use standard LiquidCrystal_I2C library by Frank de Brabander
#define HD44780

// set the LCD number of columns and rows
int lcdColumns = 16;
int lcdRows = 2;

int Loop = 0;

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


/*****************
Scrolling text
******************/
String messageStatic = "Static message";
String messageToScroll = "This is a scrolling message with more than 16 characters";

// Function to scroll text
// The function acepts the following arguments:
// row: row number where the text will be displayed
// message: message to scroll
// delayTime: delay between each character shifting
// lcdColumns: number of columns of your LCD
void scrollText(int row, String message, int delayTime, int lcdColumns) {
  for (int i = 0; i < lcdColumns; i++) {
    message = " " + message;
  }
  message = message + " ";
  for (int pos = 0; pos < message.length(); pos++) {
    lcd.setCursor(0, row);
    lcd.print(message.substring(pos, pos + lcdColumns));
    delay(delayTime);
  }
}

/**************************
Display Custom Characters
***************************/
byte AmongUs[8] = {
  B01110,
  B10001,
  B10111,
  B10111,
  B10001,
  B10001,
  B10101,
  B11011
};
byte heart[8] = {
  B00000,
  B00000,
  B01010,
  B11111,
  B11111,
  B11111,
  B01110,
  B00100
};
byte smiley[8] = {
  B00000,
  B00000,
  B01010,
  B00000,
  B10001,
  B10001,
  B01110,
  B00000
};
byte man[8] = {
  B00000,
  B00100,
  B11111,
  B11111,
  B11111,
  B01110,
  B01010,
  B01010
};
byte creature[8] = {
  B00000,
  B11111,
  B11111,
  B10101,
  B10101,
  B11111,
  B01010,
  B11011
};
byte house[8] = {
  B00000,
  B00000,
  B00100,
  B01110,
  B11111,
  B01110,
  B01110,
  B01110
};

//8th sequence custom characters
byte tongue[8] = {
  B00000,
  B01010,
  B00000,
  B11111,
  B00101,
  B00101,
  B00010,
  B00000
};
byte bell[8] = {
  B00100,
  B01110,
  B01110,
  B01110,
  B01110,
  B11111,
  B00000,
  B00100
};
byte music[8] = {
  B00001,
  B00011,
  B00101,
  B01001,
  B01001,
  B01011,
  B11011,
  B11000
};
byte down[8] = {
  B01110,
  B01010,
  B01010,
  B01010,
  B11011,
  B10001,
  B01010,
  B00100
};
byte LED[8] = {
  B01110,
  B11111,
  B11111,
  B11111,
  B11111,
  B01010,
  B01001,
  B01001
};
byte turtle[8] = {
  B00000,
  B00000,
  B10101,
  B01110,
  B01110,
  B01110,
  B01110,
  B10001
};


void setup() {
#ifdef HD44780
  lcd.begin(16, 2);
#else
  // initialize LCD
  lcd.init();
#endif
  // turn on LCD backlight
  //lcd.backlight();
  lcd.noBacklight();
  lcd.createChar(0, AmongUs);
  lcd.createChar(1, heart);
  lcd.createChar(2, smiley);
  lcd.createChar(3, man);
  lcd.createChar(4, house);
  lcd.createChar(5, tongue);
  lcd.createChar(6, bell);
  lcd.createChar(7, music);
}

void loop() {


  /**************
    Static text
  ***************/

  Loop = 0;
  while (Loop < 1) {
    // set cursor to first column, first row
    lcd.clear();
    lcd.setCursor(0, 0);
    // print message
    lcd.cursor();
    lcd.blink();
    lcd.print("Hello, World!");
    delay(2000);
    // clears the display to print new message
    lcd.clear();
    // set cursor to first column, second row
    lcd.setCursor(0, 1);
    lcd.noBlink();
    lcd.noCursor();
    lcd.print("Hello, World!");

    delay(2000);

    Loop++;
  }

  /**************
    Scrolling text
  ***************/

  // set cursor to first column, first row
  lcd.setCursor(0, 0);
  // print static message
  lcd.print(messageStatic);
  // print scrolling message
  scrollText(1, messageToScroll, 250, lcdColumns);

  /**************
   Hardware Scrolling (Scrolls the whole screen)
   The LCD hardware has a built-in command to shift everything currently visible. This moves both Line 0 and Line 1 at the same time.
  ***************/
  lcd.setCursor(0, 0);
  lcd.clear();
  lcd.print("Hardware Scrolling Text! To the right");
  lcd.setCursor(0, 1);
  lcd.print("Second row message, right");
  delay(500);
  for (int i = 0; i < 16; i++) {
    lcd.scrollDisplayRight();  // Moves everything one step right
    delay(500);
  }
  delay(1000);

  lcd.setCursor(0, 0);
  lcd.clear();
  lcd.print("Hardware Scrolling Text! To the left");
  lcd.setCursor(0, 1);
  lcd.print("Second row message, left");
  delay(500);
  for (int i = 0; i < 16; i++) {
    lcd.scrollDisplayLeft();  // Moves everything one step left
    delay(500);
  }

  /**************************
  Display autoscroll
  ***************************/
  lcd.clear();
  lcd.setCursor(15, 0);
  lcd.print("Auto scroll on");
  lcd.setCursor(15, 1);  // Start at the very end of the top line
  lcd.autoscroll();      // Turn on the "pushing" effect

  for (int i = 0; i < 16; i++) {
    lcd.print(i);  // Each number stays at Col 15, but pushes the others left
    delay(500);    // Wait so you can see the movement
  }
  lcd.noAutoscroll();
  delay(1000);

  /**************************
  scrollDisplay
  ***************************/

  // Move everything 16 steps to the left

  lcd.clear();
  lcd.setCursor(0, 0);
  //lcd.print("Test Scrolling display");
  lcd.setCursor(0, 0);  // Start at the very end of the top line
  // Every row has storage for 40 characters
  lcd.print("Test Scrolling display to the left than right, second column");
  for (int i = 0; i < 32; i++) {
    lcd.scrollDisplayLeft();
    delay(500);  // Controlling the speed of the scroll
  }
  delay(2000);
  // Move everything 16 steps back to the right
  for (int i = 0; i < 32; i++) {
    lcd.scrollDisplayRight();
    delay(500);
  }
  delay(2000);

  /**************************
  leftToRight & rightToLeft
  ***************************/
  // Standard printing
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.leftToRight();
  lcd.print("ABCDEF");  // Displays "ABCDEF" at the start of Row 0

  // Reverse printing
  lcd.setCursor(15, 1);  // Move to the far right of Row 1
  lcd.rightToLeft();
  lcd.print("ABCDEF");  // Displays "ABCDEF" backwards at the end of Row 1
  delay(5000);

  /**************************
  Display Custom Characters
  ***************************/
  lcd.clear();
  lcd.home();
  lcd.print("Custom Character");
  lcd.setCursor(0, 1);  // Go to start of second line
  for (int i = 0; i < 8; i++) {
    lcd.write(i);  // Prints 0, 1, 2... 7 in a solid block
  }
  delay(10000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Internal Character");
  lcd.setCursor(8, 1);  // Go to middle of second line
  for (int i = 0; i < 8; i++) {
    lcd.write(i + 176);  // Prints 8, 9, 10... 15 in a solid block
  }
  delay(5000);

  /**************************
  Display on/ off
  ***************************/
  lcd.clear();
  lcd.home();
  lcd.display();
  lcd.print("Display On/Off");
  delay(2000);
  lcd.noDisplay();
  delay(2000);
  lcd.display();

}
