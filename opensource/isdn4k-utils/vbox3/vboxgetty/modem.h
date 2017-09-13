/*
** $Id: modem.h,v 1.3 1998/08/31 10:43:09 michael Exp $
**
** Copyright 1996-1998 Michael 'Ghandi' Herold <michael@abadonna.mayn.de>
*/

#ifndef _VBOX_MODEM_H
#define _VBOX_MODEM_H 1

#include "libvboxmodem.h"

/** Defines **************************************************************/

#define VBOXMODEM_STAT_INIT		0
#define VBOXMODEM_STAT_WAIT		1
#define VBOXMODEM_STAT_RING		2
#define VBOXMODEM_STAT_TEST		3
#define VBOXMODEM_STAT_EXIT		255

/** Structures ***********************************************************/

struct modemsetup
{
	int echotimeout;
	int commandtimeout;
	int ringtimeout;
	int alivetimeout;
	int toggle_dtr_time;
};

extern struct modemsetup modemsetup;

/** Prototypes ***********************************************************/

extern void modem_set_timeout(int);
extern int	modem_get_timeout(void);
extern int	modem_get_sequence(struct vboxmodem *, unsigned char *);
extern void modem_flush(struct vboxmodem *, int);
extern int	modem_command(struct vboxmodem *, unsigned char *, unsigned char *);
extern int	modem_hangup(struct vboxmodem *);
extern int	modem_wait(struct vboxmodem *);
extern void modem_set_nocarrier(struct vboxmodem *, int);
extern int	modem_get_nocarrier(struct vboxmodem *);
extern int	modem_read(struct vboxmodem *, unsigned char *, int);

#endif /* _VBOX_MODEM_H */
