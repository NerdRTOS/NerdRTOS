#include "stm32h7xx_hal.h"
#include "nerd.h"

#include "main.h"
#include "app.h"
#include "usart.h"

void _init(void)
{
}

extern char __heap_start[];
extern char __heap_end[];

static void bsp_init(void);

int main(void)
{
    nd_hw_irq_disable();

    HAL_Init();
    bsp_init();

    usart_puts("Hello Nerd RTOS!\r\n");

    nd_scheduler_init();
    nd_app_init();
    nd_scheduler_start();

    return 0;
}

static void bsp_init(void)
{
    rcc_init();

    usart_init(115200);

#if ND_CFG_TICKLESS
    nd_hw_hrtimer_init();
#else
    nd_hw_tick_init();
#endif

    nd_system_heap_init(__heap_start,
                        (nd_uint32_t)(__heap_end - __heap_start));
}
