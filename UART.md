## UART Communication Protocol

**Overview**

UART (Universal Asynchronous Receiver-Transmitter) is a widely used, 2-wire (Tx/Rx) asynchronous serial communication protocol that transfers data between devices without a shared clock signal. It converts parallel data from a CPU into serial form for transmission and back to parallel at the receiving end. Both devices must be configured to the same speed (baud rate), with data framed by start/stop bits. Unlike SPI or I2C, which are synchronous, UART is asynchronous, meaning it does not use a clock signal to synchronize the data transmission between devices.

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


## Reference

https://randomnerdtutorials.com/esp32-uart-communication-serial-arduino/

https://www.circuitbasics.com/basics-uart-communication/
