# **ESP32-S3 Learning Lab: Understanding the 4×4 Matrix Keypad**

The component in your photograph is a **4×4 membrane matrix keypad**. It is a commonly used input device for embedded systems, including ESP32, Arduino, access-control systems, password locks, menu navigation and industrial control panels.

It has 16 keys arranged in four rows and four columns. Instead of requiring 16 GPIO pins to read 16 individual switches, it uses only **8 GPIO pins: 4 row signals and 4 column signals**.

We will cover:

1. Hardware construction and working principle.
2. How matrix keypad scanning works.
3. How software identifies the pressed key.
4. The Arduino `Keypad.h` library.
5. Keypad library API functions.
6. ESP32-S3 wiring and programming examples.
7. Key debounce and multiple-key considerations.

## **1. Understanding the hardware**

[GetUSCart- 10pcs 4x4 Matrix Array Membrane Switch Keypad Keyboard 16 Key Matrix Array Membrane Switch 8pin Keypad Keyboard Microcontroller](https://images.openai.com/static-rsc-4/nxWpzCS6Ua8CPo4C5coCnuHaTWxaW7uDpAdpd7VOSBJyaEZnHk6BuWOAJL-2hN2uU0qhidyv8pS-sy_VHZxmMc4jHG-VwAdH9Z-lzrubS8CBPb69D0aUqXK4fEbND8__16wuNKfHLfm7cw1g3ABnZs_iRKaUwBVx1WsEHPZjQ0M?purpose=inline)

[getuscart.com](https://www.getuscart.com/10pcs-4x4-matrix-array-membrane-switch-keypad-keyboard-16-key-matrix-array-membrane-switch-8pin-keypad-keyboard-microcontroller?utm_source=chatgpt.com)

Your keypad consists of three main parts:

- **Keypad membrane:** The front panel containing 16 buttons.
- **Flexible ribbon cable:** Connects the keypad's internal row and column conductors.
- **8-pin connector:** Provides access to four row and four column signals.

### **1.1 Key arrangement**

The keypad has the following layout:

## **4 × 4 Key Matrix**

# **1**

# **2**

# **3**

# **A**

# **4**

# **5**

# **6**

# **B**

# **7**

# **8**

# **9**

# **C**

# **\***

# **0**

# **#**

# **D**

4 rows × 4 columns = 16 individual keys

| Row | Column 1 | Column 2 | Column 3 | Column 4 |
| --- | -------- | -------- | -------- | -------- |
| R1  | 1        | 2        | 3        | A        |
| R2  | 4        | 5        | 6        | B        |
| R3  | 7        | 8        | 9        | C        |
| R4  | \*       | 0        | #        | D        |

### **1.2 Why does it need only 8 wires?**

Each button is a normally-open momentary switch.

Internally, the keypad connects each button between one row conductor and one column conductor.

For example:

- Pressing `1` connects R1 to C1.
- Pressing `5` connects R2 to C2.
- Pressing `A` connects R1 to C4.
- Pressing `0` connects R4 to C2.

[Keypad - Fads to Obsessions](https://images.openai.com/static-rsc-4/YRtRZx55CYikEA-zBrltgCvch8SrbjKQEbF_qY0mZckf05Q8RVzAQh5Oudm4rkk0Y2jrKgu6Tab_sUWjZ1Lv0I7EewfM22rMM8LpqFq9-ZiMosYMv0q90oZJhg6xPqGM01mM6eo27A0Of2QFc1sgpkfhkNqh0Ix0TSegAfEeWEc?purpose=inline)

[fadstoobsessions.com](https://fadstoobsessions.com/Learning-Electronics/Nano-Microcontroller/keypad.php?utm_source=chatgpt.com)

The keypad is essentially a collection of 16 switches arranged in a matrix.

The number of GPIO connections is:

\\[ N\_{\text{GPIO}}=N\_{\text{rows}}+N\_{\text{columns}} \\]

For a 4×4 keypad:

\\[ N\_{\text{GPIO}}=4+4=8 \\]

For comparison:

| Keypad arrangement | Keys | GPIO required |
| ------------------ | ---- | ------------- |
| 1×4                | 4    | 5             |
| 2×3                | 6    | 5             |
| 3×4                | 12   | 7             |
| 4×4                | 16   | 8             |
| 4×5                | 20   | 9             |

This is the main advantage of matrix keypads: they reduce the number of GPIO pins required.

## **2. How matrix keypad scanning works**

The ESP32 does not continuously monitor 16 separate switches. Instead, the software scans the matrix systematically.

The scanning process uses two groups of GPIO signals:

- Four row pins: R1, R2, R3 and R4.
- Four column pins: C1, C2, C3 and C4.

The basic principle is:

1. Configure rows as outputs.
2. Configure columns as inputs with pull-up resistors.
3. Activate one row at a time.
4. Read all four columns.
5. Move to the next row.
6. Repeat continuously.

### **2.1 GPIO initialisation**

A common scanning configuration is:

| Signal | GPIO mode    | Purpose                                        |
| ------ | ------------ | ---------------------------------------------- |
| R1–R4  | OUTPUT       | Activate one row at a time                     |
| C1–C4  | INPUT_PULLUP | Detect whether a key connects to an active row |

Initially, all row outputs are HIGH.

All column inputs are pulled HIGH by their internal pull-up resistors.

When no key is pressed, the ESP32 reads:

**Initial condition**

Rows

HIGH

Columns

HIGH

All four column inputs read logic HIGH when no key is pressed.

### **2.2 Step-by-step scanning**

Suppose the user presses key `5`, which is located at R2, C2.

The ESP32 scans the rows one by one.

**Step 1: Scan R1**

- Set R1 LOW.
- Keep R2, R3 and R4 HIGH.
- Read C1, C2, C3 and C4.
- All columns remain HIGH because key `5` is not connected to R1.

**Step 2: Scan R2**

- Set R2 LOW.
- Keep the other rows HIGH.
- Read all four columns.
- Since key `5` connects R2 to C2, C2 becomes LOW.

The ESP32 detects:

\\[ R2=LOW,\quad C2=LOW \\]

Therefore, the pressed key is at row 2, column 2.

**Step 3: Scan R3**

- Set R3 LOW.
- Read the four columns.
- All columns remain HIGH.

**Step 4: Scan R4**

- Set R4 LOW.
- Read the four columns.
- All columns remain HIGH.

The complete scanning sequence is illustrated below.

**Example: Key 5 is pressed**

**R1 active**\


Not detected



**C1**

HIGH

**C2**

HIGH

**C3**

HIGH

**C4**

HIGH

**R2 active**\


Key detected



**C1**

HIGH

**C2**

LOW

**C3**

HIGH

**C4**

HIGH

**R3 active**\


Not detected



**C1**

HIGH

**C2**

HIGH

**C3**

HIGH

**C4**

HIGH

**R4 active**\


Not detected



**C1**

HIGH

**C2**

HIGH

**C3**

HIGH

**C4**

HIGH

Detected position: R2, C2 → Key 5

### **2.3 Converting row and column into a key value**

Once the software knows the row and column, it uses a lookup table to identify the character.

For example:

```
char keys[4][4] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
```

The lookup operation is:

```
char key = keys[row][col];
```

For row index 1 and column index 1:

```
char key = keys[1][1];  // Returns '5'
```

Notice that software array indexes start at zero, whereas the physical row and column numbers start at one.

### **2.4 An important detail: how the Keypad library actually scans**

The `Keypad.h` library uses the reverse GPIO arrangement compared with the earlier conceptual example.

Its actual implementation is:

| Signal | GPIO configuration               |
| ------ | -------------------------------- |
| R1–R4  | INPUT_PULLUP                     |
| C1–C4  | OUTPUT, one column LOW at a time |

The scanning sequence is:

1. Configure all four row pins as `INPUT_PULLUP`.
2. Configure C1 as OUTPUT and drive it LOW.
3. Read R1–R4.
4. Configure C1 back to a high-impedance input.
5. Repeat for C2, C3 and C4.
6. Store the detected key positions in an internal bitmap.

For example, when `5` is pressed, the library activates C2 LOW and reads R2 LOW.

This produces the same result: row 2, column 2, key `5`.

The important lesson is that **either scanning direction works electrically, but the GPIO configuration must match the scanning algorithm.** Do not mix the two approaches. The actual implementation can be examined in the library's `scanKeys()` function.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://github.com\&sz=32)

GitHub+1



## **3. Understanding the Keypad.h library**

[Interfacing 4x4 Matrix Keypad with Arduino Uno](https://images.openai.com/static-rsc-4/5Lxp2U8CETtQdISQh3maX7qDwf-uL-kp8yJ3_T8sjeom6Ke4HX2cqLQ3FyyH8893pQY2n_qoUS_Sb4Z_CanRsLfLXThKW0xtKN3XgRM7Fl2LyeTYOcMHZlCnQMj2RGVyU7m3aTH6ORNcFWLd2krw6r8tIvOvsLqRAAPfBE9P1Yo?purpose=inline)

**Arduino Keypad Library**

Authors: Mark Stanley and Alexander Brevig

Library version: 3.1.1 in its repository metadata

The `Keypad.h` library is a software abstraction layer. It hides the low-level GPIO scanning operations and gives your application a simple interface to retrieve keypresses.

Instead of writing code to activate columns, read rows, identify the key and handle debounce, you can simply write:

```
char key = keypad.getKey();

if (key) {
    Serial.println(key);
}
```

The library handles the scanning and key identification internally.

### **3.1 Installing the library**

In Arduino IDE 2.x:

1. Open Arduino IDE.
2. Select **Tools → Manage Libraries** or open Library Manager from the sidebar.
3. Search for `Keypad`.
4. Find the Keypad library by Mark Stanley and Alexander Brevig.
5. Install it.

The maintained Arduino Library Manager-compatible repository is:

[Chris--A/Keypad – Official library repository](https://github.com/Chris--A/Keypad?utm_source=chatgpt.com).

The library's main files include:

| File                 | Purpose                                          |
| -------------------- | ------------------------------------------------ |
| `Keypad.h`           | Class definitions and public API                 |
| `Keypad.cpp`         | Key scanning and state-management implementation |
| `Key.h`              | Key structure and key-state definitions          |
| `library.properties` | Library metadata                                 |

### **3.2 How the library works internally**

The library performs five main operations.

**1. GPIO scanning**

Activate each column and read all row inputs

**2. Key identification**

Convert row/column positions into key characters

**3. Debouncing**

Filter mechanical contact transitions

**4. State management**

Determine PRESSED, HOLD, RELEASED or IDLE

**5. Application interface**

Return key values or notify application callbacks

### **3.3 Key states**

The library maintains four states for each key.

| State      | Meaning                                             |
| ---------- | --------------------------------------------------- |
| `IDLE`     | Key is not pressed                                  |
| `PRESSED`  | Key has just been pressed                           |
| `HOLD`     | Key remains pressed beyond the configured hold time |
| `RELEASED` | Key has just been released                          |

The normal state sequence is:

IDLE

PRESSED

HOLD

RELEASED

HOLD occurs only if the key remains pressed long enough. A short press can transition directly from PRESSED to RELEASED.

For example, if the user presses and holds `A`:

- Initially: `IDLE`
- When pressed: `PRESSED`
- After the hold threshold: `HOLD`
- When released: `RELEASED`
- Afterwards: `IDLE`

The default hold time is 500 milliseconds.

### **3.4 Debouncing**

A physical button does not always produce a clean electrical transition.

When the contacts touch, they can bounce briefly between HIGH and LOW before stabilising.

Without debouncing, one physical press may be interpreted as several presses.

The library provides:

```
keypad.setDebounceTime(50);
```

This sets the debounce/scan interval to 50 milliseconds.

The default setting is 10 milliseconds.

**Important:** In this particular library, `setDebounceTime()` controls the interval between scans as well as the time used by the state-management logic. It is not simply a fixed delay after every press.

## **4. Complete Keypad.h API reference**

The following API list is based on the public declarations in the `Keypad.h` header of the Chris--A library. It covers the public `Keypad` methods, constructor, GPIO abstraction methods and inherited key information.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://github.com\&sz=32)

GitHub+1



### **4.1 Constructor**

**Function:**

```
Keypad(
    char *userKeymap,
    byte *row,
    byte *col,
    byte numRows,
    byte numCols
);
```

Creates a keypad object.

| Parameter    | Description                                |
| ------------ | ------------------------------------------ |
| `userKeymap` | Character matrix containing the key labels |
| `row`        | Array containing row GPIO numbers          |
| `col`        | Array containing column GPIO numbers       |
| `numRows`    | Number of rows                             |
| `numCols`    | Number of columns                          |

Example:

```
Keypad keypad(
    makeKeymap(keys),
    rowPins,
    colPins,
    4,
    4
);
```

### **4.2 Keypad public API functions**

| No. | Function             | Return type | Description                                            |
| --- | -------------------- | ----------- | ------------------------------------------------------ |
| 1   | `begin()`            | `void`      | Assign or initialise the keymap                        |
| 2   | `getKey()`           | `char`      | Return one newly pressed key                           |
| 3   | `getKeys()`          | `bool`      | Scan and update the active key list                    |
| 4   | `getState()`         | `KeyState`  | Return the state of the first key in the active list   |
| 5   | `isPressed()`        | `bool`      | Check whether a specified key has just entered PRESSED |
| 6   | `setDebounceTime()`  | `void`      | Set the debounce and scan interval                     |
| 7   | `setHoldTime()`      | `void`      | Set the hold detection threshold                       |
| 8   | `addEventListener()` | `void`      | Register a key-state event callback                    |
| 9   | `findInList(char)`   | `int`       | Find an active key by character                        |
| 10  | `findInList(int)`    | `int`       | Find an active key by numeric key code                 |
| 11  | `waitForKey()`       | `char`      | Block until a key is pressed                           |
| 12  | `keyStateChanged()`  | `bool`      | Check whether the first key's state changed            |
| 13  | `numKeys()`          | `byte`      | Return the maximum active key-list capacity            |

The detailed function descriptions follow.

**1.&#x20;**`begin()`

```
void begin(char *userKeymap);
```

Assigns the keymap to the keypad object.

```
keypad.begin(makeKeymap(keys));
```

Normally, the constructor already calls this function, so you do not need to call it separately.

**2.&#x20;**`getKey()`

```
char getKey();
```

Returns the character of a newly pressed key.

```
char key = keypad.getKey();

if (key != NO_KEY) {
    Serial.println(key);
}
```

Returns `NO_KEY` when no new keypress is detected.

This is the most commonly used API.

**3.&#x20;**`getKeys()`

```
bool getKeys();
```

Scans the keypad and updates the internal active-key list.

Returns `true` if a key-state change is detected.

```
if (keypad.getKeys()) {
    // Key activity or state change detected
}
```

Unlike `getKey()`, this function supports tracking multiple keys.

**4.&#x20;**`getState()`

```
KeyState getState();
```

Returns the current state of the first key in the active-key list.

```
KeyState state = keypad.getState();

if (state == PRESSED) {
    Serial.println("Pressed");
}
```

The states are `IDLE`, `PRESSED`, `HOLD` and `RELEASED`.

**5.&#x20;**`isPressed()`

```
bool isPressed(char keyChar);
```

Checks whether a specified key has just transitioned into the PRESSED state.

```
if (keypad.isPressed('A')) {
    Serial.println("A pressed");
}
```

**6.&#x20;**`setDebounceTime()`

```
void setDebounceTime(uint debounce);
```

Sets the debounce and scanning interval in milliseconds.

```
keypad.setDebounceTime(20);
```

Default: 10 ms. Minimum: 1 ms.

**7.&#x20;**`setHoldTime()`

```
void setHoldTime(uint hold);
```

Sets how long a key must remain pressed before entering the HOLD state.

```
keypad.setHoldTime(1000);
```

This example sets the hold threshold to one second.

Default: 500 ms.

**8.&#x20;**`addEventListener()`

```
void addEventListener(void (*listener)(char));
```

Registers a callback function that is called when a key-state transition occurs.

```
void keypadEvent(KeypadEvent key) {
    Serial.println(key);
}

void setup() {
    keypad.addEventListener(keypadEvent);
}
```

The callback receives the key character. To determine whether the key was pressed, held or released, call `getState()` inside the callback.

**9.&#x20;**`findInList(char)`

```
int findInList(char keyChar);
```

Finds an active key using its character.

```
int index = keypad.findInList('5');
```

Returns the index in the active-key list, or `-1` if not found.

**10.&#x20;**`findInList(int)`

```
int findInList(int keyCode);
```

Finds an active key using its numeric position code.

The library calculates the code as:

\\[ KeyCode=(row \times numCols)+column \\]

For key `5`, using zero-based indexing:

\\[ KeyCode=(1 \times 4)+1=5 \\]

```
int index = keypad.findInList(5);
```

**11.&#x20;**`waitForKey()`

```
char waitForKey();
```

Waits until a key is pressed and returns its character.

```
char key = keypad.waitForKey();
Serial.println(key);
```

Use with caution

This function blocks program execution while waiting. Avoid it in applications using FreeRTOS, Wi-Fi, Bluetooth, motor control or other time-sensitive tasks.

**12.&#x20;**`keyStateChanged()`

```
bool keyStateChanged();
```

Checks whether the first key in the active list has changed state.

```
if (keypad.keyStateChanged()) {
    Serial.println("Key state changed");
}
```

This is mainly useful with the single-key interface.

**13.&#x20;**`numKeys()`

```
byte numKeys();
```

Returns the capacity of the internal active-key array.

```
Serial.println(keypad.numKeys());
```

The standard library defines `LIST_MAX` as 10.

This is the capacity of the key tracking list, not the number of physical keys on the keypad.

### **4.3 GPIO abstraction methods**

The library also exposes three virtual methods, which can be overridden when adapting the keypad to a different GPIO interface, such as an I/O expander.

| Function                                | Description        |
| --------------------------------------- | ------------------ |
| `pin_mode(byte pinNum, byte mode)`      | Configure pin mode |
| `pin_write(byte pinNum, boolean level)` | Write output level |
| `pin_read(byte pinNum)`                 | Read input level   |

The default implementations call Arduino's `pinMode()`, `digitalWrite()` and `digitalRead()` respectively.

### **4.4 Constants and data types**

The library also provides these important definitions:

| Definition      | Meaning                                       |
| --------------- | --------------------------------------------- |
| `NO_KEY`        | No key detected (`'\0'`)                      |
| `IDLE`          | No active press                               |
| `PRESSED`       | Key pressed                                   |
| `HOLD`          | Key held                                      |
| `RELEASED`      | Key released                                  |
| `OPEN`          | Switch open                                   |
| `CLOSED`        | Switch closed                                 |
| `KeyState`      | Enumeration describing key state              |
| `KeypadEvent`   | Character type used by event callbacks        |
| `LIST_MAX`      | Maximum active-key list size (10)             |
| `MAPSIZE`       | Internal bitmap size (10 rows)                |
| `makeKeymap(x)` | Converts the keymap array for the constructor |

### **4.5 Inherited Key class members**

`Keypad` inherits from the `Key` class.

The inherited public members are:

| Member         | Type       | Description                      |
| -------------- | ---------- | -------------------------------- |
| `kchar`        | `char`     | Character represented by the key |
| `kcode`        | `int`      | Numeric position code            |
| `kstate`       | `KeyState` | Current state                    |
| `stateChanged` | `boolean`  | Indicates a state transition     |

The `Key` class also declares:

```
Key();
Key(char userKeyChar);

void key_update(
    char userKeyChar,
    KeyState userState,
    boolean userStatus
);
```

These are lower-level key-object methods. Normal keypad applications generally use the `Keypad` API instead.

## **5. ESP32-S3 wiring**

A 4×4 membrane keypad has eight connections, usually arranged in a single ribbon connector.

The connector order is not guaranteed to be the same across manufacturers. The photograph does not provide readable pin labels, so you should verify the actual row/column order rather than assume the first four pins are rows.

### **5.1 Suggested ESP32-S3 pin assignment**

For an ESP32-S3 DevKitC-1, one possible assignment is:

| Keypad signal | ESP32-S3 GPIO |
| ------------- | ------------- |
| R1            | GPIO 1        |
| R2            | GPIO 2        |
| R3            | GPIO 14       |
| R4            | GPIO 15       |
| C1            | GPIO 16       |
| C2            | GPIO 17       |
| C3            | GPIO 18       |
| C4            | GPIO 21       |

These are example assignments, subject to the actual board pinout and other peripherals connected to your board.

This example avoids the GPIOs already used by your ILI9488 and MFRC522 configurations.

[ESP32-S3-DevKitC-1 - ESP32-S3 -  — ESP-IDF Programming Guide v4.4 documentation](https://images.openai.com/static-rsc-4/It_gDsDgMKcCQYwpM8KsnMvIG9Je4hiUnwyvy6Cq3rCX0zOj6n3fw4I6k9oxj5rjJwcLf1hNQF0e9k61QhwJIFNIDqr5DJab_NN1_q2zD0y0GtwvehiSp8-4E3uF_GuiGze41-hgcDsiBCF00Hlv8KGSTXfNDqYYVtDjCnv2rVw?purpose=inline)

[docs.espressif.com](https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32s3/hw-reference/esp32s3/user-guide-devkitc-1.html?utm_source=chatgpt.com)

**Electrical precautions:**

- ESP32-S3 GPIO operates at 3.3 V.
- Do not connect 5 V signals to ESP32-S3 GPIO.
- The membrane keypad is a passive switch matrix and normally requires no external power supply.
- Use the ESP32's internal pull-up configuration provided by the library.

### **5.2 Wiring diagram**

**ESP32-S3 to 4×4 keypad**

**ESP32-S3**

GPIO 1

GPIO 2

GPIO 14

GPIO 15

GPIO 16

GPIO 17

GPIO 18

GPIO 21

**4×4 Keypad**

R1

R2

R3

R4

C1

C2

C3

C4

Connect each GPIO to its corresponding keypad signal. No VCC or GND connection is needed for the passive keypad.

## **6. ESP32-S3 programming example**

This is a complete example using the standard `Keypad.h` library.

The program reads the keypad and displays each pressed character in the Serial Monitor.

```

#include <Keypad.h>

// Keypad dimensions
const byte ROWS = 4;
const byte COLS = 4;

// Key layout
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// ESP32-S3 GPIO assignments
byte rowPins[ROWS] = {1, 2, 14, 15};
byte colPins[COLS] = {16, 17, 18, 21};

// Create keypad object
Keypad keypad = Keypad(
  makeKeymap(keys),
  rowPins,
  colPins,
  ROWS,
  COLS
);

void setup() {
  Serial.begin(115200);

  keypad.setDebounceTime(20);
  keypad.setHoldTime(1000);

  Serial.println("ESP32-S3 4x4 Keypad Ready");
}

void loop() {

  // Read keypad
  char key = keypad.getKey();

  // Detect a new keypress
  if (key != NO_KEY) {
    Serial.print("Key pressed: ");
    Serial.println(key);
  }
}

```

### **6.1 Expected Serial Monitor output**

If you press 1, 5, A, 0 and #:

```
ESP32-S3 4x4 Keypad Ready
Key pressed: 1
Key pressed: 5
Key pressed: A
Key pressed: 0
Key pressed: #
```

### **6.2 Example: Detecting long key presses**

The following example demonstrates the event callback mechanism.

```

void keypadEvent(KeypadEvent key) {

  switch (keypad.getState()) {

    case PRESSED:
      Serial.print("Pressed: ");
      Serial.println(key);
      break;

    case HOLD:
      Serial.print("Holding: ");
      Serial.println(key);
      break;

    case RELEASED:
      Serial.print("Released: ");
      Serial.println(key);
      break;

    case IDLE:
      break;
  }
}

```

Register the callback inside `setup()`:

```
keypad.addEventListener(keypadEvent);
```

Continue calling `keypad.getKey()` in `loop()` to keep the scanning process running.

### **6.3 Example: Password entry**

One practical application is an electronic password lock.

For example:

- User enters `1234`.
- User presses `#` to submit.
- ESP32 compares the entered password.
- If correct, activate a relay or unlock a door.
- If incorrect, display an error message.

The keypad library handles the physical input. Your application handles password validation and access control.

## **7. Important considerations when using a matrix keypad**

### **7.1 Multiple simultaneous keypresses**

The standard library supports multiple keypress tracking through `getKeys()`.

However, ordinary membrane matrix keypads generally do not have individual diodes for each switch.

When multiple keys are pressed simultaneously, electrical ghosting can occur. Certain combinations may produce false key detections.

For password entry and menu navigation, where users normally press one key at a time, this is usually not a problem.

### **7.2 Blocking versus non-blocking operation**

For ESP32 applications, use:

```
char key = keypad.getKey();
```

rather than:

```
char key = keypad.waitForKey();
```

The first allows the main loop to continue executing other operations. The second blocks until a key is pressed.

For your ESP32-S3 projects involving Wi-Fi, FreeRTOS and other peripherals, non-blocking scanning is generally preferable.

### **7.3 Keypad scanning frequency**

The library defaults to a 10 ms scan interval.

A shorter interval can improve responsiveness, but scanning too frequently is usually unnecessary.

For normal human-operated keypads, 10–20 ms is a reasonable starting point. Adjust debounce timing according to the keypad's mechanical characteristics.

## **8. Summary: What you should understand**

| Concept              | Key learning                                                      |
| -------------------- | ----------------------------------------------------------------- |
| Hardware             | 16 switches arranged in 4 rows and 4 columns                      |
| GPIO requirement     | 8 pins instead of 16                                              |
| Matrix scanning      | Activate one column at a time and read rows in the Keypad library |
| Key identification   | Row/column position maps to a character                           |
| Debouncing           | Prevents repeated detections from contact bounce                  |
| State machine        | IDLE, PRESSED, HOLD and RELEASED                                  |
| `getKey()`           | Retrieves a newly pressed character                               |
| `getKeys()`          | Tracks multiple key states                                        |
| `addEventListener()` | Enables event-driven handling                                     |
| `waitForKey()`       | Blocking input method                                             |

**My recommendation for an ESP32-S3 learning lab:** First implement the keypad using the `Keypad.h` library. Then build a second exercise using direct GPIO scanning without the library. Comparing the two implementations will help learners understand how a software library abstracts GPIO operations, scanning algorithms, debouncing and state machines.

## **9. References**

The following are the primary technical references used for the library API and implementation:

1. [Chris--A/Keypad – GitHub repository](https://github.com/Chris--A/Keypad?utm_source=chatgpt.com).
2. [Keypad.h – Public API declarations](https://github.com/Chris--A/Keypad/blob/master/src/Keypad.h?utm_source=chatgpt.com).
3. [Keypad.cpp – Scanning and state-machine implementation](https://github.com/Chris--A/Keypad/blob/master/src/Keypad.cpp?utm_source=chatgpt.com).
4. [Keypad library example programs](https://github.com/Chris--A/Keypad/tree/master/examples?utm_source=chatgpt.com).

The API reference above describes the standard Chris--A library, not every third-party library that happens to use the filename `Keypad.h`.

One final hardware note: the eight ribbon pins on your particular keypad should be identified before connecting them. You can use a multimeter continuity test or a simple GPIO scanning program to establish the correct order. The physical ribbon arrangement is not reliably determined from the photograph alone.

For a teaching exercise, I would divide this into two practical labs:

- **Lab 1 – Keypad with Keypad.h:** Read keys, implement password entry, detect long presses and handle key events.
- **Lab 2 – Keypad without a library:** Write your own GPIO scanning algorithm, implement debounce using `millis()`, and build a four-state key state machine.

This gives learners an understanding of both the hardware-level operation and the software abstraction provided by Arduino libraries.
