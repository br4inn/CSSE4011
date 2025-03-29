#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>
#include <stdio.h> 
#include "rtc.h"
#include "temp_humidity_sensor.h"
#include "magnetometer_sensor.h"
#include "pressure_sensor.h" 
#include <zephyr/logging/log.h>

 

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);
 
#define SENSOR_STACK_SIZE 1024
#define SENSOR_PRIORITY 5
 
K_THREAD_STACK_DEFINE(sensor_stack, SENSOR_STACK_SIZE);
struct k_thread temp_humid_sensor_thread, pressure_sensor_thread, mag_sensor_thread;
 
extern void read_temp();
extern void read_hum();
extern void read_mag();
extern void read_pressure();
extern int set_rtc(int year, int month, int day, int hour, int min, int sec);
extern int get_date_time(void);
 

static int cmd_rtc(const struct shell *shell, size_t argc, char **argv) {
    if (argc < 2) {
        shell_print(shell, "Usage: rtc r or rtc w <year> <month> <day> <hour> <min> <sec>");
        return 0;
    }

    if (strcmp(argv[1], "r") == 0) {
        // Ensure no extra arguments are passed to "rtc r"
        if (argc != 2) {
            shell_print(shell, "Usage: rtc r");
            return 0;
        }
        get_date_time();
    } else if (strcmp(argv[1], "w") == 0) {
        // Ensure the correct number of arguments are passed to "rtc w"
        if (argc != 8) {
            shell_print(shell, "Usage: rtc w <year> <month> <day> <hour> <min> <sec>");
            return 0;
        }

        int year = atoi(argv[2]);
        int month = atoi(argv[3]);
        int day = atoi(argv[4]);
        int hour = atoi(argv[5]);
        int min = atoi(argv[6]);
        int sec = atoi(argv[7]);

        int ret = set_rtc(year, month, day, hour, min, sec);
        if (ret < 0) {
            shell_print(shell, "RTC time set failed: %d", ret);
            return ret;
        }
    } else {
        shell_print(shell, "Invalid command. Usage: rtc r or rtc w <year> <month> <day> <hour> <min> <sec>");
        return 0;
    }

    return 0;
}

static int cmd_sensor(const struct shell *shell, size_t argc, char **argv) {
    if (argc != 2) {
        shell_print(shell, "Usage: sensor <DID>");
        return 0;
    }

    const char *did = argv[1];  

    if (strcmp(did, "0") == 0) {
        read_temp();
    }
    else if (strcmp(did, "1") == 0) {
        read_hum();
    }
    else if (strcmp(did, "2") == 0) {
        read_pressure();
    }
    else if (strcmp(did, "4") == 0) {
        read_mag();
    } else if (strcmp(did, "ALL") == 0) {
        read_temp();
        read_hum();
        read_pressure();
        read_mag();
    }
    else {
        shell_error(shell, "Invalid DID: %s", did);  
        return 0;
    }

    return 0;
}

int main(void)
{
 //   LOG_INF("Starting sensors");

    // Start sensor thread
    // k_thread_create(&temp_humid_sensor_thread, sensor_stack,
    //                 K_THREAD_STACK_SIZEOF(sensor_stack),
    //                 (k_thread_entry_t)read_temperature_humidity,
    //                 NULL, NULL, NULL,
    //                 SENSOR_PRIORITY, 0, K_NO_WAIT);

    // k_thread_create(&mag_sensor_thread, sensor_stack,
    //                 K_THREAD_STACK_SIZEOF(sensor_stack),
    //                 (k_thread_entry_t)read_mag,
    //                 NULL, NULL, NULL,
    //                 SENSOR_PRIORITY, 0, K_NO_WAIT);
    // k_thread_create(&pressure_sensor_thread, sensor_stack,
    //                 K_THREAD_STACK_SIZEOF(sensor_stack),
    //                 (k_thread_entry_t)read_pressure,
    //                 NULL, NULL, NULL,
    //                 SENSOR_PRIORITY, 0, K_NO_WAIT);                
    //read_temperature_humidity();
    while (1) {
        
        k_sleep(K_FOREVER); // Main thread sleeps forever
    }
    return 0;
}

SHELL_CMD_REGISTER(sensor, NULL, "sensor <DID> #Read selected sensor", cmd_sensor);
SHELL_CMD_REGISTER(rtc, NULL, "rtc <w/r> #write/read RTC", cmd_rtc);


// // int main(void) {
// //     rtc_init();
// //     return 0;
  
// // }
/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

//  #include <zephyr/kernel.h>
//  #include <zephyr/device.h>
//  #include <zephyr/drivers/sensor.h>
//  #include <stdio.h>
//  #include <zephyr/sys/util.h>
 
//  static void process_sample(const struct device *dev)
//  {
//      static unsigned int obs;
//      struct sensor_value temp, hum;
//      if (sensor_sample_fetch(dev) < 0) {
//          printf("Sensor sample update error\n");
//          return;
//      }
 
//      if (sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp) < 0) {
//          printf("Cannot read HTS221 temperature channel\n");
//          return;
//      }
 
//      if (sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &hum) < 0) {
//          printf("Cannot read HTS221 humidity channel\n");
//          return;
//      }
 
//      ++obs;
//      printf("Observation:%u\n", obs);
//      printf("Raw Temp: %d.%06d, Raw Humidity: %d.%06d",
//         temp.val1, temp.val2, hum.val1, hum.val2);
 
//      /* display temperature */
//      printf("Temperature:%.1f C\n", sensor_value_to_double(&temp));
 
//      /* display humidity */
//      printf("Relative Humidity:%.1f%%\n",
//             sensor_value_to_double(&hum));
//  }
 
//  static void hts221_handler(const struct device *dev,
//                 const struct sensor_trigger *trig)
//  {
//      process_sample(dev);
//  }
 
//  int main(void)
//  {
//      const struct device *const dev = DEVICE_DT_GET_ONE(st_hts221);
 
//      if (!device_is_ready(dev)) {
//          printk("sensor: device not ready.\n");
//          return 0;
//      }
 
//      if (IS_ENABLED(CONFIG_HTS221_TRIGGER)) {
//          struct sensor_trigger trig = {
//              .type = SENSOR_TRIG_DATA_READY,
//              .chan = SENSOR_CHAN_ALL,
//          };
//          if (sensor_trigger_set(dev, &trig, hts221_handler) < 0) {
//              printf("Cannot configure trigger\n");
//              return 0;
//          }
//      }
 
//      while (!IS_ENABLED(CONFIG_HTS221_TRIGGER)) {
//          process_sample(dev);
//          k_sleep(K_MSEC(2000));
//      }
//      k_sleep(K_FOREVER);
//      return 0;
//  }
