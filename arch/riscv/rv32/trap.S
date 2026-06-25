#include "csr.h"

/* The SDK RISC-V vector table uses a single jal per slot, so this entry must
 * live in RAM near the SDK RAM vector table rather than in flash/XIP.
 */
.section .time_critical.riscv_trap_entry, "ax"

.global restore_dispatch
.global riscv_trap_handler
.global nd_current_thread
.global nd_schedule_irq_exit

.equ    FRAME_TYPE_PREEMPT,          2

.equ    IRQ_FRAME_SIZE,          160
.equ    IRQ_FRAME_TYPE,          0
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
.global riscv_trap_entry
riscv_trap_entry:
    addi    sp, sp, -IRQ_FRAME_SIZE

    sw      t0, IRQ_FRAME_T0(sp)

    li      t0, FRAME_TYPE_PREEMPT
    sw      t0, IRQ_FRAME_TYPE(sp)

    csrr    t0, mstatus
    sw      t0, IRQ_FRAME_MSTATUS(sp)
    csrr    t0, mepc
    sw      t0, IRQ_FRAME_MEPC(sp)
    csrr    t0, RISCV_CSR_MEICONTEXT
    sw      t0, IRQ_FRAME_MEICONTEXT(sp)

    sw      ra, IRQ_FRAME_RA(sp)
    sw      gp, IRQ_FRAME_GP(sp)
    sw      tp, IRQ_FRAME_TP(sp)
    sw      t1, IRQ_FRAME_T1(sp)
    sw      t2, IRQ_FRAME_T2(sp)
    sw      s0, IRQ_FRAME_S0(sp)
    sw      s1, IRQ_FRAME_S1(sp)
    sw      a0, IRQ_FRAME_A0(sp)
    sw      a1, IRQ_FRAME_A1(sp)
    sw      a2, IRQ_FRAME_A2(sp)
    sw      a3, IRQ_FRAME_A3(sp)
    sw      a4, IRQ_FRAME_A4(sp)
    sw      a5, IRQ_FRAME_A5(sp)
    sw      a6, IRQ_FRAME_A6(sp)
    sw      a7, IRQ_FRAME_A7(sp)
    sw      s2, IRQ_FRAME_S2(sp)
    sw      s3, IRQ_FRAME_S3(sp)
    sw      s4, IRQ_FRAME_S4(sp)
    sw      s5, IRQ_FRAME_S5(sp)
    sw      s6, IRQ_FRAME_S6(sp)
    sw      s7, IRQ_FRAME_S7(sp)
    sw      s8, IRQ_FRAME_S8(sp)
    sw      s9, IRQ_FRAME_S9(sp)
    sw      s10, IRQ_FRAME_S10(sp)
    sw      s11, IRQ_FRAME_S11(sp)
    sw      t3, IRQ_FRAME_T3(sp)
    sw      t4, IRQ_FRAME_T4(sp)
    sw      t5, IRQ_FRAME_T5(sp)
    sw      t6, IRQ_FRAME_T6(sp)

    la      t0, nd_current_thread
    lw      t1, 0(t0)
    beqz    t1, 1f
    sw      sp, 0(t1)
1:
    mv      a0, sp
    call    riscv_trap_handler

    call    nd_schedule_irq_exit

    la      t0, nd_current_thread
    lw      t1, 0(t0)
    beqz    t1, 2f
    lw      sp, 0(t1)
2:
    la      t0, restore_dispatch
    jr      t0

