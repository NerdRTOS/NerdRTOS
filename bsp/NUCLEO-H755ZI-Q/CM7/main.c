#include "stm32h7xx.h"
#include "stm32h7xx_ll_hsem.h"

volatile uint32_t boot_error;

void _init(void)
{
}

static void boot_fail(uint32_t error)
{
    boot_error = error;
    while (1) {
        __NOP();
    }
}

int main(void)
{
    uint32_t timeout = 0x1000000;

    while (RCC->CR & RCC_CR_D2CKRDY) {
        if (--timeout == 0) {
            boot_fail(1);
        }
    }

    SystemCoreClockUpdate();
    if (SystemCoreClock != 64000000UL || SystemD2Clock != 64000000UL) {
        boot_fail(2);
    }

    RCC->AHB4ENR |= RCC_AHB4ENR_HSEMEN;
    (void)RCC->AHB4ENR;
    if (LL_HSEM_1StepLock(HSEM, 0) != 0) {
        boot_fail(3);
    }
    LL_HSEM_ReleaseLock(HSEM, 0, 0);

    timeout = 0x1000000;
    while (!(RCC->CR & RCC_CR_D2CKRDY)) {
        if (--timeout == 0) {
            boot_fail(4);
        }
    }

    while (1) {
        __WFI();
    }
}
