.cpu cortex-m3 
.syntax unified
.thumb
.text

.equ    SCB_VTOR,           0xE000ED08              /* Vector Table Offset Register */
.equ    NVIC_INT_CTRL,      0xE000ED04              /* interrupt control state register */
.equ    NVIC_SYSPRI2,       0xE000ED20              /* system priority register (2) */
.equ    NVIC_PENDSV_PRI,    0x00FF0000              /* PendSV priority value (lowest) */
.equ    NVIC_PENDSVSET,     0x10000000              /* value to trigger PendSV exception */

.global nd_current_thread
.global nd_next_thread

.global nd_hw_irq_disable
.type nd_hw_irq_disable, %function
nd_hw_irq_disable:
    CPSID   I 
    BX      lr

.global nd_hw_irq_save
.type nd_hw_irq_save, %function
nd_hw_irq_save:
    MRS     r0, PRIMASK
    CPSID   I
    BX      lr

.global nd_hw_irq_restore
.type nd_hw_irq_restore, %function
nd_hw_irq_restore:
    MSR     PRIMASK, r0
    BX      lr

.global nd_hw_do_switch
.type nd_hw_do_switch, %function
nd_hw_do_switch:
    /* Set up a suspend PandSV exception */
    LDR     r0, =NVIC_INT_CTRL
    LDR     r1, =NVIC_PENDSVSET
    STR     r1, [r0]
    BX      lr

.global nd_hw_do_switch_first
.type nd_hw_do_switch_first, %function
nd_hw_do_switch_first:
    /* Set up a suspend PandSV exception */
    LDR     r0, =NVIC_INT_CTRL
    LDR     r1, =NVIC_PENDSVSET
    STR     r1, [r0]

    /* Set PendSV to lowest priority */
    LDR     r0, =NVIC_SYSPRI2
    LDR     r1, [r0]
    LDR     r2, =NVIC_PENDSV_PRI
    BIC     r1, r1, r2
    ORR     r1, r1, r2
    STR     r1, [r0]

    /* Retrieve the MSP from the interrupt vector table 
    and write it back to the MSP register. */
    LDR     r0, =SCB_VTOR
    LDR     r0, [r0]
    LDR     r0, [r0]
    MSR     MSP, r0

    CPSIE   F
    CPSIE   I
    DSB

.global isr_pendsv
.type isr_pendsv, %function
isr_pendsv:
    /* Disable interrupts */
    CPSID   I 

    LDR     r0, =nd_current_thread
    LDR     r1, [r0]

    CBZ     r1, first_switch

    /* Use the r2 register as the SP of the thread stack */
    MRS     r2, PSP

    /* Save current thread context to PSP */
    STMDB   r2!, {r4-r11, lr}

    /* nd_current_thread->sp = PSP */
    LDR     r3, =nd_current_thread
    LDR     r4, [r3]
    STR     r2, [r4]

    /* Extract nd_next_thread and pass it as the parameter 
    for the next thread to the C callback function */
    LDR     r0, =nd_next_thread
    LDR     r0, [r0]
    BL      nd_context_switch_cb

    /* Re-read the current thread pointer 
    (which is usually updated by the C callback at this point) */
    LDR     r6, =nd_current_thread
    LDR     r6, [r6]

    /* Read the stack pointer of the next thread and restore the context */
    LDR     r2, [r6]
    LDMIA   r2!, {r4-r11, lr}

    /* Change current PSP to nd_next_thread stack */
    MSR     PSP, r2

    /* Use EXC_RETURN to return to the thread mode and use PSP */
    CPSIE   I
    BX      lr
first_switch:
    BL      nd_get_highest_priority_thread
    CBZ     r0, done

    /* Temporarily allocate 8 bytes of stack space, save r0, 
    and maintain stack alignment */
    SUB     sp, sp, #8
    STR     r0, [sp]
    BL      nd_context_switch_cb
    LDR     r3, [sp]
    ADD     sp, sp, #8

    /* Read the initial stack pointer of the thread 
    and skip the preceding reserved area for software. */
    LDR     r1, [r3]
    ADD     r1, r1, #36
    MSR     PSP, r1

    /* Execution exception return: Return to thread 
    mode and use PSP(EXC_RETURN = 0xFFFFFFFD) */
    LDR     lr, =0xFFFFFFFD
    CPSIE   I 
    BX      lr
done:
    CPSIE   I 
    BX      lr


