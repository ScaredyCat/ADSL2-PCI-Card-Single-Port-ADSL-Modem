#ifndef _DIAG_
#define	_DIAG_
/*
 * Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/diag/diag.h#1 $
 */
#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <stdio.h>

#ifndef __linux__
#include <net80211/ieee80211_radiotap.h>
#endif
#include "if_athioctl.h"

#ifndef ATH_DEFAULT
#define	ATH_DEFAULT	"wifi0"
#endif

struct statshandler {
	u_long	interval;
	void	*total;
	void	*cur;

	void (*getstats)(struct statshandler *, void *);
	void (*update)(struct statshandler *);

	void (*printbanner)(struct statshandler *, FILE *);
	void (*reportdelta)(struct statshandler *, FILE *);
	void (*reporttotal)(struct statshandler *, FILE *);
	void (*reportverbose)(struct statshandler *, FILE *);
};

extern	void reportstats(FILE *fd, struct statshandler *sh);
extern	void runstats(FILE *fd, struct statshandler *sh);
extern	void reportcol(FILE *fd, u_int32_t v, const char *def_fmt,
		u_int32_t max, const char *alt_fmt);
#endif /* _DIAG_ */
