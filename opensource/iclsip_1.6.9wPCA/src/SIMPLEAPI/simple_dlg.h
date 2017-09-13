/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * simple_dlg.h
 *
 * $Id: simple_dlg.h,v 1.8 2005/02/14 08:44:51 ljchuang Exp $
 */

#ifndef SIMPLE_DLG_H
#define SIMPLE_DLG_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <uacore/ua_cm.h>
#include <sdp/sdp.h>
#include <adt/dx_lst.h>
#include <siptx/sipTx.h>
#include <uacore/ua_sub.h>
#include <uacore/ua_evtpkg.h>

/* subscribe */
/* (dialog,subscription) */
CCLAPI RCODE uaDlgSubscribe(IN UaDlg, IN UaSub,IN const char *);

//add by alan
/* subscribe */
/* (dialog,subscription) */
CCLAPI RCODE uaDlgSubscribeWithEventList(IN UaDlg, IN UaSub,IN const char *);

/* Message */
/* (dialog,destination,content) */
CCLAPI	RCODE uaDlgMessage(IN UaDlg, IN const char*, IN UaContent);

/* Notify */
/* (dialog,destination,content) */
CCLAPI	RCODE uaSubNotify(IN UaSub, IN const char*, IN UaContent);

/* publish */
CCLAPI RCODE uaDlgPublish(IN UaDlg, IN const char*,IN const char*,IN const char*,IN UaContent, IN int );
/* send a sip request
 * user should make sure the request is valid
 */
CCLAPI RCODE uaDlgSendRequest(IN UaDlg _dlg, IN SipReq _req);
#ifdef  __cplusplus
}
#endif

#endif /* SIMPLE_DLG_H */
