/********************************** (C) COPYRIGHT *********************************
 * File Name          : CH395INC.H
 * Author             : WCH
 * Version            : V2.0
 * Date               : 2025/04/02
 * Description        : CH395 Command definition specification
 **********************************************************************************/
#ifndef __CH395INC_H__
#define __CH395INC_H__
#ifdef __cplusplus
extern "C"
{
#endif

#include "debug.h"

#define CH395_UART_MODE 1
#define CH395_SPI_MODE 2
#define CH395_PARA_MODE 3
#define CH395_OP_INTERFACE_MODE CH395_SPI_MODE

   void CH395_PORT_INIT(void);
   void xWriteCH395Cmd(UINT8 cmd);
   void xWriteCH395Data(UINT8 mdata);
   UINT8 xReadCH395Data(void);
   void xEndCH395Cmd(void);

#if CH395_OP_INTERFACE_MODE == CH395_UART_MODE
   extern void Set_MCU_BaudRate(u32 BAUDRATE);
#endif

#if CH395_OP_INTERFACE_MODE == CH395_SPI_MODE
#define CH395_SPI_DMA_ENABLE 0
#endif

#if (CH395_SPI_DMA_ENABLE == ENABLE)
   extern void DMA_Tx_Init(DMA_Channel_TypeDef *DMA_CHx, u32 ppadr, u32 memadr, u16 bufsize);
   extern void DMA_Rx_Init(DMA_Channel_TypeDef *DMA_CHx, u32 ppadr, u32 memadr, u16 bufsize);
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
#ifndef NULL
#define NULL 0
#endif

/* ********************************************************************************************************************* */
/* Cmd Code */
/* A command operation sequence includes:
         A command code (for serial port mode, two synchronization codes are required before the command code),
         Several input data (can be 0),
         Several output data (can be 0)
      Rule for naming command codes: CMDxy_NAME
      x indicates the minimum number of input data (bytes), y indicates the minimum number of output data (bytes).
      y If it is W, Indicates that you need to wait until the command is successfully executed
      Some commands can read and write data blocks of 0 to more than one byte,
      the number of bytes of the data block itself is not included in the x or y above*/
/* ********************************************************************************************************************* */
#define CMD01_GET_IC_VER 0x01 /* Obtain the chip and firmware version number */
   /* Output: Version number*/

#define CMD31_SET_BAUDRATE 0x02 /* Serial port: Set the baudrate for serial port communication (The default baudrate after power-on or reset is 9600bps */
   /* Input: 3-bytes baud rate, low bytes first */
   /* Output: operation status (CMD_RET_SUCCESS, other values indicate operation incomplete) */

#define CMD00_ENTER_SLEEP 0x03 /* Enter low-power sleep suspended state */

#define CMD00_RESET_ALL 0x05 /* Perform hardware reset */

#define CMD11_CHECK_EXIST 0x06 /* Test communication interface and working condition */
   /* Input: Any data */
   /* Output: bitwise inverse of input data*/

#define CMD02_GET_GLOB_INT_STATUS_ALL 0x19 /* Gets the CH395T global interrupt status  */
   /* Output: Global interrupt status, refer to Global interrupt status definition */

#define CMD10_SET_PHY 0x20 /* Set CH395 PHY mode */
   /* Input: PHY mode ,See PHY Parameter definition for the status definition */

#define CMD60_SET_MAC_ADDR 0x21 /* Set MAC address , Must be set before CMD00H_INIT_CH395 */
   /* Input: 6-byte MAC address */

#define CMD40_SET_IP_ADDR 0x22 /* Set IP address , Must be set before CMD00H_INIT_CH395  */
   /* Input: 4-byte IP address */

#define CMD40_SET_GWIP_ADDR 0x23 /* Set GWIP address , Must be set before CMD00H_INIT_CH395 */
   /* Input: 4-byte GWIP address */

#define CMD40_SET_MASK_ADDR 0x24 /* Set MASK address , Must be set before CMD00H_INIT_CH395 */
   /* Input: 4-byte GWIP address */

#define CMD90_SET_MAC_FILT 0x25 /* Set MAC Filter */
   /* Input: 1-byte Preference of Filtering ,4-byte hash 0, 4-byte hash 1*/

#define CMD01_GET_PHY_STATUS 0x26 /* Gets the current PHY status, such as disconnected, FULL/HALF */
   /* Output: Current PHY status. See PHY Parameter definition for the status definition */

#define CMD0W_INIT_CH395 0x27 /* Initialize CH395 */
   /* This command takes about 200MS to run. This command needs to wait for the command to execute successfully */

#define CMD08_GET_UNREACH_IPPORT 0x28 /* Get unreachable information */
   /* Output: 8 bytes, the first byte is the unreachable type, refer to the unreachable type definition */
   /*         The second byte is the inaccessible protocol code */
   /*         The third and fourth bytes are unreachable ports */
   /*         Bytes 5-8 are unreachable IP */

#define CMD01_GET_GLOB_INT_STATUS 0x29 /* Gets the CH395 global interrupt status */
   /* Output: Global interrupt status, refer to Global interrupt status definition */

#define CMD10_SET_RETRAN_COUNT 0x2A /* Set retry times */
   /* Input: 1-byte retry time */

#define CMD20_SET_RETRAN_PERIOD 0x2B /* Set retry period */
   /* Input: 2-byte retry period */

#define CMD01_GET_CMD_STATUS 0x2C /* Obtain the command execution status */
   /* Output: Command execution status, refer to the command execution status definition */

#define CMD06_GET_REMOT_IPP_SN 0x2D /* Obtain the port and IP address of the remote end. This command is used in TCP server mode */
   /* Output: 6 bytes. Bytes 1-4 are the IP address of the remote end, and bytes 5-6 are the port number of the remote end */

#define CMD10_CLEAR_RECV_BUF_SN 0x2E /* Clear receive buffer  */
   /* Input: Index of the socket */

#define CMD12_GET_SOCKET_STATUS_SN 0x2F /* Obtain the socket n status*/
   /* Input:  The index of the socket*/
   /* Output: First byte: socket n open or closed
      Second byte: TCP state, meaningful only if TCP mode and the first byte is open */

#define CMD11_GET_INT_STATUS_SN 0x30 /* Gets the interrupt status of socket n */
   /* Input: index of the socket */
   /* Output: socket interrupt status, refer to socket interrupt status definition */

#define CMD50_SET_IP_ADDR_SN 0x31 /* Set the destination IP address of socket n */
   /* Input: The first byte is the index value of the socket, Bytes 2-5 are the IP address */

#define CMD30_SET_DES_PORT_SN 0x32 /* Set the destination port of socket n */
   /* Input: The first byte is the index value of the socket, Bytes 2-3 are the destination port */

#define CMD30_SET_SOUR_PORT_SN 0x33 /* Set the source port of socket n */
   /* Input: The first byte is the index value of the socket, Bytes 2-3 are the source port */

#define CMD20_SET_PROTO_TYPE_SN 0x34 /* Set the protocol type of socket n */
   /* Input: The first byte is the index value of the socket, the second byte is protocol type, refer to the protocol type definition */

#define CMD1W_OPEN_SOCKET_SN 0x35 /* Open socket n */
   /* Input: The first byte is the index value of the socket. This command needs to wait for the command to execute successfully */

#define CMD1W_TCP_LISTEN_SN 0x36 /* socket n listens, After receiving this command, socket n enters server mode, valid only for TCP mode */
   /* Input: The first byte is the index value of the socket. This command needs to wait for the command to execute successfully */

#define CMD1W_TCP_CONNECT_SN 0x37 /* socket n connection. After receiving this command, socket n enters the client mode, valid only for TCP mode */
   /* Input: The first byte is the index value of the socket. This command needs to wait for the command to execute successfully */

#define CMD1W_TCP_DISCONNECT_SN 0x38 /* socket n disconnection, valid only for TCP mode */
   /* Input: The first byte is the index value of the socket. */

#define CMD30_WRITE_SEND_BUF_SN 0x39 /* Writes data to socket n buffer */
   /* Input: The first byte is the index value of the socket, and the second to third bytes is the length ,And a certain byte of data*/

#define CMD12_GET_RECV_LEN_SN 0x3B /* Gets the length of data received by socket n */
   /* Input: index of the socket */
   /* Output: 2 bytes receive length */

#define CMD30_READ_RECV_BUF_SN 0x3C /* Gets socket n receive buffer data */
   /* Input: The first byte is the index value of the socket, and the second to third bytes is the read length n */
   /* Output: n data */

#define CMD1W_CLOSE_SOCKET_SN 0x3D /* Close socket n */
   /* Input: index of the socket */

#define CMD20_SET_IPRAW_PRO_SN 0x3E /*In IPRAW mode, configure the IP packet protocol field. */
   /* Input: The first byte is the index value of the socket, and the second byte is the IP RAW protocol type */

#define CMD10_PING_ENABLE 0x3F /* On/off PING */
   /* Input: 1 byte, 1 enable PING, 0 disable PING, Enable by default*/

#define CMD06_GET_MAC_ADDR 0x40 /* Gets the MAC address */
   /* Output: 6-byte MAC address */

#define CMD10_DHCP_ENABLE 0x41 /* Enable DHCP */
   /* Input: 1 byte, 1 enable DHCP, 0 disable DHCP */

#define CMD01_GET_DHCP_STATUS 0x42 /* Obtain DHCP status */
   /* Output: 1 byte status code, 0 indicates success, other values fail */

#define CMD014_GET_IP_INF 0x43 /* Get IP, subnet mask, gateway */
   /* Output: 20 bytes, respectively 4 bytes IP, 4 bytes gateway, 4 bytes mask, 4 bytes DNS1, 4 bytes DNS2 */

#define CMD20_SET_ARP 0x44 /* Set ARP retransmission period and number of times */
   /* Input: The first byte is ARP retransmission period, and the second byte is number of ARP times*/

#define CMD20_TCP_MSS 0x50 /* Set TCP MSS */
   /* Input: 2-byte TCP MSS */

#define CMD20_SET_TTL 0x51 /* Set the TTL. The maximum value of the TTL is 128 */
   /* Input: The first byte is the index value of the socket, and the second byte is the TTL value. The maximum value is 128 */

#define CMD30_SET_RECV_BUF 0x52 /* Sets the SOCKET receive buffer */
   /* Input: the first byte is the index value of the socket, the second byte is the starting block index, and the third byte is the number of blocks */

#define CMD30_SET_SEND_BUF 0x53 /* Sets the SOCKET send buffer */
   /* Input: the first byte is the index value of the socket, the second byte is the starting block index, and the third byte is the number of blocks */

#define CMD40_SET_FUN_PARA 0x55 /* Sets function parameter*/
   /* Input: 4-byte function parameter */

#define CMD40_SET_KEEP_LIVE_IDLE 0x56 /* Set KEEPLIVE idle time */
   /*Input: 4-byte keepalive idle time parameter, the low order first*/

#define CMD40_SET_KEEP_LIVE_INTVL 0x57 /* Set KEEPLIVE interval time */
   /* Input: 4-byte keepalive timeout interval, with the lowest bit before */

#define CMD10_SET_KEEP_LIVE_CNT 0x58 /* Set KEEPLIVE retries times */
   /* Input: retry times in 1 byte */

#define CMD20_SET_KEEP_LIVE_SN 0X59 /* Set the socket n keeplive function */
   /* Input: 1 byte Socket index, 1 byte setting */

#define CMD0W_EEPROM_ERASE 0XE9 /* EEPROM erase */
   /* This command needs to wait for the command to execute successfully */

#define CMD30_EEPROM_WRITE 0XEA /* Write EEPROM */
   /* Input: 2 byte addr, 1 byte len ,Writr n byte data */

#define CMD30_EEPROM_READ 0XEB /* Read EEPROM */
   /* Input: 2 byte addr, 1 byte len ,Read n byte data */

#define CMD11_READ_GPIO_REG 0XEC /* Read GPIO register */
   /* Input: 1 byte addr */
   /* Output: 1 byte value */

#define CMD20_WRITE_GPIO_REG 0XED /* Write GPIO register */
   /* Input: 1 byte addr, 1 byte value */

   /* ------------------ The following are ipv6-related commands, supported only by CH395X ------------------ */

#define CMD00_IPV6_AUTO_LLA 0x5A /* Enable IPv6 Link-Local Addr Auto Configuration */

#define CMD100_SET_LLA 0x5B /* Set IPv6 Link-Local Address */
   /* Input: 16 byte Link-Local Addr */

#define CMD100_SET_GUA 0x5C /* Set IPv6 Global Unicast Address */
   /* Input: 16 byte Global Unicast Addr */

#define CMD110_SET_DES_IPV6_ADDR_SN 0x5D /* Set Destination IPv6 Address for Socket n */
   /* Input: 1 byte Socket index, 16 byte Destination IPv6 Addr */

#define CMD014_GET_IPV6_UNREACH 0x5E /* Get IPv6 Unreachable Information */
   /* Output: 1 byte code + 1 byte protocol + 2 byte port + 16 byte IPv6 Addr */

#define CMD112_GET_REMOT_IPV6_SN 0x5F /* Get Remote IPv6 Info for Socket n */
   /* Input: 1 byte Socket index */
   /* Output: 16 byte IPv6 Addr + 2 byte Port */

#define CMD10_DHCPV6_ENABLE 0x60 /* Enable/Disable DHCPv6 */
   /* Input: 1 byte flag (1: enable, 0: disable) */

#define CMD01_GET_DHCPV6_STATUS 0x61 /* Get DHCPv6 Status */
   /* Output: 1 byte status value */

#define CMD020_GET_IPV6_ADDR 0x62 /* Get IPv6 Address Info */
   /* Output: 16 byte Link-Local Addr + 16 byte Unicast Addr */

   /* ------------------------------------------------------------------------------------------------------------ */

/* Protocol type */
#define PROTO_TYPE_IPRAW 0  /* IPRAW */
#define PROTO_TYPE_MACRAW 1 /* MACRAW */
#define PROTO_TYPE_UDP 2    /* UDP */
#define PROTO_TYPE_TCP 3    /* TCP */
#define PROTO_TYPE_UDP6 4   /* UDP6 */
#define PROTO_TYPE_TCP6 5   /* TCP6 */
#define PROTO_TYPE_IPRAW6 6 /* IPRAW6 */

/* PHY Status */
#define PHY_DISCONN (1 << 0)   /* PHY disconnected */
#define PHY_10M_FLL (1 << 1)   /* 10M full-duplex */
#define PHY_10M_HALF (1 << 2)  /* 10M half-duplex */
#define PHY_100M_FLL (1 << 3)  /* 100M full-duplex */
#define PHY_100M_HALF (1 << 4) /* 100M half-duplex */
#define PHY_AUTO (1 << 5)      /* PHY auto mode, used with CMD10H_SET_PHY */

/* interrupt status */
/* The following is the state generated by GLOB_INT */
#define GINT_STAT_UNREACH (1 << 0)    /* Unreachable interrupt */
#define GINT_STAT_IP_CONFLI (1 << 1)  /* IP conflict */
#define GINT_STAT_PHY_CHANGE (1 << 2) /* PHY status changes */
#define GINT_STAT_DHCP (1 << 3)       /* DHCP status change */
#define GINT_STAT_SOCK0 (1 << 4)      /* socket0 interrupt */
#define GINT_STAT_SOCK1 (1 << 5)      /* socket1 interrupt */
#define GINT_STAT_SOCK2 (1 << 6)      /* socket2 interrupt */
#define GINT_STAT_SOCK3 (1 << 7)      /* socket3 interrupt */
#define GINT_STAT_SOCK4 (1 << 8)      /* socket4 interrupt */
#define GINT_STAT_SOCK5 (1 << 9)      /* socket5 interrupt */
#define GINT_STAT_SOCK6 (1 << 10)     /* socket6 interrupt */
#define GINT_STAT_SOCK7 (1 << 11)     /* socket7 interrupt */
#define GINT_STAT_DHCPV6 (1 << 12)    /* DHCPv6 status change */

/* The following are the states generated by Sn_INT */
#define SINT_STAT_SENDBUF_FREE (1 << 0) /* Send buffer free */
#define SINT_STAT_SEND_OK (1 << 1)      /* Send successful */
#define SINT_STAT_RECV (1 << 2)         /* The socket port received data or the receive buffer was not empty */
#define SINT_STAT_CONNECT (1 << 3)      /* successful connect, This interrupt is generated in TCP mode */
#define SINT_STAT_DISCONNECT (1 << 4)   /* disconnect, This interrupt is generated in TCP mode */
#define SINT_STAT_TIM_OUT (1 << 6)      /* This interrupt is generated in TCP mode */

/* Get command status */
#define CMD_ERR_SUCCESS 0x00  /* Command operation succeeded */
#define CMD_RET_ABORT 0x5F    /* Command operation failure */
#define CH395_ERR_BUSY 0x10   /* Command is still being executed */
#define CH395_ERR_UNKNOW 0xFA /* Unknown error */

/* unreachable code */
#define UNREACH_CODE_HOST 0     /* Host unreachable */
#define UNREACH_CODE_NET 1      /* Network unreachable */
#define UNREACH_CODE_PROTOCOL 2 /* Protocol unreachable */
#define UNREACH_CODE_PROT 3     /* Prot Unreachable */
/*Refer to RFC792 document for other values*/

/* Command header */
#define SER_SYNC_CODE1 0x57 /* Serial port command synchronization code 1 */
#define SER_SYNC_CODE2 0xAB /* Serial port command synchronization code 2 */

/* tcp state  */
#define TCP_CLOSED 0
#define TCP_LISTEN 1
#define TCP_SYN_SENT 2
#define TCP_SYN_RCVD 3
#define TCP_ESTABLISHED 4
#define TCP_FIN_WAIT_1 5
#define TCP_FIN_WAIT_2 6
#define TCP_CLOSE_WAIT 7
#define TCP_CLOSING 8
#define TCP_LAST_ACK 9
#define TCP_TIME_WAIT 10

/* GPIO Register address */
#define GPIO_DIR_REG 0x80 /* GPIO direction register, 1: output; 0: input */
#define GPIO_IN_REG 0x81  /* GPIO input register */
#define GPIO_OUT_REG 0x82 /* GPIO output register */
#define GPIO_CLR_REG 0x83 /* GPIO output clear: 0=keep, 1=clear */
#define GPIO_PU_REG 0x84  /* GPIO pull-up enable register */
#define GPIO_PD_REG 0x85  /* GPIO pull-down enable register */

/* Function Parameters */
#define FUN_PARA_FLAG_TCP_SERVER (1 << 1)        /* TCP server multi-connection mode flag, supported from version 0x44 */
#define SOCK_CTRL_FLAG_SOCKET_CLOSE (1 << 3)     /* CH395 does not actively close the socket */
#define SOCK_DISABLE_SEND_OK_INT (1 << 4)        /* Send OK interrupt control bit, 1 to disable send OK interrupt */
#define FUN_PARA_FLAG_UART_FAST_DEAL (1 << 7)    /* UART fast response enable bit */
#define FUN_PARA_FLAG_UART_FLOW_CONTROL (1 << 8) /* UART flow control enable bit */

#ifdef __cplusplus
}
#endif
#endif

/**************************** endfile *************************************/
