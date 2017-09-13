/*****************************************************************************/
/* The information contained in this file is confidential and proprietary to */
/* ADMtek Incorporated. No part of this file may be reproduced or distributed*/
/* , in any form or by any means for any purpose, without the express written*/
/* permission of ADMtek Incorporated.                                        */
/*                                                                           */
/*(c) COPYRIGHT 2001,2002 ADMtek Incorporated, ALL RIGHTS RESERVED.          */
/*    http://www.admtek.com.tw                                               */ 
/*                                                                           */ 
/*****************************************************************************/  
/*  File Name: 1xdefs.h                                                      */ 
/*  File Description: This file includes all the #define's and               */
/*                    macro definitions required for 802.1x                  */
/*                                                                           */ 
/*  History:                                                                 */ 
/*      Date        Author      Version     Description                      */ 
/*      ----------  ---------   -------     --------------                   */
/*      02/21/2002  Joy Lin     V1.0        Initial Version                  */ 
/*                                                                           */ 
/*****************************************************************************/

#ifndef __1XDEFS_H__
#define __1XDEFS_H__

#include "hash.h"

/*************************** SOFTWARE DEFINITIONS ***********************/

/**********************************/
/* 802.1x event flag defination   */
/**********************************/
#define _1X_EVT_1SEC_TO         0x01
#define _1X_EVT_EAPOL           0x02
#define _1X_EVT_ALL             _1X_EVT_1SEC_TO | _1X_EVT_EAPOL

#define DOT1X_MAX_PORT_NUMBER         SERVICE_STA_NUM

#define EAPOL_QUEUE_DEPTH       50

#define DEF_REAUTHMAX           2       // 2 times
#define DEF_MAXREQ              2       // 2 times

#if 1 // IEEE Draft P802.1X/D11
#define DEF_TXPERIOD            5 //30      // 30 seconds  //wpa_speedUP
#define DEF_SUPPTIMEOUT         30      // 30 seconds
#define DEF_SERVERTIMEOUT       30      // 30 seconds
#define DEF_QUIETPERIOD         60      // 60 seconds
#else // speed up FSM
#define DEF_TXPERIOD            10
#define DEF_SUPPTIMEOUT         10
#define DEF_SERVERTIMEOUT       10
#define DEF_QUIETPERIOD         20
#endif

#define DEF_RETXTIMES           2       // Re-Send 2 times.

#define DEF_REAUTHPERIOD        3600    // 3600 seconds

#define EAPOL_PROTOCOLVERSION   1

#define EAPOL_KEY_TYPE_RC4      1

#define EAPOL_KEY_TYPE_RSN      254
#define VENDORID_MICROSOFT      311
#define MS_MPPE_SEND_KEY        0x10
#define MS_MPPE_RECV_KEY        0x11

#define LEN_OF_1X_CONF			sizeof(DOT1X_CONF)
#define DOT1X_FILE				"/nv/dot1x"

#define _1X_KEY_LEN             13
#define MULTICAST_KEY_ID        0
#define UNICAST_KEY_ID          3
#define UNICAST_KEY_TYPE        0x80 //IEEE Draft P802.1X/D11
#define NUM_OF_INDIVIDUAL_KEY   64
#define KEY_UNASSIGNED          0xff

#define REAUTHENABLED_TRUE      1
#define REAUTHENABLED_FALSE     2

#define MD5_DIGEST_LENGTH       16
#define SHA_DIGEST_LENGTH       20
#define GROUP_KEY_ID            2

/* used in PORT_CONTROL.portStatus */
#define PORT_UNAUTHORIZED       0
#define PORT_AUTHORIZED         1

/* used in PORT_CONTROL.flags */
#define FLAG_EAPLOGOFF          0x0001
#define FLAG_EAPSTART           0x0002
#define FLAG_RXRESPID           0x0004
#define FLAG_REAUTHENTICATE     0x0008
#define FLAG_AUTHSTART          0x0010
#define FLAG_AUTHSUCCESS        0x0020
#define FLAG_AUTHFAIL           0x0040
#define FLAG_AUTHTIMEOUT        0x0080
#define FLAG_AUTHABORT          0x0100
#define FLAG_RXRESP             0x0200
#define FLAG_AREQ               0x0400
#define FLAG_ASUCCESS           0x0800
#define FLAG_AFAIL              0x1000
#define FLAG_EAPOLKEYRECVD      0x2000
#define FLAG_WPAVERIFYFAIL      0x4000
#define FLAG_PSKSTART           0x8000

/* used in PORT_CONTROL.mibFlags */
#define FLAG_PORTINITIALIZE     0x0001
#define FLAG_ADMINCTRLDIR       0x0002
#define FLAG_REAUTHENABLED      0x0004
#define FLAG_KEYTXENABLED       0x0008

#define STA_IDENTITY_LEN        32
#define RAD_STATE_LEN           38  //40 is the old value.
//#define RADIUS_SECRET_LEN       32

/************************* MACRO DEFINITIONS ****************************/

#define _inc(x)             ((x == 255) ? x = 0 : x++)
#define _setFlag(x, y)      (x |= y)
#define _clearFlag(x, y)    (x &= ~y)
#define _isFlagTrue(x, y)   ((x & y) ? TRUE : FALSE)

/************************* ENUMERATION **********************************/
enum eEapolPacketType {
    EAPOL_EAP_PACKET = 0,
    EAPOL_START,
    EAPOL_LOGOFF,
    EAPOL_KEY
};

enum eEapPacketCode {
    EAP_REQUEST = 1,
    EAP_RESPONSE,
    EAP_SUCCESS,
    EAP_FAILURE
};

enum eEapPacketType {
    EPT_IDENTITY = 1,
    EPT_NOTIFICATION,
    EPT_NAK,
    EPT_MD5_CHALLENGE,
    EPT_OTP,
    EPT_GENERICTOKENCARD,
    EPT_TLS = 13,
};

/* Authenticator PAE state machine */
/* used in PORT_CONTROL.Status */
enum eAuthenticatorFsmStatus
{
    AP_DISCONNECTED = 0,
    AP_CONNECTING,
    AP_AUTHENTICATING,
    AP_AUTHENTICATED,
    AP_PTKINITNEGOTIATING,
    AP_REKEYNEGOTIATING,
    AP_SETKEYDONE,
    AP_ABORTING,
    AP_HELD,
    AP_INITIALIZE,
    AP_FORCE_AUTH,    
    AP_FORCE_UNAUTH,
    AP_NONE,
};

/* Backend Authentication state machine */
/* used in PORT_CONTROL.Status */
enum eBackendFsmStatus
{
    BA_IDLE = 0,
    BA_REQUEST,
    BA_RESPONSE,
    BA_SUCCESS,
    BA_FAIL,
    BA_TIMEOUT,
    BA_INITIALIZE,
    BA_NONE,
};

enum eKeyType {
    NO_KEY = 0,
    UNICAST_KEY,
    MULTICAST_KEY
};
 
/* used in _1X_MSGQ_STRUCT.op */
#define _1X_MSGQ_EAP_REQID      1
#define _1X_MSGQ_EAP_KEY        2
 
#define WPA_MIC_PADDED          0x5a
#define WPA_MIC_LEN             8
#define WPA_IV_EIV_LEN          8
#define WPA_SNAP_LEN            6
#define WPA_ICV_LEN             4
 
#endif /* __1XDEFS_H__ */

