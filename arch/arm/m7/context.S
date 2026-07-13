.cpu cortex-m7
.fpu fpv5-d16
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
    LDR     r0, =NVIC_INT_CTRL
    LDR     r1, =NVIC_PENDSVSET
    STR     r1, [r0]
    BX      lr

.global nd_hw_do_switch_first
.type nd_hw_do_switch_first, %function
nd_hw_do_switch_first:
    LDR     r0, =NVIC_INT_CTRL
    LDR     r1, =NVIC_PENDSVSET
    STR     r1, [r0]

    LDR     r0, =NVIC_SYSPRI2
    LDR     r1, [r0]
    LDR     r2, =NVIC_PENDSV_PRI
    BIC     r1, r1, r2
    ORR     r1, r1, r2
    STR     r1, [r0]

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
    CPSID   I

    LDR     r0, =nd_current_thread
    LDR     r1, [r0]

    CBZ     r1, first_switch

    MRS     r2, PSP

    TST     lr, #0x10
    BNE     skip_save_fpu
    VSTMDB  r2!, {s16-s31}
skip_save_fpu:
    STMDB   r2!, {r4-r11, lr}

    LDR     r3, =nd_current_thread
    LDR     r4, [r3]        /* r4 = current */
    STR     r2, [r4]        /* current->sp = r2 */

    LDR     r0, =nd_next_thread
    LDR     r0, [r0]        /* r0 = next */
    BL      nd_context_switch_cb

    LDR     r6, =nd_current_thread
    LDR     r6, [r6]        /* r6 = next */

    LDR     r2,  [r6]        /* r2 = next->sp */
    LDMIA   r2!, {r4-r11, lr}
    TST     lr,  #0x10
    BNE     skip_restore_fpu
    VLDMIA  r2!, {s16-s31}
skip_restore_fpu:
    MSR     PSP, r2

    CPSIE   I
    BX      lr

first_switch:
    BL      nd_get_highest_priority_thread
    CBZ     r0, done

    SUB     sp, sp, #8
    STR     r0, [sp]
    BL      nd_context_switch_cb
    LDR     r3, [sp]
    ADD     sp, sp, #8

    LDR     r1, [r3]
    ADD     r1, r1, #36
    MSR     PSP, r1

    LDR     lr, =0xFFFFFFFD
    CPSIE   I
    BX      lr

done:
    CPSIE   I
    BX      lr
