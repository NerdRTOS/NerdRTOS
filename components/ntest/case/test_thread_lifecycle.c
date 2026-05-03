#include "nerd.h"
#include "ntest.h"
#include "nd_shell.h"
#include "nd_mem.h"

#define TEST_STACK_SIZE 512

static volatile nd_uint32_t static_entry_ran;
static volatile nd_uint32_t dynamic_entry_ran;

ALIGN(8) static nd_thread_t static_thread;
ALIGN(8) static nd_uint8_t static_stack[TEST_STACK_SIZE];

ALIGN(8) static nd_thread_t busy_thread;
ALIGN(8) static nd_uint8_t busy_stack[TEST_STACK_SIZE];

static nd_uint8_t test_thread_priority(void)
{
    if (nd_current_thread->priority + 1 < ND_THREAD_PRIORITY_MAX) {
        return nd_current_thread->priority + 1;
    }

    return nd_current_thread->priority;
}

static void busy_entry(void *parameter)
{
    volatile nd_uint32_t *flag = (volatile nd_uint32_t *)parameter;

    (*flag)++;

    while (1) {
    }
}

static void test_static_thread_abort_join(void)
{
    static_entry_ran = 0;

    ntest_assert_equal(nd_thread_create(&static_thread,
                                        "t_static",
                                        busy_entry,
                                        test_thread_priority(),
                                        (void *)&static_entry_ran,
                                        static_stack,
                                        sizeof(static_stack),
                                        ND_THREAD_OPT_NONE,
                                        0),
                       ND_EOK);

    ntest_assert_equal(static_entry_ran, 0);
    ntest_assert_equal(nd_thread_abort(&static_thread), ND_EOK);
    ntest_assert_equal(nd_thread_join(&static_thread, ND_TIMEOUT_FOREVER),
                       ND_EOK);
    ntest_assert_equal(static_thread.stat, ND_THREAD_STAT_DEAD);
}

static void test_dynamic_thread_abort_join_free(void)
{
    dynamic_entry_ran = 0;

    nd_thread_t *thread = nd_malloc(sizeof(nd_thread_t));
    ntest_assert_not_null(thread);

    void *stack = nd_thread_stack_alloc(TEST_STACK_SIZE);
    if (stack == ND_NULL) {
        nd_free(thread);
        ntest_assert_not_null(stack);
    }

    ntest_assert_equal(nd_thread_create(thread,
                                        "t_dynamic",
                                        busy_entry,
                                        test_thread_priority(),
                                        (void *)&dynamic_entry_ran,
                                        stack,
                                        TEST_STACK_SIZE,
                                        ND_THREAD_OPT_NONE,
                                        0),
                       ND_EOK);

    ntest_assert_equal(dynamic_entry_ran, 0);
    ntest_assert_equal(nd_thread_abort(thread), ND_EOK);
    ntest_assert_equal(nd_thread_join(thread, ND_TIMEOUT_FOREVER),
                       ND_EOK);
    ntest_assert_equal(thread->stat, ND_THREAD_STAT_DEAD);

    ntest_assert_equal(nd_thread_stack_free(stack), ND_EOK);
    nd_free(thread);
}

static void test_join_current_thread(void)
{
    ntest_assert_equal(nd_thread_join(nd_current_thread, ND_TIMEOUT_FOREVER),
                       ND_EDEADLK);
}

static void test_join_null_thread(void)
{
    ntest_assert_equal(nd_thread_join(ND_NULL, ND_TIMEOUT_FOREVER),
                       ND_EINVAL);
}

static void test_join_nowait_busy(void)
{
    static_entry_ran = 0;

    ntest_assert_equal(nd_thread_create(&busy_thread,
                                        "t_nowait",
                                        busy_entry,
                                        test_thread_priority(),
                                        (void *)&static_entry_ran,
                                        busy_stack,
                                        sizeof(busy_stack),
                                        ND_THREAD_OPT_NONE,
                                        0),
                       ND_EOK);

    ntest_assert_equal(static_entry_ran, 0);
    ntest_assert_equal(nd_thread_join(&busy_thread, ND_TIMEOUT_NOWAIT),
                       ND_EBUSY);

    ntest_assert_equal(nd_thread_abort(&busy_thread), ND_EOK);
    ntest_assert_equal(nd_thread_join(&busy_thread, ND_TIMEOUT_FOREVER),
                       ND_EOK);
}

static void cmd_test_thread_lifecycle(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    NTEST_SUITE_BEGIN("thread_lifecycle");

    NTEST_RUN(test_static_thread_abort_join);
    NTEST_RUN(test_dynamic_thread_abort_join_free);
    NTEST_RUN(test_join_current_thread);
    NTEST_RUN(test_join_null_thread);
    NTEST_RUN(test_join_nowait_busy);

    NTEST_SUITE_END();
}

SHELL_CMD(test_thread_lifecycle, cmd_test_thread_lifecycle, "thread lifecycle test suite");
