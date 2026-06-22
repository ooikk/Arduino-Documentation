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
<img width="596" height="661" alt="image" src="https://github.com/user-attachments/assets/5f31ee73-85c5-424b-ac84-f4cfc34aac74" />

<img width="1275" height="806" alt="image" src="https://github.com/user-attachments/assets/83f687e2-2cc9-4bc2-aa69-88d4afd8c6de" />
