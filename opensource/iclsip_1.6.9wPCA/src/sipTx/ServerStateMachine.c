/* 
 * Copyright (C) 2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * ServerStateMachine.c
 *
 * $Id: ServerStateMachine.c,v 1.49 2006/01/12 02:08:43 tyhuang Exp $
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sip/cclsip.h>
#include "net.h"
#include "sipTx.h"
#include "TxStruct.h"
#include "Transport.h"
#include <common/cm_trace.h>

extern BOOL g_bMatchACK;

SipReq NewAckFromTx(const TxStruct TxData);

/* extern SipTxEvtCB g_txEvtCB; */
void g_txEvtCB(TxStruct TxData, SipTxEvtType event);

int sipTxAddTimer(TxStruct TxData, SipTxEvtType Timer);
void sipTxDelTimer(unsigned short TimerHandle);


void ResponseAddStatusLine(SipReq request,
			   SipRsp response, 
			   const char* strStatusCode, 
			   const char* strReason)
{
	SipRspStatusLine *line=(SipRspStatusLine*)malloc(sizeof(SipRspStatusLine));
	SipReqLine	*reqline;

	reqline=sipReqGetReqLine(request);
	line->ptrVersion=strDup("SIP/2.0");
	line->ptrStatusCode=strDup(strStatusCode);

	line->ptrReason=strDup(strReason);

	sipRspSetStatusLine(response,line);
	free(line->ptrReason);
	free(line->ptrStatusCode);
	free(line->ptrVersion);
	free(line);
}

void ResponseAddHeader(SipReq request,SipRsp response)
{
	SipFrom			*from = NULL;
	SipTo			*to = NULL;
	char			*ID = NULL;
	SipCSeq			*seq = NULL;
	SipTimestamp	*tstamp = NULL;
	SipTimestamp	*mytstamp = NULL;
	
	from = (SipFrom*)sipReqGetHdr(request,From);
	if (from)
		sipRspAddHdr(response,From,from);

	to = (SipTo*)sipReqGetHdr(request,To);
	if (to)
		sipRspAddHdr(response,To,to);

	ID = (char*)sipReqGetHdr(request,Call_ID);
	if (ID)
		sipRspAddHdr(response,Call_ID,ID);

	seq = (SipCSeq*)sipReqGetHdr(request,CSeq);
	if (seq)
		sipRspAddHdr(response,CSeq,seq);

	/*
	type = (SipContType*)sipReqGetHdr(request,Content_Type);
	if (type)
		sipRspAddHdr(response,Content_Type,type);
	 */

	tstamp = (SipTimestamp*)sipReqGetHdr(request,Timestamp);
	if (tstamp)
	{
		mytstamp = (SipTimestamp*)calloc(1,sizeof(SipTimestamp));
		mytstamp->time = strDup( tstamp->time );
		mytstamp->delay = strDup ("0.1");
		sipRspAddHdr(response,Timestamp,mytstamp);
	}

}


void ResponseAddViaHeader(SipReq request,SipRsp response)
{
	SipVia *via;

	via=(SipVia*)sipReqGetHdr(request,Via);
	if(via!= NULL)

	sipRspAddHdr(response,Via,via);
}


SipRsp NewRspFromReq(const SipReq request, const unsigned short statuscode)
{
	SipRsp response;
	response = sipRspNew();
	
	switch (statuscode)
	{
	case 100:
		ResponseAddStatusLine(request,response,"100","Trying");
		break;

	case 200:
		ResponseAddStatusLine(request,response,"200","OK");
		break;

	case 481:
		ResponseAddStatusLine(request,response,"481","Transaction Does Not exist");
		break;

	default:
		ResponseAddStatusLine(request,response,"500","Server Internal Error");
		break;
	}

	ResponseAddViaHeader(request,response);
	ResponseAddHeader(request,response);
	return response;
}

int GetStatusCode(SipRsp msg)
{
	SipRspStatusLine *pStatus;
	pStatus=sipRspGetStatusLine(msg);
	return(atoi(pStatus->ptrStatusCode));
}


void Server_Invite_SM(TxStruct TxData, SipTxEvtType EvtType, SipReq msg)
{
	int status;
	
#ifdef CCLSIP_SIPTX_ENABLE_TRACE
	int n;
#endif

	/* Lock mutex of the Tx when enter transaction statemachine */
	txMutexLock(TxData->mutex);

	TXTRACE1("[Server_Invite_SM] Entering %s Transaction\n", sipMethodTypeToName(TxData->method) );
	TXTRACE1("[Server_Invite_SM] Transaction internal id:(%d)\n", TxData->InternalNum );
	TXTRACE1("[Server_Invite_SM] TxState = %s\n", sipTxState2Phase(TxData->state) );

	if ( TX_TCP_CONNECTED == EvtType ) {
		SendResponse(TxData);
		goto TERMINATED;
	}

	if ( TX_TRANSPORT_ERROR == EvtType ) {
		g_txEvtCB(TxData, TX_TRANSPORT_ERROR);
		TxData->state = TX_TERMINATED;
		TXTRACE0("[Server_Invite_SM] Tx -> TERMINATED\n");
		g_txEvtCB(TxData, TX_TERMINATED_EVENT);
		goto TERMINATED;
	}

	switch (TxData->state) {
	case TX_INIT:
		TXTRACE0("[Server_Invite_SM] Tx -> INIT\n");
	
		TxData->LatestRsp = NewRspFromReq(TxData->OriginalReq, 100);
		
		if (RC_ERROR == SendResponse(TxData)) {
			g_txEvtCB(TxData, TX_TRANSPORT_ERROR);
			TxData->state = TX_TERMINATED;
			TXTRACE0("[Server_Invite_SM] Tx -> TERMINATED\n");
			g_txEvtCB(TxData, TX_TERMINATED_EVENT);
			goto TERMINATED;
		} else {
			g_txEvtCB(TxData, TX_REQ_RECEIVED);
			TxData->state = TX_PROCEEDING;
			TXTRACE0("[Server_Invite_SM] Tx -> PROCEEDING\n");
		}
		break;
	
	case TX_PROCEEDING:
		if (msg != NULL)
		{
			sipReqFree(msg);
			msg = NULL;
		}

		if (RC_ERROR == SendResponse(TxData)) {
			g_txEvtCB(TxData, TX_TRANSPORT_ERROR);
			TxData->state = TX_TERMINATED;
			TXTRACE0("[Server_Invite_SM] Tx -> TERMINATED\n");
			g_txEvtCB(TxData, TX_TERMINATED_EVENT);
			goto TERMINATED;
		} else {
			status = GetStatusCode(TxData->LatestRsp);
			if (status >= 100 && status < 200) {
				TxData->state = TX_PROCEEDING;
			}
			if (status >= 200 && status < 300) {
				if (g_bMatchACK) {
					g_txEvtCB(TxData, TX_2XX_FOR_INVITE);

					/* Arm TimerG */
					TxData->TimerG = T1;
					TxData->TimerHandle1 = sipTxAddTimer(TxData, TX_TIMER_G);
					TxData->TimerH += TxData->TimerG;
					
					TxData->state = TX_COMPLETED;
					TXTRACE0("[Server_Invite_SM] Tx -> COMPLETED\n");
				} else {
					/*
					TxData->state = TX_TERMINATED;
					TXTRACE0("[Server_Invite_SM] Tx -> TERMINATED\n");
					g_txEvtCB(TxData, TX_TERMINATED_EVENT);
					*/
					/*
					 * modified by tyhuang
					 * sipTx2xxClientNew(TxData);
					 */
					sipTx2xxClientNew(TxData);/* Automatically transfer 2xx retransmit for TU */

					TxData->state = TX_TERMINATED;
					TXTRACE0("[Server_Invite_SM] Tx -> TERMINATED\n");
					g_txEvtCB(TxData, TX_TERMINATED_EVENT);

					goto TERMINATED;
				}
			}
			if (status >= 300 && status < 700) {
				/* Arm TimerG */
				TxData->TimerG = T1;
				TxData->TimerHandle1 = sipTxAddTimer(TxData, TX_TIMER_G);
				TxData->TimerH += TxData->TimerG;
				
				/* Arm TimerH
				TxData->TimerH = 64 * T1;
				TxData->TimerHandle2 = sipTxAddTimer(TxData, TX_TIMER_H); */
				
				TxData->state = TX_COMPLETED;
				TXTRACE0("[Server_Invite_SM] Tx -> COMPLETED\n");
			}		
		}
		break;
	
	case TX_COMPLETED:
		if (TX_REQ_RECEIVED == EvtType) {
			TXTRACE0("[Server_Invite_SM] REQ Received\n");
			
			if (msg != NULL)
			{
				sipReqFree(msg);
				msg = NULL;
			}

			if (RC_ERROR == SendResponse(TxData)) {
				g_txEvtCB(TxData, TX_TRANSPORT_ERROR);
				TxData->state = TX_TERMINATED;
				TXTRACE0("[Server_Invite_SM] Tx -> TERMINATED\n");
				g_txEvtCB(TxData, TX_TERMINATED_EVENT);
				goto TERMINATED;
			}
		}
		if (TX_TIMER_G == EvtType) {
			TXTRACE0("[Server_Invite_SM] Process Timer G\n");
			
#ifdef CCLSIP_SIPTX_ENABLE_TRACE
			if (TxData->TimerH > T2 )
			{
				n = (int)floor( TxData->TimerH / T2 ) + 3;
			}
			else
			{
				frexp( (TxData->TimerG / T1), &n );
			}
			TXTRACE1("[Server_Invite_SM] %dth re-transmit\n", n);
#endif
			
			g_txEvtCB(TxData, TX_TIMER_G);
			
			if (RC_ERROR == SendResponse(TxData)) {
				/* no more arm TimerG */
				TxData->TimerHandle1 = -1;
				g_txEvtCB(TxData, TX_TRANSPORT_ERROR);
				TxData->state = TX_TERMINATED;
				TXTRACE0("[Server_Invite_SM] Tx -> TERMINATED\n");
				g_txEvtCB(TxData, TX_TERMINATED_EVENT);
				goto TERMINATED;
			} else {
				TxData->TimerG = ( (TxData->TimerG*2) > T2) ? T2 : (TxData->TimerG * 2);
				TxData->TimerH += TxData->TimerG;
				if ( TxData->TimerH > (64*T1) ) {
					/* no more arm TimerG */
					TxData->TimerHandle1 = -1;
					/* Arm TimerH */
					TxData->TimerH = T1;
					TxData->TimerHandle2 = sipTxAddTimer(TxData, TX_TIMER_H);
				} else {
					TxData->TimerHandle1 = sipTxAddTimer(TxData, TX_TIMER_G);
				}
			}
		}
		if (TX_TIMER_H == EvtType) {
			TXTRACE0("[Server_Invite_SM] Process Timer H\n");
			/* no more arm TimerH */
			TxData->TimerHandle2 = -1;
			g_txEvtCB(TxData, TX_TIMER_H);
			TxData->state = TX_TERMINATED;
			TXTRACE0("[Server_Invite_SM] Tx -> TERMINATED\n");
			g_txEvtCB(TxData, TX_TERMINATED_EVENT);
			goto TERMINATED;
		}
		if (TX_ACK_RECEIVED == EvtType) {
			TXTRACE0("[Server_Invite_SM] ACK Received\n");
			
			if (TxData->Ack != NULL)
				sipReqFree(TxData->Ack);
			
			TxData->Ack = msg;
			
			g_txEvtCB(TxData, TX_ACK_RECEIVED);

			/* Disarm TimerG */
			if (-1 != TxData->TimerHandle1) {
				sipTxDelTimer(TxData->TimerHandle1);
				TxData->TimerHandle1 = -1;
			}
			
			/* Disarm TimerH */
			if (-1 != TxData->TimerHandle2) {
				sipTxDelTimer(TxData->TimerHandle2);
				TxData->TimerHandle2 = -1;
			}
			
			/* Arm TimerI */
			if ( TxData->txptype == UDP ) {
				TxData->TimerI = T4;
				TxData->TimerHandle3 = sipTxAddTimer(TxData, TX_TIMER_I);
				TxData->state = TX_CONFIRMED;
				TXTRACE0("[Server_Invite_SM] Tx -> CONFIRMED\n");
			} else {
				TxData->state = TX_TERMINATED;
				TXTRACE0("[Server_Invite_SM] Tx -> TERMINATED\n");
				g_txEvtCB(TxData, TX_TERMINATED_EVENT);
				goto TERMINATED;
			}
		}
		break;
	
	case TX_CONFIRMED:
		if (msg != NULL)
		{
			sipReqFree(msg);
			msg = NULL;
		}

		if (TX_TIMER_I == EvtType) {
			TXTRACE0("[Server_Invite_SM] Process Timer I\n");
			/* no more arm TimerI */
			TxData->TimerHandle3 = -1;
			/* g_txEvtCB(TxData, TX_TIMER_I); */
			TxData->state = TX_TERMINATED;
			TXTRACE0("[Server_Invite_SM] Tx -> TERMINATED\n");
			g_txEvtCB(TxData, TX_TERMINATED_EVENT);
			goto TERMINATED;
		}
		

		break;
	
	default:
		if (msg != NULL)
		{
			sipReqFree(msg);
			msg = NULL;
		}
	}
	

TERMINATED:

	/* Unlock mutex of the Tx when leave the transaction statemachine */
	txMutexUnlock(TxData->mutex);
	TXTRACE0("[Server_Invite_SM] Leaving\n");
}


void Server_Non_Invite_SM(TxStruct TxData, SipTxEvtType EvtType, SipReq msg)
{
	int status;

	/* Lock mutex of the Tx when enter transaction statemachine */
	txMutexLock(TxData->mutex);

	TXTRACE1("[Server_Non_Invite_SM] Entering %s Transaction\n", sipMethodTypeToName(TxData->method) );
	TXTRACE1("[Server_Non_Invite_SM] Transaction internal id:(%d)\n", TxData->InternalNum );
	TXTRACE1("[Server_Non_Invite_SM] TxState = %s\n", sipTxState2Phase(TxData->state) );

	if ( TX_TCP_CONNECTED == EvtType ) {
		SendResponse(TxData);
		goto TERMINATED;
	}

	if ( TX_TRANSPORT_ERROR == EvtType ) {
		g_txEvtCB(TxData, TX_TRANSPORT_ERROR);
		TxData->state = TX_TERMINATED;
		TXTRACE0("[Server_Non_Invite_SM] Tx -> TERMINATED\n");
		g_txEvtCB(TxData, TX_TERMINATED_EVENT);
		goto TERMINATED;
	}
	
	switch (TxData->state) {
	case TX_INIT:
		TXTRACE0("[Server_Non_Invite_SM] Tx -> INIT\n");
		g_txEvtCB(TxData, TX_REQ_RECEIVED);
		TxData->state = TX_TRYING;
		TXTRACE0("[Server_Non_Invite_SM] Tx -> TRYING\n");
		break;

	case TX_TRYING:
		if (msg != NULL)
		{
			sipReqFree(msg);
			msg = NULL;
		}
		if ( NULL == sipTxGetLatestRsp(TxData) )
		{
			TXWARN0("[Server_Non_Invite_SM] no latest response from TU!\n");
			goto TERMINATED;
		}

		if (RC_ERROR == SendResponse(TxData)) {
			g_txEvtCB(TxData, TX_TRANSPORT_ERROR);
			TxData->state = TX_TERMINATED;
			TXTRACE0("[Server_Non_Invite_SM] Tx -> TERMINATED\n");
			g_txEvtCB(TxData, TX_TERMINATED_EVENT);
			goto TERMINATED;
		} else {
			status = GetStatusCode(TxData->LatestRsp);
			if (status >= 100 && status < 200) {
				TxData->state = TX_PROCEEDING;
				TXTRACE0("[Server_Non_Invite_SM] Tx -> PROCEEDING\n");
			}
			if (status >= 200 && status < 700) {
				/* Arm TimerJ */
				if ( TxData->txptype == UDP ) {				
					TxData->TimerJ = 64 * T1;
					TxData->TimerHandle1 = sipTxAddTimer(TxData, TX_TIMER_J);
					TxData->state = TX_COMPLETED;
					TXTRACE0("[Server_Non_Invite_SM] Tx -> COMPLETED\n");
				} else {
					TxData->state = TX_TERMINATED;
					TXTRACE0("[Server_Non_Invite_SM] Tx -> TERMINATED\n");
					g_txEvtCB(TxData, TX_TERMINATED_EVENT);
					goto TERMINATED;
				}
			}		
		}
		break;
		
	case TX_PROCEEDING:
		if (msg != NULL)
		{
			sipReqFree(msg);
			msg = NULL;
		}
		
		if (RC_ERROR == SendResponse(TxData)) {
			g_txEvtCB(TxData, TX_TRANSPORT_ERROR);
			TxData->state = TX_TERMINATED;
			TXTRACE0("[Server_Non_Invite_SM] Tx -> TERMINATED\n");
			g_txEvtCB(TxData, TX_TERMINATED_EVENT);
			goto TERMINATED;
		} else {
			status = GetStatusCode(TxData->LatestRsp);
			if (status >= 100 && status < 200) {
				TxData->state = TX_PROCEEDING;
			}
			if (status >= 200 && status < 700) {
				/* Arm TimerJ */
				if ( TxData->txptype == UDP ) {				
					TxData->TimerJ = 64 * T1;
					TxData->TimerHandle1 = sipTxAddTimer(TxData, TX_TIMER_J);
					TxData->state = TX_COMPLETED;
					TXTRACE0("[Server_Non_Invite_SM] Tx -> COMPLETED\n");
				} else {
					TxData->state = TX_TERMINATED;
					TXTRACE0("[Server_Non_Invite_SM] Tx -> TERMINATED\n");
					g_txEvtCB(TxData, TX_TERMINATED_EVENT);
					goto TERMINATED;
				}
			}		
		}
		break;
	
	case TX_COMPLETED:
		if (msg != NULL)
		{
			sipReqFree(msg);
			msg = NULL;
		}

		if (TX_REQ_RECEIVED == EvtType) {
			TXTRACE0("[Server_Non_Invite_SM] REQ Received\n");
			if (RC_ERROR == SendResponse(TxData)) {
				g_txEvtCB(TxData, TX_TRANSPORT_ERROR);
				TxData->state = TX_TERMINATED;
				TXTRACE0("[Server_Non_Invite_SM] Tx -> TERMINATED\n");
				g_txEvtCB(TxData, TX_TERMINATED_EVENT);
				goto TERMINATED;
			}
			TxData->state = TX_COMPLETED;
		}
		if (TX_TIMER_J == EvtType) {
			TXTRACE0("[Server_Non_Invite_SM] Process Timer J\n");
			/* no more arm TimerJ */
			TxData->TimerHandle1 = -1;
			/* g_txEvtCB(TxData, TX_TIMER_J); */
			TxData->state = TX_TERMINATED;
			TXTRACE0("[Server_Non_Invite_SM] Tx -> TERMINATED\n");
			g_txEvtCB(TxData, TX_TERMINATED_EVENT);
			goto TERMINATED;
		}
		break;
	
	default:
		if (msg != NULL)
		{
			sipReqFree(msg);
			msg = NULL;
		}
	}
	
TERMINATED:

	/* Unlock mutex of the Tx when leave the transaction statemachine */
	txMutexUnlock(TxData->mutex);
	TXTRACE0("[Server_Non_Invite_SM] Leaving\n");
}


