#include "nd_def.h"
#include "nd_internal.h"

#define FRAME_TYPE_COOP     1U
#define FRAME_TYPE_PREEMPT  2U

struct preempt_frame {
    nd_uint32_t type;
    nd_uint32_t r0, r1, r2, r3;
    nd_uint32_t r4, r5, r6, r7, r8, r9, r10, r11;
    nd_uint32_t r12;
    nd_uint32_t lr;
    nd_uint32_t pc;
    nd_uint32_t cpsr;
};

void *nd_hw_stack_init(void       *entk_fun,
                       void       *parameter,
                       nd_uint8_t *stack_addr,
                       void       *exit_fun)
{
    struct preempt_frame *preempt_frame;
    nd_uint8_t           *stk;

    stk  = stack_addr + sizeof(nd_uint32_t);
    stk  = (nd_uint8_t *)ND_ALIGN_DOWN((nd_uint32_t)stk, 8);
    stk -= sizeof(struct preempt_frame);

    preempt_frame = (struct preempt_frame *)stk;

    for (nd_size_t i = 0; i < sizeof(struct preempt_frame) / sizeof(nd_uint32_t); i++)
        ((nd_uint32_t *)preempt_frame)[i] = 0;

    preempt_frame->type = FRAME_TYPE_PREEMPT;
    preempt_frame->cpsr = 0x1F;                             /* SYS mode, ARM state, IRQ/FIQ enabled */
    preempt_frame->pc   = (unsigned long)nd_thread_entry;
    preempt_frame->r0   = (nd_uint32_t)entk_fun;
    preempt_frame->r1   = (nd_uint32_t)parameter;
    preempt_frame->lr   = (nd_uint32_t)exit_fun;

    return (void *)preempt_frame;
}
