/*
** $Id: lists.h,v 1.4 1997/04/28 16:51:58 michael Exp $
**
** Copyright (C) 1996, 1997 Michael 'Ghandi' Herold
*/

#ifndef _VBOX_LISTS_H
#define _VBOX_LISTS_H 1

/** Defines **************************************************************/

#define VBOX_MAX_BREAKLIST  128

/** Prototypes ***********************************************************/

extern char *breaklist_nr(int);
extern void  breaklist_init(void);
extern void  breaklist_exit(void);
extern void  breaklist_rem(char *);
extern int   breaklist_add(char *);
extern int   breaklist_search(char *);

#endif /* _VBOX_LISTS_H */
