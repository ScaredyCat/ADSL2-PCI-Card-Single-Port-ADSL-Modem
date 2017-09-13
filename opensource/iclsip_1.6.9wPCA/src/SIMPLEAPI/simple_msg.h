/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * simple_msg.h
 *
 * $Id: simple_msg.h,v 1.9 2005/12/06 01:02:55 tyhuang Exp $
 */

#ifndef SIMPLE_MSG_H
#define SIMPLE_MSG_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <sip/sip.h>
#include <uacore/ua_cm.h>
#include <uacore/ua_mgr.h>
#include <uacore/ua_content.h>
#include <siptx/sipTx.h>

/* accept header for subscribe message */
#define SIMPLE_Accept_Type "application/pidf+xml,application/pidf-diff+xml"
/*Subscribe message*/
/* (dialog,destination,event type) */
SipReq		uaSUBSCRIBEMsg(IN UaDlg, IN const char* ,IN const char*,IN int expires,IN UaContent);

SipReq 		uaSUBSCRIBEMsgWithEventList(IN UaDlg, IN const char* ,IN const char*,IN int expires,IN UaContent);

/* Publish message */
/* dlg,destination,event,class,content */
SipReq		uaPUBLISHMsg(IN UaDlg, IN const char*, IN const char* ,IN const char*, IN UaContent, IN int);

/* Messgae message */
/* Message(dialog,destination,content-type,content) */
SipReq		uaMESSAGEMsg(IN UaDlg, IN const char* , IN UaContent);

/*NOTIFY message*/
SipReq uaSubNOTIFYMsg(IN UaDlg,IN UaSub,IN const char*,IN UaContent);

#ifdef  __cplusplus
}
#endif

#endif /* SIMPLE_MSG_H */
