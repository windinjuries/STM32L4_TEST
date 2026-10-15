/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32l4xx_it.c
  * @brief   Interrupt Service Routines.
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

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32l4xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "modbus.h"
#include "config.h"
#include "ethernetif.h"
#ifdef CONFIG_USE_LWIP_PORT_CH395Q
#include "ch395_hw.h"
#else
#include "w5500_hw.h"
#endif
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern PCD_HandleTypeDef hpcd_USB_OTG_FS;
extern DMA_HandleTypeDef hdma_sai1_a;
extern DMA_HandleTypeDef hdma_sai1_b;
extern TIM_HandleTypeDef htim3;
extern DMA_HandleTypeDef hdma_usart2_rx;
extern DMA_HandleTypeDef hdma_usart3_rx;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;
extern TIM_HandleTypeDef htim1;

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
  while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  *
  * Diagnostic variant: the stock handler only spins, which discards the cause.
  * The values below are kept as globals so a debugger can read them when the
  * core is halted here. To see them, add them to the Watch window (or use the
  * Live Expressions view) while paused in HardFault_Handler.
  *
  *   hf_stacked_pc    - address of the instruction that faulted
  *   hf_stacked_lr    - return address (the caller)
  *   hf_stacked_psr   - xPSR
  *   hf_cfsr / hf_hfsr / hf_bfar / hf_mmfar - fault status registers
  *
  * Interpretation cheat sheet:
  *   CFSR.PRECISERR set + BFARVALID set -> invalid data access, BFAR holds the
  *   offending address. BFARVALID clear -> the access was to an address the bus
  *   could not report (frequently an unaligned or wild pointer).
  *   CFSR.IBUSERR -> bad instruction fetch (PC ran off into non-code).
  */
volatile uint32_t hf_stacked_r0;
volatile uint32_t hf_stacked_r1;
volatile uint32_t hf_stacked_r2;
volatile uint32_t hf_stacked_r3;
volatile uint32_t hf_stacked_r12;
volatile uint32_t hf_stacked_lr;
volatile uint32_t hf_stacked_pc;
volatile uint32_t hf_stacked_psr;
volatile uint32_t hf_cfsr;
volatile uint32_t hf_hfsr;
volatile uint32_t hf_bfar;
volatile uint32_t hf_mmfar;
volatile uint32_t hf_exc_return;
volatile uint32_t hf_msp;
volatile uint32_t hf_psp;

/* Defined below; invoked from the naked handler entry. */
void HardFault_Decode(uint32_t exc_return, uint32_t msp, uint32_t psp);

/* The frame registers can only be trusted if SP is sampled before the
   compiler emits a prologue, hence the naked entry. */
__attribute__((naked)) void HardFault_Handler(void)
{
  __asm volatile
  (
    "mov  r0, lr\n"        /* EXC_RETURN decides which stack holds the frame */
    "mrs  r1, msp\n"
    "mrs  r2, psp\n"
    "b    HardFault_Decode\n"
  );
}

void HardFault_Decode(uint32_t exc_return, uint32_t msp, uint32_t psp)
{
  uint32_t *frame;

  hf_exc_return = exc_return;
  hf_msp        = msp;
  hf_psp        = psp;

  hf_cfsr  = SCB->CFSR;
  hf_hfsr  = SCB->HFSR;
  hf_bfar  = SCB->BFAR;
  hf_mmfar = SCB->MMFAR;

  /* EXC_RETURN bit 2 == 0 selects the Main stack. Falling back to the Main
     stack when the Process stack pointer is implausible keeps the decode from
     faulting again inside the fault handler. */
  if (((exc_return & 0x4U) != 0U) && (psp >= 0x20000000U) && (psp < 0x20020000U))
  {
    frame = (uint32_t *)psp;
  }
  else
  {
    frame = (uint32_t *)msp;
  }

  hf_stacked_r0  = frame[0];
  hf_stacked_r1  = frame[1];
  hf_stacked_r2  = frame[2];
  hf_stacked_r3  = frame[3];
  hf_stacked_r12 = frame[4];
  hf_stacked_lr  = frame[5];
  hf_stacked_pc  = frame[6];
  hf_stacked_psr = frame[7];

  while (1)
  {
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Prefetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/******************************************************************************/
/* STM32L4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32l4xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles EXTI line3 interrupt.
  */
void EXTI3_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI3_IRQn 0 */

  /* USER CODE END EXTI3_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3);
  /* USER CODE BEGIN EXTI3_IRQn 1 */

  /* USER CODE END EXTI3_IRQn 1 */
}

/**
  * @brief This function handles DMA1 channel3 global interrupt.
  */
void DMA1_Channel3_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel3_IRQn 0 */

  /* USER CODE END DMA1_Channel3_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_usart3_rx);
  /* USER CODE BEGIN DMA1_Channel3_IRQn 1 */

  /* USER CODE END DMA1_Channel3_IRQn 1 */
}

/**
  * @brief This function handles DMA1 channel6 global interrupt.
  */
void DMA1_Channel6_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel6_IRQn 0 */

  /* USER CODE END DMA1_Channel6_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_usart2_rx);
  /* USER CODE BEGIN DMA1_Channel6_IRQn 1 */

  /* USER CODE END DMA1_Channel6_IRQn 1 */
}

/**
  * @brief This function handles TIM1 update interrupt and TIM16 global interrupt.
  */
void TIM1_UP_TIM16_IRQHandler(void)
{
  /* USER CODE BEGIN TIM1_UP_TIM16_IRQn 0 */

  /* USER CODE END TIM1_UP_TIM16_IRQn 0 */
  HAL_TIM_IRQHandler(&htim1);
  /* USER CODE BEGIN TIM1_UP_TIM16_IRQn 1 */

  /* USER CODE END TIM1_UP_TIM16_IRQn 1 */
}

/**
  * @brief This function handles TIM3 global interrupt.
  */
void TIM3_IRQHandler(void)
{
  /* USER CODE BEGIN TIM3_IRQn 0 */
  /* USER CODE END TIM3_IRQn 0 */
  HAL_TIM_IRQHandler(&htim3);
  /* USER CODE BEGIN TIM3_IRQn 1 */

  /* USER CODE END TIM3_IRQn 1 */
}

/**
  * @brief This function handles USART1 global interrupt.
  */
void USART1_IRQHandler(void)
{
  /* USER CODE BEGIN USART1_IRQn 0 */

#if (CONFIG_USE_FREEMASTER == 1)
  	extern void FMSTR_SerialIsr();
    FMSTR_SerialIsr();
#endif

  /* USER CODE END USART1_IRQn 0 */
  HAL_UART_IRQHandler(&huart1);
  /* USER CODE BEGIN USART1_IRQn 1 */

  /* USER CODE END USART1_IRQn 1 */
}

/**
  * @brief This function handles USART2 global interrupt.
  */
void USART2_IRQHandler(void)
{
  /* USER CODE BEGIN USART2_IRQn 0 */

  /* USER CODE END USART2_IRQn 0 */
  HAL_UART_IRQHandler(&huart2);
  /* USER CODE BEGIN USART2_IRQn 1 */

  /* USER CODE END USART2_IRQn 1 */
}

/**
  * @brief This function handles USART3 global interrupt.
  */
void USART3_IRQHandler(void)
{
  /* USER CODE BEGIN USART3_IRQn 0 */

  /* USER CODE END USART3_IRQn 0 */
  HAL_UART_IRQHandler(&huart3);
  /* USER CODE BEGIN USART3_IRQn 1 */
  if (__HAL_UART_GET_FLAG(&huart3, UART_FLAG_IDLE))
  {
    __HAL_UART_CLEAR_IDLEFLAG(&huart3);
    HAL_UART_DMAStop(&huart3);
    uint16_t rx_cnt = MODBUS_RX_BUF_SIZE - __HAL_DMA_GET_COUNTER(&hdma_usart3_rx);
    modbus_rx_notify(rx_cnt);
  }
  /* USER CODE END USART3_IRQn 1 */
}

/**
  * @brief This function handles DMA2 channel1 global interrupt.
  */
void DMA2_Channel1_IRQHandler(void)
{
  /* USER CODE BEGIN DMA2_Channel1_IRQn 0 */

  /* USER CODE END DMA2_Channel1_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_sai1_a);
  /* USER CODE BEGIN DMA2_Channel1_IRQn 1 */

  /* USER CODE END DMA2_Channel1_IRQn 1 */
}

/**
  * @brief This function handles DMA2 channel2 global interrupt.
  */
void DMA2_Channel2_IRQHandler(void)
{
  /* USER CODE BEGIN DMA2_Channel2_IRQn 0 */

  /* USER CODE END DMA2_Channel2_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_sai1_b);
  /* USER CODE BEGIN DMA2_Channel2_IRQn 1 */

  /* USER CODE END DMA2_Channel2_IRQn 1 */
}

/**
  * @brief This function handles USB OTG FS global interrupt.
  */
void OTG_FS_IRQHandler(void)
{
  /* USER CODE BEGIN OTG_FS_IRQn 0 */

  /* USER CODE END OTG_FS_IRQn 0 */
  HAL_PCD_IRQHandler(&hpcd_USB_OTG_FS);
  /* USER CODE BEGIN OTG_FS_IRQn 1 */

  /* USER CODE END OTG_FS_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/**
  * @brief  EXTI line detection callback (W5500 interrupt).
  * @param  GPIO_Pin: Pin number that triggered the interrupt
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
#if (CONFIG_MODULE_NETWORK_ENABLE == 1)
#ifdef CONFIG_USE_LWIP_PORT_CH395Q
    if (GPIO_Pin == CH395_INT_Pin)
#else
    if (GPIO_Pin == W5500_INT_Pin)
#endif
    {
        ethernetif_notify_rx();
    }
#endif

}

/* USER CODE END 1 */
