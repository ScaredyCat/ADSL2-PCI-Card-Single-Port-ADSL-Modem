/*=========================================================================*\
  Copyright (c) 2000-2002 Computer & Communications Research Laboratories,
                          Industrial Technology Research Institute
  
  RTP helper 
  
  Author: Jiun-Yao Huang <jyhuang@wizs.org>
  Revision: $Id: rtp_helper.h,v 1.23 2004/12/13 08:08:36 ljchuang Exp $
  Description:
\*=========================================================================*/
#ifndef	__RTP_HELPER_H__
#define	__RTP_HELPER_H__

#include "rtp.h"

typedef enum rtp_event_e {
	__EVENT_REPORT = 1,
	__EVENT_BYE = 2
} rtp_event_t;

/* duration between 1900/1/1 00:00:00 and 1970/1/1 00:00:00 */
#define	RTP_NTP_TIMEOFFSET		2208992400UL

/* debug/trace function */
#ifdef	RTPTRACE

#include <stdio.h>
#include <stdarg.h>
void rtptrace(const char *fmt, ...);

#else /* !RTPTRACE */

#ifdef	__GNUC__	/* gcc suports variable argument lists for macro */
/* #define	rtptrace(fmt, arg...)	((void) 0) */
void rtptrace(const char *fmt, ...);
#else	/* !__GNUC */
void rtptrace(const char *fmt, ...);
#endif	/* __GNU__ */

#endif	/* RTPTRACE */

#define	__SET_MEMBER_ADDR(m, a, alen)	{ \
		  memset(&m->addr, 0, sizeof(m->addr));  \
        memcpy(&m->addr, a, alen);  \
        memcpy(&m->caddr, a, alen); \
        ((struct sockaddr_in*)&(m->caddr))->sin_port = htons(ntohs(((struct sockaddr_in*)a)->sin_port) + 1);    \
}


/* helper functions for member list */
void __mlist_init(rtp_member_list_t *mlist);
rtp_member_t *__mlist_lookup(rtp_member_list_t *mlist, u_int32 ssrc);
rtp_member_t *__mlist_lookup_byaddr(rtp_member_list_t *mlist,
                                    const struct sockaddr *addr);
rtp_member_t *__mlist_add(rtp_member_list_t *mlist, u_int32 ssrc, int type);
rtp_member_t *__mlist_add_byaddr(rtp_member_list_t *mlist, 
                                 const struct sockaddr *addr, int addrlen, int type);
int __mlist_del(rtp_member_list_t *mlist, rtp_member_t *m);
void __mlist_free(rtp_member_list_t *mlist);

/* update == 0: does not update src; update != 0: update src */
u_int8 __cal_packet_lost(rtp_source_t *src, int update);

u_int16 __rtp_random(void);
double __rtp_drandom(void);

/* a simple function that generates SSRC */
u_int32 __gen_ssrc(void);

int __getipaddr(const char *addrstr, struct in_addr *addr);

/* return current time, in millisecond */
u_int32 __rtp_time(void);

/* RTCP handler */
int __rtcp_read_hook(RTP *rtp, RTCP *rtcp);
int __rtcp_write_hook(RTP *rtp, RTCP *rtcp);

size_t __send_rtcp_BYE(RTP *rtp, rtp_member_t *m);

int __write_sdes(char *buf, size_t buflen, u_int32 ssrc, int count, 
                 rtcp_sdes_type_t type[], char *value[], int length[]);

void __schedule(RTP *rtp, u_int32 t, int event);

void __getntptime(u_int32 *sec, u_int32 *frac);

#endif /* #ifndef __RTP_HELPER_H__ */
