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

<img width="678" height="419" alt="image" src="https://github.com/user-attachments/assets/2201eca3-069b-4cbd-ad99-185a0ee54233" />
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

<img width="520" height="115" alt="image" src="https://github.com/user-attachments/assets/255f7514-68f3-40f2-b0ab-8dcfe8a51839" />

**Scenario:** ADC (audio capture end) acts as Master, generating SCK and LRCK; DAC (audio output end) acts as Slave, receiving data based on timing.

**Advantage:** Capture-side timing control, ideal for "real-time recording" projects (e.g., voice recorders) to avoid data loss from receiver timing fluctuations.

**2. Receiver as Master, Transmitter as Slave**

<img width="508" height="123" alt="image" src="https://github.com/user-attachments/assets/8814994e-3568-475a-b6c6-889325854dec" />

**Scenario:** DAC (e.g., decoding chip in Hi-Fi players) acts as Master, generating timing signals; DSP (sound processing chip) acts as Slave, sending processed data in sync.

**Advantage:** Output-side timing control, suitable for "high-fidelity playback" to ensure stable sound quality.

**3. Both Transmitter and Receiver as Slaves**

<img width="526" height="223" alt="image" src="https://github.com/user-attachments/assets/99985ae4-63ea-4117-9e3e-1eccd948087c" />

**Scenario:** An independent "clock generation module" (e.g., dedicated crystal oscillator, FPGA) acts as Master, providing SCK and LRCK. Both ADC (transmitter) and DAC (receiver) operate as Slaves, enabling coordinated data transmission.

**Advantage:** Centralized timing control, ideal for complex audio systems with multiple devices (e.g., digital mixers) to reduce timing interference.


## Reference

https://www.luisllamas.es/en/esp32-i2s/

https://www.oceanlabz.in/lesson-3-testing-your-speaker-max98357a/

https://www.allaboutcircuits.com/technical-articles/introduction-to-the-i2s-interface/

https://www.elprocus.com/i2s-protocol/

https://www.cdebyte.com/news/1247
