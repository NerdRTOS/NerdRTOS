#include "ethernetif.h"

#include <stddef.h>

#include "eth.h"
#include "lan8742.h"
#include "nerd.h"
#include "nd_klibc.h"
#include "lwip/etharp.h"
#include "lwip/pbuf.h"
#include "lwip/tcpip.h"
#include "netif/ethernet.h"

#define STM32_ETH_RX_POOL_SIZE         (16U)
#define STM32_ETH_RX_THREAD_STACK_SIZE (2048U)
#define STM32_ETH_RX_THREAD_PRIORITY   (10U)
#define STM32_ETH_TX_POOL_SIZE         (ETH_TX_DESC_CNT)
#define STM32_ETH_TX_WAIT_TIMEOUT      ND_TIMEOUT_MS(100U)
#define ETH_BUFFER_INDEX_NONE          (UINT8_MAX)

typedef struct {
    struct pbuf_custom custom_pbuf;
    nd_uint8_t         in_use;
    nd_uint8_t         next_free;
    nd_uint8_t         payload[STM32_ETH_RX_BUFFER_SIZE]
        __attribute__((aligned(STM32_ETH_DMA_ALIGNMENT)));
} eth_rx_buffer_t;

typedef struct {
    nd_uint8_t in_use;
    nd_uint8_t next_free;
    nd_uint8_t payload[STM32_ETH_RX_BUFFER_SIZE]
        __attribute__((aligned(STM32_ETH_DMA_ALIGNMENT)));
} eth_tx_buffer_t;

static eth_rx_buffer_t rx_pool[STM32_ETH_RX_POOL_SIZE]
    __attribute__((section(".eth_dma.rx_pool"),
                   aligned(STM32_ETH_DMA_ALIGNMENT)));
static eth_tx_buffer_t tx_pool[STM32_ETH_TX_POOL_SIZE]
    __attribute__((section(".eth_dma.tx_pool"),
                   aligned(STM32_ETH_DMA_ALIGNMENT)));

static nd_sem_t                  rx_event;
static nd_sem_t                  tx_available;
static nd_thread_t               rx_thread;
static nd_uint8_t                rx_thread_stack[STM32_ETH_RX_THREAD_STACK_SIZE];
static struct netif              *active_netif;
static nd_uint8_t                ethernetif_initialized;
static nd_uint8_t                rx_free_head;
static nd_uint8_t                tx_free_head;
static ethernetif_stats_t        ethernetif_stats;

static err_t ethernetif_output(struct netif *netif, struct pbuf *p);
static void ethernetif_rx_thread(void *parameter);

static void ethernetif_increment_stat(uint32_t *stat)
{
    sys_prot_t protection;

    protection = sys_arch_protect();
    (*stat)++;
    sys_arch_unprotect(protection);
}

static void ethernetif_init_buffer_pools(void)
{
    nd_uint32_t index;

    nd_memset(rx_pool, 0, sizeof(rx_pool));
    for (index = 0U; index < STM32_ETH_RX_POOL_SIZE; index++) {
        rx_pool[index].next_free =
            index + 1U < STM32_ETH_RX_POOL_SIZE
                ? (nd_uint8_t)(index + 1U)
                : ETH_BUFFER_INDEX_NONE;
    }
    rx_free_head = 0U;

    nd_memset(tx_pool, 0, sizeof(tx_pool));
    for (index = 0U; index < STM32_ETH_TX_POOL_SIZE; index++) {
        tx_pool[index].next_free =
            index + 1U < STM32_ETH_TX_POOL_SIZE
                ? (nd_uint8_t)(index + 1U)
                : ETH_BUFFER_INDEX_NONE;
    }
    tx_free_head = 0U;
}

static nd_uint8_t ethernetif_release_rx_pool_buffer(
    eth_rx_buffer_t *rx_buffer)
{
    sys_prot_t protection;
    nd_uint8_t released = 0U;
    nd_uint8_t index;

    protection = sys_arch_protect();
    if (rx_buffer->in_use != 0U) {
        index = (nd_uint8_t)(rx_buffer - rx_pool);
        rx_buffer->in_use = 0U;
        rx_buffer->next_free = rx_free_head;
        rx_free_head = index;
        released = 1U;
    }
    sys_arch_unprotect(protection);

    return released;
}

static void ethernetif_release_rx_buffer(struct pbuf *p)
{
    if (ethernetif_release_rx_pool_buffer((eth_rx_buffer_t *)p) == 0U) {
        return;
    }

    (void)nd_sem_release(&rx_event);
}

static eth_rx_buffer_t *ethernetif_acquire_rx_buffer(void)
{
    eth_rx_buffer_t *rx_buffer = NULL;
    sys_prot_t protection;
    nd_uint8_t index;

    protection = sys_arch_protect();
    index = rx_free_head;
    if (index != ETH_BUFFER_INDEX_NONE) {
        rx_buffer = &rx_pool[index];
        rx_free_head = rx_buffer->next_free;
        rx_buffer->next_free = ETH_BUFFER_INDEX_NONE;
        rx_buffer->in_use = 1U;
    }
    sys_arch_unprotect(protection);

    return rx_buffer;
}

static eth_tx_buffer_t *ethernetif_acquire_tx_buffer(void)
{
    eth_tx_buffer_t *tx_buffer = NULL;
    sys_prot_t protection;
    nd_uint8_t index;

    if (nd_sem_take(&tx_available, STM32_ETH_TX_WAIT_TIMEOUT) != ND_EOK) {
        ethernetif_increment_stat(
            &ethernetif_stats.tx_buffer_wait_failures);
        return NULL;
    }

    protection = sys_arch_protect();
    index = tx_free_head;
    if (index != ETH_BUFFER_INDEX_NONE) {
        tx_buffer = &tx_pool[index];
        tx_free_head = tx_buffer->next_free;
        tx_buffer->next_free = ETH_BUFFER_INDEX_NONE;
        tx_buffer->in_use = 1U;
    }
    sys_arch_unprotect(protection);

    if (tx_buffer == NULL) {
        (void)nd_sem_release(&tx_available);
        ethernetif_increment_stat(
            &ethernetif_stats.tx_buffer_wait_failures);
    }

    return tx_buffer;
}

static nd_uint8_t ethernetif_release_tx_buffer(eth_tx_buffer_t *tx_buffer)
{
    sys_prot_t protection;
    nd_uint8_t released = 0U;
    nd_uint8_t index;

    protection = sys_arch_protect();
    if (tx_buffer->in_use != 0U) {
        index = (nd_uint8_t)(tx_buffer - tx_pool);
        tx_buffer->in_use = 0U;
        tx_buffer->next_free = tx_free_head;
        tx_free_head = index;
        released = 1U;
    }
    sys_arch_unprotect(protection);

    if (released != 0U) {
        (void)nd_sem_release(&tx_available);
    }

    return released;
}

void HAL_ETH_RxAllocateCallback(uint8_t **buffer)
{
    eth_rx_buffer_t *rx_buffer;

    if (buffer == NULL) {
        return;
    }

    *buffer = NULL;
    rx_buffer = ethernetif_acquire_rx_buffer();
    if (rx_buffer == NULL) {
        ethernetif_increment_stat(
            &ethernetif_stats.rx_alloc_failures);
        return;
    }

    rx_buffer->custom_pbuf.custom_free_function =
        ethernetif_release_rx_buffer;
    if (pbuf_alloced_custom(PBUF_RAW, 0U, PBUF_REF,
                            &rx_buffer->custom_pbuf,
                            rx_buffer->payload,
                            sizeof(rx_buffer->payload)) == NULL) {
        (void)ethernetif_release_rx_pool_buffer(rx_buffer);
        ethernetif_increment_stat(
            &ethernetif_stats.rx_alloc_failures);
        return;
    }

    *buffer = rx_buffer->payload;
}

void HAL_ETH_RxLinkCallback(void **start, void **end,
                            uint8_t *buffer, uint16_t length)
{
    eth_rx_buffer_t *rx_buffer;
    struct pbuf *p;

    if (start == NULL || end == NULL || buffer == NULL) {
        return;
    }

    rx_buffer = (eth_rx_buffer_t *)(
        buffer - offsetof(eth_rx_buffer_t, payload));
    p = &rx_buffer->custom_pbuf.pbuf;
    p->next = NULL;
    p->len = length;
    p->tot_len = length;

    if (*start == NULL) {
        *start = p;
    } else {
        pbuf_cat((struct pbuf *)*start, p);
    }
    *end = p;
}

void HAL_ETH_RxCpltCallback(ETH_HandleTypeDef *heth)
{
    if (heth == &g_eth_handle) {
        (void)nd_sem_release(&rx_event);
    }
}

void HAL_ETH_TxCpltCallback(ETH_HandleTypeDef *heth)
{
    if (heth == &g_eth_handle && HAL_ETH_ReleaseTxPacket(heth) != HAL_OK) {
        ethernetif_increment_stat(&ethernetif_stats.tx_hal_errors);
    }
}

void HAL_ETH_TxFreeCallback(uint32_t *buffer)
{
    if (ethernetif_release_tx_buffer((eth_tx_buffer_t *)buffer) != 0U) {
        ethernetif_increment_stat(&ethernetif_stats.tx_packets);
    }
}

void HAL_ETH_ErrorCallback(ETH_HandleTypeDef *heth)
{
    if (heth == &g_eth_handle) {
        (void)nd_sem_release(&rx_event);
    }
}

static err_t ethernetif_output(struct netif *netif, struct pbuf *p)
{
    ETH_BufferTypeDef tx_buffer_descriptor;
    ETH_TxPacketConfigTypeDef tx_config;
    eth_tx_buffer_t *tx_buffer;

    (void)netif;

    if (p == NULL || p->tot_len > STM32_ETH_RX_BUFFER_SIZE) {
        return ERR_IF;
    }

    tx_buffer = ethernetif_acquire_tx_buffer();
    if (tx_buffer == NULL) {
        return ERR_MEM;
    }

    if (pbuf_copy_partial(p, tx_buffer->payload,
                          p->tot_len, 0U) != p->tot_len) {
        (void)ethernetif_release_tx_buffer(tx_buffer);
        return ERR_IF;
    }

    nd_memset(&tx_buffer_descriptor, 0, sizeof(tx_buffer_descriptor));
    nd_memset(&tx_config, 0, sizeof(tx_config));

    tx_buffer_descriptor.buffer = tx_buffer->payload;
    tx_buffer_descriptor.len = p->tot_len;
    tx_config.Attributes = ETH_TX_PACKETS_FEATURES_CRCPAD |
                           ETH_TX_PACKETS_FEATURES_CSUM;
    tx_config.Length = p->tot_len;
    tx_config.TxBuffer = &tx_buffer_descriptor;
    tx_config.CRCPadCtrl = ETH_CRC_PAD_INSERT;
    tx_config.ChecksumCtrl = ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC;
    tx_config.pData = tx_buffer;

    if (HAL_ETH_Transmit_IT(&g_eth_handle, &tx_config) != HAL_OK) {
        (void)ethernetif_release_tx_buffer(tx_buffer);
        ethernetif_increment_stat(&ethernetif_stats.tx_hal_errors);
        return ERR_IF;
    }

    return ERR_OK;
}

static void ethernetif_rx_thread(void *parameter)
{
    struct pbuf *p;
    uint32_t error_code;

    (void)parameter;

    for (;;) {
        (void)nd_sem_take(&rx_event, ND_TIMEOUT_FOREVER);

        while (HAL_ETH_ReadData(&g_eth_handle, (void **)&p) == HAL_OK) {
            error_code = 0U;
            if (HAL_ETH_GetRxDataErrorCode(&g_eth_handle,
                                           &error_code) != HAL_OK ||
                error_code != HAL_ETH_ERROR_NONE) {
                ethernetif_increment_stat(
                    &ethernetif_stats.rx_dma_errors);
                pbuf_free(p);
                continue;
            }

            if (p == NULL || active_netif == NULL ||
                active_netif->input(p, active_netif) != ERR_OK) {
                ethernetif_increment_stat(
                    &ethernetif_stats.rx_input_failures);
                if (p != NULL) {
                    pbuf_free(p);
                }
                continue;
            }

            ethernetif_increment_stat(&ethernetif_stats.rx_packets);
        }
    }
}

err_t ethernetif_init(struct netif *netif)
{
    if (netif == NULL || g_eth_handle.Init.MACAddr == NULL) {
        return ERR_ARG;
    }

    if (ethernetif_initialized == 0U) {
        ethernetif_init_buffer_pools();
        nd_memset(&ethernetif_stats, 0, sizeof(ethernetif_stats));
        nd_sem_init(&rx_event, 0);
        nd_sem_init(&tx_available, STM32_ETH_TX_POOL_SIZE);
        active_netif = netif;

        if (nd_thread_create(&rx_thread, "eth_rx",
                             ethernetif_rx_thread,
                             STM32_ETH_RX_THREAD_PRIORITY, NULL,
                             rx_thread_stack, sizeof(rx_thread_stack),
                             ND_THREAD_OPT_NONE, 0U) != ND_EOK) {
            active_netif = NULL;
            return ERR_IF;
        }
        ethernetif_initialized = 1U;
    } else if (active_netif != netif) {
        return ERR_IF;
    }

    netif->name[0] = 'e';
    netif->name[1] = 'n';
    netif->hwaddr_len = STM32_ETH_MAC_ADDRESS_SIZE;
    nd_memcpy(netif->hwaddr, g_eth_handle.Init.MACAddr,
              STM32_ETH_MAC_ADDRESS_SIZE);
    netif->mtu = ETH_MAX_PAYLOAD;
    netif->flags = NETIF_FLAG_BROADCAST |
                   NETIF_FLAG_ETHARP |
                   NETIF_FLAG_ETHERNET;
    netif->output = etharp_output;
    netif->linkoutput = ethernetif_output;

    return ERR_OK;
}

void ethernetif_get_stats(ethernetif_stats_t *stats)
{
    sys_prot_t protection;

    if (stats == NULL) {
        return;
    }

    protection = sys_arch_protect();
    *stats = ethernetif_stats;
    sys_arch_unprotect(protection);
}

err_t ethernetif_update_link(struct netif *netif,
                             int32_t phy_link_state)
{
    eth_link_speed_t speed;
    eth_link_duplex_t duplex;

    if (netif == NULL) {
        return ERR_ARG;
    }

    switch (phy_link_state) {
    case LAN8742_STATUS_100MBITS_FULLDUPLEX:
        speed = STM32_ETH_LINK_SPEED_100M;
        duplex = STM32_ETH_LINK_FULL_DUPLEX;
        break;
    case LAN8742_STATUS_100MBITS_HALFDUPLEX:
        speed = STM32_ETH_LINK_SPEED_100M;
        duplex = STM32_ETH_LINK_HALF_DUPLEX;
        break;
    case LAN8742_STATUS_10MBITS_FULLDUPLEX:
        speed = STM32_ETH_LINK_SPEED_10M;
        duplex = STM32_ETH_LINK_FULL_DUPLEX;
        break;
    case LAN8742_STATUS_10MBITS_HALFDUPLEX:
        speed = STM32_ETH_LINK_SPEED_10M;
        duplex = STM32_ETH_LINK_HALF_DUPLEX;
        break;
    case LAN8742_STATUS_LINK_DOWN:
    case LAN8742_STATUS_AUTONEGO_NOTDONE:
        LOCK_TCPIP_CORE();
        netif_set_link_down(netif);
        UNLOCK_TCPIP_CORE();

        if (g_eth_handle.gState == HAL_ETH_STATE_STARTED &&
            HAL_ETH_Stop_IT(&g_eth_handle) != HAL_OK) {
            return ERR_IF;
        }
        return ERR_OK;
    default:
        return ERR_IF;
    }

    if (g_eth_handle.gState == HAL_ETH_STATE_STARTED &&
        HAL_ETH_Stop_IT(&g_eth_handle) != HAL_OK) {
        return ERR_IF;
    }
    if (eth_configure_link(speed, duplex) != HAL_OK ||
        HAL_ETH_Start_IT(&g_eth_handle) != HAL_OK) {
        return ERR_IF;
    }

    LOCK_TCPIP_CORE();
    netif_set_link_up(netif);
    UNLOCK_TCPIP_CORE();

    return ERR_OK;
}
