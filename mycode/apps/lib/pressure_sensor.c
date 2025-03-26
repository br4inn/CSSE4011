#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

const struct device *pressure_dev = DEVICE_DT_GET(DT_ALIAS(lps22hb));
