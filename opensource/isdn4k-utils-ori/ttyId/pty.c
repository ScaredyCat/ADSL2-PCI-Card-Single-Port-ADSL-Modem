/* $Id: pty.c,v 1.1 2000/08/30 18:27:01 armin Exp $
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
 * $Log: pty.c,v $
 * Revision 1.1  2000/08/30 18:27:01  armin
 * Okay, here is the first try for an user-land
 * ttyI daemon. Compilable but not useable.
 *
 *
 */

#include "ttyId.h"
#include "pty.h"

/*
 * get_pty - get a pty master/slave pair and chown the slave side
 * to the uid given.  Assumes slave_name points to >= 16 bytes of space.
 */
int
get_pty(int *master_fdp, char *slave_name)
{
	int i, mfd, sfd = -1;
	char pty_name[16];

#ifdef TIOCGPTN
	/*
	 * Try the unix98 way first.
	 */
	mfd = open("/dev/ptmx", O_RDWR | O_NONBLOCK);
	if (mfd >= 0) {
		int ptn;
		if (ioctl(mfd, TIOCGPTN, &ptn) >= 0) {
			sprintf(pty_name, "/dev/pts/%d", ptn);
			chmod(pty_name, S_IRUSR | S_IWUSR);
#ifdef TIOCSPTLCK
			ptn = 0;
			if (ioctl(mfd, TIOCSPTLCK, &ptn) < 0)
				logit(LOG_ERR,"Couldn't unlock pty slave %s", pty_name);
#endif
			sfd = 1;
		}
	}
#endif /* TIOCGPTN */

	if (sfd < 0) {
		/* the old way - scan through the pty name space */
		for (i = 0; i < 64; ++i) {
			sprintf(pty_name, "/dev/pty%c%x",
				'p' + i / 16, i % 16);
			mfd = open(pty_name, O_RDWR | O_NONBLOCK, 0);
			if (mfd >= 0) {
				pty_name[5] = 't';
				sfd = 1;
				break;
			}
			close(mfd);
		}
	}

	if (sfd < 0)
		return 0;

	strcpy(slave_name, pty_name);
	*master_fdp = mfd;

	return 1;
}

int
create_devicelink(char *old, char *new)
{
	if (symlink(old, new)) {
		logit(LOG_ERR,"Unable to create link to %s", new);
		return 1;
	}
	return 0;
}

int
writepty(unsigned char *buf, int len)
{
	return(write(pty_fd, buf, len));
}
