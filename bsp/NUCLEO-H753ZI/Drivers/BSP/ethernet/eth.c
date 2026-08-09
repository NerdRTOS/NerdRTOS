#include "eth.h"

#include "nd_internal.h"
#include "nd_klibc.h"

#define ETH_IRQ_PRIORITY    (7U)

static ETH_DMADescTypeDef rx_descriptors[ETH_RX_DESC_CNT]
    __attribute__((section(".eth_dma.rx_desc"), aligned(32)));
static ETH_DMADescTypeDef tx_descriptors[ETH_TX_DESC_CNT]
    __attribute__((section(".eth_dma.tx_desc"), aligned(32)));
static uint8_t eth_mac_address[STM32_ETH_MAC_ADDRESS_SIZE];

ETH_HandleTypeDef g_eth_handle;

HAL_StatusTypeDef eth_init(
    const uint8_t mac_address[STM32_ETH_MAC_ADDRESS_SIZE])
{
    if (mac_address == NULL) {
        return HAL_ERROR;
    }

    nd_memset(rx_descriptors, 0, sizeof(rx_descriptors));
    nd_memset(tx_descriptors, 0, sizeof(tx_descriptors));
    nd_memset(&g_eth_handle, 0, sizeof(g_eth_handle));
    nd_memcpy(eth_mac_address, mac_address, sizeof(eth_mac_address));

    g_eth_handle.Instance            = ETH;
    g_eth_handle.Init.MACAddr        = eth_mac_address;
    g_eth_handle.Init.MediaInterface = HAL_ETH_RMII_MODE;
    g_eth_handle.Init.TxDesc         = tx_descriptors;
    g_eth_handle.Init.RxDesc         = rx_descriptors;
    g_eth_handle.Init.RxBuffLen      = STM32_ETH_RX_BUFFER_SIZE;

    return HAL_ETH_Init(&g_eth_handle);
}

HAL_StatusTypeDef eth_configure_link(
    eth_link_speed_t speed,
    eth_link_duplex_t duplex)
{
    ETH_MACConfigTypeDef mac_config;

    if ((speed != STM32_ETH_LINK_SPEED_10M &&
         speed != STM32_ETH_LINK_SPEED_100M) ||
        (duplex != STM32_ETH_LINK_HALF_DUPLEX &&
         duplex != STM32_ETH_LINK_FULL_DUPLEX)) {
        return HAL_ERROR;
    }

    if (HAL_ETH_GetMACConfig(&g_eth_handle, &mac_config) != HAL_OK) {
        return HAL_ERROR;
    }

    mac_config.Speed = speed == STM32_ETH_LINK_SPEED_100M
                           ? ETH_SPEED_100M
                           : ETH_SPEED_10M;
    mac_config.DuplexMode = duplex == STM32_ETH_LINK_FULL_DUPLEX
                                ? ETH_FULLDUPLEX_MODE
                                : ETH_HALFDUPLEX_MODE;

    return HAL_ETH_SetMACConfig(&g_eth_handle, &mac_config);
}

void HAL_ETH_MspInit(ETH_HandleTypeDef *heth)
{
    GPIO_InitTypeDef gpio_init = {0};

    if (heth->Instance != ETH) {
        return;
    }

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();

    __HAL_RCC_ETH1MAC_CLK_ENABLE();
    __HAL_RCC_ETH1TX_CLK_ENABLE();
    __HAL_RCC_ETH1RX_CLK_ENABLE();

    gpio_init.Mode      = GPIO_MODE_AF_PP;
    gpio_init.Pull      = GPIO_NOPULL;
    gpio_init.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init.Alternate = GPIO_AF11_ETH;

    /* PA1: REF_CLK, PA2: MDIO, PA7: CRS_DV. */
    gpio_init.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOA, &gpio_init);

    /* PB13: TXD1. */
    gpio_init.Pin = GPIO_PIN_13;
    HAL_GPIO_Init(GPIOB, &gpio_init);

    /* PC1: MDC, PC4: RXD0, PC5: RXD1. */
    gpio_init.Pin = GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5;
    HAL_GPIO_Init(GPIOC, &gpio_init);

    /* PG11: TX_EN, PG13: TXD0. */
    gpio_init.Pin = GPIO_PIN_11 | GPIO_PIN_13;
    HAL_GPIO_Init(GPIOG, &gpio_init);

    HAL_NVIC_SetPriority(ETH_IRQn, ETH_IRQ_PRIORITY, 0U);
    HAL_NVIC_EnableIRQ(ETH_IRQn);
}

void HAL_ETH_MspDeInit(ETH_HandleTypeDef *heth)
{
    if (heth->Instance != ETH) {
        return;
    }

    HAL_NVIC_DisableIRQ(ETH_IRQn);

    __HAL_RCC_ETH1MAC_CLK_DISABLE();
    __HAL_RCC_ETH1TX_CLK_DISABLE();
    __HAL_RCC_ETH1RX_CLK_DISABLE();

    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_7);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_13);
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5);
    HAL_GPIO_DeInit(GPIOG, GPIO_PIN_11 | GPIO_PIN_13);
}

void ETH_IRQHandler(void)
{
    nd_enter_interrupt();
    HAL_ETH_IRQHandler(&g_eth_handle);
    nd_exit_interrupt();
    nd_try_schedule_irqsave();
}
