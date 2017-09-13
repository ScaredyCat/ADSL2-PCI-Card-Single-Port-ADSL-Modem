/*
 * $Id: radius.c,v 1.4 1999/06/21 13:28:52 hipp Exp $
 *
 * Copyright (C) 1996, Matjaz Godec <gody@elgo.si>
 * Copyright (C) 1996, Lars Fenneberg <in5y050@public.uni-hamburg.de>
 *
 */

#include <syslog.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <linux/ip_fw.h>
#include <radiusclient.h>
#include <ctype.h>

#include "ipppd.h"
#include "fsm.h"
#include "lcp.h"
#include "upap.h"
#include "chap.h"
#include "ipcp.h"
#include "ccp.h"
#include "pathnames.h"

char *ip_ntoa __P((u_int32_t));
int	bad_ip_adrs __P((u_int32_t));
char	username_realm[255];
char    radius_user[MAXNAMELEN];
char 	*make_username_realm ( char * );
static int client_port;
int called_radius_init = 0;
int auth_order = 0 ;
u_int32_t default_hisaddr = 0 ;
u_int32_t default_ouraddr = 0 ;
u_int32_t default_netmask = 0 ;
extern int idle_time_limit ;
extern int session_time_limit ;

void RestartIdleTimer __P((fsm *));
void RestartSessionTimer __P((fsm *));

struct ifstats 
{
	long 	rx_bytes;
	long 	rx_packets;
	long 	tx_bytes;
	long 	tx_packets;
};

/***************************************************************************
 *
 *	Name: radius_init
 *
 *	Purpose: Initializing radiusclient 
 *
 ***************************************************************************/
int radius_init() 
{

	static char *func = "radius_init" ;
	
	syslog(LOG_DEBUG, "%s: entered", func ) ;
	
	if (rc_read_config (PATH_RADIUSCLIENT_CONF) != 0)
	{
		syslog(LOG_ERR, "can't load config file %s in %s",
			PATH_RADIUSCLIENT_CONF, func ) ;
		return (-1) ;
	} ;
	
	if (rc_read_dictionary (rc_conf_str ("dictionary")) != 0)
	{
		syslog(LOG_ERR , "can't load dictionary file %s in %s" ,
			rc_conf_str ("dictionary") , func ) ;
		return (-1);
	} ;

	if (rc_read_mapfile (rc_conf_str ("mapfile")) != 0)
	{
		syslog(LOG_ERR , "can't load map file %s in %s" ,
			rc_conf_str ("mapfile") , func ) ;
		return (-1);
	} ;

	called_radius_init = 1;

	return 0;

}

/***************************************************************************
 *
 *	Name: setparams
 *
 *	Purpose: Set's up pppd parameters as received from RADIUS server
 *
 ***************************************************************************/
static int radius_setparams(unit,vp)

	int unit;
	VALUE_PAIR *vp;

{  

	fsm *f = &lcp_fsm[unit];

	ipcp_options *wo = &ipcp_wantoptions[unit];

	u_int32_t remote = 0;

	static char *func = "radius_setparams" ;
	
	syslog(LOG_DEBUG, "%s: entered", func ) ;
	/* 
 	 * service type (if not framed then quit), 
 	 * new IP address (RADIUS can define static IP for some users),
 	 * new netmask (RADIUS can define netmask),
 	 * idle time limit  ( RADIUS can define idle timeout),
 	 * session time limit ( RADIUS can limit session time ),
 	 */

	while (vp) 
	{
		switch (vp->attribute) 
		{
			case PW_SERVICE_TYPE:
			/* check for service type 	*/
			/* if not FRAMED then exit 	*/
			if (vp->lvalue != PW_FRAMED)
			{
				syslog (LOG_NOTICE, 
					"RADIUS wrong service type %ld for %s",
					vp->lvalue, radius_user );
				return (-1);
			}
			break;

			case PW_FRAMED_PROTOCOL:
			/* check for framed protocol type 	*/
			/* if not PPP then also exit	  	*/
	   		if (vp->lvalue != PW_PPP) 
	   		{
				syslog (LOG_NOTICE, 
					"RADIUS wrong framed protocol %ld for %s)", 
					vp->lvalue, radius_user );
				return (-1);
			}
			break;

			case PW_FRAMED_IP_ADDRESS:

			/* seting up static IP addresses 			  */
			/* 0xfffffffe means NAS should select an ip address       */
			/* 0xffffffff means user should be allowed to select one  */
			/* the last case probably needs special handling ???      */

			remote = vp->lvalue;
			if ((remote != 0xfffffffe) && (remote != 0xffffffff)) 
			{
				remote = htonl(remote);
				if (bad_ip_adrs (remote))
				{
					syslog (LOG_ERR, 
						"RADIUS bad remote IP address %s for %s in %s", 
						ip_ntoa (remote), 
						radius_user,
						func );
					return (-1);
				}
				wo->hisaddr = remote;
				syslog (LOG_DEBUG,
              				"Assigned remote static IP %s in %s",
              					ip_ntoa (remote), func ) ;
			}
			break;

			case PW_FRAMED_IP_NETMASK:
			/* changing netmask has some problems too 	*/
			/* Boy have I looked when I was changed   	*/
			/* server's config for USR/TC and none of my	*/
			/* linux TS didn't work any more :(	  	*/
				netmask = htonl (vp->lvalue);
	   			syslog (LOG_DEBUG, 
	   				"Assigned netmask %s in %s",
	   				ip_ntoa (netmask), func ) ;
			break;

			case PW_FRAMED_MTU:
			/* Don't know if this is OK but what the hack 	*/
			/* anyone using this succesfully ?		*/
				lcp_allowoptions[unit].mru = vp->lvalue;
				syslog (LOG_DEBUG, 
					"Assigned mtu %ld in %s" ,
					vp->lvalue , func ) ;
			break;

			case PW_IDLE_TIMEOUT:
			/* This one is operational	*/
			/* have using it for some time	*/
				idle_time_limit = vp->lvalue;
				if (idle_time_limit != 0) 
				{
					RestartIdleTimer ( f );
					syslog (LOG_DEBUG,
						"Assigned idle timeout %ld in %s" ,
						vp->lvalue , func ) ;
				}
			break;

			case PW_SESSION_TIMEOUT:	 
			/* This one works also for me	*/
				session_time_limit = vp->lvalue;
				if ( session_time_limit != 0 ) 
				{
					RestartSessionTimer ( f );
					syslog (LOG_DEBUG, 
					"assigned session timeout %ld in %s" ,
					session_time_limit, func ) ;
				}
			break;

			case PW_FILTER_ID:
			/* Idea for future implementation	*/
			/* if we get the name of the filter	*/
			/* we can run ipfwadm based script	*/
			/* and have one new functione as the 	*/
			/* big boys have. Any volunteers ?	*/
			/* I would look at ip-up imeplementation*/
			/* and copied it here			*/
				syslog (LOG_NOTICE, 
					"Dynamic filtering not implemented yet in %s",
					func );
			break;
			
			case PW_FRAMED_ROUTING:
			/* Idea for future implementation	*/
			/* Have no idea how to implement	*/
			/* for those who runs gated routing 	*/
			/* would go by default if i'm not wrong */
				syslog (LOG_NOTICE, 
					"Dynamic routing setup not implemented yet in %s",
					func );
			break;
			

		}
	    
		vp = vp->next;
	  
	}

	return 0;
}

#ifdef RADIUS_WTMP_LOGGING
/***************************************************************************
 * 
 * Name: radius_wtmp_logging
 *
 * Purpose: write user into wtmp database
 *
 ***************************************************************************/
static void
radius_wtmp_logging(user,unit)
	char 	*user;
	int 	unit ;

{
	char *tty;

	static char *func = "radius_wtmp_logging" ;

	syslog(LOG_DEBUG, "%s: entered", func ) ;

	syslog(LOG_DEBUG, "user %s logged in", user);

	tty = lns[unit].devnam;   

	if (strncmp(tty, "/dev/", 5) == 0)
	{
		tty += 5;
	}

	logwtmputmp(unit, tty, radius_user, "");

	lns[unit].logged_in = TRUE;

}
#endif

/***************************************************************************
 *
 * Name: radius_buildenv
 *
 * Purpose: 
 *
 ***************************************************************************/
int 
radius_buildenv(vp)
	VALUE_PAIR *vp ;
                      
{
                      
	char name[2048] ;
	char value[2048]; /* more than enough */
	char *p;
	int acount[256];
	int attr;
        
	script_setenv("RADIUS_USER_NAME", radius_user);

	while (vp)
	{
		strcpy(name, "RADIUS_");
		
		if (rc_avpair_tostr(vp, name+7, sizeof(name)-7, value,
				sizeof(value)) < 0)
		{
			return 1;
		}
		
		/* Translate "-" => "_" and uppercase*/
		for(p = name; *p; p++)
		{
			*p = toupper(*p);
			if (*p == '-') *p = '_';
		}
		
		/* Add to the attribute count and append the var if necessary. */
		if ((attr = vp->attribute) < 256)
		{
			int count;
			if ((count = acount[attr]++) > 0)
			{
				char buf[10];
				sprintf(buf, "_%d", count);
				strcat(name,buf);
			}
		}
	
		script_setenv(name, value);	
		
		vp = vp->next;
	}
	
	return 0;

}

/****************************************************************************
 *
 * Name: radius_pap_auth 
 *
 * Purpose:  Check the user name and password against RADIUS server 
 *	     and add accounting start record of the user if OK.
 *
 * Returns:  UPAP_AUTHNAK: Login failed.
 *	     UPAP_AUTHACK: Login succeeded.
 *
 ****************************************************************************/
int
radius_pap_auth (unit, user, passwd, msg, msglen )

	int 	unit ;
	char 	*user;
	char 	*passwd;
	char 	**msg;
	int 	*msglen;

{
	VALUE_PAIR *send ;
	VALUE_PAIR *received;
	UINT4 av_type;
	static char radius_msg[4096];
	int result;
	static char *func = "radius_pap_auth" ;
	
	syslog(LOG_DEBUG, "%s: entered", func ) ;

	send = NULL ;

	received = NULL;

	client_port = rc_map2id (lns[unit].devnam);

	av_type = PW_FRAMED;
	
	rc_avpair_add (&send, PW_SERVICE_TYPE, &av_type, 0);

	av_type = PW_PPP;

	rc_avpair_add (&send, PW_FRAMED_PROTOCOL, &av_type, 0);

	strncpy ( radius_user , make_username_realm ( user ) , 
			sizeof (radius_user));
 	 
	rc_avpair_add (&send, PW_USER_NAME, radius_user , 0);
	rc_avpair_add (&send, PW_USER_PASSWORD, passwd, 0);
	
	result = rc_auth (client_port, send, &received, radius_msg);

  	if (result == OK_RC)
  	{
		if (radius_setparams(unit,received) < 0)
		{
			syslog (LOG_ERR,"Error setting params in %s" , 
				func );
			result = ERROR_RC;
		}
		else
		{
			/* Build the environment for ip-up and ip-down */
			script_unsetenv_prefix("RADIUS_");

			radius_buildenv(received);
		}
	}
	else
	{
		syslog (LOG_ERR,"Error sending auth request in %s" ,  func );
	}
	
	rc_avpair_free(received);

	rc_avpair_free (send);
	
	*msg = radius_msg;

	*msglen = strlen(radius_msg);

#ifdef RADIUS_WTMP_LOGGING
	if (result == OK_RC)
	{
		radius_wtmp_logging(user,unit);
	}
#endif

	return (result == OK_RC)?UPAP_AUTHACK:UPAP_AUTHNAK;
}

/***************************************************************************
 * 
 * Name: radius_chap_auth 
 *
 * Purpose: CHAP authentication with RADIUS server
 *
 ***************************************************************************/
int
radius_chap_auth (unit,user, remmd, cstate )

	int	unit ;
	char 	*user;
	u_char 	*remmd;
	chap_state	*cstate;

{

	VALUE_PAIR *send;
	VALUE_PAIR *received;
	UINT4 av_type;
	static char radius_msg[4096];
	int result;
	u_char cpassword[MD5_SIGNATURE_SIZE+1];

	static char *func = "radius_chap_auth" ;
	
	syslog(LOG_DEBUG, "%s: entered", func ) ;

	/* we handle md5 digest at the moment */
	if (cstate->chal_type != CHAP_DIGEST_MD5) 
	{
		syslog(LOG_ERR, "Challenge type not MD5 in %s", func ) ;
		return(-1);
	}

	send = received = NULL;

  	client_port = rc_map2id (lns[unit].devnam);

        av_type = PW_FRAMED;

  	rc_avpair_add (&send, PW_SERVICE_TYPE, &av_type, 0);

  	av_type = PW_PPP;

  	rc_avpair_add (&send, PW_FRAMED_PROTOCOL, &av_type, 0);

	rc_avpair_add (&send, PW_USER_NAME, radius_user , 0);

	/*
	 * add the CHAP-Password and CHAP-Challenge fields 
	 */
	 
	cpassword[0] = cstate->chal_id;

	memcpy(&cpassword[1], remmd, MD5_SIGNATURE_SIZE);

	rc_avpair_add(&send, PW_CHAP_PASSWORD, cpassword, MD5_SIGNATURE_SIZE + 1);

	rc_avpair_add(&send, PW_CHAP_CHALLENGE, cstate->challenge, cstate->chal_len); 
	 	
  	result = rc_auth (client_port, send, &received, radius_msg);
 	 
 	 
  	if (result == OK_RC)
  	{
		if (radius_setparams(unit,received) < 0)
		{
			syslog (LOG_ERR,"Error setting params in %s" , 
				func );
			result = ERROR_RC;
		}
	}
	else
	{
		syslog (LOG_ERR,"Error sending auth request in %s" ,  func );
	}

	rc_avpair_free(received);

	rc_avpair_free (send);

#ifdef RADIUS_WTMP_LOGGING
	if (result == OK_RC)
	{
		radius_wtmp_logging(user,unit);
	}
#endif
	
	return (result == OK_RC)?0:(-1);

}

/***************************************************************************
 * 
 * Name: if_getipacct
 *
 * Purpose: reads ip accounting information
 *
 ***************************************************************************/

static void 
if_getipacct(ifname, ifs)

	char	*ifname ;
	struct ifstats *ifs ;

{
	FILE* 	f = fopen("/proc/net/ip_acct","r");
	char  	buf[256];
	char  	acctif[128];
	long   	dummy;
	long  	direction;
	long   	packets;
	long   	bytes;
	int   	rc;

	static char *func = "if_getipacct" ;
	
	syslog(LOG_DEBUG, "%s: entered", func ) ;

	if (!f)
	{
		syslog(LOG_ERR, "Can't open /proc/net/ip_acct in %s:" , 
			func ) ;
		return;
	}
	
	fgets(buf, sizeof(buf), f);

	acctif[0] = '\0';

	while (1) 
	{
		rc = fscanf(f, "%ld/%ld->%ld/%ld %s %ld %ld %ld %ld %ld %ld",
				&dummy,&dummy,&dummy,&dummy,acctif,&dummy,
				&direction, &dummy, &dummy, &packets, &bytes);

		fgets(buf, sizeof(buf), f);

		if (rc == EOF)
		{
			break;
		}

		if (rc != 11)
		{
			break;
		}

		if (strcmp(ifname, acctif) == 0) 
		{

			syslog(LOG_DEBUG, "Interface <%s> found in %s",
				acctif, func ) ;
				
			if (direction == 1000) /* incoming bytes */ 
			{ 
				syslog(LOG_DEBUG, 
					"Incoming bytes/packets = %ld/%ld in %s",
					bytes, packets, func ) ;
			
				ifs->rx_bytes = bytes;
				ifs->rx_packets = packets;
			} 
			else 
			{
				syslog(LOG_DEBUG, 
					"Outgoing bytes/packets = %ld/%ld in %s",
					bytes, packets, func ) ;

				ifs->tx_bytes = bytes;
				ifs->tx_packets = packets;
			}
		}
	}

	fclose(f);

}

/***************************************************************************
 *
 * Name: radius_ip_acct_on
 *
 * Purpose: set up ip accounting rules for this interface
 *
 ***************************************************************************/
static int 
radius_ip_acct_on ( unit )

	int unit ;

{
	static int sockfd = -1;
	int ret;

	struct ip_fw ip_acct ;

	static char *func = "radius_ip_acct_on" ;

	syslog ( LOG_DEBUG , "%s: entered" , func ) ;

	memset ( &ip_acct , 0x0 , sizeof ( ip_acct ) ) ;

	strncpy ( ip_acct.fw_vianame , lns[unit].ifname, IFNAMSIZ ) ;
	ip_acct.fw_flg |= IP_FW_F_ACCTIN ;
	
	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) == -1) 
	{
		syslog ( LOG_ERR , "ipfwadm: socket creation failed in %s",
				func );
		return (-1);
	}

	ret = setsockopt( sockfd, IPPROTO_IP, IP_ACCT_INSERT, &ip_acct , 
					sizeof ( ip_acct) );

	close ( sockfd ) ;

	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) == -1) 
	{
		syslog ( LOG_ERR , "ipfwadm: socket creation failed in %s",
				func );
		return (-1);
	}

	ip_acct.fw_flg &= ~IP_FW_F_ACCTIN ;
	ip_acct.fw_flg |= IP_FW_F_ACCTOUT ;

	ret = setsockopt( sockfd, IPPROTO_IP, IP_ACCT_INSERT, &ip_acct , 
	                                        sizeof ( ip_acct) );

	close ( sockfd ) ;
	
		return ret;

};

/***************************************************************************
 *
 * Name: radius_ip_acct_off
 *
 * Purpose: destroy ip accounting rules for this interface
 *
 ***************************************************************************/
static int 
radius_ip_acct_off ( unit )

	int unit ;

{
	static int sockfd = -1;
	int ret;

	struct ip_fw ip_acct ;

	static char *func = "radius_ip_acct_on" ;

	syslog ( LOG_DEBUG , "%s: entered" , func ) ;

	memset ( &ip_acct , 0x0 , sizeof ( ip_acct ) ) ;

	strncpy ( ip_acct.fw_vianame , lns[unit].ifname, IFNAMSIZ ) ;
	ip_acct.fw_flg |= IP_FW_F_ACCTIN ;

	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) == -1) 
	{
		syslog ( LOG_ERR , "ipfwadm: socket creation failed in %s",
				func );
		return (-1);
	}

	ret = setsockopt( sockfd, IPPROTO_IP, IP_ACCT_DELETE, &ip_acct , 
					sizeof ( ip_acct) );

	close ( sockfd ) ;

	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) == -1) 
	{
		syslog ( LOG_ERR , "ipfwadm: socket creation failed in %s",
				func );
		return (-1);
	}

	ip_acct.fw_flg &= ~IP_FW_F_ACCTIN ;
	ip_acct.fw_flg |= IP_FW_F_ACCTOUT ;

	ret = setsockopt( sockfd, IPPROTO_IP, IP_ACCT_DELETE, &ip_acct , 
	                                        sizeof ( ip_acct) );

	close ( sockfd ) ;
	
	return ret;

};

/***************************************************************************
 * 
 * Name: radius_acct_start
 *
 * Purpose: send acct start message to RADIUS server
 *
 ***************************************************************************/
int 
radius_acct_start(unit)

	int unit ;

{
	UINT4 	av_type;
	int 	result;
	VALUE_PAIR *send = NULL;
	ipcp_options *ho = &ipcp_hisoptions[unit];
	u_int32_t hisaddr ;

	static char *func = "radius_acct_start" ;
	
	syslog(LOG_DEBUG, "%s: entered", func ) ;

  	client_port = rc_map2id (lns[unit].devnam);

	lns[unit].start_time = time (NULL);
	
	radius_ip_acct_on ( unit ) ;

	strncpy (lns[unit].session_id, rc_mksid (), 
			sizeof (lns[unit].session_id) );

	rc_avpair_add (&send, PW_ACCT_SESSION_ID, lns[unit].session_id, 0);
	rc_avpair_add (&send, PW_USER_NAME, radius_user, 0);

	av_type = PW_STATUS_START;
	rc_avpair_add (&send, PW_ACCT_STATUS_TYPE, &av_type, 0);
      
	av_type = PW_FRAMED;
	rc_avpair_add (&send, PW_SERVICE_TYPE, &av_type, 0);
      
	av_type = PW_PPP;
	rc_avpair_add (&send, PW_FRAMED_PROTOCOL, &av_type, 0);

	av_type = PW_RADIUS;
	rc_avpair_add (&send, PW_ACCT_AUTHENTIC, &av_type, 0);

	rc_avpair_add (&send, PW_CALLING_STATION_ID, lns[unit].remote_number, 0);

	av_type = PW_ISDN_SYNC ;
	rc_avpair_add(&send, PW_NAS_PORT_TYPE, &av_type, 0);

      
        hisaddr = ho->hisaddr;
	av_type = htonl(hisaddr) ;
	rc_avpair_add (&send, PW_FRAMED_IP_ADDRESS , &av_type , 0 ) ;
      
	result = rc_acct (client_port, send);

	rc_avpair_free(send);
	
	if (result != OK_RC) 
	{
		/* RADIUS server could be down so make this a warning */
		syslog (LOG_WARNING, 
			"Accounting START failed for %s in %s", 
			radius_user,func );
	}  
	else 
	{
        	lns[unit].radius_in = TRUE;

#ifdef RADIUS_WTMP_LOGGING
		if ( lns[unit].logged_in == FALSE ) 
			radius_wtmp_logging(user,unit);
#endif
	}
	
	return ( result ) ;

}

/***************************************************************************
 * 
 * Name: radius_acct_stop
 *
 * Purpose: send acct stop message to RADIUS server
 *
 ***************************************************************************/
int 
radius_acct_stop (unit)

	int	unit ;
{
	UINT4 	av_type;
	int 	result;
	VALUE_PAIR 	*send = NULL;
	struct 	ifstats ifstats;
	ipcp_options *ho = &ipcp_hisoptions[unit];
	u_int32_t hisaddr ;

	static char *func = "radius_acct_stop" ;
	
	syslog(LOG_DEBUG, "%s: entered", func ) ;

	memset ( &ifstats , 0x0 , sizeof ( ifstats )) ;
	
	if_getipacct(lns[unit].ifname, &ifstats);

	radius_ip_acct_off ( unit ) ;
	
	rc_avpair_add (&send, PW_ACCT_SESSION_ID, lns[unit].session_id, 0);
	
	rc_avpair_add (&send, PW_USER_NAME, radius_user, 0);

	av_type = PW_STATUS_STOP;
	rc_avpair_add (&send, PW_ACCT_STATUS_TYPE, &av_type, 0);
	
	av_type = PW_FRAMED;
	rc_avpair_add (&send, PW_SERVICE_TYPE, &av_type, 0);

	av_type = PW_PPP;
	rc_avpair_add (&send, PW_FRAMED_PROTOCOL, &av_type, 0);

	av_type = PW_RADIUS;
	rc_avpair_add (&send, PW_ACCT_AUTHENTIC, &av_type, 0);

	av_type = time (NULL) - lns[unit].start_time;
	rc_avpair_add (&send, PW_ACCT_SESSION_TIME, &av_type, 0);

	av_type = ifstats.tx_bytes ;
	rc_avpair_add(&send, PW_ACCT_OUTPUT_OCTETS, &av_type, 0);
               
	av_type = ifstats.rx_bytes ;
	rc_avpair_add(&send, PW_ACCT_INPUT_OCTETS, &av_type, 0);
	
	av_type = ifstats.tx_packets ;
	rc_avpair_add(&send, PW_ACCT_OUTPUT_PACKETS, &av_type, 0);
               
	av_type = ifstats.rx_packets ;
	rc_avpair_add(&send, PW_ACCT_INPUT_PACKETS, &av_type, 0);

	rc_avpair_add (&send, PW_CALLING_STATION_ID, lns[unit].remote_number, 0);

	av_type = PW_ISDN_SYNC ;
	rc_avpair_add(&send, PW_NAS_PORT_TYPE, &av_type, 0);
	
        hisaddr = ho->hisaddr;
	av_type = htonl(hisaddr) ;
	rc_avpair_add (&send, PW_FRAMED_IP_ADDRESS , &av_type , 0 ) ;	
	
	result = rc_acct (client_port, send);

	rc_avpair_free(send);

	if (result != OK_RC)
	{
		syslog(LOG_ERR, 
			"Accounting STOP failed for %s in %s", 
			radius_user, func);
	}
    
	lns[unit].radius_in = FALSE;
	
	return ( result ) ;

}


/***************************************************************************
 * 
 * Name: make_username_realm
 *
 * Purpose: makes username_realm from user
 *
 ***************************************************************************/
char 
*make_username_realm ( user )

	char 	*user ;

{
	char 	*default_realm;	
	static char *func = "radius_acct_stop" ;

	syslog(LOG_DEBUG, "%s: entered", func ) ;
	
	if ( user != NULL ) 
	{
		strncpy (username_realm, user,sizeof (username_realm));
	} 
	else
	{
		strncpy (username_realm, "\0" , sizeof (username_realm));
	}

	default_realm = rc_conf_str ("default_realm");

	if ( (strchr (username_realm, '@') == NULL) && 
		default_realm &&
		(*default_realm != '\0'))
	{
		strncat (username_realm, "@", 
				sizeof (username_realm));
		strncat (username_realm, default_realm, 
				sizeof (username_realm));
	}
	
	return ( username_realm ) ;

}
