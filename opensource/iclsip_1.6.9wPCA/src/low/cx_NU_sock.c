/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * cx_sock.c
 *
 * $Id: cx_NU_sock.c,v 1.1 2003/02/19 13:25:33 yjliao Exp $
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "cx_NU_sock.h"

static int	sock_refcnt = 0;

static CxSockAddr	freeList = NULL;

struct cxSockAddrObj {
	char			addr_[32];
	UINT16			port_;
	struct addr_struct	sockaddr_;
	CxSockAddr		next_;
};

struct cxSockObj {
	SOCKET_FD		sockfd_;
	CxSockType		type_;
	CxSockAddr		laddr_;
	CxSockAddr		raddr_;
};

void cxSockInit_(void) {} /* just for DLL forcelink */

RCODE cxSockInit()
{
	RCODE rc = RC_OK;

	if(++sock_refcnt>1)
		return RC_OK;

	freeList = NULL;

	/* NU_Init_Net() ?? */
	return rc;
}
 
RCODE cxSockClean()
{
	CxSockAddr _this;
	RCODE rc = RC_OK;

	if(--sock_refcnt>0)
		return RC_OK;

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
CxSockAddr cxSockAddrNew(const char* addr, UINT16 port)
{
	CxSockAddr	_this;
	char		hname[64];
	NU_HOSTENT	hentry;

	if (addr == NULL) {
		if (NU_Get_Host_Name(hname, sizeof(hname)) == NU_SUCCESS) {
			if (NU_Get_Host_By_Name(hname, &hentry) != NU_SUCCESS)
				return NULL;
		} else return NULL;
	} else {
		if (NU_Get_Host_By_Addr((char*)addr, 4, NU_FAMILY_IP, &hentry) != NU_SUCCESS)
			return NULL;
	}

	if ((_this = cxSockAddrAlloc()) == NULL)
		return NULL;

	memset((char *)&_this->sockaddr_, 0, sizeof(_this->sockaddr_));
	_this->sockaddr_.family = hentry.h_addrtype;
	_this->sockaddr_.port = port;

	memcpy(&_this->sockaddr_.id, hentry.h_addr, hentry.h_length);

	sprintf(_this->addr_,"%d.%d.%d.%d", 
		_this->sockaddr_.id.is_ip_addrs[0], 
		_this->sockaddr_.id.is_ip_addrs[1], 
		_this->sockaddr_.id.is_ip_addrs[2],
		_this->sockaddr_.id.is_ip_addrs[3]);
	_this->port_ = port;
	_this->next_ = NULL;

	return _this;
}

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

char* cxSockAddrGetAddr(CxSockAddr _this)
{
	return _this->addr_;
}

UINT16 cxSockAddrGetPort(CxSockAddr _this)
{
	return _this->port_;
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
	INT8    yes = 0x1C;  /* TOS = 00011100 */;
	/*u_long nonblock;*/

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
		_this->sockfd_ = NU_Socket(NU_FAMILY_IP, NU_TYPE_DGRAM, NU_NONE);
	else if ((type == CX_SOCK_SERVER) || (type == CX_SOCK_STREAM_C))
		_this->sockfd_ = NU_Socket(NU_FAMILY_IP, NU_TYPE_STREAM, NU_NONE);

	if (_this->sockfd_ <= INVALID_SOCKET) {
		cxSockFree(_this);
		return NULL;
	} 

	if (type == CX_SOCK_DGRAM && laddr != NULL) {
		NU_Setsockopt(_this->sockfd_, SOL_SOCKET, IP_TOS, (void*)&yes, sizeof(INT8)); /* may be fail */
		if (NU_Bind(_this->sockfd_, &laddr->sockaddr_, 0) <= INVALID_SOCKET) {
			cxSockFree(_this);
			return NULL;
		}
	}

	if (type == CX_SOCK_SERVER) {
		if (laddr != NULL) {
			NU_Setsockopt(_this->sockfd_, SOL_SOCKET, IP_TOS, (void*)&yes, sizeof(INT8)); /* may be fail */
			if (NU_Bind(_this->sockfd_, &laddr->sockaddr_, 0) <= INVALID_SOCKET) {
				cxSockFree(_this);
				return NULL;
			}
		} else {
			cxSockFree(_this);
			return NULL;
		}

		if (NU_Listen(_this->sockfd_, 5) != NU_SUCCESS) {
			cxSockFree(_this);
			return NULL;
		}
	}

	return _this;
}
 
RCODE cxSockFree(CxSock _this)
{
	if( !_this )
		return RC_ERROR;

	if (_this->sockfd_ != INVALID_SOCKET) {
		NU_Close_Socket(_this->sockfd_);
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

SOCKET_FD cxSockGetSock(CxSock _this)
{
	return _this->sockfd_;
}

RCODE cxSockSetSock(CxSock _this, SOCKET_FD s)
{
	_this->sockfd_ = s;

	return RC_OK;
}

CxSockAddr cxSockGetRaddr(CxSock _this)
{
	return _this->raddr_;
}

RCODE cxSockSetRaddr(CxSock _this, CxSockAddr raddr)
{
	if (_this->raddr_ != NULL)
		cxSockAddrFree(_this->raddr_);

	_this->raddr_ = cxSockAddrDup(raddr);

	return RC_OK;
}

CxSockAddr cxSockGetLaddr(CxSock _this)
{
	return _this->laddr_;
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
	FD_SET rset;
    	struct addr_struct addr;
	UINT32 addrlen;
	SOCKET_FD s;
	char* ipaddr;
	/*u_long nonblock;*/

	if (_this->type_ != CX_SOCK_SERVER)
		return NULL;

	NU_FD_Init(&rset);
	NU_FD_Set(_this->sockfd_, &rset);

	if (NU_Select(_this->sockfd_ + 1, &rset, NULL, NULL, NU_SUSPEND) != NU_SUCCESS)
		return NULL;

	addrlen = sizeof(addr);
	if (NU_FD_Check(_this->sockfd_, &rset) == NU_TRUE) {
		if ((s = NU_Accept(_this->sockfd_, &addr, 0)) <= INVALID_SOCKET) {
			return NULL;
		}
	} else {
		return NULL;
	}

	stream = cxSockNew(CX_SOCK_STREAM_S, NULL);
	cxSockSetSock(stream, s);

	sprintf(ipaddr, "%d.%d.%d.%d", 
		addr.id.is_ip_addrs[0], 
		addr.id.is_ip_addrs[1], 
		addr.id.is_ip_addrs[2],
		addr.id.is_ip_addrs[3]);
	raddr = cxSockAddrNew(ipaddr, addr.port);
	cxSockSetRaddr(stream, raddr);
	cxSockAddrFree(raddr);

	return stream;
}
				
RCODE cxSockConnect(CxSock _this, CxSockAddr raddr)
{
	if (_this->type_ != CX_SOCK_STREAM_C)
		return RC_ERROR;

	if (NU_Connect(_this->sockfd_, &raddr->sockaddr_, 0) <= INVALID_SOCKET) {
		return RC_ERROR;
	}

	cxSockSetRaddr(_this, raddr);
	return RC_OK;
}

/* Fun: cxSockSend
 *
 * Desc: Send 'len' of bytes from buffer.
 *
 * Ret: Return number of bytes sent, -1 on error.
 */
int cxSockSend(CxSock _this, char* buf, int len)
{
	int n, nsent = 0;
	/*struct timeval timeout = {1, 0};*/

	if (_this->type_ != CX_SOCK_STREAM_C && _this->type_ != CX_SOCK_STREAM_S)
		return -1;

	if (_this->sockfd_ <= INVALID_SOCKET)
		return -1;

	for (nsent = 0; nsent < len; nsent += n) {
		n = NU_Send(_this->sockfd_, buf + nsent, len - nsent, 0);
      		if (n <= 0) {
			break;
		}
	}

	if (n <= 0)
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
	FD_SET rset;

	if (_this->type_ != CX_SOCK_STREAM_C && _this->type_ != CX_SOCK_STREAM_S)
		return -1;

	if (_this->sockfd_ <= INVALID_SOCKET)
		return -1;

	for (nrecv = 0; nrecv < len; nrecv += n) {
		NU_FD_Init(&rset);
		NU_FD_Set(_this->sockfd_, &rset);

		if( timeout<0 ) /* blocking mode */
			result = NU_Select(_this->sockfd_ + 1, &rset, NULL, NULL, NU_SUSPEND);
		else {
			result = NU_Select(_this->sockfd_ + 1, &rset, NULL, NULL, timeout);
			if (result == NU_NO_DATA) {
				ret = -1;
				break;
			}
		}

		n = NU_Recv(_this->sockfd_, buf + nrecv, len - nrecv, 0);

		if (n < 0) {
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
int cxSockTpktSend(CxSock _this, char* buf, int len)
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
int cxSockTpktRecv(CxSock _this, char* buf, int len)
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

int cxSockSendto(CxSock _this, char* buf, int len, CxSockAddr raddr)
{
	int nsent;

	if (_this->type_ != CX_SOCK_DGRAM)
		return -1;
 
	nsent = NU_Send_To(_this->sockfd_, buf, len, 0,	&(raddr->sockaddr_), 0);

	if (nsent <= 0) 
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
	struct addr_struct addr;
	UINT16 port;
	char* ipaddr;

	if (_this->type_ != CX_SOCK_DGRAM)
		return -1;

	addrlen = sizeof(addr);
	nrecv = NU_Recv_From(_this->sockfd_, buf, len, 0, &addr, 0);

	if (nrecv <= 0) 
		return -1;

	port = addr.port;
	sprintf(ipaddr, "%d.%d.%d.%d", 
		addr.id.is_ip_addrs[0], 
		addr.id.is_ip_addrs[1], 
		addr.id.is_ip_addrs[2],
		addr.id.is_ip_addrs[3]);
	raddr = cxSockAddrNew(ipaddr, port);
	cxSockSetRaddr(_this, raddr);
	cxSockAddrFree(raddr);

	return nrecv;
}
