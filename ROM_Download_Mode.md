# ROM Download Mode
This is a crucial clue! Since the cable, PC port, and drivers are proven to work with another ESP32-S3, **the issue is isolated to this specific board**.

Here is how to troubleshoot a single stubborn ESP32-S3 board:

---

## 🔌 Step 1: Disconnect ALL External Wires (Crucial)

The ESP32-S3 has specific "strapping pins" that determine its boot mode. If **GPIO 0**, **GPIO 3**, or **GPIO 46** are connected to anything (even a sensor, LED, or jumper wire), the chip will refuse to enter download mode.

- **Action**: Unplug *everything* from the board. It should be completely bare, connected only to the USB cable. Then try the manual boot sequence again.

---

## 🎮 Step 2: The "BOOT + EN" Manual Reset Sequence

The auto-reset circuit (a small capacitor and resistor that automatically triggers download mode) might be faulty or out of spec on this specific board. You can do it manually with perfect timing:

1. **Unplug** the USB cable.
2. Press and **hold down the BOOT button**.
3. While holding BOOT, press and **release the EN (or RESET) button** once.
4. Wait 1 second, then **release the BOOT button**.
5. *Immediately* run the command:

```powershell
esptool --chip esp32s3 --port COM4 erase-flash
```

**Follow this link to** [Installing Espressif's esptool
](https://github.com/ooikk/Arduino-Documentation/blob/main/ROM_Download_Mode.md#installing-espressifs-esptool)

---

## 🔄 Step 3: Are You Using the "Native" USB Port?

If your board has two USB-C ports, they behave differently:

- **UART Port** (usually labeled "UART" or has a CH340/CP210x chip): Works reliably with standard `esptool` commands.
- **Native USB Port** (usually labeled "USB"): Connects directly to the ESP32-S3's internal USB-JTAG. Standard DTR/RTS auto-reset often fails here.
- **Action**: If you are using the Native USB port, tell `esptool` to use the USB reset sequence by adding `--before usb-reset`:

```powershell
esptool --chip esp32s3 --port COM4 --before usb-reset erase-flash
```

*(And remember to use the BOOT + EN button trick right before running it).*

---

## 🔊 Step 4: Listen to Windows & Watch Device Manager

Plug the board in and watch **Device Manager** closely:

- **Scenario A**: The COM port appears, stays stable, but `esptool` still fails. → This points to a firmware crash or strapping pin issue (see Step 1 & 2).
- **Scenario B**: The COM port appears for 1 second, then disappears (and you hear the Windows "device disconnect" sound). → The board is crashing immediately upon boot. This can happen if the flashed firmware is putting the USB peripheral to sleep or causing a brownout. The **BOOT + EN** trick (Step 2) is the only way to catch it before it crashes.
- **Scenario C**: The COM port never appears at all, and the board's LED does not turn on. → The board has a hardware fault (dead USB-to-Serial chip, broken USB port, or damaged ESP32-S3 module).

---

## 🔍 Step 5: Hardware Inspection

Look closely at the board:

- Are there any **cold solder joints** or bridged pins on the USB-C connector?
- If it's a bare module (WROOM) soldered to a custom PCB, check the soldering on the ESP32-S3 module pins, especially **GPIO 0**, **GPIO 3**, and **EN**.

---

## What to do next

Try **Step 1** and **Step 2** first. If it still fails, please reply with:

1. The **exact model/name** of this specific board (e.g., "ESP32-S3-DevKitC-1-N8", "LilyGO T-Display S3", or a custom PCB).
2. Does it have **one or two** USB ports?
3. What happens in Device Manager when you plug it in (does it stay, or does it disconnect)?

We will get it working!

---
# Installing Espressif's esptool

I'll guide you through installing Espressif's esptool on Windows and using it to diagnose your ESP32-S3 upload problem.

`esptool` is the command-line utility that Arduino IDE uses to communicate with the ESP32 bootloader. Your upload log already shows:

```text
esptool v5.3.1
Serial port COM4:
Connecting......................................
A fatal error occurred: Failed to connect to ESP32-S3:
No serial data received.
```

**Important:** This shows that Arduino IDE is already running `esptool` version 5.3.1. You may not need to install it separately. Installing the standalone version will let us test COM4 directly and separate an Arduino IDE configuration issue from a USB/UART or bootloader problem.

## Step 1 — Check whether Python is installed

### 1. Open Command Prompt

Press `Win + R`, type `cmd`, and press Enter.

### 2. Check Python

Enter this command:

```powershell
python --version
```

If that doesn't work, try:

```powershell
py --version
```

You should see a version such as Python 3.12.x or Python 3.13.x. The current `esptool` version requires Python 3.10 or newer.

— esptool latest documentation

If Python isn't installed, download it from the official [Python for Windows website](https://www.python.org/downloads/windows/). During installation, select **Add Python to PATH** if that option is offered.

## Step 2 — Install esptool

In Command Prompt, run:

```powershell
python -m pip install --upgrade esptool
```

Wait for the installation to finish. If `python` isn't recognized but `py --version` worked, use:

```powershell
py -m pip install --upgrade esptool
```

The official installation instructions recommend using `python -m pip` when the standalone `pip` command isn't available.

— esptool latest documentation

## Step 3 — Verify the installation

Enter:

```powershell
python -m esptool version
```

If that command isn't recognized, try:

```powershell
py -m esptool version
```

You should see the installed `esptool` version. The exact version may differ from Arduino IDE's bundled version.

## Step 4 — Check whether Windows detects your board

Before testing, close Arduino IDE's Serial Monitor and any other application using COM4.

1. Connect the ESP32-S3 using your normal USB/UART connector.
2. Open Windows **Device Manager**.
3. Expand **Ports (COM & LPT)**.
4. Confirm that your board appears as a COM port, such as `COM4`.

The name may identify a CP210x, CH340, or another USB-to-UART bridge. If no COM port appears, check the USB cable, connector, and driver before continuing.

## Step 5 — Test communication with the ESP32-S3

In Command Prompt, run:

```powershell
python -m esptool --chip esp32s3 -p COM4 chip-id
```

If you're using `py` instead, run:

```powershell
py -m esptool --chip esp32s3 -p COM4 chip-id
```

Replace `COM4` if Device Manager shows a different port.

**Expected result if communication succeeds:** `esptool` identifies the ESP32-S3 and prints chip information.

If you get `No serial data received` again, proceed to Step 6. Esptool requires both a working serial connection and the ESP32-S3 to be in download mode.

GitHub
+1

## Step 6 — Force download mode and retry

### Manual bootloader test

1. Disconnect the USB cable.
2. Press and hold **BOOT**.
3. While holding BOOT, connect the USB cable to the normal USB/UART connector.
4. Release BOOT.
5. Close and reopen the command prompt if necessary, then run the Step 5 command again.

If the test still fails, try holding BOOT, briefly pressing and releasing **EN/RESET**, then releasing BOOT and running the command again.

**Do not erase flash at this stage.** We only need to establish communication first.

## Step 7 — Interpret the result

What happened when you ran the command?

- It detected ESP32-S3 and displayed chip information.
- It still says `No serial data received`.
- It says the COM port cannot be opened or does not exist.
- Python or `esptool` is not recognized / installation failed.

### Continue troubleshooting

---
# ROM download mode (bootloader mode)

The result will tell us whether to focus on Arduino IDE, the Windows serial port/driver, or the ESP32-S3 bootloader itself.


The most reliable way to confirm that your ESP32-S3 is in ROM download mode (bootloader mode) is to run `esptool` and see whether it can identify the chip.

The blue LED blinking is not a reliable indicator. The LED may be controlled by your existing sketch, and its behaviour varies by board.

## Step 1 — Enter download mode

Using your normal USB/UART connector:

1. Disconnect USB.
2. Press and hold **BOOT**.
3. Connect USB while continuing to hold BOOT.
4. Release BOOT.
5. Open Command Prompt and run:

```powershell
python -m esptool --chip esp32s3 -p COM4 chip-id
```

Replace `COM4` if your board uses another port.

## Step 2 — Understand the result

| What you see | What it means |
|---|---|
| `Chip is ESP32-S3` or chip information is displayed | **Success** — The bootloader is responding. |
| `DOWNLOAD_BOOT` in the ROM boot log | The chip entered download mode. |
| `No serial data received` | Communication failed; bootloader mode is not yet confirmed. |
| `SPI_FAST_FLASH_BOOT` in the ROM boot log | The chip booted normally from flash, not into download mode. |

Espressif documents the download-mode message and the BOOT/GPIO0 reset procedure in its [ESP32-S3 boot-mode guide](https://documentation.espressif.com/projects/esptool/en/latest/esp32s3/advanced-topics/boot-mode-selection.html).

Espressif Documentation

## Step 3 — Check the boot log directly (optional)

If the command still fails, you can inspect the ROM boot message:

1. Open Arduino IDE Serial Monitor or another serial terminal.
2. Select `COM4` and set the baud rate to `115200`.
3. Hold **BOOT**, press and release **EN/RESET**, then release BOOT.
4. Look for a line similar to:

```text
boot:0x... (DOWNLOAD_BOOT...)
```

If you see `DOWNLOAD_BOOT`, the chip entered download mode. If you see `SPI_FAST_FLASH_BOOT`, it started the existing firmware instead. The ROM log may not appear if the serial connection is not working or the relevant output is unavailable.

**Important:** Close Serial Monitor before running `esptool` again, because only one application can normally use `COM4` at a time.

If you still cannot confirm the mode, tell me exactly what appears when you run the `chip-id` command. That will help us decide the next diagnostic step.

---
# Recover from Bootloader Mode

Yes. For your ESP32-S3, unplugging and reconnecting the USB cable will normally return it to its regular startup sequence after you have entered ROM bootloader mode.

Here is what happens:

- Hold BOOT and reset/connect USB → the ESP32-S3 enters ROM download mode.
- Unplug USB → the chip loses power.
- Reconnect USB without pressing BOOT → the chip normally boots from flash and runs your previously uploaded sketch.

The important distinction is that entering bootloader mode does **not** erase or change your existing firmware.

| Action | Expected result |
|---|---|
| Hold BOOT while resetting | Enters ROM download mode |
| Unplug and reconnect without holding BOOT | Boots normally from flash |
| Upload new firmware successfully | Runs the new sketch after reboot |
| Erase flash | Removes stored firmware and data, depending on what is erased |

If the blue LED resumes blinking after reconnecting, your previous sketch may be running again.

For your current upload problem: you can safely unplug and reconnect to leave bootloader mode. However, this alone will not fix the `No serial data received` error. We still need to establish why `esptool` cannot communicate with the ESP32-S3 through `COM4`.
