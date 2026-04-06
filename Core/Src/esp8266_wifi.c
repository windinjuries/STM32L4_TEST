#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "main.h"
#include "cmsis_os.h"

// Simple network to host long conversion
#define ntohl(x) (((x) >> 24) | (((x) >> 8) & 0xFF00) | (((x) << 8) & 0xFF0000) | ((x) << 24))

#define DEMO_WIFI_SSID   "31-201"
#define DEMO_WIFI_PWD   "1234560789"       

typedef enum 
{
  ESP8266_MODE_STA = 1,
  ESP8266_MODE_AP,
  ESP8266_MODE_STA_AP
} esp8266_mode_t;

typedef enum
{
    ESP8266_EOK = 0,
    ESP8266_ERROR = -1,
    ESP8266_ETIMEOUT = -2,
    ESP8266_EINVAL = -3
} esp8266_error_t;

// WiFi state machine states
typedef enum
{
    WIFI_STATE_INIT = 0,
    WIFI_STATE_RESTORE_DEFAULTS,
    WIFI_STATE_AT_TEST,
    WIFI_STATE_SET_MODE,
    WIFI_STATE_SW_RESET,
    WIFI_STATE_DISABLE_ECHO,
    WIFI_STATE_CONNECT_AP,
    WIFI_STATE_GET_IP,
    WIFI_STATE_SET_SINGLE_CONNECTION,
    WIFI_STATE_SYNC_TIME,
    WIFI_STATE_IDLE
} wifi_state_t;

// WiFi state machine context
typedef struct
{
    wifi_state_t current_state;
    uint8_t retry_count;
    uint32_t last_retry_time;
    uint8_t max_retries;
    uint32_t retry_delay;
} wifi_context_t;

#define esp8266_UART_TX_BUF_SIZE  1024                      /* ATK-MW8266 UART transmit buffer size */
#define esp8266_UART_RX_BUF_SIZE  1024                      /* ATK-MW8266 UART receive buffer size */

#define ESP8266_RESET_LOW  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_RESET)
#define ESP8266_RESET_HIGH HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_SET)

extern UART_HandleTypeDef huart2;                           /* ATK-MW8266 UART */
extern DMA_HandleTypeDef hdma_usart2_rx;
static UART_HandleTypeDef *g_uart_handle = &huart2;         /* ATK-MW8266 UART */


static struct
{
    uint8_t buf[esp8266_UART_RX_BUF_SIZE];         /* Frame receive buffer */
    uint16_t finsh;                                /* Frame receive completion flag */
    uint16_t len;                                  /* Frame receive length */
} g_uart_rx_frame = {0};                           /* ATK-MW8266 UART receive frame buffer structure */

static uint8_t g_uart_tx_buf[esp8266_UART_TX_BUF_SIZE];

char ip_buf[16];
char time_buf[32];
// Initialize state machine
wifi_context_t wifi_ctx = {
		.current_state = WIFI_STATE_INIT,
		.retry_count = 0,
		.last_retry_time = 0,
		.max_retries = 3,
		.retry_delay = 5000
};

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
    HAL_UART_AbortReceive(&huart2);
    g_uart_rx_frame.len     = 0;
    g_uart_rx_frame.finsh   = 0;
    memset(g_uart_rx_frame.buf, 0, esp8266_UART_RX_BUF_SIZE);
    HAL_UARTEx_ReceiveToIdle_DMA(&huart2, g_uart_rx_frame.buf, esp8266_UART_RX_BUF_SIZE);
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
    if (g_uart_rx_frame.finsh == 1)
    {
        return g_uart_rx_frame.len;
    }
    else
    {
        return 0;
    }
}

void esp8266_uart_init(void)
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
    ESP8266_RESET_LOW;
    osDelay(100); 
    ESP8266_RESET_HIGH;
    osDelay(1000); 
}

int8_t esp8266_send_at_cmd(char *cmd, char *ack, uint32_t timeout)
{
    uint8_t *ret = NULL;
    esp8266_uart_rx_restart();
		if(cmd != NULL)
		{
				esp8266_uart_printf("%s\r\n", cmd);
		}
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
            osDelay(1);
        }
        return ESP8266_ETIMEOUT;
    }
}

int8_t esp8266_init()
{
    esp8266_hw_init();                          /* ATK-MW8266 hardware initialization */
    esp8266_hw_reset();                        /* ATK-MW8266 hardware reset */
    esp8266_uart_init();                        /* ATK-MW8266 UART initialization */
    return ESP8266_EOK;
}

/**
 * @brief       Restore ATK-MW8266 to factory settings
 * @param       None
 * @retval      ESP8266_EOK  : Successfully restored
 *              ESP8266_ERROR: Failed to restore
 */
int8_t esp8266_restore(void)
{
    int8_t ret;
    
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
int8_t esp8266_at_test(void)
{
    uint8_t i;
    int8_t ret;
    uint8_t *response;
    
    for (i=0; i<10; i++)
    {
        ret = esp8266_send_at_cmd("AT", "OK", 500);
        if (ret == ESP8266_EOK)
        {
            // AT test successful, now get firmware version
            ret = esp8266_send_at_cmd("AT+GMR", "OK", 1000);
            if (ret == ESP8266_EOK)
            {
                response = esp8266_uart_rx_get_frame();
                if (response != NULL)
                {
                    // printf("ESP8266 Firmware Version:\r\n%s\r\n", response);
                }
            }
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
 *              ESP8266_EINVAL: Invalid mode parameter
 */
int8_t esp8266_set_mode(uint8_t mode)
{
    uint8_t ret;
    
    switch (mode)
    {
        case 1:
        {
            ret = esp8266_send_at_cmd("AT+CWMODE=1", "OK", 1000);    /* Station mode */
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
            return ESP8266_EINVAL;
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
int8_t esp8266_sw_reset(void)
{
    int8_t ret;
    
    ret = esp8266_send_at_cmd("AT+RST", "OK", 500);
    if (ret == ESP8266_EOK)
    {
        osDelay(1000);
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
int8_t esp8266_ate_config(uint8_t cfg)
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
            return ESP8266_EINVAL;
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
int8_t esp8266_join_ap(char *ssid, char *pwd)
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
int8_t esp8266_get_ip(char *buf)
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
 * @brief       Connect to a server using ATK-MW8266
 * @param       type      : Connection type ("TCP" or "UDP")
 *              server_ip  : Server IP address or domain name
 *              server_port: Server port number
 * @retval      ESP8266_EOK  : Successfully connected to server
 *              ESP8266_ERROR: Failed to connect to server
 */
int8_t esp8266_connect_server(char *type, char *server_ip, char *server_port)
{
    int8_t ret;
    char cmd[64];
    
    sprintf(cmd, "AT+CIPSTART=\"%s\",\"%s\",%s", type, server_ip, server_port);
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
int8_t esp8266_enter_unvarnished(void)
{
    int8_t ret;
    
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
 * @brief       Get current time using HTTP request to Taobao time API
 * @param       time_buf: Buffer to store time string (requires at least 32 bytes)
 * @retval      ESP8266_EOK  : Successfully retrieved time
 *              ESP8266_ERROR: Failed to retrieve time
 */
int8_t esp8266_get_time_http(char *time_buf)
{
    int8_t ret;
    char *response;
    char *date_start;
    struct tm time_struct = {0};
    time_t unix_time;
    
    // Connect to Baidu server
    ret = esp8266_connect_server("TCP", "www.baidu.com", "80");
    if (ret != ESP8266_EOK)
    {
        return ESP8266_ERROR;
    }
    
    // Send HTTP HEAD request
    char http_request[] = "HEAD / HTTP/1.1\r\nHost: www.baidu.com\r\nConnection: close\r\n\r\n";
    char cmd[32];
    
    sprintf(cmd, "AT+CIPSEND=%d", strlen(http_request));
    ret = esp8266_send_at_cmd(cmd, ">", 1000);
    if (ret != ESP8266_EOK)
    {
        return ESP8266_ERROR;
    }
    
    esp8266_uart_printf("%s", http_request);
    esp8266_uart_rx_restart();
    // Wait for response
    osDelay(2000);
    
    // Get response
    response = (char *)esp8266_uart_rx_get_frame();
    if (response == NULL)
    {
        return ESP8266_ERROR;
    }
    
    // Find Date header
    date_start = strstr(response, "Date: ");
    if (date_start == NULL)
    {
        return ESP8266_ERROR;
    }
    
    date_start += strlen("Date: ");
    char *date_end = strstr(date_start, "\r\n");
    if (date_end == NULL)
    {
        return ESP8266_ERROR;
    }
    
    *date_end = '\0';
    
    // Parse date string: "Sat, 03 Jan 2026 01:51:03 GMT"
    // Use sscanf to parse
    char day[4], month[4];
    int day_num, year, hour, min, sec;
    if (sscanf(date_start, "%3s, %d %3s %d %d:%d:%d GMT", day, &day_num, month, &year, &hour, &min, &sec) != 7)
    {
        return ESP8266_ERROR;
    }
    
    // Convert month to number
    int month_num = 0;
    if (strcmp(month, "Jan") == 0) month_num = 1;
    else if (strcmp(month, "Feb") == 0) month_num = 2;
    else if (strcmp(month, "Mar") == 0) month_num = 3;
    else if (strcmp(month, "Apr") == 0) month_num = 4;
    else if (strcmp(month, "May") == 0) month_num = 5;
    else if (strcmp(month, "Jun") == 0) month_num = 6;
    else if (strcmp(month, "Jul") == 0) month_num = 7;
    else if (strcmp(month, "Aug") == 0) month_num = 8;
    else if (strcmp(month, "Sep") == 0) month_num = 9;
    else if (strcmp(month, "Oct") == 0) month_num = 10;
    else if (strcmp(month, "Nov") == 0) month_num = 11;
    else if (strcmp(month, "Dec") == 0) month_num = 12;
    else return ESP8266_ERROR;
    
    // Set tm structure
    time_struct.tm_year = year - 1900;
    time_struct.tm_mon = month_num - 1;
    time_struct.tm_mday = day_num;
    time_struct.tm_hour = hour;
    time_struct.tm_min = min;
    time_struct.tm_sec = sec;
    time_struct.tm_isdst = 0;  // GMT, no DST
    
    // Convert to Unix time
    unix_time = mktime(&time_struct);
    if (unix_time == -1)
    {
        return ESP8266_ERROR;
    }
    
    // Convert to local time structure
    struct tm *local_time = localtime(&unix_time);
    if (local_time == NULL)
    {
        return ESP8266_ERROR;
    }
    
    // Format time string
    strftime(time_buf, 32, "%Y-%m-%d %H:%M:%S", local_time);
    
    return ESP8266_EOK;
}

/**
 * @brief       Set RTC time using HTTP time API
 * @param       None
 * @retval      ESP8266_EOK  : Successfully set RTC time
 *              ESP8266_ERROR: Failed to set RTC time
 */
int8_t esp8266_set_rtc_time_http(void)
{
    extern RTC_HandleTypeDef hrtc;  // Declare external RTC handle from main.c
    
    char time_buf[32];
    int year, month, day, hour, minute, second;
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};
    int8_t ret;
    
    // Get time from HTTP API
    ret = esp8266_get_time_http(time_buf);
    if (ret != ESP8266_EOK)
    {
        return ESP8266_ERROR;
    }
    
    // Parse time string: "2024-01-02 12:34:56"
    if (sscanf(time_buf, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) != 6)
    {
        return ESP8266_ERROR;
    }
    
    // Set RTC Date
    sDate.Year = year - 2000;  // RTC year is offset from 2000
    sDate.Month = month;
    sDate.Date = day;
    sDate.WeekDay = RTC_WEEKDAY_MONDAY;  // Default, can be calculated if needed
    
    if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
    {
        return ESP8266_ERROR;
    }
    
    // Set RTC Time
    sTime.Hours = hour;
    sTime.Minutes = minute;
    sTime.Seconds = second;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;
    
    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
    {
        return ESP8266_ERROR;
    }

    return ESP8266_EOK;
}

/**
 * @brief       Get weather info from wttr.in
 * @param       city: City name (e.g., "Beijing", "Shanghai")
 * @param       weather_buf: Buffer to store weather info (requires at least 64 bytes)
 * @retval      ESP8266_EOK  : Successfully retrieved weather
 *              ESP8266_ERROR: Failed to retrieve weather
 */
int8_t esp8266_get_weather(char *city, char *weather_buf)
{
    int8_t ret;
    char *response;
    char *weather_start;
    char *weather_end;
    static char http_request[512];

    // Connect to wttr.in
    ret = esp8266_connect_server("TCP", "wttr.in", "80");
    if (ret != ESP8266_EOK)
    {
        return ESP8266_ERROR;
    }

    // Send HTTP GET request for simple format
    sprintf(http_request, "GET /%s?format=1 HTTP/1.1\r\nHost: wttr.in\r\nConnection: close\r\n\r\n", city);
    char cmd[32];

    sprintf(cmd, "AT+CIPSEND=%d", strlen(http_request));
    ret = esp8266_send_at_cmd(cmd, ">", 1000);
    if (ret != ESP8266_EOK)
    {
        return ESP8266_ERROR;
    }

    esp8266_uart_printf("%s", http_request);
    esp8266_uart_rx_restart();
    // Wait for response
    osDelay(3000);

    // Get response
    response = (char *)esp8266_uart_rx_get_frame();
    if (response == NULL)
    {
        return ESP8266_ERROR;
    }

    weather_end = strstr(response, "°C");

    if (weather_end == NULL)
    {
        return ESP8266_ERROR;
    }

    weather_start = weather_end - 3;
    strncpy(weather_buf, weather_start, 6);
    weather_buf[6] = '\0';

    return ESP8266_EOK;
}
typedef struct {
    uint32_t seconds;
    uint32_t fraction;
} ntp_timestamp_t;

/**
 * @brief       NTP packet structure
 */
typedef struct {
    uint8_t li_vn_mode;
    uint8_t stratum;
    uint8_t poll;
    uint8_t precision;
    uint32_t root_delay;
    uint32_t root_dispersion;
    uint32_t reference_id;
    ntp_timestamp_t reference_timestamp;
    ntp_timestamp_t originate_timestamp;
    ntp_timestamp_t receive_timestamp;
    ntp_timestamp_t transmit_timestamp;
} ntp_packet_t;

/**
 * @brief       Get current time using NTP protocol
 * @param       time_buf: Buffer to store time string (requires at least 32 bytes)
 * @retval      ESP8266_EOK  : Successfully retrieved time
 *              ESP8266_ERROR: Failed to retrieve time
 */
int8_t esp8266_get_time_ntp(char *time_buf)
{
    int8_t ret;
    ntp_packet_t ntp_packet = {0};
    uint8_t *response;
    uint16_t response_len;
    time_t unix_time;
    struct tm *time_info;
    
    // Connect to NTP server
    ret = esp8266_connect_server("UDP", "ntp.suning.com", "123");
    if (ret != ESP8266_EOK)
    {
        return ESP8266_ERROR;
    }
    
    // Prepare NTP packet
    ntp_packet.li_vn_mode = 0x1B;  // LI=0, VN=3, Mode=3 (client)
    
    // Send NTP packet
    esp8266_uart_printf("AT+CIPSEND=%d\r\n", sizeof(ntp_packet));
    osDelay(100);
    HAL_UART_Transmit(&huart2, (uint8_t *)&ntp_packet, sizeof(ntp_packet), HAL_MAX_DELAY);
    
    // Wait for response
    osDelay(5000);
    
    // Get response
    response = esp8266_uart_rx_get_frame();
    response_len = esp8266_uart_rx_get_frame_len();
    
    if (response == NULL || response_len < sizeof(ntp_packet))
    {
        return ESP8266_ERROR;
    }
    
    // Copy NTP response
    memcpy(&ntp_packet, response, sizeof(ntp_packet));
    
    // Convert NTP timestamp to Unix time
    // NTP timestamp starts from 1900-01-01, Unix from 1970-01-01
    unix_time = ntohl(ntp_packet.transmit_timestamp.seconds) - 2208988800UL;
    
    // Convert to local time structure
    time_info = gmtime(&unix_time);
    if (time_info == NULL)
    {
        return ESP8266_ERROR;
    }
    
    // Format time string
    strftime(time_buf, 32, "%Y-%m-%d %H:%M:%S", time_info);
    
    return ESP8266_EOK;
}

/**
 * @brief       Set RTC time using NTP protocol
 * @param       None
 * @retval      ESP8266_EOK  : Successfully set RTC time
 *              ESP8266_ERROR: Failed to set RTC time
 */
int8_t esp8266_set_rtc_time_ntp(void)
{
    extern RTC_HandleTypeDef hrtc;  // Declare external RTC handle from main.c
    
    char time_buf[32];
    int year, month, day, hour, minute, second;
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};
    int8_t ret;
    
    // Get time from NTP
    ret = esp8266_get_time_ntp(time_buf);
    if (ret != ESP8266_EOK)
    {
        return ESP8266_ERROR;
    }
    
    // Parse time string: "2024-01-02 12:34:56"
    if (sscanf(time_buf, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) != 6)
    {
        return ESP8266_ERROR;
    }
    
    // Set RTC Date
    sDate.Year = year - 2000;  // RTC year is offset from 2000
    sDate.Month = month;
    sDate.Date = day;
    sDate.WeekDay = RTC_WEEKDAY_MONDAY;  // Default, can be calculated if needed
    
    if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
    {
        return ESP8266_ERROR;
    }
    
    // Set RTC Time
    sTime.Hours = hour;
    sTime.Minutes = minute;
    sTime.Seconds = second;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;
    
    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
    {
        return ESP8266_ERROR;
    }
    
    return ESP8266_EOK;
}

void wifi_task(const void *argument)
{
    int8_t ret;
    char ip_buf[16];
    char time_buf[32];
    // Initialize state machine
    
    while (1)
    {
        uint32_t current_time = HAL_GetTick();
        switch (wifi_ctx.current_state)
        {
            case WIFI_STATE_INIT:
            {
                ret = esp8266_init();
                if (ret == ESP8266_EOK)
                {
                    wifi_ctx.current_state = WIFI_STATE_AT_TEST;
                    wifi_ctx.retry_count = 0;
                }
                else
                {
                    if (wifi_ctx.retry_count < wifi_ctx.max_retries)
                    {
                        wifi_ctx.retry_count++;
                        wifi_ctx.last_retry_time = current_time;
                    }
                }
                break;
            }
            
            case WIFI_STATE_RESTORE_DEFAULTS:
            {
                ret = esp8266_restore();
                if (ret == ESP8266_EOK)
                {
                    // Restore successful, go to AT test
                    wifi_ctx.current_state = WIFI_STATE_AT_TEST;
                    wifi_ctx.retry_count = 0;
                }
                else
                {
                    // Restore failed, go back to init anyway
                    wifi_ctx.current_state = WIFI_STATE_INIT;
                    wifi_ctx.retry_count = 0;
                }
                break;
            }
            
            case WIFI_STATE_AT_TEST:
            {
                ret = esp8266_at_test();
                if (ret == ESP8266_EOK)
                {
                    wifi_ctx.current_state = WIFI_STATE_SET_MODE;
                    wifi_ctx.retry_count = 0;
                }
                else
                {
                    if (wifi_ctx.retry_count < wifi_ctx.max_retries)
                    {
                        wifi_ctx.retry_count++;
                        wifi_ctx.last_retry_time = current_time;
                    }
                    else
                    {
                        wifi_ctx.current_state = WIFI_STATE_INIT;  // Go back to init
                        wifi_ctx.retry_count = 0;
                    }
                }
                break;
            }
            
            case WIFI_STATE_SW_RESET:
            {
                ret = esp8266_sw_reset();
                if (ret == ESP8266_EOK)
                {
                    wifi_ctx.current_state = WIFI_STATE_DISABLE_ECHO;
                    wifi_ctx.retry_count = 0;
                }
                else
                {
                    if (wifi_ctx.retry_count < wifi_ctx.max_retries)
                    {
                        wifi_ctx.retry_count++;
                        wifi_ctx.last_retry_time = current_time;
                    }
                    else
                    {
                        wifi_ctx.current_state = WIFI_STATE_INIT;
                        wifi_ctx.retry_count = 0;
                    }
                }
                break;
            }
            
            case WIFI_STATE_DISABLE_ECHO:
            {
                ret = esp8266_ate_config(0);  // Disable echo
                if (ret == ESP8266_EOK)
                {
                    wifi_ctx.current_state = WIFI_STATE_CONNECT_AP;
                    wifi_ctx.retry_count = 0;
                }
                else
                {
                    if (wifi_ctx.retry_count < wifi_ctx.max_retries)
                    {
                        wifi_ctx.retry_count++;
                        wifi_ctx.last_retry_time = current_time;
                    }
                    else
                    {
                        wifi_ctx.current_state = WIFI_STATE_INIT;
                        wifi_ctx.retry_count = 0;
                    }
                }
                break;
            }
            
            case WIFI_STATE_SET_MODE:
            {
                ret = esp8266_set_mode(1);  // Station mode
                if (ret == ESP8266_EOK)
                {
                    wifi_ctx.current_state = WIFI_STATE_SW_RESET;
                    wifi_ctx.retry_count = 0;
                }
                else
                {
                    if (wifi_ctx.retry_count < wifi_ctx.max_retries)
                    {
                        wifi_ctx.retry_count++;
                        wifi_ctx.last_retry_time = current_time;
                    }
                    else
                    {
                        wifi_ctx.current_state = WIFI_STATE_INIT;
                        wifi_ctx.retry_count = 0;
                    }
                }
                break;
            }
            
            case WIFI_STATE_CONNECT_AP:
            {
                ret = esp8266_join_ap(DEMO_WIFI_SSID, DEMO_WIFI_PWD);
                if (ret == ESP8266_EOK)
                {
                    wifi_ctx.current_state = WIFI_STATE_GET_IP;
                    wifi_ctx.retry_count = 0;
                }
                else
                {
                    if (wifi_ctx.retry_count < wifi_ctx.max_retries)
                    {
                        wifi_ctx.retry_count++;
                        wifi_ctx.last_retry_time = current_time;
                    }
                    else
                    {
                        wifi_ctx.current_state = WIFI_STATE_INIT;
                        wifi_ctx.retry_count = 0;
                    }
                }
                break;
            }
            
            case WIFI_STATE_GET_IP:
            {
                ret = esp8266_get_ip(ip_buf);
                if (ret == ESP8266_EOK)
                {
                    wifi_ctx.current_state = WIFI_STATE_SET_SINGLE_CONNECTION;
                    wifi_ctx.retry_count = 0;
                }
                else
                {
                    if (wifi_ctx.retry_count < wifi_ctx.max_retries)
                    {
                        wifi_ctx.retry_count++;
                        wifi_ctx.last_retry_time = current_time;
                    }
                    else
                    {
                        wifi_ctx.current_state = WIFI_STATE_CONNECT_AP;  // Go back to connect
                        wifi_ctx.retry_count = 0;
                    }
                }
                break;
            }
            
            case WIFI_STATE_SET_SINGLE_CONNECTION:
            {
                ret = esp8266_send_at_cmd("AT+CIPMUX=0", "OK", 500);
                if (ret == ESP8266_EOK)
                {
                    wifi_ctx.current_state = WIFI_STATE_SYNC_TIME;
                    wifi_ctx.retry_count = 0;
                }
                else
                {
                    if (wifi_ctx.retry_count < wifi_ctx.max_retries)
                    {
                        wifi_ctx.retry_count++;
                        wifi_ctx.last_retry_time = current_time;
                    }
                    else
                    {
                        wifi_ctx.current_state = WIFI_STATE_GET_IP;  // Go back to get IP
                        wifi_ctx.retry_count = 0;
                    }
                }
                break;
            }
            
            case WIFI_STATE_SYNC_TIME:
            {
                ret = esp8266_set_rtc_time_http();
                if (ret == ESP8266_EOK)
                {
                    wifi_ctx.current_state = WIFI_STATE_IDLE;
                    wifi_ctx.retry_count = 0;
                }
                else
                {
                    if (wifi_ctx.retry_count < wifi_ctx.max_retries)
                    {
                        wifi_ctx.retry_count++;
                        wifi_ctx.last_retry_time = current_time;
                    }
                    else
                    {
                        // Time sync failed, but continue to idle state
                        wifi_ctx.current_state = WIFI_STATE_IDLE;
                        wifi_ctx.retry_count = 0;
                    }
                }
                break;
            }
            
            case WIFI_STATE_IDLE:
            {
                // In idle state, periodically check connection or do other tasks
                // For now, just stay here
                break;
            }
            
            default:
            {
                wifi_ctx.current_state = WIFI_STATE_INIT;
                break;
            }
        }
        
        // Handle retry delay
        if (wifi_ctx.retry_count > 0 && 
            (current_time - wifi_ctx.last_retry_time) < wifi_ctx.retry_delay)
        {
            // Still in retry delay, wait
            osDelay(100);
            continue;
        }
        
        // Reset retry count if delay has passed
        if (wifi_ctx.retry_count > 0 && 
            (current_time - wifi_ctx.last_retry_time) >= wifi_ctx.retry_delay)
        {
            wifi_ctx.retry_count = 0;
        }
        
        osDelay(100);  // Small delay to prevent busy waiting
    }
}

int fputc(int ch, FILE *f)
{
    ITM_SendChar(ch);   
    return ch;
}































