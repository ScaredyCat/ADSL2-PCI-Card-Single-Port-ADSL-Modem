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
/*  File Name: 1xback.c                                                      */
/*  File Description: implement 802.1x backend authentication FSM            */
/*                                                                           */
/*  History:                                                                 */
/*      Date         Author       Version    Description                     */
/*      02/21/2002   Joy Lin      1.0        initial version                 */
/*                                                                           */
/*****************************************************************************/
#ifdef WLAN_HOSTOS_LINUX
#include <asm/byteorder.h>
#elif defined(WLAN_HOSTOS_VXWORKS)
#include <netinet/if_ether.h>
#endif      //WLAN_HOSTOS_LINUX

#define INCLUDE_OS_DEP
#include "wlan_include.h"

#include "ifx_wdrv_types.h"
#include "ifx_wdrv_debug.h"
#include "ifxadm_wdrv_init.h"
#include "if_wlan.h"
#include "inet_addr.h"
#include "radDef.h"
#include "1xdefs.h"
#include "1xtdfs.h"
#include "1xvars.h"
#include "1xproto.h"
#include "radDS.h"
#include "radVar.h"
#include "radMD5.h"
#include "sibapi.h"
#include "isram.h"
#include "radAuth.h"


#define APPS_IF_ADM0_LAN         "adm0"
#define APPS_IF_ADM1_WAN         "adm1"
#define APPS_IF_ADM2_WLAN         "adm2"


//====== local functions =======================================================

/* Backend Authentication state machine */
static int doIdle(PORT_CONTROL *portCtrl, UINT8 *pEapolPkt);
static int doRequest(PORT_CONTROL *portCtrl, UINT8 *pEapolPkt);
static int doResponse(PORT_CONTROL *portCtrl, UINT8 *pEapolPkt);
static int doSuccess(PORT_CONTROL *portCtrl, UINT8 *pEapolPkt);
static int doFail(PORT_CONTROL *portCtrl, UINT8 *pEapolPkt);
static int doTimeout(PORT_CONTROL *portCtrl, UINT8 *pEapolPkt);


//====== golbal variables ======================================================

extern WLAN_CONTENT_ID WLANDrvObjPtr;

/* user can provisioning */
int maxReq          = DEF_MAXREQ;
int suppTimeout     = DEF_SUPPTIMEOUT;
int serverTimeout   = DEF_SERVERTIMEOUT;
int keyTxEnabled    = TRUE; // !!! always true now

UINT8 MulticastKey[16];
UINT8 EapSessionKey[64];

//====== local variables =======================================================

static BACKEND_FSM_FUNC BackendFsmDoFunc[] = {    
    {doIdle},
    {doRequest},
    {doResponse},
    {doSuccess},
    {doFail},
    {doTimeout},    
};

static UINT8 EapolToSta[LENOFRXPKT];
static UINT16 KeyCounter = 0;

int getApIpAddr (UINT32 *serverIp, UINT32 *p);

/*-----------------------------------------------------------------------------
* ROUTINE NAME - BackendFSM
*------------------------------------------------------------------------------
* FUNCTION: IEEE 802.1x Figure 8-12 Backend Authentication state machine
*
* INPUT   : pEapolPkt: not include type/length(0x888e)
* OUTPUT  : None
* RETURN  : None
* NOTE:
*-----------------------------------------------------------------------------*/
void BackendFSM(PORT_CONTROL *portCtrl, UINT8 *pEapolPkt)
{
    int oldState, newState;
    
    oldState = portCtrl->baStatus;
    newState = BA_NONE;

    DEBUG_8202(_bDisp802_1x_, "[ 1X ] %s: oldState= %d",__FUNCTION__,oldState);
    
    if (_isFlagTrue(portCtrl->flags, FLAG_AUTHABORT))
    {
        _clearFlag (portCtrl->flags, FLAG_AUTHABORT);
        newState = BA_IDLE;
    }
    else    
    switch (oldState)
    {
    case BA_IDLE:
        if (_isFlagTrue(portCtrl->flags, FLAG_WPAVERIFYFAIL))
        {
            newState = BA_IDLE;
        }
        else if (_isFlagTrue(portCtrl->flags, FLAG_AUTHSTART))
            newState = BA_RESPONSE;
        break;
        
    case BA_REQUEST:
        if (_isFlagTrue(portCtrl->flags, FLAG_RXRESP))
            newState = BA_RESPONSE;
        
        else if (portCtrl->aWhile == 0)
        {
            if (portCtrl->reqCount < maxReq)
                newState = BA_REQUEST;
            else
                newState = BA_TIMEOUT;
        }
        break;
        
    case BA_RESPONSE:
        if (_isFlagTrue(portCtrl->flags, FLAG_AREQ))
            newState = BA_REQUEST;
            
        else if (_isFlagTrue(portCtrl->flags, FLAG_ASUCCESS))
            newState = BA_SUCCESS;
            
        else if (_isFlagTrue(portCtrl->flags, FLAG_AFAIL))
            newState = BA_FAIL;
            
        else if (portCtrl->aWhile == 0)
            newState = BA_TIMEOUT;
        break;
        
    default: break;   
    }

    if (WDEBUG_LEVEL(_bDisp802_1x_)) printf(", newState= %d",newState);

    if (newState != BA_NONE)
    {
        portCtrl->baStatus = newState;
        if ((*(BackendFsmDoFunc[newState].funcPtr))(portCtrl, pEapolPkt) == 1)
        {
            /* only IDLE state will trigger
               Backend Authentication FSM */
            AuthenticatorFSM(portCtrl, NULL);
        }
    }    
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - doIdle
*------------------------------------------------------------------------------
* FUNCTION: IEEE 802.1x IDLE state in the Backend Authentication FSM
*
* INPUT   : None
* OUTPUT  : None
* RETURN  : None
* NOTE:
*-----------------------------------------------------------------------------*/
static int doIdle (PORT_CONTROL *portCtrl, UINT8 *pEapolPkt)
{
    if (_isFlagTrue(portCtrl->flags, FLAG_WPAVERIFYFAIL))
    {
        return 1;
    }

    _clearFlag (portCtrl->flags, FLAG_AUTHSTART);
    portCtrl->reqCount = 0;
    
    portCtrl->baStatus = BA_IDLE;
    
    return 1;
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - doRequest
*------------------------------------------------------------------------------
* FUNCTION: IEEE 802.1x REQUEST state in the Backend Authentication FSM
*
* INPUT   : None
* OUTPUT  : None
* RETURN  : None
* NOTE:
*-----------------------------------------------------------------------------*/
static int doRequest (PORT_CONTROL *portCtrl, UINT8 *pEapolPkt)
{
    if (pEapolPkt != NULL) // aReq = TRUE
        txReq (portCtrl, (EAPOL_PACKET_ID)pEapolPkt);
    else // aWhile = 0
        txReq (portCtrl, (EAPOL_PACKET_ID)EapolToSta);
        
    portCtrl->aWhile = suppTimeout;
    portCtrl->reqCount++;
    
    return 0;
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - doResponse
*------------------------------------------------------------------------------
* FUNCTION: IEEE 802.1x RESPONSE state in the Backend Authentication FSM
*
* INPUT   : None
* OUTPUT  : None
* RETURN  : None
* NOTE:
*-----------------------------------------------------------------------------*/
static int doResponse (PORT_CONTROL *portCtrl, UINT8 *pEapolPkt)
{
    _clearFlag (portCtrl->flags, FLAG_AREQ);
    _clearFlag (portCtrl->flags, FLAG_ASUCCESS);
    _clearFlag (portCtrl->flags, FLAG_AUTHTIMEOUT);
    _clearFlag (portCtrl->flags, FLAG_RXRESP);
    _clearFlag (portCtrl->flags, FLAG_AFAIL);
    
    portCtrl->aWhile = serverTimeout;
    portCtrl->reqCount = 0;

    if (pEapolPkt != NULL)    
        sendRespToServer (portCtrl, pEapolPkt+4);
    portCtrl->radRespState[0] = '\0';
    
    #ifdef APPS_INCLUDE_SNMP
    (portCtrl->backendResponses)++;
    #endif
        
    return 0;
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - doSuccess
*------------------------------------------------------------------------------
* FUNCTION: IEEE 802.1x SUCCESS state in the Backend Authentication FSM
*
* INPUT   : None
* OUTPUT  : None
* RETURN  : None
* NOTE:
*-----------------------------------------------------------------------------*/
static int doSuccess (PORT_CONTROL *portCtrl, UINT8 *pEapolPkt)
{
    UINT8 encryptType;
    
    txCannedSuccess (portCtrl);
    
    _setFlag (portCtrl->flags, FLAG_AUTHSUCCESS);
    
    if ( (keyTxEnabled == TRUE) &&
         // got MS-MPPE-Send-Key and MS-MPPE-Recv-Key in Access-Accept
         (*((UINT32 *)EapSessionKey) != 0L) &&
         (*((UINT32 *)(EapSessionKey+32)) != 0L) )
    {
        SIBCfg_SecurityAlg(&encryptType);
        
        // if encryptType = ENCRYP_MODE_SHARED_KEY, can't call txKey(portCtrl, NO_KEY)
        // otherwise STA will set key(index 0) with key length = 0
        if (encryptType == ENCRYP_MODE_OPEN_SYS)
        {
            txKey (portCtrl, NO_KEY);
        }
        else if (encryptType == ENCRYP_MODE_DOT1X_KEY) 
        {
            txKey (portCtrl, MULTICAST_KEY);
            txKey (portCtrl, UNICAST_KEY);
        }
        else if (encryptType >= ENCRYP_MODE_TKIP)
        {
            memcpy(portCtrl->StaSessionKey, EapSessionKey, 32);
        }
    }   

    return (doIdle (portCtrl, NULL));
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - doFail
*------------------------------------------------------------------------------
* FUNCTION: IEEE 802.1x FAIL state in the Backend Authentication FSM
*
* INPUT   : None
* OUTPUT  : None
* RETURN  : None
* NOTE:
*-----------------------------------------------------------------------------*/
static int doFail (PORT_CONTROL *portCtrl, UINT8 *pEapolPkt)
{
    txCannedFail (portCtrl);
    
    _setFlag (portCtrl->flags, FLAG_AUTHFAIL);
    
    return (doIdle (portCtrl, NULL));
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - doTimeout
*------------------------------------------------------------------------------
* FUNCTION: IEEE 802.1x TIMEOUT state in the Backend Authentication FSM
*
* INPUT   : None
* OUTPUT  : None
* RETURN  : None
* NOTE:
*-----------------------------------------------------------------------------*/
static int doTimeout (PORT_CONTROL *portCtrl, UINT8 *pEapolPkt)
{
    if (portCtrl->portStatus == PORT_UNAUTHORIZED)
        txCannedFail (portCtrl);
    
    _setFlag (portCtrl->flags, FLAG_AUTHTIMEOUT);
    
    return (doIdle (portCtrl, NULL));
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - ReverseKey
*------------------------------------------------------------------------------
* FUNCTION: 
*           
* INPUT   :
* OUTPUT  : None
* RETURN  : None
* NOTE    : 
*-----------------------------------------------------------------------------*/

void ReverseKey(UINT16 ReqUserTableId, UINT8 *pInBuf)
{
    UINT8	b[16], p[16], c[16];
	UINT8	buf[72];
	UINT8   pPlaneText[64];
	MD5_CTX	Context;
	int     i, j, k;
	int     svrIdx;
	int     len, secretLen;
	UINT8   vendorType = *pInBuf;

    if (WDEBUG_LEVEL( _bDisp802_1x_) )       
    {       	                
        printf("\n[ 1X ] %s: cipher text:",__FUNCTION__);
        _1xDumpHex(pInBuf, *(pInBuf+1));
    }
	k = (*(pInBuf+1) - 4) / 16; // k should be 3
	if (k > 4) k = 4; // ???, prevent pPlaneText[] overflow
	svrIdx = gUserReqTable[ReqUserTableId]->svrIdx;
	secretLen = strlen((char *)(gSrvTable[svrIdx]->Secret));
    for (j=0; j<k; j++)
    {	
        if (j==0)
        {
            memcpy(buf, gSrvTable[svrIdx]->Secret, secretLen);
    	    memcpy(buf+secretLen, gUserReqTable[ReqUserTableId]->RequestAuth, LENOFAUTHENTICATOR);
    	    memcpy(buf+secretLen+LENOFAUTHENTICATOR, pInBuf+2, 2);
	        len = secretLen+LENOFAUTHENTICATOR+2;
        }
        else
        {
            memcpy(buf, gSrvTable[svrIdx]->Secret, secretLen);
            memcpy(buf+secretLen, c, 16);
            len = secretLen + 16;
        }
	    MD5Init(&Context);
	    MD5Update(&Context, buf, len);
	    MD5Final(&Context);
	
	    memcpy(b, Context.digest, 16);
	    memcpy(c, pInBuf+4 + (j * 16), 16);
	
        for (i=0;i<16;i++)
            p[i] = c[i] ^ b[i];
            
	    memcpy(pPlaneText+ (j * 16), p, 16);
    }
    // pPlaneText[0] should be 32, it's the length of key
    if (vendorType == MS_MPPE_SEND_KEY) // MS-MPPE-Send-Key
        memcpy(EapSessionKey+32, pPlaneText+1, 32);
    else                                // MS-MPPE-Recv-Key
        memcpy(EapSessionKey, pPlaneText+1, 32);
        
    if (WDEBUG_LEVEL(  _bDisp802_1x_))       
    {       	                
        printf("\n[ 1X ] %s: plain text:",__FUNCTION__);
        _1xDumpHex(pPlaneText, 40);
    }
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - ReverseSendRecvKey
*------------------------------------------------------------------------------
* FUNCTION: 
*           
* INPUT   : pInBuf = Attribute (1 byte, Send-Key or Recv-Key) +
*                    Attribute Length (1 byte, should be 0x34) +
*                    Salt Field (2 bytes) +
*                    CipherText (48 bytes usually)
*
* OUTPUT  : None
* RETURN  : None
* NOTE    : pPlaneText = key length (1 byte, should be 0x20) +
*                        key sub-fields (32 bytes usually)
*                        padding sub-field ( 1 ~ 15 bytes, will be all zeros)
*
*-----------------------------------------------------------------------------*/

void ReverseSendRecvKey(UINT16 ReqUserTableId, RAD_PACKET_ID pRadPacket)
{
    int attrLen;
    UINT8 attrSubType, attrSubLen;
    UINT8 vendorType;
    UINT8 *pAttr;
    UINT32 vendorId;
    
    *((UINT32 *)EapSessionKey) = 0;
    *((UINT32 *)(EapSessionKey+32)) = 0;
    attrLen = ntohs(pRadPacket->length) - 20;
    pAttr = (UINT8 *)&(pRadPacket->attributes);

    while (attrLen > 0)
    {
        attrSubType = *pAttr;
        attrSubLen = *(pAttr + 1);
        
        if ((attrSubType == AVENDORSPECIFIC) && (attrSubLen > 7))
        {
            memcpy(&vendorId, pAttr+2, 4);
            if (ntohl(vendorId) == VENDORID_MICROSOFT) // it's Microsoft Corp.
            {
                vendorType = *(pAttr + 6);
                // MS-MPPE-Send-Key or MS-MPPE-Recv-Key
                if ((vendorType == MS_MPPE_SEND_KEY) || 
                    (vendorType == MS_MPPE_RECV_KEY))
                    ReverseKey(ReqUserTableId, pAttr+6);
            }
        }
        pAttr += attrSubLen;
        attrLen -= attrSubLen;
    }
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - ProcessRadiusServerResp
*------------------------------------------------------------------------------
* FUNCTION: callback function
*
* INPUT   : ReqUserTableId & pointer point to Radius packet
* OUTPUT  : None
* RETURN  : None
* NOTE:
*-----------------------------------------------------------------------------*/

void ProcessRadiusServerResp (UINT16 ReqUserTableId, UINT8 *pInPacket)
{
    RAD_PACKET_ID pRadPacket;
    PORT_CONTROL *portCtrl;
    int radCode;
    int attrLen;
    UINT8 attrSubType, attrSubLen;
    UINT8 *pAttr, *pAttrEap;
    UINT8 *pEapAppendHere;
    int checkBackendFSM = FALSE;
    int hasEapAttr = FALSE;

    portCtrl = (PORT_CONTROL *)gUserReqTable[ReqUserTableId]->portControl;
    if (portCtrl == NULL)
        return;
    
    if (portCtrl->baStatus != BA_RESPONSE)
        return;
        
    pRadPacket = (RAD_PACKET_ID)pInPacket;
    radCode = pRadPacket->code;
    
    if (radCode == ACCESSACCEPT)
    {
        _setFlag (portCtrl->flags, FLAG_ASUCCESS);
        checkBackendFSM = TRUE;
        ReverseSendRecvKey(ReqUserTableId, pRadPacket);
        //if (*((UINT32 *)EapSessionKey) != 0L) // got unicast key from RADIUS server
        //    memcpy(portCtrl->unicastKey, EapSessionKey, _1X_KEY_LEN);
    }
    else if (radCode == ACCESSREJECT)
    {
        _setFlag (portCtrl->flags, FLAG_AFAIL);
        checkBackendFSM = TRUE;
    }

    attrLen = ntohs(pRadPacket->length) - 20;
    pAttr = (UINT8 *)&(pRadPacket->attributes);

    pEapAppendHere = EapolToSta + 4;
    while (attrLen > 0)
    {
        pAttrEap = NULL;
        attrSubType = *pAttr;
        attrSubLen = *(pAttr + 1);
        
        if (attrSubLen > 2)
        {
            if (attrSubType == ASTATE) /* save Attribute Type -- STATE */
            {
                if (*(pAttr+1) > RAD_STATE_LEN) // !!! display error messgae
                    memcpy(portCtrl->radRespState, pAttr, RAD_STATE_LEN);
                else
                    memcpy(portCtrl->radRespState, pAttr, *(pAttr+1));
            }
            else if (attrSubType == AEAPMESSAGE)
                pAttrEap = pAttr;
        }
        pAttr += attrSubLen;
        attrLen -= attrSubLen;
        
        if (pAttrEap != NULL)
        {
            memcpy(pEapAppendHere, pAttrEap + 2, attrSubLen - 2);
            pEapAppendHere += attrSubLen - 2;
            hasEapAttr = TRUE;
        }
    }
    
    if (hasEapAttr == FALSE) return; // no EAP attribute found

    portCtrl->currentId = *(EapolToSta + 5);
    
    if (*(EapolToSta + 4) == EAP_REQUEST)
    {
        _setFlag (portCtrl->flags, FLAG_AREQ);
        BackendFSM (portCtrl, EapolToSta); 
    }
    else if (checkBackendFSM)
        BackendFSM (portCtrl, NULL);
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - txCannedSuccess
*------------------------------------------------------------------------------
* FUNCTION: 
*
* INPUT   : None
* OUTPUT  : None
* RETURN  : None
* NOTE:
*-----------------------------------------------------------------------------*/

int txCannedSuccess (PORT_CONTROL *portCtrl)
{
    EAPOL_PACKET_ID p;
    UINT16 len = 4;
    
    p = (EAPOL_PACKET_ID)EapolToSta;
    p->protocolVersion = EAPOL_PROTOCOLVERSION;
    p->packetType = EAPOL_EAP_PACKET;
    p->bodyLength = p->eapLength = htons(len);
    p->eapCode = EAP_SUCCESS;
    p->eapId = portCtrl->currentId;
    txEapol(portCtrl->hashEntry.mac, (UINT8 *)p, 8);
    
    return 0;
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - txKey
*------------------------------------------------------------------------------
* FUNCTION: send EAPOL-KEY packet to STA
*
*           refer to RFC 1305: NTP (Network Time Protocol)
*           KEY_DESCRIPTOR_ID->replayCounter[0 ~ 1]: ???
*           KEY_DESCRIPTOR_ID->replayCounter[2 ~ 5]: seconds
*           KEY_DESCRIPTOR_ID->replayCounter[6 ~ 7]: sequence counter
*
* INPUT   : None
* OUTPUT  : None
* RETURN  : None
* NOTE:
*-----------------------------------------------------------------------------*/

int txKey (PORT_CONTROL *portCtrl, int keyType)
{
    UINT8   rc4Key[48];
    UINT32  randNum;
    EAPOL_PACKET_ID p;
    KEY_DESCRIPTOR_ID k;
    UINT16  len, keyCounter;
	UINT32  TNow;
	int     i, j;

	TNow = KNL_SECONDS();
	TNow = htonl(TNow);
	keyCounter = htons(KeyCounter++);
    
    p = (EAPOL_PACKET_ID)EapolToSta;
    k = (KEY_DESCRIPTOR_ID)(EapolToSta+4);
    p->protocolVersion = EAPOL_PROTOCOLVERSION;
    p->packetType = EAPOL_KEY;
    if (keyType == MULTICAST_KEY)
        len = sizeof(KEY_DESCRIPTOR); //0x39
    else
        len = sizeof(KEY_DESCRIPTOR) - _1X_KEY_LEN; //0x2C
    p->bodyLength = htons(len);
    
    k->type = EAPOL_KEY_TYPE_RC4;
    k->len_hi = 0;
    if (keyType == NO_KEY)
        k->len_lo = 0;
    else
        k->len_lo = _1X_KEY_LEN;
        

    if (keyType == MULTICAST_KEY)
        k->keyIndex = MULTICAST_KEY_ID;
    else
        k->keyIndex = UNICAST_KEY_TYPE | UNICAST_KEY_ID;
    
    // TODO: replay counter carries a NTP time value (RFC 1305)
    k->replayCounter[0] = 0;
    k->replayCounter[1] = 0x16; 
    memcpy(&(k->replayCounter[2]), &TNow, 4); 
    memcpy(&(k->replayCounter[6]), &keyCounter, 2); 
    
	for (i = 0; i < 16; i += sizeof(UINT32))
	{
		os_getRand((UINT8*)&randNum, sizeof(randNum));
		memcpy((k->keyIV) + i, &randNum, sizeof(UINT32));
	}
	
	if (keyType == MULTICAST_KEY)
	{
	    /* when I move this multicast key generation to _1xInit() function,
	     * it will be the same everytime after AP reset.
	     */
        if (*(UINT32 *)MulticastKey == 0L)
        {
            // multicast/global key for all stations
	        for (i = 0; i < 16; i += sizeof(UINT32))
	        {
                    os_getRand((UINT8*)&randNum, sizeof(randNum));
		        memcpy(MulticastKey + i, &randNum, sizeof(UINT32));
	        }

	        SetWepKeyId(UNICAST_KEY_ID); // for TX & unicast packet
	    }
	    memcpy(rc4Key, k->keyIV, 16);
	    memcpy(rc4Key+16, EapSessionKey, 32);
	    memcpy(k->key, MulticastKey, _1X_KEY_LEN);
        do_rc4(rc4Key, 48, k->key, _1X_KEY_LEN);
    }
    
    memset(k->signature, 0, 16);
    HMAC_MD5((UINT8 *)p, len+4, EapSessionKey+32, 32, k->signature);
    
    if (WDEBUG_LEVEL(_bDisp802_1x_))       
    {       	                
        printf("\n[ 1X ] %s: EAPOL-Key:",__FUNCTION__);
        _1xDumpHex((UINT8 *)p, len+4);
    }
    
    i = txEapol(portCtrl->hashEntry.mac, (UINT8 *)p, len+4);
    
    if ((i == OK) && (keyType == UNICAST_KEY))
    {
        //set unicast key to individual key area
        if((j = GetFreeIndividualKey()) != -1)
        {
            SetIndividualKey(j, portCtrl->hashEntry.mac, EapSessionKey, _1X_KEY_LEN, NULL);
            portCtrl->individualKeyIdx = j;
        }
        else
        {
            DEBUG_8202(_bDispErrors_, "Err txKey(): no more free individual key space");
            return ERROR; //TODO: error handling !!!
        }
    }    
    return OK;
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - txCannedFail
*------------------------------------------------------------------------------
* FUNCTION: 
*
* INPUT   : None
* OUTPUT  : None
* RETURN  : None
* NOTE:
*-----------------------------------------------------------------------------*/

int txCannedFail (PORT_CONTROL *portCtrl)
{
    EAPOL_PACKET_ID p;
    UINT16 len = 4;

    p = (EAPOL_PACKET_ID)EapolToSta;
    p->protocolVersion = EAPOL_PROTOCOLVERSION;
    p->packetType = EAPOL_EAP_PACKET;
    p->bodyLength = p->eapLength = htons(len);
    p->eapCode = EAP_FAILURE;
    p->eapId = portCtrl->currentId;
    txEapol(portCtrl->hashEntry.mac, (UINT8 *)p, 8);

    return 0;
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - txReqId
*------------------------------------------------------------------------------
* FUNCTION: 
*
* INPUT   : None
* OUTPUT  : None
* RETURN  : None
* NOTE:
*-----------------------------------------------------------------------------*/

int txReqId (PORT_CONTROL *portCtrl)
{
    EAPOL_PACKET_ID p;
    UINT16 len;
    char idBuf[max(DOT1X_NAS_ID_LEN, SIB_AP_NAME_LEN) + 16];
    
    p = (EAPOL_PACKET_ID)EapolToSta;
    p->protocolVersion = EAPOL_PROTOCOLVERSION;
    p->packetType = EAPOL_EAP_PACKET;
    p->eapCode = EAP_REQUEST;
    p->eapId = portCtrl->currentId;
    p->eapType = EPT_IDENTITY;

    memset(idBuf, 0, sizeof(idBuf));
    sprintf((void *)&(p->eapTypeData), "networkid=");
    SIBCfg_ApName(idBuf);
    strncat((void *)&(p->eapTypeData), idBuf, SIB_AP_NAME_LEN);
    strcat((void *)&(p->eapTypeData), ",nasid=");
    memset(idBuf, 0, sizeof(idBuf));
    SIBCfg_dot1xNAS_id(idBuf);
    strncat((void *)&(p->eapTypeData), idBuf, DOT1X_NAS_ID_LEN);
    strcat((void *)&(p->eapTypeData), ",portid=0");

    len = 5 + strlen((void *)&(p->eapTypeData));
    p->bodyLength = p->eapLength = htons(len);

    txEapol(portCtrl->hashEntry.mac, (UINT8 *)p, len+4);
    
    return 0;
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - txReq
*------------------------------------------------------------------------------
* FUNCTION: 
*
* INPUT   : None
* OUTPUT  : None
* RETURN  : None
* NOTE:
*-----------------------------------------------------------------------------*/

int txReq (PORT_CONTROL *portCtrl, EAPOL_PACKET_ID p)
{
    p->protocolVersion = EAPOL_PROTOCOLVERSION;
    p->packetType = EAPOL_EAP_PACKET;
    p->bodyLength = p->eapLength;

    txEapol(portCtrl->hashEntry.mac, (UINT8 *)p, ntohs(p->bodyLength)+4);
    
    return 0;
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - txEapol
*------------------------------------------------------------------------------
* FUNCTION: send EAPOL packet (pEapolPkt with len) to STA (pDestAddr)
*
* INPUT   : None
* OUTPUT  : None
* RETURN  : None
* NOTE:
*-----------------------------------------------------------------------------*/
int txEapol (UINT8 *pDestAddr, UINT8 *pEapolPkt, int len)
{
    struct ether_header EtherHeader;
    int rc;
    pHANDLE wb;
    WLAN_BUF_INFO* pBufInfo;
    
    pBufInfo = allocWlanBufInfoWithWbuf(WLANDrvObjPtr, len+sizeof(struct ether_header), &wb);
    if (!pBufInfo){
        PRINTERRM("txEapol: no mem to alloc");
        return NOK;
    }

    memcpy(EtherHeader.ether_dhost, pDestAddr, 6);
    SIBCfg_MacAddress(EtherHeader.ether_shost);
    EtherHeader.ether_type = htons(ETHERTYPE_EAPOL);  //bj_a

    memcpy(wbuf_put(wb, sizeof(struct ether_header)), &EtherHeader, sizeof(struct ether_header));
    memcpy(wbuf_put(wb, len), pEapolPkt, len);
    
    if (WDEBUG_LEVEL(_bDisp802_1x_))       
    {       	                
      	UINT8 *p;
       	int i;
        printf("\n[ 1X ] %s: len %d, dump data:\n",__FUNCTION__, len);

       p = (UINT8 *)&EtherHeader;
        for (i=0; i < 14; i++)
            printf(" %02x", p[i]);
            
        if (pEapolPkt[1] == 0x03) //EAPOL-Key packet
            _1xDumpHex(pEapolPkt, pEapolPkt[3]+4);
        else
            _1xDumpHex(pEapolPkt, 40); //len);
    }

    dot1x_xmit_toWlan(pBufInfo);
    
    #ifdef APPS_INCLUDE_SNMP
    {
    PORT_CONTROL *portCtrl;
        
    portCtrl = PortDbInquiry(pDestAddr);
    if (portCtrl)
    {
        (portCtrl->framesTx)++;
        portCtrl->octetsTx += len;
    }
    }
    #endif
        
    return OK;
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - sendRespToServer
*------------------------------------------------------------------------------
* FUNCTION: 
*
* INPUT   : None
* OUTPUT  : None
* RETURN  : None
* NOTE:
*-----------------------------------------------------------------------------*/

int sendRespToServer (PORT_CONTROL *portCtrl, UINT8 *pEapPkt)
{
    RAD_CLI_AUTH_INPUT  radAuth;
    EAP_USER_INFO       userInfo;
    int ret;

    if (RadiusServer[0].ServerIPAddress == 0L) // no RADIUS server set
        return ERROR;

    ret = getApIpAddr(&(RadiusServer[0].ServerIPAddress), &radAuth.NasIPAddress);
    if (ret != OK)
        return ERROR;
        
    radAuth.ProtocolType = RADEAP;
    radAuth.pUserInfoEAP = &userInfo;
    
    userInfo.portControl = (void *)portCtrl;
    strcpy (userInfo.UserName, portCtrl->userName);
    SIBCfg_MacAddress(userInfo.CalledStaId);
    memcpy (userInfo.CallingStaId, portCtrl->hashEntry.mac, 6);
    userInfo.FramedMtu = RADFRAMEMTU;
    userInfo.EapMsg = pEapPkt;
    userInfo.State  = portCtrl->radRespState;

    SIBCfg_dot1xNAS_id(radAuth.NasId);
    
    radAuth.NasPort = RADNASPORT;
    radAuth.NasPortType = NASWIRELESS80211;
    radAuth.pAttributeType = NULL;
    RadAuthentication (&radAuth, &RadiusServer[0], ProcessRadiusServerResp);
    
    return OK;
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - getApIpAddr
*------------------------------------------------------------------------------
* FUNCTION: 
*
* INPUT   : None
* OUTPUT  : None
* RETURN  : None
* NOTE:
*-----------------------------------------------------------------------------*/

int getApIpAddr (UINT32 *serverIp, UINT32 *p)
{
    struct in_addr *ina;
    *p = 0;

    ina = (struct in_addr *) serverIp;
    WDEBUG_DESC(WDL_WPA, "Radius server IP Address: %s--(0x%x)\n", inet_ntoa( *ina ), *serverIp);

    *p = SIBCfg_myOwnIpAddr(serverIp);

    ina = (struct in_addr *) p;
    WDEBUG_DESC(WDL_WPA, "NAS IP Address: %s--(0x%x)\n", inet_ntoa( *ina ), *p);

    if (*p == 0)
        return ERROR;
    else
        return OK;
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - GetFreeIndividualKey
*------------------------------------------------------------------------------
* FUNCTION:
*
* INPUT   : None
* OUTPUT  : None
* RETURN  : -1 not found
* NOTE:
*-----------------------------------------------------------------------------*/

int GetFreeIndividualKey(void)
{
    int keyIdx;
    UINT32 Value32;
    
    //for (keyIdx=0; keyIdx<64; keyIdx++)
    for (keyIdx=0; keyIdx<32; keyIdx++)
    {
        Value32 = ReadAsicSram((SRAM_INDIVIDUAL_KEY_BASE + (keyIdx*6)), PAGE_WEP_TABLE);
        if ((Value32 & 0xff) == 0)
            return (keyIdx);
    }
    
    return (-1);
}

