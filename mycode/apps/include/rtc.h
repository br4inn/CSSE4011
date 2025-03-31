#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>
#include <stdio.h> 

#ifndef RTC_H
#define RTC_H

int set_rtc(int year, int month, int day, int hour, int min, int sec);
int set_date_time(const struct device *rtc, int year, int month, int day, int hour, int min, int sec);
//int get_date_time(void);

const char *get_date_time(void);
#endif // RTC_H
