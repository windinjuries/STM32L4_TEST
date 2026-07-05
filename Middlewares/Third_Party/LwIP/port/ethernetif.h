#ifndef ETHERNETIF_H
#define ETHERNETIF_H

#include "lwip/netif.h"

#define ETH_SOCK                    0

#include "lwip/netif.h"

#define ETH_SOCK                    0

err_t ethernetif_init(struct netif *netif);
void ethernetif_input(struct netif *netif);
void ethernetif_notify_rx(void);
void ethernetif_start_input_task(struct netif *netif);

#endif /* ETHERNETIF_H */
