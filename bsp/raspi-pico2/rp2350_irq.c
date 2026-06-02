#include "hardware/irq.h"
#include "hardware/timer.h"
#include "nerd.h"

void nd_rp2350_timer0_irq_handler(void);
void nd_rp2350_uart0_irq_handler(void);

void nd_riscv_platform_irq_handle(nd_uint32_t irq_word)
{
    nd_uint32_t irq_num = irq_word >> 2;

    switch (irq_num) {
    case TIMER0_IRQ_0:
        nd_rp2350_timer0_irq_handler();
        break;

    case UART0_IRQ:
        nd_rp2350_uart0_irq_handler();
        break;

    default: {
        irq_handler_t handler = irq_get_vtable_handler(irq_num);
        if (handler) {
            handler();
        }
        break;
    }
    }
}

