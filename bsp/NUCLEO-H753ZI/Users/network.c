#include "network.h"

#include <limits.h>
#include <stdint.h>

#include "eth.h"
#include "ethernetif.h"
#include "lan8742.h"
#include "lwip/ip4_addr.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "nerd.h"
#include "nd_shell.h"
#include "network_perf.h"
#include "phy.h"

#define NETWORK_THREAD_STACK_SIZE       (2048U)
#define NETWORK_THREAD_PRIORITY         (20U)
#define NETWORK_LINK_POLL_INTERVAL      ND_TIMEOUT_MS(500U)

#define NETWORK_IPV4_ADDRESS_A          (192U)
#define NETWORK_IPV4_ADDRESS_B          (168U)
#define NETWORK_IPV4_ADDRESS_C          (137U)
#define NETWORK_IPV4_ADDRESS_D          (2U)

#define NETWORK_IPV4_NETMASK_A          (255U)
#define NETWORK_IPV4_NETMASK_B          (255U)
#define NETWORK_IPV4_NETMASK_C          (255U)
#define NETWORK_IPV4_NETMASK_D          (0U)

#define NETWORK_IPV4_GATEWAY_A          (192U)
#define NETWORK_IPV4_GATEWAY_B          (168U)
#define NETWORK_IPV4_GATEWAY_C          (137U)
#define NETWORK_IPV4_GATEWAY_D          (1U)

static nd_thread_t network_thread;
static nd_uint8_t network_thread_stack[NETWORK_THREAD_STACK_SIZE];
static nd_sem_t lwip_init_done;
static struct netif network_interface;
static err_t network_interface_result;

static const uint8_t network_mac_address[STM32_ETH_MAC_ADDRESS_SIZE] = {
    0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U,
};

static void cmd_eth_stats(int argc, char *argv[])
{
    ethernetif_stats_t stats;

    (void)argc;
    (void)argv;

    ethernetif_get_stats(&stats);
    shell_printf("ETH packets: RX=%u TX=%u\r\n",
                 stats.rx_packets, stats.tx_packets);
    shell_printf("ETH drops: rx_alloc=%u rx_input=%u rx_dma=%u "
                 "tx_wait=%u tx_hal=%u\r\n",
                 stats.rx_alloc_failures,
                 stats.rx_input_failures,
                 stats.rx_dma_errors,
                 stats.tx_buffer_wait_failures,
                 stats.tx_hal_errors);
}

SHELL_CMD(eth_stats, cmd_eth_stats, "Ethernet packet and drop counters");

static const char *network_link_state_name(int32_t state)
{
    switch (state) {
    case LAN8742_STATUS_LINK_DOWN:
        return "down";
    case LAN8742_STATUS_100MBITS_FULLDUPLEX:
        return "100 Mbit/s full-duplex";
    case LAN8742_STATUS_100MBITS_HALFDUPLEX:
        return "100 Mbit/s half-duplex";
    case LAN8742_STATUS_10MBITS_FULLDUPLEX:
        return "10 Mbit/s full-duplex";
    case LAN8742_STATUS_10MBITS_HALFDUPLEX:
        return "10 Mbit/s half-duplex";
    case LAN8742_STATUS_AUTONEGO_NOTDONE:
        return "auto-negotiation in progress";
    case LAN8742_STATUS_READ_ERROR:
        return "MDIO read error";
    default:
        return "unknown";
    }
}

static void network_lwip_init_done(void *argument)
{
    ip4_addr_t ip_address;
    ip4_addr_t netmask;
    ip4_addr_t gateway;

    (void)argument;

    IP4_ADDR(&ip_address,
             NETWORK_IPV4_ADDRESS_A, NETWORK_IPV4_ADDRESS_B,
             NETWORK_IPV4_ADDRESS_C, NETWORK_IPV4_ADDRESS_D);
    IP4_ADDR(&netmask,
             NETWORK_IPV4_NETMASK_A, NETWORK_IPV4_NETMASK_B,
             NETWORK_IPV4_NETMASK_C, NETWORK_IPV4_NETMASK_D);
    IP4_ADDR(&gateway,
             NETWORK_IPV4_GATEWAY_A, NETWORK_IPV4_GATEWAY_B,
             NETWORK_IPV4_GATEWAY_C, NETWORK_IPV4_GATEWAY_D);

    if (netif_add(&network_interface,
                  &ip_address, &netmask, &gateway,
                  NULL, ethernetif_init, tcpip_input) == NULL) {
        network_interface_result = ERR_IF;
    } else {
        netif_set_default(&network_interface);
        netif_set_up(&network_interface);
        network_interface_result = ERR_OK;
    }

    (void)nd_sem_release(&lwip_init_done);
}

static void network_thread_entry(void *parameter)
{
    int32_t configured_state = INT32_MIN;
    int32_t state;

    (void)parameter;

    if (eth_init(network_mac_address) != HAL_OK) {
        shell_printf("[eth] MAC/DMA initialization failed\r\n");
        return;
    }
    shell_printf("[eth] MAC/DMA initialized\r\n");

    state = phy_init();
    if (state != LAN8742_STATUS_OK) {
        shell_printf("[eth] PHY initialization failed: %d\r\n", (int)state);
        return;
    }
    shell_printf("[eth] PHY initialized; auto-negotiation started\r\n");

    nd_sem_init(&lwip_init_done, 0);
    network_interface_result = ERR_IF;
    tcpip_init(network_lwip_init_done, NULL);
    (void)nd_sem_take(&lwip_init_done, ND_TIMEOUT_FOREVER);
    if (network_interface_result != ERR_OK) {
        shell_printf("[lwip] network interface initialization failed\r\n");
        return;
    }

    shell_printf("[lwip] IPv4: 192.168.137.2/24, gateway: 192.168.137.1\r\n");

    if (network_perf_init() != ND_EOK) {
        shell_printf("[netperf] thread creation failed\r\n");
    }

    for (;;) {
        state = phy_get_link_state();
        if (state != configured_state) {
            if (ethernetif_update_link(&network_interface,
                                       state) != ERR_OK) {
                shell_printf("[eth] link configuration failed: %d\r\n",
                             (int)state);
            } else {
                shell_printf("[eth] link: %s (%d)\r\n",
                             network_link_state_name(state), (int)state);
                configured_state = state;
            }
        }

        nd_thread_delay(NETWORK_LINK_POLL_INTERVAL);
    }
}

nd_err_t network_init(void)
{
    return nd_thread_create(&network_thread, "network",
                            network_thread_entry,
                            NETWORK_THREAD_PRIORITY, NULL,
                            network_thread_stack,
                            sizeof(network_thread_stack),
                            ND_THREAD_OPT_NONE, 0U);
}
