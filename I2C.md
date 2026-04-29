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

<img src="https://github.com/user-attachments/assets/5d620f55-70f3-4ae4-a035-6ea24f924a64"  alt="image" style="width: 50%; height: auto;"/>

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

<img alt="image" style="width: 50%; height: auto;" src="https://github.com/user-attachments/assets/1df29de9-abf8-4dbd-ab89-4ca45489589a" />

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

void setup() {
  Serial.begin(115200);
  // Initialize I2C as Master
  Wire.begin(I2C_SDA, I2C_SCL); 
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

byte responseData[] = {0xAA, 0xBB, 0xCC, 0xDD}; // Data to send when Master asks

void setup() {
  Serial.begin(115200);
  // Initialize I2C as Slave
  Wire.begin(SLAVE_ADDR, I2C_SDA, I2C_SCL);
  
  // Register callback functions
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
  
  Serial.println("I2C Slave Ready");
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
