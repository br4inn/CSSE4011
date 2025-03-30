#ifndef SAMPLING_H
#define SAMPLING_H

#include <zephyr/kernel.h>
 
extern struct k_msgq sampling_rate_msgq;


K_MSGQ_DEFINE(sampling_rate_msgq, MSGQ_MSG_SIZE, MSGQ_MAX_MSGS, 4);

struct sampling {
    const char *did;
    int rtc_time;
    int value;
};

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