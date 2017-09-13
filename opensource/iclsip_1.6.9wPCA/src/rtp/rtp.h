/*=========================================================================*\
  Copyright (c) 2000-2002 Computer & Communications Research Laboratories,
                          Industrial Technology Research Institute
  
  RTP - RTP header file
  
  Author: Jiun-Yao Huang <jyhuang@wizs.org>
  Revision: $Id: rtp.h,v 1.46 2004/12/09 02:29:24 ljchuang Exp $
\*=========================================================================*/
#ifndef	__RTP_H__
#define	__RTP_H__

#include "config.h"
#include "rtp_error.h"


/*--[ types ]---------------------------------------------------------------*/
typedef unsigned char   u_int8;
typedef unsigned short  u_int16;
typedef unsigned int    u_int32;
typedef short           int16;
/*--------------------------------------------------------------------------*/

/*--[ macros ]--------------------------------------------------------------*/
#define RTP_VERSION	2
#define	RTP_SEQ_MOD	(1<<16)
#define	RTP_MAX_SDES	255

#define	RTP_MAX_DROPOUT		3000
#define	RTP_MAX_MISORDER	100
#define	RTP_MIN_SEQUENTIAL	2

/* payload type (RFC 1890) */
#define	PT_PCMU		0	/* 8000 Hz audio, 1 channel */
#define	PT_1016		1	/* 8000 Hz audio, 1 channel */
#define	PT_G721		2	/* 8000 Hz audio, 1 channel */
#define	PT_DVI4_8000	5	/* 8000 Hz audio, 1 channel */
#define	PT_DVI4_16000	6	/* 16000 Hz audio, 1 channel */
#define	PT_DVI4		PT_DVI4_8000
#define	PT_LPC		7	/* 8000 Hz audio, 1 channel */
#define	PT_PCMA		8	/* 8000 Hz audio, 1 channel */
#define	PT_G722		9	/* 8000 Hz audio, 1 channel */
#define	PT_L16_2C	10	/* 44100 Hz audio, 2 channels */
#define	PT_L16_1C	11	/* 44100 Hz audio, 1 channel */
#define	PT_L16		PT_L16_2C
#define	PT_MPA		14	/* 90000 Hz audio */
#define	PT_G728		15	/* 8000 Hz audio, 1 channel */
#define	PT_CELB		25	/* 90000 Hz video */
#define	PT_JPEG		26	/* 90000 Hz video */
#define	PT_NV		28	/* 90000 Hz video */
#define	PT_H261		31	/* 90000 Hz video */
#define	PT_MPV		32	/* 90000 Hz video */
#define	PT_MP2V		33	/* 90000 Hz audio/video */

/* RTP member type */
#define	RTP_SENDER	0x1	/* sender */
#define	RTP_RECEIVER	0x2	/* receiver */

/* RTP Events */
#define	RTPEVT_TIMEOUT	0	/* timeout */
#define	RTPEVT_READ	1	/* readable */
#define	RTPEVT_WRITE	2	/* writable */
#define	RTPEVT_ERROR	4	/* exception */

typedef enum {
	RTCP_SR   = 200,	/* sender report */
	RTCP_RR   = 201,	/* receiver report */
	RTCP_SDES = 202,	/* source description */
	RTCP_BYE  = 203,	/* goodbye */
	RTCP_APP  = 204		/* appliciation specific */
} rtcp_type_t ;

typedef enum {
	RTCP_SDES_END   = 0,	/* end of SDES list */
	RTCP_SDES_CNAME = 1,	/* canonical name */
	RTCP_SDES_NAME  = 2,	/* user name */
	RTCP_SDES_EMAIL = 3,	/* user's e-mail address */
	RTCP_SDES_PHONE = 4,	/* phone number */
	RTCP_SDES_LOC   = 5,	/* geographic user location */
	RTCP_SDES_TOOL  = 6,	/* application or tool name */
	RTCP_SDES_NOTE  = 7,	/* notice/status */
	RTCP_SDES_PRIV  = 8	/* private extension */
} rtcp_sdes_type_t;

#pragma	pack(1)

/**
 * RTP packet header
 */
typedef struct rtp_hdr {
#ifdef	WORDS_BIGENDIAN
	u_int32 ver  :2;        /* protocol version */
	u_int32 p    :1;        /* padding flag */
	u_int32 x    :1;        /* header extension flag */
	u_int32 cc   :4;        /* CSRC count */
	u_int32 m    :1;        /* marker bit */
	u_int32 pt   :7;        /* payload type */
#else	/* little endian */
	u_int32 cc   :4;        /* CSRC count */
	u_int32 x    :1;        /* header extension flag */
	u_int32 p    :1;        /* padding flag */
	u_int32 ver  :2;        /* protocol version */
	u_int32 pt   :7;        /* payload type */
	u_int32 m    :1;        /* marker bit */
#endif	/* WORDS_BIGENDIAN */
	u_int32 seq  :16;       /* sequence number */
	u_int32 ts;             /* timestamp */
	u_int32 ssrc;           /* synchronization source */
	u_int32 csrc[1];        /* optional CSRC list */
} rtp_hdr_t;

/**
 * RTCP common header word
 */
typedef struct rtcp_common {
#ifdef	WORDS_BIGENDIAN
	u_int32 ver   :2;       /* protocol version */
	u_int32 p     :1;       /* padding flag */
	u_int32 count :5;       /* varies by packet type */
#else	/* little endian */
	u_int32 count :5;       /* varies by packet type */
	u_int32 p     :1;       /* padding flag */
	u_int32 ver   :2;       /* protocol version */
#endif	/* WORDS_BIGENDIAN */
	u_int32 pt    :8;       /* RTCP packet type */
	u_int32 len   :16;      /* pkt len in words, w/o this word */
} rtcp_common_t;

#ifdef	WORDS_BIGENDIAN

/* from RFC 1889 */
#define	RTCP_VALID_MASK (0xc000 | 0x2000 | 0xfe)
#define	RTCP_VALID_VALUE ((RTP_VERSION << 14) | RTCP_SR)

#else	/* little endian */

#define	RTCP_VALID_MASK (0xc0 | 0x20 | 0xfe00)
#define	RTCP_VALID_VALUE ((RTP_VERSION << 6) | RTCP_SR << 8)

#endif	/* WORDS_BIGENDIAN */

/**
 * Reception report block
 */
typedef struct rtcp_rr {
	u_int32 ssrc;           /* data source being reported */
	u_int32 fraction :8;    /* fraction lost since last SR/RR */
	u_int32 lost     :24;   /* cumul. no. pkts lost (signed!) */
	u_int32 lseq;           /* extended last seq. no. received */
	u_int32 jitter;         /* interarrival jitter */
	u_int32 lsr;            /* last SR packet from this source */
	u_int32 dlsr;           /* delay since last SR packet */
} rtcp_rr_t;

/**
 * SDES item
 */
typedef struct rtcp_sdes_item {
	u_int8 type;            /* type of item (rtcp_sdes_type_t) */
	u_int8 len;             /* length of item (in octets) */
	char data[1];           /* text, not null-terminated */
} rtcp_sdes_item_t ;

/**
 * One RTCP packet
 */
typedef struct rtcp {
	rtcp_common_t common;   /* common header */
	union r_t {
		/* sender report (SR) */
		struct sr_s {
			u_int32   ssrc;     /* sender generating this report */
			u_int32   ntp_sec;  /* NTP timestamp */
			u_int32   ntp_frac;
			u_int32   rtp_ts;  /* RTP timestamp */
			u_int32   psent;   /* packets sent */
			u_int32   osent;   /* octets sent */
			rtcp_rr_t rr[1];   /* variable-length list */
		} sr;

		/* reception report (RR) */
		struct rr_s {
			u_int32   ssrc;  /* receiver generating this report */
			rtcp_rr_t rr[1]; /* variable-length list */
		} rr;

		/* source description (SDES) */
		struct rtcp_sdes_s {
			u_int32          src;      /* first SSRC/CSRC */
			rtcp_sdes_item_t item[1];  /* list of SDES items */
		} sdes;

		/* BYE */
		struct byt_s {
			u_int32 src[1];   /* list of sources */
			/* can't express trailing text for reason */
		} bye;
	} r;
} rtcp_pkt_t;

#pragma	pack()

typedef struct rtcp_sdes_s rtcp_sdes_t;

struct rtp_s;
struct rtcp_s;

typedef struct rtp_source {
	u_int16 max_seq;        /* highest seq. number seen */
	u_int32 cycles;         /* shifted count of seq. number cycles */
	u_int32 base_seq;       /* base seq number */
	u_int32 bad_seq;        /* last 'bad' seq number + 1 */
	u_int32 probation;      /* sequ. packets till source is valid */
	u_int32 received;       /* packets received */
	u_int32 expected_prior; /* packet expected at last interval */
	u_int32 received_prior; /* packet received at last interval */
	u_int32 transit;        /* relative trans time for prev pkt */
	u_int32 jitter;         /* estimated jitter */
} rtp_source_t;

typedef struct rtp_member {
	u_int32	ssrc;			/* the member's SSRC */
	int	type;			/* type of member: sender, receiver */

	SOCKADDR_STORAGE	addr;	/* RTP address */
	SOCKADDR_STORAGE	caddr;	/* RTCP address */
	
	u_int32	send_bytes;		/* # bytes sent */
	u_int32	send_packets;		/* # packets sent */
	u_int32	recv_bytes;		/* # packets received */

	u_int16	last_seq;		/* last output seq # (big endian) */
	
	u_int32 lsr;			/* last SR packet from this source */

	rtp_source_t	src;		/* for sender */
} rtp_member_t;

typedef struct rtp_member_list {
	rtp_member_t	*members;
	size_t	capacity;	/* capacity that members points to */
	size_t	size;		/* # members */
	size_t	receivers;	/* # RTP receivers */
	size_t	senders;	/* # RTP senders */
} rtp_member_list_t;

typedef struct rtp_events {
	u_int32	rtp_events_ts[2];	/* __EVENT_REPORT and __EVENT_BYE */
#define	ts_report	rtp_events_ts[0]
#define	ts_bye		rtp_events_ts[1]
} rtp_events_t;

typedef struct __rtp_sdes_info_s {
	char	*cname;		/* CNAME(1): canonical end-point identifier */
	char	*name;		/* NAME(2): user name */
	char	*email;		/* EMAIL(3): email */
	char	*phone;		/* PHONE(4): phone number */
	char	*loc;		/* LOC(5): geographic user location */
	char	*tool;		/* TOOL(6): application or tool name */
	char	*note;		/* NOTE(7): notice/status */
	char	*priv;		/* PRIV(8): private extensions */
} __rtp_sdes_info_t;

/** 
 * rtp structure: represents an RTP session
 */
typedef struct rtp_s {
	SOCKET sock;
	SOCKADDR_STORAGE	laddr;	/* local address */

	u_int32	ssrc;			/* my SSRC */
	int	err;			/* error code */

	__rtp_sdes_info_t	info;	/* SDES infomation */

	double	session_bw;		/* RTP session bandwidth */
	
	u_int32	last_ts;		/* last RTP time stamp */

	int	auto_rtcp;		/* nonzero: auto control RTCP */
	struct rtcp_s	*rtcp;		/* pointer to RTCP structure */
	
	rtp_member_list_t	mlist;	/* member list */
	rtp_events_t		events;	/* events */
} RTP;

typedef struct rtcp_s {
	/*SOCKET sock; */
	u_int32 sock;
	SOCKADDR_STORAGE	laddr;	/* local address */
	SOCKADDR_STORAGE	paddr;	/* peer address */
	
	u_int32	time_prev;	/* the last time an RTCP packet was sent */
	
	double	rtcp_bw;	/* RTCP bandwidth (bytes per second) */

	int	we_sent;	/* flag that is true if the application has
	   	        	   sent data since the 2nd previous RTCP
	   	        	   report was transmitted */
	
	double	avg_rtcp_size;	/* the average compound RTCP packet size,
	      	              	   over all RTCP packets sent and received
	      	              	   by this participant */
	
	int	init_flag;	/* if yet sent an RTCP packet */
	
	size_t	pmembers;	/* the estimated number of session members
	      	         	   at the next scheduled tx time (tn) was
	      	         	   last recomputed */
	
	int	err;			/* error code */
} RTCP;

typedef struct {
	u_int32	ssrc;			/* SSRC */
	u_int32	bytes_sent;		/* # bytes sent */
	u_int32	bytes_recv;		/* # bytes received */
	u_int32	pkt_sent;		/* # packets sent */
	u_int32	pkt_recv;		/* # packets received */
	u_int32 jitter;			/* estimated jitter */
	u_int32	pkt_lost;		/* # packets lost */
	double	lost_fraction;		/* packet lost fraction */
} rtp_stat_t;

/**
 * RTP packet buffer
 */
typedef struct {
	char	*data;		/* pointer to payload data */
	size_t	datalen;	/* payload length */
	char	*buf;		/* packet buffer */
	size_t	buflen;		/* buffer length */
} rtp_pkt_t;

/* rtp packet parameters (host order) */
typedef struct {
	u_int32	ts;		/* timestamp */
	u_int8	m;		/* marker */
	u_int8	pt;		/* payload type */
	u_int16	seq;		/* seq. number */
	u_int32	ssrc;		/* SSRC */
	u_int32 *csrc_list;	/* CSRC list */
	u_int8	cc;		/* CSRC count */
} rtp_pkt_param_t;

#ifdef	__cplusplus
extern "C" {
#endif

/* initialize an RTP module */
int rtp_startup(void);
/* finish an RTP module */
int rtp_cleanup(void);

/*--[ RTP ]----------------------------------------------------------------*/
/* open an RTP session (width an associated RTCP session) */
RTP *rtp_open(const char* localaddr, short af, u_int16 port, u_int32 ssrc, const char *cname);

/* close an RTP session */
int rtp_close(RTP *rtp);

SOCKET rtp_getsocket(RTP *rtp);

/* Returns: #bytes received if ok, -1 on error */
int rtp_read(RTP *rtp, rtp_pkt_t *pkt);
int rtp_readx(RTP *rtp, rtp_pkt_t *pkt, long timeout);

/* Returns: #bytes sent if ok, -1 on error */
int rtp_write(RTP *rtp, const rtp_pkt_t *pkt);

/* Returns nonnegative event value if ok, -1 on error */
int rtp_wait(RTP *rtp, int events, long timeout);

/* get/set my SSRC */
u_int32 rtp_getssrc(const RTP *rtp);
u_int32 rtp_setssrc(RTP *rtp, u_int32 ssrc);

size_t rtp_member_count(RTP *rtp);
size_t rtp_member_list(RTP *rtp, u_int32 ssrclist[], size_t count);

/* add/remove an RTP receiver */
int rtp_add_receiver(RTP *rtp, const struct sockaddr *addr, int addrlen);
int rtp_del_receiver(RTP *rtp, const struct sockaddr *addr);

/* get local address and port */
const SOCKADDR_STORAGE *rtp_getlocaladdr(RTP *rtp);
const int rtp_getlocalport(RTP *rtp);

/* get the addresses and ports of the members in an RTP session */
const SOCKADDR_STORAGE *rtp_getremoteaddr(RTP *rtp, u_int32 ssrc);
const int rtp_getremoteport(RTP *rtp, u_int32 ssrc);

/* session bandwidth management */
double rtp_setbw(RTP *rtp, double bw);	/* return previous bandwidth */
double rtp_getbw(const RTP *rtp);

/* RTP packet buffer functions */
int rtp_pkt_init(rtp_pkt_t *pkt, size_t payload_size);
int rtp_pkt_free(rtp_pkt_t *pkt);
int rtp_pkt_getparam(const rtp_pkt_t *pkt, rtp_pkt_param_t *param);
int rtp_pkt_setparam(rtp_pkt_t *pkt, const rtp_pkt_param_t *param);

/* get rtp statistic information */
int rtp_stat(RTP *rtp, rtp_stat_t *buf, size_t count);

/* set the tos */
int rtp_settos(RTP *rtp, int tos);

/*--[ RTCP ]---------------------------------------------------------------*/
/* open an RTCP sesion */
RTCP *rtcp_open(const char* localaddr, short af, u_int16 port);
/* close an RTCP session */
int rtcp_close(RTCP *rtcp);

int rtp_get_port(RTP *rtp);

SOCKET rtcp_getsocket(RTCP *rtcp);

/* RTCP: set peer address */
int rtcp_setpeeraddr(RTCP *rtcp, short af, const char *host, u_int16 port);
int rtcp_setpeeraddrx(RTCP *rtcp, const SOCKADDR_STORAGE *addr);

/* Returns: #bytes received if ok, -1 on error */
int rtcp_read(RTCP *rtcp, char *buf, size_t buflen);
int rtcp_readx(RTCP *rtcp, char *buf, size_t buflen, long timeout);
/* Returns: #bytes sent if ok, -1 on error */
int rtcp_write(RTCP *rtcp, const char *buf, size_t buflen);

/* get RTP error number */
int rtp_errno(const RTP *rtp);
int rtcp_errno(const RTCP *rtcp);

/* set SDES information */
const char *rtp_sdes_cname(RTP *rtp, const char *cname);
const char *rtp_sdes_name(RTP *rtp, const char *name);
const char *rtp_sdes_email(RTP *rtp, const char *email);
const char *rtp_sdes_phone(RTP *rtp, const char *phone);
const char *rtp_sdes_loc(RTP *rtp, const char *loc);
const char *rtp_sdes_tool(RTP *rtp, const char *tool);
const char *rtp_sdes_note(RTP *rtp, const char *note);
const char *rtp_sdes_priv(RTP *rtp, const char *priv);

extern u_int16	rtp_port_base;
extern u_int16	rtp_port_max;

#ifdef	__cplusplus
}
#endif

#endif /* #ifndef __RTP_H__ */
