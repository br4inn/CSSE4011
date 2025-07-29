#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include "bme280.h"
#include "ens160.h"
/* Custom ENS160 sensor channel for AQI */ 

void main(void)
{
    // Get BME280 device reference
    const struct device *bme280 = DEVICE_DT_GET_ANY(bosch_bme280);
    if (!device_is_ready(bme280)) {
        printk("BME280 not ready\n");
        return;
    }

    // Get ENS160 device reference
    const struct device *ens160 = check_ens160_device();
    if (!ens160 || !device_is_ready(ens160)) {
        printk("ENS160 not ready\n");
        return;
    }

    struct sensor_value temp, press, hum;
    struct sensor_value aqi, eco2, tvoc;

    while (1) {
        // --- BME280 ---
        if (sensor_sample_fetch(bme280) < 0) {
            printk("Failed to fetch BME280 data\n");
        } else {
            sensor_channel_get(bme280, SENSOR_CHAN_AMBIENT_TEMP, &temp);
            sensor_channel_get(bme280, SENSOR_CHAN_PRESS, &press);
            sensor_channel_get(bme280, SENSOR_CHAN_HUMIDITY, &hum);
        }

        // --- ENS160 ---
        if (sensor_sample_fetch(ens160) < 0) {
            printk("Failed to fetch ENS160 data\n");
        } else { 
            sensor_channel_get(ens160, SENSOR_CHAN_CO2, &eco2);
            sensor_channel_get(ens160, SENSOR_CHAN_VOC, &tvoc);
           
        }

        // Print all readings
        printk("BME280: Temp: %d.%06d C; Hum: %d.%06d %%; Press: %d.%06d kPa | ",
            temp.val1, temp.val2,
            hum.val1, hum.val2,
            press.val1, press.val2);
               
        printk("ENS160: eCO2: %d ppm; TVOC: %d ppb\n",
                eco2.val1, tvoc.val1);

        k_sleep(K_SECONDS(2));
    }
}
