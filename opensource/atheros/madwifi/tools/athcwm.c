/*
 * Copyright (c) 2002-2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 */
 
/*
 * Atheros-specific debug tool to exercise CWM state machine
 *
 *	athcwm [-i interface] [event]
 * (default interface is wifi0).  
 *
 */
#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/wireless.h>

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>

#include "ah_desc.h"
#include "if_athioctl.h"

/* defines */
#define	streq(a,b)	(strncasecmp(a,b,sizeof(b)-1) == 0)

enum ath_cwm_event {
	ATH_CWM_EVENT_TXTIMEOUT,	/* tx timeout interrupt */
	ATH_CWM_EVENT_EXTCHCLEAR,	/* ext channel sensing - clear */
	ATH_CWM_EVENT_EXTCHBUSY,	/* ext channel sensing - busy */
	ATH_CWM_EVENT_EXTCHSTOP,	/* ext channel sensing - stop */
	ATH_CWM_EVENT_EXTCHRESUME,	/* ext channel sensing - resume */
	ATH_CWM_EVENT_DESTCW20,		/* dest channel width changed to 20  */
	ATH_CWM_EVENT_DESTCW40,		/* dest channel width changed to 40  */
	ATH_CWM_EVENT_MAX,
};

/* Globals */
static const char *ath_cwm_eventname[ATH_CWM_EVENT_MAX] = {
	"TXTIMEOUT", 		/* ATH_CWM_EVENT_TXTIMEOUT */
	"EXTCHCLEAR", 		/* ATH_CWM_EVENT_EXTCHCLEAR */
	"EXTCHBUSY", 		/* ATH_CWM_EVENT_EXTCHBUSY */
	"EXTCHSTOP", 		/* ATH_CWM_EVENT_EXTCHSTOP */
	"EXTCHRESUME", 		/* ATH_CWM_EVENT_EXTCHRESUME */
	"DESTCW20", 		/* ATH_CWM_EVENT_DESTCW20 */
	"DESTCW40", 		/* ATH_CWM_EVENT_DESTCW40 */
};

const char *progname;


static void
usage(void)
{
	int i;

	fprintf(stderr, "usage: %s [-i device] info\n",
		progname);
	fprintf(stderr, "usage: %s [-i device] event num\n",
		progname);
	fprintf(stderr, "usage: %s [-i device] ctl 0/1\n",
		progname);
	fprintf(stderr, "usage: %s [-i device] ext 0/1\n",
		progname);
	fprintf(stderr, "usage: %s [-i device] vext 0/1\n",
		progname);
	fprintf(stderr, "Supported Events:\n");
	for (i = 0; i < ATH_CWM_EVENT_MAX; i++) {
		fprintf(stderr, "%s\t%d\n", ath_cwm_eventname[i], i);

	}
	exit(-1);
}


int
main(int argc, char *argv[])
{
#ifdef __linux__
	const char *ifname = "wifi0";
#else
	const char *ifname = "ath0";
#endif
	int s;
	struct ifreq ifr;
	const char *cmd;

	progname = argv[0];
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0)
		err(1, "socket");
	if (argc > 1 && strcmp(argv[1], "-i") == 0) {
		if (argc < 2) {
			fprintf(stderr, "%s: missing interface name for -i\n",
				argv[0]);
			exit(-1);
		}
		ifname = argv[2];
		argc -= 2, argv += 2;
	}
	strncpy(ifr.ifr_name, ifname, sizeof (ifr.ifr_name));
	if (argc > 1) {
		cmd = argv[1];
		if (streq(cmd, "info")) {
			struct ath_cwminfo ci;
			ifr.ifr_data = (caddr_t) &ci;
			if (ioctl(s, SIOCGATHCWMINFO, &ifr) < 0) {
				err(1, ifr.ifr_name);
			}
			fprintf(stdout, "<CWM State>\n");
			fprintf(stdout, "channel width:		%d\n", ci.ci_chwidth);
			fprintf(stdout, "mac mode:		%d\n", ci.ci_macmode);
			fprintf(stdout, "phy mode:		%d\n", ci.ci_phymode);
			fprintf(stdout, "ext ch busy (percent):	%d\n", ci.ci_extbusyper);
		} else if (streq(cmd, "event")) {
			u_long event = strtoul(argv[2], NULL, 0);
			struct ath_cwmdbg dc;
	
			dc.dc_cmd = ATH_DBGCWM_CMD_EVENT;
			dc.dc_arg = event;

			ifr.ifr_data = (caddr_t) &dc;
			if (ioctl(s, SIOCGATHCWMDBG, &ifr) < 0) {
				err(1, ifr.ifr_name);
			}
		} else if (streq(cmd, "ext")) {
			u_long ext = strtoul(argv[2], NULL, 0);
			struct ath_cwmdbg dc;
	
			dc.dc_cmd = ATH_DBGCWM_CMD_EXT;
			dc.dc_arg = ext;

			ifr.ifr_data = (caddr_t) &dc;
			if (ioctl(s, SIOCGATHCWMDBG, &ifr) < 0) {
				err(1, ifr.ifr_name);
			}

		} else if (streq(cmd, "ctl")) {
			u_long ctl = strtoul(argv[2], NULL, 0);
			struct ath_cwmdbg dc;
	
			dc.dc_cmd = ATH_DBGCWM_CMD_CTL;
			dc.dc_arg = ctl;

			ifr.ifr_data = (caddr_t) &dc;
			if (ioctl(s, SIOCGATHCWMDBG, &ifr) < 0) {
				err(1, ifr.ifr_name);
			}
		} else if (streq(cmd, "vext")) {
			u_long vext = strtoul(argv[2], NULL, 0);
			struct ath_cwmdbg dc;
	
			dc.dc_cmd = ATH_DBGCWM_CMD_VEXT;
			dc.dc_arg = vext;

			ifr.ifr_data = (caddr_t) &dc;
			if (ioctl(s, SIOCGATHCWMDBG, &ifr) < 0) {
				err(1, ifr.ifr_name);
			}
		} else {
			usage();
		}
	} else {
	       usage();
	}
	return 0;
}
