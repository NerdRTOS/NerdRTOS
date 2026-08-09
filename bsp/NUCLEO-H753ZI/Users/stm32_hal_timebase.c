#include "stm32h7xx_hal.h"
#include "nd_config.h"
#include "nd_hw.h"
#include "nd_internal.h"
#include "stm32h7xx_hal_def.h"

#define MILLISECONDS_PER_SECOND    (1000ULL)

uint32_t HAL_GetTick(void)
{
#if ND_CFG_TICKLESS
    return (uint32_t)(nd_hw_get_current() * MILLISECONDS_PER_SECOND /
                      ND_TICKLESS_FREQ);
#else
    return (uint32_t)(nd_hw_get_current() * MILLISECONDS_PER_SECOND /
                      ND_TICKS_PER_SEC);
#endif
}

HAL_StatusTypeDef HAL_InitTick(uint32_t tick_priority)
{
    (void)tick_priority;

    nd_hw_tick_init();

    return HAL_OK;
}
