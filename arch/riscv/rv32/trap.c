#include "nerd.h"
#include "nd_shell.h"

#define READ_CSR(reg) ({ unsigned long val; \
    __asm__ volatile("csrr %0, " #reg : "=r"(val)); val; })

static const char *exception_names[] = {
    "Instruction address misaligned",
    "Instruction access fault",
    "Illegal instruction",
    "Breakpoint",
    "Load address misaligned",
    "Load access fault",
    "Store/AMO address misaligned",
    "Store/AMO access fault",
    "Ecall from U-mode",
    "Ecall from S-mode",
    "(reserved)",
    "Ecall from M-mode",
    "Instruction page fault",
    "Load page fault",
    "(reserved)",
    "Store page fault",
};

void trap_handler(void)
{
    unsigned long mcause = READ_CSR(mcause);
    unsigned long mepc = READ_CSR(mepc);
    unsigned long mtval = READ_CSR(mtval);
    unsigned long mstatus = READ_CSR(mstatus);

    unsigned long code = mcause & 0xfUL;
    const char *name = (code < 16) ? exception_names[code] : "Unknown";

    shell_printf("\r\n=== EXCEPTION ===\r\n");
    shell_printf("  mcause  = 0x%08lx  [%s]\r\n", mcause, name);
    shell_printf("  mepc    = 0x%08lx\r\n", mepc);
    shell_printf("  mtval   = 0x%08lx\r\n", mtval);
    shell_printf("  mstatus = 0x%08lx  MPP=%lu\r\n", mstatus, (mstatus >> 11) & 0x3UL);
    shell_printf("System halted.\r\n");
    while (1) {
    }
}

void __attribute__((weak)) nd_riscv_platform_irq_dispatch(void)
{
    /*
     * Platform ports can override this hook after they route machine external
     * interrupts to nd_riscv_irq_entry. The RP2350/Pico SDK port will fill this
     * with the SDK-compatible soft-vector dispatch path.
     */
}

