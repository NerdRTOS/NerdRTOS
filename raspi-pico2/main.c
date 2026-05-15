#include "pico/stdlib.h"
#include "nerd.h"
#include <stdio.h>
#include "nd_shell.h"

#define LED_PIN 25

ALIGN(8)
static nd_uint8_t stack_led[4096];
ALIGN(8)
static nd_uint8_t stack_print[4096];
ALIGN(8)
static nd_uint8_t stack_shell[4096];

static nd_thread_t thread_led;
static nd_thread_t thread_print;
static nd_thread_t thread_shell;

static void task_led(void *param)
{
    (void)param;

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    while (1)
    {
        gpio_put(LED_PIN, 1);
        nd_thread_delay(500000);

        gpio_put(LED_PIN, 0);
        nd_thread_delay(500000);
    }
}

static void task_print(void *param)
{
    (void)param;
    int count = 0;
    while (1)
    {
        printf("print tick: %d\r\n", count++);
        nd_thread_delay(3000000);
    }
}

int main(void)
{
    stdio_init_all();

    unsigned long mtvec_val;
    __asm__ volatile("csrr %0, mtvec" : "=r"(mtvec_val));

    printf("mtvec = 0x%08lx\r\n", mtvec_val);
    printf("NerdRTOS booting on Hazard3...\r\n");

    nd_hw_tick_init();
    printf("tick init done\r\n");

    nd_hw_hrtimer_init();
    printf("hrtimer init done\r\n");

    nd_scheduler_init();
    printf("scheduler init done\r\n");

    shell_init();

    nd_thread_init(&thread_shell, "shell",
                   nd_shell_task_entry, 30, NULL,
                   stack_shell, sizeof(stack_shell), 0);

    nd_thread_init(&thread_led, "led",
                   task_led, 10, NULL,
                   stack_led, sizeof(stack_led), 0);

    nd_thread_init(&thread_print, "print",
                   task_print, 20, NULL,
                   stack_print, sizeof(stack_print), 0);


    printf("threads ready\r\n");
    printf("starting scheduler...\r\n");

    nd_scheduler_start();

    while (1)
        ;
}
