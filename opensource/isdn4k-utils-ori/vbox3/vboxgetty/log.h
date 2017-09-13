/*
** $Id: log.h,v 1.4 1998/08/31 10:43:06 michael Exp $
**
** Copyright 1996-1998 Michael 'Ghandi' Herold <michael@abadonna.mayn.de>
*/

#ifndef _VBOX_LOG_H
#define _VBOX_LOG_H 1

/** Defines **************************************************************/

#define LOG_E	(0)														  /* Errors	*/
#define LOG_W	(1)														/* Warnings	*/
#define LOG_I	(2)												  /* Informations	*/
#define LOG_A	(4)														  /* Action	*/
#define LOG_D	(128)															/* Debug	*/
#define LOG_X	(255)													 /* Full debug	*/

#define log log_line										 /* It looks better :-)	*/

/** Structures ***********************************************************/

struct logsequence
{
	unsigned char  code;
	unsigned char *text;
};

/** Prototypes ***********************************************************/

extern int	log_open(unsigned char *);
extern void	log_set_debuglevel(int);
extern void log_close(void);
extern void log_line(int, unsigned char *, ...);
extern void log_char(int, unsigned char);
extern void log_text(int, unsigned char *, ...);
extern void log_code(int, unsigned char *);

#endif /* _VBOX_LOG_H */
