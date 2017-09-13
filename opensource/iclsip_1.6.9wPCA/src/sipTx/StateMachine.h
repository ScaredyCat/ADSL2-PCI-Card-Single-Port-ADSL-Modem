/* 
 * Copyright (C) 2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * StateMachine.h
 *
 * $Id: StateMachine.h,v 1.8 2004/12/16 09:25:55 ljchuang Exp $
 */
#ifndef __STATEMACHINE__
#define __STATEMACHINE__

#include <sip/cclsip.h>
#include "sipTx.h"
#include "TxStruct.h"

#ifdef	__cplusplus
extern "C" {
#endif

void Server_Invite_SM(TxStruct TxData, SipTxEvtType EvtType, SipReq msg);
void Server_Non_Invite_SM(TxStruct TxData, SipTxEvtType EvtType, SipReq msg);
void Client_Invite_SM(TxStruct TxData, SipTxEvtType EvtType, SipRsp msg);
void Client_Non_Invite_SM(TxStruct TxData, SipTxEvtType EvtType, SipRsp msg);
void Client_2XX_SM(TxStruct TxData, SipTxEvtType EvtType, SipReq msg);
void Client_Ack_SM(TxStruct TxData, SipTxEvtType EvtType, SipRsp msg);

#ifdef __cplusplus
}
#endif

#endif /* ifndef __STATEMACHINE__ */

