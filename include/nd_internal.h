#ifndef __ND_INTERNAL_H__
#define __ND_INTERNAL_H__

#include "nd_def.h"
#include "nd_list.h"

struct nd_thread;

void nd_enter_interrupt(void);
void nd_exit_interrupt(void);

void nd_thread_list_init(void);
nd_list_t *nd_thread_list_get(void);

void nd_thread_ready_add_head(struct nd_thread *thread);
void nd_thread_ready_add_tail(struct nd_thread *thread);
void nd_thread_ready_remove(struct nd_thread *thread);

void nd_thread_pend(nd_list_t *wait_list, nd_uint64_t timeout);
struct nd_thread *nd_thread_wakeup(nd_list_t *wait_list);
void nd_thread_entry(void (*entry)(void *), void *parameter);

void nd_thread_slice_timeout(void *arg);

void nd_try_schedule_irqsave(void);
void nd_try_schedule(void);

nd_uint64_t nd_hw_get_current(void);
nd_uint64_t nd_idle_runtime_get(void);

void nd_context_switch_cb(struct nd_thread *next);
void nd_schedule_irq_exit(void);

extern nd_uint64_t last_switch_time;

#endif /* __ND_INTERNAL_H__ */
