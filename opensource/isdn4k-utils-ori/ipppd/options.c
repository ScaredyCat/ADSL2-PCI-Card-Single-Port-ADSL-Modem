/*
 * options.c - handles option processing for PPP.
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

char options_rcsid[] = "$Id: options.c,v 1.22 2002/07/06 00:34:08 keil Exp $";

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <termios.h>
#include <syslog.h>
#include <string.h>
#include <netdb.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

#ifdef RADIUS
#include <radiusclient.h>
#endif

#include "fsm.h"
#include "ipppd.h"
#include "pathnames.h"
#include "patchlevel.h"
#include "lcp.h"
#include "ipcp.h"
#include "upap.h"
#include "chap.h"
#include "ccp.h"
#include "cbcp.h"
#include "ipxcp.h"

#include <linux/ppp-comp.h>

#ifdef HAVE_LZSCOMP_H
#include <linux/isdn_lzscomp.h>
#else
#include "../ipppcomp/isdn_lzscomp.h"
#endif

#define FALSE	0
#define TRUE	1

#if defined(ultrix) || defined(NeXT)
char *strdup __P((char *));
#endif

#ifndef GIDSET_TYPE
#define GIDSET_TYPE	gid_t
#endif

/*
 * Option variables and default values.
 */

int	debug = 0;		/* Debug flag */
int	kdebugflag = 0;		/* Tell kernel to print debug messages */
int maxconnect = 0;         /* Maximum connect time */

#ifdef INCLUDE_OBSOLETE_FEATURES
int	crtscts = 0;		/* Use hardware flow control */
int	modem = 1;		/* Use modem control lines */
int	inspeed = 0;		/* Input/Output speed requested */
char *connector = NULL;	/* Script to establish physical link */
char *disconnector = NULL;	/* Script to disestablish physical link */
struct option_info connector_info;
struct option_info disconnector_info;
#endif

u_int32_t netmask = 0;		/* IP netmask to set on interface */
int	lockflag = 0;		/* Create lock file to lock the serial dev */
int	nodetach = 0;		/* Don't detach from controlling tty */
char user[MAXNAMELEN];	/* Username for PAP */
char passwd[MAXSECRETLEN];	/* Password for PAP */
int	ask_passwd = 0;		/* Ask user for password */
int	fdpasswd = 0;		/* Password via filedescriptor */
int	auth_required = 0;	/* Peer is required to authenticate */
int	defaultroute = 0;	/* assign default route through interface */
int hostroute = 1;
int	uselogin = 0;		/* Use /etc/passwd for checking PAP */
#ifdef RADIUS
int     useradius = 0;          /* Use RADIUS server checking PAP */
int     useradacct = 0;         /* Use RADIUS server accounting */
extern char radius_user[] ;
extern int called_radius_init ;
extern int auth_order ;

#endif
int	lcp_echo_interval = 0;	/* Interval between LCP echo-requests */
int	lcp_echo_fails = 0;	/* Tolerance to unanswered echo-requests */
char our_name[MAXNAMELEN];	/* Our name for authentication purposes */
char remote_name[MAXNAMELEN]; /* Peer's name for authentication */
int	usehostname = 0;	/* Use hostname for our_name */
int	disable_defaultip = 0;	/* Don't use hostname for default IP adrs */
char *ipparam = NULL;	/* Extra parameter for ip up/down scripts */
int	cryptpap;		/* Passwords in pap-secrets are encrypted */
int useifip=0;		/* try to get IP addresses from interface */
int deldefaultroute=0;  /* delete default route, if it exists */
int usefirstip=0;
int useifmtu=0;		/* get MTU value from network device */

int idle_time_limit = 0;    /* Disconnect if idle for this many seconds */
#ifdef RADIUS
int default_idle_time_limit = 0 ;
int session_time_limit = 0;
int default_session_time_limit = 0;
extern u_int32_t default_hisaddr ;
extern u_int32_t default_ouraddr ;
extern u_int32_t default_netmask ;
#endif

int holdoff = 30;           /* # seconds to pause before reconnecting */
int refuse_pap = 0;         /* Set to say we won't do PAP */
int refuse_chap = 0;        /* Set to say we won't do CHAP */

int log_raw_password = 0;
int force_driver = 0;

struct option_info auth_req_info;
struct option_info devnam_info;


/*
 * Prototypes
 */
static int setipaddr __P((int,char *));
static int setdebug __P((int));
static int setkdebug __P((int,char **));
static int setpassive __P((int));
static int setsilent __P((int));
static int noopt __P((int));
static int setnovj __P((int));
static int setnovjccomp __P((int));
static int setvjslots __P((int,char **));
static int reqpap __P((int));
static int nopap __P((int));
#ifdef OLD_OPTIONS
static int setupapfile __P((imt.char **));
#endif
static int nochap __P((int));
static int reqchap __P((int));
static int noaccomp __P((int));
static int noip __P((int));
static int nomagicnumber __P((int));
static int setmru __P((int,char **));
static int setmtu __P((int,char **));
static int setcallbackdelay __P((int,char **));
static int setcallbackcbcp __P((int));
static int setcallbacknocbcp __P((int));
static int setcallbackrfc __P((int));
static int setcallbacknorfc __P((int));
static int setcallbackcbcpfirst __P((int));
static int setcallbackcbcplast __P((int));
static int setcallbacktype __P((int,char **));
static int setcallback __P((int,char **));
static int setifmtu __P((int));
static int nomru __P((int));
static int nopcomp __P((int));
static int setipaddr __P((int,char *));
static int setdevname __P((char *,int));
static int setdomain __P((int,char **));
static int setnetmask __P((int,char **));
static int setnodetach __P((int));
static int setmaxconnect __P((int,char **));

#ifdef INCLUDE_OBSOLETE_FEATURES
static int setspeed __P((int,char *));
static int noasyncmap __P((int));
static int setescape __P((int,char **));
static int setasyncmap __P((int,char **));
static int setdisconnector __P((int,char **));
static int setconnector __P((int,char **));
static int setcrtscts __P((int));
static int setnocrtscts __P((int));
static int setxonxoff __P((int));
static int setmodem __P((int));
static int setlocal __P((int));
#endif

static int setlock __P((int));
static int setname __P((int,char **));
static int setuser __P((int,char **));
static int setremote __P((int,char **));
static int setauth __P((int));
static int setaskpw __P((int));
static int unsetaskpw __P((int));
static int setfdpw __P((int,char **));
static int setnoauth __P((int));
static int readfile __P((int,char **));
static int pidfile __P((int,char **));
static int callfile __P((int,char **));
static int setdefaultroute __P((int));
static int setnodefaultroute __P((int));
static int setproxyarp __P((int));
static int setnoproxyarp __P((int));
static int setdologin __P((int));
#ifdef RADIUS
static int setdoradius __P((int));
static int setdoradacct __P((int));
#endif
static int setusehostname __P((int));
static int setnoipdflt __P((int));
static int setlcptimeout __P((int,char **));
static int setlcpterm __P((int,char **));
static int setlcpconf __P((int,char **));
static int setlcpfails __P((int,char **));
static int setipcptimeout __P((int,char **));
static int setipcpterm __P((int,char **));
static int setipcpconf __P((int,char **));
static int setipcpfails __P((int,char **));
static int setpaptimeout __P((int,char **));
static int setpapreqs __P((int,char **));
static int setpapreqtime __P((int,char **));
static int setchaptimeout __P((int,char **));
static int setchapchal __P((int,char **));
static int setchapintv __P((int,char **));
static int setipcpaccl __P((int));
static int setipcpaccr __P((int));
static int setlcpechointv __P((int,char **));
static int setlcpechofails __P((int,char **));
static int setbsdcomp __P((int,char **));
static int noccp __P((int));
static int setnobsdcomp __P((int));
static int setlzs __P((int,char **));
static int setnolzs __P((int));
static int setdeflate __P((int,char **));
static int setnodeflate __P((int));
static int setpred1comp __P((int));
static int setnopred1comp __P((int));
static int setipparam __P((int,char **));
static int setpapcrypt __P((int));
static int setidle __P((int,char **));
#ifdef RADIUS
static int setsessionlimit __P((int,char **));
#endif
static int setholdoff __P((int,char **));
static int setdnsaddr __P((int,char **));
static int setgetdnsaddr __P((int,char **));
static int setwinsaddr __P((int,char **));
static int setgetwinsaddr __P((int,char **));
static int resetipxproto __P((int));
static int setuseifip __P((int));
static int setdeldefaultroute __P((int));
static int setusefirstip __P((int));
static int setmp __P((int));
static int setpwlog __P((int));

static int setipxproto __P((int));
static int setipxanet __P((int));
static int setipxalcl __P((int));
static int setipxarmt __P((int));
static int setipxnetwork __P((int,char **));
static int setipxnode __P((int,char **));
static int setipxrouter __P((int,char **));
static int setipxname __P((int,char **));
static int setipxcptimeout __P((int,char **));
static int setipxcpterm __P((int,char **));
static int setipxcpconf __P((int,char **));
static int setipxcpfails __P((int,char **));

static int sethostroute __P((int));
static int setnohostroute __P((int));

static int number_option __P((char *, u_int32_t *, int));
static int int_option __P((char *, int *));
static int readable __P((int));
static int setforcedriver(int dummy);

#ifdef RADIUS
char *make_username_realm ( char * );
int __P (radius_init ( void ));
#endif

/*
 * Valid arguments.
 */
static struct cmd {
    char *cmd_name;
    int num_args;
    int (*cmd_func)();
} cmds[] = {
    {"-all", 0, noopt},		/* Don't request/allow any options (useless) */
	{"noaccomp", 0, noaccomp}, /* Disable Address/Control compression */
    {"-ac", 0, noaccomp},	/* Disable Address/Control compress */
    {"-d", 0, setdebug},	/* Increase debugging level */
	{"nodetach", 0, setnodetach}, /* Don't detach from controlling tty */
    {"-detach", 0, setnodetach}, /* don't fork */
	{"noip", 0, noip},         /* Disable IP and IPCP */
	{"-ip", 0, noip},          /* Disable IP and IPCP */
	{"-ip-protocol", 0, noip},          /* Disable IP and IPCP */
	{"nomagic", 0, nomagicnumber}, /* Disable magic number negotiation */
    {"-mn", 0, nomagicnumber},	/* Disable magic number negotiation */
	{"default-mru", 0, nomru}, /* Disable MRU negotiation */
    {"-mru", 0, nomru},		/* Disable mru negotiation */
    {"-p", 0, setpassive},	/* Set passive mode */
	{"nopcomp", 0, nopcomp},   /* Disable protocol field compression */
    {"-pc", 0, nopcomp},	/* Disable protocol field compress */
#if OLD_OPTIONS
    {"+ua", 1, setupapfile},	/* Get PAP user and password from file */
#endif
	{"require-pap", 0, reqpap},        /* Require PAP authentication from peer */
    {"+pap", 0, reqpap},	/* Require PAP auth from peer */
	{"refuse-pap", 0, nopap},  /* Don't agree to auth to peer with PAP */
    {"-pap", 0, nopap},		/* Don't allow UPAP authentication with peer */
	{"require-chap", 0, reqchap}, /* Require CHAP authentication from peer */
    {"+chap", 0, reqchap},	/* Require CHAP authentication from peer */
	{"refuse-chap", 0, nochap},        /* Don't agree to auth to peer with CHAP */
    {"-chap", 0, nochap},	/* Don't allow CHAP authentication with peer */
	{"novj", 0, setnovj},      /* Disable VJ compression */
    {"-vj", 0, setnovj},	/* disable VJ compression */
	{"novjccomp", 0, setnovjccomp}, /* disable VJ connection-ID compression */
    {"-vjccomp", 0, setnovjccomp}, /* disable VJ connection-ID compression */
    {"vj-max-slots", 1, setvjslots}, /* Set maximum VJ header slots */
    {"maxconnect", 1, setmaxconnect},  /* specify a maximum connect time */
#ifdef INCLUDE_OBSOLETE_FEATURES
    {"asyncmap", 1, setasyncmap}, /* set the desired async map */
    {"escape", 1, setescape},	/* set chars to escape on transmission */
    {"-am", 0, noasyncmap},	/* Disable asyncmap negotiation */
    {"-as", 1, setasyncmap},	/* set the desired async map */
	{"default-asyncmap", 0, noasyncmap}, /* Disable asyncmap negoatiation */
    {"disconnect", 1, setdisconnector},	/* program to disconnect serial dev. */
    {"connect", 1, setconnector}, /* A program to set up a connection */
    {"crtscts", 0, setcrtscts},        /* set h/w flow control */
    {"nocrtscts", 0, setnocrtscts}, /* clear h/w flow control */
    {"-crtscts", 0, setnocrtscts}, /* clear h/w flow control */
    {"xonxoff", 0, setxonxoff},	/* set s/w flow control */
    {"modem", 0, setmodem},	/* Use modem control lines */
    {"local", 0, setlocal},	/* Don't use modem control lines */
#endif
    {"debug", 0, setdebug},	/* Increase debugging level */
    {"kdebug", 1, setkdebug},	/* Enable kernel-level debugging */
    {"domain", 1, setdomain},	/* Add given domain name to hostname*/
    {"mru", 1, setmru},		/* Set MRU value for negotiation */
    {"mtu", 1, setmtu},		/* Set our MTU */
    {"callback", 1, setcallback},  /* Ask for callback */
    {"callback-delay", 1, setcallbackdelay},  /* Callback delay for CBCP */
    {"callback-cbcp", 0, setcallbackcbcp},  /* Enable CBCP callback negotiation */
    {"callback-rfc1570", 0, setcallbackrfc},  /* Enable RCFC 1570 style callback negotiation */
    {"-callback-cbcp", 0, setcallbacknocbcp},  /* Disable CBCP callbacks */
    {"-callback-rfc1570", 0, setcallbacknorfc},  /* Disable RCFC 1570 style callbacks */
    {"no-callback-cbcp", 0, setcallbacknocbcp},  /* Disable CBCP callbacks */
    {"no-callback-rfc1570", 0, setcallbacknorfc},  /* Disable RCFC 1570 style callbacks */
    {"callback-type", 1, setcallbacktype},  /* Callback type for RFC 1570 style callbacks */
    {"callback-cbcp-preferred", 0, setcallbackcbcpfirst},  /* Prefer CBCP callback negotiation */
    {"callback-rfc1570-preferred", 0, setcallbackcbcplast},  /* Prefer RFC 1570 callback negotiation */
    {"useifmtu", 0, setifmtu},  /* get MTU value from attached network device */
    {"netmask", 1, setnetmask},	/* set netmask */
    {"passive", 0, setpassive},	/* Set passive mode */
    {"silent", 0, setsilent},	/* Set silent mode */
    {"lock", 0, setlock},	/* Lock serial device (with lock file) */
    {"name", 1, setname},	/* Set local name for authentication */
    {"user", 1, setuser},      /* Set name for auth with peer */
    {"usehostname", 0, setusehostname},	/* Must use hostname for auth. */
    {"remotename", 1, setremote}, /* Set remote name for authentication */
    {"askpassword", 0, setaskpw}, /* Ask user password on start */
    {"noaskpassword", 0, unsetaskpw}, /* Ask user password on start */
    {"-askpassword", 0, unsetaskpw}, /* Ask user password on start */
    {"passwordfd", 1, setfdpw}, /* Read password from fd */
    {"auth", 0, setauth},	/* Require authentication from peer */
    {"noauth", 0, setnoauth},  /* Don't require peer to authenticate */
    {"file", 1, readfile},	/* Take options from a file */
	{"pidfile",1,pidfile},
    {"call", 1, callfile},     /* Take options from a privileged file */
    {"defaultroute", 0, setdefaultroute}, /* Add default route */
    {"nodefaultroute", 0, setnodefaultroute}, /* disable defaultroute option */
    {"-defaultroute", 0, setnodefaultroute}, /* disable defaultroute option */
    {"proxyarp", 0, setproxyarp}, /* Add proxy ARP entry */
    {"noproxyarp", 0, setnoproxyarp}, /* disable proxyarp option */
    {"-proxyarp", 0, setnoproxyarp}, /* disable proxyarp option */
    {"login", 0, setdologin},	/* Use system password database for UPAP */
#ifdef RADIUS
    {"radius", 0, setdoradius},   /* Use RADIUS server for UPAP */
    {"radacct", 0, setdoradacct}, /* Use RADIUS server for accounting */
#endif
    {"noipdefault", 0, setnoipdflt}, /* Don't use name for default IP adrs */
    {"lcp-echo-failure", 1, setlcpechofails}, /* consecutive echo failures */
    {"lcp-echo-interval", 1, setlcpechointv}, /* time for lcp echo events */
    {"lcp-restart", 1, setlcptimeout}, /* Set timeout for LCP */
    {"lcp-max-terminate", 1, setlcpterm}, /* Set max #xmits for term-reqs */
    {"lcp-max-configure", 1, setlcpconf}, /* Set max #xmits for conf-reqs */
    {"lcp-max-failure", 1, setlcpfails}, /* Set max #conf-naks for LCP */
    {"ipcp-restart", 1, setipcptimeout}, /* Set timeout for IPCP */
    {"ipcp-max-terminate", 1, setipcpterm}, /* Set max #xmits for term-reqs */
    {"ipcp-max-configure", 1, setipcpconf}, /* Set max #xmits for conf-reqs */
    {"ipcp-max-failure", 1, setipcpfails}, /* Set max #conf-naks for IPCP */
    {"pap-restart", 1, setpaptimeout},	/* Set retransmit timeout for PAP */
    {"pap-max-authreq", 1, setpapreqs}, /* Set max #xmits for auth-reqs */
    {"pap-timeout", 1, setpapreqtime},	/* Set time limit for peer PAP auth. */
    {"chap-restart", 1, setchaptimeout}, /* Set timeout for CHAP */
    {"chap-max-challenge", 1, setchapchal}, /* Set max #xmits for challenge */
    {"chap-interval", 1, setchapintv}, /* Set interval for rechallenge */
    {"ipcp-accept-local", 0, setipcpaccl}, /* Accept peer's address for us */
    {"ipcp-accept-remote", 0, setipcpaccr}, /* Accept peer's address for it */
    {"noccp", 0, noccp},        /* Disable CCP negotiation */
    {"-ccp", 0, noccp},         /* Disable CCP negotiation */
    {"bsdcomp", 1, setbsdcomp},		/* request BSD-Compress */
    {"nobsdcomp", 0, setnobsdcomp},    /* don't allow BSD-Compress */
    {"-bsdcomp", 0, setnobsdcomp},	/* don't allow BSD-Compress */
    {"lzs", 1, setlzs},                /* request LZS Compression */
    {"nolzs", 0, setnolzs},    /* disable LZS Compression */
    {"-lzs", 0, setnolzs},     /* disable LZS Compression */
    {"deflate", 1, setdeflate},                /* request Deflate compression */
    {"nodeflate", 0, setnodeflate},    /* don't allow Deflate compression */
    {"-deflate", 0, setnodeflate},     /* don't allow Deflate compression */
    {"predictor1", 0, setpred1comp},   /* request Predictor-1 */
    {"nopredictor1", 0, setnopred1comp},/* don't allow Predictor-1 */
    {"-predictor1", 0, setnopred1comp},        /* don't allow Predictor-1 */
    {"ipparam", 1, setipparam},		/* set ip script parameter */
    {"papcrypt", 0, setpapcrypt},	/* PAP passwords encrypted */
    {"idle", 1, setidle},              /* idle time limit (seconds) */
#ifdef RADIUS
    {"session-limit", 1, setsessionlimit}, /* seconds for disconnect sessions */
#endif
    {"holdoff", 1, setholdoff},                /* set holdoff time (seconds) */
    {"ms-dns", 1, setdnsaddr},         /* DNS address for the peer's use */
    {"ms-wins", 1, setwinsaddr},         /* WINS address for the peer's use */
    {"ms-get-dns", 0, setgetdnsaddr},  /* DNS address for the my use */
    {"usepeerdns", 0, setgetdnsaddr},      /* for compatibility with async pppd */
    {"ms-get-wins", 0, setgetwinsaddr},    /* Nameserver for SMB over TCP/IP for me */
    {"noipx",  0, resetipxproto},      /* Disable IPXCP (and IPX) */
    {"-ipx",   0, resetipxproto},      /* Disable IPXCP (and IPX) */

    {"useifip",0,setuseifip},            /* call setifip() for IP addrs */
    {"deldefaultroute",0, setdeldefaultroute}, /* call setdeldefaultroute for defaultroute */
    {"usefirstip",0,setusefirstip}, /* use first IP from auth file for remote */
    {"+mp",0,setmp},
    {"+pwlog",0,setpwlog},

    {"ipx-network",          1, setipxnetwork}, /* IPX network number */
    {"ipxcp-accept-network", 0, setipxanet},    /* Accept peer netowrk */
    {"ipx-node",             1, setipxnode},    /* IPX node number */
    {"ipxcp-accept-local",   0, setipxalcl},    /* Accept our address */
    {"ipxcp-accept-remote",  0, setipxarmt},    /* Accept peer's address */
    {"ipx-routing",          1, setipxrouter},  /* IPX routing proto number */
    {"ipx-router-name",      1, setipxname},    /* IPX router name */
    {"ipxcp-restart",        1, setipxcptimeout}, /* Set timeout for IPXCP */
    {"ipxcp-max-terminate",  1, setipxcpterm},  /* max #xmits for term-reqs */
    {"ipxcp-max-configure",  1, setipxcpconf},  /* max #xmits for conf-reqs */
    {"ipxcp-max-failure",    1, setipxcpfails}, /* max #conf-naks for IPXCP */
#if 0
    {"ipx-compression", 1, setipxcompression}, /* IPX compression number */
#endif
    {"ipx",                 0, setipxproto},   /* Enable IPXCP (and IPX) */
    {"+ipx",                0, setipxproto},   /* Enable IPXCP (and IPX) */

#ifdef __linux__
    {"hostroute", 0, sethostroute}, /* Add host route (default) */
    {"-hostroute", 0, setnohostroute}, /* Don't add host route */
    {"nohostroute", 0, setnohostroute}, /* Don't add host route */
#endif
    {"+force-driver",0,setforcedriver},

    {NULL, 0, NULL}
};


#ifndef IMPLEMENTATION
#define IMPLEMENTATION ""
#endif

static char *usage_string = "\
ipppd version %s patch level %d%s\n\
Usage: %s [ options ], where options are:\n\
\t<device>	Communicate over the named device\n\
#ifdef INCLUDE_OBSOLETE_FEATURES
\tcrtscts		Use hardware RTS/CTS flow control\n\
\t<speed>		Set the baud rate to <speed>\n\
\tmodem		Use modem control lines\n\
#endif
\t<loc>:<rem>	Set the local and/or remote interface IP\n\
\t\taddresses.  (you also may use the option 'useifip' to get IPs).\n\
\tasyncmap <n>	Set the desired async map to hex <n>\n\
\tauth		Require authentication from peer\n\
\tconnect <p>     Invoke shell command <p> to set up the serial line\n\
\tdefaultroute	Add default route through interface\n\
\tfile <f>	Take options from file <f>\n\
\tmru <n>		Set MRU value to <n> for negotiation\n\
\tnetmask <n>	Set interface netmask to <n>\n\
See ipppd(8) for more options.\n\
";

static char *current_option;   /* the name of the option being parsed */

/*
 * parse_args - parse a string of arguments from the command line.
 */
int parse_args(int argc,char **argv)
{
    char *arg;
    struct cmd *cmdp;
    int ret;
    int slot = 0;

    while (argc > 0) {
	arg = *argv++;
	--argc;

	/*
	 * First see if it's a command.
	 */
	for (cmdp = cmds; cmdp->cmd_name; cmdp++)
	    if (!strcmp(arg, cmdp->cmd_name))
		break;

	if (cmdp->cmd_name != NULL) {
		if (argc < cmdp->num_args) {
			option_error("too few parameters for option %s", arg);
			return 0;
		}
		current_option = arg;
	    if (!(*cmdp->cmd_func)(slot,argv))
			return 0;
	    argc -= cmdp->num_args;
	    argv += cmdp->num_args;

	} else {
	    /*
	     * Maybe a tty name, speed or IP address?
	     */
            if( (ret = setdevname(arg,numdev)) )
            {
              if(ret < 0)
                return 0;
              numdev++;
            }
	    else if (
#ifdef INCLUDE_OBSOLETE_FEATURES
(ret = setspeed(slot,arg)) == 0 &&
#endif
		(ret = setipaddr(slot,arg)) == 0) {
		option_error("unrecognized option '%s'", arg);
		usage();
		return 0;
	    }
	    if (ret < 0)	/* error */
		return 0;
	}
    }

    return 1;
}


void make_options_global(int slot)
{
    int i;

    for(i=0;i<NUM_PPP;i++)
    {
      if(i == slot)
        continue;
      lcp_wantoptions[i]   = lcp_wantoptions[slot];
      lcp_allowoptions[i]  = lcp_allowoptions[slot];
      ipcp_wantoptions[i]  = ipcp_wantoptions[slot];
      ipcp_allowoptions[i] = ipcp_allowoptions[slot];
      ccp_wantoptions[i]   = ccp_wantoptions[slot];
      ccp_allowoptions[i]  = ccp_allowoptions[slot];
      ipxcp_wantoptions[i]  = ipxcp_wantoptions[slot];
      ipxcp_allowoptions[i] = ipxcp_allowoptions[slot];
      ipxcp_fsm[i] = ipxcp_fsm[slot];
      lcp_fsm[i] = lcp_fsm[slot];
      ipcp_fsm[i] = ipcp_fsm[slot];
      ccp_fsm[i] = ccp_fsm[slot];
      chap[i] = chap[slot];
      upap[i] = upap[slot];
      cbcp[i] = cbcp[slot];

      memcpy(xmit_accm[i],xmit_accm[slot],sizeof(xmit_accm[0]));
    }
}

 /*
 * scan_args - scan the command line arguments to get the tty name,
 * if specified.
 */
void scan_args(int argc,char **argv)
{
    char *arg;
    struct cmd *cmdp;

    while (argc > 0) {
       arg = *argv++;
       --argc;

       /* Skip options and their arguments */
       for (cmdp = cmds; cmdp->cmd_name; cmdp++)
           if (!strcmp(arg, cmdp->cmd_name))
               break;

       if (cmdp->cmd_name != NULL) {
           argc -= cmdp->num_args;
           argv += cmdp->num_args;
           continue;
       }

       /* Check if it's a tty name and copy it if so */
       (void) setdevname(arg, 1);
    }
}

/*
 * usage - print out a message telling how to use the program.
 */
void usage(void)
{
    fprintf(stderr, usage_string, VERSION, PATCHLEVEL, IMPLEMENTATION, progname);
}

/*
 * options_from_file - Read a string of options from a file,
 * and interpret them.
 */
int options_from_file(char *filename,int must_exist,int check_prot , int slot)
{
    FILE *f;
    int i, newline, ret;
    struct cmd *cmdp;
    char *argv[MAXARGS];
    char args[MAXARGS][MAXWORDLEN];
    char cmd[MAXWORDLEN];

    if ((f = fopen(filename, "r")) == NULL) {
	if (!must_exist && errno == ENOENT)
	    return 1;
	option_error("Can't open options file %s: %m", filename);
	return 0;
    }
    if (check_prot && !readable(fileno(f))) {
	option_error("Can't open options file %s: access denied", filename);
	fclose(f);
	return 0;
    }

    while (getword(f, cmd, &newline, filename)) {
	/*
	 * First see if it's a command.
	 */
	for (cmdp = cmds; cmdp->cmd_name; cmdp++)
	    if (!strcmp(cmd, cmdp->cmd_name))
		break;

	if (cmdp->cmd_name != NULL) {
	    for (i = 0; i < cmdp->num_args; ++i) {
		if (!getword(f, args[i], &newline, filename)) {
		    fprintf(stderr,
			    "In file %s: too few parameters for command %s\n",
			    filename, cmd);
		    fclose(f);
		    return 0;
		}
		argv[i] = args[i];
	    }
	    if (!(*cmdp->cmd_func)(slot,argv)) {
		fclose(f);
		return 0;
	    }

	} else {
	    /*
	     * Maybe a tty name, speed or IP address?
	     */

	    if((ret = setdevname(cmd,numdev)))
            {
              if(ret < 0)
                return 0;
              numdev++;
            }
            else if(
#ifdef INCLUDE_OBSOLETE_FEATURES
(ret = setspeed(slot,cmd)) == 0 &&
#endif
		(ret = setipaddr(slot,cmd)) == 0) {
		fprintf(stderr, "In file %s: unrecognized command %s\n",
			filename, cmd);
		fclose(f);
		return 0;
	    }
	    if (ret < 0)	/* error */
		return 0;
	}
    }
    return 1;
}

/*
 * options_for_tty - See if an options file exists for the serial
 * device, and if so, interpret options from it.
 * You can enable this but then you should know what you do and
 * how configuration works!!!!!!
 */
int options_for_tty()
{
#if 0
    char *dev, *path;
    int ret,i;

    for(i=0;i<numdev;i++) {
      int slot;
#if 0
      slot = i;
#else
      slot = 0;
#endif
      dev = strrchr(lns[i].devnam, '/');
      if (dev == NULL) {
        syslog(LOG_NOTICE,"oops: strange device name %s",lns[i].devnam);
	continue;
      }
      ++dev;
      path = malloc(strlen(_PATH_TTYOPT) + strlen(dev) + 1);
      if (path == NULL)
	novm("tty init file name");
      strcpy(path, _PATH_TTYOPT);
      strcat(path, dev);

      ret = options_from_file(path, 0, 0 , slot );
      free(path);
    }
#endif
    return 1;
}

 /*
 * option_error - print a message about an error in an option.
 * The message is logged, and also sent to
 * stderr if phase == PHASE_INITIALIZE.
 */
void option_error(char *fmt, ...)
{
    va_list args;
    char buf[256];

#if __STDC__
    va_start(args, fmt);
#else
    char *fmt;
    va_start(args);
    fmt = va_arg(args, char *);
#endif
    vfmtmsg(buf, sizeof(buf), fmt, args);
    va_end(args);
    fprintf(stderr, "%s: %s\n", progname, buf);
    syslog(LOG_ERR, "%s", buf);
}

/*
 * readable - check if a file is readable by the real user.
 */
static int
readable(int lfd)
{
    uid_t uid;
    int ngroups, i;
    struct stat sbuf;
    GIDSET_TYPE groups[NGROUPS_MAX];

    uid = getuid();
    if (uid == 0)
	return 1;
    if (fstat(lfd, &sbuf) != 0)
	return 0;
    if (sbuf.st_uid == uid)
	return sbuf.st_mode & S_IRUSR;
    if (sbuf.st_gid == getgid())
	return sbuf.st_mode & S_IRGRP;
    ngroups = getgroups(NGROUPS_MAX, groups);
    for (i = 0; i < ngroups; ++i)
	if (sbuf.st_gid == groups[i])
	    return sbuf.st_mode & S_IRGRP;
    return sbuf.st_mode & S_IROTH;
}

/*
 * Read a word from a file.
 * Words are delimited by white-space or by quotes (" or ').
 * Quotes, white-space and \ may be escaped with \.
 * \<newline> is ignored.
 */

int getword(FILE *f,char *word,int *newlinep,char *filename)
{
    int c, len, escape;
    int quoted, comment;
    int value, digit, got, n;

#define isoctal(c) ((c) >= '0' && (c) < '8')

    *newlinep = 0;
    len = 0;
    escape = 0;
    comment = 0;

    /*
     * First skip white-space and comments.
     */
    for (;;) {
	c = getc(f);
	if (c == EOF)
	    break;

	/*
	 * A newline means the end of a comment; backslash-newline
	 * is ignored.  Note that we cannot have escape && comment.
	 */
	if (c == '\n') {
	    if (!escape) {
		*newlinep = 1;
		comment = 0;
	    } else
		escape = 0;
	    continue;
	}

	/*
	 * Ignore characters other than newline in a comment.
	 */
	if (comment)
	    continue;

	/*
	 * If this character is escaped, we have a word start.
	 */
	if (escape)
	    break;

	/*
	 * If this is the escape character, look at the next character.
	 */
	if (c == '\\') {
	    escape = 1;
	    continue;
	}

	/*
	 * If this is the start of a comment, ignore the rest of the line.
	 */
	if (c == '#') {
	    comment = 1;
	    continue;
	}

	/*
	 * A non-whitespace character is the start of a word.
	 */
	if (!isspace(c))
	    break;
    }

    /*
     * Save the delimiter for quoted strings.
     */
    if (!escape && (c == '"' || c == '\'')) {
        quoted = c;
	c = getc(f);
    } else
        quoted = 0;

    /*
     * Process characters until the end of the word.
     */
    while (c != EOF) {
	if (escape) {
	    /*
	     * This character is escaped: backslash-newline is ignored,
	     * various other characters indicate particular values
	     * as for C backslash-escapes.
	     */
	    escape = 0;
	    if (c == '\n') {
	        c = getc(f);
		continue;
	    }

	    got = 0;
	    switch (c) {
	    case 'a':
		value = '\a';
		break;
	    case 'b':
		value = '\b';
		break;
	    case 'f':
		value = '\f';
		break;
	    case 'n':
		value = '\n';
		break;
	    case 'r':
		value = '\r';
		break;
	    case 's':
		value = ' ';
		break;
	    case 't':
		value = '\t';
		break;

	    default:
		if (isoctal(c)) {
		    /*
		     * \ddd octal sequence
		     */
		    value = 0;
		    for (n = 0; n < 3 && isoctal(c); ++n) {
			value = (value << 3) + (c & 07);
			c = getc(f);
		    }
		    got = 1;
		    break;
		}

		if (c == 'x') {
		    /*
		     * \x<hex_string> sequence
		     */
		    value = 0;
		    c = getc(f);
		    for (n = 0; n < 2 && isxdigit(c); ++n) {
			digit = toupper(c) - '0';
			if (digit > 10)
			    digit += '0' + 10 - 'A';
			value = (value << 4) + digit;
			c = getc (f);
		    }
		    got = 1;
		    break;
		}

		/*
		 * Otherwise the character stands for itself.
		 */
		value = c;
		break;
	    }

	    /*
	     * Store the resulting character for the escape sequence.
	     */
	    if (len < MAXWORDLEN-1)
		word[len] = value;
	    ++len;

	    if (!got)
		c = getc(f);
	    continue;

	}

	/*
	 * Not escaped: see if we've reached the end of the word.
	 */
	if (quoted) {
	    if (c == quoted)
		break;
	} else {
	    if (isspace(c) || c == '#') {
		ungetc (c, f);
		break;
	    }
	}

	/*
	 * Backslash starts an escape sequence.
	 */
	if (c == '\\') {
	    escape = 1;
	    c = getc(f);
	    continue;
	}

	/*
	 * An ordinary character: store it in the word and get another.
	 */
	if (len < MAXWORDLEN-1)
	    word[len] = c;
	++len;

	c = getc(f);
    }

    /*
     * End of the word: check for errors.
     */
    if (c == EOF) {
	if (ferror(f)) {
	    if (errno == 0)
		errno = EIO;
	    perror(filename);
	    die(1);
	}
	/*
	 * If len is zero, then we didn't find a word before the
	 * end of the file.
	 */
	if (len == 0)
	    return 0;
    }

    /*
     * Warn if the word was too long, and append a terminating null.
     */
    if (len >= MAXWORDLEN) {
	fprintf(stderr, "%s: warning: word in file %s too long (%.20s...)\n",
		progname, filename, word);
	len = MAXWORDLEN - 1;
    }
    word[len] = 0;

    return 1;

#undef isoctal

}

/*
 * number_option - parse an unsigned numeric parameter for an option.
 */
static int
number_option(str, valp, base)
    char *str;
    u_int32_t *valp;
    int base;
{
    char *ptr;

    *valp = strtoul(str, &ptr, base);
    if (ptr == str) {
	fprintf(stderr, "%s: invalid number: %s\n", progname, str);
	return 0;
    }
    return 1;
}


/*
 * int_option - like number_option, but valp is int *,
 * the base is assumed to be 0, and *valp is not changed
 * if there is an error.
 */
static int
int_option(str, valp)
    char *str;
    int *valp;
{
    u_int32_t v;

    if (!number_option(str, &v, 0))
	return 0;
    *valp = (int) v;
    return 1;
}


/*
 * The following procedures execute commands.
 */

static int pidfile(int slot,char **argv)
{
	strncpy(pidfilename,argv[0],MAXPATHLEN-1);
    pidfilename[MAXPATHLEN-1] = 0;
	return 1;
}

/*
 * readfile - take commands from a file.
 */
static int readfile(int slot,char **argv)
{
    return options_from_file(*argv, 1, 1 , slot);
}

/*
 * callfile - take commands from /etc/ppp/peers/<name>.
 * Name may not contain /../, start with / or ../, or end in /..
 */
static int callfile(int slot,char **argv)
{
    char *fname, *arg, *p;
    int l, ok;

    arg = *argv;
    ok = 1;
    if (arg[0] == '/' || arg[0] == 0)
       ok = 0;
    else {
       for (p = arg; *p != 0; ) {
           if (p[0] == '.' && p[1] == '.' && (p[2] == '/' || p[2] == 0)) {
               ok = 0;
               break;
           }
           while (*p != '/' && *p != 0)
               ++p;
           if (*p == '/')
               ++p;
       }
    }
    if (!ok) {
       option_error("call option value may not contain .. or start with /");
       return 0;
    }

    l = strlen(arg) + strlen(_PATH_PEERFILES) + 1;
    if ((fname = (char *) malloc(l)) == NULL)
       novm("call file name");
    strcpy(fname, _PATH_PEERFILES);
    strcat(fname, arg);

    ok = options_from_file(fname, 1, 1, slot);

    free(fname);
    return ok;
}

/*
 * setdebug - Set debug (command line argument).
 */
static int
setdebug(int slot)
{
    debug++;
    return (1);
}

/*
 * setkdebug - Set kernel debugging level.
 */
static int setkdebug(int slot,char **argv)
{
    return int_option(*argv, &kdebugflag);
}

/*
 * noopt - Disable all options.
 */
static int noopt(int slot)
{
    BZERO((char *) &lcp_wantoptions[slot], sizeof (struct lcp_options));
    BZERO((char *) &lcp_allowoptions[slot], sizeof (struct lcp_options));
    BZERO((char *) &ipcp_wantoptions[slot], sizeof (struct ipcp_options));
    BZERO((char *) &ipcp_allowoptions[slot], sizeof (struct ipcp_options));
    BZERO((char *) &ipxcp_wantoptions[slot], sizeof (struct ipxcp_options));
    BZERO((char *) &ipxcp_allowoptions[slot], sizeof (struct ipxcp_options));

    return (1);
}

/*
 * noaccomp - Disable Address/Control field compression negotiation.
 */
static int noaccomp(int slot)
{
    lcp_wantoptions[slot].neg_accompression = 0;
    lcp_allowoptions[slot].neg_accompression = 0;
    return (1);
}

/*
 * noip - Disable IP and IPCP .
 */
static int noip(int slot)
{
	ipcp_protent.enabled_flag = 0;
    return (1);
}

/*
 * nomagicnumber - Disable magic number negotiation.
 */
static int nomagicnumber(int slot)
{
    lcp_wantoptions[slot].neg_magicnumber = 0;
    lcp_allowoptions[slot].neg_magicnumber = 0;
    return (1);
}

/*
 * nomru - Disable mru negotiation.
 */
static int nomru(int slot)
{
    lcp_wantoptions[slot].neg_mru = 0;
    lcp_allowoptions[slot].neg_mru = 0;
    return (1);
}


/*
 * setmru - Set MRU for negotiation.
 */
static int setmru(int slot,char **argv)
{
    u_int32_t mru;

    if (!number_option(*argv, &mru, 0))
	return 0;
    lcp_wantoptions[slot].mru = mru;
    lcp_wantoptions[slot].neg_mru = 1;
    return (1);
}


/*
 * setmru - Set the largest MTU we'll use.
 */
static int setmtu(int slot,char **argv)
{
    u_int32_t mtu;

    if (!number_option(*argv, &mtu, 0))
	return 0;
    if (mtu < MINMRU || mtu > MAXMRU) {
	fprintf(stderr, "mtu option value of %ld is too %s\n",(long) mtu,
		(mtu < MINMRU? "small": "large"));
	return 0;
    }
    lcp_allowoptions[slot].mru = mtu;
    return (1);
}

static int setcallbackdelay(int slot,char **argv)
{
  int delay;
  if(!int_option(*argv, &delay))
    return 0;
  if (delay > 255) {
    option_error("callback delay of %d is too large", delay);
    return 0;
  }
  lcp_wantoptions[slot].cbopt.delay = delay;
  return 1;
}

static int setcallbackcbcp(int slot)
{
  lcp_wantoptions[slot].cbopt.neg_cbcp = 1;
  cbcp_protent.enabled_flag = 1;
  return 1;
}

static int setcallbacknocbcp(int slot)
{
  lcp_wantoptions[slot].cbopt.neg_cbcp = 0;
  cbcp_protent.enabled_flag = 0;
  return 1;
}

static int setcallbackrfc(int slot)
{
  lcp_wantoptions[slot].cbopt.neg_rfc = 1;
  return 1;
}

static int setcallbacknorfc(int slot)
{
  lcp_wantoptions[slot].cbopt.neg_rfc = 0;
  return 1;
}

static int setcallbackcbcpfirst(int slot)
{
  lcp_wantoptions[slot].cbopt.rfc_preferred = 0;
  return 1;
}

static int setcallbackcbcplast(int slot)
{
  lcp_wantoptions[slot].cbopt.rfc_preferred = 1;
  return 1;
}

static int setcallbacktype(int slot,char **argv)
{
  int type;
  if(!int_option(*argv, &type))
    return 0;
  switch (type) {
  case CB_AUTH:
    lcp_wantoptions[slot].cbopt.mlen = 0;
    lcp_wantoptions[slot].cbopt.message = 0;
    break;
  case CB_DIALSTRING:
  case CB_LOCATIONID:
  case CB_PHONENO:
  case CB_NAME:
    break;
  default:
    option_error("unkown callback type: %d", type);
    return 0;
  }
  lcp_wantoptions[slot].cbopt.type = type;
  return 1;
}

static int setcallback(int slot,char **argv)
{
  lcp_wantoptions[slot].neg_callback = 1;
  if (lcp_wantoptions[slot].cbopt.type != CB_AUTH) {
    lcp_wantoptions[slot].cbopt.message = strdup(*argv);
    if (lcp_wantoptions[slot].cbopt.message != 0) {
      lcp_wantoptions[slot].cbopt.mlen =
	strlen(lcp_wantoptions[slot].cbopt.message);
      if (!lcp_wantoptions[slot].cbopt.mlen)
	lcp_wantoptions[slot].cbopt.message = 0;
    } else {
      lcp_wantoptions[slot].cbopt.mlen = 0;
    }
  } else {
      lcp_wantoptions[slot].cbopt.mlen = 0;
      lcp_wantoptions[slot].cbopt.message = 0;
  }

  if (lcp_wantoptions[slot].cbopt.neg_cbcp)
    cbcp_protent.enabled_flag = 1;
  return 1;
}


/*
 * nopcomp - Disable Protocol field compression negotiation.
 */
static int nopcomp(int slot)
{
    lcp_wantoptions[slot].neg_pcompression = 0;
    lcp_allowoptions[slot].neg_pcompression = 0;
    return (1);
}


/*
 * setpassive - Set passive mode (don't give up if we time out sending
 * LCP configure-requests).
 */
static int setpassive(int slot)
{
    lcp_wantoptions[slot].passive = 1;
    return (1);
}


/*
 * setsilent - Set silent mode (don't start sending LCP configure-requests
 * until we get one from the peer).
 */
static int setsilent(int slot)
{
    lcp_wantoptions[slot].silent = 1;
    return 1;
}


/*
 * nopap - Disable PAP authentication with peer.
 */
static int nopap(int slot)
{
    lcp_allowoptions[slot].neg_upap = 0;
    refuse_pap = 1;
    return (1);
}


/*
 * reqpap - Require PAP authentication from peer.
 */
static int reqpap(int slot)
{
    lcp_wantoptions[slot].neg_upap = 1;
    setauth(slot);
    return 1;
}

static int setmp(int slot)
{
  lcp_allowoptions[slot].neg_mpmrru = 1;
  lcp_wantoptions[slot].neg_mpmrru = 1;
  lcp_wantoptions[slot].neg_mpdiscr = 1;
  return 1;
}

static int setpwlog(int slot)
{
  log_raw_password = 1;
  return 1;
}

static int setusefirstip(int slot)
{
	usefirstip = 1;
	return 1;
}

static int setuseifip(int slot)
{
  useifip = 1;
  return 1;
}

static int setdeldefaultroute(int slot)
{
  deldefaultroute = 1;
  return 1;
}

static int setifmtu(int slot)
{
  useifmtu = 1;
  return 1;
}

#if OLD_OPTIONS
/*
 * setupapfile - specifies UPAP info for authenticating with peer.
 */
static int setupapfile(int slot,char **argv)
{
    FILE * ufile;
    int l;

    lcp_allowoptions[slot].neg_upap = 1;

    /* open user info file */
    if ((ufile = fopen(*argv, "r")) == NULL) {
	fprintf(stderr, "unable to open user login data file %s\n", *argv);
	return 0;
    }
    if (!readable(fileno(ufile))) {
	fprintf(stderr, "%s: access denied\n", *argv);
	return 0;
    }
    check_access(ufile, *argv);

    /* get username */
    if (fgets(user, MAXNAMELEN - 1, ufile) == NULL
	|| fgets(passwd, MAXSECRETLEN - 1, ufile) == NULL){
	fprintf(stderr, "Unable to read user login data file %s.\n", *argv);
	return 0;
    }
    fclose(ufile);

    /* get rid of newlines */
    l = strlen(user);
    if (l > 0 && user[l-1] == '\n')
	user[l-1] = 0;
    l = strlen(passwd);
    if (l > 0 && passwd[l-1] == '\n')
	passwd[l-1] = 0;

    return (1);
}
#endif

/*
 * nochap - Disable CHAP authentication with peer.
 */
static int nochap(int slot)
{
    lcp_allowoptions[slot].neg_chap = 0;
    refuse_chap = 1;
    return (1);
}

/*
 * reqchap - Require CHAP authentication from peer.
 */
static int reqchap(int slot)
{
    lcp_wantoptions[slot].neg_chap = 1;
    setauth(slot);
    return (1);
}

/*
 * setnovj - disable vj compression
 */
static int setnovj(int slot)
{
    ipcp_wantoptions[slot].neg_vj = 0;
    ipcp_allowoptions[slot].neg_vj = 0;
    return (1);
}

/*
 * setnovjccomp - disable VJ connection-ID compression
 */
static int setnovjccomp(int slot)
{
    ipcp_wantoptions[slot].cflag = 0;
    ipcp_allowoptions[slot].cflag = 0;
    return 1;
}

/*
 * setvjslots - set maximum number of connection slots for VJ compression
 */
static int setvjslots(int slot,char **argv)
{
    int value;

    if (!int_option(*argv, &value))
	return 0;
    if (value < 2 || value > 16) {
	fprintf(stderr, "ipppd: vj-max-slots value must be between 2 and 16\n");
	return 0;
    }
    ipcp_wantoptions[slot].maxslotindex =
        ipcp_allowoptions[slot].maxslotindex = value - 1;
    return 1;
}

/*
 * setdomain - Set domain name to append to hostname
 */
static int setdomain(int slot,char **argv)
{
    gethostname(hostname, MAXNAMELEN);
    if (**argv != 0) {
	if (**argv != '.')
	    strncat(hostname, ".", MAXNAMELEN - strlen(hostname));
	strncat(hostname, *argv, MAXNAMELEN - strlen(hostname));
    }
    hostname[MAXNAMELEN-1] = 0;
    return (1);
}

/*
 * setdevname - Set the device name.
 */
static int setdevname(char *cp,int nd)
{
    struct stat statbuf;
    char *ttyname();
    char dev[MAXPATHLEN];

    if (strncmp("/dev/", cp, 5) != 0) {
	strcpy(dev, "/dev/");
	strncat(dev, cp, MAXPATHLEN - 5);
	dev[MAXPATHLEN-1] = 0;
	cp = dev;
    }

    /*
     * Check if there is a device by this name.
     */
    if (stat(cp, &statbuf) < 0) {
	if (errno == ENOENT)
	    return 0;
	syslog(LOG_ERR, "%s", cp);
	return -1;
    }

    (void) strncpy(lns[nd].devnam, cp, MAXPATHLEN);
    lns[nd].devnam[MAXPATHLEN-1] = 0;

    return 1;
}

/*
 * setipaddr - Set the IP address
 */
static int setipaddr(int slot,char *arg)
{
    struct hostent *hp;
    char *colon;
    u_int32_t local, remote;
    ipcp_options *wo = &ipcp_wantoptions[slot];

    local = 0 ;
    remote = 0 ;

    /*
     * IP address pair separated by ":".
     */
    if ((colon = strchr(arg, ':')) == NULL)
	return 0;

#ifdef ISDN4LINUX_PATCH
    if(strlen(arg) == 1)
      fprintf(stderr,"OK .. getting address from interface\n");
#endif

    /*
     * If colon first character, then no local addr.
     */
    if (colon != arg) {
	*colon = '\0';
	if ((local = inet_addr(arg)) == -1) {
	    if ((hp = gethostbyname(arg)) == NULL) {
		fprintf(stderr, "unknown host: %s\n", arg);
		return -1;
	    } else {
		local = *(u_int32_t *)hp->h_addr;
		if (our_name[0] == 0) {
		    strncpy(our_name, arg, MAXNAMELEN);
		    our_name[MAXNAMELEN-1] = 0;
		}
	    }
	}
	if (bad_ip_adrs(local)) {
	    fprintf(stderr, "bad local IP address %s\n", ip_ntoa(local));
	    return -1;
	}
	if (local != 0)
	{
	    wo->ouraddr = local;
#ifdef RADIUS
	    default_ouraddr = local ;
#endif
	}
	*colon = ':';
    }

    /*
     * If colon last character, then no remote addr.
     */
    if (*++colon != '\0') {
	if ((remote = inet_addr(colon)) == -1) {
	    if ((hp = gethostbyname(colon)) == NULL) {
		fprintf(stderr, "unknown host: %s\n", colon);
		return -1;
	    } else {
		remote = *(u_int32_t *)hp->h_addr;
		if (remote_name[0] == 0) {
		    strncpy(remote_name, colon, MAXNAMELEN);
		    remote_name[MAXNAMELEN-1] = 0;
		}
	    }
	}
	if (bad_ip_adrs(remote)) {
	    fprintf(stderr, "bad remote IP address %s\n", ip_ntoa(remote));
	    return -1;
	}
	if (remote != 0)
	{
	    wo->hisaddr = remote;
#ifdef RADIUS
	    default_hisaddr = remote ;
#endif
	}
    }

    return 1;
}

/*
 * setnoipdflt - disable setipdefault()
 */
static int setnoipdflt(int slot)
{
    disable_defaultip = 1;
    return 1;
}

/*
 * setipcpaccl - accept peer's idea of our address
 */
static int setipcpaccl(int slot)
{
    ipcp_wantoptions[slot].accept_local = 1;
    return 1;
}

/*
 * setipcpaccr - accept peer's idea of its address
 */
static int setipcpaccr(int slot)
{
    ipcp_wantoptions[slot].accept_remote = 1;
    return 1;
}

/*
 * setipdefault - default our local IP address based on our hostname.
 */
void setipdefault(void)
{
    struct hostent *hp;
    u_int32_t local;
    int i;

    for(i=0;i<numdev;i++) {
      ipcp_options *wo = &ipcp_wantoptions[i];
      /*
       * If local IP address already given, don't bother.
       */
      if (wo->ouraddr != 0 || disable_defaultip)
        return;

      /*
       * Look up our hostname (possibly with domain name appended)
       * and take the first IP address as our local IP address.
       * If there isn't an IP address for our hostname, too bad.
       */
      wo->accept_local = 1;	/* don't insist on this default value */
      if ((hp = gethostbyname(hostname)) == NULL)
	return;
      local = *(u_int32_t *)hp->h_addr;
      if (local != 0 && !bad_ip_adrs(local))
	wo->ouraddr = local;
    }
}


/*
 * setnetmask - set the netmask to be used on the interface.
 */
static int setnetmask(int slot,char **argv)
{
    u_int32_t mask;

	if (strcmp(*argv, "255.255.255.255") == 0) {
		netmask = 0xffffffff;
#ifdef RADIUS
                default_netmask = netmask ;
#endif
		return 1;
	}

    if ((mask = inet_addr(*argv)) == -1 || (netmask & ~mask) != 0) {
	fprintf(stderr, "Invalid netmask %s\n", *argv);
	return 0;
    }

    netmask = mask;
#ifdef RADIUS
       default_netmask = netmask ;
#endif
    return (1);
}

/*
 * setmaxconnect - Set the maximum connect time
 */
static int setmaxconnect(int slot,char **argv)
{
    int value;

    if (!int_option(*argv, &value))
       return 0;
    if (value < 0) {
       option_error("maxconnect time must be positive");
       return 0;
    }
    if (maxconnect > 0 && (value == 0 || value > maxconnect)) {
       option_error("maxconnect time cannot be increased");
       return 0;
    }
    maxconnect = value;
    return 1;
}


#ifdef INCLUDE_OBSOLETE_FEATURES
/*
 * setspeed - Set the speed.
 */
static int setspeed(int slot,char *arg)
{
    char *ptr;
    int spd;

    spd = strtol(arg, &ptr, 0);
    if (ptr == arg || *ptr != 0 || spd == 0)
    return 0;
    inspeed = spd;
    return 1;
}

/*
 * setasyncmap - add bits to asyncmap (what we request peer to escape).
 */
static int setasyncmap(int slot,char **argv)
{
    u_int32_t asyncmap;

    if (!number_option(*argv, &asyncmap, 16))
    return 0;
    lcp_wantoptions[slot].asyncmap |= asyncmap;
    lcp_wantoptions[slot].neg_asyncmap = 1;
    return(1);
}

/*
 * setescape - add chars to the set we escape on transmission.
 */
static int setescape(int slot,char **argv)
{
    int n, ret;
    char *p, *endp;

    p = *argv;
    ret = 1;
    while (*p) {
    n = strtol(p, &endp, 16);
    if (p == endp) {
        fprintf(stderr, "%s: invalid hex number: %s\n", progname, p);
        return 0;
    }
    p = endp;
    if (n < 0 || (0x20 <= n && n <= 0x3F) || n == 0x5E || n > 0xFF) {
        fprintf(stderr, "%s: can't escape character 0x%x\n", progname, n);
        ret = 0;
    } else
        xmit_accm[slot][n >> 5] |= 1 << (n & 0x1F);
    while (*p == ',' || *p == ' ')
        ++p;
    }
    return ret;
}

/*
 * noasyncmap - Disable async map negotiation.
 */
static int noasyncmap(int slot)
{
    lcp_wantoptions[slot].neg_asyncmap = 0;
    lcp_allowoptions[slot].neg_asyncmap = 0;
    return (1);
}

/*
 * setconnector - Set a program to connect to a serial line
 */
static int setconnector(int slot,char **argv)
{
    connector = strdup(*argv);
    if (connector == NULL)
    novm("connector string");

    return (1);
}

/*
 * setdisconnector - Set a program to disconnect from the serial line
 */
static int setdisconnector(int slot,char **argv)
{
    disconnector = strdup(*argv);
    if (disconnector == NULL)
    novm("disconnector string");

    return (1);
}

static int setmodem(int slot)
{
    modem = 1;
    return 1;
}

static int setcrtscts(int slot)
{
    crtscts = 1;
    return (1);
}

static int setnocrtscts(int slot)
{
    crtscts = -1;
    return (1);
}

static int setxonxoff(int slot)
{
    lcp_wantoptions[slot].asyncmap |= 0x000A0000;	/* escape ^S and ^Q */
    lcp_wantoptions[slot].neg_asyncmap = 1;

    crtscts = 2;
    return (1);
}

static int setlocal(int slot)
{
    modem = 0;
    return 1;
}
#endif

static int setnodetach(int slot)
{
    nodetach = 1;
    return (1);
}

static int setlock(int slot)
{
    lockflag = 1;
    return 1;
}

static int setusehostname(int slot)
{
    usehostname = 1;
    return 1;
}

static int setname(int slot,char **argv)
{
    if (our_name[0] == 0) {
	strncpy(our_name, argv[0], MAXNAMELEN);
	our_name[MAXNAMELEN-1] = 0;
    }
    return 1;
}

static int setuser(int slot,char **argv)
{
    strncpy(user, argv[0], MAXNAMELEN);
    user[MAXNAMELEN-1] = 0;
#ifdef RADIUS
       strncpy(radius_user, make_username_realm (argv[0]), MAXNAMELEN);
	radius_user[MAXNAMELEN-1] = 0;
#endif
    return 1;
}

static int setremote(int slot,char **argv)
{
    strncpy(remote_name, argv[0], MAXNAMELEN);
    remote_name[MAXNAMELEN-1] = 0;
    return 1;
}

static int setaskpw(int slot)
{
    ask_passwd = 1;
    return 1;
}

static int unsetaskpw(int slot)
{
    ask_passwd = 0;
    return 1;
}

static int setfdpw(int slot,char **argv)
{
    return int_option(*argv, &fdpasswd);
}

static int setauth(int slot)
{
    auth_required = 1;
    return 1;
}

static int setnoauth(int slot)
{
    auth_required = 0;
    return 1;
}


static int setdefaultroute(int slot)
{
    if (!ipcp_allowoptions[slot].default_route) {
	fprintf(stderr, "%s: defaultroute option is disabled\n", progname);
	return 0;
    }
    ipcp_wantoptions[slot].default_route = 1;
    return 1;
}

static int setnodefaultroute(int slot)
{
    ipcp_allowoptions[slot].default_route = 0;
    ipcp_wantoptions[slot].default_route = 0;
    return 1;
}

static int setproxyarp(int slot)
{
    if (!ipcp_allowoptions[slot].proxy_arp) {
	fprintf(stderr, "%s: proxyarp option is disabled\n", progname);
	return 0;
    }
    ipcp_wantoptions[slot].proxy_arp = 1;
    return 1;
}

static int setnoproxyarp(int slot)
{
    ipcp_wantoptions[slot].proxy_arp = 0;
    ipcp_allowoptions[slot].proxy_arp = 0;
    return 1;
}

static int setdologin(int slot)
{
    uselogin = 1;
    return 1;
}

#ifdef RADIUS
static int setdoradius(slot)
	int slot ;
{
	static char *func = "setdoradius" ;
	useradius = 1;
	if(!called_radius_init)
	{
		if ( (radius_init() < 0))
		{
			syslog(LOG_WARNING, "can't init radiusclient in %s", func);
			die (1) ;
		}
		else
		{
			called_radius_init = 1 ;
			auth_order = rc_conf_int("auth_order");
		}
	}
	return 1;
}

static int setdoradacct(slot)
	int slot ;
{
	static char *func = "setdoradacct" ;
	useradacct = 1;
	if(!called_radius_init)
	{
		if ( (radius_init() < 0))
		{
			syslog(LOG_WARNING, "can't init radiusclient in %s", func);
			die (1) ;
		}
		else
		{
			called_radius_init = 1 ;
			auth_order = rc_conf_int("auth_order");
		}
	}
	return 1;
}
#endif

/*
 * Functions to set the echo interval for modem-less monitors
 */

static int setlcpechointv(int slot,char **argv)
{
    return int_option(*argv, &lcp_echo_interval);
}

static int setlcpechofails(int slot,char **argv)
{
    return int_option(*argv, &lcp_echo_fails);
}

/*
 * Functions to set timeouts, max transmits, etc.
 */
static int setlcptimeout(int slot,char **argv)
{
    return int_option(*argv, &lcp_fsm[slot].timeouttime);
}

static int setlcpterm(int slot,char **argv)
{
    return int_option(*argv, &lcp_fsm[slot].maxtermtransmits);
}

static int setlcpconf(int slot,char **argv)
{
    return int_option(*argv, &lcp_fsm[slot].maxconfreqtransmits);
}

static int setlcpfails(int slot,char **argv)
{
    return int_option(*argv, &lcp_fsm[slot].maxnakloops);
}

static int setipcptimeout(int slot,char **argv)
{
    return int_option(*argv, &ipcp_fsm[slot].timeouttime);
}

static int setipcpterm(int slot,char **argv)
{
    return int_option(*argv, &ipcp_fsm[slot].maxtermtransmits);
}

static int setipcpconf(int slot,char **argv)
{
    return int_option(*argv, &ipcp_fsm[slot].maxconfreqtransmits);
}

static int setipcpfails(int slot,char **argv)
{
    return int_option(*argv, &lcp_fsm[slot].maxnakloops);
}

static int setpaptimeout(int slot,char **argv)
{
    return int_option(*argv, &upap[slot].us_timeouttime);
}

static int setpapreqtime(int slot,char **argv)
{
    return int_option(*argv, &upap[slot].us_reqtimeout);
}

static int setpapreqs(int slot, char **argv)
{
    return int_option(*argv, &upap[slot].us_maxtransmits);
}

static int setchaptimeout(int slot,char **argv)
{
    return int_option(*argv, &chap[slot].timeouttime);
}

static int setchapchal(int slot,char **argv)
{
    return int_option(*argv, &chap[slot].max_transmits);
}

static int setchapintv(int slot,char **argv)
{
    return int_option(*argv, &chap[slot].chal_interval);
}

static int noccp(int slot)
{
	ccp_protent.enabled_flag = 0;
	return 1;
}


static int setbsdcomp(int slot,char **argv)
{
    int rbits, abits;
    char *str, *endp;

    str = *argv;
    abits = rbits = strtol(str, &endp, 0);
    if (endp != str && *endp == ',') {
	str = endp + 1;
	abits = strtol(str, &endp, 0);
    }
    if (*endp != 0 || endp == str) {
	fprintf(stderr, "%s: invalid argument format for bsdcomp option\n",
		progname);
	return 0;
    }
    if ( (rbits != 0 && (rbits < BSD_MIN_BITS || rbits > BSD_MAX_BITS))
	|| (abits != 0 && (abits < BSD_MIN_BITS || abits > BSD_MAX_BITS))) {
	fprintf(stderr, "%s: bsdcomp option values must be 0 or %d .. %d\n",
		progname, BSD_MIN_BITS, BSD_MAX_BITS);
	return 0;
    }

    fprintf(stderr,"BsdComp: %d %d\n",rbits,abits);

    if (rbits > 0) {
	ccp_wantoptions[slot].bsd_compress = 1;
	ccp_wantoptions[slot].bsd_bits = rbits;
    } else
	ccp_wantoptions[slot].bsd_compress = 0;
    if (abits > 0) {
	ccp_allowoptions[slot].bsd_compress = 1;
	ccp_allowoptions[slot].bsd_bits = abits;
    } else
	ccp_allowoptions[slot].bsd_compress = 0;
    return 1;
}

static int setnobsdcomp(int slot)
{
    ccp_wantoptions[slot].bsd_compress = 0;
    ccp_allowoptions[slot].bsd_compress = 0;
    return 1;
}

static int setlzs(int ccp_slot,char ** argv)
{
    int rhists, rcmode, xhists, xcmode;
    char *str, *endp;

    rhists = LZS_DECOMP_DEF_HISTS;
    xhists = LZS_COMP_DEF_HISTS;
    rcmode = xcmode = LZS_CMODE_SEQNO;

    str = *argv;
    rhists = xhists = strtol(str, &endp, 0);
    if (endp != str) {
       if(*endp == ':') {
           str = endp + 1;
           rcmode = xcmode = strtol(str, &endp, 0);
       }
       if(endp != str) {
           if(*endp == ',') {
               str = endp + 1;
               xhists = strtol(str, &endp, 0);
           }
           if(endp != str) {
               if(*endp == ':') {
                   str = endp + 1;
                   xcmode = strtol(str, &endp, 0);
               }
           }
       }
    }

    if (*endp != 0 || endp == str) {
       option_error("invalid parameter '%s' for lzs option", *argv);
       return 0;
    }
    if(rhists < 0 || rhists > LZS_DECOMP_MAX_HISTS) {
       option_error("lzs recv hists must be 0 .. %d", LZS_DECOMP_MAX_HISTS);
       return 0;
    }
    if(xhists < 0 || xhists > LZS_COMP_MAX_HISTS) {
       option_error("lzs xmit hists must be 0 .. %d", LZS_COMP_MAX_HISTS);
       return 0;
    }
    if(rcmode < 0 || rcmode > 4) {
       option_error("lzs recv check mode %d unknown", rcmode);
       return 0;
    }
    if(xcmode < 0 || xcmode > 4) {
       option_error("lzs xmit check mode %d unknown", xcmode);
       return 0;
    }

    fprintf(stderr, "LZS: recv hists %d check %d xmit hists %d check %d\n",
           rhists, rcmode, xhists, xcmode);

    ccp_wantoptions[ccp_slot].lzs = 1;
    ccp_wantoptions[ccp_slot].lzs_hists = rhists;
    ccp_wantoptions[ccp_slot].lzs_cmode = rcmode;

    ccp_allowoptions[ccp_slot].lzs = 1;
    ccp_allowoptions[ccp_slot].lzs_hists = xhists;
    ccp_allowoptions[ccp_slot].lzs_cmode = xcmode;

    return 1;
}

static int setnolzs(int ccp_slot)
{
    ccp_wantoptions[ccp_slot].lzs = 0;
    ccp_allowoptions[ccp_slot].lzs = 0;
    return 1;
}

static int setdeflate(int ccp_slot,char ** argv)
{
    int rbits, abits;
    char *str, *endp;

    str = *argv;
    abits = rbits = strtol(str, &endp, 0);
    if (endp != str && *endp == ',') {
       str = endp + 1;
       abits = strtol(str, &endp, 0);
    }
    if (*endp != 0 || endp == str) {
       option_error("invalid parameter '%s' for deflate option", *argv);
       return 0;
    }
    if ( (rbits != 0 && (rbits < DEFLATE_MIN_SIZE || rbits > DEFLATE_MAX_SIZE))
       || (abits != 0 && (abits < DEFLATE_MIN_SIZE || abits > DEFLATE_MAX_SIZE))) {
       option_error("deflate option values must be 0 or %d .. %d",
                    DEFLATE_MIN_SIZE, DEFLATE_MAX_SIZE);
       return 0;
    }
    if (rbits > 0) {
       ccp_wantoptions[ccp_slot].deflate = 1;
       ccp_wantoptions[ccp_slot].deflate_size = rbits;
    } else
       ccp_wantoptions[ccp_slot].deflate = 0;
    if (abits > 0) {
       ccp_allowoptions[ccp_slot].deflate = 1;
       ccp_allowoptions[ccp_slot].deflate_size = abits;
    } else
       ccp_allowoptions[ccp_slot].deflate = 0;
    return 1;
}

static int setnodeflate(int ccp_slot)
{
    ccp_wantoptions[ccp_slot].deflate = 0;
    ccp_allowoptions[ccp_slot].deflate = 0;
    return 1;
}

static int setpred1comp(int slot)
{
    ccp_wantoptions[slot].predictor_1 = 1;
    ccp_allowoptions[slot].predictor_1 = 1;
    return 1;
}

static int setnopred1comp(int slot)
{
    ccp_wantoptions[slot].predictor_1 = 0;
    ccp_allowoptions[slot].predictor_1 = 0;
    return 1;
}

static int setipparam(int slot,char **argv)
{
    ipparam = strdup(*argv);
    if (ipparam == NULL)
	novm("ipparam string");

    return 1;
}

static int setpapcrypt(int slot)
{
    cryptpap = 1;
    return 1;
}

static int setidle (int slot,char **argv)
{
    return int_option(*argv, &idle_time_limit);
#ifdef RADIUS
    default_idle_time_limit = idle_time_limit ;
#endif
}

#ifdef RADIUS
static int setsessionlimit (slot, argv)
	int	slot ;
	char	**argv;
{
	return int_option(*argv, &session_time_limit);
	default_session_time_limit = session_time_limit ;
}
#endif

static int sethostroute(int slot)
{
    hostroute = 1;
    return 1;
}

static int setnohostroute(int slot)
{
   hostroute = 0;
   return 1;
}

static int setholdoff(int slot, char **argv)
{
    return int_option(*argv, &holdoff);
}

/*
 * setdnsaddr - set the dns address(es)
 */
static int setdnsaddr(int ipcp_slot,char **argv)
{
    u_int32_t dns;
    struct hostent *hp;

    dns = inet_addr(*argv);
    if (dns == -1) {
       if ((hp = gethostbyname(*argv)) == NULL) {
           option_error("invalid address parameter '%s' for ms-dns option",
                        *argv);
           return 0;
       }
       dns = *(u_int32_t *)hp->h_addr;
    }

    if (ipcp_allowoptions[ipcp_slot].dnsaddr[0] == 0) {
       ipcp_allowoptions[ipcp_slot].dnsaddr[0] = dns;
    } else {
       ipcp_allowoptions[ipcp_slot].dnsaddr[1] = dns;
    }

    return (1);
}

/*
 * setwinsaddr - set the wins address(es)
 */
static int setwinsaddr(int ipcp_slot,char **argv)
{
    u_int32_t wins;
    struct hostent *hp;

    wins = inet_addr(*argv);
    if (wins == -1) {
       if ((hp = gethostbyname(*argv)) == NULL) {
           option_error("invalid address parameter '%s' for ms-wins option",
                        *argv);
           return 0;
       }
       wins = *(u_int32_t *)hp->h_addr;
    }

    if (ipcp_allowoptions[ipcp_slot].winsaddr[0] == 0) {
       ipcp_allowoptions[ipcp_slot].winsaddr[0] = wins;
    } else {
       ipcp_allowoptions[ipcp_slot].winsaddr[1] = wins;
    }

    return (1);
}

/*
 * setgetdnsaddr - ask peer's idea of DNS server's address
 */
static int setgetdnsaddr(int slot,char **argv)
{
	ipcp_wantoptions[slot].neg_dns1 = 1;
	ipcp_wantoptions[slot].neg_dns2 = 1;
	return 1;
}

/*
 * setgetwinsaddr - ask peer's idea of WINS server's address
 */
static int setgetwinsaddr(int slot,char **argv)
{
	ipcp_wantoptions[slot].neg_wins1 = 1;
	ipcp_wantoptions[slot].neg_wins2 = 1;
	return 1;
}

static int setipxrouter (int slot,char **argv)
{
	char *val,arg[1024],*endp;
	int num = 0;

    ipxcp_wantoptions[slot].neg_router  = 1;
    ipxcp_allowoptions[slot].neg_router = 1;
	ipxcp_allowoptions[slot].num_router = 0;
	ipxcp_wantoptions[slot].num_router = 0;

	strncpy(arg,argv[0],1023);
	val = strtok(arg,",");
	while(val) {
		ipxcp_wantoptions[slot].router[num] = strtol(val,&endp,10);
		if(*endp)
			return 0;
		num++;
		if(num == 32)
			break;
	}

	ipxcp_wantoptions[slot].num_router = num;
	return 1;
}

static int setipxname (int slot,char **argv)
{
    char *dest = ipxcp_wantoptions[slot].name;
    char *src  = *argv;
    int  count;
    char ch;

    ipxcp_wantoptions[slot].neg_name  = 1;
    ipxcp_allowoptions[slot].neg_name = 1;
    memset (dest, '\0', sizeof (ipxcp_wantoptions[slot].name));

    count = 0;
    while (*src) {
        ch = *src++;
	if (! isalnum (ch) && ch != '_') {
	    fprintf (stderr,
		     "%s: IPX router name must be alphanumeric or _\n",
		     progname);
	    return 0;
	}

	if (count >= sizeof (ipxcp_wantoptions[slot].name)) {
	    fprintf (stderr,
		     "%s: IPX router name is limited to %d characters\n",
		     progname,
		     (int) sizeof (ipxcp_wantoptions[slot].name) - 1);
	    return 0;
	}

	dest[count++] = toupper (ch);
    }

    return 1;
}

static int setipxcptimeout(int slot,char **argv)
{
    return int_option(*argv, &ipxcp_fsm[slot].timeouttime);
}

static int setipxcpterm (int slot,char **argv)
{
    return int_option(*argv, &ipxcp_fsm[slot].maxtermtransmits);
}

static int setipxcpconf (int slot,char **argv)
{
    return int_option(*argv, &ipxcp_fsm[slot].maxconfreqtransmits);
}

static int setipxcpfails (int slot,char **argv)
{
    return int_option(*argv, &ipxcp_fsm[slot].maxnakloops);
}

static int setipxnetwork(int slot,char **argv)
{
    ipxcp_wantoptions[slot].neg_nn = 1;
    return int_option(*argv, &ipxcp_wantoptions[slot].our_network);
}

static int setipxanet(int slot)
{
    ipxcp_wantoptions[slot].accept_network = 1;
    ipxcp_allowoptions[slot].accept_network = 1;
    return 1;
}

static int setipxalcl(int slot)
{
    ipxcp_wantoptions[slot].accept_local = 1;
    ipxcp_allowoptions[slot].accept_local = 1;
    return 1;
}

static int setipxarmt(int slot)
{
    ipxcp_wantoptions[slot].accept_remote = 1;
    ipxcp_allowoptions[slot].accept_remote = 1;
    return 1;
}

static u_char *setipxnodevalue(u_char *src,u_char *dst)
{
    int indx;
    int item;

    for (;;) {
        if (!isxdigit (*src))
	    break;

	for (indx = 0; indx < 5; ++indx) {
	    dst[indx] <<= 4;
	    dst[indx] |= (dst[indx + 1] >> 4) & 0x0F;
	}

	item = toupper (*src) - '0';
	if (item > 9)
	    item -= 7;

	dst[5] = (dst[5] << 4) | item;
	++src;
    }
    return src;
}

static int setipxnode(int slot, char **argv)
{
    char *end;

    memset (&ipxcp_wantoptions[slot].our_node[0], 0, 6);
    memset (&ipxcp_wantoptions[slot].his_node[0], 0, 6);

    end = setipxnodevalue (*argv, &ipxcp_wantoptions[slot].our_node[0]);
    if (*end == ':')
	end = setipxnodevalue (++end, &ipxcp_wantoptions[slot].his_node[0]);

    if (*end == '\0') {
        ipxcp_wantoptions[slot].neg_node = 1;
        return 1;
    }

    fprintf(stderr, "%s: invalid argument for ipx-node option\n", progname);
    return 0;
}

static int setipxproto(int slot)
{
    ipxcp_protent.enabled_flag = 1;
    return 1;
}

static int resetipxproto(int slot)
{
    ipxcp_protent.enabled_flag = 0;
    return 1;
}

static int setforcedriver(int dummy)
{
    force_driver = 1;
    return 1;
}
