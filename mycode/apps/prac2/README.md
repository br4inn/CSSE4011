#Prac 2 Sensor Processing, Logging and Viewing

**Jeremy Tatham 47446957**

## Brief Description of Functionality

- Periodically samples data from temperature, humidity, pressure, and magnetometer sensors.

- Encodes the sampled data into JSON fomat.

- A button interrupt is configured to start or stop continuous sampling of sensor data. 

A Python-based GUI (gui.py) receives JSON data over a serial connection and visualises it in real-time using graphs.
Multi-Sensor Support:

- Allows dynamic adjustment of the sampling rate via commands. 

## Folder Structure

repo
|----prac1/
      |----task1/
           |----src/
           |    |----main.c
           |    |----gui.py 
           |----boards/
           |    |----disco_l475_iot1.overlay
           |----CMakeLists.txt
           |----prj.conf
 
## References
- Zephyr RTOS Documentation: https://docs.zephyrproject.org/latest/
- MCUBoot Bootloader: https://github.com/mcu-tools/mcuboot

 