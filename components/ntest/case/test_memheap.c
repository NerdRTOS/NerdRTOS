#include "nerd.h"
#include "ntest.h"
#include "nd_shell.h"
#include "nd_mem.h"
#include "nd_klibc.h"

#define TEST_HEAP_SIZE 1024

static nd_uint8_t   heap_buf[TEST_HEAP_SIZE] __attribute__((aligned(4)));
static nd_memheap_t heap;

static void heap_setup(void)
{
    nd_memheap_init(&heap, heap_buf, TEST_HEAP_SIZE);
}

static void test_memheap_init(void)
{
    heap_setup();

    ntest_assert_equal(heap.pool_size, TEST_HEAP_SIZE);
    ntest_assert_equal(heap.available_size,
                       TEST_HEAP_SIZE - sizeof(nd_memheap_item_t));
    ntest_assert_true(ITEM_IS_USED(&heap.start_item));
    ntest_assert_true(ITEM_IS_USED(&heap.end_item));
#if ND_CFG_DEBUG
    ntest_assert_equal(heap.start_item.magic, ND_MEMHEAP_MAGIC);
#endif
}

static void test_memheap_alloc_basic(void)
{
    heap_setup();

    void *p = nd_memheap_alloc(&heap, 64);
    ntest_assert_not_null(p);

    ntest_assert_true((nd_uint8_t *)p >= heap_buf);
    ntest_assert_true((nd_uint8_t *)p + 64 <= heap_buf + TEST_HEAP_SIZE);
}

static void test_memheap_free_basic(void)
{
    heap_setup();

    nd_uint32_t avail_before = heap.available_size;

    void *p = nd_memheap_alloc(&heap, 64);
    ntest_assert_not_null(p);
    ntest_assert_true(heap.available_size < avail_before);

    nd_memheap_free(p);
    ntest_assert_equal(heap.available_size, avail_before);
}

static void test_memheap_free_null(void)
{
    nd_memheap_free(ND_NULL);
}

static void test_memheap_multiple_alloc(void)
{
    heap_setup();

    void *p1 = nd_memheap_alloc(&heap, 32);
    void *p2 = nd_memheap_alloc(&heap, 32);
    void *p3 = nd_memheap_alloc(&heap, 32);

    ntest_assert_not_null(p1);
    ntest_assert_not_null(p2);
    ntest_assert_not_null(p3);

    ntest_assert_not_equal(p1, p2);
    ntest_assert_not_equal(p2, p3);
    ntest_assert_not_equal(p1, p3);

    nd_memheap_free(p1);
    nd_memheap_free(p2);
    nd_memheap_free(p3);
}

static void test_memheap_coalesce(void)
{
    heap_setup();

    nd_uint32_t avail_before = heap.available_size;

    void *p1 = nd_memheap_alloc(&heap, 64);
    void *p2 = nd_memheap_alloc(&heap, 64);
    void *p3 = nd_memheap_alloc(&heap, 64);

    nd_memheap_free(p2);
    nd_memheap_free(p1);
    nd_memheap_free(p3);

    ntest_assert_equal(heap.available_size, avail_before);

    void *p = nd_memheap_alloc(&heap, avail_before - sizeof(nd_memheap_item_t));
    ntest_assert_not_null(p);
    nd_memheap_free(p);
}

static void test_memheap_realloc_null(void)
{
    heap_setup();

    void *p = nd_memheap_realloc(&heap, ND_NULL, 64);
    ntest_assert_not_null(p);
    nd_memheap_free(p);
}

static void test_memheap_realloc_shrink(void)
{
    heap_setup();

    void *p = nd_memheap_alloc(&heap, 128);
    ntest_assert_not_null(p);

    nd_memset(p, 0xAB, 128);

    void *p2 = nd_memheap_realloc(&heap, p, 32);

    ntest_assert_equal(p, p2);

    nd_uint8_t *b = (nd_uint8_t *)p2;
    ntest_assert_equal(b[0], 0xAB);
    ntest_assert_equal(b[31], 0xAB);

    nd_memheap_free(p2);
}

static void test_memheap_realloc_grow(void)
{
    heap_setup();

    void *p = nd_memheap_alloc(&heap, 32);
    ntest_assert_not_null(p);

    nd_memset(p, 0xCD, 32);

    void *p2 = nd_memheap_realloc(&heap, p, 128);
    ntest_assert_not_null(p2);

    nd_uint8_t *b = (nd_uint8_t *)p2;

    ntest_assert_equal(b[0], 0xCD);
    ntest_assert_equal(b[31], 0xCD);

    nd_memheap_free(p2);
}

static void test_memheap_realloc_same_size(void)
{
    heap_setup();

    void *p = nd_memheap_alloc(&heap, 64);
    ntest_assert_not_null(p);

    void *p2 = nd_memheap_realloc(&heap, p, 64);
    ntest_assert_equal(p, p2);

    nd_memheap_free(p2);
}

static void test_memheap_stress(void)
{
    heap_setup();

    nd_uint32_t avail_before = heap.available_size;
    void *ptrs[8];

    for (int i = 0; i < 8; i++) {
        ptrs[i] = nd_memheap_alloc(&heap, 32);
        ntest_assert_not_null(ptrs[i]);
    }

    for (int i = 7; i >= 0; i--)
        nd_memheap_free(ptrs[i]);

    ntest_assert_equal(heap.available_size, avail_before);
}

static void cmd_test_memheap(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    NTEST_SUITE_BEGIN("memheap");

    NTEST_RUN(test_memheap_init);
    NTEST_RUN(test_memheap_alloc_basic);
    NTEST_RUN(test_memheap_free_basic);
    NTEST_RUN(test_memheap_free_null);
    NTEST_RUN(test_memheap_multiple_alloc);
    NTEST_RUN(test_memheap_coalesce);
    NTEST_RUN(test_memheap_realloc_null);
    NTEST_RUN(test_memheap_realloc_shrink);
    NTEST_RUN(test_memheap_realloc_grow);
    NTEST_RUN(test_memheap_realloc_same_size);
    NTEST_RUN(test_memheap_stress);

    NTEST_SUITE_END();
}

SHELL_CMD(test_memheap, cmd_test_memheap, "memheap test suite");
