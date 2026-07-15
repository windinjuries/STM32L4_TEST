#ifndef CH395_HW_H
#define CH395_HW_H

#include "main.h"

#define CH395_SPI_HANDLE           hspi2

#define CH395_CS_GPIO_Port         GPIOD
#define CH395_CS_Pin               GPIO_PIN_5

#define CH395_RST_GPIO_Port        GPIOD
#define CH395_RST_Pin              GPIO_PIN_4

#define CH395_INT_GPIO_Port        GPIOD
#define CH395_INT_Pin              GPIO_PIN_3
#define CH395_INT_EXTI_IRQn        EXTI3_IRQn
#define CH395_INT_EXTI_LINE        EXTI_LINE_3


extern SPI_HandleTypeDef hspi2;
void ch395_cris_enter(void);
void ch395_cris_exit(void);

#define CMD_START_HANDEL() ch395_cris_enter()
#define CMD_END_HANDEL() ch395_cris_exit()

void ch395_delay_us(uint32_t us);
void ch395_hw_init(void);
void ch395_hw_reset(void);
void ch395_hw_int_enable(void);
void ch395_hw_int_disable(void);

void ch395_cs_select(void);
void ch395_cs_deselect(void);
uint8_t ch395_spi_swapbyte(uint8_t wb);

#endif /* CH395_HW_H */
