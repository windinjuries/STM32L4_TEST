#include "es8388.h"
#include "config.h"
#if (CONFIG_USE_FREEMASTER == 0)
#include "debug_log.h"
#endif

#define ES8388_I2C_ADDR                 0x10U
#define ES8388_I2C_TIMEOUT              10U

#define ES8388_CONTROL1                 0x00U
#define ES8388_CONTROL2                 0x01U
#define ES8388_CHIPPOWER                0x02U
#define ES8388_ADCPOWER                 0x03U
#define ES8388_DACPOWER                 0x04U
#define ES8388_MASTERMODE               0x08U
#define ES8388_ADCCONTROL1              0x09U
#define ES8388_ADCCONTROL2              0x0AU
#define ES8388_ADCCONTROL3              0x0BU
#define ES8388_ADCCONTROL4              0x0CU
#define ES8388_ADCCONTROL5              0x0DU
#define ES8388_ADCCONTROL8              0x10U
#define ES8388_ADCCONTROL9              0x11U
#define ES8388_DACCONTROL1              0x17U
#define ES8388_DACCONTROL2              0x18U
#define ES8388_DACCONTROL3              0x19U
#define ES8388_DACCONTROL16             0x26U
#define ES8388_DACCONTROL17             0x27U
#define ES8388_DACCONTROL20             0x2AU
#define ES8388_DACCONTROL21             0x2BU
#define ES8388_DACCONTROL23             0x2DU

static I2C_HandleTypeDef *es8388_i2c;

static HAL_StatusTypeDef ES8388_Write(uint8_t reg, uint8_t value)
{
  if (es8388_i2c == NULL)
  {
    return HAL_ERROR;
  }

  return HAL_I2C_Mem_Write(es8388_i2c,
                           (uint16_t)(ES8388_I2C_ADDR << 1),
                           reg,
                           I2C_MEMADD_SIZE_8BIT,
                           &value,
                           1U,
                           ES8388_I2C_TIMEOUT);
}

HAL_StatusTypeDef ES8388_Init(I2C_HandleTypeDef *hi2c)
{
  static const struct
  {
    uint8_t reg;
    uint8_t value;
  } init_sequence[] =
  {
    {ES8388_DACCONTROL3, 0x04U},
    {ES8388_CONTROL2, 0x50U},
    {ES8388_CHIPPOWER, 0x00U},
    {ES8388_MASTERMODE, 0x00U},
    {ES8388_DACPOWER, 0xC0U},
    {ES8388_CONTROL1, 0x12U},
    {ES8388_DACCONTROL1, 0x18U},
    {ES8388_DACCONTROL2, 0x02U},
    {ES8388_DACCONTROL16, 0x00U},
    {ES8388_DACCONTROL17, 0x90U},
    {ES8388_DACCONTROL20, 0x90U},
    {ES8388_DACCONTROL21, 0x80U},
    {ES8388_DACCONTROL23, 0x00U},
    {ES8388_ADCPOWER, 0xFFU},
    {ES8388_ADCCONTROL1, 0xBBU},
    {ES8388_ADCCONTROL2, 0x00U},
    {ES8388_ADCCONTROL3, 0x02U},
    {ES8388_ADCCONTROL4, 0x0DU},
    {ES8388_ADCCONTROL5, 0x02U},
    {ES8388_ADCCONTROL8, 0x00U},
    {ES8388_ADCCONTROL9, 0x00U},
    {ES8388_ADCPOWER, 0x09U},
  };

  HAL_StatusTypeDef status;
  uint32_t index;

  if ((hi2c == NULL) || (hi2c->Instance == NULL))
  {
#if (CONFIG_USE_FREEMASTER == 0)
    LOG_ERROR("ES8388 init failed: I2C handle is NULL");
#endif
    return HAL_ERROR;
  }

  es8388_i2c = hi2c;
  status = HAL_I2C_IsDeviceReady(es8388_i2c,
                                 (uint16_t)(ES8388_I2C_ADDR << 1),
                                 3U,
                                 ES8388_I2C_TIMEOUT);
  if (status != HAL_OK)
  {
#if (CONFIG_USE_FREEMASTER == 0)
    LOG_ERROR("ES8388 I2C probe failed: address=0x%02X, HAL status=%lu",
              (unsigned int)ES8388_I2C_ADDR,
              (unsigned long)status);
#endif
    return status;
  }

  for (index = 0U; index < (sizeof(init_sequence) / sizeof(init_sequence[0])); index++)
  {
    status = ES8388_Write(init_sequence[index].reg, init_sequence[index].value);
    if (status != HAL_OK)
    {
#if (CONFIG_USE_FREEMASTER == 0)
      LOG_ERROR("ES8388 register write failed: index=%lu, reg=0x%02X, value=0x%02X, HAL status=%lu",
                (unsigned long)index,
                (unsigned int)init_sequence[index].reg,
                (unsigned int)init_sequence[index].value,
                (unsigned long)status);
#endif
      return status;
    }
  }

#if (CONFIG_USE_FREEMASTER == 0)
  LOG_INFO("ES8388 initialized successfully: address=0x%02X, registers=%lu",
           (unsigned int)ES8388_I2C_ADDR,
           (unsigned long)(sizeof(init_sequence) / sizeof(init_sequence[0])));
#endif
  return HAL_OK;
}

HAL_StatusTypeDef ES8388_StartAdc(void)
{
  HAL_StatusTypeDef status;

  status = ES8388_Write(ES8388_DACCONTROL21, 0x80U);
  if (status != HAL_OK)
  {
    return status;
  }

  status = ES8388_Write(ES8388_CHIPPOWER, 0xF0U);
  if (status != HAL_OK)
  {
    return status;
  }

  status = ES8388_Write(ES8388_CHIPPOWER, 0x00U);
  if (status != HAL_OK)
  {
    return status;
  }

  return ES8388_Write(ES8388_ADCPOWER, 0x00U);
}

HAL_StatusTypeDef ES8388_StopAdc(void)
{
  return ES8388_Write(ES8388_ADCPOWER, 0xFFU);
}
