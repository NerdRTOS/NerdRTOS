#ifndef __RISCV_CSR_H__
#define __RISCV_CSR_H__

#define RISCV_MSTATUS_MIE_IMM               8
#define RISCV_STRINGIFY_VALUE(value)        #value
#define RISCV_STRINGIFY(value)              RISCV_STRINGIFY_VALUE(value)

#ifdef __ASSEMBLER__
#define RISCV_MSTATUS_MIE                   0x8
#define RISCV_MIE_MTIE                      0x80
#define RISCV_MIE_MEIE                      0x800

#define RISCV_CSR_MEINEXT                   0xbe4
#define RISCV_CSR_MEICONTEXT                0xbe5
#define RISCV_MEICONTEXT_FOREGROUND         0x8000
#else
#define RISCV_MSTATUS_MIE                   0x8U
#define RISCV_MIE_MTIE                      0x80U
#define RISCV_MIE_MEIE                      0x800U

#define RISCV_CSR_MEINEXT                   0xbe4
#define RISCV_CSR_MEICONTEXT                0xbe5
#define RISCV_MEINEXT_UPDATE                1
#define RISCV_MEINEXT_NONE                  0x80000000UL
#define RISCV_MEINEXT_IRQ_SHIFT             2U
#define RISCV_MEICONTEXT_FOREGROUND         0x8000U

#define RISCV_MCAUSE_INTERRUPT              0x80000000UL
#define RISCV_MCAUSE_CODE_MASK              0x7FFFFFFFUL
#define RISCV_MCAUSE_ILLEGAL_INSTRUCTION   2U
#define RISCV_MCAUSE_ECALL_M_MODE           11U

#define RISCV_IRQ_M_SOFT                    3U
#define RISCV_IRQ_M_TIMER                   7U
#define RISCV_IRQ_M_EXT                     11U
#endif

#endif

