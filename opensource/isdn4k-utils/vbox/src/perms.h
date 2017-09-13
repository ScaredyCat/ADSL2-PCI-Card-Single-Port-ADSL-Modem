/*
** $Id: perms.h,v 1.3 1997/02/26 13:10:46 michael Exp $
**
** Copyright (C) 1996, 1997 Michael 'Ghandi' Herold
*/

#ifndef _VBOX_PERMS_H
#define _VBOX_PERMS_H 1

#include <sys/types.h>
#include <unistd.h>

/** Prototypes ***********************************************************/

extern int permissions_set(char *, uid_t, gid_t, mode_t, mode_t);
extern int permissions_drop(uid_t, gid_t, char *, char *);

#endif /* _VBOX_PERMS_H */
