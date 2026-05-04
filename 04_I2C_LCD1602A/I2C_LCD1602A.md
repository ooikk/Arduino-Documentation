# I2C LCD1602A

The advantage of using an I2C LCD is that the wiring is really simple. You just need to wire the SDA and SCL pins.

Additionally, it comes with a built-in potentiometer you can use to adjust the contrast between the background and the characters on the LCD. 


<img alt="image" style="width: 50%; height: auto;"  src="https://github.com/user-attachments/assets/b0b46067-8f01-401b-a87a-c077a97138b9" />
<br>

**Adjust the contrast and backlight:**
<br>

<img alt="image" style="width: 50%; height: auto;"  src="https://github.com/user-attachments/assets/7cdc463a-4890-4948-add1-5b92a697ad7a" />

User may add a series resistor (>1k ohm) in between the Backlight Jumper pins to reduce the backlight. Or removed jumper to disable backlight.
*Ensure it's power by at least 5V.*

https://www.edn.com/arduino-i2c-lcd-backpack-introductory-tutorial/

**I2C-to-LCD piggy-back board**

<img alt="image" style="width: 75%; height: auto;"   src="https://github.com/user-attachments/assets/ea1fd770-d6b1-4c40-819a-699da7de073b" />

https://www.handsontec.com/dataspecs/I2C_LCD_Interface.pdf

**Changing the I2C Address on an LCD Backpack**

The table below shows the full range of possible addresses. A value of 1 means the pad is left open, while 0 means the pad is shorted.

```
A0   A1   A2   Address
0    0    0    0x20
0    0    1    0x21
0    1    0    0x22
0    1    1    0x23
1    0    0    0x24
1    0    1    0x25
1    1    0    0x26
1    1    1    0x27
```

**2 rows by 16 columns** <br>
<img width="885" height="217" alt="image" src="https://github.com/user-attachments/assets/df52c290-c214-40c2-bc45-af422046cb6e" />


**8-row dots by 5 column dots** <br>

<img width="138" height="202" alt="image" src="https://github.com/user-attachments/assets/4507eb39-1b6d-4856-8a0d-54a852ad6f9a" />

**16x2 LCD Command Codes**

This table is a quick reference for the most commonly used 16x2 LCD commands. These commands apply to displays based on the HD44780 controller and are useful when controlling the screen, cursor behaviour, and text position in Arduino and other microcontroller projects.

```
Command     (Hex)   Name        Description
0x01        Clear display       Clears all text and moves the cursor to the home position
0x02        Return home         Moves the cursor to the top left without clearing the display
0x04        Entry mode set      Cursor moves left after writing a character
0x06        Entry mode set      Cursor moves right after writing a character
0x0C        Display on          Turns the display on with cursor and blink disabled
0x0E        Display on cursor   Turns the display on with the cursor visible
0x0F        Display on blink    Turns the display on with cursor blinking
0x10        Cursor move         Moves the cursor one position to the left
0x14        Cursor move         Moves the cursor one position to the right
0x80 + col  Set cursor line 1   Moves the cursor to a specific column on the first line
0xC0 + col  Set cursor line 2   Moves the cursor to a specific column on the second line
```

## Connect to LCD

```
I2C LCD          ESP32 S3
GND              GND
VCC              VIN (2.5V to 6V)  // It's recommended connect to >5V to ensure enough contrast.
SDA              GPIO 8
SCL              GPIO 9
```


<img alt="image" style="width: 50%; height: auto;"  src="https://github.com/user-attachments/assets/75a726ff-287c-40fa-894c-ef5fa2304fa2" />


## Library for LCD

There are several libraries that work with the I2C LCD.

### 1.0 LiquidCrystal_I2C by Frank de Brabander

This is the "standard" library most tutorials use. While it may still trigger the AVR warning on some versions, it is confirmed to work with ESP32.<br>
```
WARNING: library LiquidCrystal I2C claims to run on avr architecture(s) and may be incompatible with your current board
which runs on esp32 architecture(s).
```
**How to Silence the Warning (Optional)**

If you prefer to keep using your current library, you can manually stop the warning:
- Navigate to your Arduino libraries folder (usually Documents/Arduino/libraries/LiquidCrystal_I2C).
- Open the library.properties file with a text editor.  
- Find the line architectures=avr.
- Change it to architectures=* or architectures=avr,esp32.  
- Save the file and restart the Arduino IDE. The warning will disappear.

**To install the library:**
- Search for "LiquidCrystal I2C" in the Library Manager.
- Choose the one maintained by Marco Schwartz or Frank de Brabande
- Import library: **LiquidCrystal_I2C** 1.1.2 by Frank de Brabander 
- https://github.com/johnrickman/LiquidCrystal_I2C


### 2.0 Recommended Library: hd44780 by Bill Perry

The most robust and highly recommended alternative for the ESP32-S3 is the hd44780 library by Bill Perry.  

**To install the library:**
- Import library: **hd44780** 1.3.2 by Bill Perry
- https://github.com/duinoWitchery/hd44780

**Why it's a recommended Library:** <br>
- **Why it's better:** It is designed to be architecture-agnostic, meaning it will compile on the ESP32-S3 without any architecture warnings.  
- **Auto-Detection:** It can automatically detect the I2C address (e.g., 0x27 or 0x3F) and the pin mappings between the backpack and the LCD, which saves significant troubleshooting time.
- **Performance:** It is faster and more compliant with modern Arduino standards than older LiquidCrystal_I2C forks.

**To use this libary:**

**1.0 The Replacement Headers**
  
  Replace:
```
#include <LiquidCrystal_I2C.h>
```
  with these three lines:
```
#include <Wire.h>
#include <hd44780.h>                       // Main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h> // I2C expander I/O class header
```
**2.0 Update the Object Declaration**

You also need to change how you define your lcd object. The hd44780 library is smarter—it can usually auto-detect your I2C address and pin mappings, so you don't have to hardcode 0x27 or 0x3F.

Old Way:

```
LiquidCrystal_I2C lcd(0x27, 16, 2);
```

New Way (The "Auto-Config" Way):

```
hd44780_I2Cexp lcd;
```

Check the github link above to discover new capabilities in hd44780

**3.0 Comparison of setup()**

The initialization function name changes slightly as well. While LiquidCrystal_I2C often used lcd.init(), hd44780 uses the more standard lcd.begin().

```
Feature        Old (LiquidCrystal_I2C)               New (hd44780)
Object         LiquidCrystal_I2C lcd(0x27, 16, 2);   hd44780_I2Cexp lcd;
Setup          lcd.init();                           lcd.begin(16, 2);
Backlight      lcd.backlight();                      lcd.backlight(); (Same)
```

The hd44780 library is written to be "architecture-neutral." Unlike the library in LiquidCrystal_I2C) that strictly claimed to be for avr, hd44780 tells the Arduino IDE that it works on any architecture (*), which includes the ESP32-S3. No more annoying yellow warning text! </LiquidCrystal_I2C.h>


## Functions to program the LCD

Frequent used functions
```
// Defines the I2C address, number of rows, number of columns
LiquidCrystal_I2C(uint8_t lcd_Addr,uint8_t lcd_cols,uint8_t lcd_rows);

// Clears the lcd display and places cursor at the top left corner
clear();

// initiates and start the lcd
init();                          // LiquidCrystal_I2
begin(16, 2);                    // for hd44780

// Display backlight
noBacklight();                   // Turns off 
backlight();                     // Turns on the display backlight

// Display
noDisplay();                    // Turns off, backlight still ON
display(); 	                    // Turns on

// Prints out and characters or text on the lcd display
print();
/**
The print() function is a higher-level command. It translates data into a human-readable format (ASCII strings)
before sending it to the LCD.
Primary Use: Displaying variables, sensor readings, and text strings.
Behavior: If you send the number 65, the library realizes you want to see the number "65". It converts the
integer into two separate characters—'6' and '5'—and sends them sequentially.
Base Control: It allows you to specify formats, such as lcd.print(255, HEX); which would display "FF".
Example:
lcd.print(0); // Displays the digit '0'.
lcd.print(65); // Displays the digits '6' and '5'.
**/

// writes a character to whichever position in the LCD
write();
/**
lcd.write() — The "Raw" Messenger
The write() function sends binary data (raw bytes) directly to the device without any translation.
Primary Use: Displaying custom characters (0–7) or specific symbols.
Behavior: If you send the number 65, the LCD looks at its internal table, finds character #65,
and displays an 'A'.
Return Value: It returns the number of bytes successfully written (usually 1).
Example:
lcd.write(0); // Displays the Custom Character stored at index 0.
lcd.write(65); // Displays the letter 'A'. See Character Codes and Character Patterns (ROM Code: A00) below
**/

// Cursor operations
noCursor();                     // Hides the LCD cursor
cursor();                       // Creates a blinking block cursor
setCursor(uint8_t, uint8_t);    // Sets a new message on a new cursor

// Cursor to blink/ no blink
noBlink();
blink();

// Scroll control
autoscroll();                  // The display is active. When you print "A", the 'A' appears at the fixed cursor position, and
                               // the rest of the text on the screen shifts one space to the left to "push" the existing text
                               // out of the way.
noAutoscroll();                // (Default): The cursor is active. When you print "A", the 'A' appears at the cursor, and the
                               // cursor jumps one step to the right.

// To understand these functions, you have to look at how the LCD controller (the HD44780 chip) handles memory. A standard 1602
// LCD has 16 visible columns, but it actually has enough internal memory (DDRAM) to store 40 characters per line.
// Think of the physical 16x2 screen as a viewing window that sits over a much wider 40x2 strip of memory.
scrollDisplayLeft();           // Shifts the "window" one position to the right across the memory, which makes the text on your
                               // screen look like it is moving left.
scrollDisplayRight();          // Shifts the "window" one position to the left, making the text appear to move right.

// Characters display direction
leftToRight();                 // (Default): Move the cursor right after printing
rightToLeft();                 // Move the cursor left after printing

// This function is needed to create custom character in the program
createChar(uint8_t, uint8_t[]);  // uint8_t: location 0 to 7

// Example: Creates the custom character for smiley face:
byte smiley[] = {
B00000,
B10001,
B00000,
B00000,
B10001,
B01110,
B00000,
B00000,
};
createChar(0,smiley);      // location 0

write(0);                  // write custom character stored from location 0
```
## Display Static Text on the LCD

```
/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com  
*********/

#include <LiquidCrystal_I2C.h>

// set the LCD number of columns and rows
int lcdColumns = 16;
int lcdRows = 2;

// set LCD address, number of columns and rows
// if you don't know your display address, run an I2C scanner sketch
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);  

void setup(){
  // initialize LCD
  lcd.init();
  // turn on LCD backlight                      
  lcd.backlight();
}

void loop(){
  // set cursor to first column, first row
  lcd.setCursor(0, 0);
  // print message
  lcd.print("Hello, World!");
  delay(1000);
  // clears the display to print new message
  lcd.clear();
  // set cursor to first column, second row
  lcd.setCursor(0,1);
  lcd.print("Hello, World!");
  delay(1000);
  lcd.clear(); 
}
```

## Display Scrolling Text on the LCD

```
/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com  
*********/

#include <LiquidCrystal_I2C.h>

// set the LCD number of columns and rows
int lcdColumns = 16;
int lcdRows = 2;

// set LCD address, number of columns and rows
// if you don't know your display address, run an I2C scanner sketch
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);  

String messageStatic = "Static message";
String messageToScroll = "This is a scrolling message with more than 16 characters";

// Function to scroll text
// The function acepts the following arguments:
// row: row number where the text will be displayed
// message: message to scroll
// delayTime: delay between each character shifting
// lcdColumns: number of columns of your LCD
void scrollText(int row, String message, int delayTime, int lcdColumns) {
  for (int i=0; i < lcdColumns; i++) {
    message = " " + message;  
  } 
  message = message + " "; 
  for (int pos = 0; pos < message.length(); pos++) {
    lcd.setCursor(0, row);
    lcd.print(message.substring(pos, pos + lcdColumns));
    delay(delayTime);
  }
}

void setup(){
  // initialize LCD
  lcd.init();
  // turn on LCD backlight                      
  lcd.backlight();
}

void loop(){
  // set cursor to first column, first row
  lcd.setCursor(0, 0);
  // print static message
  lcd.print(messageStatic);
  // print scrolling message
  scrollText(1, messageToScroll, 250, lcdColumns);
}
```

## Display Custom Characters

In a 16×2 LCD there are 32 blocks where you can display characters. Each block is made out of 5×8 tiny pixels. You can display custom characters by defining the state of each tiny pixel. For that, you can create a byte variable to hold  the state of each pixel.

To create your custom character, you can go here to generate the byte variable for your character. 

https://maxpromer.github.io/LCD-Character-Creator/

<img alt="image" style="width: 50%; height: auto;"  src="https://github.com/user-attachments/assets/5b944f40-7727-4f61-92ab-af41c521c1b8" />


```
#include <LiquidCrystal_I2C.h>

// Set the LCD address to 0x27 in PCF8574 by NXP and Set to 0x3F in PCF8574A by Ti
LiquidCrystal_I2C lcd(0x27, 16, 2);

byte customChar[] = {
  B00000,
  B01010,
  B11111,
  B11111,
  B11111,
  B01110,
  B00100,
  B00000
};

void setup() {
  // initialize LCD
  lcd.init();
  lcd.clear();
  // turn on LCD backlight                      
  lcd.backlight();
  lcd.createChar(0, customChar);
}

void loop() { 
  lcd.setCursor(0,3);
  lcd.write(0);
}

```

## Character Codes and Character Patterns (ROM Code: A00)

<img width="512" height="864" alt="image" src="https://github.com/user-attachments/assets/9ffb9230-d2d5-459d-b8c7-0e7f68089267" />
<br>

<img width="698" height="861" alt="image" src="https://github.com/user-attachments/assets/904ddb6c-3d7e-4ad1-a57b-35879ecd3d60" />


## Reference

https://docs.arduino.cc/learn/electronics/lcd-displays/

https://randomnerdtutorials.com/esp32-esp8266-i2c-lcd-arduino-ide/

https://www.luisllamas.es/en/arduino-lcd-i2c/

https://easyelecmodule.com/how-to-use-the-lcd1602a/

- https://easyelecmodule.com/wp-content/uploads/CN0295D-other-related-document.pdf
- https://easyelecmodule.com/wp-content/uploads/eone-1602a1-1.pdf
- https://easyelecmodule.com/wp-content/uploads/PCF8574-2.pdf

https://zaitronics.com.au/blogs/guides/how-to-use-16x2-lcd-parallel-i2c?
