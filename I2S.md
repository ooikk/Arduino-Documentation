## Overview
I2S (Inter-IC Sound) is a synchronous serial bus protocol designed by Philips Semiconductors (now NXP) in 1986 for transmitting digital audio data between integrated circuits (e.g., ADCs, DACs, DSPs). It uses at least three lines—Bit Clock (BCLK), Word Select (WS/LRCK), and Serial Data (SD)—to transmit Pulse Code Modulation (PCM) stereo data, with a dedicated controller/target architecture for high-quality audio.

<img width="570" height="590" alt="image" src="https://github.com/user-attachments/assets/b3efc747-9f54-48ff-94d0-ecfb9ee7b57f" />


<img width="678" height="419" alt="image" src="https://github.com/user-attachments/assets/2201eca3-069b-4cbd-ad99-185a0ee54233" />

<img width="741" height="220" alt="image" src="https://github.com/user-attachments/assets/04f9a3d0-800c-47cf-a63b-0a69deec06c1" />


## Key Features of I2S Protocol:
**Three-Wire Interface:**
- **Bit Clock (BCLK/SCK):** Synchronizes each bit of data. The protocol does not specify a maximum data rate. The clock runs continuously.
- **Word Select (WS/LRCK):** Identifies the channel (Left or Right). When WS is low, left channel data is transmitted; high corresponds to the right channel.
- **Serial Data (SD):** Transmits PCM audio data, typically MSB first.

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


## Reference

https://www.luisllamas.es/en/esp32-i2s/

https://www.oceanlabz.in/lesson-3-testing-your-speaker-max98357a/

https://www.allaboutcircuits.com/technical-articles/introduction-to-the-i2s-interface/

https://www.elprocus.com/i2s-protocol/

https://www.cdebyte.com/news/1247
