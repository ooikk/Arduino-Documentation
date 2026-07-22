# Capacitive Touch Sensor Pins

The ESP32 S3 has 14 capacitive touch GPIOs. These GPIOs can sense variations in anything that holds an electrical charge, like the human skin. So they can detect variations induced when touching the GPIOs with a finger.      

Important note for ESP32-S3: Unlike the original ESP32 (where touch readings decrease when touched), the ESP32-S3 touch sensor readings INCREASE when touched. Keep this in mind when determining your threshold!       


## ESP32 S3 Touch Pins
```
Touch1     GPIO1
Touch2     GPIO2
Touch3     GPIO3
Touch4     GPIO4
Touch5     GPIO5
Touch6     GPIO6
Touch7     GPIO7
Touch8     GPIO8
Touch9     GPIO9
Touch10    GPIO10
Touch11    GPIO11
Touch12    GPIO12
Touch13    GPIO13
Touch14    GPIO14
```
## Detection


You can detect touch input using just a wire or a piece of foil — no external module needed. You can use the following command to read the touch pin capacitance. 

```
touchVal = touchRead(GPIO);
if (touchVal > THESHOLD) {
// touch detected
}
```

## Touch API     
The Touch Sensor API in the Arduino ESP32 core (esp32-hal-touch.h) provides functions to measure capacitance on touch-capable GPIO pins and trigger interrupts or wake-ups.     

**1. Primary Touch Sensing APIs**     
```touchRead()```: Reads the current raw capacitive value of a touch pin.
```
touch_value_t touchRead(uint8_t pin);
```     
- pin: Touch-capable GPIO pin number (e.g., TOUCH4 / GPIO 4).
- Returns: A numeric value representing capacitance. Use this in ```setup()``` via ```Serial.println(touchRead(TOUCH_PIN))``` to observe baseline vs. touched values before setting an interrupt threshold.     

```touchAttachInterrupt()```: Attaches a basic Interrupt Service Routine (ISR) callback that fires when touch capacitance passes the threshold.
```
void touchAttachInterrupt(uint8_t pin, void (*userFunc)(void), touch_value_t threshold);
```
- pin: Touch pin.
- userFunc: Pointer to your ISR callback function.
- threshold: Trigger value. On ESP32-S3, the interrupt fires when the pin reading exceeds this number.

```touchAttachInterruptArg()```: Attaches an ISR callback with custom parameters passed to the handler function.
```
void touchAttachInterruptArg(uint8_t pin, void (*userFunc)(void *), void *arg, touch_value_t threshold);
```
- userFunc: Function signature must accept a void* argument: ```void myISR(void *arg)```
- arg: Pointer to data (variable, struct, or class object) passed directly to the ISR.     

```touchDetachInterrupt()```: Disables touch interrupt capabilities on the designated pin.
```
void touchDetachInterrupt(uint8_t pin);
```      
**2. Status & Configuration Functions**      
```touchInterruptGetLastStatus()``` *(ESP32-S2 & ESP32-S3 specific)* :Checks if the touch pin is currently actively touched.      
```
bool touchInterruptGetLastStatus(uint8_t pin);
```
- Returns: true if the touch pad is actively pressed/touched, false if untouched/released.
- Use Case: Perfect for inside an ISR or background task to distinguish between a Touch Press and a Touch Release event.

```touchSetCycles()```: Adjusts the hardware measurement cycle duration and sleep periods.
```
void touchSetCycles(uint16_t measure, uint16_t sleep);
```    
- measure: Duration of the measurement phase (default: 0x1000).
- sleep: Idle period between measurements (default: 0x1000).
- Note: Tweaking these can reduce power consumption or improve filtering noise at the cost of execution speed.       

```touchSleepWakeUpEnable()```: Configures a touch pin as a wake-up trigger source to exit ESP32-S3 Deep Sleep.
```
void touchSleepWakeUpEnable(uint8_t pin, touch_value_t threshold);
```
- pin: Touch pin designated as wake-up source.
- threshold: Capacitance threshold that triggers a wake-up event.     


## Reference    

https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/touch.html

https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-touch.h
