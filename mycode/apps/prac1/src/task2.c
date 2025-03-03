#include <zephyr/kernel.h>
#include <zephyr/device.h> 
#include <zephyr/random/random.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

#define CI_PIN 13
#define DI_PIN 11

#define RGB_PRIORITY 3
#define CONTROL_PRIORITY 4
#define STACK_SIZE 1024

const struct device *gpio_dev;


K_MSGQ_DEFINE(colour_msgq, sizeof(uint32_t), 8, 4);

static const uint32_t colour_sequence[] = {
    0x000000,  
    0x0000FF,  
    0x00FF00,  
    0x00FFFF,   
    0xFF0000,   
    0xFF00FF,   
    0xFFFF00,   
    0xFFFFFF  
};
 
void init_gpio()
{
    gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpioa));
    
    gpio_pin_configure(gpio_dev, CI_PIN, GPIO_OUTPUT);
    gpio_pin_configure(gpio_dev, DI_PIN, GPIO_OUTPUT);
}



void send_rgb_data(uint8_t red, uint8_t green, uint8_t blue)
{   // 32 bit custom packet
    uint32_t packet = (0b11 << 30) |  //  max brightness 2 bits
                      ((green & 0x3F) << 24) | // green 6 bits
                      ((red & 0x3F) << 18) |  // red 6 bits
                      ((blue & 0x3F) << 12);  // blue 6 bits

    for (int i = 31; i >= 0; i--) 
    {
        gpio_pin_set(gpio_dev, DI_PIN, (packet >> i) & 1);  
        gpio_pin_set(gpio_dev, CI_PIN, 1);  
        k_sleep(K_USEC(10));   
        gpio_pin_set(gpio_dev, CI_PIN, 0);  
        k_sleep(K_USEC(10));
    }
}

void control(void) {

    uint32_t col_num = 0;

    while (1) { 
        k_msgq_put(&colour_msgq, &colour_sequence[col_num], K_NO_WAIT);
        col_num = (col_num + 1) % 8;
        k_sleep(K_SECONDS(2));
    }
}

void rgb_thread(void) {
    uint32_t colour;
    uint8_t red;
    uint8_t green;
    uint8_t blue;

    while (1) { 
        k_msgq_get(&colour_msgq, &colour, K_FOREVER);
 
        red = (colour >> 16) & 0xFF;
        green = (colour >> 8) & 0xFF;
        blue = colour & 0xFF;
    
         printk("Current col - Red: %d, Green: %d, Blue: %d\n", red, green, blue);

        send_rgb_data(red, green, blue);
    }
}
 

K_THREAD_DEFINE(rgb_id, STACK_SIZE, rgb_thread, NULL, NULL, NULL,
                RGB_PRIORITY, 0, 0);

K_THREAD_DEFINE(control_id, STACK_SIZE, control, NULL, NULL, NULL,
                CONTROL_PRIORITY, 0, 0);