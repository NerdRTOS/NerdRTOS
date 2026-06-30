.cpu cortex-a9
.arm

.global nd_current_thread
.global nd_next_thread

.equ    FRAME_TYPE_VOLUNTARY,    1
.equ    FRAME_TYPE_PREEMPT,      2

.equ    IRQ_MODE,           0x12
.equ    SYS_MODE,           0x1F

.global nd_hw_irq_disable
.type nd_hw_irq_disable, %function
nd_hw_irq_disable:
    CPSID   IF
    BX      lr

.global nd_hw_irq_enable
.type nd_hw_irq_enable, %function
nd_hw_irq_enable:
    CPSIE   IF
    BX      lr

.global nd_hw_irq_save
.type nd_hw_irq_save, %function
nd_hw_irq_save:
    MRS     r0, CPSR
    CPSID   IF
    BX      lr

.global nd_hw_irq_restore
.type nd_hw_irq_restore, %function
nd_hw_irq_restore:
    MSR     CPSR_c, r0
    BX      lr

.global nd_hw_do_switch_first
.type nd_hw_do_switch_first, %function
nd_hw_do_switch_first:
    BL      nd_get_highest_priority_thread
    CMP     r0, #0
    BEQ     done

    BL      nd_context_switch_cb

    LDR     r0, =nd_current_thread
    LDR     r0, [r0]

    CPS     #SYS_MODE
    LDR     sp, [r0]

    B       restore_dispatch

done:
    CPSIE   IF
    BX      lr

.global nd_hw_do_switch
.type nd_hw_do_switch, %function
nd_hw_do_switch:
    MRS     r2, CPSR
    CPSID   IF

    STMDB   sp!, {r4-r12, lr}
    STR     r2, [sp, #-4]!
    MOV     r2, #FRAME_TYPE_VOLUNTARY
    PUSH    {r2}

    LDR     r3, =nd_current_thread
    LDR     r4, [r3]
    STR     sp, [r4]

    LDR     r0, =nd_next_thread
    LDR     r0, [r0]
    BL      nd_context_switch_cb

    LDR     r6, =nd_current_thread
    LDR     r6, [r6]

    LDR     sp, [r6]

restore_dispatch:
    POP     {r0}

    CMP     r0, #FRAME_TYPE_VOLUNTARY
    BEQ     restore_voluntary

    CMP     r0, #FRAME_TYPE_PREEMPT
    BEQ     restore_preempt

restore_preempt:
    POP     {r0-r12 ,lr}
    RFEIA   sp!

restore_voluntary:
    LDR     r2,  [sp], #4
    LDMIA   sp!, {r4-r12, lr}
    MSR     CPSR_c, r2
    BX      lr

.global vector_irq
.type vector_irq, %function
vector_irq:
    SUB     lr, lr, #4
    SRSDB   sp!, #SYS_MODE
    CPS     #SYS_MODE

    PUSH    {r0-r12, lr}
    MOV     r2, #FRAME_TYPE_PREEMPT
    PUSH    {r2}

    LDR     r3, =nd_current_thread
    LDR     r4, [r3]
    STR     sp, [r4]

    CPS     #IRQ_MODE
    BL      gic_dispatch
    BL      nd_schedule_irq_exit

    CPS     #SYS_MODE
    LDR     r3, = nd_current_thread
    LDR     r4, [r3]
    LDR     sp, [r4]

    B       restore_dispatch
