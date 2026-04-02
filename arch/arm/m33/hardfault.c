#include <stdio.h>
#include <stdint.h>

/* Assembly wrapper chooses correct stack pointer and passes to C handler */
void HardFault_Handler(void);

void hard_fault_handler_c(uint32_t *stack)
{
    uint32_t r0 = stack[0];
    uint32_t r1 = stack[1];
    uint32_t r2 = stack[2];
    uint32_t r3 = stack[3];
    uint32_t r12 = stack[4];
    uint32_t lr = stack[5];
    uint32_t pc = stack[6];
    uint32_t psr = stack[7];

    uint32_t cfsr = (*(volatile uint32_t *)0xE000ED28);
    uint32_t hfsr = (*(volatile uint32_t *)0xE000ED2C);
    uint32_t mmfar = (*(volatile uint32_t *)0xE000ED34);
    uint32_t bfar = (*(volatile uint32_t *)0xE000ED38);

    printf("\n*** HardFault ***\n");
    printf("R0  = 0x%08x\n", r0);
    printf("R1  = 0x%08x\n", r1);
    printf("R2  = 0x%08x\n", r2);
    printf("R3  = 0x%08x\n", r3);
    printf("R12 = 0x%08x\n", r12);
    printf("LR  = 0x%08x\n", lr);
    printf("PC  = 0x%08x\n", pc);
    printf("PSR = 0x%08x\n", psr);
    printf("CFSR= 0x%08x HFSR=0x%08x\n", cfsr, hfsr);
    printf("MMFAR=0x%08x BFAR=0x%08x\n", mmfar, bfar);

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
