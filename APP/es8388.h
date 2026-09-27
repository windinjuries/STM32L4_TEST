#ifndef ES8388_H
#define ES8388_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32l4xx_hal.h"

#define AUDIO_POWER_ENABLE_GPIO_Port    GPIOA
#define AUDIO_POWER_ENABLE_Pin          GPIO_PIN_15

extern SAI_HandleTypeDef hsai_BlockA1;
extern SAI_HandleTypeDef hsai_BlockB1;

HAL_StatusTypeDef ES8388_Init(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef ES8388_StartAdc(void);
HAL_StatusTypeDef ES8388_StopAdc(void);

#ifdef __cplusplus
}
#endif

#endif
