#include "nerd.h"
#include "nd_internal.h"
#include "csr.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "pico/time.h"

void task_entry_trampoline(void);

#define FRAME_TYPE_VOLUNTARY    1U

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

    nd_tick_count++;

    nd_enter_interrupt();
    nd_timer_process();
    nd_exit_interrupt();
}

__attribute__((aligned(4), noinline, naked))
void task_entry_trampoline(void)
{
    __asm__ volatile(
        "csrsi  mstatus, " RISCV_STRINGIFY(RISCV_MSTATUS_MIE_IMM) "\n"
        "mv     a0, s1       \n"
        "mv     a1, s2       \n"
        "jr     s0           \n");
}

void nd_hw_tick_init(void)
{
    int alarm = hardware_alarm_claim_unused(false);
    if (alarm < 0) {
        return;
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

void *nd_hw_stack_init(void *entry, void *parameter, nd_uint8_t *stack_addr)
{
    nd_uint32_t *sp;

    sp = (nd_uint32_t *)ND_ALIGN_DOWN((nd_ubase_t)stack_addr, 16);

    /*
     * Initial threads enter through the voluntary restore path:
     * type, mstatus, ra, s0-s11, then one padding word to keep the
     * software frame size equal to nd_hw_do_switch().
     */
    *(--sp) = 0;
    *(--sp) = 0;                      // s11
    *(--sp) = 0;                      // s10
    *(--sp) = 0;                      // s9
    *(--sp) = 0;                      // s8
    *(--sp) = 0;                      // s7
    *(--sp) = 0;                      // s6
    *(--sp) = 0;                      // s5
    *(--sp) = 0;                      // s4
    *(--sp) = 0;                      // s3
    *(--sp) = (nd_uint32_t)parameter; // s2
    *(--sp) = (nd_uint32_t)entry;     // s1
    *(--sp) = (nd_uint32_t)nd_thread_entry; // s0
    *(--sp) = (nd_uint32_t)task_entry_trampoline;
    *(--sp) = RISCV_MSTATUS_MIE;
    *(--sp) = FRAME_TYPE_VOLUNTARY;

    return (void *)sp;
}

