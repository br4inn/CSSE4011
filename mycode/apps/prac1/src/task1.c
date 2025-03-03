#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>

#define STACK_SIZE 1024
#define DISPLAY_PRIORITY 5
#define RNG_PRIORITY 4

struct k_poll_signal signal;
uint16_t random_num;
 

void random_number_generate(void *arg1, void *arg2, void *arg3)
{
    while (1) {
        random_num = sys_rand16_get();   
        k_poll_signal_raise(&signal, random_num);  // Sig display thread
        k_sleep(K_SECONDS(2));   
    }
}
 

void display_random_number(void *arg1, void *arg2, void *arg3)
{
    k_poll_signal_init(&signal);   

    struct k_poll_event events[1] = {
        K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
                                 K_POLL_MODE_NOTIFY_ONLY,
                                 &signal),
    };

    while (1) {
        k_poll(events, 1, K_FOREVER);  
        int signaled, result;
        k_poll_signal_check(&signal, &signaled, &result);

        if (signaled) {
            printk("Random Num: %08u\n", random_num);
            signal.signaled = 0;  // Reset sig
        }
    }
}

K_THREAD_DEFINE(rng_id, STACK_SIZE, random_number_generate, NULL, NULL, NULL,
                RNG_PRIORITY, 0, 0);

K_THREAD_DEFINE(display_id, STACK_SIZE, display_random_number, NULL, NULL, NULL,
                DISPLAY_PRIORITY, 0, 0);
