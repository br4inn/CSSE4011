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
#define SLEEP_TIME_MS 1000  
#define THREAD_STACK_SIZE 1024
#define THREAD_PRIORITY 5
 
 
K_SEM_DEFINE(led_sem, 0, 1);  //semaphore

 
static int cmd_time(const struct shell *shell, size_t argc, char **argv) {
    uint64_t ms = k_uptime_get();  
    uint32_t sec = ms / 1000;

    if (argc > 1 && strcmp(argv[1], "f") == 0) {
        uint32_t hours = sec / 3600;
        uint32_t minutes = (sec % 3600) / 60;
        uint32_t seconds = sec % 60;

        shell_print(shell, "Time elapsed formatted: %02u:%02u:%02u", hours, minutes, seconds);
    } else {
        shell_print(shell, "Time elapsed: %u secs", sec);
    }
    
    return 0;
}
 
SHELL_CMD_REGISTER(time, NULL, "'time f' for formatted time", cmd_time);


static char led_command = '\0';  // either s or t
static uint8_t bitmask = 0;   


void led_thread(void *arg1, void *arg2, void *arg3) {
    while (1) {
        k_sem_take(&led_sem, K_FOREVER);  // Wait for command

        int val0 = (bitmask & 0x01) ? 1 : 0;
        int val1 = (bitmask & 0x02) ? 1 : 0;

        if (led_command == 's') {   
            gpio_pin_set_dt(&led0, val0);
            gpio_pin_set_dt(&led1, val1);
        } else if (led_command == 't') {   
            if (bitmask & 0x01) {
                gpio_pin_toggle_dt(&led0);
            }
            if (bitmask & 0x02) {
                gpio_pin_toggle_dt(&led1);
            }
        }
    }
}
 
K_THREAD_STACK_DEFINE(led_thread_stack, THREAD_STACK_SIZE);


struct k_thread led_thread_data;
 
static int cmd_led(const struct shell *shell, size_t argc, char **argv) {
    if (argc != 3) { 
        return 0;
    }

    led_command = argv[1][0];
    bitmask = (uint8_t)strtol(argv[2], NULL, 2);  // for bin operations

    if (led_command != 's' && led_command != 't') {
        shell_error(shell, "Invalid command. Use 's' for set, 't' for toggle.");
        return 0;
    }

    k_sem_give(&led_sem);  // sig the led thread
    shell_print(shell, "Command sent: %c %s", led_command, argv[2]);

    return 0;
}
 
SHELL_CMD_REGISTER(led, NULL, "Control LEDs (s=Set, t=Toggle)", cmd_led);



int main(void) {
    if (!gpio_is_ready_dt(&led0) || !gpio_is_ready_dt(&led1)) {
        return 0;
    }

    gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE);
    gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE);

    k_thread_create(&led_thread_data, led_thread_stack, K_THREAD_STACK_SIZEOF(led_thread_stack), led_thread,
                    NULL, NULL, NULL, THREAD_PRIORITY, 0, K_NO_WAIT);
}

//  // NEED COMMAND TO ACCESS SYSTEM TIMER ASWELL!
 






 