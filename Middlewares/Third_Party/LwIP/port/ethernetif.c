#include "ethernetif.h"
#include "w5500_hw.h"
#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/etharp.h"
#include "netif/ethernet.h"
#include "lwip/tcpip.h"
#include "wizchip_conf.h"
#include "socket.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include <string.h>
#include "debug_log.h"

#define IFNAME0                     'e'
#define IFNAME1                     '0'

#define ETH_RX_BUF_SIZE             1536

static struct netif *s_netif;
static SemaphoreHandle_t s_rx_sem;

static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
    struct pbuf *q;
    static uint8_t tx_buf[ETH_RX_BUF_SIZE];
    uint16_t offset = 0;
    int32_t ret;

    LWIP_UNUSED_ARG(netif);

    for (q = p; q != NULL; q = q->next)
    {
        if ((offset + q->len) > ETH_RX_BUF_SIZE)
        {
            return ERR_BUF;
        }
        memcpy(&tx_buf[offset], q->payload, q->len);
        offset = (uint16_t)(offset + q->len);
    }

    ret = sendto(ETH_SOCK, tx_buf, offset, NULL, 0);
    if (ret <= 0)
    {
        return ERR_IF;
    }

    return ERR_OK;
}

static struct pbuf *low_level_input(void)
{
    static uint8_t rx_buf[ETH_RX_BUF_SIZE];
    int32_t len;
    struct pbuf *p;
    struct pbuf *q;
    uint16_t offset = 0;

    len = getSn_RX_RSR(ETH_SOCK);
    if (len <= 0)
    {
        return NULL;
    }

    if (len > (int32_t)ETH_RX_BUF_SIZE)
    {
        len = ETH_RX_BUF_SIZE;
    }

    len = recvfrom(ETH_SOCK, rx_buf, (uint16_t)len, NULL, NULL);
    if (len <= 0)
    {
        return NULL;
    }

    p = pbuf_alloc(PBUF_RAW, (u16_t)len, PBUF_POOL);
    if (p == NULL)
    {
        return NULL;
    }

    for (q = p; q != NULL; q = q->next)
    {
        memcpy(q->payload, &rx_buf[offset], q->len);
        offset = (uint16_t)(offset + q->len);
    }

    return p;
}

static void low_level_init(struct netif *netif)
{
    uint8_t memsize[2][8] = {{16, 0, 0, 0, 0, 0, 0, 0}, {16, 0, 0, 0, 0, 0, 0, 0}};
    uint8_t mac[6];
    wiz_NetInfo netinfo;

    netif->hwaddr_len = ETH_HWADDR_LEN;
    LOG_INFO("W5500: init chip...");
    ctlwizchip(CW_INIT_WIZCHIP, (void *)memsize);
    LOG_INFO("W5500: chip init done");

    ctlnetwork(CN_GET_NETINFO, (void *)&netinfo);
    memcpy(mac, netinfo.mac, 6);
    if ((mac[0] | mac[1] | mac[2] | mac[3] | mac[4] | mac[5]) == 0U)
    {
        mac[0] = 0x02;
        mac[1] = 0x00;
        mac[2] = 0x00;
        mac[3] = 0x12;
        mac[4] = 0x34;
        mac[5] = 0x56;
        memcpy(netinfo.mac, mac, 6);
        ctlnetwork(CN_SET_NETINFO, (void *)&netinfo);
    }
    memcpy(netif->hwaddr, mac, 6);

    setSn_IMR(ETH_SOCK, Sn_IR_RECV);
    setSIMR(0x01U);

    LOG_INFO("W5500: create MACRAW socket...");
    if (socket(ETH_SOCK, Sn_MR_MACRAW, 0, 0) != ETH_SOCK)
    {
        LOG_ERROR("W5500: create MACRAW socket failed.");
        return;
    }
    LOG_INFO("W5500: MACRAW socket created.");

    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_LINK_UP;
}

err_t ethernetif_init(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));

    netif->name[0] = IFNAME0;
    netif->name[1] = IFNAME1;
    netif->output = etharp_output;
    netif->linkoutput = low_level_output;

    low_level_init(netif);
    return ERR_OK;
}

void ethernetif_input(struct netif *netif)
{
    struct pbuf *p;

    while (getSn_RX_RSR(ETH_SOCK) > 0)
    {
        p = low_level_input();
        if (p == NULL)
        {
            break;
        }

        if (netif->input(p, netif) != ERR_OK)
        {
            pbuf_free(p);
        }
    }
}

void ethernetif_notify_rx(void)
{
    BaseType_t woken = pdFALSE;

    if (s_rx_sem != NULL)
    {
        xSemaphoreGiveFromISR(s_rx_sem, &woken);
        portYIELD_FROM_ISR(woken);
    }
}

static void ethernetif_thread(void *arg)
{
    struct netif *netif = (struct netif *)arg;
    LOG_INFO("Ethernet input task started.");

    for (;;)
    {
        if (xSemaphoreTake(s_rx_sem, pdMS_TO_TICKS(10U)) == pdTRUE)
        {
            /* An RX interrupt arrived, process immediately. */
        }

        LOCK_TCPIP_CORE();
        ethernetif_input(netif);
        setSn_IR(ETH_SOCK, getSn_IR(ETH_SOCK));
        UNLOCK_TCPIP_CORE();
    }
}

void ethernetif_start_input_task(struct netif *netif)
{
    BaseType_t ret;

    s_netif = netif;

    if (s_rx_sem == NULL)
    {
        s_rx_sem = xSemaphoreCreateBinary();
        if (s_rx_sem == NULL)
        {
            LOG_ERROR("Create ethernet RX semaphore failed.");
            return;
        }
    }

    LOG_INFO("Creating ethernet input task...");
    ret = xTaskCreate(ethernetif_thread,
                      "ethif",
                      512,
                      netif,
                      TCPIP_THREAD_PRIO - 1,
                      NULL);
    if (ret != pdPASS)
    {
        LOG_ERROR("Create ethernet input task failed.");
        return;
    }

    LOG_INFO("Ethernet input task created.");
    w5500_hw_int_enable();
}
