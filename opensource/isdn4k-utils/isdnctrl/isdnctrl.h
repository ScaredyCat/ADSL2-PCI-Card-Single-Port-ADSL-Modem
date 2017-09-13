/* $Id: isdnctrl.h,v 1.20 2002/02/07 10:44:12 paul Exp $
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
 * $Log: isdnctrl.h,v $
 * Revision 1.20  2002/02/07 10:44:12  paul
 * Added cisco_h and cisco_hk as aliases for cisco-h and cisco-hk
 * because the manpage used to have this wrong.
 *
 * Revision 1.19  2001/05/23 14:59:23  kai
 * removed traces of TIMRU. I hope it's been dead for a long enough time now.
 *
 * Revision 1.18  2001/05/23 14:48:23  kai
 * make isdnctrl independent of the version of installed kernel headers,
 * we have our own copy now.
 *
 * Revision 1.17  1999/11/20 22:23:53  detabc
 * added netinterface abc-secure-counter reset (clear) support.
 *
 * Revision 1.16  1999/11/02 20:41:21  keil
 * make phonenumber ioctl compatible for ctrlconf too
 *
 * Revision 1.15  1999/09/06 08:03:26  fritz
 * Changed my mail-address.
 *
 * Revision 1.14  1999/06/07 19:25:42  paul
 * isdnctrl.man.in
 *
 * Revision 1.13  1999/03/15 15:53:06  cpetig
 * added v110 and modem to the level2 encapsulations
 *
 * Revision 1.12  1998/10/21 16:18:48  paul
 * Implementation of "dialmode" (successor of "status")
 *
 * Revision 1.11  1998/06/09 18:11:33  cal
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
 * Revision 1.10  1998/03/16 09:40:56  cal
 * fixed a problem parsing TimRu-Commands
 * started with TimRu-man-page
 *
 * Revision 1.9  1998/03/07 18:25:58  cal
 * added support for dynamic timeout-rules vs. 971110
 *
 * Revision 1.8  1998/02/12 23:14:44  he
 * added encap x25iface and l2_prot x25dte, x25dce
 *
 * Revision 1.7  1997/10/03 21:48:09  fritz
 * Added cisco-hk encapsulation.
 *
 * Revision 1.6  1997/08/21 14:47:02  fritz
 * Added Version-Checking of NET_DV.
 *
 * Revision 1.5  1997/07/30 20:09:26  luethje
 * the call "isdnctrl pppbind ipppX" will be bound the interface to X
 *
 * Revision 1.4  1997/07/23 20:39:16  luethje
 * added the option "force" for the commands delif and reset
 *
 * Revision 1.3  1997/07/22 22:36:11  luethje
 * isdnrep:  Use "&nbsp;" for blanks
 * isdnctrl: Add the option "reset"
 *
 * Revision 1.2  1997/07/20 16:36:28  calle
 * isdnctrl trigger was not working.
 *
 * Revision 1.1  1997/06/24 23:35:28  luethje
 * isdnctrl can use a config file
 *
 *
 */

#include "isdn.h"
#include "isdnif.h"


/*****************************************************************************/

#define FILE_PROC "/proc/net/dev"

/*****************************************************************************/

enum {
        ADDIF, ADDSLAVE, DELIF, DIAL,
        BIND, UNBIND, PPPBIND, PPPUNBIND,
        BUSREJECT, MAPPING, SYSTEM, HANGUP,
        ADDPHONE, DELPHONE, LIST, EAZ,
        VERBOSE, HUPTIMEOUT, CBDELAY,
        CHARGEINT, DIALMAX, SDELAY, CHARGEHUP,
        CBHUP, IHUP, SECURE, CALLBACK,
        L2_PROT, L3_PROT, ADDLINK, REMOVELINK,
        ENCAP, TRIGGER, RESET,
        DIALTIMEOUT, DIALWAIT, DIALMODE,
#ifdef I4L_CTRL_CONF
        WRITECONF, READCONF,
#endif /* I4L_CTRL_CONF */
#ifdef I4L_DWABC_UDPINFO
		ABCCLEAR,
#endif
	STATUS,
		IFDEFAULTS
};

typedef struct {
        char *cmd;
        char *argno;
} cmd_struct;

/*****************************************************************************/

#ifdef _ISDNCTRL_C_
#define _EXTERN

cmd_struct cmds[] =
{
        {"addif", "01"},
        {"addslave", "2"},
        {"delif", "12"},
        {"dial", "1"},
        {"bind", "123"},
        {"unbind", "1"},
        {"pppbind", "12"},
        {"pppunbind", "1"},
        {"busreject", "2"},
        {"mapping", "12"},
        {"system", "1"},
        {"hangup", "1"},
        {"addphone", "3"},
        {"delphone", "3"},
        {"list", "1"},
        {"eaz", "12"},
        {"verbose", "1"},
        {"huptimeout", "12"},
        {"cbdelay", "12"},
        {"chargeint", "12"},
        {"dialmax", "12"},
        {"sdelay", "12"},
        {"chargehup", "12"},
        {"cbhup", "12"},
        {"ihup", "12"},
        {"secure", "12"},
        {"callback", "12"},
        {"l2_prot", "12"},
        {"l3_prot", "12"},
        {"addlink", "1"},
        {"removelink", "1"},
        {"encap", "12"},
        {"trigger", "12"},
        {"reset", "01"},
        {"dialtimeout", "12"},
        {"dialwait", "12"},
        {"dialmode", "12"},
#ifdef I4L_CTRL_CONF
        {"writeconf", "01"},
        {"readconf", "01"},
#endif /* I4L_CTRL_CONF */
#ifdef I4L_DWABC_UDPINFO
		{"abcclear","1"},
#endif
        {"status", "1"},
        {"ifdefaults", "01"},
        {NULL,}
};

char *l2protostr[] = {
	"x75i", "x75ui", "x75bui", "hdlc", 
	"x25dte", "x25dce",
	"v110_9600", "v110_19200", "v110_38400",
	"modem",
	"\0"
};

int l2protoval[] = {
        ISDN_PROTO_L2_X75I, ISDN_PROTO_L2_X75UI,
        ISDN_PROTO_L2_X75BUI, ISDN_PROTO_L2_HDLC,
	ISDN_PROTO_L2_X25DTE, ISDN_PROTO_L2_X25DCE,
	ISDN_PROTO_L2_V11096, ISDN_PROTO_L2_V11019, ISDN_PROTO_L2_V11038,
	ISDN_PROTO_L2_MODEM,
	-1
};

char *l3protostr[] = {
        "trans", "\0"
};

int l3protoval[] = {
        ISDN_PROTO_L3_TRANS, -1
};

char *pencapstr[] = {
	"ethernet",
	"rawip",
	"ip",
	"cisco-h",
	"cisco_h",
	"syncppp",
	"uihdlc",
	"cisco-hk",
	"cisco_hk",
	"x25iface",
	"\0"
};

int pencapval[] = {
	ISDN_NET_ENCAP_ETHER,
	ISDN_NET_ENCAP_RAWIP,
	ISDN_NET_ENCAP_IPTYP,
	ISDN_NET_ENCAP_CISCOHDLC,
	ISDN_NET_ENCAP_CISCOHDLC,
	ISDN_NET_ENCAP_SYNCPPP,
	ISDN_NET_ENCAP_UIHDLC,
	ISDN_NET_ENCAP_CISCOHDLCK,
	ISDN_NET_ENCAP_CISCOHDLCK,
	ISDN_NET_ENCAP_X25IFACE,
	-1
};

char *num2callb[] = {
        "off", "in", "out"
};

#else
#define _EXTERN extern

_EXTERN cmd_struct cmds[];
_EXTERN char *l2protostr[];
_EXTERN int   l2protoval[];
_EXTERN char *l3protostr[];
_EXTERN int   l3protoval[];
_EXTERN char *pencapstr[];
_EXTERN int   pencapval[];
_EXTERN char *num2callb[];

#endif

_EXTERN int data_version;

_EXTERN char *cmd;

_EXTERN int key2num(char *key, char **keytable, int *numtable);
_EXTERN char * num2key(int num, char **keytable, int *numtable);
_EXTERN int exec_args(int fd, int argc, char **argv);

_EXTERN char * defs_basic(char *id);

_EXTERN int MSNLEN_COMPATIBILITY;

/*
 * set_isdn_net_ioctl_phone handles back/forward compatibility between
 * version 4, 5 and 6 of isdn_net_ioctl_phone
 *
 */
 
typedef union {
		isdn_net_ioctl_phone_6 phone_6;
		isdn_net_ioctl_phone_5 phone_5;
} isdn_net_ioctl_phone;

extern int set_isdn_net_ioctl_phone(isdn_net_ioctl_phone *ph, char *name, 
				    char *phone, int outflag);


#undef _EXTERN

/*****************************************************************************/
