/*=========================================================================*\
  Copyright (c) 2000-2002 Computer & Communications Research Laboratories,
                          Industrial Technology Research Institute
  
  RTP - Configuration
  
  Author: Jiun-Yao Huang <jyhuang@wizs.org>
  Revision: $Id: config.h,v 1.14 2002/11/10 10:20:57 jyhuang Exp $
\*=========================================================================*/
#ifndef	__CONFIG_H__
#define	__CONFIG_H__

/*--------------------------------------------------------------------------*/
#ifndef	__WINDOWS
	#if	(!defined(__GNUC__) && defined(_WIN32))
	#define	__WINDOWS 1          /* for Windows platforms */
	#elif	(!defined(__UNIX))
	#define	__UNIX 1             /* for UNIX-like platforms */
	#endif
#endif
/*--------------------------------------------------------------------------*/

#include <stdlib.h>
#include <string.h>

#ifdef	__WINDOWS
#include "config_win.h"
#else
#include "config_unix.h"
#endif

#ifndef	INADDR_NONE
#define	INADDR_NONE	0xffffffff
#endif

#define	RTP_PORT_BASE		9000
#define	RTP_PORT_MAX		10000

#define	RTCP_PKTBUFSIZE		(((8 + 20 + 24 * 31) / 1024 + 1) * 1024 * 2)

#define	RTP_THREAD_TIMEOUT	5000	/* 5 seconds */

/* default session bandwidth */
#define	RTP_DEF_SESSION_BW	(20 * 1024)

#define	RTP_RTCP_BW_FRACTION	.05	/* the fraction of the session bw */

/* max. RTCP packet size, in bytes */

#endif /* #ifndef __CONFIG_H__ */
