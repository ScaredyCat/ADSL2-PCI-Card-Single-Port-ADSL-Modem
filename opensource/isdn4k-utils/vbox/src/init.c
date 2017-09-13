/*
** $Id: init.c,v 1.8 2000/11/30 15:35:20 paul Exp $
**
** Copyright (C) 1996, 1997 Michael 'Ghandi' Herold
*/

#include "config.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pwd.h>

#include "init.h"
#include "rcgetty.h"
#include "log.h"
#include "script.h"
#include "perms.h"
#include "modem.h"
#include "lock.h"
#include "streamio.h"
#include "voice.h"
#include "vboxgetty.h"
#include "libvbox.h"

/** Variables ************************************************************/

setup_t setup;										    /* Global setup structure	*/

/*************************************************************************/
/** init_program(): Initialize the globals settings.							**/
/*************************************************************************/

int init_program(char *device, char *gettyrc)
{
	struct passwd *passwd;

	setup.modem.device	= device;
	setup.vboxrc			= NULL;
	setup.vboxrcname[0]	= '\0';
	setup.vboxctrl[0]		= '\0';
	setup.spool[0]			= '\0';
	setup.freespace		= 0;

	/* 
	 * Initialize the log and start the session. The name of the log
	 * is stored into the global setup.
	 */

	if (!log_init()) returnerror();

	log(L_INFO, "-----------------------[Begin session]----------------------\n");
	log(L_INFO, "Running vbox version %s...\n", VERSION);

	/*
	 * Check the version of the tcl interpreter. On bad version only
	 * a warning is displayed.
	 */

	script_check_interpreter();

	/*
	 * Parse vboxgetty.conf. This function will init the most fields
	 * in the global structure.
	 */

	if (!getty_get_settings(gettyrc)) returnerror();

	/*
	 * If the UID or GID is 0 (no user is set) return with error and
	 * exit.
	 */

	if ((setup.users.uid == 0) || (setup.users.gid == 0))
	{
		log(L_FATAL, "You *must* set a user/group (not root)!\n");

		returnerror();
	}

	/*
	 * Get the user settings from /etc/passwd. The name and the home
	 * directory are stored into the global structure.
	 */

	if (!(passwd = getpwuid(setup.users.uid)))
	{
		log(L_FATAL, "Can't get passwd entry for userid %d.\n", setup.users.uid);

		returnerror();
	}

	xstrncpy(setup.users.name, passwd->pw_name, USER_MAX_NAME);
	xstrncpy(setup.users.home, passwd->pw_dir , USER_MAX_HOME);

	if (!*setup.spool)
	{
		xstrncpy(setup.spool, SPOOLDIR		  , SETUP_MAX_SPOOLNAME);
		xstrncat(setup.spool, "/"				  , SETUP_MAX_SPOOLNAME);
		xstrncat(setup.spool, setup.users.name, SETUP_MAX_SPOOLNAME);
	}

	if (!*setup.vboxrcname)
	{
		xstrncpy(setup.vboxrcname, setup.spool , SETUP_MAX_VBOXRC);
		xstrncat(setup.vboxrcname, "/vbox.conf", SETUP_MAX_VBOXRC);
	}

	log(L_INFO, "User %s's messagebox is \"%s\"...\n", setup.users.name, setup.spool);
	log(L_INFO, "User %s's vbox.conf is \"%s\"...\n", setup.users.name, setup.vboxrcname);

	/*
	 * Create the spool directory and set the permissions to the current
	 * user (with umask).
	 */

	if ((mkdir(setup.spool, S_IRWXU) == -1) && (errno != EEXIST))
	{
		log(L_FATAL, "Can't create \"%s\" (%s).\n", setup.spool, strerror(errno));

		returnerror();
	}

	if (!permissions_set(setup.spool, setup.users.uid, setup.users.gid, S_IRWXU|S_IRWXG|S_IRWXO, setup.users.umask))
	{
		returnerror();
	}

	/*
	 * Now we check if 'vboxctrl-stop' exists. If true, loop and watch
	 * if the files is deleted.
	 */

	if (ctrl_ishere(setup.spool, CTRL_NAME_STOP))
	{
		log(L_INFO, "Control file \"%s\" exists - waiting...\n", CTRL_NAME_STOP);

		while (ctrl_ishere(setup.spool, CTRL_NAME_STOP))
		{
			log(L_JUNK, "Control file \"%s\" exists - waiting...\n", CTRL_NAME_STOP);

			xpause(5000);
		}

		log(L_INFO, "Control file deleted - back in business...\n");
	}

	if (ctrl_ishere(setup.spool, CTRL_NAME_ANSWERNOW))
	{
		if (!ctrl_remove(setup.spool, CTRL_NAME_ANSWERNOW))
		{
			log(L_WARN, "Can't remove control file \"%s\"!\n", CTRL_NAME_ANSWERNOW);
		}
	}

	if (ctrl_ishere(setup.spool, CTRL_NAME_REJECT))
	{
		if (!ctrl_remove(setup.spool, CTRL_NAME_REJECT))
		{
			log(L_WARN, "Can't remove control file \"%s\"!\n", CTRL_NAME_REJECT);
		}
	}

	/*
	 * Open the modem device - this *must* done under the rights of
	 * the root user!
	 */

	if (!modem_open_port()) returnerror();

	/*
	 * Lock the modem port and create the pid file. After this the
	 * filepermissions will set to the user, so he can delete this
	 * files if the getty quit.
	 */

	if (!lock_type_lock(LCK_PID  )) returnerror();
	if (!lock_type_lock(LCK_MODEM)) returnerror();

	/*
	 * Drop root privilegs to the current user and set the correct
	 * umask.
	 */

	if (!permissions_drop(setup.users.uid, setup.users.gid, setup.users.name, setup.users.home))
	{
		returnerror();
	}

	umask(setup.users.umask);

	/*
	 * Load vbox's configuration into memory and initialize the voice
	 * defaults.
	 */

	if (!(setup.vboxrc = streamio_open(setup.vboxrcname)))
	{
		log(L_FATAL, "Can't open \"%s\".\n", setup.vboxrcname);

		returnerror();
	}

	voice_init_section();

	/*
	 * Now the complete global setup structure is filled and can be
	 * used.
	 */

	returnok();
}

/*************************************************************************/
/** exit_program(): Exit program.													**/
/*************************************************************************/

void exit_program(int s)
{
	block_all_signals();

	log(L_INFO, "Exit program on signal %d...\n", s);

	modem_close_port();
	streamio_close(setup.vboxrc);
	lock_type_unlock(LCK_PID);
	lock_type_unlock(LCK_MODEM);

	log(L_INFO, "------------------------[End session]-----------------------\n");

	log_exit();

	exit(0);
}

void exit_program_code(int s)
{
	block_all_signals();

	log(L_INFO, "Exit program with value %d...\n", s);

	modem_close_port();
	streamio_close(setup.vboxrc);
	lock_type_unlock(LCK_PID);
	lock_type_unlock(LCK_MODEM);

	log(L_INFO, "------------------------[End session]-----------------------\n");

	log_exit();

	exit(s);
}
