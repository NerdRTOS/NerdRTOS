#include "nerd.h"
#include "nd_config.h"

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

    stk  = stack_addr + sizeof(nd_uint32_t);
    stk  = (nd_uint8_t *)ND_ALIGN_DOWN((nd_uint32_t)stk, 8);
    stk -= sizeof(struct stack_frame);

    stack_frame = (struct stack_frame *)stk;

    stack_frame->exception_stack_frame.r0  = (unsigned long)parameter; /* r0 : argument */
    stack_frame->exception_stack_frame.r1  = 0;                        /* r1 */
    stack_frame->exception_stack_frame.r2  = 0;                        /* r2 */
    stack_frame->exception_stack_frame.r3  = 0;                        /* r3 */
    stack_frame->exception_stack_frame.r12 = 0;                        /* r12 */
    stack_frame->exception_stack_frame.lr  = (unsigned long)exit_fun;  /* lr */
    stack_frame->exception_stack_frame.pc  = (unsigned long)entk_fun;  /* entry point, pc */
    stack_frame->exception_stack_frame.psr = 0x01000000L;              /* PSR */
    stack_frame->exc_return = 0xFFFFFFFD;                              /* exc_return */

    return (void *)stack_frame;
}
