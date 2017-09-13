/* $Id: loopctrl.c,v 1.3 1999/09/06 08:03:26 fritz Exp $

 * loop-ISDN driver for Linux. (Control-Utility)
 *
 * Copyright 1994,95 by Fritz Elfert (fritz@isdn4linux.de)
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
 * $Log: loopctrl.c,v $
 * Revision 1.3  1999/09/06 08:03:26  fritz
 * Changed my mail-address.
 *
 * Revision 1.2  1997/05/17 12:23:41  fritz
 * Corrected some Copyright notes to refer to GPL.
 *
 * Revision 1.1  1997/03/24 23:38:46  fritz
 * Added loopctrl utility.
 *
 */

#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <linux/isdn.h>
#include <linux/isdnif.h>
#ifdef __DEBUGVAR__
#define __KERNEL__
#endif
#include <isdnloop.h>

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
	fprintf(stderr, "usage: %s [-d Id] add [Id]                   (Add a new card\n", cmd);
#ifdef __DEBUGVAR__
	fprintf(stderr, "   or: %s [-d Id] dump                       (dump driver data)\n", cmd);
#endif
	fprintf(stderr, "   or: %s [-d Id] start <1tr6|dss1> n1 [n2 n3] (start driver)\n", cmd);
	fprintf(stderr, "   or: %s [-d Id] leased <on|off>            (Switch interface,\n",cmd);
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
	int ac;
	isdn_ioctl_struct ioctl_s;
	isdnloop_cdef newcard;
	isdnloop_sdef startparm;

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
		if ((ioctl(fd, ISDNLOOP_IOCTL_DEBUGVAR + IIOCDRVCTL, &ioctl_s)) < 0) {
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
			    (!strcmp(argv[arg_ofs + 1], "yes")))
				ioctl_s.arg = 1;
			if ((ioctl(fd, ISDNLOOP_IOCTL_LEASEDCFG + IIOCDRVCTL, &ioctl_s)) < 0) {
				perror("ioctl LEASEDCFG");
				exit(-1);
			}
			close(fd);
			return 0;
		}
	}
	if (!strcmp(argv[arg_ofs], "add")) {
		if (ac >= 2) {
			ioctl_s.arg = (unsigned long) &newcard;
			newcard.id1[0] = 0;
			strncpy(newcard.id1, argv[arg_ofs + 1], sizeof(newcard.id1) - 1);
			if ((ioctl(fd, ISDNLOOP_IOCTL_ADDCARD + IIOCDRVCTL, &ioctl_s)) < 0) {
				perror("ioctl ADDCARD");
				exit(-1);
			}
			close(fd);
			return 0;
		}
	}
	if (!strcmp(argv[arg_ofs], "start")) {
		int needed = 99;
		if (ac > 2) {
			if (!(strcmp(argv[arg_ofs + 1], "1tr6"))) {
				needed = 4;
				startparm.ptype = ISDN_PTYPE_1TR6;
			}
			if (!(strcmp(argv[arg_ofs + 1], "dss1"))) {
				needed = 6;
				startparm.ptype = ISDN_PTYPE_EURO;
			}
			if (ac >= needed) {
				strcpy(startparm.num[0], argv[arg_ofs + 2]);
				if (needed > 4) {
					strcpy(startparm.num[1], argv[arg_ofs + 3]);
					strcpy(startparm.num[2], argv[arg_ofs + 4]);
				}
				ioctl_s.arg = (unsigned long) &startparm;
				if (ioctl(fd, ISDNLOOP_IOCTL_STARTUP + IIOCDRVCTL, &ioctl_s) < 0) {
					perror("\nioctl STARTUP");
					exit(-1);
				}
				printf("done\n");
				close(fd);
				return 0;
			}
		}
		usage();
	}
	usage();
	return 0;
}
