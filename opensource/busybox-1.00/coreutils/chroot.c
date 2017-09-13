/* vi: set sw=4 ts=4: */
/*
 * Mini chroot implementation for busybox
 *
 * Copyright (C) 1999-2004 by Erik Andersen <andersen@codepoet.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

/* BB_AUDIT SUSv3 N/A -- Matches GNU behavior. */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include "busybox.h"

/*
 * We use chroot to run the upgrade program that updates rootfs image in 
 * the flash. This means we need to kill all programs except the upgrade one
 * Hence need to ignore TERM signal. - Nirav
 */
#ifdef IFX_SMALL_FOOTPRINT
#include <signal.h>
#endif

int chroot_main(int argc, char **argv)
{
	if (argc < 2) {
		bb_show_usage();
	}

	++argv;
	if (chroot(*argv) || (chdir("/"))) {
		bb_perror_msg_and_die("cannot change root directory to %s", *argv);
	}

	++argv;
	if (argc == 2) {
		argv -= 2;
		if (!(*argv = getenv("SHELL"))) {
			*argv = (char *) DEFAULT_SHELL;
		}
		argv[1] = (char *) "-i";
	}

#ifdef IFX_SMALL_FOOTPRINT
	signal(SIGTERM,SIG_IGN);
	signal(SIGHUP,SIG_IGN);	
#endif
	execvp(*argv, argv);
	bb_perror_msg_and_die("cannot execute %s", *argv);
}
