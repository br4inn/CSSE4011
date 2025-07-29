#ifndef ENS160_H
#define ENS160_H

#include <zephyr/drivers/sensor.h>


/* Custom ENS160 channel for AQI */
#define SENSOR_CHAN_ENS160_AQI ((enum sensor_channel)(SENSOR_CHAN_PRIV_START + 1))


/* Function declarations */
const struct device *check_ens160_device(void);
int ens160_read_and_print(const struct device *dev);

#endif /* ENS160_H */