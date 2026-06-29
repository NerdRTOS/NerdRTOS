#include <stdio.h>
#include "autoconf.h"
#include "devicetree_generated.h"
#include "soc.h"
#include "board.h"

int main(void)
{
    soc_init();
    board_init();

    printf("MyRTOS skeleton booted on board=%s, model=%s, sram=0x%X+0x%X, num_priorities=%d, app_log_level=%d\n",
           CONFIG_BOARD, DT_MODEL, DT_SRAM_BASE, DT_SRAM_SIZE,
           CONFIG_NUM_PRIORITIES, CONFIG_APP_LOG_LEVEL);

    return 0;
}