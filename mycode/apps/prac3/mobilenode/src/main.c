/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
*/

#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <string.h>
#include <thingy52_gas_colour.h>

#ifndef IBEACON_RSSI
#define IBEACON_RSSI 0xc8
#endif

static const char* beacon_mac[13] = {
	"F5:75:FE:85:34:67", // A
	"E5:73:87:06:1E:86", // B
	"CA:99:9E:FD:98:B1", // C
	"CB:1B:89:82:FF:FE", // D
	"D4:D2:A0:A4:5C:AC", // E
	"C1:13:27:E9:B7:7C", // F
	"F1:04:48:06:39:A0", // G
	"CA:0C:E0:DB:CE:60", // H
	"D4:7F:D4:7C:20:13", // I
	"F7:0B:21:F1:C8:E1", // J
	"FD:E0:8D:FA:3E:4A", // K
	"EE:32:F7:28:FA:AC", // L
	"EA:E4:EC:C2:0C:18" // M
};

static int8_t beacon_rssi[13] = {[0 ... 12] = -120};

// Packet
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	BT_DATA_BYTES(BT_DATA_MANUFACTURER_DATA,
		      0x4c, 0x00, /* Apple */
		      0x02, 0x15, /* iBeacon */
		      0x18, 0xee, 0x15, 0x16, /* UUID[15..12] */
		      0x01, 0x6b, /* UUID[11..10] */
		      0x4b, 0xec, /* UUID[9..8] */
		      0xad, 0x96, /* UUID[7..6] */
		      0xbc, 0xb9, 0x6d, 0x16, 0x6e, 0x97, /* UUID[5..0] */
		      0x00, 0x00, /* Major */
		      0x00, 0x00, /* Minor */
		      IBEACON_RSSI) /* Calibrated RSSI @ 1m */
};

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
	struct net_buf_simple *ad) {
	char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	
	char mac_addr[18];
	for (int i = 0; i < 17; i++) {
		mac_addr[i] = addr_str[i];
	}
	mac_addr[17] = '\0';

	for (int i = 0; i < 13; i++) {
		int cmp = strcmp(mac_addr, beacon_mac[i]);
		if (cmp == 0) {
			// Found valid beacon mac address
			printk("Beacon: %c, Mac addr: %s, (RSSI %d)\n", 0x41 + i, mac_addr, rssi);

			// Update rssi
			beacon_rssi[i] = rssi;
			
			break;
		}
	}
}

int main(void) {
	int err;

	// init RGB led
	err = thingy52_rgb_init();
	if (err) {
		printk("Thingy52 rgb failed to init (err %d)\n", err);
	}
	thingy52_rgb_colour_set(MAGENTA);

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	printk("Bluetooth initialized\n");

	struct bt_le_scan_param scan_param = {
		.type       = BT_LE_SCAN_TYPE_PASSIVE,
		.options    = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
		.interval   = BT_GAP_SCAN_FAST_INTERVAL,
		.window     = BT_GAP_SCAN_FAST_WINDOW,
	};

	err = bt_le_scan_start(&scan_param, device_found);
	if (err) {
		printk("Start scanning failed (err %d)\n", err);
		return 0;
	}
	printk("Started scanning...\n");

	/* Start advertising */
	err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, ad, ARRAY_SIZE(ad),
		NULL, 0);

	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return 0;
	}

	while (1) {
		struct bt_data ad1[] = {
			BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
			BT_DATA_BYTES(BT_DATA_MANUFACTURER_DATA,
					  0x4c, 0x00, /* Apple */
					  0x02, 0x15, /* iBeacon */
					  beacon_rssi[0],
					  beacon_rssi[1],
					  beacon_rssi[2],
					  beacon_rssi[3],
					  beacon_rssi[4],
					  beacon_rssi[5],
					  beacon_rssi[6],
					  beacon_rssi[7],
					  beacon_rssi[8],
					  beacon_rssi[9],
					  beacon_rssi[10],
					  beacon_rssi[11],
					  beacon_rssi[12],
					  0x00, 0x00, 0x00,
					  0x00, 0x00, 0x00, 0x00,
					  IBEACON_RSSI) /* Calibrated RSSI @ 1m */
		};

		int data = bt_le_adv_update_data(ad1, ARRAY_SIZE(ad1), NULL, 0);
		if (data) {
			printk("(err %d)\n", data);
		}

		k_msleep(50);
	}
}