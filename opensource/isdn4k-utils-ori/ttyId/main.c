/* $Id: main.c,v 1.1 2000/08/30 18:27:01 armin Exp $
 *
 * ttyId - CAPI TTY AT-command emulator
 *
 * based on the AT-command emulator of the isdn4linux
 * kernel subsystem.
 *
 * Copyright 2000 by Armin Schindler (mac@melware.de)
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
 * $Log: main.c,v $
 * Revision 1.1  2000/08/30 18:27:01  armin
 * Okay, here is the first try for an user-land
 * ttyI daemon. Compilable but not useable.
 *
 *
 */


#include "ttyId.h"
#include "pty.h"
#include <getopt.h>

int debug = 0;
int dev_linked = 0;
int pty_fd = -1;
char slave_name[80];
char device_name[80];

modem_info info;

void EXIT_ttyId(int rc);

static struct option Arguments[] =
{
	{ "device" , 1, NULL, 'd' },
	{ "verbose" , 0, NULL, 'v' },
	{ "help", 0, NULL, 'h' },
	{ NULL, 0, NULL, 0  }
};

void
HUP_Signal(int sig)
{
	logit(LOG_DEBUG, "got HUP signal");

}

void
TERM_Signal(int sig)
{
	logit(LOG_DEBUG, "got TERM signal");
	
	EXIT_ttyId(sig);
}

void
ALARM_Signal(int sig)
{


}

static void SetSignals(void)
{
	signal(SIGTERM		, TERM_Signal);
	signal(SIGHUP		, HUP_Signal);
	signal(SIGALRM		, ALARM_Signal);
	signal(SIGTTIN		, SIG_IGN);
	signal(SIGTTOU		, SIG_IGN);
	signal(SIGINT		, SIG_IGN);
	signal(SIGQUIT		, SIG_IGN);
	signal(SIGILL		, SIG_IGN);
	signal(SIGTRAP		, SIG_IGN);
	signal(SIGABRT		, SIG_IGN);
	signal(SIGUNUSED	, SIG_IGN);
	signal(SIGUSR1		, SIG_IGN);
	signal(SIGUSR2		, SIG_IGN);
	signal(SIGPIPE		, SIG_IGN);
	signal(SIGSTKFLT	, SIG_IGN);
	signal(SIGCHLD		, SIG_IGN);
	signal(SIGTSTP		, SIG_IGN);
	signal(SIGIO		, SIG_IGN);
	signal(SIGXCPU		, SIG_IGN);
	signal(SIGXFSZ		, SIG_IGN);
	signal(SIGVTALRM	, SIG_IGN);
	signal(SIGPROF		, SIG_IGN);
	signal(SIGWINCH		, SIG_IGN);
}

void
EXIT_ttyId(int rc)
{
	if (pty_fd)
		close(pty_fd);
	if ((dev_linked) && (strlen(device_name)))
		unlink(device_name);

	logit(LOG_NOTICE, "ending ttyId (code=%d)", rc);
	exit(rc);
}

void
Usage(void)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "Usage: ttyId OPTION OPTION OPTION [...]\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "-d, --device NAME    Device name to use (eg. /dev/ttyI5).\n");
	fprintf(stderr, "-v, --verbose        Set debug mode on.\n");
	fprintf(stderr, "-h, --help           Displays this help message.\n");
	fprintf(stderr, "\n");

	exit(100);
}


int
main(int argc, char *argv[])
{
	int Opts, i;
	int ret = 0;
	struct timeval Timeout;
	fd_set  FD;
	unsigned char buf[4096];

	*device_name = 0;

	while ((Opts = getopt_long(argc, argv, "d:hv", Arguments, (int *)0)) != EOF)
	{
		switch (Opts)
		{
			case 'v':
				debug = 1;
				break;

			case 'd':
				CopyString(device_name, optarg, sizeof(device_name) - 1);
				break;

			case 'h':
			default:
				Usage();
				break;
		}
	}
	openlog("ttyId", LOG_PID | LOG_NDELAY, LOG_DAEMON);
	logit(LOG_NOTICE, "starting ttyId version %s ...", VERSION);
	logit(LOG_DEBUG, "set device=\"%s\" debug=%d", device_name, debug);

	if (!get_pty(&pty_fd, slave_name)) {
		logit(LOG_ERR, "Unable to get pty master-slave");
		EXIT_ttyId(101);
	}
	logit(LOG_DEBUG, "got pty_fd=%d with slave: %s", pty_fd, slave_name);

	if (strlen(device_name)) {
		if (create_devicelink(slave_name, device_name))
			EXIT_ttyId(103);
		dev_linked = 1;
		logit(LOG_DEBUG, "link created for %s", device_name);
	}

	SetSignals();

	modem_init();

	while(1) {
		FD_ZERO(&FD);
		FD_SET(pty_fd, &FD);

		Timeout.tv_sec  = 10;
		Timeout.tv_usec = 0;

		ret = select(FD_SETSIZE, &FD, NULL, NULL, &Timeout);
		if (ret < 0) {
			logit(LOG_ERR, "main select error %d", errno);
			continue;
		}
		if (!ret) {
			continue;
		}

		i = read(pty_fd, buf, sizeof(buf));
		if (i < 0) {
			/* TODO help me */
			usleep(100);
		}
		if (i > 0) {
			tty_write(buf, i);
			logit(LOG_DEBUG, "got %d byte(s) from slave", i);
		}
		
		

	}

	EXIT_ttyId(0);
	return(0);
}

