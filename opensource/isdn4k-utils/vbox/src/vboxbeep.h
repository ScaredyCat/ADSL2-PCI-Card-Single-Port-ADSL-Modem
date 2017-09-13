/*
** $Id: vboxbeep.h,v 1.2 1997/05/10 10:58:53 michael Exp $
**
** Copyright (C) 1996, 1997 Michael 'Ghandi' Herold
*/

#ifndef _VBOX_BEEP_H
#define _VBOX_BEEP_H 1

/** Defines ***************************************************************/

#define MAX_MESSAGE_BOXES	10      /* Max. message boxes watch at on time */

/** Struktures ************************************************************/

struct messagebox
{
	char		*name;
	time_t	 time;
};

#endif /* _VBOX_BEEP_H */
