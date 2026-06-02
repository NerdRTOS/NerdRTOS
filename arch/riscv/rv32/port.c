#include "nerd.h"
#include "nd_internal.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "pico/time.h"

void task_entry_trampoline(void);

#define FRAME_TYPE_VOLUNTARY    1U
#define MSTATUS_MIE             0x8U
#define RISCV_MEI_VECTOR_SLOT   11U
#define RISCV_REG_ZERO          0U
#define RISCV_REG_SP            2U
#define RISCV_REG_T0            5U
#define RISCV_OPCODE_OP_IMM     0x13U
#define RISCV_OPCODE_STORE      0x23U
#define RISCV_OPCODE_LUI        0x37U
#define RISCV_OPCODE_JALR       0x67U
#define RISCV_FUNCT3_ADDI       0U
#define RISCV_FUNCT3_SW         2U

volatile nd_uint64_t nd_tick_count = 0;
static int nd_tick_alarm = -1;
static nd_bool_t nd_irq_vector_patched = ND_FALSE;

extern nd_uint32_t __vectors[];
extern void nd_riscv_irq_entry_from_vector(void);

static nd_uint32_t riscv_encode_i_type(nd_int32_t imm,
                                       nd_uint32_t rs1,
                                       nd_uint32_t funct3,
                                       nd_uint32_t rd,
                                       nd_uint32_t opcode)
{
    return (((nd_uint32_t)imm & 0xfffU) << 20) |
           ((rs1 & 0x1fU) << 15) |
           ((funct3 & 0x7U) << 12) |
           ((rd & 0x1fU) << 7) |
           opcode;
}

static nd_uint32_t riscv_encode_s_type(nd_int32_t imm,
                                       nd_uint32_t rs2,
                                       nd_uint32_t rs1,
                                       nd_uint32_t funct3,
                                       nd_uint32_t opcode)
{
    nd_uint32_t uimm = (nd_uint32_t)imm;

    return (((uimm >> 5) & 0x7fU) << 25) |
           ((rs2 & 0x1fU) << 20) |
           ((rs1 & 0x1fU) << 15) |
           ((funct3 & 0x7U) << 12) |
           ((uimm & 0x1fU) << 7) |
           opcode;
}

static nd_uint32_t riscv_encode_lui(nd_uint32_t rd, nd_uint32_t imm20)
{
    return (imm20 << 12) | ((rd & 0x1fU) << 7) | RISCV_OPCODE_LUI;
}

nd_err_t nd_riscv_patch_external_irq_vector(void)
{
    volatile nd_uint32_t *slot = &__vectors[RISCV_MEI_VECTOR_SLOT];
    nd_uint32_t target = (nd_uint32_t)(nd_ubase_t)nd_riscv_irq_entry_from_vector;
    nd_uint32_t hi20 = (target + 0x800U) >> 12;
    nd_int32_t lo12 = (nd_int32_t)(target - (hi20 << 12));

    slot[0] = riscv_encode_i_type(-4, RISCV_REG_SP, RISCV_FUNCT3_ADDI,
                                  RISCV_REG_SP, RISCV_OPCODE_OP_IMM);
    slot[1] = riscv_encode_s_type(0, RISCV_REG_T0, RISCV_REG_SP,
                                  RISCV_FUNCT3_SW, RISCV_OPCODE_STORE);
    slot[2] = riscv_encode_lui(RISCV_REG_T0, hi20);
    slot[3] = riscv_encode_i_type(lo12, RISCV_REG_T0, RISCV_FUNCT3_ADDI,
                                  RISCV_REG_ZERO, RISCV_OPCODE_JALR);
    __asm__ volatile("fence.i" ::: "memory");

    return ND_EOK;
}

void nd_riscv_patch_external_irq_vector_once(void)
{
    if (!nd_irq_vector_patched &&
        nd_riscv_patch_external_irq_vector() == ND_EOK) {
        nd_irq_vector_patched = ND_TRUE;
    }
}

nd_uint64_t nd_hw_tick_get_current(void)
{
    return nd_tick_count;
}

static void nd_tick_alarm_cb(uint alarm_num)
{
    (void)alarm_num;

    hardware_alarm_set_target((uint)nd_tick_alarm, make_timeout_time_us(1000));

    nd_tick_count++;

    nd_enter_interrupt();
    nd_timer_process();
    nd_exit_interrupt();
}

__attribute__((aligned(4), noinline, naked))
void task_entry_trampoline(void)
{
    __asm__ volatile(
        "csrsi  mstatus, 8   \n"
        "mv     a0, s1       \n"
        "mv     a1, s2       \n"
        "jr     s0           \n");
}

void nd_hw_tick_init(void)
{
    nd_riscv_patch_external_irq_vector_once();

    int alarm = hardware_alarm_claim_unused(false);
    if (alarm < 0) {
        return;
    }
    nd_tick_alarm = alarm;

    uint irq_num = TIMER0_IRQ_0 + (uint)nd_tick_alarm;

    irq_set_enabled(irq_num, false);
    irq_handler_t old = irq_get_vtable_handler(irq_num);
    if (old)
    {
        irq_remove_handler(irq_num, old);
    }

    hardware_alarm_set_callback((uint)nd_tick_alarm, nd_tick_alarm_cb);
    hardware_alarm_set_target((uint)nd_tick_alarm,
                              make_timeout_time_us(1000));
}

void *nd_hw_stack_init(void *entry, void *parameter, nd_uint8_t *stack_addr)
{
    nd_uint32_t *sp;

    sp = (nd_uint32_t *)ND_ALIGN_DOWN((nd_ubase_t)stack_addr, 16);

    /*
     * Initial threads enter through the voluntary restore path:
     * type, mstatus, ra, s0-s11, then one padding word to keep the
     * software frame size equal to nd_hw_do_switch().
     */
    *(--sp) = 0;
    *(--sp) = 0;                      // s11
    *(--sp) = 0;                      // s10
    *(--sp) = 0;                      // s9
    *(--sp) = 0;                      // s8
    *(--sp) = 0;                      // s7
    *(--sp) = 0;                      // s6
    *(--sp) = 0;                      // s5
    *(--sp) = 0;                      // s4
    *(--sp) = 0;                      // s3
    *(--sp) = (nd_uint32_t)parameter; // s2
    *(--sp) = (nd_uint32_t)entry;     // s1
    *(--sp) = (nd_uint32_t)nd_thread_entry; // s0
    *(--sp) = (nd_uint32_t)task_entry_trampoline;
    *(--sp) = MSTATUS_MIE;
    *(--sp) = FRAME_TYPE_VOLUNTARY;

    return (void *)sp;
}

