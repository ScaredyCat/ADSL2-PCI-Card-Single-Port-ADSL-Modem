/*
** $Id: tclscript.h,v 1.6 1998/09/18 15:09:05 michael Exp $
**
** Copyright 1996-1998 Michael 'Ghandi' Herold <michael@abadonna.mayn.de>
*/

#ifndef _VBOX_TCLSCRIPT_H
#define _VBOX_TCLSCRIPT_H 1

#include <tcl.h>

#include "vboxgetty.h"

/** Structures ***********************************************************/

struct vbox_tcl_function
{
	unsigned char	*name;
	Tcl_ObjCmdProc	*proc;
};

struct vbox_tcl_variable
{
	unsigned char *name;
	unsigned char *args;
};

/** Defines **************************************************************/

#define VBOX_TCLFUNC_PROTO ClientData, Tcl_Interp *, int, Tcl_Obj *CONST []
#define VBOX_TCLFUNC			ClientData data, Tcl_Interp *intp, int objc, Tcl_Obj *CONST objv[]

/** Prototypes ***********************************************************/

extern int				 scr_create_interpreter(void);
extern void				 scr_remove_interpreter(void);
extern int				 scr_execute(unsigned char *, struct vboxuser *);
extern int				 scr_init_variables(struct vbox_tcl_variable *);
extern unsigned char *scr_tcl_version(void);

#endif /* _VBOX_TCLSCRIPT_H */
