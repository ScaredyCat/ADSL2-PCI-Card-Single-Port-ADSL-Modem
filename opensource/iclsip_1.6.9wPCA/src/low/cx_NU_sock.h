/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * cx_sock.h
 *
 * $Id: cx_NU_sock.h,v 1.1 2003/02/19 13:25:33 yjliao Exp $
 */

#ifndef CX_SOCK_H
#define CX_SOCK_H

#ifdef  __cplusplus
extern "C" {
#endif

/*
 * Select uses arrays of SOCKETs.  These macros manipulate such
 * arrays.  FD_SETSIZE may be defined by the user before including
 * this file, but the default here should be >= 64.
 *
 * CAVEAT IMPLEMENTOR and USER: THESE MACROS AND TYPES MUST BE
 * INCLUDED IN WINSOCK.H EXACTLY AS SHOWN HERE.
 */

#include "net/inc/socketd.h" 
#include "net/inc/externs.h"

#define SOCKET_FD	int
#define RC_OK		0
#define RC_ERROR	-1
#define INVALID_SOCKET	-1

typedef int			RCODE;
typedef struct cxSockAddrObj*	CxSockAddr;
typedef struct cxSockObj*	CxSock;

typedef enum {
	CX_SOCK_UNKNOWN = 0,
	CX_SOCK_DGRAM,		/* UDP */
	CX_SOCK_SERVER,		/* TCP Server */ 
	CX_SOCK_STREAM_C,	/* TCP Client*/
	CX_SOCK_STREAM_S
} CxSockType;

RCODE		cxSockInit(void);
RCODE		cxSockClean(void);

CxSockAddr	cxSockAddrNew(const char* addr, UINT16 port);
RCODE		cxSockAddrFree(CxSockAddr);
CxSockAddr	cxSockAddrDup(CxSockAddr);
char*		cxSockAddrGetAddr(CxSockAddr);
UINT16		cxSockAddrGetPort(CxSockAddr);

CxSock		cxSockNew(CxSockType, CxSockAddr laddr);
RCODE		cxSockFree(CxSock);
SOCKET_FD	cxSockGetSock(CxSock);
RCODE		cxSockSetSock(CxSock, SOCKET_FD s);
CxSockAddr	cxSockGetLaddr(CxSock);
RCODE		cxSockSetLaddr(CxSock, CxSockAddr laddr);
CxSockAddr	cxSockGetRaddr(CxSock);
RCODE		cxSockSetRaddr(CxSock, CxSockAddr raddr);
CxSock		cxSockAccept(CxSock);
RCODE		cxSockConnect(CxSock, CxSockAddr raddr);
int		cxSockSend(CxSock, char* buf, int len);
int		cxSockRecvEx(CxSock, char* buf, int len, long timeout);
#define		cxSockRecv(sock,buf,len)	cxSockRecvEx(sock,buf,len,-1)	
int		cxSockSendto(CxSock, char* buf, int len, CxSockAddr raddr);
int		cxSockRecvfrom(CxSock, char* buf, int len);
int		cxSockTpktSend(CxSock, char* buf, int len);
int		cxSockTpktRecv(CxSock, char* buf, int len);

#ifdef  __cplusplus
}
#endif

#endif /* CX_SOCK_H */
