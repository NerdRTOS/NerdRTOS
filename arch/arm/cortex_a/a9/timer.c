#include "nerd.h"
#include "ptimer.h"
#include "gic.h"
#include "nd_config.h"

static volatile nd_uint64_t nd_tick_count = 0;

static void isr_ptimer(void *arg)
{
    PTIMER_ISR = PTIMER_ISR_EVENT;
    nd_tick_count++;
    nd_timer_process();
}

void nd_hw_tick_init(void)
{
    PTIMER_CTRL = 0;

    PTIMER_LOAD = (ND_CPU_CLOCK_HZ / ND_TICKS_PER_SEC) - 1;

    PTIMER_COUNTER = 0;

    PTIMER_CTRL = PTIMER_CTRL_ENABLE | PTIMER_CTRL_AUTO_RELOAD | PTIMER_CTRL_IRQ_ENABLE;

    gic_register_handler(IRQ_PRIVATE_TIMER, isr_ptimer, ND_NULL);

    gic_irq_enable(IRQ_PRIVATE_TIMER);
}

nd_uint64_t nd_hw_tick_get_current(void)
{
    return nd_tick_count;
}

static nd_uint64_t gtimer_read(void)
{
    nd_uint32_t hi, lo, hi2;

    do {
        hi  = GTIMER_COUNT_HI;
        lo  = GTIMER_COUNT_LO;
        hi2 = GTIMER_COUNT_HI;
    } while (hi != hi2);

    return ((nd_uint64_t)hi << 32) | lo;
}

void nd_hw_hrtimer_init(void)
{
    GTIMER_CTRL = GTIMER_CTRL_ENABLE;

    PTIMER_CTRL = 0;
    PTIMER_CTRL = PTIMER_CTRL_IRQ_ENABLE;

    gic_register_handler(IRQ_PRIVATE_TIMER, isr_ptimer, ND_NULL);
    gic_irq_enable(IRQ_PRIVATE_TIMER);
}

nd_uint64_t nd_hw_hrtimer_get_current(void)
{
    return gtimer_read() / (ND_CPU_CLOCK_HZ / ND_TICKLESS_FREQ);
}

void nd_hw_hrtimer_set_expire(nd_uint64_t expire)
{
    nd_uint64_t now = nd_hw_hrtimer_get_current();

    if (expire <= now) {
        PTIMER_LOAD = 1;
    } else {
        nd_uint64_t delta = expire - now;
        delta = delta * (ND_CPU_CLOCK_HZ / ND_TICKLESS_FREQ);
        PTIMER_LOAD = (delta > 0xFFFFFFFF) ? 0xFFFFFFFF : (nd_uint32_t)delta;
    }

    PTIMER_COUNTER = PTIMER_LOAD;
    PTIMER_CTRL = PTIMER_CTRL_ENABLE | PTIMER_CTRL_IRQ_ENABLE;
}

void nd_hw_hrtimer_trigger(void)
{
    PTIMER_LOAD = 1;

    PTIMER_COUNTER = 1;

    PTIMER_CTRL = PTIMER_CTRL_ENABLE | PTIMER_CTRL_IRQ_ENABLE;
}

void nd_hw_hrtimer_trigger_clear(void)
{
    PTIMER_CTRL = 0;
}
