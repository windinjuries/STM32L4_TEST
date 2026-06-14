#ifndef UNITY_CONFIG_H
#define UNITY_CONFIG_H

#include "stm32l4xx_hal.h"

/* USART1 句柄，在 main.c 中定义 */
extern UART_HandleTypeDef huart1;

/* Unity 输出字符重定向到 USART1 */
#define UNITY_OUTPUT_CHAR(c) \
    do { \
        uint8_t _c = (uint8_t)(c); \
        HAL_UART_Transmit(&huart1, &_c, 1, 100); \
    } while(0)

#endif /* UNITY_CONFIG_H */
