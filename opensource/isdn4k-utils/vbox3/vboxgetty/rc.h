/*
** $Id: rc.h,v 1.3 1998/08/31 10:43:10 michael Exp $
**
** Copyright 1996-1998 Michael 'Ghandi' Herold <michael@abadonna.mayn.de>
*/

#ifndef _VBOX_RC_H
#define _VBOX_RC_H 1

/** Defines **************************************************************/

#define VBOX_MAX_RCLINE_SIZE		255

/** Structures ***********************************************************/

struct vboxrc
{
	unsigned char *name;
	unsigned char *value;
};

/** Prototypes ***********************************************************/

extern int				 rc_read(struct vboxrc *, unsigned char *, unsigned char *);
extern void				 rc_free(struct vboxrc *);
extern unsigned char *rc_get_entry(struct vboxrc *, unsigned char *);
extern unsigned char *rc_set_entry(struct vboxrc *, unsigned char *, unsigned char *);
extern unsigned char *rc_set_empty(struct vboxrc *, unsigned char *, unsigned char *);

#endif /* _VBOX_RC_H */
