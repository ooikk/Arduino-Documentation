# Capacitive Touch Sensor Pins

The ESP32 S3 has 14 capacitive touch GPIOs. These GPIOs can sense variations in anything that holds an electrical charge, like the human skin. So they can detect variations induced when touching the GPIOs with a finger.



## ESP32 S3 Touch Pins
```
Touch1     GPIO1
Touch2     GPIO2
Touch3     GPIO3
Touch4     GPIO4
Touch5     GPIO5
Touch6     GPIO6
Touch7     GPIO7
Touch8     GPIO8
Touch9     GPIO9
Touch10    GPIO10
Touch11    GPIO11
Touch12    GPIO12
Touch13    GPIO13
Touch14    GPIO14
```
## Detection


You can detect touch input using just a wire or a piece of foil — no external module needed. You can use the following command to read the touch pin capacitance. 

```
touchVal = touchRead(GPIO);
if (touchVal > THESHOLD) {
// touch detected
}
```
