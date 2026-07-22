# Sleep Mode    

The ESP32-S3 offers five primary power modes: **Active**, **Modem-Sleep**, **Light Sleep**, **Deep Sleep** and **Hibernation**. When writing code in the Arduino IDE, you will most frequently work with **Light Sleep** and **Deep Sleep**, as these give you direct control over drastic power reductions.     

Here is a breakdown of how each sleep mode behaves, what stays powered on, and the exact Arduino IDE code to implement them.     

**Overview of Sleep Modes**    

|	Sleep Mode	     |	CPU Status	     |	Wi-Fi / Bluetooth	     |	RTC Memory	     |	Power Consumption (Approx.)	     |	Typical Use Case	     |
|	-	     |	-	     |	-	     |	-	     |	-	     |	-	     |
|	Active	     |	Running	     |	On / Ready	     |	On	     |	~80 mA – 240 mA	     |	Processing data, transmitting Wi-Fi	     |
|	Modem-Sleep	     |	Running	     |	Off	     |	On	     |	~20 mA – 40 mA	     |	Local sensor processing without wireless transmission	     |
|	Light Sleep	     |	Paused	     |	Off	     |	On	     |	~240 µA	     |	Temporary pauses, waiting for GPIO/timer interrupts	     |
|	Deep Sleep	     |	Off	     |	Off	     |	On (LP Memory)	     |	~7–10 µA	     |	Battery-powered sensors waking periodically	     |
|	Hibernation 	     |	Off	     |	Off	     |	OFF (all RTC variables are wiped)	     |	~2.5–5 µA	     |	For pure "deploy-and-forget" field devices.	     |

## 1. Modem-Sleep Mode     

This is an automatic mode managed by the Wi-Fi/Bluetooth stack. When the ESP32-S3 is connected to a Wi-Fi Access Point (AP), the modem is periodically turned off between Wi-Fi beacon frames (based on the AP's DTIM interval).     
- Behavior: The CPU continues executing code at full speed, but power drops significantly compared to active wireless transmission.
- Execution: Your loop continues running continuously.
- Arduino Implementation: You trigger this mode automatically by disconnecting/stopping Wi-Fi and Bluetooth.    
- Power Consumption: Moderate (approx. 20 mA - 30 mA).
- Best For: Devices that need to stay connected to Wi-Fi but don't need to transmit data continuously. You do not need to write code to enter this mode; it happens automatically when Wi-Fi is connected.      

Arduino Code Example:     
```
#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  
  // Turn off Wi-Fi and Bluetooth to enter Modem-Sleep
  WiFi.mode(WIFI_OFF);
  btStop();
  
  Serial.println("Modem-Sleep enabled (Wi-Fi and BT off). CPU still running.");
}

void loop() {
  // Do local processing here at lower power
  Serial.println("Working in Modem-Sleep...");
  delay(5000);
}
```
## 2. Light Sleep Mode     

In Light Sleep, the CPU clock is gated (paused), digital peripherals are paused, and Wi-Fi/Bluetooth are powered down. However, the internal RAM, RTC fast memory, RTC slow memory, and RTC peripherals remains powered, preserving the entire state of your variables and program.         
- Behavior: Code stops executing at esp_light_sleep_start(). When a wake trigger occurs, execution resumes immediately on the very next line of code.
- Wake Sources: Timer, GPIOs, UART, touch sensors.
- Wake-up Time: Extremely fast (microseconds). The CPU resumes execution exactly where it left off.
- Power Consumption: Low (approx. 0.8 mA), but not as low as Deep Sleep.
- Best For: Short pauses where you need to maintain the current state of the application, keep Wi-Fi/Bluetooth connections alive, or quickly respond to a sensor interrupt without a full reboot.       

Light Sleep example (Timer Wakeup), the code pauses at the sleep function and resumes on the very next line when it wakes up.     
```
#include "esp_sleep.h"

#define uS_TO_S_FACTOR 1000000ULL  // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  3           // Time ESP32 will go to sleep (in seconds)

void setup() {
  Serial.begin(115200);
  delay(1000); // Wait for serial monitor to connect
  Serial.println("ESP32-S3 Light Sleep Example");
}

void loop() {
  Serial.println("Running main loop...");
  Serial.flush(); // Ensure serial message finishes before pausing clock

  Serial.println("Configuring timer wake up for 3 seconds...");
  // Enable timer wake up
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  Serial.println("Going to light sleep now...");
  delay(100); // Allow serial print to finish
  
  // Enter light sleep. Execution will PAUSE here.
  esp_light_sleep_start(); 
  
  // --- WAKE UP HAPPENS HERE ---
  // Execution resumes exactly on this line after 3 seconds.
  Serial.println("Woke up from Light Sleep! Resuming loop.");
  Serial.println("-------------------------");
}
```
## 3. Deep Sleep Mode     

Deep Sleep provides the lowest possible power consumption. The CPU, main RAM, and most digital peripherals are powered off completely. Only the RTC (Real-Time Clock) controller and LP (Low-Power) memory stay powered on.      
- Behavior: The state of your standard global variables is lost. When the ESP32-S3 wakes up from Deep Sleep, it performs a full reboot (it runs setup() from the beginning).
- Retaining Data: You can preserve specific variables across reboots by marking them with the RTC_DATA_ATTR modifier.
- Wake-up Time: Slower (milliseconds). The chip undergoes a "soft reset" upon waking, meaning it runs through the bootloader and the setup() function again.
- Power Consumption: Very low (approx. 10 µA to 50 µA, depending on enabled RTC peripherals).
- Best For: Battery-powered devices that sleep for seconds, minutes, or hours. You must use the RTC_DATA_ATTR macro to store variables in RTC memory so they survive the deep sleep reset.      

**Wake Up Sources for Deep Sleep**:      
- Timer: Wakes up after a defined period.
- EXT0 / EXT1 (GPIO): Wakes up when specific RTC-capable pins change state.
- Touch Pins: Wakes up on capacitive touch detection.      

**Deep Sleep Example A: Timer Wakeup + RTC Memory**     
```
#include <esp_sleep.h>

#define TIME_TO_SLEEP_SEC  10  // Sleep duration in seconds

// Variable stored in RTC memory — persists through Deep Sleep restarts
RTC_DATA_ATTR int bootCount = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Increment and display boot count
  bootCount++;
  Serial.printf("Boot number: %d\n", bootCount);

  // Print wake-up reason
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
    Serial.println("Woke up due to timer interrupt.");
  } else {
    Serial.println("Woke up from initial power-on or reset.");
  }

  // Setup timer wakeup trigger
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP_SEC * 1000000ULL);

  Serial.println("Entering Deep Sleep now...");
  Serial.flush();

  // Start Deep Sleep (will reboot when timer expires)
  esp_deep_sleep_start();
}

void loop() {
  // This code will never be reached in Deep Sleep
}
```
**Deep Sleep Example B: External Pin Wakeup (EXT0)**    
This example configures GPIO 0 (often connected to the onboard "BOOT" button on ESP32-S3 boards) to wake the system when pulled LOW.  
```
#include <esp_sleep.h>

#define WAKEUP_PIN GPIO_NUM_0  // Uses GPIO 0 (Boot button)

void setup() {
  Serial.begin(115200);
  delay(1000);

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  
  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println("Woke up by button press on GPIO 0!");
  } else {
    Serial.println("Power-on or standard reset.");
  }

  // Configure GPIO 0 as wakeup source when logic level is 0 (LOW)
  esp_sleep_enable_ext0_wakeup(WAKEUP_PIN, 0);

  Serial.println("Entering Deep Sleep. Press the BOOT button to wake up.");
  Serial.flush();

  esp_deep_sleep_start();
}

void loop() {
  // Never reached
}
```

 **Deep Sleep Example C: Timer & External Wake-up**    
In Deep Sleep, the chip resets upon waking. The setup() function runs again. We use esp_sleep_get_wakeup_cause() to figure out why it woke up.     
```
#include "esp_sleep.h"
#include "driver/rtc_io.h"

#define uS_TO_S_FACTOR 1000000ULL
#define TIME_TO_SLEEP  10          // Sleep for 10 seconds
#define WAKEUP_PIN     GPIO_NUM_2  // Must be an RTC-capable GPIO on S3 (e.g., GPIO 0-21)

// Variables stored in RTC memory survive Deep Sleep
RTC_DATA_ATTR int bootCount = 0; 

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Increment boot counter (survives deep sleep)
  bootCount++;
  Serial.println("Boot number: " + String(bootCount));

  // Check the wake-up cause
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  switch(wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0: 
      Serial.println("Wake up from external GPIO (Button)!"); 
      break;
    case ESP_SLEEP_WAKEUP_TIMER: 
      Serial.println("Wake up from Timer!"); 
      break;
    default: 
      Serial.println("First boot or wake up was not from deep sleep"); 
      break;
  }

  // 1. Configure Timer Wake-up
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  // 2. Configure External GPIO Wake-up (Active HIGH)
  // Note: On ESP32-S3, you MUST use rtc_gpio functions for deep sleep wakeups
  rtc_gpio_init(WAKEUP_PIN);
  rtc_gpio_set_direction(WAKEUP_PIN, RTC_GPIO_MODE_INPUT_ONLY);
  rtc_gpio_pulldown_en(WAKEUP_PIN); // Pull down, wake up when pin goes HIGH
  esp_sleep_enable_ext0_wakeup(WAKEUP_PIN, HIGH); 

  Serial.println("Going to Deep Sleep now. Will wake in 10s or if GPIO 2 is HIGH.");
  delay(100);
  
  // Enter deep sleep. The chip will essentially "turn off" and reboot when waking.
  esp_deep_sleep_start(); 
}

void loop() {
  // This function will never be reached because the chip resets on wake-up
  // and starts from setup() again.
}
```

## 4. Hibernation     

Hibernation is a specialized subset of Deep Sleep. In this mode, even the RTC peripherals are powered down. Only the RTC slow memory is kept alive.     
- Power Consumption: Ultra-low (approx. 5 µA).
- Best For: Extreme battery-saving scenarios where you only need to retain a tiny amount of state (like a boot counter) and rely entirely on external RTC chips for timekeeping.     

**Arduino Code Example: Hibernation with Timer Wakeup**      
To enter Hibernation mode on the ESP32-S3, you explicitly tell the power domain manager (```esp_sleep_pd_config```) to turn off the RTC memory and peripheral domains before ```calling esp_deep_sleep_start()```.     
```
#include <esp_sleep.h>

#define TIME_TO_SLEEP_SEC 10 // Sleep for 10 seconds

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("ESP32-S3 Woke Up!");

  // Check wake reason
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
    Serial.println("Woke up from Timer interrupt.");
  } else {
    Serial.println("Normal reset or initial power-on.");
  }

  // 1. Configure the wake trigger (Timer)
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP_SEC * 1000000ULL);

  // 2. Power down RTC domains to force Hibernation state
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);

  Serial.println("Entering Hibernation Mode (~2.5 uA)...");
  Serial.flush(); // Flush serial transmission before powering down

  // 3. Start Deep Sleep (configured to hibernate)
  esp_deep_sleep_start();
}

void loop() {
  // Never reached
}
```
**Practical Hardware Tip for Hibernation**      
While the ESP32-S3 chip itself drops down to ~2.5 µA in Hibernation, your development board (e.g., ESP32-S3 DevKit, SuperMini) might still pull hundreds of microamps or a few milliamps.      
This extra power draw is usually caused by:
- Power LEDs: A single active power LED can pull 1–3 mA.
- LDO Voltage Regulator: Standard regulators (like AMS1117) have a high quiescent current (~5 mA).
- Onboard USB-to-UART Chips: Chips like the CP2102 or CH340 draw current even when idle.       

## ⚠️ Crucial ESP32-S3 Specific Considerations     
1. Native USB (USB Serial/JTAG): The ESP32-S3 features a native USB interface. When waking up from Deep Sleep, the USB connection will drop and reconnect (acting like a standard reset). If your application relies on continuous USB serial communication, Deep Sleep will interrupt it. Light sleep does not drop the USB connection.
3. RTC GPIOs: Not all GPIOs on the ESP32-S3 can wake it up from Deep Sleep. You must use RTC-capable GPIOs (typically GPIO 0 through GPIO 21). If you try to use a standard digital GPIO (like GPIO 26) for ext0 deep sleep wake-up, it will fail. Always check the ESP32-S3 pinout datasheet for "RTC" labels.
5. PSRAM and Flash: In Deep Sleep, the main flash and PSRAM are powered down. If you are using PSRAM, ensure you aren't trying to access it immediately upon waking before the system has fully re-initialized.
7. Hibernation Code Snippet: If you want to enter Hibernation mode instead of standard Deep Sleep, add this right before ```esp_deep_sleep_start()```:
   ```
   // Turn off RTC peripherals to achieve Hibernation
   esp_deep_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
   esp_deep_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF); // Optional: if you don't need RTC slow mem
   esp_deep_sleep_start();
   ```
