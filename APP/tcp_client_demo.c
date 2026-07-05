#include "tcp_client_demo.h"
#include "net_init.h"
#include "lwip/sockets.h"
#include "tcp_client_demo.h"
#include "lwip/inet.h"
#include "cmsis_os.h"
#include <string.h>

void tcp_client_demo_run(void)
{
    int sock;
    struct sockaddr_in server_addr;
    char rx_buf[128];
    int ret;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        return;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = PP_HTONS(TCP_DEMO_SERVER_PORT);
    inet_aton(TCP_DEMO_SERVER_IP, &server_addr.sin_addr);

    for (;;)
    {
        ret = connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (ret == 0)
        {
            break;
        }
        osDelay(2000);
    }

    send(sock, TCP_DEMO_SEND_MSG, (size_t)strlen(TCP_DEMO_SEND_MSG), 0);

    ret = recv(sock, rx_buf, sizeof(rx_buf) - 1U, 0);
    if (ret > 0)
    {
        rx_buf[ret] = '\0';
    }

    closesocket(sock);

    for (;;)
    {
        osDelay(1000);
    }
}
