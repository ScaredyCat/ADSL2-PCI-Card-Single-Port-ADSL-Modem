/*
 * capiconnect.c - pppd plugin to make ISDN connections over CAPI2.0
 *
 * Copyright 2000 Carsten Paeth (calle@calle.in-berlin.de)
 * Copyright 2000 AVM GmbH Berlin (info@avm.de)
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 */
#include <unistd.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "pppd.h"
#include "capiconn.h"
#include <malloc.h>
#include <string.h>
#include <dlfcn.h>
#include <errno.h>
#include <linux/if.h>
#include <linux/in.h>

static char *revision = "$Revision: 1.27 $";

/* -------------------------------------------------------------------- */

#define AVMADSLPARAMFILE "/etc/drdsl/adsl.conf"

/* -------------------------------------------------------------------- */

#include "patchlevel.h"
#ifdef VERSION
char pppd_version[] = VERSION;
#endif

#define PPPVersion(v1,v2,v3,v4) ((v1)*1000000+(v2)*10000+(v3)*100+(v4))

/* -------------------------------------------------------------------- */

#if PPPVER >= PPPVersion(2,4,1,0)
#define _timeout(a,b,c) timeout(a, b, c, 0);
#else
#define _timeout(a,b,c) timeout(a, b, c);
#endif
#if PPPVER >= PPPVersion(2,4,0,0)
#define _script_setenv(a,b) script_setenv(a,b,0)
#else
#define _script_setenv(a,b) script_setenv(a,b)
#endif

/* -------------------------------------------------------------------- */

static int exiting = 0;

/* -------------------------------------------------------------------- */

typedef struct stringlist {
    struct stringlist *next;
	char              *s;
} STRINGLIST;

/* -------------------------------------------------------------------- */

static int curphase = -1;
static int wakeupnow = 0;

/* -------------------------------------------------------------------- */

static void handlemessages(void) ;
static void stringlist_free(STRINGLIST **pp);
static int stringlist_append_string(STRINGLIST **pp, char *s);
static STRINGLIST *stringlist_split(char *tosplit, char *seps);
static void makecallback(void);
static char *phase2str(int phase);
static void wakeupdemand(void);

/* -------------------------------------------------------------------- */

static capiconn_context *ctx;
static unsigned applid;
#define CM(x)	(1<<(x))
#define	CIPMASK_ALL	0x1FFF03FF
#define	CIPMASK_VOICE	(CM(1)|CM(4)|CM(5)|CM(16)|CM(26))
#define	CIPMASK_DATA	(CM(2)|CM(3))
static unsigned long cipmask = CIPMASK_ALL;
static int controller = 1;
static capi_contrinfo cinfo = { 0 , 0, 0 };

/* -------------------------------------------------------------------- */

static char *opt_controller;
/*
 * numbers
 */
static char *opt_numberprefix = 0;
static char *opt_number = 0;
static char *opt_callbacknumber = 0;
static char *opt_msn = 0;
static char *opt_inmsn = 0;
static char *opt_cli = 0;
static STRINGLIST *numbers;
static STRINGLIST *callbacknumbers;
static STRINGLIST *clis;
static STRINGLIST *inmsns;
/*
 * protocol
 */
#define PROTO_HDLC		0
#define PROTO_X75		1
#define PROTO_V42BIS		2
#define PROTO_MODEM		3
#define PROTO_V110_ASYNC	4
#define PROTO_V120_ASYNC	5
#define PROTO_ADSLPPPOE		6
#define PROTO_ADSLPPPOA		7
#define PROTO_ADSLPPPOALLC	8
static char *opt_proto = "hdlc";
static int proto = PROTO_HDLC;
static int opt_avmadsl = 0;
static int opt_vpi = -1; /* T-DSL: 1  */
static int opt_vci = -1; /* T-DSL: 32 */
/*
 * leased line
 */
static char *opt_channels = 0;
static unsigned char AdditionalInfo[1+2+2+31];
static unsigned char B1Config[1+2+2+2];
/*
 * redialing
 */
static int opt_dialtimeout = 60;
static int opt_dialmax = 4;
static int opt_redialdelay = 5;
/*
 * callback & COSO
 */
static int opt_cbdelay = 2;
static int opt_cbflag  = 0;
static int opt_cbwait = 60;
#define COSO_CALLER	0
#define COSO_LOCAL	1
#define COSO_REMOTE	2
static char *opt_coso = 0;
static int coso = COSO_CALLER;
/*
 * connect delay
 */
static int opt_connectdelay = 0;
/*
 * accept delay
 */
static int opt_acceptdelayflag  = 0;
/*
 * demand wakeup by phone call
 */
static int opt_voicecallwakeup  = 0;

/* -------------------------------------------------------------------- */

static int optcb(void) { return opt_cbflag = 1; }
static int optacceptdelay(void) { return opt_acceptdelayflag = 1; }
static int optvoicecallwakeup(void) { return opt_voicecallwakeup = 1; }
static int optavmadsl(void) { return opt_avmadsl = 1; }

static option_t my_options[] = {
	{
		"controller", o_string, &opt_controller,
		"capi controller specification"
	},
	{
		"number", o_string, &opt_number,
		"number to call (may be comma separated)"
        },
	{
		"numberprefix", o_string, &opt_numberprefix,
		"prefix for number"
        },
	{
		"msn", o_string, &opt_msn,
		"number to call from"
        },
	{
		"protocol", o_string, &opt_proto,
		"protocol x75, hdlc, modem, adslpppoe"
        },
	{
		"inmsn", o_string, &opt_inmsn,
		"called number for incoming calls"
        },
	{
		"cli", o_string, &opt_cli,
		"calling number for incoming calls (may be comma separated list)"
        },
	{
		"clicb", o_special_noarg, &optcb,
		"call number and wait for callback"
	},
	{
		"cbwait", o_int, &opt_cbwait,
		"number of seconds to wait for callback"
	},
	{
		"dialtimeout", o_int, &opt_dialtimeout,
		"number of seconds to wait for connection or reject"
	},
	{
		"dialmax", o_int, &opt_dialmax,
		"number of dial retries"
	},
	{
		"redialdelay", o_int, &opt_redialdelay,
		"number of seconds to wait between dial retries"
	},
	{
		"channels", o_string, &opt_channels,
		"channel to use for leased line (may be comma separated list)"
        },

	{
		"cbdelay", o_int, &opt_cbdelay,
		"number of seconds to wait before calling back"
	},
	{
		"cbnumber", o_string, &opt_callbacknumber,
		"number to call (may be comma separated)"
        },
	{
		"connectdelay", o_int, &opt_connectdelay,
		"number of seconds to wait after connection is established"
	},
	{
		"acceptdelay", o_special_noarg, &optacceptdelay,
		"wait 1 second before accept incoming call"
	},
	{
		"coso", o_string, &opt_coso,
		"COSO: caller,local or remote"
        },
	{
		"voicecallwakeup", o_special_noarg, &optvoicecallwakeup,
		"call number and wait for callback"
	},
	{
		"vpi", o_int, &opt_vpi,
		"VPI for Fritz!Card DSL"
	},
	{
		"vci", o_int, &opt_vci,
		"VCI for Fritz!Card DSL"
	},
	{
		"avmadsl", o_special_noarg, &optavmadsl,
		"read DSL parameters from /etc/drdsl/adsl.conf"
	},
	{ NULL }
};

/* -------------------------------------------------------------------- */
/* -------- Handle leased lines (CAPI-Bundling) ----------------------- */
/* -------------------------------------------------------------------- */

static int decodechannels(char *teln, unsigned long *bmaskp, int *activep)
{
	unsigned long bmask = 0;
	int active = !0;
	char *s;
	int channel;
	int i;

	s = teln;
	while (*s && *s == ' ') s++;
	if (!*s)
		fatal("capiplugin: option channels: list empty");
	if (*s == 'p' || *s == 'P') {
		active = 0;
		s++;
	}
	if (*s == 'a' || *s == 'A') {
		active = !0;
		s++;
	}
	while (*s) {
		int digit1 = 0;
		int digit2 = 0;
		if (!isdigit(*s))
			goto illegal;
		while (isdigit(*s)) { digit1 = digit1*10 + (*s - '0'); s++; }
		channel = digit1;
		if (channel <= 0 && channel > 30)
			goto rangeerror;
		if (*s == 0 || *s == ',' || *s == ' ') {
			bmask |= (1 << digit1);
			digit1 = 0;
			if (*s) s++;
			continue;
		}
		if (*s != '-') 
			goto illegal;
		s++;
		if (!isdigit(*s)) return -3;
		while (isdigit(*s)) { digit2 = digit2*10 + (*s - '0'); s++; }
		channel = digit2;
		if (channel <= 0 && channel > 30)
			goto rangeerror;
		if (*s == 0 || *s == ',' || *s == ' ') {
			if (digit1 > digit2)
				for (i = digit2; i <= digit1 ; i++)
					bmask |= (1 << i);
			else 
				for (i = digit1; i <= digit2 ; i++)
					bmask |= (1 << i);
			digit1 = digit2 = 0;
			if (*s) s++;
			continue;
		}
		goto illegal;
	}
	if (activep) *activep = active;
	if (bmaskp) *bmaskp = bmask;
	return 0;
illegal:
	fatal("capiplugin: option channels: illegal octet '%c'", *s);
	return -1;
rangeerror:
	fatal("capiplugin: option channels: channel %d out of range", channel);
	return -1;
}

static int channels2capi20(char *teln, unsigned char *AdditionalInfo)
{
	unsigned long bmask;
	int active;
	int i;
   
	decodechannels(teln, &bmask, &active);
	/* info("capiplugin: \"%s\" 0x%lx %d\n", teln, bmask, active); */
	/* Length */
	AdditionalInfo[0] = 2+2+31;
        /* Channel: 3 => use channel allocation */
        AdditionalInfo[1] = 3; AdditionalInfo[2] = 0;
	/* Operation: 0 => DTE mode, 1 => DCE mode */
        if (active) {
   		AdditionalInfo[3] = 0; AdditionalInfo[4] = 0;
   	} else {
   		AdditionalInfo[3] = 1; AdditionalInfo[4] = 0;
	}
	/* Channel mask array */
	AdditionalInfo[5] = 0; /* no D-Channel */
	for (i=1; i <= 30; i++)
		AdditionalInfo[5+i] = (bmask & (1 << i)) ? 0xff : 0;
	return 0;
}



/* -------------------------------------------------------------------- */

static void plugin_check_options(void)
{
	static int init = 0;

	if (init)
		return;
	init = 1;

	if (opt_avmadsl) {
	   if (access(AVMADSLPARAMFILE, R_OK) == 0) {
	      dbglog("loading adsl parameters from %s ...", AVMADSLPARAMFILE);
              if (options_from_file (AVMADSLPARAMFILE, 0, 0, 0) == 0)
	         die(1);
	   } else {
	       dbglog("using default adsl parameters");
	       if (!opt_controller) opt_controller = "2";
	       opt_proto = "adslpppoe";
	   }
	}

	/*
	 * protocol
	 */
	if (strcasecmp(opt_proto, "hdlc") == 0) {
	   proto = PROTO_HDLC;
	} else if (strcasecmp(opt_proto, "x75") == 0) {
	   proto = PROTO_X75;
	} else if (strcasecmp(opt_proto, "v42bis") == 0) {
	   proto = PROTO_V42BIS;
	} else if (strcasecmp(opt_proto, "modem") == 0) {
	   proto = PROTO_MODEM;
	} else if (strcasecmp(opt_proto, "v110async") == 0) {
	   proto = PROTO_V110_ASYNC;
	} else if (strcasecmp(opt_proto, "v120async") == 0) {
	   proto = PROTO_V120_ASYNC;
	} else if (strcasecmp(opt_proto, "adslpppoe") == 0) {
	   proto = PROTO_ADSLPPPOE;
	   if (!opt_channels) opt_channels = "1";
	} else if (strcasecmp(opt_proto, "adslpppoa") == 0) {
	   proto = PROTO_ADSLPPPOA;
	   if (!opt_channels) opt_channels = "1";
	} else if (strcasecmp(opt_proto, "adslpppoallc") == 0) {
	   proto = PROTO_ADSLPPPOALLC;
	   if (!opt_channels) opt_channels = "1";
	} else {
	   option_error("capiplugin: unknown protocol \"%s\"", opt_proto);
	   die(1);
	}
        if (strcasecmp(opt_proto, "modem") == 0)
		cipmask = CIPMASK_VOICE;
	else cipmask = CIPMASK_DATA;

	/*
	 * coso
	 */
	if (opt_coso == 0) {
	   if (opt_cbflag) {
	      if (opt_number) coso = COSO_REMOTE;
	      else coso = COSO_LOCAL;
	   } else {
	      coso = COSO_CALLER;
	   }
	} else if (strcasecmp(opt_coso, "caller") == 0) {
	   coso = COSO_CALLER;
	} else if (strcasecmp(opt_coso, "local") == 0) {
	   coso = COSO_LOCAL;
	} else if (strcasecmp(opt_coso, "remote") == 0) {
	   coso = COSO_REMOTE;
	} else {
	    option_error("capiplugin: wrong value for option coso");
	    die(1);
	}
	if (opt_cbflag) {
	   if (opt_coso) {
	      option_error("capiplugin: option clicb ignored");
	   } else if (coso == COSO_REMOTE) {
	      dbglog("clicb selected coso remote");
	   } else if (coso == COSO_LOCAL) {
	      dbglog("clicb selected coso local");
	   } else {
	      option_error("capiplugin: option clicb ignored");
	   }
	}

	/*
	 * leased line
	 */
	if (opt_channels) {
		channels2capi20(opt_channels, AdditionalInfo);
		if (opt_number)
			option_error("capiplugin: option number ignored");
		if (opt_numberprefix)
			option_error("capiplugin: option numberprefix ignored");
		if (opt_callbacknumber)
			option_error("capiplugin: option callbacknumber ignored");
		if (opt_msn)
			option_error("capiplugin: option msn ignored");
		if (opt_inmsn)
			option_error("capiplugin: option inmsn ignored");
	/*
	 * dialout
	 */
	} else if (opt_number) {
		stringlist_free(&numbers);
		numbers = stringlist_split(opt_number, " \t,");
	/*
	 * dialin
	 */
	} else {
	   if (coso == COSO_LOCAL) {
	      if (opt_callbacknumber == 0) {
		 option_error("capiplugin: option callbacknumber missing");
		 die(1);
	       }
	   }
	}

	if (opt_callbacknumber) {
	   stringlist_free(&callbacknumbers);
	   callbacknumbers = stringlist_split(opt_callbacknumber, " \t,");
	}

	/*
	 * controller
	 */
	if (opt_controller) {
                STRINGLIST *parsed_controller = 0;
		STRINGLIST *sl;
		char *tmp;
		memset(&cinfo, 0, sizeof(cinfo));
	   	parsed_controller = stringlist_split(opt_controller, " \t,");
		sl = parsed_controller;
		if (!sl) goto illcontr;
		tmp = sl->s;
		controller = strtol(sl->s, &tmp, 10);
		if (tmp == sl->s || *tmp) goto illcontr;
		if (sl->next) {
			sl = sl->next;
			cinfo.ddi = strdup(sl->s);
			if (cinfo.ddi == 0)
			   goto illcontr;
			if (sl->next && sl->next->s) {
			        sl = sl->next;
				cinfo.ndigits = strtol(sl->s, &tmp, 10);
				if (tmp == sl->s || *tmp) {
		                   stringlist_free(&parsed_controller);
				   goto illcontr;
			        }
			}
		}
		stringlist_free(&parsed_controller);
	} else {
		memset(&cinfo, 0, sizeof(cinfo));
		controller = 1;
	}

	/*
	 * cli & inmsn
	 */
	if (opt_cli) {
		stringlist_free(&clis);
		clis = stringlist_split(opt_cli, " \t,");
	}
	if (opt_inmsn) {
		stringlist_free(&inmsns);
		inmsns = stringlist_split(opt_inmsn, " \t,");
	}

	/*
	 * dial on demand
	 */
	if (demand) {
	   if (!opt_number && !opt_channels) {
	       option_error("capiplugin: number or channels missing for demand");
	       die(1);
	   }
	   if (opt_voicecallwakeup)
	      cipmask |= CIPMASK_VOICE;
	} else if (opt_voicecallwakeup) {
	   option_error("capiplugin: option voicecallwakeup ignored");
	   opt_voicecallwakeup = 0;
	}

	if (   proto == PROTO_ADSLPPPOE
	    || proto == PROTO_ADSLPPPOA
	    || proto == PROTO_ADSLPPPOALLC) {
	   if (opt_cbflag) {
	      option_error("capiplugin: option cbflag not alloed with protocol adslpppoe");
	      die(1);
	   }
	   if (opt_coso) {
	      option_error("capiplugin: option coso not alloed with protocol adslpppoe");
	      die(1);
	   }
           if (opt_number) {
	      option_error("capiplugin: option number not alloed with protocol adslpppoe");
	      die(1);
	   }
           if (opt_callbacknumber) {
	      option_error("capiplugin: option cbnumber not alloed with protocol adslpppoe");
	      die(1);
	   }
           if (opt_inmsn) {
	      option_error("capiplugin: option inmsn ignored");
	      opt_inmsn = 0;
	   }
           if (opt_cli) {
	      option_error("capiplugin: option cli ignored");
	      opt_cli = 0;
	   }
           if (proto == PROTO_ADSLPPPOA || proto == PROTO_ADSLPPPOALLC) {
	      if (opt_vpi == -1 || opt_vci == -1) {
	         option_error("capiplugin: need options vci and vpi");
		 die(1);
	      }
	   } else {
	      if (   (opt_vpi == -1 && opt_vci != -1)
		  || (opt_vpi != -1 && opt_vci == -1)) {
	         option_error("capiplugin: need options vci and vpi");
		 die(1);
	      }
	   }
           B1Config[0] = 0;
	   if (opt_vpi != -1) {
		int vcctype = 1;
	   	if (proto == PROTO_ADSLPPPOA) vcctype = 3;
	   	else if (proto == PROTO_ADSLPPPOALLC) vcctype = 2;
		B1Config[0] = 6;
	        B1Config[1] = vcctype & 0xff;
	        B1Config[2] = (vcctype >> 8) & 0xff;
		B1Config[3] = opt_vpi & 0xff;
		B1Config[4] = (opt_vpi >> 8) & 0xff;
		B1Config[5] = opt_vci & 0xff;
		B1Config[6] = (opt_vci >> 8) & 0xff;
	   }
	}

	return;

illcontr:
	option_error("capiplugin: illegal controller specification \"%s\"",
				opt_controller);
	die(1);
}

/* -------------------------------------------------------------------- */
/* -------- STRINGLIST for parsing ------------------------------------ */
/* -------------------------------------------------------------------- */

static void stringlist_free(STRINGLIST **pp)
{
	STRINGLIST *p, *next;

	p = *pp;
	while (p) {
		next = p->next;
  		if (p->s) free(p->s);
  		free(p);
  		p = next; 
	}
	*pp = 0;
}

static int stringlist_append_string(STRINGLIST **pp, char *s)
{
	STRINGLIST *p;
	for (; *pp; pp = &(*pp)->next) ;
	if ((p = (STRINGLIST *)malloc(sizeof(STRINGLIST))) == 0)
		return -1;
	memset(p, 0, sizeof(STRINGLIST));
	if ((p->s = strdup(s)) == 0) {
		free(p);
		return -1;
	}
	p->next = 0;
	*pp = p;
	return 0;
}

static STRINGLIST *stringlist_split(char *tosplit, char *seps)
{
	STRINGLIST *p = 0;
	char *str = strdup(tosplit);
	char *s;
	if (!str) return 0;
	for (s = strtok(str, seps); s; s = strtok(0, seps)) {
		if (*s == 0) continue; /* if strtok is not working correkt */
		if (stringlist_append_string(&p, s) < 0) {
 			stringlist_free(&p);
 			free(str);
 			return 0;
		}
	}
	free(str);
	return p;
}

/* -------------------------------------------------------------------- */
/* -------- connection management ------------------------------------- */
/* -------------------------------------------------------------------- */

typedef struct conn {
    struct conn     *next;
    capi_connection *conn;
    int              type;
#define CONNTYPE_OUTGOING	0
#define CONNTYPE_INCOMING	1
#define CONNTYPE_IGNORE		2
#define CONNTYPE_REJECT		3
#define CONNTYPE_FOR_CALLBACK	4
    int              inprogress;    
    int              isconnected;    
} CONN;

static CONN *connections;

/* -------------------------------------------------------------------- */

static CONN *conn_remember(capi_connection *conn, int type)
{
	CONN *p, **pp;
	for (pp = &connections; *pp; pp = &(*pp)->next) ;
	if ((p = (CONN *)malloc(sizeof(CONN))) == 0) {
       		int serrno = errno;
       		fatal("capiplugin: malloc failed - %s (%d)",
			      			strerror(serrno), serrno);
		return 0;
	}
	memset(p, 0, sizeof(CONN));
	p->conn = conn;
	p->type = type;
	p->next = 0;
	switch (type) {
           case CONNTYPE_OUTGOING:
           case CONNTYPE_INCOMING:
           case CONNTYPE_FOR_CALLBACK:
	      p->inprogress = 1;
	      p->isconnected = 0;
	      break;
	   default:
	      break;
	}
	*pp = p;
	return p;
}

static int conn_forget(capi_connection *conn)
{
	CONN **pp, *p;
	for (pp = &connections; *pp && (*pp)->conn != conn; pp = &(*pp)->next) ;
	if (*pp) {
	   p = *pp;
	   *pp = (*pp)->next;
	   free(p);
	   return 0;
        }
	return -1;
}

static CONN *conn_find(capi_connection *cp)
{
	CONN *p;
	for (p = connections; p; p = p->next) {
	   if (p->conn == cp)
	      return p;
	}
	return 0;
}

/* -------------------------------------------------------------------- */

static int conn_connected(capi_connection *conn)
{
	CONN *p;
	for (p = connections; p; p = p->next) {
	   if (p->conn == conn) {
	      p->isconnected = 1;
	      p->inprogress = 0;
	      return 0;
	   }
	}
	fatal("capiplugin: connected connection not found ??");
	return -1;
}

/* -------------------------------------------------------------------- */

static int conn_incoming_inprogress(void)
{
    CONN *p;
    for (p = connections; p; p = p->next) {
       if (p->type == CONNTYPE_INCOMING)
	  return p->inprogress;    
    }
    return 0;
}

static int conn_incoming_connected(void)
{
    CONN *p;
    for (p = connections; p; p = p->next) {
       if (p->type == CONNTYPE_INCOMING)
	  return p->isconnected;    
    }
    return 0;
}

static int conn_incoming_exists(void)
{
    CONN *p;
    for (p = connections; p; p = p->next) {
       if (p->type == CONNTYPE_INCOMING)
	  return p->isconnected || p->inprogress;    
    }
    return 0;
}

/* -------------------------------------------------------------------- */

static int conn_inprogress(capi_connection *cp)
{
    CONN *p;
    for (p = connections; p; p = p->next) {
       if (p->conn == cp)
	  return p->inprogress;    
    }
    return 0;
}

static int conn_isconnected(capi_connection *cp)
{
    CONN *p;
    if (cp) {
       for (p = connections; p; p = p->next) {
          if (p->conn == cp)
	     return p->isconnected;    
       }
    } else {
       for (p = connections; p; p = p->next) {
	  if (p->isconnected)
	     return 1;
       }
    }
    return 0;
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */


/* -------------------------------------------------------------------- */
/* -------- Handle CAPI messages every second ------------------------- */
/* -------------------------------------------------------------------- */

static int timeoutrunning = 0;
static int timeoutshouldrun = 0;

static void timeoutfunc(void *arg)
{
	unsigned char *msg = 0;
	/* info("capiplugin: checking for capi messages"); */
	while (capi20_get_message (applid, &msg) == 0)
		capiconn_inject(applid, msg);
        if (wakeupnow && curphase == PHASE_DORMANT)
           wakeupdemand();
	if (timeoutshouldrun)
		_timeout (timeoutfunc, 0, 1);
}

static void setup_timeout(void)
{
	timeoutshouldrun = 1;
	if (!timeoutrunning)
		_timeout (timeoutfunc, 0, 1);
}

static void unsetup_timeout(void)
{
	timeoutshouldrun = 0;
	if (timeoutrunning)
		untimeout (timeoutfunc, 0);
	timeoutrunning = 0;
}

/* -------------------------------------------------------------------- */
/* -------- demand & wakeup pppd -------------------------------------- */
/* -------------------------------------------------------------------- */

static u_int32_t ouripaddr;
static u_int32_t gwipaddr;
static int nwakeuppackets = 0;

static void setupincoming_for_demand(void)
{
    struct ifreq ifr; 
    int serrno;
    int sock_fd;

    nwakeuppackets = 0;
    if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
       serrno = errno;
       fatal("capiplugin: socket(AF_INET,SOCK_DRAM): failed - %s (%d)",
			      strerror(serrno), serrno);
       return;
    }
    
    memset (&ifr, '\0', sizeof (ifr));
    strlcpy (ifr.ifr_name, ifname, sizeof (ifr.ifr_name));
    if (ioctl(sock_fd, SIOCGIFADDR, (caddr_t) &ifr) < 0) {
       serrno = errno;
       close(sock_fd);
       fatal("capiplugin: ioctl(SIOCGIFADDR): failed - %s (%d)",
			      strerror(serrno), serrno);
       return;
    }
    ouripaddr = ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr;

    memset (&ifr, '\0', sizeof (ifr));
    strlcpy (ifr.ifr_name, ifname, sizeof (ifr.ifr_name));
    if (ioctl(sock_fd, SIOCGIFDSTADDR, (caddr_t) &ifr) < 0) {
       serrno = errno;
       close(sock_fd);
       fatal("capiplugin: ioctl(SIOCGIFDSTADDR): failed - %s (%d)",
			      strerror(serrno), serrno);
       return;
    } 
    gwipaddr = ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr;

    close(sock_fd);
    (void) capiconn_listen(ctx, controller, cipmask, 0);
    info("capiplugin: waiting for demand wakeup ...");
}

static void wakeupdemand(void)
{
    char data[] = "Ignore, is for demand wakeup";
    struct sockaddr_in laddr, raddr;
    size_t addrlen;
    int sock_fd;
    int serrno;

    if (curphase != PHASE_DORMANT) {
       info("capiplugin: wakeup not possible in phase %s, delayed",
		phase2str(curphase));
       wakeupnow++;
       return;
    }

    if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
       serrno = errno;
       fatal("capiplugin: socket(AF_INET,SOCK_DRAM): failed - %s (%d)",
			      strerror(serrno), serrno);
       return;
    }
    memset (&laddr, '\0', sizeof(laddr));
    laddr.sin_family = AF_INET;
    laddr.sin_addr.s_addr = ouripaddr;
    laddr.sin_port = 0;
    if (bind(sock_fd, (struct sockaddr *)&laddr, sizeof(laddr)) != 0) {
       serrno = errno;
       close(sock_fd);
       fatal("capiplugin: bind(%I): failed - %s (%d)",
			      ouripaddr, strerror(serrno), serrno);
       return;
    }
    addrlen = sizeof(laddr);
    getsockname(sock_fd, (struct sockaddr *)&laddr, &addrlen);
    memset (&raddr, '\0', sizeof(raddr));
    raddr.sin_family = AF_INET;
    raddr.sin_addr.s_addr = gwipaddr;
    raddr.sin_port = htons(9); /* discard */
    if (connect(sock_fd, (struct sockaddr *)&raddr, sizeof(raddr)) != 0) {
       serrno = errno;
       close(sock_fd);
       fatal("capiplugin: connect(%I:%d): failed - %s (%d)",
	      gwipaddr, ntohs(raddr.sin_port), strerror(serrno), serrno);
       return;
    }
    notice("capiplugin: sending wakeup packet (UDP %I:%d -> %I:%d)",
		    ouripaddr, ntohs(laddr.sin_port),
		    gwipaddr, ntohs(raddr.sin_port));
    if (send(sock_fd, data, sizeof(data), 0) < 0) {
       serrno = errno;
       close(sock_fd);
       fatal("capiplugin: send wakup packet failed - %s (%d)",
			      strerror(serrno), serrno);
       return;
    }
    close(sock_fd);
    nwakeuppackets++;
}

/* -------------------------------------------------------------------- */
/* -------- CAPI setup & handling ------------------------------------- */
/* -------------------------------------------------------------------- */

static void init_capiconn(void)
{
	static int init = 0;

	if (init)
		return;
	init = 1;

	if (capiconn_addcontr(ctx, controller, &cinfo) != CAPICONN_OK) {
		(void)capiconn_freecontext(ctx);
		(void)capi20_release(applid);
		fatal("capiplugin: add controller %d failed", controller);
		return;
	}
	if (cinfo.ddi) 
		dbglog("capiplugin: contr=%d ddi=\"%s\" n=%d",
			controller, cinfo.ddi, cinfo.ndigits);
	else
		dbglog("capiplugin: contr=%d", controller);

	(void) capiconn_listen(ctx, controller, 0, 0);
	handlemessages();
	add_fd(capi20_fileno(applid));
	setup_timeout();
}

/* -------------------------------------------------------------------- */

static void handlemessages(void) 
{
	unsigned char *msg = 0;
	struct timeval tv;
	tv.tv_sec  = 1;
	tv.tv_usec = 0;
	if (capi20_waitformessage(applid, &tv) == 0) {
		if (capi20_get_message(applid, &msg) == 0)
			capiconn_inject(applid, msg);
	}
}

/* -------------------------------------------------------------------- */

static void dodisconnect(capi_connection *cp)
{
	CONN *conn;
	time_t t;

	if ((conn = conn_find(cp)) == 0)
		return;
	(void)capiconn_disconnect(cp, 0);
	conn->isconnected = conn->inprogress = 0;

	t = time(0)+10;
	do {
	    handlemessages();
        } while (conn_find(cp) && time(0) < t);

        if (conn_find(cp))
		fatal("capiplugin: timeout while waiting for disconnect");
}

static void disconnectall(void)
{
	time_t t;
	CONN *p;

        (void) capiconn_listen(ctx, controller, 0, 0);
	for (p = connections; p; p = p->next) {
	      if (p->inprogress || p->isconnected) {
		 p->isconnected = p->inprogress = 0;
        	 capiconn_disconnect(p->conn, 0);
	      }
	}
	t = time(0)+10;
	do {
	    handlemessages();
        } while (connections && time(0) < t);

	if (connections)
        	fatal("capiplugin: disconnectall failed");
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */

static char *conninfo(capi_connection *p)
{
	static char buf[1024];
	capi_conninfo *cp = capiconn_getinfo(p);
	char *callingnumber = "";
	char *callednumber = "";

	if (cp->callingnumber && cp->callingnumber[0] > 2)
		callingnumber = cp->callingnumber+3;
	if (cp->callednumber && cp->callednumber[0] > 1)
		callednumber = cp->callednumber+2;

	if (debug) {
		snprintf(buf, sizeof(buf),
			"\"%s\" -> \"%s\" %s (pcli=0x%x/ncci=0x%x)",
				callingnumber, callednumber,
				cp->isincoming ? "incoming" : "outgoing",
				cp->plci, cp->ncci
				);
	} else {
		snprintf(buf, sizeof(buf),
			"\"%s\" -> \"%s\" %s",
				callingnumber, callednumber,
				cp->isincoming ? "incoming" : "outgoing");
	}
	buf[sizeof(buf)-1] = 0;
	return buf;
}

/* -------------------------------------------------------------------- */
/* -------- reject reason handling (wuerg) ---------------------------- */
/* -------------------------------------------------------------------- */

static unsigned dreason = 0;

static int was_no_reject(void)
{
      if ((dreason & 0xff00) != 0x3400)
	 return 1;
      switch (dreason) {
	 case 0x34a2: /* No circuit / channel available */
	    return 1;
      }
      return 0;
}

/* -------------------------------------------------------------------- */
/* -------- disconnect handler ---------------------------------------- */
/* -------------------------------------------------------------------- */

static void disconnected(capi_connection *cp,
				int localdisconnect,
				unsigned reason,
				unsigned reason_b3)
{
	CONN *p;

	if ((p = conn_find(cp)) == 0)
	   return;
        conn_forget(cp);
	switch (p->type) {
           case CONNTYPE_OUTGOING:
           case CONNTYPE_INCOMING:
	      break;
           case CONNTYPE_IGNORE:
           case CONNTYPE_REJECT:
	      return;
           case CONNTYPE_FOR_CALLBACK:
	      dreason = reason;
              break;

	}
	if (reason != 0x3304 || debug) /* Another Applikation got the call */
		info("capiplugin: disconnect(%s): %s 0x%04x (0x%04x) - %s", 
			localdisconnect ? "local" : "remote",
			conninfo(cp),
			reason, reason_b3, capi_info2str(reason));
}

/* -------------------------------------------------------------------- */
/* -------- incoming call handler ------------------------------------- */
/* -------------------------------------------------------------------- */

static void incoming(capi_connection *cp,
				unsigned contr,
				unsigned cipvalue,
				char *callednumber,
				char *callingnumber)
{
	STRINGLIST *p;
        char *s;

	info("capiplugin: incoming call: %s (0x%x)", conninfo(cp), cipvalue);

        if (conn_incoming_exists()) {
	   info("capiplugin: ignoring call, incoming connection exists");
           conn_remember(cp, CONNTYPE_IGNORE);
	   (void) capiconn_ignore(cp);
           return;
	}
	
	if (opt_inmsn) {
	   for (p = inmsns; p; p = p->next) {
	      if (   (s = strstr(callednumber, p->s)) != 0
                  && strcmp(s, p->s) == 0)
		 break;
	   }
	   if (!p) {
	           info("capiplugin: ignoring call, msn %s not in \"%s\"",
				callednumber, opt_inmsn);
                   conn_remember(cp, CONNTYPE_IGNORE);
		   (void) capiconn_ignore(cp);
		   return;
           }
        } else if (opt_msn) {
	   if (   (s = strstr(callednumber, opt_msn)) == 0
               || strcmp(s, opt_msn) != 0) {
	           info("capiplugin: ignoring call, msn mismatch (%s != %s)",
				opt_msn, callednumber);
                   conn_remember(cp, CONNTYPE_IGNORE);
		   (void) capiconn_ignore(cp);
		   return;
           }
        }

	if (opt_cli) {
	   for (p = clis; p; p = p->next) {
	       if (   (s = strstr(callingnumber, p->s)) != 0
                   && strcmp(s, p->s) == 0)
		   break;
	   }
	   if (!p) {
	           info("capiplugin: ignoring call, cli mismatch (%s != %s)",
				opt_cli, callingnumber);
                   conn_remember(cp, CONNTYPE_IGNORE);
		   (void) capiconn_ignore(cp);
		   return;
	   }
        } else if (opt_number) {
	   for (p = numbers; p; p = p->next) {
	       if (   (s = strstr(callingnumber, p->s)) != 0
                   && strcmp(s, p->s) == 0)
		   break;
           }
	   if (!p) {
	           info("capiplugin: ignoring call, number mismatch (%s != %s)",
				opt_number, callingnumber);
                   conn_remember(cp, CONNTYPE_IGNORE);
		   (void) capiconn_ignore(cp);
		   return;
	   }
        } else if (opt_acceptdelayflag) {
		/*
		* non cli or number match,
		* give more specific listen a chance (bad)
		*/
		info("capiplugin: accept delayed, no cli or number match");
		sleep(1);
	}

	switch (cipvalue) {
		case 1:	 /* Speech */
		case 4:  /* 3.1 kHz audio */
		case 5:  /* 7 kHz audio */
		case 16: /* Telephony */
		case 26: /* 7 kHz telephony */
	                if (proto == PROTO_MODEM) {
			   if (demand) goto wakeupmatch;
			   if (coso == COSO_LOCAL) goto callback;
			   goto accept;
                        } else if (opt_voicecallwakeup) {
			   goto wakeupdemand;
                        } else {
	                   info("capiplugin: ignoring speech call from %s",
			   		callingnumber);
                           conn_remember(cp,CONNTYPE_IGNORE);
			   (void) capiconn_ignore(cp);
			}
			break;
		case 2:  /* Unrestricted digital information */
		case 3:  /* Restricted digital information */
	                if (proto == PROTO_HDLC) {
			   if (demand) goto wakeupmatch;
			   if (coso == COSO_LOCAL) goto callback;
			   goto accept;
	                } else if (proto == PROTO_X75) {
			   if (demand) goto wakeupmatch;
			   if (coso == COSO_LOCAL) goto callback;
			   goto accept;
	                } else if (proto == PROTO_V42BIS) {
			   if (demand) goto wakeupmatch;
			   if (coso == COSO_LOCAL) goto callback;
			   goto accept;
			} else {
	                   info("capiplugin: ignoring digital call from %s",
			   		callingnumber);
                           conn_remember(cp,CONNTYPE_IGNORE);
			   (void) capiconn_ignore(cp);
			}
			break;
		case 17: /* Group 2/3 facsimile */
		        info("capiplugin: ignoring fax call from %s",
			   		callingnumber);
                        conn_remember(cp,CONNTYPE_IGNORE);
			(void) capiconn_ignore(cp);
			break;
		default:
		        info("capiplugin: ignoring type %d call from %s",
			   		cipvalue, callingnumber);
                        conn_remember(cp,CONNTYPE_IGNORE);
			(void) capiconn_ignore(cp);
			break;
	}
	return;

callback:
	(void) capiconn_listen(ctx, controller, 0, 0);
	dbglog("capiplugin: rejecting call: %s (0x%x)", conninfo(cp), cipvalue);
	conn_remember(cp, CONNTYPE_REJECT);
	capiconn_reject(cp);
        makecallback();
	return;

wakeupdemand:
	dbglog("capiplugin: rejecting call: %s (0x%x)", conninfo(cp), cipvalue);
	conn_remember(cp, CONNTYPE_REJECT);
	capiconn_reject(cp);
        wakeupdemand();
	return;

wakeupmatch:
	if (coso == COSO_LOCAL)
	   goto wakeupdemand;

accept:
	switch (proto) {
	   default:
	   case PROTO_HDLC:
	      (void) capiconn_accept(cp, 0, 1, 0, 0, 0, 0, 0);
	      break;
	   case PROTO_X75:
	      (void) capiconn_accept(cp, 0, 0, 0, 0, 0, 0, 0);
	      break;
           case PROTO_V42BIS:
	      (void) capiconn_accept(cp, 0, 8, 0, 0, 0, 0, 0);
	      break;
           case PROTO_MODEM:
	      (void) capiconn_accept(cp, 8, 1, 0, 0, 0, 0, 0);
	      break;
           case PROTO_V110_ASYNC:
	      (void) capiconn_accept(cp, 2, 1, 0, 0, 0, 0, 0);
	      break;
           case PROTO_V120_ASYNC:
	      (void) capiconn_accept(cp, 0, 9, 0, 0, 0, 0, 0);
	      break;
	}
	conn_remember(cp, CONNTYPE_INCOMING);
	(void) capiconn_listen(ctx, controller, 0, 0);
	return;
}

/* -------------------------------------------------------------------- */
/* -------- connection established handler ---------------------------- */
/* -------------------------------------------------------------------- */

static void connected(capi_connection *cp, _cstruct NCPI)
{
	capi_conninfo *p = capiconn_getinfo(cp);
	char *callingnumber = "";
	char *callednumber = "";
	char buf[PATH_MAX];
	char *tty;
	int retry = 0;
	int serrno;

        info("capiplugin: connected: %s", conninfo(cp));

        tty = capi20ext_get_tty_devname(p->appid, p->ncci, buf, sizeof(buf));
        serrno = errno;
	while (tty == 0 && serrno == ESRCH) {
	   if (++retry > 4) 
	      break;
	   dbglog("capiplugin: capitty not ready, waiting for driver ...");
	   sleep(1);
	   tty = capi20ext_get_tty_devname(p->appid, p->ncci, buf, sizeof(buf));
	   serrno = errno;
	}
	if (tty == 0) {
	   if (serrno == EINVAL) {
	      fatal("capiplugin: failed to get tty devname - CAPI Middleware Support not enabled in kernel ?");
	   }
	   fatal("capiplugin: failed to get tty devname - %s (%d)",
			strerror(serrno), serrno);
	}
	if (access(tty, 0) != 0 && errno == ENOENT) {
	      fatal("capiplugin: tty %s doesn't exist - CAPI Filesystem Support not enabled in kernel or not mounted ?", tty);
	}
	info("capiplugin: using %s: %s", tty, conninfo(cp));
	strcpy(devnam, tty);

	if (p->callingnumber && p->callingnumber[0] > 2)
		callingnumber = p->callingnumber+3;
	if (p->callednumber && p->callednumber[0] > 1)
		callednumber = p->callednumber+2;
        _script_setenv("CALLEDNUMBER", callednumber);
        _script_setenv("CALLINGNUMBER", callingnumber);
	sprintf(buf, "%d", p->cipvalue); _script_setenv("CIPVALUE", buf);
	sprintf(buf, "%d", p->b1proto); _script_setenv("B1PROTOCOL", buf);
	sprintf(buf, "%d", p->b2proto); _script_setenv("B2PROTOCOL", buf);
	sprintf(buf, "%d", p->b3proto); _script_setenv("B3PROTOCOL", buf);

	conn_connected(cp);
	if (curphase == PHASE_DORMANT)
           wakeupdemand();
}

/* -------------------------------------------------------------------- */
/* -------- charge information ---------------------------------------- */
/* -------------------------------------------------------------------- */

void chargeinfo(capi_connection *cp, unsigned long charge, int inunits)
{
	if (inunits) {
        	info("capiplugin: %s: charge in units: %d", conninfo(cp), charge);
	} else {
        	info("capiplugin: %s: charge in currency: %d", conninfo(cp), charge);
	}
}

/* -------------------------------------------------------------------- */
/* -------- tranfer capi message to CAPI ------------------------------ */
/* -------------------------------------------------------------------- */

void put_message(unsigned appid, unsigned char *msg)
{
	unsigned err;
	err = capi20_put_message (appid, msg);
	if (err && !exiting)
		fatal("capiplugin: putmessage(appid=%d) - %s 0x%x",
			      appid, capi_info2str(err), err);
}

/* -------------------------------------------------------------------- */
/* -------- capiconn module setup ------------------------------------- */
/* -------------------------------------------------------------------- */

capiconn_callbacks callbacks = {
	malloc: malloc,
	free: free,

	disconnected: disconnected,
	incoming: incoming,
	connected: connected,
	received: 0, 
	datasent: 0, 
	chargeinfo: chargeinfo,

	capi_put_message: put_message,

	debugmsg: dbglog,
	infomsg: info,
	errmsg: error
};

/* -------------------------------------------------------------------- */
/* -------- create a connection --------------------------------------- */
/* -------------------------------------------------------------------- */

static capi_connection *setupconnection(char *num, int awaitingreject)
{
	struct capi_connection *cp;
	char number[256];

	snprintf(number, sizeof(number), "%s%s", 
			opt_numberprefix ? opt_numberprefix : "", num);

	if (proto == PROTO_HDLC) {
		cp = capiconn_connect(ctx,
				controller, /* contr */
				2, /* cipvalue */
				opt_channels ? 0 : number, 
				opt_channels ? 0 : opt_msn,
				0, 1, 0,
				0, 0, 0,
				opt_channels ? AdditionalInfo : 0,
				0);
	} else if (proto == PROTO_X75) {
		cp = capiconn_connect(ctx,
				controller, /* contr */
				2, /* cipvalue */
				opt_channels ? 0 : number, 
				opt_channels ? 0 : opt_msn,
				0, 0, 0,
				0, 0, 0,
				opt_channels ? AdditionalInfo : 0,
				0);
	} else if (proto == PROTO_V42BIS) {
		cp = capiconn_connect(ctx,
				controller, /* contr */
				2, /* cipvalue */
				opt_channels ? 0 : number, 
				opt_channels ? 0 : opt_msn,
				0, 8, 0,
				0, 0, 0,
				opt_channels ? AdditionalInfo : 0,
				0);
	} else if (proto == PROTO_MODEM) {
		cp = capiconn_connect(ctx,
				controller, /* contr */
				4, /* cipvalue */
				opt_channels ? 0 : number, 
				opt_channels ? 0 : opt_msn,
				8, 1, 0,
				0, 0, 0,
				opt_channels ? AdditionalInfo : 0,
				0);
	} else if (proto == PROTO_V110_ASYNC) {
		cp = capiconn_connect(ctx,
				controller, /* contr */
				2, /* cipvalue */
				opt_channels ? 0 : number, 
				opt_channels ? 0 : opt_msn,
				2, 1, 0,
				0, 0, 0,
				opt_channels ? AdditionalInfo : 0,
				0);
	} else if (proto == PROTO_V120_ASYNC) {
		cp = capiconn_connect(ctx,
				controller, /* contr */
				2, /* cipvalue */
				opt_channels ? 0 : number, 
				opt_channels ? 0 : opt_msn,
				0, 9, 0,
				0, 0, 0,
				opt_channels ? AdditionalInfo : 0,
				0);
	} else if (proto == PROTO_ADSLPPPOE) {
		cp = capiconn_connect(ctx,
				controller, /* contr */
				2, /* cipvalue */
				0,
				0,
				28, 30, 30,
				B1Config[0] ? B1Config : 0,
				0, /* B2Config */
				0, /* B3Config */
				opt_channels ? AdditionalInfo : 0,
				0);
	} else if (proto == PROTO_ADSLPPPOA || proto == PROTO_ADSLPPPOALLC) {
		cp = capiconn_connect(ctx,
				controller, /* contr */
				2, /* cipvalue */
				0,
				0,
				28, 1, 0,
				B1Config[0] ? B1Config : 0,
				0, /* B2Config */
				0, /* B3Config */
				opt_channels ? AdditionalInfo : 0,
				0);
	} else {
		fatal("capiplugin: unknown protocol \"%s\"", opt_proto);
		return 0;
	}
	if (opt_channels) {
		info("capiplugin: leased line (%s)", opt_proto);
	} else if (awaitingreject) {
		info("capiplugin: dialing %s (awaiting reject)", number);
	} else {
		info("capiplugin: dialing %s (%s)", number, opt_proto);
	}
        if (awaitingreject)
           conn_remember(cp, CONNTYPE_FOR_CALLBACK);
	else
           conn_remember(cp, CONNTYPE_OUTGOING);
        return cp;
}

/* -------------------------------------------------------------------- */
/* -------- connect leased line --------------------------------------- */
/* -------------------------------------------------------------------- */

static void makeleasedline(void)
{
	capi_connection *cp;
	time_t t;

	cp = setupconnection("", 0);

	t = time(0)+opt_dialtimeout;
	do {
		handlemessages();
		if (status != EXIT_OK && conn_find(cp)) {
			info("capiplugin: pppd status %d, disconnecting ...", status);
 			dodisconnect(cp);
		}
	} while (time(0) < t && conn_inprogress(cp));

	if (status != EXIT_OK)
		die(status);

        if (conn_isconnected(cp)) {
		t = time(0)+opt_connectdelay;
		do {
			handlemessages();
		} while (time(0) < t);
	}

	if (status != EXIT_OK)
		die(status);

        if (!conn_isconnected(cp))
	   fatal("capiplugin: couldn't make connection");
}

/* -------------------------------------------------------------------- */
/* -------- connect a dislup connection ------------------------------- */
/* -------------------------------------------------------------------- */

static void makeconnection(STRINGLIST *numbers)
{
	capi_connection *cp = 0;
	time_t t;
	STRINGLIST *p;
	int retry = 0;

	do {
	   for (p = numbers; p; p = p->next) {
		   if (retry || p != numbers) {
		      t = time(0)+opt_redialdelay;
		      do {
			 handlemessages();
			 if (status != EXIT_OK)
			    die(status);
		      } while (time(0) < t);
		   }

		   cp = setupconnection(p->s, 0);

		   t = time(0)+opt_dialtimeout;
		   do {
		      handlemessages();
		      if (status != EXIT_OK) {
			 if (conn_find(cp)) {
			    info("capiplugin: pppd status %d, disconnecting ...", status);
			    dodisconnect(cp);
			 } else {
			    die(status);
			 }
		      }
		   } while (time(0) < t && conn_inprogress(cp));

		   if (conn_isconnected(cp))
		      goto connected;

		   if (status != EXIT_OK)
		      die(status);
	   }
	} while (++retry < opt_dialmax);

connected:
        if (conn_isconnected(cp)) {
		t = time(0)+opt_connectdelay;
		do {
			handlemessages();
		} while (time(0) < t);
	}

        if (!conn_isconnected(cp))
	   fatal("capiplugin: couldn't make connection after %d retries",
			retry);
}

/* -------------------------------------------------------------------- */
/* -------- dial and wait for callback -------------------------------- */
/* -------------------------------------------------------------------- */

static void makeconnection_with_callback(void)
{
	capi_connection *cp;
	STRINGLIST *p;
	time_t t;
	int retry = 0;

	do {
   	   for (p = numbers; p; p = p->next) {

		if (retry || p != numbers) {
again:
		   t = time(0)+opt_redialdelay;
		   do {
		      handlemessages();
		      if (status != EXIT_OK)
			 die(status);
		   } while (time(0) < t);
		}

		cp = setupconnection(p->s, 1);

		/* Wait specific time for the server rejecting the call */
		t = time(0)+opt_dialtimeout;
		do {
		      handlemessages();
		      if (status != EXIT_OK)
			 die(status);
	        } while (time(0) < t && conn_inprogress(cp));

		if (conn_isconnected(cp)) {
			dodisconnect(cp);
			fatal("capiplugin: callback failed - other party answers the call (no reject)");
	        } else if (was_no_reject()) {
	        	goto again;
		} else {
			(void) capiconn_listen(ctx, controller, cipmask, 0);
			info("capiplugin: waiting for callback...");

			/* Wait for server calling back */
			t = time(0)+opt_cbwait;
			do {
				handlemessages();
				if (status != EXIT_OK) {
				   (void) capiconn_listen(ctx, controller, 0, 0);
				   die(status);
				}
			} while (!conn_incoming_connected() && time(0) < t);

                        if (conn_incoming_connected()) {
				add_fd(capi20_fileno(applid));
				setup_timeout();
				return;
			}
			info("capiplugin: callback failed (no call)");
		}
	   }
	} while (++retry < opt_dialmax);

	fatal("capiplugin: callback failed (no call)");
}

/* -------------------------------------------------------------------- */
/* -------- execute a callback ---------------------------------------- */
/* -------------------------------------------------------------------- */

static void makecallback(void)
{
	time_t t;

	t = time(0)+opt_cbdelay;
	do {
	      handlemessages();
	      if (status != EXIT_OK)
		 die(status);
	} while (time(0) < t);

	if (callbacknumbers) 
	   makeconnection(callbacknumbers);
	else makeconnection(numbers);
}

/* -------------------------------------------------------------------- */
/* -------- wait for an incoming call --------------------------------- */
/* -------------------------------------------------------------------- */

static void waitforcall(void)
{
	int try = 0;
	(void) capiconn_listen(ctx, controller, cipmask, 0);
	info("capiplugin: waiting for incoming call ...");

	do {
		handlemessages();
		if (status != EXIT_OK) {
		   (void) capiconn_listen(ctx, controller, 0, 0);
		   die(status);
		}
		if (conn_incoming_inprogress()) try=1;
		if (try && !conn_incoming_inprogress()) {
		   try = 0;
		   if (!conn_incoming_connected()) {
		      (void) capiconn_listen(ctx, controller, cipmask, 0);
		      info("capiplugin: waiting for incoming call ...");
		   }
		}
	} while (!conn_incoming_connected());

        if (conn_incoming_connected()) {
		time_t t = time(0)+opt_connectdelay;
		do {
			handlemessages();
		} while (time(0) < t);
	}
	add_fd(capi20_fileno(applid));
	setup_timeout();
}

/* -------------------------------------------------------------------- */
/* -------- PPPD state change hook ------------------------------------ */
/* -------------------------------------------------------------------- */

static char *phase2str(int phase)
{
	static struct tmpbuf {
		struct tmpbuf *next;
		char           buf[32];
	} buffer[] = {
		{ &buffer[1] },
		{ &buffer[2] },
		{ &buffer[0] },
	};
	static struct tmpbuf *p = &buffer[0];

	switch (phase) {
		case PHASE_DEAD: return "dead";
		case PHASE_INITIALIZE: return "initialize";
		case PHASE_SERIALCONN: return "serialconn";
		case PHASE_DORMANT: return "dormant";
		case PHASE_ESTABLISH: return "establish";
		case PHASE_AUTHENTICATE: return "authenticate";
		case PHASE_CALLBACK: return "callback";
		case PHASE_NETWORK: return "network";
		case PHASE_RUNNING: return "running";
		case PHASE_TERMINATE: return "terminate";
		case PHASE_DISCONNECT: return "disconnect";
		case PHASE_HOLDOFF: return "holdoff";
	}
	p = p->next;
	sprintf(p->buf,"unknown-%d", phase);
	return p->buf;
}

/* -------------------------------------------------------------------- */

static int capi_new_phase_hook(int phase)
{
	if (phase == curphase) {
	   info("capiplugin: phase %s, again.", phase2str(phase));
	   return 0;
	}
        if (curphase != -1) {
           info("capiplugin: phase %s (was %s).",
		phase2str(phase), phase2str(curphase));
	} else {
	   info("capiplugin: phase %s.", phase2str(phase));
	}
	curphase = phase;
	switch (phase) {
		case PHASE_INITIALIZE:
		case PHASE_ESTABLISH:
		case PHASE_AUTHENTICATE:
		case PHASE_CALLBACK:
		case PHASE_NETWORK:
		case PHASE_RUNNING:
		case PHASE_TERMINATE:
		case PHASE_DISCONNECT:
		case PHASE_HOLDOFF:
			break;

		case PHASE_DEAD:
                        disconnectall();
			break;

		case PHASE_DORMANT:
			status = EXIT_OK;
			plugin_check_options();
			init_capiconn();
			if (opt_inmsn || opt_cli) {
			   if (wakeupnow)
                              wakeupdemand();
		           setupincoming_for_demand();
			}
			break;

		case PHASE_SERIALCONN:
			status = EXIT_OK;
		        wakeupnow = 0;
	                if (conn_isconnected(0))
			   break;
			plugin_check_options();
			init_capiconn();
			if (opt_number) {
			     if (coso == COSO_REMOTE) {
				     makeconnection_with_callback();
			     } else {
				     makeconnection(numbers);
			     }
			} else if (opt_channels) {
			     makeleasedline();
			} else {
			     waitforcall();
			}
			break;
	}
	return 0;
}

/* -------------------------------------------------------------------- */
/* -------- init/exit function ---------------------------------------- */
/* -------------------------------------------------------------------- */

#if PPPVER >= PPPVersion(2,4,0,0)

static void plugin_exit(void)
{
	int fd;
	if ((fd = capi20_fileno(applid)) >= 0)
		remove_fd(fd);
	unsetup_timeout();
	disconnectall();
	info("capiplugin: exit");
}

static void phase_notify_func(void *p, int phase)
{
	(void)capi_new_phase_hook(phase);
}

static void exit_notify_func(void *p, int phase)
{
        exiting = 1;
	plugin_exit();
}
#endif

void plugin_init(void)
{
	int serrno;
	int err;

	info("capiplugin: %s", revision);
	info("capiconn: %s", capiconn_version());

	add_options(my_options);

	if ((err = capi20_register (30, 8, 2048, &applid)) != 0) {
		serrno = errno;
		fatal("capiplugin: CAPI_REGISTER failed - %s (0x%04x) [%s (%d)]",
				capi_info2str(err), err,
				strerror(serrno), errno);
		return;
        }
	if (capi20ext_set_flags(applid, 1) < 0) {
		serrno = errno;
		(void)capi20_release(applid);
		fatal("capiplugin: failed to set highjacking mode - %s (%d)",
				strerror(serrno), serrno);
		return;
	}
		
	if ((ctx = capiconn_getcontext(applid, &callbacks)) == 0) {
		(void)capi20_release(applid);
		fatal("capiplugin: get_context failed");
		return;
	}

#if PPPVER >= PPPVersion(2,4,0,0)
        add_notifier(&phasechange, phase_notify_func, 0);
        add_notifier(&exitnotify, exit_notify_func, 0);
#else
        new_phase_hook = capi_new_phase_hook;
#endif
}
