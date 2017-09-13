/*
** $Id: vbox.h,v 1.2 1997/05/10 10:58:52 michael Exp $
**
** Copyright (C) 1996, 1997 Michael 'Ghandi' Herold
*/

#ifndef _VBOX_H
#define _VBOX_H 1

#include <limits.h>
#include <time.h>
#include <ncurses.h>

#include "libvbox.h"

/** Defines ***************************************************************/

#define VBOX_MAX_PASSWORD   64                 /* Maximal password length */
#define VBOX_MAX_USERNAME   64                 /* Maximal username length */

#define COLTAB     color

/** Structures ************************************************************/

struct messageline
{
	char   messagename[NAME_MAX + 1];
	time_t ctime;
	time_t mtime;
	int    compression;
   int    size;
	char   name[VAH_MAX_NAME + 1];
	char   callerid[VAH_MAX_CALLERID + 1];
	char   phone[VAH_MAX_PHONE + 1];
	char   location[VAH_MAX_LOCATION + 1];
	char   new;
	char   delete;
};

struct colortable
{
	char  *name;
	int    pair;
	int    fg;
	int    bg;
	chtype cattr;
	chtype mattr;
};

struct statusled
{
	int   x;
   char *name;
	int   status;
	char *desc;
};


#endif /* _VBOX_H */
