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

#ifndef IBEACON_RSSI
#define IBEACON_RSSI 0xc8
#endif

static const char* beacon_mac[12] = {
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
	"EE:32:F7:28:FA:AC" // L
};

static int8_t beacon_rssi[12] = {
	-120,
	-120,
	-120,
	-120,
	-120,
	-120,
	-120,
	-120,
	-120,
	-120,
	-120,
	-120
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

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
	
	char mac_addr[17];
	for (int i = 0; i < 17; i++) {
		mac_addr[i] = addr_str[i];
	}

	for (int i = 0; i < 12; i++) {
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
					  0x00, 0x00, 0x00, 0x00,
					  0x00, 0x00, 0x00, 0x00,
					  IBEACON_RSSI) /* Calibrated RSSI @ 1m */
		};

		int data = bt_le_adv_update_data(ad1, ARRAY_SIZE(ad1), NULL, 0);
		if (data) {
			printk("(err %d)\n", data);
		}
	}
}


// keeping bottom stuff here for now

// single linked list
struct ibeacon_node_list {
    sys_snode_t node;  
    char name[16];
    bt_addr_t address;
    uint16_t major;
    uint16_t minor;
    uint8_t x;
    uint8_t y;
};




void add_beacons(void) {
    static struct ibeacon_node beacon_nodes[] = {
        { .name = "4011-A", .address = { { 0xF5, 0x75, 0xFE, 0x85, 0x34, 0x67 } }, .major = 2753,  .minor = 32998, .x = 0, .y = 0 },
        { .name = "4011-B", .address = { { 0xE5, 0x73, 0x87, 0x06, 0x1E, 0x86 } }, .major = 32975, .minor = 20959, .x = 2, .y = 0 },
        { .name = "4011-C", .address = { { 0xCA, 0x99, 0x9E, 0xFD, 0x98, 0xB1 } }, .major = 26679, .minor = 40363, .x = 4, .y = 0 },
        { .name = "4011-D", .address = { { 0xCB, 0x1B, 0x89, 0x82, 0xFF, 0xFE } }, .major = 41747, .minor = 38800, .x = 4, .y = 2 },
        { .name = "4011-E", .address = { { 0xD4, 0xD2, 0xA0, 0xA4, 0x5C, 0xAC } }, .major = 30679, .minor = 51963, .x = 4, .y = 4 },
        { .name = "4011-F", .address = { { 0xC1, 0x13, 0x27, 0xE9, 0xB7, 0x7C } }, .major = 6195,  .minor = 18394, .x = 2, .y = 4 },
        { .name = "4011-G", .address = { { 0xF1, 0x04, 0x48, 0x06, 0x39, 0xA0 } }, .major = 30525, .minor = 30544, .x = 0, .y = 4 },
        { .name = "4011-H", .address = { { 0xCA, 0x0C, 0xE0, 0xDB, 0xCE, 0x60 } }, .major = 57395, .minor = 28931, .x = 0, .y = 2 },
    };

    for (int i = 0; i < ARRAY_SIZE(beacon_nodes); i++) {
        sys_slist_append(&ibeacon_list, &beacon_nodes[i].node);
    }
}