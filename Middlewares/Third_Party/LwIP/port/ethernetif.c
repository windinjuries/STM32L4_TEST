#include "ethernetif.h"
#include "config.h"
#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/etharp.h"
#include "netif/ethernet.h"
#include "lwip/tcpip.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include <string.h>
#include "debug_log.h"

#ifdef CONFIG_USE_LWIP_PORT_CH395Q
#include "ch395_hw.h"
#include "CH395CMD.H"
#include "CH395INC.H"
#else
#include "w5500_hw.h"
#include "wizchip_conf.h"
#include "socket.h"
#endif

#define IFNAME0                     'e'
#define IFNAME1                     '0'

#define ETH_RX_BUF_SIZE             1536
#define CH395_MACRAW_MIN_FRAME      60U

#ifdef CONFIG_USE_LWIP_PORT_CH395Q
#define CH395_SENDBUF_WAIT_US       10U
#define CH395_SENDBUF_WAIT_RETRY    5000U

static volatile uint8_t s_ch395_sendbuf_ready;

static void ch395_poll_socket_int(void)
{
    uint8_t sint = CH395CMDGetSocketInt(ETH_SOCK);

    if (sint & SINT_STAT_SENDBUF_FREE)
    {
        s_ch395_sendbuf_ready = 1U;
    }
}

static int ch395_wait_sendbuf_free(void)
{
    uint32_t retry = CH395_SENDBUF_WAIT_RETRY;

    if (s_ch395_sendbuf_ready != 0U)
    {
        return 0;
    }

    while (retry-- > 0U)
    {
        ch395_poll_socket_int();
        if (s_ch395_sendbuf_ready != 0U)
        {
            return 0;
        }
        ch395_delay_us(CH395_SENDBUF_WAIT_US);
    }

    LOG_ERROR("CH395: wait sendbuf free timeout");
    return -1;
}
#endif

static struct netif *s_netif;
static SemaphoreHandle_t s_rx_sem;

static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
    struct pbuf *q;
    static uint8_t tx_buf[ETH_RX_BUF_SIZE];
    uint16_t offset = 0;
    uint16_t send_len;

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

    send_len = offset;
#ifdef CONFIG_USE_LWIP_PORT_CH395Q
    if (send_len < CH395_MACRAW_MIN_FRAME)
    {
        memset(&tx_buf[send_len], 0, CH395_MACRAW_MIN_FRAME - send_len);
        send_len = CH395_MACRAW_MIN_FRAME;
    }
    if (ch395_wait_sendbuf_free() != 0)
    {
        return ERR_TIMEOUT;
    }
    CH395CMDSendData(ETH_SOCK, tx_buf, send_len);
    s_ch395_sendbuf_ready = 0U;
    return ERR_OK;
#else
    if (sendto(ETH_SOCK, tx_buf, send_len, NULL, 0) <= 0)
    {
        return ERR_IF;
    }
    return ERR_OK;
#endif
}

static struct pbuf *low_level_input(void)
{
    static uint8_t rx_buf[ETH_RX_BUF_SIZE];
    int32_t len;
    struct pbuf *p;
    struct pbuf *q;
    uint16_t offset = 0;

#ifdef CONFIG_USE_LWIP_PORT_CH395Q
    len = (int32_t)CH395CMDGetRecvLength(ETH_SOCK);
#else
    len = getSn_RX_RSR(ETH_SOCK);
#endif
    if (len <= 0)
    {
        return NULL;
    }

    if (len > (int32_t)ETH_RX_BUF_SIZE)
    {
        len = ETH_RX_BUF_SIZE;
    }

#ifdef CONFIG_USE_LWIP_PORT_CH395Q
    CH395CMDGetRecvData(ETH_SOCK, (uint16_t)len, rx_buf);
#else
    len = recvfrom(ETH_SOCK, rx_buf, (uint16_t)len, NULL, NULL);
    if (len <= 0)
    {
        return NULL;
    }
#endif

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
    uint8_t mac[6];
#ifdef CONFIG_USE_LWIP_PORT_CH395Q
    uint8_t ret;
#endif

    netif->hwaddr_len = ETH_HWADDR_LEN;

#ifdef CONFIG_USE_LWIP_PORT_CH395Q

    LOG_INFO("CH395: init chip...");

    CH395CMDGetMACAddr(mac);
    if ((mac[0] | mac[1] | mac[2] | mac[3] | mac[4] | mac[5]) == 0U)
    {
        LOG_INFO("CH395: generate random MAC address");
        mac[0] = 0x00;
        mac[1] = 0x01;
        mac[2] = 0x02;
        mac[3] = 0x03;
        mac[4] = 0x04;
        mac[5] = 0x05;
        CH395CMDSetMACAddr(mac);
    }
    else 
    {
        LOG_INFO("CH395: MAC: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }

    memcpy(netif->hwaddr, mac, 6);

    ret = CH395CMDInitCH395();
    if (ret != CMD_ERR_SUCCESS)
    {
        LOG_ERROR("CH395: init chip failed, status=0x%02X", ret);
        return;
    }

    ret = CH395CMDDHCPEnable(0);
    if (ret != CMD_ERR_SUCCESS)
    {
        LOG_ERROR("CH395: disable internal DHCP failed, status=0x%02X", ret);
        return;
    }

    CH395CMDSetSocketProtType(ETH_SOCK, PROTO_TYPE_MACRAW);
    ret = CH395CMDOpenSocket(ETH_SOCK);
    if (ret != CMD_ERR_SUCCESS)
    {
        LOG_ERROR("CH395: open MACRAW socket failed, status=0x%02X", ret);
        return;
    }

    s_ch395_sendbuf_ready = 1U;
    LOG_INFO("CH395: MACRAW socket created.");
#else
    uint8_t memsize[2][8] = {{16, 0, 0, 0, 0, 0, 0, 0}, {16, 0, 0, 0, 0, 0, 0, 0}};
    wiz_NetInfo netinfo;

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
#endif

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

#ifdef CONFIG_USE_LWIP_PORT_CH395Q
    while (CH395CMDGetRecvLength(ETH_SOCK) > 0U)
#else
    while (getSn_RX_RSR(ETH_SOCK) > 0)
#endif
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
#ifdef CONFIG_USE_LWIP_PORT_CH395Q
        ch395_poll_socket_int();
#endif
        ethernetif_input(netif);
#ifndef CONFIG_USE_LWIP_PORT_CH395Q
        setSn_IR(ETH_SOCK, getSn_IR(ETH_SOCK));
#endif
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
#ifdef CONFIG_USE_LWIP_PORT_CH395Q
    ch395_hw_int_enable();
#else
    w5500_hw_int_enable();
#endif
}
