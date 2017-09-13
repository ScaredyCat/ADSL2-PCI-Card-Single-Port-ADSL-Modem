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
/*  File Name: 1xrecv.c                                                      */
/*  File Description: 802.1x task main function                              */
/*                                                                           */
/*  History:                                                                 */
/*      Date         Author       Version    Description                     */
/*      02/21/2002   Joy Lin      1.0        initial version                 */
/*                                                                           */
/*****************************************************************************/
#ifdef WLAN_HOSTOS_LINUX
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/skbuff.h>
#include <asm/byteorder.h>
#endif //WLAN_HOSTOS_LINUX

#define INCLUDE_OS_DEP
#include "wlan_include.h"

#include "ifx_wdrv_types.h"
#include "ifx_wdrv_debug.h"
#include "ifxadm_wdrv_init.h"
#include "client_db.h"
#include "wpa_cfg.h"
#include "wpaEncrypt.h"
#include "rc4.h"
#include "radDef.h"
#include "1xdefs.h"
#include "1xtdfs.h"
#include "1xvars.h"
#include "1xproto.h"
#include "802_11_frame.h"

#include "sibapi.h"
#include "radMD5.h"
#include "if_wlan.h"
#include "isram.h"
#include "ifx_wdrv_csr_access.h"



//====== local functions =======================================================

static void WPA_CalcPTK(UINT8 *StaMacAddr);
static int WPA_CheckKeyMIC(UINT8 CheckType);

/* Authenticator PAE state machine */
static int doDisconnected(PORT_CONTROL *portCtrl);
static int doConnecting(PORT_CONTROL *portCtrl);
static int doAuthenticating(PORT_CONTROL *portCtrl);
static int doAuthenticated(PORT_CONTROL *portCtrl);
static int doAborting(PORT_CONTROL *portCtrl);
static int doHeld(PORT_CONTROL *portCtrl);

static int doPtkInitNegotiating(PORT_CONTROL *portCtrl);
static int doReKeyNegotiating(PORT_CONTROL *portCtrl);
static int doSetKeyDone(PORT_CONTROL *portCtrl);

//====== golbal variables ======================================================

struct wlan_buf_listHead eapolintrq = { LIST_HEAD_INIT(eapolintrq.lh), SPIN_LOCK_UNLOCKED };


/* user can provisioning */
int reAuthMax       = DEF_REAUTHMAX;
int txPeriod        = DEF_TXPERIOD;
int quietPeriod     = DEF_QUIETPERIOD;
UINT16 ReAuthPeriod;
UINT8 ReAuthEnabled; /* reauthentication timer FSM */

//Global variables for WPA.
UINT8 GTK[40];
UINT8 *GroupTK1Key, *GroupTxMICKey;
UINT8 GNoStaPassWpa = 0;
UINT16 GLow2KeySRC = 0;
UINT32 GHigh4KeySRC = 0;
UINT8 ANonce[40];

#ifdef APPS_INCLUDE_SNMP
extern UINT32 dot1xPaeSystemAuthControl;
#endif

//====== local variables =======================================================
static UINT8 WPAKeyPkt[256], APMacAddr[6];
static UINT8 SNonce[32], GNonce[40], PMK[32], PTK[80];
static UINT16 ReplayCounter = 0;
static UINT32 ANonceCounter = 0;
UINT8 WPA_MIC_FailureTimer1 = 0;
UINT8 WPA_MIC_FailureTimer2 = 0;
UINT8 WPA_MIC_FailureTimer3 = 0;
UINT8 WPA_MIC_FailureTimer4 = 0;


static AUTH_FSM_FUNC AuthFsmDoFunc[] = {    
    {doDisconnected},
    {doConnecting},
    {doAuthenticating},
    {doAuthenticated},      //Send the 4-way handshake Message1.
    {doPtkInitNegotiating}, //Check the 4-way handshake Message2 and send the Message3.
    {doReKeyNegotiating},   //Check the 4-way handshake Message4 then set PTK and send the Group-key update Message1.
    {doSetKeyDone},         //Check the Group-key update Message2 and set GTK..
    {doAborting},
    {doHeld},    
};

//====== constant variables ====================================================

/*-----------------------------------------------------------------------------
* ROUTINE NAME - ProcessIncomingEapolPacket
*------------------------------------------------------------------------------
* FUNCTION: This module takes action for a time out occurred for a
*           transmitted packet. It retransmits the packet to the same
*           server for a no of times mentioned in the respective server's
*           information structure.
*
* INPUT   : None
* OUTPUT  : None
* RETURN  : None
* NOTE:
*-----------------------------------------------------------------------------*/

void ProcessIncomingEapolPacket (MAC_FRAME_ID pMacFrame, int len)
{
    PORT_CONTROL *portCtrl;
    int i;
    int checkAuthFSM = FALSE;
    WPA_KEY_PACKET_ID WpaPkt;
    UINT8 authType;

    #ifdef APPS_INCLUDE_SNMP
    if (dot1xPaeSystemAuthControl == 2) // DOT1XPAESYSTEMAUTHCONTROL_DISABLE
        return;
    #endif

    if ((pMacFrame->protocolVersion != 1) ||
        (pMacFrame->packetType > EAPOL_KEY)){
        return;
    }

    if ((pMacFrame->packetType == EAPOL_EAP_PACKET) &&
        (pMacFrame->eapCode != EAP_RESPONSE)){
        return;
    }

    if ((portCtrl = PortDbInquiry(pMacFrame->sourceAddr)) == NULL) // STA is coming first time
    {
        if ((portCtrl = AddPortControl(pMacFrame->sourceAddr)) == NULL){ // no more avail PORT
            return;
        }
    }
    else
    {
        if (pMacFrame->packetType == EAPOL_START)
        {
            clearPortControl(portCtrl);
        }
    }
    
    #ifdef APPS_INCLUDE_SNMP
    (portCtrl->framesRx)++;
    portCtrl->octetsRx += (len - 8);
    #endif
        
    SIBCfg_AuthenticationAlg(&authType);

    //if ((pMacFrame->packetType == EAPOL_START) && (authType == AUTH_MODE_8021X_EAP))
    if (pMacFrame->packetType == EAPOL_START)
    {
        portCtrl->ReTxCount = 0; //for WPA & WPA-PSK
        if (authType == AUTH_MODE_8021X_EAP)
        {
            _setFlag(portCtrl->flags, FLAG_EAPSTART);
            checkAuthFSM = TRUE;
        }
        else if (authType == AUTH_MODE_8021X_PSK)
        {
            if ((portCtrl->apStatus >= AP_AUTHENTICATED) && (portCtrl->apStatus < AP_SETKEYDONE))
            {
                //Return and ignore the EAPOL_START packet.
                PRINTLOCM("ignoring confused state of ardong's WinXP") ;
                return;

            }
            else
            {
	            _setFlag(portCtrl->flags, FLAG_PSKSTART);
	            checkAuthFSM = TRUE;
        	}
        }

        #ifdef APPS_INCLUDE_SNMP
        (portCtrl->eapolStartFramesRx)++;
        #endif
    }
    else if (pMacFrame->packetType == EAPOL_LOGOFF)
    {
        _setFlag(portCtrl->flags, FLAG_EAPLOGOFF);
        NotifyDot1xState(portCtrl->hashEntry.mac, DOT1XUNAUTHENTICATED);
        checkAuthFSM = TRUE;
    }    
    else if (
        (portCtrl->apStatus == AP_CONNECTING) &&
        (pMacFrame->packetType == EAPOL_EAP_PACKET) &&
        (ntohs(pMacFrame->bodyLength) >= 6) &&
        (pMacFrame->eapCode == EAP_RESPONSE) &&
        (pMacFrame->eapId == portCtrl->currentId) &&
        (pMacFrame->eapType == EPT_IDENTITY))
    {
        memset(portCtrl->userName, 0, STA_IDENTITY_LEN);
        i = ntohs(pMacFrame->eapLength) - 5;
        if (i > (STA_IDENTITY_LEN-1))
            i = STA_IDENTITY_LEN-1; //TODO: display error message
        memcpy(portCtrl->userName, &(pMacFrame->eapTypeData), i);
        _setFlag(portCtrl->flags, FLAG_RXRESPID);
        checkAuthFSM = TRUE;
        
        #ifdef APPS_INCLUDE_SNMP
        (portCtrl->eapolRespIdFramesRx)++;
        #endif        
    }
    else if (
        (portCtrl->baStatus == BA_REQUEST) &&
        (pMacFrame->packetType == EAPOL_EAP_PACKET) &&
        (ntohs(pMacFrame->bodyLength) >= 5) &&
        (pMacFrame->eapCode == EAP_RESPONSE) &&
        (pMacFrame->eapId == portCtrl->currentId) &&
        (pMacFrame->eapType != EPT_IDENTITY) &&
        ((pMacFrame->eapType <= EPT_GENERICTOKENCARD) || (pMacFrame->eapType == EPT_TLS))
        )
    {
        _setFlag(portCtrl->flags, FLAG_RXRESP);
        BackendFSM (portCtrl, &(pMacFrame->protocolVersion));

        #ifdef APPS_INCLUDE_SNMP
        (portCtrl->eapolRespFramesRx)++;
        #endif        
    }
    else if ((portCtrl->apStatus == AP_AUTHENTICATED) ||
             (portCtrl->apStatus == AP_PTKINITNEGOTIATING) ||
             (portCtrl->apStatus == AP_REKEYNEGOTIATING) ||
             (portCtrl->apStatus == AP_SETKEYDONE))
    {
        WpaPkt = (WPA_KEY_PACKET_ID)(((UINT8 *)pMacFrame) + 8);
        
//        DEBUG_8202(_bDisp802_1x_, ("\n[ WPA ] PacketType : PacketBodyLength : DescriptType : KeyLength\n"));
//        DEBUG_8202(_bDisp802_1x_, ("\n          %x,                %d,            %x,         %x\n",
//                   WpaPkt->PacketType,htons(WpaPkt->PacketBodyLength), WpaPkt->DescriptType,WpaPkt->KeyLength[1]));

        if ((WpaPkt->PacketType == EAPOL_KEY) &&
            (ntohs(WpaPkt->PacketBodyLength) >= (sizeof(WPA_KEY_PACKET) - 4)) &&
            (WpaPkt->DescriptType == EAPOL_KEY_TYPE_RSN))
        {
            memset(WPAKeyPkt, 0x00, 256);
          	memcpy(WPAKeyPkt, WpaPkt, (len - 8));
              	
            if ((WpaPkt->KeyInfomation[0] & FLAG_KEY_REQUEST) &&
                (WpaPkt->KeyInfomation[0] & FLAG_KEY_ERROR))
            {
                if (WDEBUG_LEVEL(  _bDisp802_1x_ ))
                {
                    printf("\n[ WPA ] Rx Eapol-Key MIC fail ===>");
                    _1xDumpHex((UINT8 *)WpaPkt, 107);
                }
#if 0 //bj_d
                //I did NOT check the 16 bytes Key-MIC value here.
                if (WpaPkt->KeyInfomation[1] & FLAG_KEY_TYPE_PAIRWISE_KEY)
                {
                    //Key-Type is Pairwise key.
                    //Drop the station who sends the Eapol-Key message.
                    if (!IS_ZERO_MAC(portCtrl->hashEntry.mac))
                    {
                        SIBCfg_SendDisAssoc(portCtrl->hashEntry.mac);    //Also delete record in ClientDB table.
                        DelPortControl(portCtrl->hashEntry.mac);         //Delete record in PortControl table.
                    }
                }
                else
                {
                    PRINTLOCM("droping all station");

                    //Key-Type is Group key.
                    //Drop all station.
                    WPA_DropAllStation();
                }
#endif
                WPA_ProcessEapolKeyRequestPkt();
            }
            else
            {
          	_setFlag(portCtrl->flags, FLAG_EAPOLKEYRECVD);
            checkAuthFSM = TRUE;
        }
    }
    }
    
    if (checkAuthFSM)
    {
        AuthenticatorFSM(portCtrl, &(pMacFrame->protocolVersion));
    }
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - AuthenticatorFSM
*------------------------------------------------------------------------------
* FUNCTION: state transition
*
* INPUT   : pEapolPkt: not include type/length(0x888e)
* OUTPUT  : None
* RETURN  : None
* NOTE:
*-----------------------------------------------------------------------------*/
void AuthenticatorFSM(PORT_CONTROL *portCtrl, UINT8 *pEapolPkt)
{
    int oldState, newState;
    
    oldState = portCtrl->apStatus;
    newState = AP_NONE;

    DEBUG_8202(_bDisp802_1x_, "[ 1X ] %s: oldState= %d ",__FUNCTION__,oldState);
    
    switch (oldState)
    {
    case AP_DISCONNECTED:
        if (_isFlagTrue(portCtrl->flags, FLAG_EAPSTART))
            newState = AP_CONNECTING;
            
        else if (_isFlagTrue(portCtrl->flags, FLAG_PSKSTART))
        {
            newState = AP_AUTHENTICATED;
        }            
        break;
        
    case AP_CONNECTING:
        if (_isFlagTrue(portCtrl->flags, FLAG_EAPLOGOFF) ||
            (portCtrl->reAuthCount > reAuthMax))
            newState = AP_DISCONNECTED;
        
        else // must be (portCtrl->reAuthCount <= reAuthMax)
        {
            if ((portCtrl->txWhen == 0) ||
                _isFlagTrue(portCtrl->flags, FLAG_EAPSTART) ||
                _isFlagTrue(portCtrl->flags, FLAG_REAUTHENTICATE))
            {
                if (portCtrl->reAuthCount < reAuthMax)
                    newState = AP_CONNECTING;
                else
                    newState = AP_DISCONNECTED;
            }
            else if (_isFlagTrue(portCtrl->flags, FLAG_RXRESPID))
                newState = AP_AUTHENTICATING;
            // else: don't care
        }
        break;
        
    case AP_AUTHENTICATING:
        if (_isFlagTrue(portCtrl->flags, FLAG_AUTHSUCCESS))
        {
            newState = AP_AUTHENTICATED;
        }
        else if (_isFlagTrue(portCtrl->flags, FLAG_AUTHFAIL))
            newState = AP_HELD;
            
        else if (_isFlagTrue(portCtrl->flags, FLAG_REAUTHENTICATE) ||
                 _isFlagTrue(portCtrl->flags, FLAG_EAPSTART) ||
                 _isFlagTrue(portCtrl->flags, FLAG_EAPLOGOFF) ||
                 _isFlagTrue(portCtrl->flags, FLAG_AUTHTIMEOUT))
            newState = AP_ABORTING;
        break;
        
    case AP_AUTHENTICATED:
        if (_isFlagTrue(portCtrl->flags, FLAG_EAPSTART) ||
            _isFlagTrue(portCtrl->flags, FLAG_REAUTHENTICATE))
            newState = AP_CONNECTING;
            
        else if (_isFlagTrue(portCtrl->flags, FLAG_EAPLOGOFF))
            newState = AP_DISCONNECTED;
        
        else if (_isFlagTrue(portCtrl->flags, FLAG_PSKSTART))
        {
            newState = AP_AUTHENTICATED;            
        }
        else if (_isFlagTrue(portCtrl->flags, FLAG_EAPOLKEYRECVD))
        {
            portCtrl->ReTxCount = 0;
            newState = AP_PTKINITNEGOTIATING;
        }
        else if (portCtrl->txWhen == 0)
            {
                if (portCtrl->ReTxCount > reAuthMax)
                {
                    //Transmitting the WPA packet over 3 times.
                    newState = AP_DISCONNECTED;
                }
                else
                {
                    //Re-Send the WPA packet. (Message1 of 4-way handshake)
                    newState = AP_AUTHENTICATED;
                }
            }
        break;
        
    case AP_PTKINITNEGOTIATING:
        if (_isFlagTrue(portCtrl->flags, FLAG_WPAVERIFYFAIL))
        {
            newState = AP_DISCONNECTED;
        }
        else if (_isFlagTrue(portCtrl->flags, FLAG_EAPOLKEYRECVD))
        {
            portCtrl->ReTxCount = 0;
            newState = AP_REKEYNEGOTIATING;
        }            
        else if (portCtrl->txWhen == 0)
            {
                if (portCtrl->ReTxCount > reAuthMax)
                {
                    //Transmitting the WPA packet over 3 times.
                    newState = AP_DISCONNECTED;
                }
                else
                {
                    //Re-Send the WPA packet.  (Message3 of 4-way handshake)
                    newState = AP_PTKINITNEGOTIATING;
                }
            }
        // if we received EAPOL-Start in this state, let AP send the 4-way 
        // handshake message 1 to STA on WPA-PSK mode, or send EAP/Req-ID to STA
        // on WPA mode
        else if (_isFlagTrue(portCtrl->flags, FLAG_EAPSTART))
            {
            newState = AP_CONNECTING;
            }
        else if (_isFlagTrue(portCtrl->flags, FLAG_PSKSTART))
        {
            newState = AP_AUTHENTICATED;
        }
        break;

    case AP_REKEYNEGOTIATING:
        if (_isFlagTrue(portCtrl->flags, FLAG_WPAVERIFYFAIL))
        {
            newState = AP_DISCONNECTED;
        }
        else if (_isFlagTrue(portCtrl->flags, FLAG_EAPOLKEYRECVD))
        {
            newState = AP_SETKEYDONE;
        }            
        else if (portCtrl->txWhen == 0)
            {
                if (portCtrl->ReTxCount > reAuthMax)
                {
                    //Transmitting the WPA packet over 3 times.
                    newState = AP_DISCONNECTED;
                }
                else
                {
                    //Re-Send the WPA packet.  (Message1 of Group key update)
                    newState = AP_REKEYNEGOTIATING;
                }
            }
        
        // if we received EAPOL-Start in this state, let AP send the 4-way 
        // handshake message 1 to STA on WPA-PSK mode, or send EAP/Req-ID to STA
        // on WPA mode
        else if (_isFlagTrue(portCtrl->flags, FLAG_EAPSTART))
            {
            newState = AP_CONNECTING;
            }
        else if (_isFlagTrue(portCtrl->flags, FLAG_PSKSTART))
        {
            newState = AP_AUTHENTICATED;
        }
        break;

    case AP_SETKEYDONE:
        if (_isFlagTrue(portCtrl->flags, FLAG_WPAVERIFYFAIL))
        {
            newState = AP_DISCONNECTED;
        }
        else if (_isFlagTrue(portCtrl->flags, FLAG_EAPSTART) ||
            _isFlagTrue(portCtrl->flags, FLAG_REAUTHENTICATE))
        {
            newState = AP_CONNECTING;
        }    
        else if (_isFlagTrue(portCtrl->flags, FLAG_EAPLOGOFF))
        {
            newState = AP_DISCONNECTED;
        }
        // if we received EAPOL-Start in this state, let AP send the 4-way 
        // handshake message 1 to STA on WPA-PSK mode, or send EAP/Req-ID to STA
        // on WPA mode
        else if (_isFlagTrue(portCtrl->flags, FLAG_EAPSTART))
        {
            newState = AP_CONNECTING;
        }        
        else if (_isFlagTrue(portCtrl->flags, FLAG_PSKSTART))
        {
            newState = AP_AUTHENTICATED;
        }        
        break;

    case AP_ABORTING:
        if (!_isFlagTrue(portCtrl->flags, FLAG_AUTHABORT))
        {
            if (_isFlagTrue(portCtrl->flags, FLAG_EAPLOGOFF))
                newState = AP_DISCONNECTED;
            else 
                newState = AP_CONNECTING;
        }
        break;
        
    case AP_HELD:
        // modify by joy to improve performance, not defined in FSM
        //if (portCtrl->quietWhile == 0)
        if ((portCtrl->quietWhile == 0) || 
            (_isFlagTrue(portCtrl->flags, FLAG_EAPSTART)))
            newState = AP_CONNECTING;
        break;
        
    default: break;   
    }
    
    if (WDEBUG_LEVEL( _bDisp802_1x_)) printf(", newState= %d\n",newState);
    
    if (newState != AP_NONE)
    {
        portCtrl->apStatus = newState;
        if ((*(AuthFsmDoFunc[newState].funcPtr))(portCtrl) == 1)
        {
            /* only AUTHENTICATING and ABORTING state will trigger
               Backend Authentication FSM */
            BackendFSM(portCtrl, pEapolPkt);
        }
    }
    
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - doDisconnected
*------------------------------------------------------------------------------
* FUNCTION: IEEE 802.1x DISCONNECTED state in the Authenticator PAE state machine
*
* INPUT   : None
* OUTPUT  : None
* RETURN  : None
* NOTE:
*-----------------------------------------------------------------------------*/
static int doDisconnected(PORT_CONTROL *portCtrl)
{

    portCtrl->portStatus = PORT_UNAUTHORIZED;
    _clearFlag(portCtrl->flags, FLAG_EAPLOGOFF);
    portCtrl->reAuthCount = 0;
    txCannedFail (portCtrl); 
    _inc(portCtrl->currentId);
    
    /* Reauthentication Timer FSM, 
       do not decrease it in _1xTimeoutHandler() function */
    portCtrl->reAuthWhen = 0;

    _clearFlag(portCtrl->flags, FLAG_EAPSTART);
    
    NotifyDot1xState(portCtrl->hashEntry.mac, DOT1XUNAUTHENTICATED);
    
    _clearFlag (portCtrl->flags, FLAG_WPAVERIFYFAIL);
    _clearFlag(portCtrl->flags, FLAG_PSKSTART);
    
    if (portCtrl->individualKeyIdx != KEY_UNASSIGNED)
    {
        SetIndividualKey(portCtrl->individualKeyIdx, NULL, NULL, 0, NULL);
        portCtrl->individualKeyIdx = KEY_UNASSIGNED;
        memset(portCtrl->TxRxMICKey, 0x00, 16);
    }
    
    return 0;
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - doConnecting
*------------------------------------------------------------------------------
* FUNCTION: IEEE 802.1x CONNECTING state in the Authenticator PAE state machine
*
* INPUT   : None
* OUTPUT  : None
* RETURN  : None
* NOTE:
*-----------------------------------------------------------------------------*/
static int doConnecting(PORT_CONTROL *portCtrl)
{

    _clearFlag(portCtrl->flags, FLAG_EAPSTART);
    _clearFlag(portCtrl->flags, FLAG_REAUTHENTICATE);

    portCtrl->txWhen = txPeriod;
    _clearFlag(portCtrl->flags, FLAG_RXRESPID);
    txReqId(portCtrl);
    _inc(portCtrl->reAuthCount);
    
    #ifdef APPS_INCLUDE_SNMP
    (portCtrl->entersConnecting)++;
    #endif
    
    return 0;
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - doAuthenticating
*------------------------------------------------------------------------------
* FUNCTION: IEEE 802.1x AUTHENTICATING state in the Authenticator PAE state machine
*
* INPUT   : None
* OUTPUT  : None
* RETURN  : None
* NOTE:
*-----------------------------------------------------------------------------*/
static int doAuthenticating(PORT_CONTROL *portCtrl)
{

    _clearFlag(portCtrl->flags, FLAG_AUTHSUCCESS);
    _clearFlag(portCtrl->flags, FLAG_AUTHFAIL);
    _clearFlag(portCtrl->flags, FLAG_AUTHTIMEOUT);
    _setFlag  (portCtrl->flags, FLAG_AUTHSTART);
    _clearFlag(portCtrl->flags, FLAG_AUTHABORT);
    
    #ifdef APPS_INCLUDE_SNMP
    (portCtrl->entersAuthenticating)++;
    #endif
    
    return 1;
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - doAuthenticated
*------------------------------------------------------------------------------
* FUNCTION: IEEE 802.1x AUTHENTICATED state in the Authenticator PAE state machine
*
* INPUT   : None
* OUTPUT  : None
* RETURN  : None
* NOTE: The function check the Encrypt-Type (ENCRYP_MODE_TKIP && ENCRYP_MODE_AES),
*       then build and send the Message1 of 4-way handshake.
*-----------------------------------------------------------------------------*/
static int doAuthenticated(PORT_CONTROL *portCtrl)
{
    UINT8 encryptType, authType;
    WPA_KEY_PACKET_ID WpaPkt;
    UINT16 tempShort;

    portCtrl->reAuthCount = 0;
    _inc(portCtrl->currentId);

    SIBCfg_SecurityAlg(&encryptType);
    SIBCfg_AuthenticationAlg(&authType);

    //if (encryptType != ENCRYP_MODE_TKIP)
    if (encryptType < ENCRYP_MODE_TKIP)
    {
        portCtrl->portStatus = PORT_AUTHORIZED;

        /* Reauthentication Timer FSM, start this timer */
        if (ReAuthEnabled == REAUTHENABLED_TRUE)
            portCtrl->reAuthWhen = ReAuthPeriod;
                
        NotifyDot1xState(portCtrl->hashEntry.mac, DOT1XAUTHENTICATED);
    }
    else
    {
        if (portCtrl->ReTxCount == 0)
        {
            //Getting the AP's MAC address.
            SIBCfg_MacAddress(APMacAddr);

            memset(PMK, 0x00, 32);

            if (authType == AUTH_MODE_8021X_EAP)
            {
                //WPA_TLS Mode.
                memcpy(PMK, portCtrl->StaSessionKey, 32);
            }
            else if (authType == AUTH_MODE_8021X_PSK)
            {
                //WPA_PSK Mode.
                Dot1xCfg_PSK(PMK);
            }

            _clearFlag(portCtrl->flags, FLAG_AUTHSUCCESS);
            _clearFlag(portCtrl->flags, FLAG_AUTHFAIL);
            _clearFlag(portCtrl->flags, FLAG_AUTHTIMEOUT);
            //This flag can not be cleared before finish the 4-way and Group-key handshake.
            //_setFlag(portCtrl->flags, FLAG_EAPOLKEYRECVD);

            //TNow = KNL_SECONDS();
            //TNow = htonl(TNow);

            memset(WPAKeyPkt, 0x00, 256);
            WpaPkt = (WPA_KEY_PACKET_ID)WPAKeyPkt;

            //Build thr first packet of the 4-way handshake.
            WpaPkt->ProtocolVersion = EAPOL_PROTOCOLVERSION;
            WpaPkt->PacketType = EAPOL_KEY;
            WpaPkt->PacketBodyLength = htons(sizeof(WPA_KEY_PACKET) - 4);
            WpaPkt->DescriptType = EAPOL_KEY_TYPE_RSN;
            //WpaPkt->KeyInfomation[0] is the High byte and WpaPkt->KeyInfomation[1] is the Low byte.
            if (encryptType == ENCRYP_MODE_TKIP)
            {
                WpaPkt->KeyInfomation[0] = 0x00;
                WpaPkt->KeyInfomation[1] = (FLAG_KEY_DESCRIPTOR_VERSION_1 |
                                            FLAG_KEY_TYPE_PAIRWISE_KEY |
                                            FLAG_KEY_ACK);
                WpaPkt->KeyLength[1] = 0x20;
            }
            else if (encryptType == ENCRYP_MODE_AES)
            {
                WpaPkt->KeyInfomation[0] = 0x00;
                WpaPkt->KeyInfomation[1] = (FLAG_KEY_DESCRIPTOR_VERSION_2 |
                                            FLAG_KEY_TYPE_PAIRWISE_KEY |
                                            FLAG_KEY_ACK);
                WpaPkt->KeyLength[1] = 0x10;
            }

            //memcpy(WpaPkt->ReplayCounter + 6, &ReplayCounter, 2);
            tempShort = htons(ReplayCounter++);
            memcpy(WpaPkt->ReplayCounter + 6, &tempShort, 2);
            memcpy(WpaPkt->KeyNonce, ANonce, 32);
        }

        if (WDEBUG_LEVEL( _bDisp802_1x_))
        {
            printf("\n[ WPA ] Tx ===> 4-way handshake Message1 :\n");
            //_1xDumpHex(WPAKeyPkt, sizeof(WPA_KEY_PACKET));
        }

        _inc(portCtrl->ReTxCount);
        portCtrl->txWhen = txPeriod;
        txEapol(portCtrl->hashEntry.mac, WPAKeyPkt, sizeof(WPA_KEY_PACKET));
        _clearFlag(portCtrl->flags, FLAG_EAPOLKEYRECVD);
        _clearFlag(portCtrl->flags, FLAG_PSKSTART);
    }

    #ifdef APPS_INCLUDE_SNMP
    (portCtrl->authSuccessWhileAuthenticating)++;
    #endif
    
    return 0;
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - doAborting
*------------------------------------------------------------------------------
* FUNCTION: IEEE 802.1x ABORTING state in the Authenticator PAE state machine
*
* INPUT   : None
* OUTPUT  : None
* RETURN  : None
* NOTE:
+
*-----------------------------------------------------------------------------*/
static int doAborting(PORT_CONTROL *portCtrl)
{

    _setFlag(portCtrl->flags, FLAG_AUTHABORT);
    _inc(portCtrl->currentId);

    return 1;
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - doHeld
*------------------------------------------------------------------------------
* FUNCTION: IEEE 802.1x HELD state in the Authenticator PAE state machine
*
* INPUT   : None
* OUTPUT  : None
* RETURN  : None
* NOTE:
*-----------------------------------------------------------------------------*/
static int doHeld(PORT_CONTROL *portCtrl)
{

    portCtrl->portStatus = PORT_UNAUTHORIZED;
    portCtrl->quietWhile = quietPeriod;
    _clearFlag(portCtrl->flags, FLAG_EAPLOGOFF);
    _inc(portCtrl->currentId);
    
    /* Reauthentication Timer FSM,
       do not decrease it in _1xTimeoutHandler() function */
    portCtrl->reAuthWhen = 0;
    
    NotifyDot1xState(portCtrl->hashEntry.mac, DOT1XUNAUTHENTICATED);
    
    return 0;
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - _1xTimeoutHandler
*------------------------------------------------------------------------------
* FUNCTION: This module takes action for a time out occurred for a
*           transmitted packet. It retransmits the packet to the same
*           server for a no of times mentioned in the respective server's
*           information structure.
*           Refer to the IEEE 802.1x Port Timers & Reauthentication Timer FSM
* INPUT   : None
* OUTPUT  : None
* RETURN  : None
* NOTE:
*-----------------------------------------------------------------------------*/
void _1xTimeoutHandler_(PORT_CONTROL *portCtrl)
{
    int checkAuthFSM;
    int i;
    UINT8 encryptType;

    SIBCfg_SecurityAlg(&encryptType);

        checkAuthFSM = FALSE;
        if ((ReAuthEnabled == REAUTHENABLED_TRUE) &&
            ((portCtrl->apStatus == AP_CONNECTING) ||
             (portCtrl->apStatus == AP_AUTHENTICATING) ||
             (portCtrl->apStatus == AP_AUTHENTICATED) ||
             (portCtrl->apStatus == AP_ABORTING) ||
             (portCtrl->apStatus == AP_PTKINITNEGOTIATING) ||
             (portCtrl->apStatus == AP_REKEYNEGOTIATING) ||
             (portCtrl->apStatus == AP_SETKEYDONE))
           )  
        {
            if ((portCtrl->reAuthWhen) && (--portCtrl->reAuthWhen == 0))
            {
                if (portCtrl->individualKeyIdx != KEY_UNASSIGNED)
                {
                    //clear individual key
                    SetIndividualKey(portCtrl->individualKeyIdx, NULL, NULL, 0, NULL);
                    portCtrl->individualKeyIdx = KEY_UNASSIGNED;
                }
                #if 1
                /* Reauthentication Timer FSM, 
                   enter REAUTHENTICATE state and restart timer */
                portCtrl->reAuthWhen = ReAuthPeriod;
                _setFlag(portCtrl->flags, FLAG_REAUTHENTICATE);
                    
                // AP_ABORTING state: just decrease reAuthWhen
                if (portCtrl->apStatus != AP_ABORTING)
                    checkAuthFSM = TRUE;
                #else
		        DelClientDb(portCtrl->hashEntry.mac);
	    		DelPortControl(portCtrl->hashEntry.mac);
   		        continue;
                #endif
                    
                DEBUG_8202(_bDisp802_1x_, "[ 1X ] Port %d, reAuthWhen=0\n",i);    
            }
        }

        if (portCtrl->apStatus == AP_HELD)
        {
            if ((portCtrl->quietWhile) && (--portCtrl->quietWhile == 0))
                checkAuthFSM = TRUE;
        }
        else if (portCtrl->apStatus == AP_CONNECTING)
        {
            if ((portCtrl->txWhen) && (--portCtrl->txWhen == 0))
                checkAuthFSM = TRUE;
        }
        else if (((portCtrl->apStatus == AP_AUTHENTICATED) ||
                 (portCtrl->apStatus == AP_PTKINITNEGOTIATING) ||
                 (portCtrl->apStatus == AP_REKEYNEGOTIATING)) && (encryptType >= ENCRYP_MODE_TKIP))
        {
            if ((portCtrl->txWhen) && (--portCtrl->txWhen == 0))
                checkAuthFSM = TRUE;
        }

        if (checkAuthFSM)
            AuthenticatorFSM (portCtrl, NULL);
            
        if (portCtrl->baStatus == BA_REQUEST ||
            portCtrl->baStatus == BA_RESPONSE)
        {
            if ((portCtrl->aWhile) && (--portCtrl->aWhile == 0))
                BackendFSM (portCtrl, NULL);
        }

}

void _1xTimeoutHandler (void)
{
    iterateAllPortControl((pHANDLE) _1xTimeoutHandler_);
    
    if (WPA_MIC_FailureTimer1)
    {
         WPA_MIC_FailureTimer1--;
    }

    if ((WPA_MIC_FailureTimer2) && (--WPA_MIC_FailureTimer2 == 0))
    {
        UINT8 Value8 = 0;
        SIBCfg_SetWlanEnDis(&Value8);
    }

    if (WPA_MIC_FailureTimer3)
    {
         WPA_MIC_FailureTimer3--;
    }

    if ((WPA_MIC_FailureTimer4) && (--WPA_MIC_FailureTimer4 == 0))
    {
        UINT8 Value8 = 0;
        SIBCfg_SetWlanEnDis(&Value8);
    }
    
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - eapolintr
*------------------------------------------------------------------------------
* FUNCTION: Common length and type checks are done here,
*           then the protocol-specific routine is called.
*
* INPUT   : None
* OUTPUT  : None
* RETURN  : None
* NOTE:
*-----------------------------------------------------------------------------*/

void eapolintr()
{
    WLAN_BUF_INFO *m;

    while (!listEmptyWlanBufInfo(&eapolintrq)) {
        m = deQWlanBufInfo(&eapolintrq);
        if (m == NULL){
            PRINTERRM("eapolintr");
            return;
        }

        if (WDEBUG_LEVEL( _bDisp802_1x_))
        {       	                
            PRINTLOCM("w->lan");
            _1xDumpHex(wbuf_getData(getWlanBufWbuf(m)) , min(wbuf_getLen(getWlanBufWbuf(m)), 40)); //m->m_len + 14);
        }

        /* m is the second mbuf, m->m_len = m->m_pkthdr.len,
        * m->m_len = actual length
        * m->m_data - _802_3_HDR_LEN_ = 802.3 frame (DA SA Len/Type FrameBody)
        */
        ProcessIncomingEapolPacket ((MAC_FRAME_ID)(((UINT8*)(wbuf_getData(getWlanBufWbuf(m))))+MAC_ADDR_LEN), wbuf_getLen(getWlanBufWbuf(m)) - MAC_ADDR_LEN);

        freeWlanBufInfo(m);
    }
}

void drainEapolintrQ(void)
{
    while (!listEmptyWlanBufInfo(&eapolintrq)) {
        freeWlanBufInfo(deQWlanBufInfo(&eapolintrq));
    }
}
/*-----------------------------------------------------------------------------
* ROUTINE NAME - IncreaseANonce
*------------------------------------------------------------------------------
* FUNCTION: Increasing the ANonce Counter.
*
* INPUT   : None
* OUTPUT  : None
* RETURN  : None
* NOTE:     //20030523 ardong.
*-----------------------------------------------------------------------------*/
void IncreaseANonce(void)
{
    UINT32 TempCounter;

    TempCounter = htonl(ANonceCounter);
    memcpy(ANonce + 28, &TempCounter, 4);

    if (ANonceCounter == 0xffffffff)
    {
        ANonceCounter = 0;
    }
    else
    {
        ANonceCounter++;
    }
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - doPtkInitNegotiating
*------------------------------------------------------------------------------
* FUNCTION:
*
* INPUT   :
* OUTPUT  :
* RETURN  :
* NOTE    : Check the 4-way handshake Message2 from the STA,
*         : then, build the 4-way handshake Message3 and send it to STA.
*-----------------------------------------------------------------------------*/
static int doPtkInitNegotiating(PORT_CONTROL *portCtrl)
{
    UINT8 IEofAP[] = {0xDD, 0x16, 0x00, 0x50, 0xF2, 0x01, 
                         0x01, 0x00, 0x00, 0x50, 0xF2, 0x02,    //BC/MC : 0x02->TKIP ; 0x04->AES_CCMP.
                         0x01, 0x00, 0x00, 0x50, 0xF2, 0x02,    //UC    : 0x02->TKIP ; 0x04->AES_CCMP.
                         0x01, 0x00, 0x00, 0x50, 0xF2, 0x01};   //Key management : 0x01 : TLS ; 0x02 : PSK
    WPA_KEY_PACKET_ID WpaPkt;
    UINT8 KeyInfo_0, KeyInfo_1, KeyMIC[40], authType;   //To prevent overflow. (16->40)
    UINT16 PktLen, IELen, tempShort;

    UINT8 encryptType, CheckType;

    _clearFlag(portCtrl->flags, FLAG_EAPOLKEYRECVD);
    IELen = IEofAP[1] + 2;
    PktLen = sizeof(WPA_KEY_PACKET) + IELen;

    if (portCtrl->ReTxCount == 0)
    {
        if (WDEBUG_LEVEL(  _bDisp802_1x_ ))
        {
            printf("\n[ WPA ] Rx ===> 4-way handshake Message2 :\n");
            //_1xDumpHex(WPAKeyPkt, WPAKeyPkt[3] + 4);
        }
        SIBCfg_SecurityAlg(&encryptType);
        if (encryptType == ENCRYP_MODE_TKIP)
        {
            IEofAP[11] = IEofAP[17] = 0x02;
            KeyInfo_0 = FLAG_KEY_MIC;
            KeyInfo_1 = FLAG_KEY_DESCRIPTOR_VERSION_1 | FLAG_KEY_TYPE_PAIRWISE_KEY;
            CheckType = CHECK_TYPE_MD5;
        }
        else if (encryptType == ENCRYP_MODE_AES)
        {
            IEofAP[11] = IEofAP[17] = 0x04;
            KeyInfo_0 = FLAG_KEY_MIC;
            KeyInfo_1 = FLAG_KEY_DESCRIPTOR_VERSION_2 | FLAG_KEY_TYPE_PAIRWISE_KEY;
            CheckType = CHECK_TYPE_SHA1;
        } else{
            return NOK;
        }

        WpaPkt = (WPA_KEY_PACKET_ID)WPAKeyPkt;

        memcpy(SNonce, WpaPkt->KeyNonce, 32);
        WPA_CalcPTK(portCtrl->hashEntry.mac);

        //Compare the Key Information bytes and Key MIC.
        if (((KeyInfo_0 != WpaPkt->KeyInfomation[0]) || (KeyInfo_1 != WpaPkt->KeyInfomation[1])) ||
            (WPA_CheckKeyMIC(CheckType) != OK))
        {
            _setFlag(portCtrl->flags, FLAG_WPAVERIFYFAIL);
            IncreaseANonce();
    	    return 1;
        }

        SIBCfg_AuthenticationAlg(&authType);

        if (authType == AUTH_MODE_8021X_EAP)
        {
            IEofAP[IELen - 1] = 0x01;        
        }
        else if (authType == AUTH_MODE_8021X_PSK)
        {
            IEofAP[IELen - 1] = 0x02;
        }

        //TNow = KNL_SECONDS();
        //TNow = htonl(TNow);

        memset(WPAKeyPkt, 0x00, 256);
        //WpaPkt = (WPA_KEY_PACKET_ID)WPAKeyPkt;

        WpaPkt->ProtocolVersion = EAPOL_PROTOCOLVERSION;
        WpaPkt->PacketType = EAPOL_KEY;

        WpaPkt->PacketBodyLength = htons(PktLen - 4);
        WpaPkt->DescriptType = EAPOL_KEY_TYPE_RSN;

        //WpaPkt->KeyInfomation[0] is the High byte and WpaPkt->KeyInfomation[1] is the Low byte.
        if (encryptType == ENCRYP_MODE_TKIP)
        {
            WpaPkt->KeyInfomation[0] = (FLAG_KEY_MIC);
            WpaPkt->KeyInfomation[1] = (FLAG_KEY_DESCRIPTOR_VERSION_1 | FLAG_KEY_TYPE_PAIRWISE_KEY |
                                        FLAG_KEY_INSTALL | FLAG_KEY_ACK);

            WpaPkt->KeyLength[1] = 0x20;
        }
        else if (encryptType == ENCRYP_MODE_AES)
        {
            WpaPkt->KeyInfomation[0] = (FLAG_KEY_MIC);
            WpaPkt->KeyInfomation[1] = (FLAG_KEY_DESCRIPTOR_VERSION_2 | FLAG_KEY_TYPE_PAIRWISE_KEY |
                                        FLAG_KEY_INSTALL | FLAG_KEY_ACK);

            WpaPkt->KeyLength[1] = 0x10;
        }

        //memcpy(WpaPkt->ReplayCounter + 2, &TNow, 4);
        //ReplayCounter++;
        //memcpy(WpaPkt->ReplayCounter + 6, &ReplayCounter, 2);
        tempShort = htons(ReplayCounter++);
        memcpy(WpaPkt->ReplayCounter + 6, &tempShort, 2);
        memcpy(WpaPkt->KeyNonce, ANonce, 32);

        IncreaseANonce();

        IELen = htons(IELen);
        memcpy(WpaPkt->KeyDataLength, &IELen, 2);
        IELen = htons(IELen);
        memcpy(WPAKeyPkt + sizeof(WPA_KEY_PACKET), IEofAP, IELen);
        memset(KeyMIC, 0x00, 40);

        if (encryptType == ENCRYP_MODE_TKIP)
        {
            HMAC_MD5((UINT8 *)WPAKeyPkt, PktLen, PTK, 16, KeyMIC);
        }
        else
        {
            HMAC_SHA1((UINT8 *)WPAKeyPkt, PktLen, PTK, 16, KeyMIC);
        }
        memcpy(WpaPkt->KeyMIC, KeyMIC, 16);
    }

    //PktLen = sizeof(WPA_KEY_PACKET) + IELen;    //Workaround for PktLen be updated.
    if (WDEBUG_LEVEL( _bDisp802_1x_ ))
    {
        printf("\n[ WPA ] Tx ===> 4-way handshake Message3 :\n");
        //_1xDumpHex(WPAKeyPkt, PktLen);
    }
    _inc(portCtrl->ReTxCount);
    portCtrl->txWhen = txPeriod;
    txEapol(portCtrl->hashEntry.mac, WPAKeyPkt, PktLen);

    return 0;
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - doReKeyNegotiating
*------------------------------------------------------------------------------
* FUNCTION:
*
* INPUT   :
* OUTPUT  :
* RETURN  :
* NOTE    : Check the 4-way handshake Message4 from STA and set PTK,
*         : then, build the Group-key update Message1 and send it to STA.
*-----------------------------------------------------------------------------*/
static int doReKeyNegotiating(PORT_CONTROL *portCtrl)
{
    const UINT8   Agere_OUI[3]  = { 0x00, 0x02, 0x2d };
	WPA_KEY_PACKET_ID WpaPkt;
    UINT8 KeyInfo_0, KeyInfo_1, i, KeyMIC[40], rc4Key[32], SkipData[256];   //To prevent overflow. (16->40)
    UINT16 PktLen, tempShort;
    UINT32 randNum, tempLong;
    int j;
    UINT8 encryptType, CheckType;

    _clearFlag(portCtrl->flags, FLAG_EAPOLKEYRECVD);

    SIBCfg_SecurityAlg(&encryptType);
    if (encryptType == ENCRYP_MODE_TKIP)
    {
        PktLen = sizeof(WPA_KEY_PACKET) + 32;
    }
    else if (encryptType == ENCRYP_MODE_AES)
    {
        PktLen = sizeof(WPA_KEY_PACKET) + 16 + 8;
    }else{
        return NOK;
    }
     
    if (portCtrl->ReTxCount == 0)
    {
        if (WDEBUG_LEVEL( _bDisp802_1x_))
        {
            printf("\n[ WPA ] Rx ===> 4-way handshake Message4 :\n");
            //_1xDumpHex(WPAKeyPkt, WPAKeyPkt[3] + 4);
        }
        if (encryptType == ENCRYP_MODE_TKIP)
        {
            KeyInfo_0 = FLAG_KEY_MIC;
            KeyInfo_1 = FLAG_KEY_DESCRIPTOR_VERSION_1 | FLAG_KEY_TYPE_PAIRWISE_KEY;
            CheckType = CHECK_TYPE_MD5;
        }
        else if (encryptType == ENCRYP_MODE_AES)
        {
            KeyInfo_0 = FLAG_KEY_MIC;
            KeyInfo_1 = FLAG_KEY_DESCRIPTOR_VERSION_2 | FLAG_KEY_TYPE_PAIRWISE_KEY;
            CheckType = CHECK_TYPE_SHA1;
        }else{
            return NOK;
        }

        WpaPkt = (WPA_KEY_PACKET_ID)WPAKeyPkt;

        //Compare the Key Information bytes and Key MIC.
        if (((KeyInfo_0 != WpaPkt->KeyInfomation[0]) || (KeyInfo_1 != WpaPkt->KeyInfomation[1])) ||
            (WPA_CheckKeyMIC(CheckType) != OK))
        {
            //Clean the PTK in Individual Key Table.
            SetIndividualKey(portCtrl->individualKeyIdx, NULL, NULL, 0, NULL);
            portCtrl->individualKeyIdx = KEY_UNASSIGNED;
            _setFlag(portCtrl->flags, FLAG_WPAVERIFYFAIL);
            return 1;
        }

        //Start to build the Group-key update Message1.
        //TNow = KNL_SECONDS();
        //TNow = htonl(TNow);

        memset(WPAKeyPkt, 0x00, 256);
        //WpaPkt = (WPA_KEY_PACKET_ID)WPAKeyPkt;

        WpaPkt->ProtocolVersion = EAPOL_PROTOCOLVERSION;
        WpaPkt->PacketType = EAPOL_KEY;
        WpaPkt->PacketBodyLength = htons(PktLen - 4);
        WpaPkt->DescriptType = EAPOL_KEY_TYPE_RSN;

        //memcpy(WpaPkt->ReplayCounter + 2, &TNow, 4);
        tempShort = htons(ReplayCounter++);
        memcpy(WpaPkt->ReplayCounter + 6, &tempShort, 2);
        memcpy(WpaPkt->KeyNonce, GNonce, 32);

        for (i = 0; i < 16; i += sizeof(UINT32))
        {
            os_getRand((UINT8*)&randNum, sizeof(randNum));
            memcpy((WpaPkt->EapolKeyIV) + i, &randNum, sizeof(UINT32));
        }

        //The remain 2 byte (Byte6, Byte7) of the Key-SRC is setting to "0x00".
        tempShort = htons(GLow2KeySRC);
        tempLong = htonl(GHigh4KeySRC);
        memcpy(WpaPkt->KeySRC + 2, &tempLong, 4);
        memcpy(WpaPkt->KeySRC + 6, &tempShort, 2);

        memset(KeyMIC, 0x00, 40);
        if (encryptType == ENCRYP_MODE_TKIP)
        {
            WpaPkt->KeyInfomation[0] = (FLAG_KEY_MIC | FLAG_KEY_SECURE);
            WpaPkt->KeyInfomation[1] = (FLAG_KEY_DESCRIPTOR_VERSION_1 | FLAG_KEY_INDEX | FLAG_KEY_ACK);

            WpaPkt->KeyLength[1] = 0x20;
            WpaPkt->KeyDataLength[1] = 0x20;
            memcpy(WPAKeyPkt + sizeof(WPA_KEY_PACKET), GTK, 32);

            memcpy(rc4Key, WpaPkt->EapolKeyIV, 16);
            memcpy(rc4Key + 16, PTK + 16, 16);
            rc4_encrypt_skip(WPAKeyPkt + sizeof(WPA_KEY_PACKET), WPAKeyPkt + sizeof(WPA_KEY_PACKET),
                             32, rc4Key, 32, SkipData, 256);
            HMAC_MD5((UINT8 *)WPAKeyPkt, PktLen, PTK, 16, KeyMIC);
        }
        else if (encryptType == ENCRYP_MODE_AES)
        {
            UINT8 EncryptGTK[32];

            WpaPkt->KeyInfomation[0] = (FLAG_KEY_MIC | FLAG_KEY_SECURE);
            WpaPkt->KeyInfomation[1] = (FLAG_KEY_DESCRIPTOR_VERSION_2 | FLAG_KEY_INDEX | FLAG_KEY_ACK);

            WpaPkt->KeyLength[1] = 0x10;
            WpaPkt->KeyDataLength[1] = 0x18;    //16 + 8 bytes.

            memset(EncryptGTK, 0x00, 32);
            NIST_AES_Key_Wrap(PTK + 16, GTK, EncryptGTK);
            memcpy(WPAKeyPkt + sizeof(WPA_KEY_PACKET), EncryptGTK, 16 + 8);
            HMAC_SHA1((UINT8 *)WPAKeyPkt, PktLen, PTK, 16, KeyMIC);
        }

        memcpy(WpaPkt->KeyMIC, KeyMIC, 16);
    }

    if (WDEBUG_LEVEL(_bDisp802_1x_))
    {
        printf("\n[ WPA ] Tx ===> Group-key Update Message1 :\n");
        //_1xDumpHex(WPAKeyPkt, PktLen);
    }
    if (portCtrl->individualKeyIdx == KEY_UNASSIGNED)
    {
        if ((j = GetFreeIndividualKey()) != -1)
        {
#if 0
            tSIB_RECORD *pSib;
            pSib = SIB_Get_Ptr();
            if (pSib->SIB_KEY_USED >= 64)
            { 
                j = (pSib->SIB_KEY_USED - 64);
            }
#endif
            if (encryptType == ENCRYP_MODE_TKIP)
            {
                //SetIndividualKey(j, portCtrl->hashEntry.mac, PTK + 32, 16);
                SetIndividualKey(j, portCtrl->hashEntry.mac, PTK + 32, 16, PTK + 48);
                portCtrl->individualKeyIdx = j;
    
                //The Tx/Rx MIC key of PTK will be saving in the portCtrl->TxRxMICKey.
                memcpy(portCtrl->TxRxMICKey, PTK + 48, 16);
            }
            else if (encryptType == ENCRYP_MODE_AES)
            {
                SetIndividualKey(j, portCtrl->hashEntry.mac, PTK + 32, 16, NULL);
                portCtrl->individualKeyIdx = j;
            }

            if (WDEBUG_LEVEL( _bDisp802_1x_ ))
            {
                printf("Updated TKIP key : ");
                _1xDumpHex(PTK, 64);
            }    
        }
    }

for(i=0 ; i < 3 ; i++){  //wpa_speedUP
    txEapol(portCtrl->hashEntry.mac, WPAKeyPkt, PktLen);
}

    _inc(portCtrl->ReTxCount);
    portCtrl->txWhen = 1; //txPeriod; //wpa_speedUP

    WDEBUG_DESC(WDL_WPA, "\n[ WPA ] Tx ===> Group-key Update Message1 :\n");
    
    //Don't care the Group-key msg 2, it has ICV error in this testing driver
    #if 1
    if (memcmp(portCtrl->hashEntry.mac, Agere_OUI, 3) == 0)
    {    
        portCtrl->apStatus = AP_SETKEYDONE;
        // update client_database.Dot1xState
        NotifyDot1xState(portCtrl->hashEntry.mac, DOT1XAUTHENTICATED);
        
        portCtrl->portStatus = PORT_AUTHORIZED;
    
        /* Reauthentication Timer FSM, start this timer */
        if (ReAuthEnabled == REAUTHENABLED_TRUE)
            portCtrl->reAuthWhen = ReAuthPeriod;    
    
        GNoStaPassWpa = 1;    
    }
    #endif
    
    return 0;
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - doSetKeyDone
*------------------------------------------------------------------------------
* FUNCTION:
*
* INPUT   :
* OUTPUT  :
* RETURN  :
* NOTE    : Check the Group-key update Message2 from STA and set GTK.
*-----------------------------------------------------------------------------*/
static int doSetKeyDone(PORT_CONTROL *portCtrl)
{
    WPA_KEY_PACKET_ID WpaPkt;
    UINT8 KeyInfo_0, KeyInfo_1;
    UINT8 encryptType, CheckType;

    _clearFlag(portCtrl->flags, FLAG_EAPOLKEYRECVD);

    if (WDEBUG_LEVEL( _bDisp802_1x_))
    {
        printf("\n[ WPA ] Rx ===> Group-key Update Message2 :\n");
        //_1xDumpHex(WPAKeyPkt, WPAKeyPkt[3] + 4);
    }
    WpaPkt = (WPA_KEY_PACKET_ID)WPAKeyPkt;

    SIBCfg_SecurityAlg(&encryptType);
    if (encryptType == ENCRYP_MODE_TKIP)
    {
        KeyInfo_0 = FLAG_KEY_MIC | FLAG_KEY_SECURE;
        KeyInfo_1 = FLAG_KEY_DESCRIPTOR_VERSION_1;
        CheckType = CHECK_TYPE_MD5;
        
        //Compare the Key Information bytes.
        if (((KeyInfo_0 != WpaPkt->KeyInfomation[0]) || (KeyInfo_1 != (WpaPkt->KeyInfomation[1] & 0xCF))) ||
            (WPA_CheckKeyMIC(CheckType) != OK))
        {
            //Clean the PTK in Individual Key Table.
            SetIndividualKey(portCtrl->individualKeyIdx, portCtrl->hashEntry.mac, NULL, 0, NULL);
            portCtrl->individualKeyIdx = KEY_UNASSIGNED;
            
        	_setFlag(portCtrl->flags, FLAG_WPAVERIFYFAIL);
        	return 1;
        }
    }
    else if (encryptType == ENCRYP_MODE_AES)
    {
        KeyInfo_0 = FLAG_KEY_MIC | FLAG_KEY_SECURE;
        KeyInfo_1 = FLAG_KEY_DESCRIPTOR_VERSION_2 | FLAG_KEY_INDEX;
        CheckType = CHECK_TYPE_SHA1;
    
        //Compare the Key Information bytes.
//      if (((KeyInfo_0 != WpaPkt->KeyInfomation[0]) || (KeyInfo_1 != (WpaPkt->KeyInfomation[1] & 0xCF))) ||
//          (WPA_CheckKeyMIC(CheckType) != OK))
//      {
//          //Clean the PTK in Individual Key Table.
//          //SetIndividualKey(portCtrl->individualKeyIdx, portCtrl->hashEntry.mac, NULL, 0);
//          SetIndividualKey(portCtrl->individualKeyIdx, portCtrl->hashEntry.mac, NULL, 0, NULL);
//          portCtrl->individualKeyIdx = KEY_UNASSIGNED;
//
//    	    _setFlag(portCtrl->flags, FLAG_WPAVERIFYFAIL);
//    	    return 1;
//      }
    }

    // update client_database.Dot1xState
    NotifyDot1xState(portCtrl->hashEntry.mac, DOT1XAUTHENTICATED);
    
    portCtrl->portStatus = PORT_AUTHORIZED;

    /* Reauthentication Timer FSM, start this timer */
    if (ReAuthEnabled == REAUTHENABLED_TRUE)
        portCtrl->reAuthWhen = ReAuthPeriod;    

    GNoStaPassWpa = 1;

    return 0;
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - WPA_GenerateNonce
*------------------------------------------------------------------------------
* FUNCTION:
*
* INPUT   :
* OUTPUT  :
* RETURN  :
* NOTE    : The purpose is to generate the ANonce and GNonce.
*-----------------------------------------------------------------------------*/
void WPA_GenerateNonce(UINT8 *Nonce)
{
    const UINT8 NONCE_CONST[] = "Init Counter";
    UINT8 i, RandomNum[32], NonceTemp[14];
    UINT32 RandNum, TimeNow;
    
    for(i = 0; i < 28; i += sizeof(UINT32))
	{
		os_getRand((UINT8*)&RandNum, sizeof(RandNum));
		memcpy(RandomNum + i, &RandNum, sizeof(UINT32));
	}
    
 /* //bj_r
    time(&TimeNow);
    TimeNow = htonl(TimeNow);
*/
    os_getRand((UINT8*)&TimeNow, sizeof(TimeNow));
 
	memcpy(NonceTemp, APMacAddr, 6);
	memcpy(NonceTemp + 6, &TimeNow, 4);	//The remain 4 bytes of the NTP Time is set to 0x00.
	memset(NonceTemp + 10, 0, 4);       //Clear the last 4 bytes.
	//Generate the Nonce.
	PRF(RandomNum, 32, (UINT8 *)NONCE_CONST, 12, NonceTemp, 14, Nonce, 32);
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - WPA_CalcPTK
*------------------------------------------------------------------------------
* FUNCTION:
*
* INPUT   :
* OUTPUT  :
* RETURN  :
* NOTE    : The purpose is to compute the PTK.
*-----------------------------------------------------------------------------*/
static void WPA_CalcPTK(UINT8 *StaMacAddr)
{
    const UINT8 PMK_CONST[] = "Pairwise key expansion";
    UINT8 i, PTKTemp[76];
    UINT8 StaMacIsMax = FALSE, StaNonceIsMax = FALSE;
    UINT8 encryptType, PRFLen = 0;

    SIBCfg_SecurityAlg(&encryptType);
    if (encryptType == ENCRYP_MODE_TKIP)
    {
        PRFLen = 64;    //512 bits.
    }
    else if (encryptType == ENCRYP_MODE_AES)
    {
        PRFLen = 48;    //384 bits.
    }

    //Find out the Max MAC Address is Station or AP.
	for(i = 0; i < 6; i++)
	{
		if (StaMacAddr[i] > APMacAddr[i])
		{
			StaMacIsMax = TRUE;
			break;
		}		
		else if (StaMacAddr[i] < APMacAddr[i])
		{
			break;
		}
	}

	if (StaMacIsMax)
	{
		memcpy(PTKTemp, APMacAddr, 6);          //Min(AP,STA) = AP MAC.
		memcpy(PTKTemp + 6, StaMacAddr, 6);	    //Max(AP,STA) = STA MAC.
	}
	else
	{
		memcpy(PTKTemp, StaMacAddr, 6);         //Min(AP,STA) = STA MAC.
		memcpy(PTKTemp + 6, APMacAddr, 6);      //Max(AP,STA) = AP MAC.
	}

    //Find out the Max Nonce is SNonce or ANonce.
	for(i = 0; i < 32; i++)
	{
		if (SNonce[i] > ANonce[i])
		{
			StaNonceIsMax = TRUE;
			break;
		}		
		else if (SNonce[i] < ANonce[i])
		{
			break;
		}
	}

	if (StaNonceIsMax)
	{
		memcpy(PTKTemp + 12, ANonce, 32);       //Min(ANonce,SNonce) = ANonce.
		memcpy(PTKTemp + 44, SNonce, 32);       //Max(ANonce,SNonce) = SNonce.
	}
	else
	{
		memcpy(PTKTemp + 12, SNonce, 32);       //Min(ANonce,SNonce) = SNonce.
		memcpy(PTKTemp + 44, ANonce, 32);       //Max(ANonce,SNonce) = ANonce.
	}
    
    //Create Pairwise Transient Key (PTK).
    //PRF(PMK, 32, (UINT8 *)PMK_CONST, 22, PTKTemp, 76, PTK, 64);
    memset(PTK, 0x00, 80);
    PRF(PMK, 32, (UINT8 *)PMK_CONST, 22, PTKTemp, 76, PTK, PRFLen);

    if (WDEBUG_LEVEL(  _bDisp802_1x_) )
    {
        printf("\n[ WPA ] PTK dump:");
        _1xDumpHex(PTK, 64);
    }
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - WPA_CalcGTK
*------------------------------------------------------------------------------
* FUNCTION:
*
* INPUT   :
* OUTPUT  :
* RETURN  :
* NOTE    : The purpose is to compute the GTK.
*-----------------------------------------------------------------------------*/
UINT8 FillShareKey1 = 1;
void WPA_CalcGTK(void)
{
    const UINT8 GMK_CONST[] = "Group key expansion";
    UINT8 GMK[32], GTKTemp[38];
    UINT32 randNum;
    int i;
    UINT8 encryptType, PRFLen = 0;

    SIBCfg_SecurityAlg(&encryptType);
    if (encryptType == ENCRYP_MODE_TKIP)
    {
        PRFLen = 32;    //256 bits.
    }
    else if (encryptType == ENCRYP_MODE_AES)
    {
        PRFLen = 16;    //128 bits.
    }

    WPA_GenerateNonce(GNonce);
    memcpy(GTKTemp, APMacAddr, 6);
    memcpy(GTKTemp, GNonce, 32);

    //re-generate GMK
	for (i = 0; i < 32; i += sizeof(UINT32))
    {
            os_getRand((UINT8*)&randNum, sizeof(randNum));
	    memcpy(GMK + i, &randNum, sizeof(UINT32));
    }

    //Create Group Key (GTK).
    //PRF(GMK, 32, (UINT8 *)GMK_CONST, 19, GTKTemp, 38, GTK, 32);
    PRF(GMK, 32, (UINT8 *)GMK_CONST, 19, GTKTemp, 38, GTK, PRFLen);

    #ifdef WPA_BC_MC_USE_ASIC
    //Write the TKIP bc/mc key in to SRAM. (Position of Share key 3)
    if (encryptType == ENCRYP_MODE_TKIP)
    {
        WriteAsicSram(MODE1_SHARED_KEY_3, (ENABLE_WEP_IN_SRAM_ENTRY | SUPPORT_TKIP_IN_SRAM_ENTRY), PAGE_WEP_TABLE);
        for (i = 0; i < 4; i++)
        {
            WriteAsicSram(MODE1_SHARED_KEY_3 + i + 1, cpu_to_le32((*(UINT32 *)(GTK + i * 4))), PAGE_WEP_TABLE);
            WriteAsicSram(MODE1_SHARED_KEY_3 + i + 1, cpu_to_le32(*((UINT32 *)(GTK + 16 + i * 4))), PAGE_TKIPSC_TABLE);
        }
        WriteAsicSram(MODE1_SHARED_KEY_3, 1, PAGE_TKIPSC_TABLE);


        if (FillShareKey1)	//ardong_20041213. asic send all zero pkt workaround.
        {
            WriteAsicSram(MODE1_SHARED_KEY_1, (ENABLE_WEP_IN_SRAM_ENTRY | SUPPORT_TKIP_IN_SRAM_ENTRY), PAGE_WEP_TABLE);
            WriteAsicSram(MODE1_SHARED_KEY_2, (ENABLE_WEP_IN_SRAM_ENTRY | SUPPORT_TKIP_IN_SRAM_ENTRY), PAGE_WEP_TABLE);
            WriteAsicSram(MODE1_SHARED_KEY_4, (ENABLE_WEP_IN_SRAM_ENTRY | SUPPORT_TKIP_IN_SRAM_ENTRY), PAGE_WEP_TABLE);

            for (i = 0; i < 4; i++)
            {
                WriteAsicSram(MODE1_SHARED_KEY_1 + i + 1, cpu_to_le32(*((UINT32 *)(GTK + i * 4))), PAGE_WEP_TABLE);
                WriteAsicSram(MODE1_SHARED_KEY_1 + i + 1, cpu_to_le32(*((UINT32 *)(GTK + 16 + i * 4))), PAGE_TKIPSC_TABLE);
                WriteAsicSram(MODE1_SHARED_KEY_2 + i + 1, cpu_to_le32(*((UINT32 *)(GTK + i * 4))), PAGE_WEP_TABLE);
                WriteAsicSram(MODE1_SHARED_KEY_2 + i + 1, cpu_to_le32(*((UINT32 *)(GTK + 16 + i * 4))), PAGE_TKIPSC_TABLE);
                WriteAsicSram(MODE1_SHARED_KEY_4 + i + 1, cpu_to_le32(*((UINT32 *)(GTK + i * 4))), PAGE_WEP_TABLE);
                WriteAsicSram(MODE1_SHARED_KEY_4 + i + 1, cpu_to_le32(*((UINT32 *)(GTK + 16 + i * 4))), PAGE_TKIPSC_TABLE);
            }
            WriteAsicSram(MODE1_SHARED_KEY_1, 1, PAGE_TKIPSC_TABLE);
            WriteAsicSram(MODE1_SHARED_KEY_2, 1, PAGE_TKIPSC_TABLE);
            WriteAsicSram(MODE1_SHARED_KEY_4, 1, PAGE_TKIPSC_TABLE);
            FillShareKey1 = 0;
        }
        
    }
    else if (encryptType == ENCRYP_MODE_AES)
    {
        WriteAsicSram(MODE1_SHARED_KEY_3, (ENABLE_WEP_IN_SRAM_ENTRY | SUPPORT_AES_IN_SRAM_ENTRY), PAGE_WEP_TABLE);
        for (i = 0; i < 4; i++)
        {
            WriteAsicSram(MODE1_SHARED_KEY_3 + i + 1, cpu_to_le32(*((UINT32 *)(GTK + i * 4))), PAGE_WEP_TABLE);
        }
        WriteAsicSram(MODE1_SHARED_KEY_3, 1, PAGE_TKIPSC_TABLE);
    }
    #endif

    if (WDEBUG_LEVEL(  _bDisp802_1x_) )
    {
        printf("\n[ WPA ] GTK dump:");
        _1xDumpHex(GTK, 32);
    }
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - WPA_CheckKeyMIC
*------------------------------------------------------------------------------
* FUNCTION:
*
* INPUT   :
* OUTPUT  :
* RETURN  :
* NOTE    : The purpose is to check the WPA Key MIC.
*-----------------------------------------------------------------------------*/
static int WPA_CheckKeyMIC(UINT8 CheckType)
{
    UINT8 RecvMicKey[16], KeyMIC[40];   //To prevent overflow. (16->40)
    WPA_KEY_PACKET_ID WpaPkt;
    int TempBuffLen;
    
    WpaPkt = (WPA_KEY_PACKET_ID)(WPAKeyPkt);
    
    memcpy(RecvMicKey, WpaPkt->KeyMIC, 16);
    memset(WpaPkt->KeyMIC, 0x00, 16);
    memset(KeyMIC, 0x00, 16);
    TempBuffLen = htons(WpaPkt->PacketBodyLength) + 4;

    if (CheckType == CHECK_TYPE_MD5)
    {
        HMAC_MD5((UINT8 *)WpaPkt, TempBuffLen, PTK, 16, KeyMIC);
    }
    else if (CheckType == CHECK_TYPE_SHA1)
    {
        HMAC_SHA1((UINT8 *)WpaPkt, TempBuffLen, PTK, 16, KeyMIC);
    }

    if (WDEBUG_LEVEL(  _bDisp802_1x_) )
    {
        printf("\n[ WPA ] Key MIC:");
        _1xDumpHex(KeyMIC, 16);
    }
    
    if (memcmp(RecvMicKey, KeyMIC, 16) == 0)
    {
        return OK;
    }
    else
    {
        return ERROR;
    }
}


/*-----------------------------------------------------------------------------
* ROUTINE NAME - WPA_doRxPktMIC
*------------------------------------------------------------------------------
* FUNCTION: Add the function to DECRYPT TK2 (MIC)  -> mc/bc & uc packets of RX.
*
* INPUT   :
* OUTPUT  :
* RETURN  :
* NOTE    : The packet data of Check MIC is from the packet's SNAP header.
*-----------------------------------------------------------------------------*/
int WPA_doRxPktMIC(UINT8 *pDa, UINT8 *pSa, UINT8 *PktData, UINT16 PktLen)
{
    UINT8 RecvMIC[8], ComputeMIC[8], Header[12];
    PORT_CONTROL *PortCtrl;

//    if (_8202DbgFlag & _bDisp802_1x_)
//    {
//        printf("\n[ WPA ] WPA_doRxPktMIC():");
//        _1xDumpHex(pDa, 6);
//        _1xDumpHex(pSa, 6);
//        _1xDumpHex(PktData, PktLen);
//    }

    if ((PortCtrl = PortDbInquiry(pSa)) == NULL)
    {
  	    return ERROR;
    }

    //If the field of the TxRxMICKey is NULL, then pass.    	
    if (*(UINT32 *)PortCtrl->TxRxMICKey == 0L)
    {
        return ERROR;
    }

    PktLen -= WPA_MIC_LEN;

    memcpy(RecvMIC, PktData + PktLen, WPA_MIC_LEN);

    memcpy(Header, pDa, 6);
    memcpy(Header + 6, pSa, 6);
    Mic_Simple(Header, PktData, PktLen, PortCtrl->TxRxMICKey + 8, ComputeMIC);

//    if (_8202DbgFlag & _bDisp802_1x_)
//    {
//        printf("\n[ WPA ] WPA_doRxPktMIC(): MIC value=");
//        _1xDumpHex(ComputeMIC, 8);
//    }

    if (memcmp(RecvMIC, ComputeMIC, 8) == 0)
    {
        return OK;
	}
    else
    {
        return ERROR;
    }
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - WPA_doTxPktMIC
*------------------------------------------------------------------------------
* FUNCTION: Add the function to ENCRYPT TK2 (MIC)  -> unicast packets before TX.
*
* INPUT   :
* OUTPUT  :
* RETURN  :
* NOTE    : 
*-----------------------------------------------------------------------------*/

//Add the function to ENCRYPT TK2 (MIC)  -> unicast packets before TX.
UINT16 WPA_doTxPktMIC(UINT8 *pDa, UINT8 *pSa, UINT8 *PktData, UINT16 PktLen)
{
	UINT8 TxMICKey[8], ComputeMIC[8], Header[12];
    PORT_CONTROL *PortCtrl;

//    if (_8202DbgFlag & _bDisp802_1x_)
//    {
//        printf("\n[ WPA ] WPA_doTxPktMIC:");
//        _1xDumpHex(pDa, 6);
//        _1xDumpHex(pSa, 6);
//        printf("\n[ WPA ] PktLen : %d\n", PktLen);
//    }

    if (((PortCtrl = PortDbInquiry(pDa)) == NULL) && (!IS_GROUP_MAC(pDa)))
    {
        return 0;   //Failed.
    }

    if (IS_GROUP_MAC(pDa))
    {
        memcpy(TxMICKey, GroupTxMICKey, 8);
    }
    else
    {
        if ((*((UINT32 *)(PortCtrl->TxRxMICKey)) == 0L))
        {
            return 0;
        }

        memcpy(TxMICKey, PortCtrl->TxRxMICKey, 8);
    }
    
    memcpy(Header, pDa, 6);
    memcpy(Header + 6, pSa, 6);
    
    Mic_SNAP_Simple(Header, PktData, PktLen, TxMICKey, ComputeMIC);

    return (WPA_MIC_LEN);
}


/*-----------------------------------------------------------------------------
* ROUTINE NAME - WPA_TransferPSK
*------------------------------------------------------------------------------
* FUNCTION: Transfering the less than 32 bytes PSK to 32 bytes data.
*
* INPUT   :
* OUTPUT  :
* RETURN  :
* NOTE    : 
*-----------------------------------------------------------------------------*/

void WPA_TransferPSK(char *Password, UINT8 *Ssid, int SsidLength, int Iterations, int Count, UINT8 *Output)
{
	UINT8 Digest[36], Digest1[SHA_DIGEST_LENGTH];
	int i, j;

    memset(Digest, 0x00, 36);
    memset(Digest1, 0x00, SHA_DIGEST_LENGTH);

	//U1 = PRF(P, S || int(i)).
	memcpy(Digest, Ssid, SsidLength);
	Digest[SsidLength] = (UINT8)((Count>>24) & 0xff);
	Digest[SsidLength + 1] = (UINT8)((Count>>16) & 0xff);
	Digest[SsidLength + 2] = (UINT8)((Count>>8) & 0xff);
	Digest[SsidLength + 3] = (UINT8)(Count & 0xff);
	
	//HMAC(EVP_sha1(), (UINT8 *)Password, (int)strlen(Password), Digest, SsidLength + 4, Digest1, &DigestLen1);
    HMAC_SHA1(Digest, SsidLength + 4, (UINT8 *)Password, (int)strlen(Password), Digest1);

	//Output = U1.
	memcpy(Output, Digest1, SHA_DIGEST_LENGTH);

	for (i = 1; i < Iterations; i++)
	{
		//Un = PRF(P, Un-1).
		HMAC_SHA1(Digest1, SHA_DIGEST_LENGTH, (UINT8 *)Password, (int)strlen(Password), Digest);

		memcpy(Digest1, Digest, SHA_DIGEST_LENGTH);

		//Output = output xor Un.
		for (j = 0; j < SHA_DIGEST_LENGTH; j++)
		{
			Output[j] ^= Digest[j];
		}
	}
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - PasswordHash
*------------------------------------------------------------------------------
* FUNCTION: Calling F to transfer the less than 32 bytes PSK to 32 bytes data.
* INPUT   : password - ascii string up to 63 characters in length.
*           ssid - octet string up to 32 octets.
*           ssidlength - length of ssid in octets.

* OUTPUT  : output must be 40 octets in length and outputs 256 bits of key
* RETURN  : None
* NOTE:
*-----------------------------------------------------------------------------*/
int WPA_PasswordHash(char *Password, UINT8 *Ssid, int SsidLength, UINT8 *Output)
{
	if ((strlen(Password) > 63) || (SsidLength > 32))
        return 0;

	WPA_TransferPSK(Password, Ssid, SsidLength, 4096, 1, Output);
	WPA_TransferPSK(Password, Ssid, SsidLength, 4096, 2, &Output[SHA_DIGEST_LENGTH]);
	return 1;
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - WPA_DropAllStation
*------------------------------------------------------------------------------
* FUNCTION: Drop all station records in AP.
* INPUT   : None
* OUTPUT  : None
* RETURN  : None
* NOTE:
*-----------------------------------------------------------------------------*/
void WPA_DropAllStation_(PORT_CONTROL *port)
{
    SIBCfg_SendDisAssoc(port->hashEntry.mac);    //Also delete record in ClientDB table.
    DelPortControl(port->hashEntry.mac);         //Delete record in PortControl table.
}
void WPA_DropAllStation(void)
{
    iterateAllPortControl((pHANDLE) WPA_DropAllStation_);
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - WPA_ProcessFailureMIC
*------------------------------------------------------------------------------
* FUNCTION: Processing the MIC value error.
* INPUT   : None
* OUTPUT  : None
* RETURN  : None
* NOTE:
*-----------------------------------------------------------------------------*/
void WPA_ProcessFailureMIC(void)
{
    UINT8 Value8;
printCurSecSinceBoot();

    if (WPA_MIC_FailureTimer1 == 0)
    {
        PRINTLOCM("WPA_ProcessFailureMIC: mic fail detected...");
        //Great than 60 seconds have passed since a previous MIC failure. So, continue.
        WPA_MIC_FailureTimer1 = 60;
    }
    else
    {
        PRINTLOCM("WPA_ProcessFailureMIC: mic countermeasure executed...");
        //Less than 60 seconds have passed since a previous MIC failure.
        // 1. Transition every station to State2 of 802.11 state diagrams.
        WPA_DropAllStation();

        // 2 . Disallow associations for the duration of 60 seconds.
        //   At the end of the 60 seconds, reset timer and new associations resume.
        WPA_MIC_FailureTimer1 = 0;
        WPA_MIC_FailureTimer2 = 60;
        Value8 = 1;
        SIBCfg_SetWlanEnDis(&Value8);
    }

    if (WDEBUG_LEVEL( _bDisp802_1x_))
    {
        printf("\n[ WPA ] WPA_ProcessFailureMIC() ===> Start Timer.");
    }
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - WPA_ProcessEapolKeyRequestPkt
*------------------------------------------------------------------------------
* FUNCTION: Processing the Eapol-Key packet with Request and Error bits set.
* INPUT   : None
* OUTPUT  : None
* RETURN  : None
* NOTE:
*-----------------------------------------------------------------------------*/
void WPA_ProcessEapolKeyRequestPkt(void)
{
    UINT8 Value8;
printCurSecSinceBoot();
    if (WPA_MIC_FailureTimer3 == 0)
    {
        PRINTLOCM("WPA_ProcessEapolKeyRequestPkt: mic fail detected...");
        //Since a previous Eapol-Key packet with Request and Error bits set,
        //great than 60 seconds have passed. So, continue.
        WPA_MIC_FailureTimer3 = 60;
    }
    else
    {
        PRINTLOCM("WPA_ProcessEapolKeyRequestPkt: mic countermeasure executed...");
        //Since a previous Eapol-Key packet with Request and Error bits set,
        //less than 60 seconds have passed.
        // 1. Transition every station to State2 of 802.11 state diagrams.
        WPA_DropAllStation();

        // 2. Disallow associations for the duration of 60 seconds.
        //   At the end of the 60 seconds, reset timer and new associations resume.
        WPA_MIC_FailureTimer3 = 0;
        WPA_MIC_FailureTimer4 = 60;
        Value8 = 1;
        SIBCfg_SetWlanEnDis(&Value8);
    }
    if (WDEBUG_LEVEL( _bDisp802_1x_))
    {
        printf("\n[ WPA ] WPA_ProcessEapolKeyRequestPkt() ===> Start Timer.");
    }
}




