# I2C LCD1602A

The advantage of using an I2C LCD is that the wiring is really simple. You just need to wire the SDA and SCL pins.

Additionally, it comes with a built-in potentiometer you can use to adjust the contrast between the background and the characters on the LCD. 


<img alt="image" style="width: 50%; height: auto;"  src="https://github.com/user-attachments/assets/b0b46067-8f01-401b-a87a-c077a97138b9" />
<br>

**Adjust the contrast:**
<br>

<img alt="image" style="width: 50%; height: auto;"  src="https://github.com/user-attachments/assets/32d7bda1-d868-4a08-b158-d78e17443d39" />


## Connect to LCD

```
I2C LCD          ESP32 S3
GND              GND
VCC              VIN (2.5V to 6V)
SDA              GPIO 8
SCL              GPIO 9
```

<img alt="image" style="width: 50%; height: auto;"  src="https://github.com/user-attachments/assets/af19d5a4-3f3a-4a48-9033-d0d5df3f50e8" />

## Functions to program the LCD

There are several libraries that work with the I2C LCD.

Import library: LiquidCrystal_I2C 2.0.0 by Martin Kubovcik, Frank de Brabander

Frequent used functions
```
// Defines the I2C address, number of rows, number of columns
LiquidCrystal_I2C(uint8_t lcd_Addr,uint8_t lcd_cols,uint8_t lcd_rows);

// Clears the lcd display and places cursor at the top left corner
clear();

// initiates and start the lcd
init();

// Display backlight
noBacklight();                   // Turns off 
backlight();                     // Turns on the display backlight
setBacklight(uint8_t new_val);   // alias for backlight() and nobacklight()

// Prints out and characters or text on the lcd display
print();
printstr(const char[]);
printLeft();
printRight();

// writes a character to whichever position in the LCD
write(); 

// Cursor operations
noCursor();                     // Hides the LCD cursor
cursor();                       // Creates a blinking block cursor
setCursor(uint8_t, uint8_t);    // Sets a new message on a new cursor

// Display
noDisplay();                    // Turns off
display(); 	                    // Turns on 

// Scroll control
autoscroll();                  // Set the display to auto scroll to the left each time a letter is added
noAutoscroll();                // Stops auto scrolling
scrollDisplayLeft();           // Sets the scroll display orientation one position towards the left
scrollDisplayRight();          // Sets the scroll display orientation one position towards the right

// Characters display direction
leftToRight();
rightToLeft();
shiftIncrement();
shiftDecrement();

// Display to blink/ no blink
noBlink();
blink();

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


<img width="705" height="873" alt="image" src="https://github.com/user-attachments/assets/de78a54b-a890-4319-9ca3-36a3fb443f6e" />
<br>
<img width="698" height="861" alt="image" src="https://github.com/user-attachments/assets/904ddb6c-3d7e-4ad1-a57b-35879ecd3d60" />


## Reference

https://docs.arduino.cc/learn/electronics/lcd-displays/

https://randomnerdtutorials.com/esp32-esp8266-i2c-lcd-arduino-ide/

https://www.luisllamas.es/en/arduino-lcd-i2c/
