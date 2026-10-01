# ESP32-S3 N16R8 GPIO Guide

This guide assumes an `ESP32-S3-WROOM-1-N16R8` or `WROOM-1U-N16R8` module.

Its:

- 16 MB flash uses Quad SPI.
- 8 MB PSRAM uses Octal SPI.
- GPIO operates at 3.3 V.

The N16R8 is a 3.3 V GPIO variant. Do not apply the 1.8 V GPIO47–48 restriction of the different N16R8V/R16V variants to it.

A development board can still connect otherwise available pins to its own LED, display, SD card, or other hardware.

See the [ESP32-S3-WROOM-1 datasheet](https://documentation.espressif.com/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf).

## Quick Pin Selection

For ordinary peripherals, begin with:

```text
GPIO1–2, GPIO4–18, and GPIO21
```

Add GPIO38 and GPIO47–48 only after checking your board schematic.

For analog inputs used alongside Wi-Fi, choose these pins first:

```text
GPIO1–2 and GPIO4–10
```

The preferred rating assumes that the pin is exposed and unused on your particular board.

In the table below:

- `ADC1` and `ADC2` identify analog-capable pins.
- `Digital` means the pin can be assigned to ordinary input/output, PWM output, or suitable I²C, SPI, and UART signals through the ESP32-S3 GPIO matrix.
- These functions are alternatives. One pin cannot serve two conflicting connections at the same time.

See the [ESP-IDF GPIO documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/gpio.html).

## GPIO Reference Table

| GPIO | Useful Functions | Recommendation | Limitation or Check |
|---:|---|---|---|
| `0` | Digital | Usable with conditions. | BOOT strap. It must not be held low at reset for normal flash boot. Often connected to a BOOT button. |
| `1` | Digital, ADC1 | Preferred. | Brief low-level power-up glitch. Protect a load that must remain inactive during startup. |
| `2` | Digital, ADC1 | Preferred. | Same startup-glitch consideration as GPIO1. |
| `3` | Digital, ADC1 | Usable with conditions. | JTAG-selection strap. Avoid externally forcing its reset level without checking the intended JTAG setup. Brief low-level startup glitch. |
| `4`-`10` | Digital, ADC1 | Preferred. | Brief low-level startup glitch. |
| `11` - `14` | Digital, ADC2 | Preferred for digital; conditional for ADC. | Brief low-level startup glitch. ADC2 is shared with Wi-Fi; use ADC1 for dependable analog readings while Wi-Fi runs. |
| `15` - `16` | Digital, ADC2 | Preferred for digital; conditional for ADC. | Can serve a 32 kHz crystal. Check whether your board uses one. Brief low-level startup glitch. |
| `17` | Digital, ADC2 | Preferred for digital; conditional for ADC. | Brief low-level startup glitch. |
| `18` | Digital, ADC2 | Preferred for digital; conditional for ADC. | Can have both low- and high-level startup glitches. Avoid for an unprotected enable or trigger signal. |
| `19` | Digital, ADC2 | Usable with conditions. | Native USB D−. Reassigning it can disrupt USB serial, programming, or USB JTAG. Startup glitches also occur. |
| `20` | Digital, ADC2 | Usable with conditions. | Native USB D+. Same USB conflict; startup glitches also occur. |
| `21` | Digital | Preferred. | No special module reservation. Check board wiring. |
| `22–25` | — | Avoid. | These GPIO numbers do not exist on the ESP32-S3. |
| `26` - `32` | Internal memory bus | Avoid. | Used for the module's flash/PSRAM interface; not a general module pin. |
| `33` - `34` | Internal octal PSRAM bus | Avoid. | Occupied by N16R8 octal PSRAM and not brought out as a WROOM-1 module pin. |
| `35` - `37` | Internal octal PSRAM bus | Avoid. | A module pad exists, but it is connected to octal PSRAM internally. Do not use it externally. |
| `38` | Digital | Usable with conditions. | Drives the addressable RGB LED on ESP32-S3-DevKitC-1 v1.1. Otherwise available if your board leaves it free. |
| `39` - `42`| Digital | Usable with conditions. | Pin-based JTAG TCK, TD0, TD1 & TMS. Available when that debug interface is unused. |
| `43` | Digital | Usable with conditions. | Default UART0 TX. Boot messages and a board's USB-to-UART serial console may use it. |
| `44` | Digital | Usable with conditions. | Default UART0 RX. A board's USB-to-UART programming or console connection may use it. |
| `45` | Digital | Usable with conditions. | Strap associated with flash supply voltage. On N16R8, preserve its intended reset level; do not pull it high casually. |
| `46` | Digital | Usable with conditions. | Boot/ROM-output strap. Keep it at its intended reset level, particularly when entering download mode. |
| `47` | Digital | Preferred if free on the board. | 3.3 V on N16R8. Confirm that your board exposes it and has no other connection. |
| `48` | Digital | Usable with conditions. | 3.3 V on N16R8. Drives the addressable RGB LED on the initial DevKitC-1 revision; check your board. |

Espressif documents the N16R8 PSRAM connection, strapping behaviour, USB/JTAG assignments, UART0 pins, and approximately 60 µs power-up glitches for the pins noted above.

The glitch warning matters most for:

- Relays.
- Motor drivers.
- Chip-select lines.
- Other devices that can react before `setup()` configures the pin.

See the [ESP-IDF GPIO documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/gpio.html).

## Arduino Pin Selection

| Task | Good Starting Choices | Practical Limit |
|---|---|---|
| Digital input/output, buttons, LEDs | GPIO1–2, GPIO4–18, GPIO21. | Check startup behaviour for anything that must remain off during reset. |
| Analog input with `analogRead()` | ADC1: GPIO1–2, GPIO4–10. | GPIO3 is also ADC1 but is a strap. ADC2 is GPIO11–20; readings can conflict with Wi-Fi, and GPIO19–20 also serve USB. |
| PWM output with `ledcAttach()` | A free digital output such as GPIO4, GPIO5, GPIO17, or GPIO21. | PWM can be routed through the GPIO matrix. Frequency, resolution, and available LEDC channels are hardware limits, not fixed PWM pin numbers. |
| I²C with `Wire` | Any suitable free pair, such as GPIO8 SDA and GPIO9 SCL. | Supply suitable bus pull-ups to 3.3 V. A library or board may already specify pins. |
| Peripheral SPI | GPIO12 SCK, GPIO13 MISO, GPIO11 MOSI, and GPIO10 CS. | This is a separate peripheral bus. Do not use flash/PSRAM pins GPIO26–37. |
| Additional UART | GPIO17 RX and GPIO18 TX. | Keep GPIO43 and GPIO44 for UART0 if your board uses them for programming or logs. |

The I²C, SPI, UART, and PWM pin pairs above are examples, not mandatory default assignments.

ADC2 is shared with Wi-Fi, so ADC1 is the safer analog choice for a connected project.

See the [ESP-IDF ADC documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/adc/adc_oneshot.html).

## Voltage Rule

Treat N16R8 GPIO as 3.3 V logic, not 5 V tolerant.

Match external signal levels and power requirements to the module specifications.

If your board marking includes a `V` suffix, identify the exact module before using GPIO47 or GPIO48. The documented R16V variant operates those pins at 1.8 V.

See the [ESP32-S3-WROOM-1 datasheet](https://documentation.espressif.com/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf).

---

# ESP32-S3 N16R8
For the ESP32-S3 N16R8 variant (16 MB Flash + 8 MB Octal PSRAM, R8), the Octal PSRAM uses the extra SPI data lines, so GPIO33–GPIO37 are reserved for PSRAM in addition to the standard SPI0/1 flash pins. Based on the ESP32-S3 pin restriction guidance, here is the breakdown:

## ✅ Freely usable GPIOs (Priority 2 – no restrictions)
GPIO1, GPIO2, GPIO4, GPIO5, GPIO6, GPIO7, GPIO8, GPIO9, GPIO10, GPIO11, GPIO12, GPIO13, GPIO14, GPIO15, GPIO16, GPIO17, GPIO18, GPIO21, GPIO38

These pins can be used without restriction for GPIO Matrix-mapped peripherals. 

[GPIO pin assignment](https://documentation.espressif.com/esp32-s3_datasheet_en.html#page=26)

## ⚠️ Usable with caution (Priority 3 – may conflict with important functions)
GPIO0, GPIO45, GPIO46: Strapping pins — avoid external pull-up/pull-down that could affect boot mode.
GPIO3: Strapping pin.
GPIO19, GPIO20: Used by USB Serial/JTAG by default. If reconfigured to GPIO, USB-JTAG functionality is disabled.
GPIO39, GPIO40, GPIO41, GPIO42: JTAG interface (MTCK, MTDO, MTDI, MTMS). Usable as GPIO only if you don't need JTAG debugging via these pins (the built-in USB Serial/JTAG is used by default, freeing these for GPIO use).
GPIO43, GPIO44: UART0 (console) pins — usable as GPIO only if you don't need the default UART console.

[GPIO summary](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/gpio.html)

[Peripheral pin assignment](https://documentation.espressif.com/esp32-s3_datasheet_en.html#page=26)

## ❌ Not recommended for N16R8 (Priority 4 – reserved for Flash/PSRAM)
GPIO26–GPIO32: SPI0/1 interface connected to in-package flash/PSRAM.
GPIO33–GPIO37: On N16R8 (Octal PSRAM variant, e.g. ESP32-S3R8), these pins connect to SPIIO4–SPIIO7 and SPIDQS for the Octal PSRAM, and are therefore not recommended for other uses.

[GPIO summary](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/gpio.html) 

[IO pin restrictions](https://documentation.espressif.com/esp32-s3_datasheet_en.html#page=26)

Summary: For an N16R8 module, the safest set of general-purpose GPIOs to use freely is GPIO1, 2, 4–18, 21, and 38, with GPIO0/3/19/20/39–46/45/46 usable but requiring care due to strapping, USB-JTAG, JTAG, or UART0 functions, and GPIO26–37 reserved for Flash/PSRAM.
