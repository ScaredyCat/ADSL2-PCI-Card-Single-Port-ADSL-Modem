/*
** $Id: voice.h,v 1.4 1997/05/10 10:59:02 michael Exp $
**
** Copyright (C) 1996, 1997 Michael 'Ghandi' Herold
*/

#ifndef _VBOX_VOICE_H
#define _VBOX_VOICE_H 1

#include <limits.h>

#define ETX  (0x03)
#define NL   (0x0A)
#define CR   (0x0D)
#define DLE  (0x10)
#define XON  (0x11)
#define XOFF (0x13)
#define DC4  (0x14)
#define CAN  (0x18)

#define VOICE_MAX_MESSAGE		PATH_MAX
#define VOICE_MAX_SCRIPT		PATH_MAX
#define VOICE_MAX_CHECKNEW		PATH_MAX
#define VOICE_MAX_CALLERID		64
#define VOICE_MAX_PHONE			32
#define VOICE_MAX_NAME			64
#define VOICE_MAX_SECTION		64

#define VOICE_MAGIC  "RMD1"
#define VOICE_MODEM  "ZyXEL 1496"
#define VOICE_LISDN  "ISDN4Linux"

#define MIN_COMPRESSION_NUM   2
#define MAX_COMPRESSION_NUM   6

#define ST_NO_INPUT (0x00)
#define ST_GOT_DLE  (0x01)

#define VOICE_ACTION_OK       		(0)
#define VOICE_ACTION_TOUCHTONES		(1)
#define VOICE_ACTION_ERROR				(-1)
#define VOICE_ACTION_TIMEOUT			(-2)
#define VOICE_ACTION_LOCALHANGUP		(-3)
#define VOICE_ACTION_REMOTEHANGUP	(-4)

#define TOUCHTONE_BUFFER_LEN  (64)


typedef struct {
	char  standardmsg[VOICE_MAX_MESSAGE + 1];
	char  beepmsg[VOICE_MAX_MESSAGE + 1];
	char  timeoutmsg[VOICE_MAX_MESSAGE + 1];
	char  tclscriptname[VOICE_MAX_SCRIPT + 1];
	char	callerid[VOICE_MAX_CALLERID + 1];
	char	phone[VOICE_MAX_PHONE + 1];
	char	name[VOICE_MAX_NAME + 1];
	char	section[VOICE_MAX_SECTION + 1];
	char	checknewpath[VOICE_MAX_CHECKNEW + 1];
	int	rings;
	int	ringsonnew;
	int	doanswer;
	int	dorecord;
	int	dobeep;
	int	domessage;
	int	dotimeout;
	int	recordtime;
} voice_t;

            
extern char touchtones[TOUCHTONE_BUFFER_LEN + 1];


extern void voice_init_section(void);
extern void voice_user_section(char *);
extern int	voice_put_message(char *);
extern int	voice_get_message(char *, char *, int);







#endif /* _VBOX_VOICE_H */

