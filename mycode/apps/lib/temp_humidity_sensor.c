#include "temp_humidity_sensor.h"

LOG_MODULE_REGISTER(temp_humidity_sensor, LOG_LEVEL_DBG);

static struct sensor_value temp, hum;

#define TEMP_HUMIDITY_NODE DT_ALIAS(st_hts221)

static const struct device *temp_humidity_dev = DEVICE_DT_GET_ONE(st_hts221);




// static void process_sample() {
    
//     static unsigned int obs;
    
//     if (sensor_sample_fetch(temp_humidity_dev) < 0) {
//         LOG_ERR("sample update error\n");
//         return;
//     } else if (sensor_channel_get(temp_humidity_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp) < 0) {
//        LOG_ERR("Cannot read temperature channel\n");
//         return;
//     } else if (sensor_channel_get(temp_humidity_dev, SENSOR_CHAN_HUMIDITY, &hum) < 0) {
//         LOG_ERR("Cannot read humidity channel\n");
//         return;
//     ++obs;
//     LOG_INF("Observation:%u\n", obs);
    
//     LOG_INF("Temp: %.1fC, Humidity: %.1f%%",
//                     sensor_value_to_double(&temp),
//                     sensor_value_to_double(&hum));
//     }
//     LOG_INF("Raw Temp: %d.%06d, Raw Humidity: %d.%06d",
//         temp.val1, temp.val2, hum.val1, hum.val2);
 
// }
 
   


// static void hts221_handler(const struct sensor_trigger *trig) {
//     process_sample(temp_humidity_dev);
// }

void read_temp() { 
   // temp_humidity_dev = DEVICE_DT_GET_ONE(st_hts221);
    if (!device_is_ready(temp_humidity_dev)) {
        LOG_ERR("temp/humdity sensor not ready\n");
        return;
    }

//     if (IS_ENABLED(CONFIG_HTS221_TRIGGER)) {
// 		struct sensor_trigger trig = {
// 			.type = SENSOR_TRIG_DATA_READY,
// 			.chan = SENSOR_CHAN_ALL,
// 		};
// 		if (sensor_trigger_set(temp_humidity_dev, &trig, hts221_handler) < 0) {
// 			printf("Cannot configure trigger\n");
// 			return;
// 		}
// 	}
//     while (!IS_ENABLED(CONFIG_HTS221_TRIGGER)) {
// 		process_sample();
// 		k_sleep(K_MSEC(2000));
// 	}

// }

//while (1) {
    if (sensor_sample_fetch(temp_humidity_dev) < 0) {
        LOG_ERR("Failed to fetch sensor sample");
        k_sleep(K_MSEC(2000));
  //      continue;
    }

    if (sensor_channel_get(temp_humidity_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp) < 0) {
        LOG_ERR("Failed to get temperature channel");
        k_sleep(K_MSEC(2000));
    //    continue;
    }

    // if (sensor_channel_get(temp_humidity_dev, SENSOR_CHAN_HUMIDITY, &hum) < 0) {
    //     LOG_ERR("Failed to get humidity channel");
    //     k_sleep(K_MSEC(2000));
    //     continue;
    // }

   // LOG_INF("Temperature: %.1f C", sensor_value_to_double(&temp));
   // LOG_INF("Relative Humidity: %.1f%%", sensor_value_to_double(&hum));
 
    LOG_INF("Raw Temp: %d.%06dC", temp.val1, temp.val2); //PROBABLY USE PIPE FOR THIS
 
    // if (temp_or_humid == 1) {
    // LOG_INF("Raw Humidity: %d.%06d", hum.val1, hum.val2);
    // }
  //  k_sleep(K_MSEC(2000)); // Sleep for 2 seconds before the next reading
//}
}






 
void read_hum() { 
    // temp_humidity_dev = DEVICE_DT_GET_ONE(st_hts221);
     if (!device_is_ready(temp_humidity_dev)) {
         LOG_ERR("temp/humdity sensor not ready\n");
         return;
     }
 
 //     if (IS_ENABLED(CONFIG_HTS221_TRIGGER)) {
 // 		struct sensor_trigger trig = {
 // 			.type = SENSOR_TRIG_DATA_READY,
 // 			.chan = SENSOR_CHAN_ALL,
 // 		};
 // 		if (sensor_trigger_set(temp_humidity_dev, &trig, hts221_handler) < 0) {
 // 			printf("Cannot configure trigger\n");
 // 			return;
 // 		}
 // 	}
 //     while (!IS_ENABLED(CONFIG_HTS221_TRIGGER)) {
 // 		process_sample();
 // 		k_sleep(K_MSEC(2000));
 // 	}
 
 // }
 
 //while (1) {
     if (sensor_sample_fetch(temp_humidity_dev) < 0) {
         LOG_ERR("Failed to fetch sensor sample");
         k_sleep(K_MSEC(2000));
    //     continue;
     }
 
    //  if (sensor_channel_get(temp_humidity_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp) < 0) {
    //      LOG_ERR("Failed to get temperature channel");
    //      k_sleep(K_MSEC(2000));
    //      continue;
    //  }
 
     if (sensor_channel_get(temp_humidity_dev, SENSOR_CHAN_HUMIDITY, &hum) < 0) {
         LOG_ERR("Failed to get humidity channel");
         k_sleep(K_MSEC(2000));
    //     continue;
     }
 
    // LOG_INF("Temperature: %.1f C", sensor_value_to_double(&temp));
    // LOG_INF("Relative Humidity: %.1f%%", sensor_value_to_double(&hum));
   
   
     LOG_INF("Raw Humidity: %d.%06d%%", hum.val1, hum.val2);
    
    // k_sleep(K_MSEC(2000)); // Sleep for 2 seconds before the next reading
 ///}
 }
 
 
 
 
 
 
 
     
 
 //     while (1) {
 //         if (sensor_sample_fetch(temp_humidity_dev) < 0) {
 //             LOG_ERR("sample update error\n");
 //             return;
 //         } else if (sensor_channel_get(temp_humidity_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp) < 0) {
 //             LOG_ERR("Cannot read temperature channel\n");
 //             return;
 //         } else if (sensor_channel_get(temp_humidity_dev, SENSOR_CHAN_HUMIDITY, &hum) < 0) {
 //             LOG_ERR("Cannot read humidity channel\n");
 //             return;
 //         } else {
 //             sensor_channel_get(temp_humidity_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
 //             sensor_channel_get(temp_humidity_dev, SENSOR_CHAN_HUMIDITY, &hum);
 //         //    LOG_INF("Raw temp: %d, humid: %d,", temp, hum);
            
 //             LOG_INF("Temp: %.1fC, Humidity: %.1f%%",
 //                     sensor_value_to_double(&temp),
 //                     sensor_value_to_double(&hum));
 //         }
 //         k_sleep(K_MSEC(2000));  
 //     }
 // }