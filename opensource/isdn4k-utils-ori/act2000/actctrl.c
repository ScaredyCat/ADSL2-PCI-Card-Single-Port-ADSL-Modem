/* $Id: actctrl.c,v 1.3 2002/01/31 18:50:51 paul Exp $

 * IBM Active 2000 ISDN driver for Linux. (Control-Utility)
 *
 * Copyright 1994,95 by Fritz Elfert (fritz@isdn4linux.de)
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
 * $Log: actctrl.c,v $
 * Revision 1.3  2002/01/31 18:50:51  paul
 * #include <stdlib.h> for prototypes against warnings.
 *
 * Revision 1.2  1999/09/06 08:03:23  fritz
 * Changed my mail-address.
 *
 * Revision 1.1  1997/09/25 21:41:37  fritz
 * Added actctrl.
 *
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <linux/isdn.h>
#include "act2000.h"

char *cmd;
char *ctrldev;
int arg_ofs;

void
usage()
{
#if 0
	fprintf(stderr, "usage: %s [-d driverId] io [port] [irq]             (get/set hw-io)\n", cmd);
#endif
	fprintf(stderr, "   or: %s [-d driverId] add [bus [port [irq [id]]]] (Add a new card\n", cmd);
	fprintf(stderr, "   or: %s [-d driverId] load <bootcode>             (load firmware)\n", cmd);
	fprintf(stderr, "   or: %s [-d driverId] dproto <1tr6|euro> [msn's]  (set D-channel protocol)\n", cmd);
#if 0
	fprintf(stderr, "   or: %s [-d driverId] dbg <num>                   (devel Func)\n", cmd);
#endif
	exit(-1);
}

int
main(int argc, char **argv)
{
	int fd;
	FILE *code;
	int ac;
	int a;
	u_char buf[500000];
	isdn_ioctl_struct ioctl_s;
	act2000_ddef dload;
	act2000_cdef newcard;

	cmd = strrchr(argv[0], '/');
	cmd = (cmd == NULL) ? argv[0] : ++cmd;
	if (argc > 1) {
		if (!strcmp(argv[1], "-d")) {
			strcpy(ioctl_s.drvid, argv[2]);
			arg_ofs = 3;
		} else {
			ioctl_s.drvid[0] = '\0';
			arg_ofs = 1;
		}
	} else
		usage();
	ac = argc - (arg_ofs - 1);
	fd = open("/dev/isdnctrl", O_RDWR);
	if (fd < 0) {
		perror("/dev/isdnctrl");
		exit(-1);
	}
	if (!strcmp(argv[arg_ofs], "add")) {
		if (ac < 3)
			usage();
		if (ac >= 3) {
			a = -1;
			if (!strcmp(argv[arg_ofs + 1], "isa"))
				a = ACT2000_BUS_ISA;
			if (!strcmp(argv[arg_ofs + 1], "mca"))
				a = ACT2000_BUS_MCA;
			if (!strcmp(argv[arg_ofs + 1], "pcmcia"))
				a = ACT2000_BUS_PCMCIA;
			if (a == -1) {
				fprintf(stderr, "Bus must be \"isa\", \"mca\" or \"pcmcia\"\n");
				exit(-1);
			}
			if (a != ACT2000_BUS_ISA) {
				fprintf(stderr, "Currently only isa Bus is supported\n");
				exit(-1);
			}
			newcard.bus = a;
		}
		a = -2;
		if (ac >= 4) {
			if (!strcmp(argv[arg_ofs + 2], "auto"))
				a = -1;
			else
				sscanf(argv[arg_ofs + 2], "%i", &a);
			if (a == 0x200 || a == 0x240 || a == 0x280 || a == 0x2c0
			    || a == 0x300 || a == 0x340 || a == 0x380 ||
			    a == 0xcfe0 || a == 0xcfa0 || a == 0xcf60 || a == 0xcf20
			    || a == 0xcee0 || a == 0xcea0 || a == 0xce60 || a == -1)
				newcard.port = a;
			else {
				fprintf(stderr, "Invalid port. Possible values are:\n");
				fprintf(stderr, "  0x200, 0x240, 0x280, 0x2c0, 0x300, 0x340\n");
				fprintf(stderr, "  0x380, 0xcfe0, 0xcfa0, 0xcf60, 0xcf20\n");
				fprintf(stderr, "  0xcee0, 0xcea0, 0xce60 or \"auto\"\n");
				exit(-1);
			}
		}
		newcard.irq = -1;
		if (ac >= 5) {
			a = -2;
			if (!strcmp(argv[arg_ofs + 3], "auto"))
				a = -1;
			if (!strcmp(argv[arg_ofs + 3], "none"))
				a = 0;
			if (a == -2)
				sscanf(argv[arg_ofs + 2], "%i", &a);
			if (a == 3 || a == 5 || a == 7 || a == 10 || a == 11 || a == 12
				|| a == 15 || a == -1 || a == 0)
				newcard.irq = a;
			else {
				fprintf(stderr, "Invalid IRQ. Possible values are:\n");
				fprintf(stderr, "  3, 5, 7, 10, 11, 12, 15, \"auto\" or \"none\"\n");
				exit(-1);
			}
		}
		ioctl_s.arg = (unsigned long) &newcard;
		newcard.id[0] = 0;
		if (ac >= 6)
			strncpy(newcard.id, argv[arg_ofs + 4], sizeof(newcard.id) - 1);
		if ((ioctl(fd, ACT2000_IOCTL_ADDCARD + IIOCDRVCTL, &ioctl_s)) < 0) {
			perror("ioctl ADDCARD");
			exit(-1);
		}
		close(fd);
		return 0;
	}
#if 0
	if (!strcmp(argv[arg_ofs], "io")) {
		if (ac == 4) {
			if (sscanf(argv[arg_ofs + 1], "%i", &mmio) != 1)
				usage();
			if (sscanf(argv[arg_ofs + 2], "%i", &port) != 1)
				usage();
			ioctl_s.arg = mmio;
			if ((mmio = ioctl(fd, ICN_IOCTL_SETMMIO + IIOCDRVCTL, &ioctl_s)) < 0) {
				perror("ioctl SETMMIO");
				exit(-1);
			}
			ioctl_s.arg = port;
			if ((port = ioctl(fd, ICN_IOCTL_SETPORT + IIOCDRVCTL, &ioctl_s)) < 0) {
				perror("ioctl SETPORT");
				exit(-1);
			}
		}
		if ((mmio = ioctl(fd, ICN_IOCTL_GETMMIO + IIOCDRVCTL, &ioctl_s)) < 0) {
			perror("ioctl GETMMIO");
			exit(-1);
		}
		if ((port = ioctl(fd, ICN_IOCTL_GETPORT + IIOCDRVCTL, &ioctl_s)) < 0) {
			perror("ioctl GETPORT");
			exit(-1);
		}
		printf("Memory-mapped io at 0x%08lx, port 0x%03x\n",
		       (unsigned long) mmio, (unsigned short) port);
		close(fd);
		return 0;
	}
#endif
	if (!strcmp(argv[arg_ofs], "load")) {
		if (ac == 3) {
			if (!(code = fopen(argv[arg_ofs + 1], "r"))) {
				perror(argv[arg_ofs + 1]);
				exit(-1);
			}
			dload.buffer = buf;
			if ((dload.length = fread(buf, 1, sizeof(buf), code)) < 1) {
				fprintf(stderr, "Read error on %s\n", argv[arg_ofs + 1]);
				exit(-1);
			}
			dload.length -= 32;
			dload.buffer += 32;
			printf("Loading Firmware %s ... ", argv[arg_ofs + 1]);
			fflush(stdout);
			ioctl_s.arg = (ulong) & dload;
			if (ioctl(fd, ACT2000_IOCTL_LOADBOOT + IIOCDRVCTL, &ioctl_s) < 0) {
				perror("\nioctl LOADBOOT");
				exit(-1);
			}
			fclose(code);
			printf("done\n");
			return 0;
		}
		usage();
	}
	if (!strcmp(argv[arg_ofs], "dproto")) {
		if (ac >= 3) {
			char msnlist[1024];

			int proto = -1;
			if (strcmp(argv[arg_ofs + 1], "1tr6") == 0)
				proto = 0;
			if (strcmp(argv[arg_ofs + 1], "euro") == 0)
				proto = 1;
			if (proto == -1)
				usage();
			msnlist[0] = '\0';
			if (ac > 3) {
				if (proto == 0)
					usage();
				strcpy(msnlist, argv[arg_ofs + 2]);
			}
			ioctl_s.arg = (ulong)proto;
			if (ioctl(fd, ACT2000_IOCTL_SETPROTO + IIOCDRVCTL, &ioctl_s) < 0) {
				perror("ioctl SETPROTO");
				exit(-1);
			}
			if (strlen(msnlist)) {
				char *p = msnlist;
				char m[17];
				int i = 0;
				char *q;

				while ((q = strtok(p, ",")) && (i<10)) {
					p = NULL;
					if (strlen(q) > 15) {
						fprintf(stderr, "MSN may not exceed 15 digits\n");
						exit(-1);
					}
					if (q[0] == '-')
						sprintf(m, "%d", i);
					else
						sprintf(m, "%d%s", i, q);
					ioctl_s.arg = (ulong)&m;
					if (ioctl(fd, ACT2000_IOCTL_SETMSN + IIOCDRVCTL, &ioctl_s) < 0) {
						perror("ioctl SETMSN");
						exit(-1);
					}
					i++;
				}
			}
			return 0;
		}
		usage();
	}
	if (!strcmp(argv[arg_ofs], "dbg")) {
		if (ac == 3) {
			int res;
			unsigned long parm;
			res = sscanf(argv[arg_ofs + 1], "%li", &parm);
			if (res != 1) {
				fprintf(stderr, "Couldn't read parameter\n");
				exit(-1);
			}
			ioctl_s.arg = parm;
			if (ioctl(fd, ACT2000_IOCTL_TEST + IIOCDRVCTL, &ioctl_s) < 0) {
				perror("ioctl TEST");
				exit(-1);
			}
			return 0;
		}
		usage();
	}
	usage();
	return 0;
}
