/* $Id: iprofd.c,v 1.9 2001/01/06 18:25:05 kai Exp $

 * Daemon for saving ttyIx-profiles to a file.
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
 */

#define SIGNATURE "iprofd%02x"
#define SIGLEN 9

#include <sys/types.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <string.h>
//#include <linux/isdn.h>

typedef unsigned char uchar;

int isdnctrl_fd;
char *modemsettings;

#define IIOCSIGPRF  _IO('I',14)
#define IIOCGETPRF  _IO('I',15)
#define IIOCSETPRF  _IO('I',16)
#define IIOCGETDVR  _IO('I',22)

#define ISDN_MAX_CHANNELS   64

#define ISDN_LMSNLEN_4 0
#define ISDN_LMSNLEN_5 255
#define ISDN_LMSNLEN_6 255

#define ISDN_MSNLEN_4 20
#define ISDN_MSNLEN_5 20
#define ISDN_MSNLEN_6 32

#define ISDN_MODEM_NUMREG_4 23
#define ISDN_MODEM_NUMREG_5 24
#define ISDN_MODEM_NUMREG_6 24

#define BUFSZ_4 ((ISDN_MODEM_NUMREG_4+ISDN_MSNLEN_4+ISDN_LMSNLEN_4)*ISDN_MAX_CHANNELS)
#define BUFSZ_5 ((ISDN_MODEM_NUMREG_5+ISDN_MSNLEN_5+ISDN_LMSNLEN_5)*ISDN_MAX_CHANNELS)
#define BUFSZ_6 ((ISDN_MODEM_NUMREG_6+ISDN_MSNLEN_6+ISDN_LMSNLEN_6)*ISDN_MAX_CHANNELS)

int bufsz;
int tty_dv;

void
dumpModem(int dummy)
{
	int fd;
	int len;
	char buffer[bufsz];
	char signature[SIGLEN];

	if ((len = ioctl(isdnctrl_fd, IIOCGETPRF, &buffer)) < 0) {
		perror("ioctl IIOCGETPRF");
		exit(-1);
	}
	fd = open(modemsettings, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0) {
		perror(modemsettings);
		exit(-1);
	}
	sprintf(signature, SIGNATURE, tty_dv);
	write(fd, signature, sizeof(signature));
	write(fd, buffer, len);
	close(fd);
	signal(SIGIO, dumpModem);
}

void
readModem(void)
{
	int len;
	int fd;
	char buffer[bufsz];
	char signature[SIGLEN];

	sprintf(signature, SIGNATURE, tty_dv);
	fd = open(modemsettings, O_RDONLY);
	if (fd < 0)
		return;
	len = read(fd, buffer, sizeof(signature));
	if (len < 0) {
		perror(modemsettings);
		exit(-1);
	}
	if (len == 0) {     /* empty file, ignore it */
		close(fd);
		return;
	}
	if (strcmp(buffer, signature)) {
		fprintf(stderr, "Currently running kernel does NOT match\n");
		fprintf(stderr, "signature of saved data!\n");
		fprintf(stderr, "Profiles NOT restored, use AT&W0 to update data.\n");
		close(fd);
		return;
	}
	len = read(fd, buffer, bufsz);
	if (len < 0) {
		perror(modemsettings);
		exit(-1);
	}
	close(fd);
	if (ioctl(isdnctrl_fd, IIOCSETPRF, &buffer) < 0) {
		perror("ioctl IIOCSETPRF");
		exit(-1);
	}
}

void
usage(void)
{
	fprintf(stderr, "usage: iprofd <IsdnModemProfile>\n");
	exit(-1);
}

int
main(int argc, char **argv)
{

	int data_version;

	if (argc != 2)
		usage();
	modemsettings = argv[1];
	isdnctrl_fd = open("/dev/isdninfo", O_RDWR);
	if (isdnctrl_fd < 0) {
		perror("/dev/isdninfo");
		exit(-1);
	}
	data_version = ioctl(isdnctrl_fd, IIOCGETDVR, 0);
	if (data_version < 0) {
		fprintf(stderr, "Could not get version of kernel modem-profile!\n");
		fprintf(stderr, "Make sure, you are using the correct version.\n");
		fprintf(stderr, "(Try recompiling iprofd).\n");
		exit(-1);
	}
	close(isdnctrl_fd);
	tty_dv = data_version & 0xff;
	switch (tty_dv) {
	case 4: 
		bufsz = BUFSZ_4;
		break;
	case 5: 
		bufsz = BUFSZ_5;
		break;
	case 6: 
		bufsz = BUFSZ_6;
		break;
	default:
		fprintf(stderr, "Version of kernel modem-profile (%d) is NOT handled\n",
			tty_dv);
		fprintf(stderr, "by this version of iprofd!\n");
		fprintf(stderr, "(Try to get the latest version).\n");
		exit(-1);
	}

	isdnctrl_fd = open("/dev/isdnctrl", O_RDONLY);
	if (isdnctrl_fd < 0) {
		perror("/dev/isdninfo");
		exit(-1);
	}
	readModem();
	switch (fork()) {
		case -1:
			perror("fork");
			exit(-1);
			break;
		case 0:
			dumpModem(0);
			if (ioctl(isdnctrl_fd, IIOCSIGPRF, 0)) {
				perror("ioctl IIOCSIGPRF");
				exit(-1);
			}
			while (1)
				sleep(1000);
			break;
		default:
			break;
	}
	return 0;
}
