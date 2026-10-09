# ESP32-S3 Boot-Up Troubleshooting

This guide helps troubleshoot an ESP32-S3 board that will not enter ROM download mode, cannot be detected by `esptool`, or fails with `No serial data received`.

If the USB cable, PC port, and drivers work with another ESP32-S3, the issue is isolated to this specific board.

---

## 1. Disconnect External Wires

The ESP32-S3 has strapping pins that determine its boot mode. If **GPIO 0**, **GPIO 3**, or **GPIO 46** are connected to anything—even a sensor, LED, or jumper wire—the chip may refuse to enter download mode.

1. Unplug everything from the board.
2. Leave the board completely bare, connected only to the USB cable.
3. Retry the manual boot sequence.

---

## 2. Enter Download Mode Manually

The auto-reset circuit may be faulty or out of spec. Use the manual **BOOT + EN** sequence:

1. Disconnect the USB cable.
2. Press and hold the **BOOT** button.
3. While holding **BOOT**, press and release the **EN** (or **RESET**) button once.
4. Wait 1 second.
5. Release **BOOT**.
6. Immediately run the command:

```powershell
esptool --chip esp32s3 --port COM4 erase-flash
```

**Follow this link to** [Installing Espressif's esptool](#installing-espressifs-esptool)

---

## 3. Check the USB Port

If the board has two USB-C ports, they behave differently:

| Port | Description | esptool behaviour |
|---|---|---|
| **UART Port** | Usually labeled `UART` or connected through a CH340/CP210x chip | Works reliably with standard `esptool` commands |
| **Native USB Port** | Usually labeled `USB`; connects directly to the ESP32-S3 internal USB-JTAG | Standard DTR/RTS auto-reset often fails |

If using the Native USB port, add `--before usb-reset`:

```powershell
esptool --chip esp32s3 --port COM4 --before usb-reset erase-flash
```

Use the **BOOT + EN** button trick immediately before running the command.

---

## 4. Observe Device Manager

Plug in the board and watch **Device Manager** closely:

| Scenario | Observation | Likely cause |
|---|---|---|
| **A** | The COM port appears and remains stable, but `esptool` still fails | Firmware crash or strapping-pin issue; see Steps 1 and 2 |
| **B** | The COM port appears for about 1 second, then disappears; Windows plays the device-disconnect sound | The board crashes immediately on boot. The flashed firmware may put the USB peripheral to sleep or cause a brownout. Use the **BOOT + EN** trick to catch it before it crashes |
| **C** | The COM port never appears and the board LED does not turn on | Hardware fault: dead USB-to-Serial chip, broken USB port, or damaged ESP32-S3 module |

---

## 5. Inspect the Hardware

Check the board closely for:

- Cold solder joints or bridged pins on the USB-C connector.
- Poor soldering on the ESP32-S3 module pins if it is a bare WROOM module on a custom PCB.
- Soldering problems on **GPIO 0**, **GPIO 3**, and **EN**.


---

# Installing Espressif's esptool

`esptool` is the command-line utility Arduino IDE uses to communicate with the ESP32 bootloader.

The following upload log shows that Arduino IDE is already running `esptool` version 5.3.1:

```text
esptool v5.3.1
Serial port COM4:
Connecting......................................
A fatal error occurred: Failed to connect to ESP32-S3:
No serial data received.
```

You may not need to install `esptool` separately. Installing the standalone version lets you test `COM4` directly and separate an Arduino IDE configuration issue from a USB/UART or bootloader problem.

## Check Python

1. Press `Win + R`, type `cmd`, and press Enter.
2. Run:

```powershell
python --version
```

If that does not work, run:

```powershell
py --version
```

You should see a version such as Python 3.12.x or Python 3.13.x. The current `esptool` version requires Python 3.10 or newer.

If Python is not installed, download it from the official [Python for Windows website](https://www.python.org/downloads/windows/). During installation, select **Add Python to PATH** if that option is offered.

## Install esptool

Run:

```powershell
python -m pip install --upgrade esptool
```

If `python` is not recognized but `py --version` worked, run:

```powershell
py -m pip install --upgrade esptool
```

The official installation instructions recommend using `python -m pip` when the standalone `pip` command is not available.

## Verify esptool

Run:

```powershell
python -m esptool version
```

If that command is not recognized, run:

```powershell
py -m esptool version
```

You should see the installed `esptool` version. The exact version may differ from Arduino IDE's bundled version.

```powershell
PS C:\WINDOWS\system32> python -m esptool version
esptool v5.5.0
5.5.0
```


## Check the COM Port

Before testing, close Arduino IDE's Serial Monitor and any other application using `COM4`.

1. Connect the ESP32-S3 using your normal USB/UART connector.
2. Open Windows **Device Manager**.
3. Expand **Ports (COM & LPT)**.
4. Confirm that the board appears as a COM port, such as `COM4`.

The name may identify a CP210x, CH340, or another USB-to-UART bridge. If no COM port appears, check the USB cable, connector, and driver before continuing.

## Test Communication

Run:

```powershell
python -m esptool --chip esp32s3 -p COM4 chip-id
```

If using `py`, run:

```powershell
py -m esptool --chip esp32s3 -p COM4 chip-id
```

Replace `COM4` if Device Manager shows a different port.

A successful response looks like this:

```powershell
PS C:\WINDOWS\system32> python -m esptool --chip esp32s3 -p COM4 chip-id
esptool v5.5.0
Connected to ESP32-S3 on COM4:
Chip type:          ESP32-S3 (QFN56) (revision v0.2)
Features:           Wi-Fi, BT 5 (LE), Dual Core + LP Core, 240MHz, Embedded PSRAM 8MB (AP_3v3)
Crystal frequency:  40MHz
MAC:                ac:a7:04:e0:4e:64

Stub flasher running.

WARNING: ESP32-S3 has no chip ID. Reading MAC address instead.
MAC:                ac:a7:04:e0:4e:64

Hard resetting via RTS pin...
```

If communication succeeds, `esptool` identifies the ESP32-S3 and prints chip information.

A fail response looks like this:

```powershell
PS C:\WINDOWS\system32> py -m esptool --chip esp32s3 -p COM4 chip-id
esptool v5.5.0
Serial port COM4:

ERROR: A fatal error occurred: Could not open COM4, the port is busy or doesn't exist.
(could not open port 'COM4': FileNotFoundError(2, 'The system cannot find the file specified.', None, 2))

Hint: Check if the port is correct and ESP connected
```

If you receive `No serial data received`, continue to the next step. `esptool` requires both a working serial connection and the ESP32-S3 to be in download mode.

## Force Download Mode

1. Disconnect the USB cable.
2. Press and hold **BOOT**.
3. While holding **BOOT**, connect the USB cable to the normal USB/UART connector.
4. Release **BOOT**.
5. Close and reopen Command Prompt if necessary.
6. Run the `chip-id` command again.

If the test still fails, hold **BOOT**, briefly press and release **EN/RESET**, release **BOOT**, and run the command again.

**Do not erase flash at this stage.** Establish communication first.

## Interpret the Result

- `esptool` detected the ESP32-S3 and displayed chip information.
- The command still returns `No serial data received`.
- The COM port cannot be opened or does not exist.
- Python or `esptool` is not recognized, or installation failed.

**Reference:** [esptool installation guide](https://documentation.espressif.com/projects/esptool/en/latest/esp32s3/installation.html)

---

# Confirm ROM Download Mode

The most reliable way to confirm that the ESP32-S3 is in ROM download mode is to run `esptool` and see whether it identifies the chip.

The blue LED blinking is not a reliable indicator. The LED may be controlled by the existing sketch, and its behaviour varies by board.

## Enter Download Mode

Using the normal USB/UART connector:

1. Disconnect USB.
2. Press and hold **BOOT**.
3. Connect USB while continuing to hold **BOOT**.
4. Release **BOOT**.
5. Open Command Prompt and run:

```powershell
python -m esptool --chip esp32s3 -p COM4 chip-id
```

Replace `COM4` if the board uses another port.

## Understand the Result

| What you see | What it means |
|---|---|
| `Chip is ESP32-S3` or chip information is displayed | **Success** — The bootloader is responding |
| `DOWNLOAD_BOOT` in the ROM boot log | The chip entered download mode |
| `No serial data received` | Communication failed; bootloader mode is not yet confirmed |
| `SPI_FAST_FLASH_BOOT` in the ROM boot log | The chip booted normally from flash, not into download mode |

Espressif documents the download-mode message and the BOOT/GPIO0 reset procedure in its [ESP32-S3 boot-mode guide](https://documentation.espressif.com/projects/esptool/en/latest/esp32s3/advanced-topics/boot-mode-selection.html).

## Check the Boot Log

If the command still fails, inspect the ROM boot message:

1. Open Arduino IDE Serial Monitor or another serial terminal.
2. Select `COM4` and set the baud rate to `115200`.
3. Hold **BOOT**, press and release **EN/RESET**, then release **BOOT**.
4. Look for a line similar to:

```text
boot:0x... (DOWNLOAD_BOOT...)
```

- `DOWNLOAD_BOOT` means the chip entered download mode.
- `SPI_FAST_FLASH_BOOT` means it started the existing firmware instead.
- The ROM log may not appear if the serial connection is not working or the relevant output is unavailable.

**Important:** Close Serial Monitor before running `esptool` again, because only one application can normally use `COM4` at a time.

---

# Recover from Bootloader Mode

Unplugging and reconnecting the USB cable normally returns the ESP32-S3 to its regular startup sequence after entering ROM bootloader mode.

- Hold **BOOT** and reset or connect USB → the ESP32-S3 enters ROM download mode.
- Unplug USB → the chip loses power.
- Reconnect USB without pressing **BOOT** → the chip normally boots from flash and runs the previously uploaded sketch.

Entering bootloader mode does **not** erase or change the existing firmware.

| Action | Expected result |
|---|---|
| Hold **BOOT** while resetting | Enters ROM download mode |
| Unplug and reconnect without holding **BOOT** | Boots normally from flash |
| Upload new firmware successfully | Runs the new sketch after reboot |
| Erase flash | Removes stored firmware and data, depending on what is erased |

If the blue LED resumes blinking after reconnecting, the previous sketch may be running again.

Unplugging and reconnecting safely exits bootloader mode, but it does not fix the `No serial data received` error. You still need to establish why `esptool` cannot communicate with the ESP32-S3 through `COM4`.
