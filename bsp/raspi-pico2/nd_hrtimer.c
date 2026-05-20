#include "hardware/timer.h"
#include "nerd.h"
#include <stdio.h>
#include "hardware/uart.h"
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
    int alarm = hardware_alarm_claim_unused(false);
    if (alarm < 0) {
        uart_puts(uart0, "ERROR: no free alarm for hrtimer!\r\n");
        while(1);
    }
    nd_hr_alarm = alarm;
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

