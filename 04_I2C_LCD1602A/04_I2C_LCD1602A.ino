

// use hd44780 library by Bill Perry, comment off to use standard LiquidCrystal_I2C library by Frank de Brabander
// #define HD44780


#ifdef HD44780
#include <Wire.h>
#include <hd44780.h>                        // Main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h>  // I2C expander I/O class header
hd44780_I2Cexp lcd;                         // it can usually auto-detect your I2C address and pin mappings
#else
#include <LiquidCrystal_I2C.h>

// set the LCD number of columns and rows
int lcdColumns = 16;
int lcdRows = 2;
int Loop = 0;

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
  lcd.backlight();
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

  while (Loop < 5) {
    // set cursor to first column, first row
    lcd.setCursor(0, 0);
    // print message
    lcd.print("Hello, World!");
    delay(1000);
    // clears the display to print new message
    lcd.clear();
    // set cursor to first column, second row
    lcd.setCursor(0, 1);
    lcd.print("Hello, World!");
    delay(1000);
    lcd.clear();
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
  lcd.setCursor(5, 0);
  lcd.print("Custom Character");
  lcd.setCursor(8, 1);  // Go to middle of second line
  for (int i = 0; i < 8; i++) {
    lcd.write(i);  // Prints 0, 1, 2... 7 in a solid block
  }
  delay(10000);
  Loop = 0;
}
