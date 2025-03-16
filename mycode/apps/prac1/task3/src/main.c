#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>
#include <stdio.h>

#define SLEEP_TIME_MS 1000

#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

#define THREAD_STACK_SIZE 1024
#define THREAD_PRIORITY 5

K_SEM_DEFINE(led_sem, 0, 1);  // semaphore

LOG_MODULE_REGISTER(led_module, LOG_LEVEL_DBG);

static char led_command = '\0';  // s or t
static uint8_t bitmask = 0;

static int cmd_time(const struct shell *shell, size_t argc, char **argv) {
    uint64_t ms = k_uptime_get();
    uint32_t sec = ms / 1000;

    if (argc > 1 && strcmp(argv[1], "f") == 0) {
        uint32_t hours = sec / 3600;
        uint32_t minutes = (sec % 3600) / 60;
        uint32_t seconds = sec % 60;

        shell_print(shell, "Formatted time: %02u:%02u:%02u", hours, minutes, seconds);
        LOG_INF("Formatted time requested: %02u:%02u:%02u", hours, minutes, seconds);
    } else {
        shell_print(shell, "Time elapsed: %u secs", sec);
        LOG_INF("Time elapsed: %u secs", sec);
    }

    return 0;
}

SHELL_CMD_REGISTER(time, NULL, "'time f' for formatted time", cmd_time);

void led_thread(void) {
    bool led0_state = false;  
    bool led1_state = false;

    while (1) {
        k_sem_take(&led_sem, K_FOREVER);   

        int val0 = (bitmask & 0x01) ? 1 : 0;
        int val1 = (bitmask & 0x02) ? 1 : 0;

        if (led_command == 's') {    
            // check if led 0 is already on before setting
            if (val0 && led0_state) {
                LOG_WRN("led 0 already on");
            } else {
                gpio_pin_set_dt(&led0, val0);
                led0_state = val0;  
                if (val0) {
                    LOG_INF("led 0 is on");
                } else {
                    LOG_INF("led 0 is off");
                }
            }
 
            if (val1 && led1_state) {
                LOG_WRN("led 1 already on");
            } else {
                gpio_pin_set_dt(&led1, val1);
                led1_state = val1;   
                if (val1) {
                    LOG_INF("led 1 is on");
                } else {
                    LOG_INF("led 1 is off");
                }
            }
        } else if (led_command == 't') {   
            if (bitmask & 0x01) {
                gpio_pin_toggle_dt(&led0);
                led0_state = !led0_state;   
                LOG_DBG("Toggled led 0");
            }
            if (bitmask & 0x02) {
                gpio_pin_toggle_dt(&led1);
                led1_state = !led1_state;   
                LOG_DBG("Toggled led 1");
            }
        }
    }
}

K_THREAD_STACK_DEFINE(led_thread_stack, THREAD_STACK_SIZE);
struct k_thread led_thread_data;

static int cmd_led(const struct shell *shell, size_t argc, char **argv) {
    if (argc != 3) {
        LOG_ERR("Invalid command usage");
        return -EINVAL;
    }

    led_command = argv[1][0];
    bitmask = (uint8_t)strtol(argv[2], NULL, 2);  // Convert binary string to int

    if (led_command != 's' && led_command != 't') {
        shell_error(shell, "Invalid command");
        LOG_ERR("Invalid LED command");
        return -EINVAL;
    }

    k_sem_give(&led_sem);  // Signal the LED thread 
    return 0;
}

SHELL_CMD_REGISTER(led, NULL, "Control LEDs (s=Set, t=Toggle)", cmd_led);

int main(void) {
    if (!gpio_is_ready_dt(&led0) || !gpio_is_ready_dt(&led1)) {
        LOG_ERR("GPIO not ready");
        return -ENODEV;
    }

    gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE);
    gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE);
    LOG_INF("LEDs initialized");

    k_thread_create(&led_thread_data, led_thread_stack, K_THREAD_STACK_SIZEOF(led_thread_stack), led_thread,
                    NULL, NULL, NULL, THREAD_PRIORITY, 0, K_NO_WAIT);

    LOG_INF("LED thread started");

    return 0;
}