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

int get_date_time(void)
{
	int ret = 0;
	struct rtc_time tm;

	ret = rtc_get_time(rtc, &tm);
    if (ret < 0) {
        LOG_INF("setting default time\n");

        // Set default 
        set_date_time(rtc, 2025, 1, 1, 0, 0, 0);

        // Retry getting the time
        ret = rtc_get_time(rtc, &tm);
        if (ret < 0) {
            LOG_INF("Cannot read date time even after setting default: %d\n", ret);
            return ret;
        }
    }
    LOG_INF("RTC date and time: %04d-%02d-%02d %02d:%02d:%02d\n",
        tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
        tm.tm_hour, tm.tm_min, tm.tm_sec);

    return ret;
}
// 	if (ret < 0) {
// 		printk("Cannot read date time: %d\n", ret);
// 		return ret;
// 	}

// 	printk("remove print RTC date and time: %04d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900,
// 	       tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
//  //       LOG_INF("remove print RTC date and time: %04d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900,
//    //         tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
         
// 	return ret;
// }


int set_rtc(int year, int month, int day, int hour, int min, int sec) {
 

    if (!device_is_ready(rtc)) {
		printk("Device is not ready\n");
		return 0;
	}

	set_date_time(rtc, year, month, day, hour, min, sec);
    return 0;

}

// int main(void)
// {
// 	/* Check if the RTC is ready */
// 	if (!device_is_ready(rtc)) {
// 		printk("Device is not ready\n");
// 		return 0;
// 	}

// 	set_date_time(rtc);

// 	/* Continuously read the current date and time from the RTC */
// 	while (get_date_time(rtc) == 0) {
// 		k_sleep(K_MSEC(1000));
// 	};
// 	return 0;
// } 

// int rtc_init(void) {
//     dev = DEVICE_DT_GET(DT_INST(0, st_stm32_rtc)); //board uses stm32
//     if (!device_is_ready(dev)) {
//         return 1;
//     }
//     return 0;
// }

// int set_time(int hour, int min, int sec) {
//     if (!dev) {
//         return 1;
//     }
//     // from zephr drivers
//     struct rtc_time timeptr = {
//         .tm_hour = hour, 
//         .tm_min = min, 
//         .tm_sec = sec
//     };
//     return rtc_set_time(dev, &timeptr); //zehpr func
// }

// static int cmd_time_set(const struct shell *shell, size_t argc, char **argv) {
//     if (argc != 4) {
//         shell_print(shell, "input: set_time <hour> <min> <sec>");
//         return 1;
//     }    
 
//     int hour = atoi(argv[1]);
//     int min = atoi(argv[2]);
//     int sec = atoi(argv[3]);
    
//     if (set_time(hour, min, sec) == 0) {
//         shell_print(shell, "time set to %02d:%02d:%02d", hour, min, sec);
//         printk("time set to %02d:%02d:%02d\n", hour, min, sec);
//     } else {
//         shell_print(shell, "error time not set.");
//     }
//     return 0; 
// }


 
// SHELL_CMD_REGISTER(set_time, NULL, "Set RTC time", cmd_time_set);
