#ifndef __ND_THREAD_H__
#define __ND_THREAD_H__

#include "nd_config.h"
#include "nd_def.h"
#include "nd_list.h"
#include "nd_timer.h"

typedef enum {
    ND_THREAD_STAT_INIT = 0,
    ND_THREAD_STAT_READY,
    ND_THREAD_STAT_RUNNING,
    ND_THREAD_STAT_BLOCK,
    ND_THREAD_STAT_END,
    ND_THREAD_STAT_SUSPEND,
} nd_thread_stat_t;

typedef enum {
    ND_EVENT_AND = 0,
    ND_EVENT_OR,
} nd_event_opt_t;

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
    nd_uint32_t   stack_size;
    nd_uint8_t    priority;

    nd_timer_t    timer;
    nd_list_t     prio_list;
    nd_list_t     tlist;
    nd_uint8_t    yield;

    nd_uint64_t   time_slice;
    nd_uint64_t   slice_left;
    nd_uint64_t   slice_start;
    nd_timer_t    slice_timer;

    nd_uint8_t    init_priority;
    nd_list_t     taken_list;

    nd_err_t         error;
    nd_thread_stat_t stat;

    nd_event_opt_t event_opt;
    nd_uint32_t    event_set;

    nd_thread_usage_t usage;

    char          name[ND_NAME_MAX_SIZE];
} nd_thread_t;

extern nd_thread_t *nd_current_thread;

nd_err_t nd_thread_init(nd_thread_t    *thread,
                        char           *name,
                        void           (*entry)(void *parameter),
                        nd_uint8_t     priority,
                        void           *parameter,
                        void           *stack_addr,
                        nd_uint32_t    stack_size,
                        nd_uint64_t    time_slice);

nd_err_t nd_thread_suspend(nd_thread_t *thread);
nd_err_t nd_thread_resume(nd_thread_t *thread);

nd_uint32_t nd_thread_stack_used(nd_thread_t *thread);

#endif /* __ND_THREAD_H__*/
