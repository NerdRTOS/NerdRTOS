#ifndef __GIC_H__
#define __GIC_H__

#include "nd_def.h"
#include "nd_config.h"

#define GICD_BASE                   0x1E001000
#define GICD_CTLR                   (*(volatile nd_uint32_t *)(GICD_BASE + 0x000))
#define GICD_ISENABLER(n)           (*(volatile nd_uint32_t *)(GICD_BASE + 0x100 + 4 * (n)))
#define GICD_ICENABLER(n)           (*(volatile nd_uint32_t *)(GICD_BASE + 0x180 + 4 * (n)))
#define GICD_IPRIORITYR(n)          (*(volatile nd_uint8_t  *)(GICD_BASE + 0x400 + (n)))
#define GICD_ITARGETSR(n)           (*(volatile nd_uint8_t  *)(GICD_BASE + 0x800 + (n)))
#define GICD_ICPENDR(n)             (*(volatile nd_uint32_t *)(GICD_BASE + 0x280 + 4 * (n)))

#define GIC_TARGET_CPU0             0x01

#define GICD_CTLR_ENABLE            (1U << 0)

#define GICC_BASE                   0x1E000100
#define GICC_CTLR                   (*(volatile nd_uint32_t *)(GICC_BASE + 0x000))
#define GICC_PMR                    (*(volatile nd_uint32_t *)(GICC_BASE + 0x004))
#define GICC_IAR                    (*(volatile nd_uint32_t *)(GICC_BASE + 0x00C))
#define GICC_EOIR                   (*(volatile nd_uint32_t *)(GICC_BASE + 0x010))

#define GICC_CTLR_ENABLE            (1U << 0)
#define GICC_IAR_ID_MASK            0x3FF

#define GIC_PRIORITY_MASK_ALL       0xFF
#define GIC_PRIORITY_DEFAULT        0xA0

#define IRQ_PRIVATE_TIMER           29

typedef void (*gic_isr_t)(void *arg);

typedef struct {
    gic_isr_t handler;
    void      *arg;
} gic_isr_entry_t;

void gic_init(void);
void gic_irq_enable(nd_uint32_t irq_id);
void gic_register_handler(nd_uint32_t irq_id, gic_isr_t handler, void *arg);
void gic_dispatch(void);

#endif /* __GIC_H__ */
