/********************************** (C) COPYRIGHT *******************************
 * File Name          : CH395SPI_HW.C
 * Author             : WCH
 * Version            : V2.0
 * Date               : 2025/04/02
 * Description        : Hardware abstraction layer for CH395 SPI V1.1
 *******************************************************************************/

#include "CH395INC.H"
#include "stm32f10x_spi.h"
#include "stm32f10x_dma.h"
#include "debug.h"
#if (CH395_OP_INTERFACE_MODE == CH395_SPI_MODE)
/*******************************************************************************
 * Function Name  : CH395_Port_Init
 * Description    : Initializes the spi port
 * Input          : None
 * Return         : None
 *******************************************************************************/
void CH395_PORT_INIT(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef SPI_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1, ENABLE);

    // CS
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_SetBits(GPIOA, GPIO_Pin_2);

    // CLK
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // MISO
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // MOSI
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SPI1, &SPI_InitStructure);
    SPI_Cmd(SPI1, ENABLE);
}

/*******************************************************************************
 * Function Name  : xEndCH395Cmd
 * Description    : End CH395 Cmd
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void xEndCH395Cmd(void)
{
    GPIO_WriteBit(GPIOA, GPIO_Pin_2, Bit_SET);
    CMD_END_HANDEL();
}

/*******************************************************************************
 * Function Name  : xCH395CmdStart
 * Description    : End CH395 Cmd
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void xCH395CmdStart(void)
{
    CMD_START_HANDEL();
    GPIO_WriteBit(GPIOA, GPIO_Pin_2, Bit_RESET);
}

/*******************************************************************************
 * Function Name  : SPI1_WriteRead
 * Description    : SPI read and write a byte
 * Input          : Write Data
 * Return         : Read data
 *******************************************************************************/
UINT8 SPI1_WriteRead(UINT8 data)
{
    UINT8 retry = 0;

    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET)
    {
        retry++;
        if (retry > 200)
            return 0;
    }
    SPI_I2S_SendData(SPI1, data);
    retry = 0;

    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET)
    {
        retry++;
        if (retry > 200)
            return 0;
    }
    return SPI_I2S_ReceiveData(SPI1);
}

/******************************************************************************
 * Function Name  : xWriteCH395Cmd
 * Description    : Write CH395 Cmd
 * Input          : UINT8 cmd
 * Output         : None
 * Return         : None
 *******************************************************************************/
void xWriteCH395Cmd(UINT8 cmd)
{
    xCH395CmdStart();    /* Command starts, CS pull down */
    SPI1_WriteRead(cmd); /* The SPI sends the command code */

    /*Between the command code and data transmissions, a TSC time delay is required.*/
    Delay_Us(1);
}

/******************************************************************************
 * Function Name  : xWriteCH395Data
 * Description    : Write CH395 Data
 * Input          : UINT8 mdata
 * Output         : None
 * Return         : None
 *******************************************************************************/
void xWriteCH395Data(UINT8 mdata)
{
    SPI1_WriteRead(mdata); /* The SPI sends data  */

    /*Between the data and data transmissions, a TSD time delay is required.*/
    Delay_Us(1);
}

/*******************************************************************************
 * Function Name  : xReadCH395Data
 * Description    : Read CH395 Data
 * Input          : None
 * Output         : None
 * Return         : UINT8 i
 *******************************************************************************/
UINT8 xReadCH395Data(void)
{
    UINT8 data;
    data = SPI1_WriteRead(0x00); /* SPI read Data*/

    /*Between the data and data transmissions, a TSD time delay is required.*/
    Delay_Us(1);
    return data;
}

/*******************************************************************************
 * Function Name  : DMA_Tx_Init
 * Description    : SPI DMA Tx Init
 * Input          : DMA_CHx   - select the DMA Channel
                    ppadr     - Register address
                    memadr    - Memory address
                    bufsize   - Buffer size
 * Output         : None
 * Return         : None
 *******************************************************************************/
void DMA_Tx_Init(DMA_Channel_TypeDef *DMA_CHx, u32 ppadr, u32 memadr, u16 bufsize)
{
    DMA_InitTypeDef DMA_InitStructure = {0};

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    DMA_DeInit(DMA_CHx);

    DMA_InitStructure.DMA_PeripheralBaseAddr = ppadr;
    DMA_InitStructure.DMA_MemoryBaseAddr = memadr;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize = bufsize;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA_CHx, &DMA_InitStructure);
}

/*******************************************************************************
 * Function Name  : DMA_Rx_Init
 * Description    : SPI DMA Rx Init
 * Input          : DMA_CHx  - select the DMA Channel
                    ppadr    - Register address
                    memadr   - Memory address
                    bufsize  - Buffer size
 * Output         : None
 * Return         : None
 *******************************************************************************/
void DMA_Rx_Init(DMA_Channel_TypeDef *DMA_CHx, u32 ppadr, u32 memadr, u16 bufsize)
{
    DMA_InitTypeDef DMA_InitStructure = {0};

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    DMA_DeInit(DMA_CHx);

    DMA_InitStructure.DMA_PeripheralBaseAddr = ppadr;
    DMA_InitStructure.DMA_MemoryBaseAddr = memadr;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = bufsize;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA_CHx, &DMA_InitStructure);
}
#endif
