#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>

#define STACK_SIZE 1024
#define DISPLAY_PRIORITY 5
#define RNG_PRIORITY 5

uint16_t randNum = sys_rand16_get();



K_THREAD_DEFINE(rng_id, STACK_SIZE, rng, NULL, NULL, NULL,
                RNG_PRIORITY, 0, 0);

K_THREAD_DEFINE(display_id, STACK_SIZE, display, NULL, NULL, NULL,
                DISPLAY_PRIORITY, 0, 0);