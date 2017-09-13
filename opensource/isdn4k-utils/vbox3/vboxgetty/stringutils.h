/*
** $Id: stringutils.h,v 1.5 1998/11/10 18:36:33 michael Exp $
**
** Copyright 1996-1998 Michael 'Ghandi' Herold <michael@abadonna.mayn.de>
*/

#ifndef _VBOX_STRINGUTILS_H
#define _VBOX_STRINGUTILS_H 1

/** Defines **************************************************************/

#define printstring	sprintf

/** Prototypes ***********************************************************/

extern unsigned char *xstrtoupper(unsigned char *);
extern void xstrncpy(unsigned char *, unsigned char *, int);
extern void xstrncat(unsigned char *, unsigned char *, int);
extern long xstrtol(unsigned char *, long);
extern long xstrtoo(unsigned char *, long);

#endif /* _VBOX_STRINGUTILS_H */
