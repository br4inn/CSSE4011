# Prac 3 - Base Node

Name: Hamza Khurram<br>
Student number: 47435278<br>

## Description of Functionality
Base node receieves data from mobile node and ultrasonic node, calculates
the position, fuses the positions using a kalman filter, and outputs it using uart.
Shell commands are used to add, list, or clear the used beacons. The ultrasonic node's
location must also be specified using the `ultra` command.

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
basenode
├── CMakeLists.txt
├── README.md
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