#include "soc.h"
#include "arch.h"
#include "autoconf.h"
#include "devicetree_generated.h"
#include <stdio.h>

void soc_init(void)
{
    arch_early_init();
    printf("[soc] rp2350 init, cluster=%s, dual_core=%d, timer_clk=%d Hz\n",
           CONFIG_SOC_CPU_CLUSTER,
#ifdef CONFIG_RP2350_DUAL_CORE
           1,
#else
           0,
#endif
           DT_TIMER_CLOCK_FREQUENCY);
}