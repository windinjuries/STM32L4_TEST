#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define ESP8266_EOK          0U
#define ESP8266_ERROR        -1U

#define esp8266_UART_TX_BUF_SIZE  1024                      /* ATK-MW8266 UART transmit buffer size */
#define esp8266_UART_RX_BUF_SIZE  1024                      /* ATK-MW8266 UART receive buffer size */

#define ESP8266_RESET_LOW  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET)   
#define ESP8266_RESET_HIGH HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET) 

extern UART_HandleTypeDef huart2;                           /* ATK-MW8266 UART */
static UART_HandleTypeDef *g_uart_handle = &huart2;         /* ATK-MW8266 UART */

static struct
{
    uint8_t buf[esp8266_UART_RX_BUF_SIZE];         /* Frame receive buffer */
    uint16_t finsh;                                /* Frame receive completion flag */
    uint16_t len;                                  /* Frame receive length */
} g_uart_rx_frame = {0};                           /* ATK-MW8266 UART receive frame buffer structure */

static uint8_t g_uart_tx_buf[esp8266_UART_TX_BUF_SIZE];

static void esp8266_uart_printf(char *fmt, ...)
{
    va_list ap;
    uint16_t len;
    va_start(ap, fmt);
    vsprintf((char *)g_uart_tx_buf, fmt, ap);
    va_end(ap);
    len = strlen((const char *)g_uart_tx_buf);
    HAL_UART_Transmit(g_uart_handle, g_uart_tx_buf, len, HAL_MAX_DELAY);
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART2)
    {
        g_uart_rx_frame.len     = Size;
        g_uart_rx_frame.finsh   = 1;
    }
}

/**
 * @brief       Restart receiving data on ATK-MW8266 UART
 * @param       None
 * @retval      None
 */
void esp8266_uart_rx_restart(void)
{
    g_uart_rx_frame.len     = 0;
    g_uart_rx_frame.finsh   = 0;
    HAL_UARTEx_ReceiveToIdle_DMA(&huart2, uartBuf, UART_BUF_LEN);
    __HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT);
}

/**
 * @brief       Get a frame of data received by ATK-MW8266 UART
 * @param       None
 * @retval      NULL: No frame received
 *              Others: Received frame data
 */
uint8_t *esp8266_uart_rx_get_frame(void)
{
    if (g_uart_rx_frame.finsh == 1)
    {
        g_uart_rx_frame.buf[g_uart_rx_frame.len] = '\0';
        return g_uart_rx_frame.buf;
    }
    else
    {
        return NULL;
    }
}

/**
 * @brief       Get the length of a frame of data received by ATK-MW8266 UART
 * @param       None
 * @retval      0   : No frame received
 *              Others: Length of received frame
 */
uint16_t esp8266_uart_rx_get_frame_len(void)
{
    if (g_uart_rx_frame.sta.finsh == 1)
    {
        return g_uart_rx_frame.sta.len;
    }
    else
    {
        return 0;
    }
}

void esp8266_uart_init(uint32_t baudrate)
{

}

static void esp8266_hw_init(void)
{

}

/**
 * @brief       Hardware reset for ATK-MW8266
 * @param       None
 * @retval      None
 */
void esp8266_hw_reset(void)
{
    ESP8266_RESET_LOW();
    delay_ms(100);
    ESP8266_RESET_HIGH();
    delay_ms(500);
}

uint8_t esp8266_send_at_cmd(char *cmd, char *ack, uint32_t timeout)
{
    uint8_t *ret = NULL;
    esp8266_uart_rx_restart();
    esp8266_uart_printf("%s\r\n", cmd);
    
    if ((ack == NULL) || (timeout == 0))
    {
        return ESP8266_EOK;
    }
    else
    {
        while (timeout > 0)
        {
            ret = esp8266_uart_rx_get_frame();
            if (ret != NULL)
            {
                if (strstr((const char *)ret, ack) != NULL)
                {
                    return ESP8266_EOK;
                }
                else
                {
                    esp8266_uart_rx_restart();
                }
            }
            timeout--;
            delay_ms(1);
        }
        
        return esp8266_ETIMEOUT;
    }
}

uint8_t esp8266_init()
{
    esp8266_hw_init();                          /* ATK-MW8266 hardware initialization */
    esp8266_hw_reset();                         /* ATK-MW8266 hardware reset */
    esp8266_uart_init();                        /* ATK-MW8266 UART initialization */
    if (esp8266_at_test() != ESP8266_EOK)   /* ATK-MW8266 AT command test */
    {
        return ESP8266_ERROR;
    }
    
    return ESP8266_EOK;
}

/**
 * @brief       Restore ATK-MW8266 to factory settings
 * @param       None
 * @retval      ESP8266_EOK  : Successfully restored
 *              ESP8266_ERROR: Failed to restore
 */
uint8_t esp8266_restore(void)
{
    uint8_t ret;
    
    ret = esp8266_send_at_cmd("AT+RESTORE", "ready", 3000);
    if (ret == ESP8266_EOK)
    {
        return ESP8266_EOK;
    }
    else
    {
        return ESP8266_ERROR;
    }
}

/**
 * @brief       Test AT command for ATK-MW8266
 * @param       None
 * @retval      ESP8266_EOK  : AT command test successful
 *              ESP8266_ERROR: AT command test failed
 */
uint8_t esp8266_at_test(void)
{
    uint8_t ret;
    uint8_t i;
    
    for (i=0; i<10; i++)
    {
        ret = esp8266_send_at_cmd("AT", "OK", 500);
        if (ret == ESP8266_EOK)
        {
            return ESP8266_EOK;
        }
    }
    
    return ESP8266_ERROR;
}

/**
 * @brief       Set the working mode of ATK-MW8266
 * @param       mode: 1, Station mode
 *                    2, AP mode
 *                    3, AP+Station mode
 * @retval      ESP8266_EOK   : Successfully set working mode
 *              ESP8266_ERROR : Failed to set working mode
 *              esp8266_EINVAL: Invalid mode parameter
 */
uint8_t esp8266_set_mode(uint8_t mode)
{
    uint8_t ret;
    
    switch (mode)
    {
        case 1:
        {
            ret = esp8266_send_at_cmd("AT+CWMODE=1", "OK", 500);    /* Station mode */
            break;
        }
        case 2:
        {
            ret = esp8266_send_at_cmd("AT+CWMODE=2", "OK", 500);    /* AP mode */
            break;
        }
        case 3:
        {
            ret = esp8266_send_at_cmd("AT+CWMODE=3", "OK", 500);    /* AP+Station mode */
            break;
        }
        default:
        {
            return esp8266_EINVAL;
        }
    }
    
    if (ret == ESP8266_EOK)
    {
        return ESP8266_EOK;
    }
    else
    {
        return ESP8266_ERROR;
    }
}

/**
 * @brief       Software reset for ATK-MW8266
 * @param       None
 * @retval      ESP8266_EOK  : Software reset successful
 *              ESP8266_ERROR: Software reset failed
 */
uint8_t esp8266_sw_reset(void)
{
    uint8_t ret;
    
    ret = esp8266_send_at_cmd("AT+RST", "OK", 500);
    if (ret == ESP8266_EOK)
    {
        delay_ms(1000);
        return ESP8266_EOK;
    }
    else
    {
        return ESP8266_ERROR;
    }
}

/**
 * @brief       Configure echo mode for ATK-MW8266
 * @param       cfg: 0, Disable echo
 *                   1, Enable echo
 * @retval      ESP8266_EOK  : Successfully configured echo mode
 *              ESP8266_ERROR: Failed to configure echo mode
 */
uint8_t esp8266_ate_config(uint8_t cfg)
{
    uint8_t ret;
    
    switch (cfg)
    {
        case 0:
        {
            ret = esp8266_send_at_cmd("ATE0", "OK", 500);   /* Disable echo */
            break;
        }
        case 1:
        {
            ret = esp8266_send_at_cmd("ATE1", "OK", 500);   /* Enable echo */
            break;
        }
        default:
        {
            return esp8266_EINVAL;
        }
    }
    
    if (ret == ESP8266_EOK)
    {
        return ESP8266_EOK;
    }
    else
    {
        return ESP8266_ERROR;
    }
}

/**
 * @brief       Connect to WiFi using ATK-MW8266
 * @param       ssid: WiFi name
 *              pwd : WiFi password
 * @retval      ESP8266_EOK  : Successfully connected to WiFi
 *              ESP8266_ERROR: Failed to connect to WiFi
 */
uint8_t esp8266_join_ap(char *ssid, char *pwd)
{
    uint8_t ret;
    char cmd[64];
    
    sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"", ssid, pwd);
    ret = esp8266_send_at_cmd(cmd, "WIFI GOT IP", 10000);
    if (ret == ESP8266_EOK)
    {
        return ESP8266_EOK;
    }
    else
    {
        return ESP8266_ERROR;
    }
}

/**
 * @brief       Get IP address using ATK-MW8266
 * @param       buf: Buffer for IP address (requires 16 bytes of memory)
 * @retval      ESP8266_EOK  : Successfully retrieved IP address
 *              ESP8266_ERROR: Failed to retrieve IP address
 */
uint8_t esp8266_get_ip(char *buf)
{
    uint8_t ret;
    char *p_start;
    char *p_end;
    
    ret = esp8266_send_at_cmd("AT+CIFSR", "OK", 500);
    if (ret != ESP8266_EOK)
    {
        return ESP8266_ERROR;
    }
    
    p_start = strstr((const char *)esp8266_uart_rx_get_frame(), "\"");
    p_end = strstr(p_start + 1, "\"");
    *p_end = '\0';
    sprintf(buf, "%s", p_start + 1);
    
    return ESP8266_EOK;
}

/**
 * @brief       Connect to a TCP server using ATK-MW8266
 * @param       server_ip  : TCP server IP address
 *              server_port: TCP server port number
 * @retval      ESP8266_EOK  : Successfully connected to TCP server
 *              ESP8266_ERROR: Failed to connect to TCP server
 */
uint8_t esp8266_connect_tcp_server(char *server_ip, char *server_port)
{
    uint8_t ret;
    char cmd[64];
    
    sprintf(cmd, "AT+CIPSTART=\"TCP\",\"%s\",%s", server_ip, server_port);
    ret = esp8266_send_at_cmd(cmd, "CONNECT", 5000);
    if (ret == ESP8266_EOK)
    {
        return ESP8266_EOK;
    }
    else
    {
        return ESP8266_ERROR;
    }
}

/**
 * @brief       Enter transparent transmission mode using ATK-MW8266
 * @param       None
 * @retval      ESP8266_EOK  : Successfully entered transparent mode
 *              ESP8266_ERROR: Failed to enter transparent mode
 */
uint8_t esp8266_enter_unvarnished(void)
{
    uint8_t ret;
    
    ret  = esp8266_send_at_cmd("AT+CIPMODE=1", "OK", 500);
    ret += esp8266_send_at_cmd("AT+CIPSEND", ">", 500);
    if (ret == ESP8266_EOK)
    {
        return ESP8266_EOK;
    }
    else
    {
        return ESP8266_ERROR;
    }
}

/**
 * @brief       Exit transparent transmission mode using ATK-MW8266
 * @param       None
 * @retval      None
 */
void esp8266_exit_unvarnished(void)
{
    esp8266_uart_printf("+++");
}

/**
 * @brief       Connect to ATK cloud server using ATK-MW8266
 * @param       id : ATK cloud device ID
 *              pwd: ATK cloud device password
 * @retval      ESP8266_EOK  : Successfully connected to ATK cloud server
 *              ESP8266_ERROR: Failed to connect to ATK cloud server
 */
uint8_t esp8266_connect_atkcld(char *id, char *pwd)
{
    uint8_t ret;
    char cmd[64];
    
    sprintf(cmd, "AT+ATKCLDSTA=\"%s\",\"%s\"", id, pwd);
    ret = esp8266_send_at_cmd(cmd, "CLOUD CONNECTED", 10000);
    if (ret == ESP8266_EOK)
    {
        return ESP8266_EOK;
    }
    else
    {
        return ESP8266_ERROR;
    }
}

/**
 * @brief       Disconnect from ATK cloud server using ATK-MW8266
 * @param       None
 * @retval      ESP8266_EOK  : Successfully disconnected from ATK cloud server
 *              ESP8266_ERROR: Failed to disconnect from ATK cloud server
 */
uint8_t esp8266_disconnect_atkcld(void)
{
    uint8_t ret;
    
    ret = esp8266_send_at_cmd("AT+ATKCLDCLS", "CLOUD DISCONNECT", 500);
    if (ret == ESP8266_EOK)
    {
        return ESP8266_EOK;
    }
    else
    {
        return ESP8266_ERROR;
    }
}

void wifi_task(void)
{
    uint8_t ret;
    char ip_buf[16];
    uint8_t key;
    uint8_t is_atkcld = 0;
    
    ret = esp8266_init();
    if (ret != 0)
    {
        printf("ATK-MW8266 init failed!\r\n");
        while (1)
        {
            delay_ms(200);
        }
    }
    printf("Joining to AP...\r\n");
    ret  = esp8266_restore();                               /* Restore factory settings */
    ret += esp8266_at_test();                               /* AT test */
    ret += esp8266_set_mode(1);                             /* Station mode */
    ret += esp8266_sw_reset();                              /* Software reset */
    ret += esp8266_ate_config(0);                           /* Disable echo */
    ret += esp8266_join_ap(DEMO_WIFI_SSID, DEMO_WIFI_PWD);  /* Connect to WiFi */
    ret += esp8266_get_ip(ip_buf);                          /* Get IP address */
    if (ret != 0)
    {
        printf("Error to join ap!\r\n");
        while (1)
        {
            LED0_TOGGLE();
            delay_ms(200);
        }
    }
    demo_show_ip(ip_buf);
    
    /* Restart receiving new frame data */
    esp8266_uart_rx_restart();
    
    while (1)
    {
        demo_upload_data(is_atkcld);
        delay_ms(1000);
    }
}


































