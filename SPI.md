## SPI Communication Protocol
SPI stands for Serial Peripheral Interface, and it is a synchronous serial data protocol used by microcontrollers to communicate with one or more peripherals.

For SPI communication you need four lines:<br>
- MISO: Master In Slave Out, may be labeled as SDO (Serial Data Out)
- MOSI: Master Out Slave In, may be labeled as SDI (Serial Data In)
- SCK: Serial Clock
- CS /SS: Chip Select (used to select the device when multiple peripherals are used on the same SPI bus)<br>

<img width="903" height="254" alt="image" src="https://github.com/user-attachments/assets/85e4fd23-24cf-46db-b761-6496f441d13e" />

## ESP32-S3 Default SPI Pins
The ESP32 integrates 4 SPI peripherals: SPI0, SPI1, SPI2 (commonly referred to as HSPI), and SPI3 (commonly referred to as VSPI).

SP0 and SP1 are used internally to communicate with the built-in flash memory. You can use HSPI and VSPI to communicate with other devices. HSPI and VSPI have independent bus signals, and each bus can drive up to three SPI slaves.

<img width="1007" height="190" alt="image" src="https://github.com/user-attachments/assets/4f10774c-3865-447d-8831-a3d207e15680" />

You can use the following code to find the default SPI pins:

```
/* By Rui Santos   */
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.print("MOSI: ");
  Serial.println(MOSI);
  Serial.print("MISO: ");
  Serial.println(MISO);
  Serial.print("SCK: ");
  Serial.println(SCK);
  Serial.print("SS: ");
  Serial.println(SS);  
}

void loop() {
  // put your main code here, to run repeatedly:
}
```
## Multiple SPI Devices (same bus, different CS pin)

<img width="904" height="495" alt="image" src="https://github.com/user-attachments/assets/3f7e2736-9b78-4ce8-a4e4-a5a075e19e4b" />

To select the device use:
```
digitalWrite(CS_1, HIGH); // disable CS pin from peripheral 1
digitalWrite(CS_2, LOW);  // enable CS pin to read from peripheral 2
```
## ESP32 Using Two SPI Bus Interfaces (Use HSPI and VSPI simultaneously)

<img width="897" height="497" alt="image" src="https://github.com/user-attachments/assets/f62d5fa5-74cc-4047-ad84-9c1de4100111" />

To use HSPI and VSPI simultaneously, you just need to:
```
#include <SPI.h>  // include the SPI library
```
```
// Initialize two SPIClass objects with different names
vspi = new SPIClass(VSPI);
hspi = new SPIClass(HSPI);
```
```
// To begin
vspi.begin();
hspi.begin();
// OR custom pins
vspi.begin(VSPI_CLK, VSPI_MISO, VSPI_MOSI, VSPI_SS);
hspi.begin(HSPI_CLK, HSPI_MISO, HSPI_MOSI, HSPI_SS);
```
```
//  Define the SS pins as outputs
pinMode(VSPI_SS, OUTPUT);
pinMode(HSPI_SS, OUTPUT);
```

Here is the example of the final code.
```

/* The ESP32 has four SPi buses, however as of right now only two of
 * them are available to use, HSPI and VSPI. Simply using the SPI API
 * as illustrated in Arduino examples will use VSPI, leaving HSPI unused.
 *
 * However if we simply initialize two instance of the SPI class for both
 * of these buses both can be used. However when just using these the Arduino
 * way only will actually be outputting at a time.
 *
 * Logic analyzer capture is in the same folder as this example as
 * "multiple_bus_output.png"
 *
 * created 30/04/2018 by Alistair Symonds
 */
// #include <Arduino.h>
#include <SPI.h>

// Define ALTERNATE_PINS to use non-standard GPIO pins for SPI bus

#ifdef ALTERNATE_PINS
#define VSPI_MISO 2
#define VSPI_MOSI 4
#define VSPI_SCLK 0
#define VSPI_SS   33

#define HSPI_MISO 26
#define HSPI_MOSI 27
#define HSPI_SCLK 25
#define HSPI_SS   32
#else
#define HSPI_MISO MISO   // 13
#define HSPI_MOSI MOSI   // 11
#define HSPI_SCLK SCK    // 12
#define HSPI_SS   SS     // 10

#define VSPI_MISO 37
#define VSPI_MOSI 35
#define VSPI_SCLK 36
#define VSPI_SS   39
#endif

#if !defined(CONFIG_IDF_TARGET_ESP32)
#define VSPI FSPI
#endif

static const int spiClk = 1000000;  // 1 MHz

//uninitialized pointers to SPI objects
SPIClass *vspi = NULL;
SPIClass *hspi = NULL;

void setup() {
  //initialize two instances of the SPIClass attached to VSPI and HSPI respectively
  vspi = new SPIClass(VSPI);
  hspi = new SPIClass(HSPI);

  //clock miso mosi ss

#ifndef ALTERNATE_PINS
  //initialize vspi with default pins
  //SCLK = 36, MISO = 37, MOSI = 35, SS = 39
  vspi->begin();
#else
  //alternatively route through GPIO pins of your choice
  vspi->begin(VSPI_SCLK, VSPI_MISO, VSPI_MOSI, VSPI_SS);  //SCLK, MISO, MOSI, SS
#endif

#ifndef ALTERNATE_PINS
  //initialize hspi with default pins
  //SCLK = 12, MISO = 13, MOSI = 11, SS = 10
  hspi->begin();
#else
  //alternatively route through GPIO pins
  hspi->begin(HSPI_SCLK, HSPI_MISO, HSPI_MOSI, HSPI_SS);  //SCLK, MISO, MOSI, SS
#endif

  //set up slave select pins as outputs as the Arduino API
  //doesn't handle automatically pulling SS low
  pinMode(vspi->pinSS(), OUTPUT);  //VSPI SS
  pinMode(hspi->pinSS(), OUTPUT);  //HSPI SS
}

// the loop function runs over and over again until power down or reset
void loop() {
  //use the SPI buses
  spiCommand(vspi, 0b01010101);  // junk data to illustrate usage
  spiCommand(hspi, 0b11001100);
  delay(100);
}

void spiCommand(SPIClass *spi, byte data) {
  //use it as you would the regular arduino SPI API
  spi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
  digitalWrite(spi->pinSS(), LOW);  //pull SS slow to prep other end for transfer
  spi->transfer(data);
  digitalWrite(spi->pinSS(), HIGH);  //pull ss high to signify end of data transfer
  spi->endTransaction();
}
```
## Reference: 
https://randomnerdtutorials.com/esp32-spi-communication-arduino/

