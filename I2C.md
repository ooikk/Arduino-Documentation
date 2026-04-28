## I2C Communication Protocol

The ESP32 has two I2C bus interfaces that can serve as I2C master or slave.

I²C means **I**nter **I**ntegrated **C**ircuit (it’s pronounced I-squared-C), and it is a synchronous, multi-master, multi-slave communication protocol. You can connect :
- multiple slaves to one master: for example, your ESP32 reads from a BME280 sensor using I2C and writes the sensor readings in an I2C OLED display.
- multiple masters controlling the same slave: for example, two ESP32 boards writing data to the same I2C OLED display.

<img width="1008" height="595" alt="image" src="https://github.com/user-attachments/assets/36fe91e4-1227-4a22-9cd1-9e8760c9d596" />

The ESP32 supports I2C communication through its two I2C bus interfaces that can serve as I2C master or slave, depending on the user’s configuration. Accordingly to the ESP32 datasheet, the I2C interfaces of the ESP32 supports:
- Standard mode (100 Kbit/s) 
- Fast mode (400 Kbit/s) 
- Up to 5 MHz, yet constrained by SDA pull-up strength 
- 7-bit/10-bit addressing mode 
- Dual addressing mode. Users can program command registers to control I²C interfaces, so that they have more flexibility

## Connecting I2C Devices with ESP32
I2C communication protocol uses two wires to share information. One is used for the clock signal (SCL) and the other is used to send and receive data (SDA). The SDA and SCL lines are active low, so they should be pulled up with resistors. Typical values are 4.7k Ohm for 5V devices and 2.4k Ohm for 3.3V devices.
```
I2C Device	ESP32S3
SDA (Data)  GPIO 8        
SCL (Clock) GPIO 9
Gnd         Gnd
Vcc         3.3V
```

## Customizing I2C Pins
With the ESP32 you can set almost any pin to have I2C capabilities, you just need to set that in your code. You can reassign them in your setup() function like this:
```
// Example: Using GPIO 1 and GPIO 2 for I2C
#define I2C_SDA 1
#define I2C_SCL 2

void setup() {
  // Wire.begin(SDA, SCL);
  // Assign to new
  Wire.begin(I2C_SDA, I2C_SCL);
}
```

## Multiple I2C devices (same bus, different addresses)
When we have multiple devices with different addresses, it is trivial how to set them up:
- connect both peripherals to the ESP32 SCL and SDA lines;
- in the code, refer to each peripheral by its address;

Example: The OLED and the BME280 have different addresses, we can use the same SDA and SCL lines without any problem. The OLED display address is 0x3C and the BME280 address is 0x76.

<img width="476" height="564" alt="image" src="https://github.com/user-attachments/assets/5d620f55-70f3-4ae4-a035-6ea24f924a64" />

```
if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
  Serial.println(F("SSD1306 allocation failed"));
  for(;;);
}
  
bool status = bme.begin(0x76);  
if (!status) {
  Serial.println("Could not find a valid BME280 sensor, check wiring!");
  while (1);
}
```


## Multiple I2C Buses
The ESP32-S3 actually has two independent I2C peripherals (Wire and Wire1). This is very useful if you have two devices with the same address or if you want to run one sensor at 100kHz and another at 400kHz.
```
// TwoWire I2C_one = TwoWire(0); // Create first I2C bus
TwoWire I2C_two = TwoWire(1); // Create a second I2C bus

void setup() {
  Wire.begin(8, 9);           // uses default SDA and SCL and 100000HZ freq
  I2C_two.begin(10, 11, 100000);      // Second bus on GPIO 10 and 11
}
```
Example, one of the sensors uses the default pins, and the other uses GPIO 10 and GPIO 11.
```
/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-i2c-communication-arduino-ide/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define SDA_2 11
#define SCL_2 10

Adafruit_BME280 bme1;
Adafruit_BME280 bme2;

void setup() {
  Serial.begin(115200);
  Serial.println(F("BME280 test"));

  Wire.begin();
  Wire1.begin(SDA_2, SCL_2);

  bool status1 = bme1.begin(0x76);  
  if (!status1) {
    Serial.println("Could not find a valid BME280_1 sensor, check wiring!");
    while (1);
  }
  
  bool status2 = bme2.begin(0x76, &Wire1);  
  if (!status2) {
    Serial.println("Could not find a valid BME280_2 sensor, check wiring!");
    while (1);
  }
 
  Serial.println();
}

void loop() { 
  // Read from bme1
  Serial.print("Temperature from BME1= ");
  Serial.print(bme1.readTemperature());
  Serial.println(" *C");

  Serial.print("Humidity from BME1 = ");
  Serial.print(bme1.readHumidity());
  Serial.println(" %");
  
  Serial.print("Pressure from BME1 = ");
  Serial.print(bme1.readPressure() / 100.0F);
  Serial.println(" hPa");

  Serial.println("--------------------");
  
  // Read from bme2
  Serial.print("Temperature from BME2 = ");
  Serial.print(bme2.readTemperature());
  Serial.println(" *C");

  Serial.print("Humidity from BME2 = ");
  Serial.print(bme2.readHumidity());
  Serial.println(" %");
  
  Serial.print("Pressure from BME2 = ");
  Serial.print(bme2.readPressure() / 100.0F);
  Serial.println(" hPa");

  Serial.println("--------------------");
  delay(5000);
}
```


## Reference:

https://randomnerdtutorials.com/esp32-i2c-communication-arduino-ide/
