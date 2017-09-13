/*
 * sys-linux.c - System-dependent procedures for setting up
 * PPP interfaces on Linux systems
 *
 * Fairly patched version for isdn4linux
 * copyright (c) 1995,1996,1997 of all patches by Michael Hipp
 * still no warranties (see disclaimer)
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

char sys_rcsid[] = "$Id: sys-linux.c,v 1.25 2000/07/25 20:23:51 kai Exp $";

#define _LINUX_STRING_H_

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/utsname.h>
#include <sys/sysmacros.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <memory.h>
#include <utmp.h>
#include <mntent.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include </usr/include/net/if.h>
#include </usr/include/net/if_arp.h>
#include </usr/include/net/route.h>
#if defined __GLIBC__ && __GLIBC__ >= 2
# include </usr/include/net/ppp_defs.h>
# include </usr/include/net/if_ppp.h>
# include </usr/include/net/ethernet.h>
# include "route.h"
#else
# include <linux/ppp_defs.h>
# include <linux/if_ppp.h>
# include <linux/if_ether.h>
#endif

#include <linux/isdn_ppp.h>
#if __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 1
# include <netipx/ipx.h>
#endif

#include "fsm.h"
#include "ipppd.h"
#include "ipcp.h"
#include "ipxcp.h"
#include "ccp.h"
#include "lcp.h"

extern int force_driver;

static int prev_kdebugflag     = 0;
static int has_default_route   = 0;
static int driver_version      = 0;
static int driver_modification = 0;
static int driver_patch        = 0;
static int devroute            = -1;    /* 0 for Linux >= 2.1.x */
static int last_net_mask       = 0;   /* Ugly! */

int get_ether_addr (u_int32_t ipaddr, struct sockaddr *hwaddr, char *name);
static void decode_version (char *buf, int *version,
		 int *modification, int *patch);
int sockfd;			/* socket for doing interface ioctls */

static char *lock_file;
static int kernel_version;

#define SIN_ADDR(x)     (((struct sockaddr_in *) (&(x)))->sin_addr.s_addr)
#define KVERSION(j,n,p) ((j)*1000000 + (n)*1000 + (p))

#define MAX_IFS		4096

#define FLAGS_GOOD (IFF_UP          | IFF_BROADCAST)
#define FLAGS_MASK (IFF_UP          | IFF_BROADCAST | \
		    IFF_POINTOPOINT | IFF_LOOPBACK  | IFF_NOARP)

/*
 * SET_SA_FAMILY - set the sa_family field of a struct sockaddr,
 * if it exists.
 */

#define SET_SA_FAMILY(addr, family)			\
    memset ((char *) &(addr), '\0', sizeof(addr));	\
    addr.sa_family = (family);

/*
 * Determine if the PPP connection should still be present.
 */

#define still_ppp(linkunit) (lns[linkunit].hungup == 0)

/*
 *
 */

void enable_mp(int linkunit,int flags)
{
  int mpflags,s;
  s = ioctl(lns[linkunit].fd,PPPIOCGMPFLAGS,(caddr_t) &mpflags);
  if(s < 0)
    syslog(LOG_ERR,"ipppd: Can't get MP-Flags");
  else
  {
    mpflags |= SC_MP_PROT;
    mpflags &= ~SC_REJ_MP_PROT;
    mpflags |= flags;
    s = ioctl(lns[linkunit].fd,PPPIOCSMPFLAGS,(caddr_t) &mpflags);
    if(s < 0)
      syslog(LOG_ERR,"ipppd: Can't set MP-Flags");
  }
}


/*
 * Functions to read and set the flags value in the device driver
 */

static int get_flags (int tu,int *error)
{    
	int flags;

	*error = 0;

	if(lns[tu].fd < 0) {
		*error = 1;
		return 0;
	}

	if (ioctl(lns[tu].fd, PPPIOCGFLAGS, (caddr_t) &flags) < 0) {
		syslog(LOG_ERR, "ioctl(PPPIOCGFLAGS) on %d: %m",lns[tu].fd);
		*error = 1;
		return 0;
	}

	MAINDEBUG ((LOG_DEBUG, "get flags = %x\n", flags));
	return flags;
}

static void set_flags (int flags,int tu)
{    
	MAINDEBUG ((LOG_DEBUG, "set flags = %x\n", flags));

	if(lns[tu].fd < 0)
		return;

	if (ioctl(lns[tu].fd, PPPIOCSFLAGS, (caddr_t) &flags) < 0) {
		syslog(LOG_ERR, "ioctl(PPPIOCSFLAGS, %x): %m", flags);
	}
}

/*
 * sys_init - System-dependent initialization.
 */
void sys_init(void)
{
	struct utsname uts;
	int maj, min, pat;

	openlog("ipppd", LOG_PID | LOG_NDELAY, LOG_PPP);
	setlogmask(LOG_UPTO(LOG_INFO));
    if (debug)
		setlogmask(LOG_UPTO(LOG_DEBUG));
    
	/* Get an internet socket for doing socket ioctls. */
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
		syslog(LOG_ERR, "Couldn't create IP socket: %m");
		die(1);
	}
	if (devroute < 0) {
		uname(&uts);
		maj = min = pat = 0;
		decode_version(uts.release, &maj, &min, &pat);
		if (maj >= 2 && min >= 1) /* Linux >= 2.1.x */
			devroute = 0;
		else
			devroute = 1;
	}
}

/*
 * note_debug_level - note a change in the debug level.
 */
void note_debug_level (void)
{
	if (debug) {
		MAINDEBUG ((LOG_INFO, "Debug turned ON, Level %d", debug));
		setlogmask(LOG_UPTO(LOG_DEBUG));
	}
    else
		setlogmask(LOG_UPTO(LOG_WARNING));
}

/*
 * set_kdebugflag - Define the debugging level for the kernel
 */
int set_kdebugflag (int requested_level,int tu)
{
	if (ioctl(lns[tu].fd, PPPIOCGDEBUG, &prev_kdebugflag) < 0) {
		syslog(LOG_ERR, "ioctl(PPPIOCGDEBUG): %m");
		return 0;
	}
    
	if (prev_kdebugflag != requested_level) {
		if (ioctl(lns[tu].fd, PPPIOCSDEBUG, &requested_level) < 0) {
			syslog (LOG_ERR, "ioctl(PPPIOCSDEBUG): %m");
			return 0;
		}
		MAINDEBUG ((LOG_INFO, "set kernel debugging level to %d", requested_level));
	}
	return 1;
}

/*
 * establish_ppp - get UNIT, callinfo, IF-MTU
 */
void establish_ppp (int linkunit)
{
	if (ioctl(lns[linkunit].fd, PPPIOCGUNIT, &lns[linkunit].ifunit) < 0) {
		syslog(LOG_ERR, "ioctl(PPPIOCGUNIT): %m");
		lns[linkunit].ifunit = -1;
		return;
	}
	lns[linkunit].master = -1;
	sprintf(lns[linkunit].ifname,"%s%d","ippp",lns[linkunit].ifunit);

	if( ioctl(lns[linkunit].fd, PPPIOCGCALLINFO, &lns[linkunit].pci) == 0) {
		struct pppcallinfo *pci = &lns[linkunit].pci;
		syslog(LOG_NOTICE, "Local number: %s, Remote number: %s, Type: %s",
			pci->local_num,pci->remote_num,pci->calltype & CALLTYPE_INCOMING ? "incoming" : "outgoing" );
#ifdef RADIUS
               strncpy ( lns[linkunit].remote_number, pci->remote_num, sizeof( lns[linkunit].remote_number ) ) ;
#endif
	}

	if(useifmtu) {   
		struct ifreq ifr;

		memset (&ifr, '\0', sizeof (ifr));
		strncpy(ifr.ifr_name, lns[linkunit].ifname, sizeof (ifr.ifr_name));
		if (ioctl(sockfd, SIOCGIFMTU, (caddr_t) &ifr) < 0) 
			syslog(LOG_NOTICE , "Can't get MTU from device %s",lns[linkunit].ifname);
		else
			lcp_allowoptions[lns[linkunit].lcp_unit].mru = ifr.ifr_mtu;
	}

#if 0
	set_kdebugflag (kdebugflag,linkunit);
#endif

	MAINDEBUG ((LOG_NOTICE, "Using version %d.%d.%d of PPP driver",
		driver_version, driver_modification, driver_patch));
}

/*
 * output - Output PPP packet.
 */
void output_ppp (int linkunit, unsigned char *p, int len)
{
	if (debug)
		log_packet(p, len, "sent ",linkunit);
    
	if (write(lns[linkunit].fd, p, len) < 0) {
		syslog(LOG_ERR, "write, unit: %d fd: %d: %m",linkunit,lns[linkunit].fd);
		die(1);
	}
}

/*
 * read_packet - get a PPP packet from the serial device.
 */
int read_packet (unsigned char *buf,int linkunit)
{
	int len;
  
	len = read(lns[linkunit].fd, buf, PPP_MTU + PPP_HDRLEN);
	if (len < 0) {
		if (errno == EWOULDBLOCK)
			return -1;
		syslog(LOG_ERR, "read(fd): %m");
		die(1);
	}
	return len;
}

/*
 * ppp_send_config - configure the transmit characteristics of
 * the ppp interface.
 */
void ppp_send_config (int unit,int mtu,u_int32_t asyncmap,int pcomp,int accomp)
{
	u_int x;
	struct ifreq ifr;
	int err;

	MAINDEBUG ((LOG_DEBUG, "send_config: mtu = %d\n", mtu));

	/*
	 * Ensure that the link is still up.
	 */
	if (still_ppp(unit)) {
		/*
		 * Set the MTU and other parameters for the ppp device
		 */
		if(lns[unit].master < 0) {
			memset (&ifr, '\0', sizeof (ifr));
			strncpy(ifr.ifr_name, lns[unit].ifname, sizeof (ifr.ifr_name));
			ifr.ifr_mtu = mtu;
	
			if (ioctl(sockfd, SIOCSIFMTU, (caddr_t) &ifr) < 0) {
				syslog(LOG_ERR, "ioctl(SIOCSIFMTU): %m, %d %s %d.",sockfd,ifr.ifr_name,ifr.ifr_mtu);
			}
		}
	
		x = get_flags(unit,&err);
		if(err)
			return;
		x = pcomp  ? x | SC_COMP_PROT : x & ~SC_COMP_PROT;
		x = accomp ? x | SC_COMP_AC   : x & ~SC_COMP_AC;
		set_flags(x,unit);
	}
}

/* 
 * ppp_mp_send_config, ppp_mp_recv_config
 */
void ppp_mp_send_config(int unit,int mtu)
{
	int r;
	r = ioctl(lns[unit].fd,PPPIOCSMPMTU,&mtu);
	syslog(LOG_DEBUG,"mp_mtu: %d",r);
}

void ppp_mp_recv_config(int unit,int mru)
{
	int r;
	r = ioctl(lns[unit].fd,PPPIOCSMPMRU,&mru);
	syslog(LOG_DEBUG,"mp_mru: %d",r);
}

/*
 * ppp_set_xaccm - set the extended transmit ACCM for the interface.
 */
void ppp_set_xaccm (int unit, ext_accm accm)
{
#if 0
	MAINDEBUG ((LOG_DEBUG, "set_xaccm: %08lx %08lx %08lx %08lx\n",
		accm[0], accm[1], accm[2], accm[3]));

	if (ioctl(lns[unit].fd, PPPIOCSXASYNCMAP, accm) < 0 && errno != ENOTTY)
		syslog(LOG_WARNING, "ioctl(set extended ACCM): %m");
#endif
}

/*
 * ppp_recv_config - configure the receive-side characteristics of
 * the ppp interface.
 */
void ppp_recv_config (int unit,int mru,u_int32_t asyncmap,int pcomp,int accomp)
{
	u_int x;
	int err;

	/*
	 * If we were called because the link has gone down then there is nothing
	 * which may be done. Just return without incident.
	 */
    if (!still_ppp(unit))
      return;

	/*
	 * Set the receiver parameters
	 */
	if (ioctl(lns[unit].fd, PPPIOCSMRU, (caddr_t) &mru) < 0)
		syslog(LOG_ERR, "ioctl(PPPIOCSMRU): %m");

	x = get_flags(unit,&err);
	if(err)
		return;
	x = accomp ? x & ~SC_REJ_COMP_AC : x | SC_REJ_COMP_AC;
	set_flags (x,unit);
}

/*
 * ccp_test - ask kernel whether a given compression method
 * is acceptable for use.
 */
int ccp_test (int ccp_unit, u_char *opt_ptr, int opt_len, int for_transmit)
{
#ifdef NEW_VERS
	struct isdn_ppp_comp_data data;
	int linkunit = ccp_fsm[ccp_unit].unit;

	memset (&data, '\0', sizeof (data));
	data.num = opt_ptr[0];
	data.optlen = opt_len - 2;
	if(data.optlen > ISDN_PPP_COMP_MAX_OPTIONS) {
		syslog(LOG_NOTICE, "ccp_test: options field too long!\n");
		return -1;
	}
	memcpy(data.options,opt_ptr+2,data.optlen);

	data.flags = 0;
	if(for_transmit)
		data.flags |= IPPP_COMP_FLAG_XMIT;
	if(ccp_fsm[ccp_unit].protocol == PPP_LINK_CCP)
		data.flags |= IPPP_COMP_FLAG_LINK;

	if (ioctl(lns[linkunit].fd, PPPIOCSCOMPRESSOR, (caddr_t) &data) >= 0)
		return 1;
	return (errno == ENOBUFS)? 0: -1;
#else
	return -1;
#endif

}

int ccp_get_compressors(int ccp_unit,unsigned long *protos)
{
  int linkunit = ccp_fsm[ccp_unit].unit;

  if (ioctl(lns[linkunit].fd, PPPIOCGCOMPRESSORS, protos) >= 0)
	return 0;
  return -1;
}


/*
 * ccp_flags_set - inform kernel about the current state of CCP.
 */
void ccp_flags_set (int ccp_unit, int isopen, int isup)
{
	int linkunit = ccp_fsm[ccp_unit].unit;
	int err;

    if (still_ppp(linkunit)) {
		int x = get_flags(linkunit,&err);
		if(err)
			return;
		x = isopen? x | SC_CCP_OPEN : x &~ SC_CCP_OPEN;
		x = isup?   x | SC_CCP_UP   : x &~ SC_CCP_UP;
		set_flags (x,linkunit);
	}
}

/*
 * ccp_fatal_error - returns 1 if decompression was disabled as a
 * result of an error detected after decompression of a packet,
 * 0 otherwise.  This is necessary because of patent nonsense.
 */
int ccp_fatal_error (int ccp_unit)
{
	int linkunit = ccp_fsm[ccp_unit].unit;
	int err;
	int x = get_flags(linkunit,&err);
	
	if(err)
		return 0;

	return x & SC_DC_FERROR;
}

/*
 * sifvjcomp - config tcp header compression
 */
int sifvjcomp (int unit, int vjcomp, int cidcomp, int maxcid)
{
	int err;
	u_int x = get_flags(unit,&err);

	if(err)
		return 0;

	if (vjcomp) {
		if (ioctl (lns[unit].fd, PPPIOCSMAXCID, (caddr_t) &maxcid) < 0) {
			syslog (LOG_ERR, "ioctl(PPPIOCSFLAGS): %m");
			vjcomp = 0;
		}
	}

	x = vjcomp  ? x | SC_COMP_TCP     : x & ~SC_COMP_TCP;
	x = cidcomp ? x & ~SC_NO_TCP_CCID : x | SC_NO_TCP_CCID;
	set_flags (x,unit);

	return 1;
}

/*
 * sifup - Config the interface up and enable IP packets to pass.
 */
int sifup (int u)
{
	struct ifreq ifr;
	u_int x;

	memset (&ifr, '\0', sizeof (ifr));
	strncpy(ifr.ifr_name, lns[u].ifname, sizeof (ifr.ifr_name));
	if (ioctl(sockfd, SIOCGIFFLAGS, (caddr_t) &ifr) < 0) {
		syslog(LOG_ERR, "Ifup: ioctl (SIOCGIFFLAGS): %m");
		return 0;
	}

	ifr.ifr_flags |= (IFF_UP | IFF_POINTOPOINT);
    if (ioctl(sockfd, SIOCSIFFLAGS, (caddr_t) &ifr) < 0) {
		syslog(LOG_ERR, "Ifup: ioctl(SIOCSIFFLAGS): %m");
		return 0;
	}
    if( ioctl(lns[u].fd,PPPIOCGFLAGS,(caddr_t) &x) < 0) {
		syslog(LOG_ERR,"Ifup: ioctl(PPPIOCGFLAGS) on %d: %m",lns[u].fd);
		return 0;
	}
	x |= SC_ENABLE_IP;
	if( ioctl(lns[u].fd,PPPIOCSFLAGS,(caddr_t) &x) < 0) {
		syslog(LOG_ERR,"Ifup: ioctl(PPPIOCSFLAGS): %m");
		return 0;
	}
	return 1;
}

/*
 * sifdown - Config the interface down and disable IP.
 */
int sifdown (int u)
{
	struct ifreq ifr;

	memset (&ifr, '\0', sizeof (ifr));
	strncpy(ifr.ifr_name, lns[u].ifname, sizeof (ifr.ifr_name));
    if (ioctl(sockfd, SIOCGIFFLAGS, (caddr_t) &ifr) < 0) {
		syslog(LOG_ERR, "ioctl (SIOCGIFFLAGS): %m");
		return 0;
	}

	ifr.ifr_flags &= ~IFF_UP;
	ifr.ifr_flags |= IFF_POINTOPOINT;
	if (ioctl(sockfd, SIOCSIFFLAGS, (caddr_t) &ifr) < 0) {
		syslog(LOG_ERR, "ioctl(SIOCSIFFLAGS): %m");
		return 0;
	}
	return 1;
}

/*
 * sifbundle 
 */
int sifbundle (int linkunit0,int linkunit1)
{
	return ioctl(lns[linkunit0].fd, PPPIOCBUNDLE, &lns[linkunit1].ifunit );
}

/*
 * sifaddr - Config the interface IP addresses and netmask.
 */
int sifaddr (int unit, int our_adr, int his_adr, int net_mask)
{
    struct ifreq   ifr; 
    struct rtentry rt;
    
    memset (&ifr, '\0', sizeof (ifr));
    memset (&rt,  '\0', sizeof (rt));
    
    SET_SA_FAMILY (ifr.ifr_addr,    AF_INET); 
    SET_SA_FAMILY (ifr.ifr_dstaddr, AF_INET); 
    SET_SA_FAMILY (ifr.ifr_netmask, AF_INET); 

    strncpy (ifr.ifr_name, lns[unit].ifname, sizeof (ifr.ifr_name));

	/*
	 *  Set our IP address
	 */
    ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr = our_adr;
    if (ioctl(sockfd, SIOCSIFADDR, (caddr_t) &ifr) < 0) {
		if (errno != EEXIST) {
			syslog (LOG_ERR, "ioctl(SIOCAIFADDR): %m");
		} else {
			syslog (LOG_WARNING, "ioctl(SIOCAIFADDR): Address already exists");
		}
		return (0);
	} 

	/*
	 *  Set the gateway address
	 */
    ((struct sockaddr_in *) &ifr.ifr_dstaddr)->sin_addr.s_addr = his_adr;
    if (ioctl(sockfd, SIOCSIFDSTADDR, (caddr_t) &ifr) < 0) {
		syslog (LOG_ERR, "ioctl(SIOCSIFDSTADDR): %m"); 
		return (0);
	} 

/*
 *  Set the netmask
 */
    if (net_mask != 0) {
		((struct sockaddr_in *) &ifr.ifr_netmask)->sin_addr.s_addr = net_mask;
		if (ioctl(sockfd, SIOCSIFNETMASK, (caddr_t) &ifr) < 0) {
			syslog (LOG_ERR, "ioctl(SIOCSIFNETMASK): %m"); 
			return (0);
		} 
	}
	last_net_mask = net_mask;

/*
 *  Add the device route
 */
	if (!devroute)
		return 1;

	if (hostroute) {
		SET_SA_FAMILY (rt.rt_dst,     AF_INET);
		SET_SA_FAMILY (rt.rt_gateway, AF_INET);
		SET_SA_FAMILY (rt.rt_genmask, AF_INET);
		rt.rt_dev = lns[unit].ifname;  /* MJC */

		if (net_mask)
			his_adr &= net_mask;

		((struct sockaddr_in *) &rt.rt_gateway)->sin_addr.s_addr = 0L;
		((struct sockaddr_in *) &rt.rt_dst)->sin_addr.s_addr     = his_adr;
		((struct sockaddr_in *) &rt.rt_genmask)->sin_addr.s_addr = net_mask;
		rt.rt_flags = RTF_UP;
		if (net_mask == 0)
			rt.rt_flags |= RTF_HOST;

		if (ioctl(sockfd, SIOCADDRT, &rt) < 0) {
			int severity;
			/*
			 * It's not fatal IMHO if the route already exists!
			 * Give a notice, though...
			 */
			if (errno == EEXIST) {
				severity = LOG_NOTICE;
			}
			else {
				severity = LOG_ERR;
			}
			syslog (severity, "ioctl(SIOCADDRT) device route (%s/%s/%08x): %m",
				rt.rt_dev,
				inet_ntoa(((struct sockaddr_in *)&rt.rt_dst)->sin_addr),
				ntohl(net_mask));
			if (severity == LOG_ERR)
				return 0;
		}

	}
	return 1;
}

/*
 * cifaddr - Clear the interface IP addresses, and delete routes
 * through the interface if possible.
 */

int cifaddr (int unit, int our_adr, int his_adr)
{
    struct rtentry rt;

/*
 *  Delete the route through the device
 */

	if (!devroute)
		return 1;

    memset (&rt, '\0', sizeof (rt));

    SET_SA_FAMILY (rt.rt_dst,     AF_INET);
    SET_SA_FAMILY (rt.rt_gateway, AF_INET);
	SET_SA_FAMILY (rt.rt_genmask, AF_INET);
    rt.rt_dev = lns[unit].ifname;  /* MJC */

	if (last_net_mask)
		his_adr &= last_net_mask;
    ((struct sockaddr_in *) &rt.rt_gateway)->sin_addr.s_addr = 0;
    ((struct sockaddr_in *) &rt.rt_dst)->sin_addr.s_addr     = his_adr;
	((struct sockaddr_in *) &rt.rt_genmask)->sin_addr.s_addr = last_net_mask;
	rt.rt_flags = RTF_UP;
	if (last_net_mask == 0)
		rt.rt_flags |= RTF_HOST;

	if (ioctl(sockfd, SIOCDELRT, &rt) < 0 && errno != ESRCH) {
		if (still_ppp(unit)) {
			syslog (LOG_ERR, "ioctl(SIOCDELRT) device route: %m");
			return (0);
		}
	}
	return 1;
}

/*
 * path_to_proc - determine the path to the proc file system data
 */

FILE *route_fd = (FILE *) 0;
static char route_buffer [512];

static char *path_to_proc (void);
static int open_route_table (void);
static void close_route_table (void);
static int read_route_table (struct rtentry *rt);
static int defaultroute_exists (void);

/*
 * path_to_proc - find the path to the route tables in the proc file system
 */

static char *path_to_proc (void)
  {
    struct mntent *mntent;
    FILE *fp;

    fp = fopen (MOUNTED, "r");
    if (fp != 0)
      {
	mntent = getmntent (fp);
        while (mntent != (struct mntent *) 0)
	  {
	    if (strcmp (mntent->mnt_type, MNTTYPE_IGNORE) != 0)
	      {
		if (strcmp (mntent->mnt_type, "proc") == 0)
		  {
		    strncpy (route_buffer, mntent->mnt_dir,
			     sizeof (route_buffer)-10);
		    route_buffer [sizeof (route_buffer)-10] = '\0';
		    fclose (fp);
		    return (route_buffer);
		  }
	      }
	    mntent = getmntent (fp);
	  }
	fclose (fp);
      }

    syslog (LOG_ERR, "proc file system not mounted");
    return 0;
  }

/*
 * close_route_table - close the interface to the route table
 */

static void close_route_table (void)
{
	if (route_fd != (FILE *) 0) {
		fclose (route_fd);
		route_fd = (FILE *) 0;
	}
}

/*
 * open_route_table - open the interface to the route table
 */

static int open_route_table (void)
  {
    char *path;

    close_route_table();

    path = path_to_proc();
    if (path == NULL)
      {
        return 0;
      }

    strcat (path, "/net/route");
    route_fd = fopen (path, "r");
    if (route_fd == (FILE *) 0)
      {
        syslog (LOG_ERR, "can not open %s: %m", path);
        return 0;
      }

    return 1;
  }

/*
 * read_route_table - read the next entry from the route table
 */

static int read_route_table (struct rtentry *rt)
  {
    static char delims[] = " \t\n";
    char *dev_ptr, *dst_ptr, *gw_ptr, *flag_ptr;
	
    memset (rt, '\0', sizeof (struct rtentry));

    for (;;)
      {
	if (fgets (route_buffer, sizeof (route_buffer), route_fd) ==
	    (char *) 0)
	  {
	    return 0;
	  }

	dev_ptr  = strtok (route_buffer, delims); /* interface name */
	dst_ptr  = strtok (NULL,         delims); /* destination address */
	gw_ptr   = strtok (NULL,         delims); /* gateway */
	flag_ptr = strtok (NULL,         delims); /* flags */
    
	if (flag_ptr == (char *) 0) /* assume that we failed, somewhere. */
	  {
	    return 0;
	  }
	
	/* Discard that stupid header line which should never
	 * have been there in the first place !! */
	if (isxdigit (*dst_ptr) && isxdigit (*gw_ptr) && isxdigit (*flag_ptr))
	  {
	    break;
	  }
      }

    ((struct sockaddr_in *) &rt->rt_dst)->sin_addr.s_addr =
      strtoul (dst_ptr, NULL, 16);

    ((struct sockaddr_in *) &rt->rt_gateway)->sin_addr.s_addr =
      strtoul (gw_ptr, NULL, 16);

    rt->rt_flags = (short) strtoul (flag_ptr, NULL, 16);
    rt->rt_dev   = dev_ptr;

    return 1;
  }

/*
 * defaultroute_exists - determine if there is a default route
 */

static int defaultroute_exists (void)
  {
    struct rtentry rt;
    int    result = 0;

    if (!open_route_table())
      return 0;

    while (read_route_table(&rt) != 0)
      {
        if (kernel_version > KVERSION(2,1,0) && SIN_ADDR(rt.rt_genmask) != 0)
	  continue;
	
        if ((rt.rt_flags & RTF_UP) == 0)
          continue;

        if (((struct sockaddr_in *) &rt.rt_dst)->sin_addr.s_addr == 0L)
	  {
            struct in_addr ina;
            ina.s_addr = ((struct sockaddr_in *) &rt.rt_gateway)->sin_addr.s_addr;
	    if (!deldefaultroute)
	      {
	        syslog (LOG_ERR,
		      "ppp not replacing existing default route to %s[%s]",
		       rt.rt_dev, inet_ntoa (ina) );
	        result = 1;
	        break;
	      }
	    else
	      {
	        SET_SA_FAMILY (rt.rt_dst,     AF_INET);
		SET_SA_FAMILY (rt.rt_gateway, AF_INET);
		rt.rt_flags = RTF_UP | RTF_GATEWAY;
		ioctl(sockfd, SIOCDELRT, &rt);
	      }
	  }
      }

    close_route_table();
    return result;
  }

/*
 * sifdefaultroute - assign a default route through the address given.
 */

int sifdefaultroute (int unit, int gateway)
{
	struct rtentry rt;

	if (has_default_route == 0) {
		if (defaultroute_exists())
			return 0;

		memset (&rt, '\0', sizeof (rt));
		SET_SA_FAMILY (rt.rt_dst,     AF_INET);
		SET_SA_FAMILY (rt.rt_gateway, AF_INET);
		((struct sockaddr_in *) &rt.rt_gateway)->sin_addr.s_addr = gateway;
    
		rt.rt_flags = RTF_UP | RTF_GATEWAY;
		if (ioctl(sockfd, SIOCADDRT, &rt) < 0) {
			int severity;
			/*
			 * It's not fatal IMHO if the route already exists!
			 * Give a notice, though...
			 */
			if (errno == EEXIST) {
				severity = LOG_NOTICE;
			}
			else {
				severity = LOG_ERR;
			}
			syslog (severity, "default route ioctl(SIOCADDRT): %m");
			if (severity == LOG_ERR)
				return 0;
		}
	}
	has_default_route = 1;
	return 1;
}

/*
 * cifdefaultroute - delete a default route through the address given.
 */
int cifdefaultroute (int unit, int gateway)
{
	struct rtentry rt;

	if (has_default_route) {
		memset (&rt, '\0', sizeof (rt));
		SET_SA_FAMILY (rt.rt_dst,     AF_INET);
		SET_SA_FAMILY (rt.rt_gateway, AF_INET);
		((struct sockaddr_in *) &rt.rt_gateway)->sin_addr.s_addr = gateway;
    
		rt.rt_flags = RTF_UP | RTF_GATEWAY;
		if (ioctl(sockfd, SIOCDELRT, &rt) < 0 && errno != ESRCH) {
			if (still_ppp(unit)) {
				syslog (LOG_ERR, "default route ioctl(SIOCDELRT): %m");
				return 0;
			}
		}
	}
	has_default_route = 0;
	return 1;
}

/*
 * sifproxyarp - Make a proxy ARP entry for the peer.
 */
int sifproxyarp (int linkunit, u_int32_t his_adr)
{
	struct arpreq arpreq;

	if (lns[linkunit].has_proxy_arp == 0) {
		memset (&arpreq, '\0', sizeof(arpreq));
/*
 * Get the hardware address of an interface on the same subnet
 * as our local address.
 */
		if (!get_ether_addr(his_adr, &arpreq.arp_ha, arpreq.arp_dev)) {
			syslog(LOG_ERR, "Cannot determine ethernet address for proxy ARP");
			return 0;
		}
    
		SET_SA_FAMILY(arpreq.arp_pa, AF_INET);
		((struct sockaddr_in *) &arpreq.arp_pa)->sin_addr.s_addr = his_adr;
		arpreq.arp_flags = ATF_PERM | ATF_PUBL;
	
		if (ioctl(sockfd, SIOCSARP, (caddr_t)&arpreq) < 0) {
			syslog(LOG_ERR, "ioctl(SIOCSARP): %m");
			return 0;
		}
	}
	lns[linkunit].has_proxy_arp = 1;
	return 1;
}

/*
 * cifproxyarp - Delete the proxy ARP entry for the peer.
 */
int cifproxyarp (int linkunit, u_int32_t his_adr)
{
	struct arpreq arpreq;

	if (lns[linkunit].has_proxy_arp == 1) {
		memset (&arpreq, '\0', sizeof(arpreq));
		SET_SA_FAMILY(arpreq.arp_pa, AF_INET);
		arpreq.arp_flags = ATF_PERM | ATF_PUBL;
    
		((struct sockaddr_in *) &arpreq.arp_pa)->sin_addr.s_addr = his_adr;
		if (ioctl(sockfd, SIOCDARP, (caddr_t)&arpreq) < 0) {
			syslog(LOG_WARNING, "ioctl(SIOCDARP): %m");
			return 0;
		}
	}
	lns[linkunit].has_proxy_arp = 0;
	return 1;
}
     
/*
 * get_ether_addr - get the hardware address of an interface on the
 * the same subnet as ipaddr.
 */
static int local_get_ether_addr (u_int32_t ipaddr, struct sockaddr *hwaddr,
   char *name, struct ifreq *ifs, int ifs_len)
{
	struct ifreq *ifr, *ifend;
	u_int32_t ina, mask;
	struct ifreq ifreq;
	struct ifconf ifc;
/*
 * Request the total list of all devices configured on your system.
 */
	ifc.ifc_len = ifs_len;
	ifc.ifc_req = ifs;
	if (ioctl(sockfd, SIOCGIFCONF, &ifc) < 0) {
		syslog(LOG_ERR, "ioctl(SIOCGIFCONF): %m");
		return 0;
	}

	MAINDEBUG ((LOG_DEBUG, "proxy arp: scanning %d interfaces for IP %s",
		ifc.ifc_len / sizeof(struct ifreq), ip_ntoa(ipaddr)));
/*
 * Scan through looking for an interface with an Internet
 * address on the same subnet as `ipaddr'.
 */
	ifend = ifs + (ifc.ifc_len / sizeof(struct ifreq));
	for (ifr = ifc.ifc_req; ifr < ifend; ifr++) {
		if (ifr->ifr_addr.sa_family == AF_INET) {
			ina = ((struct sockaddr_in *) &ifr->ifr_addr)->sin_addr.s_addr;
			strncpy(ifreq.ifr_name, ifr->ifr_name, sizeof(ifreq.ifr_name));
			MAINDEBUG ((LOG_DEBUG, "proxy arp: examining interface %s",
				ifreq.ifr_name));
/*
 * Check that the interface is up, and not point-to-point
 * nor loopback.
 */
			if (ioctl(sockfd, SIOCGIFFLAGS, &ifreq) < 0)
				continue;
			if (((ifreq.ifr_flags ^ FLAGS_GOOD) & FLAGS_MASK) != 0)
				continue;
/*
 * Get its netmask and check that it's on the right subnet.
 */
			if (ioctl(sockfd, SIOCGIFNETMASK, &ifreq) < 0)
				continue;

			mask = ((struct sockaddr_in *) &ifreq.ifr_addr)->sin_addr.s_addr;
			MAINDEBUG ((LOG_DEBUG, "proxy arp: interface addr %s mask %lx",
				ip_ntoa(ina), ntohl(mask)));

			if (((ipaddr ^ ina) & mask) != 0)
				continue;
			break;
		}
	}
    
	if(ifr >= ifend)
		return 0;

	memcpy (name, ifreq.ifr_name, sizeof(ifreq.ifr_name));
	syslog(LOG_INFO, "found interface %s for proxy arp", name);
/*
 * Now get the hardware address.
 */
	memset (&ifreq.ifr_hwaddr, 0, sizeof (struct sockaddr));
	if (ioctl (sockfd, SIOCGIFHWADDR, &ifreq) < 0) {
		syslog(LOG_ERR, "SIOCGIFHWADDR(%s): %m", ifreq.ifr_name);
		return 0;
	}

	memcpy (hwaddr, &ifreq.ifr_hwaddr, sizeof (struct sockaddr));

	MAINDEBUG ((LOG_DEBUG,
		"proxy arp: found hwaddr %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
		(int) ((unsigned char *) &hwaddr->sa_data)[0],
		(int) ((unsigned char *) &hwaddr->sa_data)[1],
		(int) ((unsigned char *) &hwaddr->sa_data)[2],
		(int) ((unsigned char *) &hwaddr->sa_data)[3],
		(int) ((unsigned char *) &hwaddr->sa_data)[4],
		(int) ((unsigned char *) &hwaddr->sa_data)[5],
		(int) ((unsigned char *) &hwaddr->sa_data)[6],
		(int) ((unsigned char *) &hwaddr->sa_data)[7]));
	return 1;
}

int get_ether_addr (u_int32_t ipaddr, struct sockaddr *hwaddr, char *name)
{
	int ifs_len;
	int answer;
	void *base_addr;
/*
 * Allocate memory to hold the request.
 */
	ifs_len = MAX_IFS * sizeof (struct ifreq);
	base_addr = (void *) malloc (ifs_len);
	if (!base_addr) {
		syslog(LOG_ERR, "malloc(%d) failed to return memory", ifs_len);
		return 0;
	}
/*
 * Find the hardware address associated with the controller
 */
	answer = local_get_ether_addr (ipaddr, hwaddr, name,
			(struct ifreq *) base_addr, ifs_len);

	free (base_addr);
	return answer;
}

/*
 * Return user specified netmask, modified by any mask we might determine
 * for address `addr' (in network byte order).
 * Here we scan through the system's list of interfaces, looking for
 * any non-point-to-point interfaces which might appear to be on the same
 * network as `addr'.  If we find any, we OR in their netmask to the
 * user-specified netmask.
 *
 * If our address happens to be in the same network/subnet as one of
 * the other interfaces, set the netmask to 255.255.255.255
 *
 * FIXME: This stuff is OLD and should be rewritten (we now use CIDR remember).
 */

static u_int32_t local_GetMask (u_int32_t addr, struct ifreq *ifs, int ifs_len)
{
    u_int32_t mask, nmask, ina;
    struct ifreq *ifr, *ifend, ifreq;
    struct ifconf ifc;

    ina = ntohl(addr);
    
    if (IN_CLASSA(ina)) {	/* determine network mask for address class */
		nmask = IN_CLASSA_NET;
	} else {
		if (IN_CLASSB(ina)) {
			nmask = IN_CLASSB_NET;
		} else {
			nmask = IN_CLASSC_NET;
		}
	}
	nmask = htonl(nmask);

	/* class D nets are disallowed by bad_ip_adrs */
	mask = netmask | nmask;

	if (ifs == (void *) 0) {
		return mask;
	}
/*
 * Scan through the system's network interfaces.
 */
	ifc.ifc_len = ifs_len;
	ifc.ifc_req = ifs;
	if (ioctl(sockfd, SIOCGIFCONF, &ifc) < 0) {
		syslog(LOG_WARNING, "ioctl(SIOCGIFCONF): %m");
		return mask;
	}
    
	ifend = (struct ifreq *) (ifc.ifc_buf + ifc.ifc_len);
	for (ifr = ifc.ifc_req; ifr < ifend; ifr++) {
		if (ifr->ifr_addr.sa_family != AF_INET) {
			continue;
		}
/*
 * Check that the interface is up, and not point-to-point nor loopback.
 */
		strncpy(ifreq.ifr_name, ifr->ifr_name, sizeof(ifreq.ifr_name));
		if (ioctl(sockfd, SIOCGIFFLAGS, &ifreq) < 0)
			continue;
	
		if (((ifreq.ifr_flags ^ FLAGS_GOOD) & FLAGS_MASK) != 0)
			continue;
/*
 * See if our IP address is part of this subnet.
 */
		ina = ((struct sockaddr_in *) &ifr->ifr_addr)->sin_addr.s_addr;
		if (ina == addr)
			continue;
		if ((ina & nmask) == (addr & nmask)) {
			mask = 0xFFFFFFFF;
			break;
		}
/*
 * Check the interface's internet address.
 */
		if (((ina ^ addr) & nmask) != 0)
			continue;
/*
 * Get its netmask and OR it into our mask.
 */
		if (ioctl(sockfd, SIOCGIFNETMASK, &ifreq) < 0)
			continue;

		mask |= ((struct sockaddr_in *)&ifreq.ifr_addr)->sin_addr.s_addr;
		break;
	}

    return mask;
}

u_int32_t GetMask (u_int32_t addr)
  {
    int ifs_len;
    u_int32_t answer;
    void *base_addr;

/*
 * If user set netmask assume he knows what he's doing.
 */
    if (netmask) return netmask;

/*
 * Allocate memory to hold the request.
 */
    ifs_len   = MAX_IFS * sizeof (struct ifreq);
    base_addr = (void *) malloc (ifs_len);
    if (base_addr == (void *) 0)
      {
       syslog(LOG_ERR, "malloc(%d) failed to return memory", ifs_len);
      }
/*
 * Find the netmask used on the same network.
 */
    answer = local_GetMask (addr, (struct ifreq *) base_addr, ifs_len);
    if (base_addr != (void *) 0)
      {
       free (base_addr);
      }
    return answer;
  }

/*
 * Internal routine to decode the version.modification.patch level
 */

static void decode_version (char *buf, int *version,
			    int *modification, int *patch)
  {
    *version      = (int) strtoul (buf, &buf, 10);
    *modification = 0;
    *patch        = 0;
    
    if (*buf == '.')
      {
	++buf;
	*modification = (int) strtoul (buf, &buf, 10);
	if (*buf == '.')
	  {
	    ++buf;
	    *patch = (int) strtoul (buf, &buf, 10);
	  }
      }
    
    if (*buf != '\0' && strncmp(buf, "-pre", 4) && strncmp(buf, "pre", 3))  /* ignore any "-preX" part */
      {
	*patch        = 0;
      }
  }

/*
 * ppp_available - check whether the system has any ppp interfaces
 * (in fact we check whether we can do an ioctl on devname).
 */
int ppp_available(char *devname)
{
	int s;
	struct ifreq ifr;
	char   abBuffer [1024];
	int    my_version, my_modification, my_patch;

/*
 * Open a socket for doing the ioctl operations.
 */    
	if( (s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return 0;
    
	strncpy (ifr.ifr_name, devname, sizeof (ifr.ifr_name));
	if(ioctl(s, SIOCGIFFLAGS, (caddr_t) &ifr) < 0)
		return 0;

/*
 *  This is the PPP device. Validate the version of the driver at this
 *  point to ensure that this program will work with the driver.
 */
	ifr.ifr_data = abBuffer;
    if (ioctl (s, SIOCGPPPVER, (caddr_t) &ifr) >= 0)
		decode_version (abBuffer, &driver_version, &driver_modification, &driver_patch);
	else
		driver_version = driver_modification = driver_patch = 0;

	close(s);

/*
 * Validate the version of the driver against the version that we used.
 */
	decode_version (IPPP_VERSION, &my_version, &my_modification, &my_patch);

    /* The version numbers must match and the modification levels must be legal */
    if ( (driver_version != my_version || driver_modification < my_modification) && !force_driver)
	{
		extern char *no_ppp_msg;
		no_ppp_msg = route_buffer;

		sprintf(no_ppp_msg,
			"Sorry - isdnPPP driver version %d.%d.%d is out of date.\n"
			"Maybe %s has no 'syncppp' encapsulation?\n",
			driver_version, driver_modification, driver_patch, devname);
		return 0;
	}
    return 1;
  }

/*
 * Update the wtmp file with the appropriate user name and tty device.
 */
int logwtmputmp (int unit,char *line, char *name, char *host)
{
	struct utmp ut, *utp;
	pid_t  mypid = getpid();

	/*
	 * Update the signon database for users.
	 * Christoph Lameter: Copied from poeigl-1.36 Jan 3, 1996
	 */
	utmpname(_PATH_UTMP);
	setutent();
	while( (utp = getutent()) && (utp->ut_pid != mypid) )
		;

	/*
	 * Is this call really necessary? There is another one after the 'put' 
	 */
	endutent();

    if (utp)
		memcpy(&ut, utp, sizeof(ut));
    else {
		/* some gettys/telnetds don't initialize utmp... */
		memset(&ut, 0, sizeof(ut));
	}

	if (ut.ut_id[0] == 0)
		strncpy(ut.ut_id, line + 3, sizeof(ut.ut_id));

	strncpy(ut.ut_user, name, sizeof(ut.ut_user));
	strncpy(ut.ut_line, line, sizeof(ut.ut_line));

	ut.ut_time = time((void *)0);

	ut.ut_type = USER_PROCESS;
	ut.ut_pid  = mypid;

	/*
	 * Insert the host name if one is supplied 
	 */
	if (*host)
		strncpy (ut.ut_host, host, sizeof(ut.ut_host));

	/*
	 * Insert the IP address of the remote system if IP is enabled 
	 */
	if (ipcp_hisoptions[unit].neg_addr)
       memcpy  (&ut.ut_addr, (char *) &ipcp_hisoptions[unit].hisaddr,
                sizeof(ut.ut_addr));

	/* 
	 * CL: Makes sure that the logout works 
	 */
	if (*host == 0 && *name==0)
		ut.ut_host[0]=0;

	pututline(&ut);
	endutent();

	/*
	 * Update the wtmp file.
	 */
#if (defined __GLIBC__ && __GLIBC__ >= 2)
	updwtmp (_PATH_WTMP, &ut);
#else
	{
		int wtmp = open(_PATH_WTMP, O_APPEND|O_WRONLY);
		if (wtmp >= 0) {
			flock(wtmp, LOCK_EX);
			/*
			 * we really should check for error on 
			 * the write for a full disk! 
			 */
			write (wtmp, (char *)&ut, sizeof(ut));
			close (wtmp);
			flock(wtmp, LOCK_UN);
		}
	}
#endif    
	return 0;
}

/*
 * Code for locking/unlocking the serial device.
 * This code is derived from chat.c.
 */

#ifndef LOCK_PREFIX
#define LOCK_PREFIX    "/var/lock/LCK.."
#endif

/*
 * lock - create a lock file for the named device
 */

int lock (char *dev)
  {
    char hdb_lock_buffer[12];
    int lfd, pid, n;
    char *p;

    p = strrchr(dev, '/');
    if (p != NULL)
      {
	dev = ++p;
      }

    lock_file = malloc(strlen(LOCK_PREFIX) + strlen(dev) + 1);
    if (lock_file == NULL)
      {
	novm("lock file name");
      }

    strcpy (lock_file, LOCK_PREFIX);
    strcat (lock_file, dev);
/*
 * Attempt to create the lock file at this point.
 */
    while (1)
      {
	lfd = open(lock_file, O_EXCL | O_CREAT | O_RDWR, 0644);
	if (lfd >= 0)
	  {
           pid = getpid();
#ifndef PID_BINARY
           sprintf (hdb_lock_buffer, "%010d\n", pid);
           write (lfd, hdb_lock_buffer, 11);
#else
           write(lfd, &pid, sizeof (pid));
#endif
	    close(lfd);
	    return 0;
	  }
/*
 * If the file exists then check to see if the pid is stale
 */
	if (errno == EEXIST)
	  {
	    lfd = open(lock_file, O_RDONLY, 0);
	    if (lfd < 0)
	      {
		if (errno == ENOENT) /* This is just a timing problem. */
		  {
		    continue;
		  }
		break;
	      }

	    /* Read the lock file to find out who has the device locked */
	    n = read (lfd, hdb_lock_buffer, 11);
	    close (lfd);
	    if (n < 0)
	      {
		syslog(LOG_ERR, "Can't read pid from lock file %s", lock_file);
		break;
	      }

	    /* See the process still exists. */
	    if (n > 0)
	      {
#ifndef PID_BINARY
 		hdb_lock_buffer[n] = '\0';
		sscanf (hdb_lock_buffer, " %d", &pid);
#else
               pid = ((int *) hdb_lock_buffer)[0];
#endif
               if (pid == 0 || (kill(pid, 0) == -1 && errno == ESRCH))
		  {
		    n = 0;
		  }
	      }

	    /* If the process does not exist then try to remove the lock */
	    if (n == 0 && unlink (lock_file) == 0)
	      {
		syslog (LOG_NOTICE, "Removed stale lock on %s (pid %d)",
			dev, pid);
		continue;
	      }

	    syslog (LOG_NOTICE, "Device %s is locked by pid %d", dev, pid);
	    break;
	  }

	syslog(LOG_ERR, "Can't create lock file %s: %m", lock_file);
	break;
      }

    free(lock_file);
    lock_file = NULL;
    return -1;
}

/*
 * unlock - remove our lockfile
 */

void unlock(void)
  {
    if (lock_file)
      {
	unlink(lock_file);
	free(lock_file);
	lock_file = NULL;
      }
  }

void setifip(int ipcp_unit)
{
  u_int32_t ouraddr,hisaddr;
  struct ifreq ifr;
  extern ipcp_options ipcp_wantoptions[NUM_PPP];

  ipcp_options *wo = &ipcp_wantoptions[ipcp_unit];

  memset (&ifr, '\0', sizeof (ifr));
  strncpy(ifr.ifr_name,lns[ipcp_fsm[ipcp_unit].unit].ifname,sizeof(ifr.ifr_name));
  
  if(ioctl(sockfd,SIOCGIFADDR,(caddr_t) &ifr) < 0)
  {
    syslog(LOG_ERR,"ioctl(SIOCGIFADDR): %m");
    return;
  }

  ouraddr = ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr;

  if(debug)
    syslog (LOG_DEBUG, "Useifip for %s: Got IF-Src-IP: %08lx\n",ifr.ifr_name,(unsigned long) ouraddr);
  
  if(ouraddr)
    wo->ouraddr = ouraddr;

  if(ioctl(sockfd,SIOCGIFDSTADDR,(caddr_t) &ifr) < 0)
  {
    syslog(LOG_ERR,"ioctl(SIOCGIFDSTADDR): %m");
    return; 
  }  
  
  hisaddr = ((struct sockaddr_in *) &ifr.ifr_dstaddr)->sin_addr.s_addr;

  if(debug)
    syslog (LOG_DEBUG, "Useifip for %s: Got IF-Dst-IP: %08lx\n",ifr.ifr_name,(unsigned long) hisaddr);

  if(hisaddr)
    wo->hisaddr = hisaddr;
}


/************************ IPX SUPPORT *********************************/
#if defined(__GLIBC__) && (__GLIBC__ > 1)
/* <linux/ipx.h> includes <linux/socket.h>, which breaks glibc 2.x support. Prevent that...   */
# define _LINUX_SOCKET_H
#endif

#if (__GLIBC__ > 2) || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 1)
  /* glibc-2.1 includes all the stuff via /usr/include/netipx/ipx.h */
#else
# include <linux/ipx.h>
#endif

/*
 * sipxfaddr - Config the interface IPX networknumber
 */

int sipxfaddr (int unit, u_int32_t network, unsigned char * node )
  {
    int    skfd; 
    int    result = 1;
/*    struct sockaddr_ipx  ipx_addr; */
    struct ifreq         ifr;
    struct sockaddr_ipx *sipx = (struct sockaddr_ipx *) &ifr.ifr_addr;

    skfd = socket (AF_IPX, SOCK_DGRAM, 0);
    if (skfd < 0)
      { 
	syslog (LOG_DEBUG, "socket(AF_IPX): %m");
	result = 0;
      }
    else
      {
	memset (&ifr, '\0', sizeof (ifr));
	strcpy (ifr.ifr_name, lns[unit].ifname);

	memcpy (sipx->sipx_node, node, IPX_NODE_LEN);
	sipx->sipx_family  = AF_IPX;
	sipx->sipx_port    = 0;
	sipx->sipx_network = htonl (network);
	sipx->sipx_type    = IPX_FRAME_ETHERII;
	sipx->sipx_action  = IPX_CRTITF;
/*
 *  Set the IPX device
 */
	if (ioctl(skfd, SIOCSIFADDR, (caddr_t) &ifr) < 0)
	  {
	    result = 0;
	    if (errno != EEXIST)
	      {
		syslog (LOG_DEBUG, "ioctl(SIOCAIFADDR, CRTITF): %m");
	      }
	    else
	      {
		syslog (LOG_WARNING,
			"ioctl(SIOCAIFADDR, CRTITF): Address already exists");
	      }
	  }
	close (skfd);
      }
    return result;
  }

/*
 * cipxfaddr - Clear the information for the IPX network. The IPX routes
 *	       are removed and the device is no longer able to pass IPX
 *	       frames.
 */

int cipxfaddr (int linkunit)
  {
    int    skfd; 
    int    result = 1;
/*    struct sockaddr_ipx  ipx_addr; */
    struct ifreq         ifr;
    struct sockaddr_ipx *sipx = (struct sockaddr_ipx *) &ifr.ifr_addr;

    skfd = socket (AF_IPX, SOCK_DGRAM, 0);
    if (skfd < 0)
      { 
	syslog (LOG_DEBUG, "socket(AF_IPX): %m");
	result = 0;
      }
    else
      {
	memset (&ifr, '\0', sizeof (ifr));
	strcpy (ifr.ifr_name, lns[linkunit].ifname);

	sipx->sipx_type    = IPX_FRAME_ETHERII;
	sipx->sipx_action  = IPX_DLTITF;
	sipx->sipx_family  = AF_IPX;
/*
 *  Set the IPX device
 */
	if (ioctl(skfd, SIOCSIFADDR, (caddr_t) &ifr) < 0)
	  {
	    syslog (LOG_INFO, "ioctl(SIOCAIFADDR, IPX_DLTITF) %d: %m", errno);
	    result = 0;
	  }
	close (skfd);
      }
    return result;
  }

/*
 * Turn off any option which is not supported by this implementation.
 *
 * This procedure is called after all of the options have been processed.
 * It gives the implementation a chance to alter the configurtion options
 * based upon the current support by the operating system.
 */

void remove_sys_options(void)
{
    struct stat stat_buf;
/*
 * Disable the IPX protocol if the support is not present in the kernel.
 * If we disable it then ensure that IP support is enabled.
 */
    while (ipxcp_protent.enabled_flag) {
	char *path = path_to_proc();
	if (path != NULL) {
	    strcat (path, "/net/ipx_interface");
	    if (lstat (path, &stat_buf) >= 0)
		break;
	}
	syslog (LOG_ERR, "IPX support is not present in the kernel\n");
	ipxcp_protent.enabled_flag = 0;
	ipcp_protent.enabled_flag = 1;
	break;
    }
}


