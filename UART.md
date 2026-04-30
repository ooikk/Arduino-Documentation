## UART Communication Protocol

**Overview**

UART (Universal Asynchronous Receiver-Transmitter) is a widely used, 2-wire (Tx/Rx) asynchronous serial communication protocol that transfers data between devices without a shared clock signal. It converts parallel data from a CPU into serial form for transmission and back to parallel at the receiving end. Both devices must be configured to the same speed (baud rate), with data framed by start/stop bits. 

<img alt="image" style="width: 75%; height: auto;" src="https://github.com/user-attachments/assets/c9456370-4bc9-4fe2-aec5-404b62bd8983" />


**Key Aspects of UART Protocol**
- **Asynchronous Operation:** Unlike SPI or I2C, UART does not use a clock signal to synchronize the transmitter and receiver. Instead, it relies on pre-configured, matching baud rates (e.g., 9600, 115200).
- **Data Framing:** To manage asynchronous communication, UART wraps data in packets containing a start bit, 5–9 data bits, an optional parity bit (for error checking), and 1–2 stop bits.
- **Wiring:** It only requires two wires—Transmit (Tx) and Receive (Rx)—plus a common ground, making it simple to implement.
- **Communication Modes:** UART allows for full-duplex (simultaneous two-way) or half-duplex (alternating) communication.
- **Voltage Levels:** UART typically uses TTL (transistor-transistor logic) levels. 

**Data Transmission Process**
- **Idle:** The transmission line is held at a high voltage level.
- **Start Bit:** The transmitter pulls the line low for one clock cycle.
- **Data Bits:** The data is sent, starting with the least significant bit (LSB).
- **Parity Bit (Optional):** Used for error checking.
- **Stop Bit(s):** The line is pulled high again to signal the end of the frame. 

**Advantages and Disadvantages**
- **Pros:** Only two wires, no clock signal needed, widely documented, and versatile.
- **Cons:** Slower than SPI/I2C, limited to point-to-point communication (no master/slave networks), and requires precise, pre-matched baud rates. 

**Common Uses**
UART is extensively used in embedded systems for debugging (e.g., Arduino serial monitor), connecting GPS modules, Bluetooth modules, and, historically, teletypewriters. 

## UART Communication
UARTs transmit data asynchronously, which means there is no clock signal to synchronize the output of bits from the transmitting UART to the sampling of bits by the receiving UART. Instead of a clock signal, the transmitting UART adds start and stop bits to the data packet being transferred. These bits define the beginning and end of the data packet so the receiving UART knows when to start reading the bits.

When the receiving UART detects a start bit, it starts to read the incoming bits at a specific frequency known as the baud rate.

UART transmitted data is organized into packets. Each packet contains 1 start bit, 5 to 9 data bits (depending on the UART), an optional parity bit, and 1 or 2 stop bits:

<img alt="image" style="width: 50%; height: auto;"  src="https://github.com/user-attachments/assets/63411401-fcf2-4570-abb2-e3d263cb22b9" />


<img alt="image" style="width: 50%; height: auto;"  src="https://github.com/user-attachments/assets/64213327-43f1-4b64-b994-d350b43648ef" />

**Advantages**
- Only uses two wires
- No clock signal is necessary
- Has a parity bit to allow for error checking
- The structure of the data packet can be changed as long as both sides are set up for it
- Well documented and widely used method

**Disadvantages**
- The size of the data frame is limited to a maximum of 9 bits
- Doesn’t support multiple slave or multiple master systems
- The baud rates of each UART must be within 10% of each otherAdvantages

## ESP32S3 Default UART Pins

```
UART Port    TX (Transmit)   RX (Receive)   Common Use
UART0        GPIO 43         GPIO 44        Primary debug port/Serial Monitor.
UART1        GPIO 17         GPIO 18        "External peripherals (e.g., GPS, GSM)."
UART2        Unassigned      Unassigned     User-defined or completely free to map.
```
**UART0** is usually reserved for communication with the serial monitor during upload and debugging. However, you can also use it for communication with other devices after uploading the code if the Serial Monitor is not needed.

**UART1** and **UART2**: available to communicate with external devices.

## Native USB CDC
The ESP32-S3 has a built-in USB-Serial/JTAG controller. This is a game-changer for availability:

If you enable USB CDC on Boot in your IDE settings, the "Serial" port in your code runs through the internal USB hardware, not through the hardware UARTs.

The Result: This effectively leaves all three hardware UARTs (0, 1, and 2) completely free for your external sensors while you still enjoy a working Serial Monitor over the USB cable.

## UART0 and the Serial Monitor
Here’s a basic example that shows a two-way communication between the ESP32 and the Serial Monitor (your computer).

```
/*********
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete instructions at https://RandomNerdTutorials.com/esp32-uart-communication-serial-arduino/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/

String receivedMessage = "";  // Variable to store the complete message

void setup() {
  // Start the Serial Monitor at a baud rate of 115200
  Serial.begin(115200);
  
  // Print an initial message to the Serial Monitor
  Serial.println("ESP32 is ready. Please enter a message:");
}

void loop() {
  // Check if data is available in the Serial buffer
  while (Serial.available()) {
    char incomingChar = Serial.read();  // Read each character from the buffer
    
    if (incomingChar == '\n') {  // Check if the user pressed Enter (new line character)
      // Print the message
      Serial.print("You sent: ");
      Serial.println(receivedMessage);
      
      // Clear the message buffer for the next input
      receivedMessage = "";
    } else {
      // Append the character to the message string
      receivedMessage += incomingChar;
    }
  }
}
```

## ESP32 Setting Custom UART Pins
Setting custom UART pins is quite simple. As we’ve seen previously, the ESP32 has three UART ports you can use: UART0, UART 1 and UART 2.

To use any of those ports, you just need to create an HardwareSerial instance on the desired UART port. For example, the following line creates a serial instance called mySerial using UART2.
```
#define RX_GPIO 16
#define TX_GPIO 17
HardwareSerial mySerial(2);
```

Then, you can use any pins of your choice. You just need to pass them to the begin() method like this:
```
mySerial.begin(GPS_BAUD, SERIAL_8N1, RX_GPIO, TX_GPIO);
```
## UART Communication with Other Devices

```
/*********
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete instructions at https://RandomNerdTutorials.com/esp32-neo-6m-gps-module-arduino/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/

 // Define the RX and TX pins for Serial 2
#define RXD2 16
#define TXD2 17

#define GPS_BAUD 9600

// Create an instance of the HardwareSerial class for Serial 2
HardwareSerial gpsSerial(2);

void setup(){
  // Serial Monitor
  Serial.begin(115200);
  
  // Start Serial 2 with the defined RX and TX pins and a baud rate of 9600
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);
  Serial.println("Serial 2 started at 9600 baud rate");
}

void loop(){
  while (gpsSerial.available() > 0){
    // get the byte data from the GPS
    char gpsData = gpsSerial.read();
    Serial.print(gpsData);
  }
  delay(1000);
  Serial.println("-------------------------------");
}
```
**SERIAL_8N1** means 8 data bits, No parity, 1 stop bit, which is the most commonly used. You can check all the options here.
https://docs.arduino.cc/language-reference/en/functions/communication/serial/begin/

## ESP32 Serial Communication Between ESP
If you want to communicate with another ESP32, it is not much different. You basically initialize a serial communication on the desired GPIOs and then use the functions to read and send data via serial.
```
ESP32 #1 (Sender)    ESP32 #2 (Receiver)
TXD1 (GPIO 17)       RXD1 (GPIO 18)
RXD1 (GPIO 18)       TXD1 (GPIO 17)
GND                  GND
```
<img alt="image" style="width: 50%; height: auto;" src="https://github.com/user-attachments/assets/7d9178e6-603d-4629-a423-0bb5e7d7bc0b" />


**Sender Code**
```
/*********
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete instructions at https://RandomNerdTutorials.com/esp32-uart-communication-serial-arduino/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/

// Define TX and RX pins for UART (change if needed)
#define TXD1 17
#define RXD1 18

// Use Serial1 for UART communication
HardwareSerial mySerial(1);

int counter = 0;

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600, SERIAL_8N1, RXD1, TXD1);  // UART setup
  
  Serial.println("ESP32 UART Transmitter");
}

void loop() {
  
  // Send message over UART
  mySerial.println(String(counter));
  
  Serial.println("Sent: " + String(counter));
  
  // increment the counter
  counter++;
  
  delay(1000); 
}
```
**Receiver  Code**
```
/*********
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete instructions at https://RandomNerdTutorials.com/esp32-uart-communication-serial-arduino/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/

// Define TX and RX pins for UART (change if needed)
#define TXD1 18
#define RXD1 17

// Use Serial1 for UART communication
HardwareSerial mySerial(2);

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600, SERIAL_8N1, RXD1, TXD1);  // UART setup
  
  Serial.println("ESP32 UART Receiver");
}

void loop() {
  // Check if data is available to read
  if (mySerial.available()) {
    // Read data and display it
    String message = mySerial.readStringUntil('\n');
    Serial.println("Received: " + message);
  }
}
```

## Reference

https://randomnerdtutorials.com/esp32-uart-communication-serial-arduino/

https://www.circuitbasics.com/basics-uart-communication/

https://www.oceanlabz.in/lesson-06-serial-communication/

https://www.luisllamas.es/en/esp32-uart/
