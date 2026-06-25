#include "nerd.h"
#include "nd_internal.h"
#include "csr.h"
#include "hardware/irq.h"

typedef struct {
    nd_uint32_t type;
    nd_uint32_t mstatus;
    nd_uint32_t mepc;
    nd_uint32_t regs[30];
    nd_uint32_t meicontext;
    nd_uint32_t reserved[6];
} riscv_irq_frame_t;

extern void riscv_trap_entry(void);

static nd_bool_t riscv_trap_initialized = ND_FALSE;

void riscv_trap_init(void)
{
    if (riscv_trap_initialized) {
        return;
    }

    (void)irq_set_riscv_vector_handler(RISCV_VEC_MACHINE_EXTERNAL_IRQ,
                                       riscv_trap_entry);
    __asm__ volatile("csrs mie, %0" :: "r"(RISCV_MIE_MEIE) : "memory");
    riscv_trap_initialized = ND_TRUE;
}

static nd_uint32_t riscv_read_mcause(void)
{
    nd_uint32_t value;

    __asm__ volatile("csrr %0, mcause" : "=r"(value));

    return value;
}

__attribute__((weak)) void riscv_machine_soft_irq_handler(void)
{
}

__attribute__((weak)) void riscv_machine_timer_irq_handler(void)
{
}

static nd_uint32_t riscv_external_irq_next(void)
{
    nd_uint32_t value;

    __asm__ volatile("csrrsi %0, "
                     RISCV_STRINGIFY(RISCV_CSR_MEINEXT) ", "
                     RISCV_STRINGIFY(RISCV_MEINEXT_UPDATE)
                     : "=r"(value) :: "memory");

    return value;
}

static void riscv_machine_external_irq_dispatch(void)
{
    nd_uint32_t next = riscv_external_irq_next();

    while ((next & RISCV_MEINEXT_NONE) == 0U) {
        nd_uint32_t irq = next >> RISCV_MEINEXT_IRQ_SHIFT;

        if (irq < NUM_IRQS) {
            irq_handler_t handler = irq_get_vtable_handler((uint)irq);

            if (handler != ND_NULL) {
                handler();
            }
        }

        next = riscv_external_irq_next();
    }
}

__attribute__((weak)) void riscv_machine_external_irq_handler(void)
{
    riscv_machine_external_irq_dispatch();
}

static void riscv_handle_interrupt(nd_uint32_t cause)
{
    nd_enter_interrupt();

    switch (cause & RISCV_MCAUSE_CODE_MASK) {
    case RISCV_IRQ_M_SOFT:
        riscv_machine_soft_irq_handler();
        break;
    case RISCV_IRQ_M_TIMER:
        riscv_machine_timer_irq_handler();
        break;
    case RISCV_IRQ_M_EXT:
        riscv_machine_external_irq_handler();
        break;
    default:
        break;
    }

    nd_exit_interrupt();
}

static void riscv_handle_exception(riscv_irq_frame_t *irq_frame,
                                   nd_uint32_t cause)
{
    switch (cause & RISCV_MCAUSE_CODE_MASK) {
    case RISCV_MCAUSE_ILLEGAL_INSTRUCTION:
    case RISCV_MCAUSE_ECALL_M_MODE:
        irq_frame->mepc += 4;
        break;
    default:
        break;
    }
}

void riscv_trap_handler(void *frame)
{
    nd_uint32_t cause;

    cause = riscv_read_mcause();

    if (cause & RISCV_MCAUSE_INTERRUPT) {
        riscv_handle_interrupt(cause);
        return;
    }

    riscv_handle_exception((riscv_irq_frame_t *)frame, cause);
}

