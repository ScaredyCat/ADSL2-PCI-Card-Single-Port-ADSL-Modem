/*
 * cbcp - Call Back Configuration Protocol.
 *
 * Copyright (c) 1995 Pedro Roque Marques
 * All rights reserved.
 *
 * 2000-07-25 Callback improvements by richard.kunze@web.de 
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by Pedro Roque Marques.  The name of the author may not be used to
 * endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/*
              Microsoft Call Back Configuration Protocol.
                        by Pedro Roque Marques

The CBCP is a method by which the Microsoft Windows NT Server may
implement additional security. It is possible to configure the server
in such a manner so as to require that the client systems which
connect with it are required that following a valid authentication to
leave a method by which the number may be returned call.

It is a requirement of servers so configured that the protocol be
exchanged.

So, this set of patches may be applied to the pppd process to enable
the cbcp client *only* portion of the specification. It is primarily
meant to permit connection with Windows NT Servers.

The ietf-working specification may be obtained from ftp.microsoft.com
in the developr/rfc directory.

*/

#define PPP_CBCP        0xc029  /* Callback Control Protocol */

char cbcp_rcsid[] = "$Id: cbcp.c,v 1.7 2000/07/25 20:23:51 kai Exp $";

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <syslog.h>

#include "ipppd.h"
#include "cbcp.h"
#include "fsm.h"
#include "lcp.h"
#include "ipcp.h"


/*
 * Protocol entry points.
 */
static void cbcp_init      __P((int unit));
static void cbcp_open      __P((int unit));
static void cbcp_close     __P((int unit,char *));
static void cbcp_lowerup   __P((int unit));
static void cbcp_lowerdown   __P((int unit));
static void cbcp_input     __P((int unit, u_char *pkt, int len));
static void cbcp_protrej   __P((int unit));
static int  cbcp_printpkt  __P((u_char *pkt, int len,
                void (*printer) __P((void *, char *, ...)),
                void *arg));

struct protent cbcp_protent = {
    PPP_CBCP,
    cbcp_init,
    cbcp_input,
    cbcp_protrej,
    cbcp_lowerup,
    cbcp_lowerdown,
    cbcp_open,
    cbcp_close,
    cbcp_printpkt,
    NULL,
    0,
    "CBCP",
    NULL,
    NULL,
    NULL
};


cbcp_state cbcp[NUM_PPP];	

/* internal prototypes */

void cbcp_recvreq(cbcp_state *us, char *pckt, int len);
void cbcp_resp(cbcp_state *us);
void cbcp_up(cbcp_state *us);
void cbcp_recvack(cbcp_state *us, char *pckt, int len);
void cbcp_send(cbcp_state *us, u_char code, u_char *buf, int len);

/* init state */
static void cbcp_init(int cbcp_unit)
{
    cbcp_state *us;

    us = &cbcp[cbcp_unit];
    memset(us, 0, sizeof(cbcp_state));
    us->us_unit = -1;    
    us->us_type |= (1 << CB_CONF_NO);
    us->us_type |= (1 << CB_CONF_USER);
    us->us_type |= (1 << CB_CONF_LIST);
    us->us_type |= (1 << CB_CONF_ADMIN);
}

/* lower layer is up */
static void cbcp_lowerup(int cbcp_unit)
{
#if 0
    cbcp_state *us = &cbcp[cbcp_unit];
#endif
    syslog(LOG_DEBUG, "cbcp_lowerup");
}

static void cbcp_lowerdown(int cbcp_unit)
{
	syslog(LOG_DEBUG, "cbcp_lowerdown");
}

static void cbcp_open(int cbcp_unit)
{
    syslog(LOG_DEBUG, "cbcp_open");
}

static void cbcp_close(int cbcp_unit,char *reason)
{
	syslog(LOG_DEBUG, "cbcp_close: %s",reason);
}

/* process an incomming packet */
static void cbcp_input(int cbcp_unit, u_char *inpacket, int pktlen)
{
    u_char *inp;
    u_char code, id;
    u_short len;

    cbcp_state *us = &cbcp[cbcp_unit];

    inp = inpacket;

    if (pktlen < CBCP_MINLEN) {
        syslog(LOG_ERR, "CBCP packet is too small");
	return;
    }

    GETCHAR(code, inp);
    GETCHAR(id, inp);
    GETSHORT(len, inp);

#if 0
    if (len > pktlen) {
        syslog(LOG_ERR, "CBCP packet: invalid length");
        return;
    }
#endif

    len -= CBCP_MINLEN;
 
    switch(code) {
    case CBCP_REQ:
        us->us_id = id;
	cbcp_recvreq(us, inp, len);
	break;

    case CBCP_RESP:
	syslog(LOG_DEBUG, "CBCP_RESP received");
	break;

    case CBCP_ACK:
	if (id != us->us_id)
	    syslog(LOG_DEBUG, "id doesn't match: expected %d recv %d",
		   us->us_id, id);

	cbcp_recvack(us, inp, len);
	break;

    default:
	break;
    }
}

/* protocol was rejected by foe */
static void cbcp_protrej(int cbcp_unit)
{
}

char *cbcp_codenames[] = {"Request", "Response", "Ack"};

char *cbcp_optionnames[] = {  "NoCallback",
			      "UserDefined",
			      "AdminDefined",
			      "List"};
/* pretty print a packet */
static int cbcp_printpkt(u_char *p, int plen,
		   void (*printer) __P((void *, char *, ...)),
		   void *arg)
{
    int code, opt, id, len, olen, delay;
    u_char *pstart;

    if (plen < HEADERLEN)
	return 0;
    pstart = p;
    GETCHAR(code, p);
    GETCHAR(id, p);
    GETSHORT(len, p);
    if (len < HEADERLEN || len > plen)
	return 0;

    if (code >= 1 && code <= sizeof(cbcp_codenames) / sizeof(char *))
	printer(arg, " %s", cbcp_codenames[code-1]);
    else
	printer(arg, " code=0x%x", code); 

    printer(arg, " id=0x%x", id);
    len -= HEADERLEN;

    switch (code) {
    case CBCP_REQ:
    case CBCP_RESP:
    case CBCP_ACK:
        while(len >= 2) {
	    GETCHAR(opt, p);
	    GETCHAR(olen, p);

	    if (olen < 2 || olen > len) {
	        break;
	    }

	    printer(arg, " <");
	    len -= olen;

	    if (opt >= 1 && opt <= sizeof(cbcp_optionnames) / sizeof(char *))
	    	printer(arg, " %s", cbcp_optionnames[opt-1]);
	    else
	        printer(arg, " option=0x%x", opt); 

	    if (olen > 2) {
	        GETCHAR(delay, p);
		printer(arg, " delay = %d", delay);
	    }

	    if (olen > 3) {
	        int addrt;
		char str[256];

		GETCHAR(addrt, p);
		memcpy(str, p, olen - 4);
		str[olen - 4] = 0;
		printer(arg, " number = %s", str);
	    }
	    printer(arg, ">");
	break;
	}

      default:
	break;
    }

    for (; len > 0; --len) {
	GETCHAR(code, p);
	printer(arg, " %.2x", code);
    }

    return p - pstart;
}

/* received CBCP request */

void cbcp_recvreq(cbcp_state *us, char *pckt, int pcktlen)
{
    u_char type, opt_len, delay, addr_type;
    char address[256];
    int len = pcktlen;

    address[0] = 0;

    while (len) {
        syslog(LOG_DEBUG, "length: %d", len);

	GETCHAR(type, pckt);
	GETCHAR(opt_len, pckt);

	if (opt_len > 2)
	    GETCHAR(delay, pckt);

	us->us_allowed |= (1 << type);

	switch(type) {
	case CB_CONF_NO:
	    syslog(LOG_DEBUG, "no callback allowed");
	    break;

      case CB_CONF_USER:
	    syslog(LOG_DEBUG, "user callback allowed");
	    if (opt_len > 4) {
	        GETCHAR(addr_type, pckt);
		memcpy(address, pckt, opt_len - 4);
		address[opt_len - 4] = 0;
		if (address[0])
		    syslog(LOG_DEBUG, "address: %s", address);
	    }
	break;

      case CB_CONF_ADMIN:
	    syslog(LOG_DEBUG, "user admin defined allowed");
	    break;

      case CB_CONF_LIST:
	    break;
	}
	len -= opt_len;
    }

  cbcp_resp(us);
}

void cbcp_resp(cbcp_state *us)
{
	u_char cb_type;
	u_char buf[256];
	u_char *bufp = buf;
	int len = 0;
	struct callback_opts *cbopt;

	cbopt = &lcp_wantoptions[ lns[us->us_unit].lcp_unit ].cbopt;
	
	/* Always allow "no callback" and admin defined callback */
	us->us_type |= (1 << CB_CONF_NO);
	us->us_type |= (1 << CB_CONF_ADMIN);

	/* Only go for user defined or choode from a list callback if
	   we do have a phone number to be called back at */
	if (cbopt->message && cbopt->mlen) {
	  us->us_type |= (1 << CB_CONF_USER);
	  us->us_type |= (1 << CB_CONF_LIST);
	}
	cb_type = us->us_allowed & us->us_type;
	syslog(LOG_DEBUG, "cbcp_resp: cb_type=%d", cb_type);

	if (!cb_type) {
		syslog(LOG_INFO, "Your remote side wanted a callback-type you don't allow -> doing no callback");
        cb_type = 1 << CB_CONF_NO;
	}

	if (cb_type & ( 1 << CB_CONF_USER ) ) {
		syslog(LOG_DEBUG, "cbcp_resp CONF_USER");
		PUTCHAR(CB_CONF_USER, bufp);
		len = 3 + 1 + cbopt->mlen + 1;
		PUTCHAR(len , bufp);
		PUTCHAR(cbopt->delay, bufp); /* delay */
		PUTCHAR(1, bufp); /* Message type. Always 1
				     according to the protocol specs,
				     but you never know with MS
				     protocols ;-/ */
		BCOPY(cbopt->message, bufp, cbopt->mlen + 1);
		cbcp_send(us, CBCP_RESP, buf, len);
		return;
	}

        /* XXX: Callback to one number from a server defined list not yet
	   implemented */

	if (cb_type & ( 1 << CB_CONF_ADMIN ) ) {
		PUTCHAR(CB_CONF_ADMIN, bufp);
		len = 3;
		PUTCHAR(len , bufp);
		PUTCHAR(cbopt->delay, bufp);
		cbcp_send(us, CBCP_RESP, buf, len);
		return;
	}

	if (cb_type & ( 1 << CB_CONF_NO ) ) {
		syslog(LOG_DEBUG, "cbcp_resp CONF_NO");
		PUTCHAR(CB_CONF_NO, bufp);
		len = 2;
		PUTCHAR(len , bufp);
		cbcp_send(us, CBCP_RESP, buf, len);
#if 0
/*
 * what should we do here ... check this! */
 */
	ipcp_open( lns[us->us_unit].ipcp_unit );
#endif
		return;
	}
}

void cbcp_send(cbcp_state *us, u_char code, u_char *buf, int len)
{
    u_char *outp;
    int outlen;

    outp = outpacket_buf;

    outlen = 4 + len;
    
    MAKEHEADER(outp, PPP_CBCP);

    PUTCHAR(code, outp);
    PUTCHAR(us->us_id, outp);
    PUTSHORT(outlen, outp);
    
    if (len)
        BCOPY(buf, outp, len);

    output_ppp(us->us_unit, outpacket_buf, outlen + PPP_HDRLEN);
}

void cbcp_recvack(cbcp_state *us, char *pckt, int len)
{
    u_char type, delay, addr_type;
    int opt_len;
    char address[256];

    if (len) {
        GETCHAR(type, pckt);
	GETCHAR(opt_len, pckt);
     
	if (opt_len > 2)
	    GETCHAR(delay, pckt);

	if (opt_len > 4) {
	    GETCHAR(addr_type, pckt);
	    memcpy(address, pckt, opt_len - 4);
	    address[opt_len - 4] = 0;
	    if (address[0])
	        syslog(LOG_DEBUG, "peer will call: %s", address);
	}
    }

    cbcp_up(us);
}

/* ok peer will do callback */
void cbcp_up(cbcp_state *us)
{
	int linkunit = us->us_unit;
    lcp_close( lns[linkunit].lcp_unit ,"callback initiated sucessfully");
	lns[linkunit].phase = PHASE_TERMINATE;
}



