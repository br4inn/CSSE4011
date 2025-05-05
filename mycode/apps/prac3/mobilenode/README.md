# Prac 3 - Mobile Node

Name: Hamza Khurram<br>
Student number: 47435278<br>

## Description of Functionality
Mobile node scans and filters for the beacons. Then it advertises the 
RSSI values of each beacon into a packet which is broadcasted to the 
base node.

## Folder Structure
```
mobilenode
├── CMakeLists.txt
├── README.md
├── prj.conf
├── sample.yaml
└── src
    └── main.c
```

## User instructions
To build code for mobile node, do the following command
from the repo directory:
```
west build -p -b thingy52/nrf52832 mycode/p3/mobilenode
```

To flash the board, do the following command:
```
west flash
```
