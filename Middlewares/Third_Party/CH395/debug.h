/********************************** (C) COPYRIGHT  *******************************
 * File Name          : debug.h
 * Author             : WCH
 * Version            : V2.0
 * Date               : 2025/04/02
 * Description        : This file contains all the functions prototypes for UART
 *                      Printf , Delay functions.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
 #ifndef __DEBUG_H
 #define __DEBUG_H
 
 #ifdef __cplusplus
 extern "C"
 {
 #endif
 
 #include "stdio.h"
 #include "stdint.h"
//  #include "FreeRTOS.h"
//  #include "semphr.h"
//  extern SemaphoreHandle_t xCH395cmd_Mutex;
 
 #ifndef UINT8
     typedef unsigned char UINT8;
 #endif
 #ifndef UINT16
     typedef unsigned short UINT16;
 #endif
 #ifndef UINT32
     typedef unsigned long UINT32;
 #endif

 #ifndef u16
    typedef unsigned short u16;
 #endif
 #ifndef u32
    typedef unsigned long u32;
 #endif
 #ifndef u8
    typedef unsigned char u8;
 #endif
 
//  /* UART Printf Definition */
//  #define DEBUG_UART1 1
//  #define DEBUG_UART2 2
//  #define DEBUG_UART3 3
 
//  /* DEBUG UATR Definition */
//  #ifndef DEBUG
//  #define DEBUG DEBUG_UART1
//  #endif
 
//  #define USE_FREERTOS 
 /* CH395 command operations must be atomic to prevent command frame sequence disruptions.
 If a command is interrupted by other tasks during execution, it may cause communication issues.
 When using an RTOS or similar operating system, it is recommended to add appropriate protection here,
 such as using mutexes or critical sections, to ensure the integrity of command operations. */
//  #ifdef USE_FREERTOS 
//  #define CMD_START_HANDEL() xSemaphoreTake(xCH395cmd_Mutex, portMAX_DELAY)
//  #define CMD_END_HANDEL() xSemaphoreGive(xCH395cmd_Mutex)
//  #else
//  #define CMD_START_HANDEL()
//  #define CMD_END_HANDEL()
//  #endif
 
    //  void Delay_Init(void);
    void Delay_Us(uint32_t n);
    void Delay_Ms(uint32_t n);    
    //  void USART_Printf_Init(uint32_t baudrate);
 
     
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* __DEBUG_H */
 