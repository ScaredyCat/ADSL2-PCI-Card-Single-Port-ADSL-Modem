/* 
 * Copyright (C) 2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * Transport.h
 *
 * $Id: Transport.h,v 1.15 2004/12/16 08:57:05 ljchuang Exp $
 */
#ifndef __TRANSPORT__
#define __TRANSPORT__

#include <sip/cclsip.h>
#include "sipTx.h"
#include "TxStruct.h"
#include "net.h"

#ifdef	__cplusplus
extern "C" {
#endif

#define RC_TCP_CONNECTING	RC_BASE_SIPTX+1


RCODE SendRequest(TxStruct TxData);

RCODE SendResponse(TxStruct TxData);

/* Send some message out, TxData can be NULL if you don't have one */
RCODE SendRequestEx(SipReq pMsg, BOOL bPreparedForStrictRouter, TxStruct TxData);

RCODE SendACK(TxStruct TxData);

/* alter the message for strict routers as necessary, the BOOL value 
indicate whether it has been altered or not */
RCODE PrepareRoute( SipReq pMsg, BOOL * pbPreparedForStrictRouter );

char * CalculateBranch ( SipReq pMsg );

BOOL FillVia ( SipReq pMsg, TxStruct TxData );

typedef void * PTcpConnection;

PTcpConnection FindPTcpConnection ( SipTCP tcp );

BOOL IsPTcpConnected( PTcpConnection pTcp );

void SetPTcpConnected( PTcpConnection pTcp );

/* add reference to some specific tcp connection, if the given connection
   has not been seen before, it will be registered.
   If the given tcp connection is found, the IP and port
   are ignored, or they are used to create a new connection record.

   intended to be used in sip stack callbacks
 */
PTcpConnection AddRefTcpConnection ( SipTCP tcp, unsigned int uRemoteIP, unsigned short usRemotePort, BOOL bSecure );

/* close a specific tcp connection. the given connection will be marked as closed
   and will not be returned in RequireTcpConnection 

  There are some rare cases that a SipTCP is not registered as used by any transaction (like a malformed message),
  so this function is prepared to deal with this. The return value (FALSE) means the SipTCP is not registered,
  the call back function shall handle it accordingly.
   
   intended to be used in sip stack callbacks
*/
BOOL CloseTcpConnection ( SipTCP tcp );

SipTCP GetUnderlyingTcp ( PTcpConnection pTcp );

/* decrease the reference count of the given tcp connection 
*/
void ReleaseTcpConnection ( PTcpConnection pTcp );

void InitializeTransport ( DxLst AddrPortLst );

void FinalizeTransport ();

#ifdef __cplusplus
}
#endif

#endif /* ifndef __TRANSPORT__ */

