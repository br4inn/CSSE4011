#include "bme280.h"

const struct device *const bme280_dev = DEVICE_DT_GET_ANY(bosch_bme280);
SENSOR_DT_READ_IODEV(iodev, DT_COMPAT_GET_ANY_STATUS_OKAY(bosch_bme280),
	{ SENSOR_CHAN_AMBIENT_TEMP, 0 },
	{ SENSOR_CHAN_HUMIDITY, 0 },
	{ SENSOR_CHAN_PRESS, 0 });

RTIO_DEFINE(ctx, 1, 1);

const struct device *check_bme280_device(void)
{
	if (bme280_dev == NULL) {
		printk("\nError: no BME280 device found.\n");
		return NULL;
	}
	if (!device_is_ready(bme280_dev)) {
		printk("\nError: Device \"%s\" not ready.\n", bme280_dev->name);
		return NULL;
	}
	printk("Found device \"%s\", ready to read sensor data.\n", bme280_dev->name);
	return bme280_dev;
}

int bme280_read_and_print(void)
{
	uint8_t buf[128];
	int rc = sensor_read(&iodev, &ctx, buf, sizeof(buf));
	if (rc != 0) {
		printk("%s: sensor_read() failed: %d\n", bme280_dev->name, rc);
		return rc;
	}

	const struct sensor_decoder_api *decoder;
	rc = sensor_get_decoder(bme280_dev, &decoder);
	if (rc != 0) {
		printk("%s: sensor_get_decoder() failed: %d\n", bme280_dev->name, rc);
		return rc;
	}

	uint32_t temp_fit = 0, press_fit = 0, hum_fit = 0;
	struct sensor_q31_data temp_data = {0}, press_data = {0}, hum_data = {0};

	decoder->decode(buf, (struct sensor_chan_spec){SENSOR_CHAN_AMBIENT_TEMP, 0}, &temp_fit, 1, &temp_data);
	decoder->decode(buf, (struct sensor_chan_spec){SENSOR_CHAN_PRESS, 0}, &press_fit, 1, &press_data);
	decoder->decode(buf, (struct sensor_chan_spec){SENSOR_CHAN_HUMIDITY, 0}, &hum_fit, 1, &hum_data);

	printk("temp: %s%d.%d; press: %s%d.%d; humidity: %s%d.%d\n",
		PRIq_arg(temp_data.readings[0].temperature, 6, temp_data.shift),
		PRIq_arg(press_data.readings[0].pressure, 6, press_data.shift),
		PRIq_arg(hum_data.readings[0].humidity, 6, hum_data.shift));

	return 0;
}
