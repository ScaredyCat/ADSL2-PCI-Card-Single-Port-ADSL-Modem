/*
** $Id: init.h,v 1.5 2000/11/30 15:35:20 paul Exp $
**
** Copyright (C) 1996, 1997 Michael 'Ghandi' Herold
*/

#ifndef _VBOX_INIT_H
#define _VBOX_INIT_H 1

#include <unistd.h>
#include <limits.h>
#include <sys/types.h>

#include "modem.h"
#include "voice.h"
#include "streamio.h"
#include "libvbox.h"

/** Defines **************************************************************/

#define SETUP_MAX_SPOOLNAME	(PATH_MAX)
#define SETUP_MAX_LOGNAME		(NAME_MAX)
#define SETUP_MAX_VBOXRC		(PATH_MAX)
#define SETUP_MAX_VBOXCTRL		(PATH_MAX)

#define USER_MAX_HOME			(PATH_MAX)
#define USER_MAX_NAME			(32)

/** Structures ***********************************************************/

typedef struct
{
	char		home[USER_MAX_HOME + 1];
	char		name[USER_MAX_NAME + 1];
	uid_t		uid;
	gid_t		gid;
	mode_t	umask;
} users_t;

typedef struct
{
	modem_t			modem;
	users_t			users;
	voice_t			voice;
	streamio_t	  *vboxrc;
	char				vboxrcname[SETUP_MAX_VBOXRC + 1];
	char				vboxctrl[SETUP_MAX_VBOXCTRL + 1];
	char				spool[SETUP_MAX_SPOOLNAME + 1];
	char				*logname;
	int				freespace;
} setup_t;

/** Variables ************************************************************/

extern setup_t setup;											  /* Global setup	*/

/** Prototypes ***********************************************************/

extern int	init_program(char *, char *);
extern void	exit_program(int);
extern void     exit_program_code(int);
#endif /* _VBOX_INIT_H */
