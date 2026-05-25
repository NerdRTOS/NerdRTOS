#include "led.h"
#include "nd_shell.h"
#include "app.h"

#define SHELL_STACK_SIZE    1024
#define LED_STACK_SIZE      512

static nd_uint8_t   shell_stack[SHELL_STACK_SIZE];
static nd_thread_t  shell;

static nd_uint8_t   led_stack[LED_STACK_SIZE];
static nd_thread_t  led_thread;

static void led_blink_task(void *parameter)
{
    (void)parameter;

    while (1) {
        nd_led_toggle(LED0);
        nd_thread_delay(500);
    }
}

void nd_app_init(void)
{
    shell_init();

    nd_thread_create(&led_thread, "led_blink", led_blink_task,
                     5, ND_NULL, led_stack, sizeof(led_stack), ND_THREAD_OPT_NONE, 0);

    nd_thread_create(&shell, "shell", nd_shell_task_entry,
                     30, ND_NULL, shell_stack, sizeof(shell_stack), ND_THREAD_OPT_NONE, 0);
}
