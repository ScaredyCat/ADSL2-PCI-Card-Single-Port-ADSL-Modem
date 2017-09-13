/*
** $Id: vboxd.h,v 1.4 1997/05/10 10:58:58 michael Exp $
**
** Copyright (C) 1996, 1997 Michael 'Ghandi' Herold
*/

#ifndef _VBOXD_H
#define _VBOXD_H 1

#include "libvbox.h"               /* Contains server message id defines! */

/** Defines ***************************************************************/

#define VBOXD_LEN_ACCESSLINE     512
#define VBOXD_LEN_CMDLINE        128
#define VBOXD_LEN_ARGLIST        10

#define VBOXD_ACC_NOTHING        0                           /* No access */
#define VBOXD_ACC_COUNT          1            /* Access to count messages */
#define VBOXD_ACC_READ           2             /* Access to read messages */
#define VBOXD_ACC_WRITE          4            /* Access to write messages */

#define VBOXD_ERR_OK             0
#define VBOXD_ERR_TIMEOUT        1
#define VBOXD_ERR_EOF            2
#define VBOXD_ERR_TOOLONG        3

#define pullmsg fflush

/** Structures ************************************************************/

struct servercmds
{
	char *name;
	void (*func)(int, char **);
};

#endif /* _VBOXD_H */
