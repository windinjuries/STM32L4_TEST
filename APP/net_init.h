#ifndef NET_INIT_H
#define NET_INIT_H

#include "lwip/netif.h"

extern struct netif g_netif;

void net_init(void);
uint8_t net_is_ready(void);
void net_task(void const *argument);

#endif /* NET_INIT_H */
