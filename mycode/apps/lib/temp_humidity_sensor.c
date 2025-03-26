#include "temp_humidity_sensor.h"

LOG_MODULE_REGISTER(temp_humidity_sensor, LOG_LEVEL_DBG);

static const struct device *temp_humidity_dev;
static struct sensor_value temp, hum;


void read_temperature_humidity() { 
    temp_humidity_dev = DEVICE_DT_GET_ONE(st_hts221);
    if (!device_is_ready(temp_humidity_dev)) {
        LOG_ERR("temp/humdity sensor not ready\n");
        return;
    }

    

    while (1) {
        if (sensor_sample_fetch(temp_humidity_dev) < 0) {
            LOG_ERR("sample update error\n");
        } else if (sensor_channel_get(temp_humidity_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp) < 0) {
            LOG_ERR("Cannot read temperature channel\n");
        } else if (sensor_channel_get(temp_humidity_dev, SENSOR_CHAN_HUMIDITY, &hum) < 0) {
            LOG_ERR("Cannot read humidity channel\n");
        } else {
            sensor_channel_get(temp_humidity_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
            sensor_channel_get(temp_humidity_dev, SENSOR_CHAN_HUMIDITY, &hum);
            LOG_INF("Temp: %.1fC, Humidity: %.1f%%",
                    sensor_value_to_double(&temp),
                    sensor_value_to_double(&hum));
        }
        k_sleep(K_MSEC(2000));  
    }
}