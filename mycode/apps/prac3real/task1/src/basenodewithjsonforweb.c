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

struct OutputData { 
    int node_id;       
    int8_t rssi[13];   
};

struct Position {
    int x;
    int y;
    int node_id;
    int8_t rssi;  
    int avg_vel;
    int dist_travelled; 
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

static int8_t rssi_values[13] = {
	-120,
	-120,
	-120,
	-120,
	-120,
	-120,
	-120,
	-120,
	-40,
	-40,
	-40,
	-20,
	-10,
};

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

int current_node_id;

void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
    struct net_buf_simple *ad) {
    char addr_str[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
    
    char mac_addr[18];
    strncpy(mac_addr, addr_str, 17);
    mac_addr[17] = '\0';
    
    int cmp = strcmp(mac_addr, mobile_mac);
    if (cmp == 0) {
        if (ad->len == 30) {
            int strongest_rssi = -120;  // Initialize with the lowest possible RSSI
            int strongest_node_id = -1;

            for (int i = 9; i < 21; i++) { // Iterate over nodes A–L
                int8_t node_rssi = (int8_t) ad->data[i];
                rssi_values[i - 9] = node_rssi;

                // Find the node with the strongest RSSI
                if (node_rssi > strongest_rssi) {
                    strongest_rssi = node_rssi;
                    strongest_node_id = i;
                }
            }

            // Update current_node_id to the node with the strongest RSSI
            if (strongest_node_id != -1) {
                current_node_id = strongest_node_id;
                printk("Strongest Node ID: %d, RSSI: %d\n", current_node_id, strongest_rssi);
            } else {
                printk("No valid node detected.\n");
            }
            return;
        } else {
            printk("Unexpected advertisement data length: %u\n", ad->len);
        }
    }

    cmp = strcmp(mac_addr, ultra_mac);
    if (cmp == 0) {
        if (ad->len > 13) {
            dist = (int8_t) ad->data[13];
            printk("Distance: %d cm\n", dist);
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

int main(void) {
	if (!device_is_ready(button.port)) {
		printk("Error: button device not ready\n");
		return 0;
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
        JSON_OBJ_DESCR_PRIM(struct Position, node_id, JSON_TOK_NUMBER),
        JSON_OBJ_DESCR_PRIM(struct Position, rssi, JSON_TOK_NUMBER),
        JSON_OBJ_DESCR_PRIM(struct Position, avg_vel, JSON_TOK_NUMBER),
        JSON_OBJ_DESCR_PRIM(struct Position, dist_travelled, JSON_TOK_NUMBER),
	};	


    float prev_x = 0.0f, prev_y = 0.0f;   
    float total_distance = 0.0f;        
    uint32_t start_time = k_uptime_get();  

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

        // Calc distance traveled
        float current_x = kf.x[0];
        float current_y = kf.x[1];
        float distance = sqrtf((current_x - prev_x) * (current_x - prev_x) +
                            (current_y - prev_y) * (current_y - prev_y));

        total_distance += distance;
        prev_x = current_x;
        prev_y = current_y;

        // Calc average velocity
        uint32_t elapsed_time = k_uptime_get() - start_time; 
        float average_velocity = total_distance / (elapsed_time / 1000.0f); // ms to secs
 
        int int_distance = (int)distance;
        int int_velocity = (int)average_velocity;
        pos.avg_vel = (int)average_velocity;
        pos.dist_travelled = (int)total_distance;

        pos.node_id = current_node_id;
        if (current_node_id >= 9 && current_node_id <= 21) {
            pos.rssi = rssi_values[current_node_id - 9];
        } else {
            pos.rssi = -120;   
        }  

        char json_out[128];
		int ret = json_obj_encode_buf(pos_descr, ARRAY_SIZE(pos_descr), &pos, json_out, sizeof(json_out));
		
		if (ret == 0) {
			printk("%s\n", json_out);  // Example: {"x":153,"y":274}
		} else {
			printk("JSON encode error: %d\n", ret);
		}
              
		k_msleep(250);

	}

}