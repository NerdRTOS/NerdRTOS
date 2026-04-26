#include "nerd.h"
#include "nd_klibc.h"

static nd_memheap_t k_system_heap;

nd_err_t nd_system_heap_init(void *start_addr, nd_uint32_t size)
{
    return nd_memheap_init(&k_system_heap, start_addr, size);
}

void *nd_malloc(nd_uint32_t size)
{
    return nd_memheap_alloc(&k_system_heap, size);
}

void nd_free(void *ptr)
{
    nd_memheap_free(ptr);
}

void *nd_realloc(void *ptr, nd_uint32_t size)
{
    return nd_memheap_realloc(&k_system_heap, ptr, size);
}

void *nd_calloc(nd_uint32_t count, nd_uint32_t size)
{
    if (size == 0 || count == 0) {
        return ND_NULL;
    }

    nd_uint64_t total = (nd_uint64_t)count * size;

    if (total > (nd_uint32_t)-1) {
        return ND_NULL;
    }

    void *ptr = nd_malloc((nd_uint32_t)total);

    if (ptr != ND_NULL) {
        nd_memset(ptr, 0, (nd_uint32_t)total);
    }

    return ptr;
}
