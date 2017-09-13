/*
** $Id: lock.h,v 1.2 1998/08/31 10:43:05 michael Exp $
**
** Copyright 1996-1998 Michael 'Ghandi' Herold <michael@abadonna.mayn.de>
*/

#ifndef _VBOX_LOCK_H
#define _VBOX_LOCK_H 1

/** Prototypes ***********************************************************/

extern int	 lock_create(unsigned char *);
extern int	 lock_remove(unsigned char *);

#endif /* _VBOX_LOCK_H */
