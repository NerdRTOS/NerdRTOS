#include "soc.h"
#include "arch.h"
#include "autoconf.h"
#include "devicetree_generated.h"
#include <stdio.h>

void soc_init(void)
{
    arch_early_init();
    printf("[soc] stm32f4 init, hal=%d, timer_clk=%d Hz\n",
#ifdef CONFIG_STM32F4_USE_HAL_DRIVER
           1,
#else
           0,
#endif
           DT_TIMER_CLOCK_FREQUENCY);
}