#include "net_init.h"
#include "ethernetif.h"
#include "netif/ethernet.h"
#include "tcp_client_demo.h"
#include "FreeRTOS.h"
#include "task.h"
#include "w5500_hw.h"
#include "lwip/init.h"
#include "lwip/tcpip.h"
#include "lwip/dhcp.h"
#include "lwip/ip4_addr.h"
#include "cmsis_os.h"

struct netif g_netif;
TaskHandle_t tcpip_thread_handle;

static volatile uint8_t s_net_ready;

static void netif_status_callback(struct netif *netif)
{
    if (netif_is_up(netif) && !ip4_addr_isany(netif_ip4_addr(netif)))
    {
        LOG_INFO("DHCP ready: IP=%" U16_F ".%" U16_F ".%" U16_F ".%" U16_F,
                 ip4_addr1_16(netif_ip4_addr(netif)),
                 ip4_addr2_16(netif_ip4_addr(netif)),
                 ip4_addr3_16(netif_ip4_addr(netif)),
                 ip4_addr4_16(netif_ip4_addr(netif)));
        s_net_ready = 1U;
    }
}

static void tcpip_init_done(void *arg)
{
    ip4_addr_t ipaddr;
    ip4_addr_t netmask;
    ip4_addr_t gw;

    LWIP_UNUSED_ARG(arg);

    IP4_ADDR(&ipaddr, 0, 0, 0, 0);
    IP4_ADDR(&netmask, 0, 0, 0, 0);
    IP4_ADDR(&gw, 0, 0, 0, 0);

    netif_add(&g_netif, &ipaddr, &netmask, &gw, NULL, ethernetif_init, ethernet_input);
    netif_set_default(&g_netif);
    netif_set_status_callback(&g_netif, netif_status_callback);
    netif_set_up(&g_netif);
    netif_set_link_up(&g_netif);
    LOG_INFO("Starting DHCP client for netif...");
    ethernetif_start_input_task(&g_netif);
    dhcp_start(&g_netif);
}

void net_init(void)
{
    s_net_ready = 0U;
    w5500_hw_init();
    tcpip_init(tcpip_init_done, NULL);
}

uint8_t net_is_ready(void)
{
    return s_net_ready;
}

void net_task(void const *argument)
{
    LWIP_UNUSED_ARG(argument);

    net_init();

    for (;;)
    {
        if (net_is_ready())
        {
            break;
        }
        osDelay(200);
    }

    tcp_client_demo_run();
}
