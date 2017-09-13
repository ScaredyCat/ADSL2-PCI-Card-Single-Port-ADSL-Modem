/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * cx_sock.h
 *
 * $Id: cx_sock.h,v 1.28 2005/05/13 07:41:02 ljchuang Exp $
 */

#ifndef CX_SOCK_H
#define CX_SOCK_H

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>

#endif

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

/*
it works for user to enlarge this value

#ifdef FD_SETSIZE
#undef FD_SETSIZE
#endif
#define FD_SETSIZE      128
*/

#include <common/cm_def.h>

#ifndef IP_TOS
#define IP_TOS              8
#endif

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

CCLAPI CxSockAddr	cxSockAddrNew(const char* addr, UINT16 port);
CCLAPI CxSockAddr	cxSockAddrNewCxSock (const char* addr, UINT16 port, CxSock local_socket);


CCLAPI RCODE		cxSockAddrFree(CxSockAddr);
CCLAPI CxSockAddr	cxSockAddrDup(CxSockAddr);
CCLAPI char*		cxSockAddrGetAddr(CxSockAddr);
CCLAPI UINT16		cxSockAddrGetPort(CxSockAddr);

CxSock		cxSockNew(CxSockType, CxSockAddr laddr);
RCODE		cxSockFree(CxSock);
SOCKET		cxSockGetSock(CxSock);
RCODE		cxSockSetSock(CxSock, SOCKET s);
CxSockAddr	cxSockGetLaddr(CxSock);
RCODE		cxSockSetLaddr(CxSock, CxSockAddr laddr);
CxSockAddr	cxSockGetRaddr(CxSock);
RCODE		cxSockSetRaddr(CxSock, CxSockAddr raddr);
CxSock		cxSockAccept(CxSock);
RCODE		cxSockConnect(CxSock, CxSockAddr raddr);
int		cxSockSend(CxSock, const char* buf, int len);
int		cxSockRecvEx(CxSock, char* buf, int len, long timeout);
#define		cxSockRecv(sock,buf,len)	cxSockRecvEx(sock,buf,len,-1)	
int		cxSockSendto(CxSock, const char* buf, int len, CxSockAddr raddr);
int		cxSockRecvfrom(CxSock, char* buf, int len);
int		cxSockTpktSend(CxSock, const char* buf, int len);
int		cxSockTpktRecv(CxSock, char* buf, int len);

int		cxSockSetToS(CxSock, int tos);
void	cxSockSetDefaultToS(int tos);

#ifdef  __cplusplus
}
#endif

#endif /* CX_SOCK_H */
