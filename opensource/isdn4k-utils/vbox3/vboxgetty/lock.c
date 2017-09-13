/*
** $Id: lock.c,v 1.4 1998/11/10 18:36:27 michael Exp $
**
** Copyright 1996-1998 Michael 'Ghandi' Herold <michael@abadonna.mayn.de>
*/

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>

#include "lock.h"
#include "log.h"
#include "stringutils.h"

/*************************************************************************/
/** lock_create():	Erzeugt einen Lock.											**/
/*************************************************************************/
/** => name				Name der Lockdatei.											**/
/**																							**/
/** <=					0 wenn der Lock erzeugt werden konnte oder -1 wenn	**/
/**						nicht.															**/
/*************************************************************************/

int lock_create(unsigned char *name)
{
	unsigned char  line[32];
	FILE          *lptr;
	long	         lock;
	int	         loop;

	log_line(LOG_D, "Creating lock \"%s\"...\n", name);

	lock = -1;
	loop =  5;

		/* Prüfen ob der Lock bereits existiert. Wenn ja wird die	*/
		/* PID eingelesen.														*/

	if (access(name, F_OK) == 0)
	{
		if ((lptr = fopen(name, "r")))
		{
			if (fgets(line, 32, lptr))
			{
				line[strlen(line) - 1] = '\0';

				lock = xstrtol(line, -1);
			}

			fclose(lptr);
		}
		else
		{
			log_line(LOG_E, "Lock exists but can not be opened.\n");
			
			return(-1);
		}
	}

		/* Wenn der Lock existiert, wird ein Signal 0 an den ent-	*/
		/* sprechenden Prozeß geschickt um herauszufinden ob dieser	*/
		/* noch existiert.														*/

	if (lock > 1)
	{
		if (kill(lock, 0) == 0)
		{
			log_line(LOG_E, "Lock exists - pid %ld is running.\n", lock);
			
			return(-1);
		}
		else log_line(LOG_D, "Lock exists - pid %ld not running.\n", lock);
	}

		/* Der Lock existierte nicht, die Datei wird neu ange-	*/
		/* legt.																	*/

	while (loop > 0)
	{
		if ((lptr = fopen(name, "w")))
		{
			fprintf(lptr, "%010d\n", getpid());
			fclose(lptr);
			return(0);
		}

		usleep(500);
		
		loop--;
	}

	log_line(LOG_E, "Can't create lock \"%s\".\n", name);
	
	return(-1);
}

/*************************************************************************/
/** lock_remove():	Entfernt einen Lock.											**/
/*************************************************************************/
/** => name				Name der Lockdatei.											**/
/**																							**/
/** <=					0 wenn der Lock entfernt werden konnte oder -1		**/
/**						wenn nicht.														**/
/*************************************************************************/

int lock_remove(unsigned char *name)
{
	int loop = 5;

	log_line(LOG_D, "Removing lock \"%s\"...\n", name);

	while (loop > 0)
	{
		if (remove(name) == 0) return(0);

		if (errno == ENOENT) return(0);

		usleep(500);
		
		loop--;
	}

	log_line(LOG_E, "Can't remove lock \"%s\".\n", name);

	return(-1);
}
