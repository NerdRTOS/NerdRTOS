#include "app.h"
#include "gic.h"

extern char __heap_start[];
extern char __heap_end[];

#define UART0_BASE      0x10009000
#define UART0_CR        (*(volatile nd_uint32_t *)(UART0_BASE + 0x30))

#define UART_CR_UARTEN  (1U << 0)
#define UART_CR_TXE     (1U << 8)
#define UART_CR_RXE     (1U << 9)

static void uart_init(void)
{
    UART0_CR = UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE;
}

static void bsp_init(void)
{
    uart_init();
    gic_init();

#if ND_CFG_TICKLESS
    nd_hw_hrtimer_init();
#else
    nd_hw_tick_init();
#endif

    nd_system_heap_init(__heap_start, (nd_uint32_t)(__heap_end - __heap_start));
}

int main(void)
{
    nd_kernel_def();
    nd_kernel_lock();

    bsp_init();

    nd_scheduler_init();

    nd_app_init();

    nd_scheduler_start();

    return 0;
}
