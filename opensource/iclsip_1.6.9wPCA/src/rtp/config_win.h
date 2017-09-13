/*=========================================================================*\
  Copyright (c) 2000-2002 Computer & Communications Research Laboratories,
                          Industrial Technology Research Institute
  
  RTP - Configuration specialized for Windows (Win32)
  
  Author: Jiun-Yao Huang <jyhuang@wizs.org>
  Revision: $Id: config_win.h,v 1.25 2005/05/13 07:41:02 ljchuang Exp $
\*=========================================================================*/
#ifndef	__CONFIG_WIN_H__
#define	__CONFIG_WIN_H__

#include <winsock2.h>
#include <ws2tcpip.h>

#ifndef	_WIN32_WCE

	#include <process.h>
	#include <sys/timeb.h>
	#include <time.h>
	#include <assert.h>

#else	/* _WIN32_WCE is defined */

	#include <winbase.h>
	#define assert(cond)	{}

#endif	/* _WIN32_WCE */

/*
const SOCKET invalid_socket   = INVALID_SOCKET;
const int    socket_error     = SOCKET_ERROR;
*/

#ifdef	WORDS_BIGENDIAN
	#undef	WORDS_BIGENDIAN
#endif

#ifndef SOCKADDR_STORAGE
#define SOCKADDR_STORAGE struct sockaddr_storage
#endif

#ifndef ADDRINFO
#define ADDRINFO struct addrinfo
#endif

typedef	int	socklen_t;

#pragma warning(disable:4761)

#define	SOCKET_ERRNO	WSAGetLastError()

#define	THREAD_RETURN_TYPE	unsigned __stdcall        

#endif /* #ifndef __CONFIG_WIN_H__ */
