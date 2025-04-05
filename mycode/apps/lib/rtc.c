#include "rtc.h"

#include <zephyr/logging/log.h>
//static const struct device *dev;

 
//LOG_MODULE_REGISTER(rtc, LOG_LEVEL_DBG);
LOG_MODULE_REGISTER(rtc, LOG_LEVEL_DBG);
const struct device *const rtc = DEVICE_DT_GET(DT_ALIAS(rtc));


int set_date_time(const struct device *rtc, int year, int month, int day, int hour, int min, int sec) {
    int ret = 0;
    struct rtc_time tm = {
        .tm_year = year - 1900,  
        .tm_mon = month - 1,     
        .tm_mday = day,
        .tm_hour = hour,
        .tm_min = min,
        .tm_sec = sec,
    };

    ret = rtc_set_time(rtc, &tm);
    if (ret < 0) {
        LOG_INF("Cannot write date time: %d\n", ret);
        return ret;
    }
    return ret;
}
 

#include <stdio.h> // For snprintf()

const char *get_date_time(void)
{
    static char datetime_str[32]; // Static buffer to hold the formatted date and time
    int ret = 0;
    struct rtc_time tm;

    // Get the current RTC time
    ret = rtc_get_time(rtc, &tm);
    if (ret < 0) {
        LOG_INF("Setting default time\n");

        // Set default time
        set_date_time(rtc, 2025, 1, 1, 0, 0, 0);

        // Retry getting the time
        ret = rtc_get_time(rtc, &tm);
        if (ret < 0) {
            LOG_INF("Cannot read date time even after setting default: %d\n", ret);
            return "1970-01-01 00:00:00"; // Return a default value if RTC fails
        }
    }
 
    snprintf(datetime_str, sizeof(datetime_str), "%04d-%02d-%02d %02d:%02d:%02d",
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
             tm.tm_hour, tm.tm_min, tm.tm_sec);

    return datetime_str;  
}
 

int set_rtc(int year, int month, int day, int hour, int min, int sec) {
 

    if (!device_is_ready(rtc)) {
		printk("Device is not ready\n");
		return 0;
	}

	set_date_time(rtc, year, month, day, hour, min, sec);
    return 0;

}
  
 