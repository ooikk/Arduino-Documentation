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
