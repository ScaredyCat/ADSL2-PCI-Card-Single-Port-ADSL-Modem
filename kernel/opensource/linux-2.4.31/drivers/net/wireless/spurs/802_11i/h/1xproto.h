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
/*  File Name: 1xproto.h                                                     */ 
/*  File Description: This file contains the prototype                       */
/*                    declarations of all the functions.                     */
/*                                                                           */ 
/*  History:                                                                 */ 
/*      Date        Author      Version     Description                      */ 
/*      ----------  ---------   -------     --------------                   */
/*      02/21/2002  Joy Lin     V1.0        Initial Version                  */ 
/*                                                                           */ 
/*****************************************************************************/
 

#ifndef __1XPROTO_H__
#define __1XPROTO_H__

#include "ifx_wdrv_types.h"
#include "wpa_cfg.h"
#include "ifxadm_wdrv_init.h"
#include "1xdefs.h"
#include "1xtdfs.h"


/* 1xmain.c */
void _1xTask (void); 
int _1xStart (void);

int RadCliInit(void);
void WPA_GenerateNonce(UINT8 *Nonce);
void _1xTimeoutHandler (void);
int CheckTimeOutEntry(void);
int txReq (PORT_CONTROL *portCtrl, EAPOL_PACKET_ID p);
int sendRespToServer (PORT_CONTROL *portCtrl, UINT8 *pEapPkt);
int txCannedSuccess (PORT_CONTROL *portCtrl);
int txKey (PORT_CONTROL *portCtrl, int keyType);
int txCannedFail (PORT_CONTROL *portCtrl);
void BackendFSM(PORT_CONTROL *portCtrl, UINT8 *pEapolPkt);
int txReqId (PORT_CONTROL *portCtrl);
void Notify1xCfg_ReAuthEnabled(UINT8 newValue);
void Notify1xCfg_ReAuthPeriod(UINT16 newValue);
void UpdateRadiusServerInfo(RADIUS_CONF *nvRadConf);
void TransAsciiToHex(UINT8 *dest, UINT8 *src, int len);

void _1xTimeout(int eventMask);
void AuthenticatorFSM(PORT_CONTROL *portCtrl, UINT8 *pEapolPkt);
void ether_input_eapol(struct ether_header *eh, WLAN_BUF_INFO *m);

int txEapol (UINT8 *pDestAddr, UINT8 *pEapolPkt, int len);
int GetFreeIndividualKey(void);
void WPA_CalcGTK(void);
void WPA_ProcessFailureMIC(void);
void WPA_DropAllStation(void);
void WPA_ProcessEapolKeyRequestPkt(void);

void eapolintr(void);
void do_rc4(UINT8 *key_data, int key_len, UINT8 *data, int len);
void HMAC_MD5(UINT8 *DataStr, int DataLen, UINT8 *KeyStr, int KeyLen, UINT8 *Digest);

PORT_CONTROL *PortDbInquiry(UINT8 *sta);
PORT_CONTROL *AddPortControl(UINT8 *sta);

void DelPortControl(UINT8 *sta);

void PortControlInit(void);

#ifdef IFX_WLAN_DEBUG
void _1xDumpHex(UINT8 *buf, int len);
#else //IFX_WLAN_DEBUG
#define _1xDumpHex(_buf,_len)
#endif //IFX_WLAN_DEBUG


void _1xPeriodicTask(void);   //one second
void _1xTasklet(unsigned long data);
int _1xEnqueue_EapKey(char *pDa);
int _1xEnqueue_EapReqID(char *pDa);
int WPA_PasswordHash(char *Password, UINT8 *Ssid, int SsidLength, UINT8 *Output);
void AdjustRadiusServer(int totalNumOfRad, int hasRspRad);
void _1xStop(void);
void drainEapolintrQ(void);

void iterateAllPortControl(pHANDLE func);     //this func must have/need an pointer to PORT_CONTROL as only argument

void dot1x_xmit_toWlan(WLAN_BUF_INFO* pBufInfo);

UINT16 WPA_doTxPktMIC(UINT8 *pDa, UINT8 *pSa, UINT8 *PktData, UINT16 PktLen);
int WPA_doRxPktMIC(UINT8 *pDa, UINT8 *pSa, UINT8 *PktData, UINT16 PktLen);
void clearPortControl(PORT_CONTROL *port);
void PortControlReInit(void);
void removeRadiusSocketId(void);


#endif /* __1XPROTO_H__ */

