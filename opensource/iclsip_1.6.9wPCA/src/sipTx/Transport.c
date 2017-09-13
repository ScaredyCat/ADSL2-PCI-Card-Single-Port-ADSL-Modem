/* 
 * Copyright (C) 2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * Transport.c
 *
 * $Id: Transport.c,v 1.77 2006/12/05 09:30:41 tyhuang Exp $
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <common/cm_trace.h>
#include <sip/md5_g.h>
#include <sip/md5.h>
#include <sip/cclsip.h>

#include "net.h"
#include "sipTx.h"
#include "TxStruct.h"
#include "CTransactionDatabase.h" /* for GetTopMostVia */
#include "Transport.h"

/*#include <time.h>*/


extern char * strLocalAddress;
extern unsigned short usLocalPort;

DxHash interfaceTable = NULL;
DxHash sipUDPTable = NULL;
DxHash sipTCPTable = NULL;

typedef struct {
	SipTCP tcp;
	unsigned int uRemoteIP;
	unsigned short usRemotePort;
	int iRefCount;
	BOOL bIsServerSocket;
	BOOL bKeep;
	BOOL bConnected;
	BOOL bSecure;
} TcpConnection;

/* you can define this number in the makefile or 
   somewhere eles to suppress this default */
#ifndef NUM_UDP_PORT
	#define NUM_UDP_PORT (32)
#endif

#ifndef NUM_TCP_LISTEN_PORT
	#define NUM_TCP_LISTEN_PORT (32)
#endif

#ifndef NUM_TCP_INTERFACE
	#define NUM_TCP_INTERFACE (4)
#endif

#ifndef NUM_TCP_CONNECTION
	#define NUM_TCP_CONNECTION (256)
#endif

TcpConnection arrayTcpConnection [NUM_TCP_INTERFACE][NUM_TCP_CONNECTION];

void ResetTcpConnection ( TcpConnection * pTCP );
void InternalCloseTcpConnection ( TcpConnection * pTCP );

TxStruct sipTxHolderNew(SipRsp pMsg, UserProfile profile);


void InitializeTransport ( DxLst AddrPortLst )
{
	int addrportsize = 0, i = 0;
	SipUDP tmpUDP = NULL,oldUDP;
	SipTCP tmpTCP = NULL;
	char tmpstr[64];

	interfaceTable = dxHashNew( NUM_TCP_INTERFACE, FALSE, DX_HASH_POINTER );
	sipUDPTable = dxHashNew( NUM_UDP_PORT, FALSE, DX_HASH_POINTER );
	sipTCPTable = dxHashNew( NUM_TCP_LISTEN_PORT, FALSE, DX_HASH_POINTER );

	dxHashAdd(interfaceTable, strLocalAddress, arrayTcpConnection[0]);

	sprintf(tmpstr, "%s:%d", strLocalAddress, usLocalPort);
	/*
	 * modified by tyhuang
	 * dxHashAdd(sipUDPTable, tmpstr, g_sipUDP);
	 */
	
	if (AddrPortLst != NULL)
	{
		addrportsize = dxLstGetSize( AddrPortLst );
		
		for ( i = 0; i < addrportsize; i++)
		{
			AddrPort tmpAddrPort = (AddrPort) dxLstPeek( AddrPortLst, i );
			sprintf(tmpstr, "%s:%d", tmpAddrPort->strAddr, tmpAddrPort->uPort);

			if (dxHashItem(interfaceTable, tmpAddrPort->strAddr) == NULL)
				dxHashAdd(interfaceTable, tmpAddrPort->strAddr, (void*)arrayTcpConnection[dxHashSize(interfaceTable)]);

			tmpUDP = sipUDPNew(tmpAddrPort->strAddr, tmpAddrPort->uPort);

			/*
			 * modified by tyhuang
			 * (SipUDP)dxHashAdd( sipUDPTable, tmpstr, tmpUDP );
			 */
			oldUDP=(SipUDP)dxHashAdd( sipUDPTable, tmpstr, tmpUDP );
			if (oldUDP) 
				sipUDPFree(oldUDP);

			tmpTCP = sipTCPSrvNew(tmpAddrPort->strAddr, tmpAddrPort->uPort);
			dxHashAdd( sipTCPTable, tmpstr, tmpTCP);
		}
	}
}

void FinalizeTransport ()
{
	if (interfaceTable) {
		dxHashFree( interfaceTable, NULL);
		interfaceTable = NULL;
	}

	if (sipUDPTable) {
		dxHashFree(sipUDPTable, sipUDPFree);
		sipUDPTable = NULL;
	}

	if (sipTCPTable) {
		/*
		 * modified by tyhuang
		 * dxHashFree(sipTCPTable, sipUDPFree);
		 */
		dxHashFree(sipTCPTable, sipTCPFree);
		sipTCPTable = NULL;
	}
}

void InitializeTcpConnectionArray () {
	int i = 0, j = 0;
	/* set all elements to their default values */
	for ( i = 0; i < NUM_TCP_INTERFACE; ++i ) {
		for ( j = 0; j < NUM_TCP_CONNECTION; ++j ) {
			ResetTcpConnection ( &arrayTcpConnection[i][j] );
		}
	}
}

void FinalizeTcpConnectionArray () {
	int i = 0, j = 0;
	/* set all elements to their default values */
	for ( i = 0; i < NUM_TCP_INTERFACE; ++i ) {
		for ( j = 0; j < NUM_TCP_CONNECTION; ++j ) {
			InternalCloseTcpConnection ( &arrayTcpConnection[i][j] );
			ResetTcpConnection ( &arrayTcpConnection[i][j] );
		}
	}
}

/* drop the associated tcp connection */
void InternalCloseTcpConnection ( TcpConnection * pTCP ) {
	assert(pTCP);
	if ( pTCP ) {
		if ( pTCP->tcp ) {
			if (pTCP->bSecure) {
				sipTLSFree( (SipTLS)pTCP->tcp );
			} else {
				if ( pTCP->bIsServerSocket ) {
					/* we may not free server socket here, 
					because the client should be responsible to close this connection.
					it may cause some memory leak on exit */
					sipTCPFree ( pTCP->tcp );
				} else {
					sipTCPFree ( pTCP->tcp );
				}
			}
			pTCP->tcp = NULL;
		}
	}
}

/* fill the whole struct with zero, 
   then set all elements to their default values */
void ResetTcpConnection ( TcpConnection * pTCP ) {

	assert ( pTCP );
	if ( pTCP ) {

		/* fill the whole struct with zero */
		memset ( pTCP, 0, sizeof(TcpConnection) );

		/* set all elements to their default values */
		pTCP->iRefCount = 0;
		pTCP->tcp = NULL;
		pTCP->uRemoteIP = INADDR_NONE;
		pTCP->usRemotePort = 0;
		pTCP->bIsServerSocket = FALSE;
		pTCP->bKeep = FALSE;
		pTCP->bSecure = FALSE;
	}
}

TcpConnection * FindTcpConnection ( SipTCP tcp ) {
	int i = 0, j =0;
	for ( i = 0; i < NUM_TCP_INTERFACE; ++i ) {
		for ( j = 0; j < NUM_TCP_CONNECTION; ++j ) {
			if ( tcp == arrayTcpConnection[i][j].tcp ) {
				return &arrayTcpConnection[i][j];
			}
		}
	}
	return NULL;
}

PTcpConnection FindPTcpConnection ( SipTCP tcp ) {
	int i = 0, j =0;
	for ( i = 0; i < NUM_TCP_INTERFACE; ++i ) {
		for ( j = 0; j < NUM_TCP_CONNECTION; ++j ) {
			if ( tcp == arrayTcpConnection[i][j].tcp ) {
				return &arrayTcpConnection[i][j];
			}
		}
	}	
	return NULL;
}

TcpConnection * FindFreeTcpConnection (char* localip) {
	int i = 0;
	TcpConnection * tmpTcpConnectionArray = NULL;

	if (localip == NULL)
		return NULL;

	if ( dxHashItem(interfaceTable, localip ) == NULL)
		return NULL;

	tmpTcpConnectionArray = (TcpConnection *) dxHashItem(interfaceTable, localip);

	for ( i = 0; i < NUM_TCP_CONNECTION; ++i ) {
		/* reference count equals to zero should be suffice since
		   the connection should have been closed when reference count
		   drops to zero.
		   we check the connection here just to be on the safe side */
		if ( NULL == tmpTcpConnectionArray[i].tcp ) {
			if ( 0 == tmpTcpConnectionArray[i].iRefCount ) {
				return &tmpTcpConnectionArray[i];
			}
		}
	}
	return NULL;
}

PTcpConnection AddRefTcpConnection ( SipTCP tcp, unsigned int uRemoteIP, 
				   unsigned short usRemotePort, BOOL bSecure )
{
	IN_ADDR addr;
	TcpConnection * pTCP;
	char* localip = (char*) sipTCPGetLaddr( tcp );

	/* addr.S_un.S_addr = uRemoteIP; */
	addr.s_addr = uRemoteIP;

	/* try to find an existing connection which matches this SipTCP pointer */
	pTCP = FindTcpConnection(tcp);
	if ( pTCP ) {
		/* we have found one, increase the reference count and
		return the structure */

		++(pTCP->iRefCount);

		if (bSecure)
			TXTRACE3("[AddRefTcpConnection] TLS connection %s:%d Refcount = %d \n", inet_ntoa(addr), usRemotePort, pTCP->iRefCount);
		else
			TXTRACE3("[AddRefTcpConnection] TCP connection %s:%d Refcount = %d \n", inet_ntoa(addr), usRemotePort, pTCP->iRefCount);
	
		return pTCP;
	} else {
		/* we fail to find one, create a new connection record */
		pTCP = FindFreeTcpConnection ( localip );
		if ( pTCP ) {
			/* cleaning up everything to default */
			ResetTcpConnection ( pTCP );
			
			pTCP->tcp = tcp;
			pTCP->iRefCount = 1;
			pTCP->bIsServerSocket = TRUE;
			pTCP->bKeep = FALSE;
			pTCP->uRemoteIP = uRemoteIP;
			pTCP->usRemotePort = usRemotePort;
			pTCP->bSecure = bSecure;

			if (bSecure)
				TXTRACE3("[AddRefTcpConnection] TLS connection %s:%d Refcount = %d \n", inet_ntoa(addr), usRemotePort, pTCP->iRefCount);
			else
				TXTRACE3("[AddRefTcpConnection] TCP connection %s:%d Refcount = %d \n", inet_ntoa(addr), usRemotePort, pTCP->iRefCount);

			return pTCP;
		}
	}
	return NULL;
}

/* Force close TcpConnection */
BOOL CloseTcpConnection ( SipTCP tcp ) {
	TcpConnection * pTCP = FindTcpConnection ( tcp );
	if ( pTCP ) {
		InternalCloseTcpConnection ( pTCP );
		ResetTcpConnection ( pTCP ); /* added by ljchuang 2004/06/08 */
		return TRUE;
	}
	return FALSE;
}

/* 
require a still open tcp connection which points to the given address and port 
and increase the reference count by 1. or create a new connection to the designated
address/port and set the reference count to 1.
*/
PTcpConnection _RequireTcpConnection( unsigned int uRemoteIP, 
				      unsigned short usRemotePort, 
				      BOOL bSecure, 
				      char* localip) 
{
	int i = 0;
	IN_ADDR addr;
	TcpConnection * pTCP = NULL;
	TcpConnection * tmpTcpConnectionArray = NULL;

	/* addr.S_un.S_addr = uRemoteIP; */
	addr.s_addr = uRemoteIP;

	if ( localip == NULL )
		return NULL;

	if ( dxHashItem(interfaceTable, localip ) == NULL)
		return NULL;

	tmpTcpConnectionArray = (TcpConnection *) dxHashItem(interfaceTable, localip);
	
	for ( i = 0; i < NUM_TCP_CONNECTION; ++i ) {
		pTCP = &tmpTcpConnectionArray[i];
		if ( (pTCP->tcp) && 
			(uRemoteIP == pTCP->uRemoteIP) && 
			(usRemotePort == pTCP->usRemotePort) ) {
			++(pTCP->iRefCount);

			TXTRACE3("[RequireTcpConnection] TCP connection %s:%d Refcount = %d \n", inet_ntoa(addr), usRemotePort, pTCP->iRefCount);

			return pTCP;
		}
	}

	/* cannot find an existing connection, create a new one */
	pTCP = FindFreeTcpConnection ( localip );
	if ( pTCP ) {
		IN_ADDR addr;

		/* cleaning up everything to default */
		ResetTcpConnection ( pTCP );
		
		if (bSecure)
			pTCP->tcp = (SipTCP) sipTLSNew(localip, 0);
		else
			pTCP->tcp = sipTCPNew(localip, 0);
		
		pTCP->bSecure = bSecure;
		pTCP->iRefCount = 1;
		pTCP->bIsServerSocket = FALSE;
		pTCP->bKeep = FALSE;
		pTCP->uRemoteIP = uRemoteIP;
		pTCP->usRemotePort = usRemotePort;
		pTCP->bConnected = FALSE;

		/* addr.S_un.S_addr = pTCP->uRemoteIP; */
		addr.s_addr = pTCP->uRemoteIP;

		if (bSecure) {
			TXTRACE2("[TRANSPORT] Trying TLS connection to %s:%d \n", inet_ntoa(addr), pTCP->usRemotePort);

			if ( RC_OK != sipTLSConnect ( (SipTLS) pTCP->tcp, inet_ntoa(addr), pTCP->usRemotePort) ) {
				sipTLSFree( (SipTLS) pTCP->tcp );
				pTCP->tcp = NULL;
				TXERR0("[TRANSPORT] TLS connection fail !!! \n");
			}
		} else {
			TXTRACE2("[TRANSPORT] Trying TCP connection to %s:%d \n", inet_ntoa(addr), pTCP->usRemotePort);

			if ( RC_OK != sipTCPConnect ( pTCP->tcp, inet_ntoa(addr), pTCP->usRemotePort) ) {
				sipTCPFree( pTCP->tcp );
				pTCP->tcp = NULL;
				TXERR0("[TRANSPORT] TCP connection fail !!! \n");
			}

		}
		TXTRACE3("[RequireTcpConnection] TCP connection %s:%d Refcount = %d \n", inet_ntoa(addr), usRemotePort, pTCP->iRefCount);

		return pTCP;
	}

	return NULL;
}

PTcpConnection RequireTcpConnection( unsigned int uRemoteIP, unsigned short usRemotePort, BOOL bSecure)
{
	return _RequireTcpConnection( uRemoteIP, usRemotePort, bSecure, strLocalAddress);
}

/* 
extract the tcp connection to call the sip stack functions 
*/
SipTCP GetUnderlyingTcp ( PTcpConnection pTcp ) {
	if ( pTcp ) {
		return ((TcpConnection * )pTcp)->tcp;
	}
	return NULL;
}

void ReleaseTcpConnection ( PTcpConnection pTcp ) {
	TcpConnection * pTCP = pTcp;
	IN_ADDR addr;

	if ( pTCP ) {
		addr.s_addr = pTCP->uRemoteIP;
		
		if ( 0 < pTCP->iRefCount ) {
			--(pTCP->iRefCount);
		}
		if ( ( 0 == pTCP->iRefCount ) && !pTCP->bKeep) {
			TXTRACE3("[ReleaseTcpConnection] release TCP connection %s:%d Refcount = %d \n",
					inet_ntoa(addr), pTCP->usRemotePort, pTCP->iRefCount);

			InternalCloseTcpConnection ( pTCP );
			ResetTcpConnection ( pTCP );
		} else {
			TXTRACE3("[ReleaseTcpConnection] TCP connection %s:%d can't released, Refcount = %d\n",
					inet_ntoa(addr), pTCP->usRemotePort, pTCP->iRefCount);
		}
	}
}

BOOL IsPTcpConnected( PTcpConnection pTcp ) {
	if ( pTcp )
		return  ((TcpConnection * )pTcp)->bConnected;
	return FALSE;
}

void SetPTcpConnected( PTcpConnection pTcp ) {
	((TcpConnection * )pTcp)->bConnected = TRUE;
}

BOOL GetPortInViaSentBy ( char * pSentBy, unsigned short * pusPort );
BOOL GetCopyOfIPInViaSentBy ( char * pSentBy, char ** pstrTargetIP );
RCODE SendResponse2 ( TxStruct TxData );

/* Send out TxData->OriginalRequest;
 * return RC_OK if success
 *	  RC_ERROR if fail	  
 */
RCODE SendRequest(TxStruct TxData)
{
	RCODE retcode;
	SipReq req = NULL;
	BOOL bPreparedForStrictRouter;

	if (!TxData || !TxData->OriginalReq)
		return RC_ERROR;

	req = sipReqDup( TxData->OriginalReq );


	/* ljchuang 2005/03/02
	retcode = SendRequestEx ( TxData->OriginalReq, 
		TxData->bOriginalReq_PreparedForStrictRouter, 
		TxData );
	 */

	PrepareRoute ( req, &bPreparedForStrictRouter );

	retcode = SendRequestEx ( req, 
				  bPreparedForStrictRouter, 
				  TxData );

	if (retcode == RC_ERROR)
		TXERR0("[TRANSPORT] Sending request fail !!! \n");

	sipReqFree( req );
	req = NULL;

	return retcode;
}

BOOL GetHostPortFromString ( char * strAddr, char ** pstrHost, unsigned short * pusPort ) {
	SipURL url = NULL;
	if ( (!strAddr) || (!pstrHost) || (!pusPort) ) {
		return FALSE;
	}
	url = sipURLNew ();
	if ( url ) {
		if ( sipURLGetHost(url) ) {
			*pstrHost = strDup ( sipURLGetHost(url) );
		}
		
		if ( sipURLGetPort ( url ) ) {
			*pusPort = sipURLGetPort ( url );
		}
		
		sipURLFree(url); /* ljchuang */
		return TRUE;
	}

	return FALSE;
}

BOOL IsTransportSupported ( SipURL url ) {
	int i = 0;
	int iCount = sipURLGetParamSize ( url );
	assert ( url );

	for ( i = 0; i < iCount; ++i ) {
		SipURLParam * pParam = sipURLGetParamAt ( url, i );
		if ( pParam ) {
			if ( 0 == strICmp ( pParam->pname_, "transport" ) ) {
				if ( 0 == strICmp ( pParam->pvalue_, "tcp" ) ) {
					return TRUE;
				} 
				if ( 0 == strICmp ( pParam->pvalue_, "udp" ) ) {
					return TRUE;
				} 
				if ( 0 == strICmp ( pParam->pvalue_, "tls" ) ) {
					return TRUE;
				}
				return FALSE;
			}
		}
	}
	return TRUE;
}

BOOL IsTransportEqualTcp ( SipURL url ) {
	int i = 0;
	int iCount = sipURLGetParamSize ( url );
	assert ( url );

	for ( i = 0; i < iCount; ++i ) {
		SipURLParam * pParam = sipURLGetParamAt ( url, i );
		if ( pParam ) {
			if ( 0 == strICmp ( pParam->pname_, "transport" ) ) {
				if ( 0 == strICmp ( pParam->pvalue_, "tcp" ) ) {
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

BOOL IsTransportEqualTls( SipURL url ) {
	int i = 0;
	int iCount = sipURLGetParamSize ( url );
	assert ( url );

	for ( i = 0; i < iCount; ++i ) {
		SipURLParam * pParam = sipURLGetParamAt ( url, i );
		if ( pParam ) {
			if ( 0 == strICmp ( pParam->pname_, "transport" ) ) {
				if ( 0 == strICmp ( pParam->pvalue_, "tls" ) ) {
					sipURLDelParamAt( url, i );
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

BOOL IsSecure ( SipURL url ) {
	char* scheme = sipURLGetScheme ( url );
	
	assert ( url );

	if ( 0 == strICmp( scheme, "sips" ) )
		return TRUE;

	return FALSE;
}

BOOL ParseTargetURI ( char * strSrc, char ** strHost, unsigned short * pusPort, 
		BOOL * pbTransportSupported, BOOL * pbTransportTCP, BOOL * pbSecure ) {

	SipURL url = NULL;
	
	assert ( strSrc );
	assert ( strHost );
	assert ( pusPort );
	assert ( pbTransportSupported );
	assert ( pbTransportTCP );
	assert ( pbSecure );

	url = sipURLNewFromTxt ( strSrc );
	if ( url ) {
		*strHost = strDup ( sipURLGetHost ( url ) );
		
		*pbTransportSupported = IsTransportSupported ( url );
		*pbTransportTCP = IsTransportEqualTcp ( url );
		*pbSecure = IsSecure ( url ) || IsTransportEqualTls ( url );

		if ( sipURLGetPort ( url ) ) {
			*pusPort = sipURLGetPort ( url );
		} else {
			if (*pbSecure)
				*pusPort = 5061;
			else
				*pusPort = 5060;
		}
		
		sipURLFree ( url );
		url = NULL;
		return TRUE;
	}
	return FALSE;
}

RCODE SendRequestEx (SipReq pMsg, BOOL bPreparedForStrictRouter, TxStruct TxData)
{
	RCODE result = RC_ERROR;
	char * strHost = NULL;
	unsigned short usPort = 0;
	BOOL bTransportSupported = FALSE;
	BOOL bTransportTCP = FALSE;
	BOOL bSecure = FALSE;
	SipURL url = NULL;
	SipUDP tmpudp = NULL;
	char tmpstr[64];

	/* determine target address by the following order : proxy, 
	route(if not bPreparedForStrictRouter), request-uri */

	/* try to use an outbound proxy if has been assigned */
	if ( TxData ) {
		if ( TxData->profile ) {
			if ( TxData->profile->useproxy ) {
				ParseTargetURI ( TxData->profile->proxyaddr, 
					&strHost, &usPort, &bTransportSupported, &bTransportTCP, &bSecure );
			}
		}
	}

	/* if not bPreparedForStrictRouter, try route header */
	if ( ( FALSE == bPreparedForStrictRouter ) && ( ! strHost ) ) {
		SipRoute * pRouteHeader = NULL;
		pRouteHeader = sipReqGetHdr ( pMsg, Route );
		if ( pRouteHeader ) {
			if ( pRouteHeader->sipNameAddrList ) {
				/* get the next hop assigned in the route header */
				SipRecAddr * pNextHop = pRouteHeader->sipNameAddrList;
				
				if ( pNextHop->addr ) {
					ParseTargetURI (pNextHop->addr, 
						&strHost, &usPort, &bTransportSupported, &bTransportTCP, &bSecure );
				}
			}
		}
	}

	/* use request uri */
	if ( ! strHost ) {
		SipReqLine * pReqLine = NULL;
		pReqLine = sipReqGetReqLine (pMsg);
		if ( pReqLine ) {
			if ( pReqLine->ptrRequestURI ) {
				ParseTargetURI (pReqLine->ptrRequestURI, 
					&strHost, &usPort, &bTransportSupported, &bTransportTCP, &bSecure );
			}
		}
		url = sipURLNewFromTxt ( pReqLine->ptrRequestURI );
		if (url && IsTransportEqualTls( url ) ) {
			free( pReqLine->ptrRequestURI );
			pReqLine->ptrRequestURI = strDup( sipURLToStr ( url ) );
		}
		sipURLFree( url );
		url = NULL;
	}

	if ( ( ! strHost ) || ( ! bTransportSupported ) ) {
		if( NULL !=  strHost)
			free(strHost);
		TXERR0("[TRANSPORT] Destination URI parse fail !!! \n");
		return RC_ERROR;
	}

	if ( TCP == TxData->txptype )
		bTransportTCP = TRUE;

	if ( strHost ) {

		trimWS( strHost );

		/* Do DNS query */
		if ( !ResolveName( &strHost ) )
		{
			free(strHost);
			TXERR0("[TRANSPORT] Address resolution fail !!! \n");
			return RC_ERROR;
		}

		/* check if somewhere upstream 
		has specified that this transaction must use tcp,
		or the traget address indicates that we should use tcp */
		if ( bTransportTCP || bSecure ) {
			unsigned long ulIP = INADDR_NONE;

			/* enforce the transaction to use TCP */

			if (bSecure)
				TxData->txptype = TXPTLS;
			else
				TxData->txptype = TCP;
			
			ulIP = inet_addr ( strHost );
			
			if ( ! TxData->transport ) {

				if (TxData->profile && (TxData->profile->localip))
					TxData->transport = _RequireTcpConnection ( ulIP, usPort, bSecure, TxData->profile->localip );
				else
					TxData->transport = RequireTcpConnection ( ulIP, usPort, bSecure );
			}
				
			if ( !IsPTcpConnected(TxData->transport) ){
				/* add by tyhuang */
				free(strHost);
				return RC_TCP_CONNECTING;
			}
		}

		/* check if we already has a tcp connection, or somewhere upstream 
		has specified that this transaction must use tcp,
		or the traget address indicates that we should use tcp */
		if ( (TCP == TxData->txptype)  || (TXPTLS == TxData->txptype) ) {
			
			/* chech if somewhere upstream has assigned a 
			connection for this transaction */
			if (TxData->transport) {
				/* send via existing tcp */
				SipTCP pTCP = GetUnderlyingTcp(TxData->transport);
				if ( pTCP ) {
					/* remove existing via header */
					sipReqDelViaHdrTop ( pMsg );

					/* we have a valid tcp connection */
					/* insert via header with transport = TCP */
					FillVia ( pMsg, TxData );

					if (TXPTLS == TxData->txptype) {
						TXTRACE3("[TRANSPORT] Sending request to %s:%d (TLS) \n\n%s", strHost, usPort, sipReqPrint(pMsg) );

						if ( sipTLSSendReq ( (SipTLS)pTCP, pMsg ) ) {
							TXTRACE0("[TRANSPORT] ) sent successful \n" );
							result = RC_OK;
						} else {
							TXERR0("[TRANSPORT] Sent thru ) fail !!! \n");
							result = RC_ERROR;
						}
					
					} else {
									
						TXTRACE3("[TRANSPORT] Sending request to %s:%d (TCP) \n\n%s", strHost, usPort, sipReqPrint(pMsg) );

						if ( sipTCPSendReq ( pTCP, pMsg ) ) {
							TXTRACE0("[TRANSPORT] TCP sent successful \n" );
							result = RC_OK;
						} else {
							TXERR0("[TRANSPORT] Sent thru TCP fail !!! \n");
							result = RC_ERROR;
						}
					}
				} else {
					/* the associated/assigned tcp connection has be 
					dropped for some reason, return error */
					TXERR0("[TRANSPORT] TCP connection dropped \n");
					result = RC_ERROR;
				}
			}
		} else {
			/* remove existing via header */
			sipReqDelViaHdrTop ( pMsg );

			/* insert via header with transport = UDP */
			FillVia ( pMsg, TxData );

			TXTRACE3("[TRANSPORT] Sending request to %s:%d (UDP) \n\n%s", strHost, usPort, sipReqPrint(pMsg) );


			if(TxData->profile){
				if (TxData->profile->localip) {
					sprintf(tmpstr, "%s:%d", TxData->profile->localip, TxData->profile->localport);
					tmpudp = dxHashItem(sipUDPTable, tmpstr);

					if (tmpudp == NULL) {
						tmpudp = sipUDPNew(TxData->profile->localip, TxData->profile->localport);
						dxHashAdd(sipUDPTable, tmpstr, tmpudp);
					}
				}else
					tmpudp = g_sipUDP;
			} else {
				/* use the global udp socket for transportation */
				tmpudp = g_sipUDP;
			}
			
			if  ( sipUDPSendReq ( tmpudp, pMsg, strHost, usPort ) ) {
				TXTRACE0("[TRANSPORT] UDP sent successful \n" );
				result = RC_OK;
			} else {
				TXERR0("[TRANSPORT] Sent thru UDP fail !!! \n");
				result = RC_ERROR;
			}
		}
		free ( strHost );
		strHost = NULL;
	}

	return result;
}


RCODE PrepareRoute ( SipReq pMsg, BOOL * pbPreparedForStrictRouter ) {
	if ( pMsg && pbPreparedForStrictRouter ) {
		SipReqLine * pReqLine = NULL;
		SipRoute * pRouteHeader = NULL;

		pReqLine = sipReqGetReqLine (pMsg);
		if ( ! pReqLine ) {
			return RC_ERROR;
		}

		pRouteHeader = sipReqGetHdr ( pMsg, Route );
		/* check the first item in the route set fot strict/loose router */
		if ( pRouteHeader ) {
			if ( pRouteHeader->sipNameAddrList ) {
				/* get the next hop assigned in the route header */
				SipRecAddr * pNextHop = pRouteHeader->sipNameAddrList;
				/* SipAddr * pRouteItem = pNextHop; */
				SipURL url = NULL;
				int iNumParam = 0;
				int i = 0;
				char * strRouteFromRequestURI = NULL;
				SipReqLine newReqestLine;
				
				if ( ! pNextHop->addr ) {
					return RC_ERROR;
				}

				url = sipURLNewFromTxt ( pNextHop->addr );
				if ( ! url ) {
					return RC_ERROR;
				}

				iNumParam = sipURLGetParamSize ( url );
				for ( i = 0; i < iNumParam; ++i ) {
					SipURLParam * pParam = sipURLGetParamAt ( url, i );
					if ( pParam ) {
						if ( 0 == strICmp ( pParam->pname_, "lr" ) ) {
							/* the next hop is a loose router, 
							leave the message as is */
							sipURLFree ( url );
							url = NULL;
							*pbPreparedForStrictRouter = FALSE;
							return RC_OK;
						}
					}
				}
				sipURLFree ( url );
				url = NULL;
				
				/* add a route item (from request uri) at the tail of the route set */
				strRouteFromRequestURI = (char *)malloc ( 
					strlen ( pReqLine->ptrRequestURI ) + 16 );
				sprintf ( strRouteFromRequestURI, "Route:<%s>", pReqLine->ptrRequestURI );

				sipReqAddRouteHdr ( pMsg, 1, strRouteFromRequestURI );

				/* construct a new request line */
				memset ( &newReqestLine, 0, sizeof(newReqestLine) );
				newReqestLine.iMethod = pReqLine->iMethod;
				newReqestLine.ptrRequestURI = strDup ( pNextHop->addr );
				newReqestLine.ptrSipVersion = strDup ( pReqLine->ptrSipVersion );
				sipReqSetReqLine ( pMsg, &newReqestLine );
				free(newReqestLine.ptrRequestURI);
				free(newReqestLine.ptrSipVersion);

				/* remove the first in route set */
				sipReqDelRouteHdrTop ( pMsg );

				/* Set bPreparedForStrictRouter flag */
				*pbPreparedForStrictRouter = TRUE;
			}
		}
		/* if there is no Route header at all, thus
		there is no strict/loose router problem */
		else {
			*pbPreparedForStrictRouter = FALSE;
			return RC_OK;
		}
	}
	return RC_ERROR;
}

void UpdateDigest ( MD5_CTX * pContext, char * str ) {
	if ( pContext && str ) {
		MD5Update ( pContext, (unsigned char*)str, strlen(str) );
	}
}

/* this is not a full implementation of the branch calculation */
char * CalculateBranch ( SipReq pMsg ) {
	unsigned char	md5_H[16];
	char	H_Hex[33], buffer[10];
	MD5_CTX	context;
	int i = 0;
	static char strBranch[64];
	static unsigned short gencount;
	SipReqLine * pRequestLine = NULL;
	SipCSeq * pCSeq = NULL;

	memset ( strBranch, 0, sizeof(strBranch) );
	memset ( md5_H, 0, sizeof(md5_H) );
	memset ( H_Hex, 0, sizeof(H_Hex) );

	MD5Init (&context);

	/* CALL-ID */
	UpdateDigest ( &context, (char *)sipReqGetHdr(pMsg, Call_ID) );

	/* CSeq number */
	pCSeq = sipReqGetHdr ( pMsg, CSeq );
	if ( pCSeq ) {
		char str[16];
		memset ( str, 0, sizeof(str) );
		sprintf ( str, "%u", pCSeq->seq );
		UpdateDigest ( &context, str );
	}

	/* Request URI */
	pRequestLine = sipReqGetReqLine ( pMsg );
	if ( pRequestLine ) {
		UpdateDigest ( &context, pRequestLine->ptrRequestURI );
	}

	/* Method Type */
	/* added by ljchuang 2003/8/19 */
	/* quick fix for SiPit 13 */
	if ( pRequestLine ) {
		UpdateDigest ( &context, (char*)sipMethodTypeToName(pRequestLine->iMethod) );
	}

	/* Random number */
#ifndef _WIN32_WCE
	/* srand( (unsigned)time( NULL ) ); */
	srand( (unsigned)clock() );
	i2a(rand()+gencount, buffer, 10);
	gencount=(++gencount)%1000;
#else
	i2a(gencount, buffer, 10);
	UpdateDigest( &context, buffer);
	gencount=(++gencount)%1000;
	memset(buffer,0,10);

	i2a(Random()+gencount, buffer, 10);
	gencount=(++gencount)%1000;
#endif

	UpdateDigest( &context, buffer);

	MD5Final (md5_H, &context);

#ifndef CCLSIP_SIPTX_SHORTBRANCH
	for ( i=0; i<16; i++)
		sprintf( ( H_Hex+(i<<1) ), "%02x", md5_H[i]);
	H_Hex[32] = 0;
	
	sprintf ( strBranch, "z9hG4bK%s", H_Hex );

#else
	for ( i=0; i<8; i++)
		sprintf( ( H_Hex+(i<<1) ), "%02x", md5_H[i<<1]);
	H_Hex[16] = 0;
		
	sprintf ( strBranch, "z9hG4bK%s", H_Hex );
#endif

	return strBranch;
}

/* modified by ljchuang 2003/8/19 */
/* quick fix for SiPit 13 */
BOOL FillVia ( SipReq pMsg, TxStruct TxData ) {
	SipReqLine *reqline = NULL;
	SipRspStatusLine *pLine = NULL;
	SipMethodType Method;

	if ( pMsg ) {
		char strVia [128]={'\0'};

		char * transport = NULL;

		switch ( TxData->txptype ) {
			case UDP:
				transport = "UDP";
				break;
			case TCP:
				transport = "TCP";
				break;

			case TXPTLS:
				transport = "TLS";
				break;

			default:
				transport = "UNKNOWN";
				break;
		}

		if ( TxData->txid ) {
			
			reqline = sipReqGetReqLine(pMsg);
			
			if ( reqline ) {
				Method = reqline->iMethod;
			} else {
				Method = sipTxGetMethod( TxData ); /* default */
			}

			if ( Method == ACK && TxData->LatestRsp )
			{
				pLine = sipRspGetStatusLine( TxData->LatestRsp );
				if ( pLine && ( a2i( pLine->ptrStatusCode ) == 200 )){
					sprintf ( strVia, "Via:SIP/2.0/%s %s:%d;rport;branch=%s",
						transport,
						sipTxGetLocalAddr(), 
						(int)sipTxGetLocalPort(), 
						CalculateBranch (pMsg) );
				}
			}

			if ( strlen(strVia) == 0 )
			{
				sprintf ( strVia, "Via:SIP/2.0/%s %s:%d;rport;branch=%s", 
					transport,
					sipTxGetLocalAddr(), 
					(TxData->txptype == TXPTLS)? (int)sipTxGetLocalPort()+1:(int)sipTxGetLocalPort(), 
					TxData->txid );
			}
		} else {
			sprintf ( strVia, "Via:SIP/2.0/%s %s:%d;rport", 
				transport,
				sipTxGetLocalAddr(), 
				(TxData->txptype == TXPTLS)? (int)sipTxGetLocalPort()+1:(int)sipTxGetLocalPort() );
		}

		sipReqAddViaHdrTop ( pMsg, strVia );
		
		return TRUE;
	}

	return FALSE;
}


/* Send out TxData->LatestResponse;
 * return RC_OK if success
 *	  RC_ERROR if fail	  
 */
RCODE SendResponse(TxStruct TxData)
{
	SipRsp pMsg = NULL;
	SipViaParm * pVia = NULL;
	RCODE retcode;

	/* validate argument */
	if ( NULL == TxData ) {
		return RC_ERROR;
	}

	pMsg = sipTxGetLatestRsp ( TxData );

	/* validate the response */
	if ( NULL == pMsg ) {
		return RC_ERROR;
	}

	pVia = GetTopMostVia ( pMsg, FALSE );

	if ( NULL == pVia ) {
		TXERR0("[TRANSPORT] Can't send response: no via header !!! \n");
		return RC_ERROR;
	}

	if ( NULL == pVia->transport ) {
		TXERR0("[TRANSPORT] Can't send response: no transport type !!! \n");
		return RC_ERROR;
	} 
	else if ( 0 == strICmp ( pVia->transport, "UDP" ) ) {
		retcode = SendResponse2 ( TxData );
	} 
	else if ( 0 == strICmp ( pVia->transport, "TCP" ) ) {
		retcode = SendResponse2 ( TxData );
	}
	else if ( 0 == strICmp ( pVia->transport, "TLS" ) ) {
		retcode = SendResponse2 ( TxData );
	}
	else {
		TXERR0("[TRANSPORT] Can't send response: unknown transport type !!! \n");
		return RC_ERROR;
	}

	if (retcode == RC_ERROR)
		TXERR0("[TRANSPORT] Sending response fail !!! \n");

	return retcode;
}

/* assume the related pointers has been checked */
RCODE SendResponse2 ( TxStruct TxData ) {
	BOOL bAssigned_maddr = FALSE;
	BOOL bAssigned_Received = FALSE;
	char * strTargetIP = NULL;
	char * tmpstr = NULL;
	SipRsp pMsg = NULL;

	SipViaParm * pVia = NULL;
	unsigned short usTargetPort = 5060;
	RCODE result = RC_ERROR;

	assert ( TxData );

	pMsg = sipTxGetLatestRsp ( TxData );

	assert ( pMsg );

	/* marked by tyhuang 2006/2/16 
	sipRspDelAllViaHdr( pMsg );
	
	pVia = (SipViaParm*) sipReqGetHdr( pReq, Via );

	sipRspAddHdr( pMsg, Via, pVia );
	*/

	pVia = GetTopMostVia ( pMsg, FALSE );

	assert ( pVia );

	if ( pVia->maddr ) {
		bAssigned_maddr = TRUE;
		strTargetIP = strDup( pVia->maddr );
	} else if ( pVia->received ) {
		bAssigned_Received = TRUE;
		strTargetIP = strDup( pVia->received );
	} else {
		GetCopyOfIPInViaSentBy ( pVia->sentby, &strTargetIP );

		if ( ! strTargetIP ) {
			TXERR0("[TRANSPORT] no target IP address in Via header !!! \n");
			return RC_ERROR;
		}
	}

	if ( pVia->rport > 0) {
		usTargetPort = pVia->rport;
	} else if ( pVia->sentby ) {
		GetPortInViaSentBy ( pVia->sentby, &usTargetPort );
	}

	if ( strTargetIP ) {
		trimWS( strTargetIP );

		/* Do DNS query */
		if ( !ResolveName( &strTargetIP ) )
		{
			TXERR0("[TRANSPORT] Address resolution fail !!! \n");
			return RC_ERROR;
		}
	}

	if ( 0 == strICmp ( pVia->transport, "TLS" ) ) {
		SipTCP pTCP = NULL;
		TxData->txptype = TXPTLS;
		if ( ! TxData->transport )
			TxData->transport = RequireTcpConnection ( inet_addr(strTargetIP), usTargetPort, TRUE );

		if ( !IsPTcpConnected(TxData->transport) )
			return RC_OK;

		pTCP = GetUnderlyingTcp(TxData->transport);

		tmpstr = (char*) sipTLSGetRaddr((SipTLS)pTCP);
		sipTLSGetRport((SipTLS)pTCP, &usTargetPort);

		TXTRACE3("[TRANSPORT] Sending response to %s:%d (TLS) \n\n%s", tmpstr, usTargetPort, sipRspPrint(pMsg) );

		if ( pTCP ) {
			if ( sipTLSSendRsp ( (SipTLS)pTCP, pMsg ) ) {
				TXTRACE0("[TRANSPORT] TLS sent successful \n");
				result = RC_OK;
			}
			else {
				TXERR0("[TRANSPORT] Sent thru TLS fail !!! \n");
				result = RC_ERROR;
			}
		}
		else {
			TXERR0("[TRANSPORT] TLS connection dropped \n");
			result = RC_ERROR;
		}
	}
	if ( 0 == strICmp ( pVia->transport, "TCP" ) ) {
		SipTCP pTCP = NULL;
		TxData->txptype = TCP;
		if ( ! TxData->transport )
			TxData->transport = RequireTcpConnection ( inet_addr(strTargetIP), usTargetPort, FALSE );

		if ( !IsPTcpConnected(TxData->transport) )
			return RC_OK;

		pTCP = GetUnderlyingTcp(TxData->transport);

		tmpstr = (char*) sipTCPGetRaddr(pTCP);
		sipTCPGetRport(pTCP, &usTargetPort);

		TXTRACE3("[TRANSPORT] Sending response to %s:%d (TCP) \n\n%s", tmpstr, usTargetPort, sipRspPrint(pMsg) );

		if ( pTCP ) {
			if ( sipTCPSendRsp ( pTCP, pMsg ) ) {
				TXTRACE0("[TRANSPORT] TCP sent successful \n");
				result = RC_OK;
			}
			else {
				TXERR0("[TRANSPORT] Sent thru TCP fail !!! \n");
				result = RC_ERROR;
			}
		}
		else {
			TXERR0("[TRANSPORT] TCP connection dropped \n");
			result = RC_ERROR;
		}
	} else if ( 0 == strICmp ( pVia->transport, "UDP" ) ) {
		strTargetIP = trimWS(strTargetIP);

		TXTRACE3("[TRANSPORT] Sending response to %s:%d (UDP) \n\n%s", strTargetIP, usTargetPort, sipRspPrint(pMsg) );

		if ( sipUDPSendRsp ( g_sipUDP, pMsg, strTargetIP, usTargetPort ) ) {
			TXTRACE0("[TRANSPORT] UDP sent successful \n");
			result = RC_OK;
		} else {
			TXERR0("[TRANSPORT] Sent thru UDP fail !!! \n");
			result = RC_ERROR;
		}
	}

	if ( strTargetIP ) {
		free ( strTargetIP );
	}

	return result;
}

CCLAPI RCODE SendResponseEx( SipRsp pMsg, UserProfile profile )
{
	TxStruct _this;
	RCODE retcode;

	if ( NULL == pMsg )
		return RC_ERROR;

	_this = sipTxHolderNew( pMsg, profile );

	retcode = SendResponse( _this );

	sipTxFree(_this);
	_this = NULL;

	return retcode;
}

/* Send out TxData->Ack;
 * return RC_OK if success
 *	  RC_ERROR if fail	  
 */
RCODE SendACK(TxStruct TxData)
{
	RCODE retcode;
	SipReq ack = NULL;
	BOOL bPreparedForStrictRouter;

	if (!TxData || !TxData->Ack) 
		return RC_ERROR;

	ack = sipReqDup( TxData->Ack );

	/* ljchuang 2005/03/02
	retcode = SendRequestEx ( TxData->Ack, 
				TxData->bAck_PreparedForStrictRouter, 
				TxData );
  	 */
	PrepareRoute ( ack, &bPreparedForStrictRouter );

	retcode = SendRequestEx ( ack, 
				  bPreparedForStrictRouter, 
				  TxData );

	if (retcode == RC_ERROR)
		TXERR0("[TRANSPORT] Sending ack fail !!! \n");

	sipReqFree( ack );
	ack = NULL;

	return retcode;
}

BOOL GetPortInViaSentBy ( char * pSentBy, unsigned short * pusPort ) {
	if ( pusPort && pSentBy ) {
		char * strPort = NULL;
		unsigned int uiPort = 0;
		strPort = strrchr ( pSentBy, ':' );
		*pusPort = 5060;

		/* retrun false if we cannot find the colon */
		if ( NULL == strPort ) {
			return FALSE;
		}
		/* make sure it's not the last character in the string */
		else if ( 0 == *(strPort + 1) ) {
			return FALSE;
		}

		if ( 1 != sscanf ( strPort + 1, "%u", &uiPort ) ) {
			return FALSE;
		}

		*pusPort = (unsigned short)uiPort;

		return TRUE;

	}
	return FALSE;
}

BOOL GetCopyOfIPInViaSentBy ( char * pSentBy, char ** pstrTargetIP ) {
	if ( pstrTargetIP && pSentBy ) {
		char * strColon = NULL;
		strColon = strrchr ( pSentBy, ':' );
		*pstrTargetIP = NULL;

		if ( strColon ) {
			const size_t lenIP = strColon - pSentBy;
			/* make sure the colon is not the first character */
			if ( lenIP ) {
				*pstrTargetIP = (char*)malloc ( lenIP + 1 );
				if ( *pstrTargetIP ) {
					
					memset ( *pstrTargetIP, 0, lenIP + 1 );
					strncpy ( *pstrTargetIP, pSentBy, lenIP );
					trimWS ( *pstrTargetIP );
					return TRUE;
				}
			}
		}
		else {
			*pstrTargetIP = strDup ( pSentBy );
		}
	}
	return FALSE;
}

BOOL ResolveName( char ** strHost )
{
	struct in_addr stHostAddr;
	struct in_addr ** pAddrList;
	const struct in_addr * pAddr;
	struct hostent* pHost;

	if ( !strHost)
		return FALSE;

	if ( ! *strHost )
		return FALSE;
	
	stHostAddr.s_addr = inet_addr( *strHost );

	if (stHostAddr.s_addr == INADDR_NONE)
	{
		pHost = gethostbyname(*strHost);

		if( NULL != pHost ) {
			pAddrList = (struct in_addr **)(pHost->h_addr_list);
			if ( NULL != (*pAddrList) ) {
				pAddr = *pAddrList;
				stHostAddr.s_addr = pAddr->s_addr;
			
				free ( *strHost );
				*strHost = strDup ( inet_ntoa(stHostAddr) );
				return TRUE;
			}
		}
		return FALSE;
	}

	return TRUE;
}

CCLAPI RCODE
KeepTCPConnection( char* iphost, unsigned short port )
{		
		unsigned long ulIP = INADDR_NONE;
		TcpConnection * pTCP;
		IN_ADDR addr;

		/* Do DNS query */
		if ( !ResolveName( &iphost ) )
		{
			TXERR0("[TRANSPORT] Address resolution fail !!! \n");
			return RC_ERROR;
		}

		ulIP = inet_addr ( iphost );
			
		pTCP = (TcpConnection *) RequireTcpConnection ( ulIP, port, FALSE );
		pTCP->bKeep = TRUE;
		pTCP->iRefCount--;

		/* ReleaseTcpConnection( (PTcpConnection) pTCP ); */

		addr.s_addr = pTCP->uRemoteIP;
		
		TXTRACE3("[KeepTCPConnection] Keep TCP connection %s:%d Refcount = %d \n",
			inet_ntoa(addr), pTCP->usRemotePort, pTCP->iRefCount);

		return RC_OK;
}

CCLAPI RCODE
UnKeepTCPConnection( char* iphost, unsigned short port )
{
		unsigned long ulIP = INADDR_NONE;
		TcpConnection * pTCP;
		IN_ADDR addr;

		/* Do DNS query */
		if ( !ResolveName( &iphost ) )
		{
			TXERR0("[TRANSPORT] Address resolution fail !!! \n");
			return RC_ERROR;
		}

		ulIP = inet_addr ( iphost );
			
		pTCP = (TcpConnection *) RequireTcpConnection ( ulIP, port, FALSE );
		pTCP->bKeep = FALSE;
		pTCP->iRefCount--;

		/* ReleaseTcpConnection( (PTcpConnection) pTCP ); */

		addr.s_addr = pTCP->uRemoteIP;
		
		TXTRACE3("[UnKeepTCPConnection] UnKeep TCP connection %s:%d Refcount = %d \n",
			inet_ntoa(addr), pTCP->usRemotePort, pTCP->iRefCount);

		return RC_OK;
}
