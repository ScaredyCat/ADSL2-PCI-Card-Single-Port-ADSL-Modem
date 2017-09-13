/*
 * lcp.h - Link Control Protocol definitions.
 *
 * Copyright (c) 1989 Carnegie Mellon University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by Carnegie Mellon University.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Id: lcp.h,v 1.2 2000/07/25 20:23:51 kai Exp $
 */

/*
 * Options.
 */
#define CI_MRU		1	/* Maximum Receive Unit */
#define CI_ASYNCMAP	2	/* Async Control Character Map */
#define CI_AUTHTYPE	3	/* Authentication Type */
#define CI_QUALITY	4	/* Quality Protocol */
#define CI_MAGICNUMBER	5	/* Magic Number */
#define CI_PCOMPRESSION	7	/* Protocol Field Compression */
#define CI_ACCOMPRESSION 8	/* Address/Control Field Compression */
#define CI_CALLBACK      13     /* callback */

#define CI_MPMRRU	17
#define CI_MPSHORTSEQ	18
#define CI_MPDISCRIMINATOR 19

/*
 * LCP-specific packet types.
 */
#define PROTREJ		8	/* Protocol Reject */
#define ECHOREQ		9	/* Echo Request */
#define ECHOREP		10	/* Echo Reply */
#define DISCREQ		11	/* Discard Request */


/*
 * Type constants for the CI_CALLBACK field. Types 0..4 are RFC 1570
 * callback codes that tell the peer how to interpret the callback
 * message. Type 6 requests callback negotiation by CBCP.
 */
#define CB_AUTH	        0 /* cb msg is not used */
#define CB_DIALSTRING	1 /* cb msg is a system specific dial string */
#define CB_LOCATIONID	2 /* cb msg is a location identifier */
#define CB_PHONENO	3 /* cb msg is a E.164 (i.e. phone) number */
#define CB_NAME         4 /* cb msg is a name */
#define CB_CBCP         6 /* callback will be negotiated via cbcp */

struct callback_opts {
  int neg_cbcp : 1;       /* Enable CBCP callback negotiation */
  int neg_rfc  : 1;       /* Enable RFC 1570 callback negotiation */
  int rfc_preferred : 1;  /* Try RFC 1570 callback negotiation first */
  int type;               /* callback type as defined above */
  unsigned char *message; /* callback message (phone number in most cases) */
  int mlen;               /* length of callback message */
  int delay;              /* callback delay for cbcp */
};

/*
 * The state of options is described by an lcp_options structure.
 */
typedef struct lcp_options {
    int passive : 1;		/* Don't die if we don't get a response */
    int silent : 1;		/* Wait for the other end to start first */
    int restart : 1;		/* Restart vs. exit after close */
    int neg_mru : 1;		/* Negotiate the MRU? */
    int neg_asyncmap : 1;	/* Negotiate the async map? */
    int neg_upap : 1;		/* Ask for UPAP authentication? */
    int neg_chap : 1;		/* Ask for CHAP authentication? */
    int neg_magicnumber : 1;	/* Ask for magic number? */
    int neg_pcompression : 1;	/* HDLC Protocol Field Compression? */
    int neg_accompression : 1;	/* HDLC Address/Control Field Compression? */
    int neg_lqr : 1;		/* Negotiate use of Link Quality Reports */

    int neg_mpshortseq : 1;	/* MP 12 bit instead of 24 bit sequence numbers */
    int neg_mpdiscr : 1;        /* MP protocol ? */
    int neg_mpmrru : 1;
    int neg_mp : 1;
    int neg_callback : 1;       /* Negotiate callback */
    u_char mp_class;		/* MP discri. class */
    u_char mp_addr[20];		/* MP discri. addr */
    u_char mp_alen;
    u_short mp_mrru;		/* MP mrru */
    u_char cb_type;
    u_char cb_num[20];
    u_char cb_numlen;

    u_short mru;		/* Value of MRU */
    u_char chap_mdtype;		/* which MD type (hashing algorithm) */
    u_int32_t asyncmap;		/* Value of async map */
    u_int32_t magicnumber;
    int numloops;		/* Number of loops during magic number neg. */
    u_int32_t lqr_period;	/* Reporting period for LQR 1/100ths second */
    struct callback_opts cbopt; /* Callback options */
} lcp_options;

extern fsm lcp_fsm[];
extern lcp_options lcp_wantoptions[];
extern lcp_options lcp_gotoptions[];
extern lcp_options lcp_allowoptions[];
extern lcp_options lcp_hisoptions[];
extern u_int32_t xmit_accm[][8];

extern struct protent lcp_protent;

#define DEFMRU	1500		/* Try for this */
#define MINMRU	128		/* No MRUs below this */
#define MAXMRU	16384		/* Normally limit MRU to this */

void lcp_open __P((int));
void lcp_close __P((int,char *));
void lcp_lowerup __P((int));
void lcp_lowerdown __P((int));
void lcp_sprotrej __P((int, u_char *, int));

/* Default number of times we receive our magic number from the peer
   before deciding the link is looped-back. */
#define DEFLOOPBACKFAIL	5


