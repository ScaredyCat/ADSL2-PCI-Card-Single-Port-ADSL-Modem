/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * ua_sipmsg.h
 *
 * $Id: ua_sipmsg.h,v 1.56 2006/07/13 03:27:05 tyhuang Exp $
 */

#ifndef UA_SIPMSG_H
#define UA_SIPMSG_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <sip/sip.h>
#include "ua_cm.h"
#include "ua_mgr.h"
#include "ua_content.h"
#include <sipTx/sipTx.h>

/*construct  a new sip message */
/*input a dialog, SIP URI destination,refer-by, sdp */
SipReq		uaINVITEMsg(IN UaDlg,IN const char* ,IN const char*,IN SdpSess );
SipReq		uaCANCELMsg(IN UaDlg,IN SipReq,IN SipRsp);
SipReq		uaBYEMsg(IN UaDlg,IN UaContent);/* modified 2003.11.24*/
SipReq		uaACKMsg(IN UaDlg,IN TxStruct,IN SdpSess);
SipReq		uaACKMsgEx(IN UaDlg dlg,IN SdpSess _sdp);

/*reInvite message: when changed SDP or Hold */
SipReq		uaReINVITEMsg(IN UaDlg, IN SdpSess);

/*REFER message: when transfer*/
/* input dialog, req-URI(NULL-->transfer), Refer-To  */
SipReq		uaREFERMsg(IN UaDlg,IN const char*, IN const char*); 

/*NOTIFY message*/
SipReq		uaNOTIFYMsg(IN UaDlg, IN const char*,IN const char*,IN UaContent);

/*input original register dialog and txStruct, and original sipReq, and register type */
SipReq		uaREGMsg(IN UaDlg,IN TxStruct,IN SipReq,IN UARegisterType,IN const char*);

/*input original register dialog and txStruct, and original sipReq and final 401 response*/
SipReq		uaRegAuthnMsg(IN UaDlg, IN TxStruct, IN SipReq, IN SipRsp);

/*create OPTION message, not implement yet*/
SipReq		uaOPTMsg(IN UaDlg);

/* create sip INFO message */
SipReq		uaINFOMsg(IN UaDlg,IN const char* , IN UaContent );

/* create sip UPDATE message */
SipReq		uaUPDATEMsg(IN UaDlg,IN const char* , IN UaContent );

/* create sip PRACK message */
SipReq		uaPRACKMsg(IN UaDlg,IN const char* , IN UaContent ,IN unsigned int);
/*Check Request message contain Replaces or not*/
UaDlg		uaReqCheckReplaceMsg(IN UaMgr,IN SipReq);

/*create response message from request message */
/*input request message and response code,contact address(can be NULL),SDP(can be NULL) */
SipRsp		uaCreateRspFromReq(IN SipReq,IN UAStatusCode,IN char*,IN SdpSess);
SipRsp		uaCreateRspMsg(IN SipReq ,IN UAStatusCode ,IN const char* , IN UaContent);
SipRsp		uaCreateRspFromReqEx(IN SipReq,IN UAStatusCode,IN char*,IN SdpSess,IN unsigned int _SE);
SipRsp		uaCreateRspMsgEx(IN SipReq ,IN UAStatusCode ,IN const char* , IN UaContent,IN unsigned int _SE);

/*message body, SipBody can be NULL or not*/
RCODE		uaReqAddBody(IN SipReq,IN SipBody*,IN char*);
RCODE		uaRspAddBody(IN SipRsp,IN SipBody*,IN char*);

/*duplicate From header and add tag */
SipFrom*	uaFromDupAddTag(IN SipFrom *);
RCODE		uaFromHdrFree(IN SipFrom*);

/*duplicate To header and add tag */
SipTo*		uaToDupAddTag(IN SipTo *);
RCODE		uaToHdrFree(IN SipTo*);

/*Record-Route header transfer into Route header*/
SipRoute*	uaRecordRouteCovertToRoute(IN SipRecordRoute*);
RCODE		uaRouteFree(IN SipRoute*);

/*Create a new refer-to message*/
/*input the 2nd dialg to be replaced*/
RCODE		uaCreateReferToMsg(IN UaDlg,IN char*,IN OUT int*);

/*Refer-To header operation*/
/*copy address into buffer*/
RCODE		uaReferToGetAddr(SipReferTo*,char*,int*);
char*		uaReferToGetReplaceID(SipReferTo*);
char*		uaReferToGetToTag(SipReferTo*);
char*		uaReferToGetFromTag(SipReferTo*);

/*transfer contact address into Request-URI */
RCODE		uaContactAsReqURI(IN SipContact*,IN OUT char*,IN OUT int,IN OUT int*);


/*sip related functions */
void		uaAddrFree(IN SipAddr *);
SipAddr*	uaAddrDup(IN SipAddr* ,int );
int		uaAddrAsStr(IN SipAddr *, IN OUT unsigned char *,IN int ,IN OUT int *);
SipParam*	uaParameterDupAddTag(IN SipAddr* ,IN SipParam* ,IN int ,IN OUT int *);
void		uaParameterFree(IN SipParam* );

/*sip Request message processing*/
SipMethodType	uaGetReqMethod(SipReq);
int		uaGetReqCSeq(SipReq);
SdpSess		uaGetReqSdp(SipReq);
const char*	uaGetReqCallee(SipReq);
const char*	uaGetReqReqURI(SipReq);

/*Get From and To header tag*/
char*		uaGetReqFromTag(SipReq);
char*		uaGetReqToTag(SipReq);

/*Get From and To header address*/
char*		uaGetReqFromAddr(SipReq);
char*		uaGetReqToAddr(SipReq);

/*add by tyhuang:Get From and To Displayname */
char*	uaGetReqFromDisplayname(SipReq);
char*	uaGetReqToDisplayname(SipReq);
char*	uaGetReqContact(SipReq);
/*check request message, decide response code*/
UAStatusCode	uaReqChecking(SipReq);

/*create a new SipReqLine, (Method,Req-URI,SIP-Version) */
SipReqLine*	uaReqNewReqLine(SipMethodType,char*,char*);
RCODE		uaReqLineFree(SipReqLine*);

/*create a new SipRspStatusLine, (SIP-Version,code,reason)*/
SipRspStatusLine * uaRspNewStatusLine(const char*,UAStatusCode,const char*);
RCODE		uaRspStatusLineFree(SipRspStatusLine*);

/*get REFER resposne*/
SipRsp		uaGetReferRsp(IN UaDlg);

/*create a new CSeq */
SipCSeq*	uaNewCSeq(int,SipMethodType);
RCODE		uaFreeCSeq(SipCSeq*);

/*sip Response message processing*/
SipMethodType	uaGetRspMethod(IN SipRsp);
int		uaGetRspStatusCode(IN SipRsp);
int		uaGetRspCSeq(IN SipRsp);
SdpSess		uaGetRspSdp(IN SipRsp);

/*Get From and To header tag*/
char*		uaGetRspFromTag(SipRsp);
char*		uaGetRspToTag(SipRsp);

/*get DIGEST authenticate response header*/
RCODE		uaRspGetDigestAuthn(IN SipRsp,IN UAAuthnType,IN SipDigestWWWAuthn*);

/*get DIGEST auth request header*/
RCODE	uaGetDigestAuthz(SipAuthz *authz,SipDigestAuthz* auth);

/*add DIGEST authorization header*/
RCODE		uaReqAddDigestAuthz(IN SipReq ,IN UAAuthzType ,IN SipDigestAuthz);

/*sip Body create*/
SipBody*	uaCreatSipBody(IN SdpSess);
void		uaFreeSipBody(IN SipBody*);

/*add by tyhuang . to get sip message content from request or response */
UaContent	uaGetReqContent(IN SipReq);
UaContent	uaGetRspContent(IN SipRsp);

/*sdp checking*/
BOOL		uaCheckHold(IN SdpSess);

/*create sipfrag from response message*/
/*char* GetSipfragFromRsp(IN SipRsp);*/
RCODE GetSipfragFromRsp(IN SipRsp rsp,OUT char* pbuf,IN int length);
BOOL  BeSIPSURL(IN const char*); 

char* ReqGetViaBranch(SipReq req);

unsigned char* ReqGetSupported(SipReq req);
unsigned char* RspGetSupported(SipRsp rsp);

#ifdef  __cplusplus
}
#endif

#endif /* UA_SIPMSG_H */
