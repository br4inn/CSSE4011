#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/shell/shell.h>
#include <inttypes.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>
#include <stdio.h> 
#include "rtc.h"
#include <zephyr/storage/disk_access.h>
 
#include <zephyr/fs/fs.h>
#include <zephyr/drivers/gpio.h>
#include "temp_humidity_sensor.h" 
#include "sampling.h"
#include "pressure_sensor.h" 
#include <zephyr/logging/log.h>
#include <zephyr/data/json.h>
#define SENSOR_STACK_SIZE 4096
#define BUTTON_THREAD_STACK_SIZE 1028
#define SENSOR_PRIORITY 5
#define BUTTON_THREAD_PRIORITY 7
#define MSGQ_MAX_MSGS 10
#define MSGQ_MSG_SIZE sizeof(int)
#define SAMPLING_THREAD_STACK_SIZE 4096
K_THREAD_STACK_DEFINE(sampling_thread_stack, SAMPLING_THREAD_STACK_SIZE);
//static struct k_thread sampling_thread_data;
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/fs.h>
#define MAX_PATH 128
#define JSON_THREAD_STACK_SIZE 4096
K_THREAD_STACK_DEFINE(json_thread_stack, JSON_THREAD_STACK_SIZE);
static struct k_thread temp_json_thread_data;
static struct k_thread hum_json_thread_data; 
static struct k_thread press_json_thread_data;

K_THREAD_STACK_DEFINE(temp_thread_stack, SENSOR_STACK_SIZE);
K_THREAD_STACK_DEFINE(hum_thread_stack, SENSOR_STACK_SIZE);
K_THREAD_STACK_DEFINE(pressure_thread_stack, SENSOR_STACK_SIZE); 
 
static struct k_thread temp_thread_data;
static struct k_thread hum_thread_data;
static struct k_thread pressure_thread_data; 

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#include <zephyr/sys/mutex.h>

int global_sampling_rate = 2000; // Default  
struct k_mutex sampling_rate_mutex;
 
K_THREAD_STACK_DEFINE(sensor_stack, SENSOR_STACK_SIZE);
struct k_thread temp_humid_sensor_thread, pressure_sensor_thread;

extern void read_temp();
extern void read_hum(); 
extern void read_pressure();
extern void read_temp_continous();
extern void read_hum_continous();
extern void read_pressure_continous(); 
extern int get_latest_press_val(int *press_value); 
extern int set_rtc(int year, int month, int day, int hour, int min, int sec); 


extern const char *get_date_time(void);
extern int get_latest_hum_val(int *hum_val);

   
extern int get_latest_temp_val(int *temp_value);
static int generate_temp_json(const char *did, const char *rtc_time, int value);
 
#define SW0_NODE	DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS_OKAY(SW0_NODE)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios,
    {0});
static struct gpio_callback button_cb_data;
 

struct sampling {
    const char *did;
    const char *rtc_time;
    int value; 
}; 

struct all_sensors_data {
    const char *did;
    const char *rtc_time;
    int temp_value;
    int hum_value;
    int press_value; 
};

static const struct json_obj_descr sampling_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct sampling, did, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct sampling, rtc_time, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct sampling, value, JSON_TOK_NUMBER), 
};
 
static const struct json_obj_descr all_sensors_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct all_sensors_data, did, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct all_sensors_data, rtc_time, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct all_sensors_data, temp_value, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct all_sensors_data, hum_value, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct all_sensors_data, press_value, JSON_TOK_NUMBER), 
};

 
// #include <string.h>
// #include <zephyr/net/http/client.h>
// #include <string.h>
// #include <stdio.h>

// #include <zephyr/net/wifi.h>
// #include <zephyr/logging/log.h>
// #include <zephyr/net/wifi_mgmt.h>

#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_mgmt.h>

 
static void connect_to_wifi(void) {
    struct wifi_connect_req_params wifi_params = {
        .ssid = "Telstra1B216F",  // Replace with your SSID
        .ssid_length = strlen("Telstra1B216F"),
        .psk = "cs62spe9xx",      // Replace with your password
        .psk_length = strlen("cs62spe9xx"),
        .security = WIFI_SECURITY_TYPE_PSK,
    };

    int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, NULL, &wifi_params, sizeof(wifi_params));
    if (ret) {
        LOG_ERR("Failed to connect to Wi-Fi: %d", ret);
    } else {
        LOG_INF("Connected to Wi-Fi");
    }
}

#include <zephyr/net/socket.h>
#include <zephyr/net/net_ip.h>
#include <stdio.h>
#include <string.h>


#define SERVER_ADDR "192.168.0.27"
#define INFLUXDB_TOKEN "9qF9f9iYm6HskdbanV7BO9yTOq6dl6QBzg67KXwMWAGzj8KNJLusuZC7Hr49OWL5syXI1WlzhOi5CoAmPUhvQQ=="
#define SERVER_PORT 8086

static int send_json_to_influxdb(const char *payload) {
    struct sockaddr_in server_addr;
    char send_buf[512];
    int sock;
    int ret;

    // Create socket
    sock = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        printk("Failed to create socket: %d\n", sock);
        return -1;
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    net_addr_pton(AF_INET, SERVER_ADDR, &server_addr.sin_addr);

    // Connect to server
    ret = zsock_connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0) {
        printk("Connection failed: %d\n", ret);
        zsock_close(sock);
        return -1;
    }

    // Build HTTP POST request
    snprintf(send_buf, sizeof(send_buf),
             "POST /api/v2/write?org=uq&bucket=seminar&precision=s HTTP/1.1\r\n"
             "Host: %s:%d\r\n"
            "Authorization: Token 9qF9f9iYm6HskdbanV7BO9yTOq6dl6QBzg67KXwMWAGzj8KNJLusuZC7Hr49OWL5syXI1WlzhOi5CoAmPUhvQQ==\r\n"
             "Content-Type: text/plain\r\n"
             "Content-Length: %zu\r\n"
             "\r\n"
             "%s",
             SERVER_ADDR, SERVER_PORT, strlen(payload), payload);

    // Send data
    ret = zsock_send(sock, send_buf, strlen(send_buf), 0);
    if (ret < 0) {
        printk("Send failed: %d\n", ret);
        zsock_close(sock);
        return -1;
    }

    printk("Data sent successfully\n");

    // Optionally receive response
    ret = zsock_recv(sock, send_buf, sizeof(send_buf) - 1, 0);
    if (ret > 0) {
        send_buf[ret] = '\0';
        printk("Response: %s\n", send_buf);
    }

    zsock_close(sock);
    return 0;
}

    
//     int sock;
//     struct sockaddr_in server_addr;
//     char send_buf[512];
//     int ret;

//     // Create socket
//     sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//     if (sock < 0) {
//         printk("Failed to create socket\n");
//         return -1;
//     }

//     // Configure server address
//     server_addr.sin_family = AF_INET;
//     server_addr.sin_port = htons(SERVER_PORT);
//     inet_pton(AF_INET, SERVER_ADDR, &server_addr.sin_addr);

//     // Connect to server
//     ret = connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
//     if (ret < 0) {
//         printk("Connection failed: %d\n", errno);
//         close(sock);
//         return -1;
//     }

//     // Build HTTP POST request
//      snprintf(send_buf, sizeof(send_buf),
//              "POST /api/v2/write?org=uq&bucket=seminar&precision=s HTTP/1.1\r\n"
//              "Host: %s:%d\r\n"
//              "Authorization: Token 9qF9f9iYm6HskdbanV7BO9yTOq6dl6QBzg67KXwMWAGzj8KNJLusuZC7Hr49OWL5syXI1WlzhOi5CoAmPUhvQQ==\r\n"
//              "Content-Type: text/plain\r\n"
//              "Content-Length: %zu\r\n"
//              "\r\n"
//              "%s",
//              SERVER_ADDR, SERVER_PORT, strlen(payload), payload);

//     // Send data
//     ret = send(sock, send_buf, strlen(send_buf), 0);
//     if (ret < 0) {
//         printk("Send failed: %d\n", errno);
//         close(sock);
//         return -1;
//     }

//     printk("Data sent successfully\n");

//     // Optionally receive response
//     ret = recv(sock, send_buf, sizeof(send_buf) - 1, 0);
//     if (ret > 0) {
//         send_buf[ret] = '\0';
//         printk("Response: %s\n", send_buf);
//     }

//     close(sock);
//     return 0;

// }
// #define INFLUXDB_HOST "http://localhost:8086/api/v2/write"
// #define INFLUXDB_PORT 8086
// #define INFLUXDB_ORG "uq"
// #define INFLUXDB_BUCKET "seminar"
// #define INFLUXDB_TOKEN "9qF9f9iYm6HskdbanV7BO9yTOq6dl6QBzg67KXwMWAGzj8KNJLusuZC7Hr49OWL5syXI1WlzhOi5CoAmPUhvQQ=="

 

static int generate_all_sensors_json(const char *did, const char *rtc_time, int temp_value, int hum_value, int press_value) {
    struct all_sensors_data all_data;
    all_data.did = did;
    all_data.rtc_time = rtc_time;
    all_data.temp_value = temp_value;
    all_data.hum_value = hum_value;
    all_data.press_value = press_value; 

    char str[256];
    int ret = json_obj_encode_buf(all_sensors_descr, ARRAY_SIZE(all_sensors_descr), &all_data, str, sizeof(str));

    if (ret < 0) {
        LOG_ERR("JSON encoding error: %d", ret);
        return ret;
    }
 
    
    printf("%s\n", str);


    char line_protocol[256];
    int timestamp = 0; // Convert rtc_time to Unix timestamp here

    // Convert JSON to line protocol
    snprintf(line_protocol, sizeof(line_protocol),
             "sensors,did=%s temp_value=%d,hum_value=%d,press_value=%d %d",
             did, temp_value, hum_value, press_value, timestamp);

    // Send to InfluxDB
    return send_json_to_influxdb(line_protocol);
 
}


 
 


void all_sensors_json_generation_thread(void *arg1) {
    struct sampling *sampling_data = (struct sampling *)arg1;

//LOG_INF("JSON generation thread started for DID: %s", sampling_data->did);

    while (sampling_settings.ctn_sampling_on) {
        int temp_value, hum_value, press_value; 
 
        if (get_latest_temp_val(&temp_value) != 0) {
            temp_value = -1;
        }
    
        if (get_latest_hum_val(&hum_value) != 0) {
            hum_value = -1;
        }
    
        if (get_latest_press_val(&press_value) != 0) {
            press_value = -1;
        }
     
    

    const char *datetime_str = get_date_time();
 
    generate_all_sensors_json(sampling_data->did, datetime_str, temp_value, hum_value, press_value);

    k_sleep(K_MSEC(global_sampling_rate));
}

//LOG_INF("JSON generation thread stopped for DID: %s", sampling_data->did);

k_free(sampling_data);
}

void json_generation_thread(void *arg1) {
    struct sampling *sampling_data = (struct sampling *)arg1;
    int value;

   // LOG_INF("JSON generation thread started for DID: %s", sampling_data->did);

 
    while ((strcmp(sampling_data->did, "0") == 0 && sampling_settings.ctn_temp_sampling_on) ||
           (strcmp(sampling_data->did, "1") == 0 && sampling_settings.ctn_hum_sampling_on) ||
           (strcmp(sampling_data->did, "2") == 0 && sampling_settings.ctn_pressure_sampling_on)) {
        
        int sampling_rate;
        k_mutex_lock(&sampling_rate_mutex, K_FOREVER);
        sampling_rate = global_sampling_rate;
        k_mutex_unlock(&sampling_rate_mutex);

        if (strcmp(sampling_data->did, "0") == 0) {
            if (get_latest_temp_val(&value) == 0) {
                const char *datetime_str = get_date_time();
                generate_temp_json(sampling_data->did, datetime_str, value);
            }
        } else if (strcmp(sampling_data->did, "1") == 0) {
            if (get_latest_hum_val(&value) == 0) {
                const char *datetime_str = get_date_time();
                generate_temp_json(sampling_data->did, datetime_str, value);
            }
    
        }
    else if (strcmp(sampling_data->did, "2") == 0) {
        if (get_latest_press_val(&value) == 0) {
            const char *datetime_str = get_date_time();
            generate_temp_json(sampling_data->did, datetime_str, value);
        
        }
    }

        k_sleep(K_MSEC(sampling_rate));
    }

   // LOG_INF("JSON generation thread stopped for DID: %s", sampling_data->did);

    k_free(sampling_data);
} 
 

static int generate_temp_json(const char *did, const char *rtc_time, int value) {
    struct sampling temp_data;
    temp_data.did = did;
    temp_data.rtc_time = rtc_time; 

     temp_data.value= value;
 

    char str[128];
    int ret = json_obj_encode_buf(sampling_descr, ARRAY_SIZE(sampling_descr), &temp_data, str, sizeof(str));

    if (ret < 0) {
        LOG_ERR("JSON encoding error: %d", ret);
        return ret;
    }

    printf("%s\n", str);
    return 0;
} 

static int cmd_contin(const struct shell *shell, size_t argc, char **argv) {
 
    if (argc < 3) {
        shell_print(shell, "Usage: sample <s/p> <DID> or sample w <rate>");
        return 0;
    }

//    const char *did = argv[2];
const char *command = argv[1];

    if (strcmp(command, "s") == 0 || strcmp(command, "p") == 0) {
        
        const char *did = argv[2];
        if (strcmp(did, "0") != 0 && strcmp(did, "1") != 0 &&
        strcmp(did, "2") != 0 && strcmp(did, "4") != 0 &&
        strcmp(did, "15") != 0) {
        shell_print(shell, "Invalid DID: %s. Valid DIDs are 0, 1, 2, 4, or 15.", did);
        return 0;
    }
    
    if (strcmp(argv[1], "s") == 0) { 
        if (strcmp(did, "0") == 0) {
            // Start temp sampling
        
            if (sampling_settings.ctn_temp_sampling_on) {
                shell_print(shell, "Continuous sampling for temp sensor already enabled");
                return 0;
            }

            sampling_settings.ctn_temp_sampling_on = true; 

            struct sampling *temp_sampling_data = k_malloc(sizeof(struct sampling));
            temp_sampling_data->did = did;

            k_thread_create(&temp_thread_data, temp_thread_stack,
                            K_THREAD_STACK_SIZEOF(temp_thread_stack),
                            read_temp_continous, NULL, NULL, NULL,
                            SENSOR_PRIORITY, 0, K_NO_WAIT);
      
        k_thread_create(&temp_json_thread_data, json_thread_stack,
                K_THREAD_STACK_SIZEOF(json_thread_stack),
                json_generation_thread, temp_sampling_data, NULL, NULL,
                SENSOR_PRIORITY, 0, K_NO_WAIT);

 
        //
        //     shell_print(shell, "Continuous sampling for temperature sensor started");
        }
        
        else if (strcmp(did, "1") == 0) {
            if (sampling_settings.ctn_hum_sampling_on) {
                shell_print(shell, "Continuous sampling for humidity sensor already enabled");
                return 0;
            }
            // Start hum sampling
            sampling_settings.ctn_hum_sampling_on = true;
 
            struct sampling *hum_sampling_data = k_malloc(sizeof(struct sampling));
            hum_sampling_data->did=did;
            k_thread_create(&hum_thread_data, hum_thread_stack,
                            K_THREAD_STACK_SIZEOF(hum_thread_stack),
                            read_hum_continous, NULL, NULL, NULL,
                            SENSOR_PRIORITY, 0, K_NO_WAIT);
          k_thread_create(&hum_json_thread_data, json_thread_stack,
            K_THREAD_STACK_SIZEOF(json_thread_stack),
            json_generation_thread, hum_sampling_data, NULL, NULL,
            SENSOR_PRIORITY, 0, K_NO_WAIT);

        //    shell_print(shell, "Continuous sampling for humidity sensor started");
        } else if (strcmp(did, "2") == 0) {
            // Start press sampling
            if (sampling_settings.ctn_pressure_sampling_on) {
                shell_print(shell, "Continuous sampling for pressure sensor already enabled");
                return 0;
            }
            sampling_settings.ctn_pressure_sampling_on = true;
            struct sampling *press_sampling_data = k_malloc(sizeof(struct sampling));
            press_sampling_data->did = did;
            k_thread_create(&pressure_thread_data, pressure_thread_stack,
                            K_THREAD_STACK_SIZEOF(pressure_thread_stack),
                            read_pressure_continous, NULL, NULL, NULL,
                            SENSOR_PRIORITY, 0, K_NO_WAIT);

                            
            k_thread_create(&press_json_thread_data, json_thread_stack,
                    K_THREAD_STACK_SIZEOF(json_thread_stack),
                    json_generation_thread, press_sampling_data, NULL, NULL,
                    SENSOR_PRIORITY, 0, K_NO_WAIT);

    //        shell_print(shell, "Continuous sampling for pressure sensor started");
     
 
             } else if (strcmp(did, "15") == 0) {
                if (sampling_settings.ctn_sampling_on) {
                    shell_print(shell, "Continuous sampling for all sensors already enabled");
                    return 0;
                }
            
                sampling_settings.ctn_sampling_on = true;
            
                // Start threads for all sensors
                if (!sampling_settings.ctn_temp_sampling_on) {
                    sampling_settings.ctn_temp_sampling_on = true;
                    k_thread_create(&temp_thread_data, temp_thread_stack,
                                    K_THREAD_STACK_SIZEOF(temp_thread_stack),
                                    read_temp_continous, NULL, NULL, NULL,
                                    SENSOR_PRIORITY, 0, K_NO_WAIT);
                }
            
                if (!sampling_settings.ctn_hum_sampling_on) {
                    sampling_settings.ctn_hum_sampling_on = true;
                    k_thread_create(&hum_thread_data, hum_thread_stack,
                                    K_THREAD_STACK_SIZEOF(hum_thread_stack),
                                    read_hum_continous, NULL, NULL, NULL,
                                    SENSOR_PRIORITY, 0, K_NO_WAIT);
                }
            
                if (!sampling_settings.ctn_pressure_sampling_on) {
                    sampling_settings.ctn_pressure_sampling_on = true;
                    k_thread_create(&pressure_thread_data, pressure_thread_stack,
                                    K_THREAD_STACK_SIZEOF(pressure_thread_stack),
                                    read_pressure_continous, NULL, NULL, NULL,
                                    SENSOR_PRIORITY, 0, K_NO_WAIT);
                }
            
          
                //  JSON for all sensors
                struct sampling *all_sensors_data = k_malloc(sizeof(struct sampling));
            
                all_sensors_data->did = did; 
                k_thread_create(&press_json_thread_data, json_thread_stack,
                                K_THREAD_STACK_SIZEOF(json_thread_stack),
                                all_sensors_json_generation_thread, all_sensors_data, NULL, NULL,
                                SENSOR_PRIORITY, 0, K_NO_WAIT);
            
      //          shell_print(shell, "Continuous sampling for all sensors started");
            }
    } else if (strcmp(argv[1], "p") == 0) {
       
    
        if (strcmp(did, "0") == 0) {
            if (!sampling_settings.ctn_temp_sampling_on) {
                shell_print(shell, "Temperature sampling is not running");
                return 0;
            }
            shell_print(shell, "Stopping temperature sampling");
            sampling_settings.ctn_temp_sampling_on = false;
        } else if (strcmp(did, "1") == 0) {
            if (!sampling_settings.ctn_hum_sampling_on) {
                shell_print(shell, "Humidity sampling is not running");
                return 0;
            }
            shell_print(shell, "Stopping humidity sampling");
            sampling_settings.ctn_hum_sampling_on = false;
        } else if (strcmp(did, "2") == 0) {
            if (!sampling_settings.ctn_pressure_sampling_on) {
                shell_print(shell, "Pressure sampling is not running");
                return 0;
            }
            shell_print(shell, "Stopping pressure sampling");
            sampling_settings.ctn_pressure_sampling_on = false;
       } else if (strcmp(did, "15") == 0) {
            if (!sampling_settings.ctn_sampling_on) {
                shell_print(shell, "No sensors are currently running");
                return 0;
            }
            shell_print(shell, "Stopping all sensors");
            sampling_settings.ctn_temp_sampling_on = false;
            sampling_settings.ctn_hum_sampling_on = false;
            sampling_settings.ctn_pressure_sampling_on = false; 
            sampling_settings.ctn_sampling_on = false;
        } else {
            shell_print(shell, "Invalid DID: %s", did);
            return 0;
        }
    }
    }
    
    else if (strcmp(command, "w") == 0) {
        int rate = atoi(argv[2]);
         if (strcmp(argv[1], "w") == 0) { 
            if (argc != 3) {
                shell_print(shell, "Usage: sample w <rate>");
                return 0;
            }
            
            if (rate <= 0) {
                shell_print(shell, "cant be -ms: %d", rate);
                return 0;
            }
         
            k_mutex_lock(&sampling_rate_mutex, K_FOREVER);
            global_sampling_rate = rate;
            k_mutex_unlock(&sampling_rate_mutex);
        
            shell_print(shell, "Sampling rate set to %d ms", rate);
            LOG_INF("Sampling rate changed to %d ms", rate);
        }
    } else {
        shell_print(shell, "Invalid command. Usage: sample <s/p> <DID> or sample w <rate>");
        return 0;
    }
    return 0;
}

static int cmd_rtc(const struct shell *shell, size_t argc, char **argv) {
    if (argc < 2) {
        shell_print(shell, "Usage: rtc r or rtc w <year> <month> <day> <hour> <min> <sec>");
        return 0;
    }

    if (strcmp(argv[1], "r") == 0) { 
        if (argc != 2) {
            shell_print(shell, "Usage: rtc r");
            return 0;
        } 
       const char *datetime_str = get_date_time();
       LOG_INF("RTC time: %s", datetime_str);

    } else if (strcmp(argv[1], "w") == 0) { 
        if (argc != 8) {
            shell_print(shell, "Usage: rtc w <year> <month> <day> <hour> <min> <sec>");
            return 0;
        }

        int year = atoi(argv[2]);
        int month = atoi(argv[3]);
        int day = atoi(argv[4]);
        int hour = atoi(argv[5]);
        int min = atoi(argv[6]);
        int sec = atoi(argv[7]);

        int ret = set_rtc(year, month, day, hour, min, sec);
        if (ret < 0) {
            shell_print(shell, "RTC time set failed: %d", ret);
            return ret;
        }
    } else {
        shell_print(shell, "Invalid command. Usage: rtc r or rtc w <year> <month> <day> <hour> <min> <sec>");
        return 0;
    }

    return 0;
}
 
 

int main(void) {
   connect_to_wifi();
   // Initialize RTC with the current system time
    // if (set_rtc_to_system_time() < 0) {
    //     LOG_ERR("Failed to set RTC to system time. Using default time.");
    // }

    while (1) {
    
       k_sleep(K_FOREVER); // Main thread sleeps forever
    }
    return 0;
}
 
SHELL_CMD_REGISTER(rtc, NULL, "rtc <w/r> #write/read RTC", cmd_rtc);
SHELL_CMD_REGISTER(sample, NULL, "sample <s/p> <DID> or sample w <rate>", cmd_contin);


