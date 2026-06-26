# Triple Output LED RGB – Common Cathode

A triple output LED RGB contains three individual light-emitting diodes (Red, Green, and Blue) housed in a single component. By mixing their output intensities, these components can generate millions of colors or function as independent status indicators.

**Key Technical Specs & Pinouts**      
- **Pin Configuration**: They typically feature 4 pins—one for each color and one shared Common Cathode (negative) or Common Anode (positive).
- **Forward Voltage**: Colors require varying voltage drops, generally defined as Red (≈ 2.0V), Green (≈ 3.2V), and Blue (≈ 3.2V).
- **Forward Current**: Standard 5mm units draw a maximum of 20mA per channel.

**How to Use & Control**
- **Color Mixing**: By utilizing Pulse Width Modulation (PWM) on a microcontroller (such as an Arduino), you can independently adjust the brightness of the red, green, and blue outputs to achieve custom hues.
- **Resistors Required**: You must use individual current-limiting resistors for each color pin to prevent the LED from burning out.


**Features:**    
- Forward Voltage (RGB): (2.0, 3.2, 3.2)V
- Luminosity (RGB): (800, 4000, 900)mcd

**Diagram**     

<img width="1275" height="806" alt="image" src="https://github.com/user-attachments/assets/83f687e2-2cc9-4bc2-aa69-88d4afd8c6de" />

**Wiring the LED**      
<img width="623" height="298" alt="image" src="https://github.com/user-attachments/assets/20146b6a-8ba6-4836-a817-6339e379791e" />

**Sample code**     
```
int REDPin = 4;    // RED pin of the LED to PWM pin 4
int GREENPin = 5;  // GREEN pin of the LED to PWM pin 5
int BLUEPin = 6;   // BLUE pin of the LED to PWM pin 6
int brightness = 0; // LED brightness
int increment = 5;  // brightness increment

void setup()
{
  pinMode(REDPin, OUTPUT);
  pinMode(GREENPin, OUTPUT);
  pinMode(BLUEPin, OUTPUT);
  Serial.begin(9600);
}

void loop()
{
  brightness = brightness + increment;  // increment brightness for next loop iteration

  if (brightness <= 0 || brightness >= 255)    // reverse the direction of the fading
  {
    increment = -increment;
  }
  brightness = constrain(brightness, 0, 255);
  analogWrite(REDPin, brightness);
  analogWrite(GREENPin, brightness);
  analogWrite(BLUEPin, brightness);

  delay(20);  // wait for 20 milliseconds to see the dimming effect
}
```

## Reference      

https://robot-r-us.com.sg/p/5mm-triple-output-led-rgb-common-cathode-5pcs

https://learn.sparkfun.com/tutorials/light-emitting-diodes-leds

https://wiring.org.co/learning/basics/rgbled.html
