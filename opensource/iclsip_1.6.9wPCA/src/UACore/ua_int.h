/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * ua_int.h
 *
 * $Id: ua_int.h,v 1.36 2005/01/27 06:20:12 tyhuang Exp $
 */
#ifndef UA_INT_H
#define UA_INT_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <sip/sip_cm.h>
#include "ua_mgr.h"
#include "ua_cfg.h"
#include "ua_dlg.h"
#include "ua_content.h"
#include "ua_cm.h"
#include <sipTx/sipTx.h>

/*Get Transaction dialog*/
/*if TRUE: search INVITE transaction, FALSE: search non-INVITE transaction*/
TxStruct uaDlgGetInviteTx(IN UaDlg,IN BOOL); /*search from first tx*/
TxStruct uaDlgGetAliveInviteTx(IN UaDlg _dlg,IN BOOL binv);
TxStruct uaDlgGetMethodTx(IN UaDlg, IN SipMethodType);/*search from first tx*/
void	 uaDlgPrintAllTx(IN UaDlg _dlg);

/*find a TxStructure */
TxStruct uaDlgGetSpecTx(IN UaDlg, IN TXTYPE,IN SipMethodType);

/*when get a request message*/
RCODE uaProcessReqTx(IN TxStruct);

/*when get a response message*/
RCODE uaProcessRspTx(IN TxStruct);

/*when get a cancel event from tx */
RCODE uaProcessCancel(TxStruct tx);

/*when receive a ACK, enter CONNECTED state*/
RCODE uaProcessAckTx(IN TxStruct);

/*create a UserPfofile from setting */
UserProfile uaCreateUserProfile(IN UaCfg);
RCODE uaUserProfileFree(IN UserProfile);

/*create a UserProfile for SIMPLE API from setting */
UserProfile uaCreateUserProfileForSIMPLE(IN UaCfg);

/*get match mgr, if didn't match, return NULL*/
UaMgr uaMgrMatchTx(IN TxStruct);

/*Tx will find a match Dialog, or create a new Dialog*/
UaDlg uaDlgMatchTx(IN UaMgr,IN TxStruct);


/*delete transaction from TxList*/
RCODE uaDelTxFromTxList(IN UaDlg,IN DxLst,IN TxStruct);

/*return reasone phase*/
const char* uaStatusCodeToString(IN UAStatusCode);
/*transfer sip method into string*/
const char* uaSipMethodToString(IN SipMethodType);

typedef enum _UaTimerEvent
{
	REGISTER_EXPIRE,
	INVITE_EXPIRE,
	SUBSCRIBE_EXPIRE,
	OK200_EXPIRE,
	CANCEL_EXPIRE,
	SESSION_TIMER
} UaTimerEvent;

struct _UaTimer
{
	UaDlg dlg;
	UaTimerEvent event;
	unsigned long expire;
};

typedef struct _UaTimer	 TmpTimer;
typedef struct _UaTimer* UaTimer;

int uaAddTimer(UaDlg, UaTimerEvent, unsigned long);

void uaDelTimer(int TimerHandle);


#ifdef  __cplusplus
}
#endif

#endif /* UA_INT_H */
