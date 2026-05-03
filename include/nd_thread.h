#ifndef __ND_THREAD_H__
#define __ND_THREAD_H__

#include "nd_config.h"
#include "nd_def.h"
#include "nd_list.h"
#include "nd_timer.h"

typedef struct {
    nd_uint64_t time_slice;
    nd_uint64_t slice_left;
    nd_uint64_t slice_start;
    nd_timer_t  slice_timer;
} nd_thread_slice_t;

typedef struct {
    nd_list_t taken_list;
    struct nd_mutex *pending;
} nd_thread_mutex_ctx_t;

typedef enum {
    ND_EVENT_AND = 0,
    ND_EVENT_OR,
} nd_event_opt_t;

typedef struct {
    nd_uint32_t set;
    nd_event_opt_t opt;
} nd_thread_event_ctx_t;

typedef enum {
    ND_THREAD_STAT_INIT = 0,
    ND_THREAD_STAT_READY,
    ND_THREAD_STAT_RUNNING,
    ND_THREAD_STAT_BLOCK,
    ND_THREAD_STAT_SUSPEND,
    ND_THREAD_STAT_DEAD,
} nd_thread_stat_t;

typedef enum {
    ND_THREAD_OPT_NONE      = 0u,
    ND_THREAD_OPT_ESSENTIAL = (1u << 0),
} nd_thread_option_t;

typedef struct {
    nd_uint64_t total;
    nd_uint64_t last_total;
    nd_uint32_t load;
} nd_thread_usage_t;

typedef struct nd_thread {
    void          *sp;
    void          *entry;
    void          *parameter;

    void          *stack_addr;
    nd_size_t     stack_size;

    nd_uint8_t    priority;
    nd_uint8_t    init_priority;
    nd_uint8_t    yield;
    nd_err_t      error;
    nd_thread_stat_t stat;
    nd_uint32_t   options;

    nd_timer_t    timer;

    nd_list_t     qnode;
    nd_list_t     tlist;
    nd_list_t     join_list;

    nd_thread_slice_t       slice;
    nd_thread_mutex_ctx_t   mutex;
    nd_thread_event_ctx_t   event;
    nd_thread_usage_t       usage;

    char          name[ND_NAME_MAX_SIZE];
} nd_thread_t;

extern nd_thread_t *nd_current_thread;

void *nd_thread_stack_alloc(nd_size_t size);
nd_err_t nd_thread_stack_free(void *stack);

nd_err_t nd_thread_create(nd_thread_t   *thread,
                          const char    *name,
                          void          (*entry)(void *parameter),
                          nd_uint8_t    priority,
                          void          *parameter,
                          void          *stack,
                          nd_size_t     stack_size,
                          nd_uint32_t   options,
                          nd_uint64_t   time_slice);

nd_err_t nd_thread_abort(nd_thread_t *thread);
nd_err_t nd_thread_join(nd_thread_t *thread, nd_uint64_t timeout);

nd_err_t nd_thread_suspend(nd_thread_t *thread);
nd_err_t nd_thread_resume(nd_thread_t *thread);

nd_uint32_t nd_thread_stack_used(nd_thread_t *thread);

#endif /* __ND_THREAD_H__*/
