#include "nd_internal.h"

/*
 * Assembly functions defined in context.S.
 * Called by the ISR trampolines below.
 */
extern void isr_hardfault(void);
extern void isr_pendsv(void);
extern void isr_systick(void);

struct exception_stack_frame
{
    nd_uint32_t r0;
    nd_uint32_t r1;
    nd_uint32_t r2;
    nd_uint32_t r3;
    nd_uint32_t r12;
    nd_uint32_t lr;
    nd_uint32_t pc;
    nd_uint32_t xpsr;
};

struct stack_frame
{
    nd_uint32_t r4;
    nd_uint32_t r5;
    nd_uint32_t r6;
    nd_uint32_t r7;
    nd_uint32_t r8;
    nd_uint32_t r9;
    nd_uint32_t r10;
    nd_uint32_t r11;

    nd_uint32_t exc_return;

    struct exception_stack_frame exception_stack_frame;
};

/**
 * This function will initialize thread stack.
 * Called when the thread is created.
 *
 * @param entk_fun the entry of thread
 * @param parameter the parameter of entry
 * @param stack_addr the beginning stack address
 *
 * @return stack address
 */
void *nd_hw_stack_init(void       *entk_fun,
                       void       *parameter,
                       nd_uint8_t *stack_addr)
{
    struct stack_frame  *stack_frame;
    nd_uint8_t          *stk;

    stk = stack_addr + sizeof(nd_uint32_t);
    stk = (nd_uint8_t *)ND_ALIGN_DOWN((nd_uint32_t)stk, 8);
    stk -= sizeof(struct stack_frame);

    stack_frame = (struct stack_frame *)stk;

    stack_frame->exception_stack_frame.r0   = (unsigned long)entk_fun;
    stack_frame->exception_stack_frame.r1   = (unsigned long)parameter;
    stack_frame->exception_stack_frame.r2   = 0;
    stack_frame->exception_stack_frame.r3   = 0;
    stack_frame->exception_stack_frame.r12  = 0;
    stack_frame->exception_stack_frame.lr   = 0;
    stack_frame->exception_stack_frame.pc   = (unsigned long)nd_thread_entry;
    stack_frame->exception_stack_frame.xpsr = 0x01000000L;
    stack_frame->exc_return = 0xFFFFFFFD;

    return (void *)stack_frame;
}

void HardFault_Handler(void)
{
    isr_hardfault();
}

void PendSV_Handler(void)
{
    isr_pendsv();
}

void SysTick_Handler(void)
{
    isr_systick();
}

