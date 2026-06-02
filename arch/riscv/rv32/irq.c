#include "nerd.h"
#include "irq.h"
#include "csr.h"

#define RISCV_IRQ_MAX               64U

typedef struct {
    riscv_irq_handler_t handler;
    void               *arg;
} riscv_irq_entry_t;

static riscv_irq_entry_t riscv_irq_table[RISCV_IRQ_MAX];

static nd_uint32_t riscv_read_meinext(void)
{
    nd_uint32_t value;

    __asm__ volatile("csrrsi %0, "
                     RISCV_STRINGIFY(RISCV_CSR_MEINEXT) ", "
                     RISCV_STRINGIFY(RISCV_MEINEXT_UPDATE)
                     : "=r"(value) :: "memory");

    return value;
}

static void riscv_external_irq_enable_cpu(void)
{
    nd_uint32_t meie = RISCV_MIE_MEIE;

    __asm__ volatile("csrs mie, %0" :: "r"(meie) : "memory");
    __asm__ volatile("csrsi mstatus, %0" :: "i"(RISCV_MSTATUS_MIE) : "memory");
}

void riscv_irq_init(void)
{
    for (nd_uint32_t i = 0; i < RISCV_IRQ_MAX; i++) {
        riscv_irq_table[i].handler = ND_NULL;
        riscv_irq_table[i].arg = ND_NULL;
    }
}

void riscv_irq_register(nd_uint32_t irq,
                        void (*handler)(void *arg),
                        void *arg)
{
    if (irq >= RISCV_IRQ_MAX) {
        return;
    }

    riscv_irq_table[irq].handler = handler;
    riscv_irq_table[irq].arg = arg;
}

void riscv_irq_enable(nd_uint32_t irq)
{
    (void)irq;

    riscv_external_irq_enable_cpu();
}

void riscv_irq_disable(nd_uint32_t irq)
{
    if (irq >= RISCV_IRQ_MAX) {
        return;
    }

    riscv_irq_table[irq].handler = ND_NULL;
    riscv_irq_table[irq].arg = ND_NULL;
}

void riscv_machine_external_irq_handler(void)
{
    nd_uint32_t next;

    next = riscv_read_meinext();
    while ((next & RISCV_MEINEXT_NONE) == 0U) {
        nd_uint32_t irq = next >> RISCV_MEINEXT_IRQ_SHIFT;

        if (irq < RISCV_IRQ_MAX && riscv_irq_table[irq].handler) {
            riscv_irq_table[irq].handler(riscv_irq_table[irq].arg);
        }

        next = riscv_read_meinext();
    }
}

