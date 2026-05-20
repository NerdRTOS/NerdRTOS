#include "nerd.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "pico/time.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
void task_entry_trampoline(void);

volatile nd_uint64_t nd_tick_count = 0;
static int nd_tick_alarm = -1;

nd_uint64_t nd_hw_tick_get_current(void)
{
    return nd_tick_count;
}

static void nd_tick_alarm_cb(uint alarm_num)
{
    (void)alarm_num;

    hardware_alarm_set_target((uint)nd_tick_alarm, make_timeout_time_us(1000));

    nd_enter_interrupt();

    nd_tick_count++;

    nd_timer_process();

    nd_exit_interrupt();
    nd_try_schedule_irqsave();
}

__attribute__((aligned(4), noinline, naked))
void task_entry_trampoline(void)
{
    __asm__ volatile(
        "csrsi  mstatus, 8   \n"
        "mv     a0, s1       \n"
        "jr     s0           \n");
}

void nd_hw_tick_init(void)
{
    int alarm = hardware_alarm_claim_unused(false);
    if (alarm < 0)
    {
        alarm = 0;
        hardware_alarm_claim(0);
    }
    nd_tick_alarm = alarm;

    uint irq_num = TIMER0_IRQ_0 + (uint)nd_tick_alarm;

    irq_set_enabled(irq_num, false);
    irq_handler_t old = irq_get_vtable_handler(irq_num);
    if (old)
    {
        irq_remove_handler(irq_num, old);
    }

    hardware_alarm_set_callback((uint)nd_tick_alarm, nd_tick_alarm_cb);
    hardware_alarm_set_target((uint)nd_tick_alarm,
                              make_timeout_time_us(1000));
}

void *nd_hw_stack_init(void *entry, void *parameter, nd_uint8_t *stack_addr, nd_uint32_t stack_size)
{
    nd_uint32_t *sp;

    nd_uint8_t *stack_top = stack_addr + stack_size;

    sp = (nd_uint32_t *)((nd_uint32_t)stack_top & ~0xfUL);

    // 预留13个寄存器的空间,对应ra, s0-s11
    *(--sp) = 0;                      // s11
    *(--sp) = 0;                      // s10
    *(--sp) = 0;                      // s9
    *(--sp) = 0;                      // s8
    *(--sp) = 0;                      // s7
    *(--sp) = 0;                      // s6
    *(--sp) = 0;                      // s5
    *(--sp) = 0;                      // s4
    *(--sp) = 0;                      // s3
    *(--sp) = 0;                      // s2
    *(--sp) = (nd_uint32_t)parameter; // s1
    *(--sp) = (nd_uint32_t)entry;     // s0
    *(--sp) = (nd_uint32_t)task_entry_trampoline;

    return (void *)sp;
}
