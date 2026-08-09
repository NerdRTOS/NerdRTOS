#include "bsp_memory.h"

#include "stm32h7xx_hal.h"

#define ETH_DMA_SRAM_BASE_ADDRESS    (0x30040000UL)

void bsp_memory_init(void)
{
    MPU_Region_InitTypeDef mpu_region = {0};

    /* The linker places .eth_dma in D2 SRAM3.  The clock gate must be open
       before the CPU or Ethernet DMA accesses that memory. */
    __HAL_RCC_D2SRAM3_CLK_ENABLE();

    /* SRAM3 is shared by the Cortex-M7 and Ethernet DMA.  Describing it as
       non-cacheable normal memory makes both masters observe the same data. */
    HAL_MPU_Disable();

    mpu_region.Enable           = MPU_REGION_ENABLE;
    mpu_region.Number           = MPU_REGION_NUMBER0;
    mpu_region.BaseAddress      = ETH_DMA_SRAM_BASE_ADDRESS;
    mpu_region.Size             = MPU_REGION_SIZE_32KB;
    mpu_region.SubRegionDisable = 0x00U;
    mpu_region.TypeExtField     = MPU_TEX_LEVEL1;
    mpu_region.AccessPermission = MPU_REGION_FULL_ACCESS;
    mpu_region.DisableExec      = MPU_INSTRUCTION_ACCESS_DISABLE;
    mpu_region.IsShareable      = MPU_ACCESS_SHAREABLE;
    mpu_region.IsCacheable      = MPU_ACCESS_NOT_CACHEABLE;
    mpu_region.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;

    HAL_MPU_ConfigRegion(&mpu_region);
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

    SCB_EnableICache();
    SCB_EnableDCache();
}
