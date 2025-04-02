#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>


#ifndef PRESSURE_SENSOR_H
#define PRESSURE_SENSOR_H

void read_pressure();
void read_pressure_continous();
int get_latest_press_val(int *press_value);
#endif  
