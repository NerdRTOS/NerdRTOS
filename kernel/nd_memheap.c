#include "nerd.h"
#include "nd_lock.h"
#include "nd_klibc.h"

static inline nd_memheap_item_t *item_next(nd_memheap_item_t *item)
{
    return (nd_memheap_item_t *)((nd_ubase_t)item->next & ~1);
}

static inline void item_set_next(nd_memheap_item_t *item, nd_memheap_item_t *next)
{
    item->next = (nd_memheap_item_t *)((nd_ubase_t)next | ((nd_ubase_t)item->next & 1));
}

static inline nd_uint32_t item_data_size(nd_memheap_item_t *item)
{
    return (char *)item_next(item) - (char *)item - sizeof(nd_memheap_item_t);
}

static inline nd_memheap_item_t *item_split_at(nd_memheap_item_t *item, nd_uint32_t offset)
{
    return (nd_memheap_item_t *)((char *)item + sizeof(nd_memheap_item_t) + offset);
}

static inline void *item_to_data(nd_memheap_item_t *item)
{
    return (void *)((char *)item + sizeof(nd_memheap_item_t));
}

static inline nd_memheap_item_t *data_to_item(void *ptr)
{
    return (nd_memheap_item_t *)((char *)ptr - sizeof(nd_memheap_item_t));
}

static inline void item_init(nd_memheap_item_t *item, nd_memheap_t *heap)
{
#if ND_DEBUG
    item->magic = ND_MEMHEAP_MAGIC;
#endif
    item->pool  = heap;
}

static inline void phys_list_insert_after(nd_memheap_item_t *anchor, nd_memheap_item_t *new_item)
{
    new_item->next      = item_next(anchor);
    new_item->prev      = anchor;
    item_next(anchor)->prev = new_item;
    item_set_next(anchor, new_item);
}

static inline void free_list_insert_after(nd_memheap_item_t *anchor, nd_memheap_item_t *item)
{
    item->next_free                 = anchor->next_free;
    item->prev_free                 = anchor;
    anchor->next_free->prev_free    = item;
    anchor->next_free               = item;
}

static inline void phys_list_remove(nd_memheap_item_t *item)
{
    item_set_next(item->prev, item_next(item));
    item_next(item)->prev = item->prev;
}

static inline void free_list_remove(nd_memheap_item_t *item)
{
    item->prev_free->next_free = item->next_free;
    item->next_free->prev_free = item->prev_free;
}

static inline void free_list_replace(nd_memheap_item_t *old_item, nd_memheap_item_t *new_item)
{
    new_item->prev_free             = old_item->prev_free;
    new_item->next_free             = old_item->next_free;
    old_item->next_free->prev_free  = new_item;
    old_item->prev_free->next_free  = new_item;
}

static void memheap_coalesce(nd_memheap_t *heap, nd_memheap_item_t *item)
{
    heap->available_size += item_data_size(item);

    nd_memheap_item_t *next_item = item_next(item);

    if (ITEM_IS_FREE(next_item)) {
        free_list_remove(next_item);
        phys_list_remove(next_item);
        heap->available_size += sizeof(nd_memheap_item_t);
    }

    if (ITEM_IS_FREE(item->prev)) {
        phys_list_remove(item);
        heap->available_size += sizeof(nd_memheap_item_t);
        item = item->prev;
    } else {
        free_list_insert_after(&heap->start_item, item);
    }

    ITEM_SET_FREE(item);
}

static void *_memheap_alloc(nd_memheap_t *heap, nd_uint32_t size)
{
    nd_uint32_t block_size = 0;

    nd_memheap_item_t *item = heap->start_item.next_free;

    while (item != &heap->start_item) {
        block_size = item_data_size(item);

        if (block_size >= size)
            break;

        item = item->next_free;
    }

    if (item == &heap->start_item)
        return ND_NULL;

    if (block_size >= size + sizeof(nd_memheap_item_t)) {
        nd_memheap_item_t *new_item = item_split_at(item, size);

        item_init(new_item, heap);

        free_list_replace(item, new_item);
        phys_list_insert_after(item, new_item);

        heap->available_size -= size + sizeof(nd_memheap_item_t);
    } else {
        free_list_remove(item);

        heap->available_size -= block_size;
    }

    ITEM_SET_USED(item);

    return item_to_data(item);
}

static void _memheap_free(void *ptr)
{
    nd_memheap_item_t *item = data_to_item(ptr);
#if ND_DEBUG
    ND_ASSERT(item->magic == ND_MEMHEAP_MAGIC);
#endif
    memheap_coalesce(item->pool, item);
}

nd_err_t nd_memheap_init(nd_memheap_t *heap,
                        void *start_addr,
                        nd_uint32_t size)
{
    nd_kernel_def();
    nd_kernel_lock();

    heap->start_addr        = start_addr;
    heap->pool_size         = size;
    heap->available_size    = size - sizeof(nd_memheap_item_t);

    nd_list_init(&heap->wait_list);

    nd_memheap_item_t *item_0 = (nd_memheap_item_t *)start_addr;

    item_init(&heap->start_item, heap);
    heap->start_item.next       = item_0;
    heap->start_item.prev       = item_0;
    heap->start_item.next_free  = item_0;
    heap->start_item.prev_free  = item_0;
    ITEM_SET_USED(&heap->start_item);

    item_init(item_0, heap);
    item_0->next            = &heap->end_item;
    item_0->prev            = &heap->start_item;
    item_0->next_free       = &heap->start_item;
    item_0->prev_free       = &heap->start_item;

    item_init(&heap->end_item, heap);
    heap->end_item.next     = &heap->start_item;
    heap->end_item.prev     = item_0;
    ITEM_SET_USED(&heap->end_item);

    nd_kernel_unlock();

    return ND_EOK;
}

void *nd_memheap_alloc(nd_memheap_t *heap, nd_uint32_t size)
{
    nd_kernel_def();
    nd_kernel_lock();

    void *ret = _memheap_alloc(heap, size);

    nd_kernel_unlock();

    return ret;
}

void nd_memheap_free(void *ptr)
{
    nd_kernel_def();
    nd_kernel_lock();

    if (ptr == ND_NULL) {
        nd_kernel_unlock();
        return;
    }

    _memheap_free(ptr);

    nd_kernel_unlock();
}

void *nd_memheap_realloc(nd_memheap_t *heap, void *ptr, nd_uint32_t size)
{
    nd_kernel_def();
    nd_kernel_lock();

    if (ptr == ND_NULL) {
        void *ret = _memheap_alloc(heap, size);
        nd_kernel_unlock();
        return ret;
    }

    nd_memheap_item_t *item = data_to_item(ptr);
    nd_uint32_t block_size = item_data_size(item);

#if ND_DEBUG
    ND_ASSERT(item->magic == ND_MEMHEAP_MAGIC);
#endif

    if (size == block_size) {
        nd_kernel_unlock();
        return ptr;
    }

    if (size < block_size) {
        if (block_size - size >= sizeof(nd_memheap_item_t)) {
            nd_memheap_item_t *new_item = item_split_at(item, size);

            item_init(new_item, heap);
            phys_list_insert_after(item, new_item);
            memheap_coalesce(heap, new_item);
        }

        nd_kernel_unlock();
        return ptr;
    }

    nd_memheap_item_t *next_item = item_next(item);
    nd_uint32_t next_block_size = item_data_size(next_item);

    if (ITEM_IS_FREE(next_item) && block_size + sizeof(nd_memheap_item_t) + next_block_size >= size) {
        if (block_size + next_block_size - size >= sizeof(nd_memheap_item_t)) {
            nd_memheap_item_t *new_item = item_split_at(item, size);

            free_list_remove(next_item);
            phys_list_remove(next_item);

            item_init(new_item, heap);
            phys_list_insert_after(item, new_item);
            free_list_insert_after(&heap->start_item, new_item);
        } else {
            free_list_remove(next_item);
            phys_list_remove(next_item);

            heap->available_size += sizeof(nd_memheap_item_t);
        }

        heap->available_size -= size - block_size;
        nd_kernel_unlock();
        return ptr;
    }

    void *new_ptr = _memheap_alloc(heap, size);

    if (new_ptr == ND_NULL) {
        nd_kernel_unlock();
        return ND_NULL;
    }

    nd_memcpy(new_ptr, ptr, block_size);

    _memheap_free(ptr);

    nd_kernel_unlock();

    return new_ptr;
}
