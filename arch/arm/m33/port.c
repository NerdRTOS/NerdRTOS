#include "nerd.h"

#define SYSTICK_BASE     (0xE000E010UL)
#define SYSTICK_CSR      (*(volatile unsigned long *)(SYSTICK_BASE + 0x00)) /* Control & Status */
#define SYSTICK_RVR      (*(volatile unsigned long *)(SYSTICK_BASE + 0x04)) /* Reload Value */
#define SYSTICK_CVR      (*(volatile unsigned long *)(SYSTICK_BASE + 0x08)) /* Current Value */

#define SCB_SHPR3        (*(volatile unsigned long *)(0xE000ED20UL))

static volatile nd_uint64_t nd_tick_count = 0;

void nd_hw_tick_init(void)
{
    SYSTICK_CSR = 0;

    SYSTICK_RVR = (ND_CPU_CLOCK_HZ / ND_TICKS_PER_SEC) - 1UL;

    SYSTICK_CVR = 0;

    SCB_SHPR3 |= (0xFFUL << 24);

    SYSTICK_CSR = 0x07UL;
}

static void nd_hw_tick_update(void)
{
    nd_tick_count++;
}

nd_uint64_t nd_hw_tick_get_current(void)
{
    return nd_tick_count;
}

void isr_systick(void)
{
    nd_enter_interrupt();
    nd_hw_tick_update();
    nd_timer_process();
    nd_exit_interrupt();
    nd_try_schedule_irqsave();
}

struct exception_stack_frame
{
    nd_uint32_t r0;
    nd_uint32_t r1;
    nd_uint32_t r2;
    nd_uint32_t r3;
    nd_uint32_t r12;
    nd_uint32_t lr;
    nd_uint32_t pc;
    nd_uint32_t psr;
};

struct stack_frame
{
    /* r4 ~ r7 low register */
    nd_uint32_t r4;
    nd_uint32_t r5;
    nd_uint32_t r6;
    nd_uint32_t r7;

    /* r8 ~ r11 high register */
    nd_uint32_t r8;
    nd_uint32_t r9;
    nd_uint32_t r10;
    nd_uint32_t r11;

    nd_uint32_t exc_return;

    struct exception_stack_frame exception_stack_frame;
};

/**
 * This function will initialize thread stack
 *
 * @param entk_fun the entry of thread
 * @param parameter the parameter of entry
 * @param stack_addr the beginning stack address
 * @param exit_fun the function will be called when thread exit
 *
 * @return stack address
 */
void *nd_hw_stack_init(void       *entk_fun,
                       void       *parameter,
                       nd_uint8_t *stack_addr,
                       void       *exit_fun)
{
    struct stack_frame *stack_frame;
    nd_uint8_t         *stk;
    unsigned long       i;

    stk  = stack_addr + sizeof(nd_uint32_t);
    stk  = (nd_uint8_t *)ND_ALIGN_DOWN((nd_uint32_t)stk, 8);
    stk -= sizeof(struct stack_frame);

    stack_frame = (struct stack_frame *)stk;

    /* init all register */
    for (i = 0; i < sizeof(struct stack_frame) / sizeof(nd_uint32_t); i ++)
    {
        ((nd_uint32_t *)stack_frame)[i] = 0xdeadbeef;
    }

    stack_frame->exception_stack_frame.r0  = (unsigned long)parameter; /* r0 : argument */
    stack_frame->exception_stack_frame.r1  = 0;                        /* r1 */
    stack_frame->exception_stack_frame.r2  = 0;                        /* r2 */
    stack_frame->exception_stack_frame.r3  = 0;                        /* r3 */
    stack_frame->exception_stack_frame.r12 = 0;                        /* r12 */
    stack_frame->exception_stack_frame.lr  = (unsigned long)exit_fun;  /* lr */
    stack_frame->exception_stack_frame.pc  = (unsigned long)entk_fun;  /* entry point, pc */
    stack_frame->exception_stack_frame.psr = 0x01000000L;              /* PSR */
    stack_frame->exc_return = 0xFFFFFFFD;                              /* exc_return */
    /* Return pointer to the r4..r11 area (stack_frame).
     * The context switch save/restore code pushes/pops r4..r11 and
     * stores the pointer to that area in `thread->sp`. After a
     * restore (`LDMIA r2!, {r4-r11}`) the pointer will be advanced to
     * point to the exception stack frame which the CPU expects PSP to
     * refer to on EXC_RETURN.
     */
    return (void *)stack_frame;
}
