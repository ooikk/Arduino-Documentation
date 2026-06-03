# Dino Jump

## 1-Button Dino Jump game on a 16x2 I2C LCD.

Even with just two rows of text, we can create a surprisingly fun and addictive game by using characters to represent the player, the ground, and the obstacles.

## 🎮 How This Version Works
- The Player (A or ^): Stays at column 3. Pressing the button makes them "jump" to the top row for 700ms.
- The Obstacle (M): Spawns on the right and moves left.
- The Ground (-): Fills the bottom row. If you are on the bottom row when the M hits your column, it's Game Over!
- Difficulty: The obstacle moves faster every time you successfully dodge it.

## 💡 Pro-Tips for This Specific Game

**1. No Flicker Technique:** Notice the row0 and row1 character arrays? Instead of using lcd.clear() every frame (which causes terrible flickering on I2C displays), the code builds the two lines in memory and prints them all at once. This makes the animation look smooth.
to 150ms. If you want to make it insanely hard, change 150 to 80.

**2. Custom Characters (Level Up):** If you want to get fancy, you can replace the 'A', '^', and 'M' with custom pixel art using lcd.createChar(). For example, you can draw a tiny 5x8 pixel dinosaur and a 5x8 pixel cactus.

**3. Add Sound:** Wire a small passive buzzer to GPIO 15 and GND. Add tone(15, 800, 100); inside the handleButtonPress() function when jumping, and a lower tone tone(15, 200, 500); in the GAMEOVER state for instant audio feedback!
