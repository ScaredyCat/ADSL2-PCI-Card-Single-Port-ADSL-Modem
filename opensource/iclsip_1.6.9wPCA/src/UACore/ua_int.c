/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 *
 * 
 * ua_int.c
 *
 * $Id: ua_int.c,v 1.353 2006/11/22 03:29:34 tyhuang Exp $
 */
#include <stdio.h>

#ifdef _WIN32
#include <winsock.h>
#endif

#include <low/cx_timer.h>
#include <sip/sip.h>
#include <sipTx/sipTx.h>
#include "ua_int.h"
#include "ua_cfg.h"
#include "ua_cm.h"
#include "ua_sipmsg.h"
#include "ua_msg.h"
#include "ua_evtpkg.h"

#define MILLION		1000000

extern DxLst MgrList; /*keep all mgr objects*/

/*Get Transaction dialog*/
/*if binv == TURE --> invite transaction, else non-invite transaction*/
TxStruct uaDlgGetInviteTx(IN UaDlg _dlg,IN BOOL binv)
{
	TxStruct tx=NULL;
	DxLst txList=NULL;
	TXTYPE txtype;
	TXSTATE txstate;
	int txnum=0,txpos=0;

	txList=uaDlgGetTxList(_dlg);
	if(txList == NULL)
		return NULL;

	txnum=dxLstGetSize(txList);
	for(txpos=0;txpos<txnum;txpos++){
		tx=dxLstPeek(txList,txpos);
		txtype=sipTxGetType(tx);
		txstate=sipTxGetState(tx);
		if(binv){
			if((txtype == TX_CLIENT_INVITE)||(txtype==TX_SERVER_INVITE))
				return tx;
		}else {
			if((txtype == TX_CLIENT_NON_INVITE)||(txtype==TX_SERVER_NON_INVITE))
				return tx;
		}
		tx=NULL;
	}
	return tx;
}

/*Print all transaction in Dialog*/
/*for internal debug message*/
void uaDlgPrintAllTx(IN UaDlg _dlg)
{
	TxStruct tx=NULL;
	DxLst txList=NULL;
	TXTYPE txtype=TX_NON_ASSIGNED;
	TXSTATE txstate=TX_INIT;
	int txnum=0,txpos=0;

	txList=uaDlgGetTxList(_dlg);
	if(txList == NULL)
		return;

	txnum=dxLstGetSize(txList);

	TCRPrintEx1(TRACE_LEVEL_API, "[uaDlgPrintAllTx] txnum = %d \n", txnum);

	for(txpos=0;txpos<txnum;txpos++){
		tx=dxLstPeek(txList,txpos);
		txtype=sipTxGetType(tx);
		txstate=sipTxGetState(tx);
		switch (txtype) {
		case 	TX_NON_ASSIGNED:
			TCRPrintEx(TRACE_LEVEL_API, "[[uaDlgPrintAllTx] TX_NON_ASSIGNED \n");
			break;
		case	TX_SERVER_INVITE:
			TCRPrintEx(TRACE_LEVEL_API, "[uaDlgPrintAllTx] TX_SERVER_INVITE \n");
			break;
		case	TX_SERVER_NON_INVITE:
			TCRPrintEx(TRACE_LEVEL_API, "[UACORE] TX_SERVER_NON_INVITE \n");
			break;
		case	TX_SERVER_ACK:
			TCRPrintEx(TRACE_LEVEL_API, "[uaDlgPrintAllTx] TX_SERVER_INVITE_ACK \n");
			break;
		case	TX_CLIENT_INVITE:
			TCRPrintEx(TRACE_LEVEL_API, "[uaDlgPrintAllTx] TX_CLIENT_INVITE \n");
			break;
		case	TX_CLIENT_NON_INVITE:
			TCRPrintEx(TRACE_LEVEL_API, "[uaDlgPrintAllTx] TX_CLIENT_NON_INVITE");
			break;
		}
		switch (txstate) {
		case	TX_INIT:
			TCRPrintEx(TRACE_LEVEL_API, "[uaDlgPrintAllTx] TX_INIT\n");
			break;
		case	TX_TRYING:
			TCRPrintEx(TRACE_LEVEL_API, "[uaDlgPrintAllTx] TX_TRYING\n");
			break;
		case	TX_CALLING:
			TCRPrintEx(TRACE_LEVEL_API, "[uaDlgPrintAllTx] TX_CALLING\n");
			break;
		case	TX_PROCEEDING:
			TCRPrintEx(TRACE_LEVEL_API, "[uaDlgPrintAllTx] TX_PROCEEDING");
			break;
		case	TX_COMPLETED:
			TCRPrintEx(TRACE_LEVEL_API, "[uaDlgPrintAllTx] TX_COMPLETED\n");
			break;
		case	TX_CONFIRMED:
			TCRPrintEx(TRACE_LEVEL_API, "[uaDlgPrintAllTx] TX_CONFIRMED\n");
			break;
		case	TX_TERMINATED:
			TCRPrintEx(TRACE_LEVEL_API, "[uaDlgPrintAllTx] TX_TERMINATED\n");
			break;
		}
		tx=NULL;
	}
}

/*Get Transaction dialog*/
/*if binv == TURE --> invite transaction, else non-invite transaction*/
TxStruct uaDlgGetAliveInviteTx(IN UaDlg _dlg,IN BOOL binv)
{
	TxStruct tx=NULL;
	DxLst txList=NULL;
	TXTYPE txtype;
	TXSTATE txstate;
	int txnum=0,txpos=0;

	txList=uaDlgGetTxList(_dlg);
	if(txList == NULL)
		return NULL;

	txnum=dxLstGetSize(txList);
	for(txpos=0;txpos<txnum;txpos++){
		tx=dxLstPeek(txList,txpos);
		txtype=sipTxGetType(tx);
		txstate=sipTxGetState(tx);
		if(binv){
			if((txtype == TX_CLIENT_INVITE)||(txtype==TX_SERVER_INVITE)) {
				switch (txstate) {
				case TX_INIT:
				case TX_CALLING:
				case TX_PROCEEDING:
					return tx;
				default:
					continue;
				}
			}
		}else {
			if((txtype == TX_CLIENT_NON_INVITE)||(txtype==TX_SERVER_NON_INVITE)) {
				switch (txstate) {
				case TX_INIT:
				case TX_TRYING:
				case TX_PROCEEDING:
					return tx;
				default:
					continue;
				}
			}
		}
		tx=NULL;
	}
	return tx;
}

TxStruct uaDlgGetAliveServInviteTx(IN UaDlg _dlg)
{
	TxStruct tx=NULL;
	DxLst txList=NULL;
	TXTYPE txtype;
	TXSTATE txstate;
	int txnum=0,txpos=0;

	txList=uaDlgGetTxList(_dlg);
	if(txList == NULL)
		return NULL;

	txnum=dxLstGetSize(txList);
	/*
	for(txpos=0;txpos<txnum;txpos++){
	*/	
	for(txpos=txnum;txpos>0;txpos--){
		tx=dxLstPeek(txList,txpos-1);
		txtype=sipTxGetType(tx);
		txstate=sipTxGetState(tx);
	
		if(txtype==TX_SERVER_INVITE) {
				switch (txstate) {
				case TX_INIT:
				case TX_CALLING:
				case TX_PROCEEDING:
					return tx;
				default:
					continue;
				}
		}
		tx=NULL;
	}
	return tx;
}

TxStruct uaDlgGetMethodTx(IN UaDlg _dlg, IN SipMethodType _method)
{
	TxStruct tx=NULL;
	DxLst txList=NULL;
	int txnum=0,txpos=0;


	txList=uaDlgGetTxList(_dlg);
	if(txList == NULL)
		return NULL;

	txnum=dxLstGetSize(txList);
	for(txpos=0;txpos<txnum;txpos++){
		tx=dxLstPeek(txList,txpos);
		if(_method == sipTxGetMethod(tx))
			return tx;
		tx=NULL;
	}
	return tx;
}


/*create a UserPfofile from setting */
UserProfile uaCreateUserProfile(UaCfg _cfg)
{
	UserProfile pfile=NULL;
	char buf[256]={'\0'};

	if(_cfg==NULL)
		return NULL;

	pfile=(UserProfile)calloc(1,sizeof(*pfile));
	if(pfile == NULL) 
		return NULL;
	pfile->useproxy=(unsigned short)uaCfgUsingProxy(_cfg);
	if(uaCfgUsingProxy(_cfg)){
		switch(uaCfgGetProxyTXP(_cfg)){
		case TCP:
			/*sprintf(buf,"<sip:%s:%d;transport=tcp>",uaCfgGetProxyAddr(_cfg),uaCfgGetProxyPort(_cfg));*/
			sprintf(buf,"sip:%s:%d;transport=tcp",uaCfgGetProxyAddr(_cfg),uaCfgGetProxyPort(_cfg));
			break;
		case UDP:
		default:
			sprintf(buf,"sip:%s:%d",uaCfgGetProxyAddr(_cfg),uaCfgGetProxyPort(_cfg));
			break;
		}

		pfile->proxyaddr=strDup(buf);
	}else{
		pfile->proxyaddr=NULL;
	}

	pfile->localip = NULL;
	pfile->localport = 0;

	return pfile;
}

/*create a UserProfile for SIMPLE API from setting */
UserProfile uaCreateUserProfileForSIMPLE(IN UaCfg _cfg)
{
	UserProfile pfile=NULL;
	char buf[256]={'\0'};

	if ( !uaCfgUsingSIMPLEProxy(_cfg))
		return uaCreateUserProfile( _cfg);

	if(_cfg==NULL)
		return NULL;

	pfile=(UserProfile)calloc(1,sizeof(*pfile));
	if(pfile == NULL) 
		return NULL;
	pfile->useproxy=(unsigned short)uaCfgUsingSIMPLEProxy(_cfg);
	if(uaCfgUsingSIMPLEProxy(_cfg)){
		switch(uaCfgGetSIMPLEProxyTXP(_cfg)){
		case TCP:
			sprintf(buf,"<sip:%s:%d;transport=tcp>",uaCfgGetSIMPLEProxyAddr(_cfg),uaCfgGetSIMPLEProxyPort(_cfg));
			break;
		case UDP:
		default:
			sprintf(buf,"sip:%s:%d",uaCfgGetSIMPLEProxyAddr(_cfg),uaCfgGetSIMPLEProxyPort(_cfg));
			break;
		}

		pfile->proxyaddr=strDup(buf);
	}else{
		pfile->proxyaddr=NULL;
	}

	return pfile;
}


RCODE uaUserProfileFree(UserProfile _pfile)
{
	RCODE rCode=UACORE_ERROR;

	if(_pfile != NULL){
		if(_pfile->proxyaddr!=NULL)
			free(_pfile->proxyaddr);
		_pfile->useproxy=FALSE;
		free(_pfile);
		rCode=RC_OK;
	}else
		rCode=UAUSER_PROFILE_NULL;

	return rCode;
}

/*when get a request message*/
RCODE uaProcessReqTx(TxStruct _tx)
{
	RCODE rCode=RC_OK;
	SipReq req=NULL;
	SipRsp rsp=NULL;
	SipMethodType reqMethod=UNKNOWN_METHOD;
	/*UaMsg msg=NULL;*/
	UaMgr mgr=NULL;
	UaDlg origdlg=NULL,newdlg=NULL;
	UAStatusCode retCode=sipOK;
	UACallStateType dlgstate=UACSTATE_UNKNOWN;
	UaCfg	cfg=NULL;
	SipCSeq *cseq=NULL;
	/* event header */
	SipEvent *event=NULL;
	/* Expires header */
	SipExpires *exp=NULL;
	unsigned short expires=0;
	SipSessionExpires *pSE;
	/* content */
	UaContent _content=NULL;
	char *reqtotag,*reqfromtag,*rsptotag,*rspfromtag;
	int matchflag=0;
	UINT32 csold=0,csnew=0;

	/*get request method,if method not support, response error */
	reqMethod=sipTxGetMethod(_tx);
	
	/*get incoming request message*/
	req=sipTxGetOriginalReq(_tx);
	if(req == NULL){
		UaCoreERR("[uaProcessReqTx] Get Original Request NULL!\n");
		return UATX_REQ_NOT_FOUND;
	}
	/*check request "syntax" is correct/acceptable or not*/
	/*syntax error will response a error message and terminated */
	retCode=uaReqChecking(req);
	
	if(retCode!=sipOK){
		/*should response user not found*/
		rsp=uaCreateRspFromReq(req,retCode,NULL,NULL);
		if(rsp==NULL){
			/*discard the message */
			UaCoreERR("[uaProcessReqTx] Create Response NULL, when syntax acceptable\n");
			return UATX_REQ_MSG_ERROR;
		}else{
			sipTxSendRsp(_tx,rsp);
			UaCoreERR("[uaProcessReqTx] Create Response, request syntax error!\n");
			return UATX_REQ_MSG_ERROR;
		}
	}

	/*request message checking ok*/

	/*get original mgr,match the user*/
	mgr=uaMgrMatchTx(_tx);
	/* hotcoding for some purpose from TK. 2003.5.30 modified by tyhuang */
	if(mgr==NULL){
		/*can't match a accept mgr(user)*/
		rsp=uaCreateRspFromReq(req,Not_Found,NULL,NULL);
		if(rsp==NULL){
			/*discard the message */
			UaCoreERR("[uaProcessReqTx] Create Response NULL, When can't match a user!\n");
			return UATX_REQ_MSG_ERROR;
		}else{
			sipTxSendRsp(_tx,rsp);
			UaCoreERR("[uaProcessReqTx] Create Not Found Response, Since not found user!\n");
			return UACORE_USER_NOT_FOUND;
		}
	}
	cfg=uaMgrGetCfg(mgr);
	
	/*found mgr(device), match dialog or return NULL-->should creat a new dialog*/
	origdlg=uaDlgMatchTx(mgr,_tx);
	
	if(origdlg){
		/* check cseq */
		cseq=sipReqGetHdr(req,CSeq);
		if(cseq)
			csnew=cseq->seq;
		csold=uaDlgGetRemoteCseq(origdlg);
		if(csnew<csold){
             UaCoreWARN("[uaProcessReqTx] cseq sync error!\n" );
		    /*can't match a accept mgr(user)*/
/*			bad decision ;mark by tyhuang 2006/11/22
			rsp=uaCreateRspFromReq(req,Internal_Server_Error,NULL,NULL);
			if(rsp==NULL){
				
				UaCoreERR("[uaProcessReqTx] Create Response NULL, When can't match a user!\n");
				return UATX_REQ_MSG_ERROR;
			}else{
				sipTxSendRsp(_tx,rsp);
				UaCoreERR("[uaProcessReqTx] Create Internal_Server_Error Response!\n");
				return UATX_REQ_MSG_ERROR;
			}
*/
		}else
			uaDlgSetRemoteCseq(origdlg,csnew);

		/* drop re-transmit invite request */
		if((csnew==csold)&&(reqMethod == INVITE)){
			if((uaDlgGetState(origdlg)==UACSTATE_ACCEPT)||(uaDlgGetState(origdlg)==UACSTATE_REJECTED))
				return RC_OK;
		}
		/* if origdlg is not null,should check to-tag and from-tag 
		 * if not match ,response 481 call does not exist.
		 */	
		/*rsp=uaDlgGetResponse(origdlg); mark by tyhuang 2004/11/19 */
		rsp=sipTxGetLatestRsp(uaDlgGetInviteTx(origdlg,TRUE));
		reqtotag=uaGetReqToTag(req);
		if(rsp && reqtotag &&(reqMethod == INVITE)){
			/* if  original request has rsp ,should check from tag and to tag*/
			reqfromtag=uaGetReqFromTag(req);
			rsptotag=uaGetRspToTag(rsp);
			
			rspfromtag=uaGetRspFromTag(rsp);
			if(reqfromtag&&rsptotag){
				if(strcmp(reqfromtag,rsptotag)==0){		
					if(reqtotag&&rspfromtag && (reqMethod != CANCEL)){
						if(strcmp(reqtotag,rspfromtag)==0)
							matchflag++;
					}else if(reqMethod==CANCEL)
						matchflag ++;
					else/* without to-tag */
						matchflag=0;
				}
			}
			if(reqfromtag&&rspfromtag && (matchflag==0) ){
				if(strcmp(reqfromtag,rspfromtag)==0){		
					if(reqtotag&&rsptotag&& (reqMethod != CANCEL)){
						if(strcmp(reqtotag,rsptotag)==0)
							matchflag++;
					}else if(reqMethod==CANCEL)
						matchflag ++;
					else/* without to-tag */
						matchflag=0;
				}
			}
			if(matchflag==0){
				/* should response dialog not exist */
				rsp=uaCreateRspFromReq(req,Call_Leg_Transaction_Does_Not_Exist,NULL,NULL);
				if(rsp==NULL){
					/*discard the message */
					UaCoreERR("[uaProcessReqTx] Create Response NULL, when syntax acceptable\n");
					return UATX_REQ_MSG_ERROR;
				}else{
					sipTxSendRsp(_tx,rsp);
					UaCoreERR("[uaProcessReqTx] Create Response, request syntax error!\n");
					return UATX_REQ_MSG_ERROR;
				}
			}
		}else{
			/* new request ,check from tag */
			reqfromtag=uaGetReqFromTag(req);
			if(req==NULL){
				/*can't match a accept mgr(user)*/
				rsp=uaCreateRspFromReq(req,Bad_Request,NULL,NULL);
				if(rsp==NULL){
					/*discard the message */
					UaCoreERR("[uaProcessReqTx] Create Response NULL, When can't match a user!\n");
					return UATX_REQ_MSG_ERROR;
				}else{
					sipTxSendRsp(_tx,rsp);
					UaCoreERR("[uaProcessReqTx] Create Bad Request Response!\n");
					return UATX_REQ_MSG_ERROR;
				}
			}
		}
	}
	/* end of origdlg */
	rsp=NULL;
	/*get message body */
	_content=uaGetReqContent(req);

	switch(reqMethod){
	case INVITE:
		pSE=(SipSessionExpires*)sipReqGetHdr(req,Session_Expires);
		if(NULL==origdlg){
			/*create a new dialg for transaction*/
			UaDlg byeDlg=NULL;/*should to be disconnect dialog*/

			newdlg=uaDlgNew(mgr);
			if(pSE){
				/* check Min_SE */
				if(pSE->deltasecond<MINSE_EXPIRES){
					rsp=uaCreateRspFromReq(req,Session_Inerval_Too_Small,NULL,NULL);
					if(_content)
							uaContentFree(_content);
					if(rsp==NULL){
						/*discard the message */
						UaCoreERR("[uaProcessReqTx] Create Response NULL, When can't match a user!\n");
						return UATX_REQ_MSG_ERROR;
					}else{
						sipTxSendRsp(_tx,rsp);
						UaCoreERR("[uaProcessReqTx] Create Bad Request Response!\n");
						return UATX_REQ_MSG_ERROR;
					}
				}
			}else
				uaDlgSetSETimer(newdlg,0);
			
			/*if contain Replaces header, should disconnect the original call*/
			byeDlg=uaReqCheckReplaceMsg(mgr,req);

			
			/* set new dialog */
			if(BeSIPSURL(uaGetReqReqURI(req))==TRUE)
				uaDlgSetSecureFlag(newdlg);
			uaDlgSetRemoteDisplayname(newdlg,(char*)uaGetReqFromDisplayname(req));
			uaDlgSetRemoteAddr(newdlg,uaGetReqFromAddr(req));
			uaDlgSetRemoteTarget(newdlg,uaGetReqContact(req));
			uaDlgSetMethod(newdlg,reqMethod);
			uaDlgSetRequest(newdlg,req);
			SetDlgServer(newdlg,TRUE);
			cseq=sipReqGetHdr(req,CSeq);
			if(cseq)
				csnew=cseq->seq;
			uaDlgSetRemoteCseq(newdlg,csnew);
			uaDlgAddTxIntoTxList(newdlg,_tx);
			if(NULL==byeDlg){
				/*Replace header not exist, a new coming call, notify user to ring*/
				uaDlgSetState(newdlg,UACSTATE_OFFERING);
				/* rsp=uaCreateRspFromReq(req,Ringing,NULL,NULL);
				 * sipTxSendRsp(_tx,rsp);
				 * 2003.6.13
				 * just set state to offering but not send 180 response while
				 * it will send by application.modified by tyhuang
				 */
				sipTxSetUserData(_tx,newdlg);
				/* add a timer for Expires */
				exp=sipReqGetHdr(req,Expires);

				if(exp){
					expires=(unsigned short)exp->expireSecond;
					/* add timer */
					if( expires > 0 ){
						/* expire +2: since process delay before sending 100 tring */
						uaDlgSetTimerHandle(newdlg,uaAddTimer(newdlg,INVITE_EXPIRE,((expires+2)*1000)));
					}
				}
			}else{
				uaDlgDisconnect(byeDlg);
				/*Replace header exist, should not send ringing*/
				/*just to notify user*/
				sipTxSetUserData(_tx,newdlg);
				uaDlgSetState(newdlg,UACSTATE_OFFERING_REPLACE);
			}
			uaDlgSetCallID(newdlg,(char*)sipReqGetHdr(req,Call_ID));
			/*reqSdp=uaGetReqSdp(req);*/
			dxLstPutTail(uaMgrGetDlgLst(mgr),newdlg);
			TCRPrint(TRACE_LEVEL_API,"[uaProcessReqTx] uaDlg count = %d\n",dxLstGetSize(uaMgrGetDlgLst(mgr)));
			uaMgrPostNewMessage(mgr,newdlg,UAMSG_CALLSTATE,uaGetReqCSeq(req),_content,ReqGetSupported(req));

		}else{
			/*exist dialog tx*/
			/*reINVITE message*/
			/*call state should not change, just post message to application*/
			SdpSess reqsdp=NULL,localsdp=NULL,rspsdp=NULL;
			char *contacturl=NULL;
			TxStruct ptx=NULL;

			/* callid is the same ,should check Via */
			ptx=uaDlgGetAliveServInviteTx(origdlg);
			if(ptx && (ptx!=_tx)){
				SipReq preq=sipTxGetOriginalReq(ptx);
				char *pbid=ReqGetViaBranch(preq);
				char *bid=ReqGetViaBranch(req);

				retCode=sipOK;
				if(uaGetReqToTag(req)==NULL){
					/*
					if(bid==NULL)
						retCode=Loop_Detected;
					else if(pbid && (strcmp(bid,pbid)!=0))
						retCode=Loop_Detected;
					else
						retCode=sipOK;
					*/
				}

				if(retCode==Loop_Detected){
					rsp=uaCreateRspFromReq(req,retCode,NULL,NULL);
					if(rsp==NULL){
						/*discard the message */
						UaCoreERR("[uaProcessReqTx] Create Response NULL, when syntax acceptable\n");
						return UATX_REQ_MSG_ERROR;
					}else{
						sipTxSendRsp(_tx,rsp);
						UaCoreERR("[uaProcessReqTx] Create Response, request syntax error!\n");
						return UATX_REQ_MSG_ERROR;
					}
				}
			}
			

			reqsdp=uaGetReqSdp(req);
			
			/* set minse to dialog session timer */
			if(pSE){
				if(uaDlgGetSETimer(origdlg)>0)
					uaDelTimer(uaDlgGetSEHandle(origdlg));
				uaDlgSetSEHandle(origdlg,0,-1);
			}

			if(BeSIPSURL(uaGetReqReqURI(req))==TRUE)
				uaDlgSetSecureFlag(origdlg);
			uaDlgSetRemoteTarget(origdlg,uaGetReqContact(req));

			uaDlgAddTxIntoTxList(origdlg,_tx);
			sipTxSetUserData(_tx, origdlg);
			/*uaDlgPrintAllTx(origdlg);*/
			
 			if(uaCheckHold(reqsdp)){
				localsdp=uaDlgGetLocalSdp(origdlg);
				/* add reconly in media */
				if(strstr(uaContentGetBody(_content),"0.0.0.0"))
					rspsdp=uaSDPHoldSDP(localsdp,0);
				else
					rspsdp=uaSDPHoldSDP(localsdp,2);

				if(uaDlgGetSecureFlag(origdlg)==TRUE){
					contacturl=(char*)uaUserGetSIPSContact(uaMgrGetUser(mgr));
					if(contacturl)
						rsp=uaCreateRspFromReq(req,sipOK,contacturl,rspsdp);
					else{
						UaCoreERR("[uaProcessReqTx] SIPS contact is not found!\n");
						rsp=NULL;
					}
				}else
					rsp=uaCreateRspFromReq(req,sipOK,(char*)uaMgrGetContactAddr(mgr),rspsdp);
				
				if(rsp){
					sipTxSendRsp(_tx,rsp);
					uaDlgSetResponse(origdlg,rsp);
				}
			
				/*Hold call*/
				uaDlgSetState(origdlg,UACSTATE_ONHELD);
				uaMgrPostNewMessage(mgr,origdlg,UAMSG_CALLSTATE,uaGetReqCSeq(req),_content,ReqGetSupported(req));
			}else{
				/*rsp=uaCreateRspFromReq(req,sipOK,NULL,NULL);
				/sipTxSendRsp(_tx,rsp);*/
				/*change SDP contain*/
				if(uaDlgGetState(origdlg)==UACSTATE_ONHELD){
					/*if on Hold state, change into connected state*/
					uaDlgSetState(origdlg,UACSTATE_UNHOLD);
					uaMgrPostNewMessage(mgr,origdlg,UAMSG_CALLSTATE,uaGetReqCSeq(req),_content,ReqGetSupported(req));
				}else if(uaDlgGetState(origdlg)==UACSTATE_CONNECTED){
					/*notify application new SDP*/
					uaDlgSetState(origdlg,UACSTATE_SDPCHANGING);
					/*uaMgrPostNewMessage(mgr,origdlg,UAMSG_INFO,uaGetReqCSeq(req),_content,NULL);*/
					uaMgrPostNewMessage(mgr,origdlg,UAMSG_CALLSTATE,uaGetReqCSeq(req),_content,ReqGetSupported(req));
				}
			}
			if(reqsdp){
				uaSDPFree(reqsdp);
				reqsdp=NULL;
			}
			if(localsdp){
				uaSDPFree(localsdp);
				localsdp=NULL;
			}
			if(rspsdp){
				uaSDPFree(rspsdp);
				rspsdp=NULL;
			}
		}
		break;
	case CANCEL:
		if(origdlg==NULL){
			/*can't find a match dialog*/
			UaCoreERR("[uaProcessReqTx] cancel's original dialog is not found in mgr list!\n");
			rsp=uaCreateRspFromReq(req,Call_Leg_Transaction_Does_Not_Exist,NULL,NULL);
			if(rsp)
				sipTxSendRsp(_tx,rsp);
			sipTxFree(_tx);
			rCode=UATX_DLG_NULL;
			break;
		}else{
			/*match a original dialor*/
			dlgstate=uaDlgGetState(origdlg);
			switch(dlgstate){
			case UACSTATE_CONNECTED:
				/* this means can't CANCEL this call
				 * but just send 200 ok 
				
				rsp=uaCreateRspFromReq(req,sipOK,NULL,NULL);
				if(rsp){
					sipTxSendRsp(_tx,rsp);
					uaDlgSetResponse(origdlg,rsp);
				} 
				*/
				/* 
				 *	call disconnect
				 *  tyhuang add 2004/6/18
				 */
				uaDlgDisconnect(origdlg);
				break;
			case UACSTATE_OFFERING:
			case UACSTATE_OFFERING_REPLACE:
			{
				TxStruct inviteTx=NULL;
				SipReq inviteReq=NULL;
				SipRsp inviteCancel=NULL;
				SipTo	*invitecancelto=NULL;
				/*should get original INVITE transaction*/
				inviteTx=uaDlgGetSpecTx(origdlg,TX_SERVER_INVITE,INVITE);
				if(inviteTx==NULL){
					rCode=UATX_TX_NOT_FOUND;
					break;
				}
				/*get previous INVITE request message*/
				inviteReq=sipTxGetOriginalReq(inviteTx);
				if(inviteReq==NULL){
					rCode=UATX_REQ_NOT_FOUND;
					break;
				}
				/*generate 487 Request_Cancelled message*/
				inviteCancel=uaCreateRspFromReq(inviteReq,Request_Terminated,NULL,NULL);
				invitecancelto=sipRspGetHdr(inviteCancel,To);
				if(inviteCancel == NULL){
					rCode=UATX_RSP_NOT_FOUND;
					break;
				}
				if(inviteCancel){
					uaDlgSetResponse(origdlg,inviteCancel);
					sipTxSendRsp(inviteTx,inviteCancel);
					
				}
				
				/* remove invite expire timer*/
				if(IsDlgTimerd(origdlg)==TRUE){
					uaDelTimer(uaDlgGetTimerHandle(origdlg));
					SetDlgTimerd(origdlg,FALSE);	
				}

				/*real to CANCEL it 
				rsp=uaCreateRspFromReq(req,sipOK,NULL,NULL);*/
				/*modify to-tag,it should be the same as previous 487*/
				/*sipRspAddHdr(rsp,To,invitecancelto);
				if(rsp){
					sipTxSendRsp(_tx,rsp);
				}*/
				uaDlgSetState(origdlg,UACSTATE_DISCONNECT);
				/* free inviteTx */
				uaDlgDelInviteTx(origdlg);
				uaMgrPostNewMessage(uaDlgGetMgr(origdlg),origdlg,UAMSG_CALLSTATE,uaGetRspCSeq(rsp),_content,NULL);
			}
			break;
			default:
				rsp=uaCreateRspFromReq(req,sipOK,NULL,NULL);
				if(rsp){
					uaDlgSetResponse(origdlg,rsp);
					sipTxSendRsp(_tx,rsp);
				}
				break;
			}/* end w*/
		}
		break;
	case BYE:
		/*should match dialog*/
		if((origdlg==NULL)||(uaGetReqToTag(req)==NULL)){
			/*can't find a match dialog*/
			UaCoreERR("[uaProcessReqTx] bye's original dialog is not found in mgr list!\n");
			rsp=uaCreateRspFromReq(req,Call_Leg_Transaction_Does_Not_Exist,NULL,NULL);
			sipTxSendRsp(_tx,rsp);
			rCode=UATX_DLG_NULL;
			break;
		}else{
			/*match a original dialog*/
			dlgstate=uaDlgGetState(origdlg);
			/* if((dlgstate != UACSTATE_CONNECTED)&&(dlgstate !=UACSTATE_ONHELD)){ modified by tyhuang ,2003.09.03 */
			switch(dlgstate){
			case UACSTATE_CONNECTED:
			case UACSTATE_ONHELD:
			case UACSTATE_ACCEPT:
				/* check if timer is set*/
				if(IsDlgTimerd(origdlg)==TRUE){
					uaDelTimer(uaDlgGetTimerHandle(origdlg));
					SetDlgTimerd(origdlg,FALSE);	
				}
				/*real to disconnect it */
				rsp=uaCreateRspFromReq(req,sipOK,NULL,NULL);
				if(rsp){
					uaDlgSetResponse(origdlg,rsp);
					sipTxSendRsp(_tx,rsp);
				}
#ifndef wlca_client
				uaDlgSetState(origdlg,UACSTATE_DISCONNECT);
#else
				if(_content)
					uaDlgSetState(origdlg,UACSTATE_ONHELD);
				else
					uaDlgSetState(origdlg,UACSTATE_DISCONNECT);
#endif
				/* free inviteTx */
				uaDlgDelInviteTx(origdlg);
				uaMgrPostNewMessage(uaDlgGetMgr(origdlg),origdlg,UAMSG_CALLSTATE,uaGetRspCSeq(rsp),_content,NULL);
				break;
			break;
			default:
				/*this means can't Disconnect this call*/
				rsp=uaCreateRspFromReq(req,Call_Leg_Transaction_Does_Not_Exist,NULL,NULL);
				if(rsp){
					uaDlgSetResponse(origdlg,rsp);
					sipTxSendRsp(_tx,rsp);
				}
				break;
			}
		
		}
		break;
	case ACK:
		UaCoreERR("[uaProcessReqTx] Error! Receive ACK Message!\n");
		break;
	case OPTIONS:
		if(origdlg==NULL){
			/*can't find a match dialog, it is correct*/
			rsp=uaCreateRspFromReq(req,sipOK,NULL,NULL);
			sipTxSendRsp(_tx,rsp);
			/*transaction how to free ?? --> when transaction terminated*/
		}else{
			/*if original transaction is exist*/
			/*get original SDP */
			SdpSess origsdp=NULL;

			origsdp=uaDlgGetLocalSdp(origdlg);
			rsp=uaCreateRspFromReq(req,sipOK,NULL,origsdp);
			if(origsdp != NULL)
				sdpSessFree(origsdp);
			if(rsp){
				uaDlgSetResponse(origdlg,rsp);
				sipTxSendRsp(_tx,rsp);
			}
			/*not necessary to add into dialog*/
			/*when transaction is terminated, free it*/
		}
		break;

	case REGISTER:
#ifndef _Server
		UaCoreERR("[uaProcessReqTx] Error! Receive REGISTER Message!\n");
		rsp=uaCreateRspFromReq(req,Method_Not_Allowed,NULL,NULL);
		if(rsp)
			sipTxSendRsp(_tx,rsp);
		sipTxFree(_tx);
#else
		if(origdlg==NULL){
			/*can't find a match dialog, create a new for it*/
			newdlg=uaDlgNew(mgr);
			/* set remote displayname,addr,target */
			uaDlgSetRemoteDisplayname(newdlg,(char*)uaGetReqFromDisplayname(req));
			uaDlgSetRemoteAddr(newdlg,uaGetReqFromAddr(req));
			uaDlgSetRemoteTarget(newdlg,uaGetReqContact(req));
			uaDlgSetMethod(newdlg,reqMethod);
			SetDlgServer(newdlg,TRUE);
			/*uaDlgSetState(newdlg,UACSTATE_RECEIVE_MESSAGE);*/
			uaDlgSetCallID(newdlg,(char*)sipReqGetHdr(req,Call_ID));
			/* put new dialog in mgr */
			dxLstPutTail(uaMgrGetDlgLst(mgr),newdlg);
			TCRPrint(TRACE_LEVEL_API,"[uaProcessReqTx] uaDlg count = %d\n",dxLstGetSize(uaMgrGetDlgLst(mgr)));
			
			rsp=uaCreateRspFromReq(req,sipOK,(char*)uaGetReqContact(req),NULL);
			if(rsp==NULL){
			/*discard the message */
				rCode=UATX_REQ_MSG_ERROR;
				UaCoreERR("[uaProcessReqTx] Create Response for REGISTER message NULL!\n");
			}else{
				uaDlgSetResponse(newdlg,rsp);
				sipTxSendRsp(_tx,rsp);
			}
			
			uaMgrPostNewMessage(uaDlgGetMgr(newdlg),newdlg,UAMSG_REGISTER,uaGetReqCSeq(req),_content,NULL);
		
		}else{
			rCode=uaDlgSetRemoteTarget(origdlg,uaGetReqContact(req));
			rsp=uaCreateRspFromReq(req,sipOK,(char*)uaGetReqContact(req),NULL);
			if(rsp==NULL){
			/*discard the message */
				rCode=UATX_REQ_MSG_ERROR;
				UaCoreERR("[uaProcessReqTx] Create Response for REGISTER message NULL!\n");
			}else{
				uaDlgSetResponse(origdlg,rsp);
				sipTxSendRsp(_tx,rsp);
			}
			uaMgrPostNewMessage(uaDlgGetMgr(origdlg),origdlg,UAMSG_REGISTER,uaGetReqCSeq(req),_content,NULL);
		
		}
#endif
		break;
	case REFER:/*for 3rd party call control or transfer*/
		{
		char *contacturl;

		/*match into original dialog, -->unattended transfer*/
		/*didn't change original dialog's state*/
		if(NULL !=origdlg){

			if(BeSIPSURL(uaGetReqReqURI(req))==TRUE)
				uaDlgSetSecureFlag(origdlg);
			uaDlgSetRemoteTarget(origdlg,uaGetReqContact(req));

			/*assume each REFER message is accepted, response 202 Accepted*/
			if(uaDlgGetSecureFlag(origdlg)==TRUE){
				contacturl=(char*)uaUserGetSIPSContact(uaMgrGetUser(mgr));
				if(contacturl)
					rsp=uaCreateRspFromReq(req,Accepted,contacturl,NULL);
				else{
					UaCoreERR("[uaREFERMsg] SIPS contact is not found!\n");
					rsp=NULL;
				}
			}else
				rsp=uaCreateRspFromReq(req,Accepted,(char*)uaMgrGetContactAddr(mgr),NULL);

			if(NULL != rsp)
				sipTxSendRsp(_tx,rsp);

			if(cseq)
				csnew=cseq->seq;
			uaDlgSetRemoteCseq(newdlg,csnew);
			uaDlgAddTxIntoTxList(origdlg,_tx);
			/*should notify user to create a new transaction */
			uaMgrPostNewMessage(uaDlgGetMgr(origdlg),origdlg,UAMSG_REFER,uaGetReqCSeq(req),_content,NULL);

			/*notify original application*/
			/*
			if(uaMsgGetReqReferredByAddr(origdlg,buf,&buflen)==RC_OK){
				notifyReq=uaNOTIFYMsg(origdlg,buf,notifycontent);
				if(notifyReq!=NULL){
					pfile=uaCreateUserProfile(cfg);
					if(pfile == NULL)
						return UAUSER_PROFILE_NULL;
					notifytx=sipTxClientNew(notifyReq,NULL,pfile,origdlg);
					if(notifytx == NULL){
						sipReqFree(notifyReq);
						return UATX_NULL;
					}
					uaUserProfileFree(pfile);
					uaDlgAddTxIntoTxList(origdlg,notifytx);
				}
			}
			*/
		}else{ 
			/*orig dlalog is NULL*/
			/*It maybe Attended Transfer or first REFER message*/
			newdlg=uaDlgNew(mgr);
			/* set remote displayname,addr,target */
			if(BeSIPSURL(uaGetReqReqURI(req))==TRUE)
				uaDlgSetSecureFlag(newdlg);
			uaDlgSetRemoteDisplayname(newdlg,(char*)uaGetReqFromDisplayname(req));
			uaDlgSetRemoteAddr(newdlg,uaGetReqFromAddr(req));
			uaDlgSetRemoteTarget(newdlg,uaGetReqContact(req));
			uaDlgSetMethod(newdlg,reqMethod);
			SetDlgServer(newdlg,TRUE);
			
			uaDlgAddTxIntoTxList(newdlg,_tx);
			/*uaDlgSetState(newdlg,UACSTATE_REFER);*/
			uaDlgSetCallID(newdlg,(char*)sipReqGetHdr(req,Call_ID));

			/*assume each REFER message is accepted, response 202 Accepted*/
			if(uaDlgGetSecureFlag(origdlg)==TRUE){
				contacturl=(char*)uaUserGetSIPSContact(uaMgrGetUser(mgr));
				if(contacturl)
					rsp=uaCreateRspFromReq(req,Accepted,contacturl,NULL);
				else{
					UaCoreERR("[uaREFERMsg] SIPS contact is not found!\n");
					rsp=NULL;
				}
			}else
				rsp=uaCreateRspFromReq(req,Accepted,(char*)uaMgrGetContactAddr(mgr),NULL);

			if(NULL != rsp)
				sipTxSendRsp(_tx,rsp);

			sipTxSetUserData(_tx,newdlg);
			dxLstPutTail(uaMgrGetDlgLst(mgr),newdlg);
			TCRPrint(TRACE_LEVEL_API,"[uaProcessReqTx] uaDlg count = %d\n",dxLstGetSize(uaMgrGetDlgLst(mgr)));
			
			uaMgrPostNewMessage(uaDlgGetMgr(newdlg),newdlg,UAMSG_REFER,uaGetReqCSeq(req),_content,NULL); 

			/*notify original application*/
			/*
			if(uaMsgGetReqReferredByAddr(newdlg,buf,&buflen)==RC_OK){
				notifyReq=uaNOTIFYMsg(newdlg,buf,notifycontent);
				if(notifyReq!=NULL){
					pfile=uaCreateUserProfile(cfg);
					if(pfile == NULL)
						return UAUSER_PROFILE_NULL;
					notifytx=sipTxClientNew(notifyReq,NULL,pfile,newdlg);
					if(notifytx == NULL){
						sipReqFree(notifyReq);
						return UATX_NULL;
					}
					uaUserProfileFree(pfile);
					uaDlgAddTxIntoTxList(origdlg,notifytx);
				}
			}
			*/
		}

		}
		break;
	case SIP_NOTIFY:{
		SipSubState *sipsub=NULL;
		char *url=NULL,*reason=NULL;
		char eventbuf[128];
		unsigned short expires=0;
		UaEvtpkg tmpevtpkg=NULL;
		UaSub tmpsub=NULL; 
		UASubState state;
		RCODE rcode;
		
		int pos;
		char *mgrusrname=NULL;
		char *requsername=NULL;

		event=(SipEvent*)sipReqGetHdr(req,Event);
		if(event&& event->pack &&(strICmp(event->pack,"presence")==0)){
			if (event->tmplate && event->tmplate->str) 
				sprintf(eventbuf,"%s.%s",event->pack,event->tmplate->str);
			else
				sprintf(eventbuf,"%s",event->pack);
			/* tmpevtpkg=uaMgrGetEvtpkg(mgr,event->pack); */
			
			/* for sj ualib */
			requsername=(char*)uaGetReqCallee(req);
			for(pos=0;pos<dxLstGetSize(MgrList);pos++){
				mgr=dxLstPeek(MgrList,pos);
				mgrusrname=uaMgrGetUserName(mgr);
				if(strICmp(requsername,mgrusrname)==0)
					tmpevtpkg=uaMgrGetEvtpkg(mgr,eventbuf);
				if(tmpevtpkg) break;
			}
			
			if(NULL==tmpevtpkg)
				origdlg = NULL;

			url=uaGetReqFromAddr(req);
			sipsub=sipReqGetHdr(req,Subscription_State);
			if(sipsub){
				expires=sipsub->expires;
				state=uaSubStateFromStr(sipsub->state);
			}
			if(origdlg==NULL){
				/*can't find a match dialog, send 481 for it*/
				rsp=uaCreateRspFromReq(req,Call_Leg_Transaction_Does_Not_Exist,NULL,NULL);
				sipTxSendRsp(_tx,rsp);
				rCode=UATX_DLG_NULL;
				break;
				
			}else{
				tmpsub=uaSubNew(url,expires,state,origdlg,tmpevtpkg,FALSE,FALSE);
				if(state==UASUB_TERMINATED)
					rcode=uaSubSetSubstate(tmpsub,UASUB_TERMINATED,sipsub->reason);
				/*uaDlgSetState(newdlg,UACSTATE_RECEIVE_NOTIFY);*/
				/* uaDlgAddTxIntoTxList(origdlg,_tx);  mark by tyhuang 2004.3.5 */
				/*sipTxSetUserData(_tx,origdlg);  added by ljchuang 07/14 */
				/*uaMgrPostNewSubMsg(uaDlgGetMgr(origdlg),origdlg,UAMSG_NOTIFY,uaGetReqCSeq(req),_content,tmpsub);*/
				
			}
			/* set remove tag */
			if( uaDlgGetRemoveTag(origdlg) == NULL)
				uaDlgSetRemoveTag(origdlg,uaGetReqFromTag(req));
			/* create response */
			rsp=uaCreateRspFromReq(req,sipOK,NULL,NULL);

			if(rsp != NULL){
				uaDlgSetResponse(origdlg,rsp);
				sipTxSendRsp(_tx,rsp);
			}
			uaMgrPostNewSubMsg(uaDlgGetMgr(origdlg),origdlg,UAMSG_NOTIFY,uaGetReqCSeq(req),_content,tmpsub);
			if(tmpsub!=NULL)
					uaSubFree(tmpsub);
			break;
		}
		if(origdlg==NULL){
			/*can't find a match dialog, send 481 for it*/
			rsp=uaCreateRspFromReq(req,Call_Leg_Transaction_Does_Not_Exist,NULL,NULL);
			sipTxSendRsp(_tx,rsp);
			rCode=UATX_DLG_NULL;		
		}else{
			event=(SipEvent*)sipReqGetHdr(req,Event);
			if(event){
				if(strICmp(event->pack,"refer")==0){	/*for transfer*/
					/*rsp=uaCreateRspFromReq(req,Accepted,(char*)uaMgrGetContactAddr(mgr),NULL);*/
					rsp=uaCreateRspFromReq(req,sipOK,NULL,NULL);
					/* check if subscription-state=terminated */
					sipsub=sipReqGetHdr(req,Subscription_State);
					if(sipsub){
						if(strICmp(sipsub->state,"terminated")==0){
							/* disconnect old call */
							uaDlgDisconnect(origdlg);
						}
					}
					
				}
			}
			if(rsp != NULL){
				uaDlgSetResponse(origdlg,rsp);
				sipTxSendRsp(_tx,rsp);
			}
			uaMgrPostNewMessage(uaDlgGetMgr(origdlg),origdlg,UAMSG_NOTIFY,uaGetReqCSeq(req),_content,NULL);
		}
		break;
	}
	case SIP_SUBSCRIBE:{
		SipReqLine *pLine=NULL;
		char name[128],*url=NULL,*contacturl;
		UaEvtpkg tmpevtpkg=NULL;
		UaSub tmpsub=NULL; 
		UAStatusCode statuscode;
		SubEvtCB subCB=NULL;

		/* check subscribe req */
		event=(SipEvent*)sipReqGetHdr(req,Event);
		/*name=strDup(event->pack);*/
		if (event->pack)
			if(event->tmplate && event->tmplate->str) 
				sprintf(name,"%s.%s",event->pack,event->tmplate->str);
			else
				sprintf(name,"%s",event->pack);
		exp=sipReqGetHdr(req,Expires);
		if(exp)
			expires=(unsigned short)exp->expireSecond;
		else
			expires=600;
		pLine=sipReqGetReqLine(req);
		url=pLine->ptrRequestURI;
		/* match url in class of event packages */
		
		tmpevtpkg=uaMgrGetEvtpkg(mgr,name);
		/*
		tmpclass=uaEvtpkgGetClass(tmpevtpkg,url);
		if(tmpclass == NULL){
			UaCoreERR("[uaProcessReqTx] Event not found !\n");
			rsp=uaCreateRspFromReq(req,Bad_Event,(char*)uaMgrGetContactAddr(mgr),NULL);
			if(rsp != NULL)
				sipTxSendRsp(_tx,rsp);
			break;
		}*/
		
		if(origdlg==NULL){
			/*can't find a match dialog, create a new for it*/
			newdlg=uaDlgNew(mgr);
			/* set remote displayname,addr,target */
			if(BeSIPSURL(uaGetReqReqURI(req))==TRUE)
				uaDlgSetSecureFlag(newdlg);
			uaDlgSetRemoteDisplayname(newdlg,(char*)uaGetReqFromDisplayname(req));
			uaDlgSetRemoteAddr(newdlg,uaGetReqFromAddr(req));
			uaDlgSetRemoteTarget(newdlg,uaGetReqContact(req));
			uaDlgSetMethod(newdlg,reqMethod);
			SetDlgServer(newdlg,TRUE);
			cseq=sipReqGetHdr(req,CSeq);
			if(cseq)
				csnew=cseq->seq;
			uaDlgSetRemoteCseq(newdlg,csnew);
			uaDlgAddTxIntoTxList(newdlg,_tx);
			/*uaDlgSetState(newdlg,UACSTATE_RECEIVE_SUBSCRIBE);*/
			uaDlgSetCallID(newdlg,(char*)sipReqGetHdr(req,Call_ID));
			sipTxSetUserData(_tx,newdlg);
			/* put new dialog in mgr */
			dxLstPutTail(uaMgrGetDlgLst(mgr),newdlg);
			TCRPrint(TRACE_LEVEL_API,"[uaProcessReqTx] uaDlg count = %d\n",dxLstGetSize(uaMgrGetDlgLst(mgr)));
			/* new uasub for post message to application */
			tmpsub=uaSubNew(url,expires,UASUB_NULL,newdlg,tmpevtpkg,FALSE,TRUE);
			/* use callback function to decide what to answer */
			subCB=uaEvtpkgGetSubCB(tmpevtpkg);
			if (subCB)
			{
				switch ( subCB(tmpsub) )
				{
				case UASUB_ACTIVE:
					statuscode = sipOK;
					break;

				case UASUB_PENDING:
					statuscode = Accepted;
					break;

				case UASUB_NULL:
				case UASUB_TERMINATED:
					statuscode = Forbidden;
					break;
				}
			} else {
				statuscode = Accepted;
			}
			
			if(uaDlgGetSecureFlag(origdlg)==TRUE){
				contacturl=(char*)uaUserGetSIPSContact(uaMgrGetUser(mgr));
				if(contacturl)
					rsp=uaCreateRspFromReq(req,statuscode,contacturl,NULL);
				else{
					UaCoreERR("SIPS contact is not found!\n");
					rsp=NULL;
				}
			}else
				rsp=uaCreateRspFromReq(req,statuscode,(char*)uaMgrGetContactAddr(mgr),NULL);

			if(rsp==NULL){
			/*discard the message */
				rCode=UATX_REQ_MSG_ERROR;
				UaCoreERR("[uaProcessReqTx] Create Response NULL!\n");
			}else{
				uaDlgSetResponse(newdlg,rsp);
				sipTxSendRsp(_tx,rsp);
			}
		
			uaMgrPostNewSubMsg(uaDlgGetMgr(newdlg),newdlg,UAMSG_SUBSCRIBE,uaGetReqCSeq(req),_content,tmpsub);
			if(tmpsub){
				uaSubFree(tmpsub);
				tmpsub = NULL;
			}
		}else{
			if(BeSIPSURL(uaGetReqReqURI(req))==TRUE)
				uaDlgSetSecureFlag(origdlg);
			uaDlgSetRemoteTarget(origdlg,uaGetReqContact(req));
			/*uaDlgSetState(origdlg,UACSTATE_RECEIVE_SUBSCRIBE);*/
			tmpsub=uaSubNew(url,expires,UASUB_NULL,origdlg,tmpevtpkg,FALSE,TRUE);		
			/*if original transaction is exist*/
			uaDlgAddTxIntoTxList(origdlg,_tx);
			/* create a response */
			/* use callback function to decide what to answer */
			subCB=uaEvtpkgGetSubCB(tmpevtpkg);
			if (subCB)
			{
				switch ( subCB(tmpsub) )
				{
				case UASUB_ACTIVE:
					statuscode = sipOK;
					break;

				case UASUB_PENDING:
					statuscode = Accepted;
					break;

				case UASUB_NULL:
				case UASUB_TERMINATED:
					statuscode = Forbidden;
					break;
				}
			} else {
				statuscode = Accepted;
			}
			if(uaDlgGetSecureFlag(origdlg)==TRUE){
				contacturl=(char*)uaUserGetSIPSContact(uaMgrGetUser(mgr));
				if(contacturl)
					rsp=uaCreateRspFromReq(req,statuscode,contacturl,NULL);
				else{
					UaCoreERR("SIPS contact is not found!\n");
					rsp=NULL;
				}
			}else
				rsp=uaCreateRspFromReq(req,statuscode,(char*)uaMgrGetContactAddr(mgr),NULL);

			if(rsp==NULL){
			/*discard the message */
				rCode=UATX_REQ_MSG_ERROR;
				UaCoreERR("[uaProcessReqTx] Create Response NULL!\n");
			}else{
				uaDlgSetResponse(origdlg,rsp);
				sipTxSendRsp(_tx,rsp);
			}
			
			uaMgrPostNewSubMsg(uaDlgGetMgr(origdlg),origdlg,UAMSG_SUBSCRIBE,uaGetReqCSeq(req),_content,tmpsub);
		
			if(tmpsub){
				uaSubFree(tmpsub);
				tmpsub = NULL;
			}
		}
		break;
	}	
	case SIP_PUBLISH:
		if(origdlg==NULL){
			/*can't find a match dialog, create a new for it*/
			newdlg=uaDlgNew(mgr);
			/* set remote displayname,addr,target */
			uaDlgSetRemoteDisplayname(newdlg,(char*)uaGetReqFromDisplayname(req));
			uaDlgSetRemoteAddr(newdlg,uaGetReqFromAddr(req));
			uaDlgSetRemoteTarget(newdlg,uaGetReqContact(req));
			uaDlgSetMethod(newdlg,reqMethod);
			SetDlgServer(newdlg,TRUE);
		
			/*uaDlgAddTxIntoTxList(newdlg,_tx); mark by tyhuang 2004.3.5 */
			/*uaDlgSetState(newdlg,UACSTATE_RECEIVE_PUBLISH);*/
			uaDlgSetCallID(newdlg,(char*)sipReqGetHdr(req,Call_ID));
			
			/*sipTxSetUserData(_tx,newdlg);*/
			/* put new dialog in mgr */
			dxLstPutTail(uaMgrGetDlgLst(mgr),newdlg);
			TCRPrint(TRACE_LEVEL_API,"[uaProcessReqTx] uaDlg count = %d\n",dxLstGetSize(uaMgrGetDlgLst(mgr)));
			
			rsp=uaCreateRspFromReq(req,sipOK,NULL,NULL);
			if(rsp==NULL){
			/*discard the message */
				rCode=UATX_REQ_MSG_ERROR;
				UaCoreERR("[uaProcessReqTx] Create Response NULL!\n");
			}else{
				uaDlgSetResponse(newdlg,rsp);
				sipTxSendRsp(_tx,rsp);
			}
			/* post message */
		
			uaMgrPostNewMessage(uaDlgGetMgr(newdlg),newdlg,UAMSG_PUBLISH,uaGetReqCSeq(req),_content,NULL);
			
		}else{
		
			rsp=uaCreateRspFromReq(req,sipOK,NULL,NULL);
			if(rsp==NULL){
			/*discard the message */
				rCode=UATX_REQ_MSG_ERROR;
				UaCoreERR("[uaProcessReqTx] Create Response NULL!\n");
			}else{
				uaDlgSetResponse(origdlg,rsp);
				sipTxSendRsp(_tx,rsp);
			}
			
			uaMgrPostNewMessage(uaDlgGetMgr(origdlg),origdlg,UAMSG_PUBLISH,uaGetReqCSeq(req),_content,NULL);
		
		}
		break;
	case SIP_MESSAGE:
		if(origdlg==NULL){
			/*can't find a match dialog, create a new for it*/
			newdlg=uaDlgNew(mgr);
			/* set remote displayname,addr,target */
			uaDlgSetRemoteDisplayname(newdlg,(char*)uaGetReqFromDisplayname(req));
			uaDlgSetRemoteAddr(newdlg,uaGetReqFromAddr(req));
			uaDlgSetRemoteTarget(newdlg,uaGetReqContact(req));
			uaDlgSetMethod(newdlg,reqMethod);
			SetDlgServer(newdlg,TRUE);
			
			/*uaDlgAddTxIntoTxList(newdlg,_tx); mark by tyhuang 2004.3.5 */
			/*uaDlgSetState(newdlg,UACSTATE_RECEIVE_MESSAGE);*/
			uaDlgSetCallID(newdlg,(char*)sipReqGetHdr(req,Call_ID));

			/*sipTxSetUserData(_tx,newdlg);*/
			/* put new dialog in mgr */
			dxLstPutTail(uaMgrGetDlgLst(mgr),newdlg);
			TCRPrint(TRACE_LEVEL_API,"[uaProcessReqTx] uaDlg count = %d\n",dxLstGetSize(uaMgrGetDlgLst(mgr)));
			
			rsp=uaCreateRspFromReq(req,sipOK,NULL,NULL);
			if(rsp==NULL){
			/*discard the message */
				rCode=UATX_REQ_MSG_ERROR;
				UaCoreERR("[uaProcessReqTx] Create Response NULL!\n");
			}else{
				uaDlgSetResponse(newdlg,rsp);
				sipTxSendRsp(_tx,rsp);
			}
			
			uaMgrPostNewMessage(uaDlgGetMgr(newdlg),newdlg,UAMSG_MESSAGE,uaGetReqCSeq(req),_content,NULL);
		
		}else{
			/*uaDlgSetState(origdlg,UACSTATE_RECEIVE_MESSAGE);*/
			/*if original transaction is exist*/
			/*uaDlgAddTxIntoTxList(origdlg,_tx);  mark by tyhuang 2004.3.5 */
			
			rsp=uaCreateRspFromReq(req,sipOK,NULL,NULL);
			if(rsp==NULL){
			/*discard the message */
				rCode=UATX_REQ_MSG_ERROR;
				UaCoreERR("[uaProcessReqTx] Create Response NULL!\n");
			}else{
				uaDlgSetResponse(origdlg,rsp);
				sipTxSendRsp(_tx,rsp);
			}
			uaMgrPostNewMessage(uaDlgGetMgr(origdlg),origdlg,UAMSG_MESSAGE,uaGetReqCSeq(req),_content,NULL);
		
		}
		break;
	case SIP_INFO:
		if(origdlg==NULL){
			/*can't find a match dialog, create a new for it*/
			newdlg=uaDlgNew(mgr);
			/* set remote displayname,addr,target */
			uaDlgSetRemoteDisplayname(newdlg,(char*)uaGetReqFromDisplayname(req));
			uaDlgSetRemoteAddr(newdlg,uaGetReqFromAddr(req));
			uaDlgSetRemoteTarget(newdlg,uaGetReqContact(req));
			uaDlgSetMethod(newdlg,reqMethod);
			SetDlgServer(newdlg,TRUE);
			uaDlgSetRemoteCseq(newdlg,csnew);
			uaDlgAddTxIntoTxList(newdlg,_tx); 
			/*uaDlgSetState(newdlg,UACSTATE_RECEIVE_MESSAGE);*/
			uaDlgSetCallID(newdlg,(char*)sipReqGetHdr(req,Call_ID));

			sipTxSetUserData(_tx,newdlg);
			/* put new dialog in mgr */
			dxLstPutTail(uaMgrGetDlgLst(mgr),newdlg);
			TCRPrint(TRACE_LEVEL_API,"[uaProcessReqTx] uaDlg count = %d\n",dxLstGetSize(uaMgrGetDlgLst(mgr)));
			uaMgrPostNewMessage(uaDlgGetMgr(newdlg),newdlg,UAMSG_SIP_INFO,uaGetReqCSeq(req),_content,_tx);
#ifndef CCL_DISABLE_AUTOSEND 
			rsp=uaCreateRspFromReq(req,sipOK,NULL,NULL);
			if(rsp==NULL){
			/*discard the message */
				rCode=UATX_REQ_MSG_ERROR;
				UaCoreERR("[uaProcessReqTx] Create Response NULL!\n");
			}else{
				uaDlgSetResponse(newdlg,rsp);
				sipTxSendRsp(_tx,rsp);
			}
#endif		
		}else{
			/*uaDlgSetState(origdlg,UACSTATE_RECEIVE_MESSAGE);*/
			/*if original transaction is exist*/
			uaDlgAddTxIntoTxList(origdlg,_tx);
			sipTxSetUserData(_tx,origdlg);
			uaMgrPostNewMessage(uaDlgGetMgr(origdlg),origdlg,UAMSG_SIP_INFO,uaGetReqCSeq(req),_content,_tx);
#ifndef CCL_DISABLE_AUTOSEND 
			rsp=uaCreateRspFromReq(req,sipOK,NULL,NULL);
			if(rsp==NULL){
			/*discard the message */
				rCode=UATX_REQ_MSG_ERROR;
				UaCoreERR("[uaProcessReqTx] Create Response NULL!\n");
			}else{
				uaDlgSetResponse(origdlg,rsp);
				sipTxSendRsp(_tx,rsp);
			}
#endif	
		}
		break;
	case SIP_UPDATE:
	case PRACK:
	{
		int setype;
		SipSessionExpires *pSE=NULL;
		char *contacturl;
		/*can't find a match dialog, create a new for it*/
		if(origdlg==NULL){				
			if((origdlg=uaDlgNew(mgr))==NULL){
				UaCoreERR("[uaProcessReqTx] Create New dialog is failed!\n");
				break;
			}
			/* set method */
			uaDlgSetMethod(origdlg,reqMethod);
			/* set callid */
			uaDlgSetCallID(origdlg,(char*)sipReqGetHdr(req,Call_ID));
			/* put new dialog in mgr */
			dxLstPutTail(uaMgrGetDlgLst(mgr),newdlg);
		}
		/* set remote displayname,addr,target */
		if(BeSIPSURL(uaGetReqReqURI(req))==TRUE)
			uaDlgSetSecureFlag(origdlg);
		uaDlgSetRemoteDisplayname(origdlg,(char*)uaGetReqFromDisplayname(req));
		uaDlgSetRemoteAddr(origdlg,uaGetReqFromAddr(req));
		uaDlgSetRemoteTarget(origdlg,uaGetReqContact(req));
		SetDlgServer(origdlg,TRUE);
		cseq=sipReqGetHdr(req,CSeq);
		if(cseq)
			csnew=cseq->seq;
		uaDlgSetRemoteCseq(origdlg,csnew);

		/* bind tx and dialog */
		uaDlgAddTxIntoTxList(origdlg,_tx); 
		sipTxSetUserData(_tx,origdlg);
		
		TCRPrint(TRACE_LEVEL_API,"[uaProcessReqTx] uaDlg count = %d\n",dxLstGetSize(uaMgrGetDlgLst(mgr)));
		if(reqMethod==SIP_UPDATE){
			/* refresh timer session expire timer */
			setype=uaDlgGetSEType(origdlg);
			if(setype>0){
				pSE=(SipSessionExpires*)sipReqGetHdr(req,Session_Expires);
				if(pSE){
					uaDelTimer(uaDlgGetSEHandle(origdlg));
					uaDlgSetSEHandle(origdlg,3,uaAddTimer(origdlg,SESSION_TIMER,pSE->deltasecond *1000));
				}
			}
			uaMgrPostNewMessage(uaDlgGetMgr(newdlg),origdlg,UAMSG_SIP_UPDATE,uaGetReqCSeq(req),_content,NULL);
		}else 
			uaMgrPostNewMessage(uaDlgGetMgr(newdlg),origdlg,UAMSG_SIP_PRACK,uaGetReqCSeq(req),_content,NULL);

		/* send 200 ok back */
		if(reqMethod==PRACK)
			rsp=uaCreateRspFromReq(req,sipOK,NULL,NULL);
		else{
			if(uaDlgGetSecureFlag(origdlg)==TRUE){
				contacturl=(char*)uaUserGetSIPSContact(uaMgrGetUser(mgr));
				if(contacturl)
					rsp=uaCreateRspFromReq(req,sipOK,contacturl,NULL);
				else{
					UaCoreERR("SIPS contact is not found!\n");
					rsp=NULL;
				}
			}else
				rsp=uaCreateRspFromReq(req,sipOK,(char*)uaMgrGetContactAddr(mgr),NULL);
		}
		if(rsp==NULL){
				/*discard the message */
				rCode=UATX_REQ_MSG_ERROR;
				UaCoreERR("[uaProcessReqTx] Create Response NULL!\n");
		}else{
				uaDlgSetResponse(origdlg,rsp);
				sipTxSendRsp(_tx,rsp);
		}
		break;
	}
	case UNKNOWN_METHOD:
	default:
		rsp=uaCreateRspFromReq(req,Not_Implemented,NULL,NULL);
		rCode=UATX_REQMETHOD_UNKNOWN;
		if(rsp==NULL){
			/*discard the message */
			rCode=UATX_REQ_MSG_ERROR;
			UaCoreERR("[uaProcessReqTx] Create Response NULL!\n");
		}else{
				sipTxSendRsp(_tx,rsp);
		}
		break;
	}
	
	if(_content){
		uaContentFree(_content);
		_content=NULL;
	}
	return rCode;
}

/*when get a response message*/
RCODE uaProcessRspTx(TxStruct _tx)
{
	RCODE rCode=RC_OK;
	UaDlg parentDlg=NULL,refdlg=NULL;
	SipReq origReq=NULL;
	SipRsp rsp=NULL;
	UAStatusCode rspcode=Trying;
	SipMethodType rspMethod=UNKNOWN_METHOD,origMethod=UNKNOWN_METHOD;
	UaMgr mgr=NULL;
	BOOL  bPostMsg=FALSE;
	SipReq newInvReq=NULL; /* for new transaction request message*/
	TxStruct newInvTx=NULL;/* create a new transaction, www-authenticate, redirect, proxy-authenticate*/
	UserProfile pProfile=NULL;
	SipContact* pContact=NULL; /*when redirect*/
	char* contactbuf=NULL,buf[256]={'\0'},*reqtotag,*reqfromtag,*rsptotag,*rspfromtag;
	SipReqLine *pReqLine=NULL;
	SipCSeq *newCSeq=NULL;
	SipVia *pvia=NULL;
	SipSessionExpires *pSE=NULL;
	UaUser user=NULL;
	UaContent _content=NULL;
	int expires=0,mgrnum,mgrpos,reqcseq,rspcseq;
	SipReq ackMsg=NULL;
	TxStruct ackTx=NULL;

	parentDlg=sipTxGetUserData(_tx);
	/*response should have a dialog exist*/
	if(parentDlg == NULL){
		/* match tx by uacore when trasaction can't  */
		mgrnum=dxLstGetSize(MgrList);
		for(mgrpos=0;mgrpos<mgrnum;mgrpos++){
			mgr=dxLstPeek(MgrList,mgrpos);
			parentDlg=uaDlgMatchTx(mgr,_tx);
			if(parentDlg) break;
		}
			
		if(parentDlg == NULL){
			UaCoreERR("[uaProcessRspTx] parent dialog is not found!\n");
			return UATX_PARENTDLG_NOT_FOUND;
		}
		origReq=uaDlgGetRequest(parentDlg);
	}
	/*get original request message*/
	if(origReq==NULL){
		origReq=sipTxGetOriginalReq(_tx);
		if(origReq == NULL){
			UaCoreERR("[uaProcessRspTx] can't get original request!\n");
			return UATX_REQ_NOT_FOUND;
		}
	}
	/*get final response message*/
	rsp=sipTxGetLatestRsp(_tx);
	if(rsp == NULL)
		return UATX_RSP_NOT_FOUND;

	/*get status code*/
	rspcode=uaGetRspStatusCode(rsp);

	/* check number of via header,it must be 1 */
	pvia=(SipVia*)sipRspGetHdr(rsp,Via);
	if(pvia){
		if(pvia->numParams > 1){
			UaCoreERR("[uaProcessRspTx] Too many Via Headers!\n");
			return UASIPMSG_ERROR;
		}
	}

	/* if parentDlg is not null,should check to-tag and from-tag 
	 * if not match ,discard it.
	 */
	reqfromtag=uaGetReqFromTag(origReq);

	rspfromtag=uaGetRspFromTag(rsp);
	if(reqfromtag && rspfromtag){
		if(strcmp(reqfromtag,rspfromtag)!=0){
			UaCoreERR("[uaProcessRspTx] from-tag does not match !\n");
			return UATX_REQ_NOT_FOUND;
		}
		reqtotag=uaGetReqToTag(origReq);
		if(reqtotag){
			rsptotag=uaGetRspToTag(rsp);
			if(rsptotag){/*with to-tag */
				if(strcmp(reqtotag,rsptotag)!=0){
					UaCoreERR("[uaProcessRspTx] to-tag does not match !\n");
					return UATX_REQ_NOT_FOUND;
				}
			}else{
				UaCoreERR("[uaProcessRspTx] to-tag in response is null !\n");
				return UATX_REQ_NOT_FOUND;
			}
		}
	}
	/*get method */
	origMethod=uaGetReqMethod(origReq);
	rspMethod=uaGetRspMethod(rsp);

	/*check method */
	if((rspMethod == UNKNOWN_METHOD)||(origMethod == UNKNOWN_METHOD)||(rspMethod!=origMethod)){
		UaCoreERR("[uaProcessRspTx] method is not match or unknow!\n");
		return UATX_RSPMETHOD_UNKNOWN;
	}
	/*check cseq */
	reqcseq=uaGetReqCSeq(origReq);
	rspcseq=uaGetRspCSeq(rsp);
	if(reqcseq > rspcseq){
		return UATX_REQ_NOT_FOUND;
	}
	if((mgr=uaDlgGetMgr(parentDlg))==NULL){
		UaCoreERR("[uaProcessRspTx] no manager is found !\n");
		return UATX_MGR_NULL;
	}
	/* get message body from response .add by tyhuang 2003.7.2*/
	_content=uaGetRspContent(rsp);
	if(rspcode >= sipOK)
		uaDlgSetResponse(parentDlg,rsp);
	switch(origMethod){
	case INVITE:
		if((refdlg=uaDlgGetRefDlg(parentDlg))!=NULL){
			TxStruct notifytx;
			SipReq notifyReq;
			UaContent notifycontent;
			char buffer[256]={'\0'};
			UserProfile pfile;

			/*sprintf(buffer,"%s",GetSipfragFromRsp(rsp));*/
			GetSipfragFromRsp(rsp,buffer,255);
			notifycontent=uaContentNew("message/sipfrag;version=2.0",strlen(buffer),buffer);
			
			if(rspcode >= sipOK)
				notifyReq=uaNOTIFYMsg(refdlg,uaDlgGetRemoteTarget(refdlg),"terminated",notifycontent);
			else
				notifyReq=uaNOTIFYMsg(refdlg,uaDlgGetRemoteTarget(refdlg),"active",notifycontent);
			if(notifycontent)
				uaContentFree(notifycontent);
			if(notifyReq!=NULL){
				pfile=uaCreateUserProfile(uaMgrGetCfg(mgr));
				if(pfile == NULL)
					return UAUSER_PROFILE_NULL;
				notifytx=sipTxClientNew(notifyReq,NULL,pfile,refdlg);
				if(notifytx == NULL){
					sipReqFree(notifyReq);
					return UATX_NULL;
				}
				uaUserProfileFree(pfile);
				uaDlgAddTxIntoTxList(refdlg,notifytx);
				if(rspcode >= sipOK)
					uaDlgRemoveRef(refdlg);
			}
		}
		/* get final response , shoutdown timer*/
		if(rspcode >= sipOK){
			/*uaDlgSetResponse(parentDlg,rsp);*/
			/* check if timer is set*/
			if(IsDlgTimerd(parentDlg)==TRUE){
				uaDelTimer(uaDlgGetTimerHandle(parentDlg));
				SetDlgTimerd(parentDlg,FALSE);
			}	
			/* send notify to refer dialog */
		}
		if(rspcode <sipOK){ 
			int rseq=-1;
			SipRequire *require;
			SipStr *taglist;
			/*provisional response*/
			switch(rspcode){
			case Trying:
				/* modified by tyhuang 2003.3.37 if(uaDlgGetState(parentDlg) != UACSTATE_ONHOLDING) */
				/* modified by tyhuang 2003.6.5 .add (uaDlgGetState(parentDlg) == UACSTATE_RETRIEVING)*/
				
				if((uaDlgGetState(parentDlg) == UACSTATE_DIALING)||(uaDlgGetState(parentDlg) == UACSTATE_RETRIEVING)){
					uaDlgSetState(parentDlg,UACSTATE_PROCEEDING);
					/*Add an Expire timer 92/08/13 tyhuang */
					if(DEFAULT_EXPIRES>0){
						if(IsDlgTimerd(parentDlg)==FALSE)
							uaDlgSetTimerHandle(parentDlg,uaAddTimer(parentDlg, INVITE_EXPIRE, (DEFAULT_EXPIRES*1000)));
					}
				}
				break;
			case Ringing:
			case Session_Progress:
				/* modified by tyhuang 2003.3.37 if(uaDlgGetState(parentDlg) != UACSTATE_ONHOLDING) */
				/* modified by tyhuang 2003.6.5 .add (uaDlgGetState(parentDlg) == UACSTATE_RETRIEVING)*/
				if((uaDlgGetState(parentDlg) == UACSTATE_DIALING)||(uaDlgGetState(parentDlg) == UACSTATE_PROCEEDING)||(uaDlgGetState(parentDlg) == UACSTATE_RETRIEVING)){
					uaDlgSetState(parentDlg,UACSTATE_RINGBACK);
					/*Add an Expire timer 92/08/13 tyhuang */
					if(DEFAULT_EXPIRES>0){
						if(IsDlgTimerd(parentDlg)==FALSE)
							uaDlgSetTimerHandle(parentDlg,uaAddTimer(parentDlg, INVITE_EXPIRE, (DEFAULT_EXPIRES*1000)));
					}
				}
				break;
			default:
				break;
			};

			uaMgrPostNewMessage(mgr,parentDlg,UAMSG_CALLSTATE,uaGetRspCSeq(rsp),_content,NULL);
			require=(SipRequire*)sipRspGetHdr(rsp,Require);
			if(require){
				taglist=require->sipOptionTagList;
				while(taglist){
					if(strcmp(taglist->str,"100rel")==0){
						rseq=*(int*)sipRspGetHdr(rsp,RSeq);
						break;
					}
					taglist=taglist->next;
				}
			}
			
			if(rseq>=0){
				uaDlgSetRSeq(parentDlg,rseq);
				uaMgrPostNewMessage(mgr,parentDlg,UAMSG_REQUIRE_PRACK,uaGetRspCSeq(rsp),_content,NULL);
			}
		}else if((rspcode>=sipOK)&&(rspcode<Multiple_Choice)){
			/*Success response*/
			/*assume INVITE transaction carry a SDP, so when ACK not necessary carry SDP*/
			/*better implement, check INVITE tx, if no SDP, get response sdp and choose one codec*/
			/*if(isuaDlgInviteSDP(parentDlg) == TRUE){*/
				ackMsg=uaACKMsg(parentDlg,_tx,NULL);
				if(ackMsg == NULL)
					return UASIPMSG_GENACK_FAIL;
				pProfile=uaCreateUserProfile(uaMgrGetCfg(mgr));
				/*create a new client transaction */
				ackTx=sipTxClientNew(ackMsg,NULL,pProfile,(void*)parentDlg);
				uaUserProfileFree(pProfile);

				if(ackTx == NULL){
					sipReqFree(ackMsg);
					UaCoreERR("[uaProcessRspTx] create ack Tx fail!");
					return UATX_NULL;
				}
				sipTxFree(ackTx);
			/*}*/
			switch(uaDlgGetState(parentDlg)){
				case UACSTATE_DIALING:
				case UACSTATE_PROCEEDING:
				case UACSTATE_RINGBACK:
				case UACSTATE_RETRIEVING:
				case UACSTATE_SDPCHANGING:
						uaDlgSetState(parentDlg,UACSTATE_CONNECTED);
						uaMgrPostNewMessage(mgr,parentDlg,UAMSG_CALLSTATE,uaGetRspCSeq(rsp),_content,RspGetSupported(rsp));
						break;
				case UACSTATE_CONNECTED:
				case UACSTATE_ONHOLDING:
						if( (uaContentGetBody(_content)!=NULL)&&(
							strstr((const char*)uaContentGetBody(_content),"recvonly")||
							strstr((const char*)uaContentGetBody(_content),"inactive")||
							strstr((const char*)uaContentGetBody(_content),"0.0.0.0")))
							uaDlgSetState(parentDlg,UACSTATE_ONHELD);
						else
							uaDlgSetState(parentDlg,UACSTATE_CONNECTED);
						uaMgrPostNewMessage(mgr,parentDlg,UAMSG_CALLSTATE,uaGetRspCSeq(rsp),_content,RspGetSupported(rsp));
						break;
				/* tyhuang add on 2004/6/18 */
				/*
				 *	deu to call state is cancel, it means user want to
				 *	terminate a call.
				 */
				case UACSTATE_CANCEL:
						uaDlgSetState(parentDlg,UACSTATE_CONNECTED);
						uaMgrPostNewMessage(mgr,parentDlg,UAMSG_CALLSTATE,uaGetRspCSeq(rsp),_content,NULL);
						uaDlgDisconnect(parentDlg);
						break;
				default:
						break;
				}
			
		
			/* add by tyhuang 2003.6.23 */
			pContact=sipRspGetHdr(rsp,Contact);
			if(pContact != NULL){
				if(pContact->sipContactList != NULL)
					if(pContact->sipContactList->address!=NULL){
						SipURL url;
						contactbuf=strDup(pContact->sipContactList->address->addr);
						/*check address is correct or not */
						url=sipURLNewFromTxt(contactbuf);
						if(url==NULL){
							if(contactbuf){
								free(contactbuf);
								contactbuf=NULL;
							}
						}else
							sipURLFree(url);
					}
			}
			if(contactbuf != NULL) {
				rCode=uaDlgSetRemoteTarget(parentDlg,contactbuf);
				free(contactbuf);
				contactbuf = NULL;
			}
			/* enable session expire timer */
			pSE=(SipSessionExpires*)sipRspGetHdr(rsp,Session_Expires);
			if(pSE){
				if(uaDlgGetSETimer(parentDlg)>0)
					uaDelTimer(uaDlgGetSEHandle(parentDlg));
				uaDlgSetSEHandle(parentDlg,1,uaAddTimer(parentDlg,SESSION_TIMER,pSE->deltasecond *500));
			}

		}else if((rspcode>=Multiple_Choice)&&(rspcode<Bad_Request)){
			/*if rspcode >300, should not send ACK for INVITE*/
			/*transaction layer will send ACK automatic, execpt 200 response*/
			/*redirect message*/ 
			
			/*if Contact header exist, copy from Contact header*/
			pContact=sipRspGetHdr(rsp,Contact);
			if(pContact != NULL){
				if(pContact->sipContactList != NULL)
					if(pContact->sipContactList->address!=NULL){
						SipURL url;
						contactbuf=strDup(pContact->sipContactList->address->addr);
						/*check address is correct or not */
						url=sipURLNewFromTxt(contactbuf);
						if(url==NULL){
							if(contactbuf){
								free(contactbuf);
								contactbuf=NULL;
							}
						}else
							sipURLFree(url);
					}
			}
			if(contactbuf != NULL){
				/*redirect message*/
				/*duplicate a new request message from original request message*/
				newInvReq=sipReqDup(origReq);
				/*remove via */
				sipReqDelViaHdrTop(newInvReq);
				/*change a new Call_ID*/
				sprintf(buf,"Call-ID:%s\r\n",uaDlgGetCallID(parentDlg));
				sipReqAddHdrFromTxt(newInvReq,Call_ID,buf);
				/* update new callid */
				/*uaDlgSetCallID(parentDlg,strAtt(uaDlgGetCallID(parentDlg),"redirect"));*/
				uaDlgSetCallID(parentDlg,(char*)sipReqGetHdr(newInvReq,Call_ID));
				
				if(rspcode==User_Proxy){
					/* add route header */
					sprintf(buf,"Route:<%s;lr>\r\n",contactbuf);
					sipReqAddHdrFromTxt(newInvReq,Route,buf);
				}else{
					/*change request URI and command sequence */
					pReqLine=uaReqNewReqLine(INVITE,contactbuf,SIP_VERSION);
					if(pReqLine != NULL){
						sipReqSetReqLine(newInvReq,pReqLine);
						uaReqLineFree(pReqLine);
					}
					
				}
				free(contactbuf);
				/*change CSeq*/
				uaMgrAddCmdSeq(uaDlgGetMgr(parentDlg));
				/*newCSeq=uaNewCSeq(uaGetReqCSeq(origReq),INVITE);*/
				newCSeq=uaNewCSeq(uaMgrGetCmdSeq(uaDlgGetMgr(parentDlg)),INVITE);
				if(newCSeq != NULL){
					sipReqAddHdr(newInvReq,CSeq,newCSeq);
					uaFreeCSeq(newCSeq);
				}
				/*new INVITE message only different to original INVITE message in*/
				/* CSeq +1 and Request URI copy from Contact address*/

				pProfile=sipTxGetUserProfile(_tx);
				if(pProfile == NULL){
					sipReqFree(newInvReq);
					return UATX_USER_PROFILE_NULL;
				}
				/*create new transaction*/
				newInvTx=sipTxClientNew(newInvReq,NULL,pProfile,parentDlg);
				if(newInvTx == NULL){
					sipReqFree(newInvReq);
					return UATX_NULL;
				}
				/* add new transaction into dialog*/
				uaDlgAddTxIntoTxList(parentDlg,newInvTx);
				/*delete old INVITE transaction*/
				uaDlgDelTxFromTxList(parentDlg,_tx);
				sipTxFree(_tx);
				uaDlgSetState(parentDlg,UACSTATE_DIALING);

			}else{
				/*can't not get a sip url from 3xx message . add by tyhuang 2003.11.17 */
				uaDlgSetState(parentDlg,UACSTATE_DISCONNECT);
				uaMgrPostNewMessage(mgr,parentDlg,UAMSG_CALLSTATE,uaGetRspCSeq(rsp),_content,NULL);
				UaCoreWARN("[uaProcessRspTx] Redirect message didn't contain Contact header!\n");
			}


		}else {	
			if((uaDlgGetState(parentDlg)==UACSTATE_RETRIEVING) 
				&& (rspcode!=Request_Timeout)
				&& (rspcode!=Call_Leg_Transaction_Does_Not_Exist)){
				uaDlgSetState(parentDlg,UACSTATE_ONHELD);
				uaMgrPostNewMessage(mgr,parentDlg,UAMSG_CALLSTATE,uaGetRspCSeq(rsp),_content,NULL);
			}

			if((uaDlgGetState(parentDlg)==UACSTATE_ONHOLDING) 
				&& (rspcode!=Request_Timeout)
				&& (rspcode!=Call_Leg_Transaction_Does_Not_Exist)){
				uaDlgSetState(parentDlg,UACSTATE_CONNECTED);
				uaMgrPostNewMessage(mgr,parentDlg,UAMSG_CALLSTATE,uaGetRspCSeq(rsp),NULL,NULL);
			}
			switch(rspcode){
			
			case Session_Inerval_Too_Small:
			{
				SipCSeq *pCseq;
				SipMinSE *pMinse=NULL;
				SipSessionExpires *pSE=NULL;
				SipVia *pVia;
				/* set minse to dialog session timer */
				pMinse=(SipMinSE*)sipRspGetHdr(rsp,Min_SE);
				if(pMinse){
					uaDlgSetSETimer(parentDlg,pMinse->deltasecond);
					/* auto-redial */
					newInvReq=sipReqDup(origReq);
					/* update session-expire and cseq */
					uaMgrAddCmdSeq(mgr);
					pCseq=uaNewCSeq(uaMgrGetCmdSeq(mgr),INVITE);
					if(pCseq!=NULL){
						sipReqAddHdr(newInvReq,CSeq,pCseq);
						uaFreeCSeq(pCseq);
					}
					pSE=(SipSessionExpires*)sipReqGetHdr(newInvReq,Session_Expires);
					if(pSE)
						pSE->deltasecond=pMinse->deltasecond;
					/* remove via */
					/* delete old via header */
					pVia=sipReqGetHdr(newInvReq,Via);
					while(1){
						if(pVia->numParams>0)
							sipReqDelViaHdrTop(newInvReq);	
						else
							break;
					}
					/* store new request.  add by tyhuang 2003.9.30 */
					uaDlgSetRequest(parentDlg,newInvReq);

					pProfile=sipTxGetUserProfile(_tx);
					if(pProfile == NULL){
						sipReqFree(newInvReq);
						return UATX_USER_PROFILE_NULL;
					}
					/*create new transaction*/
					newInvTx=sipTxClientNew(newInvReq,NULL,pProfile,parentDlg);
					if(newInvTx==NULL){
						sipReqFree(newInvReq);
						return UATX_NULL;
					}
					/*add new transaction into dialog*/
					uaDlgAddTxIntoTxList(parentDlg,newInvTx);
					/*delete old INVITE transaction*/
					uaDlgDelTxFromTxList(parentDlg,_tx);
					sipTxFree(_tx);
				}else{
					uaDlgSetState(parentDlg,UACSTATE_DISCONNECT);
					uaMgrPostNewMessage(mgr,parentDlg,UAMSG_CALLSTATE,uaGetRspCSeq(rsp),_content,NULL);
				}
				break;
			}
			case Busy_Everywhere:
			case Busy_Here:/*busy here*/
				/*uaDlgSetState(parentDlg,UACSTATE_BUSY);
				uaMgrPostNewMessage(mgr,parentDlg,UAMSG_CALLSTATE,uaGetRspCSeq(rsp),_content,NULL);
				break;*/
			case Not_Acceptable:/*not acceptablie*/
				/*
				uaDlgSetState(parentDlg,UACSTATE_REJECTED);
				uaMgrPostNewMessage(mgr,parentDlg,UAMSG_CALLSTATE,uaGetRspCSeq(rsp),_content,NULL);
				break;
				*/
			case Call_Leg_Transaction_Does_Not_Exist:
				uaDlgSetState(parentDlg,UACSTATE_DISCONNECT);
				uaMgrPostNewMessage(mgr,parentDlg,UAMSG_CALLSTATE,uaGetRspCSeq(rsp),_content,NULL);
				break;
			case Unauthorized:
			case Proxy_Authentication_Required:{/*not proxy authenticate(407)*/
				
				/*uaDlgSetState(parentDlg,UACSTATE_DIALING); mark by tyhuang 2004.2.23*/
				/*Doing proxy-authenticate*/
				newInvReq=uaRegAuthnMsg(parentDlg,_tx,origReq,rsp);
				if(newInvReq == NULL){
					rCode=UATX_AUTHZ_MSG_GEN_ERROR;
					/*should notify user proxy-authenticate error*/
					uaDlgSetState(parentDlg,UACSTATE_PROXYAUTHN_FAIL);
					uaMgrPostNewMessage(mgr,parentDlg,UAMSG_CALLSTATE,uaGetRspCSeq(rsp),_content,NULL);
					uaDlgSetState(parentDlg,UACSTATE_DISCONNECT);
					uaMgrPostNewMessage(mgr,parentDlg,UAMSG_CALLSTATE,uaGetRspCSeq(rsp),_content,NULL);
					break;
				}
				/* store new request.  add by tyhuang 2003.9.30 */
				uaDlgSetRequest(parentDlg,newInvReq);

				pProfile=sipTxGetUserProfile(_tx);
				if(pProfile == NULL){
					sipReqFree(newInvReq);
					return UATX_USER_PROFILE_NULL;
				}
				/*create new transaction*/
				newInvTx=sipTxClientNew(newInvReq,NULL,pProfile,parentDlg);
				if(newInvTx==NULL){
					sipReqFree(newInvReq);
					return UATX_NULL;
				}
				
				if(uaCheckHold(uaGetReqSdp(newInvReq))==TRUE)
					uaDlgSetState(parentDlg,UACSTATE_ONHOLDING);
				
				/*add new transaction into dialog*/
				uaDlgAddTxIntoTxList(parentDlg,newInvTx);
				/*delete old INVITE transaction*/
				uaDlgDelTxFromTxList(parentDlg,_tx);
				sipTxFree(_tx);
				}
				break;
			default:
				/*Error message, Client/Server/Global Failure*/
				if((uaDlgGetState(parentDlg)!=UACSTATE_ONHELD)&&(uaDlgGetState(parentDlg)!=UACSTATE_CONNECTED)){
					uaDlgSetState(parentDlg,UACSTATE_DISCONNECT);
					/* free inviteTx */
					uaDlgDelInviteTx(parentDlg);
					uaMgrPostNewMessage(mgr,parentDlg,UAMSG_CALLSTATE,uaGetRspCSeq(rsp),_content,NULL);
				}else{
					/*
					uaDlgSetState(parentDlg,UACSTATE_REJECTED);
					uaMgrPostNewMessage(mgr,parentDlg,UAMSG_CALLSTATE,uaGetRspCSeq(rsp),_content,NULL);
					*/
				}
				break;
			};
			/*uaMgrPostNewMsg(mgr,parentDlg,UAMSG_CALLSTATE,uaGetRspCSeq(rsp),uaGetRspSdp(rsp));*/
		}
		break;
	/* not INVITE transaction*/
	case BYE:
	case CANCEL:
		/* get final response , shoutdown timer*/
		if(rspcode >= sipOK){
			/* check if timer is set*/
			if(IsDlgTimerd(parentDlg)==TRUE){
				uaDelTimer(uaDlgGetTimerHandle(parentDlg));
				SetDlgTimerd(parentDlg,FALSE);	
			}
		}
		switch(rspcode){
		case Trying:/*trying*/
			
			break;
		case sipOK:/*disconnect*/
			if(IsDlgTimerd(parentDlg)==FALSE){
				/* not get a final response */
				if(origMethod==BYE){
					uaDlgSetState(parentDlg,UACSTATE_DISCONNECT);
					bPostMsg=TRUE;
				}
			}/*else{
			 */	
				/* get a final response ,send BYE to disconnect */
			/*	uaDlgDisconnect(parentDlg);
			}*/
			break;
		case Not_Found:
		case Request_Timeout:
		case Call_Leg_Transaction_Does_Not_Exist:
			uaDlgSetState(parentDlg,UACSTATE_DISCONNECT);
			bPostMsg=TRUE;
			break;
		case Unauthorized:
		case Proxy_Authentication_Required:{/*not proxy authenticate(407)*/
			/*Doing proxy-authenticate*/
				newInvReq=uaRegAuthnMsg(parentDlg,_tx,origReq,rsp);
				if(newInvReq == NULL){
					rCode=UATX_AUTHZ_MSG_GEN_ERROR;
					/*should notify user proxy-authenticate error*/
					uaDlgSetState(parentDlg,UACSTATE_PROXYAUTHN_FAIL);
					uaMgrPostNewMessage(mgr,parentDlg,UAMSG_CALLSTATE,uaGetRspCSeq(rsp),_content,NULL);
					uaDlgSetState(parentDlg,UACSTATE_DISCONNECT);
					break;
				}

				pProfile=sipTxGetUserProfile(_tx);
				if(pProfile == NULL){
					sipReqFree(newInvReq);
					return UATX_USER_PROFILE_NULL;
				}
				/*create new transaction*/
				newInvTx=sipTxClientNew(newInvReq,NULL,pProfile,parentDlg);
				if(newInvTx==NULL){
					sipReqFree(newInvReq);
					return UATX_NULL;
				}

				/*add new transaction into dialog*/
				uaDlgAddTxIntoTxList(parentDlg,newInvTx);
		}
		default:
			if(origMethod==CANCEL)
				UaCoreWARN("[uaProcessRspTx] Error! CANCEL receive failure response!\n");
			else
				UaCoreWARN("[uaProcessRspTx] Error! BYE receive failure response!\n");
			/* mark by tyhuang 2003.9.30
			uaDlgSetState(parentDlg,UACSTATE_DISCONNECT);
			bPostMsg=TRUE;
			*/
			break;
		};
		if(bPostMsg){
			/* free inviteTx */
			uaDlgDelInviteTx(parentDlg);
			uaMgrPostNewMessage(mgr,parentDlg,UAMSG_CALLSTATE,uaGetRspCSeq(rsp),_content,NULL);
			/*uaMgrDelDlg(mgr,parentDlg,TRUE);*/
		}
		break;
	
	case REGISTER:
		
		switch(rspcode){
		case Trying:/*trying*/
			
			break;
		case sipOK:/*register ok*/
			uaDlgSetState(parentDlg,UACSTATE_REGISTERED);
			bPostMsg=TRUE;
			/* add a Timer for this register */
			if(IsDlgTimerd(parentDlg)==TRUE){
				uaDelTimer(uaDlgGetTimerHandle(parentDlg));
				SetDlgTimerd(parentDlg,FALSE);
			}
			user=uaMgrGetUser(mgr);
			if(user){
				expires=uaUserGetExpires(user);
				if(expires >0 )
					uaDlgSetRegTimerHandle(parentDlg,uaAddTimer(parentDlg,REGISTER_EXPIRE, expires* 500));
			}
			
			break;
		case Proxy_Authentication_Required:
		case Unauthorized:{/*ask www-authenticate*/
			SipReq newRegReq=NULL;
			SipReq origRegReq=NULL;
			SipRsp lastRegRsp=NULL;
			TxStruct regTx=NULL;
			UserProfile pProfile=NULL;
			
			uaDlgSetState(parentDlg,UACSTATE_REGISTER);
			/*resend request message and prepare Authorization header*/
			origRegReq=sipTxGetOriginalReq(_tx);
			lastRegRsp=sipTxGetLatestRsp(_tx);
			if((origRegReq == NULL)||(lastRegRsp==NULL)){
				rCode=UATX_REQ_NOT_FOUND;
				break;
			}
			
			newRegReq=uaRegAuthnMsg(parentDlg,_tx,origRegReq,lastRegRsp);
			if(newRegReq==NULL){
				/* debug */
				TCRPrint(TRACE_LEVEL_API,"UATX_AUTHZ_MSG_GEN_ERROR\n");

				rCode=UATX_AUTHZ_MSG_GEN_ERROR;
				/*shoud notify user to create a new authorization entry*/
				uaDlgSetState(parentDlg,UACSTATE_REGISTERFAIL);
				/*uaMgrPostNewMsg(mgr,parentDlg,UAMSG_CALLSTATE,uaGetRspCSeq(rsp),NULL);*/
				uaMgrPostNewMessage(mgr,parentDlg,UAMSG_CALLSTATE,uaGetRspCSeq(rsp),_content,NULL);
				break;
			}

			/*extract original transaction user profile*/
			pProfile=sipTxGetUserProfile(_tx);
			if(pProfile == NULL){
				sipReqFree(newRegReq);
				return UATX_USER_PROFILE_NULL;
			}

			/*create new transaction*/
			regTx=sipTxClientNew(newRegReq,NULL,pProfile,parentDlg);
			if(regTx==NULL){
				sipReqFree(newRegReq);
				return UATX_NULL;
			}
			/*add transaction into dialog*/
			uaDlgAddTxIntoTxList(parentDlg,regTx);
			uaDlgSetState(parentDlg,UACSTATE_REGISTER_AUTHN);
			/*delete original transaction from dialog tx list */
			uaDelTxFromTxList(parentDlg,uaDlgGetTxList(parentDlg),_tx);

			bPostMsg=TRUE;
			}
			break;
		default:
			UaCoreWARN("[uaProcessRspTx] Error! REGISTER receive wrong response!\n");
			uaDlgSetState(parentDlg,UACSTATE_REGISTERFAIL);
			/*uaMgrPostNewMsg(uaDlgGetMgr(parentDlg),parentDlg,UAMSG_CALLSTATE,uaGetRspCSeq(rsp),uaGetRspSdp(rsp));*/
			uaMgrPostNewMessage(uaDlgGetMgr(parentDlg),parentDlg,UAMSG_CALLSTATE,uaGetRspCSeq(rsp),_content,NULL);
			break;
		};
		if(bPostMsg){
			/*uaMgrPostNewMsg(mgr,parentDlg,UAMSG_CALLSTATE,uaGetRspCSeq(rsp),uaGetRspSdp(rsp));*/
			uaMgrPostNewMessage(mgr,parentDlg,UAMSG_CALLSTATE,uaGetRspCSeq(rsp),_content,NULL);
		}	
		break;
	case REFER:
		switch(rspcode){
		case Trying:/*trying*/
			break;
		case Accepted:/*refer ok*/
		case sipOK:
			
			break;
		default:
			/*post refer failed */
			if(rspcode>=Bad_Request)
				uaMgrPostNewMessage(mgr,parentDlg,UAMSG_REFER_FAIL,uaGetRspCSeq(rsp),_content,NULL);	
			break;
		}
		break;
	case OPTIONS:

		break;
	case ACK:
	
		UaCoreWARN("[uaProcessRspTx] Error! Should not receive ACK message !\n");
		break;
	case SIP_NOTIFY:
		switch(rspcode){
			case Trying:/*trying*/
				break;
			case sipOK:
				/* post message to inform subscription-state -> terminated and reason ,retry */
				uaMgrPostNewMessage(mgr,parentDlg,UAMSG_NOTIFY,uaGetRspCSeq(rsp),_content,NULL);
				break;
			case Call_Leg_Transaction_Does_Not_Exist:
				/* add for transfer 2004/8/25 */
				uaDlgRemoveRef(	parentDlg );
				break;
			default:
				UaCoreWARN("[uaProcessRspTx] Error! NOTIFY receive response!\n");
				uaMgrPostNewMessage(mgr,parentDlg,UAMSG_NOTIFY_FAIL,uaGetRspCSeq(rsp),_content,NULL);
				break;
		};
		break;
	case SIP_SUBSCRIBE:
		switch(rspcode){
		case Trying:/*trying*/
			break;
		case Accepted:
			/* post message to inform subscription-state -> pedding  */
			uaMgrPostNewMessage(mgr,parentDlg,UAMSG_SUBSCRIBE,uaGetRspCSeq(rsp),_content,NULL);
			break;
		case sipOK:/* ok*/
			uaDlgSetRemoveTag(parentDlg,uaGetRspToTag(rsp));
			/* post message to inform subscription-state -> active and expire time */
			uaMgrPostNewMessage(mgr,parentDlg,UAMSG_SUBSCRIBE,uaGetRspCSeq(rsp),_content,NULL);
			break;
		case Moved_Temporarily:
			pContact=sipRspGetHdr(rsp,Contact);
			if(pContact != NULL){
				if(pContact->sipContactList != NULL)
					if(pContact->sipContactList->address!=NULL){
						SipURL url;
						contactbuf=strDup(pContact->sipContactList->address->addr);
						/*check address is correct or not */
						url=sipURLNewFromTxt(contactbuf);
						if(url==NULL){
							if(contactbuf){
								free(contactbuf);
								contactbuf=NULL;
							}
						}else
							sipURLFree(url);
					}
			}
			if(contactbuf != NULL){
				/*redirect message*/
				/*duplicate a new request message from original request message*/
				newInvReq=sipReqDup(origReq);
				if (!newInvReq) break;
				
				/*remove via */
				sipReqDelViaHdrTop(newInvReq);
				/* update new callid */
				/*change request URI and command sequence */
				pReqLine=uaReqNewReqLine(SIP_SUBSCRIBE,contactbuf,SIP_VERSION);
				if(pReqLine != NULL){
					sipReqSetReqLine(newInvReq,pReqLine);
					uaReqLineFree(pReqLine);
				}else{
					free(contactbuf);
					sipReqFree(newInvReq);
					break;
				}
			}else
				break;
			free(contactbuf);
			/*change CSeq*/
			uaMgrAddCmdSeq(uaDlgGetMgr(parentDlg));
			/*newCSeq=uaNewCSeq(uaGetReqCSeq(origReq),INVITE);*/
			newCSeq=uaNewCSeq(uaMgrGetCmdSeq(uaDlgGetMgr(parentDlg)),SIP_SUBSCRIBE);
			if(newCSeq != NULL){
				sipReqAddHdr(newInvReq,CSeq,newCSeq);
				uaFreeCSeq(newCSeq);
			}

			pProfile=sipTxGetUserProfile(_tx);
			if(pProfile == NULL){
				sipReqFree(newInvReq);
				return UATX_USER_PROFILE_NULL;
			}
			/*create new transaction*/
			newInvTx=sipTxClientNew(newInvReq,NULL,pProfile,parentDlg);
			if(newInvTx == NULL){
				sipReqFree(newInvReq);
				return UATX_NULL;
			}
			/* add new transaction into dialog*/
			uaDlgAddTxIntoTxList(parentDlg,newInvTx);
			/*delete old INVITE transaction*/
			uaDlgDelTxFromTxList(parentDlg,_tx);
			sipTxFree(_tx);
			break;
		default:
			UaCoreWARN("[uaProcessRspTx] Error! SUBSCRIBE receive response!\n");
			/* post message to inform subscription-state -> terminated and reason ,retry */
			uaMgrPostNewMessage(mgr,parentDlg,UAMSG_SUBSCRIBE_FAIL,uaGetRspCSeq(rsp),_content,NULL);
			break;
		};
		break;
	case SIP_PUBLISH:
		switch(rspcode){
		case Trying:/*trying*/
			break;
		case sipOK:
			{
			/* set SIP-If-Match */
			unsigned char* sipetag=NULL;
			sipetag=(unsigned char*)sipRspGetHdr(rsp,SIP_ETag);
			if(sipetag != NULL)
				uaDlgSetSIPIfMatch(parentDlg,sipetag);
			/* post event */
			uaMgrPostNewMessage(mgr,parentDlg,UAMSG_PUBLISH,uaGetRspCSeq(rsp),_content,NULL);
			break;
			}
		case Moved_Temporarily:
			pContact=sipRspGetHdr(rsp,Contact);
			if(pContact != NULL){
				if(pContact->sipContactList != NULL)
					if(pContact->sipContactList->address!=NULL){
						SipURL url;
						contactbuf=strDup(pContact->sipContactList->address->addr);
						/*check address is correct or not */
						url=sipURLNewFromTxt(contactbuf);
						if(url==NULL){
							if(contactbuf){
								free(contactbuf);
								contactbuf=NULL;
							}
						}else
							sipURLFree(url);
					}
			}
			if(contactbuf != NULL){
				/*redirect message*/
				/*duplicate a new request message from original request message*/
				newInvReq=sipReqDup(origReq);
				if (!newInvReq) break;	
				/*remove via */
				sipReqDelViaHdrTop(newInvReq);
				/* update new callid */
				/*change request URI and command sequence */
				pReqLine=uaReqNewReqLine(SIP_PUBLISH,contactbuf,SIP_VERSION);
				if(pReqLine != NULL){
					sipReqSetReqLine(newInvReq,pReqLine);
					uaReqLineFree(pReqLine);
				}else{
					free(contactbuf);
					sipReqFree(newInvReq);
					break;
				}
			}else
				break;
			free(contactbuf);
			/*change CSeq*/
			uaMgrAddCmdSeq(uaDlgGetMgr(parentDlg));
			/*newCSeq=uaNewCSeq(uaGetReqCSeq(origReq),INVITE);*/
			newCSeq=uaNewCSeq(uaMgrGetCmdSeq(uaDlgGetMgr(parentDlg)),SIP_PUBLISH);
			if(newCSeq != NULL){
				sipReqAddHdr(newInvReq,CSeq,newCSeq);
				uaFreeCSeq(newCSeq);
			}

			pProfile=sipTxGetUserProfile(_tx);
			if(pProfile == NULL){
				sipReqFree(newInvReq);
				return UATX_USER_PROFILE_NULL;
			}
			/*create new transaction*/
			newInvTx=sipTxClientNew(newInvReq,NULL,pProfile,parentDlg);
			if(newInvTx == NULL){
				sipReqFree(newInvReq);
				return UATX_NULL;
			}
			/* add new transaction into dialog*/
			uaDlgAddTxIntoTxList(parentDlg,newInvTx);
			/*delete old INVITE transaction*/
			uaDlgDelTxFromTxList(parentDlg,_tx);
			sipTxFree(_tx);
			break;
		case Conditional_Request_Failed:
			uaMgrPostNewMessage(mgr,parentDlg,UAMSG_PUBLISH_CONDITIONAL_FAIL,uaGetRspCSeq(rsp),_content,NULL);
			break;
		default:
			UaCoreWARN("[uaProcessRspTx] Error! PUBLISH receive response!\n");
			uaMgrPostNewMessage(mgr,parentDlg,UAMSG_PUBLISH_FAIL,uaGetRspCSeq(rsp),_content,NULL);
			break;
		};
		break;
	case SIP_MESSAGE:
		switch(rspcode){
		case Trying:/*trying*/
			break;
		case sipOK:
			uaMgrPostNewMessage(mgr,parentDlg,UAMSG_MESSAGE,uaGetRspCSeq(rsp),_content,NULL);
			break;
		case Moved_Temporarily:
			pContact=sipRspGetHdr(rsp,Contact);
			if(pContact != NULL){
				if(pContact->sipContactList != NULL)
					if(pContact->sipContactList->address!=NULL){
						SipURL url;
						contactbuf=strDup(pContact->sipContactList->address->addr);
						/*check address is correct or not */
						url=sipURLNewFromTxt(contactbuf);
						if(url==NULL){
							if(contactbuf){
								free(contactbuf);
								contactbuf=NULL;
							}
						}else
							sipURLFree(url);
					}
			}
			if(contactbuf != NULL){
				/*redirect message*/
				/*duplicate a new request message from original request message*/
				newInvReq=sipReqDup(origReq);
				if (!newInvReq) break;	
				/*remove via */
				sipReqDelViaHdrTop(newInvReq);
				/* update new callid */
				/*change request URI and command sequence */
				pReqLine=uaReqNewReqLine(SIP_MESSAGE,contactbuf,SIP_VERSION);
				if(pReqLine != NULL){
					sipReqSetReqLine(newInvReq,pReqLine);
					uaReqLineFree(pReqLine);
				}else{
					free(contactbuf);
					sipReqFree(newInvReq);
					break;
				}
			}else
				break;
			free(contactbuf);
			/*change CSeq*/
			uaMgrAddCmdSeq(uaDlgGetMgr(parentDlg));
			/*newCSeq=uaNewCSeq(uaGetReqCSeq(origReq),INVITE);*/
			newCSeq=uaNewCSeq(uaMgrGetCmdSeq(uaDlgGetMgr(parentDlg)),SIP_MESSAGE);
			if(newCSeq != NULL){
				sipReqAddHdr(newInvReq,CSeq,newCSeq);
				uaFreeCSeq(newCSeq);
			}

			pProfile=sipTxGetUserProfile(_tx);
			if(pProfile == NULL){
				sipReqFree(newInvReq);
				return UATX_USER_PROFILE_NULL;
			}
			/*create new transaction*/
			newInvTx=sipTxClientNew(newInvReq,NULL,pProfile,parentDlg);
			if(newInvTx == NULL){
				sipReqFree(newInvReq);
				return UATX_NULL;
			}
			/* add new transaction into dialog*/
			uaDlgAddTxIntoTxList(parentDlg,newInvTx);
			/*delete old INVITE transaction*/
			uaDlgDelTxFromTxList(parentDlg,_tx);
			sipTxFree(_tx);
			break;
		default:
			UaCoreWARN("[uaProcessRspTx] Error! MESSAGE receive response!\n");
			uaMgrPostNewMessage(mgr,parentDlg,UAMSG_MESSAGE_FAIL,uaGetRspCSeq(rsp),_content,NULL);
			break;
		};	
		break;
	case SIP_INFO:
		switch(rspcode){
		case Trying:/*trying*/
			break;
		case sipOK:
			uaMgrPostNewMessage(mgr,parentDlg,UAMSG_SIP_INFO_RSP,uaGetRspCSeq(rsp),_content,rsp);
			break;
		default:
			UaCoreWARN("[uaProcessRspTx] Error! INFO receive response!\n");
			uaMgrPostNewMessage(mgr,parentDlg,UAMSG_SIP_INFO_FAIL,uaGetRspCSeq(rsp),_content,rsp);
			break;
		};
		break;
	/* add SIP_UPDATE and PRACK .2004/4/19 */
	case SIP_UPDATE:
		if((rspcode>=sipOK)&&(rspcode<300))
			uaMgrPostNewMessage(mgr,parentDlg,UAMSG_SIP_UPDATE_RSP,uaGetRspCSeq(rsp),_content,NULL);
		else if(rspcode>=Bad_Request)
			uaMgrPostNewMessage(mgr,parentDlg,UAMSG_SIP_UPDATE_FAIL,uaGetRspCSeq(rsp),_content,NULL);
		break;
	case PRACK:
		if((rspcode>=sipOK)&&(rspcode<300))
			uaMgrPostNewMessage(mgr,parentDlg,UAMSG_SIP_PRACK_RSP,uaGetRspCSeq(rsp),_content,NULL);
		else if(rspcode>=Bad_Request)
			uaMgrPostNewMessage(mgr,parentDlg,UAMSG_SIP_PRACK_FAIL,uaGetRspCSeq(rsp),_content,NULL);
		break;
	default:
		break;

	}
	/*should change state then send uaMsg to Application*/
	if(_content){
		uaContentFree(_content);
		_content=NULL;
	}
	return rCode;
}

/*when receive a ACK, enter CONNECTED state*/
/*if already in CONNECTED state, it means re-INVITE ACK message*/
RCODE uaProcessAckTx(IN TxStruct _tx)
{
	RCODE rCode=RC_OK;
	UaDlg dlg=NULL;
	UaMgr mgr=NULL;
	UACallStateType dlgState=UACSTATE_UNKNOWN;
	SipReq reqAck=NULL;
	SipReqLine* reqline = NULL;
	SipRsp rsp=NULL;
	SdpSess acksdp=NULL;

	
	if(_tx != NULL){
		dlg=sipTxGetUserData(_tx);
		
		if(dlg==NULL){/* match tx by uacore when trasaction can't  */
			mgr=uaMgrMatchTx(_tx);
			dlg=uaDlgMatchTx(mgr,_tx);
			reqAck=sipTxGetOriginalReq(_tx);
			if (reqAck)
					reqline = sipReqGetReqLine(reqAck);

			if(dlg==NULL)
				return UADLG_NULL;
		}
		dlgState=uaDlgGetState(dlg);

		/*get parent mgr object*/
		if(mgr==NULL){
			mgr=uaDlgGetMgr(dlg);
			if(mgr==NULL)
				return UAMGR_NULL;
		}
		/* modified by tyhuang 2003.9.16 */
		if(dlg != NULL){
			if(IsDlgTimerd(dlg)==TRUE){
				uaDelTimer(uaDlgGetTimerHandle(dlg));
				SetDlgTimerd(dlg,FALSE);	
			}
		}
		/*check ACK message*/
		if(reqAck==NULL || (reqline->iMethod == INVITE) )
			reqAck=sipTxGetAck(_tx);
		if(reqAck == NULL)
			return UATX_ACK_NOT_FOUND;

		acksdp=uaGetReqSdp(reqAck);
		if(dlgState==UACSTATE_ACCEPT){
			uaDlgSetState(dlg,UACSTATE_CONNECTED);
			uaMgrPostNewMsg(mgr,dlg,UAMSG_CALLSTATE,uaGetReqCSeq(reqAck),acksdp);
		}else if(dlgState==UACSTATE_REJECT){
			uaDlgSetState(dlg,UACSTATE_DISCONNECT);
			uaMgrPostNewMsg(mgr,dlg,UAMSG_CALLSTATE,uaGetReqCSeq(reqAck),acksdp);
		}else {
			/*rsp=uaCreateRspFromReq(reqAck,Call_Leg_Transaction_Does_Not_Exist,NULL,NULL);
			sipTxSendRsp(_tx,rsp);*/
			rCode=UATX_DLG_NULL;
		}
		if(acksdp){
			uaSDPFree(acksdp);
			acksdp=NULL;
		}
	}else
		rCode=UATX_NULL;

	return rCode;
}

RCODE uaProcessCancel(TxStruct tx)
{
	UaDlg origdlg;
	UACallStateType dlgstate;

	SipReq inviteReq=NULL;
	SipRsp inviteRsp=NULL;

	if (!tx) return RC_ERROR;
	inviteReq=sipTxGetOriginalReq(tx);
	if(inviteReq==NULL)
		return UATX_REQ_NOT_FOUND;
	
	origdlg=(UaDlg)sipTxGetUserData(tx);
	if (!origdlg) {
		/* send 487 back if dlg is NULL*/
		inviteRsp=uaCreateRspFromReq(inviteReq,Request_Terminated,NULL,NULL);
		if(inviteRsp == NULL)
			return UATX_RSP_NOT_FOUND;
		
		uaDlgSetResponse(origdlg,inviteRsp);
		sipTxSendRsp(tx,inviteRsp);
	}else{
		/* if dlg exists,call state change */
		/*match a original dialor*/
		dlgstate=uaDlgGetState(origdlg);
		switch(dlgstate){
		case UACSTATE_CONNECTED:
			/* 
			 *	call disconnect
			 *  tyhuang add 2004/6/18
			 */
			uaDlgDisconnect(origdlg);
			break;
		case UACSTATE_OFFERING:
		case UACSTATE_OFFERING_REPLACE:
			/*should get original INVITE transaction*/
			/*generate 487 Request_Cancelled message*/
			inviteRsp=uaCreateRspFromReq(inviteReq,Request_Terminated,NULL,NULL);
			if(inviteRsp == NULL)
				return UATX_RSP_NOT_FOUND;
			
			uaDlgSetResponse(origdlg,inviteRsp);
			sipTxSendRsp(tx,inviteRsp);
			
			/* remove invite expire timer*/
			if(IsDlgTimerd(origdlg)==TRUE){
				uaDelTimer(uaDlgGetTimerHandle(origdlg));
				SetDlgTimerd(origdlg,FALSE);	
			}
	
			uaDlgSetState(origdlg,UACSTATE_REJECT);
			uaMgrPostNewMessage(uaDlgGetMgr(origdlg),origdlg,UAMSG_CALLSTATE,uaGetRspCSeq(inviteRsp),NULL,NULL);
			break;
		default:
			break;
		}
	}/* end w*/
	return RC_OK;
}

/*get match mgr, compare with user name*/
/*since UA didn't play as Registrar, it can't accept REGISTER method*/
UaMgr uaMgrMatchTx(IN TxStruct _tx)
{
	UaMgr mgr=NULL,supermgr=NULL;
	int mgrnum=0,mgrpos=0;
	char *username=NULL,*mgruser=NULL,*tmp=NULL,*freeptr=NULL;
	SipReq req=NULL;
	TXTYPE txType;

	req=sipTxGetOriginalReq(_tx);
	if(req == NULL){
		UaCoreERR("[uaMgrMatchTx] original request is NULL!\n");
		return NULL;
	}
	/*Client transaction*/
	txType=sipTxGetType(_tx);
	if( (TX_CLIENT_INVITE == txType) || (REGISTER==sipTxGetMethod(_tx))){
		username=strDup(uaGetReqFromAddr(req));
		freeptr=username;
		tmp=strchr(username,':');
		if(tmp) username=tmp+1;
		tmp=strchr(username,'@');
		if(tmp)
			*tmp='\0';
		else{
			free(freeptr);
			username=strDup("");
			freeptr=username;
		}
	}else{
		username=(char*)uaGetReqCallee(req);
		freeptr=username;
	}
	mgrnum=dxLstGetSize(MgrList);
	for(mgrpos=0;mgrpos<mgrnum;mgrpos++){
		mgr=dxLstPeek(MgrList,mgrpos);
		mgruser=uaMgrGetUserName(mgr);
		/*compare equal mean to match a user */
		if(strICmp(username,mgruser)==0){
			if((TX_CLIENT_INVITE == txType) || (TX_CLIENT_NON_INVITE== txType)){
				if(uaDlgMatchTx(mgr,_tx))
					break;
			}else
				break;
		}
		/* match super mgr */
		if(strICmp("",mgruser)==0)
			supermgr=mgr;
		mgr=NULL;
		mgruser=NULL;
	}
	/*after using it, should free it get from uaGetReqCallee(SipReq)*/
	if(freeptr)
		free(freeptr);
	if(mgr==NULL)
		mgr=supermgr;
	return mgr;
}

/*Tx will find a match Dialog, or create a new Dialog*/
/*compare with CallID to distinguish dialog*/
UaDlg uaDlgMatchTx(IN UaMgr _mgr,IN TxStruct _tx)
{
	UaDlg dlg=NULL;
	int dlgnum=0,dlgpos=0;
	DxLst dlgList=NULL;
	char *callid;
	
	if (sipTxGetOriginalReq(_tx)) 
		callid=(char*)sipReqGetHdr(sipTxGetOriginalReq(_tx),Call_ID);
	else
		callid=(char*)sipTxGetCallid(_tx);
	
	dlgList=uaMgrGetDlgLst(_mgr);
	if((_mgr != NULL)&&(dlgList != NULL)){
		dlgnum=dxLstGetSize(dlgList);
		for(dlgpos=0;dlgpos<dlgnum;dlgpos++){
			dlg=dxLstPeek(dlgList,dlgpos);
			if(strcmp(uaDlgGetCallID(dlg),callid)==0){
				return dlg;
			}
			dlg=NULL;
		}
	}
	return dlg;
}


/*find a TxStructure */
TxStruct uaDlgGetSpecTx(IN UaDlg _dlg, IN TXTYPE _txtype,IN SipMethodType _method)
{
	TxStruct targTx=NULL;
	int txnum=0,txpos=0;
	DxLst txList=NULL;
	TXTYPE tmptype=TX_NON_ASSIGNED;
	SipMethodType dlgmethod=UNKNOWN_METHOD;

	if(_dlg == NULL)
		return NULL;

	/*get transaction list */
	txList=uaDlgGetTxList(_dlg);
	if(txList == NULL)
		return NULL;

	txnum=dxLstGetSize(uaDlgGetTxList(_dlg));
	for(txpos=0;txpos < txnum;txpos++){
		targTx=dxLstPeek(txList,txpos);
		tmptype=sipTxGetType(targTx);
		dlgmethod=sipTxGetMethod(targTx);
		if((_txtype == tmptype)&&(_method==dlgmethod))
			break;
		else{
			targTx=NULL;
		}
	}
	return targTx;

}

/*delete transaction from TxList*/
RCODE uaDelTxFromTxList(IN UaDlg _dlg,IN DxLst _lst,IN TxStruct _tx)
{
	RCODE rCode=UATX_TX_NOT_FOUND;
	int txnum=0,txpos=0;
	TxStruct tmptx;

	if(_dlg == NULL)
		return UATX_DLG_NULL;
	if(_lst !=NULL){
		txnum=dxLstGetSize(_lst);
		/*search a exist tx*/
		for(txpos=0;txpos<txnum;txpos++){
			tmptx=dxLstPeek(_lst,txpos);
			if(tmptx == _tx){
				/*remove the found tx*/
				tmptx=dxLstGetAt(_lst,txpos);
				sipTxFree(tmptx);
				if(uaDlgGetTxCount(_dlg) >0)
					uaDlgDecreaseTxCount(_dlg);
				rCode=RC_OK;
				break;
			}
		}/*end for loop*/
	}

	return rCode;
}

/* handle timer of invite , register and subscribe */
void uaTimerManager(int e,void* param)
{
	UaTimer TimerData = (UaTimer)param;
	UaDlg dlg=NULL;
	UaMgr mgr=NULL;
	char	*tmpcontact=NULL;
	int	rc;
	int setype=0;
	long delay;

	struct timeval d = {0, 0};

	if (!TimerData)
		return;

	TCRPrintEx(TRACE_LEVEL_API,"[uaTimerManager] Entering\n");
	dlg=TimerData->dlg;
	/* check dialog */
	if(dlg==NULL){
		UaCoreERR("[uaTimerManager] dialog is NULL\n");
		return;
	}
	
	if (TimerData->expire > MILLION)
	{
		TCRPrintEx(TRACE_LEVEL_API,"[uaTimerManager] re-schedule huge timer \n");
		TimerData->expire -= MILLION;

		if (TimerData->expire < MILLION)
			delay=(long)TimerData->expire;
		else
			delay=MILLION;
		d.tv_sec = delay / 1000;
		d.tv_usec = (delay % 1000)*1000;

		rc=cxTimerSet(d,uaTimerManager, (void*)TimerData);
		
		if(rc == -1)
			UaCoreWARN("[uaTimerManager] re-schedule huge timer failed\n");
		else{
			/* update timer handle */
			switch (TimerData->event) {
				case REGISTER_EXPIRE:
					uaDlgSetRegTimerHandle(dlg,rc);
					break;
				case INVITE_EXPIRE:
					uaDlgSetTimerHandle(dlg,rc);
					break;
				case SUBSCRIBE_EXPIRE:
					break;
				case SESSION_TIMER:
					uaDlgSetSEHandle(dlg,-1,rc);
					break;
				default:
					break;
			}
		}
		return;
	}
	
	/* check manager */
	mgr=uaDlgGetMgr(dlg);
	if(mgr==NULL){
		UaCoreERR("[uaTimerManager] manager is NULL\n");
		return;
	}
	/* dispatch event and handle each process */
	switch (TimerData->event) {
	case REGISTER_EXPIRE:
		UaCoreWARN("[uaTimerManager] REGISTER expired\n");
		/* post to upper-layer, request from PCA */
		uaMgrPostNewMessage(uaDlgGetMgr(dlg),dlg,UAMSG_REGISTER_EXPIRED,uaGetReqCSeq(uaDlgGetRequest(dlg)),NULL,NULL);
		/* should re-transmit register */
		tmpcontact=uaMgrGetLastRegisterAddr(mgr);
		if(tmpcontact==NULL)
			break;
		uaMgrRegister(mgr,UA_REGISTER,tmpcontact);
		if(tmpcontact){
			free(tmpcontact);
			tmpcontact=NULL;
		}
		break;
	case INVITE_EXPIRE:{
		TxStruct tx=NULL;
		DxLst txList=NULL;
		int txnum=0,txpos=0;
		
		UaCoreERR("[uaTimerManager] INVITE expired\n");
		if(!dlg) break;
		if(IsDlgReleased(dlg)) break;
		SetDlgTimerd(dlg,FALSE);
		txnum=uaDlgGetTxCount(dlg);
		txList=uaDlgGetTxList(dlg);
		for(txpos=0;txpos<txnum;txpos++){
			/* get transaction type */
			tx=dxLstPeek(txList,txpos);
			if(sipTxGetType(tx) == TX_CLIENT_INVITE){
				/* for client transaction 
				 * => generate a cancel for it
				 */
				uaDlgCANCEL(dlg);
				break;
			}else if(sipTxGetType(tx) == TX_SERVER_INVITE){
				/* for server transaction 
				 * generate 487 for it
				 */
				uaDlgAnswerCall(dlg,UA_REJECT_CALL,Request_Terminated,NULL,NULL);
				break;
			}
			break;
		}/* end of for loop */
		}/* end of INVITE_EXPIRE */
		break;
	case SUBSCRIBE_EXPIRE:
		UaCoreERR("[uaTimerManager] SUBSCRIBE expired\n");
		break;
	case OK200_EXPIRE:	
		SetDlgTimerd(dlg,FALSE);
		/* if not get ack in 64*T1 ,so send BYE*/
		uaDlgDisconnect(dlg);
		UaCoreERR("[uaTimerManager] 200OK_EXPIRE expired\n");
		break;
	/* end of expire 200OK*/
	case CANCEL_EXPIRE:
		/* after 64*T1 and no response for invite, 
		 * should destroy the original request  
		 * rfc3261 page.54
		 */
		UaCoreERR("[uaTimerManager] CANCEL_EXPIRE expired\n");
		uaDlgDelInviteTx(dlg);
		uaDlgSetState(dlg,UACSTATE_DISCONNECT);
		uaMgrPostNewMessage(uaDlgGetMgr(dlg),dlg,UAMSG_CALLSTATE,uaGetReqCSeq(uaDlgGetRequest(dlg)),NULL,NULL);
		break;
	case SESSION_TIMER:
		/*	if uadlgstate is connected
		 *  for uac, send update and 
		 *  for uas, send bye to disconnect
		 */ 
		TCRPrintEx(TRACE_LEVEL_API,"[uaTimerManager] SESSIONTIMER_EXPIRE expired\n");
		if(uaDlgGetState(dlg)==UACSTATE_CONNECTED){
			setype=uaDlgGetSEType(dlg);
			if((setype==1)||(setype==2)){
				if(uaDlgGetRefreshMethod(dlg)==INVITE)
					uaDlgChangeSDP(dlg,uaDlgGetLocalSdp(dlg));
				else
					uaDlgUpdate(dlg,uaDlgGetRemoteTarget(dlg),NULL);
				uaDlgSetSEHandle(dlg,setype,uaAddTimer(dlg,SESSION_TIMER,(uaDlgGetSETimer(dlg)/2)*1000));
			}else if(setype==3){
				/* remove timer handle in dlg */
				uaDlgSetSEHandle(dlg,-1,-1);
				uaDlgDisconnect(dlg);
			}
		}else
			uaDlgSetSEHandle(dlg,-1,-1);
		break;
	default:
		break;
	}
	
	if (TimerData)
	{
		uaDlgRemoveReference( TimerData->dlg, &(TimerData->dlg));
		free(TimerData);
		TimerData = NULL;	
	}

	UaCoreWARN("[uaTimerManager] Leaving\n");
}

int uaAddTimer(UaDlg dlg , UaTimerEvent event, unsigned long expire)
{
	UaTimer TimerData = NULL;
	int rc;
	long delay;
	unsigned short retval;

	struct timeval d = {0, 0};

	if (!dlg)
		return -1;
		
	TimerData = (UaTimer)calloc(1,sizeof(struct _UaTimer));
	
	TimerData->dlg = dlg;
	TimerData->event = event;
	TimerData->expire = expire;
	
	/* sam add: 1 line*/
	uaDlgAddReference( dlg, &(TimerData->dlg));

	/*
	if (expire < 1000000)
		rc=CommonTimerAdd(expire,(void*)TimerData,uaTimerManager,&retval);
	else
		rc=CommonTimerAdd(1000000,(void*)TimerData,uaTimerManager,&retval);
	*/
	if (expire>MILLION)
		delay=MILLION;
	else
		delay=(long)expire;
		
	d.tv_sec = delay / 1000;	
	d.tv_usec = (delay % 1000)*1000;
	
	rc=cxTimerSet(d,uaTimerManager,(void*)TimerData);
	
	if ( rc == -1 ){
		UaCoreERR("[uaAddTimer] FATAL ERROR: cxTimer fail!!\n");
		return -1;
	}else
		TCRPrint(TRACE_LEVEL_API,"[uaAddTimer] cxTimer set successful!!\n");
	
	return rc;
}

void uaDelTimer(int TimerHandle)
{
	UaTimer TimerData=NULL;
	
	if(TimerHandle>0){
		TimerData=(UaTimer)cxTimerDel(TimerHandle);
		
	
		if (TimerData)
		{
			uaDlgRemoveReference( TimerData->dlg, &(TimerData->dlg));
			TimerData->dlg = NULL;
			TimerData->event = -1;
			TimerData->expire = 0;
			free(TimerData);
			TimerData = NULL;
		}
	}
	TCRPrintEx(TRACE_LEVEL_API,"[uaDelTimer] Get a final response -> delete timer of expires !\n");
}

