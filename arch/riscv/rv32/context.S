.section .text

.global nd_current_thread
.global nd_next_thread

.p2align 2
.global trap_entry
trap_entry:
    addi    sp, sp, -72

    sw      ra,  0(sp)
    sw      t0,  4(sp)
    sw      t1,  8(sp)
    sw      t2,  12(sp)
    sw      a0,  16(sp)
    sw      a1,  20(sp)
    sw      a2,  24(sp)
    sw      a3,  28(sp)
    sw      a4,  32(sp)
    sw      a5,  36(sp)
    sw      a6,  40(sp)
    sw      a7,  44(sp)
    sw      t3,  48(sp)
    sw      t4,  52(sp)
    sw      t5,  56(sp)
    sw      t6,  60(sp)

    csrr    t0, mepc
    sw      t0, 64(sp)
    csrr    t0, mstatus
    sw      t0, 68(sp)

    call    trap_handler

    lw      t0, 68(sp)
    csrw    mstatus, t0
    lw      t0, 64(sp)
    csrw    mepc, t0

    lw      ra,  0(sp)
    lw      t0,  4(sp)
    lw      t1,  8(sp)
    lw      t2,  12(sp)
    lw      a0,  16(sp)
    lw      a1,  20(sp)
    lw      a2,  24(sp)
    lw      a3,  28(sp)
    lw      a4,  32(sp)
    lw      a5,  36(sp)
    lw      a6,  40(sp)
    lw      a7,  44(sp)
    lw      t3,  48(sp)
    lw      t4,  52(sp)
    lw      t5,  56(sp)
    lw      t6,  60(sp)

    addi    sp, sp, 72

    mret


.p2align 2
.global nd_hw_irq_disable
nd_hw_irq_disable:
    csrci   mstatus, 8
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
    addi    sp, sp, -52

    sw      ra,   0(sp)
    sw      s0,   4(sp)
    sw      s1,   8(sp)
    sw      s2,  12(sp)
    sw      s3,  16(sp)
    sw      s4,  20(sp)
    sw      s5,  24(sp)
    sw      s6,  28(sp)
    sw      s7,  32(sp)
    sw      s8,  36(sp)
    sw      s9,  40(sp)
    sw      s10, 44(sp)
    sw      s11, 48(sp)

    la      t0, nd_current_thread
    lw      t1, 0(t0)
    sw      sp, 0(t1)

    la      t0, nd_next_thread
    lw      a0, 0(t0)

    call    nd_context_switch_cb

    la      t0, nd_current_thread
    lw      t1, 0(t0)
    lw      sp, 0(t1)

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

    addi    sp, sp, 52

    ret


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

    addi    sp, sp, 52
    ret
.done:
    addi    sp, sp, 16
    ret
