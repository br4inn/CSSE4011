#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include "ens160.h"

LOG_MODULE_REGISTER(ens160_app, CONFIG_SENSOR_LOG_LEVEL);

/* Check if ENS160 is available and ready */
const struct device *check_ens160_device(void)
{
    const struct device *dev = DEVICE_DT_GET_ONE(sciosense_ens160);

    if (!device_is_ready(dev)) {
        LOG_ERR("ENS160 device not ready");
        return NULL;
    }

    LOG_INF("ENS160 device ready");
    return dev;
}

/* Read ENS160 values and print them */
int ens160_read_and_print(const struct device *dev)
{
    if (dev == NULL) {
        LOG_ERR("Device is NULL");
        return -EINVAL;
    }

    struct sensor_value aqi, eco2, tvoc;
    int ret;

    /* Fetch all channels */
    ret = sensor_sample_fetch(dev);
    if (ret < 0) {
        LOG_ERR("Failed to fetch ENS160 data: %d", ret);
        return ret;
    }

    /* Get individual values */
    ret = sensor_channel_get(dev, SENSOR_CHAN_ENS160_AQI, &aqi);
    if (ret < 0) {
        LOG_ERR("Failed to get AQI: %d", ret);
        return ret;
    }

    ret = sensor_channel_get(dev, SENSOR_CHAN_CO2, &eco2);
    if (ret < 0) {
        LOG_ERR("Failed to get eCO2: %d", ret);
        return ret;
    }

    ret = sensor_channel_get(dev, SENSOR_CHAN_VOC, &tvoc);
    if (ret < 0) {
        LOG_ERR("Failed to get TVOC: %d", ret);
        return ret;
    }

	//prinkt("Raw AQI q31: %d", aqi_data.readings[0].val1);
    LOG_INF("ENS160 - AQI: %d, eCO2: %d ppm, TVOC: %d ppb",
            aqi.val1, eco2.val1, tvoc.val1);

    return 0;
}