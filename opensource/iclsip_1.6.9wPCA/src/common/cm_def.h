/*
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * cm_def.h
 *
 * $Id: cm_def.h,v 1.24 2005/03/11 06:44:23 txyu Exp $
 */

#ifndef COMMON_H
#define COMMON_H

#ifdef  __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#define CCLAPI			__declspec(dllexport)
#else
#define CCLAPI
#endif

#include <string.h>

#ifdef UNIX
typedef int			SOCKET;
#define INVALID_SOCKET		-1
#define SOCKET_ERROR		-1
#endif

/**************** Trace Log Flags *************/
typedef enum {
	LOG_FLAG_NONE		=0,	/* no trace log */
	LOG_FLAG_CONSOLE	=0x01,	/* log to console */
	LOG_FLAG_FILE		=0x02,	/* log to file */
	LOG_FLAG_SOCK		=0x04	/* log to a remote log server */
} LogFlag;

/**************** Trace Level *****************/
typedef enum {
	TRACE_LEVEL_NONE	=0,	/* print nothing */
	TRACE_LEVEL_ERROR	=1,	/* print error messages */
	TRACE_LEVEL_WARNING     =2,     /* print warning messages */
	TRACE_LEVEL_APP         =49,    /* print AP trace messages */
	TRACE_LEVEL_API		=99,	/* print API trace messages */
	TRACE_LEVEL_ALL		=100	/* print all trace messages */
} TraceLevel;

/* Use this macro(__CMFILE__) to replace __FILE__ macro
   if you don't want the path included. */
#define __CMFILE__	((strrchr(__FILE__,'\\'))?\
			(strrchr(__FILE__,'\\')+1):__FILE__)

/**************** Basic Types ****************/
#ifndef BOOL_DEFINED
typedef int			BOOL;
#endif

#ifndef TRUE
#define TRUE			1
#endif

#ifndef FALSE
#define FALSE			0
#endif

#ifndef INT8_DEFINED
#ifdef _WIN32
typedef signed char		INT8;
#else
typedef char			INT8;
#endif
#endif

#ifndef INT16_DEFINED
typedef short			INT16;
#endif

#ifndef INT32_DEFINED
typedef int			INT32;
#endif

#ifndef LONG_DEFINED
typedef long			LONG;
#endif

#ifndef UINT8_DEFINED
typedef unsigned char		UINT8;
#endif

#ifndef UINT16_DEFINED
typedef unsigned short		UINT16;
#endif

#ifndef UINT32_DEFINED
typedef unsigned int		UINT32;
#endif

#ifndef ULONG_DEFINED
typedef unsigned long		ULONG;
#endif

/*************** Return Codes ****************/
typedef int			RCODE;

/* general & common error codes */
#define IN
#define OUT

#define RC_OK			0
#define RC_ERROR		-1

#define RC_BASE			1000
#define RC_E_PARAM		(RC_BASE+1) /* general parameter error */

/* module specific error codes */
#define RC_BASE_SIP		(RC_BASE+2000)
#define RC_BASE_MC		(RC_BASE+3000)
#define RC_BASE_SDP		(RC_BASE+4000)
#define RC_BASE_SIPTX		(RC_BASE+5000)
#define RC_BASE_SIPUA		(RC_BASE+6000)

/* POSIX & non-POSIX system defined error codes */
#ifdef _WIN32
#define	EWOULDBLOCK		WSAEWOULDBLOCK

#define	ENOTSOCK		WSAENOTSOCK
#define	EDESTADDRREQ		WSAEDESTADDRREQ
#define	EMSGSIZE		WSAEMSGSIZE
#define	EPROTOTYPE		WSAEPROTOTYPE
#define	ENOPROTOOPT		WSAENOPROTOOPT
#define	EPROTONOSUPPORT		WSAEPROTONOSUPPORT
#define	ESOCKTNOSUPPORT		WSAESOCKTNOSUPPORT
#define	EOPNOTSUPP		WSAEOPNOTSUPP
#define	EPFNOSUPPORT		WSAEPFNOSUPPORT
#define	EAFNOSUPPORT		WSAEAFNOSUPPORT
#define	EADDRINUSE		WSAEADDRINUSE
#define	EADDRNOTAVAIL		WSAEADDRNOTAVAIL

#define	ENETDOWN		WSAENETDOWN
#define	ENETUNREACH		WSAENETUNREACH
#define	ENETRESET		WSAENETRESET
#define	ECONNABORTED		WSAECONNABORTED
#define	ECONNRESET		WSAECONNRESET
#define	ENOBUFS			WSAENOBUFS
#define	EISCONN			WSAEISCONN
#define	ENOTCONN		WSAENOTCONN
#define	ESHUTDOWN		WSAESHUTDOWN
#define	ETOOMANYREFS		WSAETOOMANYREFS
#define	ETIMEDOUT		WSAETIMEDOUT
#define	ECONNREFUSED		WSAECONNREFUSED
#define	EHOSTDOWN		WSAEHOSTDOWN
#define	EHOSTUNREACH		WSAEHOSTUNREACH
#define	EALREADY		WSAEALREADY
#define	EINPROGRESS		WSAEINPROGRESS
#endif /* _WIN32 */

#ifdef  __cplusplus
}
#endif

#endif /* COMMON_H */
