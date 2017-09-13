/*
** $Id: perms.c,v 1.6 1997/05/10 10:58:46 michael Exp $
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

#include "log.h"
#include "perms.h"
#include "libvbox.h"

/*************************************************************************/
/** permissions_set(): Sets file permissions.									**/
/*************************************************************************/

int permissions_set(char *name, uid_t uid, gid_t gid, mode_t mode, mode_t mask)
{
	mode_t realmode = (mode & ~mask);

	log(L_DEBUG, "Setting \"%s\" to %d.%d (%04o)...\n", name, uid, gid, realmode);

	if (chown(name, uid, gid) == -1)
	{
		log(L_ERROR, "Can't set owner of \"%s\" (%s).\n", name, strerror(errno));
		
		returnerror();
	}

	if (chmod(name, realmode) == -1)
	{
		log(L_ERROR, "Can't set mode of \"%s\" (%s).\n", name, strerror(errno));
		
		returnerror();
	}

	returnok();
}

/*************************************************************************/
/** permissions_drop():	Drops user privilegs and sets some environments	**/
/**							for the external commands.								**/
/*************************************************************************/

int permissions_drop(uid_t uid, gid_t gid, char *name, char *home)
{
	log(L_INFO, "Drop permissions to userid %d; groupid %d...\n", uid, gid);

	if (setregid(gid, gid) == 0)
	{
		if (setreuid(uid, uid) == 0)
		{
			if (chdir(home) == 0)
			{
				setenv("USER", name, TRUE);
				setenv("HOME", home, TRUE);

				returnok();
			}
			else log(L_FATAL, "Can't set working directory to \"%s\" (%s).\n", home, strerror(errno));
		}
		else log(L_FATAL, "Can't setreuid() to %d, %d (%s).\n", uid, uid, strerror(errno));
	}
	else log(L_FATAL, "Can't setregid() to %d, %d (%s).\n", gid, gid, strerror(errno));

	returnerror();
}
