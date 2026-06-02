#ifndef __RISCV_IRQ_H__
#define __RISCV_IRQ_H__

#include "nd_def.h"

typedef void (*riscv_irq_handler_t)(void *arg);

void riscv_irq_init(void);
void riscv_irq_register(nd_uint32_t irq, riscv_irq_handler_t handler, void *arg);
void riscv_irq_enable(nd_uint32_t irq);
void riscv_irq_disable(nd_uint32_t irq);

#endif

