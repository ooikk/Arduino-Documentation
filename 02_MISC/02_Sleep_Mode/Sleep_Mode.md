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

#define uS_TO_S_FACTOR 1000000ULL  // Conversion factor for micro seconds to seconds, ULL -> Unsigned Long Long, 64-bit unsigned integer
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

**Deep Sleep Example D: EXT1 Wake-up with Multiple Buttons**        
In this example, we will configure GPIO 2 and GPIO 3 as wake-up sources. The ESP32-S3 will wake up if ANY of the buttons are pressed (pulled HIGH).     
```
#include "esp_sleep.h"
#include "driver/rtc_io.h"

// Define the RTC-capable GPIO pins to use for wake-up
// On ESP32-S3, RTC GPIOs are typically GPIO 0 through GPIO 21
#define BUTTON_1_PIN GPIO_NUM_2
#define BUTTON_2_PIN GPIO_NUM_3

// Variable to keep track of boot count (survives deep sleep)
RTC_DATA_ATTR int bootCount = 0;

void setup() {
  Serial.begin(115200);
  delay(1000); // Give serial monitor time to connect

  bootCount++;
  Serial.printf("Boot number: %d\n", bootCount);

  // 1. Check the wake-up cause
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  
  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1) {
    Serial.println("Woke up from EXT1 GPIO Wake-up!");
  } else {
    Serial.println("First boot or woke up from another source.");
  }

  // ---------------------------------------------------------
  // 2. CONFIGURE THE WAKE_MASK
  // ---------------------------------------------------------
  // The wake_mask is a 64-bit integer. We use bitwise shift (1ULL << pin) 
  // to set the specific bit for each GPIO we want to use.
  // We combine them using the bitwise OR operator (|).
  
  uint64_t wake_mask = (1ULL << BUTTON_1_PIN) | (1ULL << BUTTON_2_PIN);
  
  Serial.printf("Wake mask calculated: 0x%llx\n", wake_mask);

  // ---------------------------------------------------------
  // 3. CONFIGURE RTC GPIOs (CRITICAL FOR ESP32-S3)
  // ---------------------------------------------------------
  // For Deep Sleep, standard pinMode() does NOT work. 
  // You MUST configure the pins using the rtc_gpio API.
  
  // Configure Button 1
  rtc_gpio_init(BUTTON_1_PIN);
  rtc_gpio_set_direction(BUTTON_1_PIN, RTC_GPIO_MODE_INPUT_ONLY);
  rtc_gpio_pulldown_en(BUTTON_1_PIN); // Pull down so it reads LOW until pressed
  rtc_gpio_pullup_dis(BUTTON_1_PIN);  // Disable pull-up

  // Configure Button 2
  rtc_gpio_init(BUTTON_2_PIN);
  rtc_gpio_set_direction(BUTTON_2_PIN, RTC_GPIO_MODE_INPUT_ONLY);
  rtc_gpio_pulldown_en(BUTTON_2_PIN); 
  rtc_gpio_pullup_dis(BUTTON_2_PIN);  

  // ---------------------------------------------------------
  // 4. ENABLE EXT1 WAKE-UP
  // ---------------------------------------------------------
  // Mode options:
  // ESP_EXT1_WAKEUP_ANY_HIGH: Wakes if ANY pin in the mask goes HIGH
  // ESP_EXT1_WAKEUP_ALL_LOW:  Wakes if ALL pins in the mask go LOW
  
  esp_err_t err = esp_sleep_enable_ext1_wakeup(wake_mask, ESP_EXT1_WAKEUP_ANY_HIGH);
  
  if (err == ESP_OK) {
    Serial.println("EXT1 Wake-up enabled successfully.");
  } else {
    Serial.println("Failed to enable EXT1 wake-up!");
  }

  Serial.println("Going to Deep Sleep... Press Button 1 OR Button 2 to wake up.");
  delay(100); // Allow serial to flush

  // ---------------------------------------------------------
  // 5. ENTER DEEP SLEEP
  // ---------------------------------------------------------
  esp_deep_sleep_start();
}

void loop() {
  // This code will never be reached because the chip resets 
  // and starts from setup() every time it wakes up from Deep Sleep.
}
```

**Breakdown of the Key Concepts**     
1. Understanding the wake_mask
   The wake_mask is a uint64_t (64-bit unsigned integer).
   - Bit 0 represents GPIO 0.
   - Bit 1 represents GPIO 1.
   - Bit 21 represents GPIO 21.
   
   To turn on the "wake-up" feature for GPIO 2, you shift the number 1 to the left by 2 positions: 1ULL << 2 (which equals binary 100, or decimal 4).     
   To include GPIO 3 as well, you shift 1 left by 3: 1ULL << 3 (binary 1000, decimal 8).
   You combine them using the bitwise OR | operator: 4 | 8 = 12 (binary 1100).
   Note: Always use 1ULL (Unsigned Long Long) instead of just 1. If you use 1 << 20, it might overflow a standard 32-bit integer and fail to set the higher bits correctly.      
2. Understanding the mode Parameter
   The second parameter dictates the logic for the mask:
   - ```ESP_EXT1_WAKEUP_ANY_HIGH```: The chip wakes up if at least one of the GPIOs in the mask goes HIGH. (Used in the example above; pressing either button wakes the chip).
   - ```ESP_EXT1_WAKEUP_ALL_LOW```: The chip wakes up only if all of the GPIOs in the mask go LOW simultaneously. (Useful for scenarios where you need a specific combination of switches to be closed to wake the device).     
3. The ESP32-S3 "Gotcha": RTC GPIO Initialization
   This is the #1 reason EXT1 wake-up fails on the ESP32-S3.     
   When the chip goes into Deep Sleep, the main digital GPIO matrix is powered off. The wake-up logic is handled by the separate, ultra-low-power RTC controller.    
   Therefore, you cannot use ```pinMode(pin, INPUT)``` or ```digitalRead()```. You must explicitly tell the RTC controller to take over the pin using:
   - ```rtc_gpio_init()```
   - ```rtc_gpio_set_direction()```
   - ```rtc_gpio_pulldown_en()``` or ```rtc_gpio_pullup_en()``` (Crucial: if you don't enable internal pull resistors, the pins will "float" and cause the ESP32 to wake up randomly or draw massive amounts of current while sleeping).       


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
## Sleep mode API     

**1. Entering Sleep Modes**     
- ```void esp_deep_sleep_start(void)```
  - Enters Deep Sleep (or Hibernation, if power domains were disabled). The main CPU, digital peripherals, and RAM are powered off. Waking up causes a system reboot.
- ```esp_err_t esp_light_sleep_start(void)```
  - Enters Light Sleep. CPU clock is gated, but RAM remains intact. Execution resumes on the next line of code upon wakeup.
- ```void esp_deep_sleep(uint64_t time_in_us)```
  Helper function that configures a timer wakeup source and enters Deep Sleep in a single call.     

**2. Wakeup Source Configuration**      

**Timer**     
- ```esp_err_t esp_sleep_enable_timer_wakeup(uint64_t time_in_us)```
  -Sets an RTC timer to wake the ESP32-S3 after time_in_us microseconds. Works in both Light and Deep Sleep.
  

**External GPIO Pins**      
- ```esp_err_t esp_sleep_enable_ext0_wakeup(gpio_num_t gpio_num, int level)```
  - Configures a single RTC GPIO pin (gpio_num) to wake the chip when it reaches a specific logic level (0 for LOW, 1 for HIGH). Works in Deep and Light Sleep.
- ```esp_err_t esp_sleep_enable_ext1_wakeup(uint64_t wake_mask, int mode)```
  - Uses a bitmask to configure multiple RTC GPIO pins. mode can be ESP_EXT1_WAKEUP_ANY_HIGH or ESP_EXT1_WAKEUP_ALL_LOW. Works in Deep and Light Sleep. See "Deep Sleep Example D" above.
- ```esp_err_t esp_sleep_enable_gpio_wakeup(void)```
  - Enables GPIO wakeup for Light Sleep only. Allows non-RTC GPIOs to wake the chip while standard digital peripherals are paused.	
- ```gpio_wakeup_enable(gpio_num_t gpio_num, gpio_int_type_t intr_type)```
  - Configures a specific GPIO pin for Light Sleep wakeup (e.g., GPIO_INTR_LOW_LEVEL or GPIO_INTR_HIGH_LEVEL).	  

**Touch & Peripherals**        
- ```esp_err_t esp_sleep_enable_touchpad_wakeup(void)```
  - Enables capacitive touch sensors as a wakeup trigger. Works in Deep and Light Sleep (Note: disabled in Hibernation).	  
- ```esp_err_t esp_sleep_enable_ulp_wakeup(void)```
  - Configures the ULP (Ultra-Low-Power) co-processor to wake the main CPUs. Works in Deep and Light Sleep.   
- ```esp_err_t esp_sleep_enable_uart_wakeup(int uart_num)```
  - Configures a UART interface (e.g., UART0 or UART1) to wake the chip when incoming serial data is received. Works in Light Sleep only.	   

**3. Disabling Wakeup Sources**     
If you need to clear previously set wakeup triggers before entering sleep:      

- ```esp_sleep_disable_wakeup_source(esp_sleep_source_t source)```
  - Disables a specific wakeup source (e.g., ESP_SLEEP_WAKEUP_TIMER, ESP_SLEEP_WAKEUP_EXT0).	 
- ```gpio_wakeup_disable(gpio_num_t gpio_num)```
  - Disables Light Sleep GPIO wakeup on a specific pin.	  

**4. Querying Wakeup Status**      
These APIs help you determine why the board woke up and which specific pin or pad triggered the wake event.    

- ```esp_sleep_wakeup_cause_t esp_sleep_get_wakeup_cause(void)```
  - Returns esp_sleep_wakeup_cause_t indicating the wake trigger (e.g., ESP_SLEEP_WAKEUP_TIMER, ESP_SLEEP_WAKEUP_EXT0, ESP_SLEEP_WAKEUP_TOUCHPAD).	 
- ```uint64_t esp_sleep_get_ext1_wakeup_status(void)```
  - Returns a uint64_t bitmask indicating which specific GPIO(s) triggered an EXT1 wakeup.	 
- ```int esp_sleep_get_touchpad_wakeup_status(void)```
  - Returns touch_pad_t indicating which capacitive touch pad triggered the wakeup.	 

**Wakeup Cause Enum Reference (esp_sleep_wakeup_cause_t)**     
- ```ESP_SLEEP_WAKEUP_UNDEFINED```: Not caused by sleep (e.g., standard cold boot / reset button).
- ```ESP_SLEEP_WAKEUP_EXT0```: External signal using RTC_IO (single pin).
- ```ESP_SLEEP_WAKEUP_EXT1```: External signal using RTC_CNTL (multiple pins).
- ```ESP_SLEEP_WAKEUP_TIMER```: RTC timer expired.
- ```ESP_SLEEP_WAKEUP_TOUCHPAD```: Touch sensor triggered.
- ```ESP_SLEEP_WAKEUP_ULP```: ULP co-processor triggered.
- ```ESP_SLEEP_WAKEUP_GPIO```: Light Sleep GPIO trigger.
- ```ESP_SLEEP_WAKEUP_UART```: Light Sleep UART incoming data trigger.

**5. Power Domain & Hibernation Management**      
Use these APIs to selectively power off internal domains to tune power consumption down to Hibernation levels (~2.5 µA).      

- ```esp_err_t esp_sleep_pd_config(esp_sleep_pd_domain_t domain, esp_sleep_pd_option_t option)```
  - Configures whether a power domain stays powered during sleep.	  

**Power Domains (esp_sleep_pd_domain_t)**      
- ```ESP_PD_DOMAIN_RTC_PERIPH```: RTC I/O, touch, and low-power peripherals.
- ```ESP_PD_DOMAIN_RTC_SLOW_MEM```: RTC SLOW memory (used for ULP or RTC_DATA_ATTR).
- ```ESP_PD_DOMAIN_RTC_FAST_MEM```: RTC FAST memory.
- ```ESP_PD_DOMAIN_XTAL```: Crystal oscillator domain.
- ```ESP_PD_DOMAIN_VDDSDIO```: Power supply domain for internal flash/PSRAM.       

**Power Options (esp_sleep_pd_option_t)**     
- ```ESP_PD_OPTION_OFF```: Power down the domain during sleep.
- ```ESP_PD_OPTION_ON```: Keep the domain powered during sleep.
- ```ESP_PD_OPTION_AUTO```: Allow the system to automatically decide based on active wake sources.

**6. Keeping Pin States & Internal Memory Active**          

**GPIO Hold & Isolation**     
- ```gpio_hold_en(gpio_num_t gpio_num)```
  - Holds the current state (HIGH, LOW, or pull-up/down) of a single GPIO pin during Deep Sleep. Prevents floating output signals to connected hardware.	 
- ```gpio_hold_dis(gpio_num_t gpio_num)```
  - Releases the pin lock after the system wakes up, returning control back to standard GPIO routines.	 
- ```gpio_deep_sleep_hold_en()```
  - Globally enables GPIO hold functionality across all configured pins during Deep Sleep.	 
- ```gpio_deep_sleep_hold_dis()```
  - Globally disables GPIO hold functionality during Deep Sleep.	 
- ```esp_sleep_config_gpio_isolate()```
  - Isolates digital GPIOs during sleep to prevent current leakage (floating pins drawing unexpected power).	 

**Memory Retention**     
- ```RTC_DATA_ATTR``` (Macro)
  - Modifier for global variables (e.g., RTC_DATA_ATTR int counter = 0;). Stores the variable in RTC Slow Memory so its value persists across Deep Sleep reboots.	 
- ```RTC_FAST_ATTR``` (Macro)
  - Modifier for placing code routines or variables into RTC Fast Memory, allowing rapid access immediately upon waking.	 
- ```esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON)```
  - Forces RTC Slow Memory to stay powered in Deep Sleep (enabled by default unless modified for Hibernation).	 


**7. Advanced Sleep Configuration & Optimization**     
These auxiliary functions let you fine-tune UART behavior, sleep timing, and sub-system execution prior to sleeping.     

**Subsystem & Peripheral Management**    
- ```esp_sleep_set_flash_use_usleep(bool enable)```
  - Enables or disables fast flash memory power-down during Light Sleep to save additional microamps.	  
- ```esp_sleep_pd_config(...)```
  - Dynamically changes power options (```ESP_PD_OPTION_ON```, ```ESP_PD_OPTION_OFF```, ```ESP_PD_OPTION_AUTO```) for specific internal domains like ```ESP_PD_DOMAIN_XTAL``` or ```ESP_PD_DOMAIN_VDDSDIO```.	 

**UART Handling Control**       
- ```esp_sleep_set_uart_handling_mode(esp_sleep_uart_handling_mode_t mode)```
  - Controls how UART TX/RX buffers are processed before entering sleep.	 

**UART Handling Modes (esp_sleep_uart_handling_mode_t)**      
- ```ESP_SLEEP_AUTO_FLUSH_SUSPEND_UART```: Default option. Flushes TX buffers before Deep Sleep and suspends during Light Sleep.
- ```ESP_SLEEP_ALWAYS_FLUSH_UART```: Waits until all queued data in the serial buffer is transmitted before entering sleep.
- ```ESP_SLEEP_ALWAYS_SUSPEND_UART```: Suspends transmission mid-frame; resumes upon wake if the UART domain remained powered.
- ```ESP_SLEEP_ALWAYS_DISCARD_UART```: Immediately drops unsent buffer data for faster sleep entry.       

**8. Automatic Light Sleep & Power Management (PM) APIs**       
If you want the ESP32-S3 to automatically enter Light Sleep whenever the CPU is idle (without explicitly calling esp_light_sleep_start()), you use the Power Management APIs.     
Requires: ```#include "esp_pm.h"```     

- ```esp_err_t esp_pm_configure(const esp_pm_config_t *config)```    
  - Enables automatic light sleep and Dynamic Frequency Scaling (DFS). You pass a struct defining max/min CPU frequencies and whether to enable light_sleep_enable = true.
- ```esp_err_t esp_pm_lock_create(esp_pm_lock_type_t lock_type, int arg, const char *name, esp_pm_lock_handle_t *out_handle)```
  - Creates a "lock" to prevent the chip from entering automatic light sleep or dropping CPU frequency while a specific task is running.
- ```esp_err_t esp_pm_lock_acquire(esp_pm_lock_handle_t handle)```
  - Acquires the lock (prevents auto-sleep / lowers CPU freq).
- ```esp_err_t esp_pm_lock_release(esp_pm_lock_handle_t handle)```
  - Releases the lock (allows auto-sleep / CPU freq scaling).
- ```esp_err_t esp_pm_lock_delete(esp_pm_lock_handle_t handle)```
  - Deletes the lock object to free memory.
**9. Modem Sleep APIs (Wi-Fi & Bluetooth)**      
Modem sleep is managed by the wireless stacks, not the core sleep API. It turns off the Wi-Fi/BT radio between beacons to save power while staying connected.      
Requires: ```#include "esp_wifi.h"``` and ```#include "esp_bt.h"```     
- ```esp_err_t esp_wifi_set_ps(wifi_ps_type_t type)```
  - Configures Wi-Fi power save.
  - WIFI_PS_NONE: Modem always on (highest power).
  - WIFI_PS_MIN_MODEM: Modem sleeps between DTIM beacons (default, moderate power).
  - WIFI_PS_MAX_MODEM: Modem sleeps between all beacons (lowest power, higher latency).
- ```esp_err_t esp_bt_sleep_enable(void)```
  - (Note: Usually handled automatically by the BT stack when initialized, but available if needed).

**10. Crucial ESP32-S3 RTC GPIO APIs**     
For the ESP32-S3, if you want to use GPIOs for Deep Sleep wake-up (ext0 / ext1), you cannot use standard pinMode() or digitalRead(). You must configure them as RTC GPIOs first.      
Requires: ```#include "driver/rtc_io.h"```      
- ```esp_err_t rtc_gpio_init(gpio_num_t gpio_num)```
  - Initializes a GPIO as an RTC GPIO (required before using it for deep sleep wake).
- ```esp_err_t rtc_gpio_set_direction(gpio_num_t gpio_num, rtc_gpio_mode_t mode)```
  - Sets the direction. Use RTC_GPIO_MODE_INPUT_ONLY for wake-up buttons.
- ```esp_err_t rtc_gpio_pulldown_en(gpio_num_t gpio_num) / rtc_gpio_pullup_en(...)```
  - Enables internal pull-down or pull-up resistors. Crucial for preventing floating pins from causing accidental wake-ups.
- ```esp_err_t rtc_gpio_hold_en(gpio_num_t gpio_num) / rtc_gpio_hold_dis(...)```
  - Holds the GPIO state during deep sleep. Useful if you need to keep an external sensor powered on or off while the ESP32-S3 is asleep.      

**esp_err_t**       
In the ESP32 ecosystem (ESP-IDF and the underlying C code for the Arduino core), ```esp_err_t``` is the standard data type used for returning error codes from functions.     
It is essentially a typedef (alias) for a 32-bit signed integer (int32_t).      
Whenever you call an ESP-IDF API function (like ```esp_sleep_enable_timer_wakeup()``` or ```esp_wifi_init()```), it will return an ```esp_err_t``` to tell you if the operation succeeded or failed.       

1. The Values of esp_err_t     
Unlike some C conventions where 0 means false/failure, in the ESP-IDF framework:     
- 0 means SUCCESS.
- Any non-zero value means an ERROR.      
The most common predefined error codes are defined in esp_err.h:   

|	Constant Name	|	Value	|	Meaning	|
|	-	|	-	|	-	|
|	```ESP_OK```	|	0x000	|	Success. The function executed perfectly.	|
|	```ESP_FAIL```	|	0x001	|	Generic failure or unspecified error.	|
|	```ESP_ERR_NO_MEM```	|	0x101	|	Out of memory (RAM or PSRAM).	|
|	```ESP_ERR_INVALID_ARG```	|	0x102	|	Invalid argument passed to the function (e.g., wrong GPIO pin).	|
|	```ESP_ERR_INVALID_STATE```	|	0x103	|	The ESP32 is in the wrong state for this function to run.	|
|	```ESP_ERR_INVALID_SIZE```	|	0x104	|	Size argument is invalid.	|
|	```ESP_ERR_NOT_FOUND```	|	0x105	|	Requested resource or setting was not found.	|
|	```ESP_ERR_NOT_SUPPORTED```	|	0x106	|	The feature is not supported on this specific chip/mode.	|
|	```ESP_ERR_TIMEOUT```	|	0x107	|	Operation timed out.	|

2. How to Use ```esp_err_t``` in Code     
You should always capture the return value of ESP-IDF functions and check it. Here is the standard pattern:
```
#include "esp_err.h"
#include "esp_sleep.h"

void setup() {
  Serial.begin(115200);
  
  // Call an ESP-IDF function and store the result in an esp_err_t variable
  esp_err_t err = esp_sleep_enable_timer_wakeup(5000000); // 5 seconds

  // Check if it succeeded
  if (err == ESP_OK) {
    Serial.println("Timer wake-up configured successfully!");
  } else {
    Serial.println("Failed to configure timer wake-up!");
  }
}

void loop() {}
```

3. Pro-Tip: Debugging with ```esp_err_to_name()```          
When a function fails, just printing "Error" isn't very helpful. The ESP-IDF provides a built-in function called ```esp_err_to_name()``` which converts the numeric ```esp_err_t``` code into a human-readable string (like "```ESP_ERR_INVALID_ARG```").     
Here is how you use it for powerful debugging:
```
#include "esp_err.h"
#include "esp_sleep.h"

void setup() {
  Serial.begin(115200);
  
  // Let's intentionally pass an invalid GPIO pin to trigger an error
  // (Assuming GPIO 99 doesn't exist on the ESP32-S3)
  esp_err_t err = esp_sleep_enable_ext0_wakeup(GPIO_NUM_99, HIGH);

  if (err != ESP_OK) {
    // Print the exact error name!
    Serial.printf("Function failed with error: %s\n", esp_err_to_name(err));
  }
}

void loop() {}
```

Output in Serial Monitor:     
```
Function failed with error: ```ESP_ERR_INVALID_ARG```      
```


## Reference      

https://github.com/espressif/esp-idf/blob/master/components/esp_hw_support/include/esp_sleep.h

https://github.com/espressif/esp-idf/blob/master/components/esp_pm/include/esp_pm.h

https://github.com/espressif/esp-idf/blob/master/components/esp_driver_gpio/include/driver/rtc_io.h

https://github.com/espressif/esp-idf/blob/master/components/bt/include/esp32/include/esp_bt.h

https://github.com/espressif/esp-idf/blob/master/components/esp_wifi/include/esp_wifi.h

https://www.oceanlabz.in/esp32-deep-sleep/

https://www.luisllamas.es/esp32-consumo-energia/

https://randomnerdtutorials.com/esp32-deep-sleep-arduino-ide-wake-up-sources/

https://deepbluembedded.com/esp32-sleep-modes-power-consumption/
