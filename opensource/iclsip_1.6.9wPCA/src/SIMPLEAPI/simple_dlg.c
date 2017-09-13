/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * simple_dlg.c
 *
 * $Id: simple_dlg.c,v 1.32 2006/01/27 01:30:54 tyhuang Exp $
 */
#include <stdio.h>
#include "simple_dlg.h"
#include <uacore/ua_dlg.h>
#include <uacore/ua_cm.h>
#include <uacore/ua_mgr.h>
#include <uacore/ua_sipmsg.h>
#include "simple_msg.h"
#include <uacore/ua_int.h>
#include <sip/sip.h>
#include <adt/dx_vec.h>
#include <adt/dx_lst.h>


//add by alan, sent subscribe with Supported: eventlist-->for group subscribe
CCLAPI RCODE uaDlgSubscribeWithEventList(IN UaDlg _dlg, IN UaSub _sub,IN const char *_eventname)
{

	RCODE rCode=RC_OK;
	SipReq _req=NULL;
	TxStruct subscribeTx=NULL;
	UserProfile pfile=NULL;
	UaMgr	mgr=NULL;
	UaUser  user=NULL;
	UaCfg	cfg=NULL;
	char *_tmpurl=NULL;
	/*SipReferTo *pReferTo=NULL;*/
	char _dest[256]={'\0'};
	int  _destlen=256;


	/*new dialog should not be NULL*/
	if(NULL==_dlg  ){
		TCRPrint(1,"*** [uaDlgSubscribe] dlg is NULL!\n");
		return UADLG_NULL;
	}
	/*target address and refer dialog can't NULL simtinuous*/
	if(NULL == _sub) {
		TCRPrint(1,"*** [uaDlgSubscribe] subscription is NULL!\n");
		return UADLG_NULL;
	}
	if((_tmpurl=uaSubGetURI(_sub) )== NULL){
		TCRPrint(1,"*** [uaDlgSubscribe] url in sub is NULL!\n");
		return UADLG_NULL;
	}
	strcpy(_dest,_tmpurl);

	mgr=uaDlgGetMgr(_dlg);
	if((NULL==mgr)){
		TCRPrint(1,"*** [uaDlgSubscribe] parent mgr is NULL!\n");
		return UAMGR_NULL;
	}
	user=uaMgrGetUser(mgr);
	if(( NULL==user )){
		TCRPrint(1,"*** [uaDlgSubscribe] user is NULL!\n");
		return UAUSER_NULL;
	}
	cfg=uaMgrGetCfg(mgr);
	if((NULL==cfg)){
		TCRPrint(1,"*** [uaDlgSubscribe] cfg is NULL!\n");
		return UACFG_NULL;
	}
	
	/* construct subscribe request message */
	_req=uaSUBSCRIBEMsgWithEventList(_dlg,_dest,_eventname,uaSubGetExpires(_sub),NULL);
	
	if(_req==NULL){
		TCRPrint(1,"*** [uaDlgSubscribe] new request is NULL!\n");
		return UASIPMSG_REQ_NULL;
	}
	pfile=uaCreateUserProfileForSIMPLE(cfg);
	/*pfile=uaCreateUserProfile(cfg);*/
	if(pfile == NULL){
		TCRPrint(1,"*** [uaDlgSubscribe] user profile is NULL!\n");
		return UAUSER_PROFILE_NULL;
	}
	uaDlgSetCallID(_dlg,(char*)sipReqGetHdr(_req,Call_ID));
	/* set remote address */
	if(uaDlgGetRemoteParty(_dlg)==NULL)
		uaDlgSetRemoteAddr(_dlg,uaGetReqToAddr(_req));
	uaDlgSetRemoteTarget(_dlg,_dest);
	/*uaDlgSetState(_this,UACSTATE_SEND_SUBSCRIBE);
	uaMgrPostNewMsg(mgr,_this,UAMSG_CALLSTATE,uaGetReqCSeq(_req),uaGetReqSdp(_req));*/

	/*uaMgrPostNewMessage(mgr,_dlg,UAMSG_SUBSCRIBE_SEND,uaGetReqCSeq(_req),NULL);*/
	/*create a new client transaction */
	subscribeTx=sipTxClientNew(_req,NULL,pfile,(void*)_dlg);
	uaUserProfileFree(pfile);

	if(subscribeTx == NULL){
		sipReqFree(_req);
		TCRPrint(1,"*** [uaDlgSubscribe] subscribe Tx is NULL!\n");
		return UATX_NULL;
	}
	/*add a transaction into dialog*/
	rCode=uaDlgAddTxIntoTxList(_dlg,subscribeTx);
	
	if(uaDlgGetMethod(_dlg)==UNKNOWN_METHOD)
		uaDlgSetMethod(_dlg,SIP_SUBSCRIBE);
	/*add dialog into mgr if it doesnt exist*/
	if(uaMgrGetMatchDlg(mgr,uaDlgGetCallID(_dlg))==NULL)
		dxLstPutTail(uaMgrGetDlgLst(mgr),_dlg);

	TCRPrint(TRACE_LEVEL_API,"*** [uaDlgSubscribe] uaDlg count = %d\n",dxLstGetSize(uaMgrGetDlgLst(mgr)));
 
	return rCode;
}


/* subscribe */
/** \fn RCODE uaDlgSubscribe(IN UaDlg _dlg, IN UaSub _sub)
 *  \brief send a subscribe message by a dlg
 *  \param _dlg a dialog will be used to send the message
 *  \param _sub a UaSub object contains the information of a subscription
 *  \retval RCODE :RC_OK if success,or other error code if error occurs
 */
CCLAPI RCODE uaDlgSubscribe(IN UaDlg _dlg, IN UaSub _sub,IN const char *_eventname)
{

	RCODE rCode=RC_OK;
	SipReq _req=NULL;
	TxStruct subscribeTx=NULL;
	UserProfile pfile=NULL;
	UaMgr	mgr=NULL;
	UaUser  user=NULL;
	UaCfg	cfg=NULL;
	char *_tmpurl=NULL;
	/*SipReferTo *pReferTo=NULL;*/
	char _dest[256]={'\0'};
	int  _destlen=256;


	/*new dialog should not be NULL*/
	if(NULL==_dlg  ){
		TCRPrint(1,"*** [uaDlgSubscribe] dlg is NULL!\n");
		return UADLG_NULL;
	}
	/*target address and refer dialog can't NULL simtinuous*/
	if(NULL == _sub) {
		TCRPrint(1,"*** [uaDlgSubscribe] subscription is NULL!\n");
		return UADLG_NULL;
	}
	if((_tmpurl=uaSubGetURI(_sub) )== NULL){
		TCRPrint(1,"*** [uaDlgSubscribe] url in sub is NULL!\n");
		return UADLG_NULL;
	}
	strcpy(_dest,_tmpurl);

	mgr=uaDlgGetMgr(_dlg);
	if((NULL==mgr)){
		TCRPrint(1,"*** [uaDlgSubscribe] parent mgr is NULL!\n");
		return UAMGR_NULL;
	}
	user=uaMgrGetUser(mgr);
	if(( NULL==user )){
		TCRPrint(1,"*** [uaDlgSubscribe] user is NULL!\n");
		return UAUSER_NULL;
	}
	cfg=uaMgrGetCfg(mgr);
	if((NULL==cfg)){
		TCRPrint(1,"*** [uaDlgSubscribe] cfg is NULL!\n");
		return UACFG_NULL;
	}
	
	/* construct subscribe requets message */
	_req=uaSUBSCRIBEMsg(_dlg,_dest,_eventname,uaSubGetExpires(_sub),NULL);
	
	if(_req==NULL){
		TCRPrint(1,"*** [uaDlgSubscribe] new request is NULL!\n");
		return UASIPMSG_REQ_NULL;
	}
	/*pfile=uaCreateUserProfileForSIMPLE(cfg); mark by tyhuang 2004/11/23 */
	pfile=uaCreateUserProfileForSIMPLE(cfg);
	if(pfile == NULL){
		TCRPrint(1,"*** [uaDlgSubscribe] user profile is NULL!\n");
		return UAUSER_PROFILE_NULL;
	}
	uaDlgSetCallID(_dlg,(char*)sipReqGetHdr(_req,Call_ID));
	/* set remote address */
	if(uaDlgGetRemoteParty(_dlg)==NULL)
		uaDlgSetRemoteAddr(_dlg,uaGetReqToAddr(_req));
	uaDlgSetRemoteTarget(_dlg,_dest);
	/*uaDlgSetState(_this,UACSTATE_SEND_SUBSCRIBE);
	uaMgrPostNewMsg(mgr,_this,UAMSG_CALLSTATE,uaGetReqCSeq(_req),uaGetReqSdp(_req));*/

	/*uaMgrPostNewMessage(mgr,_dlg,UAMSG_SUBSCRIBE_SEND,uaGetReqCSeq(_req),NULL);*/
	/*create a new client transaction */
	subscribeTx=sipTxClientNew(_req,NULL,pfile,(void*)_dlg);
	uaUserProfileFree(pfile);

	if(subscribeTx == NULL){
		sipReqFree(_req);
		TCRPrint(1,"*** [uaDlgSubscribe] subscribe Tx is NULL!\n");
		return UATX_NULL;
	}
	/*add a transaction into dialog*/
	rCode=uaDlgAddTxIntoTxList(_dlg,subscribeTx);
	
	if(uaDlgGetMethod(_dlg)==UNKNOWN_METHOD)
		uaDlgSetMethod(_dlg,SIP_SUBSCRIBE);
	/*add dialog into mgr if it doesnt exist*/
	if(uaMgrGetMatchDlg(mgr,uaDlgGetCallID(_dlg))==NULL)
		dxLstPutTail(uaMgrGetDlgLst(mgr),_dlg);

	TCRPrint(TRACE_LEVEL_API,"*** [uaDlgSubscribe] uaDlg count = %d\n",dxLstGetSize(uaMgrGetDlgLst(mgr)));
 
	return rCode;
}

/** Send a Message by the specificed dialog 
 *  \param _dlg a dialog will be used to send the message
 *  \param _dest the destination url 
 *  \param _content the message body with content-type
 *  \retval RCODE :RC_OK if success,or other error code if error occurs
 */
CCLAPI	RCODE uaDlgMessage(IN UaDlg _dlg, IN const char* _dest, IN UaContent _content)
{
	RCODE rCode=RC_OK;
	SipReq _req=NULL;
	TxStruct messageTx=NULL;
	UserProfile pfile=NULL;
	UaMgr	mgr=NULL;
	UaUser  user=NULL;
	UaCfg	cfg=NULL;
	char *_tmpurl=NULL;

	/*new dialog should not be NULL*/
	if(NULL==_dlg  ){
		TCRPrint(1,"*** [uaDlgMessage] dlg is NULL!\n");
		return UADLG_NULL;
	}
	/*target address should not be NULL*/
	if(NULL == _dest){ 
		TCRPrint(1,"*** [uaDlgMessage] destination is NULL!\n");
		return UADLG_NULL;
	}
	mgr=uaDlgGetMgr(_dlg);
	if((NULL==mgr)){
		TCRPrint(1,"*** [uaDlgMessage] parent manager is NULL!\n");
		return UAMGR_NULL;
	}
	user=uaMgrGetUser(mgr);
	if(( NULL==user )){
		TCRPrint(1,"*** [uaDlgMessage] user is NULL!\n");
		return UAUSER_NULL;
	}
	cfg=uaMgrGetCfg(mgr);
	if((NULL==cfg)){
		TCRPrint(1,"*** [uaDlgMessage] cfg is NULL!\n");
		return UACFG_NULL;
	}

	/* construct subscribe request message */
	_req=uaMESSAGEMsg(_dlg,_dest,_content);
	
	if(_req==NULL){
		TCRPrint(1,"*** [uaDlgMessage] new request is NULL!\n");
		return UASIPMSG_REQ_NULL;
	}
	pfile=uaCreateUserProfileForSIMPLE(cfg);
	/*pfile=uaCreateUserProfile(cfg);	*/
	if(pfile == NULL){
		TCRPrint(1,"*** [uaDlgMessage] user profile is NULL!\n");
		return UAUSER_PROFILE_NULL;
	}
	uaDlgSetCallID(_dlg,(char*)sipReqGetHdr(_req,Call_ID));
	/* set remote address */
	if(uaDlgGetRemoteParty(_dlg)==NULL)
		uaDlgSetRemoteAddr(_dlg,uaGetReqToAddr(_req));
	uaDlgSetRemoteTarget(_dlg,_dest);
	/*uaDlgSetState(_this,UACSTATE_SEND_MESSAGE);
	uaMgrPostNewMsg(mgr,_this,UAMSG_CALLSTATE,uaGetReqCSeq(_req),uaGetReqSdp(_req));*/
	/*uaMgrPostNewMessage(mgr,_dlg,UAMSG_MESSAGE_SEND,uaGetReqCSeq(_req),_content);*/
	/*create a new client transaction */
	messageTx=sipTxClientNew(_req,NULL,pfile,(void*)_dlg);
	uaUserProfileFree(pfile);

	if(messageTx == NULL){
		sipReqFree(_req);
		TCRPrint(1,"*** [uaDlgMessage] messageTx is NULL!\n");
		return UATX_NULL;
	}
	
	if(uaDlgGetMethod(_dlg)==UNKNOWN_METHOD)
		uaDlgSetMethod(_dlg,SIP_MESSAGE);
	/*add a transaction into dialog*/
	rCode=uaDlgAddTxIntoTxList(_dlg,messageTx);

	/*add dialog into mgr if it doesnt exist*/
	if(uaMgrGetMatchDlg(mgr,uaDlgGetCallID(_dlg))==NULL)
		dxLstPutTail(uaMgrGetDlgLst(mgr),_dlg);

	TCRPrint(TRACE_LEVEL_API,"*** [uaDlgMessage] uaDlg count = %d\n",dxLstGetSize(uaMgrGetDlgLst(mgr)));
	
	return rCode;
}

/** Send a Notify message
 *  \param _sub a subscription will be used to send the message
 *  \param _dest the destination url 
 *  \param _content the message body with content-type  
 *  \retval RCODE :RC_OK if success,or other error code if error occurs
 */
CCLAPI	RCODE uaSubNotify(IN UaSub _sub, IN const char* _dest, IN UaContent _content)
{
	RCODE rCode=RC_OK;
	SipReq _req=NULL;
	TxStruct notifyTx=NULL;
	UserProfile pfile=NULL;
	UaMgr	mgr=NULL;
	UaUser  user=NULL;
	UaCfg	cfg=NULL;
	UaDlg	_dlg=NULL;
	char *_tmpurl=NULL;

	if(_sub==NULL){
		TCRPrint(1,"*** [uaSubNotify] subscription is NULL!\n");
		return UADLG_NULL;
	}
	_dlg=uaSubGetDlg(_sub);
	/*new dialog should not be NULL*/
	if(NULL==_dlg  ){
		TCRPrint(1,"*** [uaSubNotify] dlg is NULL!\n");
		return UADLG_NULL;
	}
	/*target address should not be NULL*/
	if(NULL == _dest){ 
		TCRPrint(1,"*** [uaSubNotify] destination is NULL!\n");
		return UADLG_NULL;
	}
	mgr=uaDlgGetMgr(_dlg);
	if((NULL==mgr)){
		TCRPrint(1,"*** [uaSubNotify] parent manager is NULL!\n");
		return UAMGR_NULL;
	}
	user=uaMgrGetUser(mgr);
	if(( NULL==user )){
		TCRPrint(1,"*** [uaSubNotify] user is NULL!\n");
		return UAUSER_NULL;
	}
	cfg=uaMgrGetCfg(mgr);
	if((NULL==cfg)){
		TCRPrint(1,"*** [uaSubNotify] cfg is NULL!\n");
		return UACFG_NULL;
	}

	/* construct notify requets message */
	_req=uaSubNOTIFYMsg(_dlg,_sub,_dest,_content);
	
	if(_req==NULL){
		TCRPrint(1,"*** [uaSubNotify] new request is NULL!\n");
		return UASIPMSG_REQ_NULL;
	}
	pfile=uaCreateUserProfileForSIMPLE(cfg);
	if(pfile == NULL){
		TCRPrint(1,"*** [uaSubNotify] user profile is NULL!\n");
		return UAUSER_PROFILE_NULL;
	}
	uaDlgSetCallID(_dlg,(char*)sipReqGetHdr(_req,Call_ID));

	/*uaDlgSetState(_this,UACSTATE_SEND_NOTIFY);
	uaMgrPostNewMsg(mgr,_this,UAMSG_CALLSTATE,uaGetReqCSeq(_req),uaGetReqSdp(_req));*/
	/*uaMgrPostNewMessage(mgr,_dlg,UAMSG_NOTIFY_SEND,uaGetReqCSeq(_req),_content);*/
	/*create a new client transaction */
	notifyTx=sipTxClientNew(_req,NULL,pfile,(void*)_dlg);
	uaUserProfileFree(pfile);

	if(notifyTx == NULL){
		sipReqFree(_req);
		TCRPrint(1,"*** [uaSubNotify] notifyTx is NULL!\n");
		return UATX_NULL;
	}
	if(uaDlgGetMethod(_dlg)==UNKNOWN_METHOD)
		uaDlgSetMethod(_dlg,SIP_NOTIFY);
	/*add a transaction into dialog*/
	rCode=uaDlgAddTxIntoTxList(_dlg,notifyTx);
 
	return rCode;
}

/** Send a Publish 
 *  \param _dlg a dialog will be used to send the message
 *  \param _dest the destination url 
 *  \param _event specify event type
 *  \param _class class list of the message body
 *  \param _content the message body with content-type 
 *  \retval RCODE :RC_OK if success,or other error code if error occurs
 */
CCLAPI RCODE uaDlgPublish(IN UaDlg _dlg, IN const char* _dest,IN const char* _event,IN const char* _class,IN UaContent _content, IN int expires)
{
	RCODE rCode=RC_OK;
	SipReq _req=NULL;
	TxStruct publishTx=NULL;
	UserProfile pfile=NULL;
	UaMgr	mgr=NULL;
	UaUser  user=NULL;
	UaCfg	cfg=NULL;
	char *_tmpurl=NULL;

	/*new dialog should not be NULL*/
	if(NULL==_dlg  ){
		TCRPrint(1,"*** [uaDlgPublish] dlg is NULL!\n");
		return UADLG_NULL;
	}
	/*target address should not be NULL*/
	if(NULL == _dest){ 
		TCRPrint(1,"*** [uaDlgPublish] destination is NULL!\n");
		return UADLG_NULL;
	}
	mgr=uaDlgGetMgr(_dlg);
	if((NULL==mgr)){
		TCRPrint(1,"*** [uaDlgPublish] parent manager is NULL!\n");
		return UAMGR_NULL;
	}
	user=uaMgrGetUser(mgr);
	if(( NULL==user )){
		TCRPrint(1,"*** [uaDlgPublish] user is NULL!\n");
		return UAUSER_NULL;
	}
	cfg=uaMgrGetCfg(mgr);
	if((NULL==cfg)){
		TCRPrint(1,"*** [uaDlgPublish] cfg is NULL!\n");
		return UACFG_NULL;
	}

	/* construct subscribe requets message */
	_req=uaPUBLISHMsg(_dlg,_dest,_event,_class,_content,expires);
	
	if(_req==NULL){
		TCRPrint(1,"*** [uaDlgPublish] new request is NULL!\n");
		return UASIPMSG_REQ_NULL;
	}
	
	/*pfile=uaCreateUserProfile(cfg);*/
	pfile=uaCreateUserProfileForSIMPLE(cfg);
	if(pfile == NULL){
		TCRPrint(1,"*** [uaDlgPublish] user profile is NULL!\n");
		return UAUSER_PROFILE_NULL;
	}
	uaDlgSetCallID(_dlg,(char*)sipReqGetHdr(_req,Call_ID));
	/* set remote address */
	if(uaDlgGetRemoteParty(_dlg)==NULL)
		uaDlgSetRemoteAddr(_dlg,uaGetReqToAddr(_req));
	uaDlgSetRemoteTarget(_dlg,_dest);
	/*uaDlgSetState(_this,UACSTATE_SEND_PUBLISH);
	uaMgrPostNewMsg(mgr,_this,UAMSG_CALLSTATE,uaGetReqCSeq(_req),uaGetReqSdp(_req));*/
	/*uaMgrPostNewMessage(mgr,_dlg,UAMSG_PUBLISH_SEND,uaGetReqCSeq(_req),_content);*/
	/*create a new client transaction */
	publishTx=sipTxClientNew(_req,NULL,pfile,(void*)_dlg);
	uaUserProfileFree(pfile);

	if(publishTx == NULL){
		sipReqFree(_req);
		TCRPrint(1,"*** [uaDlgPublish] publishTx is NULL!\n");
		return UATX_NULL;
	}
	/*add a transaction into dialog*/
	rCode=uaDlgAddTxIntoTxList(_dlg,publishTx);
	if(uaDlgGetMethod(_dlg)==UNKNOWN_METHOD)
		uaDlgSetMethod(_dlg,SIP_PUBLISH);
	/*add dialog into mgr if it doesnt exist*/
	if(uaMgrGetMatchDlg(mgr,uaDlgGetCallID(_dlg))==NULL)
		dxLstPutTail(uaMgrGetDlgLst(mgr),_dlg);

	TCRPrint(TRACE_LEVEL_API,"*** [uaDlgPublish] uaDlg count = %d\n",dxLstGetSize(uaMgrGetDlgLst(mgr)));
 
	return rCode;
}


/** Send a Publish 
 *  \param _dlg a dialog will be used to send the message
 *  \param _req the request message 
 *  \retval RCODE :RC_OK if success,or other error code if error occurs
 *	    ps: when UATX_NULL,user may free the request itself.	
 */
CCLAPI RCODE uaDlgSendRequest(IN UaDlg _dlg, IN SipReq _req)
{
	RCODE rCode=RC_OK;
	TxStruct reqTx=NULL;
	UserProfile pfile=NULL;
	UaMgr	mgr=NULL;
	UaUser  user=NULL;
	UaCfg	cfg=NULL;


	/*new dialog should not be NULL*/
	if(NULL==_dlg  ){
		TCRPrint(1,"*** [uaDlgPublish] dlg is NULL!\n");
		return UADLG_NULL;
	}
	/*target address should not be NULL*/
	/*if(NULL == _dest){ 
		TCRPrint(1,"*** [uaDlgPublish] destination is NULL!\n");
		return UADLG_NULL;
	}*/
	mgr=uaDlgGetMgr(_dlg);
	if((NULL==mgr)){
		TCRPrint(1,"*** [uaDlgPublish] parent manager is NULL!\n");
		return UAMGR_NULL;
	}
	user=uaMgrGetUser(mgr);
	if(( NULL==user )){
		TCRPrint(1,"*** [uaDlgPublish] user is NULL!\n");
		return UAUSER_NULL;
	}
	cfg=uaMgrGetCfg(mgr);
	if((NULL==cfg)){
		TCRPrint(1,"*** [uaDlgPublish] cfg is NULL!\n");
		return UACFG_NULL;
	}
	
	if(_req==NULL){
		TCRPrint(1,"*** [uaDlgPublish] new request is NULL!\n");
		return UASIPMSG_REQ_NULL;
	}
//	pfile=uaCreateUserProfileForSIMPLE(cfg);
	pfile=uaCreateUserProfile(cfg);
	if(pfile == NULL){
		TCRPrint(1,"*** [uaDlgPublish] user profile is NULL!\n");
		return UAUSER_PROFILE_NULL;
	}
	uaDlgSetCallID(_dlg,(char*)sipReqGetHdr(_req,Call_ID));
	/* set remote address */
	uaDlgSetRemoteAddr(_dlg,uaGetReqToAddr(_req));
	uaDlgSetRemoteTarget(_dlg,uaGetReqReqURI(_req));
	/*uaDlgSetState(_this,UACSTATE_SEND_PUBLISH);
	uaMgrPostNewMsg(mgr,_this,UAMSG_CALLSTATE,uaGetReqCSeq(_req),uaGetReqSdp(_req));*/
	/*uaMgrPostNewMessage(mgr,_dlg,UAMSG_PUBLISH_SEND,uaGetReqCSeq(_req),_content);*/
	/*create a new client transaction */
	reqTx=sipTxClientNew(_req,NULL,pfile,(void*)_dlg);
	uaUserProfileFree(pfile);

	if(reqTx == NULL){
		/*sipReqFree(_req);*/
		TCRPrint(1,"*** [uaDlgPublish] publishTx is NULL!\n");
		return UATX_NULL;
	}
	/*add a transaction into dialog*/
	rCode=uaDlgAddTxIntoTxList(_dlg,reqTx);
	if(uaDlgGetMethod(_dlg)==UNKNOWN_METHOD)
		uaDlgSetMethod(_dlg,sipTxGetMethod(reqTx));
	/*add dialog into mgr if it doesnt exist*/
	if(uaMgrGetMatchDlg(mgr,uaDlgGetCallID(_dlg))==NULL)
		dxLstPutTail(uaMgrGetDlgLst(mgr),_dlg);

	TCRPrint(TRACE_LEVEL_API,"*** [uaDlgPublish] uaDlg count = %d\n",dxLstGetSize(uaMgrGetDlgLst(mgr)));
 
	return rCode;
}
