/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
*/

#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/data/json.h>
#include <zephyr/sys/slist.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include <string.h>
#include <kalman_filter.h>
#include <math.h>

#define ROWS 8
#define COLS 2

char* mobile_mac = "D1:31:A2:EB:63:2A";
char* ultra_mac = "CF:E1:0A:1C:80:C7";

typedef struct {
    char name;
    uint16_t major;
    uint16_t minor;
    char address[18];
    float x, y;
	char left;
	char right;
} BeaconInfo;

typedef struct {
    sys_dnode_t node;
    BeaconInfo info;
} BeaconNode;

struct data_item_t {
    void *fifo_reserved;   /* 1st word reserved for use by FIFO */
    float x;
	float y;
	float vel;
	float tot_dist;
};

struct JSONPosData {
	int A, B, C, D, E, F, G, H, I, J, K, L, M;
	int x, y, vel, tot_dist;
};

static BeaconInfo beacon_nodes[13] = {
	{ .name = 'A', .address = "F5:75:FE:85:34:67", .major = 0,  .minor = 0, .x = 0, .y = 0, .left = '-', .right = '-' },
	{ .name = 'B', .address = "E5:73:87:06:1E:86", .major = 0, .minor = 0, .x = 1.5, .y = 0, .left = '-', .right = '-' },
	{ .name = 'C', .address = "CA:99:9E:FD:98:B1", .major = 0, .minor = 0, .x = 3, .y = 0 , .left = '-', .right = '-'},
	{ .name = 'D', .address = "CB:1B:89:82:FF:FE", .major = 0, .minor = 0, .x = 3, .y = 2 , .left = '-', .right = '-'},
	{ .name = 'E', .address = "D4:D2:A0:A4:5C:AC", .major = 0, .minor = 0, .x = 3, .y = 4 , .left = '-', .right = '-'},
	{ .name = 'F', .address = "C1:13:27:E9:B7:7C", .major = 0,  .minor = 0, .x = 1.5, .y = 4 , .left = '-', .right = '-'},
	{ .name = 'G', .address = "F1:04:48:06:39:A0", .major = 0, .minor = 0, .x = 0, .y = 4 , .left = '-', .right = '-'},
	{ .name = 'H', .address = "CA:0C:E0:DB:CE:60", .major = 0, .minor = 0, .x = 0, .y = 2 , .left = '-', .right = '-'},
	{ .name = 'I', .address = "D4:7F:D4:7C:20:13", .major = 0, .minor = 0, .x = 4.5, .y = 0 , .left = '-', .right = '-'},
	{ .name = 'J', .address = "F7:0B:21:F1:C8:E1", .major = 0,  .minor = 0, .x = 6, .y = 0 , .left = '-', .right = '-'},
	{ .name = 'K', .address = "FD:E0:8D:FA:3E:4A", .major = 0, .minor = 0, .x = 6, .y = 2 , .left = '-', .right = '-'},
	{ .name = 'L', .address = "EE:32:F7:28:FA:AC", .major = 0, .minor = 0, .x = 6, .y = 4 , .left = '-', .right = '-'},
	{ .name = 'M', .address = "EA:E4:EC:C2:0C:18", .major = 0, .minor = 0, .x = 4.5, .y = 4 , .left = '-', .right = '-'}
};

static sys_dlist_t beacon_list;

struct k_fifo pos_queue;

K_MUTEX_DEFINE(dlist_mutex);
K_MUTEX_DEFINE(nodes_mutex);
K_MUTEX_DEFINE(rssi_mutex);
K_MUTEX_DEFINE(ultra_mutex);

int8_t rssi_values[13] = {[0 ... 12] = -120};

static int dist = -1;
static int ultra_node_index = -1;

#define ALPHA 2.0

float rssi_to_distance(int index, int rssi) {
    float t = -60.0;
    return powf(10.0, (t - rssi) / (10.0 * ALPHA));
}

void calc_least_squares(float A[ROWS][COLS], float B[ROWS], float* x, float* y) {
    // Transpose of matrix A
    float A_t[COLS][ROWS];
    for (int i = 0; i < COLS; i++) {
        for (int j = 0; j < ROWS; j++) {
            A_t[i][j] = A[j][i];
        }
    }

    // A_t * A
    float A_p[2][2] = {{0, 0}, {0, 0}};
    for (int k = 0; k < 2; k++) {
        for (int j = 0; j < 2; j++) {
            for (int i = 0; i < ROWS; i++) {
                A_p[k][j] += A_t[k][i] * A[i][j];
            }
        }
    }

    // A_t * B
    float B_p[2] = {0, 0};
    for (int j = 0; j < 2; j++) {
        for (int i = 0; i < ROWS; i++) {
            B_p[j] += A_t[j][i] * B[i];
        }
    }

    // Inverse of A_p
    float A_i[2][2] = {{A_p[1][1], -1.0 * A_p[0][1]},
        {-1.0 * A_p[1][0], A_p[0][0]}};
    float det = (A_p[0][0] * A_p[1][1]) - (A_p[0][1] * A_p[1][0]);
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            A_i[i][j] *= 1.0 / det;
        }
    }

    // x = A_i * B
    float position[2] = {0, 0};
    for (int j = 0; j < 2; j++) {
        for (int i = 0; i < 2; i++) {
            position[j] += A_i[j][i] * B_p[i];
        }
    }

    *x = position[0];
    *y = position[1];
}

static int cmd_add(const struct shell *sh, size_t argc, char **argv) {
    if (argc != 4) {
        shell_print(sh, "Usage: add <NAME> <MAJOR> <MINOR>");
        return -EINVAL;
    }
	
	if (argv[1][0] < 'A' || argv[1][0] > 'M') {
		shell_error(sh, "Unknown beacon name");
        return -ENOENT;
	}
	int index = argv[1][0] - 0x41;

	int major = (uint16_t) atoi(argv[2]);
    int minor = (uint16_t) atoi(argv[3]);

	if (major == 0 || minor == 0) {
		shell_error(sh, "Invalid major/minor value");
		return -EINVAL;
	}
	
	k_mutex_lock(&nodes_mutex, K_FOREVER);
	beacon_nodes[index].major = major;
	beacon_nodes[index].minor = minor;

	BeaconNode* node = k_malloc(sizeof(BeaconNode));
	node->info = beacon_nodes[index];
	k_mutex_unlock(&nodes_mutex);

	k_mutex_lock(&dlist_mutex, K_FOREVER);
    sys_dlist_append(&beacon_list, &node->node);

	BeaconNode *prev = CONTAINER_OF(sys_dlist_peek_prev(&beacon_list, &node->node), BeaconNode, node);
	BeaconNode *next = CONTAINER_OF(sys_dlist_peek_next(&beacon_list, &node->node), BeaconNode, node);

    if (prev) {
        node->info.left = prev->info.name;
        prev->info.right = node->info.name;
    } else {
        node->info.left = '-';
    }

    if (next) {
        node->info.right = next->info.name;
        next->info.left = node->info.name;
    } else {
        node->info.right = '-';
    }

    shell_print(sh, "Beacon %c added.", node->info.name);
	k_mutex_unlock(&dlist_mutex);
    return 0;
}

static int cmd_add_ultra(const struct shell *sh, size_t argc, char **argv) {
	if (argv[1][0] == 'X') {
		k_mutex_lock(&ultra_mutex, K_FOREVER);
		shell_print(sh, "Ultrasonic node cleared");
		ultra_node_index = -1;
		k_mutex_unlock(&ultra_mutex);
		return 0;
	}
	if (argv[1][0] < 'A' || argv[1][0] > 'M') {
		shell_error(sh, "Unknown beacon name");
        return -EINVAL;
	}
	k_mutex_lock(&ultra_mutex, K_FOREVER);
	ultra_node_index = argv[1][0] - 0x41;
	k_mutex_unlock(&ultra_mutex);
    shell_print(sh, "Ultrasonic node set to %c", argv[1][0]);
    return 0;
}

static int cmd_list(const struct shell *sh, size_t argc, char **argv) {
    if (argc == 2 && strcmp(argv[1], "-a") == 0) {
        shell_print(sh, "Registered iBeacons:");
        BeaconNode *node;
		k_mutex_lock(&dlist_mutex, K_FOREVER);
        SYS_DLIST_FOR_EACH_CONTAINER(&beacon_list, node, node) {
			char x_pos[10], y_pos[10];
			snprintf(x_pos, 10, "%d.%02d", (int) node->info.x, (int)((node->info.x - ((int) node->info.x)) * 100));
			snprintf(y_pos, 10, "%d.%02d", (int) node->info.y, (int)((node->info.y - ((int) node->info.y)) * 100));
            shell_print(sh, "  %c: MAC=%s MAJOR=%d MINOR=%d POS=(%s, %s) LEFT=%c RIGHT=%c",
                node->info.name, node->info.address, node->info.major, node->info.minor,
                x_pos, y_pos, node->info.left, node->info.right);
        }
		k_mutex_unlock(&dlist_mutex);
    } else if (argc == 2 && strlen(argv[1]) == 1 && argv[1][0] >= 'A' && argv[1][0] <= 'M') {
        char name = argv[1][0];
        BeaconNode *node;
		k_mutex_lock(&dlist_mutex, K_FOREVER);
        SYS_DLIST_FOR_EACH_CONTAINER(&beacon_list, node, node) {
			char x_pos[10], y_pos[10];
			snprintf(x_pos, 10, "%d.%02d", (int) node->info.x, (int)((node->info.x - ((int) node->info.x)) * 100));
			snprintf(y_pos, 10, "%d.%02d", (int) node->info.y, (int)((node->info.y - ((int) node->info.y)) * 100));
            if (node->info.name == name) {
                shell_print(sh, "%c: MAC=%s MAJOR=%d MINOR=%d POS=(%s, %s) LEFT=%c RIGHT=%c",
                    node->info.name, node->info.address, node->info.major, node->info.minor,
                    x_pos, y_pos, node->info.left, node->info.right);
				k_mutex_unlock(&dlist_mutex);
                return 0;
            }
        }
		k_mutex_unlock(&dlist_mutex);
        shell_error(sh, "Beacon %c not found", name);
        return -ENOENT;
    } else {
        shell_print(sh, "Usage: ibeacon list <A-M> | -a");
        return -EINVAL;
    }

    return 0;
}

static int cmd_clear(const struct shell *sh, size_t argc, char **argv) {
    if (argc != 2) {
        shell_print(sh, "Usage: clear <NAME> or clear -a");
        return -EINVAL;
    }

    if (strcmp(argv[1], "-a") == 0) {
        BeaconNode *node, *tmp;
		k_mutex_lock(&dlist_mutex, K_FOREVER);
        SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&beacon_list, node, tmp, node) {
            sys_dlist_remove(&node->node);
            k_free(node);
        }
		k_mutex_unlock(&dlist_mutex);
        shell_print(sh, "Cleared all nodes.");
        return 0;
    }

    char target = argv[1][0];
    if (target < 'A' || target > 'M') {
        shell_error(sh, "Invalid beacon name.");
        return -EINVAL;
    }

    BeaconNode *node, *tmp;
	k_mutex_lock(&dlist_mutex, K_FOREVER);
    SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&beacon_list, node, tmp, node) {
        if (node->info.name == target) {
            BeaconNode *prev = CONTAINER_OF(sys_dlist_peek_prev(&beacon_list, &node->node), BeaconNode, node);
			BeaconNode *next = CONTAINER_OF(sys_dlist_peek_next(&beacon_list, &node->node), BeaconNode, node);

            if (prev) {
                prev->info.right = next ? next->info.name : '-';
            }
            if (next) {
                next->info.left = prev ? prev->info.name : '-';
            }

            sys_dlist_remove(&node->node);
            k_free(node);
            shell_print(sh, "Removed beacon %c.", target);
			k_mutex_unlock(&dlist_mutex);
            return 0;
        }
    }
	k_mutex_unlock(&dlist_mutex);
    shell_error(sh, "Beacon %c not found.", target);
    return -ENOENT;
}

SHELL_CMD_ARG_REGISTER(list, NULL, "List beacons: list <A-M> or list -a", cmd_list, 2, 0);
SHELL_CMD_ARG_REGISTER(ultra, NULL, "Add Ultra node", cmd_add_ultra, 2, 0);
SHELL_CMD_ARG_REGISTER(add, NULL, "Add beacon node", cmd_add, 4, 0);
SHELL_CMD_ARG_REGISTER(clear, NULL, "Clear a beacon: clear <A-M> or clear -a", cmd_clear, 2, 0);

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
	struct net_buf_simple *ad) {
	char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	
	char mac_addr[18];
	strncpy(mac_addr, addr_str, 17);
	mac_addr[17] = '\0';
	
	int cmp = strcmp(mac_addr, mobile_mac);
	if (cmp == 0) {
		if (ad->len == 30) {
			k_mutex_lock(&rssi_mutex, K_FOREVER);
			for (int i = 9; i < 21; i++) {
				rssi_values[i - 9] = (int8_t) ad->data[i];
			}
			k_mutex_unlock(&rssi_mutex);
			return;
		}
	}

	cmp = strcmp(mac_addr, ultra_mac);
	if (cmp == 0) {
		if (ad->len > 13) {
			k_mutex_lock(&ultra_mutex, K_FOREVER);
			dist = (int8_t) ad->data[13];
			k_mutex_unlock(&ultra_mutex);
		}
	}
}

void thread_json(void) {
	struct data_item_t* recv;

	const struct json_obj_descr allPosData[] = {
        JSON_OBJ_DESCR_PRIM(struct JSONPosData, A, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct JSONPosData, B, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct JSONPosData, C, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct JSONPosData, D, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct JSONPosData, E, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct JSONPosData, F, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct JSONPosData, G, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct JSONPosData, H, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct JSONPosData, I, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct JSONPosData, J, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct JSONPosData, K, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct JSONPosData, L, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct JSONPosData, M, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct JSONPosData, x, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct JSONPosData, y, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct JSONPosData, vel, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM(struct JSONPosData, tot_dist, JSON_TOK_NUMBER),
    };

    while (1) {
        recv = k_fifo_get(&pos_queue, K_FOREVER);

		int x = (int)(recv->x * 100);
		int y = (int)(recv->y * 100);
		int vel = (int)(recv->vel * 100);
		int tot_dist = (int)(recv->tot_dist * 100);

		k_mutex_lock(&rssi_mutex, K_FOREVER);
		struct JSONPosData data = {
			.A = rssi_values[0],
			.B = rssi_values[1],
			.C = rssi_values[2],
			.D = rssi_values[3],
			.E = rssi_values[4],
			.F = rssi_values[5],
			.G = rssi_values[6],
			.H = rssi_values[7],
			.I = rssi_values[8],
			.J = rssi_values[9],
			.K = rssi_values[10],
			.L = rssi_values[11],
			.M = rssi_values[12],
			.x = x,
			.y = y,
			.vel = vel,
			.tot_dist = tot_dist
		};
		k_mutex_unlock(&rssi_mutex);

		char jsonBuffer[300];
        int ret = json_obj_encode_buf(allPosData,
            ARRAY_SIZE(allPosData),
            &data, jsonBuffer, sizeof(jsonBuffer));
		
		if (ret != 0) {
			printk("JSON error\n");
		} else {
			printk("%s\n", jsonBuffer);
		}

		k_msleep(10);
    }
}

void thread_pos(void) {
	float distances[13] = {[0 ... 12] = -1.0};
	bool exists[13] = {[0 ... 12] = false};

    float x, y;
	float x_hat[4] = {0, 0, 0, 0};
    float cov[4][4] = {
        {10, 0, 0, 0},
        {0, 10, 0, 0},
        {0, 0, 10, 0},
        {0, 0, 0, 10}
    };

	// For sending x and y to fifo
	struct data_item_t p = {
		.x = 0,
		.y = 0,
		.vel = 0,
		.tot_dist = 0
	};

	while (1) {
		k_msleep(100);

		k_mutex_lock(&dlist_mutex, K_FOREVER);
		int exists_len = 0;
		if (exists_len != 8) {
			BeaconNode *node, *tmp;
    		SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&beacon_list, node, tmp, node) {
        		int index = node->info.name - 0x41;
				exists[index] = true;
				exists_len += 1;
    		}
		}
		k_mutex_unlock(&dlist_mutex);

		if (exists_len != 8) {
			continue;
		}

		k_mutex_lock(&rssi_mutex, K_FOREVER);
		
		for (int i = 0; i < 13; i++) {
			if (exists[i] == true) {
				distances[i] = rssi_to_distance(i, rssi_values[i]);
			} else {
				distances[i] = -1;
			}
		}
		
		// form matrix A and B
		int node_num = 0;
		float A[ROWS][COLS], B[ROWS];
		int min_index = -1;
		for (int i = 0; i < 13; i++) {

			if (exists[i] != true) {
				continue;
			}

			if (min_index == -1) {
				min_index = i;
			}
			
			A[node_num][0] = 2 * (beacon_nodes[i].x - beacon_nodes[min_index].x);
        	A[node_num][1] = 2 * (beacon_nodes[i].y - beacon_nodes[min_index].y);

       		B[node_num] = powf(distances[min_index], 2) - powf(distances[i], 2) - powf(beacon_nodes[min_index].x, 2) - 
            	powf(beacon_nodes[min_index].y, 2) + powf(beacon_nodes[i].x, 2) + powf(beacon_nodes[i].y, 2);
			
			node_num += 1;
		}

		k_mutex_unlock(&rssi_mutex);

		calc_least_squares(A, B, &x, &y);

		// calc error
		float x_var = 4.0;
		float y_var = x_var;
		float co_var = 0;

		float pos[2] = {x, y};
		
		kalman_filter(x_hat, cov, x_var, y_var, co_var, pos);

		k_mutex_lock(&ultra_mutex, K_FOREVER);
		if (dist != -1 && ultra_node_index != -1) {
			// find ultrasound coord
			float ultra_dist = dist / 100.0;

			char node = ultra_node_index + 0x41;
			
			if (node == 'B') {
				pos[0] = 1.5;
				pos[1] = ultra_dist;
			} else if (node == 'F') {
				pos[0] = 1.5;
				pos[1] = 4 - ultra_dist;
			} else if (node == 'I') {
				pos[0] = 4.5;
				pos[1] = ultra_dist;
			} else if (node == 'M') {
				pos[0] = 4.5;
				pos[1] = 4 - ultra_dist;
			} else if (node == 'H') {
				pos[0] = ultra_dist;
				pos[1] = 2.0;
			}
			kalman_filter(x_hat, cov, x_var, y_var, co_var, pos);
		}
		k_mutex_unlock(&ultra_mutex);

		p.x = x_hat[0];
		p.y = x_hat[1];
		p.vel = sqrtf(powf(x_hat[2], 2.0) + powf(x_hat[3], 2.0));
		p.tot_dist += sqrtf(powf(x_hat[0], 2.0) + powf(x_hat[1], 2.0));

		if (p.x > 3 || p.x < 0 || p.y < 0 || p.y > 4) {
			continue;
		} 
		k_fifo_put(&pos_queue, &p);
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
		.options    = BT_LE_SCAN_OPT_NONE,
		.interval   = BT_GAP_SCAN_FAST_INTERVAL,
		.window     = BT_GAP_SCAN_FAST_WINDOW,
	};

	err = bt_le_scan_start(&scan_param, device_found);
	if (err) {
		printk("Start scanning failed (err %d)\n", err);
		return 0;
	}
	printk("Started scanning...\n");

	k_fifo_init(&pos_queue);
	sys_dlist_init(&beacon_list);
}

K_THREAD_DEFINE(pos_id, 2048, thread_pos,
    NULL, NULL, NULL, 5, 0, 50);

K_THREAD_DEFINE(json_id, 2048, thread_json,
	NULL, NULL, NULL, 5, 0, 50);