#include "rtc.h"
static const struct device *dev;


LOG_MODULE_REGISTER(rtc_module, LOG_LEVEL_DBG);


int rtc_init(void) {
    dev = DEVICE_DT_GET(DT_INST(0, st_stm32_rtc)); //board uses stm32
    if (!device_is_ready(dev)) {
        return 1;
    }
    return 0;
}

int set_time(int hour, int min, int sec) {
    if (!dev) {
        return 1;
    }
    // from zephr drivers
    struct rtc_time timeptr = {
        .tm_hour = hour, 
        .tm_min = min, 
        .tm_sec = sec
    };
    return rtc_set_time(dev, &timeptr); //zehpr func
}

static int cmd_time_set(const struct shell *shell, size_t argc, char **argv) {
    if (argc != 4) {
        shell_print(shell, "input: set_time <hour> <min> <sec>");
        return 1;
    }    
 
    int hour = atoi(argv[1]);
    int min = atoi(argv[2]);
    int sec = atoi(argv[3]);
    
    if (set_time(hour, min, sec) == 0) {
        shell_print(shell, "time set to %02d:%02d:%02d", hour, min, sec);
        printk("time set to %02d:%02d:%02d\n", hour, min, sec);
    } else {
        shell_print(shell, "error time not set.");
    }
    return 0; 
}


 
SHELL_CMD_REGISTER(set_time, NULL, "Set RTC time", cmd_time_set);
