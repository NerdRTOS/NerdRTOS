.section .text

.global nd_current_thread
.global nd_next_thread
.global nd_interrupt_nest_count

.equ    FRAME_TYPE_VOLUNTARY,    1
.equ    VOLUNTARY_FRAME_SIZE,    64
.equ    VOLUNTARY_RESTORE_SIZE,  56


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
    addi    sp, sp, 4

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
.done:
    addi    sp, sp, 16
    ret

