/*
** $Id: lock.c,v 1.5 1997/05/10 10:58:43 michael Exp $
**
** Copyright (C) 1996, 1997 Michael 'Ghandi' Herold
*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <string.h>

#include "lock.h"
#include "log.h"
#include "init.h"
#include "perms.h"
#include "libvbox.h"

/** Variables ************************************************************/

static int modemfd = -1;
static int gettyfd = -1;

static struct locks locks[] =
{
	{ LCK_MODEM, &modemfd, LCKFILEDIR "/LCK..%s"          , "?"  },
	{ LCK_PID  , &gettyfd, PIDFILEDIR "/vboxgetty-%s.pid" , "?"  },
	{ 0        , NULL    , NULL                           , NULL }
};

/** Prototypes ***********************************************************/

static int  lock_locking(int, int);
static int  lock_unlocking(int, int);
static void lock_init_locknames(void);

/**************************************************************************/
/** lock_init_locknames(): Sets locknames.                               **/
/**************************************************************************/

static void lock_init_locknames(void)
{
	locks[0].desc = "modem port";
	locks[1].desc = "vboxgetty";
}

/*************************************************************************/
/** lock_type_lock(): Create vbox lock.											**/
/*************************************************************************/

int lock_type_lock(int type)
{
	char *name;
	char	temp[32];
	char *device;
	int	i;
	int   size;

	lock_init_locknames();

	if (!(device = rindex(setup.modem.device, '/')))
		device = setup.modem.device;
	else
		device++;
	
	i = 0;
	
	while (TRUE)
	{
		if (locks[i].file == NULL)
		{
			log(L_WARN, "Lock setup for type %d not found.\n", type);

			returnerror();
		}

		if (locks[i].type == type) break;

		i++;
	}

	size = (strlen(locks[i].file) + strlen(device) + 2);
	
	if ((name = (char *)malloc(size)))
	{
		printstring(name, locks[i].file, device);

		log(L_DEBUG, "Locking %s (%s)...\n", locks[i].desc, name);

		if (*(locks[i].fd) == -1)
		{
			if ((*(locks[i].fd) = open(name, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)) != -1)
			{
				if (lock_locking(*(locks[i].fd), 5))
				{
					printstring(temp, "%d\n", getpid());

					write(*(locks[i].fd), temp, strlen(temp));

						/*
						 * Set permissions but make the locks readable to
						 * all (overrides umask).
						 */

					permissions_set(name, setup.users.uid, setup.users.gid, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH, (setup.users.umask & ~(S_IRUSR|S_IRGRP|S_IROTH)));

					free(name);
					returnok();
				}
				else log(L_FATAL, "Can't lock \"%s\".\n", name);
			}
			else log(L_FATAL, "Can't create \"%s\".\n", name);
		}
		else
		{
			log(L_WARN, "Use existing lock for \"%s\" (%d).\n", name, *(locks[i].fd));
					
			free(name);
			returnok();
		}
	}
	else log(L_FATAL, "Not enough memory to allocate lockname.\n");
				
	returnerror();
}

/*************************************************************************/
/** lock_type_unlock():	Deletes vbox lock.										**/
/*************************************************************************/

int lock_type_unlock(int type)
{
	char *name;
	char *device;
	int	i;
	int   size;

	lock_init_locknames();

	if (!(device = rindex(setup.modem.device, '/')))
		device = setup.modem.device;
	else
		device++;
	
	i = 0;
	
	while (TRUE)
	{
		if (locks[i].file == NULL)
		{
			log(L_WARN, "Lock setup for type %d not found.\n", type);

			returnerror();
		}

		if (locks[i].type == type) break;

		i++;
	}

	size = (strlen(locks[i].file) + strlen(device) + 2);
	
	if ((name = (char *)malloc(size)))
	{
		printstring(name, locks[i].file, device);

		log(L_DEBUG, "Unlocking %s (%s)...\n", locks[i].desc, name);

		if (*(locks[i].fd) != -1)
		{
			lock_unlocking(*(locks[i].fd), 5);

			close(*(locks[i].fd));

			*(locks[i].fd) = -1;
		}

		if (unlink(name) != 0)
		{
			log(L_WARN, "Can't remove lock \"%s\".\n", name);
		}

		free(name);

		returnok();
	}
	else log(L_FATAL, "Not enough memory to allocate lockname.\n");
				
	returnerror();
}

/*************************************************************************/
/** lock_locking(): Locks a file descriptor with delay.						**/
/*************************************************************************/

static int lock_locking(int fd, int trys)
{
	while (trys > 0)
	{
		if (flock(fd, LOCK_EX|LOCK_NB) == 0) returnok();
            
		xpause(1000);
                  
		trys--;
	}

	returnerror();
}

/*************************************************************************/
/** lock_unlocking(): Unlocks a file descriptor with delay.				   **/
/*************************************************************************/

static int lock_unlocking(int fd, int trys)
{
	while (trys > 0)
	{
		if (flock(fd, LOCK_UN) == 0) returnok();
            
		xpause(1000);
                  
		trys--;
	}

	returnerror();
}
