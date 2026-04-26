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

static void return_entry(void *parameter)
{
    volatile nd_uint32_t *flag = (volatile nd_uint32_t *)parameter;

    (*flag)++;
}

static void test_static_thread_return_join(void)
{
    static_entry_ran = 0;

    ntest_assert_equal(nd_thread_create(&static_thread,
                                        "t_static",
                                        return_entry,
                                        nd_current_thread->priority,
                                        (void *)&static_entry_ran,
                                        static_stack,
                                        sizeof(static_stack),
                                        0),
                       ND_EOK);

    ntest_assert_equal(nd_thread_join(&static_thread, ND_TIMEOUT_FOREVER),
                       ND_EOK);

    ntest_assert_equal(static_entry_ran, 1);
    ntest_assert_equal(static_thread.stat, ND_THREAD_STAT_DEAD);
}

static void test_dynamic_thread_return_join_detach(void)
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
                                        return_entry,
                                        nd_current_thread->priority,
                                        (void *)&dynamic_entry_ran,
                                        stack,
                                        TEST_STACK_SIZE,
                                        0),
                       ND_EOK);

    ntest_assert_equal(nd_thread_join(thread, ND_TIMEOUT_FOREVER),
                       ND_EOK);

    ntest_assert_equal(dynamic_entry_ran, 1);
    ntest_assert_equal(thread->stat, ND_THREAD_STAT_DEAD);

    ntest_assert_equal(nd_thread_detach(thread), ND_EOK);
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

static void test_detach_null_thread(void)
{
    ntest_assert_equal(nd_thread_detach(ND_NULL), ND_EINVAL);
}

static void test_detach_live_thread_should_fail(void)
{
    static_entry_ran = 0;

    ntest_assert_equal(nd_thread_init(&busy_thread,
                                      "t_busy",
                                      return_entry,
                                      nd_current_thread->priority,
                                      (void *)&static_entry_ran,
                                      busy_stack,
                                      sizeof(busy_stack),
                                      0),
                       ND_EOK);

    busy_thread.stat = ND_THREAD_STAT_READY;

    ntest_assert_equal(nd_thread_detach(&busy_thread), ND_ERROR);
    ntest_assert_equal(static_entry_ran, 0);
}

static void test_join_nowait_busy(void)
{
    static_entry_ran = 0;

    ntest_assert_equal(nd_thread_init(&busy_thread,
                                      "t_nowait",
                                      return_entry,
                                      nd_current_thread->priority,
                                      (void *)&static_entry_ran,
                                      busy_stack,
                                      sizeof(busy_stack),
                                      0),
                       ND_EOK);

    busy_thread.stat = ND_THREAD_STAT_READY;

    ntest_assert_equal(nd_thread_join(&busy_thread, ND_TIMEOUT_NOWAIT),
                       ND_EBUSY);

    ntest_assert_equal(static_entry_ran, 0);
}

static void cmd_test_thread_lifecycle(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    NTEST_SUITE_BEGIN("thread_lifecycle");

    NTEST_RUN(test_static_thread_return_join);
    NTEST_RUN(test_dynamic_thread_return_join_detach);
    NTEST_RUN(test_join_current_thread);
    NTEST_RUN(test_join_null_thread);
    NTEST_RUN(test_detach_null_thread);
    NTEST_RUN(test_detach_live_thread_should_fail);
    NTEST_RUN(test_join_nowait_busy);

    NTEST_SUITE_END();
}

SHELL_CMD(test_thread_lifecycle, cmd_test_thread_lifecycle, "thread lifecycle test suite");
