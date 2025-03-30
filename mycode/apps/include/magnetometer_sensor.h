#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>


#ifndef MAG_SENSOR_H
#define MAG_SENSOR_H

void read_mag();
void read_mag_continous();
#endif  
