/*
** $Id: libvboxmodem.h,v 1.3 1998/08/31 10:43:04 michael Exp $
**
** Copyright 1996-1998 Michael 'Ghandi' Herold <michael@abadonna.mayn.de>
*/

#ifndef _VBOX_LIBMODEM_H
#define _VBOX_LIBMODEM_H 1

#include <unistd.h>
#include <errno.h>

/** Defines **************************************************************/

#define VBOXMODEM_BUFFER_SIZE	255				/* Modem input buffer size	*/

/** Variables ************************************************************/

typedef struct termios TIO;

struct vboxmodem
{
	int				fd;
	unsigned char *devicename;
	unsigned char *input;
	int				inputpos;
	int				inputlen;
	int				nocarrier;
	int				nocarrierpos;
	unsigned char *nocarriertxt;
};

/** Internal junk ********************************************************/

#define set_modem_error(A)		strcpy(lastmodemerrmsg, A)

/** Prototypes ***********************************************************/

extern int				 vboxmodem_open(struct vboxmodem *, unsigned char *);
extern int				 vboxmodem_close(struct vboxmodem *);
extern unsigned char *vboxmodem_error(void);
extern int				 vboxmodem_raw_read(struct vboxmodem *, unsigned char *, int);
extern size_t			 vboxmodem_raw_write(struct vboxmodem *, unsigned char *, int);
extern int				 vboxmodem_set_termio(struct vboxmodem *, TIO *);
extern int				 vboxmodem_get_termio(struct vboxmodem *, TIO *);

#endif /* _VBOX_LIBMODEM_H */
