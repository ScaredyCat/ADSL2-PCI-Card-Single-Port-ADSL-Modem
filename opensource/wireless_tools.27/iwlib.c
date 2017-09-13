/*
 *	Wireless Tools
 *
 *		Jean II - HPLB 97->99 - HPL 99->03
 *
 * Common subroutines to all the wireless tools...
 *
 * This file is released under the GPL license.
 *     Copyright (c) 1997-2003 Jean Tourrilhes <jt@hpl.hp.com>
 */

/***************************** INCLUDES *****************************/

#include "iwlib.h"		/* Header */

/************************ CONSTANTS & MACROS ************************/

/* Various versions information */
/* Recommended Wireless Extension version */
#define WE_VERSION	16
/* Version of Wireless Tools */
#define WT_VERSION	27

/*
 * Constants fof WE-9->15
 */
#define IW15_MAX_FREQUENCIES	16
#define IW15_MAX_BITRATES	8
#define IW15_MAX_TXPOWER	8
#define IW15_MAX_ENCODING_SIZES	8
#define IW15_MAX_SPY		8
#define IW15_MAX_AP		8

/****************************** TYPES ******************************/

/*
 *	Struct iw_range up to WE-15
 */
struct	iw15_range
{
	__u32		throughput;
	__u32		min_nwid;
	__u32		max_nwid;
	__u16		num_channels;
	__u8		num_frequency;
	struct iw_freq	freq[IW15_MAX_FREQUENCIES];
	__s32		sensitivity;
	struct iw_quality	max_qual;
	__u8		num_bitrates;
	__s32		bitrate[IW15_MAX_BITRATES];
	__s32		min_rts;
	__s32		max_rts;
	__s32		min_frag;
	__s32		max_frag;
	__s32		min_pmp;
	__s32		max_pmp;
	__s32		min_pmt;
	__s32		max_pmt;
	__u16		pmp_flags;
	__u16		pmt_flags;
	__u16		pm_capa;
	__u16		encoding_size[IW15_MAX_ENCODING_SIZES];
	__u8		num_encoding_sizes;
	__u8		max_encoding_tokens;
	__u16		txpower_capa;
	__u8		num_txpower;
	__s32		txpower[IW15_MAX_TXPOWER];
	__u8		we_version_compiled;
	__u8		we_version_source;
	__u16		retry_capa;
	__u16		retry_flags;
	__u16		r_time_flags;
	__s32		min_retry;
	__s32		max_retry;
	__s32		min_r_time;
	__s32		max_r_time;
	struct iw_quality	avg_qual;
};

/*
 * Union for all the versions of iwrange.
 * Fortunately, I mostly only add fields at the end, and big-bang
 * reorganisations are few.
 */
union	iw_range_raw
{
	struct iw15_range	range15;	/* WE 9->15 */
	struct iw_range		range;		/* WE 16->current */
};

/*
 * Offsets in iw_range struct
 */
#define iwr15_off(f)	( ((char *) &(((struct iw15_range *) NULL)->f)) - \
			  (char *) NULL)
#define iwr_off(f)	( ((char *) &(((struct iw_range *) NULL)->f)) - \
			  (char *) NULL)

/**************************** VARIABLES ****************************/

/* Modes as human readable strings */
const char * const iw_operation_mode[] = { "Auto",
					"Ad-Hoc",
					"Managed",
					"Master",
					"Repeater",
					"Secondary",
					"Monitor" };

/* Disable runtime version warning in iw_get_range_info() */
int	iw_ignore_version = 0;

/************************ SOCKET SUBROUTINES *************************/

/*------------------------------------------------------------------*/
/*
 * Open a socket.
 * Depending on the protocol present, open the right socket. The socket
 * will allow us to talk to the driver.
 */
int
iw_sockets_open(void)
{
  static const int families[] = {
    AF_INET, AF_IPX, AF_AX25, AF_APPLETALK
  };
  unsigned int	i;
  int		sock;

  /*
   * Now pick any (exisiting) useful socket family for generic queries
   * Note : don't open all the socket, only returns when one matches,
   * all protocols might not be valid.
   * Workaround by Jim Kaba <jkaba@sarnoff.com>
   * Note : in 99% of the case, we will just open the inet_sock.
   * The remaining 1% case are not fully correct...
   */

  /* Try all families we support */
  for(i = 0; i < sizeof(families)/sizeof(int); ++i)
    {
      /* Try to open the socket, if success returns it */
      sock = socket(families[i], SOCK_DGRAM, 0);
      if(sock >= 0)
	return sock;
  }

  return -1;
}

/*------------------------------------------------------------------*/
/*
 * Extract the interface name out of /proc/net/wireless or /proc/net/dev.
 */
static inline char *
iw_get_ifname(char *	name,	/* Where to store the name */
	      int	nsize,	/* Size of name buffer */
	      char *	buf)	/* Current position in buffer */
{
  char *	end;

  /* Skip leading spaces */
  while(isspace(*buf))
    buf++;

#ifndef IW_RESTRIC_ENUM
  /* Get name up to the last ':'. Aliases may contain ':' in them,
   * but the last one should be the separator */
  end = strrchr(buf, ':');
#else
  /* Get name up to ": "
   * Note : we compare to ": " to make sure to process aliased interfaces
   * properly. Doesn't work on /proc/net/dev, because it doesn't guarantee
   * a ' ' after the ':'*/
  end = strstr(buf, ": ");
#endif

  /* Not found ??? To big ??? */
  if((end == NULL) || (((end - buf) + 1) > nsize))
    return(NULL);

  /* Copy */
  memcpy(name, buf, (end - buf));
  name[end - buf] = '\0';

  return(end + 2);
}

/*------------------------------------------------------------------*/
/*
 * Enumerate devices and call specified routine
 * The new way just use /proc/net/wireless, so get all wireless interfaces,
 * whether configured or not. This is the default if available.
 * The old way use SIOCGIFCONF, so get only configured interfaces (wireless
 * or not).
 */
void
iw_enum_devices(int		skfd,
		iw_enum_handler	fn,
		char *		args[],
		int		count)
{
  char		buff[1024];
  FILE *	fh;
  struct ifconf ifc;
  struct ifreq *ifr;
  int		i;

#ifndef IW_RESTRIC_ENUM
  /* Check if /proc/net/wireless is available */
  fh = fopen(PROC_NET_DEV, "r");
#else
  /* Check if /proc/net/wireless is available */
  fh = fopen(PROC_NET_WIRELESS, "r");
#endif

  if(fh != NULL)
    {
      /* Success : use data from /proc/net/wireless */

      /* Eat 2 lines of header */
      fgets(buff, sizeof(buff), fh);
      fgets(buff, sizeof(buff), fh);

      /* Read each device line */
      while(fgets(buff, sizeof(buff), fh))
	{
	  char	name[IFNAMSIZ + 1];
	  char *s;

	  /* Extract interface name */
	  s = iw_get_ifname(name, sizeof(name), buff);

	  if(!s)
	    /* Failed to parse, complain and continue */
#ifndef IW_RESTRIC_ENUM
	    fprintf(stderr, "Cannot parse " PROC_NET_DEV "\n");
#else
	    fprintf(stderr, "Cannot parse " PROC_NET_WIRELESS "\n");
#endif
	  else
	    /* Got it, print info about this interface */
	    (*fn)(skfd, name, args, count);
	}

      fclose(fh);
    }
  else
    {
      /* Get list of configured devices using "traditional" way */
      ifc.ifc_len = sizeof(buff);
      ifc.ifc_buf = buff;
      if(ioctl(skfd, SIOCGIFCONF, &ifc) < 0)
	{
	  fprintf(stderr, "SIOCGIFCONF: %s\n", strerror(errno));
	  return;
	}
      ifr = ifc.ifc_req;

      /* Print them */
      for(i = ifc.ifc_len / sizeof(struct ifreq); --i >= 0; ifr++)
	(*fn)(skfd, ifr->ifr_name, args, count);
    }
}

/*********************** WIRELESS SUBROUTINES ************************/

/*------------------------------------------------------------------*/
/*
 * Extract WE version number from /proc/net/wireless
 * In most cases, you really want to get version information from
 * the range info (range->we_version_compiled), see below...
 *
 * If we have WE-16 and later, the WE version is available at the
 * end of the header line of the file.
 * For version prior to that, we can only detect the change from
 * v11 to v12, so we do an approximate job. Fortunately, v12 to v15
 * are highly binary compatible (on the struct level).
 */
int
iw_get_kernel_we_version(void)
{
  char		buff[1024];
  FILE *	fh;
  char *	p;
  int		v;

  /* Check if /proc/net/wireless is available */
  fh = fopen(PROC_NET_WIRELESS, "r");

  if(fh == NULL)
    {
      fprintf(stderr, "Cannot read " PROC_NET_WIRELESS "\n");
      return(-1);
    }

  /* Read the first line of buffer */
  fgets(buff, sizeof(buff), fh);

  if(strstr(buff, "| WE") == NULL)
    {
      /* Prior to WE16, so explicit version not present */

      /* Black magic */
      if(strstr(buff, "| Missed") == NULL)
	v = 11;
      else
	v = 15;
      fclose(fh);
      return(v);
    }

  /* Read the second line of buffer */
  fgets(buff, sizeof(buff), fh);

  /* Get to the last separator, to get the version */
  p = strrchr(buff, '|');
  if((p == NULL) || (sscanf(p + 1, "%d", &v) != 1))
    {
      fprintf(stderr, "Cannot parse " PROC_NET_WIRELESS "\n");
      fclose(fh);
      return(-1);
    }

  fclose(fh);
  return(v);
}

/*------------------------------------------------------------------*/
/*
 * Print the WE versions of the interface.
 */
static int
print_iface_version_info(int	skfd,
			 char *	ifname,
			 char *	args[],		/* Command line args */
			 int	count)		/* Args count */
{
  struct iwreq		wrq;
  char			buffer[sizeof(iwrange) * 2];	/* Large enough */
  struct iw_range *	range;

  /* Avoid "Unused parameter" warning */
  args = args; count = count;

  /* If no wireless name : no wireless extensions.
   * This enable us to treat the SIOCGIWRANGE failure below properly. */
  if(iw_get_ext(skfd, ifname, SIOCGIWNAME, &wrq) < 0)
    return(-1);

  /* Cleanup */
  memset(buffer, 0, sizeof(buffer));

  wrq.u.data.pointer = (caddr_t) buffer;
  wrq.u.data.length = sizeof(buffer);
  wrq.u.data.flags = 0;
  if(iw_get_ext(skfd, ifname, SIOCGIWRANGE, &wrq) < 0)
    {
      /* Interface support WE (see above), but not IWRANGE */
      fprintf(stderr, "%-8.16s  Driver has no Wireless Extension version information.\n\n", ifname);
      return(0);
    }

  /* Copy stuff at the right place, ignore extra */
  range = (struct iw_range *) buffer;

  /* For new versions, we can check the version directly, for old versions
   * we use magic. 300 bytes is a also magic number, don't touch... */
  if(wrq.u.data.length >= 300)
    {
      /* Version is always at the same offset, so it's ok */
      printf("%-8.16s  Recommend Wireless Extension v%d or later,\n",
	     ifname, range->we_version_source);
      printf("          Currently compiled with Wireless Extension v%d.\n\n",
	     range->we_version_compiled);
    }
  else
    {
      fprintf(stderr, "%-8.16s  Wireless Extension version too old.\n\n",
		      ifname);
    }


  return(0);
}

/*------------------------------------------------------------------*/
/*
 * Print the WE versions of the tools.
 */
int
iw_print_version_info(const char *	toolname)
{
  int		skfd;			/* generic raw socket desc.	*/
  int		we_kernel_version;

  /* Create a channel to the NET kernel. */
  if((skfd = iw_sockets_open()) < 0)
    {
      perror("socket");
      return -1;
    }

  /* Information about the tools themselves */
  if(toolname != NULL)
    printf("%-8.16s  Wireless-Tools version %d\n", toolname, WT_VERSION);
  printf("          Compatible with Wireless Extension v11 to v%d.\n\n",
	 WE_VERSION);

  /* Get version from kernel */
  we_kernel_version = iw_get_kernel_we_version();
  /* Only version >= 16 can be verified, other are guessed */
  if(we_kernel_version > 15)
    printf("Kernel    Currently compiled with Wireless Extension v%d.\n\n",
	   we_kernel_version);

  /* Version for each device */
  iw_enum_devices(skfd, &print_iface_version_info, NULL, 0);

  close(skfd);

  return 0;
}

/*------------------------------------------------------------------*/
/*
 * Get the range information out of the driver
 */
int
iw_get_range_info(int		skfd,
		  const char *	ifname,
		  iwrange *	range)
{
  struct iwreq		wrq;
  char			buffer[sizeof(iwrange) * 2];	/* Large enough */
  union iw_range_raw *	range_raw;

  /* Cleanup */
  bzero(buffer, sizeof(buffer));

  wrq.u.data.pointer = (caddr_t) buffer;
  wrq.u.data.length = sizeof(buffer);
  wrq.u.data.flags = 0;
  if(iw_get_ext(skfd, ifname, SIOCGIWRANGE, &wrq) < 0)
    return(-1);

  /* Point to the buffer */
  range_raw = (union iw_range_raw *) buffer;

  /* For new versions, we can check the version directly, for old versions
   * we use magic. 300 bytes is a also magic number, don't touch... */
  if(wrq.u.data.length < 300)
    {
      /* That's v10 or earlier. Ouch ! Let's make a guess...*/
      range_raw->range.we_version_compiled = 9;
    }

  /* Check how it needs to be processed */
  if(range_raw->range.we_version_compiled > 15)
    {
      /* This is our native format, that's easy... */
      /* Copy stuff at the right place, ignore extra */
      memcpy((char *) range, buffer, sizeof(iwrange));
    }
  else
    {
      /* Zero unknown fields */
      bzero((char *) range, sizeof(struct iw_range));

      /* Initial part unmoved */
      memcpy((char *) range,
	     buffer,
	     iwr15_off(num_channels));
      /* Frequencies pushed futher down towards the end */
      memcpy((char *) range + iwr_off(num_channels),
	     buffer + iwr15_off(num_channels),
	     iwr15_off(sensitivity) - iwr15_off(num_channels));
      /* This one moved up */
      memcpy((char *) range + iwr_off(sensitivity),
	     buffer + iwr15_off(sensitivity),
	     iwr15_off(num_bitrates) - iwr15_off(sensitivity));
      /* This one goes after avg_qual */
      memcpy((char *) range + iwr_off(num_bitrates),
	     buffer + iwr15_off(num_bitrates),
	     iwr15_off(min_rts) - iwr15_off(num_bitrates));
      /* Number of bitrates has changed, put it after */
      memcpy((char *) range + iwr_off(min_rts),
	     buffer + iwr15_off(min_rts),
	     iwr15_off(txpower_capa) - iwr15_off(min_rts));
      /* Added encoding_login_index, put it after */
      memcpy((char *) range + iwr_off(txpower_capa),
	     buffer + iwr15_off(txpower_capa),
	     iwr15_off(txpower) - iwr15_off(txpower_capa));
      /* Hum... That's an unexpected glitch. Bummer. */
      memcpy((char *) range + iwr_off(txpower),
	     buffer + iwr15_off(txpower),
	     iwr15_off(avg_qual) - iwr15_off(txpower));
      /* Avg qual moved up next to max_qual */
      memcpy((char *) range + iwr_off(avg_qual),
	     buffer + iwr15_off(avg_qual),
	     sizeof(struct iw_quality));
    }

  /* We are now checking much less than we used to do, because we can
   * accomodate more WE version. But, there are still cases where things
   * will break... */
  if(!iw_ignore_version)
    {
      /* We don't like very old version (unfortunately kernel 2.2.X) */
      if(range->we_version_compiled <= 10)
	{
	  fprintf(stderr, "Warning: Driver for device %s has been compiled with an ancient version\n", ifname);
	  fprintf(stderr, "of Wireless Extension, while this program support version 11 and later.\n");
	  fprintf(stderr, "Some things may be broken...\n\n");
	}

      /* We don't like future versions of WE, because we can't cope with
       * the unknown */
      if(range->we_version_compiled > WE_VERSION)
	{
	  fprintf(stderr, "Warning: Driver for device %s has been compiled with version %d\n", ifname, range->we_version_compiled);
	  fprintf(stderr, "of Wireless Extension, while this program supports up to version %d.\n", WE_VERSION);
	  fprintf(stderr, "Some things may be broken...\n\n");
	}

      /* Driver version verification */
      if((range->we_version_compiled > 10) &&
	 (range->we_version_compiled < range->we_version_source))
	{
	  fprintf(stderr, "Warning: Driver for device %s recommend version %d of Wireless Extension,\n", ifname, range->we_version_source);
	  fprintf(stderr, "but has been compiled with version %d, therefore some driver features\n", range->we_version_compiled);
	  fprintf(stderr, "may not be available...\n\n");
	}
      /* Note : we are only trying to catch compile difference, not source.
       * If the driver source has not been updated to the latest, it doesn't
       * matter because the new fields are set to zero */
    }

  /* Don't complain twice.
   * In theory, the test apply to each individual driver, but usually
   * all drivers are compiled from the same kernel. */
  iw_ignore_version = 1;

  return(0);
}

/*------------------------------------------------------------------*/
/*
 * Get information about what private ioctls are supported by the driver
 *
 * Note : there is one danger using this function. If it return 0, you
 * still need to free() the buffer. Beware.
 */
int
iw_get_priv_info(int		skfd,
		 const char *	ifname,
		 iwprivargs **	ppriv)
{
  struct iwreq		wrq;
  iwprivargs *		priv = NULL;	/* Not allocated yet */
  int			maxpriv = 16;	/* Minimum for compatibility WE<13 */
  iwprivargs *		newpriv;

  /* Some driver may return a very large number of ioctls. Some
   * others a very small number. We now use a dynamic allocation
   * of the array to satisfy everybody. Of course, as we don't know
   * in advance the size of the array, we try various increasing
   * sizes. Jean II */
  do
    {
      /* (Re)allocate the buffer */
      newpriv = realloc(priv, maxpriv * sizeof(priv[0]));
      if(newpriv == NULL)
	{
	  fprintf(stderr, "%s: Allocation failed\n", __FUNCTION__);
	  break;
	}
      priv = newpriv;

      /* Ask the driver if it's large enough */
      wrq.u.data.pointer = (caddr_t) priv;
      wrq.u.data.length = maxpriv;
      wrq.u.data.flags = 0;
      if(iw_get_ext(skfd, ifname, SIOCGIWPRIV, &wrq) >= 0)
	{
	  /* Success. Pass the buffer by pointer */
	  *ppriv = priv;
	  /* Return the number of ioctls */
	  return(wrq.u.data.length);
	}

      /* Only E2BIG means the buffer was too small, abort on other errors */
      if(errno != E2BIG)
	{
	  /* Most likely "not supported". Don't barf. */
	  break;
	}

      /* Failed. We probably need a bigger buffer. Check if the kernel
       * gave us any hints. */
      if(wrq.u.data.length > maxpriv)
	maxpriv = wrq.u.data.length;
      else
	maxpriv *= 2;
    }
  while(maxpriv < 1000);

  /* Cleanup */
  if(priv)
    free(priv);
  *ppriv = NULL;

  return(-1);
}

/*------------------------------------------------------------------*/
/*
 * Get essential wireless config from the device driver
 * We will call all the classical wireless ioctl on the driver through
 * the socket to know what is supported and to get the settings...
 * Note : compare to the version in iwconfig, we extract only
 * what's *really* needed to configure a device...
 */
int
iw_get_basic_config(int			skfd,
		    const char *	ifname,
		    wireless_config *	info)
{
  struct iwreq		wrq;

  memset((char *) info, 0, sizeof(struct wireless_config));

  /* Get wireless name */
  if(iw_get_ext(skfd, ifname, SIOCGIWNAME, &wrq) < 0)
    /* If no wireless name : no wireless extensions */
    return(-1);
  else
    {
      strncpy(info->name, wrq.u.name, IFNAMSIZ);
      info->name[IFNAMSIZ] = '\0';
    }

  /* Get network ID */
  if(iw_get_ext(skfd, ifname, SIOCGIWNWID, &wrq) >= 0)
    {
      info->has_nwid = 1;
      memcpy(&(info->nwid), &(wrq.u.nwid), sizeof(iwparam));
    }

  /* Get frequency / channel */
  if(iw_get_ext(skfd, ifname, SIOCGIWFREQ, &wrq) >= 0)
    {
      info->has_freq = 1;
      info->freq = iw_freq2float(&(wrq.u.freq));
      info->freq_flags = wrq.u.freq.pad;
    }

  /* Get encryption information */
  wrq.u.data.pointer = (caddr_t) info->key;
  wrq.u.data.length = IW_ENCODING_TOKEN_MAX;
  wrq.u.data.flags = 0;
  if(iw_get_ext(skfd, ifname, SIOCGIWENCODE, &wrq) >= 0)
    {
      info->has_key = 1;
      info->key_size = wrq.u.data.length;
      info->key_flags = wrq.u.data.flags;
    }

  /* Get ESSID */
  wrq.u.essid.pointer = (caddr_t) info->essid;
  wrq.u.essid.length = IW_ESSID_MAX_SIZE + 1;
  wrq.u.essid.flags = 0;
  if(iw_get_ext(skfd, ifname, SIOCGIWESSID, &wrq) >= 0)
    {
      info->has_essid = 1;
      info->essid_on = wrq.u.data.flags;
    }

  /* Get operation mode */
  if(iw_get_ext(skfd, ifname, SIOCGIWMODE, &wrq) >= 0)
    {
      info->mode = wrq.u.mode;
      if((info->mode < IW_NUM_OPER_MODE) && (info->mode >= 0))
	info->has_mode = 1;
    }

  return(0);
}

/*------------------------------------------------------------------*/
/*
 * Set essential wireless config in the device driver
 * We will call all the classical wireless ioctl on the driver through
 * the socket to know what is supported and to set the settings...
 * We support only the restricted set as above...
 */
int
iw_set_basic_config(int			skfd,
		    const char *	ifname,
		    wireless_config *	info)
{
  struct iwreq		wrq;
  int			ret = 0;

  /* Get wireless name (check if interface is valid) */
  if(iw_get_ext(skfd, ifname, SIOCGIWNAME, &wrq) < 0)
    /* If no wireless name : no wireless extensions */
    return(-2);

  /* Set Network ID, if available (this is for non-802.11 cards) */
  if(info->has_nwid)
    {
      memcpy(&(wrq.u.nwid), &(info->nwid), sizeof(iwparam));
      wrq.u.nwid.fixed = 1;	/* Hum... When in Rome... */

      if(iw_set_ext(skfd, ifname, SIOCSIWNWID, &wrq) < 0)
	{
	  fprintf(stderr, "SIOCSIWNWID: %s\n", strerror(errno));
	  ret = -1;
	}
    }

  /* Set frequency / channel */
  if(info->has_freq)
    {
      iw_float2freq(info->freq, &(wrq.u.freq));

      if(iw_set_ext(skfd, ifname, SIOCSIWFREQ, &wrq) < 0)
	{
	  fprintf(stderr, "SIOCSIWFREQ: %s\n", strerror(errno));
	  ret = -1;
	}
    }

  /* Set encryption information */
  if(info->has_key)
    {
      int		flags = info->key_flags;

      /* Check if there is a key index */
      if((flags & IW_ENCODE_INDEX) > 0)
	{
	  /* Set the index */
	  wrq.u.data.pointer = (caddr_t) NULL;
	  wrq.u.data.flags = (flags & (IW_ENCODE_INDEX)) | IW_ENCODE_NOKEY;
	  wrq.u.data.length = 0;

	  if(iw_set_ext(skfd, ifname, SIOCSIWENCODE, &wrq) < 0)
	    {
	      fprintf(stderr, "SIOCSIWENCODE(%d): %s\n",
		      errno, strerror(errno));
	      ret = -1;
	    }
	}

      /* Mask out index to minimise probability of reject when setting key */
      flags = flags & (~IW_ENCODE_INDEX);

      /* Set the key itself (set current key in this case) */
      wrq.u.data.pointer = (caddr_t) info->key;
      wrq.u.data.length = info->key_size;
      wrq.u.data.flags = flags;

      /* Compatibility with WE<13 */
      if(flags & IW_ENCODE_NOKEY)
	wrq.u.data.pointer = NULL;

      if(iw_set_ext(skfd, ifname, SIOCSIWENCODE, &wrq) < 0)
	{
	  fprintf(stderr, "SIOCSIWENCODE(%d): %s\n",
		  errno, strerror(errno));
	  ret = -1;
	}
    }

  /* Set ESSID (extended network), if available */
  if(info->has_essid)
    {
      wrq.u.essid.pointer = (caddr_t) info->essid;
      wrq.u.essid.length = strlen(info->essid) + 1;
      wrq.u.data.flags = info->essid_on;

      if(iw_set_ext(skfd, ifname, SIOCSIWESSID, &wrq) < 0)
	{
	  fprintf(stderr, "SIOCSIWESSID: %s\n", strerror(errno));
	  ret = -1;
	}
    }

  /* Set the current mode of operation */
  if(info->has_mode)
    {
      strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
      wrq.u.mode = info->mode;

      if(iw_get_ext(skfd, ifname, SIOCSIWMODE, &wrq) < 0)
	{
	  fprintf(stderr, "SIOCSIWMODE: %s\n", strerror(errno));
	  ret = -1;
	}
    }

  return(ret);
}

/*********************** PROTOCOL SUBROUTINES ***********************/
/*
 * Fun stuff with protocol identifiers (SIOCGIWNAME).
 * We assume that drivers are returning sensible values in there,
 * which is not always the case :-(
 */

/*------------------------------------------------------------------*/
/*
 * Compare protocol identifiers.
 * We don't want to know if the two protocols are the exactly same,
 * but if they interoperate at some level, and also if they accept the
 * same type of config (ESSID vs NWID, freq...).
 * This is supposed to work around the alphabet soup.
 * Return 1 if protocols are compatible
 */
int
iw_protocol_compare(const char *	protocol1,
		    const char *	protocol2)
{
  char *	dot11 = "IEEE 802.11";
  char *	dot11_ds = "Dbg";

  /* If the strings are the same -> easy */
  if(!strncmp(protocol1, protocol2, IFNAMSIZ))
    return(1);

  /* Are we dealing with one of the 802.11 variant ? */
  if( (!strncmp(protocol1, dot11, strlen(dot11))) &&
      (!strncmp(protocol2, dot11, strlen(dot11))) )
    {
      const char *	sub1 = protocol1 + strlen(dot11);
      const char *	sub2 = protocol2 + strlen(dot11);

      /* Skip optional separator */
      if(*sub1 == '-')
	sub1++;
      if(*sub2 == '-')
	sub2++;

      /* Check if they are both 2.4 GHz Direct Sequence compatible */
      if( (strchr(dot11_ds, *sub1) != NULL) &&
	  (strchr(dot11_ds, *sub2) != NULL) )
	return(1);
    }
  /* Not compatible */
  return(0);
}

/********************** FREQUENCY SUBROUTINES ***********************/
/*
 * Note : the two functions below are the cause of troubles on
 * various embeeded platforms, as they are the reason we require
 * libm (math library).
 * In this case, please use enable BUILD_NOLIBM in the makefile
 *
 * FIXME : check negative mantissa and exponent
 */

/*------------------------------------------------------------------*/
/*
 * Convert a floating point the our internal representation of
 * frequencies.
 * The kernel doesn't want to hear about floating point, so we use
 * this custom format instead.
 */
void
iw_float2freq(double	in,
	      iwfreq *	out)
{
#ifdef WE_NOLIBM
  /* Version without libm : slower */
  out->e = 0;
  while(in > 1e9)
    {
      in /= 10;
      out->e++;
    }
  out->m = (long) in;
#else	/* WE_NOLIBM */
  /* Version with libm : faster */
  out->e = (short) (floor(log10(in)));
  if(out->e > 8)
    {
      out->m = ((long) (floor(in / pow(10,out->e - 6)))) * 100;
      out->e -= 8;
    }
  else
    {
      out->m = (long) in;
      out->e = 0;
    }
#endif	/* WE_NOLIBM */
}

/*------------------------------------------------------------------*/
/*
 * Convert our internal representation of frequencies to a floating point.
 */
double
iw_freq2float(const iwfreq *	in)
{
#ifdef WE_NOLIBM
  /* Version without libm : slower */
  int		i;
  double	res = (double) in->m;
  for(i = 0; i < in->e; i++)
    res *= 10;
  return(res);
#else	/* WE_NOLIBM */
  /* Version with libm : faster */
  return ((double) in->m) * pow(10,in->e);
#endif	/* WE_NOLIBM */
}

/*------------------------------------------------------------------*/
/*
 * Output a frequency with proper scaling
 */
void
iw_print_freq(char *	buffer,
	      int	buflen,
	      double	freq,
	      int	freq_flags)
{
  char	sep = ((freq_flags & IW_FREQ_FIXED) ? '=' : ':');

  if(freq < KILO)
    snprintf(buffer, buflen, "Channel%c%g", sep, freq);
  else
    {
      if(freq >= GIGA)
	snprintf(buffer, buflen, "Frequency%c%gGHz", sep, freq / GIGA);
      else
	{
	  if(freq >= MEGA)
	    snprintf(buffer, buflen, "Frequency%c%gMHz", sep, freq / MEGA);
	  else
	    snprintf(buffer, buflen, "Frequency%c%gkHz", sep, freq / KILO);
	}
    }
}

/*------------------------------------------------------------------*/
/*
 * Convert a frequency to a channel (negative -> error)
 */
int
iw_freq_to_channel(double			freq,
		   const struct iw_range *	range)
{
  double	ref_freq;
  int		k;

  /* Check if it's a frequency or not already a channel */
  if(freq < KILO)
    return(-1);

  /* We compare the frequencies as double to ignore differences
   * in encoding. Slower, but safer... */
  for(k = 0; k < range->num_frequency; k++)
    {
      ref_freq = iw_freq2float(&(range->freq[k]));
      if(freq == ref_freq)
	return(range->freq[k].i);
    }
  /* Not found */
  return(-2);
}

/*********************** BITRATE SUBROUTINES ***********************/

/*------------------------------------------------------------------*/
/*
 * Output a bitrate with proper scaling
 */
void
iw_print_bitrate(char *	buffer,
		 int	buflen,
		 int	bitrate)
{
  double	rate = bitrate;

  if(rate >= GIGA)
    snprintf(buffer, buflen, "%gGb/s", rate / GIGA);
  else
    if(rate >= MEGA)
      snprintf(buffer, buflen, "%gMb/s", rate / MEGA);
    else
      snprintf(buffer, buflen, "%gkb/s", rate / KILO);
}

/************************ POWER SUBROUTINES *************************/

/*------------------------------------------------------------------*/
/*
 * Convert a value in dBm to a value in milliWatt.
 */
int
iw_dbm2mwatt(int	in)
{
#ifdef WE_NOLIBM
  /* Version without libm : slower */
  int		ip = in / 10;
  int		fp = in % 10;
  int		k;
  double	res = 1.0;

  /* Split integral and floating part to avoid accumulating rounding errors */
  for(k = 0; k < ip; k++)
    res *= 10;
  for(k = 0; k < fp; k++)
    res *= LOG10_MAGIC;
  return((int) res);
#else	/* WE_NOLIBM */
  /* Version with libm : faster */
  return((int) (floor(pow(10.0, (((double) in) / 10.0)))));
#endif	/* WE_NOLIBM */
}

/*------------------------------------------------------------------*/
/*
 * Convert a value in milliWatt to a value in dBm.
 */
int
iw_mwatt2dbm(int	in)
{
#ifdef WE_NOLIBM
  /* Version without libm : slower */
  double	fin = (double) in;
  int		res = 0;

  /* Split integral and floating part to avoid accumulating rounding errors */
  while(fin > 10.0)
    {
      res += 10;
      fin /= 10.0;
    }
  while(fin > 1.000001)	/* Eliminate rounding errors, take ceil */
    {
      res += 1;
      fin /= LOG10_MAGIC;
    }
  return(res);
#else	/* WE_NOLIBM */
  /* Version with libm : faster */
  return((int) (ceil(10.0 * log10((double) in))));
#endif	/* WE_NOLIBM */
}

/********************** STATISTICS SUBROUTINES **********************/

/*------------------------------------------------------------------*/
/*
 * Read /proc/net/wireless to get the latest statistics
 */
int
iw_get_stats(int		skfd,
	     const char *	ifname,
	     iwstats *		stats,
	     const iwrange *	range,
	     int		has_range)
{
  /* Fortunately, we can always detect this condition properly */
  if((has_range) && (range->we_version_compiled > 11))
    {
      struct iwreq		wrq;
      wrq.u.data.pointer = (caddr_t) stats;
      wrq.u.data.length = 0;
      wrq.u.data.flags = 1;		/* Clear updated flag */
      strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
      if(iw_get_ext(skfd, ifname, SIOCGIWSTATS, &wrq) < 0)
	return(-1);

      /* Format has not changed since WE-12, no conversion */
      return(0);
    }
  else
    {
      FILE *	f = fopen(PROC_NET_WIRELESS, "r");
      char	buf[256];
      char *	bp;
      int	t;

      if(f==NULL)
	return -1;
      /* Loop on all devices */
      while(fgets(buf,255,f))
	{
	  bp=buf;
	  while(*bp&&isspace(*bp))
	    bp++;
	  /* Is it the good device ? */
	  if(strncmp(bp,ifname,strlen(ifname))==0 && bp[strlen(ifname)]==':')
	    {
	      /* Skip ethX: */
	      bp=strchr(bp,':');
	      bp++;
	      /* -- status -- */
	      bp = strtok(bp, " ");
	      sscanf(bp, "%X", &t);
	      stats->status = (unsigned short) t;
	      /* -- link quality -- */
	      bp = strtok(NULL, " ");
	      if(strchr(bp,'.') != NULL)
		stats->qual.updated |= 1;
	      sscanf(bp, "%d", &t);
	      stats->qual.qual = (unsigned char) t;
	      /* -- signal level -- */
	      bp = strtok(NULL, " ");
	      if(strchr(bp,'.') != NULL)
		stats->qual.updated |= 2;
	      sscanf(bp, "%d", &t);
	      stats->qual.level = (unsigned char) t;
	      /* -- noise level -- */
	      bp = strtok(NULL, " ");
	      if(strchr(bp,'.') != NULL)
		stats->qual.updated += 4;
	      sscanf(bp, "%d", &t);
	      stats->qual.noise = (unsigned char) t;
	      /* -- discarded packets -- */
	      bp = strtok(NULL, " ");
	      sscanf(bp, "%d", &stats->discard.nwid);
	      bp = strtok(NULL, " ");
	      sscanf(bp, "%d", &stats->discard.code);
	      bp = strtok(NULL, " ");
	      sscanf(bp, "%d", &stats->discard.misc);
	      fclose(f);
	      /* No conversion needed */
	      return 0;
	    }
	}
      fclose(f);
      return -1;
    }
}

/*------------------------------------------------------------------*/
/*
 * Output the link statistics, taking care of formating
 */
void
iw_print_stats(char *		buffer,
	       int		buflen,
	       const iwqual *	qual,
	       const iwrange *	range,
	       int		has_range)
{
  /* Just do it */
  if(has_range && (qual->level != 0))
    {
      /* If the statistics are in dBm */
      if(qual->level > range->max_qual.level)
	{
	  /* Statistics are in dBm (absolute power measurement) */
	  snprintf(buffer, buflen,
		  "Quality:%d/%d  Signal level:%d dBm  Noise level:%d dBm%s",
		  qual->qual, range->max_qual.qual,
		  qual->level - 0x100, qual->noise - 0x100,
		  (qual->updated & 0x7) ? " (updated)" : "");
	}
      else
	{
	  /* Statistics are relative values (0 -> max) */
	  snprintf(buffer, buflen,
		  "Quality:%d/%d  Signal level:%d/%d  Noise level:%d/%d%s",
		  qual->qual, range->max_qual.qual,
		  qual->level, range->max_qual.level,
		  qual->noise, range->max_qual.noise,
		  (qual->updated & 0x7) ? " (updated)" : "");
	}
    }
  else
    {
      /* We can't read the range, so we don't know... */
      snprintf(buffer, buflen,
	      "Quality:%d  Signal level:%d  Noise level:%d%s",
	      qual->qual, qual->level, qual->noise,
	      (qual->updated & 0x7) ? " (updated)" : "");
    }
}

/*********************** ENCODING SUBROUTINES ***********************/

/*------------------------------------------------------------------*/
/*
 * Output the encoding key, with a nice formating
 */
void
iw_print_key(char *			buffer,
	     int			buflen,
	     const unsigned char *	key,		/* Must be unsigned */
	     int			key_size,
	     int			key_flags)
{
  int	i;

  /* Check buffer size -> 1 bytes => 2 digits + 1/2 separator */
  if((key_size * 3) > buflen)
    {
      snprintf(buffer, buflen, "<too big>");
      return;
    }

  /* Is the key present ??? */
  if(key_flags & IW_ENCODE_NOKEY)
    {
      /* Nope : print on or dummy */
      if(key_size <= 0)
	strcpy(buffer, "on");			/* Size checked */
      else
	{
	  strcpy(buffer, "**");			/* Size checked */
	  buffer +=2;
	  for(i = 1; i < key_size; i++)
	    {
	      if((i & 0x1) == 0)
		strcpy(buffer++, "-");		/* Size checked */
	      strcpy(buffer, "**");		/* Size checked */
	      buffer +=2;
	    }
	}
    }
  else
    {
      /* Yes : print the key */
      sprintf(buffer, "%.2X", key[0]);		/* Size checked */
      buffer +=2;
      for(i = 1; i < key_size; i++)
	{
	  if((i & 0x1) == 0)
	    strcpy(buffer++, "-");		/* Size checked */
	  sprintf(buffer, "%.2X", key[i]);	/* Size checked */
	  buffer +=2;
	}
    }
}

/*------------------------------------------------------------------*/
/*
 * Convert a passphrase into a key
 * ### NOT IMPLEMENTED ###
 * Return size of the key, or 0 (no key) or -1 (error)
 */
static int
iw_pass_key(const char *	input,
	    unsigned char *	key)
{
  input = input; key = key;
  fprintf(stderr, "Error: Passphrase not implemented\n");
  return(-1);
}

/*------------------------------------------------------------------*/
/*
 * Parse a key from the command line.
 * Return size of the key, or 0 (no key) or -1 (error)
 */
int
iw_in_key(const char *		input,
	  unsigned char *	key)
{
  int		keylen = 0;

  /* Check the type of key */
  if(!strncmp(input, "s:", 2))
    {
      /* First case : as an ASCII string (Lucent/Agere cards) */
      keylen = strlen(input + 2);		/* skip "s:" */
      if(keylen > IW_ENCODING_TOKEN_MAX)
	keylen = IW_ENCODING_TOKEN_MAX;
      strncpy(key, input + 2, keylen);
    }
  else
    if(!strncmp(input, "p:", 2))
      {
	/* Second case : as a passphrase (PrismII cards) */
	return(iw_pass_key(input + 2, key));		/* skip "p:" */
      }
    else
      {
	char *	buff;
	char *	hex;
	char *	out;
	char *	p;

	/* Third case : as hexadecimal digits */
	buff = malloc(IW_ENCODING_TOKEN_MAX + strlen(input) + 1);
	if(buff == NULL)
	  {
	    fprintf(stderr, "Malloc failed (string too long ?)\n");
	    return(-1);
	  }
	/* Preserve original buffers (both in & out) */
	hex = buff + IW_ENCODING_TOKEN_MAX;
	strcpy(hex, input);				/* Size checked */
	out = buff;

	/* Parse */
	p = strtok(hex, "-:;.,");
	while((p != (char *) NULL) && (keylen < IW_ENCODING_TOKEN_MAX))
	  {
	    int	temph;
	    int	templ;
	    int	count;
	    int	len;
	    /* Get each char separatly (and not by two) so that we don't
	     * get confused by 'enc' (=> '0E'+'0C') and similar */
	    count = sscanf(p, "%1X%1X", &temph, &templ);
	    if(count < 1)
	      return(-1);		/* Error -> non-hex char */
	    /* Fixup odd strings such as '123' is '01'+'23' and not '12'+'03'*/
	    len = strlen(p);
	    if(len % 2)
	      count = 1;
	    /* Put back two chars as one byte */
	    if(count == 2)
	      templ |= temph << 4;
	    else
	      templ = temph;
	    out[keylen++] = (unsigned char) (templ & 0xFF);
	    /* Check where to get next char from */
	    if(len > count)	/* Token not finished yet */
	      p += count;
	    else
	      p = strtok((char *) NULL, "-:;.,");
	  }
	memcpy(key, out, keylen);
	free(buff);
      }

  return(keylen);
}

/*------------------------------------------------------------------*/
/*
 * Parse a key from the command line.
 * Return size of the key, or 0 (no key) or -1 (error)
 */
int
iw_in_key_full(int		skfd,
	       const char *	ifname,
	       const char *	input,
	       unsigned char *	key,
	       __u16 *		flags)
{
  int		keylen = 0;
  char *	p;

  if(!strncmp(input, "l:", 2))
    {
      struct iw_range	range;

      /* Extra case : as a login (user:passwd - Cisco LEAP) */
      keylen = strlen(input + 2) + 1;		/* skip "l:", add '\0' */
      /* Most user/password is 8 char, so 18 char total, < 32 */
      if(keylen > IW_ENCODING_TOKEN_MAX)
	keylen = IW_ENCODING_TOKEN_MAX;
      memcpy(key, input + 2, keylen);

      /* Separate the two strings */
      p = strchr(key, ':');
      if(p == NULL)
	{
	  fprintf(stderr, "Error: Invalid login format\n");
	  return(-1);
	}
      *p = '\0';

      /* Extract range info */
      if(iw_get_range_info(skfd, ifname, &range) < 0)
	/* Hum... Maybe we should return an error ??? */
	memset(&range, 0, sizeof(range));

      if(range.we_version_compiled > 15)
	{

	  printf("flags = %X, index = %X\n",
		 *flags, range.encoding_login_index);
	  if((*flags & IW_ENCODE_INDEX) == 0)
	    {
	      /* Extract range info */
	      if(iw_get_range_info(skfd, ifname, &range) < 0)
		memset(&range, 0, sizeof(range));
	      printf("flags = %X, index = %X\n", *flags, range.encoding_login_index);
	      /* Set the index the driver expects */
	      *flags |= range.encoding_login_index & IW_ENCODE_INDEX;
	    }
	  printf("flags = %X, index = %X\n", *flags, range.encoding_login_index);
	}
    }
  else
    /* Simpler routine above */
    keylen = iw_in_key(input, key);

  return(keylen);
}

/******************* POWER MANAGEMENT SUBROUTINES *******************/

/*------------------------------------------------------------------*/
/*
 * Output a power management value with all attributes...
 */
void
iw_print_pm_value(char *	buffer,
		  int		buflen,
		  int		value,
		  int		flags)
{
  /* Check size */
  if(buflen < 25)
    {
      snprintf(buffer, buflen, "<too big>");
      return;
    }
  buflen -= 25;

  /* Modifiers */
  if(flags & IW_POWER_MIN)
    {
      strcpy(buffer, " min");				/* Size checked */
      buffer += 4;
    }
  if(flags & IW_POWER_MAX)
    {
      strcpy(buffer, " max");				/* Size checked */
      buffer += 4;
    }

  /* Type */
  if(flags & IW_POWER_TIMEOUT)
    {
      strcpy(buffer, " timeout:");			/* Size checked */
      buffer += 9;
    }
  else
    {
      strcpy(buffer, " period:");			/* Size checked */
      buffer += 8;
    }

  /* Display value without units */
  if(flags & IW_POWER_RELATIVE)
    snprintf(buffer, buflen, "%g", ((double) value) / MEGA);
  else
    {
      /* Display value with units */
      if(value >= (int) MEGA)
	snprintf(buffer, buflen, "%gs", ((double) value) / MEGA);
      else
	if(value >= (int) KILO)
	  snprintf(buffer, buflen, "%gms", ((double) value) / KILO);
	else
	  snprintf(buffer, buflen, "%dus", value);
    }
}

/*------------------------------------------------------------------*/
/*
 * Output a power management mode
 */
void
iw_print_pm_mode(char *	buffer,
		 int	buflen,
		 int	flags)
{
  /* Check size */
  if(buflen < 28)
    {
      snprintf(buffer, buflen, "<too big>");
      return;
    }

  /* Print the proper mode... */
  switch(flags & IW_POWER_MODE)
    {
    case IW_POWER_UNICAST_R:
      strcpy(buffer, "mode:Unicast only received");	/* Size checked */
      break;
    case IW_POWER_MULTICAST_R:
      strcpy(buffer, "mode:Multicast only received");	/* Size checked */
      break;
    case IW_POWER_ALL_R:
      strcpy(buffer, "mode:All packets received");	/* Size checked */
      break;
    case IW_POWER_FORCE_S:
      strcpy(buffer, "mode:Force sending");		/* Size checked */
      break;
    case IW_POWER_REPEATER:
      strcpy(buffer, "mode:Repeat multicasts");		/* Size checked */
      break;
    default:
      strcpy(buffer, "");				/* Size checked */
      break;
    }
}

/***************** RETRY LIMIT/LIFETIME SUBROUTINES *****************/

/*------------------------------------------------------------------*/
/*
 * Output a retry value with all attributes...
 */
void
iw_print_retry_value(char *	buffer,
		     int	buflen,
		     int	value,
		     int	flags)
{
  /* Check buffer size */
  if(buflen < 18)
    {
      snprintf(buffer, buflen, "<too big>");
      return;
    }
  buflen -= 18;

  /* Modifiers */
  if(flags & IW_RETRY_MIN)
    {
      strcpy(buffer, " min");				/* Size checked */
      buffer += 4;
    }
  if(flags & IW_RETRY_MAX)
    {
      strcpy(buffer, " max");				/* Size checked */
      buffer += 4;
    }

  /* Type lifetime of limit */
  if(flags & IW_RETRY_LIFETIME)
    {
      strcpy(buffer, " lifetime:");			/* Size checked */
      buffer += 10;

      /* Display value without units */
      if(flags & IW_POWER_RELATIVE)
	snprintf(buffer, buflen, "%g", ((double) value) / MEGA);
      else
	{
	  /* Display value with units */
	  if(value >= (int) MEGA)
	    snprintf(buffer, buflen, "%gs", ((double) value) / MEGA);
	  else
	    if(value >= (int) KILO)
	      snprintf(buffer, buflen, "%gms", ((double) value) / KILO);
	    else
	      snprintf(buffer, buflen, "%dus", value);
	}
    }
  else
    snprintf(buffer, buflen, " limit:%d", value);
}

/************************* TIME SUBROUTINES *************************/

/*------------------------------------------------------------------*/
/*
 * Print timestamps
 * Inspired from irdadump...
 */
void
iw_print_timeval(char *			buffer,
		 int			buflen,
		 const struct timeval *	time)
{
        int s;

	s = (time->tv_sec) % 86400;
	snprintf(buffer, buflen, "%02d:%02d:%02d.%06u ", 
		s / 3600, (s % 3600) / 60, 
		s % 60, (u_int32_t) time->tv_usec);
}

/*********************** ADDRESS SUBROUTINES ************************/
/*
 * This section is mostly a cut & past from net-tools-1.2.0
 * manage address display and input...
 */

/*------------------------------------------------------------------*/
/*
 * Check if interface support the right MAC address type...
 */
int
iw_check_mac_addr_type(int		skfd,
		       char *		ifname)
{
  struct ifreq		ifr;

  /* Get the type of hardware address */
  strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
  if((ioctl(skfd, SIOCGIFHWADDR, &ifr) < 0) ||
     (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER))
    {
      /* Deep trouble... */
      fprintf(stderr, "Interface %s doesn't support MAC addresses\n",
	     ifname);
      return(-1);
    }

#ifdef DEBUG
  printf("Hardware : %d - %s\n", ifr.ifr_hwaddr.sa_family,
	 iw_ether_ntoa((struct ether_addr *) ifr.ifr_hwaddr.sa_data));
#endif

  return(0);
}


/*------------------------------------------------------------------*/
/*
 * Check if interface support the right interface address type...
 */
int
iw_check_if_addr_type(int		skfd,
		      char *		ifname)
{
  struct ifreq		ifr;

  /* Get the type of interface address */
  strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
  if((ioctl(skfd, SIOCGIFADDR, &ifr) < 0) ||
     (ifr.ifr_addr.sa_family !=  AF_INET))
    {
      /* Deep trouble... */
      fprintf(stderr, "Interface %s doesn't support IP addresses\n", ifname);
      return(-1);
    }

#ifdef DEBUG
  printf("Interface : %d - 0x%lX\n", ifr.ifr_addr.sa_family,
	 *((unsigned long *) ifr.ifr_addr.sa_data));
#endif

  return(0);
}

#if 0
/*------------------------------------------------------------------*/
/*
 * Check if interface support the right address types...
 */
int
iw_check_addr_type(int		skfd,
		   char *	ifname)
{
  /* Check the interface address type */
  if(iw_check_if_addr_type(skfd, ifname) < 0)
    return(-1);

  /* Check the interface address type */
  if(iw_check_mac_addr_type(skfd, ifname) < 0)
    return(-1);

  return(0);
}
#endif

/*------------------------------------------------------------------*/
/*
 * Ask the kernel for the MAC address of an interface.
 */
int
iw_get_mac_addr(int			skfd,
		const char *		ifname,
		struct ether_addr *	eth,
		unsigned short *	ptype)
{
  struct ifreq	ifr;
  int		ret;

  /* Prepare request */
  bzero(&ifr, sizeof(struct ifreq));
  strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

  /* Do it */
  ret = ioctl(skfd, SIOCGIFHWADDR, &ifr);

  memcpy(eth->ether_addr_octet, ifr.ifr_hwaddr.sa_data, 6); 
  *ptype = ifr.ifr_hwaddr.sa_family;
  return(ret);
}

/*------------------------------------------------------------------*/
/*
 * Display an Ethernet address in readable format.
 */
void
iw_ether_ntop(const struct ether_addr* eth, char* buf)
{
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
	  eth->ether_addr_octet[0], eth->ether_addr_octet[1],
	  eth->ether_addr_octet[2], eth->ether_addr_octet[3],
	  eth->ether_addr_octet[4], eth->ether_addr_octet[5]);
}

/*------------------------------------------------------------------*/
/*
 * Display an Ethernet address in readable format.
 * Same with a static buffer
 */
char *
iw_ether_ntoa(const struct ether_addr* eth)
{
  static char buf[20];
  iw_ether_ntop(eth, buf);
  return buf;
}

/*------------------------------------------------------------------*/
/*
 * Input an Ethernet address and convert to binary.
 */
int
iw_ether_aton(const char *orig, struct ether_addr *eth)
{
  const char *bufp;
  int i;

  i = 0;
  for(bufp = orig; *bufp != '\0'; ++bufp) {
	unsigned int val;
	unsigned char c = *bufp++;
	if (isdigit(c)) val = c - '0';
	else if (c >= 'a' && c <= 'f') val = c - 'a' + 10;
	else if (c >= 'A' && c <= 'F') val = c - 'A' + 10;
	else break;

	val <<= 4;
	c = *bufp++;
	if (isdigit(c)) val |= c - '0';
	else if (c >= 'a' && c <= 'f') val |= c - 'a' + 10;
	else if (c >= 'A' && c <= 'F') val |= c - 'A' + 10;
	else break;

	eth->ether_addr_octet[i] = (unsigned char) (val & 0377);
	if(++i == ETH_ALEN) {
		/* That's it.  Any trailing junk? */
		if (*bufp != '\0') {
#ifdef DEBUG
			fprintf(stderr, "iw_ether_aton(%s): trailing junk!\n", orig);
			errno = EINVAL;
			return(0);
#endif
		}
#ifdef DEBUG
		fprintf(stderr, "iw_ether_aton(%s): %s\n",
			orig, ether_ntoa(eth));
#endif
		return(1);
	}
	if (*bufp != ':')
		break;
  }

#ifdef DEBUG
  fprintf(stderr, "iw_ether_aton(%s): invalid ether address!\n", orig);
#endif
  errno = EINVAL;
  return(0);
}


/*------------------------------------------------------------------*/
/*
 * Input an Internet address and convert to binary.
 */
int
iw_in_inet(char *name, struct sockaddr *sap)
{
  struct hostent *hp;
  struct netent *np;
  struct sockaddr_in *sain = (struct sockaddr_in *) sap;

  /* Grmpf. -FvK */
  sain->sin_family = AF_INET;
  sain->sin_port = 0;

  /* Default is special, meaning 0.0.0.0. */
  if (!strcmp(name, "default")) {
	sain->sin_addr.s_addr = INADDR_ANY;
	return(1);
  }

  /* Try the NETWORKS database to see if this is a known network. */
  if ((np = getnetbyname(name)) != (struct netent *)NULL) {
	sain->sin_addr.s_addr = htonl(np->n_net);
	strcpy(name, np->n_name);
	return(1);
  }

  /* Always use the resolver (DNS name + IP addresses) */
  if ((hp = gethostbyname(name)) == (struct hostent *)NULL) {
	errno = h_errno;
	return(-1);
  }
  memcpy((char *) &sain->sin_addr, (char *) hp->h_addr_list[0], hp->h_length);
  strcpy(name, hp->h_name);
  return(0);
}

/*------------------------------------------------------------------*/
/*
 * Input an address and convert to binary.
 */
int
iw_in_addr(int		skfd,
	   char *	ifname,
	   char *	bufp,
	   struct sockaddr *sap)
{
  /* Check if it is a hardware or IP address */
  if(index(bufp, ':') == NULL)
    {
      struct sockaddr	if_address;
      struct arpreq	arp_query;

      /* Check if we have valid interface address type */
      if(iw_check_if_addr_type(skfd, ifname) < 0)
	{
	  fprintf(stderr, "%-8.16s  Interface doesn't support IP addresses\n", ifname);
	  return(-1);
	}

      /* Read interface address */
      if(iw_in_inet(bufp, &if_address) < 0)
	{
	  fprintf(stderr, "Invalid interface address %s\n", bufp);
	  return(-1);
	}

      /* Translate IP addresses to MAC addresses */
      memcpy((char *) &(arp_query.arp_pa),
	     (char *) &if_address,
	     sizeof(struct sockaddr));
      arp_query.arp_ha.sa_family = 0;
      arp_query.arp_flags = 0;
      /* The following restrict the search to the interface only */
      /* For old kernels which complain, just comment it... */
      strncpy(arp_query.arp_dev, ifname, IFNAMSIZ);
      if((ioctl(skfd, SIOCGARP, &arp_query) < 0) ||
	 !(arp_query.arp_flags & ATF_COM))
	{
	  fprintf(stderr, "Arp failed for %s on %s... (%d)\nTry to ping the address before setting it.\n",
		  bufp, ifname, errno);
	  return(-1);
	}

      /* Store new MAC address */
      memcpy((char *) sap,
	     (char *) &(arp_query.arp_ha),
	     sizeof(struct sockaddr));

#ifdef DEBUG
      printf("IP Address %s => Hw Address = %s\n",
	     bufp, iw_ether_ntoa((struct ether_addr *) sap->sa_data));
#endif
    }
  else	/* If it's an hardware address */
    {
      /* Check if we have valid mac address type */
      if(iw_check_mac_addr_type(skfd, ifname) < 0)
	{
	  fprintf(stderr, "%-8.16s  Interface doesn't support MAC addresses\n", ifname);
	  return(-1);
	}

      /* Get the hardware address */
      if(iw_in_ether(bufp, sap) < 0)
	{
	  fprintf(stderr, "Invalid hardware address %s\n", bufp);
	  return(-1);
	}
    }

#ifdef DEBUG
  printf("Hw Address = %s\n", iw_ether_ntoa((struct ether_addr *) sap->sa_data));
#endif

  return(0);
}

/************************* MISC SUBROUTINES **************************/

/* Size (in bytes) of various events */
static const int priv_type_size[] = {
	0,				/* IW_PRIV_TYPE_NONE */
	1,				/* IW_PRIV_TYPE_BYTE */
	1,				/* IW_PRIV_TYPE_CHAR */
	0,				/* Not defined */
	sizeof(__u32),			/* IW_PRIV_TYPE_INT */
	sizeof(struct iw_freq),		/* IW_PRIV_TYPE_FLOAT */
	sizeof(struct sockaddr),	/* IW_PRIV_TYPE_ADDR */
	0,				/* Not defined */
};

/*------------------------------------------------------------------*/
/*
 * Max size in bytes of an private argument.
 */
int
iw_get_priv_size(int	args)
{
  int	num = args & IW_PRIV_SIZE_MASK;
  int	type = (args & IW_PRIV_TYPE_MASK) >> 12;

  return(num * priv_type_size[type]);
}

/************************ EVENT SUBROUTINES ************************/
/*
 * The Wireless Extension API 14 and greater define Wireless Events,
 * that are used for various events and scanning.
 * Those functions help the decoding of events, so are needed only in
 * this case.
 */

/* Type of headers we know about (basically union iwreq_data) */
#define IW_HEADER_TYPE_NULL	0	/* Not available */
#define IW_HEADER_TYPE_CHAR	2	/* char [IFNAMSIZ] */
#define IW_HEADER_TYPE_UINT	4	/* __u32 */
#define IW_HEADER_TYPE_FREQ	5	/* struct iw_freq */
#define IW_HEADER_TYPE_ADDR	6	/* struct sockaddr */
#define IW_HEADER_TYPE_POINT	8	/* struct iw_point */
#define IW_HEADER_TYPE_PARAM	9	/* struct iw_param */
#define IW_HEADER_TYPE_QUAL	10	/* struct iw_quality */

/* Headers for the various requests */
static const char standard_ioctl_hdr[] = {
	IW_HEADER_TYPE_NULL,	/* SIOCSIWCOMMIT */
	IW_HEADER_TYPE_CHAR,	/* SIOCGIWNAME */
	IW_HEADER_TYPE_PARAM,	/* SIOCSIWNWID */
	IW_HEADER_TYPE_PARAM,	/* SIOCGIWNWID */
	IW_HEADER_TYPE_FREQ,	/* SIOCSIWFREQ */
	IW_HEADER_TYPE_FREQ,	/* SIOCGIWFREQ */
	IW_HEADER_TYPE_UINT,	/* SIOCSIWMODE */
	IW_HEADER_TYPE_UINT,	/* SIOCGIWMODE */
	IW_HEADER_TYPE_PARAM,	/* SIOCSIWSENS */
	IW_HEADER_TYPE_PARAM,	/* SIOCGIWSENS */
	IW_HEADER_TYPE_NULL,	/* SIOCSIWRANGE */
	IW_HEADER_TYPE_POINT,	/* SIOCGIWRANGE */
	IW_HEADER_TYPE_NULL,	/* SIOCSIWPRIV */
	IW_HEADER_TYPE_POINT,	/* SIOCGIWPRIV */
	IW_HEADER_TYPE_NULL,	/* SIOCSIWSTATS */
	IW_HEADER_TYPE_POINT,	/* SIOCGIWSTATS */
	IW_HEADER_TYPE_POINT,	/* SIOCSIWSPY */
	IW_HEADER_TYPE_POINT,	/* SIOCGIWSPY */
	IW_HEADER_TYPE_POINT,	/* SIOCSIWTHRSPY */
	IW_HEADER_TYPE_POINT,	/* SIOCGIWTHRSPY */
	IW_HEADER_TYPE_ADDR,	/* SIOCSIWAP */
	IW_HEADER_TYPE_ADDR,	/* SIOCGIWAP */
	IW_HEADER_TYPE_NULL,	/* -- hole -- */
	IW_HEADER_TYPE_POINT,	/* SIOCGIWAPLIST */
	IW_HEADER_TYPE_PARAM,	/* SIOCSIWSCAN */
	IW_HEADER_TYPE_POINT,	/* SIOCGIWSCAN */
	IW_HEADER_TYPE_POINT,	/* SIOCSIWESSID */
	IW_HEADER_TYPE_POINT,	/* SIOCGIWESSID */
	IW_HEADER_TYPE_POINT,	/* SIOCSIWNICKN */
	IW_HEADER_TYPE_POINT,	/* SIOCGIWNICKN */
	IW_HEADER_TYPE_NULL,	/* -- hole -- */
	IW_HEADER_TYPE_NULL,	/* -- hole -- */
	IW_HEADER_TYPE_PARAM,	/* SIOCSIWRATE */
	IW_HEADER_TYPE_PARAM,	/* SIOCGIWRATE */
	IW_HEADER_TYPE_PARAM,	/* SIOCSIWRTS */
	IW_HEADER_TYPE_PARAM,	/* SIOCGIWRTS */
	IW_HEADER_TYPE_PARAM,	/* SIOCSIWFRAG */
	IW_HEADER_TYPE_PARAM,	/* SIOCGIWFRAG */
	IW_HEADER_TYPE_PARAM,	/* SIOCSIWTXPOW */
	IW_HEADER_TYPE_PARAM,	/* SIOCGIWTXPOW */
	IW_HEADER_TYPE_PARAM,	/* SIOCSIWRETRY */
	IW_HEADER_TYPE_PARAM,	/* SIOCGIWRETRY */
	IW_HEADER_TYPE_POINT,	/* SIOCSIWENCODE */
	IW_HEADER_TYPE_POINT,	/* SIOCGIWENCODE */
	IW_HEADER_TYPE_PARAM,	/* SIOCSIWPOWER */
	IW_HEADER_TYPE_PARAM,	/* SIOCGIWPOWER */
};
static const unsigned int standard_ioctl_num = sizeof(standard_ioctl_hdr);

/*
 * Meta-data about all the additional standard Wireless Extension events
 * we know about.
 */
static const char	standard_event_hdr[] = {
	IW_HEADER_TYPE_ADDR,	/* IWEVTXDROP */
	IW_HEADER_TYPE_QUAL,	/* IWEVQUAL */
	IW_HEADER_TYPE_POINT,	/* IWEVCUSTOM */
	IW_HEADER_TYPE_ADDR,	/* IWEVREGISTERED */
	IW_HEADER_TYPE_ADDR,	/* IWEVEXPIRED */
};
static const unsigned int standard_event_num = sizeof(standard_event_hdr);

/* Size (in bytes) of various events */
static const int event_type_size[] = {
	IW_EV_LCP_LEN,		/* IW_HEADER_TYPE_NULL */
	0,
	IW_EV_CHAR_LEN,		/* IW_HEADER_TYPE_CHAR */
	0,
	IW_EV_UINT_LEN,		/* IW_HEADER_TYPE_UINT */
	IW_EV_FREQ_LEN,		/* IW_HEADER_TYPE_FREQ */
	IW_EV_ADDR_LEN,		/* IW_HEADER_TYPE_ADDR */
	0,
	IW_EV_POINT_LEN,	/* Without variable payload */
	IW_EV_PARAM_LEN,	/* IW_HEADER_TYPE_PARAM */
	IW_EV_QUAL_LEN,		/* IW_HEADER_TYPE_QUAL */
};

/*------------------------------------------------------------------*/
/*
 * Initialise the struct stream_descr so that we can extract
 * individual events from the event stream.
 */
void
iw_init_event_stream(struct stream_descr *	stream,	/* Stream of events */
		     char *			data,
		     int			len)
{
  /* Cleanup */
  memset((char *) stream, '\0', sizeof(struct stream_descr));

  /* Set things up */
  stream->current = data;
  stream->end = data + len;
}

/*------------------------------------------------------------------*/
/*
 * Extract the next event from the event stream.
 */
int
iw_extract_event_stream(struct stream_descr *	stream,	/* Stream of events */
			struct iw_event *	iwe,	/* Extracted event */
			int			we_version)
{
  int		event_type = 0;
  unsigned int	event_len = 1;		/* Invalid */
  char *	pointer;
  /* Don't "optimise" the following variable, it will crash */
  unsigned	cmd_index;		/* *MUST* be unsigned */

  /* Unused for now. Will be later on... */
  we_version = we_version;

  /* Check for end of stream */
  if((stream->current + IW_EV_LCP_LEN) > stream->end)
    return(0);

#if 0
  printf("DBG - stream->current = %p, stream->value = %p, stream->end = %p\n",
	 stream->current, stream->value, stream->end);
#endif

  /* Extract the event header (to get the event id).
   * Note : the event may be unaligned, therefore copy... */
  memcpy((char *) iwe, stream->current, IW_EV_LCP_LEN);

#if 0
  printf("DBG - iwe->cmd = 0x%X, iwe->len = %d\n",
	 iwe->cmd, iwe->len);
#endif

  /* Check invalid events */
  if(iwe->len <= IW_EV_LCP_LEN)
    return(-1);

  /* Get the type and length of that event */
  if(iwe->cmd <= SIOCIWLAST)
    {
      cmd_index = iwe->cmd - SIOCIWFIRST;
      if(cmd_index < standard_ioctl_num)
	event_type = standard_ioctl_hdr[cmd_index];
    }
  else
    {
      cmd_index = iwe->cmd - IWEVFIRST;
      if(cmd_index < standard_event_num)
	event_type = standard_event_hdr[cmd_index];
    }
  /* Unknown events -> event_type=0 => IW_EV_LCP_LEN */
  event_len = event_type_size[event_type];

  /* Check if we know about this event */
  if(event_len <= IW_EV_LCP_LEN)
    {
      /* Skip to next event */
      stream->current += iwe->len;
      return(2);
    }
  event_len -= IW_EV_LCP_LEN;

  /* Set pointer on data */
  if(stream->value != NULL)
    pointer = stream->value;			/* Next value in event */
  else
    pointer = stream->current + IW_EV_LCP_LEN;	/* First value in event */

#if 0
  printf("DBG - event_type = %d, event_len = %d, pointer = %p\n",
	 event_type, event_len, pointer);
#endif

  /* Copy the rest of the event (at least, fixed part) */
  if((pointer + event_len) > stream->end)
    {
      /* Go to next event */
      stream->current += iwe->len;
      return(-2);
    }
  memcpy((char *) iwe + IW_EV_LCP_LEN, pointer, event_len);

  /* Skip event in the stream */
  pointer += event_len;

  /* Special processing for iw_point events */
  if(event_type == IW_HEADER_TYPE_POINT)
    {
      /* Check the length of the payload */
      if((iwe->len - (event_len + IW_EV_LCP_LEN)) > 0)
	/* Set pointer on variable part (warning : non aligned) */
	iwe->u.data.pointer = pointer;
      else
	/* No data */
	iwe->u.data.pointer = NULL;

      /* Go to next event */
      stream->current += iwe->len;
    }
  else
    {
      /* Is there more value in the event ? */
      if((pointer + event_len) <= (stream->current + iwe->len))
	/* Go to next value */
	stream->value = pointer;
      else
	{
	  /* Go to next event */
	  stream->value = NULL;
	  stream->current += iwe->len;
	}
    }
  return(1);
}