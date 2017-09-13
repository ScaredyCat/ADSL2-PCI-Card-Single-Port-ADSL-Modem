/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * ua_class.h
 *
 * $Id: ua_class.h,v 1.3 2005/02/23 05:44:51 tyhuang Exp $
 */

#ifndef UA_CLASS_H
#define UA_CLASS_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <sip/sip.h>
#include "ua_cm.h"
#include "ua_mgr.h"
#include "ua_sub.h"
#include <adt/dx_lst.h>

/* create a class object*/
UaClass uaClassNew(IN const char*);
/* Free a UaClass */
void	uaClassFree(IN UaClass );
/* get class name of a UaClass */
char* uaClassGetName(IN UaClass); 
/*Add a UaSub to in/out list */
RCODE uaClassAddSub(IN UaClass, IN UaSub ,IN BOOL);
/*Delete a UaSub from in/out list */
RCODE uaClassDelSub(IN UaClass, IN UaSub, IN BOOL);
/* get a UaSub from in/out list */
UaSub uaClassGetSub(IN UaClass, IN char*, IN BOOL);
/* get a UaSub from in/out list */
UaSub uaClassGetSubAt(IN UaClass, IN int, IN BOOL);
/* get number of sub in in/out list */
int	uaClassGetNumOfSub(IN UaClass, IN BOOL);

#ifdef  __cplusplus
}
#endif

#endif /* end of UA_CLASS_H */
