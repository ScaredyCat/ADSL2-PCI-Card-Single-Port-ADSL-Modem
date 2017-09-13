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
/*  File Name: 1xtdfs.h                                                      */ 
/*  File Description: This file includes all the type definitions for        */
/*                    802.1x module                                          */
/*                                                                           */ 
/*  History:                                                                 */ 
/*      Date        Author      Version     Description                      */ 
/*      ----------  ---------   -------     --------------                   */
/*      02/21/2002  Joy Lin     V1.0        Initial Version                  */ 
/*                                                                           */ 
/*****************************************************************************/
 

#ifndef __1XTDFS_H__
#define __1XTDFS_H__

#include "hash.h"

/* Data Structure for 802.1x module */

/* for port management, need to re-arrange member for alignment */
typedef struct port_control {
    hashEntry_t    hashEntry;     /* This must be the first. */
    UINT16  reAuthWhen;     /* Reauthentication Timer FSM */

    //keep this struct size in DWORD and
    //I read these four bytes below as a long    
    UINT8   portStatus;     /* Unauthorized=0/Authorized=1 */   
    UINT8   apStatus;       /* Authenticator PAE FSM */
    UINT8   baStatus;       /* Backend Authentication FSM */
    UINT8   reqCount;       /* Backend Authentication FSM */
    
    UINT8   currentId;      /* EPAOL's Identifier */
    UINT8   reAuthCount;    /* Authenticator PAE FSM */   
    UINT8   txWhen;         /* Authenticator PAE FSM */
    UINT8   quietWhile;     /* Authenticator PAE FSM */
    UINT8   aWhile;         /* Backend Authentication FSM */
    UINT8   individualKeyIdx;
   
    /* 0x0001: eapLogoff, set if EAPOL-Logoff received */
    /* 0x0002: eapStart, set if EAPOL-Start received */
    /* 0x0004: rxRespId, set if EAP-Packet/Response/Identity received */
    /* 0x0008: reAuthenticate, set when reAuthWhen decrease to zero */
    /* 0x0010: authStart, set when enter AUTHENTICATING state */
    /* 0x0020: authSuccess, set by Backend Authentication FSM */
    /* 0x0040: authFail, set by Backend Authentication FSM */
    /* 0x0080: authTimeout, set by Backend Authentication FSM */
    /* 0x0100: authAbort, set when enter ABORTING state */
    /* 0x0200: rxResp, set if EAP-Packet/Response(other than Identity) received */
    /* 0x0400: aReq, set if an EAP packet is received from the AS */
    /* 0x0800: aSuccess, set if an Accept packet is received from the AS */
    /* 0x1000: aFail, set if a Reject packet is received from the AS */
    UINT16  flags;

//    UINT8   unicastKey[16];
    UINT8   TxRxMICKey[16];	    //First 8 bytes is the Tx-key, the next 8 bytes is Rx-Key.
    UINT8   StaSessionKey[32];
    UINT8   userName[STA_IDENTITY_LEN]; //32
    
    // IAS has 0x18 bytes, freeRadius in linux has 0x26 bytes
    UINT8   radRespState[RAD_STATE_LEN]; //38
    UINT8   ReTxCount;
#ifdef APPS_INCLUDE_SNMP    
    UINT8   portNumber;

    UINT16  mibFlags;
    UINT16  reAuthPeriod;
    
    UINT8   quietPeriod;
    UINT8   txPeriod;
    UINT8   suppTimeout;    
    UINT8   serverTimeout;
    
    UINT8   authPortControl;
    UINT8   maxReq;
    UINT16  framesRx;    
    
    UINT32  octetsRx;
    UINT32  octetsTx;
    
    UINT16  framesTx;
    UINT16  entersConnecting;
    UINT16  entersAuthenticating;
    UINT16  authSuccessWhileAuthenticating;
    
    UINT16  backendResponses;
    UINT16  eapolStartFramesRx;
    UINT16  eapolRespIdFramesRx;
    UINT16  eapolRespFramesRx;
    
#else
    UINT8   UnUsed;     //So far, this byte is unused. Just for packed.
#endif

} PORT_CONTROL, *PORT_CONTROL_ID;

typedef struct auth_fsm_func {
    int (*funcPtr)(PORT_CONTROL *);
} AUTH_FSM_FUNC, *AUTH_FSM_FUNC_ID;

typedef struct backend_fsm_func {
    int (*funcPtr)(PORT_CONTROL *, UINT8 *);
} BACKEND_FSM_FUNC, *BACKEND_FSM_FUNC_ID;

typedef struct mac_frame {
   UINT8    sourceAddr[6];
   UINT16   ethernetType;
   UINT8    protocolVersion;
   UINT8    packetType;
   UINT16   bodyLength;
   UINT8    eapCode;
   UINT8    eapId;
   UINT16   eapLength;
   UINT8    eapType;
   UINT8    eapTypeData;
} MAC_FRAME, *MAC_FRAME_ID;

typedef struct eapol_packet {
   UINT8    protocolVersion;
   UINT8    packetType;
   UINT16   bodyLength;
   UINT8    eapCode;
   UINT8    eapId;
   UINT16   eapLength;
   UINT8    eapType;
   UINT8    eapTypeData;
} EAPOL_PACKET, *EAPOL_PACKET_ID;

typedef struct eap_packet {
   UINT8    eapCode;
   UINT8    eapId;
   UINT16   eapLength;
   UINT8    eapType;
   UINT8    eapTypeData;
} EAP_PACKET, *EAP_PACKET_ID;

struct key_descriptor {
   UINT8    type;
   UINT8    len_hi;
   UINT8    len_lo;
   UINT8    replayCounter[8];
   UINT8    keyIV[16];
   UINT8    keyIndex;
   UINT8    signature[16];
   UINT8    key[_1X_KEY_LEN];
}__attribute__((__packed__));

typedef struct key_descriptor KEY_DESCRIPTOR, *KEY_DESCRIPTOR_ID;

typedef struct rad_packet {
   UINT8    code;
   UINT8    identifier;
   UINT16   length;
   UINT8    authenticator[16];
   UINT8    attributes;
} RAD_PACKET, *RAD_PACKET_ID;

#define FLAG_KEY_DESCRIPTOR_VERSION_1   0x01	//Key information bit 0~2. TKIP.
#define FLAG_KEY_DESCRIPTOR_VERSION_2   0x02	//Key information bit 0~2. AES.
#define FLAG_KEY_TYPE_PAIRWISE_KEY      0x08	//Key information bit 3.
#define FLAG_KEY_INDEX                  0x20	//Key information bit 4~5 (Key_ID = 2)
#define FLAG_KEY_INSTALL                0x40	//Key information bit 6.
#define FLAG_KEY_ACK                    0x80	//Key information bit 7.
#define FLAG_KEY_MIC                    0x01	//Key information bit 8.
#define FLAG_KEY_SECURE                 0x02	//Key information bit 9.
#define FLAG_KEY_ERROR                  0x04	//Key information bit 10.
#define FLAG_KEY_REQUEST                0x08	//Key information bit 11.

#define CHECK_TYPE_MD5                  0x01
#define CHECK_TYPE_SHA1                 0x02

struct wpa_key_packet
{
	UINT8	ProtocolVersion;
	UINT8	PacketType;
	UINT16	PacketBodyLength;
	UINT8	DescriptType;
	UINT8	KeyInfomation[2];
	UINT8	KeyLength[2];
	UINT8	ReplayCounter[8];
	UINT8	KeyNonce[32];
	UINT8	EapolKeyIV[16];
	UINT8	KeySRC[8];
	UINT8	KeyID[8];
	UINT8	KeyMIC[16];
	UINT8	KeyDataLength[2];
}__attribute__((__packed__));

typedef struct wpa_key_packet WPA_KEY_PACKET, *WPA_KEY_PACKET_ID;

typedef struct __1X_MSGQ_STRUCT
{
    UINT16 op;
    UINT8  staMAC[6];
} _1X_MSGQ_STRUCT;

#endif /* __1XTDFS_H__ */

