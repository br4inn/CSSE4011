#Prac 1 Introduction to Zephyr RTOS and Bootloader

**Jeremy Tatham 47446957**

## Brief Description of Functionality

### Design Task 1: Random Number Generator
This task implements a multithreaded program that uses the Zephyr rng to generate and display a random number every 2 seconds via serial output. 

### Design Task 2: RGB LED Control
This task involves controlling an RGB LED to display the 8 primary colours every 2 seconds with a Grove Chainable RGB LED.

### Design Task 3: Command Line Interface Shell
This task creates a command line interface shell accessible via serial terminal  with led control and time 

## Folder Structure

repo
|----prac1/
      |----task1/
           |----src/
           |    |----main.c
           |----boards/
           |    |----disco_l475_iot1.overlay
           |----CMakeLists.txt
           |----prj.conf
           |----README.md
      |----task2/
           |----src/
           |    |----main.c
           |----boards/
           |    |----disco_l475_iot1.overlay
           |----CMakeLists.txt
           |----prj.conf
           |----README.md
      |----task3/
           |----src/
           |    |----main.c
           |----boards/
           |    |----disco_l475_iot1.overlay
           |----CMakeLists.txt
           |----prj.conf
           |----README.md
## References
- Zephyr RTOS Documentation: https://docs.zephyrproject.org/latest/
- MCUBoot Bootloader: https://github.com/mcu-tools/mcuboot

 