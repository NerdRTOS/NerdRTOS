#include "gic.h"
#include "nd_internal.h"

static gic_isr_entry_t isr_table[GIC_MAX_INTERRUPTS];

#define GIC_IRQ_PER_REG             32U
#define GIC_SPI_BASE                32U
#define GIC_ENABLE_REG_COUNT        ((GIC_MAX_INTERRUPTS + GIC_IRQ_PER_REG - 1U) / GIC_IRQ_PER_REG)

void gic_init(void)
{
    nd_uint32_t i;

    GICD_CTLR = 0;
    GICC_CTLR = 0;

    for (i = 0; i < GIC_ENABLE_REG_COUNT; i++) {
        GICD_ICENABLER(i) = 0xFFFFFFFFU;
        GICD_ICPENDR(i)   = 0xFFFFFFFFU;
    }

    for (i = 0; i < GIC_MAX_INTERRUPTS; i++) {
        GICD_IPRIORITYR(i) = GIC_PRIORITY_DEFAULT;
    }

    for (i = GIC_SPI_BASE; i < GIC_MAX_INTERRUPTS; i++) {
        GICD_ITARGETSR(i) = GIC_TARGET_CPU0;
    }

    for (i = 0; i < GIC_MAX_INTERRUPTS; i++) {
        isr_table[i].handler = ND_NULL;
        isr_table[i].arg     = ND_NULL;
    }

    GICC_PMR  = GIC_PRIORITY_MASK_ALL;
    GICC_CTLR = GICC_CTLR_ENABLE;
    GICD_CTLR = GICD_CTLR_ENABLE;
}

void gic_irq_enable(nd_uint32_t irq_id)
{
    GICD_IPRIORITYR(irq_id)     = GIC_PRIORITY_DEFAULT;

    if (irq_id >= 32) {
        GICD_ITARGETSR(irq_id) = GIC_TARGET_CPU0;
    }

    GICD_ISENABLER(irq_id / 32) = (1U << (irq_id % 32));
}

void gic_register_handler(nd_uint32_t irq_id, gic_isr_t handler, void *arg)
{
    if (irq_id >= GIC_MAX_INTERRUPTS) {
        return;
    }

    isr_table[irq_id].handler = handler;
    isr_table[irq_id].arg     = arg;
}

void gic_dispatch(void)
{
    nd_enter_interrupt();

    nd_uint32_t iar     = GICC_IAR;
    nd_uint32_t irq_id  = iar & GICC_IAR_ID_MASK;

    if (irq_id < GIC_MAX_INTERRUPTS && isr_table[irq_id].handler) {
        isr_table[irq_id].handler(isr_table[irq_id].arg);
    }

    GICC_EOIR = iar;

    nd_exit_interrupt();
}
