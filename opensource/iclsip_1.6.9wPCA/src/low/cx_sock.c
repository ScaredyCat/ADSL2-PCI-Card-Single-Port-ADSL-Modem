/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * cx_sock.c
 *
 * $Id: cx_sock.c,v 1.41 2006/08/28 08:36:36 tyhuang Exp $
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef UNIX
#include <errno.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#elif defined(_WIN32_WCE)
#elif defined(_WIN32)
#include <errno.h>
#endif
#include "cx_sock.h"
#include "cx_misc.h"
#include <common/cm_trace.h>

#ifndef EINVAL
#define EINVAL WSAEINVAL
#endif

#ifdef _WIN32
static int	need_winsock_cleanup = 0;
#endif
static int	sock_refcnt = 0;

static CxSockAddr	freeList = NULL;

struct cxSockAddrObj {
	char			addr_[32];
	UINT16			port_;
	struct sockaddr_in	sockaddr_;
	CxSockAddr		next_;
};

struct cxSockObj {
	SOCKET			sockfd_;
	CxSockType		type_;
	CxSockAddr		laddr_;
	CxSockAddr		raddr_;
};

void cxSockInit_(void) {} /* just for DLL forcelink */

RCODE cxSockInit()
{
	RCODE rc = RC_OK;
#ifdef _WIN32
	WSADATA	wsadata;
	SOCKET sock;
#endif
	if(++sock_refcnt>1)
		return RC_OK;

#ifdef _WIN32
	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
		if (WSAStartup(MAKEWORD(2, 1), &wsadata) == SOCKET_ERROR) {
			rc = RC_ERROR;
		} else
			need_winsock_cleanup = 1;
	} else
		closesocket(sock);
#endif
	freeList = NULL;

	return rc;
}
 
RCODE cxSockClean()
{
	CxSockAddr _this;
	RCODE rc = RC_OK;

	if(--sock_refcnt>0)
		return RC_OK;

#ifdef _WIN32
	if (need_winsock_cleanup)
		if (WSACleanup() == SOCKET_ERROR) {
			rc = RC_ERROR;
		}
#endif
	while (freeList != NULL) {
		_this = freeList;
		freeList = freeList->next_;
		free(_this);
	}
	sock_refcnt = 0;

	return rc;
}

static CxSockAddr cxSockAddrAlloc(void)
{
	CxSockAddr _this;

	if( freeList != NULL ) {
		_this        = freeList;
		freeList     = freeList->next_;
		_this->next_ = NULL;
	} else {
		_this = malloc(sizeof *_this);
		if (_this == NULL) 
			return NULL;
	}

	return _this;
}

/* Fun: cxSockAddrNew
 *
 * Desc: addr - IP address or domain name, auto-fill local IP addr if NULL.
 *
 * Ret: NULL if error.
 */
CCLAPI
CxSockAddr cxSockAddrNew(const char* addr, UINT16 port)
{
	struct hostent *he;
	CxSockAddr _this = cxSockAddrAlloc();

	if (_this == NULL) return NULL;

	memset((char *)&_this->sockaddr_, 0, sizeof(_this->sockaddr_));
	_this->sockaddr_.sin_family = AF_INET;
	_this->sockaddr_.sin_port = htons(port);

	if(addr == NULL) {
		_this->sockaddr_.sin_addr.s_addr = htonl(INADDR_ANY); /* auto-fill with local IP */		
	/*} else if (!isalpha((int)addr[0])) {*/
	} else if ( inet_addr(addr)!=-1 ) {
		_this->sockaddr_.sin_addr.s_addr = inet_addr(addr);	
	} else if ((he=gethostbyname(addr)) != NULL) {
		memcpy(&_this->sockaddr_.sin_addr, he->h_addr, he->h_length);
	} else {
		TCRPrint(1, "<cxSockAddrNew>: Invalid addr.");
		cxSockAddrFree(_this);
		return NULL;
	}

	strcpy(_this->addr_, inet_ntoa(_this->sockaddr_.sin_addr));
	_this->port_ = port;
	_this->next_ = NULL;

	return _this;
}

/* Fun: cxSockAddrNewCxSock
 *
 * Desc: addr - IP address or domain name, auto-fill local IP addr if NULL.
 * Ret: NULL if error.
 */
CCLAPI
CxSockAddr cxSockAddrNewCxSock (const char* addr, UINT16 port, CxSock local_socket)
{
/*
 * This function is the same as cxSockAddrNew in IPv4 environment
 */
	return cxSockAddrNew(addr, port);
}


CCLAPI
RCODE cxSockAddrFree(CxSockAddr _this)
{
	CxSockAddr n = freeList;
	if( _this == NULL )
		return RC_ERROR;
	
	while( n ) {
		if( n==_this )
			return RC_ERROR;
		n = n->next_;
	}
	_this->next_ = freeList;
	freeList = _this;

	return RC_OK;
}

CCLAPI
CxSockAddr cxSockAddrDup(CxSockAddr _this)
{
	CxSockAddr _copy;

	if (_this == NULL) return NULL;

	_copy = cxSockAddrAlloc();
	if (_copy == NULL) return NULL;

	strcpy(_copy->addr_, _this->addr_);
	_copy->port_ = _this->port_;
	_copy->sockaddr_ = _this->sockaddr_;

	return _copy;
}

CCLAPI
char* cxSockAddrGetAddr(CxSockAddr _this)
{
	return (_this)?_this->addr_:NULL;
}

CCLAPI
UINT16 cxSockAddrGetPort(CxSockAddr _this)
{
	return (_this)?_this->port_:0;
}

struct sockaddr_in* cxSockAddrGetSockAddr(CxSockAddr _this)
{
	return (_this)?&(_this->sockaddr_):NULL;
}

static int s_nDefaultTos = 0;

/* Fun: cxSockSetDefaultToS
 *
 * Desc: Set the default TOS. UACom can set the tos value directly.
 *
 * Ret: none.
 */
void cxSockSetDefaultToS(int tos)
{
	s_nDefaultTos = tos;
}

/* Fun: cxSockSetToS
 *
 * Desc: Set the TOS.
 *
 * Ret: If successful, return 0. If fail, return the error code from WSAGetLastError()
 */
int cxSockSetToS(CxSock _this, int tos)
{
	SOCKET thisSocket;
	int tos_len;
	int nError = 0;

	thisSocket = _this->sockfd_;
	tos_len = sizeof(tos);
	if (setsockopt( thisSocket, IPPROTO_IP, IP_TOS, (char*)&tos, tos_len) == SOCKET_ERROR)
	{
		#ifdef _WIN32
		nError = WSAGetLastError();
		#endif
	}
	return nError;
}

/* Fun: cxSockNew
 *
 * Desc: Bind address to socket if laddr != NULL.
 *       Listen if a TCP server socket.
 *
 * Ret: NULL if error.
 */
CxSock cxSockNew(CxSockType type, CxSockAddr laddr)
{
	CxSock _this;
	int    yes = 1;

	if ((type != CX_SOCK_DGRAM) && (type != CX_SOCK_SERVER)
		&& (type != CX_SOCK_STREAM_C) && (type != CX_SOCK_STREAM_S))
		return NULL;

	_this = (CxSock)malloc(sizeof *_this);
	if (_this == NULL) return NULL;

	_this->sockfd_ = INVALID_SOCKET;
	_this->type_ = type;

	if (laddr == NULL)
		_this->laddr_ = NULL;
	else
		_this->laddr_ = cxSockAddrDup(laddr);

	_this->raddr_ = NULL;

	if (type == CX_SOCK_STREAM_S)
		return _this;

	if (type == CX_SOCK_DGRAM)
		_this->sockfd_ = socket(PF_INET, SOCK_DGRAM, 0);
	else if ((type == CX_SOCK_SERVER) || (type == CX_SOCK_STREAM_C))
		_this->sockfd_ = socket(PF_INET, SOCK_STREAM, 0);

	if (_this->sockfd_ == INVALID_SOCKET) {
		TCRPrint(1, "<cxSockNew>: socket() error.\n");
		cxSockFree(_this);
		return NULL;
	} else
		TCRPrint(100, "<cxSockNew>: socket = %d\n", _this->sockfd_);

	/* set socket in non-blocking mode */

#ifdef NONBLOCK
	{
#if defined(_WIN32)
	unsigned long nonblock;
	nonblock = 1;
	ioctlsocket(_this->sockfd_, FIONBIO, &nonblock);
#else
	int flags;
	if ((flags = fcntl(_this->sockfd_, F_GETFL, 0)) < 0) return NULL;
	flags |= O_NONBLOCK;
	if (fcntl(_this->sockfd_, F_SETFL, flags) < 0) return NULL;
#endif
	}
#endif

	if (type == CX_SOCK_DGRAM && laddr != NULL) {
		if (setsockopt(_this->sockfd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&yes, sizeof(int)) == SOCKET_ERROR) {
			TCRPrint(1, "<cxSockNew>: setsockopt() error.\n");
			cxSockFree(_this);
			return NULL;
		}
		if (bind(_this->sockfd_, (struct sockaddr *)&laddr->sockaddr_, sizeof(laddr->sockaddr_)) 
			== -1) {
			TCRPrint(1, "<cxSockNew>: bind() error.\n");
			cxSockFree(_this);
			return NULL;
		}
	}
	
	if (type == CX_SOCK_STREAM_C && laddr != NULL) {
		if (setsockopt(_this->sockfd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&yes, sizeof(int)) == SOCKET_ERROR) {
			TCRPrint(1, "<cxSockNew>: setsockopt() error.\n");
			cxSockFree(_this);
			return NULL;
		}		
		if (bind(_this->sockfd_, (struct sockaddr *)&laddr->sockaddr_, sizeof(laddr->sockaddr_)) 
			== -1) {
			TCRPrint(1, "<cxSockNew>: bind() error.\n");
			cxSockFree(_this);
			return NULL;
		}
	}
	
	if (type == CX_SOCK_SERVER) {
		if (laddr != NULL) {
			if (setsockopt(_this->sockfd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&yes, sizeof(int)) == SOCKET_ERROR) {
				TCRPrint(1, "<cxSockNew>: setsockopt() error.\n");
				cxSockFree(_this);
				return NULL;
			}		
			if (bind(_this->sockfd_, (struct sockaddr *)&laddr->sockaddr_, sizeof(laddr->sockaddr_)) 
				== -1) {
				TCRPrint(1, "<cxSockNew>: bind() error.\n");
				cxSockFree(_this);
				return NULL;
			}
		}
		else {
			TCRPrint(1, "<cxSockNew>: CX_SOCK_SERVER needs $laddr.\n");
			cxSockFree(_this);
			return NULL;
		}

		if (listen(_this->sockfd_, 5) == -1) {
			TCRPrint(1, "<cxSockNew>: listen() error.\n");
			cxSockFree(_this);
			return NULL;
		}
	}

	cxSockSetToS( _this, s_nDefaultTos);

	return _this;
}
 
RCODE cxSockFree(CxSock _this)
{
	if( !_this )
		return RC_ERROR;

	if (_this->sockfd_ != INVALID_SOCKET) {
		closesocket(_this->sockfd_);
	}

	if (_this->laddr_ != NULL) {
		cxSockAddrFree(_this->laddr_);
	}

	if (_this->raddr_ != NULL) {
		cxSockAddrFree(_this->raddr_);
	}

	free(_this);

	return RC_OK;
}

SOCKET cxSockGetSock(CxSock _this)
{
	return (_this)?_this->sockfd_:-1;
}

RCODE cxSockSetSock(CxSock _this, SOCKET s)
{
	if( !_this )
		return RC_ERROR;
	_this->sockfd_ = s;

	return RC_OK;
}

CxSockAddr cxSockGetRaddr(CxSock _this)
{
	return (_this)?_this->raddr_:NULL;
}

RCODE cxSockSetRaddr(CxSock _this, CxSockAddr raddr)
{
	if (!_this)
		return RC_ERROR;
	if (_this->raddr_ != NULL)
		cxSockAddrFree(_this->raddr_);

	if ( !raddr ) 
		_this->raddr_ = NULL;
	else
		_this->raddr_ = cxSockAddrDup(raddr);

	return RC_OK;
}

CxSockAddr cxSockGetLaddr(CxSock _this)
{
	return (_this)?_this->laddr_:NULL;
}

RCODE cxSockSetLaddr(CxSock _this, CxSockAddr laddr)
{
	if (_this->laddr_ != NULL)
		cxSockAddrFree(_this->laddr_);

	_this->laddr_ = cxSockAddrDup(laddr);

	return RC_OK;
}

CxSock cxSockAccept(CxSock _this)
{
	CxSock stream;
	CxSockAddr raddr;
	fd_set rset;
	int n;
    	struct sockaddr_in  addr;
	UINT32 addrlen;
	SOCKET s;
	UINT16 port;
	char* ipaddr;
	/*u_long nonblock;*/

	if (!_this)
		return NULL; 
	if (_this->type_ != CX_SOCK_SERVER)
		return NULL;

	FD_ZERO(&rset);
	FD_SET(_this->sockfd_, &rset);

	n = select(_this->sockfd_ + 1, &rset, NULL, NULL, NULL/*blocking*/);

	addrlen = sizeof(addr);
	if (FD_ISSET(_this->sockfd_, &rset)) {
		s = accept(_this->sockfd_, (struct sockaddr *)&addr, &addrlen);
		if (s == INVALID_SOCKET) {
			TCRPrint(1, "<cxSockAccept>: accept() error.\n");
			return NULL;
		}
/*
#if defined(_WIN32)
		nonblock = 1;
		ioctlsocket(s, FIONBIO, &nonblock);
#else
		int flags;
		if ((flags = fcntl(s, F_GETFL, 0)) < 0) return NULL;
		flags |= O_NONBLOCK;
		if (fcntl(s, F_SETFL, flags) < 0) return NULL;
#endif
*/
	}

	/*stream = cxSockNew(CX_SOCK_STREAM_S, NULL); modified by tyhuang */
	stream = cxSockNew(CX_SOCK_STREAM_S, cxSockGetLaddr(_this));
	cxSockSetSock(stream, s);

	port = ntohs((UINT16)addr.sin_port);
	ipaddr = inet_ntoa(addr.sin_addr);
	raddr = cxSockAddrNew(ipaddr, port);
	cxSockSetRaddr(stream, raddr);
	cxSockAddrFree(raddr);

	return stream;
}
				
RCODE cxSockConnect(CxSock _this, CxSockAddr raddr)
{
	int result;

#ifndef NONBLOCK
	int n;
	int error;
	UINT32 len;
	fd_set rset, wset;
#endif

	if (!_this)
		return RC_ERROR; 
	if (_this->type_ != CX_SOCK_STREAM_C)
		return RC_ERROR;

	result = connect(_this->sockfd_, (struct sockaddr *)&raddr->sockaddr_, sizeof(struct sockaddr));
	
	if (result == SOCKET_ERROR) {
#ifdef NONBLOCK
		switch ( getError() ) {
			case EWOULDBLOCK:
			case EALREADY:
			case EINVAL:
				break;
			default:
				return RC_ERROR;
		}
#else
		/* have to take care of reentrant conditions */
		error = getError();

		if (error != EINPROGRESS)
			return RC_ERROR;
		else {
			/* connect still in progress */
			FD_ZERO(&rset);
			FD_SET(_this->sockfd_, &rset);
			wset = rset;

			n = select(_this->sockfd_ + 1, &rset, &wset, NULL, NULL/*blocking*/);
			
			if (n == 0) {
				/* timeout */
				closesocket(_this->sockfd_);
				return RC_ERROR;
			} 

			if (FD_ISSET(_this->sockfd_, &rset) || FD_ISSET(_this->sockfd_, &wset)) {
				len = sizeof(error);
				if (getsockopt(_this->sockfd_, SOL_SOCKET, SO_ERROR, (char*)&error, &len) < 0)
					/* Solaris pending error */
					return RC_ERROR;
			} else
				/* other errors */
				return RC_ERROR;
		}
#endif

	}
	cxSockSetRaddr(_this, raddr);

	/* connected */
	return RC_OK;
}

/* Fun: cxSockSend
 *
 * Desc: Send 'len' of bytes from buffer.
 *
 * Ret: Return number of bytes sent, -1 on error.
 */
int cxSockSend(CxSock _this, const char* buf, int len)
{
	int n, nsent = 0;
	int error = 0;
	int result;
	fd_set wset;
	fd_set xset;
	struct timeval tv;
	int err;

	if (!_this || !buf)
		return -1;
	if (_this->type_ != CX_SOCK_STREAM_C && _this->type_ != CX_SOCK_STREAM_S)
		return -1;

	if (_this->sockfd_ == INVALID_SOCKET)
		return -1;

	for (nsent = 0; nsent < len; nsent += n) {
		FD_ZERO(&wset);
		FD_SET(_this->sockfd_, &wset);

		FD_ZERO(&xset);
		FD_SET(_this->sockfd_, &xset);

		tv.tv_sec = 10;
		tv.tv_usec = 0;

		result = select(_this->sockfd_ + 1, NULL, &wset, &xset, NULL); /*blocking*/

		if (result == SOCKET_ERROR) {
			error = 1;
			break;
		}

		n = send(_this->sockfd_, buf + nsent, len - nsent, 0/*no flags*/);

		err = getError();
      
		/* note that errno cannot be EWOULDBLOCK since select()
                 * just told us that data can be written.
		 */
		if (n == SOCKET_ERROR || n == 0) {
			error = 1;
			break;
		}
	}

	if (error)
		return -1;
	else
		return nsent;
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
int cxSockRecvEx(CxSock _this, char* buf, int len, long timeout)
{
	int n, nrecv = 0;
	int ret = 0;
	int result;
	fd_set rset;
	struct timeval t;

	if (!_this || !buf)
		return -1;

	if (_this->type_ != CX_SOCK_STREAM_C && _this->type_ != CX_SOCK_STREAM_S)
		return -1;

	if (_this->sockfd_ == INVALID_SOCKET)
		return -1;

	for (nrecv = 0; nrecv < len; nrecv += n) {
		FD_ZERO(&rset);
		FD_SET(_this->sockfd_, &rset);

		if( timeout<0 ) /* blocking mode */
			result = select(_this->sockfd_ + 1, &rset, NULL, NULL, NULL);
		else {
			t.tv_sec = timeout/1000;
			t.tv_usec = (timeout%1000)*1000;
			result = select(_this->sockfd_ + 1, &rset, NULL, NULL, &t);
		}
		/* Acer Modify SipIt 10 */
		/* should check if result ==0 Timeout */
		if ((result == SOCKET_ERROR)) {
			ret = -1;
			break;
		}
		else if ( result == 0 )  {
			ret = -2;
			break;
		}

		n = recv(_this->sockfd_, buf + nrecv, len - nrecv, 0/*no flags*/);

		if (n == SOCKET_ERROR) {
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

	/* modified by tkhung 2004-10-19
	if (ret<0)
		return ret;
	else
		return nrecv;
	*/
		
	if ( nrecv > 0)
                return nrecv;
        else
                return ret;
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
int cxSockTpktSend(CxSock _this, const char* buf, int len)
{
	int nSent, nTpktSent;
	unsigned char tpktHeader[4];

	if (!_this || !buf)
		return -1;

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
int cxSockTpktRecv(CxSock _this, char* buf, int len)
{
	int nRecv, nTPDU;
	unsigned char tpktHeader[4];

	if (!_this || !buf)
		return -1;

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

int cxSockSendto(CxSock _this, const char* buf, int len, CxSockAddr raddr)
{
	int nsent;

	if (!_this || !buf)
		return -1;

	if (_this->type_ != CX_SOCK_DGRAM)
		return -1;
 
	nsent = sendto( _this->sockfd_, 
			buf, 
			len, 
			0, /* No flags */
			(struct sockaddr*)&(raddr->sockaddr_),
			sizeof(raddr->sockaddr_));

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
int cxSockRecvfrom(CxSock _this, char* buf, int len)
{
	CxSockAddr raddr;
	int nrecv;
	UINT32 addrlen;
	struct sockaddr_in addr;
	UINT16 port;
	char* ipaddr;

	if (!_this || !buf)
		return -1;

	if (_this->type_ != CX_SOCK_DGRAM)
		return -1;

	addrlen = sizeof(addr);
	nrecv = recvfrom( _this->sockfd_, 
			buf, 
			len, 
			0, /* no flags */
			(struct sockaddr*)&addr, 
			&addrlen);

	if (nrecv == SOCKET_ERROR) 
		return -1;

	port = ntohs((UINT16)addr.sin_port);
	ipaddr = inet_ntoa(addr.sin_addr);
	raddr = cxSockAddrNew(ipaddr, port);
	cxSockSetRaddr(_this, raddr);
	cxSockAddrFree(raddr);

	return nrecv;
}
