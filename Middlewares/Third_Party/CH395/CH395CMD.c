/********************************** (C) COPYRIGHT *******************************
 * File Name          : CH395CMD.C
 * Author             : WCH
 * Version            : V2.0
 * Date               : 2025/04/02
 * Description        : CH395 command interface file
 *******************************************************************************/
#include "CH395cmd.h"
#include "CH395INC.h"
#include "debug.h"

/********************************************************************************
 * Function Name  : CH395Reset
 * Description    : Reset the CH395
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH395Reset(void)
{
    GPIO_WriteBit(GPIOA, GPIO_Pin_4, Bit_RESET);
    Delay_Ms(10);
    GPIO_WriteBit(GPIOA, GPIO_Pin_4, Bit_SET);
}

/********************************************************************************
 * Function Name  : CH395CMDGetVer
 * Description    : Obtain the chip and firmware version number
 * Input          : None
 * Output         : None
 * Return         : Version number
 *******************************************************************************/
UINT8 CH395CMDGetVer(void)
{
    UINT8 i;
    xWriteCH395Cmd(CMD01_GET_IC_VER);
    i = xReadCH395Data();
    xEndCH395Cmd();
    return i;
}

/********************************************************************************
 * Function Name  : CH395CMDSetUartBaudRate
 * Description    : Set the baudrate for serial port communication
 * Input          : baudrate
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH395CMDSetUartBaudRate(UINT32 baudrate)
{
    xWriteCH395Cmd(CMD31_SET_BAUDRATE);
    xWriteCH395Data((UINT8)baudrate);
    xWriteCH395Data((UINT8)((u16)baudrate >> 8));
    xWriteCH395Data((UINT8)(baudrate >> 16));
    xEndCH395Cmd();
}

/********************************************************************************
 * Function Name  : CH395CMDEnterSleep
 * Description    : Enter low-power sleep suspended state
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH395CMDEnterSleep(void)
{
    xWriteCH395Cmd(CMD00_ENTER_SLEEP);
    xEndCH395Cmd();
}

/********************************************************************************
 * Function Name  : CH395CMDReset
 * Description    : Reset the CH395
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH395CMDReset(void)
{
    xWriteCH395Cmd(CMD00_RESET_ALL);
    xEndCH395Cmd();
}

/********************************************************************************
 * Function Name  : CH395CMDCheckExist
 * Description    : Test communication interface and working condition
 * Input          : testdata
 * Output         : None
 * Return         : bitwise inverse of input data
 *******************************************************************************/
UINT8 CH395CMDCheckExist(UINT8 testdata)
{
    UINT8 i;
    xWriteCH395Cmd(CMD11_CHECK_EXIST);
    xWriteCH395Data(testdata);
    i = xReadCH395Data();
    xEndCH395Cmd();
    return i;
}

/*******************************************************************************
 * Function Name  : CH395CMDGetGlobIntStatus
 * Description    : Gets the CH395 global interrupt status
 * Input          : None
 * Output         : None
 * Return         : Global interrupt status
 *******************************************************************************/
u16 CH395CMDGetGlobIntStatus(void)
{
    u16 init_status;
    xWriteCH395Cmd(CMD02_GET_GLOB_INT_STATUS_ALL);
    init_status = xReadCH395Data();
    init_status |= (UINT16)xReadCH395Data() << 8;
    xEndCH395Cmd();
    return init_status;
}

/*******************************************************************************
 * Function Name  : CH395CMDSetPHY
 * Description    : Set the PHY mode
 * Input          : PHY mode
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH395CMDSetPHY(UINT8 PhyMode)
{
    xWriteCH395Cmd(CMD10_SET_PHY);
    xWriteCH395Data(PhyMode);
    xEndCH395Cmd();
}

/********************************************************************************
 * Function Name  : CH395CMDSetMACAddr
 * Description    : Set MAC address
 * Input          : macaddr
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH395CMDSetMACAddr(UINT8 *macaddr)
{
    UINT8 i;
    xWriteCH395Cmd(CMD60_SET_MAC_ADDR);
    for (i = 0; i < 6; i++)
        xWriteCH395Data(*macaddr++);
    xEndCH395Cmd();
}

/********************************************************************************
 * Function Name  : CH395CMDSetIPAddr
 * Description    : Set IP address
 * Input          : ipaddr
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH395CMDSetIPAddr(UINT8 *ipaddr)
{
    UINT8 i;
    xWriteCH395Cmd(CMD40_SET_IP_ADDR);
    for (i = 0; i < 4; i++)
        xWriteCH395Data(*ipaddr++);
    xEndCH395Cmd();
}

/********************************************************************************
 * Function Name  : CH395CMDSetGWIPAddr
 * Description    : Set GWIP address
 * Input          : ipaddr
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH395CMDSetGWIPAddr(UINT8 *gwipaddr)
{
    UINT8 i;
    xWriteCH395Cmd(CMD40_SET_GWIP_ADDR);
    for (i = 0; i < 4; i++)
        xWriteCH395Data(*gwipaddr++);
    xEndCH395Cmd();
}

/********************************************************************************
 * Function Name  : CH395CMDSetMASKAddr
 * Description    : Set MASK address ,The default is 255.255.255.0
 * Input          : maskaddr
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH395CMDSetMASKAddr(UINT8 *maskaddr)
{
    UINT8 i;
    xWriteCH395Cmd(CMD40_SET_MASK_ADDR);
    for (i = 0; i < 4; i++)
        xWriteCH395Data(*maskaddr++);
    xEndCH395Cmd();
}

/*******************************************************************************
 * Function Name  : CH395CMDSetMACFilt
 * Description    : Set MAC filtering
 * Input          : filtype     - Preference of Filtering
                    table0      - Hash0
                    table1      - Hash1
 * Output         : None
 * Return         : None
*******************************************************************************/
void CH395CMDSetMACFilt(UINT8 filtype, UINT32 table0, UINT32 table1)
{
    xWriteCH395Cmd(CMD90_SET_MAC_FILT);
    xWriteCH395Data(filtype);
    xWriteCH395Data((UINT8)table0);
    xWriteCH395Data((UINT8)((UINT16)table0 >> 8));
    xWriteCH395Data((UINT8)(table0 >> 16));
    xWriteCH395Data((UINT8)(table0 >> 24));

    xWriteCH395Data((UINT8)table1);
    xWriteCH395Data((UINT8)((UINT16)table1 >> 8));
    xWriteCH395Data((UINT8)(table1 >> 16));
    xWriteCH395Data((UINT8)(table1 >> 24));
    xEndCH395Cmd();
}

/*******************************************************************************
 * Function Name  : CH395CMDGetPHYStatus
 * Description    : Gets the current PHY status
 * Input          : None
 * Output         : None
 * Return         : Current PHY status. See PHY Parameter definition for the status definition
 *******************************************************************************/
UINT8 CH395CMDGetPHYStatus(void)
{
    UINT8 i;
    xWriteCH395Cmd(CMD01_GET_PHY_STATUS);
    i = xReadCH395Data();
    xEndCH395Cmd();
    return i;
}

/********************************************************************************
 * Function Name  : CH395CMDInitCH395
 * Description    : Initialize CH395
 * Input          : None
 * Output         : None
 * Return         : UINT8 s
 *******************************************************************************/
UINT8 CH395CMDInitCH395(void)
{
    UINT8 i = 0;
    UINT8 s = 0;
    xWriteCH395Cmd(CMD0W_INIT_CH395);
    xEndCH395Cmd();
    while (1)
    {
        Delay_Ms(20);               /* Delay query, more than 20MS is recommended*/
        s = CH395CMDGetCmdStatus(); /* Do not query too frequently*/
        if (s != CH395_ERR_BUSY)
            break;
        if (i++ > 200)
            return CH395_ERR_UNKNOW; /* Timeout exits. This function needs more than 400MS to complete */
    }
    return s;
}

/********************************************************************************
 * Function Name  : CH395CMDGetUnreachIPPT
 * Description    : Get unreachable information (IP,Port,Protocol Type)
 * Input          : list
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH395CMDGetUnreachIPPT(UINT8 *list)
{
    UINT8 i;
    xWriteCH395Cmd(CMD08_GET_UNREACH_IPPORT);
    for (i = 0; i < 8; i++)
    {
        *list++ = xReadCH395Data();
    }
    xEndCH395Cmd();
}

/********************************************************************************
 * Function Name  : CH395CMDSetRetranCount
 * Description    : Set retry times
 * Input          : retry times
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH395CMDSetRetranCount(UINT8 time)
{
    xWriteCH395Cmd(CMD10_SET_RETRAN_COUNT);
    xWriteCH395Data(time);
    xEndCH395Cmd();
}

/********************************************************************************
 * Function Name  : CH395CMDSetRetranPeriod
 * Description    : Set retry period
 * Input          : retry period
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH395CMDSetRetranPeriod(UINT16 period)
{
    xWriteCH395Cmd(CMD20_SET_RETRAN_PERIOD);
    xWriteCH395Data((UINT8)period);
    xWriteCH395Data((UINT8)(period >> 8));
    xEndCH395Cmd();
}

/*******************************************************************************
 * Function Name  : CH395GetCmdStatus
 * Description    : Obtain the command execution status
 * Input          : None
 * Output         : None
 * Return         : Command execution status
 *******************************************************************************/
UINT8 CH395CMDGetCmdStatus(void)
{
    UINT8 i;
    xWriteCH395Cmd(CMD01_GET_CMD_STATUS);
    i = xReadCH395Data();
    xEndCH395Cmd();
    return i;
}

/********************************************************************************
 * Function Name  : CH395CMDGetRemoteIPP
 * Description    : Obtain the port and IP address of the remote end
 * Input          : sockindex      - sockindex
                    list           - Save the IP address and port number
 * Output         : None
 * Return         : None
*******************************************************************************/
void CH395CMDGetRemoteIPP(UINT8 sockindex, UINT8 *list)
{
    UINT8 i;

    xWriteCH395Cmd(CMD06_GET_REMOT_IPP_SN);
    xWriteCH395Data(sockindex);
    for (i = 0; i < 6; i++)
    {
        *list++ = xReadCH395Data();
    }
    xEndCH395Cmd();
}

/*******************************************************************************
 * Function Name  : CH395ClearRecvBuf
 * Description    : Clear receive buffer
 * Input          : sockindex
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH395CMDClearRecvBuf(UINT8 sockindex)
{
    xWriteCH395Cmd(CMD10_CLEAR_RECV_BUF_SN);
    xWriteCH395Data((UINT8)sockindex);
    xEndCH395Cmd();
}

/********************************************************************************
 * Function Name  : CH395CMDGetSocketStatus
 * Description    : Obtain the socket n status
 * Input          : sockindex
 * Output         : None
 * Return         : First byte     - socket n open or closed
                    Second byte    - TCP state, meaningful only if TCP mode and the first byte is open
*******************************************************************************/
u16 CH395CMDGetSocketStatus(UINT8 sockindex)
{
    u16 status;
    xWriteCH395Cmd(CMD12_GET_SOCKET_STATUS_SN);
    xWriteCH395Data(sockindex);
    status = xReadCH395Data() << 8;
    status |= xReadCH395Data();
    xEndCH395Cmd();
    return status;
}

/*******************************************************************************
 * Function Name  : CH395GetSocketInt
 * Description    : Gets the interrupt status of socket n
 * Input          : sockindex
 * Output         : None
 * Return         : socket interrupt status
 *******************************************************************************/
UINT8 CH395CMDGetSocketInt(UINT8 sockindex)
{
    UINT8 intstatus;
    xWriteCH395Cmd(CMD11_GET_INT_STATUS_SN);
    xWriteCH395Data(sockindex);
    /*In between sending and receiving bytes, a TSC time delay is required.*/
    Delay_Us(1);
    intstatus = xReadCH395Data();
    xEndCH395Cmd();
    return intstatus;
}

/*******************************************************************************
 * Function Name  : CH395SetSocketDesIP
 * Description    : Set the destination IP address of socket n
 * Input          : sockindex     - socket index
                    ipaddr        - destination IP address
 * Output         : None
 * Return         : None
*******************************************************************************/
void CH395CMDSetSocketDesIP(UINT8 sockindex, UINT8 *ipaddr)
{
    xWriteCH395Cmd(CMD50_SET_IP_ADDR_SN);
    xWriteCH395Data(sockindex);
    xWriteCH395Data(*ipaddr++);
    xWriteCH395Data(*ipaddr++);
    xWriteCH395Data(*ipaddr++);
    xWriteCH395Data(*ipaddr++);
    xEndCH395Cmd();
}

/*******************************************************************************
 * Function Name  : CH395SetSocketDesPort
 * Description    : Set the destination port of socket n
 * Input          : sockindex   - socket index
                    desport     - destination port
 * Output         : None
 * Return         : None
*******************************************************************************/
void CH395CMDSetSocketDesPort(UINT8 sockindex, u16 desport)
{
    xWriteCH395Cmd(CMD30_SET_DES_PORT_SN);
    xWriteCH395Data(sockindex);
    xWriteCH395Data((UINT8)desport);
    xWriteCH395Data((UINT8)(desport >> 8));
    xEndCH395Cmd();
}

/*******************************************************************************
 * Function Name  : CH395SetSocketSourPort
 * Description    : Set the source port of socket n
 * Input          : sockindex     - socket index
                    sorport       - source port
 * Output         : None
 * Return         : None
*******************************************************************************/
void CH395CMDSetSocketSourPort(UINT8 sockindex, u16 sorport)
{
    xWriteCH395Cmd(CMD30_SET_SOUR_PORT_SN);
    xWriteCH395Data(sockindex);
    xWriteCH395Data((UINT8)sorport);
    xWriteCH395Data((UINT8)(sorport >> 8));
    xEndCH395Cmd();
}

/*******************************************************************************
 * Function Name  : CH395SetSocketProtType
 * Description    : Set the protocol type of socket n
 * Input          : sockindex   - socket index
                    prottype    - protocol type
 * Output         : None
 * Return         : None
*******************************************************************************/
void CH395CMDSetSocketProtType(UINT8 sockindex, UINT8 prottype)
{
    xWriteCH395Cmd(CMD20_SET_PROTO_TYPE_SN);
    xWriteCH395Data(sockindex);
    xWriteCH395Data(prottype);
    xEndCH395Cmd();
}

/*******************************************************************************
 * Function Name  : CH395OpenSocket
 * Description    : Open socket n
 * Input          : sockindex
 * Output         : None
 * Return         : s
 *******************************************************************************/
UINT8 CH395CMDOpenSocket(UINT8 sockindex)
{
    UINT8 i = 0;
    UINT8 s = 0;
    xWriteCH395Cmd(CMD1W_OPEN_SOCKET_SN);
    xWriteCH395Data(sockindex);
    xEndCH395Cmd();
    while (1)
    {
        Delay_Ms(20);               /* Delay query, more than 20MS is recommended*/
        s = CH395CMDGetCmdStatus(); /* Do not query too frequently*/
        if (s != CH395_ERR_BUSY)
            break;
        if (i++ > 200)
            return CH395_ERR_UNKNOW;
    }
    return s;
}

/******************************************************************************
 * Function Name  : CH395TCPListen
 * Description    : socket n listens, After receiving this command, socket n enters server mode, valid only for TCP mode
 * Input          : sockindex
 * Output         : None
 * Return         : s
 *******************************************************************************/
UINT8 CH395CMDTCPListen(UINT8 sockindex)
{
    UINT8 i = 0;
    UINT8 s = 0;
    xWriteCH395Cmd(CMD1W_TCP_LISTEN_SN);
    xWriteCH395Data(sockindex);
    xEndCH395Cmd();
    while (1)
    {
        Delay_Ms(20);               /* Delay query, more than 20MS is recommended*/
        s = CH395CMDGetCmdStatus(); /* Do not query too frequently*/
        if (s != CH395_ERR_BUSY)
            break;
        if (i++ > 200)
            return CH395_ERR_UNKNOW;
    }
    return s;
}

/********************************************************************************
 * Function Name  : CH395TCPConnect
 * Description    : socket n connection. After receiving this command, socket n enters the client mode, valid only for TCP mode
 * Input          : sockindex
 * Output         : None
 * Return         : s
 *******************************************************************************/
UINT8 CH395CMDTCPConnect(UINT8 sockindex)
{
    UINT8 i = 0;
    UINT8 s = 0;
    xWriteCH395Cmd(CMD1W_TCP_CONNECT_SN);
    xWriteCH395Data(sockindex);
    xEndCH395Cmd();
    while (1)
    {
        Delay_Ms(20);               /* Delay query, more than 20MS is recommended*/
        s = CH395CMDGetCmdStatus(); /* Do not query too frequently*/
        if (s != CH395_ERR_BUSY)
            break;
        if (i++ > 200)
            return CH395_ERR_UNKNOW;
    }
    return s;
}

/********************************************************************************
 * Function Name  : CH395CMDTCPDisconnect
 * Description    : socket n disconnection
 * Input          : sockindex
 * Output         : None
 * Return         : s
 *******************************************************************************/
UINT8 CH395CMDTCPDisconnect(UINT8 sockindex)
{
    UINT8 i = 0;
    UINT8 s = 0;
    xWriteCH395Cmd(CMD1W_TCP_DISCONNECT_SN);
    xWriteCH395Data(sockindex);
    xEndCH395Cmd();
    while (1)
    {
        Delay_Ms(20);               /* Delay query, more than 20MS is recommended*/
        s = CH395CMDGetCmdStatus(); /* Do not query too frequently*/
        if (s != CH395_ERR_BUSY)
            break;
        if (i++ > 200)
            return CH395_ERR_UNKNOW;
    }
    return s;
}

/********************************************************************************
 * Function Name  : CH395CMDSendData
 * Description    : Writes data to socket n buffer
 * Input          : sockindex    - socket index
                    data buf     - data buf
                    len          - data length
 * Output         : None
 * Return         : None
*******************************************************************************/
void CH395CMDSendData(UINT8 sockindex, UINT8 *databuf, u16 len)
{
#if (CH395_SPI_DMA_ENABLE == DISABLE)
    u16 i;
    xWriteCH395Cmd(CMD30_WRITE_SEND_BUF_SN);
    xWriteCH395Data((UINT8)sockindex);
    xWriteCH395Data((UINT8)len);
    xWriteCH395Data((UINT8)(len >> 8));
    for (i = 0; i < len; i++)
    {
        xWriteCH395Data(*databuf++);
    }
    xEndCH395Cmd();
#else
    if (!len)
        return;
    xWriteCH395Cmd(CMD30_WRITE_SEND_BUF_SN);
    xWriteCH395Data((u8)sockindex);
    xWriteCH395Data((u8)len);
    xWriteCH395Data((u8)(len >> 8));

    Delay_Us(1);

    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx, ENABLE);
    DMA_Tx_Init(DMA1_Channel3, (u32)&SPI1->DR, (u32)databuf, len);
    DMA_Rx_Init(DMA1_Channel2, (u32)&SPI1->DR, (u32)databuf, len);
    DMA_Cmd(DMA1_Channel2, ENABLE);
    DMA_Cmd(DMA1_Channel3, ENABLE);
    while (!DMA_GetFlagStatus(DMA1_FLAG_TC3) || !DMA_GetFlagStatus(DMA1_FLAG_TC2))
    {
        if (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == RESET)
            break;
    }
    DMA_ClearFlag(DMA1_FLAG_TC3);
    DMA_ClearFlag(DMA1_FLAG_TC2);

    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx, DISABLE);
    DMA_Cmd(DMA1_Channel2, DISABLE);
    DMA_Cmd(DMA1_Channel3, DISABLE);
    xEndCH395Cmd();
#endif
}

/*******************************************************************************
 * Function Name  : CH395CMDGetRecvLength
 * Description    : Gets the length of data received by socket n
 * Input          : socket index
 * Output         : None
 * Return         : 2 bytes receive length
 *******************************************************************************/
u16 CH395CMDGetRecvLength(UINT8 sockindex)
{
    u16 i;

    xWriteCH395Cmd(CMD12_GET_RECV_LEN_SN);
    xWriteCH395Data((UINT8)sockindex);
    /*In between sending and receiving bytes, a TSC time delay is required.*/
    Delay_Us(1);
    i = xReadCH395Data();
    i = (u16)(xReadCH395Data() << 8) + i;
    xEndCH395Cmd();
    return i;
}

/********************************************************************************
 * Function Name  : CH395CMDGetRecvData
 * Description    : Gets socket n receive buffer data
 * Input          : sockindex    - socket index
                    len          - data length
                    pbuf         - data buf
 * Output         : None
 * Return         : None
*******************************************************************************/
void CH395CMDGetRecvData(UINT8 sockindex, u16 len, UINT8 *pbuf)
{
#if (CH395_SPI_DMA_ENABLE == 0)
    u16 i;
    if (!len)
        return;
    xWriteCH395Cmd(CMD30_READ_RECV_BUF_SN);
    xWriteCH395Data(sockindex);
    xWriteCH395Data((UINT8)len);
    xWriteCH395Data((UINT8)(len >> 8));
    for (i = 0; i < len; i++)
    {
        *pbuf = xReadCH395Data();
        pbuf++;
    }
    xEndCH395Cmd();

#else
    if (!len)
        return;
    xWriteCH395Cmd(CMD30_READ_RECV_BUF_SN);
    xWriteCH395Data(sockindex);
    xWriteCH395Data((u8)len);
    xWriteCH395Data((u8)(len >> 8));

    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx, ENABLE);
    DMA_Tx_Init(DMA1_Channel3, (u32)&SPI1->DR, (u32)pbuf, len);
    DMA_Rx_Init(DMA1_Channel2, (u32)&SPI1->DR, (u32)pbuf, len);
    DMA_Cmd(DMA1_Channel2, ENABLE);
    DMA_Cmd(DMA1_Channel3, ENABLE);
    while (!DMA_GetFlagStatus(DMA1_FLAG_TC3) || !DMA_GetFlagStatus(DMA1_FLAG_TC2))
    {
        if (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == RESET)
            break;
    }
    DMA_ClearFlag(DMA1_FLAG_TC3);
    DMA_ClearFlag(DMA1_FLAG_TC2);

    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx, DISABLE);
    DMA_Cmd(DMA1_Channel2, DISABLE);
    DMA_Cmd(DMA1_Channel3, DISABLE);
    xEndCH395Cmd();
#endif
}

/*******************************************************************************
 * Function Name  : CH395CMDCloseSocket
 * Description    : Close socket n
 * Input          : sockindex
 * Output         : None
 * Return         : s
 *******************************************************************************/
UINT8 CH395CMDCloseSocket(UINT8 sockindex)
{
    UINT8 i = 0;
    UINT8 s = 0;
    xWriteCH395Cmd(CMD1W_CLOSE_SOCKET_SN);
    xWriteCH395Data(sockindex);
    xEndCH395Cmd();
    while (1)
    {
        Delay_Ms(20);               /* Delay query, more than 20MS is recommended*/
        s = CH395CMDGetCmdStatus(); /* Do not query too frequently*/
        if (s != CH395_ERR_BUSY)
            break;
        if (i++ > 200)
            return CH395_ERR_UNKNOW;
    }
    return s;
}

/******************************************************************************
 * Function Name  : CH395SetSocketIPRAWProto
 * Description    : In IP mode, configure the IP packet protocol field.
 * Input          : sockindex     - SocketIndex
                    prototype     - 1 byte protocol field
 * Output         : None
 * Return         : None
*******************************************************************************/
void CH395CMDSetSocketIPRAWProto(UINT8 sockindex, UINT8 prototype)
{
    xWriteCH395Cmd(CMD20_SET_IPRAW_PRO_SN);
    xWriteCH395Data(sockindex);
    xWriteCH395Data(prototype);
    xEndCH395Cmd();
}

/********************************************************************************
 * Function Name  : CH395EnablePing
 * Description    : On/off PING
 * Input          : 1 Enable PING ; 0  Disable PING
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH395CMDEnablePing(UINT8 enable)
{
    xWriteCH395Cmd(CMD10_PING_ENABLE);
    xWriteCH395Data(enable);
    xEndCH395Cmd();
}

/********************************************************************************
 * Function Name  : CH395CMDGetMACAddr
 * Description    : Gets the MAC address
 * Input          : MAC address pointer
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH395CMDGetMACAddr(UINT8 *macaddr)
{
    UINT8 i;
    xWriteCH395Cmd(CMD06_GET_MAC_ADDR);
    for (i = 0; i < 6; i++)
        *macaddr++ = xReadCH395Data();
    xEndCH395Cmd();
}

/******************************************************************************
 * Function Name  : CH395DHCPEnable
 * Description    : Enable DHCP
 * Input          : flag : 1 enable DHCP, 0 disable DHCP
 * Output         : None
 * Return         : s
 *******************************************************************************/
UINT8 CH395CMDDHCPEnable(UINT8 flag)
{
    UINT8 i = 0;
    UINT8 s;
    xWriteCH395Cmd(CMD10_DHCP_ENABLE);
    xWriteCH395Data(flag);
    xEndCH395Cmd();
    while (1)
    {
        Delay_Ms(20);               /* Delay query, more than 20MS is recommended*/
        s = CH395CMDGetCmdStatus(); /* Do not query too frequently*/
        if (s != CH395_ERR_BUSY)
            break;
        if (i++ > 200)
            return CH395_ERR_UNKNOW;
    }
    return s;
}

/******************************************************************************
 * Function Name  : CH395GetDHCPStatus
 * Description    : Obtain DHCP status
 * Input          : None
 * Output         : None
 * Return         : 1 byte status code, 0 indicates success, other values fail
 *******************************************************************************/
UINT8 CH395CMDGetDHCPStatus(void)
{
    UINT8 status;
    xWriteCH395Cmd(CMD01_GET_DHCP_STATUS);
    status = xReadCH395Data();
    xEndCH395Cmd();
    return status;
}

/*******************************************************************************
 * Function Name  : CH395GetIPInf
 * Description    : Get IP, subnet mask, gateway
 * Input          : None
 * Output         : 20 bytes, respectively 4 bytes IP, 4 bytes gateway, 4 bytes mask, 4 bytes DNS1, 4 bytes DNS2
 * Return         : None
 *******************************************************************************/
void CH395CMDGetIPInf(UINT8 *addr)
{
    UINT8 i;
    xWriteCH395Cmd(CMD014_GET_IP_INF);
    for (i = 0; i < 20; i++)
    {
        *addr++ = xReadCH395Data();
    }
    xEndCH395Cmd();
}

/*******************************************************************************
 * Function Name  : CH395CMDSetARP
 * Description    : Set ARP retransmission period and number of times
 *  Input         : period   - 1 byte ARP retransmission period
                    cnt      - 1 byte ARP retransmission number
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH395CMDSetARP(UINT8 period, UINT8 cnt)
{
    xWriteCH395Cmd(CMD20_SET_ARP);
    xWriteCH395Data(period);
    xWriteCH395Data(cnt);
    xEndCH395Cmd();
}

/*******************************************************************************
 * Function Name  : CH395CMDSetTCPMSS
 * Description    : Set TCP MSS
 * Input          : mss
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH395CMDSetTCPMSS(UINT16 mss)
{
    xWriteCH395Cmd(CMD20_TCP_MSS);
    xWriteCH395Data((UINT8)mss);
    xWriteCH395Data((UINT8)(mss >> 8));
    xEndCH395Cmd();
}

/*******************************************************************************
 * Function Name  : CH395SetTTLNum
 * Description    : Set the TTL
 * Input          : sockindex   - SocketIndex
 *                  TTLnum      - 1 byte TTL
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH395CMDSetTTLNum(UINT8 sockindex, UINT8 TTLnum)
{
    xWriteCH395Cmd(CMD20_SET_TTL);
    xWriteCH395Data(sockindex);
    xWriteCH395Data(TTLnum);
    xEndCH395Cmd();
}

/*******************************************************************************
 * Function Name  : CH395SetSocketRecvBuf
 * Description    : Sets the SOCKET receive buffer
 * Input          : sockindex    - sockindex
                    startblk     - starting block index
                    blknum       - the number of blocks
 * Output         : None
 * Return         : None
*******************************************************************************/
void CH395CMDSetSocketRecvBuf(UINT8 sockindex, UINT8 startblk, UINT8 blknum)
{
    xWriteCH395Cmd(CMD30_SET_RECV_BUF);
    xWriteCH395Data(sockindex);
    xWriteCH395Data(startblk);
    xWriteCH395Data(blknum);
    xEndCH395Cmd();
}

/*******************************************************************************
 * Function Name  : CH395CMDSetSocketSendBuf
 * Description    : Sets the SOCKET send buffer
 * Input          : sockindex    - sockindex
                    startblk     - starting block index
                    blknum       - the number of blocks
 * Output         : None
 * Return         : None
*******************************************************************************/
void CH395CMDSetSocketSendBuf(UINT8 sockindex, UINT8 startblk, UINT8 blknum)
{
    xWriteCH395Cmd(CMD30_SET_SEND_BUF);
    xWriteCH395Data(sockindex);
    xWriteCH395Data(startblk);
    xWriteCH395Data(blknum);
    xEndCH395Cmd();
}

/*******************************************************************************
 * Function Name  : CH395CMDSetFunPapr
 * Description    : Sets function parameter
 * Input          : Four-byte function parameter
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH395CMDSetFunPapr(UINT8 PARA1, UINT8 PARA2, UINT8 PARA3, UINT8 PARA4)
{
    xWriteCH395Cmd(CMD40_SET_FUN_PARA);
    xWriteCH395Data(PARA1);
    xWriteCH395Data(PARA2);
    xWriteCH395Data(PARA3);
    xWriteCH395Data(PARA4);
    xEndCH395Cmd();
}

/*******************************************************************************
 * Function Name  : CH395KeepLiveIDLE
 * Description    : Set KEEPLIVE idle time
 * Input          : idle time (ms)
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH395CMDKeepLiveIDLE(UINT32 idle)
{
    xWriteCH395Cmd(CMD40_SET_KEEP_LIVE_IDLE);
    xWriteCH395Data((UINT8)idle);
    xWriteCH395Data((UINT8)((u16)idle >> 8));
    xWriteCH395Data((UINT8)(idle >> 16));
    xWriteCH395Data((UINT8)(idle >> 24));
    xEndCH395Cmd();
}

/*******************************************************************************
 * Function Name  : CH395KeepLiveINTVL
 * Description    : Set KEEPLIVE interval time
 * Input          : timeout interval (ms)
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH395CMDKeepLiveINTVL(UINT32 intvl)
{
    xWriteCH395Cmd(CMD40_SET_KEEP_LIVE_INTVL);
    xWriteCH395Data((UINT8)intvl);
    xWriteCH395Data((UINT8)((u16)intvl >> 8));
    xWriteCH395Data((UINT8)(intvl >> 16));
    xWriteCH395Data((UINT8)(intvl >> 24));
    xEndCH395Cmd();
}

/*******************************************************************************
 * Function Name  : CH395KeepLiveCNT
 * Description    : Set KEEPLIVE retries times
 * Input          : retry times
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH395CMDKeepLiveCNT(UINT8 cnt)
{
    xWriteCH395Cmd(CMD10_SET_KEEP_LIVE_CNT);
    xWriteCH395Data(cnt);
    xEndCH395Cmd();
}

/*******************************************************************************
 * Function Name  : CH395SetKeepLive
 * Description    : Set the socket n keeplive function
 * Input          : sockindex   - sockindex
 *                  cmd         - 0: close 1:open
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH395CMDSetKeepLive(UINT8 sockindex, UINT8 cmd)
{
    xWriteCH395Cmd(CMD20_SET_KEEP_LIVE_SN);
    xWriteCH395Data(sockindex);
    xWriteCH395Data(cmd);
    xEndCH395Cmd();
}

/*******************************************************************************
 * Function Name  : CH395CMDEEPROMErase
 * Description    : EEPROM erase
 * Input          : None
 * Output         : None
 * Return         : executing state
 *******************************************************************************/
UINT8 CH395CMDEEPROMErase(void)
{
    UINT8 i, s;
    xWriteCH395Cmd(CMD0W_EEPROM_ERASE);
    xEndCH395Cmd();
    while (1)
    {
        Delay_Ms(20);               /* Delay query, more than 20MS is recommended*/
        s = CH395CMDGetCmdStatus(); /* Do not query too frequently*/
        if (s != CH395_ERR_BUSY)
            break;
        if (i++ > 200)
            return CH395_ERR_UNKNOW;
    }
    return i;
}

/*******************************************************************************
 * Function Name  : CH395EEPROMWrite
 * Description    : Write EEPROM
 * Input          : eepaddr   - EEPROM addr
 *                ：buf       - buffer address
 *                ：len       - len
 * Output         : None
 * Return         : executing state
 *******************************************************************************/
UINT8 CH395CMDEEPROMWrite(UINT16 eepaddr, UINT8 *buf, UINT8 len)
{
    UINT8 i, s;
    xWriteCH395Cmd(CMD30_EEPROM_WRITE);
    xWriteCH395Data((UINT8)(eepaddr));
    xWriteCH395Data((UINT8)(eepaddr >> 8));
    xWriteCH395Data(len);
    while (len--)
        xWriteCH395Data(*buf++);
    xEndCH395Cmd();
    while (1)
    {
        Delay_Ms(20);               /* Delay query, more than 20MS is recommended*/
        s = CH395CMDGetCmdStatus(); /* Do not query too frequently*/
        if (s != CH395_ERR_BUSY)
            break;
        if (i++ > 200)
            return CH395_ERR_UNKNOW;
    }
    return s;
}

/*******************************************************************************
 * Function Name  : CH395EEPROMRead
 * Description    : Read EEPROM
 * Input          : eepaddr   - EEPROM addr
 *                ：buf       - buffer address
 *                ：len       - len
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH395CMDEEPROMRead(UINT16 eepaddr, UINT8 *buf, UINT8 len)
{
    xWriteCH395Cmd(CMD30_EEPROM_READ);
    xWriteCH395Data((UINT8)(eepaddr));
    xWriteCH395Data((UINT8)(eepaddr >> 8));
    xWriteCH395Data(len);
    Delay_Us(30);
    while (len--)
        *buf++ = xReadCH395Data();
    xEndCH395Cmd();
}

/*******************************************************************************
 * Function Name  : CH395CMDReadGPIOAddr
 * Description    : Read GPIO register
 * Input          : register addr
 * Output         : None
 * Return         : register value
 *******************************************************************************/
UINT8 CH395CMDReadGPIOAddr(UINT8 regadd)
{
    UINT8 i;
    xWriteCH395Cmd(CMD11_READ_GPIO_REG);
    xWriteCH395Data(regadd);
    i = xReadCH395Data();
    xEndCH395Cmd();
    return i;
}

/*******************************************************************************
 * Function Name  : CH395CMDWriteGPIOAddr
 * Description    : Write GPIO register
 * Input          : regadd    - register addr
 *                ：regval    - register value
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH395CMDWriteGPIOAddr(UINT8 regadd, UINT8 regval)
{
    xWriteCH395Cmd(CMD20_WRITE_GPIO_REG);
    xWriteCH395Data(regadd);
    xWriteCH395Data(regval);
    xEndCH395Cmd();
}

/*******************************************************************************
 * Function Name  : CH395SetUartBaudRate
 * Description    : Set 395 Uart BaudRate
 * Input          : BaudRate
 * Output         : None
 * Return         : s
 *******************************************************************************/
UINT8 CH395SetUartBaudRate(UINT32 baudrate)
{
    UINT8 s = CH395_ERR_UNKNOW;
#if (CH395_OP_INTERFACE_MODE == CH395_UART_MODE)
    CH395CMDSetUartBaudRate(baudrate); /* Set BaudRate */
    Delay_Ms(1);
    Set_MCU_BaudRate(baudrate);
    s = xReadCH395Data(); /* If the setting is successful CH395 return CMD_ERR_SUCCESS */
    if (s == CMD_ERR_SUCCESS)
        printf("Set Success\r\n");
#endif
    return s;
}

/*******************************************************************************
 * Function Name  : CH395UDPSendTo
 * Description    : UDP sends data to the specified IP address and port
 * Input          : buf     - Send data buffer
                    len     - Send data length
                    ip      - DES IP
                    port    - DES Port
                    sockeid - socket index
 * Output         : None
 * Return         : None
*******************************************************************************/
void CH395UDPSendTo(UINT8 *buf, UINT32 len, UINT8 *ip, UINT16 port, UINT8 sockindex)
{
    CH395CMDSetSocketDesIP(sockindex, ip);
    CH395CMDSetSocketDesPort(sockindex, port);
    CH395CMDSendData(sockindex, buf, len);
}

/*******************************************************************************
 * Function Name  : CH395CMDIPv6AutoLLA
 * Description    : Enable automatic generation of Link-Local Address
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH395CMDIPv6AutoLLA(void)
{
    xWriteCH395Cmd(CMD00_IPV6_AUTO_LLA);
    xEndCH395Cmd();
}

/*******************************************************************************
 * Function Name  : CH395CMDSetLLA
 * Description    : Set IPv6 Link-Local Address
 * Input          : pointer to 16-byte Link-Local address
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH395CMDSetLLA(UINT8 *pLLA)
{
    UINT8 i;
    xWriteCH395Cmd(CMD100_SET_LLA);
    for (i = 0; i < 16; i++)
        xWriteCH395Data(pLLA[i]);
    xEndCH395Cmd();
}

/*******************************************************************************
 * Function Name  : CH395CMDSetGUA
 * Description    : Set IPv6 Global Unicast Address
 * Input          : pointer to 16-byte Global Unicast address
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH395CMDSetGUA(UINT8 *pGUA)
{
    UINT8 i;
    xWriteCH395Cmd(CMD100_SET_GUA);
    for (i = 0; i < 16; i++)
        xWriteCH395Data(pGUA[i]);
    xEndCH395Cmd();
}

/*******************************************************************************
 * Function Name  : CH395CMDSetDesIPv6Addr
 * Description    : Set destination IPv6 address for socket n
 * Input          : socket index, pointer to 16-byte IPv6 address
 * Output         : None
 * Return         : None
 *******************************************************************************/
void CH395CMDSetDesIPv6Addr(UINT8 sockindex, UINT8 *pIPv6)
{
    UINT8 i;
    xWriteCH395Cmd(CMD110_SET_DES_IPV6_ADDR_SN);
    xWriteCH395Data(sockindex);
    for (i = 0; i < 16; i++)
        xWriteCH395Data(pIPv6[i]);
    xEndCH395Cmd();
}

/*******************************************************************************
 * Function Name  : CH395CMDGetIPv6Unreach
 * Description    : Get IPv6 unreachable information
 * Input          : pointer to 20-byte buffer (store code+proto+port+IPv6)
 * Output         : 20 bytes data
 * Return         : None
 *******************************************************************************/
void CH395CMDGetIPv6Unreach(UINT8 *pBuf)
{
    UINT8 i;
    xWriteCH395Cmd(CMD014_GET_IPV6_UNREACH);
    for (i = 0; i < 20; i++)
        pBuf[i] = xReadCH395Data();
    xEndCH395Cmd();
}

/*******************************************************************************
 * Function Name  : CH395CMDGetRemoteIPv6
 * Description    : Get remote IPv6 address and port of socket n
 * Input          : socket index
 * Output         : pointer to 18-byte buffer (16-byte IPv6 + 2-byte port)
 * Return         : None
 *******************************************************************************/
void CH395CMDGetRemoteIPv6(UINT8 sockindex, UINT8 *pBuf)
{
    UINT8 i;
    xWriteCH395Cmd(CMD112_GET_REMOT_IPV6_SN);
    xWriteCH395Data(sockindex);
    for (i = 0; i < 18; i++)
        pBuf[i] = xReadCH395Data();
    xEndCH395Cmd();
}

/*******************************************************************************
 * Function Name  : CH395CMDDHCPv6Enable
 * Description    : Enable or disable DHCPv6
 * Input          : 1 byte flag (1: enable, 0: disable)
 * Output         : None
 * Return         : s
 *******************************************************************************/
UINT8 CH395CMDDHCPv6Enable(UINT8 flag)
{
    UINT8 i = 0;
    UINT8 s;
    xWriteCH395Cmd(CMD10_DHCPV6_ENABLE);
    xWriteCH395Data(flag);
    xEndCH395Cmd();
    while (1)
    {
        Delay_Ms(20);               /* Delay query, more than 20MS is recommended*/
        s = CH395CMDGetCmdStatus(); /* Do not query too frequently*/
        if (s != CH395_ERR_BUSY)
            break;
        if (i++ > 200)
            return CH395_ERR_UNKNOW;
    }
    return s;
}

/*******************************************************************************
 * Function Name  : CH395CMDGetDHCPv6Status
 * Description    : Get DHCPv6 status
 * Input          : None
 * Output         : None
 * Return         : 1 byte status
 *******************************************************************************/
UINT8 CH395CMDGetDHCPv6Status(void)
{
    UINT8 status;
    xWriteCH395Cmd(CMD01_GET_DHCPV6_STATUS);
    status = xReadCH395Data();
    xEndCH395Cmd();
    return status;
}

/*******************************************************************************
 * Function Name  : CH395CMDGetIPv6Addr
 * Description    : Get IPv6 Link-Local and Unicast addresses
 * Input          : pointer to 32-byte buffer (store LLA + GUA)
 * Output         : 32 bytes address data
 * Return         : None
 *******************************************************************************/
void CH395CMDGetIPv6Addr(UINT8 *pBuf)
{
    UINT8 i;
    xWriteCH395Cmd(CMD020_GET_IPV6_ADDR);
    for (i = 0; i < 32; i++)
        pBuf[i] = xReadCH395Data();
    xEndCH395Cmd();
}

/*******************************************************************************
 * Function Name  : CH395UDP6SendTo
 * Description    : UDP sends data to the specified IP address and port
 * Input          : buf     - Send data buffer
                    len     - Send data length
                    ip      - DES IP
                    port    - DES Port
                    sockeid - socket index
 * Output         : None
 * Return         : None
*******************************************************************************/
void CH395UDP6SendTo(UINT8 *buf, UINT32 len, UINT8 *ip, UINT16 port, UINT8 sockindex)
{
    CH395CMDSetDesIPv6Addr(sockindex, ip);
    CH395CMDSetSocketDesPort(sockindex, port);
    CH395CMDSendData(sockindex, buf, len);
}
