#ifndef __ND_TIMER_H__
#define __ND_TIMER_H__

#include "nd_config.h"
#include "nd_def.h"
#include "lib/rbtree.h"

typedef enum {
    ND_TIMER_TYPE_ONE_SHOT = 0,
    ND_TIMER_TYPE_PERIODIC = 1
} nd_timer_type_t;

typedef struct nd_timer {
    char            name[ND_NAME_MAX_SIZE];
    nd_timer_type_t type;
    nd_uint64_t     timeout;
    void            (*callback)(void *arg);
    void            *arg;

    struct rb_node  node;
    nd_uint64_t     expire_time;
} nd_timer_t;

nd_err_t nd_timer_init(nd_timer_t *timer,
                       const char *name,
                       nd_timer_type_t type,
                       nd_uint64_t timeout,
                       void (*callback)(void *arg),
                       void *arg);
nd_err_t nd_timer_start(nd_timer_t *timer);
nd_err_t nd_timer_stop(nd_timer_t *timer);
void nd_timer_process(void);

#endif /* __ND_TIMER_H__ */
