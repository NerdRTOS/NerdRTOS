#include "board.h"
#include "autoconf.h"
#include "devicetree_generated.h"
#include <stdio.h>

void board_init(void)
{
    printf("[board] nucleo_f429zi init, ext_osc=%d, flash=0x%X bytes, console=%s@0x%X irq=%d baud=%d\n",
#ifdef CONFIG_NUCLEO_USE_EXTERNAL_OSC
           1,
#else
           0,
#endif
           DT_FLASH_SIZE, DT_CONSOLE_LABEL, DT_CONSOLE_BASE,
           DT_CONSOLE_IRQ, DT_CONSOLE_BAUD);
}