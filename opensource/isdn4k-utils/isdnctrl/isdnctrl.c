/* $Id: isdnctrl.c,v 1.50 2002/01/31 19:53:41 paul Exp $
 * ISDN driver for Linux. (Control-Utility)
 *
 * Copyright 1994,95 by Fritz Elfert (fritz@isdn4linux.de)
 * Copyright 1995 Thinking Objects Software GmbH Wuerzburg
 *
 * This file is part of Isdn4Linux.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Log: isdnctrl.c,v $
 * Revision 1.50  2002/01/31 19:53:41  paul
 * Fixed error messages when opening /dev/isdnctrl - /dev/isdn/isdnctrl etc.,
 * only /dev/isdnctrl was mentioned and people assumed that isdnctrl wasn't
 * devfs-compliant yet when the open failed due to other reasons.
 * Zero the phone struct before use.
 *
 * Revision 1.49  2001/06/11 17:55:58  paul
 * Added 'break' statement after handling data version 5 (otherwise fallthrough
 * into data version 6 handling!!)
 *
 * Revision 1.48  2001/05/23 14:59:23  kai
 * removed traces of TIMRU. I hope it's been dead for a long enough time now.
 *
 * Revision 1.47  2001/05/23 14:48:23  kai
 * make isdnctrl independent of the version of installed kernel headers,
 * we have our own copy now.
 *
 * Revision 1.46  2001/03/15 22:02:44  kai
 * fixed a stack overflow when using isdnctrl status
 *
 * Revision 1.45  2001/03/01 14:59:15  paul
 * Various patches to fix errors when using the newest glibc,
 * replaced use of insecure tempnam() function
 * and to remove warnings etc.
 *
 * Revision 1.44  2000/08/17 09:24:06  paul
 * Added --version option to display (isdn4k-utils) version,
 * and fixed a compile warning on alpha.
 *
 * Revision 1.43  2000/06/29 17:38:26  akool
 *  - Ported "imontty", "isdnctrl", "isdnlog", "xmonisdn" and "hisaxctrl" to
 *    Linux-2.4 "devfs" ("/dev/isdnctrl" -> "/dev/isdn/isdnctrl")
 *
 * Revision 1.42  2000/04/27 06:32:28  calle
 * DriverId can be longer than 8 for "mapping" and "busreject".
 *
 * Revision 1.41  2000/04/12 21:49:40  detabc
 * add test for maybe undefined IIOCNETDWRSET define
 *
 * Revision 1.40  2000/01/27 15:08:09  paul
 * Error messages from addlink/removelink are now userfriendly.
 *
 * Revision 1.39  1999/11/23 10:17:27  paul
 * Made error message for 'status' command clearer if IIOCNETGPN
 * is not implemented in kernel (e.g. 2.0.x kernels).
 *
 * Revision 1.38  1999/11/20 22:23:53  detabc
 * added netinterface abc-secure-counter reset (clear) support.
 *
 * Revision 1.37  1999/11/07 22:04:05  detabc
 * add dwabc-udpinfo-utilitys in isdnctrl
 *
 * Revision 1.36  1999/11/02 20:41:21  keil
 * make phonenumber ioctl compatible for ctrlconf too
 *
 * Revision 1.35  1999/10/27 14:36:19  keil
 * make the phone number struct compatible between NET_DV 5 and 6
 *
 * Revision 1.34  1999/09/06 08:03:25  fritz
 * Changed my mail-address.
 *
 * Revision 1.33  1999/06/07 19:25:38  paul
 * isdnctrl.man.in
 *
 * Revision 1.32  1998/12/23 12:51:44  paul
 * didn't compile with old kernel source
 *
 * Revision 1.31  1998/11/24 18:18:57  paul
 * detect kernel < 2.0.36; warn if dialmode is accessed with older kernels
 *
 * Revision 1.30  1998/11/18 13:20:07  fritz
 * Fixed display of dialmode.
 *
 * Revision 1.29  1998/11/17 18:29:31  paul
 * isdnctrl.c now compiles with kernel sources without dialmode stuff.
 *
 * Revision 1.28  1998/11/11 23:53:02  fritz
 * Make isdnctrl compile without TIMRU in kernel (2.0.36-pre20/21)
 *
 * Revision 1.27  1998/10/28 16:12:18  paul
 * Implemented "dialmode all" mode.
 *
 * Revision 1.26  1998/10/21 16:18:45  paul
 * Implementation of "dialmode" (successor of "status")
 *
 * Revision 1.25  1998/07/22 19:07:20  keil
 * Make it compiling with older I4L versions
 *
 * Revision 1.24  1998/06/27 00:36:19  fritz
 * Misc. Fixes.
 * Added fallback to libdb for isdnctrl.
 * Added -V version check in isdnctrl.
 *
 * Revision 1.23  1998/06/12 12:09:53  detabc
 * cleanup abc
 *
 * Revision 1.22  1998/06/09 18:11:31  cal
 * added the command "isdnctrl name ifdefaults": the named device is reset
 * to some reasonable defaults.
 *
 * Internally, isdnctrl.c contains a list of functions (defs_fcns []), which
 * are called one after the other with the interface-name as a patameter.
 * Each function returns a char* to a string containing iscnctrl-commands
 * to be executed. Example:
 *
 * char *
 * defs_budget(char *id) {
 * 	static char	r [1024];
 * 	char	*p = r;
 *
 * 	p += sprintf(p, "budget %s dial 10 1min\n", id);
 * 	p += sprintf(p, "budget %s charge 100 1day\n", id);
 * 	p += sprintf(p, "budget %s online 8hour 1day\n", id);
 *
 * 	return(r);
 * }
 *
 * The advantage of this approach is, that even complex commands can be executed.
 *
 * PS: The function defs_basic() in isdnctrl.c is not complete.
 *
 * Revision 1.21  1998/06/02 12:17:15  detabc
 * wegen einer einstweiliger verfuegung gegen DW ist zur zeit
 * die abc-extension bis zur klaerung der rechtslage nicht verfuegbar
 *
 * Revision 1.20  1998/04/28 08:34:28  paul
 * Fixed compiler warnings from egcs.
 *
 * Revision 1.19  1998/04/18 17:36:13  detabc
 * modify display of callbackdelay (cbdelay) value to %.1f sec.
 * if abc-extension is enabled
 *
 * Revision 1.18  1998/03/21 17:10:36  detabc
 * change to use the abc-ext-options -TU on all encapsulations
 * the option -A (abc-router) will only works with rawip
 *
 * Revision 1.17  1998/03/19 15:39:02  detabc
 * change define CONFIG_ISDN_WITH_ABC to HAVE_ABCEXT.
 * HAVE_ABCEXT will be set with the configure utility.
 * to enable isdnctrl with ABC-Extension-support please make
 * first a kernelconfig with ABC-Extension enabled.
 * Thanks
 *
 * Revision 1.16  1998/03/12 15:10:11  hipp
 * Cosmetic. Changed 'addlink' error message.
 *
 * Revision 1.15  1998/03/08 01:04:19  fritz
 * Fix: Did not compile without TIMRU in kernel.
 *
 * Revision 1.14  1998/03/08 00:18:25  detabc
 * include config-support for abc-extension
 * only isdnctrl encap will be used and only use the options [-ATU]rawip
 * thanks
 *
 * Revision 1.13  1998/03/07 18:25:57  cal
 * added support for dynamic timeout-rules vs. 971110
 *
 * Revision 1.12  1997/10/26 23:12:20  fritz
 * Get rid of including ../.config in Makefile
 * Now all configuration is done in configure.
 *
 * Revision 1.11  1997/09/26 09:07:18  fritz
 * Check for missing triggercps in configuration.
 *
 * Revision 1.10  1997/09/11 19:03:32  fritz
 * Bugfix: Tried to get Version-Info on wrong device.
 *
 * Revision 1.9  1997/08/21 14:47:00  fritz
 * Added Version-Checking of NET_DV.
 *
 * Revision 1.8  1997/07/30 20:09:24  luethje
 * the call "isdnctrl pppbind ipppX" will be bound the interface to X
 *
 * Revision 1.7  1997/07/23 20:39:15  luethje
 * added the option "force" for the commands delif and reset
 *
 * Revision 1.6  1997/07/22 22:36:10  luethje
 * isdnrep:  Use "&nbsp;" for blanks
 * isdnctrl: Add the option "reset"
 *
 * Revision 1.5  1997/07/20 16:36:26  calle
 * isdnctrl trigger was not working.
 *
 * Revision 1.4  1997/06/24 23:35:26  luethje
 * isdnctrl can use a config file
 *
 * Revision 1.3  1997/06/22 11:58:21  fritz
 * Added ability to adjust slave triggerlevel.
 *
 * Revision 1.2  1997/03/10 09:51:24  fritz
 * Bugfix: mapping was broken.
 *
 * Revision 1.1  1997/02/17 00:09:21  fritz
 * New CVS tree
 *
 * Revision 1.14  1996/06/06 22:08:46  fritz
 * Bugfix: verbose and getconf checked wrong number of parameters.
 * thanks to Andreas Jaeger <aj@arthur.pfalz.de>
 *
 * Revision 1.13  1996/04/30 12:48:35  fritz
 * Added Michael's ippp-bind patch.
 *
 * Revision 1.12  1996/04/30 12:43:18  fritz
 * Changed ioctl-names according to kernel-version
 *
 * Revision 1.11  1996/01/04 02:44:52  fritz
 * Changed copying policy to GPL
 * Added addslave, dial, sdelay and mapping.
 *
 * Revision 1.10  1995/12/18  18:03:19  fritz
 * New License, minor cleanups.
 *
 * Revision 1.9  1995/10/29  21:38:51  fritz
 * Changed all references to driver-numbers to new DriverId's
 *
 * Revision 1.8  1995/07/15  20:39:56  fritz
 * Added support for cisco_h Encapsulation.
 * Added suppurt for pre-binding an interface to a channel.
 *
 * Revision 1.7  1995/04/29  13:13:44  fritz
 * Added new command verbose.
 *
 * Revision 1.6  1995/04/23  13:38:34  fritz
 * Adapted addphone and delphone to support changes in isdn.c
 *
 * Revision 1.5  1995/03/25  23:35:35  fritz
 * Added ihup-Feature.
 *
 * Revision 1.4  1995/03/15  12:44:15  fritz
 * Added generic conversion-routines for keyword<->value conversion.
 * Added display of phone-numbers in list-routine
 * Corrected some typos
 *
 * Revision 1.3  1995/02/20  03:38:59  fritz
 * Added getmax and rmax for performance-tests of tty-driver.
 *
 * Revision 1.2  1995/01/29  23:27:52  fritz
 * Added keywords: list, l2_proto, l3_proto, huptimeout, chargehup and
 * encap.
 *
 * Revision 1.1  1995/01/09  07:35:35  fritz
 * Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>

#include "config.h"
#define _ISDNCTRL_C_
#include "isdnctrl.h"

#ifdef I4L_DWABC_UDPINFO
#include <isdn_dwabclib.h>
#endif

#ifdef I4L_CTRL_CONF
#	include "../lib/libisdn.h"
#	include "ctrlconf.h"
#endif /* I4L_CTRL_CONF */

#define CMD_IFCONFIG "ifconfig"
#define CMD_OPT_IFCONFIG "down"


/* list of functions to obtain default-configuration of interface */
typedef	char *(*defs_fcn_t)();

defs_fcn_t defs_fcns [] = {
	defs_basic,
	NULL
};

char nextslaveif[10];

int set_isdn_net_ioctl_phone(isdn_net_ioctl_phone *ph, char *name, 
			     char *phone, int outflag)
{
	switch (data_version) {
	case 0x04:
	case 0x05:
		if (strlen(phone) > 19) {
			fprintf(stderr, "phone-number must not exceed %d characters\n", 19);
			return -1;
		}
		/*
		 * null termination happens automatically because
		 * we clear the entire struct first
		 */
		strncpy(ph->phone_5.name, name, sizeof(ph->phone_5.name)-1);
		strncpy(ph->phone_5.phone, phone, sizeof(ph->phone_5.phone)-1);
		ph->phone_5.outgoing = outflag;
                break;
	case 0x06:
		if (strlen(phone) > 31) {
			fprintf(stderr, "phone-number must not exceed %d characters\n", 31);
			return -1;
		}
		strncpy(ph->phone_6.name, name, sizeof(ph->phone_6.name)-1);
		strncpy(ph->phone_6.phone, phone, sizeof(ph->phone_6.phone)-1);
		ph->phone_6.outgoing = outflag;
	}
	return 0;
}

int exec_args(int fd, int argc, char **argv);

void usage(void)
{
        fprintf(stderr, "%s \n", cmd);
        fprintf(stderr, "usage: %s <command> <options>\n", cmd);
        fprintf(stderr, "\n");
        fprintf(stderr, "where <command> is one of the following:\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "    addif [name]               add net-interface\n");
        fprintf(stderr, "    delif name [force]         remove net-interface\n");
        fprintf(stderr, "    reset [force]              remove all net-interfaces\n");
        fprintf(stderr, "    dialmode name [off|manual|auto]  set the dial mode\n");
        fprintf(stderr, "    addphone name in|out num   add phone-number to interface\n");
        fprintf(stderr, "    delphone name in|out num   remove phone-number from interface\n");
        fprintf(stderr, "    eaz name [eaz|msn]         get/set eaz for interface\n");
        fprintf(stderr, "    huptimeout name [seconds]  get/set hangup-timeout for interface\n");
        fprintf(stderr, "    ihup name [on|off]         get/set incoming-hangup for interface\n");
        fprintf(stderr, "    chargehup name [on|off]    get/set charge-hangup for interface\n");
        fprintf(stderr, "    chargeint name [seconds]   get/set charge-interval if not given by telco\n");
        fprintf(stderr, "    secure name [on|off]       get/set secure-feature for interface\n");
        fprintf(stderr, "    callback name [in|outon|off]\n");
        fprintf(stderr, "                               get/set active callback-feature for interface\n");
        fprintf(stderr, "    cbhup name [on|off]        get/set reject-before-callback for interface\n");
        fprintf(stderr, "    cbdelay name [seconds]     get/set delay before callback for interface\n");
        fprintf(stderr, "    dialmax name [num]         get/set number of dial-atempts for interface\n");
        fprintf(stderr, "    dialtimeout name [seconds] get/set timeout for successful dial-attempt\n");
        fprintf(stderr, "    dialwait name [seconds]    get/set waittime after failed dial-attempt\n");
        fprintf(stderr, "    encap name [encapname]     get/set packet-encapsulation for interface\n");
        fprintf(stderr, "    l2_prot name [protocol]    get/set layer-2-protocol for interface\n");
        fprintf(stderr, "    l3_prot name [protocol]    get/set layer-3-protocol for interface\n");
        fprintf(stderr, "    bind name [drvId,channel [exclusive]]\n");
        fprintf(stderr, "                               pre-bind interface to a channel\n");
        fprintf(stderr, "    unbind name                delete pre-binding\n");
        fprintf(stderr, "    list name|all              show current setup of interface(s)\n");
        fprintf(stderr, "    verbose num                set verbose-level\n");
        fprintf(stderr, "    hangup name                force hangup of interface\n");
        fprintf(stderr, "    busreject drvId on|off     set bus-reject-mode\n");
        fprintf(stderr, "    mapping drvId [MSN,MSN...] set MSN<->EAZ-Mapping\n");
        fprintf(stderr, "    addslave name slavename    add slave-interface\n");
        fprintf(stderr, "    sdelay mastername delay    set slave-activation delay\n");
        fprintf(stderr, "    trigger mastername cps     set slave trigger level\n");
        fprintf(stderr, "    dial name                  force dialing of interface\n");
        fprintf(stderr, "    system on|off              switch isdn-system on or off\n");
        fprintf(stderr, "    addlink name               MPPP, increase number of links (dial)\n");
        fprintf(stderr, "    removelink name            MPPP, decrease number of links (hangup)\n");
        fprintf(stderr, "    pppbind name [devicenum]   PPP, bind interface to ippp-device (exclusive)\n");
        fprintf(stderr, "    pppunbind name             PPP, remove ippp-device binding\n");
        fprintf(stderr, "    addrule name rule ...      add timeout-rule\n");
        fprintf(stderr, "    insrule name rule ...      insert timeout-rule\n");
        fprintf(stderr, "    delrule name rule ...      delete timeout-rule\n");
        fprintf(stderr, "    showrules name             show all timeout-rules\n");
        fprintf(stderr, "    flushrules name rule-type  delete all timeout-rules of a spec. type\n");
        fprintf(stderr, "    flushallrules name         delete all timeout-rules\n");
        fprintf(stderr, "    default name rule-type ... set default for a spec. rule-type\n");
        fprintf(stderr, "    budget name type ...       set various budgets\n");
        fprintf(stderr, "    showbudgets name           show budget-settings\n");
        fprintf(stderr, "    savebudgets name           output budget-settings for later restore\n");
        fprintf(stderr, "    restorebudgets name ...    restore budget-settings\n");
#ifdef I4L_CTRL_CONF
        fprintf(stderr, "    writeconf [file]           write the settings to file\n");
        fprintf(stderr, "    readconf [file]            read the settings from file\n");
#endif /* I4L_CTRL_CONF */
        fprintf(stderr, "    status name                show interface status (connected or not)\n");
#ifdef I4L_DWABC_UDPINFO
        fprintf(stderr, "    abcclear  name             reset (clear) abc-secure-counter\n");
#endif
#ifdef I4L_DWABC_UDPINFO
		fprintf(stderr,"    -udpisisdn   destination-host or ip-number\n");
		fprintf(stderr,"    -udponline   destination-host or ip-number\n");
		fprintf(stderr,"    -udphangup   destination-host or ip-number\n");
		fprintf(stderr,"    -udpdial     destination-host or ip-number\n");
		fprintf(stderr,"    -udpclear    destination-host-or ip-number\n");
#endif
        fprintf(stderr, "    -V                         display API versions\n");
        fprintf(stderr, "    --version                  display isdnctrl version\n");
        exit(-2);
}

int key2num(char *key, char **keytable, int *numtable)
{
        int i = -1;
        while (strlen(keytable[++i]))
                if (!strcmp(keytable[i], key))
                        return numtable[i];
        return -1;
}

int reset_interfaces(int fd, char *option)
{
	FILE *iflst;
	char *p;
	char s[255];
	char name[255];
	char *argv[4] = {cmds[DELIF].cmd, name, option, NULL};
	isdn_net_ioctl_cfg cfg;


	if (option != NULL && strcmp(option, "force"))
	{
		usage();
		return -1;
	}

	if ((iflst = fopen(FILE_PROC, "r")) == NULL) {
		perror(FILE_PROC);
		return -1;
	}

	while (!feof(iflst)) {
		fgets(s, sizeof(s), iflst);
		if ((p = strchr(s, ':'))) {
			*p = 0;

			sscanf(s, "%s", name);
			strcpy(cfg.name, name);

		  if (ioctl(fd, IIOCNETGCF, &cfg) < 0)
	      continue;

			if (exec_args(fd, 2 + (option?1:0), argv) == -2)
				return -1;
		}
	}

	fclose(iflst);
	return 0;
}

char * num2key(int num, char **keytable, int *numtable)
{
        int i = -1;
        while (numtable[++i] >= 0)
                if (numtable[i] == num)
                        return keytable[i];
        return "???";
}

static void listbind(char *s, int e)
{
        if (strlen(s)) {
                char *p = strchr(s, ',');
                int ch;
                sscanf(p + 1, "%d", &ch);
                *p = '\0';
                printf("%s, channel %d%s\n", s, ch, (e > 0) ? ", exclusive" : "");
        } else
                printf("Nothing\n");
}

static void listif(int isdnctrl, char *name, int errexit)
{
        isdn_net_ioctl_cfg cfg;
	char ph_in[1024], ph_out[1024];

        memset(&cfg, 0, sizeof cfg);	/* clear in case of older kernel */
	cfg.dialmode = 0xDEADBEEF;
        strcpy(cfg.name, name);
        if (ioctl(isdnctrl, IIOCNETGCF, &cfg) < 0) {
                if (errexit) {
                        perror(name);
                        exit(-1);
                } else
                        return;
        }
	set_isdn_net_ioctl_phone((isdn_net_ioctl_phone *) ph_in, 
				 name, "", 0);
        if (ioctl(isdnctrl, IIOCNETGNM, &ph_in) < 0) {
                if (errexit) {
                        perror(name);
                        exit(-1);
                } else
                        return;
        }
	set_isdn_net_ioctl_phone((isdn_net_ioctl_phone *) ph_out, 
				 name, "", 1);
        if (ioctl(isdnctrl, IIOCNETGNM, &ph_out) < 0) {
                if (errexit) {
                        perror(name);
                        exit(-1);
                } else
                        return;
        }
        printf("\nCurrent setup of interface '%s':\n\n", cfg.name);
        printf("EAZ/MSN:                %s\n", cfg.eaz);
        printf("Phone number(s):\n");
        printf("  Outgoing:             %s\n", ph_out);
        printf("  Incoming:             %s\n", ph_in);
        printf("Dial mode:              ");
	if (cfg.dialmode == ISDN_NET_DM_OFF)
		puts("off");
	else if (cfg.dialmode == ISDN_NET_DM_AUTO)
		puts("auto");
	else if (cfg.dialmode == ISDN_NET_DM_MANUAL)
		puts("manual");
	else if (cfg.dialmode == 0xDEADBEEF)
		puts("not in kernel (please upgrade your kernel)");
	else
		printf("unknown value (0x%x)\n", cfg.dialmode);
        printf("Secure:                 %s\n", cfg.secure ? "on" : "off");
        printf("Callback:               %s\n", num2callb[cfg.callback]);
        if (cfg.callback == 2)
                printf("Hangup after Dial       %s\n", cfg.cbdelay ? "on" : "off");
        else
                printf("Reject before Callback: %s\n", cfg.cbhup ? "on" : "off");
        printf("Callback-delay:         %d\n",cfg.cbdelay / 5);
        printf("Dialmax:                %d\n", cfg.dialmax);
        printf("Hangup-Timeout:         %d\n", cfg.onhtime);
        printf("Incoming-Hangup:        %s\n", cfg.ihup ? "on" : "off");
        printf("ChargeHangup:           %s\n", cfg.chargehup ? "on" : "off");
        printf("Charge-Units:           %d\n", cfg.charge);
		if (data_version < 2)
        	printf("Charge-Interval:        n.a.\n");
		else
        	printf("Charge-Interval:        %d\n", cfg.chargeint);
        printf("Layer-2-Protocol:       %s\n", num2key(cfg.l2_proto, l2protostr, l2protoval));
        printf("Layer-3-Protocol:       %s\n", num2key(cfg.l3_proto, l3protostr, l3protoval));
        printf("Encapsulation:          %s\n", num2key(cfg.p_encap, pencapstr, pencapval));
        printf("Slave Interface:        %s\n", strlen(cfg.slave) ? cfg.slave : "None");
        printf("Slave delay:            %d\n", cfg.slavedelay);
		if (data_version < 3)
        	printf("Slave trigger:          n.a.\n");
#if HAVE_TRIGGERCPS
		else
        	printf("Slave trigger:          %d cps\n", cfg.triggercps);
#endif
        printf("Master Interface:       %s\n", strlen(cfg.master) ? cfg.master : "None");
        printf("Pre-Bound to:           ");
        listbind(cfg.drvid, cfg.exclusive);
        printf("PPP-Bound to:           ");
        if (cfg.pppbind >= 0)
        	printf("%d\n", cfg.pppbind);
        else
        	printf("Nothing\n");

        if (cfg.slave && *cfg.slave) {
                strncpy(nextslaveif, cfg.slave, 9);
		nextslaveif[9] = 0;
	}
        else
                nextslaveif[0] = 0;
}


static void statusif(int isdnctrl, char *name, int errexit)
{
        isdn_net_ioctl_phone phone;
	int rc;
	static int isdninfo = -1;

	if (isdninfo < 0) {
	        isdninfo = open("/dev/isdn/isdninfo", O_RDONLY);
		if (isdninfo < 0)
		        isdninfo = open("/dev/isdninfo", O_RDONLY);
		if (isdninfo < 0) {
			perror("Can't open /dev/isdninfo or /dev/isdn/isdninfo");
			exit(-1);
		}
	}

	memset(&phone, 0, sizeof phone);
	set_isdn_net_ioctl_phone(&phone, name, "", 0);
	rc = ioctl(isdninfo, IIOCNETGPN, &phone);
	if (rc < 0) {
		if (errno == ENOTCONN) {
			printf("%s is not connected\n", name);
			if (errexit) {
				exit(1); /* exit 1 if interface specified & not conn. */
		}
			return;
		}
		if (errexit) {
			perror(name);
			exit(-1);
		}
	}
	switch (data_version) {
	case 0x04:
	case 0x05:
		printf("%s connected %s %s\n",
		       name, phone.phone_5.outgoing?"to":"from", phone.phone_5.phone);
		return;
	case 0x06:
		printf("%s connected %s %s\n",
		       name, phone.phone_6.outgoing?"to":"from", phone.phone_6.phone);
		return;
	}
}

int findcmd(char *str)
{
	int i;

	if (str != NULL)
		for (i = 0; cmds[i].cmd; i++)
			if (!strcmp(cmds[i].cmd, str))
				return i;

	return -1;
}


/*
 * do_dialmode() - handle dialmode settings
 *		   parameters:
 *			args	 - number of args given
 *			dialmode - what to set it to
 *			fd	 - fd for ioctl
 *			id	 - name of interface
 *			errexit  - exit if error (useful for nonisdn interfaces)
 * 
 * If called with args == 2, set the value of interface id,
 * else show the setting.
 */

static void
do_dialmode(int args, int dialmode, int fd, char *id, int errexit)
{
	isdn_net_ioctl_cfg cfg;

	memset(&cfg, 0, sizeof cfg);	/* clear in case of older kernel */
	cfg.dialmode = 0xDEADBEEF;
	/* first get settings */
	strcpy(cfg.name, id);
	if (ioctl(fd, IIOCNETGCF, &cfg) < 0) {
		if (!errexit)
			return;
		perror(id);
		exit(-1);
	}
	if (cfg.dialmode == 0xDEADBEEF) {
		fputs("dialmode setting not in kernel\n", stderr);
		/*
		 * exit true if setting "auto"
		 * I want to be able to "isdnctrl dialmode if auto" without
		 * error if kernel has no dialmode, as then the behaviour is
		 * equivalent to "auto".
		 */
		exit((args == 2 && dialmode == ISDN_NET_DM_AUTO) ? 0 : -1);
	}
	/* hack for following a chain of interfaces */
	if (cfg.slave && *cfg.slave) {
		strncpy(nextslaveif, cfg.slave, 9);
		nextslaveif[9] = 0;
	}
	else
		nextslaveif[0] = 0;

	if (args == 2) {	/* set a value */
		cfg.dialmode = dialmode;
		if (ioctl(fd, IIOCNETSCF, &cfg) < 0) {
			perror(id);
			exit(-1);
		}
		return;
	}

	printf("Dial mode for %s: ", id);
	/* no args specified, so show dialmode */
	if      (cfg.dialmode == ISDN_NET_DM_OFF)
		puts("off");
	else if (cfg.dialmode == ISDN_NET_DM_AUTO)
		puts("auto");
	else if (cfg.dialmode == ISDN_NET_DM_MANUAL)
		puts("manual");
	else
		puts("illegal value (wrong kernel version?)");
}

int exec_args(int fd, int argc, char **argv)
{
	int i,
	 n,
	 args;
	int result;
	FILE *iflst;
	char *p;
	char s[255], dummy[255];
	isdn_net_ioctl_phone phone;
	isdn_net_ioctl_cfg cfg;
	isdn_ioctl_struct iocts;
	unsigned long j;
	char nstring[255];
#ifdef I4L_CTRL_CONF
	char conffile[PATH_MAX];
#endif /* I4L_CTRL_CONF */
	char *id;
	char *arg1;
	char *arg2;
	int  outflag;

	for (; *argv != NULL; argv++, argc--) {
		if ((i = findcmd(argv[0])) < 0) {    /* Unknown command */
			fprintf(stderr, "The given command \"%s\" is unknown.\n\n", argv[0]);
			usage();
			return -1;
		}

		args = cmds[i].argno[0] - '0';
		id = argv[1];

		if (args > argc - 1) {
			fprintf(stderr, "Too few arguments given for \"%s\".\n\n", argv[0]);
			usage();
			return -1;
		}

#ifdef I4L_CTRL_CONF
		if (id != NULL && i != RESET && i != WRITECONF && i != READCONF) {
#else
		if (id != NULL && i != RESET) {
#endif /* I4L_CTRL_CONF */
			if (i == BUSREJECT || i == MAPPING) {
			   if (strlen(id) > sizeof(iocts.drvid)-1) {
				fprintf(stderr, "DriverId must not exceed %u characters!\n", (unsigned int)sizeof(iocts.drvid)-1);
				close(fd);
				return -1;
			   }
			} else if (strlen(id) > 8) {
				fprintf(stderr, "Interface name must not exceed 8 characters!\n");
				close(fd);
				return -1;
			}
		}

		for (n = 1; cmds[i].argno[n]; n++) {
			args = cmds[i].argno[n] - '0';
			if (((args > argc - 1) || findcmd(argv[args]) >= 0)) {
				args = cmds[i].argno[n - 1] - '0';
				break;
			}
		}

		arg1 = (args > 1) ? argv[2] : "";
		arg2 = (args > 2) ? argv[3] : "";
		argc -= args;
		argv += args;

		memset(&cfg, 0, sizeof cfg);	/* clear in case of older kernel */

		switch (i) {
#ifdef I4L_DWABC_UDPINFO
			case ABCCLEAR:
			        if ((result = ioctl(fd, IIOCNETDWRSET, id)) < 0) {
			        	perror(id);
			        	return -1;
			        }
			        printf("ABC secure-counter for %s now clear\n", id);
					break;
#endif
			case ADDIF:
			        strcpy(s, args?id:"");
			        if ((result = ioctl(fd, IIOCNETAIF, s)) < 0) {
			        	perror("addif");
			        	return -1;
			        }
			        printf("%s added\n", s);
			        break;

			case ADDSLAVE:
			        if (strlen(arg1) > 8) {
			        	fprintf(stderr, "slavename must not exceed 8 characters\n");
			        	return -1;
			        }
			        sprintf(s, "%s,%s", id, arg1);
			        if ((result = ioctl(fd, IIOCNETASL, s)) < 0) {
			        	perror("addslave");
			        	return -1;
			        }
			        printf("%s added as slave to %s\n", s, id);
			        break;

			case DELIF:
			        if (args == 2) {
			        	if (!strcmp(arg1, "force")) {
			        		char command[255];
			        		sprintf(command,"%s %s %s",CMD_IFCONFIG, id, CMD_OPT_IFCONFIG);

			        		if (system(command))
			        			return -2;
			        	} else
			        		usage();
				}
			        if ((result = ioctl(fd, IIOCNETDIF, id)) < 0) {
			        	perror(id);
			        	return -1;
			        }
			        printf("%s deleted\n", id);
			        break;

			case DIAL:
			        if ((result = ioctl(fd, IIOCNETDIL, id)) < 0) {
			        	perror(id);
			        	return -1;
			        }
			        printf("Dialing of %s triggered\n", id);
			        break;

			case BIND:
			        if (args == 3)
			        	if (strncmp(arg2, "excl", 4))
			        		usage();
			        strcpy(cfg.name, id);
			        if ((result = ioctl(fd, IIOCNETGCF, &cfg)) < 0) {
			        	perror(id);
			        	return -1;
			        }
			        if (args > 1)
			        {
			        	sscanf(arg1, "%s", cfg.drvid);
			        	cfg.exclusive = (args == 3);
			        	if ((result = ioctl(fd, IIOCNETSCF, &cfg)) < 0) {
			        		sprintf(s, "%s or %s", id, arg2);
			        		perror(s);
			        		return -1;
			        	}
			        }
			        printf("%s bound to ", id);
			        listbind(cfg.drvid, cfg.exclusive);
			        break;

			case UNBIND:
			        strcpy(cfg.name, id);
			        if ((result = ioctl(fd, IIOCNETGCF, &cfg)) < 0) {
			        	perror(id);
			        	return -1;
			        }
			        if (!strlen(cfg.drvid)) {
			        	printf("%s was not bound to anything\n", id);
			        	return -1;
			        }
			        cfg.drvid[0] = '\0';
			        cfg.exclusive = -1;
			        if ((result = ioctl(fd, IIOCNETSCF, &cfg)) < 0) {
			        	perror(id);
			        	return -1;
			        }
			        printf("%s unbound successfully\n", id);
			        break;

			case PPPBIND:
			        strcpy(cfg.name, id);
			        if ((result = ioctl(fd, IIOCNETGCF, &cfg)) < 0) {
                                	perror(id);
                                	return -1;
			        }
			        if ((args == 2 && sscanf(arg1, "%d%s", &cfg.pppbind,dummy) == 1) ||
			            (args == 1 && sscanf(id, "ippp%d%s", &cfg.pppbind,dummy) == 1)) {
			       	 	if ((result = ioctl(fd, IIOCNETSCF, &cfg)) < 0) {
			        		sprintf(s, "%s or %s", id, arg1);
			        		perror(s);
			        		return -1;
              				}
             			} else {
           		  		if (args == 1)
			       			fprintf(stderr,"Unknown interface `%s', use ipppX\n", id);
			       		else
			       			fprintf(stderr,"Unknown argument `%s'\n", arg1);
			       		return -1;
				}
			        printf("%s bound to ", id);
			        if (cfg.pppbind >= 0)
			        	printf("%d\n", cfg.pppbind);
			        else
			        	printf("nothing\n");
			        break;

			case PPPUNBIND:
			        strcpy(cfg.name, id);
			        if ((result = ioctl(fd, IIOCNETGCF, &cfg)) < 0) {
			        	perror(id);
			        	return -1;
			        }
			        if (cfg.pppbind < 0) {
			        	printf("%s was not bound to anything\n", id);
			        	return -1;
			        }
			        cfg.pppbind = -1;
			        if ((result = ioctl(fd, IIOCNETSCF, &cfg)) < 0) {
			        	perror(id);
			        	return -1;
			        }
			        printf("%s unbound successfully\n", id);
			        break;

			case BUSREJECT:
			        strcpy(iocts.drvid, id);
			        if (strcmp(arg1, "on") && strcmp(arg1, "off")) {
			        	fprintf(stderr, "Bus-Reject must be 'on' or 'off'\n");
			        	return -1;
			        }
			        iocts.arg = strcmp(arg1, "off");
			        if ((result = ioctl(fd, IIOCSETBRJ, &iocts)) < 0) {
			        	perror(id);
			        	return -1;
			        }
			        break;

			case MAPPING:
			        strcpy(iocts.drvid, id);
			        if (args == 1) {
			        	iocts.arg = (unsigned long) &nstring;
			        	if ((result = ioctl(fd, IIOCGETMAP, &iocts)) < 0) {
			        		perror(id);
			        		return -1;
			        	}
			        	printf("MSN/EAZ-mapping for %s:\n%s\n", id, nstring);
			        } else {
			        	char buf[400];
			        	strncpy(buf, arg1, sizeof(buf) - 1);
			        	iocts.arg = (unsigned long) buf;
			        	if ((result = ioctl(fd, IIOCSETMAP, &iocts)) < 0) {
			        		perror(id);
			        		return -1;
			        	}
			        }
			        break;

			case SYSTEM:
			        if (strcmp(id, "on") && strcmp(id, "off")) {
			        	fprintf(stderr, "System-Mode must be 'on' or 'off'\n");
			        	return -1;
			        }
			        j = strcmp(id, "on");
			        if ((result = ioctl(fd, IIOCSETGST, j)) < 0) {
			        	perror(id);
			        	return -1;
			        }
			        break;

			case HANGUP:
			        if ((result = ioctl(fd, IIOCNETHUP, id)) < 0) {
			        	perror(id);
			        	return -1;
			        }
			        if (result)
			        	printf("%s not connected\n", id);
			        else
			        	printf("%s hung up\n", id);
			        break;

			case ADDPHONE:
			        if (strcmp(arg1, "in") && strcmp(arg1, "out")) {
			        	fprintf(stderr, "Direction must be \"in\" or \"out\"\n");
			        	return -1;
			        }
			        outflag = strcmp(arg1, "out") ? 0 : 1;
			        if (set_isdn_net_ioctl_phone(&phone, id, arg2, outflag))
			        	return -1;
			        if ((result = ioctl(fd, IIOCNETANM, &phone)) < 0) {
			        	perror(id);
			        	return -1;
			        }
			        break;

			case DELPHONE:
			        if (strcmp(arg1, "in") && strcmp(arg1, "out")) {
			        	fprintf(stderr, "Direction must be \"in\" or \"out\"\n");
			        	return -1;
			        }
			        outflag = strcmp(arg1, "out") ? 0 : 1;
			        if (set_isdn_net_ioctl_phone(&phone, id, arg2, outflag))
			        	return -1;
			        if ((result = ioctl(fd, IIOCNETDNM, &phone)) < 0) {
			        	perror(id);
			        	return -1;
			        }
			        break;

		        case LIST:
			        if (!strcmp(id, "all")) {
			        	char name[10];
			        	if ((iflst = fopen(FILE_PROC, "r")) == NULL) {
			        		perror(FILE_PROC);
			        		return -1;
			        	}
			        	while (!feof(iflst)) {
			        		fgets(s, sizeof(s), iflst);
			        		if ((p = strchr(s, ':'))) {
			        			*p = 0;
			        			sscanf(s, "%s", name);
			        			listif(fd, name, 0);
			        			while (*nextslaveif)
			        				listif(fd, nextslaveif, 0);
			        		}
			        	}
			        	fclose(iflst);
			        } else
			        	listif(fd, id, 1);
			        break;

			case STATUS:
			        if (!strcmp(id, "all")) {
			        	char name[10];
			        	if ((iflst = fopen(FILE_PROC, "r")) == NULL) {
			        		perror(FILE_PROC);
			        		return -1;
			        	}
			        	while (!feof(iflst)) {
			        		fgets(s, sizeof(s), iflst);
			        		if ((p = strchr(s, ':'))) {
			        			*p = 0;
			        			sscanf(s, "%s", name);
			        			statusif(fd, name, 0);
			        			while (*nextslaveif)
			        				statusif(fd, nextslaveif, 0);
			        		}
			        	}
			        	fclose(iflst);
			        } else {
			        	statusif(fd, id, 1);
				}
			        break;

			case EAZ:
			        strcpy(cfg.name, id);
			        if ((result = ioctl(fd, IIOCNETGCF, &cfg)) < 0) {
			        	perror(id);
			        	return -1;
			        }
			        if (args == 2) {
			        	i = -1;
			        	strncpy(cfg.eaz, arg1, sizeof(cfg.eaz) - 1);
			        	if ((result = ioctl(fd, IIOCNETSCF, &cfg)) < 0) {
			        		perror(id);
			        		return -1;
			        	}
			        }
			        printf("EAZ/MSN for %s is %s\n", cfg.name, cfg.eaz);
			        break;

			case VERBOSE:
			        i = -1;
			        sscanf(id, "%d", &i);
			        if (i < 0) {
			        	fprintf(stderr, "Verbose-level must be >= 0\n");
			        	return -1;
			        }
			        if ((result = ioctl(fd, IIOCSETVER, i)) < 0) {
			        	perror("IIOCSETVER");
			        	return -1;
			        }
			        printf("Verbose-level set to %d.\n", i);
			        break;

			case HUPTIMEOUT:
			        strcpy(cfg.name, id);
			        if ((result = ioctl(fd, IIOCNETGCF, &cfg)) < 0) {
			        	perror(id);
			        	return -1;
			        }
			        if (args == 2) {
			        	i = -1;
			        	sscanf(arg1, "%d", &i);
			        	if (i < 0) {
			        		fprintf(stderr, "Hangup-Timeout must be >= 0\n");
			        		return -1;
			        	}
			        	cfg.onhtime = i;
			        	if ((result = ioctl(fd, IIOCNETSCF, &cfg)) < 0) {
			        		perror(id);
			        		return -1;
			        	}
			        }
			        printf("Hangup-Timeout for %s is %d sec.\n", cfg.name, cfg.onhtime);
			        break;

			case CBDELAY:
			        strcpy(cfg.name, id);
			        if ((result = ioctl(fd, IIOCNETGCF, &cfg)) < 0) {
			        	perror(id);
			        	return -1;
			        }
			        if (args == 2) {
			        	i = -1;
			        	sscanf(arg1, "%d", &i);
			        	if (i < 0) {
			        		fprintf(stderr, "Callback delay must be >= 0\n");
			        		return -1;
			        	}
			        	cfg.cbdelay = i * 5;
			        	if ((result = ioctl(fd, IIOCNETSCF, &cfg)) < 0) {
			        		perror(id);
			        		return -1;
			        	}
			        }
			        printf("Callback delay for %s is %d sec.\n", cfg.name, cfg.cbdelay / 5);
			        break;

			case CHARGEINT:
			        strcpy(cfg.name, id);
			        if ((result = ioctl(fd, IIOCNETGCF, &cfg)) < 0) {
			        	perror(id);
			        	return -1;
			        }
			        if (args == 2) {
			        	i = -1;
			        	sscanf(arg1, "%d", &i);
			        	if (i < 0) {
			        		fprintf(stderr, "Charge interval must be >= 0\n");
			        		return -1;
			        	}
			        	cfg.chargeint = i;
			        	if ((result = ioctl(fd, IIOCNETSCF, &cfg)) < 0) {
			        		perror(id);
			        		return -1;
			        	}
			        }
					if (data_version < 2)
						printf("Option 'chargeint' IGNORED!\n");
					else
			        	printf("Charge Interval for %s is %d sec.\n", cfg.name, cfg.chargeint);
			        break;

			case DIALMAX:
			        strcpy(cfg.name, id);
			        if ((result = ioctl(fd, IIOCNETGCF, &cfg)) < 0) {
			        	perror(id);
			        	return -1;
			        }
			        if (args == 2) {
			        	i = -1;
			        	sscanf(arg1, "%d", &i);
			        	if (i < 1) {
			        		fprintf(stderr, "Dialmax must be > 0\n");
			        		return -1;
			        	}
			        	cfg.dialmax = i;
			        	if ((result = ioctl(fd, IIOCNETSCF, &cfg)) < 0) {
			        		perror(id);
			        		return -1;
			        	}
			        }
			        printf("Dialmax for %s is %d times.\n", cfg.name, cfg.dialmax);
			        break;

			case SDELAY:
			        strcpy(cfg.name, id);
			        if ((result = ioctl(fd, IIOCNETGCF, &cfg)) < 0) {
			        	perror(id);
			        	return -1;
			        }
			        if (args == 2) {
			        	i = -1;
			        	sscanf(arg1, "%d", &i);
			        	if (i < 1) {
			        		fprintf(stderr, "Slave-activation delay must be >= 1\n");
			        		return -1;
			        	}
			        	cfg.slavedelay = i;
			        	if ((result = ioctl(fd, IIOCNETSCF, &cfg)) < 0) {
			        		perror(id);
			        		return -1;
			        	}
			        }
			        printf("Slave-activation delay for %s is %d sec.\n", cfg.name,
			               cfg.slavedelay);
			        break;

			case TRIGGER:
			        strcpy(cfg.name, id);
			        if ((result = ioctl(fd, IIOCNETGCF, &cfg)) < 0) {
			        	perror(id);
			        	exit(-1);
			        }
			        if (args == 2) {
			        	i = -1;
			        	sscanf(arg1, "%d", &i);
			        	if (i < 0) {
			        		fprintf(stderr, "Slave triggerlevel must be >= 0 (%s)\n", arg1);
			        		exit(-1);
			        	}
			        	cfg.triggercps = i;
			        	if ((result = ioctl(fd, IIOCNETSCF, &cfg)) < 0) {
			        		perror(id);
			        		exit(-1);
			        	}
			        }
					if (data_version < 3)
						printf("Option 'trigger' IGNORED!\n");
					else
			        	printf("Slave triggerlevel for %s is %d cps.\n",
								cfg.name, cfg.triggercps);
			        break;

			case CHARGEHUP:
			        strcpy(cfg.name, id);
			        if ((result = ioctl(fd, IIOCNETGCF, &cfg)) < 0) {
			        	perror(id);
			        	return -1;
			        }
			        if (args == 2) {
			        	i = -1;
			        	if (strcmp(arg1, "on") && strcmp(arg1, "off")) {
			        		fprintf(stderr, "Charge-Hangup must be 'on' or 'off'\n");
			        		return -1;
			        	}
			        	cfg.chargehup = strcmp(arg1, "off");
			        	if ((result = ioctl(fd, IIOCNETSCF, &cfg)) < 0) {
			        		perror(id);
			        		return -1;
			        	}
			        }
			        printf("Charge-Hangup for %s is %s\n", cfg.name, cfg.chargehup ? "on" : "off");
			        break;

			case CBHUP:
			        strcpy(cfg.name, id);
			        if ((result = ioctl(fd, IIOCNETGCF, &cfg)) < 0) {
			        	perror(id);
			        	return -1;
			        }
			        if (args == 2) {
			        	i = -1;
			        	if (strcmp(arg1, "on") && strcmp(arg1, "off")) {
			        		fprintf(stderr, "Callback-Hangup must be 'on' or 'off'\n");
			        		return -1;
			        	}
			        	cfg.cbhup = strcmp(arg1, "off");
			        	if ((result = ioctl(fd, IIOCNETSCF, &cfg)) < 0) {
			        		perror(id);
			        		return -1;
			        	}
			        }
			        printf("Reject before Callback for %s is %s\n", cfg.name, cfg.cbhup ? "on" : "off");
			        break;

			case IHUP:
			        strcpy(cfg.name, id);
			        if ((result = ioctl(fd, IIOCNETGCF, &cfg)) < 0) {
			        	perror(id);
			        	return -1;
			        }
			        if (args == 2) {
			        	i = -1;
			        	if (strcmp(arg1, "on") && strcmp(arg1, "off")) {
			        		fprintf(stderr, "Incoming-Hangup must be 'on' or 'off'\n");
			        		return -1;
			        	}
			        	cfg.ihup = strcmp(arg1, "off");
			        	if ((result = ioctl(fd, IIOCNETSCF, &cfg)) < 0) {
			        		perror(id);
			        		return -1;
			        	}
			        }
			        printf("Incoming-Hangup for %s is %s\n", cfg.name, cfg.ihup ? "on" : "off");
			        break;

			case SECURE:
			        strcpy(cfg.name, id);
			        if ((result = ioctl(fd, IIOCNETGCF, &cfg)) < 0) {
			        	perror(id);
			        	return -1;
			        }
			        if (args == 2) {
			        	i = -1;
			        	if (strcmp(arg1, "on") && strcmp(arg1, "off")) {
			        		fprintf(stderr, "Secure-parameter must be 'on' or 'off'\n");
			        		return -1;
			        	}
			        	cfg.secure = strcmp(arg1, "off");
			        	if ((result = ioctl(fd, IIOCNETSCF, &cfg)) < 0) {
			        		perror(id);
			        		return -1;
			        	}
			        }
			        printf("Security for %s is %s\n", cfg.name, cfg.secure ? "on" : "off");
			        break;

			case CALLBACK:
			        strcpy(cfg.name, id);
			        if ((result = ioctl(fd, IIOCNETGCF, &cfg)) < 0) {
			        	perror(id);
			        	return -1;
			        }
			        if (args == 2) {
			        	i = -1;
			        	if (strcmp(arg1, "on") && strcmp(arg1, "off") &&
					    strcmp(arg1, "in") && strcmp(arg1, "out")) {
			        		fprintf(stderr, "Callback-parameter must be 'on', 'in', 'out' or 'off'\n");
			        		return -1;
			        	}
			        	cfg.callback = strcmp(arg1, "off") ? 1 : 0;
			        	if (!strcmp(arg1, "out"))
			        		cfg.callback = 2;
			        	if ((result = ioctl(fd, IIOCNETSCF, &cfg)) < 0) {
			        		perror(id);
			        		return -1;
			        	}
			        }
			        printf("Callback for %s is %s\n", cfg.name, num2callb[cfg.callback]);
			        break;

			case L2_PROT:
			        strcpy(cfg.name, id);
			        if ((result = ioctl(fd, IIOCNETGCF, &cfg)) < 0) {
			        	perror(id);
			        	return -1;
			        }
			        if (args == 2) {
			        	i = key2num(arg1, l2protostr, l2protoval);
			        	if (i < 0) {
			        		fprintf(stderr, "Layer-2-Protocol must be one of the following:\n");
			        		i = 0;
			        		while (strlen(l2protostr[i]))
			        			fprintf(stderr, "\t\"%s\"\n", l2protostr[i++]);
			        		return -1;
			        	}
			        	cfg.l2_proto = i;
			        	if ((result = ioctl(fd, IIOCNETSCF, &cfg)) < 0) {
			        		perror(id);
			        		return -1;
			        	}
			        }
			        printf("Layer-2-Protocol for %s is %s\n", cfg.name,
			          num2key(cfg.l2_proto, l2protostr, l2protoval));
			        break;

			case L3_PROT:
			        strcpy(cfg.name, id);
			        if ((result = ioctl(fd, IIOCNETGCF, &cfg)) < 0) {
			        	perror(id);
			        	return -1;
			        }
			        if (args == 2) {
			        	i = key2num(arg1, l3protostr, l3protoval);
			        	if (i < 0) {
			        		fprintf(stderr, "Layer-3-Protocol must be one of the following:\n");
			        		i = 0;
			        		while (strlen(l3protostr[i]))
			        			fprintf(stderr, "\t\"%s\"\n", l3protostr[i++]);
			        		return -1;
			        	}
			        	cfg.l3_proto = i;
			        	if ((result = ioctl(fd, IIOCNETSCF, &cfg)) < 0) {
			        		perror(id);
			        		return -1;
			        	}
			        }
			        printf("Layer-3-Protocol for %s is %s\n", cfg.name,
			          num2key(cfg.l3_proto, l3protostr, l3protoval));
			        break;

			case ADDLINK:
			        if ((result = ioctl(fd, IIOCNETALN, id)) < 0) {
			        	perror(id);
			        	return -1;
			        }
				if (result) {
					printf("Can't increase the number of links:\n\t");
					switch (result) {
						case -1: printf("MPPP not in the kernel config.\n");
							 break;
						case 1: printf("%s doesn't exist\n", id);
							break;
						case 2: printf("no slave devices configured for %s\n", id);
							break;
						case 5: printf("%s not currently connected.\n", id);
							break;
						default: printf("unknown error %d\n", result);
					}
				}
			        else
			        	printf("Ok, added a new link. (dialing)\n");
			        break;

			case REMOVELINK:
			        if ((result = ioctl(fd, IIOCNETDLN, id)) < 0) {
			        	perror(id);
			        	return -1;
			        }
				if (result) {
					printf("Can't decrease the number of links:\n\t");
					switch (result) {
						case -1: printf("MPPP not in the kernel config.\n");
							 break;
						case 1: printf("%s doesn't exist\n", id);
							break;
						case 2: printf("no slave devices configured for %s\n", id);
							break;
						case 5: printf("%s not currently connected.\n", id);
							break;
						default: printf("unknown error %d\n", result);
					}
				}
			        else
			        	printf("Ok, removed a link. (hangup)\n");
			        break;

			case ENCAP:
			        strcpy(cfg.name, id);
			        if ((result = ioctl(fd, IIOCNETGCF, &cfg)) < 0) {
			        	perror(id);
			        	return -1;
			        }
			        if (args == 2) {
			        	i = key2num(arg1, pencapstr, pencapval);
			        	if (i < 0) {
			        		fprintf(stderr, "Encapsulation must be one of the following:\n");
			        		i = 0;
			        		while (strlen(pencapstr[i]))
			        			fprintf(stderr, "\t\"%s\"\n", pencapstr[i++]);
			        		return -1;
			        	}
			        	cfg.p_encap = i;
			        	if ((result = ioctl(fd, IIOCNETSCF, &cfg)) < 0) {
			        		perror(id);
			        		return -1;
			        	}
			        }
			        printf("Encapsulation for %s is %s\n", cfg.name,
			             num2key(cfg.p_encap, pencapstr, pencapval));
			        break;

			case RESET:
			        reset_interfaces(fd, args?id:NULL);
			        break;

                	case DIALMODE:
				if(args == 2) {
					if (!strcmp(arg1, "on") || !strcmp(arg1, "manual"))
						cfg.dialmode = ISDN_NET_DM_MANUAL;
					else if (!strcmp(arg1, "off"))
						cfg.dialmode = ISDN_NET_DM_OFF;
					else if (!strncmp(arg1, "auto", 4))
					    /* also automatic, autodial, ... */
						cfg.dialmode = ISDN_NET_DM_AUTO;
					else {
						fprintf(stderr, "dialmode must be 'off', 'manual' or 'auto'\n");
						exit(-1);
					}
				}
				if (!strcmp(id, "all")) { /* do all interfaces*/
					if ((iflst = fopen(FILE_PROC, "r")) == NULL) {
						perror(FILE_PROC);
						return -1;
					}
					while (!feof(iflst)) {
						fgets(s, sizeof(s), iflst);
						if ((p = strchr(s, ':'))) {
							*p = 0;
							p = s;
							while (*p && isspace(*p))
								p++;
							do_dialmode(args, cfg.dialmode, fd, p, 0);
							while (*nextslaveif)
								do_dialmode(args, cfg.dialmode, fd, nextslaveif, 0);
						}
					}
					fclose(iflst);
				}
				else {
					do_dialmode(args, cfg.dialmode, fd, id, 1);
				}

                        	break;

#ifdef I4L_CTRL_CONF
			case WRITECONF:
			        if (args == 0) {
			          sprintf(conffile, "%s%c%s", confdir(), C_SLASH, CONFFILE);
			          id = conffile;
			        }

			        if (writeconfig(fd, id))
			        	return -1;

			        printf("ISDN Configuration written to %s.\n", id);
			        break;

			case READCONF:
			        if (args == 0) {
			          sprintf(conffile, "%s%c%s", confdir(), C_SLASH, CONFFILE);
			          id = conffile;
			        }

			        if (readconfig(fd, id))
			        	return -1;

			        printf("ISDN Configuration read from %s.\n", id);
			        break;
#endif /* I4L_CTRL_CONF */

			case IFDEFAULTS: {
#define	MAX_DEFS_ARGS	64

				int	defs_argc;
				char	*defs_argv [MAX_DEFS_ARGS + 1];
				defs_fcn_t	defs_fcn_p;
				int		i;
				char	*s, *s0, *t, *u;


				i = 0;
				while((defs_fcn_p = defs_fcns [i++]) != NULL) {

					s = (*defs_fcn_p)(id);

					if(!s || !*s)
						continue;

					s0 = s = strdup(s);

					while(s && *s) {
						t = strdup(strtok(s, "\n"));
						s += strlen(t) + 1;

						if(!t || !*t)
							continue;

						defs_argc = 0;
						defs_argv [defs_argc] = NULL;

						u = strtok(t, " \t");

						while(u && *u) {
							if(++defs_argc >= MAX_DEFS_ARGS) {
								fprintf(stderr, "default-values overflow.");
								exit(1);
							}

							defs_argv [defs_argc - 1] = strdup(u);
							defs_argv [defs_argc] = NULL;

							u = strtok(NULL, " \t");
						}

						if(defs_argc)  {
							exec_args(fd, defs_argc, defs_argv);

							while(defs_argc--)
								free(defs_argv [defs_argc]);
						}

						free(t);
					}

					free(s0);
				}

			}
			break;
		default:
			printf("here %d\n",i);
		}

#if DEBUG
		if (argc > 1) {
			printf("args=%d nextcmd %s\n",args, argv[1]);
		}
#endif /* DEBUG */
	}

	return 0;
}


char	*defs_basic(char *id) {
	static char	r [1024];
	char	*p = r;

	p += sprintf(p, "dialmode %s off\n", id);
	p += sprintf(p, "huptimeout %s 60\n", id);

	return(r);
}

void check_version(int report) {
	int fd;

	if (report) {
		printf("isdnctrl\n");
	}
	fd = open("/dev/isdn/isdninfo", O_RDWR);
	if (fd < 0)
	        fd = open("/dev/isdninfo", O_RDONLY);
	if (fd < 0) {
                perror("Can't open /dev/isdninfo or /dev/isdn/isdninfo");
		exit(-1);
	}
	data_version = ioctl(fd, IIOCGETDVR, 0);
	if (data_version < 0) {
		fprintf(stderr, "Could not get version of kernel ioctl structs!\n");
		fprintf(stderr, "Make sure that you are using the correct version.\n");
		fprintf(stderr, "(Try recompiling isdnctrl).\n");
		exit(-1);
	}
	close(fd);
	if (report) {
		printf("Kernel's view of API-versions:\n");
		printf("ttyI: %d, net: %d, info: %d\n",
			data_version & 0xff,
			(data_version >> 8) & 0xff,
			(data_version >> 16) & 0xff);
		return;
	}
	data_version = (data_version >> 8) & 0xff;

	if (data_version < 4) {
		fprintf(stderr, "Kernel-version too old, terminating.\n");
		fprintf(stderr, "UPDATE YOUR KERNEL.\n");
		exit(-1);
	}
	if (data_version > 6) {
		fprintf(stderr, "Kernel-version newer than isdnctrl-version, terminating.\n");
		fprintf(stderr, "GET A NEW VERSION OF isdn4k-utils.\n");
		exit(-1);
	}
}

int main(int argc, char **argv)
{
	int fd;
	int rc;

	if ((cmd = strrchr(argv[0], '/')) != NULL)
		*cmd++ = '\0';
	else
		cmd = argv[0];

	if (argc == 1) {
		usage();
		exit(-1);
	}
	if ((argc == 2) && (!strcmp(argv[1], "-V"))) {
		check_version(1);
		exit(0);
	}
	if ((argc == 2) && (!strcmp(argv[1], "--version"))) {
		printf("isdnctrl\n");
		exit(0);
	}
#ifdef I4L_DWABC_UDPINFO
	{
		int art = 0;
		char *p = argv[1];

		if(!strcmp(p,"-udpisisdn")) {
			art = 1;
		} else if(!strcmp(p,"-udponline")) {
			art = 2;
		} else if(!strcmp(p,"-udphangup")) {
			art = 3;
		} else if(!strcmp(p,"-udpdial")) {
			art = 4;
		} else if(!strcmp(p,"-udpclear")) {
			art = 5;
		}

		if(art) {

			char *err = NULL;
			int retw = 0;
			p = argv[2];

			if(argc != 3) {

				usage();
				exit(1);
			}

			switch(art) {
			case 1:	retw = isdn_udp_isisdn(p,&err);		break;
			case 2:	retw = isdn_udp_online(p,&err);		break;
			case 3:	retw = isdn_udp_hangup(p,&err);		break;
			case 4:	retw = isdn_udp_dial(p,&err);		break;
			case 5:	retw = isdn_udp_clear_ggau(p,&err);	break;
			}

			if(err != NULL) {

				fprintf(stderr,"%s\n",err);
				free(err);
				err = NULL;
			}

			if(!retw) {

				switch(art) {
				case 1:
					printf("destination %s is NOT over i4l routed\n",p);
					break;
				case 2:
					printf("destination %s is NOT online\n",p);
					break;
				case 3:
					printf("destination %s CANNOT hangup the line\n",p);
					break;
				case 4:
					printf("destination %s CANNOT dialing\n",p);
					break;
				case 5:
					printf("CANNOT reset (clear) abc-secure-counter's for destination %s\n",p);
				}

			} else {

				switch(art) {
				case 1:
					printf("destination %s is over i4l routed\n",p);
					break;
				case 2:
					printf("destination %s is online\n",p);
					break;
				case 3:
					printf("destination %s trigger hangup \n",p);
					break;
				case 4:
					printf("destination %s trigger dialing\n",p);
					break;
				case 5:
					printf("reset (clear) abc-secure-counter's for destination %s\n",p);
				}
			}

			exit(!retw);
		}
	}
#endif
	check_version(0);

	fd = open("/dev/isdn/isdnctrl", O_RDWR);
	if (fd < 0)
	        fd = open("/dev/isdnctrl", O_RDWR);
	if (fd < 0) {
		perror("Can't open /dev/isdnctrl or /dev/isdn/isdnctrl");
		exit(-1);
	}


	rc = exec_args(fd,argc-1,argv+1);
	close(fd);
	return rc;
}
