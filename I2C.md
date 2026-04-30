## I2C Communication Protocol

The ESP32 has two I2C bus interfaces that can serve as I2C master or slave.

I²C means **I**nter **I**ntegrated **C**ircuit (it’s pronounced I-squared-C), and it is a synchronous, multi-master, multi-slave communication protocol. You can connect :
- multiple slaves to one master: for example, your ESP32 reads from a BME280 sensor using I2C and writes the sensor readings in an I2C OLED display.
- multiple masters controlling the same slave: for example, two ESP32 boards writing data to the same I2C OLED display.

<img src="https://github.com/user-attachments/assets/36fe91e4-1227-4a22-9cd1-9e8760c9d596" alt="image" style="width: 75%; height: auto;" />

The ESP32 supports I2C communication through its two I2C bus interfaces that can serve as I2C master or slave, depending on the user’s configuration. Accordingly to the ESP32 datasheet, the I2C interfaces of the ESP32 supports:
- Standard mode (100 Kbit/s) 
- Fast mode (400 Kbit/s) 
- Up to 5 MHz, yet constrained by SDA pull-up strength 
- 7-bit/10-bit addressing mode 
- Dual addressing mode. Users can program command registers to control I²C interfaces, so that they have more flexibility

```
Mode          Frequency          Status with ESP32-S3
Standard      100 kHz            Very stable, long wires okay.
Fast Mode     400 kHz            Standard for most sensors.
Fast Mode+    1 MHz              High performance.
Ultra Fast    5 MHz              Ultra-high speed; requires very short wires.
```

## Overview

The I2C (Inter-Integrated Circuit) protocol is a synchronous, multi-master/multi-slave, two-wire serial bus used for short-distance communication between microcontrollers and peripherals. It uses only two lines—Serial Data (SDA) and Serial Clock (SCL)—to manage communication, typically supporting speeds of 100 kbps, 400 kbps, and up to 5 Mbps. 

## Key Features of I2C Protocol:
- **Two-Wire Interface:** SDA (data) and SCL (clock) lines, requiring pull-up resistors.
- **Addressing:** Each slave device has a unique address, allowing up to 1008 nodes on the same bus.
- **Master/Slave Relationship:** A master device initiates communication and controls the clock, while slave devices respond to address matching.
- **Data Validity:** Data on the SDA line must remain stable during the high period of the clock.
- **Acknowledgement (ACK/NACK):** Each byte sent is followed by an acknowledgement bit from the receiver.
- **Common Applications:** Interfacing sensors, displays, and EEPROM with microcontrollers. 

## How I2C Communication Works:
- **Start Condition:** The master pulls the SDA line low while SCL is high.
- **Address Frame:** The master sends the 7-bit or 10-bit address of the target slave, along with a Read/Write bit.
- **ACK/NACK Bit:** The slave with the matching address pulls the SDA line low to signal acknowledgement.
- **Data Transfer:** Data is transferred in 8-bit bytes, with each byte acknowledged.
- **Stop Condition:** The master releases the lines to signal the end of the communication. 

<img alt="image" style="width: 75%; height: auto;" src="https://github.com/user-attachments/assets/569360c1-eb5a-40a0-a0f3-5d4f3ce48f9b" />


I2C is valued for its simplicity and efficiency in connecting multiple devices with minimal wiring. However, it is a half-duplex, relatively slower protocol compared to SPI. 


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

## Scan I2C Address with ESP32
With I2C communication, each slave on the bus has its own address, a hexadecimal number that allows the ESP32 to communicate with each device.

The I2C address can be usually found on the component’s datasheet. However, if it is difficult to find out, you may need to run an I2C scanner sketch to find out the I2C address.

```
/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com  
*********/

#include <Wire.h>
 
void setup() {
  Wire.begin();
  Serial.begin(115200);
  Serial.println("\nI2C Scanner");
}
 
void loop() {
  byte error, address;
  int nDevices;
  Serial.println("Scanning...");
  nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
      nDevices++;
    }
    else if (error==4) {
      Serial.print("Unknow error at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  }
  else {
    Serial.println("done\n");
  }
  delay(5000);          
}
```

## Multiple I2C devices (same bus, different addresses)
When we have multiple devices with different addresses, it is trivial how to set them up:
- connect both peripherals to the ESP32 SCL and SDA lines;
- in the code, refer to each peripheral by its address;

Example: The OLED and the BME280 have different addresses, we can use the same SDA and SCL lines without any problem. The OLED display address is 0x3C and the BME280 address is 0x76.


<img alt="image" style="width: 50%; height: auto;"  src="https://github.com/user-attachments/assets/963cf283-f44c-4629-90f7-6306649d322f" />


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

## I2C Communication Between Two ESP32
Start by connecting the two ESP32 boards with each other. Use the default I2C pins for the boards you’re using. Don’t forget to connect the GND pins together.


<img alt="image" style="width: 50%; height: auto;" src="https://github.com/user-attachments/assets/3758ad53-f2c3-4404-a8bd-57f993075ca7" />
  
```
I2C Signal     ESP32 Master	 ESP32 Slave
SDA (Data)     GPIO 8        GPIO 8        
SCL (Clock)    GPIO 9        GPIO 9
Gnd            Gnd           Gnd
```

Here’s how I2C communication between two ESP32 boards works:

<img src="https://github.com/user-attachments/assets/6a64a712-342f-4e68-88e3-ea2e65add45c" alt="image" style="width: 75%; height: auto;"/>




**Master Code**

The Master initiates all communication. It "pushes" data to the slave and "pulls" data when needed.

```
#include <Wire.h>
#define I2C_SDA 8
#define I2C_SCL 9
#define SLAVE_ADDR 0x08
#define I2C_FREQ 5000000 // 5MHz standard speed

void setup() {
  Serial.begin(115200);
  // Initialize I2C as Master
  Wire.begin(I2C_SDA, I2C_SCL, I2C_FREQ); 

  Serial.println("Initializing I2C Pins...");
  // On ESP32-S3, it's safer to set the pins first, then the address
  bool success = Wire.begin(I2C_SDA, I2C_SCL, I2C_FREQ); 
  
  if (success) {
    Serial.println("I2C Hardware Initialized.");
  } else {
    Serial.println("I2C Hardware Initialization FAILED.");
    return;
  }

  Serial.println("I2C Master Initialized");
}

void loop() {
  // Example 1: Send Data
  byte dataToSend[] = {0x10, 0x20, 0x30, 0x40};
  sendDataToSlave(dataToSend, sizeof(dataToSend));
  
  delay(2000);

  // Example 2: Receive Data
  int requestLen = 4;
  byte* received = receiveDataFromSlave(requestLen);
  
  if (received != nullptr) {
    Serial.print("Master Received: ");
    for (int i = 0; i < requestLen; i++) {
      Serial.print(received[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  }
  
  delay(2000);
}

// Function to send a byte array to Slave
void sendDataToSlave(byte data[], int length) {
  Wire.beginTransmission(SLAVE_ADDR);
  Wire.write(data, length);
  byte status = Wire.endTransmission();
  
  if (status == 0) Serial.println("Send Success");
  else Serial.println("Send Failed");
}

// Function to return data received from Slave
byte* receiveDataFromSlave(int length) {
  static byte buffer[32]; // Buffer to hold incoming bytes
  Wire.requestFrom(SLAVE_ADDR, length);
  
  int i = 0;
  while (Wire.available() && i < length) {
    buffer[i] = Wire.read();
    i++;
  }
  
  return (i > 0) ? buffer : nullptr;
}


```
**Slave Code**

The Slave is passive. It waits for the Master to trigger either the onReceive event (receiving data) or the onRequest event (sending data).

```
#include <Wire.h>
#define I2C_SDA 8
#define I2C_SCL 9
#define SLAVE_ADDR 0x08
#define I2C_FREQ 5000000 // 5MHz standard speed

byte responseData[] = {0xAA, 0xBB, 0xCC, 0xDD}; // Data to send when Master asks

void setup() {
  Serial.begin(115200);
  // Initialize I2C as Slave
  Serial.println("Initializing I2C Pins...");
  // On ESP32-S3, it's safer to set the pins first, then the address
  bool success = Wire.begin(SLAVE_ADDR, I2C_SDA, I2C_SCL, I2C_FREQ); 
  
  if (success) {
    Serial.println("I2C Hardware Initialized.");
  } else {
    Serial.println("I2C Hardware Initialization FAILED.");
    return;
  }

  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
  
  Serial.println("I2C Slave Ready and Listening.");
}

void loop() {
  delay(100); // Slave loop remains empty or handles local tasks
}

// Called when Master sends data to Slave
void receiveEvent(int howMany) {
  Serial.print("Slave Received: ");
  while (Wire.available()) {
    byte b = Wire.read();
    Serial.print(b, HEX);
    Serial.print(" ");
  }
  Serial.println();
}

// Called when Master requests data from Slave
void requestEvent() {
  // Master determines how many bytes it wants; Slave sends its buffer
  Wire.write(responseData, sizeof(responseData));
  Serial.println("Slave sent data to Master");
}

```

**Key Technical Considerations**

Buffer Sizes: The standard Wire library has a buffer limit (usually 32 bytes). If you need to send larger data arrays, you must break them into chunks.

Clock Stretching: ESP32-S3 supports clock stretching. If your Slave is busy processing when the Master requests data, the Slave will hold the SCL line low to tell the Master to wait.

Static Buffers: In the Master code, the buffer is declared as static. This ensures the pointer returned to the loop() remains valid after the function scope ends.

Error Handling: Wire.endTransmission() returns a status code.
- 0: Success.
- 2: Address NACK (Slave not found/wrong address).
- 4: Other error (usually electrical noise or missing pull-ups).

## Reference:

https://randomnerdtutorials.com/esp32-i2c-communication-arduino-ide/

https://www.circuitbasics.com/basics-of-the-i2c-communication-protocol/

https://www.oceanlabz.in/lesson-07-i2c/

https://www.luisllamas.es/en/esp32-i2c/
