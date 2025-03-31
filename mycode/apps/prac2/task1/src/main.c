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
#include "sampling.h"
#include "pressure_sensor.h" 
#include <zephyr/logging/log.h>
#include <zephyr/data/json.h>
#define SENSOR_STACK_SIZE 1024
#define SENSOR_PRIORITY 5
#define MSGQ_MAX_MSGS 10
#define MSGQ_MSG_SIZE sizeof(int)
#define SAMPLING_THREAD_STACK_SIZE 1024
K_THREAD_STACK_DEFINE(sampling_thread_stack, SAMPLING_THREAD_STACK_SIZE);
static struct k_thread sampling_thread_data;

#define JSON_THREAD_STACK_SIZE 1024
K_THREAD_STACK_DEFINE(json_thread_stack, JSON_THREAD_STACK_SIZE);
static struct k_thread temp_json_thread_data;
static struct k_thread hum_json_thread_data;


K_THREAD_STACK_DEFINE(temp_thread_stack, SENSOR_STACK_SIZE);
K_THREAD_STACK_DEFINE(hum_thread_stack, SENSOR_STACK_SIZE);
K_THREAD_STACK_DEFINE(pressure_thread_stack, SENSOR_STACK_SIZE);
K_THREAD_STACK_DEFINE(mag_thread_stack, SENSOR_STACK_SIZE);
 
static struct k_thread temp_thread_data;
static struct k_thread hum_thread_data;
static struct k_thread pressure_thread_data;
static struct k_thread mag_thread_data;

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#include <zephyr/sys/mutex.h>

int global_sampling_rate = 2000; // Default  
struct k_mutex sampling_rate_mutex;

// Define the message queue
//K_MSGQ_DEFINE(sampling_rate_msgq, MSGQ_MSG_SIZE, MSGQ_MAX_MSGS, 4);

K_THREAD_STACK_DEFINE(sensor_stack, SENSOR_STACK_SIZE);
struct k_thread temp_humid_sensor_thread, pressure_sensor_thread, mag_sensor_thread;

extern void read_temp();
extern void read_hum();
extern void read_mag();
extern void read_pressure();
extern void read_temp_continous();
extern void read_hum_continous();
extern void read_pressure_continous();
extern void read_mag_continous();
extern int set_rtc(int year, int month, int day, int hour, int min, int sec);
//extern int get_date_time(void);
extern const char *get_date_time(void);
extern int get_latest_hum_val(int *hum_val);

extern int get_latest_temp_val(int *temp_value);
static int generate_temp_json(const char *did, const char *rtc_time, int value);
struct sampling {
    const char *did;
    const char *rtc_time;
    int value;
};

static const struct json_obj_descr sampling_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct sampling, did, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct sampling, rtc_time, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct sampling, value, JSON_TOK_NUMBER),
};


// void json_generation_thread(void *arg1) {

//     struct sampling *sampling_data = (struct sampling *)arg1; // Retrieve the sampling_data struct
//     int value;
//     int sampling_rate = 2000;

//    // LOG_INF("JSON generation thread started for DID: %s", sampling_data->did);

//     // Check the appropriate sampling flag based on the DID
//     while ((strcmp(sampling_data->did, "0") == 0 && sampling_settings.ctn_temp_sampling_on) ||
//            (strcmp(sampling_data->did, "1") == 0 && sampling_settings.ctn_hum_sampling_on)) {
//         int new_rate;
//         if (k_msgq_get(&sampling_rate_msgq, &new_rate, K_NO_WAIT) == 0) {
//             sampling_rate = new_rate;
//             LOG_INF("Updated sampling rate: %d ms", sampling_rate);
//         }

//         if (strcmp(sampling_data->did, "0") == 0) {
//             if (get_latest_temp_val(&value) == 0) {
//                 const char *datetime_str = get_date_time();
//                 generate_temp_json(sampling_data->did, datetime_str, value);
//             } else {
//           //      LOG_ERR("No temperature values available for JSON generation");
//             }
//         } else if (strcmp(sampling_data->did, "1") == 0) {
//             if (get_latest_hum_val(&value) == 0) {
//                 const char *datetime_str = get_date_time();
//                 generate_temp_json(sampling_data->did, datetime_str, value);
//             } else {
//          //       LOG_ERR("No humidity values available for JSON generation");
//             }
//         }
        
//         k_sleep(K_MSEC(sampling_rate));
//     }

//  //   LOG_INF("JSON generation thread stopped for DID: %s", sampling_data->did);

//     // Free the sampling_data struct
//     k_free(sampling_data);
// }


void json_generation_thread(void *arg1) {
    struct sampling *sampling_data = (struct sampling *)arg1; // Retrieve the sampling_data struct
    int value;

    LOG_INF("JSON generation thread started for DID: %s", sampling_data->did);

    while ((strcmp(sampling_data->did, "0") == 0 && sampling_settings.ctn_temp_sampling_on) ||
           (strcmp(sampling_data->did, "1") == 0 && sampling_settings.ctn_hum_sampling_on)) {
        // Get the curr sampling rate
        int sampling_rate;
        k_mutex_lock(&sampling_rate_mutex, K_FOREVER);
        sampling_rate = global_sampling_rate;
        k_mutex_unlock(&sampling_rate_mutex);

        if (strcmp(sampling_data->did, "0") == 0) {
            if (get_latest_temp_val(&value) == 0) {
                const char *datetime_str = get_date_time();
                generate_temp_json(sampling_data->did, datetime_str, value);
            } else {
                LOG_ERR("No temperature values available for JSON generation");
            }
        } else if (strcmp(sampling_data->did, "1") == 0) {
            if (get_latest_hum_val(&value) == 0) {
                const char *datetime_str = get_date_time();
                generate_temp_json(sampling_data->did, datetime_str, value);
            } else {
                LOG_ERR("No humidity values available for JSON generation");
            }
        }

        k_sleep(K_MSEC(sampling_rate));
    }

    LOG_INF("JSON generation thread stopped for DID: %s", sampling_data->did);

    // Free the sampling_data struct
    k_free(sampling_data);
}
//    // const char *did = "0"; // Device ID for temperature sensor
//     struct sampling *sampling_data = (struct sampling *)arg1; // Retrieve the sampling_data struct
   
//     int temp_value, hum_value;
//     int sampling_rate = 2000;

    // while (sampling_settings.ctn_temp_sampling_on) {
    //    // int num_retrieved = get_latest_temp_val(temp_values);

    //    int new_rate;
    //    if (k_msgq_get(&sampling_rate_msgq, &new_rate, K_NO_WAIT) == 0) {
    //        sampling_rate = new_rate;
    //      //  LOG_INF("Updated sampling rate: %d ms", sampling_rate);
    //    }

    //    if (strcmp(sampling_data->did, "0") == 0) {
    //     //if (num_retrieved > 0) {
    //         if (get_latest_temp_val(&temp_value) == 0) {
    //         const char *datetime_str = get_date_time();
    //     //    LOG_INF("Retrieved RTC time: %s", datetime_str);

    //         // Generate JSON with the retrieved values
            
    //         generate_temp_json(sampling_data->did, datetime_str, temp_value);
    //     } else {
    //         LOG_ERR("No temperature values available for JSON generation");
    //     }
    //     k_sleep(K_MSEC(sampling_rate));
//     }
// }

//     while (sampling_settings.ctn_hum_sampling_on) {
//       if (strcmp(sampling_data->did, "1") == 0) {
//         if (get_latest_hum_val(&hum_value) == 0) {
//             const char *datetime_str = get_date_time();
//         //    LOG_INF("Retrieved RTC time: %s", datetime_str);

//             // Generate JSON with the retrieved values
            
//             generate_temp_json(sampling_data->did, datetime_str, hum_value);
//         } else {
//             LOG_ERR("No temperature values available for JSON generation");
//         }
//      }
//         k_sleep(K_MSEC(sampling_rate));
//     //    k_sleep(K_SECONDS(2)); // Adjust the interval as needed
//     }

    
//     LOG_INF("JSON generation thread stopped");
// }


 
static int generate_temp_json(const char *did, const char *rtc_time, int value) {
    struct sampling temp_data;
    temp_data.did = did;
    temp_data.rtc_time = rtc_time; // Use the formatted string

     temp_data.value= value;
 

    char str[128];
    int ret = json_obj_encode_buf(sampling_descr, ARRAY_SIZE(sampling_descr), &temp_data, str, sizeof(str));

    if (ret < 0) {
        LOG_ERR("JSON encoding error: %d", ret);
        return ret;
    }

    LOG_INF("%s", str);
    return 0;
} 

static int cmd_contin(const struct shell *shell, size_t argc, char **argv) {
    static struct sampling sampling_data;
    
    if (argc < 3) {
        shell_print(shell, "Usage: sample <s/p> <DID> or sample w <rate>");
        return 0;
    }

    const char *did = argv[2];
    if (strcmp(argv[1], "s") == 0) { 
        if (strcmp(did, "0") == 0) {
            // Start temp sampling
        
            if (sampling_settings.ctn_temp_sampling_on) {
                shell_print(shell, "Continuous sampling for temp sensor already enabled");
                return 0;
            }

            sampling_settings.ctn_temp_sampling_on = true;
//            sampling_data.did = did;

            struct sampling *temp_sampling_data = k_malloc(sizeof(struct sampling));
            temp_sampling_data->did = did;

        
            k_thread_create(&temp_thread_data, temp_thread_stack,
                            K_THREAD_STACK_SIZEOF(temp_thread_stack),
                            read_temp_continous, NULL, NULL, NULL,
                            SENSOR_PRIORITY, 0, K_NO_WAIT);
      
        k_thread_create(&temp_json_thread_data, json_thread_stack,
                K_THREAD_STACK_SIZEOF(json_thread_stack),
                json_generation_thread, temp_sampling_data, NULL, NULL,
                SENSOR_PRIORITY, 0, K_NO_WAIT);

 
            shell_print(shell, "Continuous sampling for temperature sensor started");
        }
        
        else if (strcmp(did, "1") == 0) {
            if (sampling_settings.ctn_hum_sampling_on) {
                shell_print(shell, "Continuous sampling for humidity sensor already enabled");
                return 0;
            }
            // Start hum sampling
            sampling_settings.ctn_hum_sampling_on = true;
 
               struct sampling *hum_sampling_data = k_malloc(sizeof(struct sampling));
               hum_sampling_data->did = did;
        //    sampling_data.did = did;
            k_thread_create(&hum_thread_data, hum_thread_stack,
                            K_THREAD_STACK_SIZEOF(hum_thread_stack),
                            read_hum_continous, NULL, NULL, NULL,
                            SENSOR_PRIORITY, 0, K_NO_WAIT);
          k_thread_create(&hum_json_thread_data, json_thread_stack,
            K_THREAD_STACK_SIZEOF(json_thread_stack),
            json_generation_thread, hum_sampling_data, NULL, NULL,
            SENSOR_PRIORITY, 0, K_NO_WAIT);

            shell_print(shell, "Continuous sampling for humidity sensor started");
        } else if (strcmp(did, "2") == 0) {
            // Start press sampling
            if (sampling_settings.ctn_pressure_sampling_on) {
                shell_print(shell, "Continuous sampling for pressure sensor already enabled");
                return 0;
            }
            sampling_settings.ctn_pressure_sampling_on = true;
            k_thread_create(&pressure_thread_data, pressure_thread_stack,
                            K_THREAD_STACK_SIZEOF(pressure_thread_stack),
                            read_pressure_continous, NULL, NULL, NULL,
                            SENSOR_PRIORITY, 0, K_NO_WAIT);
        } else if (strcmp(did, "4") == 0) {
            if (sampling_settings.ctn_mag_sampling_on) {
                shell_print(shell, "Continuous sampling for mag sensor already enabled");
                return 0;
            }
            // Start mag sampling
            sampling_settings.ctn_mag_sampling_on = true;
            k_thread_create(&mag_thread_data, mag_thread_stack,
                            K_THREAD_STACK_SIZEOF(mag_thread_stack),
                            read_mag_continous, NULL, NULL, NULL,
                            SENSOR_PRIORITY, 0, K_NO_WAIT);

            shell_print(shell, "Continuous sampling for all sensors started");
        } else if (strcmp(did, "15") == 0) {
            // Start all sensors
            if (sampling_settings.ctn_sampling_on) {
                shell_print(shell, "Continuous sampling for all sensors already enabled");
                return 0;
            }

            sampling_settings.ctn_sampling_on = true;

            if (!sampling_settings.ctn_temp_sampling_on) {
                sampling_settings.ctn_temp_sampling_on = true;
                k_thread_create(&temp_thread_data, temp_thread_stack,
                                K_THREAD_STACK_SIZEOF(temp_thread_stack),
                                read_temp_continous, NULL, NULL, NULL,
                                SENSOR_PRIORITY, 0, K_NO_WAIT);
            }

            if (!sampling_settings.ctn_hum_sampling_on) {
                sampling_settings.ctn_hum_sampling_on = true;
                k_thread_create(&hum_thread_data, hum_thread_stack,
                                K_THREAD_STACK_SIZEOF(hum_thread_stack),
                                read_hum_continous, NULL, NULL, NULL,
                                SENSOR_PRIORITY, 0, K_NO_WAIT);
            }

            if (!sampling_settings.ctn_pressure_sampling_on) {
                sampling_settings.ctn_pressure_sampling_on = true;
                k_thread_create(&pressure_thread_data, pressure_thread_stack,
                                K_THREAD_STACK_SIZEOF(pressure_thread_stack),
                                read_pressure_continous, NULL, NULL, NULL,
                                SENSOR_PRIORITY, 0, K_NO_WAIT);
            }

            if (!sampling_settings.ctn_mag_sampling_on) {
                sampling_settings.ctn_mag_sampling_on = true;
                k_thread_create(&mag_thread_data, mag_thread_stack,
                                K_THREAD_STACK_SIZEOF(mag_thread_stack),
                                read_mag_continous, NULL, NULL, NULL,
                                SENSOR_PRIORITY, 0, K_NO_WAIT);
            }

            shell_print(shell, "Continuous sampling for all sensors started");
        } else {
            shell_print(shell, "Invalid DID: %s", did);
            return 0;
        }
    } else if (strcmp(argv[1], "p") == 0) {
        // Stop sampling
        if (!sampling_settings.ctn_temp_sampling_on || !sampling_settings.ctn_hum_sampling_on || !sampling_settings.ctn_pressure_sampling_on) {
            shell_print(shell, "No sampling is currently running");
            return 0;
        }
    } else if (strcmp(argv[1], "p") == 0) {
        if (strcmp(did, "0") == 0) {
            if (!sampling_settings.ctn_temp_sampling_on) {
                shell_print(shell, "Temperature sampling is not running");
                return 0;
            }
            shell_print(shell, "Stopping temperature sampling");
            sampling_settings.ctn_temp_sampling_on = false;
        } else if (strcmp(did, "1") == 0) {
            if (!sampling_settings.ctn_hum_sampling_on) {
                shell_print(shell, "Humidity sampling is not running");
                return 0;
            }
            shell_print(shell, "Stopping humidity sampling");
            sampling_settings.ctn_hum_sampling_on = false;
        } else if (strcmp(did, "2") == 0) {
            if (!sampling_settings.ctn_pressure_sampling_on) {
                shell_print(shell, "Pressure sampling is not running");
                return 0;
            }
            shell_print(shell, "Stopping pressure sampling");
            sampling_settings.ctn_pressure_sampling_on = false;
        } else if (strcmp(did, "4") == 0) {
            if (!sampling_settings.ctn_mag_sampling_on) {
                shell_print(shell, "Magnetometer sampling is not running");
                return 0;
            }
            shell_print(shell, "Stopping magnetometer sampling");
            sampling_settings.ctn_mag_sampling_on = false;
        } else if (strcmp(did, "15") == 0) {
            if (!sampling_settings.ctn_sampling_on) {
                shell_print(shell, "No sensors are currently running");
                return 0;
            }
            shell_print(shell, "Stopping all sensors");
            sampling_settings.ctn_temp_sampling_on = false;
            sampling_settings.ctn_hum_sampling_on = false;
            sampling_settings.ctn_pressure_sampling_on = false;
            sampling_settings.ctn_mag_sampling_on = false;
            sampling_settings.ctn_sampling_on = false;
        } else {
            shell_print(shell, "Invalid DID: %s", did);
            return 0;
        }
      }  else if (strcmp(argv[1], "w") == 0) { 
            if (argc != 3) {
                shell_print(shell, "Usage: sample w <rate>");
                return 0;
            }
            int rate = atoi(argv[2]);
            if (rate <= 0) {
                shell_print(shell, "cant be -ms: %d", rate);
                return 0;
            }
         
            k_mutex_lock(&sampling_rate_mutex, K_FOREVER);
            global_sampling_rate = rate;
            k_mutex_unlock(&sampling_rate_mutex);
        
            shell_print(shell, "Sampling rate set to %d ms", rate);
            LOG_INF("Sampling rate changed to %d ms", rate);
     //   }
 
    // } else if (strcmp(argv[1], "w") == 0) { 
    //     if (argc != 3) {
    //         shell_print(shell, "Usage: sample w <rate>");
    //         return 0;
    //     }
        // int rate = atoi(argv[2]);
        // if (rate < 0) {
        //     shell_print(shell, "Invalid rate: %d", rate);
        //     return 0;
        // }
        // if (k_msgq_put(&sampling_rate_msgq, &rate, K_NO_WAIT) != 0) {
        //     shell_print(shell, "Failed to update sampling rate");
        //     return 0;
        // }
        // shell_print(shell, "Sampling rate set to %d ms", rate);
    } else {
        shell_print(shell, "Invalid command. Usage: sample <s/p> <DID> or sample w <rate>");
        return 0;
    }
    return 0;
}

static int cmd_rtc(const struct shell *shell, size_t argc, char **argv) {
    if (argc < 2) {
        shell_print(shell, "Usage: rtc r or rtc w <year> <month> <day> <hour> <min> <sec>");
        return 0;
    }

    if (strcmp(argv[1], "r") == 0) { 
        if (argc != 2) {
            shell_print(shell, "Usage: rtc r");
            return 0;
        }
       // get_date_time();
       const char *datetime_str = get_date_time();
       LOG_INF("RTC time: %s", datetime_str);

    } else if (strcmp(argv[1], "w") == 0) { 
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
    } else if (strcmp(did, "15") == 0) {
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
SHELL_CMD_REGISTER(sample, NULL, "sample <s/p> <DID> or sample w <rate>", cmd_contin);


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
