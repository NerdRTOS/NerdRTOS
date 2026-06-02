#include "nerd.h"
#include "ntest.h"

#define PREEMPT_STACK_SIZE 512
#define PREEMPT_DELAY_US  5000ULL
#define PREEMPT_BUSY_LOOPS 3000000U

static volatile nd_uint32_t preempt_ran;
static volatile nd_uint32_t preempt_done;

ALIGN(8) static nd_thread_t preempt_thread;
ALIGN(8) static nd_uint8_t preempt_stack[PREEMPT_STACK_SIZE];

static nd_uint8_t preempt_test_priority(void)
{
    if (nd_current_thread->priority > 0) {
        return nd_current_thread->priority - 1;
    }

    return nd_current_thread->priority;
}

static void preempt_entry(void *parameter)
{
    (void)parameter;

    nd_thread_delay(PREEMPT_DELAY_US);
    preempt_ran++;
    preempt_done = 1;
}

static void test_irq_exit_preempts_busy_thread(void)
{
    preempt_ran = 0;
    preempt_done = 0;

    ntest_assert_equal(nd_thread_create(&preempt_thread,
                                        "t_preempt",
                                        preempt_entry,
                                        preempt_test_priority(),
                                        ND_NULL,
                                        preempt_stack,
                                        sizeof(preempt_stack),
                                        ND_THREAD_OPT_NONE,
                                        0),
                       ND_EOK);

    for (volatile nd_uint32_t loops = 0; loops < PREEMPT_BUSY_LOOPS; loops++) {
        if (preempt_done) {
            break;
        }
    }

    if (preempt_ran != 1) {
        nd_thread_abort(&preempt_thread);
        nd_thread_join(&preempt_thread, ND_TIMEOUT_FOREVER);
    }

    ntest_assert_equal(preempt_ran, 1);
    ntest_assert_equal(nd_thread_join(&preempt_thread, ND_TIMEOUT_FOREVER),
                       ND_EOK);
}

static void cmd_test_preempt(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    NTEST_SUITE_BEGIN("preempt");
    NTEST_RUN(test_irq_exit_preempts_busy_thread);
    NTEST_SUITE_END();
}

SHELL_CMD(test_preempt, cmd_test_preempt, "preemption test suite");

