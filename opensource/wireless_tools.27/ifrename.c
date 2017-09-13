/*
 *	Wireless Tools
 *
 *		Jean II - HPL 04
 *
 * Main code for "ifrename". This is tool allows to rename network
 * interfaces based on various criteria (not only wireless).
 * You need to link this code against "iwlib.c" and "-lm".
 *
 * This file is released under the GPL license.
 *     Copyright (c) 2004 Jean Tourrilhes <jt@hpl.hp.com>
 */

/* 
 * This work is a nearly complete rewrite of 'nameif.c'.
 * Original CopyRight of version of 'nameif' I used is :
 * -------------------------------------------------------
 * Name Interfaces based on MAC address.
 * Writen 2000 by Andi Kleen.
 * Subject to the Gnu Public License, version 2.  
 * TODO: make it support token ring etc.
 * $Id: nameif.c,v 1.3 2003/03/06 23:26:52 ecki Exp $
 * Add hotplug compatibility : ifname -i eth0. Jean II - 03.12.03
 * Add MAC address wildcard : 01:23:45:*. Jean II - 03.12.03
 * Add interface name wildcard : wlan*. Jean II - 03.12.03
 * Add interface name probing for modular systems. Jean II - 18.02.03
 * -------------------------------------------------------
 *
 *	The last 4 patches never made it into the regular version of
 * 'nameif', and had some 'issues', which is the reason of this rewrite.
 *	Difference with 'nameif' :
 *	o 'nameif' has only a single selector, the interface MAC address.
 *	o Modular selector architecture, easily add new selectors.
 *	o hotplug invocation support.
 *	o module loading support.
 *	o MAC address wildcard.
 *	o Interface name wildcard ('eth*' or 'wlan*').
 */

/***************************** INCLUDES *****************************/

/* This is needed to enable GNU extensions such as getline & FNM_CASEFOLD */
#ifndef _GNU_SOURCE 
#define _GNU_SOURCE
#endif

#include <getopt.h>		/* getopt_long() */
#include <linux/sockios.h>	/* SIOCSIFNAME */
#include <fnmatch.h>		/* fnmatch() */
//#include <sys/syslog.h>

#include "iwlib.h"		/* Wireless Tools library */

// This would be cool, unfortunately...
//#include <linux/ethtool.h>	/* Ethtool stuff -> struct ethtool_drvinfo */

/************************ CONSTANTS & MACROS ************************/

//#define DEBUG 1

/* Our default configuration file */
const char DEFAULT_CONF[] =		"/etc/iftab"; 

/* Debian stuff */
const char DEBIAN_CONFIG_FILE[] =	"/etc/network/interfaces";

/* Backward compatibility */
#ifndef ifr_newname
#define ifr_newname ifr_ifru.ifru_slave
#endif

/* Types of selector we support. Must match selector_list */
const int SELECT_MAC		= 0;	/* Select by MAC address */
const int SELECT_ETHADDR	= 1;	/* Select by MAC address */
const int SELECT_ARP		= 2;	/* Select by ARP type */
const int SELECT_LINKTYPE	= 3;	/* Select by ARP type */
const int SELECT_DRIVER		= 4;	/* Select by Driver name */
const int SELECT_BUSINFO	= 5;	/* Select by Bus-Info */
const int SELECT_BASEADDR	= 6;	/* Select by HW Base Address */
const int SELECT_IRQ		= 7;	/* Select by HW Irq line */
const int SELECT_INTERRUPT	= 8;	/* Select by HW Irq line */
const int SELECT_IWPROTO	= 9;	/* Select by Wireless Protocol */
#define SELECT_NUM		10

#define HAS_MAC_EXACT	1
#define HAS_MAC_FILTER	2

const struct ether_addr	zero_mac = {{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

const struct option long_opt[] =
{ 
  {"config-file", 1, NULL, 'c' },
  {"debian", 0, NULL, 'd' },
  {"interface", 1, NULL, 'i' },
  {"newname", 1, NULL, 'n' },
  {"help", 0, NULL, '?' },
  {NULL, 0, NULL, '\0' }, 
};

/****************************** TYPES ******************************/

/* Cut'n'paste from ethtool.h */
#define ETHTOOL_BUSINFO_LEN	32
/* these strings are set to whatever the driver author decides... */
struct ethtool_drvinfo {
	__u32	cmd;
	char	driver[32];	/* driver short name, "tulip", "eepro100" */
	char	version[32];	/* driver version string */
	char	fw_version[32];	/* firmware version string, if applicable */
	char	bus_info[ETHTOOL_BUSINFO_LEN];	/* Bus info for this IF. */
				/* For PCI devices, use pci_dev->slot_name. */
	char	reserved1[32];
	char	reserved2[16];
	__u32	n_stats;	/* number of u64's from ETHTOOL_GSTATS */
	__u32	testinfo_len;
	__u32	eedump_len;	/* Size of data from ETHTOOL_GEEPROM (bytes) */
	__u32	regdump_len;	/* Size of data from ETHTOOL_GREGS (bytes) */
};
#define ETHTOOL_GDRVINFO	0x00000003 /* Get driver info. */

/* Description of an interface mapping */
typedef struct if_mapping
{ 
  /* Linked list */
  struct if_mapping *	next;

  /* Name of this interface */
  char			ifname[IFNAMSIZ+1];

  /* Selectors for this interface */
  int			active[SELECT_NUM];	/* Selectors active */

  /* Selector data */
  struct ether_addr	mac;			/* Exact MAC address, hex */
  char			mac_filter[6*3 + 1];	/* WildCard, ascii */
  unsigned short	hw_type;		/* Link/ARP type */
  char			driver[32];		/* driver short name */
  char		bus_info[ETHTOOL_BUSINFO_LEN];	/* Bus info for this IF. */
  unsigned short	base_addr;		/* HW Base I/O address */ 
  unsigned char		irq;			/* HW irq line */
  char			iwproto[IFNAMSIZ + 1];	/* Wireless/protocol name */
} if_mapping; 

/* Prototype for adding a selector to a mapping. Return -1 if invalid value. */
typedef int (*mapping_add)(struct if_mapping *	ifnode,
			   int *		active,
			   char *		pos,
			   size_t		len,
			   int			linenum);

/* Prototype for comparing the selector of two mapping. Return 0 if matches. */
typedef int (*mapping_cmp)(struct if_mapping *	ifnode,
			   struct if_mapping *	target);
/* Prototype for extracting selector value from live interface */
typedef int (*mapping_get)(int			skfd,
			   const char *		ifname,
			   struct if_mapping *	target,
			   int			flag);

/* How to handle a selector */
typedef struct mapping_selector
{
  char *	name;
  mapping_add	add_fn;
  mapping_cmp	cmp_fn;
  mapping_get	get_fn;
} mapping_selector;

/**************************** PROTOTYPES ****************************/

static int
	mapping_addmac(struct if_mapping *	ifnode,
		       int *			active,
		       char *			pos,
		       size_t			len,
		       int			linenum);
static int
	mapping_cmpmac(struct if_mapping *	ifnode,
		       struct if_mapping *	target);
static int
	mapping_getmac(int			skfd,
		       const char *		ifname,
		       struct if_mapping *	target,
		       int			flag);
static int
	mapping_addarp(struct if_mapping *	ifnode,
		       int *			active,
		       char *			pos,
		       size_t			len,
		       int			linenum);
static int
	mapping_cmparp(struct if_mapping *	ifnode,
		       struct if_mapping *	target);
static int
	mapping_getarp(int			skfd,
		       const char *		ifname,
		       struct if_mapping *	target,
		       int			flag);
static int
	mapping_adddriver(struct if_mapping *	ifnode,
			  int *			active,
			  char *		pos,
			  size_t		len,
			  int			linenum);
static int
	mapping_cmpdriver(struct if_mapping *	ifnode,
			  struct if_mapping *	target);
static int
	mapping_addbusinfo(struct if_mapping *	ifnode,
			   int *		active,
			   char *		pos,
			   size_t		len,
			   int			linenum);
static int
	mapping_cmpbusinfo(struct if_mapping *	ifnode,
			   struct if_mapping *	target);
static int
	mapping_getdriverbusinfo(int			skfd,
				 const char *		ifname,
				 struct if_mapping *	target,
				 int			flag);
static int
	mapping_addbaseaddr(struct if_mapping *	ifnode,
			    int *		active,
			    char *		pos,
			    size_t		len,
			    int			linenum);
static int
	mapping_cmpbaseaddr(struct if_mapping *	ifnode,
			    struct if_mapping *	target);
static int
	mapping_addirq(struct if_mapping *	ifnode,
		       int *			active,
		       char *			pos,
		       size_t			len,
		       int			linenum);
static int
	mapping_cmpirq(struct if_mapping *	ifnode,
		       struct if_mapping *	target);
static int
	mapping_getbaseaddrirq(int			skfd,
			       const char *		ifname,
			       struct if_mapping *	target,
			       int			flag);
static int
	mapping_addiwproto(struct if_mapping *	ifnode,
			   int *		active,
			   char *		pos,
			   size_t		len,
			   int			linenum);
static int
	mapping_cmpiwproto(struct if_mapping *	ifnode,
			   struct if_mapping *	target);
static int
	mapping_getiwproto(int			skfd,
			   const char *		ifname,
			   struct if_mapping *	target,
			   int			flag);

/**************************** VARIABLES ****************************/

/* List of mapping read for config file */
struct if_mapping *	mapping_list = NULL;

/* List of selectors we can handle */
const struct mapping_selector	selector_list[] =
{
  /* MAC address and ARP/Link type from ifconfig */
  { "mac", &mapping_addmac, &mapping_cmpmac, &mapping_getmac },
  { "ethaddr", &mapping_addmac, &mapping_cmpmac, &mapping_getmac },
  { "arp", &mapping_addarp, &mapping_cmparp, &mapping_getarp },
  { "linktype", &mapping_addarp, &mapping_cmparp, &mapping_getarp },
  /* Driver name and Bus-Info from ethtool -i */
  { "driver", &mapping_adddriver, &mapping_cmpdriver,
    &mapping_getdriverbusinfo },
  { "businfo", &mapping_addbusinfo, &mapping_cmpbusinfo,
    &mapping_getdriverbusinfo },
  /* Base Address and IRQ from ifconfig */
  { "baseaddress", &mapping_addbaseaddr, &mapping_cmpbaseaddr,
    &mapping_getbaseaddrirq },
  { "irq", &mapping_addirq, &mapping_cmpirq, &mapping_getbaseaddrirq },
  { "interrupt", &mapping_addirq, &mapping_cmpirq, &mapping_getbaseaddrirq },
  /* Wireless Protocol from iwconfig */
  { "iwproto", &mapping_addiwproto, &mapping_cmpiwproto, &mapping_getiwproto },
  /* The Terminator */
  { NULL, NULL, NULL, NULL },
};
const int selector_num = sizeof(selector_list)/sizeof(selector_list[0]);

/* List of active selectors */
int	selector_active[SELECT_NUM];	/* Selectors active */

/******************** INTERFACE NAME MANAGEMENT ********************/
/*
 * Bunch of low level function for managing interface names.
 */

/*------------------------------------------------------------------*/
/*
 * Compare two interface names, with wildcards.
 * We can't use fnmatch() because we don't want expansion of '[...]'
 * expressions, '\' sequences and matching of '.'.
 * We only want to match a single '*' (converted to a %d at that point)
 * to a numerical value (no ascii).
 * Return 0 is matches.
 */
static int
if_match_ifname(const char *	pattern,
		const char *	value)
{
  const char *	p;
  const char *	v;
  int		n;
  int		ret;

  /* Check for a wildcard (converted from '*' to '%d' in mapping_create()) */
  p = strstr(pattern, "%d");

  /* No wildcard, simple comparison */
  if(p == NULL)
    return(strcmp(pattern, value));

  /* Check is prefixes match */
  n = (p - pattern);
  ret = strncmp(pattern, value, n);
  if(ret)
    return(ret);

  /* Check that value has some digits at this point */
  v = value + n;
  if(!isdigit(*v))
    return(-1);

  /* Skip digits to go to value suffix */
  do
    v++;
  while(isdigit(*v));

  /* Pattern suffix */
  p += 2;

  /* Compare suffixes */
  return(strcmp(p, v));
}

/*------------------------------------------------------------------*/
/*
 * Ask the kernel to change the name of an interface.
 * That's what we want to do. All the rest is to make sure we call this
 * appropriately.
 */
static int
if_set_name(int			skfd,
	    const char *	oldname,
	    const char *	newname,
	    char *		retname)
{
  struct ifreq	ifr;
  int		ret;

  /* The kernel doesn't check is the interface already has the correct
   * name and may return an error, so check ourselves.
   * In the case of wildcard, the result can be weird : if oldname='eth0'
   * and newname='eth*', retname would be 'eth1'.
   * So, if the oldname value matches the newname pattern, just return
   * success. */
  if(!if_match_ifname(newname, oldname))
    {
#ifdef DEBUG
      fprintf(stderr, "Interface `%s' already matches `%s'.\n",
	      oldname, newname);
#endif

      strcpy(retname, oldname);
      return(0);
    }

  /* Prepare request */
  bzero(&ifr, sizeof(struct ifreq));
  strncpy(ifr.ifr_name, oldname, IFNAMSIZ); 
  strncpy(ifr.ifr_newname, newname, IFNAMSIZ); 

  /* Do it */
  ret = ioctl(skfd, SIOCSIFNAME, &ifr);

  if(!ret)
    /* Get the real new name (in case newname is a wildcard) */
    strcpy(retname, ifr.ifr_newname);

  return(ret);
}

/************************ SELECTOR HANDLING ************************/
/*
 * Handle the various selector we support
 */

/*------------------------------------------------------------------*/
/*
 * Add a MAC address selector to a mapping
 */
static int
mapping_addmac(struct if_mapping *	ifnode,
	       int *			active,
	       char *			string,
	       size_t			len,
	       int			linenum)
{
  size_t	n;

  /* Verify validity of string */
  if(len >= sizeof(ifnode->mac_filter))
    { 
      fprintf(stderr, "MAC address too long at line %d\n", linenum);  
      return(-1);
    }
  n = strspn(string, "0123456789ABCDEFabcdef:*"); 
  if(n < len)
    {
      fprintf(stderr, "Error: Invalid MAC address `%s' at line %d\n",
	      string, linenum);
      return(-1);
    }

  /* Copy as filter in all cases */
  memcpy(ifnode->mac_filter, string, len + 1); 

  /* Check the type of MAC address */
  if (strchr(ifnode->mac_filter, '*') != NULL)
    {
      /* This is a wilcard. Usual format : "01:23:45:*"
       * Unfortunately, we can't do proper parsing. */
      ifnode->active[SELECT_MAC] = HAS_MAC_FILTER;
      active[SELECT_MAC] = HAS_MAC_FILTER;
    }
  else
    {
      /* Not a wildcard : "01:23:45:67:89:AB" */
      if(iw_ether_aton(ifnode->mac_filter, &ifnode->mac) != 1)
	{
	  fprintf(stderr, "Error: Invalid MAC address `%s' at line %d\n",
		  ifnode->mac_filter, linenum);
	  return(-1);
	}

      /* Check that it's not NULL */
      if(!memcmp(&ifnode->mac, &zero_mac, 6))
	{
	  fprintf(stderr,
		  "MAC address is null at line %d, this is dangerous...\n",
		  linenum);
	}

      ifnode->active[SELECT_MAC] = HAS_MAC_EXACT;
      if(active[SELECT_MAC] == 0)
	active[SELECT_MAC] = HAS_MAC_EXACT;
    }

#ifdef DEBUG
  fprintf(stderr,
	  "Added %s MAC address `%s' from line %d.\n",
	  ifnode->active[SELECT_MAC] == HAS_MAC_FILTER ? "filter" : "exact",
	  ifnode->mac_filter, linenum);
#endif

  return(0);
}

/*------------------------------------------------------------------*/
/*
 * Compare the mac address of two mappings
 */
static int
mapping_cmpmac(struct if_mapping *	ifnode,
	       struct if_mapping *	target)
{
  /* Check for wildcard matching */
  if(ifnode->active[SELECT_MAC] == HAS_MAC_FILTER)
    /* Do wildcard matching, case insensitive */
    return(fnmatch(ifnode->mac_filter, target->mac_filter, FNM_CASEFOLD));
  else
    /* Exact matching, in hex */
    return(memcmp(&ifnode->mac.ether_addr_octet, &target->mac.ether_addr_octet,
		  6));
}

/*------------------------------------------------------------------*/
/*
 * Extract the MAC address and Link Type of an interface
 */
static int
mapping_getmac(int			skfd,
	       const char *		ifname,
	       struct if_mapping *	target,
	       int			flag)
{
  int	ret;

  /* Extract MAC address */
  ret = iw_get_mac_addr(skfd, ifname, &target->mac, &target->hw_type);
  if(ret < 0)
    {
      fprintf(stderr, "Error: Can't read MAC address on interface `%s' : %s\n",
	      ifname, strerror(errno));
      return(-1);
    }

  /* Check the type of comparison */
  if(flag == HAS_MAC_FILTER)
    {
      /* Convert to ASCII */
      iw_ether_ntop(&target->mac, target->mac_filter);
    }

  target->active[SELECT_MAC] = flag;
  target->active[SELECT_ARP] = 1;
  return(0);
}

/*------------------------------------------------------------------*/
/*
 * Add a ARP/Link type selector to a mapping
 */
static int
mapping_addarp(struct if_mapping *	ifnode,
	       int *			active,
	       char *			string,
	       size_t			len,
	       int			linenum)
{
  size_t	n;
  unsigned int	type;

  /* Verify validity of string, convert to int */
  n = strspn(string, "0123456789"); 
  if((n < len) || (sscanf(string, "%d", &type) != 1))
    {
      fprintf(stderr, "Error: Invalid ARP/Link Type `%s' at line %d\n",
	      string, linenum);
      return(-1);
    }

  ifnode->hw_type = (unsigned short) type;
  ifnode->active[SELECT_ARP] = 1;
  active[SELECT_ARP] = 1;

#ifdef DEBUG
  fprintf(stderr, "Added ARP/Link Type `%d' from line %d.\n",
	  ifnode->hw_type, linenum);
#endif

  return(0);
}

/*------------------------------------------------------------------*/
/*
 * Compare the ARP/Link type of two mappings
 */
static int
mapping_cmparp(struct if_mapping *	ifnode,
	       struct if_mapping *	target)
{
  return(!(ifnode->hw_type == target->hw_type));
}

/*------------------------------------------------------------------*/
/*
 * Extract the ARP/Link type of an interface
 */
static int
mapping_getarp(int			skfd,
	       const char *		ifname,
	       struct if_mapping *	target,
	       int			flag)
{
  /* We may have already extracted the MAC address */
  if(target->active[SELECT_MAC])
    return(0);

  /* Otherwise just do it */
  return(mapping_getmac(skfd, ifname, target, flag));
}

/*------------------------------------------------------------------*/
/*
 * Add a Driver name selector to a mapping
 */
static int
mapping_adddriver(struct if_mapping *	ifnode,
		  int *			active,
		  char *		string,
		  size_t		len,
		  int			linenum)
{
  /* Plain string, minimal verification */
  if(len >= sizeof(ifnode->driver))
    { 
      fprintf(stderr, "Driver name too long at line %d\n", linenum);  
      return(-1);
    }

  /* Copy */
  memcpy(ifnode->driver, string, len + 1); 

  /* Activate */
  ifnode->active[SELECT_DRIVER] = 1;
  active[SELECT_DRIVER] = 1;

#ifdef DEBUG
  fprintf(stderr,
	  "Added Driver name `%s' from line %d.\n",
	  ifnode->driver, linenum);
#endif

  return(0);
}

/*------------------------------------------------------------------*/
/*
 * Compare the Driver name of two mappings
 */
static int
mapping_cmpdriver(struct if_mapping *	ifnode,
		  struct if_mapping *	target)
{
  /* Do wildcard matching, case insensitive */
  return(fnmatch(ifnode->driver, target->driver, FNM_CASEFOLD));
}

/*------------------------------------------------------------------*/
/*
 * Add a Bus-Info selector to a mapping
 */
static int
mapping_addbusinfo(struct if_mapping *	ifnode,
		   int *		active,
		   char *		string,
		   size_t		len,
		   int			linenum)
{
  size_t	n;

  /* Verify validity of string */
  if(len >= sizeof(ifnode->bus_info))
    { 
      fprintf(stderr, "Bus Info too long at line %d\n", linenum);  
      return(-1);
    }
  n = strspn(string, "0123456789ABCDEFabcdef:.*"); 
  if(n < len)
    {
      fprintf(stderr, "Error: Invalid Bus Info `%s' at line %d\n",
	      string, linenum);
      return(-1);
    }

  /* Copy */
  memcpy(ifnode->bus_info, string, len + 1); 

  /* Activate */
  ifnode->active[SELECT_BUSINFO] = 1;
  active[SELECT_BUSINFO] = 1;

#ifdef DEBUG
  fprintf(stderr,
	  "Added Bus Info `%s' from line %d.\n",
	  ifnode->bus_info, linenum);
#endif

  return(0);
}

/*------------------------------------------------------------------*/
/*
 * Compare the Bus-Info of two mappings
 */
static int
mapping_cmpbusinfo(struct if_mapping *	ifnode,
		   struct if_mapping *	target)
{
  /* Do wildcard matching, case insensitive */
  return(fnmatch(ifnode->bus_info, target->bus_info, FNM_CASEFOLD));
}

/*------------------------------------------------------------------*/
/*
 * Extract the Driver name and Bus-Info from a live interface
 */
static int
mapping_getdriverbusinfo(int			skfd,
			 const char *		ifname,
			 struct if_mapping *	target,
			 int			flag)
{
  struct ifreq	ifr;
  struct ethtool_drvinfo drvinfo;
  int	ret;

  /* Avoid "Unused parameter" warning */
  flag = flag;

  /* We may come here twice, so do the job only once */
  if(target->active[SELECT_DRIVER] || target->active[SELECT_BUSINFO])
    return(0);

  /* Prepare request */
  bzero(&ifr, sizeof(struct ifreq));
  bzero(&drvinfo, sizeof(struct ethtool_drvinfo));
  strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
  drvinfo.cmd = ETHTOOL_GDRVINFO;
  ifr.ifr_data = (caddr_t) &drvinfo;

  /* Do it */
  ret = ioctl(skfd, SIOCETHTOOL, &ifr);
  if(ret < 0)
    {
#ifdef DEBUG
      /* Most drivers don't support that, keep quiet for now */
      fprintf(stderr,
	      "Error: Can't read driver/bus-info on interface `%s' : %s\n",
	      ifname, strerror(errno));
#endif
      return(-1);
    }

  /* Copy over */
  strcpy(target->driver, drvinfo.driver);
  strcpy(target->bus_info, drvinfo.bus_info);

  /* Activate */
  target->active[SELECT_DRIVER] = 1;
  target->active[SELECT_BUSINFO] = 1;
  return(0);
}

/*------------------------------------------------------------------*/
/*
 * Add a Base Address selector to a mapping
 */
static int
mapping_addbaseaddr(struct if_mapping *	ifnode,
		    int *		active,
		    char *		string,
		    size_t		len,
		    int			linenum)
{
  size_t	n;
  unsigned int	address;

  /* Verify validity of string */
  n = strspn(string, "0123456789ABCDEFabcdefx"); 
  if((n < len) || (sscanf(string, "0x%X", &address) != 1))
    {
      fprintf(stderr, "Error: Invalid Base Address `%s' at line %d\n",
	      string, linenum);
      return(-1);
    }

  /* Copy */
  ifnode->base_addr = (unsigned short) address;

  /* Activate */
  ifnode->active[SELECT_BASEADDR] = 1;
  active[SELECT_BASEADDR] = 1;

#ifdef DEBUG
  fprintf(stderr,
	  "Added Base Address `0x%X' from line %d.\n",
	  ifnode->base_addr, linenum);
#endif

  return(0);
}

/*------------------------------------------------------------------*/
/*
 * Compare the Base Address of two mappings
 */
static int
mapping_cmpbaseaddr(struct if_mapping *	ifnode,
		    struct if_mapping *	target)
{
  /* Do wildcard matching, case insensitive */
  return(!(ifnode->base_addr == target->base_addr));
}

/*------------------------------------------------------------------*/
/*
 * Add a IRQ selector to a mapping
 */
static int
mapping_addirq(struct if_mapping *	ifnode,
	       int *			active,
	       char *			string,
	       size_t			len,
	       int			linenum)
{
  size_t	n;
  unsigned int	irq;

  /* Verify validity of string */
  n = strspn(string, "0123456789"); 
  if((n < len) || (sscanf(string, "%d", &irq) != 1))
    {
      fprintf(stderr, "Error: Invalid Base Address `%s' at line %d\n",
	      string, linenum);
      return(-1);
    }

  /* Copy */
  ifnode->irq = (unsigned char) irq;

  /* Activate */
  ifnode->active[SELECT_IRQ] = 1;
  active[SELECT_IRQ] = 1;

#ifdef DEBUG
  fprintf(stderr,
	  "Added IRQ `%d' from line %d.\n",
	  ifnode->irq, linenum);
#endif

  return(0);
}

/*------------------------------------------------------------------*/
/*
 * Compare the IRQ of two mappings
 */
static int
mapping_cmpirq(struct if_mapping *	ifnode,
	       struct if_mapping *	target)
{
  /* Do wildcard matching, case insensitive */
  return(!(ifnode->irq == target->irq));
}

/*------------------------------------------------------------------*/
/*
 * Extract the Driver name and Bus-Info from a live interface
 */
static int
mapping_getbaseaddrirq(int			skfd,
		       const char *		ifname,
		       struct if_mapping *	target,
		       int			flag)
{
  struct ifreq	ifr;
  struct ifmap	map;		/* hardware setup        */
  int	ret;

  /* Avoid "Unused parameter" warning */
  flag = flag;

  /* We may come here twice, so do the job only once */
  if(target->active[SELECT_DRIVER] || target->active[SELECT_BUSINFO])
    return(0);

  /* Prepare request */
  bzero(&ifr, sizeof(struct ifreq));
  bzero(&map, sizeof(struct ifmap));
  strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

  /* Do it */
  ret = ioctl(skfd, SIOCGIFMAP, &ifr);
  if(ret < 0)
    {
#ifdef DEBUG
      /* Don't know if every interface has that, so keep quiet... */
      fprintf(stderr,
	      "Error: Can't read base address/irq on interface `%s' : %s\n",
	      ifname, strerror(errno));
#endif
      return(-1);
    }

  /* Copy over, activate */
  if(ifr.ifr_map.base_addr >= 0x100)
    {
      target->base_addr = ifr.ifr_map.base_addr;
      target->active[SELECT_BASEADDR] = 1;
    }
  target->irq = ifr.ifr_map.irq;
  target->active[SELECT_IRQ] = 1;

  return(0);
}

/*------------------------------------------------------------------*/
/*
 * Add a Wireless Protocol selector to a mapping
 */
static int
mapping_addiwproto(struct if_mapping *	ifnode,
		   int *		active,
		   char *		string,
		   size_t		len,
		   int			linenum)
{
  /* Verify validity of string */
  if(len >= sizeof(ifnode->iwproto))
    { 
      fprintf(stderr, "Wireless Protocol too long at line %d\n", linenum);  
      return(-1);
    }

  /* Copy */
  memcpy(ifnode->iwproto, string, len + 1); 

  /* Activate */
  ifnode->active[SELECT_IWPROTO] = 1;
  active[SELECT_IWPROTO] = 1;

#ifdef DEBUG
  fprintf(stderr,
	  "Added Wireless Protocol `%s' from line %d.\n",
	  ifnode->iwproto, linenum);
#endif

  return(0);
}

/*------------------------------------------------------------------*/
/*
 * Compare the Wireless Protocol of two mappings
 */
static int
mapping_cmpiwproto(struct if_mapping *	ifnode,
		   struct if_mapping *	target)
{
  /* Do wildcard matching, case insensitive */
  return(fnmatch(ifnode->iwproto, target->iwproto, FNM_CASEFOLD));
}

/*------------------------------------------------------------------*/
/*
 * Extract the Wireless Protocol from a live interface
 */
static int
mapping_getiwproto(int			skfd,
		   const char *		ifname,
		   struct if_mapping *	target,
		   int			flag)
{
  struct iwreq		wrq;

  /* Avoid "Unused parameter" warning */
  flag = flag;

  /* Get wireless name */
  if(iw_get_ext(skfd, ifname, SIOCGIWNAME, &wrq) < 0)
    /* Don't complain about it, Ethernet cards will never support this */
    return(-1);

  strncpy(target->iwproto, wrq.u.name, IFNAMSIZ);
  target->iwproto[IFNAMSIZ] = '\0';

  /* Activate */
  target->active[SELECT_IWPROTO] = 1;
  return(0);
}



/*********************** MAPPING MANAGEMENTS ***********************/
/*
 * Manage interface mappings.
 * Each mapping tell us how to identify a specific interface name.
 * It is composed of a bunch of selector values.
 */

/*------------------------------------------------------------------*/
/*
 * Create a new interface mapping and verify its name
 */
static struct if_mapping *
mapping_create(char *	pos,
	       int	len,
	       int	linenum)
{
  struct if_mapping *	ifnode;
  char *		star;

  /* Check overflow. */
  if(len > IFNAMSIZ)
    {
      fprintf(stderr, "Error: Interface name `%.*s' too long at line %d\n",
	      (int) len, pos, linenum);  
      return(NULL);
    }

  /* Create mapping, zero it */
  ifnode = calloc(1, sizeof(if_mapping));
  if(!ifnode)
    {
      fprintf(stderr, "Error: Can't allocate interface mapping.\n");  
      return(NULL);
    }

  /* Set the name, terminates it */
  memcpy(ifnode->ifname, pos, len); 
  ifnode->ifname[len] = '\0'; 

  /* Check the interface name and issue various pedantic warnings */
  if((!strcmp(ifnode->ifname, "eth0")) || (!strcmp(ifnode->ifname, "wlan0")))
    fprintf(stderr,
	    "Interface name is `%s' at line %d, can't be mapped reliably.\n",
	    ifnode->ifname, linenum);
  if(strchr(ifnode->ifname, ':'))
    fprintf(stderr, "Alias device `%s' at line %d probably can't be mapped.\n",
	    ifnode->ifname, linenum);

  /* Check for wildcard interface name, such as 'eth*' or 'wlan*'...
   * This require specific kernel support (2.6.2-rc1 and later).
   * We externally use '*', but the kernel doesn't know about that,
   * so convert it to something it knows about... */
  star = strchr(ifnode->ifname, '*');
  if(star != NULL)
    {
      /* We need an extra char */
      if(len >= IFNAMSIZ)
	{
	  fprintf(stderr,
		  "Error: Interface wildcard `%s' too long at line %d\n",
		  ifnode->ifname, linenum);  
	  free(ifnode);
	  return(NULL);
	}

      /* Replace '*' with '%d' */
      memmove(star + 2, star + 1, len + 1 - (star - ifnode->ifname));
      star[0] = '%';
      star[1] = 'd';
    }

#ifdef DEBUG
  fprintf(stderr, "Added Mapping `%s' from line %d.\n",
	  ifnode->ifname, linenum);
#endif

  /* Done */
  return(ifnode);
}

/*------------------------------------------------------------------*/
/*
 * Find the most appropriate selector matching a given selector name
 */
static inline const struct mapping_selector *
selector_find(const char *	string,
	      size_t		slen,
	      int		linenum)
{
  const struct mapping_selector *	found = NULL;
  int			ambig = 0;
  int			i;

  /* Go through all selectors */
  for(i = 0; selector_list[i].name != NULL; ++i)
    {
      /* No match -> next one */
      if(strncasecmp(selector_list[i].name, string, slen) != 0)
	continue;

      /* Exact match -> perfect */
      if(slen == strlen(selector_list[i].name))
	return &selector_list[i];

      /* Partial match */
      if(found == NULL)
	/* First time */
	found = &selector_list[i];
      else
	/* Another time */
	if (selector_list[i].add_fn != found->add_fn)
	  ambig = 1;
    }

  if(found == NULL)
    {
      fprintf(stderr, "Error: Unknown selector `%.*s' at line %d.\n",
	      (int) slen, string, linenum);
      return NULL;
    }

  if(ambig)
    {
      fprintf(stderr, "Selector `%.*s'at line %d is ambiguous.\n",
	      (int) slen, string, linenum);
      return NULL;
    }

  return found;
}

/*------------------------------------------------------------------*/
/*
 * Read the configuration file and extract all valid mappings and their
 * selectors.
 */
static int
mapping_readfile(const char *	filename)
{
  FILE *	stream;
  char *	linebuf = NULL;
  size_t	linelen = 0; 
  int		linenum = 0; 

  /* Reset the list of filters */
  bzero(selector_active, sizeof(selector_active));

  /* Check filename */
  if(!strcmp(filename, "-"))
    {
      /* Read from stdin */
      stream = stdin;

    }
  else
    {
      /* Open the file for reading */
      stream = fopen(filename, "r");
      if(!stream) 
	{
	  fprintf(stderr, "Error: Can't open configuration file `%s': %s\n",
		  filename, strerror(errno)); 
	  return(-1);
	}
    }

  /* Read each line of file
   * getline is a GNU extension :-( The buffer is recycled and increased
   * as needed by getline. */
  while(getline(&linebuf, &linelen, stream) > 0)
    {
      struct if_mapping *	ifnode;
      char *			p;
      char *			e;
      size_t			n;
      int			ret = -13;	/* Complain if no selectors */

      /* Keep track of line number */
      linenum++;

      /* Every comments terminates parsing */
      if((p = strchr(linebuf,'#')) != NULL)
	*p = '\0';

      /* Get interface name */
      p = linebuf;
      while(isspace(*p))
	++p; 
      if(*p == '\0')
	continue;	/* Line ended */
      n = strcspn(p, " \t\n");

      /* Create mapping */
      ifnode = mapping_create(p, n, linenum);
      if(!ifnode)
	continue;	/* Ignore this line */
      p += n;
      p += strspn(p, " \t\n"); 

      /* Loop on all selectors */
      while(*p != '\0')
	{
	  const struct mapping_selector *	selector = NULL;

	  /* Selector name length */
	  n = strcspn(p, " \t\n");

	  /* Find it */
	  selector = selector_find(p, n, linenum);
	  if(!selector)
	    {
	      ret = -1;
	      break;
	    }

	  /* Get to selector value */
	  p += n;
	  p += strspn(p, " \t\n"); 
	  if(*p == '\0')
	    {
	      fprintf(stderr, "Error: no value for selector `%s' on line %d\n",
		      selector->name, linenum);
	      ret = -1;
	      break;	/* Line ended */
	    }
	  /* Check for quoted arguments */
	  if(*p == '"')
	    {
	      p++;
	      e = strchr(p, '"');
	      if(e == NULL)
		{
		  fprintf(stderr,
			  "Error: unterminated quoted value on line %d\n",
			  linenum);
		  ret = -1;
		  break;	/* Line ended */
		}
	      n = e - p;
	      e++;
	    }
	  else
	    {
	      /* Just end at next blank */
	      n = strcspn(p, " \t\n");
	      e = p + n;
	    }
	  /* Make 'e' point past the '\0' we are going to add */
	  if(*e != '\0')
	    e++;
	  /* Terminate selector value */
	  p[n] = '\0';

	  /* Add it to the mapping */
	  ret = selector->add_fn(ifnode, selector_active, p, n, linenum);
	  if(ret < 0)
	    break;

	  /* Go to next selector */
	  p = e;
	  p += strspn(p, " \t\n"); 
	}

      /* We add a mapping only if it has at least one selector and if all
       * selectors were parsed properly. */
      if(ret < 0)
	{
	  /* If we have not yet printed an error, now is a good time ;-) */
	  if(ret == -13)
	    fprintf(stderr, "Error: Line %d ignored, no valid selectors\n",
		    linenum);
	  else
	    fprintf(stderr, "Error: Line %d ignored due to prior errors\n",
		    linenum);

	  free(ifnode);
	}
      else
	{
	  /* Link it in the list */
	  ifnode->next = mapping_list;
	  mapping_list = ifnode;
	}
    }

  /* Cleanup */
  free(linebuf);

  /* Finished reading, close the file */
  if(stream != stdin)
    fclose(stream);
  return(0);
}

/*------------------------------------------------------------------*/
/*
 * Extract all the interesting selectors for the interface in consideration
 */
static struct if_mapping *
mapping_extract(int		skfd,
		const char *	ifname)
{
  struct if_mapping *	target;
  int			i;

  /* Create mapping, zero it */
  target = calloc(1, sizeof(if_mapping));
  if(!target)
    {
      fprintf(stderr, "Error: Can't allocate interface mapping.\n");  
      return(NULL);
    }

  /* Set the interface name */
  strcpy(target->ifname, ifname);

  /* Loop on all active selectors */
  for(i = 0; i < SELECT_NUM; i++)
    {
      /* Check if this selector is active */
      if(selector_active[i] != 0)
	{
	  /* Extract selector */
	  selector_list[i].get_fn(skfd, ifname, target, selector_active[i]);

	  /* Ignore errors. Some mapping may not need all selectors */
	}
    }

  return(target);
} 

/*------------------------------------------------------------------*/
/*
 * Find the first mapping in the list matching the one we want.
 */
static struct if_mapping *
mapping_find(struct if_mapping *	target)
{
  struct if_mapping *	ifnode;
  int			i;

  /* Look over all our mappings */
  for(ifnode = mapping_list; ifnode != NULL; ifnode = ifnode->next)
    {
      int		matches = 1;

      /* Look over all our selectors, all must match */
      for(i = 0; i < SELECT_NUM; i++)
	{
	  /* Check if this selector is active */
	  if(ifnode->active[i] != 0)
	    {
	      /* If this selector doesn't match, game over for this mapping */
	      if((target->active[i] == 0) ||
		 (selector_list[i].cmp_fn(ifnode, target) != 0))
		{
		  matches = 0;
		  break;
		}
	    }
	}

      /* Check is this mapping was "the one" */
      if(matches)
	return(ifnode);
    }

  /* Not found */
  return(NULL);
} 

/************************** MODULE SUPPORT **************************/
/*
 * Load all necessary module so that interfaces do exist.
 * This is necessary for system that are fully modular when
 * doing the boot time processing, because we need to run before
 * 'ifup -a'.
 */

/*------------------------------------------------------------------*/
/*
 * Probe interfaces based on our list of mappings.
 * This is the default, but usually not the best way to do it.
 */
static void
probe_mappings(int		skfd)
{
  struct if_mapping *	ifnode;
  struct ether_addr	mac;			/* Exact MAC address, hex */
  unsigned short	hw_type;

  /* Look over all our mappings */
  for(ifnode = mapping_list; ifnode != NULL; ifnode = ifnode->next)
    {
      /* Can't load wildcards interface name :-( */
      if(strchr(ifnode->ifname, '%') != NULL)
	continue;

#ifdef DEBUG
      fprintf(stderr, "loading interface [%s]\n", ifnode->ifname);
#endif

      /* Trick the kernel into loading the interface.
       * This allow us to not depend on the exact path and
       * name of the '/sbin/modprobe' command.
       * Obviously, we expect this command to 'fail', as
       * the interface will load with the old/wrong name.
       */
      iw_get_mac_addr(skfd, ifnode->ifname, &mac, &hw_type);
    }
}

/*------------------------------------------------------------------*/
/*
 * Probe interfaces based on Debian's config files.
 * This allow to enly load modules for interfaces the user want active,
 * all built-in interfaces that should remain unconfigured won't
 * be probed (and can have mappings).
 */
static void
probe_debian(int		skfd)
{
  FILE *		stream;
  char *		linebuf = NULL;
  size_t		linelen = 0; 
  struct ether_addr	mac;			/* Exact MAC address, hex */
  unsigned short	hw_type;

  /* Open Debian config file */
  stream = fopen(DEBIAN_CONFIG_FILE, "r");
  if(stream == NULL)
    {
      fprintf(stderr, "Error: can't open file [%s]\n", DEBIAN_CONFIG_FILE);
      return;
    }

  /* Read each line of file
   * getline is a GNU extension :-( The buffer is recycled and increased
   * as needed by getline. */
  while(getline(&linebuf, &linelen, stream) > 0)
    {
      char *			p;
      char *			e;
      size_t			n;

      /* Check for auto keyword, ignore when commented out */
      if(!strncasecmp(linebuf, "auto ", 5))
	{
	  /* Skip "auto" keyword */
	  p = linebuf + 5;

	  /* Terminate at first comment */
	  e = strchr(p, '#');
	  if(e != NULL)
	    *e = '\0';

	  /* Loop on all interfaces given */
	  while(*p != '\0')
	    {
	      /* Interface name length */
	      n = strcspn(p, " \t\n");

	      /* Look for end of interface name */
	      e = p + n;
	      /* Make 'e' point past the '\0' we are going to add */
	      if(*e != '\0')
		e++;
	      /* Terminate interface name */
	      p[n] = '\0';

#ifdef DEBUG
	      fprintf(stderr, "loading interface [%s]\n", p);
#endif

	      /* Do it ! */
	      iw_get_mac_addr(skfd, p, &mac, &hw_type);

	      /* Go to next interface name */
	      p = e;
	      p += strspn(p, " \t\n"); 
	    }
	}
    }

  /* Done */
  fclose(stream);
  return;
}

/**************************** MAIN LOGIC ****************************/

/*------------------------------------------------------------------*/
/*
 * Rename an interface to a specified new name.
 */
static int
process_rename(int	skfd,
	       char *	ifname,
	       char *	pattern)
{
  char		newname[IFNAMSIZ+1];
  char		retname[IFNAMSIZ+1];
  int		len;
  char *	star;

  len = strlen(pattern);
  star = strchr(pattern, '*');

  /* Check newname length, need one extra char for wildcard */
  if((len + (star != NULL)) > IFNAMSIZ)
    {
      fprintf(stderr, "Error: Interface name `%s' too long.\n",
	      pattern);  
      return(-1);
    }

  /* Copy to local buffer */
  memcpy(newname, pattern, len + 1);

  /* Convert wildcard to the proper format */
  if(star != NULL)
    {
      /* Replace '*' with '%d' in the new buffer */
      star += newname - pattern;
      memmove(star + 2, star + 1, len + 1 - (star - newname));
      star[0] = '%';
      star[1] = 'd';
    }


  /* Change the name of the interface */
  if(if_set_name(skfd, ifname, newname, retname) < 0)
    {
      fprintf(stderr, "Error: cannot change name of %s to %s: %s\n",
	      ifname, newname, strerror(errno)); 
      return(-1);
    }

  /* Always print out the *new* interface name so that
   * the calling script can pick it up and know where its interface
   * has gone. */
  printf("%s\n", retname);

  /* Done */
  return(0);
}

/*------------------------------------------------------------------*/
/*
 * Process a specified interface.
 */
static int
process_ifname(int	skfd,
	       char *	ifname,
	       char *	args[],
	       int	count)
{
  struct if_mapping *		target;
  const struct if_mapping *	mapping;
  char				retname[IFNAMSIZ+1];

  /* Avoid "Unused parameter" warning */
  args = args; count = count;

  /* Get description of this interface */
  target = mapping_extract(skfd, ifname);
  if(target == NULL)
    return(-1);

  /* Find matching mapping */
  mapping = mapping_find(target);
  if(mapping == NULL)
    return(-1);

  /* Change the name of the interface */
  if(if_set_name(skfd, target->ifname, mapping->ifname, retname) < 0)
    {
      fprintf(stderr, "Error: cannot change name of %s to %s: %s\n",
	      target->ifname, mapping->ifname, strerror(errno)); 
      return(-1);
    }

  /* Check if called with an explicit interface name */
  if(!count)
    {
      /* Always print out the *new* interface name so that
       * the calling script can pick it up and know where its interface
       * has gone. */
      printf("%s\n", retname);
    }

  /* Done */
  return(0);
}

/*------------------------------------------------------------------*/
/*
 * Process all network interface present on the system.
 */
static inline int
process_iflist(int	skfd,
	       char *	args[],
	       int	count)
{
  /* Just do it */
  iw_enum_devices(skfd, &process_ifname, args, count);

  /* Done */
  return(0);
}

/******************************* MAIN *******************************/


/*------------------------------------------------------------------*/
/*
 */
static void
usage(void)
{
  fprintf(stderr, "usage: nameif [-c configurationfile] [-i ifname] [-p]\n"); 
  exit(1); 
}

/*------------------------------------------------------------------*/
/*
 * The main !
 */
int
main(int	argc,
     char *	argv[]) 
{
  const char *	conf_file = DEFAULT_CONF;
  char *	ifname = NULL;
  char *	newname = NULL;
  int		use_probe = 0;
  int		is_debian = 0;
  int		skfd;
  int		ret;

  /* Loop over all command line options */
  while(1)
    {
      int c = getopt_long(argc, argv, "c:di:n:p", long_opt, NULL);
      if(c == -1)
	break;

      switch(c)
	{ 
	default:
	case '?':
	  usage(); 
	case 'c':
	  conf_file = optarg;
	  break;
	case 'd':
	  is_debian = 1;
	  break;
	case 'i':
	  ifname = optarg;
	  break;
	case 'n':
	  newname = optarg;
	  break;
	case 'p':
	  use_probe = 1;
	  break;
	}
    }

  /* Read the specified/default config file, or stdin. */
  if(mapping_readfile(conf_file) < 0)
    return(-1);

  /* Create a channel to the NET kernel. */
  if((skfd = iw_sockets_open()) < 0)
    {
      perror("socket");
      return(-1);
    }

  /* Check if interface name was specified with -i. */
  if(ifname)
    {
      /* Check is target name specified */
      if(newname != NULL)
	{
	  /* User want to simply rename an interface to a specified name */
	  ret = process_rename(skfd, ifname, newname);
	}
      else
	{
	  /* Rename only this interface based on mappings
	   * Mostly used for HotPlug processing (from /etc/hotplug/net.agent).
	   * Process the network interface specified on the command line,
	   * and return the new name on stdout.
	   */
	  ret = process_ifname(skfd, ifname, NULL, 0);
	}
    }
  else
    {
      /* Load all the necesary modules */
      if(use_probe)
	{
	  if(is_debian)
	    probe_debian(skfd);
	  else
	    probe_mappings(skfd);
	}

      /* Rename all system interfaces
       * Mostly used for boot time processing (from init scripts).
       */
      ret = process_iflist(skfd, &newname, 1);
    }

  /* Cleanup */
  close(skfd);
  return(ret);
} 
