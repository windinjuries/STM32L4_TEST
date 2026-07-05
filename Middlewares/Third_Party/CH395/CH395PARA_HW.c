/********************************** (C) COPYRIGHT *******************************
 * File Name          : CH395SPI_HW.C
 * Author             : WCH
 * Version            : V2.0
 * Date               : 2025/04/02
 * Description        : Hardware abstraction layer for CH395 parallel port V1.1
 *
 *******************************************************************************/

/* Hardware-related macro definitions. */
/* The hardware connection in this example is as follows (the actual application circuit can refer to and modify the following definitions and subroutines). */
/*  Pins of the F103     Pins of the CH395
        PD14                D0
        PD15                D1
        PD0                 D2
        PD1                 D3
        PE7                 D4
        PE8                 D5
        PE9                 D6
        PE10                D7
        PD11                A0
        PD4                 RD#
        PD5                 WR#
        PD7                 PCS#

    If only the CH395 is connected to the parallel port, the CS# can be directly tied to low level to force chip selection. */

#include "CH395INC.H"
#include "debug.h"
#if (CH395_OP_INTERFACE_MODE == CH395_PARA_MODE)

#define CH395_CMD_PORT ((u32)0x60010000) /* It refers to the assumed I/O address of the CH395 command port. */
#define CH395_DAT_PORT ((u32)0x60080000) /* It refers to the assumed I/O address of the CH395 data port. */

/*******************************************************************************
 * Function Name  : CH395_FSMC_PORT_INIT
 * Description    : Initializes the port I/O
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH395_FSMC_PORT_INIT(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14 | GPIO_Pin_15 | GPIO_Pin_0 | GPIO_Pin_1; // D0 D1 D2 D3
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10; // D4 D5 D6 D7
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOE, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_7; // RD# WR# PCS#
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11; // A0
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
}

/*******************************************************************************
 * Function Name  : CH395_FSMC_Config
 * Description    : Initializes the parallel port
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH395_FSMC_Config(void)
{
    FSMC_NORSRAMInitTypeDef FSMC_NORSRAMInitStructure;
    FSMC_NORSRAMTimingInitTypeDef readWriteTiming;

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, ENABLE);

    readWriteTiming.FSMC_AddressSetupTime = 3;
    readWriteTiming.FSMC_DataSetupTime = 8;
    readWriteTiming.FSMC_AccessMode = FSMC_AccessMode_A;
    readWriteTiming.FSMC_AddressHoldTime = 0x00;
    readWriteTiming.FSMC_BusTurnAroundDuration = 0;
    readWriteTiming.FSMC_CLKDivision = 0;
    readWriteTiming.FSMC_DataLatency = 0;

    FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM1;
    FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable;
    FSMC_NORSRAMInitStructure.FSMC_MemoryType = FSMC_MemoryType_NOR;
    FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_8b;
    FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode = FSMC_BurstAccessMode_Disable;
    FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
    FSMC_NORSRAMInitStructure.FSMC_AsynchronousWait = FSMC_AsynchronousWait_Disable;
    FSMC_NORSRAMInitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;
    FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;
    FSMC_NORSRAMInitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;
    FSMC_NORSRAMInitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;
    FSMC_NORSRAMInitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Disable;
    FSMC_NORSRAMInitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable;
    FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &readWriteTiming;
    FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &readWriteTiming;

    FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure);
    FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM1, ENABLE);
}

/*******************************************************************************
 * Function Name  : CH395_PORT_INIT
 * Description    : Hardware initialization
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH395_PORT_INIT(void)
{
    CH395_FSMC_PORT_INIT();
    CH395_FSMC_Config();
}

/********************************************************************************
 * Function Name  : xWriteCH395Cmd
 * Description    : Write CH395 Cmd
 * Input          : UINT8 cmd
 * Output         : None
 * Return         : None
 *******************************************************************************/
void xWriteCH395Cmd(u8 cmd)
{
    CMD_START_HANDEL();
    *(__IO u8 *)(CH395_CMD_PORT) = cmd;
    Delay_Us(1); /*Between the command code and data transmissions, a TSC time delay is required.*/
}

/********************************************************************************
 * Function Name  : xWriteCH395Data
 * Description    : Write CH395 Data
 * Input          : UINT8 mdata
 * Output         : None
 * Return         : None
 *******************************************************************************/
void xWriteCH395Data(u8 mdata)
{
    *(__IO u8 *)(CH395_DAT_PORT) = mdata;
    Delay_Us(1); /*Between the data and data transmissions, a TSD time delay is required.*/
}

/*******************************************************************************
 * Function Name  : CH395_PORT_INIT
 * Description    : Read CH395 Data
 * Input          : None
 * Output         : None
 * Return         : UINT8 i
 *******************************************************************************/
u8 xReadCH395Data(void)
{
    u8 i;
    i = (*(__IO u8 *)(CH395_DAT_PORT));
    Delay_Us(1);
    return i;
}

/*******************************************************************************
 * Function Name  : PARAEndCH395Cmd
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
