.cpu cortex-a9
.arm

.section .vectors, "ax"
.global _start
_start:
    LDR pc, =Reset_Handler      @ 0x00: Reset
    B   .                       @ 0x04: Undefined Instruction
    B   .                       @ 0x08: Supervisor Call
    B   .                       @ 0x0C: Prefetch Abort
    B   .                       @ 0x10: Data Abort
    B   .                       @ 0x14: Reserved
    LDR pc, =vector_irq         @ 0x18: IRQ interrupt
    B   .                       @ 0x1C: FIQ interrupt

@
@ CPSR_c bit assignments:
@   [7] [6] [5] [4:0]
@    I   F   T    M
@
.text
Reset_Handler:
    LDR     r0, =_start
    MCR     p15, 0, r0, c12, c0, 0

    MSR CPSR_c, #0xD1           @ 0xD1 = I=1, F=1, mode=FIQ(10001)
    LDR SP,     =_fiq_stack_top

    MSR CPSR_c, #0xD2           @ 0xD2 = I=1, F=1, mode=IRQ(10010)
    LDR SP,     =_irq_stack_top

    MSR CPSR_c, #0xD7           @ 0xD7 = I=1, F=1, mode=ABT(10111)
    LDR SP,     =_abt_stack_top

    MSR CPSR_c, #0xDB           @ 0xDB = I=1, F=1, mode=UND(11011)
    LDR SP,     =_und_stack_top

    MSR CPSR_c, #0xD3           @ 0xD3 = I=1, F=1, mode=SVC(10011)
    LDR SP,     =_svc_stack_top

    LDR r0,     =_bss_start
    LDR r1,     =_bss_end
    MOV r2,     #0

bss_loop:
    CMP     r0, r1
    STRLT   r2, [r0],   #4
    BLT     bss_loop

    BL      main
    B       .
