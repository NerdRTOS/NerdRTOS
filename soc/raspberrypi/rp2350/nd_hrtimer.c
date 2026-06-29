#include "hardware/timer.h"
#include "nerd.h"
#include "nd_internal.h"
#include "nerd_hw_metadata.h"
#include <stdio.h>

#define ND_HRTIMER_CLOCK_HZ NERD_SOC_TIMER_CLOCK_FREQUENCY

#if NERD_SOC_TIMER_BASE != 0x40054000
#error "RP2350 hrtimer expects the DTS timer to be timer0"
#endif

static int nd_hr_alarm = -1;

static void nd_hr_alarm_callback(uint alarm_num) {
    (void) alarm_num;
    nd_enter_interrupt();
    nd_timer_process();
    nd_exit_interrupt();
    nd_try_schedule_irqsave();
}

void nd_hw_hrtimer_init(void)
{
    (void)ND_HRTIMER_CLOCK_HZ;
    nd_hr_alarm = hardware_alarm_claim_unused(true);
    hardware_alarm_set_callback((uint)nd_hr_alarm, nd_hr_alarm_callback);
}

void nd_hw_hrtimer_set_expire(nd_uint64_t expire)
{
    if (hardware_alarm_set_target((uint)nd_hr_alarm, (absolute_time_t)expire)) {
        printf("Failed to set hardware timer target\n");
    }
}

void nd_hw_hrtimer_trigger(void)
{
    hardware_alarm_force_irq((uint)nd_hr_alarm);
}

void nd_hw_hrtimer_trigger_clear(void)
{
    hardware_alarm_cancel((uint)nd_hr_alarm);
}

nd_uint64_t nd_hw_hrtimer_get_current(void)
{
    return time_us_64();
}
