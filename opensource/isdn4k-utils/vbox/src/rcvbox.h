/*
** $Id: rcvbox.h,v 1.3 1997/02/26 13:10:48 michael Exp $
**
** Copyright (C) 1996, 1997 Michael 'Ghandi' Herold
*/

#ifndef _VBOX_RC_H
#define _VBOX_RC_H 1

/** Defines **************************************************************/

#define VBOXRC_MAX_RCLINE	512					/* Max .vboxrc line length	*/
#define VBOXRC_DEF_RINGS	6						/* Default number of rings	*/

/** Prototypes ***********************************************************/

extern void vboxrc_find_user_section(char *);
extern void vboxrc_find_user_from_id(char *);
extern int	vboxrc_get_rings_to_wait(void);

#endif /* _VBOX_RC_H */
