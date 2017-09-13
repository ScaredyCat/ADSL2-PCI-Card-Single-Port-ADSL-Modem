/* config_unix.h.  Generated automatically by configure.  */
/*=========================================================================*\
  Copyright (c) 2000-2002 Computer & Communications Research Laboratories,
                          Industrial Technology Research Institute
  
  RTP - Configuration specialized for Unix
  
  Author: Jiun-Yao Huang <jyhuang@wizs.org>
  Revision: $Id: config_unix.h,v 1.4 2005/02/15 08:40:51 txyu Exp $
\*=========================================================================*/
#ifndef	__CONFIG_UNIX_H__
#define	__CONFIG_UNIX_H__

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <time.h>
#include <netdb.h>
/* #include <pthread.h> */

#ifndef SOCKADDR_STORAGE
#define SOCKADDR_STORAGE struct sockaddr_storage
#endif

#ifndef ADDRINFO
#define ADDRINFO struct addrinfo
#endif

/* Words are stored with most significant byte first. */
/* #undef WORDS_BIGENDIAN */

/* Socket descriptor type */
#define SOCKET int

/* Define if the member __ss_family of 'sockaddr_storage' is found */
/* __ss_family is used in older version Linux, new Linux version uses ss_family. */
/* Check /usr/include/bits/socket.h to find the structure of 'sockaddr_storage' */
/* #define HAVE__SS_FAMILY 1 */

/* Define if the type `socklen_t' is found */
#define HAVE_SOCKLEN_T 1

#ifndef	HAVE_SOCKLEN_T
typedef	int	socklen_t;
#endif

#define INVALID_SOCKET	-1
#define	SOCKET_ERROR	-1

#define	SOCKET_ERRNO	(errno)
#define closesocket(sd)	close(sd)
#define	ioctlsocket(sd, cmd, val)	ioctl(sd, cmd, val)

#define	THREAD_RETURN_TYPE	void *

#endif /* #ifndef __CONFIG_UNIX_H__ */
