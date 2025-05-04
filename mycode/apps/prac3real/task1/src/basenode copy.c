/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
*/

#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/data/json.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/slist.h>
#include <stdio.h>

#define MAX_BEACONS 8
#define BUTTON_NODE DT_NODELABEL(button0)

char* mobile_mac = "D1:31:A2:EB:63:2A";
char* ultra_mac = "CF:E1:0A:1C:80:C7";

typedef struct {
    float x[4];       // state: [x, y, vx, vy]
    float P[4][4];    // error covariance
    float Q[4][4];    // process noise
    float R[2][2];    // measurement noise (same for both sources)
} Kalman2D;

typedef struct {
    float x;
    float y;
} BeaconPosition;

struct Position {
    int x;
    int y;
};

BeaconPosition beacons_grid_a[MAX_BEACONS] = {
    {0.0f, 4.0f},  // A
    {1.5f, 4.0f},  // B
    {3.0f, 4.0f},  // C
    {3.0f, 2.0f},  // D
    {3.0f, 0.0f},  // E
    {1.5f, 0.0f},  // F
    {0.0f, 0.0f},  // G
    {0.0f, 2.0f},  // H
};

BeaconPosition beacons_grid_b[MAX_BEACONS] = {
    {0.0f, 4.0f},  // C
    {1.5f, 4.0f},  // I
    {3.0f, 4.0f},  // J
    {3.0f, 2.0f},  // K
    {3.0f, 0.0f},  // L
    {1.5f, 0.0f},  // M
    {0.0f, 0.0f},  // E
    {0.0f, 2.0f},  // D
};

typedef struct {
    char name[16];
    uint16_t major;
    uint16_t minor;
    char mac[18];
    float x, y;
} BeaconInfo;

typedef struct {
    sys_snode_t node;
    BeaconInfo info;
} BeaconNode;

static sys_slist_t beacon_list;

typedef struct {
    const char *name;
    const char *mac;
    float x, y;
} FixedBeacon;

static const FixedBeacon fixed_map[] = {
    {"A", "F5:75:FE:85:34:67", 0.0f, 0.0f},
    {"B", "E5:73:87:06:1E:86", 1.5f, 0.0f},
    {"C", "CA:99:9E:FD:98:B1", 3.0f, 0.0f},
    {"D", "CB:1B:89:82:FF:FE", 3.0f, 2.0f},
    {"E", "D4:D2:A0:A4:5C:AC", 3.0f, 4.0f},
    {"F", "C1:13:27:E9:B7:7C", 1.5f, 4.0f},
    {"G", "F1:04:48:06:39:A0", 0.0f, 4.0f},
    {"H", "CA:0C:E0:DB:CE:60", 0.0f, 2.0f},
    {"I", "D4:7F:D4:7C:20:13", 4.5f, 0.0f},
    {"J", "F7:0B:21:F1:C8:E1", 6.0f, 0.0f},
    {"K", "FD:E0:8D:FA:3E:4A", 6.0f, 2.0f},
    {"L", "EE:32:F7:28:FA:AC", 6.0f, 4.0f},
};

int8_t rssi_values[13] = {[0 ... 12] = -120};
int grid_a_indices[8] = {0, 1, 2, 3, 4, 5, 6, 7}; // A–H
int grid_b_indices[8] = {2, 8, 9, 10, 11, 12, 4, 3}; // C, I, J, K, L, M, E, D

int dist = -1;

char current_grid = 'A';
BeaconPosition *active_positions = beacons_grid_a;
int *grid_indices = grid_a_indices;

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(BUTTON_NODE, gpios, {0});
static struct gpio_callback button_cb_data;

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins){
    current_grid = (current_grid == 'A') ? 'B' : 'A';
    active_positions = (current_grid == 'A') ? beacons_grid_a : beacons_grid_b;
    grid_indices = (current_grid == 'A') ? grid_a_indices : grid_b_indices;

    printk("Switched to Grid %c\n", current_grid);
}

void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
	struct net_buf_simple *ad) {
	char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	// printk("Device found: %s (RSSI %d), type %u, AD data len %u\n",
	//        addr_str, rssi, type, ad->len);
	
	char mac_addr[18];
	strncpy(mac_addr, addr_str, 17);
	mac_addr[17] = '\0';
	
	int cmp = strcmp(mac_addr, mobile_mac);
	if (cmp == 0) {
		if (ad->len == 30) {
			for (int i = 9; i < 21; i++) {
				//printk("Beacon %c-RSSI %d, ", 0x41 + i - 9, (int8_t) ad->data[i]);
				rssi_values[i - 9] = (int8_t) ad->data[i];
			}
			//printk("\n");
			return;
		}
	}

	cmp = strcmp(mac_addr, ultra_mac);
	if (cmp == 0) {
		if (ad->len > 13) {
			dist = (int8_t) ad->data[13];
			//printk("Distance: %d cm\n", dist);
		}
	}
}

float rssi_to_distance(float rssi, float A, float n) {
    return powf(10.0f, (A - rssi) / (10.0f * n));
}

// Solves least squares multilateration
int multilateration_least_squares(BeaconPosition beacons[], float distances[], int count, float *x_out, float *y_out) {
    if (count < 3) return -1;

    float A[8][2];  // max 8 beacons
    float b[8];
    float x1 = beacons[0].x;
    float y1 = beacons[0].y;
    float d1 = distances[0];

    for (int i = 1; i < count; i++) {
        float xi = beacons[i].x;
        float yi = beacons[i].y;
        float di = distances[i];

        A[i - 1][0] = 2 * (xi - x1);
        A[i - 1][1] = 2 * (yi - y1);
        b[i - 1] = d1*d1 - di*di - x1*x1 + xi*xi - y1*y1 + yi*yi;
    }

    float AtA[2][2] = {{0}};
    float Atb[2] = {0};

    for (int i = 1; i < count; i++) {
        AtA[0][0] += A[i - 1][0] * A[i - 1][0];
        AtA[0][1] += A[i - 1][0] * A[i - 1][1];
        AtA[1][0] += A[i - 1][1] * A[i - 1][0];
        AtA[1][1] += A[i - 1][1] * A[i - 1][1];

        Atb[0] += A[i - 1][0] * b[i - 1];
        Atb[1] += A[i - 1][1] * b[i - 1];
    }

    float det = AtA[0][0] * AtA[1][1] - AtA[0][1] * AtA[1][0];
    if (fabsf(det) < 1e-6) return -2;

    *x_out = (Atb[0] * AtA[1][1] - Atb[1] * AtA[0][1]) / det;
    *y_out = (Atb[1] * AtA[0][0] - Atb[0] * AtA[1][0]) / det;
    return 0;
}

static void mat_add(float A[4][4], float B[4][4], float out[4][4]) {
    for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) out[i][j] = A[i][j] + B[i][j];
}

void kalman2d_init(Kalman2D *kf, float x0, float y0) {
    kf->x[0] = x0;
    kf->x[1] = y0;
    kf->x[2] = 0;
    kf->x[3] = 0;

    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            kf->P[i][j] = (i == j) ? 1.0f : 0.0f;

    // Assume some process noise
    float q = 0.01f;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            kf->Q[i][j] = (i == j) ? q : 0.0f;

    // Default measurement noise
    kf->R[0][0] = 0.5f;
    kf->R[1][1] = 0.5f;
    kf->R[0][1] = kf->R[1][0] = 0;
}

void kalman2d_predict(Kalman2D *kf, float dt) {
    float F[4][4] = {
        {1, 0, dt, 0},
        {0, 1, 0, dt},
        {0, 0, 1, 0 },
        {0, 0, 0, 1 }
    };

    float x_pred[4];
    for (int i = 0; i < 4; i++) {
        x_pred[i] = 0;
        for (int j = 0; j < 4; j++)
            x_pred[i] += F[i][j] * kf->x[j];
    }

    float FP[4][4] = {0};
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            for (int k = 0; k < 4; k++)
                FP[i][j] += F[i][k] * kf->P[k][j];

    float Ft[4][4] = {
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {dt, 0, 1, 0},
        {0, dt, 0, 1}
    };

    float newP[4][4] = {0};
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            for (int k = 0; k < 4; k++)
                newP[i][j] += FP[i][k] * Ft[k][j];

    mat_add(newP, kf->Q, kf->P);

    for (int i = 0; i < 4; i++)
        kf->x[i] = x_pred[i];
}

void kalman2d_update(Kalman2D *kf, float mx, float my, float R_meas[2][2]) {
    float H[2][4] = {
        {1, 0, 0, 0},
        {0, 1, 0, 0}
    };

    float z[2] = {mx, my};
    float y[2];
    for (int i = 0; i < 2; i++) {
        y[i] = z[i];
        for (int j = 0; j < 4; j++)
            y[i] -= H[i][j] * kf->x[j];
    }

    float S[2][2] = {0};
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 2; j++)
            for (int k = 0; k < 4; k++)
                S[i][j] += H[i][k] * kf->P[k][j];

    S[0][0] += R_meas[0][0];
    S[1][1] += R_meas[1][1];

    float K[4][2] = {0};
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 2; j++)
            for (int k = 0; k < 2; k++)
                K[i][j] += kf->P[i][k] * H[j][k] / (S[k][k] + 1e-6f);

    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 2; j++)
            kf->x[i] += K[i][j] * y[j];
}

static const FixedBeacon *lookup_fixed(const char *name) {
    for (int i = 0; i < ARRAY_SIZE(fixed_map); i++) {
        if (strcmp(name, fixed_map[i].name) == 0) {
            return &fixed_map[i];
        }
    }
    return NULL;
}

static int cmd_add(const struct shell *sh, size_t argc, char **argv) {
    if (argc != 4) {
        shell_print(sh, "Usage: add <NAME> <MAJOR> <MINOR>");
        return -EINVAL;
    }

    const FixedBeacon *fixed = lookup_fixed(argv[1]);
    if (!fixed) {
        shell_error(sh, "Unknown beacon name");
        return -ENOENT;
    }

    BeaconNode *node = k_malloc(sizeof(BeaconNode));
    if (!node) return -ENOMEM;

    strncpy(node->info.name, argv[1], sizeof(node->info.name));
    strncpy(node->info.mac, fixed->mac, sizeof(node->info.mac));
    node->info.x = fixed->x;
    node->info.y = fixed->y;
    node->info.major = (uint16_t)atoi(argv[2]);
    node->info.minor = (uint16_t)atoi(argv[3]);

    sys_slist_append(&beacon_list, &node->node);
    shell_print(sh, "Beacon %s added.", node->info.name);
    return 0;
}

static int cmd_list(const struct shell *sh, size_t argc, char **argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(sh, "Registered iBeacons:");
    BeaconNode *node;
    SYS_SLIST_FOR_EACH_CONTAINER(&beacon_list, node, node) {
        shell_print(sh, "  %s: MAC=%s MAJOR=%d MINOR=%d POS=(%.1f, %.1f)",
            node->info.name, node->info.mac, node->info.major, node->info.minor,
            node->info.x, node->info.y);
    }

    return 0;
}

static int cmd_clear(const struct shell *sh, size_t argc, char **argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    BeaconNode *node, *tmp;
    SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&beacon_list, node, tmp, node) {
        sys_slist_find_and_remove(&beacon_list, &node->node);
        k_free(node);
    }

    shell_print(sh, "Cleared all nodes.");
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_ibeacon,
    SHELL_CMD(add, NULL, "Add beacon <NAME> <MAJOR> <MINOR>", cmd_add),
    SHELL_CMD(list, NULL, "List all beacons", cmd_list),
    SHELL_CMD(clear, NULL, "Clear all beacons", cmd_clear),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(ibeacon, &sub_ibeacon, "iBeacon node list", NULL);

int main(void) {
	if (!device_is_ready(button.port)) {
		printk("Error: button device not ready\n");
		return;
	}
	
	gpio_pin_configure_dt(&button, GPIO_INPUT);
	gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);
	
	printk("Button interrupt configured for Grid switching!\n");

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

	Kalman2D kf;
	// Start with a known position (0, 0) and no velocity
	// Well actually its (0,4) or (3,4) ie. Node G or E
	kalman2d_init(&kf, 0.0f, 0.0f); 

	const struct json_obj_descr pos_descr[] = {
		JSON_OBJ_DESCR_PRIM(struct Position, x, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct Position, y, JSON_TOK_NUMBER),
	};	

	while (1) {

		// Predict step (dt = 0.25s)
		kalman2d_predict(&kf, 0.25f);

		// Prepare beacon distances
		BeaconPosition selected_beacons[MAX_BEACONS];
		float distances[MAX_BEACONS];
		int used = 0;

		for (int i = 0; i < 8; i++) {
			int idx = grid_indices[i];
			if (rssi_values[idx] > -100) {
				distances[used] = rssi_to_distance((float)rssi_values[idx], -60.0f, 2.0f);
				selected_beacons[used] = active_positions[i];
				used++;
			}
		}

		// Multilateration if valid
		float x_rssi = 0, y_rssi = 0;
		if (used >= 3 && multilateration_least_squares(selected_beacons, distances, used, &x_rssi, &y_rssi) == 0) {
			float R_rssi[2][2] = {{1.5f, 0}, {0, 1.5f}};
			kalman2d_update(&kf, x_rssi, y_rssi, R_rssi);
		}

		// Ultrasonic override
		if (dist != -1) {
			float x_ultra = ((float)dist / 100.0f);
			float y_ultra = 2.0f;
			float R_ultra[2][2] = {{0.01f, 0}, {0, 0.01f}};
			kalman2d_update(&kf, x_ultra, y_ultra, R_ultra);
		}

		// Send Kalman filtered position
		struct Position pos;
		// i couldnt get to send floats
		pos.x = (int)(kf.x[0] * 100);
		pos.y = (int)(kf.x[1] * 100);


		char json_out[64];
		int ret = json_obj_encode_buf(pos_descr, ARRAY_SIZE(pos_descr), &pos, json_out, sizeof(json_out));
		
		if (ret == 0) {
			printk("%s\n", json_out);  // Example: {"x":153,"y":274}
		} else {
			printk("JSON encode error: %d\n", ret);
		}
		

		k_msleep(250);

	}

}