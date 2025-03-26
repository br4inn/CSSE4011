#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>
#include <stdio.h> 

#ifndef RTC_H
#define RTC_H


int rtc_init(void);
int set_time(int hour, int min, int sec); 
#endif // RTC_H
