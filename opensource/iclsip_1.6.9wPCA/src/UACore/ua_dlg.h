/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * ua_dlg.h
 *
 * $Id: ua_dlg.h,v 1.86 2006/07/13 03:27:06 tyhuang Exp $
 */

#ifndef UA_DLG_H
#define UA_DLG_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "ua_cm.h"
#include <sdp/sdp.h>
#include <adt/dx_lst.h>
#include <sipTx/sipTx.h>

typedef enum{
	UA_ANSWER_CALL,
	UA_REJECT_CALL,
	UA_BUSY_HERE,
	UA_RINGING,
	UA_REDIRECT,
	UA_100rel
}UAAnswerType;

/* Dialog Initialize function and free */
/*create a dialog to attach parent */
CCLAPI UaDlg uaDlgNew(IN UaMgr);	

/*Release dialog is to release control handle */
/*will clean all resource when all transaction is terminated*/
CCLAPI void uaDlgRelease(IN UaDlg);

CCLAPI BOOL IsDlgReleased(IN UaDlg);

/*get parent - uaMgr */	
CCLAPI UaMgr uaDlgGetMgr(IN UaDlg);

CCLAPI char* uaDlgGetUser(IN UaDlg);

/* change parent -uaMgr */
CCLAPI RCODE uaDlgSetMgr(IN UaDlg,IN UaMgr);

/* Dialog get a callstate */
CCLAPI UACallStateType uaDlgGetState(IN UaDlg);
RCODE uaDlgSetState(IN UaDlg,IN UACallStateType);

/*dialog callid*/
CCLAPI char* uaDlgGetCallID(IN UaDlg);
RCODE uaDlgSetCallID(IN UaDlg,IN const char*);

/*dialog user id*/
CCLAPI char* uaDlgGetUserID(IN UaDlg);
RCODE uaDlgSetUserID(IN UaDlg, IN const char*);

/*dialog initial method 2003.6.24.tyhuang */
CCLAPI	SipMethodType uaDlgGetMethod(IN UaDlg);

/* priavcy and p-preferred-identity setting. non-doc */
CCLAPI RCODE uaDlgSetPrivacy(IN UaDlg,IN const char*);
CCLAPI RCODE uaDlgSetPPreferredID(IN UaDlg,IN const char*);
CCLAPI RCODE uaDlgSetUserPrivacy(IN UaDlg,IN BOOL);

/* make a call to destination, input SIP-URI destination and sdp */
CCLAPI  RCODE uaDlgGetReferTo(IN UaDlg,IN OUT char*,IN OUT int*);
/* make a call to destination, input SIP-URI destination and sdp */
CCLAPI RCODE uaDlgDial(IN UaDlg, IN const char* ,IN SdpSess ); 

/* make a call to destination*/
			  /* new INVITE dialog,new destination, ref dialog, sdp*/
CCLAPI RCODE uaDlgDialRefer(IN UaDlg,IN const char*, IN UaDlg, IN SdpSess);
/*when receive UAMSG_REFER, call this function to create a new INVITE transactoin*/
/* it should be uaDlgDialRefer(new INVITE Dialog, 
			       NULL(will depden on REFER message Refer-To header),
			       REFER dialog(get from UAMSG_REFER callback message),
			       New INVITE SDP*/

/* cancel a pending call */
CCLAPI RCODE uaDlgCANCEL(IN UaDlg);	

/* disconnect a active call */
CCLAPI RCODE uaDlgDisconnect(IN UaDlg); 
CCLAPI RCODE uaDlgDisconnectEx(IN UaDlg,IN UaContent);
/* Anser/reject/busy call, user can give detail response code */
CCLAPI RCODE uaDlgAnswer(IN UaDlg, IN UAAnswerType, IN UAStatusCode, IN SdpSess);
CCLAPI RCODE uaDlgAnswerCall(IN UaDlg ,IN UAAnswerType , IN UAStatusCode , IN UaContent ,IN const char* ); 

/* send a Info message */
CCLAPI RCODE uaDlgInfo(IN UaDlg, IN const char*, IN UaContent);

/* send a Info message */
CCLAPI RCODE uaDlgUpdate(IN UaDlg, IN const char*, IN UaContent);

/* send a Info message */
CCLAPI RCODE uaDlgPRAck(IN UaDlg, IN const char*, IN UaContent,IN unsigned int);

/* Hold an active call, rfc3261 & rfc2543 */
CCLAPI RCODE uaDlgHold(IN UaDlg);

/* Retrieve a hold call rfc3261 & rfc2543 */
CCLAPI RCODE uaDlgUnHold(IN UaDlg,IN SdpSess);

/* send a reinvite messgae, call state == UACSTATE_CONNECTED
 * should input a  new sdp for change codecs,or add/close a media stream
 */
CCLAPI RCODE uaDlgChangeSDP(IN UaDlg _dlg,IN SdpSess _sdp);

/* based on draft-ietf-sipping-service-examples02.txt */
/* UnAttend transfer call */
CCLAPI RCODE uaDlgUnAttendTransfer(IN UaDlg, IN const char* );

/* Attend Transfer call */
/*input origin dialog, second dialog and REFER dialog */
CCLAPI RCODE uaDlgAttendTransfer(IN UaDlg, IN UaDlg, IN UaDlg );

/* get remote party contact address */
CCLAPI const char * uaDlgGetRemoteTarget(IN UaDlg);

/*get remote party address, get from From or To header*/
CCLAPI const char* uaDlgGetRemoteParty(IN UaDlg);

/*Get remote party displayname, get from From header*/
CCLAPI const char* uaDlgGetRemotePartyDisplayname(IN UaDlg);

/*get remote tag from From or To header*/
/* if TRUE --> remote tag, FALSE --> local tag */
CCLAPI const char* uaDlgGetRemoteTag(IN UaDlg,IN BOOL);

/*Get local party address, get from From or To header*/
CCLAPI const char* uaDlgGetLocalParty(IN UaDlg );

/*get dialog status code --> for INVITE Transaction*/
CCLAPI UAStatusCode uaDlgGetLastestStatusCode(IN UaDlg);

CCLAPI RCODE uaDlgSetRefreshMethod(IN UaDlg,IN SipMethodType);

CCLAPI RCODE uaDlgSetRSeq(IN UaDlg ,IN unsigned int);
CCLAPI unsigned int uaDlgGetRSeq(IN UaDlg );

/*----------internal use ------------*/
/*destroy a dialog,including transaction */
void uaDlgFree(IN UaDlg);
/* send a Ack message */
RCODE uaDlgAck(IN UaDlg, IN const char*, IN SdpSess);

UaDlg uaDlgGetRefDlg(IN UaDlg);
BOOL  uaDlgGetSecureFlag(IN UaDlg);
RCODE uaDlgSetSecureFlag(IN UaDlg);
RCODE uaDlgSetMethod(IN UaDlg, IN SipMethodType);
char* uaDlgGetReferToReplaceID(IN UaDlg);
char* uaDlgGetReferToToTag(IN UaDlg);
char* uaDlgGetReferToFromTag(IN UaDlg); 
/*Set Record-Route */
/*uaDlgSetRecordRoute(dialog,record-route); */
RCODE uaDlgSetReqRecordRoute(IN UaDlg,IN const char*);
const char* uaDlgGetReqRecordRoute(IN UaDlg);
RCODE uaDlgSetRspRecordRoute(IN UaDlg,IN const char*);
const char* uaDlgGetRspRecordRoute(IN UaDlg);


RCODE uaDlgGetInviteSDP(IN UaDlg);
BOOL  isuaDlgInviteSDP(IN UaDlg);
/*add unsupport Realm*/
RCODE uaDlgAddUnSupportRealm(IN UaDlg, IN char*);
RCODE uaDlgDelUnSupportRealm(IN UaDlg);

/*get dialog list*/
DxLst uaDlgGetTxList(IN UaDlg);

/*create a new Transaction list */
RCODE uaDlgNewTxList(IN UaDlg);

/*delete transaction from TxList*/
RCODE uaDlgDelTxFromTxList(IN UaDlg,IN TxStruct);

/*add transaction into TxList, since Incoming call*/
RCODE uaDlgAddTxIntoTxList(IN UaDlg,IN TxStruct);

/* free all invite tx when dialogstate is disconnect */
void uaDlgDelInviteTx(IN UaDlg);

/*decrease txCount number*/
RCODE uaDlgDecreaseTxCount(IN UaDlg);

/*increase txCount number*/
RCODE uaDlgIncreaseTxCount(IN UaDlg);

/*get txCount number*/
int uaDlgGetTxCount(IN UaDlg);

/*get bRelease flag, if TRUE: release*/
BOOL uaDlgGetReleaseFlag(IN UaDlg);

/* set remote id/addr/target */
RCODE uaDlgSetRemoteDisplayname(IN UaDlg , IN const char*);
RCODE uaDlgSetRemoteAddr(IN UaDlg , IN const char* );
RCODE uaDlgSetRemoteTarget(IN UaDlg , IN const char* );
char* uaDlgGetLocalTag(IN UaDlg);
char* uaDlgGetRemoveTag(IN UaDlg);
RCODE uaDlgSetRemoveTag(IN UaDlg,IN const char*);


RCODE uaDlgSetResponse(IN UaDlg, IN SipRsp);
/* get a lastest final response  */
SipRsp uaDlgGetResponse(IN UaDlg);

RCODE uaDlgSetRequest(IN UaDlg, IN SipReq);
/* get a requst  */
SipReq uaDlgGetRequest(IN UaDlg);
/*get local sdp, when play as Client, retrieve from request message*/
/*		 when paly as Server, retrieve from response message*/
SdpSess uaDlgGetLocalSdp(IN UaDlg);

BOOL IsDlgTimerd(IN UaDlg);
RCODE SetDlgTimerd(IN UaDlg,BOOL);
RCODE uaDlgSetTimerHandle(IN UaDlg,int);
int uaDlgGetTimerHandle(IN UaDlg);
RCODE uaDlgSetRegTimerHandle(IN UaDlg _dlg,int handle);

BOOL IsDlgServer(IN UaDlg);
RCODE SetDlgServer(IN UaDlg,BOOL);

RCODE uaDlgSetRemoteCseq(IN UaDlg _this,UINT32 cseq);
UINT32 uaDlgGetRemoteCseq(IN UaDlg _this);

/* session expires */
int uaDlgGetSEType(IN UaDlg);
unsigned int uaDlgGetSETimer(IN UaDlg);
RCODE uaDlgSetSETimer(IN UaDlg ,unsigned int);
int uaDlgGetSEHandle(IN UaDlg );
RCODE uaDlgSetSEHandle(IN UaDlg ,int ,int );
SipMethodType uaDlgGetRefreshMethod(IN UaDlg);

char* uaDlgGetPrivacy(IN UaDlg);
char* uaDlgGetPPreferredID(IN UaDlg);
BOOL uaDlgGetUserPrivacy(IN UaDlg);
/* sam add: followed 2 functions */
/* used to management reference to uaDlg object and prevent dangling pointers */
void uaDlgAddReference(IN UaDlg, IN UaDlg*);
void uaDlgRemoveReference(IN UaDlg, IN UaDlg*);

/* for handle refer event */
void uaDlgRemoveRef(IN UaDlg);
RCODE uaDlgSetREFERID(IN UaDlg,IN char*);

/* set SIP-If-Match */
RCODE uaDlgSetSIPIfMatch(IN UaDlg,IN unsigned char *sipetag);
unsigned char* uaDlgGetSIPIfMatch(IN UaDlg);

/* set supported */
RCODE uaDlgSetSupported(IN UaDlg,IN unsigned char *newsupp);
unsigned char* uaDlgGetSupported(IN UaDlg);

/*----------end of internal use ------------*/


/*not implement here*/
CCLAPI RCODE uaDlgConference(IN UaDlg, IN const char* destination);

#ifdef  __cplusplus
}
#endif

#endif /* UA_DLG_H */
