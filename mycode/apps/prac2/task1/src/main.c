#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>
#include <stdio.h> 
#include "rtc.h"
#include "temp_humidity_sensor.h"
#include <zephyr/logging/log.h>


#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

// Thread stack size and priority
#define SENSOR_STACK_SIZE 1024
#define SENSOR_PRIORITY 5

// Thread objects and stacks
K_THREAD_STACK_DEFINE(sensor_stack, SENSOR_STACK_SIZE);
struct k_thread sensor_thread;

// External function from sensor library
extern void read_temp_humidity();

int main(void)
{
    LOG_INF("Starting sensor application...");

    // Start sensor thread
    k_thread_create(&sensor_thread, sensor_stack,
                    K_THREAD_STACK_SIZEOF(sensor_stack),
                    (k_thread_entry_t)read_temperature_humidity,
                    NULL, NULL, NULL,
                    SENSOR_PRIORITY, 0, K_NO_WAIT);

    while (1) {
        k_sleep(K_FOREVER); // Main thread sleeps forever
    }
    return 0;
}


// int main(void) {
//     rtc_init();
//     return 0;
  
// }