#include "nerd.h"

#if defined(NERD_RISCV_PORT)

#include "csr.h"

#define RP2350_SIO_BASE         0xD0000000UL
#define RP2350_SIO_MTIME_CTRL   (*(volatile nd_uint32_t *)(RP2350_SIO_BASE + 0x1A4U))
#define RP2350_SIO_MTIME        (*(volatile nd_uint32_t *)(RP2350_SIO_BASE + 0x1B0U))
#define RP2350_SIO_MTIMEH       (*(volatile nd_uint32_t *)(RP2350_SIO_BASE + 0x1B4U))
#define RP2350_SIO_MTIMECMP     (*(volatile nd_uint32_t *)(RP2350_SIO_BASE + 0x1B8U))
#define RP2350_SIO_MTIMECMPH    (*(volatile nd_uint32_t *)(RP2350_SIO_BASE + 0x1BCU))

#define RP2350_MTIME_CTRL_EN    0x1U

static nd_uint64_t riscv_mtimer_read(void)
{
    nd_uint32_t hi;
    nd_uint32_t lo;
    nd_uint32_t hi2;

    do {
        hi = RP2350_SIO_MTIMEH;
        lo = RP2350_SIO_MTIME;
        hi2 = RP2350_SIO_MTIMEH;
    } while (hi != hi2);

    return ((nd_uint64_t)hi << 32) | lo;
}

static void riscv_mtimer_set_compare(nd_uint64_t value)
{
    RP2350_SIO_MTIMECMP = 0xFFFFFFFFU;
    RP2350_SIO_MTIMECMPH = (nd_uint32_t)(value >> 32);
    RP2350_SIO_MTIMECMP = (nd_uint32_t)value;
}

void nd_hw_hrtimer_init(void)
{
    nd_uint32_t mtie = RISCV_MIE_MTIE;

    RP2350_SIO_MTIME_CTRL |= RP2350_MTIME_CTRL_EN;
    riscv_mtimer_set_compare(~0ULL);
    __asm__ volatile("csrc mie, %0" :: "r"(mtie) : "memory");
}

void nd_hw_hrtimer_set_expire(nd_uint64_t expire)
{
    nd_uint32_t mtie = RISCV_MIE_MTIE;

    riscv_mtimer_set_compare(expire);
    __asm__ volatile("csrs mie, %0" :: "r"(mtie) : "memory");
    __asm__ volatile("csrsi mstatus, %0" :: "i"(RISCV_MSTATUS_MIE) : "memory");
}

void nd_hw_hrtimer_trigger(void)
{
    nd_hw_hrtimer_set_expire(riscv_mtimer_read());
}

void nd_hw_hrtimer_trigger_clear(void)
{
    nd_uint32_t mtie = RISCV_MIE_MTIE;

    riscv_mtimer_set_compare(~0ULL);
    __asm__ volatile("csrc mie, %0" :: "r"(mtie) : "memory");
}

nd_uint64_t nd_hw_hrtimer_get_current(void)
{
    return riscv_mtimer_read();
}

void riscv_machine_timer_irq_handler(void)
{
    nd_uint32_t mtie = RISCV_MIE_MTIE;

    riscv_mtimer_set_compare(~0ULL);
    __asm__ volatile("csrc mie, %0" :: "r"(mtie) : "memory");

    nd_timer_process();
}

#else

#include "hardware/timer.h"
#include "nd_internal.h"
#include <stdio.h>

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

#endif

