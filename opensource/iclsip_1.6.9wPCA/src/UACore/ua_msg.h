/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * ua_msg.h
 *
 * $Id: ua_msg.h,v 1.32 2006/07/13 03:27:06 tyhuang Exp $
 */

#ifndef UA_MSG_H
#define UA_MSG_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "ua_cm.h"
#include "ua_sdp.h"
#include "ua_content.h"
#include "ua_sub.h"

struct uaMsgObj{
	UaDlg		dlg;		/*dialog, e.g. call dialog*/
	UAEvtType	evtType;	/*event type*/
	UACallStateType callstate;	/*call state*/
	UAStatusCode	responsecode;	/*response call .add by tyhuang 2003.10.07 */
	void*		reserved;
	char*		callid;		/*call id*/
	int		cmdseq;		/*command sequence*/
	BOOL		bBody;		/*exist a body or not */
	UaContent	rcontent;	/* message content */
	UaSub		rsubscribe;     /* receive subscription. add by tyhuang.2003.6.9*/	
};

/*dulicate a uaMsg*/
CCLAPI UaMsg uaMsgDup(IN UaMsg);

/*Dialog operation*/
CCLAPI UaDlg uaMsgGetDlg(IN UaMsg);

/*msg information*/
CCLAPI UACallStateType uaMsgGetCallState(IN UaMsg);
CCLAPI UAEvtType uaMsgGetEvtType(IN UaMsg);
/*msg Response status code */
CCLAPI UAStatusCode uaMsgGetResponseCode(IN UaMsg _this);
CCLAPI char*	uaMsgGetCallID(IN UaMsg);
CCLAPI int	uaMsgGetCmdSeq(IN UaMsg);
CCLAPI BOOL	uaMsgContainSDP(IN UaMsg);
/* will return a cpoy of sdp in uamsg */
CCLAPI SdpSess	uaMsgGetSdp(IN UaMsg);
/* add by tyhuang 2003.6.9*/
/* will return a cpoy of content in uamsg */
CCLAPI BOOL	uaMsgContainContent(IN UaMsg);
CCLAPI UaContent uaMsgGetContent(IN UaMsg);

CCLAPI UaSub	 uaMsgGetSub(IN UaMsg);
RCODE  uaMsgSetSubscription(IN UaMsg _this,IN UaSub _sub);

/*input buffer and buffer length to put Contact information*/
/*if get Contact header, return RC_OK*/
/*all contact address is stored at character buffer*/
/*the last parameter is buffer length*/
CCLAPI RCODE uaMsgGetRspContactAddr(IN UaMsg,IN OUT char*, IN OUT int*);

/*get REFER request contain Refer-To header*/
CCLAPI SipReferTo* uaMsgGetReqReferTo(IN UaDlg);
CCLAPI RCODE	   uaMsgGetReqReferToAddr(IN UaDlg, IN OUT char*, IN OUT int*);
/*get REFER request contain Referred-By header*/
CCLAPI SipReferredBy* uaMsgGetReqReferredBy(IN UaDlg);
CCLAPI RCODE	      uaMsgGetReqReferredByAddr(IN UaDlg,IN OUT char*, IN OUT int*);

/*extract information*/
CCLAPI const char* uaMsgGetUser(IN UaMsg);

/*Get remote party address, get from From or To header*/
CCLAPI const char* uaMsgGetRemoteParty(IN UaMsg);

#ifdef CCL_DISABLE_AUTOSEND 
/* get request from msg */
CCLAPI SipReq uaMsgGetRequest(IN UaMsg _this);
/* send response */
CCLAPI RCODE uaMsgSendResponse(IN UaMsg _this, IN UAStatusCode, IN UaContent);
#endif

CCLAPI unsigned char* uaMsgGetSupported(IN UaMsg _this);

/*-----------------------internal function------------------------------*/
/*create a new UA message*/
/* uaMsgNew(UaDlg, CSeq, Sdp)*/
/*CCLAPI UaMsg uaMsgNew(IN UaDlg,IN UAEvtType,IN int cseq,IN SdpSess);*/
UaMsg uaMsgNew(IN UaDlg,IN UAEvtType,IN int,IN UaContent,IN void* reserved);
RCODE uaMsgFree(IN UaMsg);

#ifdef  __cplusplus
}
#endif

#endif /* UA_MSG_H */
