/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * ua_sub.h
 *
 * $Id: ua_sub.h,v 1.4 2004/12/03 06:48:09 tyhuang Exp $
 */

#ifndef UA_SUB_H
#define UA_SUB_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <sip/sip.h>
#include "ua_cm.h"
#include "ua_mgr.h"

/* create a subscription object*/
/*UaSub uaSubNew(URI,Expires,Sate,Parent,Evtpkg,bBlock,bIn);*/
UaSub	uaSubNew(const char *,unsigned short,UASubState,UaDlg,UaEvtpkg,BOOL,BOOL);

/* Duplicate a uaSub */
UaSub uaSubDup(IN UaSub);

/* Free a uaSub */
void	uaSubFree(IN UaSub);

/* Get uaSub's dialog */
UaDlg	uaSubGetDlg(IN UaSub);

/* Set URI of uaSub */
RCODE	uaSubSetURI(IN UaSub,const char*);

/* get URI of uaSub */
char*	uaSubGetURI(IN UaSub);

/* set expires of uaSub */
RCODE uaSubSetExpires(IN UaSub,unsigned short);

/* get expires of uaSub */
unsigned short uaSubGetExpires(IN UaSub);

/*set state of subscription */
RCODE	uaSubSetSubstate(IN UaSub,IN UASubState ,IN const char*);

/*get state of subscription */
UASubState uaSubGetSubstate(IN UaSub);

/*get reason of a terminated ubscription */
char* uaSubGetReason(IN UaSub);

RCODE uaSubSetReason(IN UaSub,IN const char*);

/*get eventpackage of UaSub */
UaEvtpkg uaSubGetEvtpkg(IN UaSub);

/* check if uaSub is block */
BOOL	isuaSubBlock(IN UaSub);

/* set uaSub block or unblokc*/
RCODE	uaSubSetBlock(IN UaSub,IN BOOL);

/*check if uaSub is in in/out-list */
BOOL	isuaSubIn(IN UaSub);

/*covert subscription state to string */
const char *uaSubStateToStr(IN UASubState);

/*covert subscription state from string */
UASubState uaSubStateFromStr(IN const char* _state);

#ifdef  __cplusplus
}
#endif

#endif /* end of UA_SUB_H */
