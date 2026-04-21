#ifndef __NERD_H__
#define __NERD_H__

#include "nd_config.h"
#include "nd_def.h"
#include "nd_hw.h"
#include "nd_bitops.h"
#include "nd_thread.h"
#include "nd_ipc.h"
#include "nd_mem.h"

#define ND_THREAD_PRIORITY_MAX      32

#if ND_CFG_TICKLESS
    #define ND_TIMEOUT_S(s)     ((nd_uint64_t)(s)  * ND_TICKLESS_FREQ)
    #define ND_TIMEOUT_MS(ms)   ((nd_uint64_t)(ms) * ND_TICKLESS_FREQ / 1000ULL)
    #define ND_TIMEOUT_US(us)   ((nd_uint64_t)(us) * ND_TICKLESS_FREQ / 1000000ULL)
#else
    #define ND_TIMEOUT_S(s)     ((nd_uint64_t)(s) * ND_TICKS_PER_SEC)

    #define ND_TIMEOUT_MS(ms)   \
        (((nd_uint64_t)(ms) * ND_TICKS_PER_SEC) / 1000)
    #define ND_TIMEOUT_US(us)   \
        (((nd_uint64_t)(us) * ND_TICKS_PER_SEC) / 1000000)
#endif

void nd_scheduler_init(void);
void nd_scheduler_start(void);

void nd_thread_yield(void);
void nd_thread_delay(nd_uint64_t delay);

void nd_enter_interrupt(void);
void nd_exit_interrupt(void);

void nd_thread_ready_add_head(nd_thread_t *thread);
void nd_thread_ready_remove(nd_thread_t *thread);

void nd_thread_slice_timeout(void *arg);

void nd_scheduler(void);
void nd_try_schedule_irqsave(void);
void nd_try_schedule(void);

nd_uint64_t nd_hw_get_current(void);

#endif /* __NERD_H__ */
