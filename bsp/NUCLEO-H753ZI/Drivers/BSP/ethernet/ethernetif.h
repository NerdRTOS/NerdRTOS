#ifndef __ETHERNETIF_H
#define __ETHERNETIF_H

#include <stdint.h>

#include "lwip/err.h"
#include "lwip/netif.h"

typedef struct {
    uint32_t rx_packets;
    uint32_t tx_packets;
    uint32_t rx_alloc_failures;
    uint32_t rx_input_failures;
    uint32_t rx_dma_errors;
    uint32_t tx_buffer_wait_failures;
    uint32_t tx_hal_errors;
} ethernetif_stats_t;

err_t ethernetif_init(struct netif *netif);
err_t ethernetif_update_link(struct netif *netif,
                             int32_t phy_link_state);
void ethernetif_get_stats(ethernetif_stats_t *stats);

#endif /* __ETHERNETIF_H */
