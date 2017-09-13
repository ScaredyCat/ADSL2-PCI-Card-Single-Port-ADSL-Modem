/*
** $Id: control.c,v 1.4 1998/09/18 15:08:57 michael Exp $
**
** Copyright 1996-1998 Michael 'Ghandi' Herold <michael@abadonna.mayn.de>
*/

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>

#include "control.h"
#include "stringutils.h"
#include "log.h"

/** Variablen ************************************************************/

static unsigned char ctrlpathname[PATH_MAX + 1];
static unsigned char ctrllastline[VBOX_CTRL_MAX_RCLINE + 1];

/*************************************************************************/
/** ctrl_exists():	Untersucht ob eine Kontrolldatei existiert und		**/
/**						gibt deren Inhalt zurück.									**/
/*************************************************************************/
/** => home				Homeverzeichnis des Benutzers.							**/
/** => name				Name der Kontrolldatei (ohne vboxctrl-).				**/
/**																							**/
/** <=					NULL wenn die Datei nicht gelesen werden konnte		**/
/**						oder ein Zeiger auf deren Initstring.					**/
/*************************************************************************/

char *ctrl_exists(unsigned char *home, unsigned char *name, unsigned char *ttyd)
{
	FILE *cptr;
	char *stop;

	if (!ttyd)
		printstring(ctrlpathname, "%s/vboxctrl-%s", home, name);
	else
		printstring(ctrlpathname, "%s/vboxctrl-%s-%s", home, name, ttyd);

	if ((cptr = fopen(ctrlpathname, "r")))
	{
		if (fgets(ctrllastline, VBOX_CTRL_MAX_RCLINE, cptr))
		{
			ctrllastline[strlen(ctrllastline) - 1] = '\0';

			if ((stop = index(ctrllastline, '\r'))) *stop = '\0';

			fclose(cptr);
			return(ctrllastline);
		}

		fclose(cptr);
	}

	return(NULL);
}

/*************************************************************************/
/** ctrl_create():	Erzeugt eine Kontrolldatei und schreibt den Init-	**/
/**						string.															**/
/*************************************************************************/
/** => home				Homeverzeichnis des Benutzers.							**/
/** => name				Name der Kontrolldatei (ohne vboxctrl-).				**/
/** => init				Initstring.														**/
/**																							**/
/** <=					0 wenn die Datei erzeugt werden konnte oder -1 bei	**/
/**						einem Fehler.													**/
/*************************************************************************/

int ctrl_create(unsigned char *home, unsigned char *name, unsigned char *ttyd, unsigned char *init)
{
	FILE *cptr = NULL;
	int	loop = 5;

	if (!ttyd)
		printstring(ctrlpathname, "%s/vboxctrl-%s", home, name);
	else
		printstring(ctrlpathname, "%s/vboxctrl-%s-%s", home, name, ttyd);

	while (loop > 0)
	{
		log_line(LOG_D, "Creating control \"vboxctrl-%s:%s\" (%s)...\n", name, init, (char *)ttyd ? (char *)ttyd : "global");

		if ((cptr = fopen(ctrlpathname, "w")))
		{
			fprintf(cptr, "%s\n", init);
			fclose(cptr);
			return(0);
		}

		usleep(500);
		
		loop--;
	}

	log_line(LOG_E, "Can't create \"%s\".\n", ctrlpathname);

	return(-1);
}

/*************************************************************************/
/** ctrl_remove():	Löscht eine Kontrolldatei.									**/
/*************************************************************************/
/** => home				Homeverzeichnis des Benutzers.							**/
/** => name				Name der Kontrolldatei (ohne vboxctrl-).				**/
/**																							**/
/** <=					0 wenn die Datei gelöscht werden konnte oder -1		**/
/**						bei einem Fehler.												**/
/*************************************************************************/

int ctrl_remove(unsigned char *home, unsigned char *name, unsigned char *ttyd)
{
	int loop = 5;

	if (!ttyd)
		printstring(ctrlpathname, "%s/vboxctrl-%s", home, name);
	else
		printstring(ctrlpathname, "%s/vboxctrl-%s-%s", home, name, ttyd);

	while (loop > 0)
	{
		log_line(LOG_D, "Removing control \"vboxctrl-%s\" (%s)...\n", name, (char *)ttyd ? (char *)ttyd : "global");

		if (remove(ctrlpathname) == 0) return(0);

		if (errno == ENOENT) return(0);

		usleep(500);

		loop--;
	}	

	log_line(LOG_E, "Can't remove \"%s\".\n", ctrlpathname);

	return(-1);
}
