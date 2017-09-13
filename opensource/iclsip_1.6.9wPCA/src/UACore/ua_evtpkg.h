/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * ua_evtpkg.h
 *
 * $Id: ua_evtpkg.h,v 1.4 2005/08/17 06:37:11 tyhuang Exp $
 */

#ifndef UA_EVTPKG_H
#define UA_EVTPKG_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <sip/sip.h>
#include "ua_cm.h"
#include "ua_mgr.h"
#include "ua_sub.h"
#include "ua_class.h"
#include <adt/dx_lst.h>

/* create a event package object*/
CCLAPI UaEvtpkg	uaEvtpkgNew(IN const char*);
/* Free a UaClass */
CCLAPI void	uaEvtpkgFree(IN UaEvtpkg );
/* get event name */
CCLAPI char*	uaEvtpkgGetName(IN UaEvtpkg);
/*Add a class to list */
CCLAPI RCODE	uaEvtpkgAddClass(IN UaEvtpkg, IN UaClass);
/*Delete a class from list */
CCLAPI RCODE	uaEvtpkgDelClass(IN UaEvtpkg, IN const char* );
/*get a class from list */
CCLAPI UaClass uaEvtpkgGetClass(IN UaEvtpkg, IN const char* ); 

/*get a class from list */
CCLAPI UaClass uaEvtpkgGetClassByPos(IN UaEvtpkg, IN int); 
/* get number of class */
CCLAPI int	uaEvtpkgGetNumOfClass(IN UaEvtpkg);

/*add a new event package */
CCLAPI RCODE uaMgrInsertEvtpkg(IN UaMgr, IN UaEvtpkg);

/*remote a event package */
CCLAPI RCODE uaMgrRemoveEvtpkg(IN UaMgr, IN const char*);

/*get a event package */
CCLAPI UaEvtpkg uaMgrGetEvtpkg(IN UaMgr, IN const char*);

typedef UASubState (*SubEvtCB)(UaSub sub);

/*Get the subscription state callback */
CCLAPI SubEvtCB uaEvtpkgGetSubCB(IN UaEvtpkg _this);

/*Set the subscription state callback */
CCLAPI RCODE uaEvtpkgSetSubCB(IN UaEvtpkg _this, IN SubEvtCB subCB);

#ifdef  __cplusplus
}
#endif

#endif /* end of UA_EVTPAK_H */
