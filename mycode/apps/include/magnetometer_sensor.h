#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>


#ifndef MAG_SENSOR_H
#define MAG_SENSOR_H
struct mag_data {
    struct sensor_value x;
    struct sensor_value y;
    struct sensor_value z; 
};

int get_latest_mag_val(struct mag_data *mag_val);
void read_mag();
void read_mag_continous();
#endif  
