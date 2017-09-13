/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * ua_mgr.h
 *
 * $Id: ua_mgr.h,v 1.47 2004/09/24 05:45:56 tyhuang Exp $
 */

#ifndef UA_MGR_H
#define UA_MGR_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <sip/sip_cm.h>
#include <sip/sip_cfg.h>
#include <sdp/sdp.h>
#include <adt/dx_lst.h>
#include "ua_cm.h"
#include "ua_user.h"
#include "ua_msg.h"

typedef void (*UAEvtCB)(UAEvtType event, void* msg);

/* UA SDK Initialize 
 * input callback function 
 * modified 2003.10.6 tyhuang
 */
CCLAPI RCODE uaLibInit(IN UAEvtCB,IN SipConfigData); 
CCLAPI RCODE uaLibInitWithStun(IN UAEvtCB,IN SipConfigData,IN const char* STUNserver); 

/* clear ua core stack */
CCLAPI RCODE uaLibClean(void);

/* dispatch ua events */
CCLAPI RCODE uaEvtDispatch(const int timeout);

/* Manager */
/*create a new Mgr, similar to create a new device */
CCLAPI UaMgr uaMgrNew(void);	

/*destruct a Mgr object, similar to destruct a old device */
CCLAPI void uaMgrFree(IN UaMgr);

/*get dialog list */
CCLAPI DxLst uaMgrGetDlgLst(IN UaMgr);

/* get event list */
CCLAPI DxLst uaMgrGetEventLst(IN UaMgr);

/*get user object */
CCLAPI UaUser uaMgrGetUser(IN UaMgr);

/* set user setting ,user should free UaUser object 
 * this function will duplicate a uauser for this mgr
 */
CCLAPI RCODE uaMgrSetUser(IN UaMgr, IN UaUser);

/*delete a user object*/
CCLAPI RCODE  uaMgrDelUser(IN UaMgr);

/*get mgr user name,user object user, user->nameid */
CCLAPI char* uaMgrGetUserName(IN UaMgr);

/*Manager-config functions */
/*get config data about this manager */
CCLAPI UaCfg uaMgrGetCfg(IN UaMgr);

/* set config data,user should free UaCfg object 
 * this function will duplicate a uacfg for this mgr
 */
CCLAPI RCODE uaMgrSetCfg(IN UaMgr, IN UaCfg);

/*registration function */
/*register/unregister/query single user to registrar */
CCLAPI RCODE uaMgrRegister(IN UaMgr,IN UARegisterType,IN const char*);

/* perform sending INFO message */
CCLAPI 
RCODE uaMgrInfo(IN UaMgr _this,IN const char* _destination,IN UaContent _content);

/* Manager Dlg */
/*get total dialog number */
CCLAPI int uaMgrGetDlgCount(IN UaMgr);	

/*get a dialog handle */
CCLAPI UaDlg uaMgrGetDlg(IN UaMgr,IN int pos);  

/*get registration dialog*/
CCLAPI UaDlg uaMgrGetRegDlg(IN UaMgr);

/*create a new dialog*/
CCLAPI UaDlg uaMgrAddNewDlg(IN UaMgr);	

/*not destroy a dialog,cut the link to Mgr object */
CCLAPI RCODE uaMgrDelDlgLink(IN UaMgr, IN UaDlg);

/*destory the dialog list, free all dialogs and construct a new dialog list */
CCLAPI RCODE uaMgrDelAllDlg(IN UaMgr);	

/*get current dialog state */
CCLAPI UACallStateType uaMgrGetDlgState(IN UaMgr, IN UaDlg);	

/*make a call directly */
CCLAPI UaDlg uaMgrMakeCall(IN UaMgr, IN const char*, IN SdpSess);

/*command sequence */
CCLAPI int uaMgrGetCmdSeq(IN UaMgr);
CCLAPI RCODE uaMgrAddCmdSeq(IN UaMgr);

/*get Contact address*/
CCLAPI const char* uaMgrGetContactAddr(IN UaMgr);

/*get Public address*/
CCLAPI const char* uaMgrGetPublicAddr(IN UaMgr);

/*get match dialog,input call-id*/
CCLAPI UaDlg uaMgrGetMatchDlg(IN UaMgr,IN const char*);

char* uaMgrGetLastRegisterAddr(IN UaMgr);

/*------------------------internal function--------------------*/
/*response status to application*/
/*CCLAPI void uaMgrPostMsg(IN UaMgr, IN UaMsg);*/
RCODE uaMgrPostNewMsg(IN UaMgr,IN UaDlg,IN UAEvtType,IN int,IN SdpSess);

/* new report function .the same as previous one but with UaContent for msg body */
RCODE uaMgrPostNewMessage(IN UaMgr,IN UaDlg,IN UAEvtType,IN int,IN UaContent,IN void*);

RCODE uaMgrPostNewSubMsg(IN UaMgr ,IN UaDlg ,IN UAEvtType ,IN int ,IN UaContent ,IN UaSub);

char* uaMgrNewPublicSIPS(IN UaMgr );

#ifdef  __cplusplus
}
#endif

#endif /* UA_MGR_H */
