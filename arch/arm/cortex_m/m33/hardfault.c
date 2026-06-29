#include "nd_def.h"
#include "nd_shell.h"

/* Assembly wrapper chooses correct stack pointer and passes to C handler */
void HardFault_Handler(void);

void hard_fault_handler_c(nd_uint32_t *stack)
{
    nd_uint32_t r0 = stack[0];
    nd_uint32_t r1 = stack[1];
    nd_uint32_t r2 = stack[2];
    nd_uint32_t r3 = stack[3];
    nd_uint32_t r12 = stack[4];
    nd_uint32_t lr = stack[5];
    nd_uint32_t pc = stack[6];
    nd_uint32_t psr = stack[7];

    nd_uint32_t cfsr = (*(volatile nd_uint32_t *)0xE000ED28);
    nd_uint32_t hfsr = (*(volatile nd_uint32_t *)0xE000ED2C);
    nd_uint32_t mmfar = (*(volatile nd_uint32_t *)0xE000ED34);
    nd_uint32_t bfar = (*(volatile nd_uint32_t *)0xE000ED38);

    shell_printf("\n*** HardFault ***\n");
    shell_printf("R0  = 0x%08lx\n", r0);
    shell_printf("R1  = 0x%08lx\n", r1);
    shell_printf("R2  = 0x%08lx\n", r2);
    shell_printf("R3  = 0x%08lx\n", r3);
    shell_printf("R12 = 0x%08lx\n", r12);
    shell_printf("LR  = 0x%08lx\n", lr);
    shell_printf("PC  = 0x%08lx\n", pc);
    shell_printf("PSR = 0x%08lx\n", psr);
    shell_printf("CFSR= 0x%08lx HFSR=0x%08lx\n", cfsr, hfsr);
    shell_printf("MMFAR=0x%08lx BFAR=0x%08lx\n", mmfar, bfar);

    /* spin here for debugger */
    while (1) {
        __asm volatile ("bkpt #0");
    }
}

__attribute__((naked)) void isr_hardfault(void)
{
    __asm volatile (
        "tst lr, #4\n"              /* test bit 2 of LR to see which stack pointer */
        "ite eq\n"
        "mrseq r0, msp\n"
        "mrsne r0, psp\n"
        "b hard_fault_handler_c\n"
    );
}
