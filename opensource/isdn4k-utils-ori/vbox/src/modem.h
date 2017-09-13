/*
** $Id: modem.h,v 1.7 2002/01/31 20:08:41 paul Exp $
**
** Copyright (C) 1996, 1997 Michael 'Ghandi' Herold
*/

#ifndef _VBOX_MODEM_H
#define _VBOX_MODEM_H 1

#include <termio.h>
#include <sys/types.h>
#include <unistd.h>

#include "libvbox.h"

/** Defines **************************************************************/

#define MODEM_MIN_S_REGISTER		0			 /* Min. available S-register	*/
#define MODEM_MAX_S_REGISTER		22			 /* Max. available S-register	*/

#define MODEM_TOGGLETIME			800
#define MODEM_RINGTIMEOUT			6
#define MODEM_ECHOTIMEOUT			3
#define MODEM_CMDTIMEOUT			3
#define MODEM_TIMEOUT				1800
#define MODEM_SPEED					38400

#define MODEM_MAX_INITSTRING		128
#define MODEM_MAX_TTYNAME			64
#define MODEM_MAX_RCCMD				64
#define MODEM_MAX_RCARG				256
#define MODEM_MAX_RCLINE			MODEM_MAX_RCCMD + MODEM_MAX_RCARG + 32

#define MODEM_STATE_INITIALIZE	1
#define MODEM_STATE_WAITING		2
#define MODEM_STATE_RING			3
#define MODEM_STATE_CHECK			4
#define MODEM_STATE_EXIT			0

#define MODEM_COMMAND_SUFFIX  	'\r'
#define MODEM_BUFFER_LEN      	256
#define MODEM_INPUT_LEN				MODEM_BUFFER_LEN

typedef struct termios TIO;

/** Structures ***********************************************************/

typedef struct
{
	char	init[MODEM_MAX_INITSTRING + 1];
	char	interninita[MODEM_MAX_INITSTRING + 1];
	char	interninitb[MODEM_MAX_INITSTRING + 1];
	int	toggle_dtr_time;
	int	fd;
	int	badinitsexit;
	int	timeout_ring;
	int	timeout_echo;
	int	timeout_cmd;
	int	timeout_alive;
	int	compression;
	int	initpause;
	char *device;
} modem_t;

/** Prototypes ***********************************************************/

extern int		modem_open_port(void);
extern int		modem_initialize(void);
extern void		modem_close_port(void);
extern int		modem_set_termio(TIO *);
extern int		modem_get_termio(TIO *);
extern int		modem_command(char *, char *);
extern size_t	modem_raw_write(char *, int);
extern int		modem_raw_read(char *, int);
extern void		modem_set_timeout(int);
extern int		modem_get_timeout(void);
extern void		modem_flush(int);
extern int		modem_wait(void);
extern int		modem_count_rings(int);
extern void		modem_set_nocarrier_state(int);
extern int		modem_get_nocarrier_state(void);
extern int		modem_check_input(void);
extern int		modem_hangup(void);
extern char	  *modem_get_s_register(int);
extern int		modem_get_sequence(char *);
extern int		modem_wait_sequence(char *);

#endif /* _VBOX_MODEM_H */
