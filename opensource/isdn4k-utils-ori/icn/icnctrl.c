/* $Id: icnctrl.c,v 1.5 2002/01/31 18:55:45 paul Exp $

 * ICN-ISDN driver for Linux. (Control-Utility)
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
 * $Log: icnctrl.c,v $
 * Revision 1.5  2002/01/31 18:55:45  paul
 * #include <stdlib.h> for prototypes against warnings.
 *
 * Revision 1.4  1999/09/06 08:03:24  fritz
 * Changed my mail-address.
 *
 * Revision 1.3  1997/06/21 14:38:22  fritz
 * Added option for enabling only one channel in leased mode.
 *
 * Revision 1.2  1997/05/17 12:23:29  fritz
 * Corrected some Copyright notes to refer to GPL.
 *
 * Revision 1.1  1997/02/17 00:08:53  fritz
 * New CVS tree
 *
 * Revision 1.2  1996/11/22 15:04:25  fritz
 * Minor cosmetix fix.
 *
 * Revision 1.1.1.1  1996/10/08 10:03:12  fritz
 * First Import.
 *
 * Revision 1.6  1995/12/18  18:25:24  fritz
 * Support for ICN-4B Cards.
 *
 * Revision 1.5  1995/10/29  21:44:02  fritz
 * Changed to support DriverId's.
 * Added support for leased lines.
 *
 * Revision 1.4  1995/04/23  13:43:02  fritz
 * Changed Copyright.
 *
 * Revision 1.3  1995/02/01  11:03:26  fritz
 * Added Messages, when loading bootcode and protocol.
 *
 * Revision 1.2  1995/01/09  07:41:59  fritz
 * Added GPL-Notice
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
#ifdef __DEBUGVAR__
#define __KERNEL__
#endif
#include <icn.h>

#ifdef __DEBUGVAR__
typedef struct dbgstruct {
	ulong card;
	ulong dev;
} dbgstruct;
dbgstruct dbg;
#endif

int mem_fd;

char *cmd;
char *ctrldev;
int arg_ofs;

#if 0
/* #ifdef _POSIX_MAPPED_FILES */
#define DO_VIA_MMAP
#else
#include <malloc.h>
#endif

void
usage()
{
	fprintf(stderr, "usage: %s [-d driverId] io [mmio port]             (get/set hw-io)\n", cmd);
#ifdef __DEBUGVAR__
	fprintf(stderr, "   or: %s [-d driverId] dump                       (dump driver data)\n", cmd);
#endif
	fprintf(stderr, "   or: %s [-d driverId] add <portbase> [id1 [id2]] (Add a new card\n", cmd);
	fprintf(stderr, "   or: %s [-d driverId] load <bootcode> <protocol> (load firmware)\n", cmd);
	fprintf(stderr, "   or: %s [-d driverId] leased <1|2|on|off>         (Switch interface,\n",cmd);
    fprintf(stderr, "                                                    Channel n\n");
	fprintf(stderr, "                                                    into Leased-Line-Mode)\n");
	exit(-1);
}

#ifdef __DEBUGVAR__
u_char *
mapmem(ulong where, long size)
{
	u_char *addr;
#ifdef DO_VIA_MMAP
	ulong realw;
	long ofs;
#endif

	if ((mem_fd = open("/dev/mem", O_RDWR)) < 0) {
		perror("open /dev/mem");
		exit(1);
	}
#ifdef DO_VIA_MMAP
	size = ((size / PAGE_SIZE) + 2) * PAGE_SIZE;
	realw = PAGE_ALIGN(where) - PAGE_SIZE;
	ofs = where - realw;
	printf("%08lx - %08lx = %08x\n", where, realw, ofs);
	addr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED,
		    mem_fd, realw);
	if ((int) addr < 0) {
		perror("mmap");
		exit(1);
	}
	return (addr + ofs);
#else
	addr = malloc(size);
	lseek(mem_fd, where, SEEK_SET);
	read(mem_fd, addr, size);
	return addr;
#endif
}

void
dump_dev(icn_dev * p)
{
	printf("dev struct at:          %p\n", p);
	printf("phys. dev struct at:    0x%08lx\n", dbg.dev);
	printf("shmem:                  %p\n", p->shmem);
	printf("mvalid:                 %d\n", p->mvalid);
	printf("channel:                %d\n", p->channel);
	printf("mcard:                  %p\n", p->mcard);
	printf("chanlock:               %d\n", p->chanlock);
	printf("firstload:              %d\n", p->firstload);
}

void
qdump(ulong queue)
{
	struct sk_buff_head *p;

	p = (struct sk_buff_head *) mapmem(queue, sizeof(struct sk_buff_head));
	printf("  len:                  %d\n", p->qlen);
}

void
dump_card(icn_card * p)
{
	printf("card struct at:         %p\n", p);
	printf("phys. card struct at:   0x%08lx\n", dbg.card);
	printf("next:                   %p\n", p->next);
	printf("other:                  %p\n", p->other);
	printf("port:                   0x%04x\n", p->port);
	printf("myid:                   %d\n", p->myid);
	printf("rvalid:                 %d\n", p->rvalid);
	printf("leased:                 %d\n", p->leased);
	printf("flags:                  0x%04x\n", p->flags);
	printf("doubleS0:               %d\n", p->doubleS0);
	printf("secondhalf:             %d\n", p->secondhalf);
	printf("fw_rev:                 %d\n", p->fw_rev);
	printf("ptype:                  %d\n", p->ptype);
	printf("st_timer\n");
	printf("rb_timer\n");
	printf("rcvbuf\n");
	printf("rcvidx[0]:              %d\n", p->rcvidx[0]);
	printf("rcvidx[1]:              %d\n", p->rcvidx[1]);
	printf("l2_proto[0]             %d\n", p->l2_proto[0]);
	printf("l2_proto[1]             %d\n", p->l2_proto[1]);
	printf("interface:\n");
	printf("iptr:                   %d\n", p->iptr);
	printf("imsg:                   %s\n", p->imsg);
	printf("msg_buf:                %p\n", p->msg_buf);
	printf("msg_buf_write:          %p\n", p->msg_buf_write);
	printf("msg_buf_read:           %p\n", p->msg_buf_read);
	printf("msg_buf_end:            %p\n", p->msg_buf_end);
	printf("sndcount[0]:            %d\n", p->sndcount[0]);
	printf("sndcount[1]:            %d\n", p->sndcount[1]);
	printf("spqueue[0]:             %p\n", &p->spqueue[0]);
	qdump((ulong) & p->spqueue[0]);
	printf("spqueue[1]:             %p\n", &p->spqueue[1]);
	qdump((ulong) & p->spqueue[1]);
	printf("regname:                %s\n", p->regname);
	printf("xmit_lock[0]:           %d\n", p->xmit_lock[0]);
	printf("xmit_lock[1]:           %d\n", p->xmit_lock[1]);
}
#endif

int
main(int argc, char **argv)
{
	int fd;
	FILE *code;
	int mmio,
	 port;
	int ac;
	int a;
	u_char buf[0x20000];
	isdn_ioctl_struct ioctl_s;
	icn_cdef newcard;

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
		perror(ctrldev);
		exit(-1);
	}
#ifdef __DEBUGVAR__
	if (!strcmp(argv[arg_ofs], "dump")) {
		char *p;
		ioctl_s.arg = (ulong) & dbg;
		if ((ioctl(fd, ICN_IOCTL_DEBUGVAR + IIOCDRVCTL, &ioctl_s)) < 0) {
			perror("ioctl DEBUGVAR");
			exit(-1);
		}
		close(fd);
		p = mapmem(dbg.dev, sizeof(icn_dev));
		dump_dev((icn_dev *) p);
		p = mapmem(dbg.card, sizeof(icn_card));
		dump_card((icn_card *) p);
		return 0;
	}
#endif
	if (!strcmp(argv[arg_ofs], "leased")) {
		if (ac == 3) {
			ioctl_s.arg = 0;
			if ((!strcmp(argv[arg_ofs + 1], "on")) ||
			    (!strcmp(argv[arg_ofs + 1], "2"))  ||
			    (!strcmp(argv[arg_ofs + 1], "yes")))
				ioctl_s.arg = 3;
			if (!strcmp(argv[arg_ofs + 1], "1"))
				ioctl_s.arg = 1;
			if ((ioctl(fd, ICN_IOCTL_LEASEDCFG + IIOCDRVCTL, &ioctl_s)) < 0) {
				perror("ioctl LEASEDCFG");
				exit(-1);
			}
			close(fd);
			return 0;
		}
	}
	if (!strcmp(argv[arg_ofs], "add")) {
		if (ac >= 3) {
			sscanf(argv[arg_ofs + 1], "%i", &a);
			if (a == 0x300 || a == 0x310 || a == 0x320 || a == 0x330
			    || a == 0x340 || a == 0x350 || a == 0x360 ||
			    a == 0x308 || a == 0x318 || a == 0x328 || a == 0x338
			    || a == 0x348 || a == 0x358 || a == 0x368) {
				ioctl_s.arg = (unsigned long) &newcard;
				newcard.port = a;
				newcard.id1[0] = 0;
				newcard.id2[0] = 0;
				if (ac >= 4)
					strncpy(newcard.id1, argv[arg_ofs + 2], sizeof(newcard.id1) - 1);
				if (ac == 5)
					strncpy(newcard.id2, argv[arg_ofs + 3], sizeof(newcard.id2) - 1);
				if ((ioctl(fd, ICN_IOCTL_ADDCARD + IIOCDRVCTL, &ioctl_s)) < 0) {
					perror("ioctl ADDCARD");
					exit(-1);
				}
				close(fd);
				return 0;
			}
		}
	}
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
	if (!strcmp(argv[arg_ofs], "load")) {
		int needed;
		switch (ioctl(fd, ICN_IOCTL_GETDOUBLE + IIOCDRVCTL, &ioctl_s)) {
			case 0:
				needed = 4;
				break;
			case 1:
				needed = 5;
				break;
			default:
				perror("ioctl GETDOUBLE");
				exit(-1);
		}
		if (ac == needed) {
			if (!(code = fopen(argv[arg_ofs + 1], "r"))) {
				perror(argv[arg_ofs + 1]);
				exit(-1);
			}
			if (fread(buf, 4096, 1, code) < 1) {
				fprintf(stderr, "Read error on %s\n", argv[arg_ofs + 1]);
				exit(-1);
			}
			printf("Loading Bootcode %s ... ", argv[arg_ofs + 1]);
			fflush(stdout);
			ioctl_s.arg = (ulong) buf;
			if (ioctl(fd, ICN_IOCTL_LOADBOOT + IIOCDRVCTL, &ioctl_s) < 0) {
				perror("\nioctl LOADBOOT");
				exit(-1);
			}
			fclose(code);
			printf("done\n");
			if (!(code = fopen(argv[arg_ofs + 2], "r"))) {
				perror(argv[arg_ofs + 2]);
				exit(-1);
			}
			if (fread(buf, 65536, 1, code) < 1) {
				fprintf(stderr, "Read error on %s\n", argv[arg_ofs + 2]);
				exit(-1);
			}
			fclose(code);
			if (needed == 5) {
				if (!(code = fopen(argv[arg_ofs + 3], "r"))) {
					perror(argv[arg_ofs + 3]);
					exit(-1);
				}
				if (fread(buf + 65536, 65536, 1, code) < 1) {
					fprintf(stderr, "Read error on %s\n", argv[arg_ofs + 3]);
					exit(-1);
				}
				fclose(code);
				printf("Loading Protocols %s\n", argv[arg_ofs + 2]);
				printf("              and %s ... ", argv[arg_ofs + 3]);
			} else
				printf("Loading Protocol %s ... ", argv[arg_ofs + 2]);
			fflush(stdout);
			ioctl_s.arg = (ulong) buf;
			if (ioctl(fd, ICN_IOCTL_LOADPROTO + IIOCDRVCTL, &ioctl_s) < 0) {
				perror("\nioctl LOADPROTO");
				exit(-1);
			}
			printf("done\n");
			close(fd);
			return 0;
		}
		usage();
	}
	usage();
	return 0;
}
