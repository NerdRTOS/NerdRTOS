#ifndef __ND_INTERNAL_H__
#define __ND_INTERNAL_H__

#include "nd_def.h"
#include "nd_list.h"
#include "nd_thread.h"

void nd_enter_interrupt(void);
void nd_exit_interrupt(void);

void nd_thread_list_init(void);
nd_list_t *nd_thread_list_get(void);

void nd_thread_ready_add_head(nd_thread_t *thread);
void nd_thread_ready_add_tail(nd_thread_t *thread);
void nd_thread_ready_remove(nd_thread_t *thread);

void nd_thread_pend(nd_list_t *wait_list, nd_uint64_t timeout);
nd_thread_t *nd_thread_wakeup(nd_list_t *wait_list);
void nd_thread_entry(void (*entry)(void *), void *parameter);

void nd_thread_slice_timeout(void *arg);

void nd_try_schedule_irqsave(void);
void nd_try_schedule(void);

nd_uint64_t nd_hw_get_current(void);
nd_uint64_t nd_idle_runtime_get(void);

void nd_context_switch_cb(nd_thread_t *next);
void nd_schedule_core_irq(void);

extern nd_uint64_t last_switch_time;

#endif /* __ND_INTERNAL_H__ */
