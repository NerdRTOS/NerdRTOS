#include "board.h"
#include "autoconf.h"
#include "devicetree_generated.h"
#include <stdio.h>

void board_init(void)
{
    printf("[board] qemu_a9 init, ram=0x%X bytes, console=%s@0x%X irq=%d baud=%d\n",
           DT_SRAM_SIZE, DT_CONSOLE_LABEL, DT_CONSOLE_BASE,
           DT_CONSOLE_IRQ, DT_CONSOLE_BAUD);
}