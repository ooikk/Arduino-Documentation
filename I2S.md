## Overview
I2S (Inter-IC Sound) is a synchronous serial bus protocol designed by Philips Semiconductors (now NXP) in 1986 for transmitting digital audio data between integrated circuits (e.g., ADCs, DACs, DSPs). It uses at least three lines—Bit Clock (BCLK), Word Select (WS/LRCK), and Serial Data (SD)—to transmit Pulse Code Modulation (PCM) stereo data, with a dedicated controller/target architecture for high-quality audio.

## Key Features of I2S Protocol:
**Three-Wire Interface:**
- **Bit Clock (BCLK/SCK):** Synchronizes each bit of data. The protocol does not specify a maximum data rate. The clock runs continuously.<br>
  SCK Frequency = Number of Channels × Sampling Frequency × Data Bit Width <br>
  Example: For stereo audio with a 44.1 kHz sampling rate (CD standard) and 16-bit width:<br>
  SCK Frequency = 2 × 44.1 kHz × 16 = 1.4112 MHz<br>

- **Word Select (WS/LRCK):** Identifies the channel (Left or Right). When WS is low, left channel data is transmitted; high corresponds to the right channel.
- **Serial Data (SD):** Transmits PCM audio data, typically MSB first.<br>
  SD (Serial Data) is the "carrier" for audio data, with two directions:<br>
  **SDOUT:* Serial output pin of transmitters (e.g., ADC, DSP).<br>
  **SDIN:* Serial input pin of receivers (e.g., DAC, MCU).<br>

<img width="586" height="214" alt="image" src="https://github.com/user-attachments/assets/23fb797f-96e4-444a-8c95-630219285356" />

<br>
<img width="741" height="220" alt="image" src="https://github.com/user-attachments/assets/04f9a3d0-800c-47cf-a63b-0a69deec06c1" />
<br>

**Controller/Target Architecture:** The component providing the clock signals (BCLK/WS) acts as the controller (formerly master), while the receiving component acts as the target (formerly slave).

**Audio Focus:** I2S is designed specifically for stereo audio. It supports PCM data and is commonly used in, for example, this DigiKey tutorial to demonstrate audio communication.

**Timing:** Data is clocked on the leading edge of the clock signal, with the WS line changing one clock period before the MSB of a new word, as shown in the NXP specification. 

## Common I2S Applications & Variants:
- **Audio Interfacing:** Widely used to connect DACs, ADCs, digital microphones, and DSPs in audio devices.
- **PCM/TDM Mode:** Some devices support Pulse Code Modulation (PCM) or Time Division Multiplexing (TDM) to handle more than two channels, which can be explored in the Qualcomm programming guide.
- **Differences from I2C:** Unlike I2C (which is for control), I2S is specifically for data audio transport. 

## Data Formats:
- **Philips Standard (I2S):** Standard mode.
- **Left-Justified (LJ):** Data starts immediately at WS edge, described in the Cdebyte blog.
- **Right-Justified (RJ):** Data ends with the WS edge.

## Master-Slave Working Modes
I2S systems require clear "Master" and "Slave" roles, determined by which device generates the clock (SCK) and frame synchronization (LRCK) signals. The Master provides timing references, while the Slave passively follows. Common configurations include:

**1. Transmitter as Master, Receiver as Slave**


<img width="480" height="143" alt="image" src="https://github.com/user-attachments/assets/c7090346-d45f-4e17-974f-e3ba96766e5c" />


**Scenario:** ADC (audio capture end) acts as Master, generating SCK and LRCK; DAC (audio output end) acts as Slave, receiving data based on timing.

**Advantage:** Capture-side timing control, ideal for "real-time recording" projects (e.g., voice recorders) to avoid data loss from receiver timing fluctuations.

**2. Receiver as Master, Transmitter as Slave**

<img width="481" height="143" alt="image" src="https://github.com/user-attachments/assets/12523f30-278d-4dcc-96c6-083c781fd0e5" />


**Scenario:** DAC (e.g., decoding chip in Hi-Fi players) acts as Master, generating timing signals; DSP (sound processing chip) acts as Slave, sending processed data in sync.

**Advantage:** Output-side timing control, suitable for "high-fidelity playback" to ensure stable sound quality.

**3. Both Transmitter and Receiver as Slaves**

<img width="492" height="239" alt="image" src="https://github.com/user-attachments/assets/3f01a11a-49e5-4168-a8e4-1ed74e11edfd" />


**Scenario:** An independent "clock generation module" (e.g., dedicated crystal oscillator, FPGA) acts as Master, providing SCK and LRCK. Both ADC (transmitter) and DAC (receiver) operate as Slaves, enabling coordinated data transmission.

**Advantage:** Centralized timing control, ideal for complex audio systems with multiple devices (e.g., digital mixers) to reduce timing interference.

## Connecting to EPS32 S3
The ESP32-S3 uses a GPIO Matrix, meaning there are no fixed hardware pins for I2S. You can assign the I2S signals to almost any available GPIO pin in your code.  

The ESP32-S3 contains two independent I2S peripherals (I2S0 and I2S1).


**1. Standard I2S Signal Pins.** 

To set up an I2S connection (e.g., for a microphone or speaker), you typically need to define these three main lines:  
```
Signal Name          Common Label            Description
BCLK                 SCK / BCK               Bit Clock: Synchronizes every bit of data.
WS                   LRCK / FS               Word Select: Indicates Left or Right audio channel.
DIN / DOUT           SD / DATA               Data In/Out: The actual digital audio stream.
MCLK                 CLK                     Master Clock: Optional; only needed for some high-end DACs.
```
**2. Example Pin Configurations**

Since you must define these in your code, here are common pin mappings used in popular ESP32-S3 projects:  
```
Device Example          BCLK      WS (LRCK)      DATA (SD)
Generic Microphone      GPIO 47   GPIO 10        GPIO 21 (Input)
MAX98357A Amplifier     GPIO 2    GPIO 1         GPIO 38 (Output)
LilyGo T7-S3            GPIO 4    GPIO 5         GPIO 1
```

**3. Implementation in Arduino**

If you are using the ESP32-I2S library or the native driver/i2s.h, you define the pins during initialization like this:
```
#include "driver/i2s.h"

#define I2S_BCLK 4
#define I2S_LRCK 5
#define I2S_SDAT 1

void setup() {
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRCK,
    .data_out_num = I2S_SDAT, // Use I2S_PIN_NO_CHANGE if input only
    .data_in_num = I2S_PIN_NO_CHANGE
  };
  i2s_set_pin(I2S_NUM_0, &pin_config);
}
```
**4. Connecting ESP32S3 to MAX98357A and 3W speaker**

To drive a MAX98357A with your ESP32-S3, you are moving into the world of digital audio. This specific chip is an I2S Class-D amplifier, meaning it takes the digital signal directly from the ESP32 and pumps it into your 3W speaker without needing a separate DAC.

**Hardware Wiring**

The MAX98357A works best with a 5V supply to achieve that full 3W output. If you use 3.3V, the volume will be significantly lower.
```
MAX98357A Pin   ESP32-S3 Pin   Note
LRC             GPIO 5         Word Select (Left/Right Clock)
BCLK            GPIO 4         Bit Clock
DIN             GPIO 1         Data Input
GAIN            GND            Sets gain to 9dB (Leave open for 12dB. See comment below.
SD              NC / 3.3V      Mode/Enable: Leave disconnected for Mono; Jump to 3.3V to force "ON"; Jump to Gnd to shutdown
VIN             5V             Power (Use a stable source for 3W)
GND             GND            Shared Ground
```

**Sample Code (Arduino IDE)**

This example uses the popular ESP32-audioI2S library, which is the most robust way to handle everything from simple tones to MP3s and Web Radio.

Library Requirement: In the Library Manager, search for and install "ESP32-audioI2S" by Wolle (Schreibfaul1).

```
#include "Audio.h"

// Updated I2S Pins for ESP32-S3
#define I2S_LRC       5
#define I2S_BCLK      4
#define I2S_DOUT      1   // Changed from 6 to avoid Flash Memory pins
#define AMP_SD_PIN    7   // Optional: Connect to SD pin for software control

Audio audio;

void setup() {
    Serial.begin(115200);

    // Optional: If you connected SD to GPIO 7, wake up the amp
    // pinMode(AMP_SD_PIN, OUTPUT);
    // digitalWrite(AMP_SD_PIN, HIGH); 

    // Initialize I2S for the MAX98357A
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    
    // Set volume (0 to 21)
    audio.setVolume(12); 

    // Option 1: Play a beep/tone to test
    // audio.playTone(1000, 1000); // 1kHz for 1sec

    // Option 2: Connect to WiFi and play a stream
    /*
    WiFi.begin("Your_SSID", "Your_Password");
    while (WiFi.status() != WL_CONNECTED) delay(500);
    audio.connecttohost("http://stream.radioparadise.com/mp3-128");
    */
    Serial.println("Audio Initialized on Pins: BCLK:4, LRC:5, DOUT:1");
}

void loop() {
    audio.loop(); // Required to keep the audio buffer filled
}
```

**Key Technical Notes**

**1. Power Supply for 3W**

A 3W speaker can pull roughly 600mA to 800mA at peak volume. If you are powering your ESP32-S3 via a standard computer USB port, you might experience "brownouts" or distorted sound because the port can't provide enough current. For a 3W speaker, a dedicated 5V/2A power supply is recommended.

**2. The "Gain" Pin**

The MAX98357A has a configurable gain. By default (if the pin is not connected), it is 12dB.
- GND: 9dB
- Pull-up (3.3V): 3dB
- Via 100k resistor to GND: 15dB

If your speaker is "clipping" (sounding crunchy at high volume), lower the gain by grounding the Gain pin.

**3. Mono vs. Stereo**

The MAX98357A is a Mono amplifier. It automatically averages the Left and Right I2S channels into one signal. If you want Stereo, you would need two MAX98357A chips—one with the SD pin pulled to GND via a resistor and the other to 3.3V (refer to the datasheet for specific resistor values to assign Left/Right).

## Reference

https://www.luisllamas.es/en/esp32-i2s/

https://www.oceanlabz.in/lesson-3-testing-your-speaker-max98357a/

https://www.allaboutcircuits.com/technical-articles/introduction-to-the-i2s-interface/

https://www.elprocus.com/i2s-protocol/

https://www.cdebyte.com/news/1247

https://web.archive.org/web/20070102004400/http://www.nxp.com/acrobat_download/various/I2SBUS.pdf
