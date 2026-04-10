#include "nerd.h"
#include "ntest.h"
#include "nd_shell.h"
#include "nd_mem.h"
#include "nd_klibc.h"

#define TEST_BLOCK_SIZE  32
#define TEST_BLOCK_COUNT 8
#define TEST_POOL_SIZE   (TEST_BLOCK_SIZE * TEST_BLOCK_COUNT)

static char pool_buf[TEST_POOL_SIZE] __attribute__((aligned(4)));
static nd_mempool_t pool;

static void pool_setup(void)
{
    nd_mempool_init(&pool, pool_buf, TEST_BLOCK_SIZE, TEST_BLOCK_COUNT);
}

static void test_mempool_init(void)
{
    pool_setup();

    ntest_assert_equal(pool.block_size, TEST_BLOCK_SIZE);
    ntest_assert_equal(pool.block_count, TEST_BLOCK_COUNT);
    ntest_assert_equal(pool.free_count, TEST_BLOCK_COUNT);
    ntest_assert_not_null(pool.free_list);
}

static void test_mempool_alloc_basic(void)
{
    pool_setup();

    void *p = nd_mempool_alloc(&pool, ND_TIMEOUT_NOWAIT);
    ntest_assert_not_null(p);
    ntest_assert_equal(pool.free_count, TEST_BLOCK_COUNT - 1);

    ntest_assert_true((char *)p >= pool_buf);
    ntest_assert_true((char *)p < pool_buf + TEST_POOL_SIZE);
}

static void test_mempool_free_basic(void)
{
    pool_setup();

    void *p = nd_mempool_alloc(&pool, ND_TIMEOUT_NOWAIT);
    ntest_assert_not_null(p);

    nd_mempool_free(&pool, p);
    ntest_assert_equal(pool.free_count, TEST_BLOCK_COUNT);
}

static void test_mempool_alloc_all(void)
{
    pool_setup();

    void *ptrs[TEST_BLOCK_COUNT];

    for (int i = 0; i < TEST_BLOCK_COUNT; i++) {
        ptrs[i] = nd_mempool_alloc(&pool, ND_TIMEOUT_NOWAIT);
        ntest_assert_not_null(ptrs[i]);
    }

    ntest_assert_equal(pool.free_count, 0);

    void *p = nd_mempool_alloc(&pool, ND_TIMEOUT_NOWAIT);
    ntest_assert_null(p);

    for (int i = 0; i < TEST_BLOCK_COUNT; i++)
        nd_mempool_free(&pool, ptrs[i]);
}

static void test_mempool_no_overlap(void)
{
    pool_setup();

    void *ptrs[TEST_BLOCK_COUNT];

    for (int i = 0; i < TEST_BLOCK_COUNT; i++)
        ptrs[i] = nd_mempool_alloc(&pool, ND_TIMEOUT_NOWAIT);

    for (int i = 0; i < TEST_BLOCK_COUNT; i++) {
        for (int j = i + 1; j < TEST_BLOCK_COUNT; j++)
            ntest_assert_not_equal(ptrs[i], ptrs[j]);
    }

    for (int i = 0; i < TEST_BLOCK_COUNT; i++)
        nd_mempool_free(&pool, ptrs[i]);
}

static void test_mempool_alignment(void)
{
    pool_setup();

    void *p = nd_mempool_alloc(&pool, ND_TIMEOUT_NOWAIT);
    ntest_assert_not_null(p);
    ntest_assert_equal((nd_ubase_t)p % sizeof(void *), 0);

    nd_mempool_free(&pool, p);
}

static void test_mempool_reuse(void)
{
    pool_setup();

    void *p1 = nd_mempool_alloc(&pool, ND_TIMEOUT_NOWAIT);
    nd_mempool_free(&pool, p1);

    void *p2 = nd_mempool_alloc(&pool, ND_TIMEOUT_NOWAIT);
    ntest_assert_equal(p1, p2);

    nd_mempool_free(&pool, p2);
}

static void test_mempool_free_count(void)
{
    pool_setup();

    void *ptrs[TEST_BLOCK_COUNT];

    for (int i = 0; i < TEST_BLOCK_COUNT; i++) {
        ptrs[i] = nd_mempool_alloc(&pool, ND_TIMEOUT_NOWAIT);
        ntest_assert_equal(pool.free_count, (nd_uint32_t)(TEST_BLOCK_COUNT - i - 1));
    }

    for (int i = 0; i < TEST_BLOCK_COUNT; i++) {
        nd_mempool_free(&pool, ptrs[i]);
        ntest_assert_equal(pool.free_count, (nd_uint32_t)(i + 1));
    }
}

static void test_mempool_data_integrity(void)
{
    pool_setup();

    void *p1 = nd_mempool_alloc(&pool, ND_TIMEOUT_NOWAIT);
    void *p2 = nd_mempool_alloc(&pool, ND_TIMEOUT_NOWAIT);
    ntest_assert_not_null(p1);
    ntest_assert_not_null(p2);

    nd_memset(p1, 0xAA, TEST_BLOCK_SIZE);
    nd_memset(p2, 0x55, TEST_BLOCK_SIZE);

    nd_uint8_t *b1 = (nd_uint8_t *)p1;
    nd_uint8_t *b2 = (nd_uint8_t *)p2;

    ntest_assert_equal(b1[0], 0xAA);
    ntest_assert_equal(b1[TEST_BLOCK_SIZE - 1], 0xAA);
    ntest_assert_equal(b2[0], 0x55);
    ntest_assert_equal(b2[TEST_BLOCK_SIZE - 1], 0x55);

    nd_mempool_free(&pool, p1);
    nd_mempool_free(&pool, p2);
}

static void cmd_test_mempool(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    NTEST_SUITE_BEGIN("mempool");

    NTEST_RUN(test_mempool_init);
    NTEST_RUN(test_mempool_alloc_basic);
    NTEST_RUN(test_mempool_free_basic);
    NTEST_RUN(test_mempool_alloc_all);
    NTEST_RUN(test_mempool_no_overlap);
    NTEST_RUN(test_mempool_alignment);
    NTEST_RUN(test_mempool_reuse);
    NTEST_RUN(test_mempool_free_count);
    NTEST_RUN(test_mempool_data_integrity);

    NTEST_SUITE_END();
}

SHELL_CMD(test_mempool, cmd_test_mempool, "mempool test suite");
