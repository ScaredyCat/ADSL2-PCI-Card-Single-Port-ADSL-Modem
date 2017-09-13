/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * cx_sock.c
 *
 * $Id: cx_sock6.c,v 1.4 2004/10/08 06:07:39 txyu Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* I have to include this file before cm_def.h to prevent it to include old winsock.h */
#ifdef _WIN32
#include <winsock2.h>
#endif
/* I have to include this file before cx_ex header to prevent some symbols from re-definition */
#include <common/cm_def.h>

#include <cx_ex/CxSocketEx.h>
#include <cx_ex/InetAddress.h>
#include <cx_ex/CxExTrace.h>

#include <low/cx_sock.h>

#include <common/cm_trace.h>

#ifdef _WIN32
static int	need_winsock_cleanup = 0;
#endif

static void 
funcCxExTrace (	const int iTraceLevel, 
		const char* strFormat, va_list arg_ptr )
{
	int iConvertedTraceLevel = TRACE_LEVEL_ERROR;
	char strBuf[1024];
	memset ( strBuf, 0, 1024 );

#ifdef _WIN32
	_vsnprintf ( strBuf, 1023, strFormat, arg_ptr );
#else
	vsnprintf ( strBuf, 1023, strFormat, arg_ptr );
#endif
	switch ( iTraceLevel ) {
		case CxExTrace_ERROR:
			iConvertedTraceLevel = TRACE_LEVEL_ERROR;
			break;
		case CxExTrace_WARNING:
			iConvertedTraceLevel = TRACE_LEVEL_WARNING;
			break;
	}

	TCRPrint ( iConvertedTraceLevel, "%s\n", strBuf );
}

RCODE 
cxSockInit ()
{
	g_funcCxExTrace = funcCxExTrace;

	if ( 0 == InitializeSocketLibrary () ) {
		return RC_OK;
	}
	return RC_ERROR;
}
 
RCODE 
cxSockClean ()
{
	if ( 0 == FinalizeSocketLibrary () ) {
		return RC_OK;
	}
	return RC_ERROR;
}

/* Fun: cxSockAddrNew
 *
 * Desc: addr - IP address or domain name, auto-fill local IP addr if NULL.
 *
 * Ret: NULL if error.
 */
CCLAPI CxSockAddr 
cxSockAddrNew (const char* addr, UINT16 port)
{
	int iPassive = 0;
	IPEndPoint * pEndPoint = NULL;

	if ( ( NULL == addr ) && ( 0 == port ) ) {
		CxExTrace0 ( CxExTrace_WARNING, 
			"Invalid argument, addr and uport may not be both zero(NULL)" );
	}

	/*
	The interface of this function doesn't have a "passive" option, so
	I use the NULL address as an indication. not very good, 
	but no better estimation for now
	*/
	if ( NULL == addr ) {
		iPassive = 1;
	}

	if ( RC_OK != CreateIPEndPointFromAddrPort ( &pEndPoint, addr, port, iPassive, SOCK_STREAM ) ) {
		CxExTrace0 ( CxExTrace_ERROR, "Cannot create IPEndInfo from addr and port" );

		return NULL;
	}

	return (CxSockAddr)pEndPoint;
}

CCLAPI CxSockAddr 
cxSockAddrNewCxSock (const char* addr, UINT16 port, CxSock local_socket)
{
	int iPassive = 0;
	IPEndPoint * pEndPoint = NULL;
	const CxSocketEx *pSocket = (CxSocketEx *) local_socket; 

	if ( ( NULL == addr ) && ( 0 == port ) ) {
		CxExTrace0 ( CxExTrace_WARNING, 
			"Invalid argument, addr and uport may not be both zero(NULL)" );
	}

	/*
	The interface of this function doesn't have a "passive" option, so
	I use the NULL address as an indication. not very good, 
	but no better estimation for now
	*/
	if ( NULL == addr ) {
		iPassive = 1;
	}

	if ( RC_OK != CreateIPEndPointFromAddrPortAiFamily ( &pEndPoint, addr, port, iPassive, SOCK_STREAM, pSocket->m_pLocalEndPoint->m_sockaddr.ss_family) ) {
		CxExTrace0 ( CxExTrace_ERROR, "Cannot create IPEndInfo from addr and port" );

		return NULL;
	}

	return (CxSockAddr)pEndPoint;
}


CCLAPI RCODE 
cxSockAddrFree(CxSockAddr _this)
{
	IPEndPoint * pEndPoint = (IPEndPoint*) _this;

	if ( ! _this ) {
		CxExTrace0 ( CxExTrace_ERROR, "Invalid argument" );
		return RC_ERROR;
	}

	if ( RC_OK != DestroyIPEndPoint ( &pEndPoint ) ) {
		CxExTrace0 ( CxExTrace_ERROR, "Cannot destroy IPEndPoint" );
		return RC_ERROR;
	}

	return RC_OK;
}

CCLAPI CxSockAddr 
cxSockAddrDup(CxSockAddr _this)
{
	IPEndPoint * pSrcEndPoint = (IPEndPoint *)_this;
	IPEndPoint * pDestEndPoint = NULL; 

	if ( ! _this ) {
		CxExTrace0 ( CxExTrace_ERROR, "Invalid argument" );
		return NULL;
	}

	if ( RC_OK == DulicateIPEndPoint ( &pDestEndPoint, pSrcEndPoint ) ) {
		return (CxSockAddr)pDestEndPoint;
	}
	else {
		CxExTrace0 ( CxExTrace_ERROR, "Cannot duplicate IPEndPoint" );
	}
	return NULL;
}

CCLAPI char* 
cxSockAddrGetAddr(CxSockAddr _this)
{
	IPEndPoint * pEndPoint = (IPEndPoint*) _this;

	if ( ! _this ) {
		CxExTrace0 ( CxExTrace_ERROR, "Invalid argument" );
		return NULL;
	}

	if ( pEndPoint->m_strResolvedName ) {
		return pEndPoint->m_strResolvedName;		
	}
	else if ( pEndPoint->m_strNumericName ) {
		return pEndPoint->m_strNumericName;		
	}

	if ( RC_OK != ResolveNameFromEndPoint ( pEndPoint, 1 ) ) {
		CxExTrace0 ( CxExTrace_ERROR, "Unable to resolve IPEndPoint" );
		return NULL;
	}

	if ( pEndPoint->m_strResolvedName ) {
		return pEndPoint->m_strResolvedName;		
	}
	else if ( pEndPoint->m_strNumericName ) {
		return pEndPoint->m_strNumericName;		
	}

	return NULL;
}

CCLAPI
UINT16 cxSockAddrGetPort(CxSockAddr _this)
{
	IPEndPoint * pEndPoint = (IPEndPoint*) _this;

	if ( ! _this ) {
		CxExTrace0 ( CxExTrace_ERROR, "Invalid argument" );
		return 0;
	}
	return (UINT16) (pEndPoint->m_uPort);
}

struct sockaddr_in* 
cxSockAddrGetSockAddr(CxSockAddr _this)
{
	IPEndPoint * pEndPoint = (IPEndPoint*) _this;

	if ( ! _this ) {
		CxExTrace0 ( CxExTrace_ERROR, "Invalid argument" );
		return NULL;
	}
	return (struct sockaddr_in*) (&(pEndPoint->m_sockaddr));
}

/* Fun: cxSockNew
 *
 * Desc: Bind address to socket if laddr != NULL.
 *       Listen if a TCP server socket.
 *
 * Ret: NULL if error.
 */
CxSock 
cxSockNew(CxSockType type, CxSockAddr laddr)
{
	CxSocketEx * pSocket = NULL;
	ConnectionType new_type = CT_UNKNOWNON;
	IPEndPoint * pEndPoint = (IPEndPoint *)laddr;
/*
	if ( ! laddr ) {
		CxExTrace0 ( CxExTrace_ERROR, "Invalid argument" );
		return NULL;
	}
*/
	switch ( type ) {
		case CX_SOCK_DGRAM:
			new_type = CT_UDP_SERVER;
			break;
		case CX_SOCK_SERVER:
			new_type = CT_TCP_SERVER;
			break;
		case CX_SOCK_STREAM_C:
			new_type = CT_TCP_CLIENT;
			break;
		default:
			CxExTrace1 ( CxExTrace_ERROR, "Unknown socket type %d", (int)type );
			return NULL;
	}

	if ( RC_OK != CxExCreateSocket ( &pSocket, new_type, 1 ) ) {
		CxExTrace0 ( CxExTrace_ERROR, "Cannot create socket" );
		return NULL;
	}

	if ( pEndPoint ) {
		if ( RC_OK != CxExBindToIPEndPoint ( pSocket, pEndPoint ) ) {

			CxExTrace0 ( CxExTrace_ERROR, 
				"Cannot bind to given address" );

			if ( RC_OK != CxExDestroySocket ( &pSocket ) ) {

				CxExTrace0 ( CxExTrace_ERROR, 
					"Cannot destroy socket" );
			}
			return NULL;
		}
	}

	return (CxSock)pSocket;
}

RCODE 
cxSockFree(CxSock _this)
{
	CxSocketEx * pSocket = (CxSocketEx *)_this;
	if ( ! pSocket ) {
		CxExTrace0 ( CxExTrace_ERROR, "Invalid argument" );
		return RC_ERROR;
	}
	if ( RC_OK != CxExDestroySocket ( &pSocket ) ) {
		CxExTrace0 ( CxExTrace_ERROR, "Cannot destroy socket" );
		return RC_ERROR;
	}
	return RC_OK;
}

SOCKET 
cxSockGetSock(CxSock _this)
{
	CxSocketEx * pSocket = (CxSocketEx *)_this;
	if ( ! pSocket ) {
		CxExTrace0 ( CxExTrace_ERROR, "Invalid argument" );
		return INVALID_SOCKET;
	}
	return pSocket->m_fd;
}

RCODE 
cxSockSetSock(CxSock _this, SOCKET s)
{
	CxSocketEx * pSocket = (CxSocketEx *)_this;
	if ( ! pSocket ) {
		CxExTrace0 ( CxExTrace_ERROR, "Invalid argument" );
		return INVALID_SOCKET;
	}
	pSocket->m_fd = s;

	return RC_OK;
}

CxSockAddr 
cxSockGetRaddr(CxSock _this)
{
	CxSocketEx * pSocket = (CxSocketEx *)_this;
	if ( ! pSocket ) {
		CxExTrace0 ( CxExTrace_ERROR, "Invalid argument" );
		return NULL;
	}
	return (CxSockAddr)(pSocket->m_pRemoteEndPoint);
}

CxSockAddr 
cxSockGetLaddr(CxSock _this)
{
	CxSocketEx * pSocket = (CxSocketEx *)_this;
	if ( ! pSocket ) {
		CxExTrace0 ( CxExTrace_ERROR, "Invalid argument" );
		return NULL;
	}
	return (CxSockAddr)(pSocket->m_pLocalEndPoint);
}

CxSock 
cxSockAccept(CxSock _this)
{
	CxSocketEx * pSrcSocket = (CxSocketEx *)_this;
	CxSocketEx * pNewSocket = NULL;

	if ( ! pSrcSocket ) {
		CxExTrace0 ( CxExTrace_ERROR, "Invalid argument" );
		return NULL;
	}

	if ( RC_OK != CxExAccept ( &pNewSocket, pSrcSocket ) ) {
		CxExTrace0 ( CxExTrace_ERROR, "Failed to accept connection" );
		return NULL;
	}

	return (CxSock)pNewSocket;
}
				
RCODE 
cxSockConnect(CxSock _this, CxSockAddr raddr)
{
	CxSocketEx * const pSocket = (CxSocketEx *)_this;
	IPEndPoint * const pEndPoint = (IPEndPoint *)raddr;

	if ( ! (pSocket && pEndPoint) ) {
		CxExTrace0 ( CxExTrace_ERROR, "Invalid argument" );
		return RC_ERROR;
	}
	if ( RC_OK != CxExConnectToIPEndPoint ( pSocket, pEndPoint ) ) {
		CxExTrace0 ( CxExTrace_ERROR, "Cannot connect to remote end point" );
		return RC_ERROR;
	}
	return RC_OK;
}

/* Fun: cxSockSend
 *
 * Desc: Send 'len' of bytes from buffer.
 *
 * Ret: Return number of bytes sent, -1 on error.
 */
int 
cxSockSend(CxSock _this, const char* buf, int len)
{
	int iLen = len;
	CxSocketEx * const pSocket = (CxSocketEx *)_this;

	if ( ! (pSocket && buf && (len>=0) ) ) {
		CxExTrace0 ( CxExTrace_ERROR, "Invalid argument" );
		return RC_ERROR;
	}

	if ( RC_OK != CxExSend ( pSocket, buf, &iLen ) ) {
		CxExTrace0 ( CxExTrace_ERROR, "Unable to send data through socket" );
		return -1;
	}
	return iLen;
}

/* Fun: cxSockRecvEx
 *
 * Desc: Receive 'len' of bytes into buffer. Wait for $timeout msec.
 *	 Set $timeout==-1 for blocking mode.
 *
 * Ret: Return number of byets actually received when success, 
 *      0, connection closed.
 *      -1, error, 
 *      -2, timeout.
 */
int
cxSockRecvEx(CxSock _this, char* buf, int len, long timeout)
{
	int n, nrecv = 0;
	int ret = 0;
	int result;
	fd_set rset;
	struct timeval t;

	CxSocketEx * const pSocket = (CxSocketEx *)_this;

	if ( ! (pSocket && buf && (len >= 0) ) ) {
		CxExTrace0 ( CxExTrace_ERROR, "Invalid argument" );
		return RC_ERROR;
	}

	if ( ( pSocket->m_type != CT_TCP_SERVER ) && 
		( pSocket->m_type != CT_TCP_CLIENT ) ) {

		CxExTrace0 ( CxExTrace_ERROR, "Invalid socket type, must by TCP" );

		return -1;
	}

	if ( INVALID_SOCKET == pSocket->m_fd ) {

		CxExTrace0 ( CxExTrace_ERROR, "Invalid socket" );

		return -1;
	}

	for (nrecv = 0; nrecv < len; nrecv += n) {
		FD_ZERO(&rset);
		FD_SET(pSocket->m_fd, &rset);

		if( timeout<0 ) /* blocking mode */
			result = select(pSocket->m_fd + 1, &rset, NULL, NULL, NULL);
		else {
			t.tv_sec = timeout/1000;
			t.tv_usec = (timeout%1000)*1000;
			result = select(pSocket->m_fd + 1, &rset, NULL, NULL, &t);
		}
		/* Acer Modify SipIt 10 */
		/* should check if result ==0 Timeout */
		if ((result == SOCKET_ERROR)) {

			CxExTrace1 ( CxExTrace_WARNING, 
				"recv failed, error code : %d", 
				NET_ERROR_CODE );

			ret = -1;
			break;
		}
		else if ( result == 0 )  {
			ret = -2;
			break;
		}

		n = recv(pSocket->m_fd, buf + nrecv, len - nrecv, 0/*no flags*/);

		if (n == SOCKET_ERROR) {
			
			CxExTrace1 ( CxExTrace_WARNING, 
				"recv failed, error code : %d", 
				NET_ERROR_CODE );

			ret = -1;
			break;
		}

		if (n == 0) { /* connection closed */
			ret = 0;

			/* received data should be delivered to caller,
			   so, nrecv should remain its value. */
			/* nrecv = 0; */
			break;
		}
	}

	if (ret<0)
		return ret;
	else
		return nrecv;
}

/*
 *  RFC: 2126 (ISO Transport Service on top of TCP (ITOT))
 * 
 *	A TPKT consists of two part:
 *	
 *  - a Packet Header
 *  - a TPDU.
 *
 *  The format of the Packet Header is constant regardless of the type of
 *  TPDU. The format of the Packet Header is as follows:
 *
 *  +--------+--------+----------------+-----------....---------------+
 *  |version |reserved| packet length  |             TPDU             |
 *  +----------------------------------------------....---------------+
 *  <8 bits> <8 bits> <   16 bits    > <       variable length       >
 *
 *  where:
 *
 *  - Protocol Version Number
 *    length: 8 bits
 *    Value:  3
 *
 *  - Reserved
 *    length: 8 bits
 *    Value:  0
 *
 *  - Packet Length
 *    length: 16 bits
 *    Value:  Length of the entire TPKT in octets, including Packet
 *            Header
 */

/* Fun: cxSockTpktSend
 *
 * Desc: Send data with TPKT header.
 *
 * Ret: Return length of len + 4, if success. Otherwise, return -1.
 */
int 
cxSockTpktSend(CxSock _this, const char* buf, int len)
{
	int nSent, nTpktSent;
	unsigned char tpktHeader[4];

	tpktHeader[0] = 3;		/* Protocol Version Number, always 3 */
	tpktHeader[1] = 0;		/* Reserved, 0 */
	tpktHeader[2] = (len + 4) >> 8;	/* high byte of length */
	tpktHeader[3] = (len + 4);	/* low byte of length */

	if ((nTpktSent = cxSockSend(_this, (char *)tpktHeader, 4)) != -1) {
		if (nTpktSent == 4) {
			if ((nSent = cxSockSend(_this, buf, len)) != -1)
				nTpktSent = (nSent == len) ? (nTpktSent + nSent) : -1;
			else 
				nTpktSent = -1;
		} else 
			nTpktSent = -1;
	}

	return nTpktSent;
}

/* Fun: cxSockTpktRecv
 *
 * Desc: Receive data with TPKT header. Buffer will not include TPKT header.
 *
 * Ret: Return length of len + 4, if success. Otherwise, return -1.
 */
int 
cxSockTpktRecv(CxSock _this, char* buf, int len)
{
	int nRecv, nTPDU;
	unsigned char tpktHeader[4];

	if ((nRecv = cxSockRecv(_this, (char*)tpktHeader, 4)) != -1) {
		if (nRecv == 4) {
			nTPDU = (int) (( tpktHeader[2]) << 8)+tpktHeader[3];
			nTPDU -= 4;
			if ((nRecv = cxSockRecv(_this, buf, nTPDU)) != -1) {
				if (nRecv != nTPDU) nRecv = (nRecv > 0) ? -1 : 0;
			}
		} else 
			nRecv = (nRecv > 0) ? -1 : 0;
	} else 
		return 0;

	return nRecv;
}

int 
cxSockSendto(CxSock _this, const char* buf, int len, CxSockAddr raddr)
{
	int nsent;
	CxSocketEx * const pSocket = (CxSocketEx *)_this;
	IPEndPoint * pRemoteEndPoint = (IPEndPoint *)raddr;

	if ( ! (pSocket && buf && pRemoteEndPoint && (len >= 0) ) ) {
		CxExTrace0 ( CxExTrace_ERROR, "Invalid argument" );
		return RC_ERROR;
	}

	if ( (pSocket->m_type != CT_UDP_SERVER) &&
		(pSocket->m_type != CT_UDP_CLIENT) ) {

		return -1;
	}
 
	nsent = sendto( pSocket->m_fd, 
			buf, 
			len, 
			0, /* No flags */
			(struct sockaddr*)&(pRemoteEndPoint->m_sockaddr),
			pRemoteEndPoint->m_addrlen );

	if(nsent == SOCKET_ERROR) 
		return -1;

	return nsent;
}

/* Fun: cxSockRecvfrom
 *
 * Desc: Receive 'len' of bytes into buffer.
 *       Side effect - source address updated.
 *
 * Ret: Return number of byets actually received, -1 on error.
 */
int 
cxSockRecvfrom(CxSock _this, char* buf, int len)
{
	int iLen = len;
	CxSocketEx * const pSocket = (CxSocketEx *)_this;

	if ( ! (pSocket && buf && (len >= 0) ) ) {
		CxExTrace0 ( CxExTrace_ERROR, "Invalid argument" );
		return RC_ERROR;
	}

	if ( (pSocket->m_type != CT_UDP_SERVER) &&
		(pSocket->m_type != CT_UDP_CLIENT) ) {

		return -1;
	}

	if ( RC_OK == CxExRecv ( pSocket, buf, &iLen ) ) {
		return iLen;
	}
	else {
		return -1;
	}
}
