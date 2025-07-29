// #include <zephyr/kernel.h>
// #include <zephyr/drivers/sensor.h>
// #include <zephyr/logging/log.h>
// #include "veml6030.h"

// LOG_MODULE_REGISTER(veml6030, CONFIG_SENSOR_LOG_LEVEL);

// static const struct device *veml_dev;

// int veml6030_init(void)
// {
//     veml_dev = DEVICE_DT_GET(DT_NODELABEL(veml6030));
    
//     if (!device_is_ready(veml_dev)) {
//         LOG_ERR("VEML6030 device not ready");
//         return -ENODEV;
//     }
    
//     LOG_INF("VEML6030 initialized");
//     return 0;
// }

// int veml6030_read(struct sensor_value *light)
// {
//     int ret = sensor_sample_fetch(veml_dev);
//     if (ret < 0) {
//         LOG_ERR("Failed to fetch VEML6030 data (err %d)", ret);
//         return ret;
//     }
    
//     ret = sensor_channel_get(veml_dev, SENSOR_CHAN_LIGHT, light);
//     if (ret < 0) {
//         LOG_ERR("Failed to get light channel (err %d)", ret);
//     }
    
//     return ret;
// }

// void veml6030_print(const struct sensor_value *light)
// {
//     printk("Light: %d lux\n", light->val1);
// }