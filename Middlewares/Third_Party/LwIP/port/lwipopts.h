#ifndef LWIP_LWIPOPTS_H
#define LWIP_LWIPOPTS_H

#define LWIP_IPV4                  1
#define LWIP_IPV6                  0

#define NO_SYS                     0
#define LWIP_SOCKET                1
#define LWIP_NETCONN               1
#define LWIP_NETIF_API             1

#define LWIP_IGMP                  0
#define LWIP_ICMP                  1
#define LWIP_DNS                   1
#define LWIP_DHCP                  1
#define LWIP_ARP                   1

#define LWIP_HAVE_LOOPIF           0
#define LWIP_NETIF_LOOPBACK        0

#define TCP_LISTEN_BACKLOG         1
#define LWIP_COMPAT_SOCKETS        1
#define LWIP_SO_RCVTIMEO           1

#define LWIP_TCPIP_CORE_LOCKING    1
#define LWIP_NETIF_LINK_CALLBACK   1
#define LWIP_NETIF_STATUS_CALLBACK 1

#define MEM_ALIGNMENT              4U
#define MEM_SIZE                   (8 * 1024)

#define MEMP_NUM_PBUF              8
#define MEMP_NUM_RAW_PCB           2
#define MEMP_NUM_UDP_PCB           4
#define MEMP_NUM_TCP_PCB           4
#define MEMP_NUM_TCP_PCB_LISTEN    4
#define MEMP_NUM_TCP_SEG           16
#define MEMP_NUM_SYS_TIMEOUT       12
#define MEMP_NUM_NETBUF            2
#define MEMP_NUM_NETCONN           4
#define MEMP_NUM_TCPIP_MSG_API     8
#define MEMP_NUM_TCPIP_MSG_INPKT   8

#define PBUF_POOL_SIZE             8
#define PBUF_POOL_BUFSIZE          1536

#define SYS_LIGHTWEIGHT_PROT       1

#define LWIP_TCP                   1
#define TCP_TTL                    255
#define TCP_MSS                    1460
#define TCP_SND_BUF                (2 * TCP_MSS)
#define TCP_WND                    (2 * TCP_MSS)

#define LWIP_STATS                 0
#define LWIP_DEBUG                 1
#define LWIP_DBG_TYPES_ON          (LWIP_DBG_ON|LWIP_DBG_TRACE|LWIP_DBG_STATE)
#define LWIP_DBG_MIN_LEVEL         LWIP_DBG_LEVEL_ALL
#define DHCP_DEBUG                 (LWIP_DBG_ON|LWIP_DBG_TRACE|LWIP_DBG_STATE)
#define ETHARP_DEBUG               (LWIP_DBG_ON|LWIP_DBG_TRACE|LWIP_DBG_STATE)
#define UDP_DEBUG                  (LWIP_DBG_ON|LWIP_DBG_TRACE|LWIP_DBG_STATE)
#define IP_DEBUG                   (LWIP_DBG_ON|LWIP_DBG_TRACE|LWIP_DBG_STATE)
#define NETIF_DEBUG                (LWIP_DBG_ON|LWIP_DBG_TRACE|LWIP_DBG_STATE)
#define TCPIP_DEBUG                (LWIP_DBG_ON|LWIP_DBG_TRACE|LWIP_DBG_STATE)

#define TCPIP_THREAD_NAME          "tcpip"
#define TCPIP_THREAD_STACKSIZE     1024
#define TCPIP_THREAD_PRIO          (configMAX_PRIORITIES - 2)
#define TCPIP_MBOX_SIZE            8
#define DEFAULT_RAW_RECVMBOX_SIZE  4
#define DEFAULT_UDP_RECVMBOX_SIZE  4
#define DEFAULT_TCP_RECVMBOX_SIZE  4
#define DEFAULT_ACCEPTMBOX_SIZE    4

#define LWIP_FREERTOS_THREAD_STACKSIZE_IS_STACKWORDS  1

/* Provide a basic random function for LWIP if not supplied by the port.
	This maps `LWIP_RAND()` to the C library `rand()` so components that
	expect LWIP_RAND compile correctly. If your platform provides a
	better RNG, override `LWIP_RAND` in a custom lwipopts.h instead. */
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#ifndef LWIP_RAND
#define LWIP_RAND() ((uint32_t)rand())
#endif

#include "FreeRTOS.h"
#include "task.h"
#include "FreeRTOSConfig.h"

#define LWIP_MARK_TCPIP_THREAD()   tcpip_thread_handle = xTaskGetCurrentTaskHandle()
#define LWIP_ASSERT_CORE_LOCKED()
extern TaskHandle_t tcpip_thread_handle;

#endif /* LWIP_LWIPOPTS_H */
