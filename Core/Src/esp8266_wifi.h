#ifndef __ESP8266_WIFI_H__
#define __ESP8266_WIFI_H__

#include "main.h"

int8_t esp8266_get_time_http(char *time_buf);
int8_t esp8266_set_rtc_time_http(void);
int8_t esp8266_get_time_ntp(char *time_buf);
int8_t esp8266_set_rtc_time_ntp(void);
int wifi_task(void *argument);

#endif /* __ESP8266_WIFI_H__ */