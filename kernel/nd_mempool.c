#include "nerd.h"
#include "nd_internal.h"
#include "nd_lock.h"

void nd_mempool_init(nd_mempool_t *pool,
                    char *start,
                    nd_uint32_t block_size,
                    nd_uint32_t block_count)
{
    nd_kernel_def();
    nd_kernel_lock();

    pool->start         = start;
    pool->block_size    = ND_ALIGN(block_size, sizeof(void *));
    pool->block_count   = block_count;
    pool->free_count    = block_count;

    nd_list_init(&pool->wait_list);

    void **current;

    for (nd_uint32_t i = 0; i < block_count; i++) {
        current = (void **)(start + i * block_size);

        if (i < block_count - 1)
            *current = (void *)(start + (i + 1) * block_size);
        else
            *current = ND_NULL;
    }

    pool->free_list = (void *)start;

    nd_kernel_unlock();
}

void *nd_mempool_alloc(nd_mempool_t *pool, nd_uint64_t timeout)
{
    nd_kernel_def();
    nd_kernel_lock();

    while (pool->free_count == 0) {
        if (timeout == ND_TIMEOUT_NOWAIT) {
            nd_kernel_unlock();
            return ND_NULL;
        }

        nd_thread_pend(&pool->wait_list, timeout);
        nd_scheduler();

        if (nd_current_thread->error == ND_ETIMEOUT) {
            nd_kernel_unlock();
            return ND_NULL;
        }
    }

    void *block = pool->free_list;
    pool->free_list = *(void **)block;
    pool->free_count--;

    nd_kernel_unlock();
    return block;
}

void nd_mempool_free(nd_mempool_t *pool, void *ptr)
{
    nd_kernel_def();
    nd_kernel_lock();

#if ND_DEBUG
    ND_ASSERT(ptr >= (void *)pool->start);
    ND_ASSERT(ptr < (void *)(pool->start + pool->block_size * pool->block_count));
    ND_ASSERT(((char *)ptr - pool->start) % pool->block_size == 0);
#endif

    *(void **)ptr = pool->free_list;
    pool->free_list = ptr;
    pool->free_count++;

    if (!nd_list_is_empty(&pool->wait_list)) {
        nd_thread_wakeup(&pool->wait_list);
        nd_scheduler();
    }

    nd_kernel_unlock();
}
