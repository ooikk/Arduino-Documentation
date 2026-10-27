[![logo-color](https://img.shields.io/badge/Index-Control_DC_motor_with_L298N_using_LEDC-AB03A9?logo=github&logoColor=white&labelColor=5B7E3C)](https://github.com/ooikk/Arduino-Documentation/blob/main/02_IO_Pins/02_PWM_DAC/Control_DC_motor_with_L298N.md)
[![logo-color](https://img.shields.io/badge/Index-RTOS_PID_Motor_Control-8ACFF8?logo=github&logoColor=white&labelColor=5B7E3C)](https://github.com/ooikk/Arduino-Documentation/blob/main/02_MISC/02_FreeRTOS/FreeRTOS.md)

# PWM as DAC

Connecting PWM pin to a R-C circuit will function as cheap DAC for external control. Below is the simple R-C circuit connected as passive low pass filter.

<img alt="image" style="width: 30%; height: auto;"  src="https://github.com/user-attachments/assets/a8417038-d23b-4f46-b1e5-802ebba57f0a" />

The equation for charging the capacitor is given:

<img width="362" height="50" alt="image" src="https://github.com/user-attachments/assets/10dcb452-49c4-4342-a43a-6d7495e5b342" />

Discharging capacitor equation:

<img width="203" height="50" alt="image" src="https://github.com/user-attachments/assets/adaf09e6-76c3-407d-9acc-ec67f5c9f2e4" />

Below is the example of the response for a given settings:

<img width="300" height=auto alt="image" src="https://github.com/user-attachments/assets/855332f4-876c-4781-bed9-abde151c2392" />

Voltage output:

<img alt="image" style="width: 50%; height: auto;"  src="https://github.com/user-attachments/assets/ba57d7cb-c586-4d60-a22e-5b3142474510" />


Link to PWM R-C circuit response: [https://docs.google.com/spreadsheets/d/10Ih6O-8kVcTtxQ6LSR1iQ8BdQ8ajPfoUlZrpEv3AAdg/edit?gid=0#gid=0](https://docs.google.com/spreadsheets/d/10Ih6O-8kVcTtxQ6LSR1iQ8BdQ8ajPfoUlZrpEv3AAdg/edit?gid=0#gid=0)

Online Circuit Simulator: [Falstad.com](https://www.falstad.com/circuit/)

Circuit Simulator
```text
<cir f="1" ts="0.000005" ic="7.943983955226134" cb="50" pb="50" vr="5" mts="5e-11">
  <r x="64 112 208 112" f="0" r="1000"/>
  <c x="208 112 208 240" f="0" c="0.000001" iv="0.001" sr="0" vd="0.9724239059608885"/>
  <g x="208 240 208 272" f="0"/>
  <w x="64 240 208 240" f="0"/>
  <v x="64 240 64 112" f="16" wf="2" fr="10000" maxv="1.65" bias="1.65" dutyCycle="0.3"/>
  <o en="1" sp="4" f="x3" p="0">
    <p v="0" sc="1.25"/>
    <p v="3" sc="0.0125"/>
  </o>
</cir>
```
