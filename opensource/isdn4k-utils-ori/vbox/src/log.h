/*
** $Id: log.h,v 1.4 1997/10/22 20:47:09 fritz Exp $
**
** Copyright (C) 1996, 1997 Michael 'Ghandi' Herold
*/
 
#ifndef _VBOX_LOG_H
#define _VBOX_LOG_H 1

#include "libvbox.h"
#include "init.h"

/** Defines **************************************************************/

#define LOG_MAX_LOGNAME	(256)

#define L_FATAL			(0)
#define L_ERROR			(1)
#define L_WARN				(2)
#define L_INFO				(4)
#define L_DEBUG			(8)
#define L_JUNK				(16)
#define L_STDERR			(32)
#define L_DEFAULT			(L_FATAL|L_ERROR|L_WARN|L_INFO|L_STDERR)

#define log					log_line

/** Structures ***********************************************************/

struct logsequence
{
	char	code;
	char *text;
};

/** Prototypes ***********************************************************/

extern int	 log_init(void);
extern void	 log_exit(void);
extern void	 log_debuglevel(long);
extern void	 log_char(long, char);
extern void	 log_code(long, char *);
extern void	 log_text(long, char *, ...);
extern void	 log_line(long, char *, ...);

#endif /* _VBOX_LOG_H */
