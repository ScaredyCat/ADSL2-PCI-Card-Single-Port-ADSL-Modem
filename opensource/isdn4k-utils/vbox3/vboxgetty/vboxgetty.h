/*
** $Id: vboxgetty.h,v 1.5 1998/09/18 15:09:08 michael Exp $
**
** Copyright 1996-1998 Michael 'Ghandi' Herold <michael@abadonna.mayn.de>
*/

#ifndef _VBOXGETTY_H
#define _VBOXGETTY_H 1

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>

/** Defines **************************************************************/

#define VBOX_ROOT_UMASK		"0022"

#define VBOXUSER_CALLID		64
#define VBOXUSER_NUMBER		64
#define VBOXUSER_USERNAME	64

/** Structures ***********************************************************/

struct vboxuser
{
	uid_t				uid;
	gid_t				gid;
	int				umask;
	long				space;
	unsigned char	incomingid[VBOXUSER_CALLID + 1];
	unsigned char	localphone[VBOXUSER_NUMBER + 1];
	unsigned char	name[VBOXUSER_USERNAME + 1];
	unsigned char	home[PATH_MAX + 1];
};

extern struct vboxmodem vboxmodem;

/** Variables ************************************************************/

extern unsigned char temppathname[PATH_MAX + 1];
extern unsigned char savettydname[NAME_MAX + 1];

/** Prototypes ***********************************************************/

extern void	 quit_program(int);
extern int	 set_process_permissions(uid_t, gid_t, int);

#endif /* _VBOXGETTY_H */
