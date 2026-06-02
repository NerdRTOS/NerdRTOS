.section .text

.global nd_current_thread
.global nd_next_thread
.global nd_interrupt_nest_count

.equ    FRAME_TYPE_VOLUNTARY,    1
.equ    FRAME_TYPE_PREEMPT,      2
.equ    VOLUNTARY_FRAME_SIZE,    64
.equ    PREEMPT_FRAME_SIZE,      144
.equ    VOLUNTARY_RESTORE_SIZE,  56
.equ    PREEMPT_RESTORE_SIZE,    140
.equ    SDK_IRQ_FRAME_SIZE,      80
.equ    VOLUNTARY_PAYLOAD_SIZE,  60
.equ    RVCSR_MEINEXT_OFFSET,    0xbe4
.equ    RVCSR_MEICONTEXT_OFFSET, 0xbe5
.equ    RVCSR_MEINEXT_UPDATE_BITS,       0x1
.equ    RVCSR_MEICONTEXT_CLEARTS_BITS,   0x2
.equ    MSTATUS_MIE,             0x8
.equ    MSTATUS_MPIE,            0x80
.equ    MSTATUS_MPP_MASK,        0x1800
.equ    MSTATUS_MPP_M,           0x1800

.macro SAVE_PREEMPT_FRAME
    addi    sp, sp, -PREEMPT_FRAME_SIZE

    sw      t0,   16(sp)
    li      t0, FRAME_TYPE_PREEMPT
    sw      t0,    0(sp)
    sw      ra,    4(sp)
    sw      gp,    8(sp)
    sw      tp,   12(sp)
    sw      t1,   20(sp)
    sw      t2,   24(sp)
    sw      s0,   28(sp)
    sw      s1,   32(sp)
    sw      a0,   36(sp)
    sw      a1,   40(sp)
    sw      a2,   44(sp)
    sw      a3,   48(sp)
    sw      a4,   52(sp)
    sw      a5,   56(sp)
    sw      a6,   60(sp)
    sw      a7,   64(sp)
    sw      s2,   68(sp)
    sw      s3,   72(sp)
    sw      s4,   76(sp)
    sw      s5,   80(sp)
    sw      s6,   84(sp)
    sw      s7,   88(sp)
    sw      s8,   92(sp)
    sw      s9,   96(sp)
    sw      s10, 100(sp)
    sw      s11, 104(sp)
    sw      t3,  108(sp)
    sw      t4,  112(sp)
    sw      t5,  116(sp)
    sw      t6,  120(sp)

    csrr    t0, mepc
    sw      t0, 124(sp)
    csrr    t0, mstatus
    sw      t0, 128(sp)
.endm

.macro SAVE_CURRENT_THREAD_SP
    la      t0, nd_current_thread
    lw      t1, 0(t0)
    beqz    t1, 1f
    sw      sp, 0(t1)
1:
.endm

.macro RESTORE_CURRENT_THREAD
    la      t0, nd_current_thread
    lw      t1, 0(t0)
    beqz    t1, restore_dispatch
    lw      sp, 0(t1)
    j       restore_dispatch
.endm

.macro RESTORE_CURRENT_THREAD_IRQ
    la      t0, nd_current_thread
    lw      t1, 0(t0)
    beqz    t1, restore_dispatch_irq
    lw      sp, 0(t1)
    j       restore_dispatch_irq
.endm

.p2align 2
.global trap_entry
trap_entry:
    SAVE_PREEMPT_FRAME
    SAVE_CURRENT_THREAD_SP
    call    trap_handler
    RESTORE_CURRENT_THREAD


.p2align 2
.global nd_riscv_irq_entry_from_vector
nd_riscv_irq_entry_from_vector:
    lw      t0, 0(sp)
    addi    sp, sp, 4

.global nd_riscv_irq_entry
nd_riscv_irq_entry:
    SAVE_PREEMPT_FRAME
    SAVE_CURRENT_THREAD_SP
    call    nd_riscv_platform_irq_dispatch
    RESTORE_CURRENT_THREAD_IRQ


.p2align 2
.global nd_riscv_platform_irq_dispatch
nd_riscv_platform_irq_dispatch:
    addi    sp, sp, -SDK_IRQ_FRAME_SIZE
    sw      ra,  0(sp)
    sw      t0,  4(sp)
    sw      t1,  8(sp)
    sw      t2, 12(sp)
    sw      a0, 16(sp)
    sw      a1, 20(sp)
    sw      a2, 24(sp)
    sw      a3, 28(sp)
    sw      a4, 32(sp)
    sw      a5, 36(sp)
    sw      a6, 40(sp)
    sw      a7, 44(sp)
    sw      t3, 48(sp)
    sw      t4, 52(sp)
    sw      t5, 56(sp)
    sw      t6, 60(sp)
    csrr    a0, mepc
    csrr    a1, mstatus
    sw      a0, 64(sp)
    sw      a1, 68(sp)

.Lsave_meicontext:
    csrrsi  a2, RVCSR_MEICONTEXT_OFFSET, RVCSR_MEICONTEXT_CLEARTS_BITS
    sw      a2, 72(sp)

.Lget_first_irq:
    csrrsi  a0, RVCSR_MEINEXT_OFFSET, RVCSR_MEINEXT_UPDATE_BITS
    bltz    a0, .Lno_more_irqs

.Ldispatch_irq:
    csrsi   mstatus, 0x8
    call    nd_riscv_platform_irq_handle
    csrci   mstatus, 0x8

.Lget_next_irq:
    csrrsi  a0, RVCSR_MEINEXT_OFFSET, RVCSR_MEINEXT_UPDATE_BITS
    bgez    a0, .Ldispatch_irq

.Lno_more_irqs:
    call    nd_schedule_irq_exit

    lw      a0, 64(sp)
    lw      a1, 68(sp)
    lw      a2, 72(sp)
    csrw    mepc, a0
    csrw    mstatus, a1
    csrw    RVCSR_MEICONTEXT_OFFSET, a2
    lw      ra,  0(sp)
    lw      t0,  4(sp)
    lw      t1,  8(sp)
    lw      t2, 12(sp)
    lw      a1, 20(sp)
    lw      a2, 24(sp)
    lw      a3, 28(sp)
    lw      a4, 32(sp)
    lw      a5, 36(sp)
    lw      a6, 40(sp)
    lw      a7, 44(sp)
    lw      t3, 48(sp)
    lw      t4, 52(sp)
    lw      t5, 56(sp)
    lw      t6, 60(sp)

.Lcheck_irq_before_exit:
    csrr    a0, RVCSR_MEINEXT_OFFSET
    bgez    a0, .Lsave_meicontext
    csrci   mstatus, 0x8
    lw      a0, 16(sp)
    addi    sp, sp, SDK_IRQ_FRAME_SIZE
    ret


.p2align 2
.global nd_hw_irq_disable
nd_hw_irq_disable:
    csrci   mstatus, 8
    ret


.p2align 2
.global nd_hw_irq_enable
nd_hw_irq_enable:
    csrsi   mstatus, 8
    ret


.p2align 2
.global nd_hw_irq_save
nd_hw_irq_save:
    csrrci  a0, mstatus, 8
    ret


.p2align 2
.global nd_hw_irq_restore
nd_hw_irq_restore:
    csrw    mstatus, a0
    ret


.p2align 2
.global nd_hw_do_switch
nd_hw_do_switch:
    addi    sp, sp, -VOLUNTARY_FRAME_SIZE

    li      t0, FRAME_TYPE_VOLUNTARY
    sw      t0,   0(sp)
    csrr    t0, mstatus
    sw      t0,   4(sp)
    sw      ra,   8(sp)
    sw      s0,  12(sp)
    sw      s1,  16(sp)
    sw      s2,  20(sp)
    sw      s3,  24(sp)
    sw      s4,  28(sp)
    sw      s5,  32(sp)
    sw      s6,  36(sp)
    sw      s7,  40(sp)
    sw      s8,  44(sp)
    sw      s9,  48(sp)
    sw      s10, 52(sp)
    sw      s11, 56(sp)

    la      t0, nd_current_thread
    lw      t1, 0(t0)
    sw      sp, 0(t1)

    la      t0, nd_next_thread
    lw      a0, 0(t0)

    call    nd_context_switch_cb

    la      t0, nd_current_thread
    lw      t1, 0(t0)
    lw      sp, 0(t1)

    j       restore_dispatch


.p2align 2
.global nd_hw_do_switch_first
nd_hw_do_switch_first:
    csrci   mstatus, 8

    addi    sp, sp, -16

    call    nd_get_highest_priority_thread

    beqz    a0, .done

    sw      a0, 0(sp)
    call    nd_context_switch_cb
    
    lw      t0, 0(sp)
    addi    sp, sp, 16
    lw      sp, 0(t0)

    j       restore_dispatch

restore_dispatch:
    lw      t0, 0(sp)
    addi    sp, sp, 4

    li      t1, FRAME_TYPE_VOLUNTARY
    beq     t0, t1, restore_voluntary

    li      t1, FRAME_TYPE_PREEMPT
    beq     t0, t1, restore_preempt

restore_dispatch_irq:
    lw      t0, 0(sp)
    addi    sp, sp, 4

    li      t1, FRAME_TYPE_VOLUNTARY
    beq     t0, t1, restore_voluntary_irq

    li      t1, FRAME_TYPE_PREEMPT
    beq     t0, t1, restore_preempt

restore_voluntary:
    lw      t0,   0(sp)
    csrw    mstatus, t0
    addi    sp, sp, 4

    lw      ra,   0(sp)
    lw      s0,   4(sp)
    lw      s1,   8(sp)
    lw      s2,  12(sp)
    lw      s3,  16(sp)
    lw      s4,  20(sp)
    lw      s5,  24(sp)
    lw      s6,  28(sp)
    lw      s7,  32(sp)
    lw      s8,  36(sp)
    lw      s9,  40(sp)
    lw      s10, 44(sp)
    lw      s11, 48(sp)

    addi    sp, sp, VOLUNTARY_RESTORE_SIZE
    ret

restore_voluntary_irq:
    lw      t0,   0(sp)
    lw      t1,   4(sp)
    csrw    mepc, t1

    andi    t1, t0, MSTATUS_MIE
    slli    t1, t1, 4
    li      t2, ~(MSTATUS_MIE | MSTATUS_MPIE | MSTATUS_MPP_MASK)
    and     t0, t0, t2
    or      t0, t0, t1
    li      t1, MSTATUS_MPP_M
    or      t0, t0, t1
    csrw    mstatus, t0

    lw      ra,   4(sp)
    lw      s0,   8(sp)
    lw      s1,  12(sp)
    lw      s2,  16(sp)
    lw      s3,  20(sp)
    lw      s4,  24(sp)
    lw      s5,  28(sp)
    lw      s6,  32(sp)
    lw      s7,  36(sp)
    lw      s8,  40(sp)
    lw      s9,  44(sp)
    lw      s10, 48(sp)
    lw      s11, 52(sp)

    addi    sp, sp, VOLUNTARY_PAYLOAD_SIZE
    mret

restore_preempt:
    lw      t0, 124(sp)
    csrw    mstatus, t0
    lw      t0, 120(sp)
    csrw    mepc, t0

    lw      ra,    0(sp)
    lw      gp,    4(sp)
    lw      tp,    8(sp)
    lw      t0,   12(sp)
    lw      t1,   16(sp)
    lw      t2,   20(sp)
    lw      s0,   24(sp)
    lw      s1,   28(sp)
    lw      a0,   32(sp)
    lw      a1,   36(sp)
    lw      a2,   40(sp)
    lw      a3,   44(sp)
    lw      a4,   48(sp)
    lw      a5,   52(sp)
    lw      a6,   56(sp)
    lw      a7,   60(sp)
    lw      s2,   64(sp)
    lw      s3,   68(sp)
    lw      s4,   72(sp)
    lw      s5,   76(sp)
    lw      s6,   80(sp)
    lw      s7,   84(sp)
    lw      s8,   88(sp)
    lw      s9,   92(sp)
    lw      s10,  96(sp)
    lw      s11, 100(sp)
    lw      t3,  104(sp)
    lw      t4,  108(sp)
    lw      t5,  112(sp)
    lw      t6,  116(sp)

    addi    sp, sp, PREEMPT_RESTORE_SIZE
    mret
.done:
    addi    sp, sp, 16
    ret

