// bme280.h

#ifndef BME280_H
#define BME280_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_data_types.h>
#include <zephyr/rtio/rtio.h>


// Expose the sensor device
extern const struct device *const bme280_dev;

// Define RTIO context
RTIO_DEFINE(bme280_ctx, 1, 1);

// Expose RTIO context
#define BME280_CTX bme280_ctx
 

// Function to validate and return the sensor device
const struct device *check_bme280_device(void);

#endif // BME280_H
