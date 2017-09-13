/* 
 * Copyright (C) 2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sipTx.h
 *
 * $Id: sipTx.h,v 1.50 2005/04/27 03:27:39 ljchuang Exp $
 */
#ifndef __SIP_TX__
#define __SIP_TX__

#ifdef	__cplusplus
extern "C" {
#endif

#include <sip/sip.h>
#include <adt/dx_lst.h>
#include <adt/dx_hash.h>

#ifndef CCLSIP_ENABLE_TLS

#define sipTLSConnect(x, y, z) sipTCPConnect((SipTCP)x, y, z)
#define sipTLSNew(x, y) sipTCPNew(x, y)
#define sipTLSFree(x) sipTCPFree((SipTCP)x)
#define	sipTLSGetRport(x, y) sipTCPGetRport((SipTCP)x, y)
#define sipTLSGetRaddr(x) sipTCPGetRaddr((SipTCP)x)
#define sipTLSSendReq(x, y) sipTCPSendReq((SipTCP)x, y)
#define sipTLSSendRsp(x, y) sipTCPSendRsp((SipTCP)x, y)

#endif

typedef enum _TXPTYPE
{
	UDP,
	TCP,
	TXPTLS,
	UNKNOWN_TXP
} TXPTYPE;

typedef enum _STATETYPE
{
	TX_NULL,
	TX_INIT,
	TX_TRYING,
	TX_CALLING,
	TX_PROCEEDING,
	TX_COMPLETED,
	TX_CONFIRMED,
	TX_TERMINATED
} TXSTATE;

typedef enum _TXTYPE
{
	TX_NON_ASSIGNED,
	TX_SERVER_INVITE,
	TX_SERVER_NON_INVITE,
	TX_SERVER_ACK,
	TX_CLIENT_INVITE,
	TX_CLIENT_NON_INVITE,
	TX_CLIENT_2XX,
	TX_CLIENT_ACK,
	TX_HOLDER
} TXTYPE;

typedef enum SipTxEvtType_t
{
	TX_REQ_RECEIVED,
	TX_RSP_RECEIVED,
	TX_CANCEL_RECEIVED,
	TX_ACK_RECEIVED,
	TX_SEND_REQ,
	TX_SEND_RSP,
	TX_TRANSPORT_ERROR,
	TX_TERMINATED_EVENT,
	TX_TIMER_A,
	TX_TIMER_B,
	TX_TIMER_C,
	TX_TIMER_D,
	TX_TIMER_E,
	TX_TIMER_F,
	TX_TIMER_G,
	TX_TIMER_H,
	TX_TIMER_I,
	TX_TIMER_J,
	TX_TIMER_K,
	TX_2XX_FOR_INVITE,
	TX_NOMATCH_RSP_RECEIVED,
	TX_BADREQ_RECEIVED,
	TX_TCP_CONNECTED
} SipTxEvtType;

typedef struct _AddrPort
{
	char *strAddr;
	unsigned short uPort;
}* AddrPort;

typedef struct _UserProfile
{
	unsigned short useproxy;
	char* proxyaddr;
	char* localip;
	unsigned short localport;
}* UserProfile;


typedef void (*CommonTimerCB)(unsigned short id, void* data);

typedef struct _TxStruct* TxStruct;

typedef void (*SipTxEvtCB)(TxStruct TxData, SipTxEvtType event);

typedef struct _CommonTimer
{
	unsigned long time;
	void* data;
	CommonTimerCB cb;
}* CommonTimer;

/* Initialize the Tx Layer */
CCLAPI RCODE sipTxLibInit(IN SipTxEvtCB,
			  IN SipConfigData,
			  IN BOOL bMatchAck, 
			  IN unsigned short MaxTxCap);

/* Extension version of sipTxLibInit */
CCLAPI RCODE sipTxLibInitEx(IN SipTxEvtCB,
			    IN SipConfigData,
			    IN DxLst AddrPortLst,
			    IN BOOL bMatchAck, 
			    IN unsigned short MaxTxCap, 
			    IN const char* STUNserver);

/* Terminate the Tx Layer */
CCLAPI RCODE sipTxLibEnd(void);

/* The event dispatcher of Tx Layer */
CCLAPI RCODE sipTxEvtDispatch(int timeout);

/* Create a TxStruct */
CCLAPI TxStruct sipTxClientNew(SipReq msg,
			       char* txid,
			       UserProfile profile,
			       void* data);

/* Create a 2xx retransmit transaction */
CCLAPI TxStruct sipTx2xxClientNew( TxStruct origTx );

/* Delete a TxStruct */
CCLAPI void sipTxFree(TxStruct TxData);

/* Get the User profile from TxStruct */
CCLAPI UserProfile sipTxGetUserProfile(TxStruct TxData);

/* Set the User data for TxStruct */
CCLAPI RCODE sipTxSetUserData(TxStruct TxData, void* data);

/* Get the User data from TxStruct */
CCLAPI void* sipTxGetUserData(TxStruct TxData);

/* Send a response via a transaction */
CCLAPI void sipTxSendRsp(TxStruct TxData, SipRsp msg);

/* Set a SipReq to Tx as the ACK message */
CCLAPI void sipTxSetAck(TxStruct TxData, SipReq msg);

/* Get the original SipReq from TxStruct */
CCLAPI SipReq sipTxGetOriginalReq(TxStruct TxData);

/* Get the latest SipRsp from TxStruct */
CCLAPI SipRsp sipTxGetLatestRsp(TxStruct TxData);

/* Get the ACK message from TxStruct */
CCLAPI SipReq sipTxGetAck(TxStruct TxData);

/* Get the current Tx State */
CCLAPI TXSTATE sipTxGetState(TxStruct TxData);

/* Get the Transaction id */
CCLAPI const char* sipTxGetTxid(TxStruct Txdata);

/* Get the Callid */
CCLAPI const char* sipTxGetCallid(TxStruct Txdata);

/* Get the method of the original request */
CCLAPI SipMethodType sipTxGetMethod(TxStruct TxData);

/* Get the type of the transaction */
CCLAPI TXTYPE sipTxGetType(TxStruct TxData);

/* Get the reference count of a TxStruct */
CCLAPI unsigned short sipTxGetRefCount(TxStruct TxData);

/* Increase the reference count of a TxStruct */
CCLAPI unsigned short sipTxIncRefCount(TxStruct TxData);

/* Decrease the reference count of a TxStruct */
CCLAPI unsigned short sipTxDecRefCount(TxStruct TxData);

/* get the local address (assigned when the transaction layer is initialized ) */
CCLAPI const char * sipTxGetLocalAddr();

/* get the local port (assigned when the transaction layer is initialized ) */
CCLAPI UINT16 sipTxGetLocalPort();

/* get the remote address */
CCLAPI char * sipTxGetRemoteAddr(TxStruct TxData);

/* get the remote port */
CCLAPI UINT16 sipTxGetRemotePort(TxStruct TxData);

/* Send a response out, which is not belong to any transaction */
/* Useful for retransmitting 2xx response of INVITE transaction */
CCLAPI RCODE SendResponseEx(SipRsp pMsg, UserProfile profile);

/* Keep alive a TCP connection to iphost:port */
CCLAPI RCODE KeepTCPConnection( char* iphost, unsigned short port );

/* Unkeep alive a TCP connection to iphost:port */
CCLAPI RCODE UnKeepTCPConnection( char* iphost, unsigned short port );

/* Duplicate user profile */
CCLAPI UserProfile UserProfileDup(UserProfile profile);

/* Free user profile*/
CCLAPI void UserProfileFree(UserProfile profile);

CCLAPI const char* sipTxState2Phase(TXSTATE);

CCLAPI const char* sipTxType2Phase(TXTYPE);

CCLAPI const char* sipTxEvtType2Phase(SipTxEvtType);

CCLAPI const unsigned short sipTxGetInternalNum(TxStruct);

CCLAPI RCODE CommonTimerAdd(IN unsigned long time, 
			    IN void* data, 
			    IN CommonTimerCB cb, 
			    OUT unsigned short *timerid);

CCLAPI RCODE CommonTimerDel(IN unsigned short timerid, OUT void** data);

/* Resolve DNS hostname */
CCLAPI BOOL ResolveName( char ** strHost );

CCLAPI SipRsp NewRspFromReq(const SipReq request, const unsigned short statuscode);

CCLAPI AddrPort AddrPortNew( char* addr, unsigned short port );
CCLAPI void AddrPortFree( AddrPort tmpAddrPort );

#ifdef __cplusplus
}
#endif

#endif /* ifndef __SIP_TX__ */

