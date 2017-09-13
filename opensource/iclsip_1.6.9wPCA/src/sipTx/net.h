#ifndef _SIPTX_NET_
#define _SIPTX_NET_

#ifdef UNIX

#ifndef inet_addr
#include <arpa/inet.h>
#endif

#ifndef IN_ADDR
typedef struct in_addr IN_ADDR;
#endif

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff /* should have been in <netinet/in.h> */
#endif

#ifndef hostent
#include <netdb.h>
#endif

#include <assert.h>

#endif /* UNIX */

#ifdef _WIN32

#ifndef _WIN32_WCE
	#include <assert.h>
	#include <time.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
#else
	#ifndef assert
		#define assert(cond)	{ if (cond) {} }
	#endif
	#include <winsock.h>
#endif

#endif /* _WIN32 */

#endif /* _SIPTX_NET_ */

