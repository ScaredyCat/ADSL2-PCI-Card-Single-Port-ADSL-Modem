/*
** $Id: voice.h,v 1.9 1998/11/10 18:36:40 michael Exp $
**
** Copyright 1996-1998 Michael 'Ghandi' Herold <michael@abadonna.mayn.de>
*/

#ifndef _VBOX_VOICE_H
#define _VBOX_VOICE_H 1

#ifdef HAVE_CONFIG_H
#	include "../config.h"
#endif

#if TIME_WITH_SYS_TIME
#   include <sys/time.h>
#   include <time.h>
#else
#   if HAVE_SYS_TIME_H
#      include <sys/time.h>
#   else
#      include <time.h>
#   endif
#endif

#include "vboxgetty.h"
#include "vboxrc.h"

/** Defines **************************************************************/

#define VBOXVOICE_BUFSIZE  		64			  /* Voice/Audio Buffer Größe	*/
#define VBOXVOICE_SAMPLERATE		8000		/* Samplerate (nur zum Check)	*/
#define VBOXVOICE_SEQUENCE			64				 /* Touchtone Buffer Größe	*/

#define VBOXVOICE_STAT_OK			0
#define VBOXVOICE_STAT_TIMEOUT	1
#define VBOXVOICE_STAT_HANGUP		2
#define VBOXVOICE_STAT_TOUCHTONE	4
#define VBOXVOICE_STAT_SUSPEND	8
#define VBOXVOICE_STAT_DONE		16

#define ETX  (0x03)
#define NL   (0x0A)
#define CR   (0x0D)
#define DLE  (0x10)
#define XON  (0x11)
#define XOFF (0x13)
#define DC4  (0x14)
#define CAN  (0x18)

#define VBOXSAVE_NAME				64
#define VBOXSAVE_CAID				64
#define VBOXSAVE_VBOX				 6

/** Variables ************************************************************/

extern unsigned char voice_touchtone_sequence[VBOXVOICE_SEQUENCE + 1];

/** Prototypes ***********************************************************/

extern int voice_init(struct vboxuser *, struct vboxcall *);
extern int voice_save(int);
extern int voice_hear(int);
extern int voice_wait(int);
extern int voice_play(unsigned char *);

#endif /* _VBOX_VOICE_H */
