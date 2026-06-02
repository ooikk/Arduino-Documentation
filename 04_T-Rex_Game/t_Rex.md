# T_Rex Game
The T-Rex Game (also known as the Chrome Dino Game) is a simple, built-in side-scrolling runner game created by Google.

It famously appears in the Google Chrome web browser whenever you try to visit a webpage but lose your internet connection. The character design is a nod to the "prehistoric ages" of being offline.

**How It Works**
The gameplay is incredibly straightforward but gets addictive as the speed increases:

The Goal: Guide a pixelated Tyrannosaurus rex across a desert landscape for as long as possible to get a high score.

Obstacles: You must dodge oncoming obstacles like cacti and flying pterodactyls.

**Controls:**

Spacebar or Up Arrow: Jump over obstacles.

Down Arrow: Duck under flying obstacles (on desktop).

Tap Screen: Jump (on mobile devices).

As you advance, the game transitions between day mode (light background) and night mode (dark background) every 700 points.

**How to Play It (Even When Online)**
You don't actually have to pull your internet plug to play it. You can launch it anytime by opening a new tab in Google Chrome and typing this into the address bar:

chrome://dino

Fun Fact: The game is theoretically endless. The developers capped the maximum playtime at approximately 17 million years—a playful reference to how long the T-Rex was alive on Earth before extinction.

<img width="991" height="285" alt="image" src="https://github.com/user-attachments/assets/f703984a-98c2-4fa8-af7e-9a35bc16a68d" />


# The hardware setup

Follow the link for LCD1602A setup:

```
I2C LCD          ESP32 S3
GND              GND
VCC              VIN (2.5V to 6V)  // It's recommended connect to >5V to ensure enough contrast.
SDA              GPIO 8
SCL              GPIO 9
```

https://github.com/ooikk/Arduino-Documentation/blob/main/04_I2C_LCD1602A/I2C_LCD1602A.md

Below final circuit with push button switch.

```
// Toggle switch connects to GPIO 4 of ESP32
const int togglePin = 4;     // switch to trigger jump action
```

<img alt="image" style="width: 75%; height: auto;" src="https://github.com/user-attachments/assets/4f50c7ff-d5d9-4342-916f-aa5fc1925757" />

