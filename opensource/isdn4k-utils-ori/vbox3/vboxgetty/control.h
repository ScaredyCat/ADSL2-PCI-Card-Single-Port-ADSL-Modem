/*
** $Id: control.h,v 1.3 1998/08/31 10:43:02 michael Exp $
**
** Copyright 1996-1998 Michael 'Ghandi' Herold <michael@abadonna.mayn.de>
*/

#ifndef _VBOX_CONTROL_H
#define _VBOX_CONTROL_H 1

/** Defines **************************************************************/

#define VBOX_CTRL_MAX_RCLINE	64

/** Prototypes ***********************************************************/

extern char *ctrl_exists(unsigned char *, unsigned char *, unsigned char *);
extern int	 ctrl_create(unsigned char *, unsigned char *, unsigned char *, unsigned char *);
extern int	 ctrl_remove(unsigned char *, unsigned char *, unsigned char *);

#endif /* _VBOX_CONTROL_H */
