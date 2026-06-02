#include "csr.h"

.section .text

.global nd_current_thread
.global nd_next_thread
.global nd_interrupt_nest_count

.equ    FRAME_TYPE_VOLUNTARY,    1
.equ    FRAME_TYPE_IRQ,          2
.equ    VOLUNTARY_FRAME_SIZE,    64
.equ    VOLUNTARY_RESTORE_SIZE,  56

.equ    IRQ_FRAME_SIZE,          160
.equ    IRQ_FRAME_MSTATUS,       4
.equ    IRQ_FRAME_MEPC,          8
.equ    IRQ_FRAME_RA,            12
.equ    IRQ_FRAME_GP,            16
.equ    IRQ_FRAME_TP,            20
.equ    IRQ_FRAME_T0,            24
.equ    IRQ_FRAME_T1,            28
.equ    IRQ_FRAME_T2,            32
.equ    IRQ_FRAME_S0,            36
.equ    IRQ_FRAME_S1,            40
.equ    IRQ_FRAME_A0,            44
.equ    IRQ_FRAME_A1,            48
.equ    IRQ_FRAME_A2,            52
.equ    IRQ_FRAME_A3,            56
.equ    IRQ_FRAME_A4,            60
.equ    IRQ_FRAME_A5,            64
.equ    IRQ_FRAME_A6,            68
.equ    IRQ_FRAME_A7,            72
.equ    IRQ_FRAME_S2,            76
.equ    IRQ_FRAME_S3,            80
.equ    IRQ_FRAME_S4,            84
.equ    IRQ_FRAME_S5,            88
.equ    IRQ_FRAME_S6,            92
.equ    IRQ_FRAME_S7,            96
.equ    IRQ_FRAME_S8,            100
.equ    IRQ_FRAME_S9,            104
.equ    IRQ_FRAME_S10,           108
.equ    IRQ_FRAME_S11,           112
.equ    IRQ_FRAME_T3,            116
.equ    IRQ_FRAME_T4,            120
.equ    IRQ_FRAME_T5,            124
.equ    IRQ_FRAME_T6,            128
.equ    IRQ_FRAME_MEICONTEXT,    132


.p2align 2
.global nd_hw_irq_disable
nd_hw_irq_disable:
    csrci   mstatus, RISCV_MSTATUS_MIE
    ret


.p2align 2
.global nd_hw_irq_enable
nd_hw_irq_enable:
    csrsi   mstatus, RISCV_MSTATUS_MIE
    ret


.p2align 2
.global nd_hw_irq_save
nd_hw_irq_save:
    csrrci  a0, mstatus, RISCV_MSTATUS_MIE
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
    csrci   mstatus, RISCV_MSTATUS_MIE

    addi    sp, sp, -16

    call    nd_get_highest_priority_thread

    beqz    a0, .done

    sw      a0, 0(sp)
    call    nd_context_switch_cb
    
    lw      t0, 0(sp)
    addi    sp, sp, 16
    lw      sp, 0(t0)

    j       restore_dispatch

.global restore_dispatch
restore_dispatch:
    lw      t0, 0(sp)
    li      t1, FRAME_TYPE_IRQ
    beq     t0, t1, restore_irq

    addi    sp, sp, 4

restore_voluntary:
    li      t0, RISCV_MEICONTEXT_FOREGROUND
    csrw    RISCV_CSR_MEICONTEXT, t0

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

restore_irq:
    la      t0, nd_current_thread
    lw      t1, 0(t0)
    beqz    t1, 1f
    addi    t2, sp, IRQ_FRAME_SIZE
    sw      t2, 0(t1)
1:
    lw      t0, IRQ_FRAME_MEPC(sp)
    csrw    mepc, t0
    lw      t0, IRQ_FRAME_MSTATUS(sp)
    csrw    mstatus, t0
    lw      t0, IRQ_FRAME_MEICONTEXT(sp)
    csrw    RISCV_CSR_MEICONTEXT, t0

    lw      ra, IRQ_FRAME_RA(sp)
    lw      gp, IRQ_FRAME_GP(sp)
    lw      tp, IRQ_FRAME_TP(sp)
    lw      t1, IRQ_FRAME_T1(sp)
    lw      t2, IRQ_FRAME_T2(sp)
    lw      s0, IRQ_FRAME_S0(sp)
    lw      s1, IRQ_FRAME_S1(sp)
    lw      a0, IRQ_FRAME_A0(sp)
    lw      a1, IRQ_FRAME_A1(sp)
    lw      a2, IRQ_FRAME_A2(sp)
    lw      a3, IRQ_FRAME_A3(sp)
    lw      a4, IRQ_FRAME_A4(sp)
    lw      a5, IRQ_FRAME_A5(sp)
    lw      a6, IRQ_FRAME_A6(sp)
    lw      a7, IRQ_FRAME_A7(sp)
    lw      s2, IRQ_FRAME_S2(sp)
    lw      s3, IRQ_FRAME_S3(sp)
    lw      s4, IRQ_FRAME_S4(sp)
    lw      s5, IRQ_FRAME_S5(sp)
    lw      s6, IRQ_FRAME_S6(sp)
    lw      s7, IRQ_FRAME_S7(sp)
    lw      s8, IRQ_FRAME_S8(sp)
    lw      s9, IRQ_FRAME_S9(sp)
    lw      s10, IRQ_FRAME_S10(sp)
    lw      s11, IRQ_FRAME_S11(sp)
    lw      t3, IRQ_FRAME_T3(sp)
    lw      t4, IRQ_FRAME_T4(sp)
    lw      t5, IRQ_FRAME_T5(sp)
    lw      t6, IRQ_FRAME_T6(sp)
    lw      t0, IRQ_FRAME_T0(sp)

    addi    sp, sp, IRQ_FRAME_SIZE
    mret
.done:
    addi    sp, sp, 16
    ret

