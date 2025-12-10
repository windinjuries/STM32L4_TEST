/*
 * Copyright 2024 NXP
 *
 * License: NXP LA_OPT_NXP_Software_License
 *
 * NXP Confidential and Proprietary. This software is owned or controlled
 * by NXP and may only be used strictly in accordance with the applicable
 * license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are
 * agreeing that you have read, and that you agree to comply with and are
 * bound by, such license terms. If you do not agree to be bound by the
 * applicable license terms, then you may not retain, install, activate
 * or otherwise use the software.
 *
 * FreeMASTER Communication Driver - Serial Communication Interface
 */
#include "freemaster.h"
#include "freemaster_private.h"
#include "stm32l4xx_hal.h"

/* Compile this code only if the SERIAL driver is selected in freemaster_cfg.h. */
#if defined(FMSTR_SERIAL_DRV) && (FMSTR_MK_IDSTR(FMSTR_SERIAL_DRV) == FMSTR_SERIAL_STM32L4_UART)
#if !(FMSTR_DISABLE)

#include "freemaster_serial.h"

/***********************************
*  local function prototypes
***********************************/

/* Interface function - Initialization of SCI driver adapter */
static FMSTR_BOOL _FMSTR_S32_Init(void);
static void _FMSTR_S32_EnableTransmit(FMSTR_BOOL enable);
static void _FMSTR_S32_EnableReceive(FMSTR_BOOL enable);
static void _FMSTR_S32_EnableTransmitInterrupt(FMSTR_BOOL enable);
static void _FMSTR_S32_EnableTransmitCompleteInterrupt(FMSTR_BOOL enable);
static void _FMSTR_S32_EnableReceiveInterrupt(FMSTR_BOOL enable);
static FMSTR_BOOL _FMSTR_S32_IsTransmitRegEmpty(void);
static FMSTR_BOOL _FMSTR_S32_IsReceiveRegFull(void);
static FMSTR_BOOL _FMSTR_S32_IsTransmitterActive(void);
static void _FMSTR_S32_PutChar(FMSTR_BCHR ch);
static FMSTR_BCHR _FMSTR_S32_GetChar(void);
static void _FMSTR_S32_Flush(void);

/***********************************
*  global variables
***********************************/
/* Interface of this SCI driver */

const FMSTR_SERIAL_DRV_INTF FMSTR_SERIAL_STM32L4_UART =
{
    .Init                            = _FMSTR_S32_Init,
    .EnableTransmit                  = _FMSTR_S32_EnableTransmit,
    .EnableReceive                   = _FMSTR_S32_EnableReceive,
    .EnableTransmitInterrupt         = _FMSTR_S32_EnableTransmitInterrupt,
    .EnableTransmitCompleteInterrupt = _FMSTR_S32_EnableTransmitCompleteInterrupt,
    .EnableReceiveInterrupt          = _FMSTR_S32_EnableReceiveInterrupt,
    .IsTransmitRegEmpty              = _FMSTR_S32_IsTransmitRegEmpty,
    .IsReceiveRegFull                = _FMSTR_S32_IsReceiveRegFull,
    .IsTransmitterActive             = _FMSTR_S32_IsTransmitterActive,
    .PutChar                         = _FMSTR_S32_PutChar,
    .GetChar                         = _FMSTR_S32_GetChar,
    .Flush                           = _FMSTR_S32_Flush,
};

/****************************************************************************************
* LPUART module constants
*****************************************************************************************/
/* LPUART module registers */

extern UART_HandleTypeDef huart1;

/**************************************************************************//*!
*
* @brief    SCI communication initialization
*
******************************************************************************/

uint8_t received_data = 0;
static FMSTR_BOOL _FMSTR_S32_Init(void)
{

    return FMSTR_TRUE;
}


/**************************************************************************//*!
*
* @brief    Enable/Disable LPUART transmitter
*
******************************************************************************/

static void _FMSTR_S32_EnableTransmit(FMSTR_BOOL enable)
{
    if(enable)
    {
        /* Enable transmitter */
        huart1.Instance->CR1 |= USART_CR1_TE;
    }
    else
    {
        huart1.Instance->CR1 &= ~USART_CR1_TE;
        /* Disable transmitter */
    }
}

/**************************************************************************//*!
*
* @brief    Enable/Disable LPUART receiver
*
******************************************************************************/

static void _FMSTR_S32_EnableReceive(FMSTR_BOOL enable)
{
    if(enable)
    {   
        /* Enable receiver (enables single-wire connection) */
        huart1.Instance->CR1 |= USART_CR1_RE;
    }
    else
    {
        /* Disable receiver */
        huart1.Instance->CR1 &= ~USART_CR1_RE;
    }
}

/**************************************************************************//*!
*
* @brief    Enable/Disable interrupt from transmit register empty event
*
******************************************************************************/

static void _FMSTR_S32_EnableTransmitInterrupt(FMSTR_BOOL enable)
{
    if(enable)
    {
        /* Enable interrupt */
        huart1.Instance->CR1 |= USART_CR1_TXEIE;
    }
    else
    {
        /* Disable interrupt */
        huart1.Instance->CR1 &= ~USART_CR1_TXEIE;
    }
}

/**************************************************************************//*!
*
* @brief    Enable/Disable interrupt when transmission is complete
*
******************************************************************************/

static void _FMSTR_S32_EnableTransmitCompleteInterrupt(FMSTR_BOOL enable)
{
    if(enable)
    {
        /* Enable interrupt */
        huart1.Instance->CR1 |= USART_CR1_TCIE;
    }
    else
    {
        /* Disable interrupt */
        huart1.Instance->CR1 &= ~USART_CR1_TCIE;
    }
}

/**************************************************************************//*!
*
* @brief    Enable/Disable interrupt from receive register full event
*
******************************************************************************/

static void _FMSTR_S32_EnableReceiveInterrupt(FMSTR_BOOL enable)
{
    if(enable)
    {
        /* Enable interrupt */
        huart1.Instance->CR1 |= USART_CR1_RXNEIE;
    }
    else
    {
        /* Disable interrupt */
        huart1.Instance->CR1 &= ~USART_CR1_RXNEIE;
    }
}

/**************************************************************************//*!
*
* @brief    Returns TRUE if the transmit register is empty, and it's possible to put next char
*
******************************************************************************/

static FMSTR_BOOL _FMSTR_S32_IsTransmitRegEmpty(void)
{
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_TXE)) 
    {
        return FMSTR_TRUE;
    }
    else
    {   
        return FMSTR_FALSE;
    }
}

/**************************************************************************//*!
*
* @brief    Returns TRUE if the receive register is full, and it's possible to get received char
*
******************************************************************************/

static FMSTR_BOOL _FMSTR_S32_IsReceiveRegFull(void)
{
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE)) 
    {
        return FMSTR_TRUE;
    }
    else
    {   
        return FMSTR_FALSE;
    }
}

/**************************************************************************//*!
*
* @brief    Returns TRUE if the transmitter is still active
*
******************************************************************************/

static FMSTR_BOOL _FMSTR_S32_IsTransmitterActive(void)
{
    /* 0 - Transmission in progress, 1 - No transmission in progress */
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_TC)) 
    {
        return FMSTR_TRUE;
    }
    else
    {   
        return FMSTR_FALSE;
    }
}

/**************************************************************************//*!
*
* @brief    The function puts the char for transmit
*
******************************************************************************/

static void _FMSTR_S32_PutChar(FMSTR_BCHR  ch)
{
    huart1.Instance->TDR = (uint8_t)ch;
}

/**************************************************************************//*!
*
* @brief    The function gets the received char
*
******************************************************************************/
static FMSTR_BCHR _FMSTR_S32_GetChar(void)
{
    FMSTR_BCHR c = 0;
    huart1.Instance->RDR = (uint8_t)c;
    return c;
}

/**************************************************************************//*!
*
* @brief    The function sends buffered data
*
******************************************************************************/

static void _FMSTR_S32_Flush(void)
{
}

/**************************************************************************//*!
*
* @brief    Assign FreeMASTER communication module base address
*
******************************************************************************/

void FMSTR_SerialSetBaseAddress(FMSTR_ADDR base)
{
    // fmstr_LPUARTBaseAddr = base;
}

/**************************************************************************//*!
*
* @brief    Process FreeMASTER serial interrupt (call this function from SCI ISR)
*
******************************************************************************/

void FMSTR_SerialIsr()
{
    /* process incoming or just transmitted byte */
    #if (FMSTR_LONG_INTR) || (FMSTR_SHORT_INTR)
        FMSTR_ProcessSerial();
    #endif
}

#else /* !(FMSTR_DISABLE) */

/* Empty API functions when FMSTR_DISABLE is set */
void FMSTR_SerialSetBaseAddress(FMSTR_ADDR base)
{
    FMSTR_UNUSED(base);
}

void FMSTR_SerialIsr()
{
}

#endif /* !(FMSTR_DISABLE) */
#endif /* defined(FMSTR_SERIAL_DRV) && (FMSTR_MK_IDSTR(FMSTR_SERIAL_DRV) == FMSTR_SERIAL_S32_LPUART_ID) */
