#include "nerd.h"
#include "nd_shell.h"
#include "nd_klibc.h"

#define TASK_STACK_SIZE    1024

static nd_queue_t queue;
static nd_thread_t TaskA;
static nd_thread_t TaskB;

ALIGN(8) static nd_uint8_t TaskA_stack[TASK_STACK_SIZE];
ALIGN(8) static nd_uint8_t TaskB_stack[TASK_STACK_SIZE];

static nd_bool_t initialized = ND_FALSE;

static char buf[16];

static void TaskA_entry(void *para)
{
    (void)para;

    nd_uint32_t counter = 0;

    while (1) {
        nd_queue_send(&queue, &counter, ND_TIMEOUT_FOREVER);

        counter++;

        nd_thread_delay(ND_TIMEOUT_MS(500));
    }
}

static void TaskB_entry(void *para)
{
    (void)para;

    nd_uint32_t expected = 0;
    nd_uint32_t rx = 0;

    while (1) {
        nd_queue_recv(&queue, &rx, ND_TIMEOUT_FOREVER);

        shell_printf("rx=%d, expectted:%d\r\n", rx, expected);

        expected++;

        nd_thread_delay(ND_TIMEOUT_MS(500));
    }
}

static void queue_test(int argc, char *argv[])
{
    if (argc != 2) {
        shell_puts("[usage]:queue start\r\n");
        shell_puts("        queue stop\r\n");
        return;
    }

    if (nd_strcmp(argv[1], "start") == 0) {
        if (initialized == ND_FALSE){
            nd_queue_init(&queue, buf, 4, 4);

            nd_thread_init(&TaskA, "TASK A", TaskA_entry, 20, ND_NULL, TaskA_stack, TASK_STACK_SIZE, ND_TIMEOUT_MS(500));
            nd_thread_init(&TaskB, "TASK B", TaskB_entry, 20, ND_NULL, TaskB_stack, TASK_STACK_SIZE, ND_TIMEOUT_MS(500));

            initialized = ND_TRUE;
        } else {
            nd_thread_resume(&TaskB);
            nd_thread_resume(&TaskA);
        }
    }

    else if (nd_strcmp(argv[1], "stop") == 0) {
        nd_thread_suspend(&TaskB);
        nd_thread_suspend(&TaskA);
    }

    else {
        shell_puts("[usage]:queue start\r\n");
        shell_puts("        queue stop\r\n");
    }
}

SHELL_CMD(queue, queue_test, "queue test");
