#ifndef __ETH_H
#define __ETH_H

#include <stdint.h>

#include "stm32h7xx_hal.h"

#define STM32_ETH_MAC_ADDRESS_SIZE    (6U)
#define STM32_ETH_DMA_ALIGNMENT       (32U)
#define STM32_ETH_RX_BUFFER_SIZE      \
    (((ETH_MAX_PACKET_SIZE + STM32_ETH_DMA_ALIGNMENT - 1U) / \
      STM32_ETH_DMA_ALIGNMENT) * STM32_ETH_DMA_ALIGNMENT)

typedef enum {
    STM32_ETH_LINK_SPEED_10M = 0,
    STM32_ETH_LINK_SPEED_100M,
} eth_link_speed_t;

typedef enum {
    STM32_ETH_LINK_HALF_DUPLEX = 0,
    STM32_ETH_LINK_FULL_DUPLEX,
} eth_link_duplex_t;

extern ETH_HandleTypeDef g_eth_handle;

HAL_StatusTypeDef eth_init(
    const uint8_t mac_address[STM32_ETH_MAC_ADDRESS_SIZE]);
HAL_StatusTypeDef eth_configure_link(
    eth_link_speed_t speed,
    eth_link_duplex_t duplex);

#endif /* __ETH_H */
