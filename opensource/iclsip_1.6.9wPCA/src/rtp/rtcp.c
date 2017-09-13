/*=========================================================================*\
  Copyright (c) 2000-2002 Computer & Communications Research Laboratories,
                          Industrial Technology Research Institute
  
  RTCP - RTCP implementation
  
  Author: Jiun-Yao Huang <jyhuang@wizs.org>
  Revision: $Id: rtcp.c,v 1.41 2004/12/13 08:08:36 ljchuang Exp $
\*=========================================================================*/
#include "rtp.h"
#include "rtp_helper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int __rtcp_header_check(rtcp_pkt_t *r, size_t len);
static void __on_rtcp_received(const char *buf, size_t buflen);
static size_t __gen_report(RTP *rtp, rtp_member_t *m, 
                           char *buf, size_t buflen);
static size_t __gen_sdes(RTP *rtp, char *buf, size_t buflen);

extern u_int16	__rtp_port_base;
extern u_int16	__rtp_port_max;

RTCP *rtcp_open(const char* localaddr, short af, u_int16 port)
{
	RTCP *rtcp = NULL;
	ADDRINFO hints,*ai;
	char Port[16];

	sprintf(Port,"%d",port);

	rtcp = (RTCP *) malloc(sizeof(RTCP));
	if (rtcp == NULL)
		return NULL;

	memset(rtcp, 0, sizeof(RTCP));

	memset(&hints, 0, sizeof(hints));
   hints.ai_family = af;
   hints.ai_socktype = SOCK_DGRAM;
   hints.ai_flags = AI_PASSIVE;
	if(getaddrinfo(localaddr, Port, &hints, &ai) != 0) return NULL;
	memcpy(&rtcp->laddr,ai->ai_addr,ai->ai_addrlen);
	af = ai->ai_family;
	freeaddrinfo(ai);

	rtcp->sock = socket(af, SOCK_DGRAM, 0);
	
	if (rtcp->sock == INVALID_SOCKET) {
		free(rtcp);
		return NULL;
	}
	
	/*
	rtcp->laddr.sin_family = AF_INET;
	rtcp->laddr.sin_addr.s_addr = INADDR_ANY;
	 */
	
	if (port != 0) {
		/*rtcp->laddr.sin_port = htons(port);*/
		((struct sockaddr_in*)&(rtcp->laddr))->sin_port = htons(port);
		if (bind(rtcp->sock, (struct sockaddr *) &rtcp->laddr,
		         sizeof(rtcp->laddr)) == SOCKET_ERROR) {
			/* cannot bind local address */
			free(rtcp);
			return NULL;
		}
	} else {
		int	i, r;

		for (i = __rtp_port_base + 1; i < __rtp_port_max; i += 2) {
			/*rtcp->laddr.sin_port = htons(i);*/
			((struct sockaddr_in*)&(rtcp->laddr))->sin_port = htons(i);
			r = bind(rtcp->sock, (struct sockaddr *) &rtcp->laddr,
			         sizeof(rtcp->laddr));
			if (r != SOCKET_ERROR)	/* found */
				break;
		}

		if (i >= __rtp_port_max) {
			closesocket(rtcp->sock);
			free(rtcp);
			return NULL;
		}
	}
	
	rtcp->init_flag = 1;
		
	return rtcp;
}

int rtcp_close(RTCP *rtcp)
{
	assert(rtcp != NULL);

	if (rtcp == NULL)
		return -1;

	if (rtcp->sock != INVALID_SOCKET) {
		closesocket(rtcp->sock);
		rtcp->sock = INVALID_SOCKET;
	}
	
	free(rtcp);

	return 0;
}

SOCKET rtcp_getsocket(RTCP *rtcp)
{
	return rtcp->sock;
}

int rtcp_setpeeraddr(RTCP *rtcp, short af, const char *host, u_int16 port)
{
	char buf[16];
	ADDRINFO hints, *ai;

	assert(rtcp != NULL);
	assert(host != NULL);

	if (rtcp == NULL || host == NULL) {
		if (rtcp)
			rtcp->err = RTP_EINVAL;
		return -1;
	}

	/*
	if (__getipaddr(host, &inaddr) < 0)
		return -1;
	
	rtcp->paddr.sin_family = AF_INET;
	rtcp->paddr.sin_addr = inaddr;
	rtcp->paddr.sin_port = htons(port);
	*/
	sprintf(buf,"%d",port);
	memset(&hints,0,sizeof(hints));
	hints.ai_family = af;
	if(getaddrinfo(host,buf,&hints,&ai)!= 0) return -1;
	memcpy(&rtcp->paddr,ai->ai_addr,ai->ai_addrlen);
	
	if (connect(rtcp->sock, (struct sockaddr *) &rtcp->paddr, 
	            sizeof(rtcp->paddr)) == SOCKET_ERROR) {
		/* cannot make a connected UDP socket */
		rtcp->err = RTP_ECONN;
		return -1;
	}

	freeaddrinfo(ai);
	
	return 0;
}

int rtcp_setpeeraddrx(RTCP *rtcp, const SOCKADDR_STORAGE *addr)
{
	assert(rtcp != NULL);
	assert(addr != NULL);

	if (rtcp == NULL || addr == NULL) {
		if (rtcp)
			rtcp->err = RTP_EINVAL;
		return -1;
	}
	
	memcpy(&rtcp->paddr, addr, sizeof(SOCKADDR_STORAGE));

	if (connect(rtcp->sock, (struct sockaddr *) &rtcp->paddr, 
	            sizeof(rtcp->paddr)) == SOCKET_ERROR) {
		/* cannot make a connected UDP socket */
		rtcp->err = RTP_ECONN;
		return -1;
	}

	return 0;
}

int rtcp_errno(const RTCP *rtcp)
{
	assert(rtcp != NULL);

	return rtcp->err;
}

int rtcp_read(RTCP *rtcp, char *buf, size_t buflen)
{
	int	n;
	SOCKADDR_STORAGE	fromaddr;
	socklen_t		fromlen = sizeof(fromaddr);

	assert(rtcp != NULL);

	if (rtcp == NULL)
		return -1;
	
	if (buf == NULL || buflen == 0) {
		rtcp->err = RTP_EBUF;
		return -1;
	}

	n = recvfrom(rtcp->sock, buf, buflen, 0, 
	             (struct sockaddr *) &fromaddr, &fromlen);
	if (n < 0) {	/* error */
		/* error handling */
		rtcp->err = RTP_EIO;
		return -1;
	}
	
	__on_rtcp_received(buf, n);

	return n;
}

int rtcp_readx(RTCP *rtcp, char *buf, size_t buflen, long timeout)
{
	assert(rtcp != NULL);

	if (rtcp == NULL)
		return -1;
	
	if (timeout < 0) {
		return rtcp_read(rtcp, buf, buflen);
	} else {
		int	n;
		fd_set	readset;
		struct timeval	tv;

		FD_ZERO(&readset);
		FD_SET(rtcp->sock, &readset);

		tv.tv_sec = timeout / 1000;
		tv.tv_usec = (timeout % 1000) * 1000;

		n = select(rtcp->sock + 1, &readset, NULL, NULL, &tv);
		
		if (n < 0) {
			/* select() failed */
			rtcp->err = RTP_EIO;
			return -1;
		}

		if (n > 0 && FD_ISSET(rtcp->sock, &readset))
			return rtcp_read(rtcp, buf, buflen);
	}
	
	rtcp->err = RTP_ETIMEOUT;

	return -1;
}

int rtcp_write(RTCP *rtcp, const char *buf, size_t buflen)
{
	int	n;

	assert(rtcp != NULL);

	if (rtcp == NULL)
		return -1;
	
	if (buf == NULL || buflen == 0) {
		rtcp->err = RTP_EBUF;
		return -1;
	}
	
	n = sendto(rtcp->sock, buf, buflen, 0,
	         (struct sockaddr *) &(rtcp->paddr), sizeof(rtcp->paddr));
	
	if (n < 0) {
		rtcp->err = RTP_EIO;
		return -1;
	}
	
	return n;
}

static void __on_rtcp_received(const char *buf, size_t buflen)
{
	if (__rtcp_header_check((rtcp_pkt_t *) buf, buflen) < 0)  /* failed */
		return;
}

int __rtcp_read_hook(RTP *rtp, RTCP *rtcp)
{
	int	n;
	struct timeval	tv = {0, 0};
	fd_set	rset;
	char	buf[RTCP_PKTBUFSIZE];
	
	/* check RTCP packet */
	FD_ZERO(&rset);
	FD_SET(rtcp->sock, &rset);
	
	n = select(rtcp->sock + 1, &rset, NULL, NULL, &tv);
	if (n < 0) {
		/* handle error condition */
		return -1;
	} else if (n == 0) {
		return 0;
	}
	
	if (FD_ISSET(rtcp->sock, &rset)) {
		n = rtcp_read(rtcp, buf, sizeof(buf));
		if (n < 0)
			return -1;
	}

	return 0;
}

int __rtcp_write_hook(RTP *rtp, RTCP *rtcp)
{

	return 0;
}

/* len: length of compound RTCP packet in 32-bit words */
static int __rtcp_header_check(rtcp_pkt_t *r, size_t len)
{
	rtcp_pkt_t	*end;
	
	if ((*(u_int16 *)r & RTCP_VALID_MASK) != RTCP_VALID_VALUE) {
		return -1;
	}
	
	end = (rtcp_pkt_t *)((u_int32 *)r + len);
	
	do r = (rtcp_pkt_t *)((u_int32 *)r + r->common.len + 1);
	while (r < end && r->common.ver == RTP_VERSION);

	if (r != end) {
		return -1;
	}

	return 0;
}

/* computing the RTCP transmission interval (from RFC 1889) */
u_int32 __rtcp_interval(int members, int senders, double rtcp_bw, int we_sent,
                        double avg_rtcp_size, int initial)
{
	/* 
	 * Minimum average time between RTCP packets from this site (in
	 * seconds).  This time prevents the reports from 'clumping' when
	 * sessions are small and the law of large numbers isn't helping
	 * to smooth out the traffic.  It also keeps the report interval
	 * from becoming ridiculously small during transient outages like
	 * a network partition.
	 */
	const double RTCP_MIN_TIME = 5.;
	
	/* 
	 * Fraction of the RTCP bandwidth to be shared among active 
	 * senders. (This fraction was chosen so that in a typical 
	 * session with one or two active senders, the computed report 
	 * time would be roughly equal to the minimum report time so that 
	 * we don't unnecessarily slow down receiver reports.)  
	 * The receiver fraction must be 1 - the sender fraction.
	 */
	const double RTCP_SENDER_BW_FRACTION = 0.25;
	const double RTCP_RECEIVER_BW_FRACTION = (1 - RTCP_SENDER_BW_FRACTION);
	
	/*
	 * to compensate for "unconditional reconsideration" converging to a
	 * value below the intended average.
	 */
	const double COMPENSATION = 2.71828 - 1.5;
	
	double	t;	/* interval */
	double	rtcp_min_time = RTCP_MIN_TIME;
	int	n;	/* # members for computation */
	
	/*
	 * Very first call at application shart-up uses half the min delay
	 * for quicker notification while still allowing some time before
	 * reporting for randomization and to learn about other sources
	 * so the report interval will converge to the correct interval
	 * more quickly.
	 */
	if (initial)
		rtcp_min_time /= 2;
	
	/* 
	 * If there were active senders, give them at least a minimum share
	 * of the RTCP bandwidth.  Otherwise all participants share the
	 * RTCP bandwidth equally.
	 */
	 n = members;
	 if (senders > 0 && senders < members * RTCP_SENDER_BW_FRACTION) {
	 	if (we_sent) {
	 		rtcp_bw *= RTCP_SENDER_BW_FRACTION;
	 		n = senders;
	 	} else {
	 		rtcp_bw *= RTCP_RECEIVER_BW_FRACTION;
	 		n -= senders;
	 	}
	 }
	
	/* 
	 * The effective number of sites times the average packet size is
	 * the total number of octets sent when each site sends a report.
	 * Dividing this by the effective bandwidth gives the time
	 * interval over which those packets must be sent in order to
	 * meet the bandwidth target, with a minimum enforced. In that
	 * time interval we send one report so this time is also our
	 * average time between reports.
	 */
	t = avg_rtcp_size * n / rtcp_bw;
	if (t < rtcp_min_time)
		t = rtcp_min_time;
	
	/*
	 * To avoid traffic bursts from unintended synchronization with
	 * other sites, we then pick our actual next report interval as a
	 * random number uniformly distributed between 0.5*t and 1.5*t.
	 */
	t = t * (__rtp_drandom() + 0.5);
	t = t / COMPENSATION;
	
	/*
	rtptrace("__rtcp_interval(%d,%d,%.0f,%d,%.0f,%d) = %f\n", 
	         members, senders, rtcp_bw, we_sent, avg_rtcp_size, initial, 
	         (t * 1000));
 */
	return (u_int32) (t * 1000);
}

static size_t __gen_sdes(RTP *rtp, char *buf, size_t buflen)
{
	rtcp_pkt_t	*pkt = (rtcp_pkt_t *) &buf[0];
	size_t	n = 0, count = 0;

	rtcp_sdes_type_t	types[10];
	char			*values[10];
	int			length[10];

	pkt->common.ver = RTP_VERSION;
	pkt->common.p = 0;
	pkt->common.count = 1;		/* initial count */
	pkt->common.pt = RTCP_SDES;
	
	if (rtp->info.cname) {
		types[count]	= RTCP_SDES_CNAME;
		values[count]	= rtp->info.cname;
		length[count]	= strlen(rtp->info.cname);
		count++;
	}
	
	if (rtp->info.name) {
		types[count]	= RTCP_SDES_NAME;
		values[count]	= rtp->info.name;
		length[count]	= strlen(rtp->info.name);
		count++;
	}

	if (rtp->info.email) {
		types[count]	= RTCP_SDES_EMAIL;
		values[count]	= rtp->info.email;
		length[count]	= strlen(rtp->info.email);
		count++;
	}

	if (rtp->info.phone) {
		types[count]	= RTCP_SDES_PHONE;
		values[count]	= rtp->info.phone;
		length[count]	= strlen(rtp->info.phone);
		count++;
	}

	if (rtp->info.loc) {
		types[count]	= RTCP_SDES_LOC;
		values[count]	= rtp->info.loc;
		length[count]	= strlen(rtp->info.loc);
		count++;
	}

	if (rtp->info.tool) {
		types[count]	= RTCP_SDES_TOOL;
		values[count]	= rtp->info.tool;
		length[count]	= strlen(rtp->info.tool);
		count++;
	}

	if (rtp->info.note) {
		types[count]	= RTCP_SDES_NOTE;
		values[count]	= rtp->info.note;
		length[count]	= strlen(rtp->info.note);
		count++;
	}

	if (rtp->info.priv) {
		types[count]	= RTCP_SDES_PRIV;
		values[count]	= rtp->info.priv;
		length[count]	= strlen(rtp->info.priv);
		count++;
	}
	
	types[count]	= RTCP_SDES_END;
	values[count]	= "";
	length[count]	= 0;
	count++;
	
	n = __write_sdes((char *) &(pkt->r.sdes),
	                 buflen - sizeof(rtcp_common_t), 
	                 rtp->ssrc, count, types, values, length);

	n += sizeof(rtcp_common_t);
	
	pkt->common.len = htons(n / 4 - 1 + (n % 4 ? 1 : 0));
	
	return n;
}

/* return the length of occupied buffer */
int __write_sdes(char *buf, size_t buflen, u_int32 ssrc, int count, 
                 rtcp_sdes_type_t type[], char *value[], int length[])
{
	char	*b = &buf[0];
	rtcp_sdes_t	*s = (rtcp_sdes_t *) b;
	rtcp_sdes_item_t *rsp;
	int	i, len, pad;
	
	/* SSRC header */
	s->src = htonl(ssrc);
	rsp = &s->item[0];
	
	/* SDES items */
	for (i = 0; i < count; i++) {
		rsp->type = type[i];
		len = length[i];
		if (len > RTP_MAX_SDES) {
			/* invalid length */
			len = RTP_MAX_SDES;
		}
		rsp->len = len;
		memcpy(rsp->data, value[i], len);
		rsp = (rtcp_sdes_item_t *) &rsp->data[len];
	}
	
	/* terminate with end marker and pad to next 4-octet boundary */
	len = ((char *) rsp) - b;
	pad = 4 - (len & 0x3);
	b = (char *) rsp;
	while (pad--)
		*b++ = RTCP_SDES_END;

	return (int) (b - buf);
}

size_t __send_rtcp_BYE(RTP *rtp, rtp_member_t *m)
{
	size_t	n = 0;
	char	buf[RTCP_PKTBUFSIZE];
	rtcp_pkt_t	*pkt = (rtcp_pkt_t *) &buf[0];

	/*rtptrace("__send_rtcp_BYE: SSRC=%u\n", m->ssrc);*/
	
	pkt->common.ver = RTP_VERSION;
	pkt->common.p = 0;
	pkt->common.count = 1;
	pkt->common.pt = RTCP_BYE;
	
	n = sizeof(rtcp_common_t) + sizeof(u_int32) * pkt->common.count;

	if (m == NULL) { 	/* send BYE to all receivers */
		size_t	i;

		pkt->r.bye.src[0] = htonl(m->ssrc);
		for (i = 0; i < rtp->mlist.size; i++) {
			n = sendto(rtp->rtcp->sock, buf, n, 0,
			           (struct sockaddr *) 
			           &rtp->mlist.members[i].caddr,
			           sizeof(SOCKADDR_STORAGE));
		}

	} else {	/* send BYE to a specific receiver */
		pkt->r.bye.src[0] = htonl(m->ssrc);
		n = sendto(rtp->rtcp->sock, buf, n, 0,
		           (struct sockaddr *) &m->caddr,
		           sizeof(SOCKADDR_STORAGE));
	}
	
	return n;
}

size_t  __send_rtcp_report(RTP *rtp)
{
	size_t	i, n = 0, n2 = 0;
	char	buf[RTCP_PKTBUFSIZE];
	
	for (i = 0; i < rtp->mlist.size; i++) {
		n = __gen_report(rtp, &rtp->mlist.members[i],
		                 buf, sizeof(buf));
		
		/* append SDES */
		n2 = __gen_sdes(rtp, &buf[n], sizeof(buf) - n);

		n += n2;
		
		/*rtptrace("__send_rtcp_report: __gen_report=%d\n", n);*/

		n = sendto(rtp->rtcp->sock, buf, n, 0,
		           (struct sockaddr *) &rtp->mlist.members[i].caddr,
		           sizeof(SOCKADDR_STORAGE));
	}
	
	/* rtptrace("__send_rtcp_report\n"); */
	
	return n;
}

static size_t __gen_report(RTP *rtp, rtp_member_t *m, char *buf, size_t buflen)
{
	size_t		n;
	rtcp_pkt_t	*pkt = (rtcp_pkt_t *) &buf[0];
	rtcp_rr_t	*rr;
	
	pkt->common.ver = RTP_VERSION;
	pkt->common.p = 0;
	pkt->common.count = 0;
	pkt->common.pt = RTCP_RR;
	
	if (buflen < sizeof(rtp_pkt_t))
		return 0;
	
	rr = &pkt->r.rr.rr[0];

	if (m->type & RTP_RECEIVER) {	/* generate SR */
		/*rtptrace("__gen_report: SR\n");*/

		pkt->common.pt = RTCP_SR;
		pkt->r.sr.ssrc = htonl(rtp->ssrc);
		__getntptime(&pkt->r.sr.ntp_sec, &pkt->r.sr.ntp_frac);
		pkt->r.sr.rtp_ts = rtp->last_ts;
		pkt->r.sr.osent = htonl(m->send_bytes);
		pkt->r.sr.psent = htonl(m->send_packets);
		rr = &pkt->r.sr.rr[0];
	}

	if (m->type & RTP_SENDER) {	/* generate RR blocks */
		u_int32	lost;

		/*rtptrace("__gen_report: RR\n");*/

		/* TODO: support multiple RR blocks in multicasting 
		         environment. */

		pkt->common.count = 1;
		pkt->r.rr.ssrc = htonl(rtp->ssrc);
		rr->ssrc = htonl(m->ssrc);
		rr->fraction = __cal_packet_lost(&m->src, 1);
		rr->lseq = htonl(m->src.cycles + m->src.max_seq);
		lost = rr->lseq - m->src.received;

		if (lost != htonl(lost))
			rr->lost = htonl(lost << 8);
		else
			rr->lost = htonl(lost);

		rr->jitter = htonl(m->src.jitter);
		rr->lsr = htonl(m->lsr);

		if (m->lsr == 0) {
			rr->dlsr = 0;
		} else {
			u_int32	t, f;
			__getntptime(&t, &f);
			rr->dlsr = rr->lsr - ((t & 0xffff0000) + (f & 0xffff));
		}
	}
	
	n = ((char *) rr) - buf + sizeof(rtcp_rr_t) * pkt->common.count;

	pkt->common.len = htons(n / 4 - 1 + (n % 4 ? 1 : 0));

	return n;
}

void __schedule(RTP *rtp, u_int32 t, int event)
{
	/*rtptrace("__schedule: event=%d, t=%u\n", event, t);*/

	if (event == __EVENT_REPORT)
		rtp->events.ts_report = t;
	else if (event == __EVENT_BYE)
		rtp->events.ts_bye = t;
}

int __on_expire(RTP *rtp, rtp_event_t e)
{
	/*
	 * This function is responsible for deciding whether to send an
	 * RTCP report or BYE packet now, or to reschedule transmission.
	 * It is also responsible for updating some variables.  This
	 * function should be called upon expiration of event timer used
	 * __schedule().
	 */
	size_t	n;
	 
	RTCP *rtcp = rtp->rtcp;
	 
	u_int32	t;	/* interval */
	u_int32	tn;	/* next transmit time */
	
	u_int32	tc = __rtp_time();
	 
	/* 
	 * In the case of a BYE, we use "unconditional reconsideration"
	 * to reschedule the transmission of the BYE if necessary
	 */
	if (e == __EVENT_BYE) {
		/*rtptrace("__on_expire: e == __EVENT_BYE\n");*/

		t = __rtcp_interval(rtp->mlist.size,
		                    rtp->mlist.senders,
		                    rtcp->rtcp_bw,
		                    rtcp->we_sent,
		                    rtcp->avg_rtcp_size,
		                    rtcp->init_flag);
		
		tn = rtcp->time_prev + t;
		
		if (tn <= tc) {
			__send_rtcp_BYE(rtp, NULL);
			return 1;
		} else {
			__schedule(rtp, tn, e);
		}
		
	} else if (e == __EVENT_REPORT) {
		t = __rtcp_interval(rtp->mlist.size,
		                    rtp->mlist.senders,
		                    rtcp->rtcp_bw,
		                    rtcp->we_sent,
		                    rtcp->avg_rtcp_size,
		                    rtcp->init_flag);
		
		tn = rtcp->time_prev + t;
		
		if (tn <= tc) {
			n = __send_rtcp_report(rtp);

			rtcp->avg_rtcp_size = (1./16.) * n + 
			                      (15./16.) * rtcp->avg_rtcp_size;
			rtcp->time_prev = tc;
			
			/* 
			 * We must redraw the interval. Don't reuse the one
			 * computed above, since its not actually distributed
			 * the same, as we are conditioned on it being small
			 * enough to cause a packet to be sent
			 */
			t = __rtcp_interval(rtp->mlist.size, 
			                    rtp->mlist.senders,
			                    rtcp->rtcp_bw, 
			                    rtcp->we_sent, 
			                    rtcp->avg_rtcp_size, 
			                    rtcp->init_flag);

			__schedule(rtp, t + tc, e);
			rtcp->init_flag = 0;
		} else {	
			__schedule(rtp, tn, e);
		}
		
		rtcp->pmembers = rtp->mlist.size;
	}

	return 0;
}
