/*
** $Id: breaklist.c,v 1.2 1998/09/18 15:08:56 michael Exp $
**
** Copyright 1996-1998 Michael 'Ghandi' Herold <michael@abadonna.mayn.de>
*/

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "breaklist.h"
#include "log.h"

/** Variables ************************************************************/

unsigned char *breaklist[VBOXBREAK_MAX_ENTRIES];

/************************************************************************* 
 **
 *************************************************************************/

void breaklist_init(void)
{
	int i;

	log(LOG_D, "Initializing breaklist...\n");

	for (i = 0; i < VBOXBREAK_MAX_ENTRIES; i++) breaklist[i] = NULL;
}

/************************************************************************* 
 **
 *************************************************************************/

void breaklist_clear(void)
{
	int i;

	log(LOG_D, "Clearing breaklist...\n");

	for (i = 0; i < VBOXBREAK_MAX_ENTRIES; i++)
	{
		if (breaklist[i]) free(breaklist[i]);
	}

	breaklist_init();
}

/************************************************************************* 
 **
 *************************************************************************/

unsigned char *breaklist_add(unsigned char *seq)
{
	int i;

	log(LOG_D, "Adding \"%s\" to breaklist...\n", seq);

	for (i = 0; i < VBOXBREAK_MAX_ENTRIES; i++)
	{
		if (!breaklist[i])
		{
			if (!(breaklist[i] = strdup(seq)))
			{
				log(LOG_E, "Can't add \"%s\" to breaklist.\n", seq);
			}

			return(breaklist[i]);
		}
	}

	log(LOG_E, "Can't add \"%s\" - breaklist full.\n", seq);

	return(NULL);
}

/************************************************************************* 
 **
 *************************************************************************/

int breaklist_del(unsigned char *seq)
{
	int i;

	log(LOG_D, "Removing \"%s\" from breaklist...\n", seq);

	for (i = 0; i < VBOXBREAK_MAX_ENTRIES; i++)
	{
		if (breaklist[i])
		{
			if (strcmp(seq, breaklist[i]) == 0)
			{
				free(breaklist[i]);

				breaklist[i] = NULL;

				return(0);
			}
		}
	}

	log(LOG_E, "Can't find \"%s\" in breaklist.\n", seq);

	return(-1);
}

/************************************************************************* 
 **
 *************************************************************************/

void breaklist_dump(void)
{
	int i;
	int b;
	int u;

	log(LOG_D, "Dumping breaklist...\n");

	for (i = 0, b = 0, u = 0; i < VBOXBREAK_MAX_ENTRIES; i++)
	{
		if (breaklist[i])
		{
			b++;

			log(LOG_D, "Breaklist: \"%s\"\n", breaklist[i]);
		}
		else u++;
	}

	log(LOG_D, "Breaklist: %d used and %d unused entries.\n", b, u);
}
