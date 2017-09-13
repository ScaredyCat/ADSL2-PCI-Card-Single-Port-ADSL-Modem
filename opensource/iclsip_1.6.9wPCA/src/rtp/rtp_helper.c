/*=========================================================================*\
  Copyright (c) 2000-2002 Computer & Communications Research Laboratories,
                          Industrial Technology Research Institute
  
  RTP helper 
  
  Author: Jiun-Yao Huang <jyhuang@wizs.org>
  Revision: $Id: rtp_helper.c,v 1.29 2005/02/15 08:40:51 txyu Exp $
  Description:
\*=========================================================================*/
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "rtp_helper.h"
#ifdef	__UNIX     /* [ for UNIX ] */
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#else              /* [ for Windows ] */
#include <windows.h>
#endif             /* [ #ifdef __UNIX ] */


#ifdef	RTPTRACE
void rtptrace(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
}
#else	/* !RTPTRACE */
#ifndef	__GNUC__
void rtptrace(const char *fmt, ...) {}
#endif
#endif /* RTPTRACE */

#define	__INIT_MEMBER(m, s, t) {	\
		memset(m, 0, sizeof(rtp_member_t)); \
		m->ssrc = s; \
		m->src.probation = RTP_MIN_SEQUENTIAL - 1; \
		m->type |= t;	\
	}

void __mlist_init(rtp_member_list_t *mlist)
{
	assert(mlist);

	mlist->members = (rtp_member_t *) malloc(sizeof(rtp_member_t));
	mlist->capacity = 1;
	mlist->size = 0;
	mlist->senders = 0;
}

rtp_member_t *__mlist_lookup(rtp_member_list_t *mlist, u_int32 ssrc)
{
	size_t	i;
	
	assert(mlist);
	
	for (i = 0; i < mlist->size; ++i)
		if (mlist->members[i].ssrc == ssrc)
			return &mlist->members[i];
	
	return NULL;
}

rtp_member_t *__mlist_lookup_byaddr(rtp_member_list_t *mlist,
                                    const struct sockaddr *xaddr)
{
	size_t	i;
	rtp_member_t	*pmem;
	
	assert(mlist != NULL && xaddr != NULL);
	
	for (i = 0; i < mlist->size; ++i) {
		pmem = &mlist->members[i];

#ifdef	HAVE__SS_FAMILY
		if (xaddr->sa_family != pmem->addr.__ss_family)
			continue;
#else
		if (xaddr->sa_family != pmem->addr.ss_family)
			continue;
#endif

		if(xaddr->sa_family == AF_INET6){
			const struct sockaddr_in6 *addr = (const struct sockaddr_in6 *)xaddr;
			
			if((addr->sin6_port == ((struct sockaddr_in6 *)&(pmem->addr))->sin6_port &&
				memcmp(&(addr->sin6_addr),&(((struct sockaddr_in6 *)&(pmem->addr))->sin6_addr),sizeof(addr->sin6_addr))== 0) ||
			   (addr->sin6_port == 0 &&
				memcmp(&(addr->sin6_addr),&(((struct sockaddr_in6 *)&(pmem->addr))->sin6_addr),sizeof(addr->sin6_addr)) == 0) ||
			   (addr->sin6_port == ((struct sockaddr_in6 *)&(pmem->addr))->sin6_port &&
				memcmp(&(addr->sin6_addr),&in6addr_any,sizeof(addr->sin6_addr)) == 0))
				return pmem;
		}else if(xaddr->sa_family == AF_INET){
			const struct sockaddr_in *addr = (const struct sockaddr_in *)xaddr;
			if ((addr->sin_port == ((struct sockaddr_in *)&(pmem->addr))->sin_port &&
			     addr->sin_addr.s_addr == ((struct sockaddr_in *)&(pmem->addr))->sin_addr.s_addr) ||
			    (addr->sin_port == 0 && 
			     addr->sin_addr.s_addr == ((struct sockaddr_in *)&(pmem->addr))->sin_addr.s_addr) ||
			    (addr->sin_addr.s_addr == INADDR_ANY &&
			     addr->sin_port == ((struct sockaddr_in *)&(pmem->addr))->sin_port) )
				return pmem;
		}
	}

	return NULL;	/* not found */
}

rtp_member_t *__mlist_add(rtp_member_list_t *mlist, u_int32 ssrc, int type)
{
	assert(mlist);
	
	if (mlist->size < mlist->capacity) {	/* fine */
		__INIT_MEMBER((&mlist->members[mlist->size]), ssrc, type)
		mlist->size++;
		return (&mlist->members[mlist->size-1]);
	} else {
		rtp_member_t	*pmem;
		mlist->capacity *= 2;
		pmem = (rtp_member_t *) 
		       malloc(sizeof(rtp_member_t) * mlist->capacity);
		memcpy(pmem, mlist->members, 
		       sizeof(rtp_member_t) * mlist->size);
		free(mlist->members);
		mlist->members = pmem;
		return __mlist_add(mlist, ssrc, type);
	}
}

rtp_member_t *__mlist_add_byaddr(rtp_member_list_t *mlist, 
                                 const struct sockaddr *addr, int addrlen, int type)
{
	rtp_member_t	*pmem = NULL;
	
	assert(mlist != NULL && addr != NULL);

	pmem = __mlist_lookup_byaddr(mlist, addr);
	
	if (pmem != NULL)	/* the receiver exists */
		return NULL;
	
	if (mlist->size < mlist->capacity) {	/* ok */
		pmem = &mlist->members[mlist->size];
		__INIT_MEMBER(pmem, 0, type)
		mlist->size++;
		__SET_MEMBER_ADDR(pmem, addr, addrlen);

	} else {
		mlist->capacity *= 2;
		pmem = (rtp_member_t *) 
		       malloc(sizeof(rtp_member_t) * mlist->capacity);
		memcpy(pmem, mlist->members, 
		       sizeof(rtp_member_t) * mlist->size);
		free(mlist->members);
		mlist->members = pmem;

		pmem = __mlist_add_byaddr(mlist, addr, addrlen, type);
	}
	
	return pmem;
}

int __mlist_del(rtp_member_list_t *mlist, rtp_member_t *m)
{
	size_t	i, n;
	char	*from, *to;
	
	assert(mlist);
	
	for (i = 0; i < mlist->size; ++i)
		if (&(mlist->members[i]) == m)
			break;
	
	if (i >= mlist->size)
		return -1;	/* failed */
	
	to = (char *) &mlist->members[i];
	from = (char *) &mlist->members[i + 1];
	
	n = (mlist->size - i - 1) * sizeof(rtp_member_t);
	
	for (i = 0; i < n; i++)
		*to++ = *from++;
	
	mlist->size--;
	
	return 0;	/* ok */
}

void __mlist_free(rtp_member_list_t *mlist)
{
	assert(mlist);
	free(mlist->members);
}

u_int32 __gen_ssrc(void)
{
	u_int32	ans = 0;
#ifdef	__UNIX     /* [ for UNIX ] */

	ans |= (getpid() & 0xffff0000);
	ans |= (__rtp_random() & 0xffff);
#else              /* [ for Windows ] */
	ans |= (GetCurrentThreadId() & 0xffff0000);
	ans |= (__rtp_random() & 0xffff);
#endif             /* [ #ifdef __UNIX ] */
	return ans;
}


int __getipaddr(const char *addrstr, struct in_addr *addr)
{
	if (addr == NULL)
		return -1;
	
	if ((addr->s_addr = inet_addr(addrstr)) == INADDR_NONE) { /* failed */
		struct hostent	*hp;
		if ((hp = gethostbyname(addrstr)) != NULL) {
			int	i;
			for (i = 0; i < hp->h_length; ++i)
				((char *)addr)[i] = hp->h_addr[i];
		} else
			return -1;
	}

	return 0;
}

u_int16 __rtp_random(void)
{
	return (rand() & 0xFFFF);
}

double __rtp_drandom(void)
{
	return ((double) __rtp_random()) / 0xFFFF;
}

u_int32 __rtp_time(void)
{
#ifdef	__UNIX     /* [ for UNIX ] */
	struct timeval	tv;
	gettimeofday(&tv, NULL);

	return (u_int32) (((unsigned) tv.tv_sec) * 1000) + tv.tv_usec / 1000;

#else              /* [ for Windows ] */
	return (u_int32) GetTickCount();
#endif             /* [ #ifdef __UNIX ] */
}


void __getntptime(u_int32 *sec, u_int32 *frac)
{
	double	x;
#ifdef	__UNIX     /* [ for UNIX ] */
	struct timeval	tv;
	gettimeofday(&tv, NULL);
	
	*sec = htonl(((u_int32) tv.tv_sec) + RTP_NTP_TIMEOFFSET);
	x = (double) tv.tv_usec;

#else              /* [ for Windows ] */
	const static u_int32	UNITS_PER_SEC = 10 * 1000 * 1000;
	const static u_int32	FACTOR = (0xffffffff / (10 * 1000 * 1000));
	static SYSTEMTIME	stm_1900 = {1900, 1, 0, 1, 0, 0, 0, 0};
	static FILETIME		ftm_1900 = {0, 0};
	SYSTEMTIME	stm;
	FILETIME	ftm;
	u_int32		ts;

	if (ftm_1900.dwHighDateTime == 0)
		SystemTimeToFileTime(&stm_1900, &ftm_1900);

	GetLocalTime(&stm);

	SystemTimeToFileTime(&stm, &ftm);

	if (ftm.dwLowDateTime > ftm_1900.dwLowDateTime) {
		ftm.dwHighDateTime -= ftm_1900.dwHighDateTime;
		ftm.dwLowDateTime -= ftm_1900.dwLowDateTime;
	} else {
		ftm.dwHighDateTime -= ftm_1900.dwHighDateTime;
		ftm.dwHighDateTime--;
		ftm.dwLowDateTime = ftm_1900.dwLowDateTime - 
		                    ftm.dwLowDateTime;
		ftm.dwLowDateTime = 0xffffffff - ftm.dwLowDateTime;
	}

	ts = ftm.dwLowDateTime / UNITS_PER_SEC;
	ts += ftm.dwHighDateTime * FACTOR;

	*sec = htonl(((u_int32) ts));
	x = (double) ((ftm.dwLowDateTime / 10) % (1000 * 1000));

#endif             /* [ #ifdef __UNIX ] */

	x *= (65536. * 65536.)/1000000;
	*frac = htonl((u_int32) x);
}
