#include "nerd.h"
#include "nd_internal.h"

#define SYSTICK_BASE     (0xE000E010UL)
#define SYSTICK_CSR      (*(volatile unsigned long *)(SYSTICK_BASE + 0x00)) /* Control & Status */
#define SYSTICK_RVR      (*(volatile unsigned long *)(SYSTICK_BASE + 0x04)) /* Reload Value */
#define SYSTICK_CVR      (*(volatile unsigned long *)(SYSTICK_BASE + 0x08)) /* Current Value */

#define SCB_SHPR3        (*(volatile unsigned long *)(0xE000ED20UL))

static volatile nd_uint64_t nd_tick_count = 0;

/**
 * @brief Initialize SysTick
 * using the internal clock, set the lowest priority, and set the reload value
 */
void nd_hw_tick_init(void)
{
    SYSTICK_CSR = 0;
    SYSTICK_RVR = (ND_CPU_CLOCK_HZ / ND_TICKS_PER_SEC) - 1UL;
    SYSTICK_CVR = 0;
    SCB_SHPR3 |= (0xFFUL << 24);
    SYSTICK_CSR = 0x07UL;
}

/**
 * @brief Update system operation time (cycle)
 */
static void nd_hw_tick_update(void)
{
    nd_tick_count++;
}

/**
 * @brief Obtain the current system tick rate
 * @return nd_uint64_t : current system tick number
 */
nd_uint64_t nd_hw_tick_get_current(void)
{
    return nd_tick_count;
}

/**
 * @brief SysTick counter overflow callback, update the beat
 */
void isr_systick(void)
{
    nd_enter_interrupt();
    nd_hw_tick_update();
    nd_timer_process();
    nd_exit_interrupt();
    nd_try_schedule_irqsave();
}
