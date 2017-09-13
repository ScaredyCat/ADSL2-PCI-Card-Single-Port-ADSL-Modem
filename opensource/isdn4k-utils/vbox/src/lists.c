/*
** $Id: lists.c,v 1.5 2000/11/30 15:35:20 paul Exp $
**
** Copyright (C) 1996, 1997 Michael 'Ghandi' Herold
*/

#include <stdlib.h>
#include <string.h>

#include "lists.h"
#include "libvbox.h"

/** Variables *************************************************************/

static char *breaklist[VBOX_MAX_BREAKLIST];

/**************************************************************************/
/** breaklist_init(): Initialize the breaklist.                          **/
/**************************************************************************/

void breaklist_init(void)
{
	int i;

	for (i = 0; i < VBOX_MAX_BREAKLIST; i++) breaklist[i] = NULL;
}

/**************************************************************************/
/** breaklist_exit(): Frees all breaklist resources.                     **/
/**************************************************************************/

void breaklist_exit(void)
{
	int i;

	for (i = 0; i < VBOX_MAX_BREAKLIST; i++)
	{
		if (breaklist[i]) free(breaklist[i]);
	}

	breaklist_init();
}

/**************************************************************************/
/** breaklist_add(): Adds one entry to the breaklist.                    **/
/**************************************************************************/

int breaklist_add(char *line)
{
	int i;

	if ((line) && (*line))
	{
		for (i = 0; i < VBOX_MAX_BREAKLIST; i++)
		{
			if (!breaklist[i])
			{
				breaklist[i] = strdup(line);

				if (breaklist[i]) returnok();
			}
		}
	}

	returnerror();
}

/**************************************************************************/
/** breaklist_rem(): Removes entry from the breaklist (all matches).     **/
/**************************************************************************/

void breaklist_rem(char *line)
{
	int i;

	if ((line) && (*line))
	{
		for (i = 0; i < VBOX_MAX_BREAKLIST; i++)
		{
			if (breaklist[i])
			{
				if (strcasecmp(breaklist[i], line) == 0)
				{
					free(breaklist[i]);

					breaklist[i] = NULL;
				}
			}
		}
	}
}

/**************************************************************************/
/** breaklist_search(): Searchs entry in the breaklist.                  **/
/**************************************************************************/

int breaklist_search(char *line)
{
	int i;

	if ((line) && (*line))
	{
		for (i = 0; i < VBOX_MAX_BREAKLIST; i++)
		{
			if (breaklist[i])
			{
				if (strcasecmp(breaklist[i], line) == 0) returnok();
				if ((strcmp(breaklist[i], "ALL") == 0) && (strlen(line) > 3)) returnok();
			}
		}
	}

	returnerror();
}

/**************************************************************************/
/**   **/
/**************************************************************************/

char *breaklist_nr(int nr)
{
	if ((nr >= 0) && (nr < VBOX_MAX_BREAKLIST)) return(breaklist[nr]);

	return(NULL);
}
