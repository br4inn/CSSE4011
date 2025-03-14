### Task 1: Random Number Generator
This task implements a multithreaded program that uses Zephyr's random number generator (RNG) to generate and display a random number every 2 seconds via serial output.  
- **Functionality Achieved**:
  - A thread generates random 16-bit numbers using `sys_rand16_get()` which uses the hardware random number generator
  - Another thread displays the generated random numbers using `printk()` via a serial terminal (screen /dev/ttyACM0 115200)
  - Synchronisation between threads is achieved using `k_poll_signal` 

## User Instructions
1- Compile using west build -b disco_l475_iot1 mycode/apps/prac1/task1 --pristine=always --sysbuild -Dmcuboot_EXTRA_DTC_OVERLAY_FILE="/home/brain/repo/mycode/apps/prac1/task1/boards/disco_l475_iot1.overlay"

2 - flash using  west flash --runner jlink

3- Screen serial using screen /dev/ttyACM0 115200

