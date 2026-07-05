/********************************** (C) COPYRIGHT *******************************
 * File Name          : CH395CMD.H
 * Author             : WCH
 * Version            : V2.0
 * Date               : 2025/04/02
 * Description        : CH395 command interface file
 *******************************************************************************/
#ifndef __CH395CMD_H__
#define __CH395CMD_H__
#include "debug.h"
#include "CH395INC.H"

void CH395Reset(void); /* hardware Reset */

UINT8 CH395CMDGetVer(void); /* Obtain the chip and firmware version number */

void CH395CMDEnterSleep(void); /* Enter low-power sleep suspended state */

void CH395CMDSetUartBaudRate(UINT32 baudrate); /* Set the baudrate for serial port communication */

void CH395CMDReset(void); /* Reset */

UINT8 CH395CMDCheckExist(UINT8 testdata); /* Test communication interface and working condition */

UINT16 CH395CMDGetGlobIntStatus(void); /* Gets the CH395 global interrupt status */

void CH395CMDSetPHY(UINT8 PhyMode); /* Set the PHY mode */

void CH395CMDSetMACAddr(UINT8 *macaddr); /* Set MAC address */

void CH395CMDSetIPAddr(UINT8 *ipaddr); /* Set IP address */

void CH395CMDSetGWIPAddr(UINT8 *gwipaddr); /* Set GWIP address */

void CH395CMDSetMASKAddr(UINT8 *maskaddr); /* Set MASK address */

void CH395CMDSetMACFilt(UINT8 filtype, UINT32 table0, UINT32 table1); /* Set MAC filtering. */

UINT8 CH395CMDGetPHYStatus(void); /* Gets the current PHY status */

UINT8 CH395CMDInitCH395(void); /* Initialize CH395 */

void CH395CMDGetUnreachIPPT(UINT8 *list); /* Get unreachable information (IP,Port,Protocol Type) */

void CH395CMDSetRetranCount(UINT8 time); /* Set retry times */

void CH395CMDSetRetranPeriod(UINT16 period); /* Set retry period */

UINT8 CH395CMDGetCmdStatus(void); /* Obtain the command execution status */

void CH395CMDGetRemoteIPP(UINT8 sockindex, UINT8 *list); /*Obtain the port and IP address of the remote end */

void CH395CMDClearRecvBuf(UINT8 sockindex); /* Clear receive buffer  */

UINT16 CH395CMDGetSocketStatus(UINT8 sockindex); /* Obtain the socket n status */

UINT8 CH395CMDGetSocketInt(UINT8 sockindex); /* Gets the interrupt status of socket n  */

void CH395CMDSetSocketDesIP(UINT8 sockindex, UINT8 *ipaddr); /* Set the destination IP address of socket n  */

void CH395CMDSetSocketDesPort(UINT8 sockindex, UINT16 desport); /* Set the destination port of socket n */

void CH395CMDSetSocketSourPort(UINT8 sockindex, UINT16 sorport); /* Set the source port of socket n */

void CH395CMDSetSocketProtType(UINT8 sockindex, UINT8 prottype); /* Set the protocol type of socket n  */

UINT8 CH395CMDOpenSocket(UINT8 sockindex); /* Open socket n */

UINT8 CH395CMDTCPListen(UINT8 sockindex); /* TCP Listen */

UINT8 CH395CMDTCPConnect(UINT8 sockindex); /* TCP Connect */

UINT8 CH395CMDTCPDisconnect(UINT8 sockindex); /* TCP Disconnect */

void CH395CMDSendData(UINT8 sockindex, UINT8 *databuf, UINT16 len); /* Writes data to socket n buffer */

UINT16 CH395CMDGetRecvLength(UINT8 sockindex); /* Gets the length of data received by socket n */

void CH395CMDGetRecvData(UINT8 sockindex, UINT16 len, UINT8 *pbuf); /* Gets socket n receive buffer data */

UINT8 CH395CMDCloseSocket(UINT8 sockindex); /* Close socket n*/

void CH395CMDSetSocketIPRAWProto(UINT8 sockindex, UINT8 prototype); /* In IP mode, configure the IP packet protocol field. */

void CH395CMDEnablePing(UINT8 enable); /* On/off PING*/

void CH395CMDGetMACAddr(UINT8 *macaddr); /* Gets the MAC address */

UINT8 CH395CMDDHCPEnable(UINT8 flag); /* Enable DHCP */

UINT8 CH395CMDGetDHCPStatus(void); /* Obtain DHCP status */

void CH395CMDGetIPInf(UINT8 *addr); /* Get IP, subnet mask, gateway */

void CH395CMDSetARP(UINT8 period, UINT8 cnt); /* Set ARP retransmission period and number of times */

void CH395CMDSetTCPMSS(UINT16 mss); /* Set TCP MSS */

void CH395CMDSetTTLNum(UINT8 sockindex, UINT8 TTLnum); /* Set the TTL */

void CH395CMDSetSocketRecvBuf(UINT8 sockindex, UINT8 startblk, UINT8 blknum); /* Sets the SOCKET receive buffer */

void CH395CMDSetSocketSendBuf(UINT8 sockindex, UINT8 startblk, UINT8 blknum); /* Sets the SOCKET send buffer */

void CH395CMDSetFunPapr(UINT8 PARA1, UINT8 PARA2, UINT8 PARA3, UINT8 PARA4); /*Setup parameters*/

void CH395CMDKeepLiveIDLE(UINT32 idle); /* Set KEEPLIVE idle time */

void CH395CMDKeepLiveINTVL(UINT32 intvl); /* Set KEEPLIVE interval time */

void CH395CMDKeepLiveCNT(UINT8 cnt); /* Set KEEPLIVE retries times */

void CH395CMDSetKeepLive(UINT8 sockindex, UINT8 cmd); /* Set the socket n keeplive function */

UINT8 CH395CMDEEPROMErase(void); /* Erasure EEPROM */

UINT8 CH395CMDEEPROMWrite(UINT16 eepaddr, UINT8 *buf, UINT8 len); /* Write EEPROM */

void CH395CMDEEPROMRead(UINT16 eepaddr, UINT8 *buf, UINT8 len); /* Read EEPROM */

UINT8 CH395CMDReadGPIOAddr(UINT8 regadd); /* Read GPIO register */

void CH395CMDWriteGPIOAddr(UINT8 regadd, UINT8 regval); /* Write GPIO register */

void CH395UDPSendTo(UINT8 *buf, UINT32 len, UINT8 *ip, UINT16 port, UINT8 sockindex); /*UDP mode Send to assign IP */

UINT8 CH395SetUartBaudRate(UINT32 baudrate); /* Set 395 Uart BaudRate */

void CH395CMDIPv6AutoLLA(void); /* Enable automatic generation of Link-Local Address */

void CH395CMDSetLLA(UINT8 *pLLA); /* Set IPv6 Link-Local Address */

void CH395CMDSetGUA(UINT8 *pGUA); /* Set IPv6 Global Unicast Address */

void CH395CMDSetDesIPv6Addr(UINT8 sockindex, UINT8 *pIPv6); /* Set destination IPv6 address for socket n */

void CH395CMDGetIPv6Unreach(UINT8 *pBuf); /* Get IPv6 unreachable information (20 bytes) */

void CH395CMDGetRemoteIPv6(UINT8 sockindex, UINT8 *pBuf); /* Get remote IPv6 address and port of socket n (18 bytes) */

UINT8 CH395CMDDHCPv6Enable(UINT8 flag); /* Enable or disable DHCPv6 (1: enable, 0: disable) */

UINT8 CH395CMDGetDHCPv6Status(void); /* Get DHCPv6 status */

void CH395CMDGetIPv6Addr(UINT8 *pBuf); /* Get IPv6 Link-Local and Global Unicast addresses (32 bytes) */

void CH395UDP6SendTo(UINT8 *buf, UINT32 len, UINT8 *ip, UINT16 port, UINT8 sockindex); /* UDPv6 send data to specified IP and port */

#endif
/**************************** endfile *************************************/
