# Prac 3 - Ultrasonic Node

Name: Hamza Khurram<br>
Student number: 47435278<br>

## Description of Functionality
The ultrasonic node measures the distance based on the ultrasonic ranger 
and broadcasts it to the base node. In the overlay file, the ultrasonic ranger's 
trigger pin is connected to P0.13, and echo pin is connected to P0.15. 

## Folder Structure
```
ultrasonic
├── CMakeLists.txt
├── README.md
├── boards
│   └── nrf52840dk_nrf52840.overlay
├── prj.conf
├── sample.yaml
└── src
    └── main.c
```

## User instructions
To build code for ultrasonic node, do the following command
from the repo directory:
```
west build -p -b nrf52840dk/nrf52840 mycode/p3/ultrasonic
```

To flash the board, do the following command:
```
west flash --recover
```
