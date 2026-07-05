/********************************** (C) COPYRIGHT *********************************
 * File Name          : CH395.H
 * Author             : WCH
 * Version            : V2.0
 * Date               : 2025/04/02
 * Description        : CH395 structure description
 **********************************************************************************/
#ifndef __CH395_H__
#define __CH395_H__

#include "stdio.h"
#include "string.h"
#include "CH395INC.H"
#include "CH395CMD.H"

struct _CH395_SYS
{
    UINT8 IPAddr[4];        /* CH395 IP addr 32bit*/
    UINT8 GWIPAddr[4];      /* CH395 GW IP addr 32bit*/
    UINT8 MASKAddr[4];      /* CH395 MASK addr 32bit*/
    UINT8 MacAddr[6];       /* CH395 MAC addr 48bit*/
    UINT8 PHYStat;          /* CH395 PHY state 8bit*/
    UINT8 IntfMode;         /* Interface mode */
    UINT8 UnreachIPAddr[4]; /* Unreachable IP */
    UINT16 UnreachPort;     /* Unreachable Port */
};

struct _SOCK_INF
{
    UINT8 IPAddr[4];  /* Socket des IP addr 32bit*/
    UINT8 IPv6Addr[16];  /* Socket des IPv6 addr 128bit,Only CH395X is supported*/
    UINT8 ProtoType;  /* Proto type */
    UINT8 ScokStatus; /* Socket status */
    UINT16 DesPort;   /* DesPort */
    UINT16 SourPort;  /* SourPort */
    UINT16 SendLen;   /* Send date len */
    UINT16 RemLen;    /* Remain len */
    UINT8 *pSend;     /* Send point */
};

void mStopIfError(UINT8 iError);
#endif
