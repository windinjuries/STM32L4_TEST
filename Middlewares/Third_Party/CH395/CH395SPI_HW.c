/********************************** (C) COPYRIGHT *******************************
 * File Name          : CH395SPI_HW.C
 * Author             : WCH
 * Version            : V2.0
 * Date               : 2025/04/02
 * Description        : Hardware abstraction layer for CH395 SPI V1.1
 *******************************************************************************/

#include "CH395INC.H"
#include "CH395_hw.h"
#include "debug.h"

#define Delay_Us(x) ch395_delay_us(x)


/*******************************************************************************
 * Function Name  : xEndCH395Cmd
 * Description    : End CH395 Cmd
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void xEndCH395Cmd(void)
{
    ch395_cs_deselect();
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
    ch395_cs_select();
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
    ch395_spi_swapbyte(cmd); /* The SPI sends the command code */

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
    ch395_spi_swapbyte(mdata); /* The SPI sends data  */

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
    data = ch395_spi_swapbyte(0x00); /* SPI read Data*/

    /*Between the data and data transmissions, a TSD time delay is required.*/
    Delay_Us(1);
    return data;
}