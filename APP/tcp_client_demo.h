#ifndef TCP_CLIENT_DEMO_H
#define TCP_CLIENT_DEMO_H

/* 修改为目标 TCP 服务器地址（联调前请改成你 PC/服务器的 IP） */
#define TCP_DEMO_SERVER_IP      "192.168.1.100"
#define TCP_DEMO_SERVER_PORT    8080
#define TCP_DEMO_SEND_MSG       "Hello from STM32 W5500\r\n"

void tcp_client_demo_run(void);

#endif /* TCP_CLIENT_DEMO_H */
