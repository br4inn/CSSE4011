#include <zephyr/kernel.h>
#include <zephyr/device.h> 
#include <zephyr/devicetree.h>
#include <zephyr/random/random.h>
#include <stdio.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

#define CI_NODE DT_ALIAS(ci_led)
#define DI_NODE DT_ALIAS(di_led)

#define RGB_PRIORITY 3
#define CONTROL_PRIORITY 4
#define STACK_SIZE 1024
 

static const struct gpio_dt_spec ci = GPIO_DT_SPEC_GET(CI_NODE, gpios);
static const struct gpio_dt_spec di = GPIO_DT_SPEC_GET(DI_NODE, gpios);

int main(void) {
   
    if (!gpio_is_ready_dt(&ci) || !gpio_is_ready_dt(&di)) {
        return 0;
    }

    int ci_ret = gpio_pin_configure_dt(&ci, GPIO_OUTPUT_ACTIVE);
    int di_ret = gpio_pin_configure_dt(&di, GPIO_OUTPUT_ACTIVE);
    if (ci_ret < 0 || di_ret < 0) {
       
        return 0;
    }
 
    return 0;
}

void clk(void) {
    gpio_pin_set_dt(&ci, 0); // checked and is toggling
    k_usleep(20);

    gpio_pin_set_dt(&ci, 1); 
    k_usleep(20);
}


void send_byte(uint8_t byte){
     
     for (uint8_t i=0; i<8; i++)
     { 
         if ((byte & 0x80) != 0)
            gpio_pin_set_dt(&di, 1);
         else
            gpio_pin_set_dt(&di, 0);
         clk();
   
         byte <<= 1;
     }
}

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

void send_rgb_data(uint8_t red, uint8_t green, uint8_t blue) {
    printk("Sending Colour R: %d, G: %d, B: %d\n", red, green, blue);
    send_byte(0x00);
    send_byte(0x00);
    send_byte(0x00);
    send_byte(0x00);
     

    uint8_t prefix = 0b11000000;
    if ((blue & 0x80) == 0)     prefix|= 0b00100000;
    if ((blue & 0x40) == 0)     prefix|= 0b00010000; 
    if ((green & 0x80) == 0)    prefix|= 0b00001000;
    if ((green & 0x40) == 0)    prefix|= 0b00000100;
    if ((red & 0x80) == 0)      prefix|= 0b00000010;
    if ((red & 0x40) == 0)      prefix|= 0b00000001;
    send_byte(prefix);

    send_byte(blue);  
    send_byte(green);  
    send_byte(red);   

    send_byte(0x00);
    send_byte(0x00);
    send_byte(0x00);
    send_byte(0x00);

    


}

void control(void) {
    uint32_t col_num = 0;
    while (1) {
        uint32_t colour = colour_sequence[col_num];
        k_msgq_put(&colour_msgq, &colour, K_NO_WAIT);
        
        col_num = (col_num + 1) % 8;  
        k_sleep(K_SECONDS(2));
    }
}


void rgb_thread(void) {
    uint32_t colour;
    while (1) {  
        k_msgq_get(&colour_msgq, &colour, K_FOREVER); 
       // send_rgb_data(255, 0, 0); // Set to full red

        send_rgb_data((colour >> 16) & 0xFF, (colour >> 8) & 0xFF, colour & 0xFF);
    }
}

K_THREAD_DEFINE(rgb_id, STACK_SIZE, rgb_thread, NULL, NULL, NULL,
                RGB_PRIORITY, 0, 0);

K_THREAD_DEFINE(control_id, STACK_SIZE, control, NULL, NULL, NULL,
                CONTROL_PRIORITY, 0, 0);
