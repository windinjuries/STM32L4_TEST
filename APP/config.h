#ifndef __CONFIG_H
#define __CONFIG_H

#define CONFIG_MODULE_LVGL_ENABLE   0

/* 串口复用选择：
 *   1 = USART1 用于 FreeMaster 调试监控
 *   0 = USART1 用于日志输出 (debug_log)
 */
#define CONFIG_USE_FREEMASTER      0


#define CONFIG_USE_LWIP_PORT_CH395Q 1
// #define CONFIG_USE_LWIP_PORT_W5500  0


#endif

