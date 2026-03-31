#include <stdio.h>
#include "nerd.h"
#include "nd_shell.h"
#include "nd_klibc.h"

#define TASK_STACK_SIZE    1024
#define MON_STACK_SIZE     1024

static nd_thread_t task1;
static nd_thread_t task2;
static nd_thread_t monitor;

ALIGN(8) static nd_uint8_t task1_stack[TASK_STACK_SIZE];
ALIGN(8) static nd_uint8_t task2_stack[TASK_STACK_SIZE];
ALIGN(8) static nd_uint8_t monitor_stack[MON_STACK_SIZE];

static volatile nd_uint32_t g_cnt1 = 0;
static volatile nd_uint32_t g_cnt2 = 0;
static nd_uint32_t g_last1 = 0;
static nd_uint32_t g_last2 = 0;

static nd_bool_t initialized = ND_FALSE;

static void task1_entry(void *param)
{
    (void)param;
    while (1) {
        g_cnt1++;

        nd_thread_delay(ND_TIMEOUT_MS(200));
    }
}

static void task2_entry(void *param)
{
    (void)param;
    while (1) {
        g_cnt2++;

         nd_thread_delay(ND_TIMEOUT_MS(200));
    }
}

static void monitor_entry(void *param)
{
    (void)param;

    while (1) {
        nd_thread_delay(ND_TIMEOUT_S(1));

        nd_uint32_t c1 = g_cnt1;
        nd_uint32_t c2 = g_cnt2;

        nd_uint32_t d1 = c1 - g_last1;
        nd_uint32_t d2 = c2 - g_last2;

        g_last1 = c1;
        g_last2 = c2;

        nd_uint64_t total = (nd_uint64_t)d1 + (nd_uint64_t)d2;
        nd_uint32_t p1 = total ? (nd_uint32_t)(((nd_uint64_t)d1 * 100u) / total) : 0;
        nd_uint32_t p2 = total ? (nd_uint32_t)(((nd_uint64_t)d2 * 100u) / total) : 0;

        printf("[%llu us] RR stats: d1=%u (%u%%), d2=%u (%u%%)\r\n",
            nd_hw_get_current(), d1, p1, d2, p2);
    }
}

static void cmd_rr(int argc, char *argv[])
{
    if (argc != 2) {
        shell_puts("[usage]:rr start\r\n");
        shell_puts("        rr stop\r\n");
        return;
    }

    if (nd_strcmp(argv[1], "start") == 0) {
        if (initialized == ND_FALSE) {
            nd_thread_init(&task1, "task1", task1_entry, 10, ND_NULL, task1_stack, TASK_STACK_SIZE, 500);
            nd_thread_init(&task2, "task2", task2_entry, 10, ND_NULL, task2_stack, TASK_STACK_SIZE, 500);
            nd_thread_init(&monitor, "monitor", monitor_entry, 15, ND_NULL, monitor_stack, MON_STACK_SIZE, 100);

            initialized = ND_TRUE;
        } else {
            nd_thread_resume(&task1);
            nd_thread_resume(&task2);
            nd_thread_resume(&monitor);
        }
    }

    else if (nd_strcmp(argv[1], "stop") == 0) {
        nd_thread_suspend(&task1);
        nd_thread_suspend(&task2);
        nd_thread_suspend(&monitor);
    }

    else {
        shell_puts("[usage]:rr start\r\n");
        shell_puts("        rr stop\r\n");
    }
}

SHELL_CMD(rr, cmd_rr, "RR Monitor");
