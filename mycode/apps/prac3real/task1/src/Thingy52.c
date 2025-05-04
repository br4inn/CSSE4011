#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/conn.h>
#include <string.h>

#define BT_UUID_IBEACON_SERVICE_VAL \
	BT_UUID_128_ENCODE(0xf0debc9a, 0x7856, 0x3412, 0xabcd, 0xef0987654321)

#define BT_UUID_IBEACON_CHAR_VAL \
	BT_UUID_128_ENCODE(0xf0debc9a, 0x7856, 0x3412, 0xabcd, 0xef0987654322)

static struct bt_conn *default_conn;
static struct bt_gatt_write_params write_params;
static struct bt_gatt_subscribe_params subscribe_params;

// Device information
static struct bt_addr_le base_node_addr;
static bool connected = false;

// Callback when the connection is established
static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err == 0) {
		printk("Connected to base node\n");
		default_conn = bt_conn_ref(conn);
		connected = true;
	} else {
		printk("Failed to connect (err %u)\n", err);
	}
}

// Callback when the connection is disconnected
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason %u)\n", reason);
	connected = false;
	if (default_conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

// Callback to handle GATT writes
static ssize_t write_ibeacon(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	printk("iBeacon data received on the base node: %s\n", (char *)buf);
	return len;
}

// Function to send data to the iBeacon characteristic on the base node
static void send_ibeacon_data(const char *data)
{
	int err;

	write_params.handle = 0x000c;  // iBeacon characteristic handle (example, update after discovery)
	write_params.data = data;
	write_params.length = strlen(data);
	write_params.offset = 0;
	write_params.func = NULL;

	err = bt_gatt_write(default_conn, &write_params);
	if (err) {
		printk("Error writing to the characteristic (err %d)\n", err);
	} else {
		printk("iBeacon data sent to the base node!\n");
	}
}

// Start scanning for the base node
static void start_scan(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, NULL);
	if (err) {
		printk("Scan failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning for base node...\n");
}

// Callback when a device is found during scanning
static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	// Check if this is the base node by comparing the advertising data
	if (ad->len >= 30 && ad->data[0] == 0x4C && ad->data[1] == 0x00 &&
		ad->data[2] == 0x02 && ad->data[3] == 0x15) {
		printk("Found base node! RSSI: %d\n", rssi);

		// Save the base node's address
		bt_addr_le_copy(&base_node_addr, addr);
		bt_le_scan_stop();

		// Connect to the base node
		int err = bt_conn_le_create(&base_node_addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT, &default_conn);
		if (err) {
			printk("Failed to connect to the base node (err %d)\n", err);
		}
	}
}

// Initialize Bluetooth and start scanning
void main(void)
{
	int err;

	printk("Starting iBeacon Client\n");

	// Initialize Bluetooth
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}
	printk("Bluetooth initialized\n");

	// Start scanning for available devices
	start_scan();
}
