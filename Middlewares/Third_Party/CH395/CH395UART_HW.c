/********************************** (C) COPYRIGHT *******************************
 * File Name          : CH395UART.C
 * Author             : WCH
 * Version            : V2.0
 * Date               : 2025/04/02
 * Description        : Hardware abstraction layer for CH395 UART V1.1
 *******************************************************************************/
#include "CH395INC.H"
#include "debug.h"
#if (CH395_OP_INTERFACE_MODE == CH395_UART_MODE)
#define UART_INIT_BAUDRATE 9600 /*The default baudrate of communication is 9600bps, but higher baud rate can be set by command */

/*******************************************************************************
 * Function Name  : CH395_Port_Init
 * Description    : Initialize the UART Port
 * Input          : None
 * Return         : None
 *******************************************************************************/
void CH395_PORT_INIT(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = UART_INIT_BAUDRATE;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

	USART_Init(USART2, &USART_InitStructure);
	USART_Cmd(USART2, ENABLE);
}

/*******************************************************************************
 * Function Name  : Set_MCU_BaudRate
 * Description    : Set baudrate
 * Input          : baudrate
 * Return         : None
 *******************************************************************************/
void Set_MCU_BaudRate(u32 BAUDRATE)
{
	USART_InitTypeDef USART_InitStructure;

	USART_InitStructure.USART_BaudRate = BAUDRATE;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

	USART_Init(USART2, &USART_InitStructure);
}

/*******************************************************************************
 * Function Name  : UARTWriteData
 * Description    : The USART2 sends one byte
 * Input          : UINT8 Data
 * Return         : None
 *******************************************************************************/
void UARTWriteData(UINT8 Data)
{
	USART_SendData(USART2, Data);
	while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)
	{
	}
}

/*******************************************************************************
 * Function Name  : UARTReadData
 * Description    : The USART2 receive one byte
 * Input          : None
 * Return         : UINT8 i
 *******************************************************************************/
UINT8 UARTReadData(void)
{
	UINT8 i;
	while (USART_GetFlagStatus(USART2, USART_FLAG_RXNE) == RESET)
	{
	}
	i = USART_ReceiveData(USART2);
	return i;
}

/********************************************************************************
 * Function Name  : xWriteCH395Cmd
 * Description    : Write CH395 Cmd
 * Input          : UINT8 cmd
 * Output         : None
 * Return         : None
 *******************************************************************************/
void xWriteCH395Cmd(UINT8 cmd)
{
	CMD_START_HANDEL();
	UARTWriteData(SER_SYNC_CODE1); /* Start operation of the first serial port synchronization code */
	UARTWriteData(SER_SYNC_CODE2); /* Start operation of the second serial port synchronization code*/
	UARTWriteData(cmd);			   /* cmd */
}

/********************************************************************************
 * Function Name  : xWriteCH395Data
 * Description    : Write CH395 data
 * Input          : mdata data
 * Output         : None
 * Return         : None
 *******************************************************************************/
void xWriteCH395Data(UINT8 mdata)
{
	UARTWriteData(mdata); /* cmd */
}

/********************************************************************************
 * Function Name  : xReadCH395Data
 * Description    : Read CH395 data
 * Input          : mdata data
 * Output         : None
 * Return         : None
 *******************************************************************************/
UINT8 xReadCH395Data(void)
{
	return UARTReadData();
}

/*******************************************************************************
 * Function Name  : UARTEndCH395Cmd
 * Description    : End CH395 Cmd
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void xEndCH395Cmd(void)
{
	CMD_END_HANDEL();
}
#endif
