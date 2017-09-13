/* 
 * Copyright (C) 2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sipTx.c
 *
 * $Id: sipTx.c,v 1.148 2006/12/05 09:44:15 tyhuang Exp $
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#ifdef CCLSIP_ENABLE_STUN
#include "stunapi.h"
#endif

#include <common/cm_def.h>
#include <adt/dx_lst.h>
#include <sip/cclsip.h>
#include "sipTx.h"
#include "TxStruct.h"
#include "Transport.h"
#include "StateMachine.h"
#include "CTransactionDatabase.h"

TTransactionDB_C * TxDatabase;
SipTxEvtCB TU_txEvtCB;
/*
DxLst ReqSendQ = NULL;
DxLst RspSendQ = NULL;
DxLst TxList = NULL;
DxLst GarbageQ = NULL;
*/

DxLst TimerQ = NULL;
DxLst DispatchEvtQ = NULL;
int txnum = 0;
SipUDP g_sipUDP;
SipUDP *g_sipUDPs = NULL;
SipTCP *g_sipTCPs = NULL;
BOOL g_bMatchACK;

unsigned short g_nMaxTxCount;
unsigned short *TimerTable = NULL;
unsigned short g_nTimerHandle;
CxMutex TimerMutex;

#define TIMERTICK 100	/* do not modify */
#define MAXTIMERCOUNT 65535 /* must be below 65535 */

CommonTimer *CommonTimerTable = NULL;

char * strLocalAddress = NULL;
UINT16 usLocalPort = 0;
unsigned short g_nAddrPortNum = 0;

#ifdef CCLSIP_ENABLE_STUN
	char strExtAddress[128];
	UINT16 usExtPort = 0;
	char* strSTUNserver = NULL;
#endif

#ifdef _WIN32_WCE
#include <windows.h>
HANDLE gTxTimerThreadHandle;
HANDLE gTxTimerEventHandle;
#endif

void _sipTxFree(TxStruct _this);

CCLAPI AddrPort AddrPortNew( char* addr, unsigned short port )
{
	AddrPort tmpAddrPort = NULL;
	
	if ((NULL == addr) || (strlen(addr) == 0))
		return NULL;

	tmpAddrPort = (AddrPort)malloc( sizeof (struct _AddrPort ) );
	tmpAddrPort->strAddr = strDup( addr );
	tmpAddrPort->uPort = port;

	return tmpAddrPort;
}

CCLAPI void AddrPortFree( AddrPort tmpAddrPort )
{
	if (NULL == tmpAddrPort)
		return;

	free(tmpAddrPort->strAddr);
	tmpAddrPort->strAddr = NULL;

	free( tmpAddrPort );
	tmpAddrPort = NULL;
	return;
}

void txPutDispatchEvtQ( TxStruct TxData, TXDISPATCHEVTTYPE evttype)
{
	TxDispatchEvtObj evtObj = NULL;

	if (!TxData)
		return;
	
	/* Lock mutex of the Tx when enter */
	txMutexLock(TxData->mutex);

	if(evttype == TX_DISPATCH_GARBAGE){
		/* put in garbage queue */
		/* check only if it exists */
		if(TxData->bGarbage!=TRUE)
			TxData->bGarbage=TRUE;
		
	}

	evtObj = (TxDispatchEvtObj) calloc(1, sizeof(struct _TxDispatchEvtObj) );

	evtObj->txdata = TxData;
	evtObj->evt = evttype;
	dxLstPutTail(DispatchEvtQ, evtObj);
	

	/* Unlock mutex of the Tx when leave */
	txMutexUnlock(TxData->mutex);
}

void g_txEvtCB(TxStruct TxData, SipTxEvtType event)
{
	if (TxData->bNoCB)
	{	
		if (TxData->state == TX_TERMINATED)
			txPutDispatchEvtQ(TxData, TX_DISPATCH_GARBAGE);	
		return;

	}

	TXTRACE1("[g_txEvtCB] Post event to TU: %s\n", sipTxEvtType2Phase(event) );

	/* Unlock mutex of the Tx before callback to TU */
	txMutexUnlock(TxData->mutex);
	
	/* Callback to TU */
	TU_txEvtCB(TxData, event);
	
	/* Lock mutex after TU callback function return */
	txMutexLock(TxData->mutex);
}

void sipTxTimerProcess(unsigned short id, void* param)
{
	TxTimer TimerData = (TxTimer)calloc(1,sizeof(struct _TxTimer));
	
	if(!TimerData) return;
	TimerData->data = (TxStruct) param;
	TimerData->type = TimerData->data->TimerEvent;

	switch (TimerData->type) {
		case TX_TIMER_A:
			TXTRACE1("[sipTxTimerProcess] Timer A Fired:(%d)\n", TimerData->data->InternalNum);
			break;
		case TX_TIMER_B:	
			TXTRACE1("[sipTxTimerProcess] Timer B Fired:(%d)\n", TimerData->data->InternalNum);
			break;
		case TX_TIMER_C:	
			TXTRACE1("[sipTxTimerProcess] Timer C Fired:(%d)\n", TimerData->data->InternalNum);
			break;
		case TX_TIMER_D:
			TXTRACE1("[sipTxTimerProcess] Timer D Fired:(%d)\n", TimerData->data->InternalNum);
			break;
		case TX_TIMER_E:	
			TXTRACE1("[sipTxTimerProcess] Timer E Fired:(%d)\n", TimerData->data->InternalNum);
			break;
		case TX_TIMER_F:	
			TXTRACE1("[sipTxTimerProcess] Timer F Fired:(%d)\n", TimerData->data->InternalNum);
			break;
		case TX_TIMER_G:
			TXTRACE1("[sipTxTimerProcess] Timer G Fired:(%d)\n", TimerData->data->InternalNum);
			break;
		case TX_TIMER_H:
			TXTRACE1("[sipTxTimerProcess] Timer H Fired:(%d)\n", TimerData->data->InternalNum);
			break;
		case TX_TIMER_I:	
			TXTRACE1("[sipTxTimerProcess] Timer I Fired:(%d)\n", TimerData->data->InternalNum);
			break;
		case TX_TIMER_J:
			TXTRACE1("[sipTxTimerProcess] Timer J Fired:(%d)\n", TimerData->data->InternalNum);
			break;
		case TX_TIMER_K:	
			TXTRACE1("[sipTxTimerProcess] Timer K Fired:(%d)\n", TimerData->data->InternalNum);
			break;
		default:
			TXWARN0("[sipTxTimerProcess] Unknown Timer event!\n");
	}

	dxLstPutTail(TimerQ, TimerData);
	TXTRACE1("[sipTxTimerProcess] TimerQ : size = %d \n",dxLstGetSize(TimerQ));
}

void sipTxTimerManager(int e,void* param)
{
	UINT16 i;
	cxMutexLock(TimerMutex);
	
	for (i=0; i < MAXTIMERCOUNT; i++)
	{
		if (NULL != CommonTimerTable[i])
		{
			CommonTimerTable[i]->time -= TIMERTICK;
			if (CommonTimerTable[i]->time == 0)
			{
				CommonTimerTable[i]->cb( i, CommonTimerTable[i]->data);
				free(CommonTimerTable[i]);
				CommonTimerTable[i] = NULL;
			}
		}
	}


	cxMutexUnlock(TimerMutex);

#ifndef _WIN32_WCE
	g_nTimerHandle = sipTimerSet(TIMERTICK, sipTxTimerManager, NULL);
#endif

}

#ifdef _WIN32_WCE
void sipTxTimerThread(void)
{
	int e=0;
	while ( WAIT_OBJECT_0 != WaitForSingleObject(gTxTimerEventHandle, TIMERTICK) ) {
		sipTxTimerManager(e,NULL);
	}

	CloseHandle(gTxTimerEventHandle);
	CloseHandle(gTxTimerThreadHandle);
	gTxTimerEventHandle = NULL;
	gTxTimerThreadHandle = NULL;
}
#endif


int sipTxAddTimer(TxStruct TxData, SipTxEvtType TimerEvent)
{
	unsigned short TimerTime;
	unsigned short retval = 0;

	TxData->TimerEvent = TimerEvent;

	switch (TimerEvent) {
	case TX_TIMER_A:	
		TimerTime = TxData->TimerA;
		break;
	case TX_TIMER_B:	
		TimerTime = TxData->TimerB;
		break;
	case TX_TIMER_C:	
		TimerTime = TxData->TimerC;
		break;
	case TX_TIMER_D:	
		TimerTime = TxData->TimerD;
		break;
	case TX_TIMER_E:	
		TimerTime = TxData->TimerE;
		break;
	case TX_TIMER_F:	
		TimerTime = TxData->TimerF;
		break;
	case TX_TIMER_G:	
		TimerTime = TxData->TimerG;
		break;
	case TX_TIMER_H:	
		TimerTime = TxData->TimerH;
		break;
	case TX_TIMER_I:	
		TimerTime = TxData->TimerI;
		break;
	case TX_TIMER_J:	
		TimerTime = TxData->TimerJ;
		break;
	case TX_TIMER_K:	
		TimerTime = TxData->TimerK;
		break;
	default:
		break;
	}
	
	if (RC_OK == CommonTimerAdd( TimerTime, (void*)TxData, (CommonTimerCB)sipTxTimerProcess, &retval ) )
	{
		TXTRACE1("[sipTxAddTimer] Timer #%d set successful!!\n", TxData->InternalNum);
		return retval;
	}
	return -1;
}

void sipTxDelTimer(unsigned short TimerHandle)
{
	void* tmp = NULL;
	RCODE	retval=CommonTimerDel( TimerHandle, &tmp );
	
	if ( RC_OK == retval )
		TXTRACE1("[sipTxDelTimer] Timer #%d delete successful\n", TimerHandle);
	else
	{
		TXERR1("[sipTxDelTimer] Timer #%d delete fail !!! \n", TimerHandle);
		printf("ERR %d\n",retval);
	}	
}

RCODE CommonTimerAdd(unsigned long time, 
					 void* data, 
					 CommonTimerCB cb, 
					 unsigned short *timerid)
{
	UINT16 i;
	CommonTimer _this;

	if (NULL == CommonTimerTable)
		return RC_ERROR;

	if (NULL == cb)
		return RC_ERROR;

	if (NULL == timerid)
		return RC_ERROR;

	cxMutexLock(TimerMutex);

	for (i=0; i < MAXTIMERCOUNT; i++)
	{
		if ( NULL == CommonTimerTable[i] )
		{
			_this = (CommonTimer) malloc( sizeof(struct _CommonTimer) );
			_this->time = time;
			_this->cb = cb;
			_this->data = data;
			
			CommonTimerTable[i] = _this;

			*timerid = i;
			
			cxMutexUnlock(TimerMutex);
			
			return RC_OK;
		}
	}

	cxMutexUnlock(TimerMutex);
	return RC_ERROR;		
}

RCODE CommonTimerDel(unsigned short timerid, void** data)
{
	CommonTimer _this;
	
	if (NULL == CommonTimerTable)
		return RC_ERROR;
	
	if ( NULL == CommonTimerTable[timerid] )
		return RC_OK;
		
	cxMutexLock(TimerMutex);
	
	_this = CommonTimerTable[timerid];	
	
	CommonTimerTable[timerid] = NULL;

 	*data = _this->data;

	free(_this);
	_this = NULL;

	cxMutexUnlock(TimerMutex);
	
	return RC_OK;
}

TxStruct
sipTxHolderNew(SipRsp rsp,
	       UserProfile profile )
{
	TxStruct _this = NULL;

	_this = (TxStruct)malloc(sizeof (struct _TxStruct));
	if ( !_this ) {
		return NULL;
	}

	/* memset(_this, 0, sizeof (TxStruct)); */
	memset(_this, 0, sizeof (struct _TxStruct));

	_this->OriginalReq = NULL;
	_this->LatestRsp = rsp;
	_this->callid = (char*)sipRspGetHdr(rsp, Call_ID);
	_this->profile = UserProfileDup(profile);
	_this->UserData = NULL;
	_this->RefCount = 1;
	_this->transport = NULL;

	_this->TimerHandle1 = -1;
	_this->TimerHandle2 = -1;
	_this->TimerHandle3 = -1;

	_this->bNoCB = FALSE;
	_this->bGarbage = FALSE;
	
	_this->rport = 0;
	_this->raddr = NULL;

	_this->mutex = cxMutexNew(CX_MUTEX_INTERTHREAD, 0);
	_this->txid = NULL;
	_this->txtype = TX_HOLDER;
	_this->state = TX_TERMINATED;

	/*dxLstPutTail(TxList, _this);*/

	TXTRACE1("[sipTxHolderNew] Transaction internal id:(%d)\n", _this->InternalNum);
	TXTRACE1("[sipTxHolderNew] Current transaction count: %d\n", ++txnum);

	return _this;
}

CCLAPI TxStruct
sipTx2xxClientNew( TxStruct origTx )
{
	TxStruct _this = NULL;
	RCODE retcode;

	if ( origTx == NULL )
		return NULL;

	_this = (TxStruct)calloc(1,sizeof (struct _TxStruct));
	
	if ( !_this )
		return NULL;

	_this->OriginalReq = sipReqDup(origTx->OriginalReq);
	_this->LatestRsp = sipRspDup(origTx->LatestRsp);
	
	if (origTx->callid)
		/*_this->callid = strDup(origTx->callid);*/
		_this->callid = (char*)sipRspGetHdr(_this->LatestRsp, Call_ID);
	else
		_this->callid = NULL;

	_this->profile = NULL;/*UserProfileDup(origTx->profile);*/
	_this->UserData = NULL; /* origTx->UserData; */
	_this->RefCount = 1;
	_this->transport = NULL;

	_this->TimerHandle1 = -1;
	_this->TimerHandle2 = -1;
	_this->TimerHandle3 = -1;

	_this->bNoCB = FALSE;
	_this->bGarbage =FALSE;
	_this->rport = 0;
	_this->raddr = NULL;

	_this->mutex = cxMutexNew(CX_MUTEX_INTERTHREAD, 0);
	_this->txid = origTx->txid;;
	_this->txtype = TX_CLIENT_2XX;
	_this->state = TX_COMPLETED;
	_this->method = origTx->method;

	/*dxLstPutTail(TxList, _this);*/

	retcode = InsertTransaction ( TxDatabase, _this );
	
	if (RC_OK != retcode)
	{
		TXERR0("[sipTx2xxClientNew] insert Tx into transaction database fail !!! \n");
		_sipTxFree(_this);
		_this = NULL;
		return NULL;
	}

	TXTRACE1("[sipTx2xxClientNew] Transaction internal id:(%d)\n", _this->InternalNum);
	TXTRACE1("[sipTx2xxClientNew] Current transaction count: %d\n", ++txnum);

	_this->TimerG = T1;
	_this->TimerHandle1 = sipTxAddTimer(_this, TX_TIMER_G);

	return _this;
}


TxStruct sipTxServerNew(SipReq msg, 
			void* transport, 
			TXPTYPE txptype)
{
	RCODE retcode;
	TxStruct _this = NULL;
	SipReqLine* reqline = NULL;

	_this = (TxStruct)malloc(sizeof (struct _TxStruct));
	if ( !_this ) {
		return NULL;
	}

	/* memset(_this, 0, sizeof (TxStruct)); */
	memset(_this, 0, sizeof (struct _TxStruct));

	_this->OriginalReq = msg;
	_this->LatestRsp = NULL;
	_this->callid = (char*)sipReqGetHdr(msg, Call_ID);
	_this->profile = NULL;
	_this->UserData = NULL;
	_this->state = TX_INIT;
	_this->RefCount = 1;
	_this->txptype = txptype;
	/* _this->transport = transport; */

	_this->TimerHandle1 = -1;
	_this->TimerHandle2 = -1;
	_this->TimerHandle3 = -1;

	_this->bNoCB = FALSE;
	_this->bGarbage = FALSE;
	
	if (txptype == TCP) {
		_this->raddr = strDup( sipTCPGetRaddr(transport) );
		sipTCPGetRport(transport, &(_this->rport) );
		_this->transport = transport;
	} else {
		_this->raddr = strDup( sipUDPGetRaddr(transport) );
		sipUDPGetRport(transport, &(_this->rport) );
		_this->transport = NULL;
	}

	_this->mutex = cxMutexNew(CX_MUTEX_INTERTHREAD, 0);

	/* the transaction id is not useful in server transaction */
	_this->txid = NULL;

	reqline = sipReqGetReqLine(msg);
	_this->method = reqline->iMethod;

	if (INVITE == _this->method) { 
		TXTRACE0("[sipTxServerNew] new TX_SERVER_INVITE transaction\n");
		_this->txtype = TX_SERVER_INVITE;
	} else if (ACK == _this->method) {
		TXTRACE0("[sipTxServerNew] new TX_SERVER_ACK transaction\n");
		_this->txtype = TX_SERVER_ACK;
		_this->state = TX_TERMINATED;
	} else {
		TXTRACE0("[sipTxServerNew] new TX_SERVER_NON_INVITE transaction\n");
		_this->txtype = TX_SERVER_NON_INVITE;
	}

	/*dxLstPutTail(TxList, _this);*/

	retcode = InsertTransaction ( TxDatabase, _this );
	
	if (RC_OK != retcode)
	{
		/* Possibly caused by NULL Call-id */
		TXERR0("[sipTxServerNew] insert Tx into transaction database fail !!! \n");
		TXERR0("[sipTxServerNew] Change TxType to TX_HOLDER\n");
		_this->txtype = TX_HOLDER;
	}

	TXTRACE1("[sipTxServerNew] Transaction internal id:(%d)\n", _this->InternalNum);
	TXTRACE1("[sipTxServerNew] Current transaction count: %d\n", ++txnum);

	return _this;
}

CCLAPI TxStruct sipTxClientNew(SipReq msg,
			       char* txid,
			       UserProfile profile,
			       void* data)
{
	RCODE retcode;

	/* initiate all pointers to NULL */
	TxStruct _this = NULL;
	SipReqLine* reqline = NULL;
	char * strViaBranch = NULL;
	
	/* try to allocate memory */
	_this = (TxStruct)malloc(sizeof (struct _TxStruct));

	/* return failure if the memory allocation fail */
	if ( ! _this ) {
		TXERR0("[sipTxClientNew] memory allocation fail !!!");
		return NULL;
	}

	/* fill the whole structure with 0 */
	memset(_this, 0, sizeof (struct _TxStruct));

	/* fill basic fields */
	_this->OriginalReq = msg;
	_this->bOriginalReq_PreparedForStrictRouter = FALSE;
	
	_this->Ack = NULL;
	_this->bAck_PreparedForStrictRouter = FALSE;

	_this->LatestRsp = NULL;
	_this->callid = (char*)sipReqGetHdr(msg, Call_ID);
	_this->profile = UserProfileDup(profile);
	_this->UserData = data;
	_this->state = TX_INIT;
	_this->RefCount = 1;
	_this->txptype = UDP;
	_this->transport = NULL;

	_this->TimerHandle1 = -1;
	_this->TimerHandle2 = -1;
	_this->TimerHandle3 = -1;

	_this->bNoCB = FALSE;
	_this->bGarbage =FALSE;
	_this->rport = 0;
	_this->raddr = NULL;

	_this->mutex = cxMutexNew(CX_MUTEX_INTERTHREAD, 0);

	reqline = sipReqGetReqLine(msg);
	if ( reqline ) {
		_this->method = reqline->iMethod;
	} else {
		_this->method = UNKNOWN_METHOD;
	}

	/* rearrange route header and request uri as necessary */

	/* ljchuang 2005/03/02 */
	/* PrepareRoute ( msg, &(_this->bOriginalReq_PreparedForStrictRouter) ); */

	/* If already has a transaction id, make a copy 
	   else generate a new transaction id        */
	if ( txid) {
		_this->txid = strDup ( txid );
	} 
	else {
		strViaBranch = CalculateBranch ( msg );
			
		if ( strViaBranch ) {
			_this->txid = strDup ( strViaBranch );
		} 
		else {
			_this->txid = NULL;
		}
	}
	
	/* the transaction matching rule need a full via */
	FillVia ( msg, _this );

	if (INVITE == _this->method) { 
		TXTRACE0("[sipTxClientNew] new TX_CLIENT_INVITE transaction\n");
		_this->txtype = TX_CLIENT_INVITE;
	} else if (ACK == _this->method) {
		TXTRACE0("[sipTxClientNew] new TX_CLIENT_ACK transaction\n");
		_this->txtype = TX_CLIENT_ACK;
	} else {
		TXTRACE0("[sipTxClientNew] new TX_CLIENT_NON_INVITE transaction\n");
		_this->txtype = TX_CLIENT_NON_INVITE;
	}

	/*dxLstPutTail(TxList, _this);*/

	retcode = InsertTransaction ( TxDatabase, _this );

	if (RC_OK != retcode)
	{
		TXERR0("[sipTxClientNew] insert Tx into transaction database fail !!! \n");
		_sipTxFree(_this);
		_this = NULL;
		return NULL;
	}

	/*dxLstPutTail(ReqSendQ, _this);*/
	txPutDispatchEvtQ(_this, TX_DISPATCH_SENDREQ);

	TXTRACE1("[sipTxClientNew] Transaction internal id:(%d)\n", _this->InternalNum);
	TXTRACE1("[sipTxClientNew] Current transaction count: %d\n", ++txnum);

	return _this;
}


CCLAPI void sipTxFree(TxStruct _this)
{
	if (NULL == _this) return;

	_this->bNoCB = TRUE;
	TXTRACE1("[sipTxFree] TU free TX #%d\n", _this->InternalNum);

	if (_this->state == TX_TERMINATED)
		txPutDispatchEvtQ(_this, TX_DISPATCH_GARBAGE);
	return;
}


void _sipTxFree(TxStruct _this)
{
	if (NULL == _this) return;

	/* force sipTx free,even it's not terminated
	if (_this->state != TX_TERMINATED) {
		_this->bNoCB = TRUE;
		return;
	}
	*/

	switch (_this->txtype) {
	case TX_NON_ASSIGNED:
			TXTRACE0("[_sipTxFree] Free non assigned Transaction\n");
			break;
	case TX_SERVER_INVITE:
	case TX_SERVER_NON_INVITE:
	case TX_SERVER_ACK:
			TXTRACE1("[_sipTxFree] Free %s server Transaction\n", sipMethodTypeToName(_this->method) );
			break;
	case TX_CLIENT_INVITE:
	case TX_CLIENT_NON_INVITE: 
	case TX_CLIENT_ACK:
			TXTRACE1("[_sipTxFree] Free %s client Transaction\n", sipMethodTypeToName(_this->method) );
			break;

	case TX_CLIENT_2XX:
			TXTRACE0("[_sipTxFree] Free TX_CLIENT_2XX Transaction\n" );
			break;

	case TX_HOLDER:
			TXTRACE0("[_sipTxFree] Free Holder Transaction\n" );
			break;
	}

	TXTRACE1("[_sipTxFree] Transaction internal id:(%d)\n", _this->InternalNum);
	/*
	 * modified by tyhuang
	 *	if (_this->txtype != TX_HOLDER)
		{
			if (RC_ERROR == RemoveTransaction(TxDatabase, _this))
			{
				TXTRACE0("[_sipTxFree] Can't remove Tx from transaction database\n" );
			}
		}
	 */		
	if (RC_ERROR == RemoveTransaction(TxDatabase, _this))
	{
		TXTRACE0("[_sipTxFree] Can't remove Tx from transaction database\n" );
	}
	
	
	/*
	count = dxLstGetSize(TxList);
	
	for ( pos = 0; pos < count; pos++ ){
		tmpTx = dxLstPeek(TxList, pos);
		if ( tmpTx == _this) {
			dxLstGetAt(TxList, pos);
			break;
		}
	}
	*/

	if (NULL != _this->txid) free(_this->txid);
	if (NULL != _this->OriginalReq) sipReqFree(_this->OriginalReq);
	if (NULL != _this->Ack) sipReqFree(_this->Ack);
	if (NULL != _this->LatestRsp) sipRspFree(_this->LatestRsp);
	if (NULL != _this->profile) UserProfileFree(_this->profile);
	if (-1 != _this->TimerHandle1) sipTxDelTimer(_this->TimerHandle1);
	if (-1 != _this->TimerHandle2) sipTxDelTimer(_this->TimerHandle2);
	if (-1 != _this->TimerHandle3) sipTxDelTimer(_this->TimerHandle3);

	if (NULL != _this->transport) ReleaseTcpConnection ( _this->transport );
	if (NULL != _this->raddr) free(_this->raddr);

	if (NULL != _this->mutex) cxMutexFree(_this->mutex);

	/* add by tyhuang 2005/5/18; */
	memset(_this,0, sizeof(struct _TxStruct));
	free(_this);
	/*_this = NULL;*/
	txnum--;
	TXTRACE1("[_sipTxFree] Left transaction count: %d\n", txnum);
	return;
}


CCLAPI void sipTxSendRsp(TxStruct TxData, SipRsp msg)
{
	if (!TxData || !msg)
		return;

	txMutexLock(TxData->mutex);

	if (TxData->LatestRsp)
		sipRspFree(TxData->LatestRsp);
	
	TxData->LatestRsp = msg;
	
	/*dxLstPutTail(RspSendQ, TxData);*/
	txPutDispatchEvtQ(TxData, TX_DISPATCH_SENDRSP);

    /*
	switch (TxData->txtype) {
	case TX_SERVER_INVITE:
			Server_Invite_SM(TxData, TX_SEND_RSP, NULL);
			break;
	case TX_SERVER_NON_INVITE:
			Server_Non_Invite_SM(TxData, TX_SEND_RSP, NULL);						
			break;
	default:
			break;
	}
	*/

	txMutexUnlock(TxData->mutex);
}

CCLAPI void sipTxSetAck(TxStruct TxData, SipReq msg)
{
	SipVia *via;
	
	if (!TxData || !msg)
		return;

	txMutexLock(TxData->mutex);

	/*
	   We use the same Via header of the original INVITE
	   in the ACK for 200 OK
	 */
	via = (SipVia*)sipReqGetHdr(TxData->OriginalReq,Via);
	
	if (via != NULL)
		sipReqAddHdr(msg,Via,via);
	
	if (NULL != TxData->Ack)
		sipReqFree(TxData->Ack);
	TxData->Ack = msg;

	/* rearrange route header and request uri as necessary */
	/* ljchuang 2005/03/02 */
	/* PrepareRoute ( TxData->Ack, &(TxData->bAck_PreparedForStrictRouter) ); */

	txMutexUnlock(TxData->mutex);
}

#ifdef CCLSIP_ENABLE_TLS

void CBSipTLSEventHandle(SipTLS tls, SipTLSEvtType event, SipMsgType msgtype, void* msg)
{
	SipReqLine* reqline = NULL;
	SipRsp tmpRsp = NULL;
	SipMethodType method;
	TxStruct TxData = NULL;
	TxStruct tmpTx = NULL;
	UINT16 tmpPort = 0;
	TxTimer TimerData;
	SipTCP tcp = (SipTCP) tls;

#ifdef CCLSIP_SIPTX_ENABLE_TRACE
	char* tmpStr;
#endif

	TXTRACE0("[TLSEventHandle] Entering\n");
	switch (event) {
	case SIP_TLSEVT_SERVER_CLOSE:
		/* peer server close connection */
		TXWARN0("[TLSEventHandle] TLS connection closed by server\n");
		TxData = FindTxFromSipTCP( TxDatabase, tcp );

		if ( TxData )
			TxData->transport = NULL;

		if ( FALSE == CloseTcpConnection ( tcp ) ) {
			/* FALSE: can't not find existing TcpConnection */
			sipTLSFree( tls );
		}
		break;

	case SIP_TLSEVT_CLIENT_CLOSE:
		/* peer client close connection */
		TXWARN0("[TLSEventHandle] TLS connection closed by client\n");
		TxData = FindTxFromSipTCP( TxDatabase, tcp );

		if ( TxData )
			TxData->transport = NULL;

		if ( FALSE == CloseTcpConnection ( tcp ) ) {
			/* FALSE: can't not find existing TcpConnection */
			sipTLSFree( tls );
		}
		break;

	case SIP_TLSEVT_END:
		TXERR0("[TLSEventHandle] TLS connection attempt fail !!! \n");
		TxData = FindTxFromSipTCP( TxDatabase, tcp );

		if ( FALSE == CloseTcpConnection ( tcp ) ) {
			/* FALSE: can't not find existing TcpConnection */
			sipTLSFree( tls );
		}

		if ( TxData ) {
			TxData->transport = NULL;
			TimerData = (TxTimer)malloc(sizeof(struct _TxTimer));
			TimerData->data = TxData;
			TimerData->type = TX_TRANSPORT_ERROR;
			dxLstPutTail(TimerQ, TimerData);
		}
		break;

	case SIP_TLSEVT_CONNECTED:
		TXTRACE0("[TLSEventHandle] TLS connected\n");
		TxData = FindTxFromSipTCP( TxDatabase, tcp );

		SetPTcpConnected( TxData->transport );
		
		TimerData = (TxTimer)malloc(sizeof(struct _TxTimer));
		TimerData->data = TxData;
		TimerData->type = TX_TCP_CONNECTED;
	
		dxLstPutTail(TimerQ, TimerData);
		break;

	case SIP_TLSEVT_DATA:
		switch (msgtype) {
		case SIP_MSG_UNKNOWN:
		case SIP_MSG_REQ:
			if (msg != NULL) {
#ifdef CCLSIP_SIPTX_ENABLE_TRACE
				tmpStr = sipReqPrint((SipReq)msg);
				sipTLSGetRport(tls, &tmpPort);
				TXTRACE3("[TLSEventHandle] Request received from %s:%d \r\n\n%s\n",
						sipTLSGetRaddr(tls), tmpPort, tmpStr);
#endif
												
				/* Get the method of the incoming request */
				reqline = sipReqGetReqLine(msg);
				method = reqline->iMethod;

				TxData = FindMatchingTransactionReq(TxDatabase, (SipReq)msg, TRUE);
				
				if ( method == CANCEL ) {
					/* If can't find CANCEL Candidate, reject with a 481 */
					if ( TxData == NULL ) {
						tmpRsp = NewRspFromReq( (SipReq)msg, 481 );
					} else { /* Found existing CANCEL transaction or CANCEL Candidate */
						/* Found CANCEL Candidate, reset TxData */
						if ( TxData->method == INVITE )
						{
							tmpTx = TxData;
							TxData = FindMatchingTransactionReq(TxDatabase, (SipReq)msg, FALSE);
						
							if (TxData == NULL) /* First CANCEL */
							{
								txMutexLock(tmpTx->mutex);
								g_txEvtCB(tmpTx, TX_CANCEL_RECEIVED);
								txMutexUnlock(tmpTx->mutex);
							}
						}
					}
				} 
				
				if ( TxData ) {
					TXTRACE0("[TLSEventHandle] Found an existing transaction \n");
					
					if ((TxData->txtype == TX_CLIENT_INVITE) || 
						(TxData->txtype == TX_CLIENT_NON_INVITE) )
					{
						TXTRACE0("[TLSEventHandle] WARNING: transaction type mismatch, discard message \n");
						sipReqFree((SipReq)msg);
						msg = NULL;
						break;
					}

					txMutexLock(TxData->mutex);
					TxData->txptype = TXPTLS;
					
					if (!TxData->raddr)
					{
						TxData->raddr = strDup( sipTLSGetRaddr(tls) );
						sipTLSGetRport(tls, &(TxData->rport) );
					}

					if ( TxData->transport ) {
						/* if this is a new connection, we shall release the old one and adapt this new one */
						if ( GetUnderlyingTcp(TxData->transport) != tcp ) {
							ReleaseTcpConnection ( TxData->transport );
							sipTLSGetRport(tls, &tmpPort);
							TxData->transport = AddRefTcpConnection ( tcp, inet_addr(sipTLSGetRaddr (tls)), tmpPort, TRUE );
							SetPTcpConnected( TxData->transport );
						}
					} else {
						sipTLSGetRport(tls, &tmpPort);
						TxData->transport = AddRefTcpConnection ( tcp, inet_addr(sipTLSGetRaddr (tls)), tmpPort, TRUE );
					}
					txMutexUnlock(TxData->mutex);

				} else { /* NULL == TxData */
					if (ACK != method) {
						TXTRACE0("[TLSEventHandle] Create a new transaction \n");
						TxData = sipTxServerNew((SipReq)msg, (void*)tcp, TXPTLS);
						sipTLSGetRport(tls, &tmpPort);
						TxData->transport = AddRefTcpConnection ( tcp, inet_addr(sipTLSGetRaddr (tls)), tmpPort, TRUE );
						SetPTcpConnected( TxData->transport );
					} else { /* ACK == method */
						if (!g_bMatchACK) {
							/* ACK for nobody (200 OK?), should pass to TU directly */
							TXTRACE0("[TLSEventHandle] Create a new transaction (ACK for 200 OK) \n");
							TxData = sipTxServerNew((SipReq)msg, (void*)tcp, TXPTLS);
							sipTLSGetRport(tls, &tmpPort);
							TxData->transport = AddRefTcpConnection ( tcp, inet_addr(sipTLSGetRaddr (tls)), tmpPort, TRUE );
							SetPTcpConnected( TxData->transport );
						}
					}
					if (CANCEL == method) {
						tmpRsp = NewRspFromReq( (SipReq)msg, 200 );
					}
				}
				
				if ( TxData ) {
					switch (TxData->txtype) {
					case TX_SERVER_INVITE:
						if (ACK == method) {
							Server_Invite_SM(TxData, TX_ACK_RECEIVED, (SipReq)msg);
						} else {
							Server_Invite_SM(TxData, TX_REQ_RECEIVED, (SipReq)msg);
						}
						break;

					case TX_SERVER_NON_INVITE:
						if (CANCEL == method && tmpRsp) { /* CANCEL which can't match any INVITE */
							TxData->bNoCB = TRUE;
							Server_Non_Invite_SM(TxData, TX_REQ_RECEIVED, (SipReq)msg);
							sipTxSendRsp(TxData, tmpRsp);
						} else { /* CANCEL, BYE, OPTIONS */
							Server_Non_Invite_SM(TxData, TX_REQ_RECEIVED, (SipReq)msg);
						}
						break;

					case TX_SERVER_ACK: /* ACK for 200 OK */
						TU_txEvtCB(TxData, TX_REQ_RECEIVED);
						break;

					case TX_CLIENT_2XX:
						if (ACK == method) {
							Client_2XX_SM(TxData, TX_ACK_RECEIVED, (SipReq)msg);
						} else {
							Client_2XX_SM(TxData, TX_REQ_RECEIVED, (SipReq)msg);
						}
						break;

					case TX_HOLDER: /* Problem REQUEST */
						TU_txEvtCB(TxData, TX_BADREQ_RECEIVED);
						break;
						
					default:
						break;
					}
				}
			}
			break;
		
		case SIP_MSG_RSP:
			if (msg != NULL) {
				/* mac : response is not allowed to change the transport connection in a given 
				transaction, this just doesn't make sense. */ 

#ifdef CCLSIP_SIPTX_ENABLE_TRACE
				tmpStr = sipRspPrint((SipRsp)msg);
				sipTLSGetRport(tls, &tmpPort);
				TXTRACE3("[TLSEventHandle] Response received from %s:%d \r\n\n%s\n",
						sipTLSGetRaddr(tls), tmpPort, tmpStr);
#endif
								
				TxData = FindMatchingTransactionRsp(TxDatabase, (SipRsp)msg);
				
				if (NULL != TxData) {
					TXTRACE0("[TLSEventHandle] Found an existing transaction \n");

					if ((TxData->txtype != TX_CLIENT_INVITE) && (TxData->txtype != TX_CLIENT_NON_INVITE) )
					{
						TXTRACE0("[TLSEventHandle] WARNING: transaction type mismatch, discard message \n");
						sipRspFree((SipRsp)msg);
						msg = NULL;
						break;
					}

					if (!TxData->raddr)
					{
						txMutexLock(TxData->mutex);
						TxData->raddr = strDup( sipTLSGetRaddr(tls) );
						sipTLSGetRport(tls, &(TxData->rport));
						txMutexUnlock(TxData->mutex);
					}
					
					switch (TxData->txtype) {
					case TX_CLIENT_INVITE:
						Client_Invite_SM(TxData, TX_RSP_RECEIVED, (SipRsp)msg);
						break;
					case TX_CLIENT_NON_INVITE:
						Client_Non_Invite_SM(TxData, TX_RSP_RECEIVED, (SipRsp)msg);
						break;
					default:
						break;
					}
				} else {
					/* no match Tx, what now? */
					TXTRACE0("[TLSEventHandle] Response can't find matching transaction \n");
				
					/* try find a TX_CLIENT_ACK */
					TxData = FindMatchingAck(TxDatabase, (SipRsp)msg);

					if (TxData)	{
						Client_Ack_SM(TxData, TX_RSP_RECEIVED, (SipRsp)msg);
					} else {

						/* new a holder Tx and info TU */
						TxData = sipTxHolderNew(msg, NULL);
						sipTLSGetRport(tls, &tmpPort);
						TxData->transport = AddRefTcpConnection ( tcp, inet_addr(sipTLSGetRaddr (tls)), tmpPort, TRUE );
						
						TU_txEvtCB(TxData, TX_NOMATCH_RSP_RECEIVED);
					}
				}
			}
			break;
		}
		break;
	
	default:
		break;
	}
	TXTRACE0("[TLSEventHandle] Leaving\n");

}

#endif

void CBSipTCPEventHandle(SipTCP tcp, SipTCPEvtType event, SipMsgType msgtype, void* msg)
{
	SipReqLine* reqline = NULL;
	SipRsp tmpRsp = NULL;
	SipMethodType method;
	TxStruct TxData = NULL;
	TxStruct tmpTx = NULL;
	UINT16 tmpPort = 0;
	TxTimer TimerData;

	#ifdef CCLSIP_SIPTX_ENABLE_TRACE
	char* tmpStr;
	#endif

	TXTRACE0("[TCPEventHandle] Entering\n");
	switch (event) {
	case SIP_TCPEVT_SERVER_CLOSE:
		/* peer server close connection */
		TXWARN0("[TCPEventHandle] TCP connection closed by server\n");
		TxData = FindTxFromSipTCP( TxDatabase, tcp );

		if ( TxData )
			TxData->transport = NULL;

		if ( FALSE == CloseTcpConnection ( tcp ) ) {
			/* FALSE: can't not find existing TcpConnection */
			sipTCPFree ( tcp );
		}
		break;

	case SIP_TCPEVT_CLIENT_CLOSE:
		/* peer client close connection */
		TXWARN0("[TCPEventHandle] TCP connection closed by client\n");
		TxData = FindTxFromSipTCP( TxDatabase, tcp );

		if ( TxData )
			TxData->transport = NULL;

		if ( FALSE == CloseTcpConnection ( tcp ) ) {
			/* FALSE: can't not find existing TcpConnection */
			sipTCPFree ( tcp );
		}
		break;

	case SIP_TCPEVT_END:
		TXERR0("[TCPEventHandle] TCP connection attempt fail !!! \n");
		TxData = FindTxFromSipTCP( TxDatabase, tcp );

		if ( FALSE == CloseTcpConnection ( tcp ) ) {
			/* FALSE: can't not find existing TcpConnection */
			sipTCPFree ( tcp );
		}

		if ( TxData ) {
			TxData->transport = NULL;
			TimerData = (TxTimer)malloc(sizeof(struct _TxTimer));
			TimerData->data = TxData;
			TimerData->type = TX_TRANSPORT_ERROR;
			dxLstPutTail(TimerQ, TimerData);
		}
		break;

	case SIP_TCPEVT_CONNECTED:
		TXTRACE0("[TCPEventHandle] TCP connected\n");
		TxData = FindTxFromSipTCP( TxDatabase, tcp );

		if (TxData) {
			SetPTcpConnected( TxData->transport );
		
			TimerData = (TxTimer)malloc(sizeof(struct _TxTimer));
			TimerData->data = TxData;
			TimerData->type = TX_TCP_CONNECTED;
	
			dxLstPutTail(TimerQ, TimerData);
		}
		break;

	case SIP_TCPEVT_DATA:
		switch (msgtype) {
		case SIP_MSG_UNKNOWN:
		case SIP_MSG_REQ:
			if (msg != NULL) {
#ifdef CCLSIP_SIPTX_ENABLE_TRACE
				tmpStr = sipReqPrint((SipReq)msg);
				sipTCPGetRport(tcp, &tmpPort);
				TXTRACE3("[TCPEventHandle] Request received from %s:%d \r\n\n%s\n",
						sipTCPGetRaddr(tcp), tmpPort, tmpStr);
#endif
												
				/* Get the method of the incoming request */
				reqline = sipReqGetReqLine(msg);
				method = reqline->iMethod;

				TxData = FindMatchingTransactionReq(TxDatabase, (SipReq)msg, TRUE);
				
				if ( method == CANCEL ) {
					/* If can't find CANCEL Candidate, reject with a 481 */
					if ( TxData == NULL ) {
						tmpRsp = NewRspFromReq( (SipReq)msg, 481 );
					} else { /* Found existing CANCEL transaction or CANCEL Candidate */
						/* Found CANCEL Candidate, reset TxData */
						if ( TxData->method == INVITE )
						{
							tmpTx = TxData;
							TxData = FindMatchingTransactionReq(TxDatabase, (SipReq)msg, FALSE);
						
							if (TxData == NULL) /* First CANCEL */
							{
								txMutexLock(tmpTx->mutex);
								g_txEvtCB(tmpTx, TX_CANCEL_RECEIVED);
								txMutexUnlock(tmpTx->mutex);
							}
						}
					}
				} 
				
				if ( TxData ) {
					TXTRACE0("[TCPEventHandle] Found an existing transaction \n");
					
					if ((TxData->txtype == TX_CLIENT_INVITE) || 
						(TxData->txtype == TX_CLIENT_NON_INVITE) )
					{
						TXTRACE0("[TCPEventHandle] WARNING: transaction type mismatch, discard message \n");
						sipReqFree((SipReq)msg);
						msg = NULL;
						break;
					}

					txMutexLock(TxData->mutex);
					TxData->txptype = TCP;
					
					if (!TxData->raddr)
					{
						TxData->raddr = strDup( sipTCPGetRaddr(tcp) );
						sipTCPGetRport(tcp, &(TxData->rport) );
					}

					if ( TxData->transport ) {
						/* if this is a new connection, we shall release the old one and adapt this new one */
						if ( GetUnderlyingTcp(TxData->transport) != tcp ) {
							ReleaseTcpConnection ( TxData->transport );
							sipTCPGetRport(tcp, &tmpPort);
							TxData->transport = AddRefTcpConnection ( tcp, inet_addr(sipTCPGetRaddr (tcp)), tmpPort, FALSE );
							SetPTcpConnected( TxData->transport );
						}
					} else {
						sipTCPGetRport(tcp, &tmpPort);
						TxData->transport = AddRefTcpConnection ( tcp, inet_addr(sipTCPGetRaddr (tcp)), tmpPort, FALSE );
					}
					txMutexUnlock(TxData->mutex);

				} else { /* NULL == TxData */
					if (ACK != method) {
						TXTRACE0("[TCPEventHandle] Create a new transaction \n");
						TxData = sipTxServerNew((SipReq)msg, (void*)tcp, TCP);
						sipTCPGetRport(tcp, &tmpPort);
						TxData->transport = AddRefTcpConnection ( tcp, inet_addr(sipTCPGetRaddr (tcp)), tmpPort, FALSE );
						SetPTcpConnected( TxData->transport );
					} else { /* ACK == method */
						if (!g_bMatchACK) {
							/* ACK for nobody (200 OK?), should pass to TU directly */
							TXTRACE0("[TCPEventHandle] Create a new transaction (ACK for 200 OK) \n");
							TxData = sipTxServerNew((SipReq)msg, (void*)tcp, TCP);
							sipTCPGetRport(tcp, &tmpPort);
							TxData->transport = AddRefTcpConnection ( tcp, inet_addr(sipTCPGetRaddr (tcp)), tmpPort, FALSE );
							SetPTcpConnected( TxData->transport );
						}
					}
					if (CANCEL == method) {
						tmpRsp = NewRspFromReq( (SipReq)msg, 200 );
					}
				}
				
				if ( TxData ) {
					switch (TxData->txtype) {
					case TX_SERVER_INVITE:
						if (ACK == method) {
							Server_Invite_SM(TxData, TX_ACK_RECEIVED, (SipReq)msg);
						} else {
							Server_Invite_SM(TxData, TX_REQ_RECEIVED, (SipReq)msg);
						}
						break;

					case TX_SERVER_NON_INVITE:
						if (CANCEL == method && tmpRsp) { /* CANCEL which can't match any INVITE */
							TxData->bNoCB = TRUE;
							Server_Non_Invite_SM(TxData, TX_REQ_RECEIVED, (SipReq)msg);
							sipTxSendRsp(TxData, tmpRsp);
						} else { /* CANCEL, BYE, OPTIONS */
							Server_Non_Invite_SM(TxData, TX_REQ_RECEIVED, (SipReq)msg);
						}
						break;

					case TX_SERVER_ACK: /* ACK for 200 OK */
						TU_txEvtCB(TxData, TX_REQ_RECEIVED);
						break;

					case TX_CLIENT_2XX:
						if (ACK == method) {
							Client_2XX_SM(TxData, TX_ACK_RECEIVED, (SipReq)msg);
						} else {
							Client_2XX_SM(TxData, TX_REQ_RECEIVED, (SipReq)msg);
						}
						break;

					case TX_HOLDER: /* Problem REQUEST */
						TU_txEvtCB(TxData, TX_BADREQ_RECEIVED);
						break;
						
					default:
						break;
					}
				}
			}
			break;
		
		case SIP_MSG_RSP:
			if (msg != NULL) {
				/* mac : response is not allowed to change the transport connection in a given 
				transaction, this just doesn't make sense. */ 

#ifdef CCLSIP_SIPTX_ENABLE_TRACE
				tmpStr = sipRspPrint((SipRsp)msg);
				sipTCPGetRport(tcp, &tmpPort);
				TXTRACE3("[TCPEventHandle] Response received from %s:%d \r\n\n%s\n",
						sipTCPGetRaddr(tcp), tmpPort, tmpStr);
#endif
								
				TxData = FindMatchingTransactionRsp(TxDatabase, (SipRsp)msg);
				
				if (NULL != TxData) {
					TXTRACE0("[TCPEventHandle] Found an existing transaction \n");

					if ((TxData->txtype != TX_CLIENT_INVITE) && (TxData->txtype != TX_CLIENT_NON_INVITE) )
					{
						TXTRACE0("[TCPEventHandle] WARNING: transaction type mismatch, discard message \n");
						sipRspFree((SipRsp)msg);
						msg = NULL;
						break;
					}

					if (!TxData->raddr)
					{
						txMutexLock(TxData->mutex);
						TxData->raddr = strDup( sipTCPGetRaddr(tcp) );
						sipTCPGetRport(tcp, &(TxData->rport));
						txMutexUnlock(TxData->mutex);
					}
					
					switch (TxData->txtype) {
					case TX_CLIENT_INVITE:
						Client_Invite_SM(TxData, TX_RSP_RECEIVED, (SipRsp)msg);
						break;
					case TX_CLIENT_NON_INVITE:
						Client_Non_Invite_SM(TxData, TX_RSP_RECEIVED, (SipRsp)msg);
						break;
					default:
						break;
					}
				} else {
					/* no match Tx, what now? */
					TXTRACE0("[TCPEventHandle] Response can't find matching transaction \n");
				
					/* try find a TX_CLIENT_ACK */
					TxData = FindMatchingAck(TxDatabase, (SipRsp)msg);

					if (TxData)	{
						Client_Ack_SM(TxData, TX_RSP_RECEIVED, (SipRsp)msg);
					} else {

						/* new a holder Tx and info TU */
						TxData = sipTxHolderNew(msg, NULL);
						sipTCPGetRport(tcp, &tmpPort);
						TxData->transport = AddRefTcpConnection ( tcp, inet_addr(sipTCPGetRaddr (tcp)), tmpPort, FALSE );
						
						TU_txEvtCB(TxData, TX_NOMATCH_RSP_RECEIVED);
					}
				}
			}
			break;
		}
		break;
	
	default:
		break;
	}
	TXTRACE0("[TCPEventHandle] Leaving\n");

}


void CBSipUDPEventHandle(SipUDP udp, SipUDPEvtType event, SipMsgType msgtype,void* msg)
{
	SipReqLine* reqline = NULL;
	SipMethodType method;
	SipRsp tmpRsp = NULL;
	TxStruct TxData = NULL;
	TxStruct tmpTx = NULL;
	UINT16 tmpPort = 0;

#ifdef CCLSIP_SIPTX_ENABLE_TRACE
	char* tmpStr;
#endif

	TXTRACE0("[UDPEventHandle] Entering\n");

	switch (event) {
	case SIP_UDPEVT_DATA:
		switch (msgtype) {
		case SIP_MSG_UNKNOWN:
		case SIP_MSG_REQ:
			if (msg != NULL) {
#ifdef CCLSIP_SIPTX_ENABLE_TRACE
				tmpStr = sipReqPrint((SipReq)msg);
				sipUDPGetRport(udp, &tmpPort);
				TXTRACE3("[UDPEventHandle] Request received from %s:%d \r\n\n%s\n",
						sipUDPGetRaddr(udp), tmpPort, tmpStr);
#endif
								
				/* Get the method of the incoming request */
				reqline = sipReqGetReqLine(msg);
				method = reqline->iMethod;

				TxData = FindMatchingTransactionReq(TxDatabase, (SipReq)msg, TRUE);

				if ( method == CANCEL ) {
					/* If can't find CANCEL Candidate, reject with a 481 */
					if ( TxData == NULL ) {
						tmpRsp = NewRspFromReq( (SipReq)msg, 481 );
					} else { /* Found existing CANCEL transaction or CANCEL Candidate */
						/* Found CANCEL Candidate, reset TxData */
						if ( TxData->method == INVITE )
						{
							tmpTx = TxData;
							TxData = FindMatchingTransactionReq(TxDatabase, (SipReq)msg, FALSE);
						
							if (TxData == NULL) /* First CANCEL */
							{
								txMutexLock(tmpTx->mutex);
								g_txEvtCB(tmpTx, TX_CANCEL_RECEIVED);
								txMutexUnlock(tmpTx->mutex);
							}
						}
					}
				} 
				
				if (TxData) {
					TXTRACE0("[UDPEventHandle] Found an existing transaction \n");
					
					if ((TxData->txtype == TX_CLIENT_INVITE) || (TxData->txtype == TX_CLIENT_NON_INVITE) )
					{
						TXTRACE0("[UDPEventHandle] WARNING: transaction type mismatch, discard message \n");
						sipReqFree((SipReq)msg);
						msg = NULL;
						break;
					}

					/* it MAY happen that the same request comes from first tcp, then from udp */
					if ( TxData->transport ) {
						txMutexLock(TxData->mutex);
						ReleaseTcpConnection ( TxData->transport );
						TxData->transport = NULL;
						TxData->txptype = UDP;
						txMutexUnlock(TxData->mutex);
					}
					
				} else { /* NULL == TxData */
					if (ACK != method) {
						TXTRACE0("[UDPEventHandle] Create a new transaction \n");
						/* mac : udp should keep transport to NULL */
						/* TxData = sipTxServerNew((SipReq)msg, (void*)udp, UDP); */
						TxData = sipTxServerNew((SipReq)msg, udp, UDP);
					} else { /* ACK == method */
						if (!g_bMatchACK) {
							/* ACK for nobody (200 OK?), should pass to TU directly */
							TXTRACE0("[UDPEventHandle] Create a new transaction (ACK for 200 OK) \n");
							
							/* mac : udp should keep transport to NULL */
							/* TxData = sipTxServerNew((SipReq)msg, (void*)udp, UDP); */
							TxData = sipTxServerNew((SipReq)msg, udp, UDP);
						}
					}
					if (!tmpRsp && (CANCEL == method)) {
						tmpRsp = NewRspFromReq( (SipReq)msg, 200 );
					}
				}
				
				if (TxData) {
					/* mac : udp should not use this pointer */
					/* TxData->transport = (void*)udp; */

					if (!TxData->raddr)
					{
						txMutexLock(TxData->mutex);
						TxData->raddr = strDup( sipUDPGetRaddr(udp) );
						sipUDPGetRport(udp, &(TxData->rport));
						txMutexUnlock(TxData->mutex);
					}
										
					switch (TxData->txtype) {
					case TX_SERVER_INVITE:
						if (ACK == method) {
							Server_Invite_SM(TxData, TX_ACK_RECEIVED, (SipReq)msg);
						} else {
							Server_Invite_SM(TxData, TX_REQ_RECEIVED, (SipReq)msg);
						}
						break;

					case TX_SERVER_NON_INVITE:
						if (CANCEL == method && tmpRsp) { /* CANCEL which can't match any INVITE */
							TxData->bNoCB = TRUE;
							Server_Non_Invite_SM(TxData, TX_REQ_RECEIVED, (SipReq)msg);
							sipTxSendRsp(TxData, tmpRsp);
						} else { /* CANCEL, BYE, OPTIONS */
							Server_Non_Invite_SM(TxData, TX_REQ_RECEIVED, (SipReq)msg);
						}
						break;

					case TX_SERVER_ACK: /* ACK for 200 OK */
						TU_txEvtCB(TxData, TX_REQ_RECEIVED);
						break;

					case TX_CLIENT_2XX:
						if (ACK == method) {
							Client_2XX_SM(TxData, TX_ACK_RECEIVED, (SipReq)msg);
						} else {
							Client_2XX_SM(TxData, TX_REQ_RECEIVED, (SipReq)msg);
						}
						break;

					case TX_HOLDER: /* Problem REQUEST */
						TU_txEvtCB(TxData, TX_BADREQ_RECEIVED);
						break;
						
					default:
						break;
					}
				}
			}
			break;
		
		case SIP_MSG_RSP:
			if (msg != NULL) {
#ifdef CCLSIP_SIPTX_ENABLE_TRACE
				tmpStr = sipRspPrint((SipRsp)msg);
				sipUDPGetRport(udp, &tmpPort);
				TXTRACE3("[UDPEventHandle] Response received from %s:%d \r\n\n%s\n",
						sipUDPGetRaddr(udp), tmpPort, tmpStr);
#endif
				
				TxData = FindMatchingTransactionRsp(TxDatabase, (SipRsp)msg);
				
				if (NULL != TxData) {
					TXTRACE0("[UDPEventHandle] Found an existing transaction \n");

					if ((TxData->txtype != TX_CLIENT_INVITE) && 
						(TxData->txtype != TX_CLIENT_NON_INVITE) )
					{
						TXTRACE0("[UDPEventHandle] WARNING: transaction type mismatch, discard message \n");
						sipRspFree((SipRsp)msg);
						msg = NULL;
						break;
					}

					if (!TxData->raddr)
					{
						TxData->raddr = strDup( sipUDPGetRaddr(udp) );
						sipUDPGetRport(udp, &(TxData->rport));
					}
					
					switch (TxData->txtype) {
					case TX_CLIENT_INVITE:
						Client_Invite_SM(TxData, TX_RSP_RECEIVED, (SipRsp)msg);
						break;
					case TX_CLIENT_NON_INVITE:
						Client_Non_Invite_SM(TxData, TX_RSP_RECEIVED, (SipRsp)msg);
						break;
					default:
						break;
					}
				} else {
					/* no match Tx, what now? */
					TXTRACE0("[UDPEventHandle] WARNING: Response can't find matching transaction, handover to TU \n");

					/* try find a TX_CLIENT_ACK */
					TxData = FindMatchingAck(TxDatabase, (SipRsp)msg);

					if (TxData) {
						Client_Ack_SM(TxData, TX_RSP_RECEIVED, (SipRsp)msg);
					} else {

						/* new a holder Tx and info TU */
						TxData = sipTxHolderNew(msg, NULL);
						TU_txEvtCB(TxData, TX_NOMATCH_RSP_RECEIVED);
					}
				}
			}
			break;

		}
		break;
	
	default:
		break;
	}
	TXTRACE0("[UDPEventHandle] Leaving\n");
}


CCLAPI RCODE
_sipTxLibInit(SipTxEvtCB txEvtCB, SipConfigData ConfigStr, BOOL bMatchACK, unsigned short MaxTxCap, const char* STUNserver)
{
	RCODE retcode;

	if ( 0 >= MaxTxCap )
		return RC_ERROR;

	/*
	ReqSendQ = dxLstNew(DX_LST_POINTER);
	RspSendQ = dxLstNew(DX_LST_POINTER);
	TxList = dxLstNew(DX_LST_POINTER);
	GarbageQ = dxLstNew(DX_LST_POINTER);
	*/

	DispatchEvtQ = dxLstNew(DX_LST_POINTER);
	
	TimerQ = dxLstNew(DX_LST_POINTER);
	
	g_bMatchACK = bMatchACK;
	g_nMaxTxCount = MaxTxCap;

	TimerTable = (unsigned short *) calloc( g_nMaxTxCount, sizeof(unsigned short) );
	TimerMutex = cxMutexNew(CX_MUTEX_INTERTHREAD, 0);

	CommonTimerTable = (CommonTimer*) calloc( MAXTIMERCOUNT, sizeof(struct _CommonTimer) );
	
	TxDatabase = CreateTransactionDB ( MaxTxCap, TRUE );

	if (NULL != txEvtCB)
		TU_txEvtCB = txEvtCB;

	if ( ConfigStr.laddr )
		strLocalAddress = strDup ( ConfigStr.laddr );

	usLocalPort = ConfigStr.tcp_port;

#ifdef CCLSIP_ENABLE_STUN
	if ( NULL != STUNserver )
	{
		strSTUNserver = strDup( STUNserver );
		stunGetExtAddr( strSTUNserver, strExtAddress );
		usExtPort = stunGetExtPort( strSTUNserver, usLocalPort );
	} else {
		strcpy( strExtAddress, strLocalAddress );
		usExtPort = usLocalPort;
	}
#endif

#ifndef CCLSIP_ENABLE_TLS

	retcode = sipLibInit(SIP_TRANS_UDP | SIP_TRANS_TCP_CLIENT | SIP_TRANS_TCP_SERVER,
		   CBSipTCPEventHandle, 
		   CBSipUDPEventHandle, 
		   ConfigStr);

#else
		   
	retcode = sipLibInitWithTLS(SIP_TRANS_UDP | SIP_TRANS_TCP_CLIENT | SIP_TRANS_TCP_SERVER | SIP_TRANS_TLS_SERVER | SIP_TRANS_TLS_CLIENT,
		   CBSipTCPEventHandle, 
		   CBSipUDPEventHandle, 
		   CBSipTLSEventHandle,
		   ConfigStr.tcp_port + 1,
		   ConfigStr);

#endif

#ifndef _WIN32_WCE 
	if ( RC_OK == retcode )
		g_nTimerHandle = sipTimerSet(TIMERTICK, sipTxTimerManager, NULL);
	else{
		g_nTimerHandle =-1;
		/* clean alloc memory */
		sipTxLibEnd();
	}
#else
	gTxTimerThreadHandle = CreateThread(NULL, 0, 
			(unsigned long(__stdcall*)(void*))sipTxTimerThread, 
			NULL, 0, NULL);
	gTxTimerEventHandle = CreateEvent( NULL, FALSE, FALSE, NULL);
#endif

	return retcode;
}

CCLAPI RCODE
sipTxLibInit(SipTxEvtCB txEvtCB, SipConfigData ConfigStr, BOOL bMatchACK, unsigned short MaxTxCap)
{
	RCODE retcode;
	DxLst AddrPortLst = NULL;
	AddrPort tmpAddrPort = NULL;

	/* marked by tyhuang 2006/12/5
	 * due to AddrPortLst is used for multi-address
	 *
	tmpAddrPort = AddrPortNew( ConfigStr.laddr, ConfigStr.tcp_port );
	 
	if (!tmpAddrPort)
	   return RC_ERROR;

	AddrPortLst = dxLstNew( DX_LST_POINTER );
	dxLstPutTail( AddrPortLst, tmpAddrPort );
	*/
	retcode = _sipTxLibInit(txEvtCB, ConfigStr, bMatchACK, MaxTxCap, NULL);

	if (RC_OK == retcode) {
		g_sipUDP = sipUDPNew(strLocalAddress,usLocalPort);
		InitializeTransport ( AddrPortLst );
		TXTRACE0("[sipTxLibInit] sipTxLib initialized\n");
	} else {
		TXERR0("[sipTxLibInit] sipTxLib initializ fail !!\n");
	}

	dxLstFree( AddrPortLst, AddrPortFree );

	return retcode;
}

/* Initialize the Tx Lib */
RCODE sipTxLibInitEx(SipTxEvtCB txEvtCB, SipConfigData ConfigStr, DxLst AddrPortLst, BOOL bMatchACK, unsigned short MaxTxCap, const char* STUNserver)
{
	RCODE retcode;
	char localhostname[64];

	retcode = _sipTxLibInit(txEvtCB, ConfigStr, bMatchACK, MaxTxCap, STUNserver);

	if (RC_OK == retcode) {
		g_sipUDP = sipUDPNew (strLocalAddress, usLocalPort);
		InitializeTransport ( AddrPortLst );
		TXTRACE0("[sipTxLibInit] sipTxLib initialized \n");
	} else {
		TXERR0("[sipTxLibInit] sipTxLib initializ fail !!! \n");
	}

	if ( 0 != gethostname( localhostname, 64) )
		TXERR0("[sipTxLibInit] get local hostname fail \n");
	else
		TXTRACE1("[sipTxLibInit] local hostname = %s \n", localhostname);

	return retcode;
}


CCLAPI RCODE sipTxLibEnd(void)
{

#ifdef _WIN32_WCE
	SetEvent( gTxTimerEventHandle );
#endif

	sipTxEvtDispatch(100);
	
	/*
	count = dxLstGetSize(TxList);
	TXTRACE1("[sipTxLibEnd] Current TxList size = %d\n", count);
	for ( pos = count-1; pos >= 0; pos-- ){
		tmpTx = dxLstPeek(TxList, pos);
		tmpTx->state = TX_TERMINATED;
		_sipTxFree(tmpTx);
	}
	dxLstFree(TxList, NULL);
	TxList = NULL;
	*/
/*
	dxLstFree(ReqSendQ, NULL);
	ReqSendQ = NULL;

	dxLstFree(RspSendQ, NULL);
	RspSendQ = NULL;

	dxLstFree(GarbageQ, NULL);
	GarbageQ = NULL;
*/
	
	dxLstFree(TimerQ, free);
	TimerQ = NULL;

	dxLstFree(DispatchEvtQ, free);
	DispatchEvtQ = NULL;

	DestroyTransactionDB(&TxDatabase);

	cxMutexLock(TimerMutex);
	if(g_nTimerHandle>0){
		sipTimerDel(g_nTimerHandle);
		g_nTimerHandle=-1;
	}
	cxMutexUnlock(TimerMutex);

	if (NULL != TimerMutex){ 
		cxMutexFree(TimerMutex);
		TimerMutex = NULL;
	}

	if (TimerTable) {
		free(TimerTable);
		TimerTable = NULL;
	}

	if (CommonTimerTable) {
		free(CommonTimerTable);
		CommonTimerTable = NULL;
	}


	if (NULL != g_sipUDP) {
		sipUDPFree(g_sipUDP);
		g_sipUDP = NULL;
	}


	FinalizeTransport();

	if (strLocalAddress != NULL){
		free(strLocalAddress);
		strLocalAddress = NULL;
	}

#ifdef CCLSIP_ENABLE_STUN
	if (strSTUNserver) {
		free( strSTUNserver );
		strSTUNserver = NULL;
	}
#endif

	
	TXTRACE1("[sipTxLibEnd] Left transaction count: %d\n", txnum);

	sipLibClean();
	return RC_OK;
}


CCLAPI RCODE sipTxEvtDispatch(int timeout)
{
	TxStruct TxData = NULL;
	TxTimer TimerData = NULL;
	TxDispatchEvtObj evtobj = NULL;
	TXDISPATCHEVTTYPE evttype;
	RCODE retcode;

	retcode = sipEvtDispatch(timeout);
	if (retcode <= 0)
		retcode = RC_ERROR;
	else
		retcode = RC_OK;

	while ( DispatchEvtQ && (dxLstGetSize(DispatchEvtQ) > 0 ) ) {
		evtobj = dxLstGetHead( DispatchEvtQ );
		if(!evtobj)
			continue;

		TxData = evtobj->txdata;
		evttype = evtobj->evt;

		switch (evttype) {
		
		case TX_DISPATCH_SENDREQ:
			switch (TxData->txtype) {
			case TX_CLIENT_INVITE:
				Client_Invite_SM(TxData, TX_SEND_REQ, NULL);
				break;
			case TX_CLIENT_NON_INVITE:
				Client_Non_Invite_SM(TxData, TX_SEND_REQ, NULL);						
				break;
			case TX_CLIENT_ACK:
				Client_Ack_SM(TxData, TX_SEND_REQ, NULL);
				break;
			default:
				break;
			}
			break;
		
		case TX_DISPATCH_SENDRSP:
			switch (TxData->txtype) {
			case TX_SERVER_INVITE:
				Server_Invite_SM(TxData, TX_SEND_RSP, NULL);
				break;
			case TX_SERVER_NON_INVITE:
				Server_Non_Invite_SM(TxData, TX_SEND_RSP, NULL);						
				break;
			default:
				TXWARN0("[sipTxEvtDispatch] Transaction type other than TX_SERVER_INVITE or TX_SERVER_NON_INVITE shouldn't be here! \n");
				break;
			}
			break;
		
		case TX_DISPATCH_GARBAGE:
			
			TXTRACE1("[sipTxEvtDispatch] Collect garbage transaction txtype = %s \n",sipTxType2Phase(TxData->txtype) );
			_sipTxFree(TxData);
			TxData = NULL;
			break;
		
		default:
			break;
		}

		free(evtobj);
		evtobj = NULL;
	}

	/*
	while ( RspSendQ && (dxLstGetSize(RspSendQ) > 0) ) {
		TXTRACE1("[sipTxEvtDispatch] Process RspSendQ : size = %d \n",dxLstGetSize(RspSendQ));
		TxData = dxLstGetHead(RspSendQ);
		if (TxData)	{
			switch (TxData->txtype) {
			case TX_SERVER_INVITE:
				Server_Invite_SM(TxData, TX_SEND_RSP, NULL);
				break;
			case TX_SERVER_NON_INVITE:
				Server_Non_Invite_SM(TxData, TX_SEND_RSP, NULL);						
				break;
			default:
				TXWARN0("[sipTxEvtDispatch] Transaction type other than TX_SERVER_INVITE or TX_SERVER_NON_INVITE shouldn't be here! \n");
				break;
			}
		} else {
			TXWARN0("[sipTxEvtDispatch] NULL TxStruct in RspSendQ! \n");
		}
	}

	if ( ReqSendQ && (dxLstGetSize(ReqSendQ) > 0) ) {
		TXTRACE1("[sipTxEvtDispatch] Process ReqSendQ : size = %d \n",dxLstGetSize(ReqSendQ));
		TxData = dxLstGetHead(ReqSendQ);
		if (TxData)	{
			switch (TxData->txtype) {
			case TX_CLIENT_INVITE:
				Client_Invite_SM(TxData, TX_SEND_REQ, NULL);
				break;
			case TX_CLIENT_NON_INVITE:
				Client_Non_Invite_SM(TxData, TX_SEND_REQ, NULL);						
				break;
			case TX_CLIENT_ACK:
				Client_Ack_SM(TxData, TX_SEND_REQ, NULL);
				break;
			default:
				break;
			}
		} else {
			TXWARN0("[sipTxEvtDispatch] NULL TxStruct in ReqSendQ! \n");
		}
	}

	while ( GarbageQ && (dxLstGetSize(GarbageQ) > 0) ) {
		TXTRACE1("[sipTxEvtDispatch] Process GarbageQ : size = %d \n",dxLstGetSize(GarbageQ));
		TxData = dxLstGetHead(GarbageQ);
		if (TxData)	{
			TXTRACE1("[sipTxEvtDispatch] Collect garbage transaction txtype = %s \n",sipTxType2Phase(TxData->txtype) );
			_sipTxFree(TxData);
			TxData = NULL;
		} else {
			TXWARN0("[sipTxEvtDispatch] NULL TxStruct in GarbageQ! \n");
		}
	}
*/
	while ( TimerQ && (dxLstGetSize(TimerQ) > 0) ) {
		TXTRACE1("[sipTxEvtDispatch] Process TimerQ : size = %d \n",dxLstGetSize(TimerQ));
		TimerData = dxLstGetHead(TimerQ);
		if (TimerData)
		{
			switch (TimerData->data->txtype) {
			case TX_CLIENT_INVITE:
				Client_Invite_SM(TimerData->data, TimerData->type, NULL);
				break;
			case TX_CLIENT_NON_INVITE:
				Client_Non_Invite_SM(TimerData->data, TimerData->type, NULL);
				break;
			case TX_SERVER_INVITE:
				Server_Invite_SM(TimerData->data, TimerData->type, NULL);
				break;
			case TX_SERVER_NON_INVITE:
				Server_Non_Invite_SM(TimerData->data, TimerData->type, NULL);
				break;
			case TX_CLIENT_2XX:
				Client_2XX_SM(TimerData->data, TimerData->type, NULL);
				break;
			case TX_CLIENT_ACK:
				Client_Ack_SM(TimerData->data, TimerData->type, NULL);
				break;
			default:
				break;
			}
			free(TimerData);
			TimerData = NULL;
		}
	}

	return retcode;
}	


CCLAPI UserProfile sipTxGetUserProfile(TxStruct TxData)
{ 
	if ( TxData )
		return TxData->profile;
	
	return NULL;
}


CCLAPI RCODE sipTxSetUserData(TxStruct TxData, void* data)
{
	if ( TxData ) {
		TxData->UserData = data;
		return RC_OK;
	}
	return RC_ERROR;
}


CCLAPI void* sipTxGetUserData(TxStruct TxData)
{ 
	if ( TxData )
		return TxData->UserData;
	return NULL;
}


CCLAPI SipMethodType sipTxGetMethod(TxStruct TxData)
{
	SipReqLine* reqline = NULL;
	if ( TxData ) {
		if ( TxData->OriginalReq ) {
			reqline = sipReqGetReqLine(TxData->OriginalReq);
			return reqline->iMethod;
		}
	}
	return UNKNOWN_METHOD;
}


CCLAPI SipReq sipTxGetOriginalReq(TxStruct TxData)
{ 
	if ( TxData )
		return TxData->OriginalReq;
	return NULL;
}


CCLAPI SipRsp sipTxGetLatestRsp(TxStruct TxData)
{
	if ( TxData )
		return TxData->LatestRsp;
	return NULL;
}


CCLAPI SipReq sipTxGetAck(TxStruct TxData)
{ 
	if ( TxData )
		return TxData->Ack;
	return NULL;
}


CCLAPI TXTYPE sipTxGetType(TxStruct TxData)
{ 
	if ( TxData )
		return TxData->txtype;
	return TX_NON_ASSIGNED;
}


CCLAPI TXSTATE sipTxGetState(TxStruct TxData)
{
	if ( TxData )
		return TxData->state;
	return TX_NULL;
}


CCLAPI const char* sipTxGetTxid(TxStruct TxData)
{
	if ( TxData )
		return (const char*)TxData->txid;
	return NULL;
}


CCLAPI const char* sipTxGetCallid(TxStruct TxData)
{ 
	if ( TxData )
		return (const char*)TxData->callid;
	return NULL;
}


CCLAPI unsigned short sipTxGetRefCount(TxStruct TxData)
{ 
	if ( TxData )
		return TxData->RefCount;
	return 0;
}


CCLAPI unsigned short sipTxIncRefCount(TxStruct TxData)
{ 
	if ( TxData )
		return ++TxData->RefCount;
	return 0;
}


CCLAPI unsigned short sipTxDecRefCount(TxStruct TxData)
{ 
	if ( TxData )
		return --TxData->RefCount;
	return 0;
}


const char* sipTxGetLocalAddr()
{
#ifdef CCLSIP_ENABLE_STUN
	if ( NULL!=strSTUNserver ) {
		stunGetExtAddr( strSTUNserver, strExtAddress );
		TXTRACE1("[sipTxGetLocalAddr] Get external address via STUN = %s\n", strSTUNserver);
		TXTRACE1("ExtAddr=%d,\n", strExtAddress);
	}
	return (const char*)strExtAddress;
#else
	return (const char*)strLocalAddress;
#endif
}


UINT16 sipTxGetLocalPort()
{
#ifdef CCLSIP_ENABLE_STUN
	if ( NULL!=strSTUNserver ) {
		usExtPort = stunGetExtPort( strSTUNserver, usLocalPort );
		TXTRACE1("[sipTxGetLocalPort] Get external port via STUN = %s\n", strSTUNserver);
		TXTRACE1("LocalPort=%d,\n", usLocalPort);
		TXTRACE1("ExtPort=%d\n", usExtPort);
	}
	return usExtPort;
#else
	return usLocalPort;
#endif
}


char * sipTxGetRemoteAddr(TxStruct TxData)
{
	if ( TxData )
		return TxData->raddr;
	return NULL;
}


UINT16 sipTxGetRemotePort(TxStruct TxData)
{
	if ( TxData )
		return TxData->rport;
	return 0;
}


CCLAPI UserProfile UserProfileDup(UserProfile profile)
{
	UserProfile _this = NULL;

	if (!profile)
		return NULL;
	
	_this = (UserProfile) malloc(sizeof (*_this));
	assert (_this);
	memset(_this, 0, sizeof (*_this));
	
	_this->useproxy = profile->useproxy;
	_this->proxyaddr = strDup(profile->proxyaddr);
	_this->localip = strDup(profile->localip);
	_this->localport = profile->localport;

	return _this;
}


CCLAPI void UserProfileFree(UserProfile profile)
{
	if (!profile)
		return;

	if (profile->proxyaddr) {
		free(profile->proxyaddr);
		profile->proxyaddr = NULL;
	}

	if (profile->localip) {
		free(profile->localip);
		profile->localip = NULL;
	}

	free(profile);
	profile = NULL;
}

CCLAPI const char* sipTxState2Phase(TXSTATE state)
{
	switch (state)
	{
	case TX_NULL:
		return "TX_NULL";

	case TX_INIT:
		return "TX_INIT";

	case TX_TRYING:
		return "TX_TRYING";

	case TX_CALLING:
		return "TX_CALLING";

	case TX_PROCEEDING:
		return "TX_PROCEEDING";

	case TX_COMPLETED:
		return "TX_COMPLETED";

	case TX_CONFIRMED:
		return "TX_CONFIRMED";

	case TX_TERMINATED:
		return "TX_TERMINATED";
	}

	return NULL;
}


CCLAPI const char* sipTxType2Phase(TXTYPE type)
{
	switch (type)
	{
	case TX_NON_ASSIGNED:
		return "TX_NON_ASSIGNED";

	case TX_SERVER_INVITE:
		return "TX_SERVER_INVITE";
	
	case TX_SERVER_NON_INVITE:
		return "TX_SERVER_NON_INVITE";

	case TX_SERVER_ACK:
		return "TX_SERVER_ACK";

	case TX_CLIENT_INVITE:
		return "TX_CLIENT_INVITE";

	case TX_CLIENT_NON_INVITE:
		return "TX_CLIENT_NON_INVITE";

	case TX_CLIENT_ACK:
		return "TX_CLIENT_ACK";

	case TX_CLIENT_2XX:
		return "TX_CLEINT_2XX";

	case TX_HOLDER:
		return "TX_HOLDER";

	}

	return NULL;
}


CCLAPI const char* sipTxEvtType2Phase(SipTxEvtType evttype)
{

	switch (evttype)
	{
	case TX_REQ_RECEIVED:
		return "TX_REQ_RECEIVED";

	case TX_RSP_RECEIVED:
		return "TX_RSP_RECEIVED";
	
	case TX_CANCEL_RECEIVED:
		return "TX_CANCEL_RECEIVED";
	
	case TX_ACK_RECEIVED:
		return "TX_ACK_RECEIVED";
	
	case TX_SEND_REQ:
		return "TX_SEND_REQ";
	
	case TX_SEND_RSP:
		return "TX_SEND_RSP";
	
	case TX_TRANSPORT_ERROR:
		return "TX_TRANSPORT_ERROR";
	
	case TX_TERMINATED_EVENT:
		return "TX_TERMINATED_EVENT";
	
	case TX_TIMER_A:
		return "TX_TIMER_A";
	
	case TX_TIMER_B:
		return "TX_TIMER_B";
	
	case TX_TIMER_C:
		return "TX_TIMER_C";
	
	case TX_TIMER_D:
		return "TX_TIMER_D";
	
	case TX_TIMER_E:
		return "TX_TIMER_E";
	
	case TX_TIMER_F:
		return "TX_TIMER_F";
	
	case TX_TIMER_G:
		return "TX_TIMER_G";
	
	case TX_TIMER_H:
		return "TX_TIMER_H";
	
	case TX_TIMER_I:
		return "TX_TIMER_I";

	case TX_TIMER_J:
		return "TX_TIMER_J";
	
	case TX_TIMER_K:
		return "TX_TIMER_K";
	
	case TX_2XX_FOR_INVITE:
		return "TX_2XX_FOR_INVITE";
	
	case TX_NOMATCH_RSP_RECEIVED:
		return "TX_NOMATCH_RSP_RECEIVED";

	default:
		break;
	}
	return NULL;
}


CCLAPI const unsigned short sipTxGetInternalNum(TxStruct _this)
{
	if (_this);
		return (const unsigned short)_this->InternalNum;

	return -1;
}
