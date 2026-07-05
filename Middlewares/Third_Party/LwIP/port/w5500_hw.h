#ifndef W5500_HW_H
#define W5500_HW_H

#include "main.h"

#define W5500_SPI_HANDLE           hspi2

#define W5500_CS_GPIO_Port         GPIOD
#define W5500_CS_Pin               GPIO_PIN_5

#define W5500_RST_GPIO_Port        GPIOD
#define W5500_RST_Pin              GPIO_PIN_4

#define W5500_INT_GPIO_Port        GPIOD
#define W5500_INT_Pin              GPIO_PIN_3
#define W5500_INT_EXTI_IRQn        EXTI3_IRQn
#define W5500_INT_EXTI_LINE        EXTI_LINE_3

extern SPI_HandleTypeDef hspi2;

void w5500_hw_init(void);
void w5500_hw_reset(void);
void w5500_spi1_init(void);
void w5500_hw_register_callbacks(void);
uint8_t w5500_hw_register_read_test(void);
void w5500_hw_int_enable(void);
void w5500_hw_int_disable(void);

#endif /* W5500_HW_H */
