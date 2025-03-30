#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>


#ifndef TEMP_HUMIDITY_SENSOR_H
#define TEMP_HUMIDITY_SENSOR_H

void read_temp();
void read_hum();
void hts221_handler(const struct sensor_trigger *trig);
void process_sample();
void read_temp_continous();
void read_hum_continous();
int get_latest_temp_val(int *temp_value);

#endif  
