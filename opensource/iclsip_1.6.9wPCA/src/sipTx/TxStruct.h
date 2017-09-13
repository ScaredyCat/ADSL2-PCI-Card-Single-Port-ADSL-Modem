/* 
 * Copyright (C) 2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * TxStruct.h
 *
 * $Id: TxStruct.h,v 1.24 2006/07/17 08:21:33 tyhuang Exp $
 */
#ifndef __TXSTRUCT__
#define __TXSTRUCT__

#include <sip/cclsip.h>
#include <low/cx_mutex.h>
#include "sipTx.h"

#ifdef	__cplusplus
extern "C" {
#endif

struct _TxStruct
{
	/* note : the flags must be updated accordingly if the message shall be modfied ! */
	SipReq OriginalReq;
	BOOL bOriginalReq_PreparedForStrictRouter;
	
	/* note : the flags must be updated accordingly if the message shall be modfied ! */
	SipReq Ack;
	BOOL bAck_PreparedForStrictRouter;

	SipRsp LatestRsp;
	SipMethodType method;
	char* callid;
	char* txid;
	void* transport;
	TXSTATE state;
	TXTYPE txtype;
	TXPTYPE txptype;
	unsigned short RefCount;
	UserProfile profile;
	void* UserData;
	unsigned short TimerA;
	unsigned short TimerB;
	unsigned short TimerC;
	unsigned short TimerD;
	unsigned short TimerE;
	unsigned short TimerF;
	unsigned short TimerG;
	unsigned short TimerH;
	unsigned short TimerI;
	unsigned short TimerJ;
	unsigned short TimerK;
	short TimerHandle1; /* Handle of TimerA or TimerE or TimerG or TimerJ */
	short TimerHandle2; /* Handle of TimerB or TimerF or TimerH */
	short TimerHandle3; /* Handle of TimerD or TimerK or TimerI */

	SipTxEvtType TimerEvent;
	
	unsigned short bNoCB; /* 1 : don't callback UACore  0 : callback UACore */

	char* raddr;
	unsigned short rport;

	CxMutex mutex;
	unsigned short InternalNum;

	BOOL	bGarbage;
};

typedef struct _TxTimer
{
	TxStruct data;
	SipTxEvtType type;
}* TxTimer;

typedef enum _TXDISPATCHEVTTYPE
{
	TX_DISPATCH_SENDREQ,
	TX_DISPATCH_SENDRSP,
	TX_DISPATCH_GARBAGE
} TXDISPATCHEVTTYPE;

typedef struct _TxDispatchEvtObj
{
	TXDISPATCHEVTTYPE evt;
	TxStruct txdata;
}* TxDispatchEvtObj;

#define	T1	500
#define	T2	4000
#define	T4	5000

#ifdef _DEBUG
#define CCLSIP_SIPTX_ENABLE_TRACE
#endif

#ifdef CCLSIP_SIPTX_ENABLE_TRACE
	#define TXTRACE0(W) TCRPrint ( TRACE_LEVEL_API, "[SIPTX] " W )
	#define TXTRACE1(W,X) TCRPrint ( TRACE_LEVEL_API, "[SIPTX] " W, X )
	#define TXTRACE2(W,X,Y) TCRPrint ( TRACE_LEVEL_API, "[SIPTX] " W, X, Y )
	#define TXTRACE3(W,X,Y,Z) TCRPrint ( TRACE_LEVEL_API, "[SIPTX] " W, X, Y, Z )
	#define TXWARN0(W) TCRPrint ( TRACE_LEVEL_WARNING, "[SIPTX] WARNING: " W )
	#define TXWARN1(W,X) TCRPrint ( TRACE_LEVEL_WARNING, "[SIPTX] WARNING: " W, X )
	#define TXWARN2(W,X,Y) TCRPrint ( TRACE_LEVEL_WARNING, "[SIPTX] WARNING: " W, X, Y )
	#define TXWARN3(W,X,Y,Z) TCRPrint ( TRACE_LEVEL_WARNING, "[SIPTX] WARNING: " W, X, Y, Z )
#else
	#define TXTRACE0(W)
	#define TXTRACE1(W,X)
	#define TXTRACE2(W,X,Y)
	#define TXTRACE3(W,X,Y,Z)
	#define TXWARN0(W)
	#define TXWARN1(W,X)
	#define TXWARN2(W,X,Y)
	#define TXWARN3(W,X,Y,Z)
#endif



#define TXERR0(W) TCRPrint ( TRACE_LEVEL_ERROR, "[SIPTX] ERROR: " \
				W "(at %s, %d)\n", __FILE__, __LINE__)
#define TXERR1(W,X) TCRPrint ( TRACE_LEVEL_ERROR, "[SIPTX] ERROR: " \
				W "(at %s, %d)\n", X, __FILE__, __LINE__ )
#define TXERR2(W,X,Y) TCRPrint ( TRACE_LEVEL_ERROR, "[SIPTX] ERROR: " \
				W "(at %s, %d)\n", X, Y, __FILE__, __LINE__ )
#define TXERR3(W,X,Y,Z) TCRPrint ( TRACE_LEVEL_ERROR, "[SIPTX] ERROR: " \
				W "(at %s, %d)\n", X, Y, Z, __FILE__, __LINE__ )

#define txMutexLock(x)
#define txMutexUnlock(x)

void txPutDispatchEvtLst( TxStruct , TXDISPATCHEVTTYPE );

extern SipUDP g_sipUDP;
 
#ifdef __cplusplus
}
#endif

#endif /* ifndef __TXSTRUCT__ */

