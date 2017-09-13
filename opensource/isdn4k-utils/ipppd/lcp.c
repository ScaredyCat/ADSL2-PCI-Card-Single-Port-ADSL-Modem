/*
 * lcp.c - PPP Link Control Protocol.
 *
 * Fairly patched version for isdn4linux (eg the MLP part)
 * copyright (c) 1995,1996,1997 of all patches by Michael Hipp
 * still no warranties (see disclaimer)
 *
 * Copyright (c) 1989 Carnegie Mellon University.
 * All rights reserved.
 *
 * 2000-07-25 Callback improvements by richard.kunze@web.de 
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
 */

char lcp_rcsid[] = "$Id: lcp.c,v 1.12 2000/11/12 16:06:42 kai Exp $";

/*
 * TODO:
 */

#include <stdio.h>
#include <string.h>
#include <syslog.h>
/* #include <assert.h>*/
#define assert(x) do {} while(0)
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>

#include "ipppd.h"
#include "fsm.h"
#include "lcp.h"
#include "magic.h"
#include "chap.h"
#include "upap.h"
#include "ipcp.h"
#include "ipxcp.h"

#ifdef __linux__		/* Needs ppp ioctls */
#if defined __GLIBC__ && __GLIBC__ >= 2
# include </usr/include/net/if.h>
# include </usr/include/net/if_ppp.h>
#else
# include </usr/include/net/if.h>
# include <linux/if_ppp.h>
#endif
#endif

u_char our_discr_class = 4; /* 0x2; */
u_char our_discr_addr[] = { 0,0,0,0,0,0,0,0 }; /* experimental */

/* global vars */
fsm lcp_fsm[NUM_PPP];			/* LCP fsm structure (global)*/
lcp_options lcp_wantoptions[NUM_PPP];	/* Options that we want to request */
lcp_options lcp_gotoptions[NUM_PPP];	/* Options that peer ack'd */
lcp_options lcp_allowoptions[NUM_PPP];	/* Options we allow peer to request */
lcp_options lcp_hisoptions[NUM_PPP];	/* Options that we ack'd */

u_int32_t xmit_accm[NUM_PPP][8];		/* extended transmit ACCM */
static u_int32_t lcp_echos_pending = 0;	/* Number of outstanding echo msgs */
static u_int32_t lcp_echo_number   = 0;	/* ID number of next echo frame */
static u_int32_t lcp_echo_timer_running = 0;  /* TRUE if a timer is running */

static u_char nak_buffer[PPP_MRU];	/* where we construct a nak packet */

#ifdef __linux__
u_int32_t idle_timer_running = 0;
extern int idle_time_limit;
#ifdef RADIUS
u_int32_t session_timer_running = 0;
extern int session_time_limit;
#endif
#endif

/*
 * Callbacks for fsm code.  (CI = Configuration Information)
 */
static void lcp_resetci __P((fsm *));	/* Reset our CI */
static int  lcp_cilen __P((fsm *));		/* Return length of our CI */
static void lcp_addci __P((fsm *, u_char *, int *)); /* Add our CI to pkt */
static int  lcp_ackci __P((fsm *, u_char *, int)); /* Peer ack'd our CI */
static int  lcp_nakci __P((fsm *, u_char *, int)); /* Peer nak'd our CI */
static int  lcp_rejci __P((fsm *, u_char *, int)); /* Peer rej'd our CI */
static int  lcp_reqci __P((fsm *, u_char *, int *, int)); /* Rcv peer CI */
static void lcp_up __P((fsm *));		/* We're UP */
static void lcp_down __P((fsm *));		/* We're DOWN */
static void lcp_starting __P((fsm *));	/* We need lower layer up */
static void lcp_finished __P((fsm *));	/* We need lower layer down */
static int  lcp_extcode __P((fsm *, int, int, u_char *, int));
static void lcp_rprotrej __P((fsm *, u_char *, int));

/*
 * routines to send LCP echos to peer
 */

static void lcp_echo_lowerup __P((int));
static void lcp_echo_lowerdown __P((int));
static void LcpEchoTimeout __P((caddr_t));
static void lcp_received_echo_reply __P((fsm *, int, u_char *, int));
static void LcpSendEchoRequest __P((fsm *));
static void LcpLinkFailure __P((fsm *));

static fsm_callbacks lcp_callbacks = {	/* LCP callback routines */
    lcp_resetci,		/* Reset our Configuration Information */
    lcp_cilen,			/* Length of our Configuration Information */
    lcp_addci,			/* Add our Configuration Information */
    lcp_ackci,			/* ACK our Configuration Information */
    lcp_nakci,			/* NAK our Configuration Information */
    lcp_rejci,			/* Reject our Configuration Information */
    lcp_reqci,			/* Request peer's Configuration Information */
    lcp_up,			/* Called when fsm reaches OPENED state */
    lcp_down,			/* Called when fsm leaves OPENED state */
    lcp_starting,		/* Called when we want the lower layer up */
    lcp_finished,		/* Called when we want the lower layer down */
    NULL,			/* Called when Protocol-Reject received */
    NULL,			/* Retransmission is necessary */
    lcp_extcode,		/* Called to handle LCP-specific codes */
    "LCP"			/* String name of protocol */
};

/*
 * Protocol entry points.
 * Some of these are called directly.
 */

static void lcp_init __P((int));
static void lcp_input __P((int, u_char *, int));
static void lcp_protrej __P((int));
static int  lcp_printpkt __P((u_char *, int,
                  void (*) __P((void *, char *, ...)), void *));

struct protent lcp_protent = {
    PPP_LCP,
    lcp_init,
    lcp_input,
    lcp_protrej,
    lcp_lowerup,
    lcp_lowerdown,
    lcp_open,
    lcp_close,
    lcp_printpkt,
    NULL,
    1,
    "LCP",
    NULL,
    NULL,
    NULL
};

int lcp_loopbackfail = DEFLOOPBACKFAIL;

/*
 * Length of each type of configuration option (in octets)
 */
#define CILEN_VOID	2
#define CILEN_CHAR	3
#define CILEN_SHORT	4	/* CILEN_VOID + sizeof(short) */
#define CILEN_CHAP	5	/* CILEN_VOID + sizeof(short) + 1 */
#define CILEN_LONG	6	/* CILEN_VOID + sizeof(long) */
#define CILEN_LQR	8	/* CILEN_VOID + sizeof(short) + sizeof(long) */
#define CILEN_CB	3

#define CODENAME(x)	((x) == CONFACK ? "ACK" : \
			 (x) == CONFNAK ? "NAK" : "REJ")


/*
 * lcp_init - Initialize LCP.
 */
static void lcp_init(int unit)
{
  static int first_call=1;

    fsm *f = &lcp_fsm[unit];
    lcp_options *wo = &lcp_wantoptions[unit];
    lcp_options *ao = &lcp_allowoptions[unit];

    if(first_call)
    {
      *(u_int32_t *) (&our_discr_addr[0]) = magic();
      *(u_int32_t *) (&our_discr_addr[4]) = magic();
     
      first_call = 0;
    }

	memset(f,0,sizeof(fsm));

    f->unit = -1;
    f->protocol = PPP_LCP;
    f->callbacks = &lcp_callbacks;
    f->inuse = 0;

    fsm_init(f);

    wo->passive = 0;
    wo->silent = 0;
    wo->restart = 0;			/* Set to 1 in kernels or multi-line
					   implementations */
    wo->neg_mru = 1;
    wo->mru = DEFMRU;
    wo->neg_asyncmap = 0;
    wo->asyncmap = 0;
    wo->neg_chap = 0;			/* Set to 1 on server */
    wo->neg_upap = 0;			/* Set to 1 on server */
    wo->chap_mdtype = CHAP_DIGEST_MD5;
    wo->neg_magicnumber = 1;
    wo->neg_pcompression = 1;
    wo->neg_accompression = 1;
    wo->neg_lqr = 0;			/* no LQR implementation yet */
    wo->neg_callback = 0;
    wo->cbopt.neg_cbcp = 1;
    wo->cbopt.neg_rfc  = 1;
    wo->cbopt.rfc_preferred  = 0;
    wo->cbopt.type = CB_CBCP;
    wo->cbopt.delay = 5;             /* Default to 5 seconds callback delay */
    wo->neg_mp = 0;                     /* we set this later, if nec. */
    wo->neg_mpdiscr = 0;
    wo->neg_mpmrru = 0;
    wo->neg_mpshortseq = 0;	/* don't allow short seq. numbers */
    wo->mp_mrru = DEFMRU;
    wo->mp_class = our_discr_class;
    wo->mp_alen = sizeof(our_discr_addr);
    wo->numloops = 0;
    memcpy(wo->mp_addr,our_discr_addr,wo->mp_alen);

    ao->neg_mru = 1;
    ao->mru = MAXMRU;
    ao->neg_asyncmap = 1;
    ao->asyncmap = 0;
    ao->neg_chap = 1;
    ao->chap_mdtype = CHAP_DIGEST_MD5;
    ao->neg_upap = 1;
    ao->neg_magicnumber = 1;
    ao->neg_pcompression = 1;
    ao->neg_accompression = 1;
    ao->neg_lqr = 0;			/* no LQR implementation yet */
    ao->neg_mpmrru = 0;
    ao->neg_mpdiscr = 1;
    ao->neg_mpshortseq = 0;
    ao->neg_callback = 0;               /* Always reject callback requests */

    memset(xmit_accm[unit], 0, sizeof(xmit_accm[0]));
    xmit_accm[unit][3] = 0x60000000;
}

/*
 *
 */

int lcp_getunit(int linkunit)
{
  int i;
  for(i=0;i<NUM_PPP;i++)
    if(!lcp_fsm[i].inuse)
    {
      lcp_fsm[i].inuse = 1;
      lcp_fsm[i].state = INITIAL;
      lcp_fsm[i].flags = 0;
      lcp_fsm[i].id = 0;
      lcp_fsm[i].unit = linkunit;
      return i;
    }
  return -1;
}

void lcp_freeunit(int unit)
{
  lcp_fsm[unit].inuse = 0;
  lcp_fsm[unit].unit = -1;
}

/*
 * lcp_open - LCP is allowed to come up.
 */
void lcp_open(int unit)
{
    fsm *f = &lcp_fsm[unit];
    lcp_options *wo = &lcp_wantoptions[unit];

    f->flags = 0;
    if (wo->passive)
	f->flags |= OPT_PASSIVE;
    if (wo->silent)
	f->flags |= OPT_SILENT;
    fsm_open(f);
}


/*
 * lcp_close - Take LCP down.
 */
void lcp_close(int unit,char *reason)
{
    fsm *f = &lcp_fsm[unit];

	if(unit < 0) {
		syslog(LOG_ERR, "lcp_closed called with unit < 0");
		return;
	}

    if (f->state == STOPPED && f->flags & (OPT_PASSIVE|OPT_SILENT)) {
	/*
	 * This action is not strictly according to the FSM in RFC1548,
	 * but it does mean that the program terminates if you do a
	 * lcp_close(0) in passive/silent mode when a connection hasn't
	 * been established.
	 */
	f->state = CLOSED;
	lcp_finished(f);

    } else
	fsm_close(&lcp_fsm[unit],reason);
}

#ifdef __linux__
static void IdleTimeCheck __P((caddr_t));

/*
 * Timer expired for the LCP echo requests from this process.
 */

void RestartIdleTimer(fsm *f)
{
  struct link_struct *tlns = &lns[f->unit];
    u_long             delta;
    struct ppp_idle    ddinfo;
/*
 * Read the time since the last packet was received.
 */
    if (ioctl (tlns->fd, PPPIOCGIDLE, &ddinfo) < 0) {
        syslog (LOG_ERR, "ioctl(PPPIOCGIDLE): %m");
		return;
    }
/*
 * Compute the time since the last packet was received. If the timer
 *  has expired then disconnect the line.
 */
    delta = idle_time_limit - (u_long) ddinfo.recv_idle;
    if (((int) delta <= 0L) && (f->state == OPENED)) {
        syslog (LOG_NOTICE, "No IP frames received within idle time limit");
	lcp_close(tlns->lcp_unit,"No IP frames received within idle time limit");		/* Reset connection */
	lns[f->unit].phase = PHASE_TERMINATE;	/* Mark it down */
    } else {
        if ((int) delta <= 0L)
	    delta = (u_long) idle_time_limit;
        assert (idle_timer_running==0);
        TIMEOUT (IdleTimeCheck, (caddr_t) f, delta);
        idle_timer_running = 1;
    }
}

/*
 * IdleTimeCheck - Timer expired on the IDLE detection for IP frames
 */

static void
IdleTimeCheck (arg)
    caddr_t arg;
{
    if (idle_timer_running != 0) {
        idle_timer_running = 0;
        RestartIdleTimer ((fsm *) arg);
    }
}
#ifdef RADIUS
/***************************************************************************
 *
 * Name: StopSession 
 *
 * Purpose: Stops the session after session_time_limit seconds
 *
 ***************************************************************************/
static void 
StopSession (f)

	fsm *f;

{
	lcp_close(f->unit,"session max time expired"); 	/* Reset connection */
	lns[f->unit].phase = PHASE_TERMINATE;		/* Mark it down     */
}

/***************************************************************************
 *
 * Name: RestartSessionTimer
 *
 * Purpose: Restarts session timer
 *
 ***************************************************************************/
      
void
RestartSessionTimer (f)
	fsm *f;
{
	if (session_timer_running == 1) 
	{
		UNTIMEOUT (StopSession, (caddr_t) f);
	}
	
        TIMEOUT (StopSession, (caddr_t) f, session_time_limit);
        
        session_timer_running = 1;
}
#endif

#endif

/*
 * lcp_lowerup - The lower layer is up.
 */
void lcp_lowerup(int unit)
{
  fsm *f = &lcp_fsm[unit];
  struct link_struct *tlns = &lns[f->unit];

  ppp_set_xaccm(f->unit, xmit_accm[unit]);
  ppp_send_config(f->unit, PPP_MRU, 0xffffffff, 0, 0);
  ppp_recv_config(f->unit, PPP_MRU, 0x00000000, 0, 0);

  tlns->peer_mru = PPP_MRU;
  lcp_allowoptions[tlns->lcp_unit].asyncmap = xmit_accm[unit][0];
  fsm_lowerup(&lcp_fsm[unit]);
}


/*
 * lcp_lowerdown - The lower layer is down.
 */
void lcp_lowerdown(int unit)
{
	if(unit < 0) {
		syslog(LOG_ERR, "lcp_lowerdown called with unit < 0");
	}
    fsm_lowerdown(&lcp_fsm[unit]);
}


/*
 * lcp_input - Input LCP packet.
 */
void lcp_input(int linkunit,u_char *p,int len)
{
    int oldstate;
    int unit = lns[linkunit].lcp_unit;
    fsm *f = &lcp_fsm[unit];
    lcp_options *go = &lcp_gotoptions[unit];

    oldstate = f->state;
    fsm_input(f, p, len);
    if (oldstate == REQSENT && f->state == ACKSENT) {
	/*
	 * The peer will probably send us an ack soon and then
	 * immediately start sending packets with the negotiated
	 * options.  So as to be ready when that happens, we set
	 * our receive side to accept packets as negotiated now.
	 */
	ppp_recv_config(f->unit, PPP_MRU,
			go->neg_asyncmap? go->asyncmap: 0x00000000,
			go->neg_pcompression, go->neg_accompression);
    }
}


/*
 * lcp_extcode - Handle a LCP-specific code.
 */
static int lcp_extcode(fsm *f,int code,int id,u_char *inp,int len)
{
    u_char *magp;

    switch( code ){
    case PROTREJ:
	lcp_rprotrej(f, inp, len);
	break;
    
    case ECHOREQ:
	if (f->state != OPENED)
	    break;
	LCPDEBUG((LOG_INFO, "lcp: Echo-Request, Rcvd id %d", id));
	magp = inp;
	PUTLONG(lcp_gotoptions[lns[f->unit].lcp_unit].magicnumber, magp);
	fsm_sdata(f, ECHOREP, id, inp, len);
	break;
    
    case ECHOREP:
	lcp_received_echo_reply(f, id, inp, len);
	break;

    case DISCREQ:
	break;

    default:
	return 0;
    }
    return 1;
}

    
/*
 * lcp_rprotrej - Receive an Protocol-Reject.
 *
 * Figure out which protocol is rejected and inform it.
 */
static void lcp_rprotrej(fsm *f,u_char *inp,int len)
{
    u_short prot;

    LCPDEBUG((LOG_INFO, "lcp_rprotrej."));

    if (len < sizeof (u_short)) {
	LCPDEBUG((LOG_INFO,
		  "lcp_rprotrej: Rcvd short Protocol-Reject packet!"));
	return;
    }

    GETSHORT(prot, inp);

    LCPDEBUG((LOG_INFO,
	      "lcp_rprotrej: Rcvd Protocol-Reject packet for %x!",
	      prot));

    /*
     * Protocol-Reject packets received in any state other than the LCP
     * OPENED state SHOULD be silently discarded.
     */
    if( f->state != OPENED ){
	LCPDEBUG((LOG_INFO, "Protocol-Reject discarded: LCP in state %d",
		  f->state));
	return;
    }

/* here exception: give link unit .. */
    DEMUXPROTREJ(f->unit, prot);	/* Inform protocol */
}


/*
 * lcp_protrej - A Protocol-Reject was received.
 */
/*ARGSUSED*/
void lcp_protrej(int linkunit)
{
  int unit = lns[linkunit].lcp_unit;
    /*
     * Can't reject LCP!
     */
    LCPDEBUG((LOG_WARNING,
	      "lcp_protrej: Received Protocol-Reject for LCP!"));
    fsm_protreject(&lcp_fsm[unit]);
}


/*
 * lcp_sprotrej - Send a Protocol-Reject for some protocol.
 */
void lcp_sprotrej(int linkunit,u_char *p,int len)
{
  int unit = lns[linkunit].lcp_unit;
  fsm *f = &lcp_fsm[unit];

    /*
     * Send back the protocol and the information field of the
     * rejected packet.  We only get here if LCP is in the OPENED state.
     */
    p += 2;
    len -= 2;

    fsm_sdata(f, PROTREJ, ++(f->id), p, len);
}


/*
 * lcp_resetci - Reset our CI.
 */
static void lcp_resetci(fsm *f)
{
  struct link_struct *tlns = &lns[f->unit];

    lcp_wantoptions[tlns->lcp_unit].magicnumber = magic();
#if 0
    lcp_wantoptions[tlns->lcp_unit].numloops = 0;
#endif
    lcp_gotoptions[tlns->lcp_unit] = lcp_wantoptions[tlns->lcp_unit];
    tlns->peer_mru = PPP_MRU;
}


/*
 * lcp_cilen - Return length of our CI.
 */
static int lcp_cilen(fsm *f)
{
    lcp_options *go = &lcp_gotoptions[ lns[f->unit].lcp_unit ];

#define LENCIVOID(neg)	(neg ? CILEN_VOID : 0)
#define LENCICHAP(neg)	(neg ? CILEN_CHAP : 0)
#define LENCISHORT(neg)	(neg ? CILEN_SHORT : 0)
#define LENCILONG(neg)	(neg ? CILEN_LONG : 0)
#define LENCILQR(neg)	(neg ? CILEN_LQR: 0)
#define LENCIMPDISCRI(neg,alen) (neg ? 3+alen : 0)
#define LENCIMPMRRU(neg) (neg ? CILEN_SHORT : 0)
#define LENCIMPSHORTSEQ(neg) (neg ? CILEN_VOID : 0)
#define LENCICB(neg,type,mlen) (neg ? CILEN_CB + (type!=CB_CBCP&&type!=CB_AUTH?mlen:0) : 0)

    /*
     * NB: we only ask for one of CHAP and UPAP, even if we will
     * accept either.
     */
    return (LENCISHORT(go->neg_mru) +
	    LENCILONG(go->neg_asyncmap) +
	    LENCICHAP(go->neg_chap) +
	    LENCISHORT(!go->neg_chap && go->neg_upap) +
	    LENCILQR(go->neg_lqr) +
	    LENCILONG(go->neg_magicnumber) +
	    LENCIVOID(go->neg_pcompression) +
	    LENCIVOID(go->neg_accompression) +
	    LENCICB(go->neg_callback,go->cbopt.type,go->cbopt.mlen) +
	    LENCIMPDISCRI(go->neg_mpdiscr,go->mp_alen) +
	    LENCIMPMRRU(go->neg_mpmrru) +
	    LENCIMPSHORTSEQ(go->neg_mpshortseq) );
}


/*
 * lcp_addci - Add our desired CIs to a packet.
 */
static void
lcp_addci(fsm *f,u_char *ucp,int *lenp)
{
    lcp_options *go = &lcp_gotoptions[ lns[f->unit].lcp_unit ];
    u_char *start_ucp = ucp;

#define ADDCIVOID(opt, neg) \
    if (neg) { \
	PUTCHAR(opt, ucp); \
	PUTCHAR(CILEN_VOID, ucp); \
    }
#define ADDCISHORT(opt, neg, val) \
    if (neg) { \
	PUTCHAR(opt, ucp); \
	PUTCHAR(CILEN_SHORT, ucp); \
	PUTSHORT(val, ucp); \
    }
#define ADDCICHAP(opt, neg, val, digest) \
    if (neg) { \
	PUTCHAR(opt, ucp); \
	PUTCHAR(CILEN_CHAP, ucp); \
	PUTSHORT(val, ucp); \
	PUTCHAR(digest, ucp); \
    }
#define ADDCILONG(opt, neg, val) \
    if (neg) { \
	PUTCHAR(opt, ucp); \
	PUTCHAR(CILEN_LONG, ucp); \
	PUTLONG(val, ucp); \
    }
#define ADDCILQR(opt, neg, val) \
    if (neg) { \
	PUTCHAR(opt, ucp); \
	PUTCHAR(CILEN_LQR, ucp); \
	PUTSHORT(PPP_LQR, ucp); \
	PUTLONG(val, ucp); \
    }
#define ADDCICHAR(opt, neg, val) \
    if (neg) { \
       PUTCHAR(opt, ucp); \
       PUTCHAR(CILEN_CHAR, ucp); \
       PUTCHAR(val, ucp); \
    }
#define ADDCIMPDISCR(opt,neg,clas,aval,alen) \
    if(neg) { \
       int i; \
       PUTCHAR(opt,ucp); \
       PUTCHAR((alen+3),ucp); \
       PUTCHAR(clas,ucp); \
       for(i=0;i<alen;i++) \
         PUTCHAR(aval[i],ucp); \
    } 
#define ADDCICB(opt,neg,type,aval,alen) \
    if(neg) { \
       int i; \
       int checklen = ((type)==CB_CBCP||(type==CB_AUTH)?0:(alen)); \
       PUTCHAR(opt,ucp); \
       PUTCHAR((checklen+3),ucp); \
       PUTCHAR(type,ucp); \
       for(i=0;i<checklen;i++) \
         PUTCHAR(aval[i],ucp); \
    }


    ADDCISHORT(CI_MRU, go->neg_mru, go->mru);
    ADDCILONG(CI_ASYNCMAP, go->neg_asyncmap, go->asyncmap);
    ADDCICHAP(CI_AUTHTYPE, go->neg_chap, PPP_CHAP, go->chap_mdtype);
    ADDCISHORT(CI_AUTHTYPE, !go->neg_chap && go->neg_upap, PPP_PAP);
    ADDCILQR(CI_QUALITY, go->neg_lqr, go->lqr_period);
    ADDCILONG(CI_MAGICNUMBER, go->neg_magicnumber, go->magicnumber);
    ADDCIVOID(CI_PCOMPRESSION, go->neg_pcompression);
    ADDCIVOID(CI_ACCOMPRESSION, go->neg_accompression);
    ADDCICB(CI_CALLBACK, go->neg_callback, go->cbopt.type,
	    go->cbopt.message, go->cbopt.mlen );
    ADDCISHORT(CI_MPMRRU,go->neg_mpmrru,go->mp_mrru);
    ADDCIVOID(CI_MPSHORTSEQ,go->neg_mpshortseq);
    ADDCIMPDISCR(CI_MPDISCRIMINATOR,go->neg_mpdiscr,go->mp_class,go->mp_addr,go->mp_alen );

    if (ucp - start_ucp != *lenp) {
	/* this should never happen, because peer_mtu should be 1500 */
	syslog(LOG_ERR, "Bug in lcp_addci: wrong length");
    }
}


/*
 * lcp_ackci - Ack our CIs.
 * This should not modify any state if the Ack is bad.
 *
 * Returns:
 *	0 - Ack was bad.
 *	1 - Ack was good.
 */
static int lcp_ackci(fsm *f,u_char *p,int len)
{
    lcp_options *go = &lcp_gotoptions[ lns[f->unit].lcp_unit ];
    u_char cilen, citype, cichar;
    u_short cishort;
    u_int32_t cilong;

    /*
     * CIs must be in exactly the same order that we sent.
     * Check packet length and CI length at each step.
     * If we find any deviations, then this packet is bad.
     */
#define ACKCIVOID(opt, neg) \
    if (neg) { \
	if ((len -= CILEN_VOID) < 0) \
	    goto bad; \
	GETCHAR(citype, p); \
	GETCHAR(cilen, p); \
	if (cilen != CILEN_VOID || \
	    citype != opt) \
	    goto bad; \
    }
#define ACKCISHORT(opt, neg, val) \
    if (neg) { \
	if ((len -= CILEN_SHORT) < 0) \
	    goto bad; \
	GETCHAR(citype, p); \
	GETCHAR(cilen, p); \
	if (cilen != CILEN_SHORT || \
	    citype != opt) \
	    goto bad; \
	GETSHORT(cishort, p); \
	if (cishort != val) \
	    goto bad; \
    }
#define ACKCICHAR(opt, neg, val) \
    if (neg) { \
       if ((len -= CILEN_CHAR) < 0) \
           goto bad; \
       GETCHAR(citype, p); \
       GETCHAR(cilen, p); \
       if (cilen != CILEN_CHAR || \
           citype != opt) \
           goto bad; \
       GETCHAR(cichar, p); \
       if (cichar != val) \
           goto bad; \
    }
#define ACKCICHAP(opt, neg, val, digest) \
    if (neg) { \
	if ((len -= CILEN_CHAP) < 0) \
	    goto bad; \
	GETCHAR(citype, p); \
	GETCHAR(cilen, p); \
	if (cilen != CILEN_CHAP || \
	    citype != opt) \
	    goto bad; \
	GETSHORT(cishort, p); \
	if (cishort != val) \
	    goto bad; \
	GETCHAR(cichar, p); \
	if (cichar != digest) \
	  goto bad; \
    }
#define ACKCILONG(opt, neg, val) \
    if (neg) { \
	if ((len -= CILEN_LONG) < 0) \
	    goto bad; \
	GETCHAR(citype, p); \
	GETCHAR(cilen, p); \
	if (cilen != CILEN_LONG || \
	    citype != opt) \
	    goto bad; \
	GETLONG(cilong, p); \
	if (cilong != val) \
	    goto bad; \
    }
#define ACKCILQR(opt, neg, val) \
    if (neg) { \
	if ((len -= CILEN_LQR) < 0) \
	    goto bad; \
	GETCHAR(citype, p); \
	GETCHAR(cilen, p); \
	if (cilen != CILEN_LQR || \
	    citype != opt) \
	    goto bad; \
	GETSHORT(cishort, p); \
	if (cishort != PPP_LQR) \
	    goto bad; \
	GETLONG(cilong, p); \
	if (cilong != val) \
	  goto bad; \
    }
#define ACKCIMPDISCRI(opt,neg,clas,aval,alen) \
   if(neg) { \
     int i; \
     if((len -= 3+alen) < 0) \
       goto bad; \
     GETCHAR(citype,p); \
     GETCHAR(cilen,p); \
     GETCHAR(cichar,p); \
     if(cilen != alen+3 || citype != opt || cichar != clas) \
       goto bad; \
     for(i=0;i<alen;i++) { \
       GETCHAR(cichar,p); \
       if(cichar != aval[i]) \
         goto bad; \
     } \
   }
#define ACKCICB(opt,neg,type,aval,alen) \
   if(neg) { \
     int i; \
     int checklen; \
     checklen = (type!=CB_CBCP&&opt!=CB_AUTH?alen:0); \
     LCPDEBUG((LOG_DEBUG, "opt: %d, type: %d, checklen: %d, alen: %d, aval: %s", \
	       opt, type, checklen, alen, aval)); \
     if((len -= 3+checklen) < 0) \
       goto bad; \
     GETCHAR(citype,p); \
     GETCHAR(cilen,p); \
     GETCHAR(cichar,p); \
     if(cilen != checklen+3 || citype != opt || cichar != type) \
       goto bad; \
     for(i=0;i<checklen;i++) { \
       GETCHAR(cichar,p); \
       if(cichar != aval[i]) \
         goto bad; \
     } \
     LCPDEBUG((LOG_DEBUG, "ADDCICB OK!")); \
   }

    ACKCISHORT(CI_MRU, go->neg_mru, go->mru);
    ACKCILONG(CI_ASYNCMAP, go->neg_asyncmap, go->asyncmap);
    ACKCICHAP(CI_AUTHTYPE, go->neg_chap, PPP_CHAP, go->chap_mdtype);
    ACKCISHORT(CI_AUTHTYPE, !go->neg_chap && go->neg_upap, PPP_PAP);
    ACKCILQR(CI_QUALITY, go->neg_lqr, go->lqr_period);
    ACKCILONG(CI_MAGICNUMBER, go->neg_magicnumber, go->magicnumber);
    ACKCIVOID(CI_PCOMPRESSION, go->neg_pcompression);
    ACKCIVOID(CI_ACCOMPRESSION, go->neg_accompression);
    ACKCICB(CI_CALLBACK, go->neg_callback, go->cbopt.type ,
            go->cbopt.message,go->cbopt.mlen );
    ACKCISHORT(CI_MPMRRU,go->neg_mpmrru,go->mp_mrru);
    ACKCIVOID(CI_MPSHORTSEQ,go->neg_mpshortseq);
    ACKCIMPDISCRI(CI_MPDISCRIMINATOR,go->neg_mpdiscr,go->mp_class,go->mp_addr,go->mp_alen);

    /*
     * If there are any remaining CIs, then this packet is bad.
     */
    if (len != 0)
	goto bad;
    return (1);
bad:
    LCPDEBUG((LOG_WARNING, "lcp_acki: received bad Ack!"));
    return (0);
}


/*
 * lcp_nakci - Peer has sent a NAK for some of our CIs.
 * This should not modify any state if the Nak is bad
 * or if LCP is in the OPENED state.
 *
 * Returns:
 *	0 - Nak was bad.
 *	1 - Nak was good.
 */
static int lcp_nakci(fsm *f,u_char *p,int len)
{
  struct link_struct *tlns = &lns[f->unit];
    lcp_options *go = &lcp_gotoptions[tlns->lcp_unit];
    lcp_options *wo = &lcp_wantoptions[tlns->lcp_unit];
    u_char citype, cichar, *next;
    u_short cishort;
    u_int32_t cilong;
    lcp_options no;		/* options we've seen Naks for */
    lcp_options try;		/* options to request next time */
    int looped_back = 0;
    int cilen;

    BZERO(&no, sizeof(no));
    try = *go;

    /*
     * Any Nak'd CIs must be in exactly the same order that we sent.
     * Check packet length and CI length at each step.
     * If we find any deviations, then this packet is bad.
     */
#define NAKCIVOID(opt, neg, code) \
    if (go->neg && \
	len >= CILEN_VOID && \
	p[1] == CILEN_VOID && \
	p[0] == opt) { \
	len -= CILEN_VOID; \
	INCPTR(CILEN_VOID, p); \
	no.neg = 1; \
	code \
    }
#define NAKCICHAP(opt, neg, code) \
    if (go->neg && \
	len >= CILEN_CHAP && \
	p[1] == CILEN_CHAP && \
	p[0] == opt) { \
	len -= CILEN_CHAP; \
	INCPTR(2, p); \
	GETSHORT(cishort, p); \
	GETCHAR(cichar, p); \
	no.neg = 1; \
	code \
    }
#define NAKCISHORT(opt, neg, code) \
    if (go->neg && \
	len >= CILEN_SHORT && \
	p[1] == CILEN_SHORT && \
	p[0] == opt) { \
	len -= CILEN_SHORT; \
	INCPTR(2, p); \
	GETSHORT(cishort, p); \
	no.neg = 1; \
	code \
    }
#define NAKCICHAR(opt, neg, code) \
    if (go->neg && \
       len >= CILEN_CHAR && \
       p[1] == CILEN_CHAR && \
       p[0] == opt) { \
       len -= CILEN_CHAR; \
       INCPTR(2, p); \
       GETCHAR(cichar, p); \
       no.neg = 1; \
       code \
    }
#define NAKCILONG(opt, neg, code) \
    if (go->neg && \
	len >= CILEN_LONG && \
	p[1] == CILEN_LONG && \
	p[0] == opt) { \
	len -= CILEN_LONG; \
	INCPTR(2, p); \
	GETLONG(cilong, p); \
	no.neg = 1; \
	code \
    }
#define NAKCILQR(opt, neg, code) \
    if (go->neg && \
	len >= CILEN_LQR && \
	p[1] == CILEN_LQR && \
	p[0] == opt) { \
	len -= CILEN_LQR; \
	INCPTR(2, p); \
	GETSHORT(cishort, p); \
	GETLONG(cilong, p); \
	no.neg = 1; \
	code \
    }
#define NAKCIMPDISCRI(opt,neg,code,alen) \
    if(go->neg && (len >= alen+3) && (p[1] == alen+3) && p[0] == opt) { \
      int i; \
      INCPTR(2,p); \
      len -= alen+3; \
      GETCHAR(cichar,p); \
      for(i=0;i<alen;i++) \
        GETCHAR(cichar,p); \
      no.neg = 1; \
      code \
    }


    /*
     * We don't care if they want to send us smaller packets than
     * we want.  Therefore, accept any MRU less than what we asked for,
     * but then ignore the new value when setting the MRU in the kernel.
     * If they send us a bigger MRU than what we asked, accept it, up to
     * the limit of the default MRU we'd get if we didn't negotiate.
     */
    NAKCISHORT(CI_MRU, neg_mru,
	       if (cishort <= wo->mru || cishort < DEFMRU)
		   try.mru = cishort;
	       );

    /*
     * Add any characters they want to our (receive-side) asyncmap.
     */
    NAKCILONG(CI_ASYNCMAP, neg_asyncmap,
	      try.asyncmap = go->asyncmap | cilong;
	      );

    /*
     * If they've nak'd our authentication-protocol, check whether
     * they are proposing a different protocol, or a different
     * hash algorithm for CHAP.
     */
    if ((go->neg_chap || go->neg_upap)
	&& len >= CILEN_SHORT
	&& p[0] == CI_AUTHTYPE && p[1] >= CILEN_SHORT && p[1] <= len) {
	cilen = p[1];
	len -= cilen;
	no.neg_chap = go->neg_chap;
	no.neg_upap = go->neg_upap;
	INCPTR(2, p);
        GETSHORT(cishort, p);
	if (cishort == PPP_PAP && cilen == CILEN_SHORT) {
	    /*
	     * If we were asking for CHAP, they obviously don't want to do it.
	     * If we weren't asking for CHAP, then we were asking for PAP,
	     * in which case this Nak is bad.
	     */
	    if (!go->neg_chap)
		goto bad;
	    try.neg_chap = 0;

	} else if (cishort == PPP_CHAP && cilen == CILEN_CHAP) {
	    GETCHAR(cichar, p);
	    if (go->neg_chap) {
		/*
		 * We were asking for CHAP/MD5; they must want a different
		 * algorithm.  If they can't do MD5, we'll have to stop
		 * asking for CHAP.
		 */
		if (cichar != go->chap_mdtype) {
		    LCPDEBUG((LOG_WARNING, "lcp_nakci: chap nak due to mdtype differences"));
 		    go->neg_chap = 0;
		}
	    } else {
		/*
		 * Stop asking for PAP if we were asking for it.
		 */
		try.neg_upap = 0;
	    }

	} else {
	    /*
	     * We don't recognize what they're suggesting.
	     * Stop asking for what we were asking for.
	     */
	    if (go->neg_chap)
		try.neg_chap = 0;
	    else
		try.neg_upap = 0;
	    p += cilen - CILEN_SHORT;
	}
    }

    /*
     * If they can't cope with our link quality protocol, we'll have
     * to stop asking for LQR.  We haven't got any other protocol.
     * If they Nak the reporting period, take their value XXX ?
     */
    NAKCILQR(CI_QUALITY, neg_lqr,
	     if (cishort != PPP_LQR)
		 try.neg_lqr = 0;
	     else
		 try.lqr_period = cilong;
	     );

    /*
     * If they've nak'd our callback request, see if we can fall
     * back to a different method. If not, give up asking for callback.
     */
    if(go->neg_callback && len >= CILEN_CB
	&& p[0] == CI_CALLBACK && p[1] >= CILEN_CB && p[1] <= len) {
	 int i;
	 int cb_type;
	 int mlen = p[1] - CILEN_CB;
	 INCPTR(2,p);
	 len -= mlen+CILEN_CB;
	 GETCHAR(cb_type,p); 
	 for (i=0;i<mlen;i++) {
	   GETCHAR(cichar,p);
	 }
	 no.neg_callback = 1;
	 LCPDEBUG((LOG_DEBUG, "Got NAK for callback request. Peer suggested cb type %d", cb_type));
	 /* See if the suggested callback type fits our wanted type */
	 switch (cb_type) {
	 case CB_AUTH:
	   try.neg_callback = wo->cbopt.neg_rfc
	     && (wo->cbopt.type == CB_AUTH
		 || (wo->cbopt.type == CB_CBCP && wo->cbopt.mlen == 0));
	   break;
	 case CB_PHONENO:
	   try.neg_callback = wo->cbopt.neg_rfc
	     && (wo->cbopt.type == CB_PHONENO
		 || (wo->cbopt.type == CB_CBCP && wo->cbopt.mlen != 0));
	   break;
	 case CB_CBCP:
	   try.neg_callback = wo->cbopt.neg_cbcp;
	   break;
	 case CB_DIALSTRING:
	 case CB_LOCATIONID:
	 case CB_NAME:
	   try.neg_callback = wo->cbopt.neg_rfc && wo->cbopt.mlen != 0
	     && wo->cbopt.type == cb_type;
	   break;
	 default:
	   try.neg_callback = 0;
	   break;
	 }
	 if (try.neg_callback) {
	   try.cbopt.type = cb_type;
	   if (cb_type != CB_AUTH && cb_type != CB_CBCP) {
	     try.cbopt.mlen = wo->cbopt.mlen;
	     try.cbopt.message = wo->cbopt.message;
	   }
	   LCPDEBUG((LOG_DEBUG, "Trying callback type %d", try.cbopt.type));
	 } else {
	   syslog(LOG_INFO, "Requested callback type %d does not match our configuration -> doing no callback", cb_type);
	 }
    }
    
    /*
     * Check for a looped-back line.
     */
    NAKCILONG(CI_MAGICNUMBER, neg_magicnumber,
	      try.magicnumber = magic();
	      looped_back = 1;
	      );

    /*
     * Peer shouldn't send Nak for protocol compression or
     * address/control compression requests; they should send
     * a Reject instead.  If they send a Nak, treat it as a Reject.
     */
    NAKCIVOID(CI_PCOMPRESSION, neg_pcompression,
	      try.neg_pcompression = 0;
	      );
    NAKCIVOID(CI_ACCOMPRESSION, neg_accompression,
	      try.neg_accompression = 0;
	      );
    NAKCISHORT(CI_MPMRRU,neg_mpmrru, try.mp_mrru = cishort;);
    NAKCIVOID(CI_MPSHORTSEQ,neg_mpshortseq,try.neg_mpshortseq=0;)
    NAKCIMPDISCRI(CI_MPDISCRIMINATOR,neg_mpdiscr,try.neg_mpdiscr=0;,go->mp_alen);

    /*
     * There may be remaining CIs, if the peer is requesting negotiation
     * on an option that we didn't include in our request packet.
     * If we see an option that we requested, or one we've already seen
     * in this packet, then this packet is bad.
     * If we wanted to respond by starting to negotiate on the requested
     * option(s), we could, but we don't, because except for the
     * authentication type and quality protocol, if we are not negotiating
     * an option, it is because we were told not to.
     * For the authentication type, the Nak from the peer means
     * `let me authenticate myself with you' which is a bit pointless.
     * For the quality protocol, the Nak means `ask me to send you quality
     * reports', but if we didn't ask for them, we don't want them.
     * An option we don't recognize represents the peer asking to
     * negotiate some option we don't support, so ignore it.
     */
    while (len > CILEN_VOID) {
	GETCHAR(citype, p);
	GETCHAR(cilen, p);
	if (cilen < CILEN_VOID || (len -= cilen) < 0)
	    goto bad;
	next = p + cilen - 2;

	switch (citype) {
	case CI_MRU:
	    if (go->neg_mru || no.neg_mru || cilen != CILEN_SHORT)
		goto bad;
	    GETSHORT(cishort, p);
	    if (cishort < DEFMRU)
		try.mru = cishort;
	    break;
	case CI_ASYNCMAP:
	    if (go->neg_asyncmap || no.neg_asyncmap || cilen != CILEN_LONG)
		goto bad;
	    break;
	case CI_AUTHTYPE:
	    if (go->neg_chap || no.neg_chap || go->neg_upap || no.neg_upap)
		goto bad;
	    break;
	case CI_CALLBACK:
	    if (go->neg_callback || no.neg_callback)
		goto bad;
	    break;
	case CI_MAGICNUMBER:
	    if (go->neg_magicnumber || no.neg_magicnumber ||
		cilen != CILEN_LONG)
		goto bad;
	    break;
	case CI_PCOMPRESSION:
	    if (go->neg_pcompression || no.neg_pcompression
		|| cilen != CILEN_VOID)
		goto bad;
	    break;
	case CI_ACCOMPRESSION:
	    if (go->neg_accompression || no.neg_accompression
		|| cilen != CILEN_VOID)
		goto bad;
	    break;
	case CI_QUALITY:
	    if (go->neg_lqr || no.neg_lqr || cilen != CILEN_LQR)
		goto bad;
	    break;
	}
	p = next;
    }

    /* If there is still anything left, this packet is bad. */
    if (len != 0)
	goto bad;

    /*
     * OK, the Nak is good.  Now we can update state.
     */
    if (f->state != OPENED) {
	if (looped_back) {
	    if (++try.numloops >= lcp_loopbackfail) {
		syslog(LOG_NOTICE, "Serial line is looped back.");
		lcp_close(tlns->lcp_unit,"Serial line is looped back.");
	    }
	} else
	    try.numloops = 0;
	*go = try;
    }

    return 1;

bad:
    LCPDEBUG((LOG_WARNING, "lcp_nakci: received bad Nak!"));
    return 0;
}


/*
 * lcp_rejci - Peer has Rejected some of our CIs.
 * This should not modify any state if the Reject is bad
 * or if LCP is in the OPENED state.
 *
 * Returns:
 *	0 - Reject was bad.
 *	1 - Reject was good.
 */
static int
lcp_rejci(fsm *f,u_char *p,int len)
{
    lcp_options *go = &lcp_gotoptions[ lns[f->unit].lcp_unit ];
    u_char cichar;
    u_short cishort;
    u_int32_t cilong;
#ifdef DEBUGLCP
    u_char *start = p;
    int plen = len;
#endif
    lcp_options try;		/* options to request next time */

    try = *go;

    /*
     * Any Rejected CIs must be in exactly the same order that we sent.
     * Check packet length and CI length at each step.
     * If we find any deviations, then this packet is bad.
     */
#define REJCIVOID(opt, neg) \
    if (go->neg && \
	len >= CILEN_VOID && \
	p[1] == CILEN_VOID && \
	p[0] == opt) { \
	len -= CILEN_VOID; \
	INCPTR(CILEN_VOID, p); \
	try.neg = 0; \
	LCPDEBUG((LOG_INFO, "lcp_rejci rejected void opt %d", opt)); \
    }
#define REJCISHORT(opt, neg, val) \
    if (go->neg && \
	len >= CILEN_SHORT && \
	p[1] == CILEN_SHORT && \
	p[0] == opt) { \
	len -= CILEN_SHORT; \
	INCPTR(2, p); \
	GETSHORT(cishort, p); \
	/* Check rejected value. */ \
	if (cishort != val) \
	    goto bad; \
	try.neg = 0; \
	LCPDEBUG((LOG_INFO,"lcp_rejci rejected short opt %d", opt)); \
    }
#define REJCICHAP(opt, neg, val, digest) \
    if (go->neg && \
	len >= CILEN_CHAP && \
	p[1] == CILEN_CHAP && \
	p[0] == opt) { \
	len -= CILEN_CHAP; \
	INCPTR(2, p); \
	GETSHORT(cishort, p); \
	GETCHAR(cichar, p); \
	/* Check rejected value. */ \
	if (cishort != val || cichar != digest) \
	    goto bad; \
	try.neg = 0; \
	try.neg_upap = 0; \
	LCPDEBUG((LOG_INFO,"lcp_rejci rejected chap opt %d", opt)); \
    }
#define REJCILONG(opt, neg, val) \
    if (go->neg && \
	len >= CILEN_LONG && \
	p[1] == CILEN_LONG && \
	p[0] == opt) { \
	len -= CILEN_LONG; \
	INCPTR(2, p); \
	GETLONG(cilong, p); \
	/* Check rejected value. */ \
	if (cilong != val) \
	    goto bad; \
	try.neg = 0; \
	LCPDEBUG((LOG_INFO,"lcp_rejci rejected long opt %d", opt)); \
    }
#define REJCILQR(opt, neg, val) \
    if (go->neg && \
	len >= CILEN_LQR && \
	p[1] == CILEN_LQR && \
	p[0] == opt) { \
	len -= CILEN_LQR; \
	INCPTR(2, p); \
	GETSHORT(cishort, p); \
	GETLONG(cilong, p); \
	/* Check rejected value. */ \
	if (cishort != PPP_LQR || cilong != val) \
	    goto bad; \
	try.neg = 0; \
	LCPDEBUG((LOG_INFO,"lcp_rejci rejected LQR opt %d", opt)); \
    }
#define REJCIMPDISCRI(opt,neg,clas,aval,alen) \
    if(go->neg && (len >= alen+3) && (p[1] == alen+3) && p[0] == opt) { \
      int i; \
      len -= alen+3; \
      INCPTR(2,p); \
      GETCHAR(cichar,p); \
      if(cichar != clas) { \
        syslog(LOG_WARNING , "Lcp-ConfRej, MPdiscr: bad class!\n"); \
        goto bad; } \
      for(i=0;i<alen;i++) { \
        GETCHAR(cichar,p); \
        if(cichar != aval[i]) { \
          syslog(LOG_WARNING , "Lcp-ConfRej, MPdiscr: bad ID/Address!\n"); \
          goto bad; } \
      } \
      try.neg = 0; \
    }

#define REJCICB(opt,neg,type,aval,alen) { \
    int checklen = (type!=CB_CBCP&&type!=CB_AUTH?alen:0); \
    if(go->neg && (len >= checklen+3) && (p[1] == checklen+3) && p[0] == opt) { \
      int i; \
      len -= checklen+3; \
      INCPTR(2,p); \
      GETCHAR(cichar,p); \
      if(cichar != type) { \
        syslog(LOG_WARNING , "Lcp-ConfRej, Callback: bad type!\n"); \
        goto bad; } \
      for(i=0;i<checklen;i++) { \
        GETCHAR(cichar,p); \
        if(cichar != aval[i]) { \
          syslog(LOG_WARNING , "Lcp-ConfRej, Callback: bad type arg!\n"); \
          goto bad; } \
      } \
      try.neg = 0; \
      LCPDEBUG((LOG_INFO,"lcp_rejci rejected Callback opt %d", opt)); \
    }}


    REJCISHORT(CI_MRU, neg_mru, go->mru);
    REJCILONG(CI_ASYNCMAP, neg_asyncmap, go->asyncmap);
    REJCICHAP(CI_AUTHTYPE, neg_chap, PPP_CHAP, go->chap_mdtype);
    if (!go->neg_chap) {
	REJCISHORT(CI_AUTHTYPE, neg_upap, PPP_PAP);
    }
    REJCILQR(CI_QUALITY, neg_lqr, go->lqr_period);
    REJCILONG(CI_MAGICNUMBER, neg_magicnumber, go->magicnumber);
    REJCIVOID(CI_PCOMPRESSION, neg_pcompression);
    REJCIVOID(CI_ACCOMPRESSION, neg_accompression);
    REJCICB(CI_CALLBACK, neg_callback, go->cbopt.type, go->cbopt.message, go->cbopt.mlen );
    REJCISHORT(CI_MPMRRU,neg_mpmrru,go->mp_mrru);
    REJCIVOID(CI_MPSHORTSEQ,neg_mpshortseq);
    REJCIMPDISCRI(CI_MPDISCRIMINATOR,neg_mpdiscr,go->mp_class,go->mp_addr,go->mp_alen);

    /*
     * If there are any remaining CIs, then this packet is bad.
     */
    if (len != 0)
	goto bad;
    /*
     * Now we can update state.
     */
    if (f->state != OPENED)
	*go = try;
    return 1;

bad:
    LCPDEBUG((LOG_WARNING, "lcp_rejci: received bad Reject!"));
    LCPDEBUG((LOG_WARNING, "lcp_rejci: plen %d len %d off %d",
	      plen, len, p - start));
    return 0;
}


/*
 * lcp_reqci - Check the peer's requested CIs and send appropriate response.
 *
 * Returns: CONFACK, CONFNAK or CONFREJ and input packet modified
 * appropriately.  If reject_if_disagree is non-zero, doesn't return
 * CONFNAK; returns CONFREJ if it can't return CONFACK.
 */
static int lcp_reqci(fsm *f,u_char *inp,int *lenp,int reject_if_disagree)
{
    struct link_struct *tlns = &lns[f->unit];
    lcp_options *go = &lcp_gotoptions[tlns->lcp_unit];
    lcp_options *ho = &lcp_hisoptions[tlns->lcp_unit];
    lcp_options *ao = &lcp_allowoptions[tlns->lcp_unit];
    u_char *cip, *next;		/* Pointer to current and next CIs */
    u_char cilen, citype=0, cichar;/* Parsed len, type, char value */
    u_short cishort;		/* Parsed short value */
    u_int32_t cilong;		/* Parse long value */
    int rc = CONFACK;		/* Final packet return code */
    int orc;			/* Individual option return code */
    u_char *p;			/* Pointer to next char to parse */
    u_char *rejp;		/* Pointer to next char in reject frame */
    u_char *nakp;		/* Pointer to next char in Nak frame */
    int l = *lenp;		/* Length left */
    u_char mpclass;
    u_char mpaddr[20];
    u_short mpmrru;

    /*
     * Reset all his options.
     */
    BZERO(ho, sizeof(*ho));

    /*
     * Process all his options.
     */
    next = inp;
    nakp = nak_buffer;
    rejp = inp;
    while (l) {
	orc = CONFACK;			/* Assume success */
	cip = p = next;			/* Remember begining of CI */
	if (l < 2 ||			/* Not enough data for CI header or */
	    p[1] < 2 ||			/*  CI length too small or */
	    p[1] > l) {			/*  CI length too big? */
	    LCPDEBUG((LOG_WARNING, "lcp_reqci: bad CI length!"));
	    orc = CONFREJ;		/* Reject bad CI */
	    cilen = l;			/* Reject till end of packet */
	    l = 0;			/* Don't loop again */
	    citype = 0;
	    goto endswitch;
	}
	GETCHAR(citype, p);		/* Parse CI type */
	GETCHAR(cilen, p);		/* Parse CI length */
	l -= cilen;			/* Adjust remaining length */
	next += cilen;			/* Step to next CI */

	switch (citype) {		/* Check CI type */
	case CI_MRU:
	    LCPDEBUG((LOG_INFO, "lcp_reqci: rcvd MRU"));
	    if (!ao->neg_mru ||		/* Allow option? */
		cilen != CILEN_SHORT) {	/* Check CI length */
		orc = CONFREJ;		/* Reject CI */
		break;
	    }
	    GETSHORT(cishort, p);	/* Parse MRU */
	    LCPDEBUG((LOG_INFO, "(%d)", cishort));

	    /*
	     * He must be able to receive at least our minimum.
	     * No need to check a maximum.  If he sends a large number,
	     * we'll just ignore it.
	     */
	    if (cishort < MINMRU) {
		orc = CONFNAK;		/* Nak CI */
		PUTCHAR(CI_MRU, nakp);
		PUTCHAR(CILEN_SHORT, nakp);
		PUTSHORT(MINMRU, nakp);	/* Give him a hint */
		break;
	    }
	    ho->neg_mru = 1;		/* Remember he sent MRU */
	    ho->mru = cishort;		/* And remember value */
	    break;

	case CI_ASYNCMAP:
	    LCPDEBUG((LOG_INFO, "lcp_reqci: rcvd ASYNCMAP"));
	    if (!ao->neg_asyncmap ||
		cilen != CILEN_LONG) {
		orc = CONFREJ;
		break;
	    }
	    GETLONG(cilong, p);
	    LCPDEBUG((LOG_INFO, "(%x)", (unsigned int) cilong));

	    /*
	     * Asyncmap must have set at least the bits
	     * which are set in lcp_allowoptions[unit].asyncmap.
	     */
	    if ((ao->asyncmap & ~cilong) != 0) {
		orc = CONFNAK;
		PUTCHAR(CI_ASYNCMAP, nakp);
		PUTCHAR(CILEN_LONG, nakp);
		PUTLONG(ao->asyncmap | cilong, nakp);
		break;
	    }
	    ho->neg_asyncmap = 1;
	    ho->asyncmap = cilong;
	    break;

	case CI_AUTHTYPE:
	    LCPDEBUG((LOG_INFO, "lcp_reqci: rcvd AUTHTYPE"));
	    if (cilen < CILEN_SHORT ||
		!(ao->neg_upap || ao->neg_chap)) {
		/*
		 * Reject the option if we're not willing to authenticate.
		 */
		orc = CONFREJ;
		if (!ao->neg_upap && !ao->neg_chap) {
		    LCPDEBUG((LOG_INFO, " we're not willing to authenticate"));
                }
		else {
		    LCPDEBUG((LOG_INFO, " cilen is too short!"));
                }
		break;
	    }
	    GETSHORT(cishort, p);
	    LCPDEBUG((LOG_INFO, "(%x)", cishort));

	    /*
	     * Authtype must be PAP, MSCHAP (if enabled), or CHAP.
	     *
	     * Note: if both ao->neg_upap and ao->neg_chap are set,
	     * and the peer sends a Configure-Request with two
	     * authenticate-protocol requests, one for CHAP and one
	     * for PAP, then we will reject the second request.
	     * Whether we end up doing CHAP or PAP depends then on
	     * the ordering of the CIs in the peer's Configure-Request.
	     */

	    if (cishort == PPP_PAP) {
		if (ho->neg_chap ||	/* we've already accepted CHAP */
		    cilen != CILEN_SHORT) {
		    LCPDEBUG((LOG_WARNING,
			      "lcp_reqci: rcvd AUTHTYPE PAP, rejecting..."));
		    orc = CONFREJ;
		    break;
		}
		if (!ao->neg_upap) {	/* we don't want to do PAP */
		    orc = CONFNAK;	/* NAK it and suggest CHAP */
		    PUTCHAR(CI_AUTHTYPE, nakp);
		    PUTCHAR(CILEN_CHAP, nakp);
		    PUTSHORT(PPP_CHAP, nakp);
		    PUTCHAR(ao->chap_mdtype, nakp);
		    break;
		}
		ho->neg_upap = 1;
		break;
	    }
	    if (cishort == PPP_CHAP) {
		if (ho->neg_upap ||	/* we've already accepted PAP */
		    cilen != CILEN_CHAP) {
		    LCPDEBUG((LOG_INFO,
			      "lcp_reqci: rcvd AUTHTYPE CHAP, rejecting..."));
		    orc = CONFREJ;
		    break;
		}
		if (!ao->neg_chap) {	/* we don't want to do CHAP */
		    orc = CONFNAK;	/* NAK it and suggest PAP */
		    PUTCHAR(CI_AUTHTYPE, nakp);
		    PUTCHAR(CILEN_SHORT, nakp);
		    PUTSHORT(PPP_PAP, nakp);
		    break;
		}
		GETCHAR(cichar, p);	/* get digest type*/
#ifndef USE_MSCHAP
               if (cichar != ao->chap_mdtype) {
#else
               if (cichar != ao->chap_mdtype && cichar != CHAP_MICROSOFT) {
#endif /* USE_MSCHAP */
		    orc = CONFNAK;
		    PUTCHAR(CI_AUTHTYPE, nakp);
		    PUTCHAR(CILEN_CHAP, nakp);
		    PUTSHORT(PPP_CHAP, nakp);
		    PUTCHAR(ao->chap_mdtype, nakp);
		    break;
		}
		ho->chap_mdtype = cichar; /* save md type */
		ho->neg_chap = 1;
		break;
	    }

	    /*
	     * We don't recognize the protocol they're asking for.
	     * Nak it with something we're willing to do.
	     * (At this point we know ao->neg_upap || ao->neg_chap.)
	     */
	    orc = CONFNAK;
	    PUTCHAR(CI_AUTHTYPE, nakp);
	    if (ao->neg_chap) {
		PUTCHAR(CILEN_CHAP, nakp);
		PUTSHORT(PPP_CHAP, nakp);
		PUTCHAR(ao->chap_mdtype, nakp);
	    } else {
		PUTCHAR(CILEN_SHORT, nakp);
		PUTSHORT(PPP_PAP, nakp);
	    }
	    break;

	case CI_QUALITY:
	    LCPDEBUG((LOG_INFO, "lcp_reqci: rcvd QUALITY"));
	    if (!ao->neg_lqr ||
		cilen != CILEN_LQR) {
		orc = CONFREJ;
		break;
	    }

	    GETSHORT(cishort, p);
	    GETLONG(cilong, p);
	    LCPDEBUG((LOG_INFO, "(%x %x)", cishort, (unsigned int) cilong));

	    /*
	     * Check the protocol and the reporting period.
	     * XXX When should we Nak this, and what with?
	     */
	    if (cishort != PPP_LQR) {
		orc = CONFNAK;
		PUTCHAR(CI_QUALITY, nakp);
		PUTCHAR(CILEN_LQR, nakp);
		PUTSHORT(PPP_LQR, nakp);
		PUTLONG(ao->lqr_period, nakp);
		break;
	    }
	    break;

	case CI_MAGICNUMBER:
	    LCPDEBUG((LOG_INFO, "lcp_reqci: rcvd MAGICNUMBER"));
	    if (!(ao->neg_magicnumber || go->neg_magicnumber) ||
		cilen != CILEN_LONG) {
		orc = CONFREJ;
		break;
	    }
	    GETLONG(cilong, p);
	    LCPDEBUG((LOG_INFO, "(%x)", (unsigned int) cilong));

	    /*
	     * He must have a different magic number.
	     */
	    if (go->neg_magicnumber &&
		cilong == go->magicnumber) {
		cilong = magic();	/* Don't put magic() inside macro! */
		orc = CONFNAK;
		PUTCHAR(CI_MAGICNUMBER, nakp);
		PUTCHAR(CILEN_LONG, nakp);
		PUTLONG(cilong, nakp);
		break;
	    }
	    ho->neg_magicnumber = 1;
	    ho->magicnumber = cilong;
	    break;


	case CI_PCOMPRESSION:
	    LCPDEBUG((LOG_INFO, "lcp_reqci: rcvd PCOMPRESSION"));
	    if (!ao->neg_pcompression ||
		cilen != CILEN_VOID) {
		orc = CONFREJ;
		break;
	    }
	    ho->neg_pcompression = 1;
	    break;

	case CI_ACCOMPRESSION:
	    LCPDEBUG((LOG_INFO, "lcp_reqci: rcvd ACCOMPRESSION"));
	    if (!ao->neg_accompression ||
		cilen != CILEN_VOID) {
		orc = CONFREJ;
		break;
	    }
	    ho->neg_accompression = 1;
	    break;
	case CI_MPDISCRIMINATOR:
          LCPDEBUG((LOG_INFO, "lcp_reqci: rcvd MP Discriminator"));
          if(!ao->neg_mpdiscr || cilen < 3) {
            orc = CONFREJ;
            break; 
          }
          GETCHAR(mpclass,p);
          {
            int i;
            for(i=3;i<cilen;i++)
              GETCHAR(mpaddr[i-3],p);
          }
          ho->neg_mpdiscr = 1; /* usually, the DISCRIMINATOR is NO way to enable MP */
          ho->mp_class = mpclass;
          memcpy(ho->mp_addr,mpaddr,cilen-3); 
          ho->mp_alen = cilen-3;
          break;
	case CI_MPMRRU:
          if(!ao->neg_mpmrru || cilen != CILEN_SHORT) {
            orc = CONFREJ;
            break;
          }
          LCPDEBUG((LOG_INFO, "lcp_reqci: rcvd MP MRRU"));
          GETSHORT(mpmrru,p);
          ho->neg_mpmrru = 1;
          ho->mp_mrru = mpmrru;
          break;
	case CI_MPSHORTSEQ:
          LCPDEBUG((LOG_INFO, "lcp_reqci: rcvd MP short sequence"));
          if(!ao->neg_mpshortseq || cilen != CILEN_VOID)
          {
            orc = CONFREJ;
            break;
          }
          ho->neg_mpshortseq = 0; /* we don't want it yet */
          orc = CONFREJ;
          break;

	default:
	    LCPDEBUG((LOG_INFO, "lcp_reqci: rcvd unknown option %d",
		      citype));
	    orc = CONFREJ;
	    break;
	}

endswitch:
	LCPDEBUG((LOG_INFO, " (%s)", CODENAME(orc)));
	if (orc == CONFACK &&		/* Good CI */
	    rc != CONFACK)		/*  but prior CI wasnt? */
	    continue;			/* Don't send this one */

	if (orc == CONFNAK) {		/* Nak this CI? */
	    if (reject_if_disagree	/* Getting fed up with sending NAKs? */
		&& citype != CI_MAGICNUMBER) {
		orc = CONFREJ;		/* Get tough if so */
	    } else {
		if (rc == CONFREJ)	/* Rejecting prior CI? */
		    continue;		/* Don't send this one */
		rc = CONFNAK;
	    }
	}
	if (orc == CONFREJ) {		/* Reject this CI */
	    rc = CONFREJ;
	    if (cip != rejp)		/* Need to move rejected CI? */
		BCOPY(cip, rejp, cilen); /* Move it */
	    INCPTR(cilen, rejp);	/* Update output pointer */
	}
    }

    /*
     * If we wanted to send additional NAKs (for unsent CIs), the
     * code would go here.  The extra NAKs would go at *nakp.
     * At present there are no cases where we want to ask the
     * peer to negotiate an option.
     */

    switch (rc) {
    case CONFACK:
	*lenp = next - inp;
	break;
    case CONFNAK:
	/*
	 * Copy the Nak'd options from the nak_buffer to the caller's buffer.
	 */
	*lenp = nakp - nak_buffer;
	BCOPY(nak_buffer, inp, *lenp);
	break;
    case CONFREJ:
	*lenp = rejp - inp;
	break;
    }

    LCPDEBUG((LOG_INFO, "lcp_reqci: returning CONF%s.", CODENAME(rc)));
    return (rc);			/* Return final code */
}


/*
 * lcp_up - LCP has come UP.
 *
 * Start UPAP, IPCP, etc.
 */
static void lcp_up(fsm *f)
{
    struct link_struct *tlns = &lns[f->unit]; 

    lcp_options *wo = &lcp_wantoptions[tlns->lcp_unit];
    lcp_options *ho = &lcp_hisoptions[tlns->lcp_unit];
    lcp_options *go = &lcp_gotoptions[tlns->lcp_unit];
    lcp_options *ao = &lcp_allowoptions[tlns->lcp_unit];

	syslog(LOG_DEBUG, "lcp layer is UP");

    if (!go->neg_magicnumber)
	go->magicnumber = 0;
    if (!ho->neg_magicnumber)
	ho->magicnumber = 0;

    /*
     * Set our MTU to the smaller of the MTU we wanted and
     * the MRU our peer wanted.  If we negotiated an MRU,
     * set our MRU to the larger of value we wanted and
     * the value we got in the negotiation.
     */
    ppp_send_config(f->unit, MIN(ao->mru, (ho->neg_mru? ho->mru: PPP_MRU)),
		    (ho->neg_asyncmap? ho->asyncmap: 0xffffffff),
		    ho->neg_pcompression, ho->neg_accompression);
    /*
     * If the asyncmap hasn't been negotiated, we really should
     * set the receive asyncmap to ffffffff, but we set it to 0
     * for backwards contemptibility.
     */
    ppp_recv_config(f->unit, (go->neg_mru? MAX(wo->mru, go->mru): PPP_MRU),
		    (go->neg_asyncmap? go->asyncmap: 0x00000000),
		    go->neg_pcompression, go->neg_accompression);

    if (ho->neg_mru)
      tlns->peer_mru = ho->mru;

    (*chap_protent.lowerup)(tlns->chap_unit);	/* Enable CHAP */
    (*pap_protent.lowerup)(tlns->upap_unit);	/* Enable UPAP */
    lcp_echo_lowerup(tlns->lcp_unit);   /* Enable echo messages */

    link_established(f->unit);
}


/*
 * lcp_down - LCP has gone DOWN.
 *
 * Alert other protocols.
 */
static void lcp_down(fsm *f)
{
  struct link_struct *tlns = &lns[f->unit];

  link_down(f->unit); /* -> takes down ipcp,ccp */

  lcp_echo_lowerdown(tlns->lcp_unit);
  (*chap_protent.lowerdown)(tlns->chap_unit);
  (*pap_protent.lowerdown)(tlns->upap_unit);

#ifndef ISDN4LINUX_PATCH
    sifdown(f->unit);
#endif

  ppp_send_config(f->unit, PPP_MRU, 0xffffffff, 0, 0);
  ppp_recv_config(f->unit, PPP_MRU, 0x00000000, 0, 0);
  tlns->peer_mru = PPP_MRU;

}


/*
 * lcp_starting - LCP needs the lower layer up.
 */
static void lcp_starting(fsm *f)
{
  link_required(f->unit);
}


/*
 * lcp_finished - LCP has finished with the lower layer.
 */
static void lcp_finished(fsm *f)
{
  link_terminated(f->unit);
}


/*
 * lcp_printpkt - print the contents of an LCP packet.
 */
char *lcp_codenames[] = {
    "ConfReq", "ConfAck", "ConfNak", "ConfRej",
    "TermReq", "TermAck", "CodeRej", "ProtRej",
    "EchoReq", "EchoRep", "DiscReq"
};

int lcp_printpkt(u_char *p,int plen,void (*printer)(void*,char*,...),void *arg)
{
    int code, id, len, olen;
    u_char *pstart, *optend;
    u_short cishort;
    u_int32_t cilong;
    u_char cichar;

#if 0
	{
		int i;
		char msg[1024];
		for(i=0;i<plen;i++) {
			sprintf(msg+i*3,"%02x ",(int)(unsigned char)p[i]);
		}
		syslog(LOG_DEBUG, "lcp_frame: %s",msg);
	}
#endif

    if (plen < HEADERLEN)
	return 0;
    pstart = p;
    GETCHAR(code, p);
    GETCHAR(id, p);
    GETSHORT(len, p);
    if (len < HEADERLEN || len > plen)
	return 0;

    if (code >= 1 && code <= sizeof(lcp_codenames) / sizeof(char *))
	printer(arg, " %s", lcp_codenames[code-1]);
    else
	printer(arg, " code=0x%x", code);
    printer(arg, " id=0x%x", id);
    len -= HEADERLEN;
    switch (code) {
    case CONFREQ:
    case CONFACK:
    case CONFNAK:
    case CONFREJ:
	/* print option list */
	while (len >= 2) {
	    GETCHAR(code, p);
	    GETCHAR(olen, p);
	    p -= 2;
	    if (olen < 2 || olen > len) {
			break;
	    }
	    printer(arg, " <");
	    len -= olen;
	    optend = p + olen;
	    switch (code) {
		    case CI_MRU:
		if (olen == CILEN_SHORT) {
		    p += 2;
		    GETSHORT(cishort, p);
		    printer(arg, "mru %d", cishort);
		}
		break;
	    case CI_ASYNCMAP:
		if (olen == CILEN_LONG) {
		    p += 2;
		    GETLONG(cilong, p);
		    printer(arg, "asyncmap 0x%x", cilong);
		}
		break;
	    case CI_AUTHTYPE:
		if (olen >= CILEN_SHORT) {
		    p += 2;
		    printer(arg, "auth ");
		    GETSHORT(cishort, p);
		    switch (cishort) {
		    case PPP_PAP:
			printer(arg, "pap");
			break;
		    case PPP_CHAP:
			printer(arg, "chap");
			if (p < optend) {
			    GETCHAR(code, p);
			    if (code == CHAP_DIGEST_MD5)
			        printer (arg, " md5");
			    else
			        if (code == CHAP_MICROSOFT)
				    printer (arg, " msoft");
			        else
				    --p;
			}
			break;
		    default:
			printer(arg, "0x%x", cishort);
		    }
		}
		break;
	    case CI_QUALITY:
		if (olen >= CILEN_SHORT) {
		    p += 2;
		    printer(arg, "quality ");
		    GETSHORT(cishort, p);
		    switch (cishort) {
		    case PPP_LQR:
			printer(arg, "lqr");
			break;
		    default:
			printer(arg, "0x%x", cishort);
		    }
		}
		break;
		case CI_CALLBACK:
               if (olen >= CILEN_CHAR) {
                   p += 2;
                   GETCHAR(cichar, p);
           		   printer(arg, "callback %d",cichar);
					if(olen > CILEN_CHAR) {
		                int i;
						printer(arg, ",");
        		        for(i=0;i<olen-3;i++)
		                {
		                  GETCHAR(cichar,p);
		                  printer(arg,"%c",cichar);
		                }
					}
               }
			break;
	    case CI_MAGICNUMBER:
		if (olen == CILEN_LONG) {
		    p += 2;
		    GETLONG(cilong, p);
		    printer(arg, "magic 0x%x", cilong);
		}
		break;
	    case CI_PCOMPRESSION:
		if (olen == CILEN_VOID) {
		    p += 2;
		    printer(arg, "pcomp");
		}
		break;
	    case CI_ACCOMPRESSION:
		if (olen == CILEN_VOID) {
		    p += 2;
		    printer(arg, "accomp");
		}
		break;
		case CI_MPMRRU:
              p+=2;
              GETSHORT(cishort,p);
              printer(arg,"MPmrru %d",cishort);
              break;
            case CI_MPSHORTSEQ:
              p+=2;
              printer(arg,"MPshortseq");
              break;
            case CI_MPDISCRIMINATOR:
              p+=2;
              GETCHAR(cichar,p);
              printer(arg,"MPdiscr: 0x%x [", (int)cichar);
              {
                int i;
                for(i=0;i<olen-3;i++)
                {
                  GETCHAR(cichar,p);
                  printer(arg," %02x",(int)cichar);
                }
              }
              printer(arg," ]");
              break; 
	    }
	    while (p < optend) {
		GETCHAR(code, p);
		printer(arg, " %.2x", code);
	    }
	    printer(arg, ">");
	}
	break;
    }

    /* print the rest of the bytes in the packet */
    for (; len > 0; --len) {
	GETCHAR(code, p);
	printer(arg, " %.2x", code);
    }

    return p - pstart;
}

/*
 * Time to shut down the link because there is nothing out there.
 */

static void LcpLinkFailure (fsm *f)
{
    if (f->state == OPENED) {
        syslog (LOG_NOTICE, "Excessive lack of response to LCP echo frames.");
        lcp_close(lns[f->unit].lcp_unit,"Excessive lack of response to LCP echo frames.");		/* Reset connection */
    }
}

/*
 * Timer expired for the LCP echo requests from this process.
 */

static void LcpEchoCheck (fsm *f)
{
    long int delta;
#ifdef __linux__
    struct ppp_idle    ddinfo;
/*
 * Read the time since the last packet was received.
 */
    if (ioctl (lns[f->unit].fd, PPPIOCGIDLE, &ddinfo) < 0) {
        syslog (LOG_ERR, "ioctl(PPPIOCGIDLE): %m");
		return;
    }
/*
 * Compute the time since the last packet was received. If the timer
 *  has expired then send the echo request and reset the timer to maximum.
 */
    delta = (long int) lcp_echo_interval - (long int) ddinfo.recv_idle;
    if (delta < 0L) {
        LcpSendEchoRequest (f);
        delta = (int) lcp_echo_interval;
    }

#else /* Other implementations do not have ability to find delta */
    LcpSendEchoRequest (f);
    delta = (int) lcp_echo_interval;
#endif

/*
 * Start the timer for the next interval.
 */
    assert (lcp_echo_timer_running==0);
    TIMEOUT (LcpEchoTimeout, (caddr_t) f, (u_int32_t) delta);
    lcp_echo_timer_running = 1;
}

/*
 * LcpEchoTimeout - Timer expired on the LCP echo
 */

static void
LcpEchoTimeout (arg)
    caddr_t arg;
{
    if (lcp_echo_timer_running != 0) {
        lcp_echo_timer_running = 0;
        LcpEchoCheck ((fsm *) arg);
    }
}

/*
 * LcpEchoReply - LCP has received a reply to the echo
 */

static void lcp_received_echo_reply (fsm *f,int id,u_char *inp,int len)
{
    u_int32_t magic;

    /* Check the magic number - don't count replies from ourselves. */
    if (len < 4) {
	syslog(LOG_DEBUG, "lcp: received short Echo-Reply, length %d", len);
	return;
    }
    GETLONG(magic, inp);
    if (lcp_gotoptions[lns[f->unit].lcp_unit].neg_magicnumber
	&& magic == lcp_gotoptions[lns[f->unit].lcp_unit].magicnumber) {
	syslog(LOG_WARNING, "appear to have received our own echo-reply!");
	return;
    }

    /* Reset the number of outstanding echo frames */
    lcp_echos_pending = 0;
}

/*
 * LcpSendEchoRequest - Send an echo request frame to the peer
 */

static void
LcpSendEchoRequest (f)
    fsm *f;
{
    u_int32_t lcp_magic;
    u_char pkt[4], *pktp;
    struct link_struct *tlns = &lns[f->unit];

/*
 * Detect the failure of the peer at this point.
 */
    if (lcp_echo_fails != 0) {
        if (lcp_echos_pending++ >= lcp_echo_fails) {
            LcpLinkFailure(f);
	    lcp_echos_pending = 0;
	}
    }
/*
 * Make and send the echo request frame.
 */
    if (f->state == OPENED) {
        lcp_magic = lcp_gotoptions[tlns->lcp_unit].neg_magicnumber
	            ? lcp_gotoptions[tlns->lcp_unit].magicnumber
	            : 0L;
	pktp = pkt;
	PUTLONG(lcp_magic, pktp);
      
        fsm_sdata(f, ECHOREQ,
		  lcp_echo_number++ & 0xFF, pkt, pktp - pkt);
    }
}

/*
 * lcp_echo_lowerup - Start the timer for the LCP frame
 */

static void lcp_echo_lowerup (int unit)
{
  fsm *f = &lcp_fsm[unit];
    /* Clear the parameters for generating echo frames */
    lcp_echos_pending      = 0;
    lcp_echo_number        = 0;
    lcp_echo_timer_running = 0;
  
    /* If a timeout interval is specified then start the timer */
    if (lcp_echo_interval != 0)
        LcpEchoCheck (f);
#ifdef __linux__
    /* If a idle time limit is given then start it */
    if (idle_time_limit != 0)
        RestartIdleTimer (f);
#ifdef RADIUS
	if ( session_time_limit != 0 ) 
	{

		RestartSessionTimer ( f );
	}
#endif
#endif
}

/*
 * lcp_echo_lowerdown - Stop the timer for the LCP frame
 */

static void lcp_echo_lowerdown (int unit)
{
    fsm *f = &lcp_fsm[unit];

    if (lcp_echo_timer_running != 0) {
        UNTIMEOUT (LcpEchoTimeout, (caddr_t) f);
        lcp_echo_timer_running = 0;
    }
#ifdef __linux__
    /* If a idle time limit is running then stop it */
    if (idle_timer_running != 0) {
        UNTIMEOUT (IdleTimeCheck, (caddr_t) f);
        idle_timer_running = 0;
    }
#ifdef RADIUS
	if ( session_time_limit != 0 ) 
	{
		UNTIMEOUT (StopSession, (caddr_t) f );
		session_timer_running = 0 ;
	}
#endif
#endif
}
