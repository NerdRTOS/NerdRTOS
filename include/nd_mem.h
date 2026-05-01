#ifndef __ND_MEM_H__
#define __ND_MEM_H__

#include "nd_config.h"
#include "nd_def.h"
#include "nd_list.h"

#define ND_MEMHEAP_MAGIC    0x1ea01ea0

typedef struct nd_mempool {
    char            *start;
    nd_uint32_t     block_size;
    nd_uint32_t     block_count;
    nd_uint32_t     free_count;
    void            *free_list;

    nd_list_t       wait_list;
} nd_mempool_t;

typedef struct nd_memheap_item {
#if ND_CFG_DEBUG
    nd_uint32_t             magic;
#endif
    struct nd_memheap       *pool;
    struct nd_memheap_item  *next;
    struct nd_memheap_item  *prev;
    struct nd_memheap_item  *next_free;
    struct nd_memheap_item  *prev_free;
} nd_memheap_item_t;

typedef struct nd_memheap {
    nd_memheap_item_t   start_item;
    nd_memheap_item_t   end_item;
    void                *start_addr;
    nd_uint32_t         pool_size;
    nd_uint32_t         available_size;
    nd_list_t           wait_list;
} nd_memheap_t;

#define ITEM_IS_USED(item)    ((nd_ubase_t)(item)->next & 1)
#define ITEM_IS_FREE(item)    (!ITEM_IS_USED(item))
#define ITEM_SET_USED(item)   ((item)->next = (nd_memheap_item_t *)((nd_ubase_t)(item)->next | 1))
#define ITEM_SET_FREE(item)   ((item)->next = (nd_memheap_item_t *)((nd_ubase_t)(item)->next & ~1))

void nd_mempool_init(nd_mempool_t *pool,
                     char *start,
                     nd_uint32_t block_size,
                     nd_uint32_t block_count);
void *nd_mempool_alloc(nd_mempool_t *pool, nd_uint64_t timeout);
void  nd_mempool_free(nd_mempool_t *pool, void *ptr);

nd_err_t nd_memheap_init(nd_memheap_t *heap,
                        void *start_addr,
                        nd_uint32_t size);
void *nd_memheap_alloc(nd_memheap_t *heap, nd_uint32_t size);
void nd_memheap_free(void *ptr);
void *nd_memheap_realloc(nd_memheap_t *heap, void *ptr, nd_uint32_t size);

nd_err_t nd_system_heap_init(void *start_addr, nd_uint32_t size);

void *nd_malloc(nd_uint32_t size);
void  nd_free(void *ptr);
void *nd_realloc(void *ptr, nd_uint32_t size);
void *nd_calloc(nd_uint32_t count, nd_uint32_t size);

#endif
