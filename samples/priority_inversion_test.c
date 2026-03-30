#include "nerd.h"
#include "nd_shell.h"
#include "nd_ipc.h"
#include "nd_timer.h"
#include "nd_thread.h"

#define TASK_STACK_SIZE    1024

static nd_bool_t initialized = ND_FALSE;

static nd_thread_t h_thread;
static nd_thread_t m_thread;
static nd_thread_t l_thread;

ALIGN(8) static nd_uint8_t h_stack[TASK_STACK_SIZE];
ALIGN(8) static nd_uint8_t m_stack[TASK_STACK_SIZE];
ALIGN(8) static nd_uint8_t l_stack[TASK_STACK_SIZE];

static nd_mutex_t iver_mutex;

static void high_pri_thread_entry(void *para)
{
    (void)para;

    nd_thread_delay(ND_TIMEOUT_S(1));

    while (1){
        nd_mutex_lock(&iver_mutex, ND_TIMEOUT_FOREVER);

        printf("[%llu] High priority thread is running\r\n", nd_hw_get_current());

        nd_thread_delay(ND_TIMEOUT_MS(300));

        nd_mutex_unlock(&iver_mutex);
    }
}

static void middle_pri_thread_entry(void *para)
{
    (void)para;

    nd_thread_delay(ND_TIMEOUT_S(1));

    while (1){
        printf("[%llu] Middle priority thread is running\r\n", nd_hw_get_current());

        nd_thread_delay(ND_TIMEOUT_S(1));
    }
}

static void low_pri_thread_entry(void *para)
{
    (void)para;

    while (1){
        nd_mutex_lock(&iver_mutex, ND_TIMEOUT_FOREVER);

        printf("[%llu] Low priority thread is running,priority:%d\r\n", nd_hw_get_current(), nd_current_thread->priority);

        for (int i = 0; i < 5; i++) {
            printf("[%llu] Low priority thread working (priority:%d)\r\n",
                        nd_hw_get_current(), nd_current_thread->priority);

            nd_uint64_t start = nd_hw_get_current();
            while (nd_hw_get_current() - start < ND_TIMEOUT_S(1));
        }

        nd_mutex_unlock(&iver_mutex);

        printf("Low priority thread unlock,priority:%d\r\n", nd_current_thread->priority);
    }
}

static void cmd_invert_test(int argc, char *argv[])
{
    if (argc != 2){
        shell_puts("[usage]:invert start\r\n");
        shell_puts("        invert stop\r\n");
        return;
    }

    if (nd_strcmp(argv[1], "start") == 0) {
        if (initialized == ND_FALSE){
            nd_mutex_init(&iver_mutex);

            nd_thread_init(&l_thread, "low pri thread", low_pri_thread_entry, 20, ND_NULL, l_stack, TASK_STACK_SIZE, 100);
            nd_thread_init(&m_thread, "Mid pri thread", middle_pri_thread_entry, 10, ND_NULL, m_stack, TASK_STACK_SIZE, 500);
            nd_thread_init(&h_thread, "High pri thread", high_pri_thread_entry, 5, ND_NULL, h_stack, TASK_STACK_SIZE, 500);

            initialized = ND_TRUE;
        } else {
            nd_thread_resume(&l_thread);
            nd_thread_resume(&m_thread);
            nd_thread_resume(&h_thread);
        }
    }

    else if (nd_strcmp(argv[1], "stop") == 0) {
        nd_thread_suspend(&l_thread);
        nd_thread_suspend(&m_thread);
        nd_thread_suspend(&h_thread);
    }

    else {
        shell_puts("[usage]:invert start\r\n");
        shell_puts("        invert stop\r\n");
    }
}

SHELL_CMD(invert, cmd_invert_test, "priority inversion test");
