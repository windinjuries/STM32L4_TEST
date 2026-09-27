/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_audio_if.c
  * @version        : v2.0_Cube
  * @brief          : Generic media access layer.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
 /* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "usbd_audio_if.h"

/* USER CODE BEGIN INCLUDE */
#include "main.h"
#include "es8388.h"
#include "config.h"
#if (CONFIG_USE_FREEMASTER == 0)
#include "debug_log.h"
#endif
/* USER CODE END INCLUDE */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @brief Usb device library.
  * @{
  */

/** @addtogroup USBD_AUDIO_IF
  * @{
  */

/** @defgroup USBD_AUDIO_IF_Private_TypesDefinitions USBD_AUDIO_IF_Private_TypesDefinitions
  * @brief Private types.
  * @{
  */

/* USER CODE BEGIN PRIVATE_TYPES */

/* USER CODE END PRIVATE_TYPES */

/**
  * @}
  */

/** @defgroup USBD_AUDIO_IF_Private_Defines USBD_AUDIO_IF_Private_Defines
  * @brief Private defines.
  * @{
  */

/* USER CODE BEGIN PRIVATE_DEFINES */

/* USER CODE END PRIVATE_DEFINES */

/**
  * @}
  */

/** @defgroup USBD_AUDIO_IF_Private_Macros USBD_AUDIO_IF_Private_Macros
  * @brief Private macros.
  * @{
  */

/* USER CODE BEGIN PRIVATE_MACRO */

/* USER CODE END PRIVATE_MACRO */

/**
  * @}
  */

/** @defgroup USBD_AUDIO_IF_Private_Variables USBD_AUDIO_IF_Private_Variables
  * @brief Private variables.
  * @{
  */

/* USER CODE BEGIN PRIVATE_VARIABLES */
volatile AUDIO_IF_DiagnosticsTypeDef audio_if_diagnostics = {0};
/* USER CODE END PRIVATE_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_AUDIO_IF_Exported_Variables USBD_AUDIO_IF_Exported_Variables
  * @brief Public variables.
  * @{
  */

extern USBD_HandleTypeDef hUsbDeviceFS;

/* USER CODE BEGIN EXPORTED_VARIABLES */

/* USER CODE END EXPORTED_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_AUDIO_IF_Private_FunctionPrototypes USBD_AUDIO_IF_Private_FunctionPrototypes
  * @brief Private functions declaration.
  * @{
  */

static int8_t AUDIO_Init_FS(uint32_t AudioFreq, uint32_t Volume, uint32_t options);
static int8_t AUDIO_DeInit_FS(uint32_t options);
static int8_t AUDIO_AudioCmd_FS(uint8_t* pbuf, uint32_t size, uint8_t cmd);
static int8_t AUDIO_VolumeCtl_FS(uint8_t vol);
static int8_t AUDIO_MuteCtl_FS(uint8_t cmd);
static int8_t AUDIO_PeriodicTC_FS(uint8_t *pbuf, uint32_t size, uint8_t cmd);
static int8_t AUDIO_GetState_FS(void);

/* USER CODE BEGIN PRIVATE_FUNCTIONS_DECLARATION */
static void AUDIO_IF_RecordError(uint32_t error, HAL_StatusTypeDef status);
/* USER CODE END PRIVATE_FUNCTIONS_DECLARATION */

/**
  * @}
  */

USBD_AUDIO_ItfTypeDef USBD_AUDIO_fops_FS =
{
  AUDIO_Init_FS,
  AUDIO_DeInit_FS,
  AUDIO_AudioCmd_FS,
  AUDIO_VolumeCtl_FS,
  AUDIO_MuteCtl_FS,
  AUDIO_PeriodicTC_FS,
  AUDIO_GetState_FS,
};

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Initializes the AUDIO media low layer over USB FS IP
  * @param  AudioFreq: Audio frequency used to play the audio stream.
  * @param  Volume: Initial volume level (from 0 (Mute) to 100 (Max))
  * @param  options: Reserved for future use
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t AUDIO_Init_FS(uint32_t AudioFreq, uint32_t Volume, uint32_t options)
{
  /* USER CODE BEGIN 0 */
  UNUSED(AudioFreq);
  UNUSED(Volume);
  UNUSED(options);
  return (USBD_OK);
  /* USER CODE END 0 */
}

/**
  * @brief  De-Initializes the AUDIO media low layer
  * @param  options: Reserved for future use
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t AUDIO_DeInit_FS(uint32_t options)
{
  /* USER CODE BEGIN 1 */
  UNUSED(options);
  (void)AUDIO_AudioCmd_FS(NULL, 0U, AUDIO_CMD_STOP);
  return (USBD_OK);
  /* USER CODE END 1 */
}

/**
  * @brief  Handles AUDIO command.
  * @param  pbuf: Pointer to buffer of data to be sent
  * @param  size: Number of data to be sent (in bytes)
  * @param  cmd: Command opcode
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t AUDIO_AudioCmd_FS(uint8_t* pbuf, uint32_t size, uint8_t cmd)
{
  /* USER CODE BEGIN 2 */
  static const uint8_t clock_frame[4U] = {0U, 0U, 0U, 0U};
  HAL_StatusTypeDef status;

  switch(cmd)
  {
    case AUDIO_CMD_START:
      audio_if_diagnostics.start_count++;
      if ((pbuf == NULL) || (size != AUDIO_TOTAL_BUF_SIZE))
      {
        AUDIO_IF_RecordError(AUDIO_IF_ERROR_INVALID_BUFFER, HAL_ERROR);
        return USBD_FAIL;
      }

      status = ES8388_StartAdc();
      if (status != HAL_OK)
      {
        AUDIO_IF_RecordError(AUDIO_IF_ERROR_ES8388_START, status);
        return USBD_FAIL;
      }

      status = HAL_SAI_Transmit(&hsai_BlockA1, (uint8_t *)clock_frame, 2U, 0U);
      if (status != HAL_OK)
      {
        AUDIO_IF_RecordError(AUDIO_IF_ERROR_SAI_TX, status);
        (void)ES8388_StopAdc();
        return USBD_FAIL;
      }

      status = HAL_SAI_Receive_DMA(&hsai_BlockB1, pbuf, (uint16_t)(AUDIO_TOTAL_BUF_SIZE / 2U));
      if (status != HAL_OK)
      {
        AUDIO_IF_RecordError(AUDIO_IF_ERROR_SAI_RX, status);
        HAL_SAI_Abort(&hsai_BlockA1);
        (void)ES8388_StopAdc();
        return USBD_FAIL;
      }
      audio_if_diagnostics.start_success_count++;
      break;

    case AUDIO_CMD_PLAY:
      break;

    case AUDIO_CMD_STOP:
      status = HAL_SAI_DMAStop(&hsai_BlockB1);
      if (status != HAL_OK)
      {
        AUDIO_IF_RecordError(AUDIO_IF_ERROR_SAI_STOP, status);
      }
      status = HAL_SAI_Abort(&hsai_BlockA1);
      if (status != HAL_OK)
      {
        AUDIO_IF_RecordError(AUDIO_IF_ERROR_SAI_STOP, status);
      }
      status = ES8388_StopAdc();
      if (status != HAL_OK)
      {
        AUDIO_IF_RecordError(AUDIO_IF_ERROR_ES8388_START, status);
      }
      break;

    default:
      AUDIO_IF_RecordError(AUDIO_IF_ERROR_INVALID_BUFFER, HAL_ERROR);
      return USBD_FAIL;
  }

  return (USBD_OK);
  /* USER CODE END 2 */
}

/**
  * @brief  Controls AUDIO Volume.
  * @param  vol: volume level (0..100)
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t AUDIO_VolumeCtl_FS(uint8_t vol)
{
  /* USER CODE BEGIN 3 */
  UNUSED(vol);
  return (USBD_OK);
  /* USER CODE END 3 */
}

/**
  * @brief  Controls AUDIO Mute.
  * @param  cmd: command opcode
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t AUDIO_MuteCtl_FS(uint8_t cmd)
{
  /* USER CODE BEGIN 4 */
  UNUSED(cmd);
  return (USBD_OK);
  /* USER CODE END 4 */
}

/**
  * @brief  AUDIO_PeriodicT_FS
  * @param  cmd: Command opcode
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t AUDIO_PeriodicTC_FS(uint8_t *pbuf, uint32_t size, uint8_t cmd)
{
  /* USER CODE BEGIN 5 */
  UNUSED(pbuf);
  UNUSED(size);
  UNUSED(cmd);
  return (USBD_OK);
  /* USER CODE END 5 */
}

/**
  * @brief  Gets AUDIO State.
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t AUDIO_GetState_FS(void)
{
  /* USER CODE BEGIN 6 */
  return (USBD_OK);
  /* USER CODE END 6 */
}

/**
  * @brief  Manages the DMA full transfer complete event.
  * @retval None
  */
void TransferComplete_CallBack_FS(void)
{
  /* USER CODE BEGIN 7 */
  USBD_AUDIO_Sync(&hUsbDeviceFS, AUDIO_OFFSET_FULL);
  /* USER CODE END 7 */
}

/**
  * @brief  Manages the DMA Half transfer complete event.
  * @retval None
  */
void HalfTransfer_CallBack_FS(void)
{
  /* USER CODE BEGIN 8 */
  USBD_AUDIO_Sync(&hUsbDeviceFS, AUDIO_OFFSET_HALF);
  /* USER CODE END 8 */
}

/* USER CODE BEGIN PRIVATE_FUNCTIONS_IMPLEMENTATION */

static void AUDIO_IF_RecordError(uint32_t error, HAL_StatusTypeDef status)
{
  audio_if_diagnostics.last_error = error;
  audio_if_diagnostics.last_hal_status = (uint32_t)status;
  audio_if_diagnostics.error_count++;
}

void HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
  if (hsai->Instance == SAI1_Block_B)
  {
    audio_if_diagnostics.dma_half_count++;
    USBD_AUDIO_Sync(&hUsbDeviceFS, AUDIO_OFFSET_HALF);
  }
}

void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai)
{
  if (hsai->Instance == SAI1_Block_B)
  {
    audio_if_diagnostics.dma_full_count++;
    USBD_AUDIO_Sync(&hUsbDeviceFS, AUDIO_OFFSET_FULL);
  }
}

void AUDIO_IF_ReportDiagnostics(void)
{
#if (CONFIG_USE_FREEMASTER == 0)
  static const char *const error_names[] =
  {
    "none",
    "invalid buffer",
    "ES8388 ADC start",
    "SAI Block A TX",
    "SAI Block B RX",
    "SAI stop",
  };
  static uint32_t reported_error_count;
  static uint32_t reported_set_interface_fail_count;
  static uint32_t reported_open_ep_fail_count;
  static uint32_t reported_tx_fail_count;
  static uint32_t reported_iso_incomplete_count;
  static uint8_t capture_reported;
  static uint32_t capture_start_tick;
  static uint32_t last_heartbeat_tick;
  static uint32_t baseline_dma_count;
  static uint32_t baseline_sof_count;
  static uint32_t baseline_tx_count;
  static uint32_t last_stall_report_tick;
  USBD_AUDIO_DiagTypeDef class_diagnostics;
  uint32_t now;
  uint32_t dma_count;
  uint32_t error_index;

  USBD_AUDIO_GetDiagnostics(&hUsbDeviceFS, &class_diagnostics);
  now = HAL_GetTick();

  if (audio_if_diagnostics.error_count != reported_error_count)
  {
    error_index = audio_if_diagnostics.last_error;
    if (error_index >= (sizeof(error_names) / sizeof(error_names[0])))
    {
      error_index = 0U;
    }
    LOG_ERROR("USB audio start failed: stage=%s, HAL status=%lu",
              error_names[error_index],
              (unsigned long)audio_if_diagnostics.last_hal_status);
    reported_error_count = audio_if_diagnostics.error_count;
  }

  if (class_diagnostics.open_ep_fail_count != reported_open_ep_fail_count)
  {
    LOG_ERROR("USB audio isochronous endpoint open failed");
    reported_open_ep_fail_count = class_diagnostics.open_ep_fail_count;
  }

  if (class_diagnostics.set_interface_fail_count != reported_set_interface_fail_count)
  {
    LOG_ERROR("USB SET_INTERFACE audio capture start/stop failed, count=%lu",
              (unsigned long)class_diagnostics.set_interface_fail_count);
    reported_set_interface_fail_count = class_diagnostics.set_interface_fail_count;
  }

  if (class_diagnostics.capture_active == 0U)
  {
    capture_reported = 0U;
    return;
  }

  if (capture_reported == 0U)
  {
    capture_reported = 1U;
    capture_start_tick = now;
    last_heartbeat_tick = now;
    baseline_dma_count = audio_if_diagnostics.dma_half_count +
                         audio_if_diagnostics.dma_full_count;
    baseline_sof_count = class_diagnostics.sof_count;
    baseline_tx_count = class_diagnostics.tx_count;
    last_stall_report_tick = now;
    LOG_INFO("USB microphone capture started: format=2ch/16bit/%luHz",
             (unsigned long)USBD_AUDIO_FREQ);
    return;
  }

  dma_count = audio_if_diagnostics.dma_half_count +
              audio_if_diagnostics.dma_full_count;
  if (((now - capture_start_tick) >= 2000U) &&
      ((now - last_stall_report_tick) >= 5000U))
  {
    if (dma_count == baseline_dma_count)
    {
      LOG_ERROR("SAI RX DMA has no half/full callback after 2 seconds");
      last_stall_report_tick = now;
    }
    else if (class_diagnostics.sof_count == baseline_sof_count)
    {
      LOG_ERROR("USB SOF callback is not running while capture is active");
      last_stall_report_tick = now;
    }
    else if (class_diagnostics.tx_count == baseline_tx_count)
    {
      LOG_ERROR("USB audio IN endpoint has no completed transfer");
      last_stall_report_tick = now;
    }
  }

  if (class_diagnostics.tx_fail_count != reported_tx_fail_count)
  {
    LOG_ERROR("USB audio IN transmit failed, count=%lu",
              (unsigned long)class_diagnostics.tx_fail_count);
    reported_tx_fail_count = class_diagnostics.tx_fail_count;
  }

  if (class_diagnostics.iso_incomplete_count != reported_iso_incomplete_count)
  {
    LOG_WARN("USB audio isochronous IN incomplete, count=%lu",
             (unsigned long)class_diagnostics.iso_incomplete_count);
    reported_iso_incomplete_count = class_diagnostics.iso_incomplete_count;
  }

  if ((now - last_heartbeat_tick) >= 5000U)
  {
    LOG_DEBUG("USB audio diagnostics: SOF=%lu, TX=%lu, DMA=%lu, errors=%lu",
              (unsigned long)class_diagnostics.sof_count,
              (unsigned long)class_diagnostics.tx_count,
              (unsigned long)dma_count,
              (unsigned long)audio_if_diagnostics.error_count);
    last_heartbeat_tick = now;
  }
#else
  (void)0;
#endif
}

/* USER CODE END PRIVATE_FUNCTIONS_IMPLEMENTATION */

/**
  * @}
  */

/**
  * @}
  */
