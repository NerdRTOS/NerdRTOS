/* arch/riscv/rv32/trap.c */
#include <stdio.h>
#include "nerd.h"

#define READ_CSR(reg) ({ unsigned long val; \
    __asm__ volatile("csrr %0, " #reg : "=r"(val)); val; })


static const char *exception_names[] = {
    "Instruction address misaligned", /* 0 */
    "Instruction access fault",       /* 1 */
    "Illegal instruction",            /* 2 */
    "Breakpoint",                     /* 3 */
    "Load address misaligned",        /* 4 */
    "Load access fault",              /* 5 */
    "Store/AMO address misaligned",   /* 6 */
    "Store/AMO access fault",         /* 7 */
    "Ecall from U-mode",              /* 8 */
    "Ecall from S-mode",              /* 9 */
    "(reserved)",                     /* 10 */
    "Ecall from M-mode",              /* 11 */
    "Instruction page fault",         /* 12 */
    "Load page fault",                /* 13 */
    "(reserved)",                     /* 14 */
    "Store page fault",               /* 15 */
};

void trap_handler(void)
{
    unsigned long mcause = READ_CSR(mcause);
    unsigned long mepc = READ_CSR(mepc);
    unsigned long mtval = READ_CSR(mtval);
    unsigned long mstatus = READ_CSR(mstatus);

    // /* bit31=1 中断处理 */
    // if (mcause & 0x80000000) // 未完成，还需补充中断分发处理
    // {
    //     unsigned long code = mcause & 0x7fffffff;
    //     /* 转发给 pico-sdk 的中断处理 */
    //     /* 外部中断直接调用 RVIC 的处理函数 */
    //     extern void isr_irq0(void);
    //     /* 根据 code 分发 */
    //     printf("IRQ code=%lu mepc=0x%08lx\r\n", code, mepc);
    //     /* 暂时不挂死，直接返回 */
    //     return;
    // }

    /* 其他异常处理 */
    unsigned long code = mcause & 0xfUL;
    const char *name = (code < 16) ? exception_names[code] : "Unknown";

    printf("\r\n=== EXCEPTION ===\r\n");
    printf("  mcause  = 0x%08lx  [%s]\r\n", mcause, name);
    printf("  mepc    = 0x%08lx\r\n", mepc);
    printf("  mtval   = 0x%08lx\r\n", mtval);
    printf("  mstatus = 0x%08lx  MPP=%lu\r\n", mstatus, (mstatus >> 11) & 0x3UL);
    printf("System halted.\r\n");
    while (1);
}
