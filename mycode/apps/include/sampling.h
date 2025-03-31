#ifndef SAMPLING_H
#define SAMPLING_H

#include <zephyr/kernel.h>
 
extern struct k_msgq sampling_rate_msgq;
#define MSGQ_MAX_MSGS 10
#define MSGQ_MSG_SIZE sizeof(int)
 
// struct sampling {
//     const char *did;
//     int rtc_time;
//     int value;
// };

struct sampling_ctl {
    bool ctn_sampling_on;
    bool ctn_temp_sampling_on;
    bool ctn_hum_sampling_on;
    bool ctn_pressure_sampling_on;
    bool ctn_mag_sampling_on;
 //   struct sampling sampling_val;
};

extern struct sampling_ctl sampling_settings;


#endif // SAMPLING_H