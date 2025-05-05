/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
*/

#include <zephyr/drivers/sensor.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>

const struct device *const ultrasonic = DEVICE_DT_GET_ONE(hc_sr04);

int payload[3] = {0xAB, 0x00, 0x00};

const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	BT_DATA(BT_DATA_MANUFACTURER_DATA, payload, sizeof(payload)),
};

static int process_sample(const struct device *dev) {
	struct sensor_value dist;

	if (sensor_sample_fetch(dev) < 0) {
		printk("Sensor sample update error\n");
		return -1;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_DISTANCE, &dist) < 0) {
		printk("Cannot read HC_SR04 distance channel\n");
		return -1;
	}

	/* display distance */
	int dist_cm = (dist.val1 * 100) + (dist.val2 / 10000);
	printk("Distance: %d cm\n", dist_cm);
	if (dist_cm < 60) {
		return dist_cm;
	} else {
		return -1;
	}
}

int main(void) {
	if (!device_is_ready(ultrasonic)) {
		printk("Device %s is not ready\n", ultrasonic->name);
		return 0;
	}

	int err = bt_enable(NULL);
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return 0;
    }
    printk("Bluetooth ready\n");

	/* Start advertising */
	err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, ad, ARRAY_SIZE(ad),
		NULL, 0);

	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return 0;
	}

	while (1) {
		payload[2] = process_sample(ultrasonic);

		struct bt_data ad1[] = {
			BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
			BT_DATA(BT_DATA_MANUFACTURER_DATA, payload, sizeof(payload)),
		};

		int data = bt_le_adv_update_data(ad1, ARRAY_SIZE(ad1), NULL, 0);
		if (data) {
			printk("(err %d)\n", data);
		}

		k_msleep(250);
	}
	
	return 0;
}
