/* $Id: hisaxctrl.c,v 1.2 2000/06/29 17:38:26 akool Exp $
 *
 * Configuration tool for HiSax ISDN cards
 *
 * Copyright 2000 by Karsten Keil (keil@isdn4linux.de)
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
 */

#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/isdn.h>

char           *progname;
unsigned char         *buffer = NULL;
#define MAX_SIZE 0x10000

unsigned char *
read_firmware(unsigned char *fname)
{
	FILE *infile;
	int  *p, cnt;
	
	if (!(infile = fopen(fname, "rb"))) {
		fprintf(stderr, "cannot open file %s\n", fname);
		exit(-1);
	}
	p = (int *) buffer = (unsigned char *) malloc(MAX_SIZE+4);
	if (!buffer) {
		fprintf(stderr, "cannot get %d byte memory\n", MAX_SIZE+4);
		exit(-1);
	}
	cnt = fread(buffer + 4, 1, MAX_SIZE, infile);
	fclose(infile);
	if (cnt==MAX_SIZE) {
		fprintf(stderr, "wrong filesize\n");
		exit(-1);
	}
	*p = cnt;
	return(buffer);
}

void 
usage()
{
	fprintf(stderr, "usage: %s <DriverId> <IoctlCmd> <IoctlArg>\n", progname);
	exit(-1);
}

int 
main(int argc, char *argv[])
{
	int             fd;
	int             cmd;
	isdn_ioctl_struct ioctl_s;

	if ((progname = strrchr(argv[0], '/')))
		progname++;
	else
		progname = argv[0];

	if (strcmp(progname, "report")) {
		if (argc != 4)
			usage();
		strcpy(ioctl_s.drvid, argv[1]);
		cmd = strtol(argv[2], NULL, 0);
		if (cmd == 9) {
			ioctl_s.arg = (ulong) read_firmware(argv[3]);
		} else {
			ioctl_s.arg = strtol(argv[3], NULL, 0);
		}
	} else {
		ioctl_s.drvid[0] = '\0';
		ioctl_s.arg = 0;
		cmd = 0;
	}
	fd = open("/dev/isdn/isdnctrl", O_RDWR);
	if (fd < 0)
        	fd = open("/dev/isdnctrl", O_RDWR);
	if (fd < 0) {
		perror("/dev/isdnctrl");
		exit(-1);
	}
	if (ioctl(fd, IIOCDRVCTL + cmd, &ioctl_s) < 0)
		perror(argv[1]);
	close(fd);
	if (buffer)
		free(buffer);
	return 0;
}
