# Prac 3

Name: Hamza Khurram<br>
Student number: 47435278<br>

## Description of Functionality
In this prac, boards were programmed to track the location of a mobile node.
The mobile node broadcasts the RSSI values of each beacon to the base node.
The ultrasonic node broadcasts the measured distance between beacon and person
and broadcasts it to the base node.
The base node calculates the position from each node and fuses them using a
kalman filter. Then it sends this data to the PC GUI which displays the location
and sends it to the web dashboard.


## Shell Commands

### `list`

```sh
list <A-M> – Shows info for beacon named 'A'.
list -a – Lists **all** registered beacons.
```

---

### `ultra`

```sh
ultra <name> Registers an **ultrasonic node** 
```
---

### `add`

```sh
add <name> <major> <minor> Adds a beacon node with specified name 
```

---

### `clear`

```sh
clear <A-M> – Removes selected beacon.
clear -a – Clears **all** beacons.
```

---


## Folder Structure
```
p3
├── README.md
├── basenode
│   ├── CMakeLists.txt
│   ├── README.md
│   ├── prj.conf
│   ├── sample.yaml
│   └── src
│       └── main.c
├── gui.py
├── mobilenode
│   ├── CMakeLists.txt
│   ├── README.md
│   ├── prj.conf
│   ├── sample.yaml
│   └── src
│       └── main.c
└── ultrasonic
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
To build code for base node, do the following command
from the repo directory:
```
west build -p -b nrf52840dk/nrf52840 mycode/p3/basenode
```

To flash the board, do the following command:
```
west flash --recover
```

To build code for mobile node, do the following command
from the repo directory:
```
west build -p -b thingy52/nrf52832 mycode/p3/mobilenode
```

To flash the board, do the following command:
```
west flash
```

To build code for ultrasonic node, do the following command
from the repo directory:
```
west build -p -b nrf52840dk/nrf52840 mycode/p3/ultrasonic
```

To flash the board, do the following command:
```
west flash --recover
```
