/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usb_device.c
  * @brief          : This file implements the USB Device
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* ============================================================================
 * This file is maintained by this project (CubeMX no longer generates it:
 * USB_DEVICE has been removed from TEST.ioc).
 *
 * The device enumerates as a USB composite device built by the official
 * ST CompositeBuilder:
 *
 *   classId 0 : Audio  (custom UAC1 microphone, ES8388 via SAI/DMA)
 *               interface 0 = AudioControl, interface 1 = AudioStreaming
 *               endpoint 0x81 isochronous IN
 *   classId 1 : CDC    (virtual COM port, official ST CDC class)
 *               interface 2 = CDC Control,   interface 3 = CDC Data
 *               endpoints 0x83 (INTR IN), 0x01 (BULK OUT), 0x82 (BULK IN)
 *
 * Interface numbers and the configuration descriptor are produced at run time
 * by usbd_composite_builder.c; the endpoint addresses below are the input to
 * that builder and must stay in sync with the FIFO allocation in
 * USB_DEVICE/Target/usbd_conf.c and the define overrides in user.cmake.
 * ==========================================================================*/

/* Includes ------------------------------------------------------------------*/

#include "usb_device.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_audio.h"
#include "usbd_audio_if.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"
#include "usbd_composite_builder.h"

/* USER CODE BEGIN Includes */
#include "config.h"
#if (CONFIG_USE_FREEMASTER == 0)
#include "debug_log.h"
#endif
/* USER CODE END Includes */

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/**
  * Endpoint addresses handed to the CompositeBuilder.
  *
  * Ordering matters: the builder consumes them per class in the order its
  * descriptor code expects.
  *   Audio : Eps[0] = isochronous IN  (AUDIO_IN_EP)
  *   CDC   : Eps[0] = bulk OUT, Eps[1] = bulk IN, Eps[2] = interrupt IN
  */
static uint8_t CdcEpAdd[3] = {CDC_OUT_EP, CDC_IN_EP, CDC_CMD_EP};
static uint8_t AudioEpAdd[1] = {AUDIO_IN_EP};

/* USER CODE END PV */

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USB Device Core handle declaration. */
USBD_HandleTypeDef hUsbDeviceFS;
extern USBD_DescriptorsTypeDef FS_Desc;

/*
 * -- Insert your variables declaration here --
 */
/* USER CODE BEGIN 0 */
extern PCD_HandleTypeDef hpcd_USB_OTG_FS;
/* USER CODE END 0 */

/*
 * -- Insert your external function declaration here --
 */
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/**
  * Init USB device Library, add supported classes and start the library
  * @retval None
  */
void MX_USB_DEVICE_Init(void)
{
  /* USER CODE BEGIN USB_DEVICE_Init_PreTreatment */

  /* USER CODE END USB_DEVICE_Init_PreTreatment */

  /* Init Device Library. */
  if (USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS) != USBD_OK)
  {
    Error_Handler();
  }

  /* The composite builder owns the configuration descriptor: the class held
     by the core is USBD_CMPSIT, whose Init/Setup/DataIn entries are NULL. */
  if (USBD_RegisterClass(&hUsbDeviceFS, &USBD_CMPSIT) != USBD_OK)
  {
    Error_Handler();
  }

  /* Add the Audio function (custom UAC1 microphone). Becomes classId 0. */
  if (USBD_RegisterClassComposite(&hUsbDeviceFS, &USBD_AUDIO,
                                  CLASS_TYPE_AUDIO, AudioEpAdd) != USBD_OK)
  {
    Error_Handler();
  }

  /* Add the CDC function (virtual COM port). Becomes classId 1. */
  if (USBD_RegisterClassComposite(&hUsbDeviceFS, &USBD_CDC,
                                  CLASS_TYPE_CDC, CdcEpAdd) != USBD_OK)
  {
    Error_Handler();
  }

  /* Register the per-class callback tables.
     The class drivers' RegisterInterface() helpers would store them at
     pUserData[pdev->classId], but classId is left on the last registered class,
     so the tables would land one slot too high. Do NOT "fix" this by assigning
     pdev->classId before each call: USBD_RegisterClassComposite() relies on its
     own classId++ to pick the next slot for both pClass[] and tclasslist[].
     Overwriting classId makes the second AddClass() see an already active slot
     and silently skip the whole class (which is exactly what happened before).

     The two classes were registered in order, so their indices are known. */
  hUsbDeviceFS.pUserData[0] = &USBD_AUDIO_fops_FS;
  hUsbDeviceFS.pUserData[1] = &USBD_CDC_fops_FS;

  /* Leave classId on a valid class; USBD_SetClassConfig() reassigns it for
     every class while processing SET_CONFIGURATION. */
  hUsbDeviceFS.classId = 0U;

  /* Do NOT rewrite hUsbDeviceFS.NumClasses here. USBD_RegisterClass() counts
     the USBD_CMPSIT holder and each USBD_RegisterClassComposite() counts its
     class, leaving the counter at 3 for two functions. That is the value the
     library expects: USBD_CMPSIT_FindFreeIFNbr() walks tclasslist[] up to
     NumClasses to allocate interface numbers, so lowering the count would make
     it skip a slot. */

  if (USBD_Start(&hUsbDeviceFS) != USBD_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN USB_DEVICE_Init_PostTreatment */
  hpcd_USB_OTG_FS.Init.Sof_enable = ENABLE;
  USB_OTG_FS->GINTMSK |= USB_OTG_GINTMSK_SOFM;
#if (CONFIG_USE_FREEMASTER == 0)
  if ((USB_OTG_FS->GINTMSK & USB_OTG_GINTMSK_SOFM) == 0U)
  {
    LOG_ERROR("USB SOF interrupt enable failed");
  }
  else
  {
    LOG_INFO("USB composite device started: Audio (48kHz mic) + CDC (VCP)");
  }
#endif
  /* USER CODE END USB_DEVICE_Init_PostTreatment */
}

/**
  * @}
  */

/**
  * @}
  */
