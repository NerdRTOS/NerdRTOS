#include "nerd.h"
#include "nd_internal.h"
#include "csr.h"

typedef struct {
    nd_uint32_t type;
    nd_uint32_t mstatus;
    nd_uint32_t mepc;
    nd_uint32_t regs[30];
    nd_uint32_t meicontext;
    nd_uint32_t reserved[6];
} riscv_irq_frame_t;

extern void riscv_trap_entry(void);

void riscv_trap_init(void)
{
    __asm__ volatile("csrw mtvec, %0" :: "r"(riscv_trap_entry) : "memory");
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

__attribute__((weak)) void riscv_machine_external_irq_handler(void)
{
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

