#include "hardware/address_mapped.h"
#include "hardware/irq.h"
#include "hardware/timer.h"
#include "pico/time.h"
#include "nerd.h"
#include "nd_internal.h"

#define ND_HRTIMER_ALARM_NUM 0U

void nd_riscv_patch_external_irq_vector_once(void);

void nd_rp2350_timer0_irq_handler(void)
{
    timer_hw_t *timer = PICO_DEFAULT_TIMER_INSTANCE();
    nd_uint32_t mask = 1u << ND_HRTIMER_ALARM_NUM;

    timer->intr = mask;
    hw_clear_bits(&timer->intf, mask);

    nd_enter_interrupt();
    nd_timer_process();
    nd_exit_interrupt();
}

void nd_hw_hrtimer_init(void)
{
    nd_riscv_patch_external_irq_vector_once();

    timer_hw_t *timer = PICO_DEFAULT_TIMER_INSTANCE();
    nd_uint32_t mask = 1u << ND_HRTIMER_ALARM_NUM;
    uint irq_num = TIMER0_IRQ_0 + ND_HRTIMER_ALARM_NUM;

    irq_set_enabled(irq_num, false);

    irq_handler_t old_handler = irq_get_vtable_handler(irq_num);
    if (old_handler) {
        irq_remove_handler(irq_num, old_handler);
    }

    irq_set_exclusive_handler(irq_num, nd_rp2350_timer0_irq_handler);
    irq_set_priority(irq_num, 0);

    timer->armed = mask;
    timer->intr = mask;
    hw_clear_bits(&timer->intf, mask);
    hw_set_bits(&timer->inte, mask);

    irq_set_enabled(irq_num, true);
}

void nd_hw_hrtimer_set_expire(nd_uint64_t expire)
{
    if (expire <= nd_hw_hrtimer_get_current()) {
        nd_hw_hrtimer_trigger();
        return;
    }

    timer_hw_t *timer = PICO_DEFAULT_TIMER_INSTANCE();
    nd_uint32_t mask = 1u << ND_HRTIMER_ALARM_NUM;

    timer->intr = mask;
    hw_clear_bits(&timer->intf, mask);
    timer->alarm[ND_HRTIMER_ALARM_NUM] = (nd_uint32_t)expire;

    if (nd_hw_hrtimer_get_current() >= expire) {
        nd_hw_hrtimer_trigger();
    }
}

void nd_hw_hrtimer_trigger(void)
{
    timer_hw_t *timer = PICO_DEFAULT_TIMER_INSTANCE();
    nd_uint32_t mask = 1u << ND_HRTIMER_ALARM_NUM;

    hw_set_bits(&timer->intf, mask);
}

void nd_hw_hrtimer_trigger_clear(void)
{
    timer_hw_t *timer = PICO_DEFAULT_TIMER_INSTANCE();
    nd_uint32_t mask = 1u << ND_HRTIMER_ALARM_NUM;

    timer->armed = mask;
    timer->intr = mask;
    hw_clear_bits(&timer->intf, mask);
}

nd_uint64_t nd_hw_hrtimer_get_current(void)
{
    return time_us_64();
}

