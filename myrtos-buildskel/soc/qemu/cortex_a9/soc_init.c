#include "soc.h"
#include "arch.h"
#include "autoconf.h"
#include "devicetree_generated.h"
#include <stdio.h>

void soc_init(void)
{
    arch_early_init();
    printf("[soc] qemu cortex_a9 init, cores=%d, timer_clk=%d Hz\n",
           CONFIG_QEMU_A9_NUM_CORES,
           DT_TIMER_CLOCK_FREQUENCY);
}