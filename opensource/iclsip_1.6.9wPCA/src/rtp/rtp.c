/*=========================================================================*\
  Copyright (c) 2000-2002 Computer & Communications Research Laboratories,
                          Industrial Technology Research Institute
  
  RTP - RTP implementation
  
  Author: Jiun-Yao Huang <jyhuang@wizs.org>
  Revision: $Id: rtp.c,v 1.84 2005/05/23 13:54:01 ljchuang Exp $
\*=========================================================================*/
#include "rtp.h"
#include "rtp_helper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*HANDLE		hSemSendWait; */ /*add by Alan for k 9403117*/

static RTP *__rtp_open(const char* localaddr, short af, u_int16 port, u_int32 ssrc);
static void __init_seq(rtp_source_t *src, u_int16 seq);
static int __update_seq(rtp_source_t *src, u_int16 seq);
int __on_expire(RTP *rtp, rtp_event_t e);

#ifdef	__UNIX     /* [ for UNIX ] */

#else              /* [ for Windows ] */

static WSADATA	wsadata;
static int	need_winsock_cleanup = 0;

#endif             /* [ #ifdef __UNIX ] */

u_int16	rtp_port_base = RTP_PORT_BASE;
u_int16	rtp_port_max = RTP_PORT_MAX;

u_int16	__rtp_port_base = RTP_PORT_BASE;
u_int16	__rtp_port_max = RTP_PORT_MAX;


#define	__CHECK_RTP_EVENTS(rtp)	{				\
		u_int32	tc = __rtp_time();			\
		if (rtp->events.ts_report != 0 &&		\
		    rtp->events.ts_report <= tc)		\
			__on_expire(rtp, __EVENT_REPORT);	\
		if (rtp->events.ts_bye != 0 && 			\
		    rtp->events.ts_bye <= tc)			\
			__on_expire(rtp, __EVENT_BYE);		\
	}

#define	__CHECK_AND_FREE(__ptr) 				\
		if (__ptr) {					\
			free(__ptr);				\
			__ptr = NULL;				\
		}

int rtp_startup(void)
{
	int	ret = 0;

#ifdef	__UNIX     /* [ for UNIX ] */
	/* nothing by default */
#else              /* [ for Windows ] */
	SOCKET	sock;
	
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
		/* we need to initialize winsock */
		DWORD	dw;

		dw = WSAStartup(MAKEWORD(2, 1), &wsadata);

		if (dw == 0)
			need_winsock_cleanup = 1;
		else
			return -1;
	} else {	/* no need to initialize winsock */
		closesocket(sock);
	}
	
#endif             /* [ #ifdef __UNIX ] */


	srand((unsigned) __rtp_time());
	rand();
	rand();
	
	__rtp_port_base = rtp_port_base;
	__rtp_port_max = rtp_port_max;


	return ret;
}

int rtp_cleanup(void)
{
#ifdef	__UNIX     /* [ for UNIX ] */

#else              /* [ for Windows ] */
	if (need_winsock_cleanup)
		WSACleanup();

#endif             /* [ #ifdef __UNIX ] */

	rtp_port_base = RTP_PORT_BASE;
	rtp_port_max = RTP_PORT_MAX;

	return 0;
}

int rtp_errno(const RTP *rtp)
{
	return rtp->err;
}

RTP *rtp_open(const char* localaddr, short af, u_int16 port, u_int32 ssrc, const char *cname)
{
	RTP	*rtp;

	rtp = __rtp_open(localaddr, af, port, ssrc);
	if (rtp == NULL)	/* failed */
		return NULL;

	memset(&(rtp->info), 0, sizeof(__rtp_sdes_info_t));
	
	rtp_sdes_cname(rtp, cname);
	
	rtp->auto_rtcp = 1;
	/* RTCP port = RTP port + 1 */
	rtp->rtcp = rtcp_open(localaddr, af,ntohs(((struct sockaddr_in*)&rtp->laddr)->sin_port) + 1);
	if (rtp->rtcp == NULL) {	/* failed to create an RTCP session */
		rtp_close(rtp);
		return NULL;
	}
	/* default session bandwidth */
	rtp_setbw(rtp, RTP_DEF_SESSION_BW);

	port = ntohs(((struct sockaddr_in*)&rtp->laddr)->sin_port);

	/*hSemSendWait = CreateSemaphore(NULL,0,1,_T("hSemSendWait"));*/ /*add by Alan for k 9403117*/
	
	return rtp;
}

static RTP *__rtp_open(const char* localaddr, short af, u_int16 port, u_int32 ssrc)
{
	RTP	*rtp = NULL;
	int	ret;

	ADDRINFO hints, *ai;
	char Port[16];
	sprintf(Port, "%d", port);

	rtp = (RTP *) malloc(sizeof(RTP));
	if (rtp == NULL)
		return NULL;
	
	memset(rtp, 0, sizeof(RTP));

	/*
	rtp->laddr.sin_family = AF_INET;
	rtp->laddr.sin_addr.s_addr = INADDR_ANY;
	*/

   memset(&hints, 0, sizeof(hints));
   hints.ai_family = af;
   hints.ai_socktype = SOCK_DGRAM;
   hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = IPPROTO_UDP;
	
	if(getaddrinfo(localaddr, Port, &hints, &ai) != 0) return NULL;
	
	memcpy(&rtp->laddr,ai->ai_addr,ai->ai_addrlen);
	af = ai->ai_family;
	freeaddrinfo(ai);

	if (port != 0) {
		rtp->sock = socket(af, SOCK_DGRAM, 0);
		if (rtp->sock == INVALID_SOCKET) {
			/* cannot create a UDP socket */
			free(rtp);
			return NULL;
		}

		/*rtp->laddr.sin_port = htons(port);*/
		ret = bind(rtp->sock, (struct sockaddr *) &rtp->laddr,
		           sizeof(rtp->laddr));
		if (ret == SOCKET_ERROR) {
			/* cannot bind local address */
			closesocket(rtp->sock);
			free(rtp);
			return NULL;
		}

	} else {	/* find the unbound port pair for RTP and RTCP */
		SOCKET	sd;
		SOCKADDR_STORAGE	addr;
		int	ret2;
		unsigned short	i;

		/*
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = INADDR_ANY;
		*/

		memset(&addr,0,sizeof(addr));

		if(getaddrinfo(localaddr, Port, &hints, &ai) != 0) return NULL;
		memcpy(&addr,ai->ai_addr,ai->ai_addrlen);
		af = ai->ai_family;
		freeaddrinfo(ai);
				
		rtp->sock = sd = INVALID_SOCKET;
		for (i = __rtp_port_base; i < __rtp_port_max; i += 2) {
			if (rtp->sock == INVALID_SOCKET) {
				rtp->sock = socket(af, SOCK_DGRAM, 0);
				if (rtp->sock == INVALID_SOCKET) {
					/* cannot create a UDP socket */
					free(rtp);
					return NULL;
				}
			}

			if (sd == INVALID_SOCKET) {
				sd = socket(af, SOCK_DGRAM, 0);
				if (sd == INVALID_SOCKET) {
					/* cannot create a UDP socket */
					free(rtp);
					return NULL;
				}
			}
		
			/*
			rtp->laddr.sin_port = htons(i);
			addr.sin_port = htons((unsigned short) (i + 1));
			*/
			((struct sockaddr_in*)&(rtp->laddr))->sin_port = htons(i);
			((struct sockaddr_in*)&(addr))->sin_port = htons((unsigned short) (i + 1));

			ret = bind(rtp->sock, (struct sockaddr *) &rtp->laddr,
			           sizeof(rtp->laddr));
			ret2 = bind(sd, (struct sockaddr *) &addr,
			            sizeof(addr));

			if (ret != SOCKET_ERROR && ret2 != SOCKET_ERROR) {
				closesocket(sd);
				break;
			}
			
			if (ret != SOCKET_ERROR) {
				closesocket(rtp->sock);
				rtp->sock = INVALID_SOCKET;
			}
			if (ret2 != SOCKET_ERROR) {
				closesocket(sd);
				sd = INVALID_SOCKET;
			}
		}
		
		if (i >= __rtp_port_max) {
			if (rtp->sock != INVALID_SOCKET)
				closesocket(rtp->sock);
			free(rtp);
			return NULL;
		}
		
		if (rtp->sock == INVALID_SOCKET) {
			free(rtp);
			return NULL;
		}
		
	}
	
	rtp->auto_rtcp = 0;
	
	/* generate my SSRC */
	rtp->ssrc = (ssrc == 0) ? __gen_ssrc() : ssrc;
	
	/* initialize member list */
	__mlist_init(&rtp->mlist);
	
	return rtp;
}

int rtp_get_port(RTP *rtp)
{
	assert(rtp != NULL);
	if (rtp == NULL)
		return -1;

	return ntohs(((struct sockaddr_in*)&rtp->laddr)->sin_port);
}

int rtp_close(RTP *rtp)
{
	assert(rtp != NULL);
	if (rtp == NULL)
		return -1;
	
	if (rtp->rtcp)
		rtcp_close(rtp->rtcp);
	
	if (rtp->sock != INVALID_SOCKET) {
		closesocket(rtp->sock);
		rtp->sock = INVALID_SOCKET;
	}
	
	__mlist_free(&rtp->mlist);
	
	/*
	 * free allocated memory for SDES infomation
	 */
	__CHECK_AND_FREE(rtp->info.cname);
	__CHECK_AND_FREE(rtp->info.name);
	__CHECK_AND_FREE(rtp->info.email);
	__CHECK_AND_FREE(rtp->info.phone);
	__CHECK_AND_FREE(rtp->info.loc);
	__CHECK_AND_FREE(rtp->info.tool);
	__CHECK_AND_FREE(rtp->info.note);
	__CHECK_AND_FREE(rtp->info.priv);

	free(rtp);

	/*CloseHandle(hSemSendWait);*/ /*add by Alan for k 9403117*/

	return 0;
}

SOCKET rtp_getsocket(RTP *rtp)
{
	return rtp->sock;
}

double rtp_setbw(RTP *rtp, double bw)
{
	double	d;
	assert(rtp != NULL);
	
	d = rtp->session_bw;
	rtp->session_bw = bw;
	
	if (rtp->rtcp != NULL)
		rtp->rtcp->rtcp_bw = rtp->session_bw * RTP_RTCP_BW_FRACTION;

	return d;
}

double rtp_getbw(const RTP *rtp)
{
	assert(rtp != NULL);
	
	return rtp->session_bw;
}

size_t rtp_member_count(RTP *rtp)
{
	assert(rtp != NULL);
	if (rtp == NULL)
		return 0;

	return rtp->mlist.size;
}

size_t rtp_member_list(RTP *rtp, u_int32 ssrclist[], size_t count)
{
	size_t	max, i;
	assert(rtp != NULL);
	if (rtp == NULL)
		return 0;

	max = count > rtp->mlist.size ? rtp->mlist.size : count;
	for (i = 0; i < max; i++)
		ssrclist[i] = rtp->mlist.members[i].ssrc;

	return max;
}

int rtp_add_receiver(RTP *rtp, const struct sockaddr *addr, int addrlen)
{
	rtp_member_t	*m;
	
	assert(rtp != NULL);
	if (rtp == NULL)
		return -1;	/* failed */
	
	m = __mlist_lookup_byaddr(&rtp->mlist, addr);
	if (m == NULL) {
		m = __mlist_add_byaddr(&rtp->mlist, addr, addrlen, RTP_RECEIVER);
		rtp->mlist.receivers++;
	} else {
		if ((m->type & RTP_RECEIVER) == 0)
			rtp->mlist.receivers++;
		m->type |= RTP_RECEIVER;
	}

	if (m == NULL)
		return -1;

	/* initialize seq # */
	m->last_seq = __rtp_random();

	__schedule(rtp, __rtp_time(), __EVENT_REPORT);
	
	return 0;
}

int rtp_del_receiver(RTP *rtp, const struct sockaddr *addr)
{
	/* Removed from member list */
	rtp_member_t	*m;
	
	assert(rtp != NULL);
	if (rtp == NULL)
		return -1;	/* failed */

	assert(addr != NULL);
	if (addr == NULL)
		return -1;	/* failed */
	
	/* m = __mlist_lookup(&rtp->mlist, ssrc); */
	m = __mlist_lookup_byaddr(&rtp->mlist, addr);
	if (m) {
		/* __send_rtcp_BYE(rtp, m); */
		/* __mlist_del(&rtp->mlist, ssrc); */
		__mlist_del(&rtp->mlist, m);
	} else {
		return -1;	/* not found */
	}
	
	return 0;		/* ok */
}

int rtp_read(RTP *rtp, rtp_pkt_t *pkt)
{
	int	n, transit, d, cflag = 0;
	u_int8	pad_len = 0, hdr_len = 0;
	u_int16	seq;
	u_int32	ssrc;
	rtp_hdr_t	*hdr;
	rtp_member_t	*m;
	SOCKADDR_STORAGE	fromaddr;
	socklen_t		fromlen = sizeof(fromaddr);
	
	assert(rtp != NULL && pkt != NULL);

	if (rtp == NULL || pkt == NULL) {
		if (rtp)  rtp->err = RTP_EINVAL;
		return -1;
	}

	n = recvfrom(rtp->sock, pkt->buf, pkt->buflen, 0, 
	             (struct sockaddr *) &fromaddr, &fromlen);
	if (n < 0) {	/* error */
		/* error handling */
		rtp->err = RTP_EIO;
		return -1;
	}

	hdr = (rtp_hdr_t *) pkt->buf;

	/* RTP version must equal 2 (RTP_VERSION) */
	if (hdr->ver != RTP_VERSION) {
		rtp->err = RTP_EVERSION;
		return -1;
	}
	
	/* calculate header length */
	hdr_len = sizeof(rtp_hdr_t) + (((int) hdr->cc) - 1) * sizeof(u_int32);

	/* check padding */
	if (hdr->p != 0) {
		/* 
		 * The last octet of the packet must contain a valid octet
		 * count (< packet length - header length).
		 */
		pad_len = ((u_int8 *)pkt->buf)[n-1];
		if (pad_len > (n - hdr_len)) {
			rtp->err = RTP_EPKT;
			return -1;
		}
	}

	/* check RTP extension */
	if (hdr->x != 0) {
		/* TODO: handle RTP extension */
	}

	/* calculate payload length */
	pkt->datalen = n - hdr_len - pad_len;

	seq = ntohs(hdr->seq);
	ssrc = ntohl(hdr->ssrc);

	m = __mlist_lookup(&rtp->mlist, ssrc);
	
	if (m == NULL) {	/* may be the first packet */
		m = __mlist_lookup_byaddr(&rtp->mlist, (const struct sockaddr*)&fromaddr);
		if (m != NULL) {
			m->ssrc = ssrc;
			m->type |= RTP_SENDER;
			rtp->mlist.senders++;
			__SET_MEMBER_ADDR(m, (&fromaddr), sizeof(fromaddr));
		} else {
			m = __mlist_add(&rtp->mlist, ssrc, RTP_SENDER);
			rtp->mlist.senders++;
			__SET_MEMBER_ADDR(m, (&fromaddr), sizeof(fromaddr));
		}
	
		if (m) {
			__init_seq(&m->src, seq);
		}
		
	} else {
		if (__update_seq(&m->src, seq) < 0) {
			rtp->err = RTP_ESEQ;
			return -1;
		}
		cflag = 1;
	}

	assert(m != NULL);
	
	rtp->err = 0;
	
	if (cflag) {
		m->recv_bytes += pkt->datalen;
	
		/*
		 * estimating the interarrival jitter
		 */
		transit = __rtp_time() - ntohl(hdr->ts);
		if (m->src.transit != 0) {
			d = transit - m->src.transit;
			m->src.transit = transit;
			if (d < 0) d = -d;
			m->src.jitter += (u_int32) ((1./16.) * 
			                 ((double) d - m->src.jitter));
		} else
			m->src.transit = transit;
		
		if (rtp->auto_rtcp)
			__rtcp_read_hook(rtp, rtp->rtcp);
	}

	__CHECK_RTP_EVENTS(rtp)

	return n;
}

/*
 * Extended RTP read
 * timeout is measured in millisecond, negative value for infinite.
 */
int rtp_readx(RTP *rtp, rtp_pkt_t *pkt, long timeout)
{
	assert(rtp != NULL && pkt != NULL);

	if (rtp == NULL || pkt == NULL) {
		if (rtp)  rtp->err = RTP_EINVAL;
		return -1;
	}

	if (timeout < 0) {
		return rtp_read(rtp, pkt);
	} else {
		int	n;
		fd_set	readset;
		struct timeval	tv;

		FD_ZERO(&readset);
		FD_SET(rtp->sock, &readset);

		tv.tv_sec = timeout / 1000;
		tv.tv_usec = (timeout % 1000) * 1000;
		
		n = select(rtp->sock + 1, &readset, NULL, NULL, &tv);
		
		if (n < 0) {
			/* select() failed */
			rtp->err = RTP_EIO;
			return -1;
		}

		if (n > 0) {
			if (FD_ISSET(rtp->sock, &readset))
				return rtp_read(rtp, pkt);
		}
	}
	
	rtp->err = RTP_ETIMEOUT;

	return -1;
}

int rtp_write(RTP *rtp, const rtp_pkt_t *pkt)
{
	int	n = 0;
	size_t	i;
	rtp_hdr_t	*hdr;
	rtp_member_t	*pmem;

	assert(rtp != NULL && pkt != NULL);

	if (rtp == NULL || pkt == NULL) {
		if (rtp)  rtp->err = RTP_EINVAL;
		return -1;
	}
	
	hdr = (rtp_hdr_t *) pkt->buf;

	for (i = 0; i < rtp->mlist.size; ++i) {
		pmem = &rtp->mlist.members[i];
		if ((pmem->type & RTP_RECEIVER) == 0)	/* not a receiver */
			continue;	/* ignored */

		if (hdr->seq == 0)	/* generate seq */
			hdr->seq = htons(pmem->last_seq + 1);
		pmem->last_seq = ntohs(hdr->seq);

		if (hdr->ssrc == 0)
			hdr->ssrc = htonl(rtp->ssrc);
	
		assert(pkt->data > pkt->buf);
		n = sendto(rtp->sock, pkt->buf,
		         pkt->datalen + (pkt->data - pkt->buf), 0,
		         (struct sockaddr*)&(pmem->addr), sizeof(pmem->addr));

		/*WaitForSingleObject(hSemSendWait, 1);*/ /*add by Alan for k 9403117 */

		if (n < 0) {
			/* rtp->err = RTP_EIO; */
			rtp->err = SOCKET_ERRNO;
			return -1;
		}
		
		pmem->send_bytes += pkt->datalen;
		pmem->send_packets++;
	}
	
	if (rtp->auto_rtcp)
		__rtcp_write_hook(rtp, rtp->rtcp);

	if (n >= 0)
		rtp->last_ts = ((rtp_hdr_t *) &pkt->buf[0])->ts;

	__CHECK_RTP_EVENTS(rtp)

	return n;
}

/* timeout is measured in millisecond, negative value for infinite. */
int rtp_wait(RTP *rtp, int events, long timeout)
{
	struct timeval	tv;
	fd_set	rset, wset;
	fd_set	*prset = NULL, *pwset = NULL;
	int	n, ret = 0;

	assert(rtp);

	if (rtp == NULL)
		return -1;
	
	if (events & RTPEVT_READ) {
		FD_ZERO(&rset);
		FD_SET(rtp->sock, &rset);
		prset = &rset;
	}

	if (events & RTPEVT_WRITE) {
		FD_ZERO(&wset);
		FD_SET(rtp->sock, &wset);
		pwset = &wset;
	}

	if (timeout >= 0) {
		tv.tv_sec = timeout / 1000;
		tv.tv_usec = (timeout % 1000) * 1000;
		n = select(rtp->sock + 1, prset, pwset, NULL, &tv);
	} else {	/* infinite */
		n = select(rtp->sock + 1, prset, pwset, NULL, NULL);
	}

	if (n < 0) {	/* failed */
		rtp->err = RTP_EIO;
		return -1;
	} else if (n == 0) {	/* timeout */
		return 0;
	}

	if (prset && FD_ISSET(rtp->sock, &rset))
		ret |= RTPEVT_READ;

	if (pwset && FD_ISSET(rtp->sock, &wset))
		ret |= RTPEVT_WRITE;

	return ret;
}

const SOCKADDR_STORAGE *rtp_getlocaladdr(RTP *rtp)
{
	assert(rtp);

	return rtp ? &rtp->laddr : 0;
}

const int rtp_getlocalport(RTP *rtp)
{
	assert(rtp);

	return ntohs(((struct sockaddr_in*)&(rtp->laddr))->sin_port);
}

const SOCKADDR_STORAGE  *rtp_getremoteaddr(RTP *rtp, u_int32 ssrc)
{
	rtp_member_t	*m = NULL;
	
	assert(rtp);
	
	if (ssrc == 0) {
		if (rtp->mlist.size > 0)
			m = rtp->mlist.members;
	} else {
		m = __mlist_lookup(&rtp->mlist, ssrc);
	}	
	
	return (m ? &m->addr : NULL);
}

const int rtp_getremoteport(RTP *rtp, u_int32 ssrc)
{
	rtp_member_t	*m = NULL;
	
	assert(rtp);
	
	if (ssrc == 0) {
		if (rtp->mlist.size > 0)
			m = rtp->mlist.members;
	} else {
		m = __mlist_lookup(&rtp->mlist, ssrc);
	}
	
	return (m ? ntohs(((struct sockaddr_in*)&(m->addr))->sin_port) : 0);
}

u_int32 rtp_getssrc(const RTP *rtp)
{
	assert(rtp);

	return rtp ? rtp->ssrc : 0;
}

u_int32 rtp_setssrc(RTP *rtp, u_int32 ssrc)
{
	u_int32	save_ssrc;

	assert(rtp);

	if (rtp == NULL) {
		rtp->err = RTP_EINVAL;
		return 0;
	}
	
	save_ssrc = rtp->ssrc;
	rtp->ssrc = ssrc;

	return save_ssrc;
}

/*
char *rtp_getdataptr(char *rtpbuf, size_t ncsrc)
{
	return (rtpbuf + ( sizeof(rtp_hdr_t) - sizeof(u_int32) +
	                   ncsrc*sizeof(u_int32) ) );
}
*/

int rtp_settos(RTP *rtp, int tos)
{
	int	r;

#ifdef	__UNIX     /* [ for UNIX ] */
	
	r = setsockopt(rtp->sock, IPPROTO_IP, IP_TOS,
	               (const char *) &tos, sizeof(tos));

	if (r < 0)
		return -1;

#else              /* [ for Windows ] */

	r = setsockopt(rtp->sock, IPPROTO_IP, IP_TOS,
	               (const char *) &tos, sizeof(tos));

	if (r == SOCKET_ERROR)
		return -1;
#endif             /* [ #ifdef __UNIX ] */

	return 0;
}

int rtp_pkt_init(rtp_pkt_t *pkt, size_t payload_size)
{
	rtp_hdr_t	*hdr;

	memset(pkt, 0, sizeof(pkt));
	pkt->buflen = payload_size + sizeof(rtp_hdr_t) + 
	              sizeof(u_int32) * 15;
	pkt->buf = malloc(pkt->buflen);
	if (pkt->buf == NULL)
		return -1;

	memset(pkt->buf, 0, pkt->buflen);
	pkt->data = pkt->buf + sizeof(rtp_hdr_t) - sizeof(u_int32);
	hdr = (rtp_hdr_t *) pkt->buf;
	hdr->ver = RTP_VERSION;

	return 0;
}

int rtp_pkt_free(rtp_pkt_t *pkt)
{
	assert(pkt);

	if (pkt == NULL)
		return -1;
	if (pkt->buf) {
		free(pkt->buf);
		pkt->buf = NULL;
	}

	pkt->buflen = 0;
	pkt->data = NULL;

	return 0;
}

int rtp_pkt_getparam(const rtp_pkt_t *pkt, rtp_pkt_param_t *param)
{
	rtp_hdr_t	*hdr;
	
	assert(pkt && param);
	
	hdr = (rtp_hdr_t *) pkt->buf;
	if (param) {
		param->ts	= ntohl(hdr->ts);
		param->m	= (u_int8) hdr->m;
		param->pt	= (u_int8) hdr->pt;
		param->ssrc	= ntohl(hdr->ssrc);
		param->seq	= ntohs(hdr->seq);
		param->cc	= (u_int8) hdr->cc;
		param->csrc_list = &hdr->csrc[0];
	}

	return 0;
}

int rtp_pkt_setparam(rtp_pkt_t *pkt, const rtp_pkt_param_t *param)
{
	rtp_hdr_t	*hdr;

	assert(pkt && param);
	
	hdr = (rtp_hdr_t *) pkt->buf;

	if (param) {
		hdr->ts		= htonl(param->ts);
		hdr->m		= param->m;
		hdr->pt		= param->pt;
		hdr->ssrc	= htonl(param->ssrc);
		hdr->seq	= htons(param->seq);
		hdr->cc		= param->cc;

		if (hdr->cc > 0 && param->csrc_list != NULL)
			memcpy(&hdr->csrc[0], param->csrc_list,
			       sizeof(u_int32) * hdr->cc);

		pkt->data = pkt->buf + sizeof(rtp_hdr_t) +
		            sizeof(u_int32) * (((int) hdr->cc) - 1);
	} else {
		return -1;
	}

	return 0;
}

/* 
 * called while generating rtcp sender/receiver reports.
 * return: 8-bit fixed point number with the binary point at the left edge
 */
u_int8 __cal_packet_lost(rtp_source_t *src, int update)
{
	u_int32	extended_max, expected;
	/* long	lost; */
	u_int32	expected_interval, received_interval, lost_interval;
	u_int8	fraction;

	extended_max = src->cycles + src->max_seq;
	expected = extended_max - src->base_seq + 1;
	/* lost = expected - src->received; */
	
	expected_interval = expected - src->expected_prior;
	received_interval = src->received - src->received_prior;
	
	if (update) {
		src->expected_prior = expected;
		src->received_prior = src->received;
	}
	
	lost_interval = expected_interval - received_interval;
	
	if (expected_interval == 0 || lost_interval <= 0)
		fraction = 0;
	else
		fraction = ((lost_interval << 8) / expected_interval);

	return fraction;
}

int rtp_stat(RTP *rtp, rtp_stat_t *buf, size_t count)
{
	rtp_member_t	*m;
	size_t	i;

	assert(rtp != NULL);
	assert(buf != NULL);
	
	if (rtp == NULL || buf == NULL)
		return -1;
	
	for (i = 0; i < count && i < rtp->mlist.size; i++) {
		m = &rtp->mlist.members[i];
		if (m == NULL)
			continue;
		
		memset(&buf[i], 0, sizeof(rtp_stat_t));

		buf[i].ssrc = m->ssrc;
		
		buf[i].pkt_lost = m->src.cycles + m->src.max_seq -
		                  m->src.base_seq + 1 - m->src.received;
		buf[i].lost_fraction =
			(double) __cal_packet_lost(&m->src, 0) / 256.;

		buf[i].jitter = m->src.jitter;

		buf[i].bytes_recv = m->recv_bytes;
		buf[i].pkt_recv = m->src.received;

		buf[i].bytes_sent = m->send_bytes;
		buf[i].pkt_sent = m->send_packets;
	}
	
	return i;
}

/*
 * RTP data header validity checks: __init_seq() and __update_seq()
 */
static void __init_seq(rtp_source_t *src, u_int16 seq)
{
	assert(src);

	src->base_seq = seq - 1;
	src->max_seq = seq;
	src->bad_seq = RTP_SEQ_MOD + 1;
	src->cycles = 0;
	src->received = 0;
	src->received_prior = 0;
	src->expected_prior = 0;
	src->transit = 0;
}

static int __update_seq(rtp_source_t *src, u_int16 seq)
{
	u_int16	udelta;
	
	assert(src);
	udelta = seq - src->max_seq;
	
	/*
	 * Source is not valid until RTP_MIN_SEQENTIAL packets with sequential
	 * numbres have beed received.
	 */
	if (src->probation) {
		/* packet is in sequence */
		if (seq == src->max_seq + 1) {
			src->probation--;
			src->max_seq = seq;
			if (src->probation == 0) {
				__init_seq(src, seq);
				src->received += RTP_MIN_SEQUENTIAL;
				return 0;	/* permission */
			}
		} else {
			src->probation = RTP_MIN_SEQUENTIAL - 1;
			src->max_seq = seq;
		}
		return -1;	/* invalid */
	} else if (udelta < RTP_MAX_DROPOUT) {
		/* in order, with permissible gap */
		if (seq < src->max_seq) {
			/* Sequence number wrapped - count anther 64K cycle */
			src->cycles += RTP_SEQ_MOD;
		}
		src->max_seq = seq;
	} else if (udelta <= RTP_SEQ_MOD - RTP_MAX_MISORDER) {
		/* the sequence number made a very large jump */
		if (seq == (u_int16) src->bad_seq) {
			/* 
			 * 2 sequential packets -- assume that the other side
			 * restarted without telling us so just re-sync
			 * (i.e., pretend this was the first packet).
			 */
			 __init_seq(src, seq);
		} else {
			src->bad_seq = (seq + 1) & (RTP_SEQ_MOD - 1);
			return -1;	/* bad */
		}
	} else {
		/* duplicate or reordered packet */
		/* ignored */
	}

	src->received++;

	return 0;
}

/*
 * SDES information
 */
const char *rtp_sdes_cname(RTP *rtp, const char *cname)
{
	if (cname) {
		__CHECK_AND_FREE(rtp->info.cname);

		rtp->info.cname = (char *) malloc(strlen(cname) + 1);
		if (rtp->info.cname)
			strcpy(rtp->info.cname, cname);
	}

	return rtp->info.cname ? rtp->info.cname : "";
}

const char *rtp_sdes_name(RTP *rtp, const char *name)
{
	if (name) {
		__CHECK_AND_FREE(rtp->info.name);

		rtp->info.name = (char *) malloc(strlen(name) + 1);
		if (rtp->info.name)
			strcpy(rtp->info.name, name);
	}

	return rtp->info.name ? rtp->info.name : "";
}

const char *rtp_sdes_email(RTP *rtp, const char *email)
{
	if (email) {
		__CHECK_AND_FREE(rtp->info.email);

		rtp->info.email = (char *) malloc(strlen(email) + 1);
		if (rtp->info.email)
			strcpy(rtp->info.email, email);
	}

	return rtp->info.email ? rtp->info.email : "";
}

const char *rtp_sdes_phone(RTP *rtp, const char *phone)
{
	if (phone) {
		__CHECK_AND_FREE(rtp->info.phone);

		rtp->info.phone = (char *) malloc(strlen(phone) + 1);
		if (rtp->info.phone)
			strcpy(rtp->info.phone, phone);
	}

	return rtp->info.phone ? rtp->info.phone : "";
}

const char *rtp_sdes_loc(RTP *rtp, const char *loc)
{
	if (loc) {
		__CHECK_AND_FREE(rtp->info.loc);

		rtp->info.loc = (char *) malloc(strlen(loc) + 1);
		if (rtp->info.loc)
			strcpy(rtp->info.loc, loc);
	}

	return rtp->info.loc ? rtp->info.loc : "";
}

const char *rtp_sdes_tool(RTP *rtp, const char *tool)
{
	if (tool) {
		__CHECK_AND_FREE(rtp->info.tool);

		rtp->info.tool = (char *) malloc(strlen(tool) + 1);
		if (rtp->info.tool)
			strcpy(rtp->info.tool, tool);
	}

	return rtp->info.tool ? rtp->info.tool : "";
}

const char *rtp_sdes_note(RTP *rtp, const char *note)
{
	if (note) {
		__CHECK_AND_FREE(rtp->info.note);

		rtp->info.note = (char *) malloc(strlen(note) + 1);
		if (rtp->info.note)
			strcpy(rtp->info.note, note);
	}

	return rtp->info.note ? rtp->info.note : "";
}

const char *rtp_sdes_priv(RTP *rtp, const char *priv)
{
	if (priv) {
		__CHECK_AND_FREE(rtp->info.priv);

		rtp->info.priv = (char *) malloc(strlen(priv) + 1);
		if (rtp->info.priv)
			strcpy(rtp->info.priv, priv);
	}

	return rtp->info.priv ? rtp->info.priv : "";
}

