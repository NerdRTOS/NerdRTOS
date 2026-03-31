#ifndef __ND_HW_H__
#define __ND_HW_H__

#include "nd_def.h"

void nd_hw_irq_disable(void);
void nd_hw_irq_enable(void);
nd_size_t nd_hw_irq_save(void);
void nd_hw_irq_restore(nd_size_t level);

void nd_hw_hrtimer_init(void);
void nd_hw_hrtimer_set_expire(nd_uint64_t expire);
void nd_hw_hrtimer_trigger(void);
void nd_hw_hrtimer_trigger_clear(void);
nd_uint64_t nd_hw_hrtimer_get_current(void);

void nd_hw_tick_init(void);
nd_uint64_t nd_hw_tick_get_current(void);

void nd_hw_do_switch_first(void);
void nd_hw_do_switch(void);

void *nd_hw_stack_init(void *entk_fun, void *parameter, nd_uint8_t *stack_addr, void *exit_fun);

#endif /* __ND_HW_H__ */
