/*
 * upap.c - User/Password Authentication Protocol.
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
 */

char upap_rcsid[] = "$Id: upap.c,v 1.6 1999/11/10 08:01:33 werner Exp $";

/*
 * TODO:
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <syslog.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "fsm.h"
#include "ipppd.h"
#include "ipcp.h"
#include "upap.h"
#include "pathnames.h"

extern int log_raw_password;
/*
 * Protocol entry points.
 */
static void upap_init __P((int));
static void upap_lowerup __P((int));
static void upap_lowerdown __P((int));
static void upap_input __P((int, u_char *, int));
static void upap_protrej __P((int));
static int  upap_printpkt __P((u_char *, int,
                   void (*) __P((void *, char *, ...)), void *));

struct protent pap_protent = {
    PPP_PAP,
    upap_init,
    upap_input,
    upap_protrej,
    upap_lowerup,
    upap_lowerdown,
    NULL,
    NULL,
    upap_printpkt,
    NULL,
    1,
    "PAP",
    NULL,
    NULL,
    NULL
};


upap_state upap[NUM_PPP];		/* UPAP state; one for each unit */


static void upap_timeout __P((caddr_t));
static void upap_reqtimeout __P((caddr_t));
static void upap_rauthreq __P((upap_state *, u_char *, int, int));
static void upap_rauthack __P((upap_state *, u_char *, int, int));
static void upap_rauthnak __P((upap_state *, u_char *, int, int));
static void upap_sauthreq __P((upap_state *));
static void upap_sresp __P((upap_state *, int, int, char *, int));

void set_userip(char *ruser,int ruserlen)
{
	char line[1024];
	char name[64];
	FILE *fd;
	int base;
	int ofs;
	char *lname;
	char *lip;
	ipcp_options *wo = &ipcp_wantoptions[0];

	if (ruserlen) {
		memcpy(name,ruser, ruserlen);
		name[ruserlen]='\0';
	}
	else
		strncpy(name, ruser, sizeof(name)-1);
	fd = fopen(_PATH_USERIPTAB, "r");
	if (fd) {
		while (fgets(line, sizeof(line)-1, fd)) {
			base = strspn(line, " \t\n\r");
			if (line[base] == '#')
				continue;
			lname = &(line[base]);
			ofs = strcspn(lname, " \n\t\r\n");
			lname[ofs] = '\0';
			lip = &(lname[ofs+1]);
			base = strspn(lip, " \t\n\r");
			lip = &(lip[base]);
			ofs = strcspn(lip, " \n\t\r\n");
			lip[ofs] = '\0';
			if (!strcmp(name, lname)) {
				unsigned long int ip;
				if (inet_aton(lip, (struct in_addr *)&ip)) {
					wo->hisaddr = ip;
					UPAPDEBUG((LOG_INFO, "set_userip: found users(%s) ip(%s).",lname, lip)); 
				}
				break;
			}
		}
		fclose(fd);
	}
}

/*
 * upap_init - Initialize a UPAP unit.
 */
void upap_init(int unit)
{
    upap_state *u = &upap[unit];

    u->us_unit = unit;
    u->us_ruser[0] = 0;
    u->us_ruserlen = 0;
    u->us_rpasswd[0] = 0;
    u->us_rpasswdlen = 0;
    u->us_user = NULL;
    u->us_userlen = 0;
    u->us_passwd = NULL;
    u->us_passwdlen = 0;
    u->us_clientstate = UPAPCS_INITIAL;
    u->us_serverstate = UPAPSS_INITIAL;
    u->us_id = 0;
    u->us_timeouttime = UPAP_DEFTIMEOUT;
    u->us_maxtransmits = 10;
    u->us_reqtimeout = UPAP_DEFREQTIME;
}


/*
 * upap_authwithpeer - Authenticate us with our peer (start client).
 *
 * Set new state and send authenticate's.
 */
void upap_authwithpeer(int unit,char *user,char *password)
{
    upap_state *u = &upap[unit];

    /* Save the username and password we're given */
    u->us_user = user;
    u->us_userlen = strlen(user);
    u->us_passwd = password;
    u->us_passwdlen = strlen(password);
    u->us_transmits = 0;

    /* Lower layer up yet? */
    if (u->us_clientstate == UPAPCS_INITIAL ||
	u->us_clientstate == UPAPCS_PENDING) {
	u->us_clientstate = UPAPCS_PENDING;
	return;
    }

    upap_sauthreq(u);			/* Start protocol */
}


/*
 * upap_authpeer - Authenticate our peer (start server).
 *
 * Set new state.
 */
void upap_authpeer(int unit)
{
    upap_state *u = &upap[unit];

    /* Lower layer up yet? */
    if (u->us_serverstate == UPAPSS_INITIAL ||
	u->us_serverstate == UPAPSS_PENDING) {
	u->us_serverstate = UPAPSS_PENDING;
	return;
    }

    u->us_serverstate = UPAPSS_LISTEN;
    if (u->us_reqtimeout > 0)
	TIMEOUT(upap_reqtimeout, (caddr_t) u, u->us_reqtimeout);
}


/*
 * upap_timeout - Retransmission timer for sending auth-reqs expired.
 */
static void upap_timeout(caddr_t arg)
{
    upap_state *u = (upap_state *) arg;

    if (u->us_clientstate != UPAPCS_AUTHREQ)
	return;

    if (u->us_transmits >= u->us_maxtransmits) {
	/* give up in disgust */
	syslog(LOG_ERR, "No response to PAP authenticate-requests");
	u->us_clientstate = UPAPCS_BADAUTH;
	auth_withpeer_fail(u->us_unit, PPP_PAP, AUTH_ERR_TIME | AUTH_ERR_PAP);
	return;
    }

    upap_sauthreq(u);		/* Send Authenticate-Request */
}


/*
 * upap_reqtimeout - Give up waiting for the peer to send an auth-req.
 */
static void
upap_reqtimeout(arg)
    caddr_t arg;
{
    upap_state *u = (upap_state *) arg;

    if (u->us_serverstate != UPAPSS_LISTEN)
	return;			/* huh?? */

    auth_peer_fail(u->us_unit, PPP_PAP, AUTH_ERR_TIME | AUTH_ERR_PAP);
    u->us_serverstate = UPAPSS_BADAUTH;
}


/*
 * upap_lowerup - The lower layer is up.
 *
 * Start authenticating if pending.
 */
void upap_lowerup(int unit)
{
    upap_state *u = &upap[unit];

    if (u->us_clientstate == UPAPCS_INITIAL)
	u->us_clientstate = UPAPCS_CLOSED;
    else if (u->us_clientstate == UPAPCS_PENDING) {
	upap_sauthreq(u);	/* send an auth-request */
    }

    if (u->us_serverstate == UPAPSS_INITIAL)
	u->us_serverstate = UPAPSS_CLOSED;
    else if (u->us_serverstate == UPAPSS_PENDING) {
	u->us_serverstate = UPAPSS_LISTEN;
	if (u->us_reqtimeout > 0)
	    TIMEOUT(upap_reqtimeout, (caddr_t) u, u->us_reqtimeout);
    }
}


/*
 * upap_lowerdown - The lower layer is down.
 *
 * Cancel all timeouts.
 */
void upap_lowerdown(int unit)
{
    upap_state *u = &upap[unit];

    if (u->us_clientstate == UPAPCS_AUTHREQ)	/* Timeout pending? */
	UNTIMEOUT(upap_timeout, (caddr_t) u);	/* Cancel timeout */
    if (u->us_serverstate == UPAPSS_LISTEN && u->us_reqtimeout > 0)
	UNTIMEOUT(upap_reqtimeout, (caddr_t) u);

    u->us_clientstate = UPAPCS_INITIAL;
    u->us_serverstate = UPAPSS_INITIAL;
}


/*
 * upap_protrej - Peer doesn't speak this protocol.
 *
 * This shouldn't happen.  In any case, pretend lower layer went down.
 */
void upap_protrej(int linkunit)
{
  int unit = lns[linkunit].upap_unit;
    upap_state *u = &upap[unit]; /* link unit! */

    if (u->us_clientstate == UPAPCS_AUTHREQ) {
	syslog(LOG_ERR, "PAP authentication failed due to protocol-reject");
	auth_withpeer_fail(u->us_unit, PPP_PAP, AUTH_ERR_PROT | AUTH_ERR_PAP);
    }
    if (u->us_serverstate == UPAPSS_LISTEN) {
	syslog(LOG_ERR, "PAP authentication of peer failed (protocol-reject)");
	auth_peer_fail(u->us_unit, PPP_PAP, AUTH_ERR_PROT | AUTH_ERR_PAP);
    }
    upap_lowerdown(unit);
}


/*
 * upap_input - Input UPAP packet.
 */
void upap_input(int linkunit,u_char *inpacket,int l)
{
  int unit = lns[linkunit].upap_unit;
    upap_state *u = &upap[unit];
    u_char *inp;
    u_char code, id;
    int len;

    /*
     * Parse header (code, id and length).
     * If packet too short, drop it.
     */
    inp = inpacket;
    if (l < UPAP_HEADERLEN) {
	UPAPDEBUG((LOG_INFO, "upap_input: rcvd short header."));
	return;
    }
    GETCHAR(code, inp);
    GETCHAR(id, inp);
    GETSHORT(len, inp);
    if (len < UPAP_HEADERLEN) {
	UPAPDEBUG((LOG_INFO, "upap_input: rcvd illegal length."));
	return;
    }
    if (len > l) {
	UPAPDEBUG((LOG_INFO, "upap_input: rcvd short packet."));
	return;
    }
    len -= UPAP_HEADERLEN;

    /*
     * Action depends on code.
     */
    switch (code) {
    case UPAP_AUTHREQ:
	upap_rauthreq(u, inp, id, len);
	break;

    case UPAP_AUTHACK:
	upap_rauthack(u, inp, id, len);
	break;

    case UPAP_AUTHNAK:
	upap_rauthnak(u, inp, id, len);
	break;

    default:				/* XXX Need code reject */
	break;
    }
}


/*
 * upap_rauth - Receive Authenticate.
 */
static void upap_rauthreq(upap_state *u,u_char *inp,int id,int len)
{
	u_char ruserlen, rpasswdlen;
	char *ruser, *rpasswd;
	int retcode;
	char *msg;
	int msglen;

	UPAPDEBUG((LOG_INFO, "upap_rauth: Rcvd id %d.", id));

	if (u->us_serverstate < UPAPSS_LISTEN)
		return;

	/*
	 * If we receive a duplicate authenticate-request, we are
	 * supposed to return the same status as for the first request.
	 */
	if (u->us_serverstate == UPAPSS_OPEN) {
		upap_sresp(u, UPAP_AUTHACK, id, "", 0);	/* return auth-ack */
		return;
	}
	if (u->us_serverstate == UPAPSS_BADAUTH) {
		upap_sresp(u, UPAP_AUTHNAK, id, "", 0);	/* return auth-nak */
		return;
	}

	/*
	 * Parse user/passwd.
	 */
	if (len < sizeof (u_char)) {
		UPAPDEBUG((LOG_INFO, "upap_rauth: rcvd short packet."));
		return;
	}
	GETCHAR(ruserlen, inp);
	len -= sizeof (u_char) + ruserlen + sizeof (u_char);
	if (len < 0) {
		UPAPDEBUG((LOG_INFO, "upap_rauth: rcvd short packet."));
		return;
	}
	ruser = (char *) inp;
	INCPTR(ruserlen, inp);
	GETCHAR(rpasswdlen, inp);
	if (len < rpasswdlen) {
		UPAPDEBUG((LOG_INFO, "upap_rauth: rcvd short packet."));
		return;
	}
	rpasswd = (char *) inp;

	/*
	 * Check the username and password given.
	 */
	if(ruserlen > 64)
		ruserlen = 64;
	if(rpasswdlen > 64)
		rpasswdlen = 64;
	strncpy(u->us_ruser,ruser,(int)ruserlen);
	strncpy(u->us_rpasswd,rpasswd,(int)rpasswdlen);
	u->us_rpasswdlen = rpasswdlen;
	u->us_ruserlen = ruserlen;
#ifdef RADIUS
        retcode = radius_check_passwd(u->us_unit, ruser, ruserlen, rpasswd,
                       rpasswdlen, &msg, &msglen);
#else
	retcode = check_passwd(u->us_unit, ruser, ruserlen, rpasswd,
		rpasswdlen, &msg, &msglen);
#endif
	upap_sresp(u, retcode, id, msg, msglen);

	if (retcode == UPAP_AUTHACK) {
		set_userip(ruser, ruserlen);
		u->us_serverstate = UPAPSS_OPEN;
		auth_peer_success(u->us_unit, PPP_PAP);
	} else {
		u->us_serverstate = UPAPSS_BADAUTH;
		auth_peer_fail(u->us_unit, PPP_PAP, AUTH_ERR_USER | AUTH_ERR_PAP);
	}

	if (u->us_reqtimeout > 0)
		UNTIMEOUT(upap_reqtimeout, (caddr_t) u);
}


/*
 * upap_rauthack - Receive Authenticate-Ack.
 */
static void
upap_rauthack(u, inp, id, len)
    upap_state *u;
    u_char *inp;
    int id;
    int len;
{
    u_char msglen;
    char *msg;

    UPAPDEBUG((LOG_INFO, "upap_rauthack: Rcvd id %d.", id));
    if (u->us_clientstate != UPAPCS_AUTHREQ) /* XXX */
	return;

    /*
     * Parse message.
     */
    if (len < sizeof (u_char)) {
	UPAPDEBUG((LOG_INFO, "upap_rauthack: rcvd short packet."));
	return;
    }
    GETCHAR(msglen, inp);
    len -= sizeof (u_char);
    if (len < msglen) {
	UPAPDEBUG((LOG_INFO, "upap_rauthack: rcvd short packet."));
	return;
    }
    msg = (char *) inp;
    PRINTMSG(msg, msglen);

    u->us_clientstate = UPAPCS_OPEN;

    auth_withpeer_success(u->us_unit, PPP_PAP);
}


/*
 * upap_rauthnak - Receive Authenticate-Nakk.
 */
static void
upap_rauthnak(u, inp, id, len)
    upap_state *u;
    u_char *inp;
    int id;
    int len;
{
    u_char msglen;
    char *msg;

    UPAPDEBUG((LOG_INFO, "upap_rauthnak: Rcvd id %d.", id));
    if (u->us_clientstate != UPAPCS_AUTHREQ) /* XXX */
	return;

    /*
     * Parse message.
     */
    if (len < sizeof (u_char)) {
	UPAPDEBUG((LOG_INFO, "upap_rauthnak: rcvd short packet."));
	return;
    }
    GETCHAR(msglen, inp);
    len -= sizeof (u_char);
    if (len < msglen) {
	UPAPDEBUG((LOG_INFO, "upap_rauthnak: rcvd short packet."));
	return;
    }
    msg = (char *) inp;
    PRINTMSG(msg, msglen);

    u->us_clientstate = UPAPCS_BADAUTH;

    syslog(LOG_ERR, "PAP authentication failed");
    auth_withpeer_fail(u->us_unit, PPP_PAP, AUTH_ERR_USER | AUTH_ERR_PAP);
}


/*
 * upap_sauthreq - Send an Authenticate-Request.
 */
static void
upap_sauthreq(u)
    upap_state *u;
{
    u_char *outp;
    int outlen;

    outlen = UPAP_HEADERLEN + 2 * sizeof (u_char) +
	u->us_userlen + u->us_passwdlen;
    outp = outpacket_buf;
    
    MAKEHEADER(outp, PPP_PAP);

    PUTCHAR(UPAP_AUTHREQ, outp);
    PUTCHAR(++u->us_id, outp);
    PUTSHORT(outlen, outp);
    PUTCHAR(u->us_userlen, outp);
    BCOPY(u->us_user, outp, u->us_userlen);
    INCPTR(u->us_userlen, outp);
    PUTCHAR(u->us_passwdlen, outp);
    BCOPY(u->us_passwd, outp, u->us_passwdlen);

    output_ppp(u->us_unit, outpacket_buf, outlen + PPP_HDRLEN);

    UPAPDEBUG((LOG_INFO, "upap_sauth: Sent id %d.", u->us_id));

    TIMEOUT(upap_timeout, (caddr_t) u, u->us_timeouttime);
    ++u->us_transmits;
    u->us_clientstate = UPAPCS_AUTHREQ;
}


/*
 * upap_sresp - Send a response (ack or nak).
 */
static void
upap_sresp(u, code, id, msg, msglen)
    upap_state *u;
    u_char code, id;
    char *msg;
    int msglen;
{
    u_char *outp;
    int outlen;

    outlen = UPAP_HEADERLEN + sizeof (u_char) + msglen;
    outp = outpacket_buf;
    MAKEHEADER(outp, PPP_PAP);

    PUTCHAR(code, outp);
    PUTCHAR(id, outp);
    PUTSHORT(outlen, outp);
    PUTCHAR(msglen, outp);
    BCOPY(msg, outp, msglen);
    output_ppp(u->us_unit, outpacket_buf, outlen + PPP_HDRLEN);

    UPAPDEBUG((LOG_INFO, "upap_sresp: Sent code %d, id %d.", code, id));
}

/*
 * upap_printpkt - print the contents of a PAP packet.
 */
char *upap_codenames[] = {
    "AuthReq", "AuthAck", "AuthNak"
};

int
upap_printpkt(p, plen, printer, arg)
    u_char *p;
    int plen;
    void (*printer) __P((void *, char *, ...));
    void *arg;
{
    int code, id, len;
    int mlen, ulen, wlen;
    char *user, *pwd, *msg;
    u_char *pstart;

    if (plen < UPAP_HEADERLEN)
	return 0;
    pstart = p;
    GETCHAR(code, p);
    GETCHAR(id, p);
    GETSHORT(len, p);
    if (len < UPAP_HEADERLEN || len > plen)
	return 0;

    if (code >= 1 && code <= sizeof(upap_codenames) / sizeof(char *))
	printer(arg, " %s", upap_codenames[code-1]);
    else
	printer(arg, " code=0x%x", code);
    printer(arg, " id=0x%x", id);
    len -= UPAP_HEADERLEN;
    switch (code) {
    case UPAP_AUTHREQ:
	if (len < 1)
	    break;
	ulen = p[0];
	if (len < ulen + 2)
	    break;
	wlen = p[ulen + 1];
	if (len < ulen + wlen + 2)
	    break;
	user = (char *) (p + 1);
	pwd = (char *) (p + ulen + 2);
	p += ulen + wlen + 2;
	len -= ulen + wlen + 2;
	printer(arg, " user=");
	print_string(user, ulen, printer, arg);
    if(log_raw_password) {
	  printer(arg, " password=");
	  print_string(pwd, wlen, printer, arg);
    }
    else
      printer(arg, " password not logged for security reasons! Use '+pwlog' option to enable full logging.");
	break;
    case UPAP_AUTHACK:
    case UPAP_AUTHNAK:
	if (len < 1)
	    break;
	mlen = p[0];
	if (len < mlen + 1)
	    break;
	msg = (char *) (p + 1);
	p += mlen + 1;
	len -= mlen + 1;
	printer(arg, "msg=");
	print_string(msg, mlen, printer, arg);
	break;
    }

    /* print the rest of the bytes in the packet */
    for (; len > 0; --len) {
	GETCHAR(code, p);
	printer(arg, " %.2x", code);
    }

    return p - pstart;
}
