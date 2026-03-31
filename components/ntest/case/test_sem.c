#include "ntest.h"
#include "nerd.h"

static void test_sem_init(void)
{
    nd_sem_t sem;
    nd_sem_init(&sem, 5);

    ntest_assert_equal(sem.val, 5);
    ntest_assert_true(nd_list_is_empty(&sem.wait_list));
}

static void test_sem_take(void)
{
    nd_sem_t sem;

    nd_sem_init(&sem, 1);

    nd_err_t err = nd_sem_take(&sem, ND_TIMEOUT_NOWAIT);

    ntest_assert_equal(err, ND_EOK);
    ntest_assert_equal(sem.val, 0);
}

static void test_sem_take_nowait(void)
{
    nd_sem_t sem;

    nd_sem_init(&sem, 0);

    nd_err_t err = nd_sem_take(&sem, ND_TIMEOUT_NOWAIT);

    ntest_assert_equal(err, ND_EBUSY);
}

static void test_sem_release(void)
{
    nd_sem_t sem;

    nd_sem_init(&sem, 0);            

    nd_sem_release(&sem);
    ntest_assert_equal(sem.val, 1);

    nd_sem_release(&sem);
    ntest_assert_equal(sem.val, 2);
}

static void test_sem_take_release(void)
{
    nd_sem_t sem;

    nd_sem_init(&sem, 3);

    ntest_assert_equal(nd_sem_take(&sem, ND_TIMEOUT_NOWAIT), ND_EOK);
    ntest_assert_equal(nd_sem_take(&sem, ND_TIMEOUT_NOWAIT), ND_EOK);
    ntest_assert_equal(nd_sem_take(&sem, ND_TIMEOUT_NOWAIT), ND_EOK);
    ntest_assert_equal(nd_sem_take(&sem, ND_TIMEOUT_NOWAIT), ND_EBUSY); // 第4次应该失败
    ntest_assert_equal(sem.val, 0);

    nd_sem_release(&sem);

    ntest_assert_equal(nd_sem_take(&sem, ND_TIMEOUT_NOWAIT), ND_EOK);   // release 后又能 take
}

static void cmd_test_sem(int argc, char *argv[])
{
    (void)argc; (void)argv;

    NTEST_SUITE_BEGIN("sem");
    NTEST_RUN(test_sem_init);
    NTEST_RUN(test_sem_take);
    NTEST_RUN(test_sem_take_nowait);
    NTEST_RUN(test_sem_release);
    NTEST_RUN(test_sem_take_release);
    NTEST_SUITE_END();
}

SHELL_CMD(test_sem, cmd_test_sem, "semaphore test suite");
