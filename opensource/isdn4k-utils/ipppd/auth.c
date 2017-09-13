/*
 * auth.c - PPP authentication and phase control.
 *
 * Fairly patched version for isdn4linux
 * copyright (c) 1995,1996,1997 of all patches by Michael Hipp
 * still no warranties (see disclaimer)
 *
 * Copyright (c) 1993 The Australian National University.
 * All rights reserved.
 *
 * 2000-07-25 Callback improvements by richard.kunze@web.de 
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the Australian National University.  The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
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

char auth_rcsid[] = "$Id: auth.c,v 1.21 2002/07/06 00:34:08 keil Exp $";

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <syslog.h>
#include <pwd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#if defined __GLIBC__ && __GLIBC__ >= 2
# include <crypt.h>
#endif

#include "config.h"
#ifdef HAVE_SHADOW_H
#include <shadow.h>
#endif

#ifdef RADIUS
#include <radiusclient.h>
int  radius_pap_auth __P((int, char *, char *, char **, int *));
extern int auth_order ;
extern int idle_time_limit ;
extern int default_idle_time_limit ;
extern int session_time_limit ;
extern int default_session_time_limit ;
#endif

#include "fsm.h"
#include "ipppd.h"
#include "lcp.h"
#include "upap.h"
#include "chap.h"
#include "cbcp.h"
#include "ipcp.h"
#include "ipxcp.h"

#include "ccp.h"
#include "pathnames.h"

#if defined(sun) && defined(sparc)
#include <alloca.h>
#endif /*sparc*/

/* Bits in scan_authfile return value */
#define NONWILD_SERVER        1
#define NONWILD_CLIENT        2

#define ISWILD(word)        (word[0] == '*' && word[1] == 0)

#define FALSE        0
#define TRUE        1

/* Bits in auth_pending[] */
#define UPAP_WITHPEER    1
#define UPAP_PEER        2
#define CHAP_WITHPEER    4
#define CHAP_PEER        8

/* Prototypes */
void check_access __P((FILE *, char *));

static void network_phase __P((int));
static void callback_phase __P((int));
static int  link_lastlink(int);
static int  check_login __P((char *, char *, char **, int *,int));
static void do_logout __P((int));
static int  null_login __P((int));
static int  get_upap_passwd __P((void));
static int  have_upap_secret __P((void));
static int  have_chap_secret __P((char *, char *));
static int  scan_authfile __P((FILE *, char *, char *, char *,
	  struct wordlist **, char *));
static void free_wordlist __P((struct wordlist *));
static void auth_script __P((int,char *,int));

/*
 * An Open on LCP has requested a change from Dead to Establish phase.
 * Do what's necessary to bring the physical layer up.
 */
void link_required(int unit)
{
}

/*
 * LCP has terminated the link; go to the Dead phase and take the
 * physical layer down.
 */
void link_terminated(int linkunit)
{

#ifdef RADIUS
	extern u_int32_t	default_hisaddr;
	extern u_int32_t	default_ouraddr;	
	extern u_int32_t	default_netmask;
	struct in_addr hisifaddr;
	struct in_addr ourifaddr;
#endif
	
	if(lns[linkunit].auth_up_script)
		auth_script(linkunit,_PATH_AUTHDOWN,0);
	if (lns[linkunit].phase == PHASE_DEAD)
		return;
	if (lns[linkunit].logged_in) {
		do_logout(linkunit);
		lns[linkunit].logged_in = 0;
	}
#ifdef RADIUS
	if (lns[linkunit].radius_in )
	{
		lns[linkunit].radius_in = FALSE ;
		if ( useradacct ) 
			radius_acct_stop(linkunit) ;
	}
	/* reset some parameters to their default (option-parsed) values */
	session_time_limit = default_session_time_limit ;
	idle_time_limit = default_idle_time_limit ;
	/* and reset IP addresses to their default values */
	hisifaddr.s_addr = default_hisaddr ? default_hisaddr : ipcp_wantoptions[linkunit].hisaddr ;
	ourifaddr.s_addr = default_ouraddr ? default_ouraddr : ipcp_wantoptions[linkunit].ouraddr ;
	syslog(LOG_DEBUG,
		"reseting remote ippp interface addresses for linkunit %d [ifunit %d] to %s:%s\n",
				linkunit,
				lns[linkunit].ifunit,
				inet_ntoa(ourifaddr),
				inet_ntoa(hisifaddr)
		);
	sifaddr(linkunit,
		default_ouraddr ? default_ouraddr :ipcp_wantoptions[linkunit].ouraddr,
		default_hisaddr ? default_hisaddr : ipcp_wantoptions[linkunit].hisaddr,
		default_netmask ? default_netmask : 0xffffffff);
#endif
	lns[linkunit].phase = PHASE_DEAD;
	syslog(LOG_NOTICE, "Connection terminated.");
}

/*
 * LCP has gone down; it will either die or try to re-establish.
 */
void link_down(int unit)
{
	if(lns[unit].ccp_l_unit >= 0) {
		(*ccp_link_protent.lowerdown)(lns[unit].ccp_l_unit);
		(*ccp_link_protent.close)(lns[unit].ccp_l_unit,"link went down");
		ccp_freeunit(lns[unit].ccp_l_unit);
		lns[unit].ccp_l_unit = -1;
	}
	if(link_lastlink(unit)) {
		if(lns[unit].ccp_unit >= 0) {
			(*ccp_protent.lowerdown)(lns[unit].ccp_unit);
			(*ccp_protent.close)(lns[unit].ccp_unit,"link went down");
			ccp_freeunit(lns[unit].ccp_unit);
			lns[unit].ccp_unit = -1;
		}
		if(lns[unit].ipcp_unit >= 0) {
			(*ipcp_protent.lowerdown)(lns[unit].ipcp_unit);
			(*ipcp_protent.close)(lns[unit].ipcp_unit,"link went down");
			ipcp_freeunit(lns[unit].ipcp_unit);
			lns[unit].ipcp_unit = -1;
		}
		if(lns[unit].cbcp_unit >= 0) {
			(*cbcp_protent.lowerdown)(lns[unit].cbcp_unit);
			(*cbcp_protent.close)(lns[unit].cbcp_unit,"link went down");
			lns[unit].cbcp_unit = -1;
		}
		if(lns[unit].ipxcp_unit >= 0)
		{
	  		(*ipxcp_protent.lowerdown)(lns[unit].ipxcp_unit);
	  		(*ipxcp_protent.close)(lns[unit].ipxcp_unit,"link went down");
	  		ipxcp_freeunit(lns[unit].ipxcp_unit);
	 	 	lns[unit].ipxcp_unit = -1;      
		}
	}
	else {
		struct link_struct *q;
		int i; /* bugcheck, stop after 1024 links */
		for(i=1024,q=lns[unit].bundle_next;i && q!=&lns[unit];q=q->bundle_next,i--) {
			if(q->bundle_next == &lns[unit])
			break;
		}
		if(!i || q == &lns[unit]) {
			syslog(LOG_ERR,"Error on 'unbundling' link");
		}
		else {
			/*
			 * maybe the dead link was our communication link -> set a valid 'fd'
			 */
			ipcp_fsm[lns[unit].ipcp_unit].unit = q->unit;
			ccp_fsm[lns[unit].ccp_unit].unit = q->unit; 
			ipxcp_fsm[lns[unit].ipxcp_unit].unit = q->unit;
			q->bundle_next = lns[unit].bundle_next;
			lns[unit].bundle_next = &lns[unit];
		}
	}
	lns[unit].phase = PHASE_TERMINATE;
}

/* 
 * check whether 'unit' is the last in the bundle
 */
static int link_lastlink(int unit)
{
	if(lns[unit].bundle_next == &lns[unit])
		return 1;
	else
		return 0;
}


/*
 * The link is established.
 * Proceed to the Dead, Authenticate or Network phase as appropriate.
 */
void link_established(int linkunit)
{
	int auth;
	lcp_options *wo = &lcp_wantoptions[ lns[linkunit].lcp_unit ];
	lcp_options *go = &lcp_gotoptions[ lns[linkunit].lcp_unit ];
	lcp_options *ho = &lcp_hisoptions[ lns[linkunit].lcp_unit ];
	
	if (auth_required && !(go->neg_chap || go->neg_upap)) {
		/*
		 * We wanted the peer to authenticate itself, and it refused:
		 * treat it as though it authenticated with PAP using a username
		 * of "" and a password of "".  If that's not OK, boot it out.
		 */
		if (!wo->neg_upap || !null_login(linkunit)) {
			syslog(LOG_WARNING, "peer refused to authenticate");
			lcp_close(lns[linkunit].lcp_unit,"peer refused to authenticate");
			lns[linkunit].phase = PHASE_TERMINATE;
			return;
		}
	}

	lns[linkunit].phase = PHASE_AUTHENTICATE;
	auth = 0;
	if (go->neg_chap) {
		ChapAuthPeer(lns[linkunit].chap_unit, our_name, go->chap_mdtype);
		auth |= CHAP_PEER;
	} else if (go->neg_upap) {
		upap_authpeer(lns[linkunit].upap_unit);
		auth |= UPAP_PEER;
	}
	if (ho->neg_chap) {
		ChapAuthWithPeer(lns[linkunit].chap_unit, our_name, ho->chap_mdtype);
		auth |= CHAP_WITHPEER;
	} else if (ho->neg_upap) {
		upap_authwithpeer(lns[linkunit].upap_unit, user, passwd);
		auth |= UPAP_WITHPEER;
	}
	lns[linkunit].auth_pending = auth;
	if (!auth)
		callback_phase(linkunit);
}

/*
 * Proceed to the network phase.
 */
static void network_phase(int linkunit)
{
  int lcp_unit = lns[linkunit].lcp_unit;
  lcp_options *go=&lcp_gotoptions[lcp_unit],*ho=&lcp_hisoptions[lcp_unit];
  int higher_up = 1;
  int discr,i;

  if(go->neg_mpmrru || go->neg_mpshortseq)
	go->neg_mp = 1;
  if(ho->neg_mpmrru || ho->neg_mpshortseq) 
	ho->neg_mp = 1;

  syslog(LOG_INFO,"MPPP negotiation, He: %s We: %s",go->neg_mp?"Yes":"No",ho->neg_mp?"Yes":"No");

  if(go->neg_mp && ho->neg_mp)
	for(i=0;i<numdev && higher_up;i++)
	{
	  if(i != linkunit && lns[i].phase == PHASE_NETWORK)
	  {
		if(lcp_gotoptions[i].neg_mp  && lcp_hisoptions[i].neg_mp)
		{
		  discr = lcp_hisoptions[i].neg_mpdiscr ? 1 : 0;
		  if(ho->neg_mpdiscr)
			discr++;

		  syslog(LOG_INFO,"ipppd[%d]: discr: %d\n",linkunit,discr);

		  switch(discr)            
		  {
			case 1: /* one peer has a discriminator, the other not */
			  break; 
			case 2:
			  /*
			   * maybe, we should check our side, too 
			   * (but currently we use always the same discriminator)
			   */
			  if(ho->mp_alen != lcp_hisoptions[i].mp_alen || ho->mp_class != lcp_hisoptions[i].mp_class)
				break;
			  syslog(LOG_INFO,"ipppd[%d]: passed 1\n",linkunit);
			  if(memcmp(ho->mp_addr,lcp_hisoptions[i].mp_addr,ho->mp_alen))
				break;
			  syslog(LOG_INFO,"ipppd[%d]: passed 2\n",linkunit);
			case 0:
			   /* 
				* login check when peer has authentification
				*/
			  if(go->neg_chap)
			  {
				syslog(LOG_INFO,"ipppd[%d]: chap-check: NOT IMPLEMENTED YET\n",linkunit);
			  }
			  else if(go->neg_upap)
			  {
				int l;
				syslog(LOG_INFO,"ipppd[%d]: pap-check\n",linkunit);
				if((l=upap[lns[linkunit].upap_unit].us_ruserlen) == upap[lns[i].upap_unit].us_ruserlen)
				  if(strncmp(upap[lns[linkunit].upap_unit].us_ruser,upap[lns[i].upap_unit].us_ruser,l))
				  {
					syslog(LOG_INFO,"Making no bundle because of different login names");
					break;
				  }
			  }
			  syslog(LOG_INFO,"ipppd[%d]: pap/chap-check passed\n",linkunit);
			  higher_up = 0;
			  /* 
			   * ok: found a valid bundle
			   */
			  syslog(LOG_INFO,"ok, found a valid bundle with linkunit %d",i);
			  lns[linkunit].bundle_next = lns[i].bundle_next;
			  lns[i].bundle_next = &lns[linkunit];
			  lns[linkunit].ifunit = lns[i].ifunit;
			  lns[linkunit].master = i;
			  lns[linkunit].ipcp_unit = lns[i].ipcp_unit; /* use fsm state of other link */
			  /* 
			   * same fsm state for ccp, too???
			   */
			  lns[linkunit].ccp_unit = lns[i].ccp_unit; 
			  lns[linkunit].ipxcp_unit = lns[i].ipxcp_unit;
			  lns[linkunit].phase = PHASE_NETWORK;
			  /* 
			   * bundle the links on driver layer
			   */
			  syslog(LOG_INFO,"bundle: %d",
				 sifbundle(linkunit,i)); /* sets MP Flags */
			  break;
		  }
		}
	  }
	}

  if(higher_up) /* no bundle -> kick higher layers */
  {
	if(go->neg_mp && ho->neg_mp)
	  enable_mp(linkunit,0x0);

		if(ipcp_protent.enabled_flag) {
				int ipcp_unit = lns[linkunit].ipcp_unit = ipcp_getunit(linkunit);
				if(ipcp_unit >= 0) {
						if(useifip)
								setifip(lns[linkunit].ipcp_unit);
						if(usefirstip && lns[linkunit].addresses) {
								ipcp_options *wo = &ipcp_wantoptions[ipcp_unit];
								struct hostent *hp = gethostbyname(lns[linkunit].addresses->word);
								if (hp != NULL && hp->h_addrtype == AF_INET) {
										wo->hisaddr = *(u_int32_t *)hp->h_addr;
								}
						}
						(*ipcp_protent.lowerup)(lns[linkunit].ipcp_unit);
						(*ipcp_protent.open)(lns[linkunit].ipcp_unit);
				}
		}
		if(ipxcp_protent.enabled_flag) {
				lns[linkunit].ipxcp_unit = ipxcp_getunit(linkunit);
				(*ipxcp_protent.lowerup)(lns[linkunit].ipxcp_unit);
				(*ipxcp_protent.open)(lns[linkunit].ipxcp_unit);
		}
		if(ccp_link_protent.enabled_flag) {
			syslog(LOG_NOTICE,"linkCCP enabled! Trying CCP.\n");
			lns[linkunit].ccp_unit = ccp_getunit(linkunit,PPP_LINK_CCP);
			(*ccp_link_protent.lowerup)(lns[linkunit].ccp_unit);
			(*ccp_link_protent.open)(lns[linkunit].ccp_unit);
		}
		if(ccp_protent.enabled_flag) {
			syslog(LOG_NOTICE,"CCP enabled! Trying CCP.\n");
			lns[linkunit].ccp_unit = ccp_getunit(linkunit,PPP_CCP);
			(*ccp_protent.lowerup)(lns[linkunit].ccp_unit);
			(*ccp_protent.open)(lns[linkunit].ccp_unit);
		}
	lns[linkunit].phase = PHASE_NETWORK;
  }

  if(ccp_link_protent.enabled_flag) {
    syslog(LOG_NOTICE,"linkCCP enabled! Trying CCP.\n");
    lns[linkunit].ccp_l_unit = ccp_getunit(linkunit,PPP_LINK_CCP);
    (*ccp_link_protent.lowerup)(lns[linkunit].ccp_l_unit);
    (*ccp_link_protent.open)(lns[linkunit].ccp_l_unit);
  }
}

/*
 * Proceed to the callback phase which may be empty.
 */
static void callback_phase(int linkunit)
{
  lcp_options *go = &lcp_gotoptions[ lns[linkunit].lcp_unit ];
  
  /* hack here: remote is always the server for callback */
  if (go->neg_callback && !(lns[linkunit].pci.calltype & CALLTYPE_INCOMING)) {
    /* Do CBCP if we did negotiate CBCP, take the lionk
       down and wait for callback if we negotiated RFC
       1570 style callback */
    if (go->cbopt.type == CB_CBCP) {
      lns[linkunit].phase = PHASE_CALLBACK;
      /* cbcp always corresponds to a link */
      lns[linkunit].cbcp_unit = linkunit; 
      cbcp[ lns[linkunit].cbcp_unit ].us_unit = linkunit;
      (*cbcp_protent.lowerup)(lns[linkunit].cbcp_unit);
      (*cbcp_protent.open)(lns[linkunit].cbcp_unit);
    } else {
	lns[linkunit].phase = PHASE_TERMINATE;
    }
  } else {
    network_phase(linkunit);
  }
}

/*
 * The peer has failed to authenticate himself using `protocol'.
 */
void auth_peer_fail(int unit,int protocol, int reason)
{
	/*
	 * Authentication failure: take the link down
	 */
	lcp_close(lns[unit].lcp_unit,"auth failure");
	lns[unit].phase = PHASE_TERMINATE;
	auth_script(unit, _PATH_AUTHFAIL, reason);
}

/*
 * The peer has been successfully authenticated using `protocol'.
 */
void auth_peer_success(int linkunit,int protocol)
{
	int bit;

		switch (protocol) {
				case PPP_CHAP:
						bit = CHAP_PEER;
						break;
				case PPP_PAP:
						bit = UPAP_PEER;
						break;
				default:
						syslog(LOG_WARNING, "auth_peer_success: unknown protocol %x",protocol);
						return;
	}

	/*
	 * If there is no more authentication still to be done,
	 * proceed to the network phase. (via callback phase)
	 */
	if ((lns[linkunit].auth_pending &= ~bit) == 0 &&
             lns[linkunit].phase == PHASE_AUTHENTICATE) {
		callback_phase(linkunit);
	}
}

/*
 * We have failed to authenticate ourselves to the peer using `protocol'.
 */
void auth_withpeer_fail(int unit,int protocol,int reason)
{
	/*
	 * We've failed to authenticate ourselves to our peer.
	 * He'll probably take the link down, and there's not much
	 * we can do except wait for that.
	 */
	auth_script(unit, _PATH_AUTHFAIL, reason);
}

/*
 * We have successfully authenticated ourselves with the peer using `protocol'.
 */
void auth_withpeer_success(int linkunit,int protocol)
{
	int bit;

	switch (protocol) {
		case PPP_CHAP:
			bit = CHAP_WITHPEER;
			break;
		case PPP_PAP:
			bit = UPAP_WITHPEER;
			break;
		default:
			syslog(LOG_WARNING, "auth_peer_success: unknown protocol %x",protocol);
			bit = 0;
	}

	/*
	 * If there is no more authentication still being done,
	 * proceed to the network phase.
	 */
	if ((lns[linkunit].auth_pending &= ~bit) == 0 && 
	     lns[linkunit].phase == PHASE_AUTHENTICATE)
			callback_phase(linkunit);
}

/*
 * check_auth_options - called to check authentication options.
 */
void check_auth_options()
{
	int i;

	lcp_options *wo = &lcp_wantoptions[0];
	lcp_options *ao = &lcp_allowoptions[0];

	/* Default our_name to hostname, and user to our_name */
	if (our_name[0] == 0 || usehostname)
		strcpy(our_name, hostname);
	if (user[0] == 0)
		strcpy(user, our_name);

	/* If authentication is required, ask peer for CHAP or PAP. */
	if (auth_required && !wo->neg_chap && !wo->neg_upap) {
		wo->neg_chap = 1;
		wo->neg_upap = 1;
	}

	/*
	 * Check whether we have appropriate secrets to use
	 * to authenticate ourselves and/or the peer.
	 */
#ifdef RADIUS
	if (ao->neg_upap && passwd[0] == 0 && !get_upap_passwd() 
		&& !useradius ) {
#else
	if (ao->neg_upap && passwd[0] == 0 && !get_upap_passwd()) {
#endif
		syslog(LOG_INFO,"info: no PAP secret entry for this user!\n");
		ao->neg_upap = 0;
	}
#ifdef RADIUS
	if (wo->neg_upap && !uselogin && !ask_passwd && !fdpasswd &&
		!have_upap_secret() && !useradius )
#else
	if (wo->neg_upap && !uselogin && !ask_passwd && !fdpasswd &&
		!have_upap_secret())
#endif
		wo->neg_upap = 0;
#ifdef RADIUS
	if (ao->neg_chap && !ask_passwd && !fdpasswd &&
		!have_chap_secret(our_name, remote_name) && !useradius ) {
#else
	if (ao->neg_chap && !ask_passwd && !fdpasswd &&
		!have_chap_secret(our_name, remote_name)) {
#endif
		syslog(LOG_INFO,"info: no CHAP secret entry for this user!\n");
		ao->neg_chap = 0;
	}
#ifdef RADIUS
	if (wo->neg_chap && !ask_passwd && !fdpasswd &&
		!have_chap_secret(remote_name, our_name) && !useradius )
#else
	if (wo->neg_chap && !ask_passwd && !fdpasswd &&
		!have_chap_secret(remote_name, our_name))
#endif
		wo->neg_chap = 0;

	if (auth_required && !wo->neg_chap && !wo->neg_upap) {
            fprintf(stderr,"ipppd: peer authentication required but no authentication files accessible\n or user %s not found in auth files",user);
		exit(1);
	}

	for(i=1;i<NUM_PPP;i++) /* duplicate options */
	{
	  lcp_wantoptions[i].neg_upap = lcp_wantoptions[0].neg_upap;
	  lcp_wantoptions[i].neg_chap = lcp_wantoptions[0].neg_chap;
	  lcp_allowoptions[i].neg_upap = lcp_allowoptions[0].neg_upap;
	  lcp_allowoptions[i].neg_chap = lcp_allowoptions[0].neg_chap;
	}

}

/*
 * reload PW
 */
void auth_reload_upap_pw(void)
{
  if(lcp_allowoptions[0].neg_upap)
    get_upap_passwd();
}

/*
 * check_passwd - Check the user name and passwd against the PAP secrets
 * file.  If requested, also check against the system password database,
 * and login the user if OK.
 *
 * returns:
 *        UPAP_AUTHNAK: Authentication failed.
 *        UPAP_AUTHACK: Authentication succeeded.
 * In either case, msg points to an appropriate message.
 */
int check_passwd(int linkunit,char *auser,int userlen,char *apasswd,
	int passwdlen,char **msg,int *msglen)
{
	int ret;
	char *filename;
	FILE *f;
	struct wordlist *addrs;
	char passwd[256], user[256];
	char secret[MAXWORDLEN];

	/*
	 * Make copies of apasswd and auser, then null-terminate them.
	 */
	BCOPY(apasswd, passwd, passwdlen);
	passwd[passwdlen] = '\0';
	BCOPY(auser, user, userlen);
	user[userlen] = '\0';

	syslog(LOG_INFO,"Check_passwd called with user=%s\n",user);

	snprintf(lns[linkunit].peer_authname,sizeof(lns[0].peer_authname), "%s", user);

	/*
	 * Open the file of upap secrets and scan for a suitable secret
	 * for authenticating this user.
	 */
	filename = _PATH_UPAPFILE;
	addrs = NULL;
	ret = UPAP_AUTHACK;
	f = fopen(filename, "r");
	if (f == NULL) {
		if (!uselogin ) {
			syslog(LOG_ERR, "Can't open PAP password file %s: %m", filename);
			ret = UPAP_AUTHNAK;
		}
	} else {
		check_access(f, filename);
		if (scan_authfile(f, user, our_name, secret, &addrs, filename) < 0
			|| (secret[0] != 0 && (cryptpap || strcmp(passwd, secret) != 0)
				&& strcmp(crypt(passwd, secret), secret) != 0)) {
			syslog(LOG_WARNING, "PAP authentication failure for %s", user);
			ret = UPAP_AUTHNAK;
		}
		fclose(f);
	}

	if (uselogin && ret == UPAP_AUTHACK) {
		ret = check_login(user, passwd, msg, msglen,linkunit);
		if (ret == UPAP_AUTHNAK) {
			syslog(LOG_WARNING, "PAP login failure for %s", user);
		}
		else
			lns[linkunit].logged_in = TRUE;
	}

	if (ret == UPAP_AUTHNAK) {
		*msg = "Login incorrect";
		*msglen = strlen(*msg);
		if (lns[linkunit].attempts++ >= 10) {
			syslog(LOG_WARNING, "%d LOGIN FAILURES ON %s, %s",
			lns[linkunit].attempts, lns[linkunit].devnam, user);
			lcp_close(lns[linkunit].lcp_unit,"max auth exceed");
			lns[linkunit].phase = PHASE_TERMINATE;
		}
#if 0
		if (attempts > 3)
			sleep((u_int) (attempts - 3) * 5);
#endif
		if (addrs != NULL)
			free_wordlist(addrs);
	} else {
		lns[linkunit].attempts = 0;                        /* Reset count */
		*msg = "Login ok";
		*msglen = strlen(*msg);
		if (lns[linkunit].addresses != NULL)
			free_wordlist(lns[linkunit].addresses);
		lns[linkunit].addresses = addrs;
		auth_script(linkunit,_PATH_AUTHUP,0);
		lns[linkunit].auth_up_script = 1;
	}
	return ret;
}

#ifdef RADIUS
/*
 * Name: radius_check_passwd 
 *
 * Purpose: Check the user name and passwd against RADIUS server
 * 	    and acording to configuration also perform standard check_passwd
 *
 * returns:
 *        UPAP_AUTHNAK: Authentication failed.
 *        UPAP_AUTHACK: Authentication succeeded.
 * In either case, msg points to an appropriate message.
 */
int radius_check_passwd(linkunit,auser,userlen,apasswd,passwdlen,msg,msglen)
	int linkunit ;
	char *auser ;
	int userlen ;
	char *apasswd ;
	int passwdlen ;
	char **msg ;
	int *msglen ;			
{
	int ret;
	char passwd[256], user[256];

	/*
	* Make copies of apasswd and auser, then null-terminate them.
	*/
	BCOPY(apasswd, passwd, passwdlen);
	passwd[passwdlen] = '\0';
	BCOPY(auser, user, userlen);
	user[userlen] = '\0';
	snprintf(lns[linkunit].peer_authname,sizeof(lns[0].peer_authname), "%s", user);

	ret = UPAP_AUTHACK;

	if ( useradius )
	{
		if ( auth_order & AUTH_RADIUS_FST )
		{
			ret = radius_pap_auth( linkunit, user, passwd, msg, msglen );
			if ((ret == UPAP_AUTHNAK) && (auth_order & AUTH_LOCAL_SND))
			{
				ret = check_passwd (linkunit,auser,userlen,apasswd,passwdlen,msg,msglen) ;
			}
		}
		else if ( auth_order & AUTH_LOCAL_FST )
		{
			ret = check_passwd (linkunit,auser,userlen,apasswd,passwdlen,msg,msglen) ;
			if ((auth_order & AUTH_RADIUS_SND) && (ret == UPAP_AUTHNAK))
			{
				ret = radius_pap_auth( linkunit, user, passwd, msg, msglen );
			}
		}
	} 
	else
	{
		ret = check_passwd (linkunit,auser,userlen,apasswd,passwdlen,msg,msglen) ;
	}

	if (ret == UPAP_AUTHNAK) 
	{
		*msg = "Login incorrect";
		*msglen = strlen(*msg);
		if (lns[linkunit].attempts++ >= 10)
		{
			syslog(LOG_WARNING, "%d LOGIN FAILURES ON %s, %s",
				lns[linkunit].attempts, 
				lns[linkunit].devnam, user);
				lcp_close(lns[linkunit].lcp_unit,
				"max auth exceed");
			lns[linkunit].phase = PHASE_TERMINATE;
/* should I die here it must be something wrong here then */
/* then outside script can reestablish ipppd 		  */
		}

	} 
	else 
	{
		lns[linkunit].attempts = 0;		/* Reset count */
		*msg = "Login ok";
		*msglen = strlen(*msg);
	}

	return ret;
}
#endif

/*
 * check_login - Check the user name and password against the system
 * password database, and login the user if OK.
 *
 * returns:
 *        UPAP_AUTHNAK: Login failed.
 *        UPAP_AUTHACK: Login succeeded.
 * In either case, msg points to an appropriate message.
 */
static int check_login(char *user,char *passwd,char **msg,int *msglen,int unit)
{
	struct passwd *pw;
	char *epasswd;
	char *tty;

#ifdef HAVE_SHADOW_H
	struct spwd *spwd;
	extern int isexpired (struct passwd *, struct spwd *);
#endif

	if ((pw = getpwnam(user)) == NULL) {
		return (UPAP_AUTHNAK);
	 }

#ifdef HAVE_SHADOW_H
	/* try to deal with shadow */
	spwd = getspnam(user);
	if (spwd) {
	   /* check the age of the password entry */
	   if (isexpired(pw, spwd)) {
		   syslog(LOG_WARNING,"Expired password for %s",user);
		   return (UPAP_AUTHNAK);
	   }
	   pw->pw_passwd = spwd->sp_pwdp;
	}
	endspent();
#endif

	 /*
	  * XXX If no passwd, let them login without one.
	  */
	if (!pw->pw_passwd || *pw->pw_passwd == '\0') {
		return (UPAP_AUTHACK);
	 }

	epasswd = crypt(passwd, pw->pw_passwd);
	if (strcmp(epasswd, pw->pw_passwd)) {
	   return (UPAP_AUTHNAK);
	}

	syslog(LOG_INFO, "user %s logged in", user);

	/*
	 * Write a wtmp entry for this user.
	 */
	tty = lns[unit].devnam;
	if (strncmp(tty, "/dev/", 5) == 0)
		tty += 5;
	logwtmputmp(unit, tty, user, "");                /* Add wtmp login entry */

	return (UPAP_AUTHACK);
}

/*
 * logout - Logout the user.
 */
static void do_logout(int unit)
{
	char *tty;

	tty = lns[unit].devnam;
	if (strncmp(tty, "/dev/", 5) == 0)
		tty += 5;
	logwtmputmp(unit, tty, "", "");                /* Wipe out wtmp logout entry */
}


/*
 * null_login - Check if a username of "" and a password of "" are
 * acceptable, and iff so, set the list of acceptable IP addresses
 * and return 1.
 */
static int null_login(int unit)
{
	char *filename;
	FILE *f;
	int i, ret;
	struct wordlist *addrs;
	char secret[MAXWORDLEN];

	/*
	 * Open the file of upap secrets and scan for a suitable secret.
	 * We don't accept a wildcard client.
	 */
	filename = _PATH_UPAPFILE;
	addrs = NULL;
	f = fopen(filename, "r");
	if (f == NULL)
		return 0;
	check_access(f, filename);

	i = scan_authfile(f, "", our_name, secret, &addrs, filename);
	ret = i >= 0 && (i & NONWILD_CLIENT) != 0 && secret[0] == 0;

	if (ret) {
		if (lns[unit].addresses != NULL)
			free_wordlist(lns[unit].addresses);
		lns[unit].addresses = addrs;
	}

	fclose(f);
	return ret;
}


/*
 * get_upap_passwd - get a password for authenticating ourselves with
 * our peer using PAP.  Returns 1 on success, 0 if no suitable password
 * could be found.
 */
static int get_upap_passwd(void)
{
	char *filename;
	FILE *f;
	struct wordlist *addrs;
	char secret[MAXWORDLEN];

	if ((ask_passwd || fdpasswd) && passwd[0])
		return 1;
	filename = _PATH_UPAPFILE;
	addrs = NULL;
	f = fopen(filename, "r");
	if (f == NULL)
		return 0;
	check_access(f, filename);
	if (scan_authfile(f, user, remote_name, secret, NULL, filename) < 0)
		return 0;
	strncpy(passwd, secret, MAXSECRETLEN);
	passwd[MAXSECRETLEN-1] = 0;
	return 1;
}


/*
 * have_upap_secret - check whether we have a PAP file with any
 * secrets that we could possibly use for authenticating the peer.
 */
static int have_upap_secret(void)
{
	FILE *f;
	int ret;
	char *filename;

	filename = _PATH_UPAPFILE;
	f = fopen(filename, "r");
	if (f == NULL)
		return 0;

	ret = scan_authfile(f, NULL, our_name, NULL, NULL, filename);
	fclose(f);
	if (ret < 0)
		return 0;

	return 1;
}


/*
 * have_chap_secret - check whether we have a CHAP file with a
 * secret that we could possibly use for authenticating `client'
 * on `server'.  Either can be the null string, meaning we don't
 * know the identity yet.
 */
static int have_chap_secret(char *client,char *server)
{
	FILE *f;
	int ret;
	char *filename;

	filename = _PATH_CHAPFILE;
	f = fopen(filename, "r");
	if (f == NULL)
		return 0;

	if (client[0] == 0)
		client = NULL;
	else if (server[0] == 0)
		server = NULL;

	ret = scan_authfile(f, client, server, NULL, NULL, filename);
	fclose(f);
	if (ret < 0)
		return 0;

	return 1;
}


/*
 * get_secret - open the CHAP secret file and return the secret
 * for authenticating the given client on the given server.
 * (We could be either client or server).
 */
int get_secret(unit, client, server, secret, secret_len, save_addrs)
	int unit;
	char *client;
	char *server;
	char *secret;
	int *secret_len;
	int save_addrs;
{
	FILE *f;
	int ret, len;
	char *filename;
	struct wordlist *addrs;
	char secbuf[MAXWORDLEN];

	if ((ask_passwd || fdpasswd) && passwd[0]) {
		len = strlen(passwd);
		BCOPY(passwd, secret, len);
		*secret_len = len;
		return 1;
	}
	filename = _PATH_CHAPFILE;
	addrs = NULL;
	secbuf[0] = 0;

	f = fopen(filename, "r");
	if (f == NULL) {
		syslog(LOG_ERR, "Can't open chap secret file %s: %m", filename);
		return 0;
	}
	check_access(f, filename);

	ret = scan_authfile(f, client, server, secbuf, &addrs, filename);
	fclose(f);
	if (ret < 0)
		return 0;

	if (save_addrs) {
		if (lns[unit].addresses != NULL)
			free_wordlist(lns[unit].addresses);
		lns[unit].addresses = addrs;
	}

	len = strlen(secbuf);
	if (len > MAXSECRETLEN) {
		syslog(LOG_ERR, "Secret for %s on %s is too long", client, server);
		len = MAXSECRETLEN;
	}
	BCOPY(secbuf, secret, len);
	*secret_len = len;

	return 1;
}

/*
 * auth_ip_addr - check whether the peer is authorized to use
 * a given IP address.  Returns 1 if authorized, 0 otherwise.
 */
int auth_ip_addr(int unit,u_int32_t addr)
{
	u_int32_t a;
	int       accept;
	u_int32_t mask;
	u_char    *ptr_word, *ptr_mask;
	struct hostent *hp;
	struct netent *np;
	struct wordlist *addrs;

	/* don't allow loopback or multicast address */
	if (bad_ip_adrs(addr))
		return 0;

	if ((addrs = lns[unit].addresses) == NULL)
		return 1;                /* no restriction */

	for (; addrs != NULL; addrs = addrs->next) {
		/* "-" means no addresses authorized */
		ptr_word = addrs->word;
		if (strcmp(ptr_word, "-") == 0)
			break;

		/* "*" means any addresses authorized */
		if (strcmp(ptr_word, "*") == 0)
			return 1;

		accept = 1;

		if (*ptr_word == '!') {
			accept = 0;
			++ptr_word;
		}

		ptr_mask = strchr (ptr_word, '/');
		if (ptr_mask == NULL)
			mask = 0xFFFFFFFFUL;
		else {
			int bit_count;
			*ptr_mask = '\0';
			bit_count = (int) strtol (ptr_mask, (char **) 0, 10);
			if (bit_count <= 0 || bit_count > 32) {
				syslog (LOG_WARNING,
						"invalid address length %s in auth. address list",
						ptr_mask);
				*ptr_mask = '/';
				 continue;
			}
			mask = ~((1UL << (32 - bit_count)) - 1UL);
		 }

		hp = gethostbyname(ptr_word);
		if (hp != NULL && hp->h_addrtype == AF_INET) {
			a    = *(u_int32_t *)hp->h_addr;
			mask = -1L;
		} else {
			np = getnetbyname (ptr_word);
			if (np != NULL && np->n_addrtype == AF_INET)
				a = htonl ((unsigned long)np->n_net);
			else
				a = inet_addr (ptr_word);
		}

		if (ptr_mask)
			*ptr_mask = '/';

		if (a == (u_int32_t)-1L)
			syslog (LOG_WARNING,
					"unknown host %s in auth. address list",
					addrs->word);
		else
			if (((addr ^ a) & mask) == 0)
				return accept;
	  }
	 return 0;                        /* not in list => can't have it */
}

/*
 * bad_ip_adrs - return 1 if the IP address is one we don't want
 * to use, such as an address in the loopback net or a multicast address.
 * addr is in network byte order.
 */
int bad_ip_adrs(u_int32_t addr)
{
	addr = ntohl(addr);
	return (addr >> IN_CLASSA_NSHIFT) == IN_LOOPBACKNET
		|| IN_MULTICAST(addr) || IN_BADCLASS(addr);
}

/*
 * check_access - complain if a secret file has too-liberal permissions.
 */
void check_access(FILE *f,char *filename)
{
	struct stat sbuf;

	if (fstat(fileno(f), &sbuf) < 0) {
		syslog(LOG_WARNING, "cannot stat secret file %s: %m", filename);
	} else if ((sbuf.st_mode & (S_IRWXG | S_IRWXO)) != 0) {
		syslog(LOG_WARNING, "Warning - secret file %s has world and/or group access", filename);
	}
}


/*
 * scan_authfile - Scan an authorization file for a secret suitable
 * for authenticating `client' on `server'.  The return value is -1
 * if no secret is found, otherwise >= 0.  The return value has
 * NONWILD_CLIENT set if the secret didn't have "*" for the client, and
 * NONWILD_SERVER set if the secret didn't have "*" for the server.
 * Any following words on the line (i.e. address authorization
 * info) are placed in a wordlist and returned in *addrs.  
 */
static int scan_authfile(FILE *f,char *client,char *server,char *secret,struct wordlist ** addrs,char *filename)
{
	int newline, xxx;
	int got_flag, best_flag;
	FILE *sf;
	struct wordlist *ap, *addr_list, *addr_last;
	char word[MAXWORDLEN];
	char atfile[MAXWORDLEN];

	if (addrs != NULL)
		*addrs = NULL;
	addr_list = NULL;
	if (!getword(f, word, &newline, filename))
		return -1;                /* file is empty??? */
	newline = 1;
	best_flag = -1;
	for (;;) {
		/*
		 * Skip until we find a word at the start of a line.
		 */
		while (!newline && getword(f, word, &newline, filename))
			;
		if (!newline)
			break;                /* got to end of file */

		/*
		 * Got a client - check if it's a match or a wildcard.
		 */
		got_flag = 0;
		if (client != NULL && strcmp(word, client) != 0 && !ISWILD(word)) {
			newline = 0;
			continue;
		}
		if (!ISWILD(word))
			got_flag = NONWILD_CLIENT;

		/*
		 * Now get a server and check if it matches.
		 */
		if (!getword(f, word, &newline, filename))
			break;
		if (newline)
			continue;
		if (server != NULL && strcmp(word, server) != 0 && !ISWILD(word))
			continue;
		if (!ISWILD(word))
			got_flag |= NONWILD_SERVER;

		/*
		 * Got some sort of a match - see if it's better than what
		 * we have already.
		 */
		if (got_flag <= best_flag)
			continue;

		/*
		 * Get the secret.
		 */
		if (!getword(f, word, &newline, filename))
			break;
		if (newline)
			continue;

		/*
		 * Special syntax: @filename means read secret from file.
		 */
		if (word[0] == '@') {
			strcpy(atfile, word+1);
			if ((sf = fopen(atfile, "r")) == NULL) {
				syslog(LOG_WARNING, "can't open indirect secret file %s",
					   atfile);
				continue;
			}
			check_access(sf, atfile);
			if (!getword(sf, word, &xxx, atfile)) {
				syslog(LOG_WARNING, "no secret in indirect secret file %s",
					   atfile);
				fclose(sf);
				continue;
			}
			fclose(sf);
		}
		if (secret != NULL)
			strcpy(secret, word);
				
		best_flag = got_flag;

		/*
		 * Now read address authorization info and make a wordlist.
		 */
		if (addr_list)
			free_wordlist(addr_list);
		addr_list = addr_last = NULL;
		for (;;) {
			if (!getword(f, word, &newline, filename) || newline)
				break;
			ap = (struct wordlist *) malloc(sizeof(struct wordlist) + strlen(word));
			if (ap == NULL)
				novm("authorized addresses");
			ap->next = NULL;
			strcpy(ap->word, word);
			if (addr_list == NULL)
				addr_list = ap;
			else
				addr_last->next = ap;
			addr_last = ap;
		}
		if (!newline)
			break;
	}

	if (addrs != NULL)
		*addrs = addr_list;
	else if (addr_list != NULL)
		free_wordlist(addr_list);

	return best_flag;
}

/*
 * free_wordlist - release memory allocated for a wordlist.
 */
static void free_wordlist(struct wordlist *wp)
{
	struct wordlist *next;

	while (wp != NULL) {
		next = wp->next;
		free(wp);
		wp = next;
	}
}

/*
 * auth_script - execute a script with arguments
 * interface-name peer-name real-user tty speed remote-number [fail-reason]
 */
static void auth_script(int linkunit,char *script,int error_reason)
{
	char strspeed[32];
	struct passwd *pw;
	char struid[32];
	char *user_name;
	char *argv[9];

	if ((pw = getpwuid(getuid())) != NULL && pw->pw_name != NULL)
		user_name = pw->pw_name;
	else {
		sprintf(struid, "%d", getuid());
		user_name = struid;
	}
	sprintf(strspeed, "%d", 64000);

	argv[0] = script;
	argv[1] = lns[linkunit].ifname;
	argv[2] = lns[linkunit].peer_authname;
	argv[3] = user_name;
	argv[4] = lns[linkunit].devnam;
	argv[5] = strspeed;
	argv[6] = lns[linkunit].pci.remote_num;
	argv[7] = NULL;
	if (error_reason) {
	  sprintf(struid,"%d",error_reason);
	  argv[7] = struid;
	  argv[8] = NULL;
	}
	run_program(script, argv, debug,linkunit);
}



