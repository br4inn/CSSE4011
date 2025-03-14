### Task 2: RGB LED Control
This task involves controlling an RGB LED to display the 8 primary colors every 2 seconds using a Grove Chainable RGB LED.  
- **Functionality Achieved**:
  - The RGB LED is controlled via GPIO pins defined in the device tree overlay.
  - A thread cycles through the 8 primary colors (red, green, blue, cyan, magenta, yellow, white, and black) and updates the LED every 2 seconds.


## User Instructions
1- Compile using west build -b disco_l475_iot1 mycode/apps/prac1/task2 --pristine=always --sysbuild -Dmcuboot_EXTRA_DTC_OVERLAY_FILE="/home/brain/repo/mycode/apps/prac1/task2/boards/disco_l475_iot1.overlay"

2 - Flash using  west flash --runner jlink
 
