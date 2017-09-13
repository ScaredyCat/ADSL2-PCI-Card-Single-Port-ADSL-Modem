/* 
 * Copyright (C) 2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * ClientStateMachine.c
 *
 * $Id: ClientStateMachine.c,v 1.46 2005/10/13 07:20:03 ljchuang Exp $
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


/* extern SipTxEvtCB g_txEvtCB; */
void g_txEvtCB(TxStruct TxData, SipTxEvtType event);

int GetStatusCode(SipRsp msg);
int sipTxAddTimer(TxStruct TxData, SipTxEvtType Timer);
void sipTxDelTimer(unsigned short TimerHandle);


SipReq NewAckFromTx(const TxStruct TxData)
{
	SipReq ackmsg;
	SipReqLine* newReqLine;
	SipReqLine* oriReqLine;
	SipVia *via;
	SipFrom *from;
	SipTo	*to;
	SipRoute *route = NULL;
	char	*ID;
	SipCSeq	*newseq;
	SipCSeq *oriseq;
	int maxfor;

	ackmsg = sipReqNew();
	
	/*Construct request line from original request*/
	newReqLine = (SipReqLine*)malloc(sizeof(SipReqLine));
	assert (newReqLine);
	oriReqLine = sipReqGetReqLine(TxData->OriginalReq);
	newReqLine->iMethod = ACK;
	newReqLine->ptrRequestURI = strDup(oriReqLine->ptrRequestURI);
	newReqLine->ptrSipVersion = strDup(oriReqLine->ptrSipVersion);
	
	sipReqSetReqLine(ackmsg, newReqLine);
	
	free(newReqLine->ptrRequestURI);
	free(newReqLine->ptrSipVersion);
	free(newReqLine);

	/*Construct via header from original request*/
	via=(SipVia*)sipReqGetHdr(TxData->OriginalReq,Via);
	if (via!= NULL)
		sipReqAddHdr(ackmsg,Via,via);
		
	/*Construct other header from original request*/
	from=(SipFrom*)sipReqGetHdr(TxData->OriginalReq,From);
	sipReqAddHdr(ackmsg,From,from);
	
	to=(SipTo*)sipRspGetHdr(TxData->LatestRsp,To);
	sipReqAddHdr(ackmsg,To,to);
	
	ID=(char*)sipReqGetHdr(TxData->OriginalReq,Call_ID);
	sipReqAddHdr(ackmsg,Call_ID,ID);

	route =(SipRoute*)sipReqGetHdr(TxData->OriginalReq,Route);
	if (route)
		sipReqAddHdr(ackmsg,Route,route);

	maxfor=(int)sipReqGetHdr(TxData->OriginalReq,Max_Forwards);
	sipReqAddHdr(ackmsg,Max_Forwards,(void*)maxfor);
	
	oriseq=(SipCSeq*)sipReqGetHdr(TxData->OriginalReq,CSeq);
	newseq=(SipCSeq*)malloc(sizeof(SipCSeq));
	assert (newseq);
	newseq->Method = ACK;
	newseq->seq = oriseq->seq;
	sipReqAddHdr(ackmsg,CSeq,newseq);
	free(newseq);
	
	return ackmsg;
}

void Client_Invite_SM(TxStruct TxData, SipTxEvtType EvtType, SipRsp msg)
{
	int status;
	
#ifdef CCLSIP_SIPTX_ENABLE_TRACE
	int n;
#endif

	/* Lock mutex of the Tx when enter transaction statemachine */
	txMutexLock(TxData->mutex);

	TXTRACE1("[Client_Invite_SM] Entering %s Transaction\n", sipMethodTypeToName(TxData->method) );
	TXTRACE1("[Client_Invite_SM] Transaction internal id:(%d)\n", TxData->InternalNum );
	TXTRACE1("[Client_Invite_SM] TxState = %s\n", sipTxState2Phase(TxData->state) );

	if ( TX_TCP_CONNECTED == EvtType ) {
		SendRequest(TxData);
		goto TERMINATED;
	}

	if ( TX_TRANSPORT_ERROR == EvtType ) {
		g_txEvtCB(TxData, TX_TRANSPORT_ERROR);
		TxData->state = TX_TERMINATED;
		TXTRACE0("[Client_Invite_SM] Tx -> TERMINATED\n");
		g_txEvtCB(TxData, TX_TERMINATED_EVENT);
		goto TERMINATED;
	}

	switch (TxData->state) {
	case TX_INIT:
		if ( RC_ERROR == SendRequest(TxData) ) {
			g_txEvtCB(TxData, TX_TRANSPORT_ERROR);
			TxData->state = TX_TERMINATED;
			TXTRACE0("[Client_Invite_SM] Tx -> TERMINATED\n");
			g_txEvtCB(TxData, TX_TERMINATED_EVENT);
			goto TERMINATED;	
		}

		if (TxData->txptype == UDP) {
			TxData->TimerA = T1;
			TxData->TimerHandle1 = sipTxAddTimer(TxData, TX_TIMER_A);
		} else {
			TxData->TimerB = 64 * T1;
			TxData->TimerHandle2 = sipTxAddTimer(TxData, TX_TIMER_B);
		}
		TxData->state = TX_CALLING;
		TXTRACE0("[Client_Invite_SM] Tx -> CALLING\n");
		break;

	case TX_CALLING:
		if (TX_TIMER_A == EvtType) {
			TXTRACE0("[Client_Invite_SM] Process Timer A\n");
			
#ifdef CCLSIP_SIPTX_ENABLE_TRACE
			frexp( (TxData->TimerA / T1), &n );
			TXTRACE1("[Client_Invite_SM] %dth re-transmit\n", n+1 );
#endif

			g_txEvtCB(TxData, TX_TIMER_A);

			if ( RC_ERROR == SendRequest(TxData) ) {
				/* no more arm TimerA */
				TxData->TimerHandle1 = -1;
				g_txEvtCB(TxData, TX_TRANSPORT_ERROR);
				TxData->state = TX_TERMINATED;
				TXTRACE0("[Client_Invite_SM] Tx -> TERMINATED\n");
				g_txEvtCB(TxData, TX_TERMINATED_EVENT);
				goto TERMINATED;
			}
			if ( TxData->TimerA == (32*T1) ) {
				/* no more arm TimerA */
				TxData->TimerHandle1 = -1;
				/* Arm TimerB */
				TxData->TimerB = T1;
				TxData->TimerHandle2 = sipTxAddTimer(TxData, TX_TIMER_B);
			} else {
				TxData->TimerA = TxData->TimerA * 2;
				TxData->TimerHandle1 = sipTxAddTimer(TxData, TX_TIMER_A);
			}
		}
		if (TX_TIMER_B == EvtType) {
			TXTRACE0("[Client_Invite_SM] Process Timer B\n");
			/* no more arm TimerB */
			TxData->TimerHandle2 = -1;
			g_txEvtCB(TxData, TX_TIMER_B);
			TxData->state = TX_TERMINATED;
			TXTRACE0("[Client_Invite_SM] Tx -> TERMINATED\n");
			g_txEvtCB(TxData, TX_TERMINATED_EVENT);
			goto TERMINATED;
		}
		if (TX_RSP_RECEIVED == EvtType) {
			TXTRACE0("[Client_Invite_SM] RSP Received\n");

			/* Disarm TimerA */
			if (-1 != TxData->TimerHandle1) {
				sipTxDelTimer(TxData->TimerHandle1);
				TxData->TimerHandle1 = -1;
			}
			
			/* Disarm TimerB */
			if (-1 != TxData->TimerHandle2) {
				sipTxDelTimer(TxData->TimerHandle2);
				TxData->TimerHandle2 = -1;
			}

			if (NULL != TxData->LatestRsp) {
				sipRspFree(TxData->LatestRsp);
			}
			TxData->LatestRsp = msg;
	
			g_txEvtCB(TxData, TX_RSP_RECEIVED);
			
			status = GetStatusCode(msg);
			if (status >= 100 && status < 200) {
				TxData->state = TX_PROCEEDING;
				TXTRACE0("[Client_Invite_SM] Tx -> PROCEEDING\n");
			}
			if (status >= 200 && status < 700) {
				if (status >= 200 && status < 300) {
					if (g_bMatchACK) {
						/* let the transaction user to assign an ACK message */
						g_txEvtCB(TxData, TX_2XX_FOR_INVITE);
					} else {
						TxData->state = TX_TERMINATED;
						TXTRACE0("[Client_Invite_SM] Tx -> TERMINATED\n");
						g_txEvtCB(TxData, TX_TERMINATED_EVENT);
						goto TERMINATED;
					}
				} else {
					/* automatically generate ACK for 300-699 responses */
					TxData->Ack = NewAckFromTx(TxData);
				}

				if ( RC_ERROR == SendACK(TxData) ) {
					g_txEvtCB(TxData, TX_TRANSPORT_ERROR);
					TxData->state = TX_TERMINATED;
					TXTRACE0("[Client_Invite_SM] Tx -> TERMINATED\n");
					g_txEvtCB(TxData, TX_TERMINATED_EVENT);
					goto TERMINATED;
				}
				
				/* Arm TimerD */
				if ( TxData->txptype == UDP ) {
					TxData->TimerD = 32000;
					TxData->TimerHandle3 = sipTxAddTimer(TxData, TX_TIMER_D);
					TxData->state = TX_COMPLETED;
					TXTRACE0("[Client_Invite_SM] Tx -> COMPLETED\n");
				} else {
					TxData->state = TX_TERMINATED;
					TXTRACE0("[Client_Invite_SM] Tx -> TERMINATED\n");
					g_txEvtCB(TxData, TX_TERMINATED_EVENT);
					goto TERMINATED;
				}
			}
		}
		break;

	case TX_PROCEEDING:
		if (TX_RSP_RECEIVED == EvtType) {
			TXTRACE0("[Client_Invite_SM] RSP Received\n");
			
			if (NULL != TxData->LatestRsp) {
				sipRspFree(TxData->LatestRsp);
			}
			TxData->LatestRsp = msg;

			g_txEvtCB(TxData, TX_RSP_RECEIVED);
			
			status = GetStatusCode(msg);
			if (status >= 100 && status < 200) {
				TxData->state = TX_PROCEEDING;
				TXTRACE0("[Client_Invite_SM] Tx -> PROCEEDING\n");
			}
			if (status >= 200 && status < 700) {
				if (status >= 200 && status < 300) {
					if (g_bMatchACK) {
						/* let the transaction user to assign an ACK message */
						g_txEvtCB(TxData, TX_2XX_FOR_INVITE);
					} else {
						TxData->state = TX_TERMINATED;
						TXTRACE0("[Client_Invite_SM] Tx -> TERMINATED\n");
						g_txEvtCB(TxData, TX_TERMINATED_EVENT);
						goto TERMINATED;
					}
				} else {
					/* automatically generate ACK for 300-699 responses */
					TxData->Ack = NewAckFromTx(TxData);
				}

				if ( RC_ERROR == SendACK(TxData) ) {
					g_txEvtCB(TxData, TX_TRANSPORT_ERROR);
					TxData->state = TX_TERMINATED;
					TXTRACE0("[Client_Invite_SM] Tx -> TERMINATED\n");
					g_txEvtCB(TxData, TX_TERMINATED_EVENT);
					goto TERMINATED;
				}
				
				/* Arm TimerD */
				if ( TxData->txptype == UDP ) {
					TxData->TimerD = 32000;
					TxData->TimerHandle3 = sipTxAddTimer(TxData, TX_TIMER_D);
					TxData->state = TX_COMPLETED;
					TXTRACE0("[Client_Invite_SM] Tx -> COMPLETED\n");
				} else {
					TxData->state = TX_TERMINATED;
					TXTRACE0("[Client_Invite_SM] Tx -> TERMINATED\n");
					g_txEvtCB(TxData, TX_TERMINATED_EVENT);
					goto TERMINATED;
				}
			}
		}
		break;

	case TX_COMPLETED:
		if (TX_TIMER_D == EvtType) {
			TXTRACE0("[Client_Invite_SM] Process Timer D\n");
			/* no more arm TimerD */
			TxData->TimerHandle3 = -1;
			/* g_txEvtCB(TxData, TX_TIMER_D); */
			TxData->state = TX_TERMINATED;
			TXTRACE0("[Client_Invite_SM] Tx -> TERMINATED\n");
			g_txEvtCB(TxData, TX_TERMINATED_EVENT);
			goto TERMINATED;
		}
		if (TX_RSP_RECEIVED == EvtType) {
			TXTRACE0("[Client_Invite_SM] RSP Received\n");
			
			if (NULL != TxData->LatestRsp) {
				sipRspFree(TxData->LatestRsp);
			}
			TxData->LatestRsp = msg;

			status = GetStatusCode(msg);
			/* if (status >= 300 && status < 700) { */
			if (status >= 200 && status < 700) {
				if (status >= 200 && status < 300) {
					if (!g_bMatchACK) {
						TXERR0("[Client_Invite_SM] Received 2xx response in COMPLETED state");
						goto TERMINATED;
					}
				}
				if ( RC_ERROR == SendACK(TxData) ) {
					g_txEvtCB(TxData, TX_TRANSPORT_ERROR);
					TxData->state = TX_TERMINATED;
					TXTRACE0("[Client_Invite_SM] Tx -> TERMINATED\n");
					g_txEvtCB(TxData, TX_TERMINATED_EVENT);
					goto TERMINATED;
				}
				TxData->state = TX_COMPLETED;
				TXTRACE0("[Client_Invite_SM] Tx -> COMPLETED\n");
			}
		}
		break;

	default:
		if (msg != NULL)
		{
			sipRspFree(msg);
			msg = NULL;
		}
	}

TERMINATED:

	/* Unlock mutex of the Tx when leave the transaction statemachine */
	txMutexUnlock(TxData->mutex);
	TXTRACE0("[Client_Invite_SM] Leaving\n");
}


void Client_Non_Invite_SM(TxStruct TxData, SipTxEvtType EvtType, SipRsp msg)
{
	int status;
	
#ifdef CCLSIP_SIPTX_ENABLE_TRACE
	int n;
#endif

	/* Lock mutex of the Tx when enter transaction statemachine */
	txMutexLock(TxData->mutex);

	TXTRACE1("[Client_Non_Invite_SM] Entering %s Transaction\n", sipMethodTypeToName(TxData->method) );
	TXTRACE1("[Client_Non_Invite_SM] Transaction internal id:(%d)\n", TxData->InternalNum );
	TXTRACE1("[Client_Non_Invite_SM] TxState = %s\n", sipTxState2Phase(TxData->state) );

	if ( TX_TCP_CONNECTED == EvtType ) {
		SendRequest(TxData);
		goto TERMINATED;
	}

	if ( TX_TRANSPORT_ERROR == EvtType ) {
		g_txEvtCB(TxData, TX_TRANSPORT_ERROR);
		TxData->state = TX_TERMINATED;
		TXTRACE0("[Client_Invite_SM] Tx -> TERMINATED\n");
		g_txEvtCB(TxData, TX_TERMINATED_EVENT);
		goto TERMINATED;
	}
	
	switch (TxData->state) {
	case TX_INIT:
		if ( RC_ERROR == SendRequest(TxData) ) {
			g_txEvtCB(TxData, TX_TRANSPORT_ERROR);
			TxData->state = TX_TERMINATED;
			TXTRACE0("[Client_Invite_SM] Tx -> TERMINATED\n");
			g_txEvtCB(TxData, TX_TERMINATED_EVENT);
			goto TERMINATED;	
		}

		if (TxData->txptype == UDP) {
			TxData->TimerE = T1;
			TxData->TimerHandle1 = sipTxAddTimer(TxData, TX_TIMER_E);
		} else {
			TxData->TimerF = 64 * T1;
			TxData->TimerHandle2 = sipTxAddTimer(TxData, TX_TIMER_F);
		}
		TxData->state = TX_TRYING;
		TXTRACE0("[Client_Non_Invite_SM] Tx -> TRYING\n");
		break;

	case TX_TRYING:
	case TX_PROCEEDING:
		if (TX_TIMER_E == EvtType) {
			TXTRACE0("[Client_Non_Invite_SM] Process Timer E\n");

#ifdef CCLSIP_SIPTX_ENABLE_TRACE
			if (TxData->TimerF > T2 )
			{
				n = (int)floor( TxData->TimerF / T2 ) + 3;
			}
			else
			{
				frexp( (TxData->TimerE / T1), &n );
			}
			TXTRACE1("[Client_Non_Invite_SM] %dth re-transmit\n", n);
#endif
			
			g_txEvtCB(TxData, TX_TIMER_E);
		
			if ( RC_ERROR == SendRequest(TxData) ) {
				/* no more arm TimerE */
				TxData->TimerHandle1 = -1;
				g_txEvtCB(TxData, TX_TRANSPORT_ERROR);
				TxData->state = TX_TERMINATED;
				TXTRACE0("[Client_Non_Invite_SM] Tx -> TERMINATED\n");
				g_txEvtCB(TxData, TX_TERMINATED_EVENT);
				goto TERMINATED;
			}
			
			if ( TxData->state == TX_PROCEEDING )
				TxData->TimerE = T2;
			else
				TxData->TimerE = ( (TxData->TimerE*2) > T2) ? T2 : (TxData->TimerE * 2);
				
			TxData->TimerF += TxData->TimerE;
			if ( TxData->TimerF > (64*T1) ) {
				/* no more arm TimerE */
				TxData->TimerHandle1 = -1;
				/* Arm TimerF */
				TxData->TimerF = T1;
				TxData->TimerHandle2 = sipTxAddTimer(TxData, TX_TIMER_F);
			} else {
				TxData->TimerHandle1 = sipTxAddTimer(TxData, TX_TIMER_E);
			}
		}
		if (TX_TIMER_F == EvtType) {
			TXTRACE0("[Client_Non_Invite_SM] Process Timer F\n");
			/* no more arm TimerF */
			TxData->TimerHandle2 = -1;
			g_txEvtCB(TxData, TX_TIMER_F);
			TxData->state = TX_TERMINATED;
			TXTRACE0("[Client_Non_Invite_SM] Tx -> TERMINATED\n");
			g_txEvtCB(TxData, TX_TERMINATED_EVENT);
			goto TERMINATED;
		}
		if (TX_RSP_RECEIVED == EvtType) {
			TXTRACE0("[Client_Non_Invite_SM] RSP Received\n");

			if (NULL != TxData->LatestRsp) {
				sipRspFree(TxData->LatestRsp);
			}
			TxData->LatestRsp = msg;

			g_txEvtCB(TxData, TX_RSP_RECEIVED);
			
			status = GetStatusCode(msg);
			if (status >= 100 && status < 200) {
				TxData->state = TX_PROCEEDING;
				TXTRACE0("[Client_Non_Invite_SM] Tx -> PROCEEDING\n");
			}
			if (status >= 200 && status < 700) {
				/* Disarm TimerE */
				if (-1 != TxData->TimerHandle1) {
					sipTxDelTimer(TxData->TimerHandle1);
					TxData->TimerHandle1 = -1;
				}
			
				/* Disarm TimerF */
				if (-1 != TxData->TimerHandle2) {
					sipTxDelTimer(TxData->TimerHandle2);
					TxData->TimerHandle2 = -1;
				}
				
				/* Arm TimerK */
				if ( TxData->txptype == UDP ) {
					TxData->TimerK = T4;
					TxData->TimerHandle3 = sipTxAddTimer(TxData, TX_TIMER_K);
					TxData->state = TX_COMPLETED;
					TXTRACE0("[Client_Non_Invite_SM] Tx -> COMPLETED\n");
				} else {
					TxData->state = TX_TERMINATED;
					TXTRACE0("[Client_Non_Invite_SM] Tx -> TERMINATED\n");
					g_txEvtCB(TxData, TX_TERMINATED_EVENT);
					goto TERMINATED;
				}
			}
		}
		break;
	
	case TX_COMPLETED:
		if (msg != NULL)
		{
			sipRspFree(msg);
			msg = NULL;
		}

		if (TX_TIMER_K == EvtType) {
			TXTRACE0("[Client_Non_Invite_SM] Process Timer K\n");
			/* no more arm TimerK */
			TxData->TimerHandle3 = -1;
			TxData->state = TX_TERMINATED;
			TXTRACE0("[Client_Non_Invite_SM] Tx -> TERMINATED\n");
			g_txEvtCB(TxData, TX_TERMINATED_EVENT);
			goto TERMINATED;
		}

		break;

	default:
		if (msg != NULL)
		{
			sipRspFree(msg);
			msg = NULL;
		}
	}

TERMINATED:

	/* Unlock mutex of the Tx when leave the transaction statemachine */
	txMutexUnlock(TxData->mutex);
	TXTRACE0("[Client_Non_Invite_SM] Leaving\n");
}

void Client_2XX_SM(TxStruct TxData, SipTxEvtType EvtType, SipReq msg)
{

#ifdef CCLSIP_SIPTX_ENABLE_TRACE
	int n;
#endif

	/* Lock mutex of the Tx when enter transaction statemachine */
	txMutexLock(TxData->mutex);

	TXTRACE1("[Client_2XX_SM] Entering %s Transaction\n", sipMethodTypeToName(TxData->method) );
	TXTRACE1("[Client_2XX_SM] Transaction internal id:(%d)\n", TxData->InternalNum );
	TXTRACE1("[Client_2XX_SM] TxState = %s\n", sipTxState2Phase(TxData->state) );

	if ( TX_TCP_CONNECTED == EvtType ) {
		SendResponse(TxData);
		goto TERMINATED;
	}

	if ( TX_TRANSPORT_ERROR == EvtType ) {
		g_txEvtCB(TxData, TX_TRANSPORT_ERROR);
		TxData->state = TX_TERMINATED;
		TXTRACE0("[Client_2XX_SM] Tx -> TERMINATED\n");
		g_txEvtCB(TxData, TX_TERMINATED_EVENT);
		goto TERMINATED;
	}

	switch (TxData->state) {
	case TX_COMPLETED:
		if (TX_REQ_RECEIVED == EvtType) {
			TXTRACE0("[Client_2XX_SM] REQ Received\n");
			
			if (msg != NULL)
			{
				sipReqFree(msg);
				msg = NULL;
			}

			if (RC_ERROR == SendResponse(TxData)) {
				g_txEvtCB(TxData, TX_TRANSPORT_ERROR);
				TxData->state = TX_TERMINATED;
				TXTRACE0("[Client_2XX_SM] Tx -> TERMINATED\n");
				g_txEvtCB(TxData, TX_TERMINATED_EVENT);
				goto TERMINATED;
			}
		}
		if (TX_TIMER_G == EvtType) {
			TXTRACE0("[Client_2XX_SM] Process Timer G\n");
			
#ifdef CCLSIP_SIPTX_ENABLE_TRACE
			if (TxData->TimerH > T2 )
			{
				n = (int)floor( TxData->TimerH / T2 ) + 3;
			}
			else
			{
				frexp( (TxData->TimerG / T1), &n );
			}
			TXTRACE1("[Client_2XX_SM] %dth re-transmit\n", n);
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
			TXTRACE0("[Client_2XX_SM] Process Timer H\n");
			/* no more arm TimerH */
			TxData->TimerHandle2 = -1;
			g_txEvtCB(TxData, TX_TIMER_H);
			TxData->state = TX_TERMINATED;
			TXTRACE0("[Client_2XX_SM] Tx -> TERMINATED\n");
			g_txEvtCB(TxData, TX_TERMINATED_EVENT);
			goto TERMINATED;
		}
		if (TX_ACK_RECEIVED == EvtType) {
			TXTRACE0("[Client_2XX_SM] ACK Received\n");
			
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
				TXTRACE0("[Client_2XX_SM] Tx -> CONFIRMED\n");
			} else {
				TxData->state = TX_TERMINATED;
				TXTRACE0("[Client_2XX_SM] Tx -> TERMINATED\n");
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
			TXTRACE0("[Client_2XX_SM] Process Timer I\n");
			/* no more arm TimerI */
			TxData->TimerHandle3 = -1;
			/* g_txEvtCB(TxData, TX_TIMER_I); */
			TxData->state = TX_TERMINATED;
			TXTRACE0("[Client_2XX_SM] Tx -> TERMINATED\n");
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
	TXTRACE0("[Client_2XX_SM] Leaving\n");

}

void Client_Ack_SM(TxStruct TxData, SipTxEvtType EvtType, SipRsp msg)
{
	int status;
	RCODE retcode;
	
	/* Lock mutex of the Tx when enter transaction statemachine */
	txMutexLock(TxData->mutex);

	TXTRACE1("[Client_Ack_SM] Entering %s Transaction\n", sipMethodTypeToName(TxData->method) );
	TXTRACE1("[Client_Ack_SM] Transaction internal id:(%d)\n", TxData->InternalNum );
	TXTRACE1("[Client_Ack_SM] TxState = %s\n", sipTxState2Phase(TxData->state) );

	/*
	if ( TX_TCP_CONNECTED == EvtType ) {
		SendRequest(TxData);
		goto TERMINATED;
	}
	*/

	if ( TX_TRANSPORT_ERROR == EvtType ) {
		g_txEvtCB(TxData, TX_TRANSPORT_ERROR);
		TxData->state = TX_TERMINATED;
		TXTRACE0("[Client_Ack_SM] Tx -> TERMINATED\n");
		g_txEvtCB(TxData, TX_TERMINATED_EVENT);
		goto TERMINATED;
	}

	switch (TxData->state) {
	case TX_INIT:
		retcode = SendRequest(TxData);

		if ( RC_ERROR == retcode ) {
			g_txEvtCB(TxData, TX_TRANSPORT_ERROR);
			TxData->state = TX_TERMINATED;
			TXTRACE0("[Client_Ack_SM] Tx -> TERMINATED\n");
			g_txEvtCB(TxData, TX_TERMINATED_EVENT);
			goto TERMINATED;	
		}

		if ( RC_TCP_CONNECTING == retcode )
			goto TERMINATED;

		/* Arm TimerD */
		if ( TxData->txptype == UDP ) {
			TxData->TimerD = 32000;
			TxData->TimerHandle3 = sipTxAddTimer(TxData, TX_TIMER_D);
			TxData->state = TX_COMPLETED;
			TXTRACE0("[Client_Ack_SM] Tx -> COMPLETED\n");
		} else {
			TxData->state = TX_TERMINATED;
			TXTRACE0("[Client_Ack_SM] Tx -> TERMINATED\n");
			g_txEvtCB(TxData, TX_TERMINATED_EVENT);
			goto TERMINATED;
		}
		break;

	case TX_COMPLETED:
		if (TX_TIMER_D == EvtType) {
			TXTRACE0("[Client_Ack_SM] Process Timer D\n");
			/* no more arm TimerD */
			TxData->TimerHandle3 = -1;
			/* g_txEvtCB(TxData, TX_TIMER_D); */
			TxData->state = TX_TERMINATED;
			TXTRACE0("[Client_Ack_SM] Tx -> TERMINATED\n");
			g_txEvtCB(TxData, TX_TERMINATED_EVENT);
			goto TERMINATED;
		}
		if (TX_RSP_RECEIVED == EvtType) {
			TXTRACE0("[Client_Ack_SM] RSP Received\n");
			
			if (NULL != TxData->LatestRsp) {
				sipRspFree(TxData->LatestRsp);
			}
			TxData->LatestRsp = msg;

			status = GetStatusCode(msg);
			/* if (status >= 300 && status < 700) { */
			if (status >= 200 && status < 700) {
				if ( RC_ERROR == SendRequest(TxData) ) {
					g_txEvtCB(TxData, TX_TRANSPORT_ERROR);
					TxData->state = TX_TERMINATED;
					TXTRACE0("[Client_Ack_SM] Tx -> TERMINATED\n");
					g_txEvtCB(TxData, TX_TERMINATED_EVENT);
					goto TERMINATED;
				}
			}
		}
		break;

	default:
		if (msg != NULL)
		{
			sipRspFree(msg);
			msg = NULL;
		}
	}

TERMINATED:

	/* Unlock mutex of the Tx when leave the transaction statemachine */
	txMutexUnlock(TxData->mutex);
	TXTRACE0("[Client_Ack_SM] Leaving\n");

}

