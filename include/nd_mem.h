#ifndef __ND_MEM_H__
#define __ND_MEM_H__

#include "nd_def.h"
#include "nd_list.h"

typedef struct nd_mempool {
    char            *start;
    nd_uint32_t     block_size;
    nd_uint32_t     block_count;
    nd_uint32_t     free_count;
    nd_uint32_t     *bitmap;

    nd_list_t       wait_list;
} nd_mempool_t;

void nd_mempool_init(nd_mempool_t *pool,
                     char *start,
                     nd_uint32_t block_size,
                     nd_uint32_t block_count,
                     nd_uint32_t *bitmap);

nd_err_t nd_mempool_alloc(nd_mempool_t *pool, nd_uint32_t timeout)

#endif
