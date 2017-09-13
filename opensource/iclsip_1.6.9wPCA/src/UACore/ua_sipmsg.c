/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * ua_sipmsg.c
 *
 * $Id: ua_sipmsg.c,v 1.293 2006/07/13 03:27:06 tyhuang Exp $
 */

#include <stdio.h>
#include "ua_sipmsg.h"
#include "ua_mgr.h"
#include "ua_dlg.h"
#include "ua_user.h"
#include "ua_int.h"
#include <sipTx/sipTx.h>
#include <adt/dx_str.h>

#ifdef _WIN32_WCE /*WinCE didn't support time.h*/
#elif defined(_WIN32)
#include <time.h>
#elif defined(UNIX)
#include <time.h>
#endif

/*construct  a new sip message */
/* input a dialog, destination, sdp */
SipReq uaINVITEMsg(IN UaDlg _dlg,IN const char* _dest,IN const char* _refby,IN SdpSess _sdp)
{
	RCODE rCode;
	SipReq _this;
	UaMgr  mgr;
	UaUser user;
	UaCfg  cfg;
	SipReqLine *line=NULL;
	char buf[MAXBUFFERSIZE]={'\0'},sdpbuf[1024]={'\0'};
	char ip[MAXADDRESIZE]={'\0'},*username,*laddr,*hostname,*callid,*recroute,*dname;
	SipFrom *pFrom=NULL,*pFromTag=NULL;
	SipURL url=NULL;
	unsigned int bodylen=1024;
	SipBody *tmpBody=NULL;
	SipAllow pAllow;
	int maxfor=70,idx=0;	
	char *priv;

	/*check input parameter is NULL or not*/
	if((_dlg == NULL)||(_dest == NULL)){
		UaCoreERR("[uaINVITEMsg] dialog , destination or sdp is NULL!\n");
		return NULL;
	}
	/*get parent mgr, if NULL : Error */
	mgr=uaDlgGetMgr(_dlg);
	if(mgr==NULL){
		UaCoreERR("[uaINVITEMsg] mgr is NULL!\n");
		return NULL;
	}
	/*get user object, get user information*/
	user=uaMgrGetUser(mgr);
	if(user == NULL){
		UaCoreERR("[uaINVITEMsg] user is NULL!\n");
		return NULL;
	}
	/*get cfg object, get configuration information*/
	cfg=uaMgrGetCfg(mgr);
	if(cfg==NULL){
		UaCoreERR("[uaINVITEMsg] cfg is NULL!\n");
		return NULL;
	}
	/*create new empty request message*/
	_this=sipReqNew();
	if(_this == NULL){
		UaCoreERR("[uaINVITEMsg] To create new Request is failed!\n");
		return NULL;
	}
	/*request line*/
	url=sipURLNewFromTxt((char*)_dest);
	line=uaReqNewReqLine(INVITE,sipURLToStr(url),SIP_VERSION);
	if(url)
		sipURLFree(url);
	if(line != NULL){
		rCode=sipReqSetReqLine(_this,line);
		if(rCode != RC_OK){
			uaReqLineFree(line);
			sipReqFree(_this);
			UaCoreERR("[uaINVITEMsg] To set request line is failed!\n");
			return NULL;
		}
		uaReqLineFree(line);
	}else{
		sipReqFree(_this);
		UaCoreERR("[uaINVITEMsg] To create request line is failed!\n");
		return NULL;
	}

	username=(char*)uaUserGetName(user);
	laddr=(char*)uaUserGetLocalAddr(user);
	hostname=(char*)uaUserGetLocalHost(user);
	if((username==NULL)||(laddr==NULL)||(hostname==NULL)){
		sipReqFree(_this);
		UaCoreERR("[uaINVITEMsg] username, localaddr or hostname is NULL!!\n");
		return NULL;
	}

	/*copy local ip address into ip buffer*/
	strcpy(ip,laddr);

	
	/*Call-ID header*/
	callid=uaDlgGetCallID(_dlg);
	if(callid==NULL){
		callid=sipCallIDGen(hostname,ip);
		uaDlgSetCallID(_dlg,callid);
	}
	sprintf(buf,"Call-ID:%s\r\n",callid);
	rCode=sipReqAddHdrFromTxt(_this,Call_ID,buf);
	/*From header and Contact header. modified by tyhuang*/
	priv=uaDlgGetPrivacy(_dlg);
	if(priv){
		sprintf(buf,"Privacy:%s\r\n",priv);
		rCode=sipReqAddHdrFromTxt(_this,Privacy,buf);
		
	}
	if(uaDlgGetUserPrivacy(_dlg)==TRUE)
		sprintf(buf,"From:\"Anonymous\"<sip:anonymous@anonymous.invalid>\r\n");
	else{
		dname=(char*)uaUserGetDisplayName(user);
		if(dname)
			sprintf(buf,"From:\"%s\"<%s>\r\n",dname,uaMgrGetPublicAddr(mgr));
		else
			sprintf(buf,"From:\"%s\"<%s>\r\n",uaMgrGetUserName(mgr),uaMgrGetPublicAddr(mgr));
	}
	rCode=sipReqAddHdrFromTxt(_this,From,buf);
	
	priv=uaDlgGetPPreferredID(_dlg);
	/* P-Preferred-Identity */
	if(priv){
		sprintf(buf,"P-Preferred-Identity:%s\r\n",priv);
		rCode=sipReqAddHdrFromTxt(_this,P_Preferred_Identity,buf);
	}
	if(uaDlgGetSecureFlag(_dlg)==TRUE){
		if(uaUserGetSIPSContact(user))
			sprintf(buf,"Contact:%s\r\n",uaUserGetSIPSContact(user));
		else{
			UaCoreERR("[uaINVITEMsg] SIPS contact is not found!\n");
			sipReqFree(_this);
			return NULL;
		}
	}else
		sprintf(buf,"Contact:%s\r\n",uaMgrGetContactAddr(mgr));
	rCode=sipReqAddHdrFromTxt(_this,Contact,buf);
	pFrom=(SipFrom*)sipReqGetHdr(_this,From);
	pFromTag=uaFromDupAddTag(pFrom);
	if(pFromTag != NULL){
		rCode=sipReqAddHdr(_this,From,(void*)pFromTag);
		uaFromHdrFree(pFromTag);
	}
	/*Allow header*/	
	for(idx=0;idx<SIP_SUBSCRIBE;idx++)
		pAllow.Methods[idx]=1;

	sipReqAddHdr(_this,Allow,&pAllow);	
	/*To header*/
	sprintf(buf,"To:<%s>\r\n",_dest);
	rCode=sipReqAddHdrFromTxt(_this,To,buf);

	/*Referred-By*/
	if(NULL!=_refby){
		sprintf(buf,"Referred-By:%s\r\n",_refby);
		sipReqAddHdrFromTxt(_this,Referred_By,buf);
	}
	/*Max-Forward. modified by tyhuang */
	maxfor=uaUserGetMaxForwards(user);
	if(maxfor<70) maxfor=70;
	sprintf(buf,"Max-Forwards:%d\r\n",maxfor);
	if((rCode=sipReqAddHdrFromTxt(_this,Max_Forwards,buf)) != RC_OK)
		UaCoreERR("[uaINVITEMsg]add MaxForward failed!\n");

	/*CSeq header*/
	uaMgrAddCmdSeq(uaDlgGetMgr(_dlg));
	sprintf(buf,"CSeq:%d INVITE\r\n",uaMgrGetCmdSeq(mgr));
	rCode=sipReqAddHdrFromTxt(_this,CSeq,buf);
	
	
	/* Expires header */
	if(DEFAULT_EXPIRES>0){
		sprintf(buf,"Expires:%d\r\n",DEFAULT_EXPIRES);
		rCode=sipReqAddHdrFromTxt(_this,Expires,buf);
	}
	
	/* Session-Expires header */
	if(uaDlgGetSETimer(_dlg)>0){
		sprintf(buf,"Session-Expires:%d\r\n",uaDlgGetSETimer(_dlg));
		rCode=sipReqAddHdrFromTxt(_this,Session_Expires,buf);
	}


	/* Record-Route */
	recroute=(char*)uaDlgGetReqRecordRoute(_dlg);
	if(recroute){
		sprintf(buf,"Record-Route:%s\r\n",recroute);
		sipReqAddHdrFromTxt(_this,Record_Route,buf);
	}

	/* enable/disable session-timer */
	/* Supported Header */
	if(uaDlgGetSupported(_dlg)){
		sprintf(buf,"Supported:%s\r\n",uaDlgGetSupported(_dlg));
	}else{
		if (uaDlgGetSETimer(_dlg)>0) 
			sprintf(buf,"Supported:%s\r\n",SUPPORT_LIST);
		else
			sprintf(buf,"Supported:%s\r\n",SUPPORT_LIST_NO_TIMER);
	}
	rCode=sipReqAddHdrFromTxt(_this,Supported,buf);

#ifdef _DEBUG
	/* User-Agent header */
	sprintf(buf,"User-Agent:%s\r\n",USER_AGENT);
	rCode=sipReqAddHdrFromTxt(_this,User_Agent,buf);
#endif
	
	if(_sdp){
		/*body*/
		bodylen=1024;
		if(RC_ERROR ==(rCode=sdpSess2Str(_sdp,sdpbuf,&bodylen))){
			sipReqFree(_this);
			UaCoreERR("[uaINVITEMsg] sdpSess2Str is failed!\n");
			return NULL;
		}
		/* Content-Length */
		sprintf(buf,"Content-Length:%d\r\n",bodylen);
		rCode=sipReqAddHdrFromTxt(_this,Content_Length,buf);
		if(bodylen>0){
			rCode=sipReqAddHdrFromTxt(_this,Content_Type,"Content-Type:application/sdp\r\n");
#ifdef _DEBUG			
			/* Content-Disposition */
			rCode=sipReqAddHdrFromTxt(_this,Content_Disposition,"Content-Disposition:session\r\n");
			/* Content-Language */
			sprintf(buf,"Content-Language:%s\r\n",DEFAULT_LANGUAGE);
			rCode=sipReqAddHdrFromTxt(_this,Content_Language,buf);
#endif
			/* Content-Encoding 
			sprintf(buf,"Content-Encoding:%s\r\n",DEFAULT_ENCODING);
			rCode=sipReqAddHdrFromTxt(_this,Content_Encoding,buf);*/
		}
	}else{
		/* Content-Length */
		sprintf(buf,"Content-Length:0\r\n");
	}
	
	if(_sdp!=NULL){
		tmpBody=(SipBody*)calloc(1,sizeof(SipBody));
		tmpBody->length=bodylen;
		tmpBody->content=(unsigned char*)sdpbuf;
		rCode=sipReqSetBody(_this,tmpBody);
		if(rCode != RC_OK){
		sipReqFree(_this);
		_this=NULL;
		UaCoreERR("[uaINVITEMsg] To set body is failed!\n");
		}
		if(tmpBody)
			free(tmpBody);
	}
	
	
#ifdef _DEBUG
	TCRPrint(TRACE_LEVEL_API,"[uaINVITEMsg]create a INVITE message\n%s!\n",sipReqPrint(_this));
#endif
	return _this;
}

/*gvie dialog and original response,create CANCEL message */
SipReq uaCANCELMsg(IN UaDlg _dlg,IN SipReq _origreq,IN SipRsp _lastrsp)
{
	SipReq cancelMsg=NULL;
	UACallStateType callstate=UACSTATE_UNKNOWN;
	SipCSeq *pCancelCseq=NULL,*pOrigCseq=NULL;
	SipReqLine *pOrigLine=NULL,*pCancelLine=NULL;
	char buf[MAXBUFFERSIZE]={'\0'};
	int maxfor,rCode;
	UaUser user;

	if((_dlg == NULL)||(_origreq == NULL)){
		UaCoreERR("[uaCANCELMsg] Error! dialog or original request is NULL!\n");
		return NULL;
	}
	user=uaMgrGetUser(uaDlgGetMgr(_dlg));
	callstate=uaDlgGetState(_dlg);
	/* modified by tyhuang .add (callstate == UACSTATE_RETRIEVING) */
	if((callstate == UACSTATE_PROCEEDING)||
	   (callstate == UACSTATE_RINGBACK) ||
	   (callstate == UACSTATE_DIALING)||
	   (callstate == UACSTATE_RETRIEVING)){
		cancelMsg=sipReqNew();
		if(cancelMsg==NULL){
			UaCoreERR("[uaCANCELMsg] To create new Request is failed!\n");
			return NULL;
		}
		/*add request line*/
		pOrigLine=sipReqGetReqLine(_origreq);
		if(pOrigLine != NULL){
			pCancelLine=uaReqNewReqLine(CANCEL,pOrigLine->ptrRequestURI,pOrigLine->ptrSipVersion);
			sipReqSetReqLine(cancelMsg,pCancelLine);
			uaReqLineFree(pCancelLine);
		}

		/*add TO header,copy from original request, if have resposne copy from response*/
		/*if(_lastrsp != NULL)
			sipReqAddHdr(cancelMsg,To,sipRspGetHdr(_lastrsp,To));
		else marked by tyhuang 2003.9.8-
		to-tag should respect to original request
		*/
		sipReqAddHdr(cancelMsg,To,sipReqGetHdr(_origreq,To));
		/*add From header, copy from original request*/
		sipReqAddHdr(cancelMsg,From,sipReqGetHdr(_origreq,From));
		/*add Call-ID header, copy from original request*/
		sipReqAddHdr(cancelMsg,Call_ID,sipReqGetHdr(_origreq,Call_ID));
		/*add CSeq header, same as original request*/
		pOrigCseq=sipReqGetHdr(_origreq,CSeq);
		if(pOrigCseq != NULL){
			pCancelCseq=uaNewCSeq(pOrigCseq->seq,CANCEL);
			sipReqAddHdr(cancelMsg,CSeq,pCancelCseq);
			uaFreeCSeq(pCancelCseq);
		}

#ifdef _DEBUG
		/* User-Agent header */
		sprintf(buf,"User-Agent:%s\r\n",USER_AGENT);
		sipReqAddHdrFromTxt(cancelMsg,User_Agent,buf);
#endif

		/*Max-Forward. modified by tyhuang */
		maxfor=uaUserGetMaxForwards(user);
		if(maxfor<70) maxfor=70;
		sprintf(buf,"Max-Forwards:%d\r\n",maxfor);
		if((rCode=sipReqAddHdrFromTxt(cancelMsg,Max_Forwards,buf)) != RC_OK)
			UaCoreERR("[uaCANCELMsg]add MaxForward failed!\n");
		
		uaReqAddBody(cancelMsg,NULL,NULL);
	} /*end if */
	TCRPrint(TRACE_LEVEL_API,"[uaCANCELMsg]create a CANCEL message\n%s!\n",sipReqPrint(cancelMsg));
	return cancelMsg;
}

/* When INVITE Transaction as Client:					*/
/*	if contain Contact header,							*/
/*		copy from contact header						*/
/*	else												*/
/*		copy request-Line from original request message	*/
/*	From: form original Req								*/
/*	To:   copy from final response						*/
/*	CallID: copy from original request					*/
/*	CSeq:	original req +1								*/
/* When INVITE Transacetion as Server:					*/
/*	Request Line: if request contain Contact			*/
/*			 copy from Contact							*/
/*		      else										*/
/*			 copy from From								*/
/*	From:copy from response								*/
/*	To:  copy from response								*/
/*	CallID: copy from response							*/
/*	CSeq:	randon										*/
/*	Add Contact header									*/
SipReq uaBYEMsg(IN UaDlg _dlg,IN UaContent _content)
{
	SipReq origReq,byeReq=NULL;
	SipRsp finalRsp=NULL;
	TxStruct tx;
	SipReqLine *pLine=NULL,*pNewLine=NULL;
	SipCSeq *cseq;
	RCODE rCode;
	SipContact *pContact=NULL;
	SipURL url=NULL;
	BOOL bContactOK=FALSE;
	SipFrom *pFrom=NULL;
	char buf[MAXBUFFERSIZE]={'\0'};
	UaMgr mgr=NULL;
	SipRecordRoute *pRecordRoute=NULL;
	SipRoute *pRoute=NULL;
	char uribuf[256]={'\0'};
	int  urilen=0,maxfor,bodylen;
	TXTYPE txType;
	SipBody *tmpBody=NULL;
	char *priv;

	if(_dlg==NULL){
		UaCoreERR("[uaBYEMsg] dialog is NULL!\n");
		return NULL;
	}
	mgr=uaDlgGetMgr(_dlg);
	if(mgr== NULL){
		UaCoreERR("[uaBYEMsg] mgr is NULL!\n");
		return NULL;
	}
	tx=uaDlgGetInviteTx(_dlg,TRUE);
	if(tx != NULL){
		byeReq=sipReqNew();
		if(byeReq == NULL){
			UaCoreERR("[uaBYEMsg] To create new Request is failed!\n");
			return NULL;
		}
		origReq=sipTxGetOriginalReq(tx);
		finalRsp=sipTxGetLatestRsp(tx);

		/*Client transaction*/
		txType=sipTxGetType(tx);
		if(TX_CLIENT_INVITE == txType ){
			if(origReq == NULL){
				sipReqFree(byeReq);
				UaCoreERR("[uaBYEMsg] original request is NULL!\n");
				return NULL;
			}
			/*final response contain Contact header*/
			pContact=sipRspGetHdr(finalRsp,Contact);
			if(pContact !=NULL)
				bContactOK=TRUE;

			
			/*Set request line*/
			pLine=sipReqGetReqLine(origReq);
			if(bContactOK){
				/*copy Contact header as Request-URI */
				/*uaContactAsReqURI(pContact,uribuf,256,&urilen);*/
				sprintf(uribuf,"%s",uaDlgGetRemoteTarget(_dlg));
				pNewLine=uaReqNewReqLine(BYE,uribuf,SIP_VERSION);
				/*pNewLine=uaReqNewReqLine(BYE,uribuf,pLine->ptrSipVersion);*/
			}else
				pNewLine=uaReqNewReqLine(BYE,pLine->ptrRequestURI,pLine->ptrSipVersion);
			rCode=sipReqSetReqLine(byeReq,pNewLine);
			uaReqLineFree(pNewLine);
			if(rCode != RC_OK){
				sipReqFree(byeReq);
				UaCoreERR("[uaBYEMsg] To set request line is failed!\n");
				return NULL;
			}
			
			/*copy From header */
			rCode=sipReqAddHdr(byeReq,From,sipReqGetHdr(origReq,From));
			
			/*copy Call-ID header*/
			rCode=sipReqAddHdr(byeReq,Call_ID,sipReqGetHdr(origReq,Call_ID));
			
			/*copy To header*/
			rCode=sipReqAddHdr(byeReq,To,sipRspGetHdr(finalRsp,To));
			
			/*copy CSeq*/
			uaMgrAddCmdSeq(mgr);
			cseq=uaNewCSeq(uaMgrGetCmdSeq(mgr),BYE);
			sipReqAddHdr(byeReq,CSeq,cseq);
			uaFreeCSeq(cseq);

			/*Add Route header if exist Reocord-Route header*/
			pRecordRoute=(SipRecordRoute*)sipRspGetHdr(finalRsp,Record_Route);
			if(pRecordRoute != NULL){
				pRoute=uaRecordRouteCovertToRoute(pRecordRoute);
				if(pRoute != NULL){
					sipReqAddHdr(byeReq,Route,pRoute);
					uaRouteFree(pRoute);
				}
			}	
		}else if(TX_SERVER_INVITE==txType) {
			/*Server transaction*/
			pContact=sipReqGetHdr(origReq,Contact);
			if(pContact != NULL){
				url=sipURLNewFromTxt(pContact->sipContactList->address->addr);
				if(url != NULL)
					bContactOK=TRUE;
			}
			if(bContactOK){/*copy from Contact */
				/*uaContactAsReqURI(pContact,uribuf,256,&urilen);*/
				sprintf(uribuf,"%s",uaDlgGetRemoteTarget(_dlg));
				pNewLine=uaReqNewReqLine(BYE,uribuf,SIP_VERSION);
				sipURLFree(url);
			}else{ /*copy from from header*/
				pFrom=sipRspGetHdr(finalRsp,From);
				if(pFrom != NULL)
					pNewLine=uaReqNewReqLine(BYE,pFrom->address->addr,SIP_VERSION);
				else{
					sipReqFree(byeReq);
					UaCoreERR("[uaBYEMsg] From header is NULL!\n");
					return NULL;
				}
			}
			rCode=sipReqSetReqLine(byeReq,pNewLine);
			uaReqLineFree(pNewLine);
			
			/*copy From header */
			/*notice that copy from To header, since should copy the origin tag*/
			rCode=sipReqAddHdr(byeReq,From,sipRspGetHdr(finalRsp,To));
			
			/*copy To header*/
			/*notice that copy from From header, since should copy the origin tag*/
			rCode=sipReqAddHdr(byeReq,To,sipRspGetHdr(finalRsp,From));

			/*copy Call-ID header*/
			rCode=sipReqAddHdr(byeReq,Call_ID,sipReqGetHdr(origReq,Call_ID));
			
			/*copy CSeq*/
			uaMgrAddCmdSeq(uaDlgGetMgr(_dlg));
			cseq=uaNewCSeq(uaMgrGetCmdSeq(uaDlgGetMgr(_dlg)),BYE);
			rCode=sipReqAddHdr(byeReq,CSeq,cseq);
			uaFreeCSeq(cseq);
			
			/*Add Route header if exist Reocord-Route header*/
			pRecordRoute=(SipRecordRoute*)sipRspGetHdr(finalRsp,Record_Route);
			if(pRecordRoute != NULL){
				pRoute=(SipRoute*)pRecordRoute;
				sipReqAddHdr(byeReq,Route,pRoute);
			}
			
		}

#ifdef _DEBUG
		/* User-Agent header */
		sprintf(buf,"User-Agent:%s\r\n",USER_AGENT);
		rCode=sipReqAddHdrFromTxt(byeReq,User_Agent,buf);
#endif
		/* Content-Language 
		sprintf(buf,"Content-Language:%s\r\n",DEFAULT_LANGUAGE);
		rCode=sipReqAddHdrFromTxt(byeReq,Content_Language,buf);*/
		/* Content-Encoding 
		sprintf(buf,"Content-Encoding:%s\r\n",DEFAULT_ENCODING);
		rCode=sipReqAddHdrFromTxt(byeReq,Content_Encoding,buf);*/
		/*Max-Forward. modified by tyhuang */
		maxfor=uaUserGetMaxForwards(uaMgrGetUser(mgr));
		if(maxfor<70) maxfor=70;
		sprintf(buf,"Max-Forwards:%d\r\n",maxfor);
		if((rCode=sipReqAddHdrFromTxt(byeReq,Max_Forwards,buf)) != RC_OK)
			UaCoreERR("[uaBYEMsg]add MaxForward failed!\n");
		/* privacy */
		priv=uaDlgGetPrivacy(_dlg);
		if(priv){
			sprintf(buf,"Privacy:%s\r\n",priv);
			rCode=sipReqAddHdrFromTxt(byeReq,Privacy,buf);
		}

		priv=uaDlgGetPPreferredID(_dlg);
		/* P-Preferred-Identity */
		if(priv){
				sprintf(buf,"P-Preferred-Identity:%s\r\n",priv);
				rCode=sipReqAddHdrFromTxt(byeReq,P_Preferred_Identity,buf);
		}
		bodylen=uaContentGetLength(_content);
		if(bodylen>0){
			sprintf(buf,"Content-Length:%d\r\n",strlen(uaContentGetBody(_content)));
			rCode=sipReqAddHdrFromTxt(byeReq,Content_Length,buf);
			sprintf(buf,"Content-Type:%s\r\n",uaContentGetType(_content));
			rCode=sipReqAddHdrFromTxt(byeReq,Content_Type,buf);
		}else{
			sprintf(buf,"Content-Length:0\r\n");
			rCode=sipReqAddHdrFromTxt(byeReq,Content_Length,buf);
		}
		/*Add NULL body*/
		if(_content==NULL){
			uaReqAddBody(byeReq,NULL,NULL);
		}else{
			tmpBody=(SipBody*)calloc(1,sizeof(SipBody));
			tmpBody->length=bodylen;
			tmpBody->content=strDup(uaContentGetBody(_content));
			rCode=sipReqSetBody(byeReq,tmpBody);
			if(rCode != RC_OK){
				sipReqFree(byeReq);
				byeReq=NULL;
			}
		}
		if(tmpBody)
			sipBodyFree(tmpBody);
	}
#ifdef _DEBUG
	TCRPrint(TRACE_LEVEL_API,"[uaBYEMsg]create a BYE message\n%s!\n",sipReqPrint(byeReq));
#endif
	return byeReq;
}

/*reInvite message: when changed SDP or Hold */
SipReq	uaReINVITEMsg(IN UaDlg _dlg, IN SdpSess _sdp)
{
	SipReq origReq=NULL,reInvReq=NULL;
	SipRsp finalRsp=NULL;
	TxStruct tx=NULL;
	SipReqLine *pLine=NULL,*pNewLine=NULL;
	SipCSeq *cseq;
	RCODE rCode;
	SipContact *pContact=NULL;
	SipURL url=NULL;
	BOOL bContactOK=FALSE;
	SipFrom *pFrom=NULL;
	UaMgr mgr=NULL;
	UaUser user=NULL;
	SipRecordRoute *pRecordRoute=NULL;
	SipRoute *pRoute=NULL;
	char uribuf[256]={'\0'},buf[MAXBUFFERSIZE]={'\0'};
	SipBody *tmpBody=NULL;
	char sdpbuf[1024]={'\0'};
	unsigned int bodylen=1024,maxfor;
	TXTYPE txType;
	char *priv,*authz=NULL;

	if(_dlg==NULL){
		UaCoreERR("[uaReINVITEMsg] dialog is NULL!\n");
		return NULL;
	}
	mgr=uaDlgGetMgr(_dlg);
	if(mgr==NULL){
		UaCoreERR("[uaReINVITEMsg] mgr is NULL!\n");
		return NULL;
	}
	user=uaMgrGetUser(mgr);
	if(!user){
		UaCoreERR("[uaReINVITEMsg] user is NULL!\n");
		return NULL;
	}
	/*get INVITE Transaction*/
	tx=uaDlgGetInviteTx(_dlg,TRUE);
	if(tx != NULL){
		reInvReq=sipReqNew();
		if(reInvReq == NULL){
			UaCoreERR("[uaReINVITEMsg] To create new Request is failed!\n");
			return NULL;
		}
		origReq=sipTxGetOriginalReq(tx);
		finalRsp=sipTxGetLatestRsp(tx);

		/*Client transaction*/
		txType=sipTxGetType(tx);
		if( TX_CLIENT_INVITE == txType){
			if(origReq == NULL){
				sipReqFree(reInvReq);
				UaCoreERR("[uaReINVITEMsg] original request is NULL!\n");
				return NULL;
			}
			/*final response contain Contact header*/
			pContact=sipRspGetHdr(finalRsp,Contact);
			if(pContact !=NULL)
				bContactOK=TRUE;
			
			/*Set request line*/
			pLine=sipReqGetReqLine(origReq);
			if(bContactOK){
				/*copy Contact header as Request-URI */
				/*uaContactAsReqURI(pContact,uribuf,256,&urilen);*/
				sprintf(uribuf,"%s",uaDlgGetRemoteTarget(_dlg));
				pNewLine=uaReqNewReqLine(INVITE,uribuf,pLine->ptrSipVersion);
			}else
				pNewLine=uaReqNewReqLine(INVITE,pLine->ptrRequestURI,pLine->ptrSipVersion);
			rCode=sipReqSetReqLine(reInvReq,pNewLine);
			uaReqLineFree(pNewLine);
			if(rCode != RC_OK){
				sipReqFree(reInvReq);
				UaCoreERR("[uaReINVITEMsg] To set request line is failed!\n");
				return NULL;
			}
			
			/*copy From header */
			rCode=sipReqAddHdr(reInvReq,From,sipReqGetHdr(origReq,From));
			
			/*copy Call-ID header*/
			rCode=sipReqAddHdr(reInvReq,Call_ID,sipReqGetHdr(origReq,Call_ID));
			
			/*copy To header*/
			rCode=sipReqAddHdr(reInvReq,To,sipRspGetHdr(finalRsp,To));
			
			/*copy CSeq*/
			uaMgrAddCmdSeq(mgr);
			cseq=uaNewCSeq(uaMgrGetCmdSeq(mgr),INVITE);
			sipReqAddHdr(reInvReq,CSeq,cseq);
			uaFreeCSeq(cseq);

			/*Add Route header if exist Reocord-Route header*/
			pRecordRoute=(SipRecordRoute*)sipRspGetHdr(finalRsp,Record_Route);
			if(pRecordRoute != NULL){
				pRoute=uaRecordRouteCovertToRoute(pRecordRoute);
				if(pRoute != NULL){
					sipReqAddHdr(reInvReq,Route,pRoute);
					uaRouteFree(pRoute);
				}
			}
		}else if(TX_SERVER_INVITE==txType){
			/*Server transaction*/
			pContact=sipReqGetHdr(origReq,Contact);
			if(pContact != NULL){
				url=sipURLNewFromTxt(pContact->sipContactList->address->addr);
				if(url != NULL)
					bContactOK=TRUE;
			}
			if(bContactOK){/*copy from Contact */
				/*uaContactAsReqURI(pContact,uribuf,256,&urilen);*/
				sprintf(uribuf,"%s",uaDlgGetRemoteTarget(_dlg));
				pNewLine=uaReqNewReqLine(INVITE,uribuf,SIP_VERSION);
				sipURLFree(url);
			}else{ /*copy from from header*/
				pFrom=sipRspGetHdr(finalRsp,From);
				if(pFrom != NULL)
					pNewLine=uaReqNewReqLine(INVITE,pFrom->address->addr,SIP_VERSION);
				else{
					sipReqFree(reInvReq);
					UaCoreERR("[uaReINVITEMsg] From header is NULL!\n");
					return NULL;
				}
			}
			rCode=sipReqSetReqLine(reInvReq,pNewLine);
			uaReqLineFree(pNewLine);
			
			/*copy From header */
			/*notice that copy from To header, since should copy the origin tag*/
			rCode=sipReqAddHdr(reInvReq,From,sipRspGetHdr(finalRsp,To));
			
			/*copy To header*/
			/*notice that copy from From header, since should copy the origin tag*/
			rCode=sipReqAddHdr(reInvReq,To,sipRspGetHdr(finalRsp,From));

			/*copy Call-ID header*/
			rCode=sipReqAddHdr(reInvReq,Call_ID,sipReqGetHdr(origReq,Call_ID));
			
			/*copy CSeq*/
			uaMgrAddCmdSeq(uaDlgGetMgr(_dlg));
			cseq=uaNewCSeq(uaMgrGetCmdSeq(uaDlgGetMgr(_dlg)),INVITE);
			rCode=sipReqAddHdr(reInvReq,CSeq,cseq);
			uaFreeCSeq(cseq);

			/*Add Route header if exist Reocord-Route header*/
			pRecordRoute=(SipRecordRoute*)sipRspGetHdr(finalRsp,Record_Route);
			if(pRecordRoute != NULL){
				pRoute=(SipRoute*)pRecordRoute;
				sipReqAddHdr(reInvReq,Route,pRoute);
			}
		}

		/*Add Contact header */ 
		if(uaDlgGetSecureFlag(_dlg)==TRUE){
			if(uaUserGetSIPSContact(user))
				sprintf(buf,"Contact:%s\r\n",uaUserGetSIPSContact(user));
			else{
				UaCoreERR("[uaINVITEMsg] SIPS contact is not found!\n");
				sipReqFree(reInvReq);
				return NULL;
			}
		}else
			sprintf(buf,"Contact:%s\r\n",uaMgrGetContactAddr(mgr));
		
		rCode=sipReqAddHdrFromTxt(reInvReq,Contact,buf);

#ifdef _DEBUG
		/* User-Agent header */
		sprintf(buf,"User-Agent:%s\r\n",USER_AGENT);
		rCode=sipReqAddHdrFromTxt(reInvReq,User_Agent,buf);

		/* Content-Language */
		sprintf(buf,"Content-Language:%s\r\n",DEFAULT_LANGUAGE);
		rCode=sipReqAddHdrFromTxt(reInvReq,Content_Language,buf);
#endif
		/*Max-Forward. modified by tyhuang */
		maxfor=uaUserGetMaxForwards(uaMgrGetUser(mgr));
		if(maxfor<70) maxfor=70;
		sprintf(buf,"Max-Forwards:%d\r\n",maxfor);
		if((rCode=sipReqAddHdrFromTxt(reInvReq,Max_Forwards,buf)) != RC_OK)
			UaCoreERR("[uaReINVITEMsg]add MaxForward failed!\n");
		
		/* privacy */
		priv=uaDlgGetPrivacy(_dlg);
		if(priv){
			sprintf(buf,"Privacy:%s\r\n",priv);
			rCode=sipReqAddHdrFromTxt(reInvReq,Privacy,buf);
		}

		priv=uaDlgGetPPreferredID(_dlg);
		/* P-Preferred-Identity */
		if(priv){
			sprintf(buf,"P-Preferred-Identity:%s\r\n",priv);
			rCode=sipReqAddHdrFromTxt(reInvReq,P_Preferred_Identity,buf);
		}
		/* Session-Expires header */
		if(uaDlgGetSETimer(_dlg)>0){
			sprintf(buf,"Session-Expires:%d;refresher=uac\r\n",uaDlgGetSETimer(_dlg));
			rCode=sipReqAddHdrFromTxt(reInvReq,Session_Expires,buf);
		}

		/* Supported header */
		if(uaDlgGetSupported(_dlg)){
			sprintf(buf,"Supported:%s\r\n",uaDlgGetSupported(_dlg));
			rCode=sipReqAddHdrFromTxt(reInvReq,Supported,buf);
		}

		/*Add New sdp body*/
		if(_sdp != NULL){
			/*body*/
			if(RC_ERROR ==(rCode=sdpSess2Str(_sdp,sdpbuf,&bodylen))){
				sipReqFree(reInvReq);
				UaCoreERR("[uaReINVITEMsg] sdpSess2Str is failed!\n");
				return NULL;
			}
			sprintf(buf,"Content-Length:%d\r\n",bodylen);
			rCode=sipReqAddHdrFromTxt(reInvReq,Content_Length,buf);
			rCode=sipReqAddHdrFromTxt(reInvReq,Content_Type,"Content-Type:application/sdp\r\n");

			tmpBody=(SipBody*)calloc(1,sizeof(SipBody));
			tmpBody->length=bodylen;
			tmpBody->content=(unsigned char*)sdpbuf;
			uaReqAddBody(reInvReq,tmpBody,"application/sdp");
			if(tmpBody)
				free(tmpBody);
		}else
			uaReqAddBody(reInvReq,NULL,NULL);
	}
#ifdef _DEBUG
	TCRPrint(TRACE_LEVEL_API,"[uareINVITEMsg]create a reINVITE message\n%s!\n",sipReqPrint(reInvReq));
#endif
	return reInvReq;
}

SipReq uaACKMsg(IN UaDlg dlg,IN TxStruct _tx,IN SdpSess _sdp)
{
	SipReq req=NULL,origReq=NULL;
	SipRsp finalRsp=NULL;
	SipReqLine *pLine=NULL,*pNewLine=NULL;
	SipContact *pContact=NULL;
	SipCSeq *pCseq=NULL,*pNewCseq=NULL;
	SipRoute *pRoute=NULL;
	SipRecordRoute *pRecordRoute=NULL;
	SipAuthz *authz=NULL;
	char* contactbuf=NULL,buf[MAXBUFFERSIZE]={'\0'};
	int maxfor;
	RCODE rCode;
	char *priv;

	UaMgr mgr=NULL;
	UaUser user=NULL;
	/*
	TxStruct tx;
	TXTYPE	txType;
	SipReq invitereq;
	SipRsp invitersp;
	*/
	
	/*check dialog and transaction*/
	if((dlg==NULL)||(_tx==NULL)){
		UaCoreERR("[uaACKMsg] Error! get NULL dialog or transaction!\n");
		return NULL;
	}
	/*create new request message for ACK*/
	req=sipReqNew();
	if(req==NULL){
		UaCoreERR("[uaACKMsg] To create new Request is failed!\n");
		return NULL;
	}
	/*get original request message and response message*/
	/*origReq=sipTxGetOriginalReq(_tx);*/
	origReq=uaDlgGetRequest(dlg);
	if(origReq==NULL)
		origReq=sipTxGetOriginalReq(_tx);
	finalRsp=sipTxGetLatestRsp(_tx);
	
	if((origReq==NULL)||(finalRsp==NULL)){
		sipReqFree(req);
		UaCoreERR("[uaACKMsg] original request message or response message is NULL!\n");
		return NULL;
	}

	/*copy request line*/
	pLine=sipReqGetReqLine(origReq);
	if(pLine == NULL){
		sipReqFree(req);
		UaCoreERR("[uaACKMsg] To set request line is failed!\n");
		return NULL;
	}
	/*if Contact header exist, copy from Contact header*/
	pContact=sipRspGetHdr(finalRsp,Contact);
	if(pContact != NULL){
		if(pContact->sipContactList != NULL)
			if(pContact->sipContactList->address!=NULL){
				SipURL url;
				/*check address is correct or not */
				url=sipURLNewFromTxt(pContact->sipContactList->address->addr);
				if(url==NULL){
					free(contactbuf);
					contactbuf=NULL;
				}else
					sipURLFree(url);
			}
	}
	
	pRecordRoute=(SipRecordRoute*)sipRspGetHdr(finalRsp,Record_Route);
	if(pRecordRoute){
		pRoute=uaRecordRouteCovertToRoute(pRecordRoute);
		/*
		if(pRoute != NULL){
			sipReqAddHdr(req,Route,pRoute);
			uaRouteFree(pRoute);
		}*/
	}else{
		pRoute=(SipRoute*)sipReqGetHdr(origReq,Route);	
	}
	
	if(pRoute != NULL){
		sipReqAddHdr(req,Route,pRoute);
		uaRouteFree(pRoute);
	}

	if(contactbuf != NULL){
		pNewLine=uaReqNewReqLine(ACK,contactbuf,SIP_VERSION);
		free(contactbuf);
	}else{
		if(pContact && pContact->sipContactList &&pContact->sipContactList->address)
			pNewLine=uaReqNewReqLine(ACK,pContact->sipContactList->address->addr,SIP_VERSION);
		else
			pNewLine=uaReqNewReqLine(ACK,pLine->ptrRequestURI,SIP_VERSION);
	}
	sipReqSetReqLine(req,pNewLine);
	uaReqLineFree(pNewLine);

	/*copy From,To,Call_ID header from response*/
	sipReqAddHdr(req,From,sipRspGetHdr(finalRsp,From));
	sipReqAddHdr(req,To,sipRspGetHdr(finalRsp,To));
	sipReqAddHdr(req,Call_ID,sipRspGetHdr(finalRsp,Call_ID));
	/*copy CSeq header*/
	pCseq=sipRspGetHdr(finalRsp,CSeq);
	pNewCseq=uaNewCSeq(pCseq->seq,ACK);
	if(pNewCseq!=NULL){
		sipReqAddHdr(req,CSeq,pNewCseq);
		uaFreeCSeq(pNewCseq);
	}else{
		sipReqFree(req);
		UaCoreERR("[uaACKMsg] To create new CSeq is failed!\n");
		return NULL;
	}

	/* add contact header */
	mgr=uaDlgGetMgr(dlg);
	user=uaMgrGetUser(mgr);
	if(uaDlgGetSecureFlag(dlg)==TRUE){
		if(uaUserGetSIPSContact(user))
			sprintf(buf,"Contact:%s\r\n",uaUserGetSIPSContact(user));
	}else{
		if(mgr != NULL)
			sprintf(buf,"Contact:%s\r\n",uaMgrGetContactAddr(mgr));
	}
	if((rCode=sipReqAddHdrFromTxt(req,Contact,buf)) != RC_OK)
		UaCoreERR("[uaACKMsg]add Contact failed!\n");
#ifdef _DEBUG	
	/* User-Agent header */
	sprintf(buf,"User-Agent:%s\r\n",USER_AGENT);
	sipReqAddHdrFromTxt(req,User_Agent,buf);

#endif

	/*Max-Forward. modified by tyhuang */
	maxfor=uaUserGetMaxForwards(uaMgrGetUser(uaDlgGetMgr(dlg)));
	if(maxfor<70) maxfor=70;
	sprintf(buf,"Max-Forwards:%d\r\n",maxfor);
	if((rCode=sipReqAddHdrFromTxt(req,Max_Forwards,buf)) != RC_OK)
		UaCoreERR("[uaACKMsg]add MaxForward failed!\n");
	/* privacy */	
	priv=uaDlgGetPrivacy(dlg);
	if(priv){
		sprintf(buf,"Privacy:%s\r\n",priv);
		rCode=sipReqAddHdrFromTxt(req,Privacy,buf);
	}

	/* authz*/
	authz=sipReqGetHdr(origReq,Authorization);
	if(authz)
		sipReqAddHdr(req,Authorization,authz);
	
	authz=sipReqGetHdr(origReq,Proxy_Authorization);
	/* proxy-authz */
	if(authz)
		sipReqAddHdr(req,Proxy_Authorization,authz);
	
	if(_sdp !=NULL){
		SipBody *tmpBody=NULL;
		char sdpbuf[1024]={'\0'};
		unsigned int bodylen=1024;

		if(RC_ERROR == (sdpSess2Str(_sdp,sdpbuf,&bodylen))){
			sipReqFree(req);
			UaCoreERR("[uaACKMsg] To sdpSess2Str is failed!\n");
			return NULL;
		}
		tmpBody=(SipBody*)calloc(1,sizeof(SipBody));
		tmpBody->length=bodylen;
		tmpBody->content=(unsigned char*)sdpbuf;
		uaReqAddBody(req,tmpBody,"application/sdp");
		free(tmpBody);
	}else
		uaReqAddBody(req,NULL,NULL);
#ifdef _DEBUG
	TCRPrint(TRACE_LEVEL_API,"[uaACKMsg]create a ACK message\n%s!\n",sipReqPrint(req));
#endif	
	return req;
}

SipReq uaACKMsgEx(IN UaDlg dlg,IN SdpSess _sdp)
{
	SipReq req=NULL,origReq=NULL;
	SipRsp finalRsp=NULL;
	SipReqLine *pLine=NULL,*pNewLine=NULL;
	SipContact *pContact=NULL;
	SipCSeq *pCseq=NULL,*pNewCseq=NULL;
	SipRoute *pRoute=NULL;
	SipRecordRoute *pRecordRoute=NULL;
	SipAuthz *authz=NULL;
	char* contactbuf=NULL,*priv,buf[MAXBUFFERSIZE]={'\0'};
	int maxfor;
	RCODE rCode;

	TxStruct tx;
	TXTYPE	txType;
	SipReq invitereq;
	SipRsp invitersp;

	/*check dialog and transaction*/
	if(dlg==NULL){
		UaCoreERR("[uaACKMsgEx] Error! get NULL dialog !\n");
		return NULL;
	}
	/*create new request message for ACK*/
	req=sipReqNew();
	if(req==NULL){
		UaCoreERR("[uaACKMsgEx] To create new Request is failed!\n");
		return NULL;
	}
	/*get original request message and response message*/
	/*origReq=sipTxGetOriginalReq(_tx);*/
	origReq=uaDlgGetRequest(dlg);

	finalRsp=uaDlgGetResponse(dlg);
	if((origReq==NULL)||(finalRsp==NULL)){
		sipReqFree(req);
		UaCoreERR("[uaACKMsgEx] original request message or response message is NULL!\n");
		return NULL;
	}

	/*copy request line*/
	pLine=sipReqGetReqLine(origReq);
	if(pLine == NULL){
		sipReqFree(req);
		UaCoreERR("[uaACKMsgEx] To set request line is failed!\n");
		return NULL;
	}
	/*if Contact header exist, copy from Contact header*/
	pContact=sipRspGetHdr(finalRsp,Contact);
	if(pContact != NULL){
		if(pContact->sipContactList != NULL)
			if(pContact->sipContactList->address!=NULL){
				SipURL url;
				contactbuf=strDup(pContact->sipContactList->address->addr);
				/*check address is correct or not */
				url=sipURLNewFromTxt(contactbuf);
				if(url==NULL){
					free(contactbuf);
					contactbuf=NULL;
				}else
					sipURLFree(url);
			}
	}
	if(contactbuf != NULL){
		pNewLine=uaReqNewReqLine(ACK,contactbuf,SIP_VERSION);
		free(contactbuf);
	}else
		pNewLine=uaReqNewReqLine(ACK,pLine->ptrRequestURI,SIP_VERSION);
	sipReqSetReqLine(req,pNewLine);
	uaReqLineFree(pNewLine);

	/*Add Route header if exist Reocord-Route header*/
	tx=uaDlgGetInviteTx(dlg,TRUE);
	if(tx != NULL){
		invitereq=sipTxGetOriginalReq(tx);
		invitersp=sipTxGetLatestRsp(tx);
		/*Client transaction*/
		txType=sipTxGetType(tx);
		if((TX_CLIENT_INVITE == txType) && invitersp){
			/*Add Route header if exist Reocord-Route header*/
			pRecordRoute=(SipRecordRoute*)sipRspGetHdr(invitersp,Record_Route);
			if(pRecordRoute){
				pRoute=uaRecordRouteCovertToRoute(pRecordRoute);
				if(pRoute != NULL){
					sipReqAddHdr(req,Route,pRoute);
					uaRouteFree(pRoute);
				}
			}	
		}else if((TX_SERVER_INVITE==txType) && invitereq) {
			/*Add Route header if exist Reocord-Route header*/
			pRecordRoute=(SipRecordRoute*)sipRspGetHdr(invitersp,Record_Route);
			if(pRecordRoute){
				pRoute=(SipRoute*)pRecordRoute;
				sipReqAddHdr(req,Route,pRoute);
			}
			
		}
	}
	/*copy From,To,Call_ID header from response*/
	sipReqAddHdr(req,From,sipRspGetHdr(finalRsp,From));
	sipReqAddHdr(req,To,sipRspGetHdr(finalRsp,To));
	sipReqAddHdr(req,Call_ID,sipRspGetHdr(finalRsp,Call_ID));
	/*copy CSeq header*/
	pCseq=sipRspGetHdr(finalRsp,CSeq);
	pNewCseq=uaNewCSeq(pCseq->seq,ACK);
	if(pNewCseq!=NULL){
		sipReqAddHdr(req,CSeq,pNewCseq);
		uaFreeCSeq(pNewCseq);
	}else{
		sipReqFree(req);
		UaCoreERR("[uaACKMsgEx] To create new CSeq is failed!\n");
		return NULL;
	}
	
#ifdef _DEBUG
	/* User-Agent header */
	sprintf(buf,"User-Agent:%s\r\n",USER_AGENT);
	sipReqAddHdrFromTxt(req,User_Agent,buf);
	
	/* Content-Language */
	sprintf(buf,"Content-Language:%s\r\n",DEFAULT_LANGUAGE);
	sipReqAddHdrFromTxt(req,Content_Language,buf);
#endif

	/*Max-Forward. modified by tyhuang */
	maxfor=uaUserGetMaxForwards(uaMgrGetUser(uaDlgGetMgr(dlg)));
	if(maxfor<70) maxfor=70;
	sprintf(buf,"Max-Forwards:%d\r\n",maxfor);
	if((rCode=sipReqAddHdrFromTxt(req,Max_Forwards,buf)) != RC_OK)
		UaCoreERR("[uaACKMsgEx]add MaxForward failed!\n");
	/* privacy */	
	priv=uaDlgGetPrivacy(dlg);
	if(priv){
		sprintf(buf,"Privacy:%s\r\n",priv);
		rCode=sipReqAddHdrFromTxt(req,Privacy,buf);
	}
	/* authz*/
	authz=sipReqGetHdr(origReq,Authorization);
	if(authz)
		sipReqAddHdr(req,Authorization,authz);
	
	authz=sipReqGetHdr(origReq,Proxy_Authorization);
	/* proxy-authz */
	if(authz)
		sipReqAddHdr(req,Proxy_Authorization,authz);
	
	if(_sdp !=NULL){
		SipBody *tmpBody=NULL;
		char sdpbuf[1024]={'\0'};
		unsigned int bodylen=1024;

		if(RC_ERROR == (sdpSess2Str(_sdp,sdpbuf,&bodylen))){
			sipReqFree(req);
			UaCoreERR("[uaACKMsg] To sdpSess2Str is failed!\n");
			return NULL;
		}
		tmpBody=(SipBody*)calloc(1,sizeof(SipBody));
		tmpBody->length=bodylen;
		tmpBody->content=(unsigned char*)sdpbuf;
		uaReqAddBody(req,tmpBody,"application/sdp");
		free(tmpBody);
	}else
		uaReqAddBody(req,NULL,NULL);
#ifdef _DEBUG
	TCRPrint(TRACE_LEVEL_API,"[uaACKMsgEx]create a ACK message\n%s!\n",sipReqPrint(req));
#endif
	return req;
}
/*input original register dialog and txStruct, and original sipReq */
/*_tx and _origreq maybe NULL*/
SipReq uaREGMsg(IN UaDlg _dlg,IN TxStruct _tx,IN SipReq _origreq,IN UARegisterType _regtype,IN const char *_addr)
{
	RCODE	rCode=RC_OK;
	SipReq  newRegReq=NULL;
	UaMgr   mgr=NULL;
	UaUser  user=NULL;
	UaCfg	cfg=NULL;
	SipReqLine *pLine=NULL;
	SipCSeq *pCseq=NULL;
	SipFrom *pFrom=NULL,*pFromTag=NULL;
	/* register message buffer size 1023 */
	char buf[MAXBUFFERSIZE]={'\0'},ip[MAXADDRESIZE]={'\0'},*laddr=NULL,*tmp=NULL,*callid=NULL;
	char *hostname=NULL,*username=NULL,*authz=NULL,*dname;
	int size,pos,maxfor,expires;

	if(_dlg == NULL){ /*dialog not exist*/
		UaCoreERR("[uaREGMsg] dialog is NULL!\n");
		return NULL;
	}
	mgr=uaDlgGetMgr(_dlg);
	if(mgr==NULL){
		UaCoreERR("[uaREGMsg] mgr is NULL!\n");
		return NULL;
	}
	cfg=uaMgrGetCfg(mgr);
	user=uaMgrGetUser(mgr);
	if((cfg==NULL)||(user==NULL)){
		UaCoreERR("[uaREGmsg] cfg or user is NULL!\n");
		return NULL;
	}
	if(!uaCfgUsingRegistrar(cfg)){
		UaCoreERR("[uaREGmsg] not using registrar!\n");
		return NULL;
	}
	if((_regtype==UA_REGISTERSIPS)||(_regtype==UA_UNREGISTERSIPS)){
		if(BeSIPSURL(_addr)!=TRUE){
			UaCoreERR("[uaREGmsg] Not a SIPS URL!\n");
			return NULL;
		}
	}
	if(_regtype>UA_REGISTERSIPS_QUERY){
		UaCoreWARN("[uaREGmsg] input type is illeagle\n");
		_regtype=UA_REGISTERSIPS_QUERY;
	}
	newRegReq=sipReqNew();
	if(newRegReq == NULL){
		UaCoreERR("[uaREGmsg] To create new sipReq is failed!\n");
		return NULL;
	}
	/*set request line*/
	/* modified by tyhuang: case TCP ,with transport=tcp */
	switch(uaCfgGetRegistrarTXP(cfg)){
		case TCP:
			sprintf(buf,"sip:%s:%d;transport=tcp",uaCfgGetRegistrarAddr(cfg),uaCfgGetRegistrarPort(cfg));
			break;
		case UDP:
		default:
		sprintf(buf,"sip:%s:%d",uaCfgGetRegistrarAddr(cfg),uaCfgGetRegistrarPort(cfg));
	}
	pLine=uaReqNewReqLine(REGISTER,buf,SIP_VERSION);
	if(pLine == NULL){
		/* free reqest */
		sipReqFree(newRegReq);
		UaCoreERR("[uaREGmsg] To create new Reqline is failed!\n");
		return NULL;
	}
	rCode=sipReqSetReqLine(newRegReq,pLine);
	uaReqLineFree(pLine);
	/*add Call-ID */
	callid=uaDlgGetCallID(_dlg);
	if(callid != NULL){
		/*sipReqAddHdr(newRegReq,Call_ID,sipReqGetHdr(_origreq,Call_ID));*/
		sprintf(buf,"Call-ID:%s\r\n",callid);
		rCode=sipReqAddHdrFromTxt(newRegReq,Call_ID,buf);
	}else{
		username=(char*)uaUserGetName(user);
		laddr=(char*)uaUserGetLocalAddr(user);
		hostname=(char*)uaUserGetLocalHost(user);
		if((username==NULL)||(laddr==NULL)||(hostname==NULL)){
			sipReqFree(newRegReq);
			rCode=UAUSER_ERROR;
			UaCoreERR("[uaREGmsg] username, localaddr or hostname is NULL!\n");
			return NULL;
		}

		/*copy local ip address into ip buffer*/
		strcpy(ip,laddr);
		callid=sipCallIDGen(hostname,ip);
		sprintf(buf,"Call-ID:%s\r\n",callid);
		rCode=sipReqAddHdrFromTxt(newRegReq,Call_ID,buf);
		uaDlgSetCallID(_dlg,callid);
	}
	/*add To header and from header */
	if((_regtype<=UA_REGISTER_QUERY)&&(_regtype>=0)){
		sprintf(buf,"To:\"%s\"<%s>\r\n",uaMgrGetUserName(mgr),uaMgrGetPublicAddr(mgr));
		sipReqAddHdrFromTxt(newRegReq,To,buf);
		dname=(char*)uaUserGetDisplayName(user);
		if(dname)
			sprintf(buf,"From:\"%s\"<%s>\r\n",dname,uaMgrGetPublicAddr(mgr));
		else
			sprintf(buf,"From:\"%s\"<%s>\r\n",uaMgrGetUserName(mgr),uaMgrGetPublicAddr(mgr));
		sipReqAddHdrFromTxt(newRegReq,From,buf);
	}else{

		laddr=uaMgrNewPublicSIPS(mgr);
		if(!laddr){
			UaCoreERR("[uaREGmsg] get public addr is failed!\n");
			sipReqFree(newRegReq);
			return NULL;
		}
		sprintf(buf,"To:\"%s\"<%s>\r\n",uaMgrGetUserName(mgr),laddr);
		sipReqAddHdrFromTxt(newRegReq,To,buf);
		dname=(char*)uaUserGetDisplayName(user);
		if(dname)
			sprintf(buf,"From:\"%s\"<%s>\r\n",dname,laddr);
		else
			sprintf(buf,"From:<%s>\r\n",laddr);
		sipReqAddHdrFromTxt(newRegReq,From,buf);
		free(laddr);
	}
	
	/*add From header*/
	/*modify by tyhuang:add displayname to From sprintf(buf,"From:%s\r\n",uaMgrGetPublicAddr(mgr));*/
	/* add from-tag */
	pFrom=(SipFrom*)sipReqGetHdr(newRegReq,From);
	pFromTag=uaFromDupAddTag(pFrom);
	if(pFromTag != NULL){
		rCode=sipReqAddHdr(newRegReq,From,(void*)pFromTag);
		uaFromHdrFree(pFromTag);
	}
	/*add cseq*/
	/*if(_origreq == NULL){*/
		/*add a new CSeq number*/
		uaMgrAddCmdSeq(mgr);
		pCseq=uaNewCSeq(uaMgrGetCmdSeq(mgr),REGISTER);
		sipReqAddHdr(newRegReq,CSeq,pCseq);
		uaFreeCSeq(pCseq);
		
	/*}else{*/
		/*based on the previous CSeq +1 */
	/*	pCseq=uaNewCSeq(uaGetReqCSeq(_origreq)+1,REGISTER);
		sipReqAddHdr(newRegReq,CSeq,pCseq);
		uaFreeCSeq(pCseq);
	}*/
	/*add contact*/
	switch(_regtype){
	case UA_REGISTER:
		/*add contact address*/
		if(_addr){
			/* user may add this contact to UaUser config */
			sprintf(buf,"Contact:%s\r\n",_addr);
			sipReqAddHdrFromTxt(newRegReq,Contact,buf);
		}else{
			/* should make sure ,default contact is in list */
			tmp=(char*)uaMgrGetContactAddr(mgr);
			size=uaUserGetNumOfContactAddr(user);
			for(pos=0;pos<size;pos++){
				sprintf(buf,"Contact:%s\r\n",uaUserGetContactAddr(user,pos));
				sipReqAddHdrFromTxt(newRegReq,Contact,buf);
			}
		}
		/*add expire*/
		expires=uaUserGetExpires(user);
		if(expires>0){
			sprintf(buf,"Expires:%d\r\n",uaUserGetExpires(user));
			sipReqAddHdrFromTxt(newRegReq,Expires,buf);
		}
		break;
	case UA_REGISTERSIPS:
		/* user may add this contact to UaUser config */
		sprintf(buf,"Contact:%s\r\n",_addr);
		sipReqAddHdrFromTxt(newRegReq,Contact,buf);
		break;
	case UA_UNREGISTER:
	case UA_UNREGISTERSIPS:/* modify by tyhuang */
		/*add contact address*/
		if(_addr)
			sprintf(buf,"Contact:%s\r\n",_addr);
		else
			sprintf(buf,"Contact:%s\r\n",uaMgrGetContactAddr(mgr));
		sipReqAddHdrFromTxt(newRegReq,Contact,buf);
		/*add expire = 0*/
		sprintf(buf,"Expires:0\r\n");
		sipReqAddHdrFromTxt(newRegReq,Expires,buf);
		break;
	case UA_UNREGISTERALLSIPS:
	case UA_UNREGISTERALL:/* modify by tyhuang */
		/*add "*" as contact address*/
		sprintf(buf,"Contact: * \r\n");
		sipReqAddHdrFromTxt(newRegReq,Contact,buf);
		/*add expire*/
		sprintf(buf,"Expires:0\r\n");
		sipReqAddHdrFromTxt(newRegReq,Expires,buf);
		break;
	case UA_REGISTERSIPS_QUERY:
	case UA_REGISTER_QUERY:
		/*no add contact address*/
		/*no add expire*/
		break;
	default:
		UaCoreERR("[uaREGmsg] Wrong Register Type !\n");
		break;
	}

#ifdef _DEBUG
	/* User-Agent header */
	sprintf(buf,"User-Agent:%s\r\n",USER_AGENT);
	rCode=sipReqAddHdrFromTxt(newRegReq,User_Agent,buf);
#endif
	
	maxfor=uaUserGetMaxForwards(user);
	if(maxfor<70) maxfor=70;
	sprintf(buf,"Max-Forwards:%d\r\n",maxfor);
	if((rCode=sipReqAddHdrFromTxt(newRegReq,Max_Forwards,buf)) != RC_OK)
		UaCoreERR("[uaREGISTERMsg]add MaxForward failed!\n");

	/* privacy */	
	if(uaDlgGetPrivacy(_dlg)){
		sprintf(buf,"Privacy:%s\r\n",uaDlgGetPrivacy(_dlg));
		rCode=sipReqAddHdrFromTxt(newRegReq,Privacy,buf);
	}

	/* Supported Header */
	if(uaDlgGetSupported(_dlg)){
		sprintf(buf,"Supported:%s\r\n",uaDlgGetSupported(_dlg));
		rCode=sipReqAddHdrFromTxt(newRegReq,Supported,buf);
	}

	/* add authdata to message 2004.3.8 add by tyhuang */
	authz=uaUserGetAuthData(user);
	if(authz){
		sprintf(buf,"%s",authz);
		rCode=sipReqAddHdrFromTxt(newRegReq,Authorization,buf);
	}
	/*add body*/
	uaReqAddBody(newRegReq,NULL,NULL);
#ifdef _DEBUG
	TCRPrint(TRACE_LEVEL_API,"[uaREGISTERMsg]create a REGISTER message\n%s!\n",sipReqPrint(newRegReq));
#endif
	return newRegReq;
}

/* input dialog, req-URI(NULL-->transfer), Refer-To: addrss  */
SipReq uaREFERMsg(IN UaDlg _dlg,IN const char* _requri, IN const char* _referto)
{
	SipReq refReq=NULL;
	UACallStateType dlgstate=UACSTATE_UNKNOWN;
	UaMgr	mgr;
	UaUser	user;
	UaCfg	cfg;
	RCODE	rCode=RC_OK;
	SipReqLine *line=NULL,*pLine=NULL;
	SipFrom *pFrom=NULL, *pFromTag=NULL;
	char uribuf[256]={'\0'};
	int  urilen=0;
	char buf[MAXBUFFERSIZE]={'\0'};
	char ip[MAXADDRESIZE]={'\0'},*username=NULL,*laddr=NULL,*hostname=NULL,*dname;
	TxStruct tx;
	SipReq origReq=NULL;
	SipRsp finalRsp=NULL;
	SipRecordRoute *pRecordRoute=NULL;
	SipRoute *pRoute=NULL;
	SipContact *pContact=NULL;
	SipCSeq *cseq=NULL;
	SipURL url=NULL;
	BOOL bContactOK=FALSE;
	TXTYPE txType;
	int maxfor;
	char *priv;

	/*check dialog exist or not*/
	if((_dlg == NULL)||(_referto ==NULL)){
		UaCoreERR("[uaREFERMsg] dlg or referto is NULL!\n");
		return NULL;
	}
	/*get parent mgr, if NULL : Error */
	mgr=uaDlgGetMgr(_dlg);
	if(mgr==NULL)
		return NULL;
	
	/*get user object, get user information*/
	user=uaMgrGetUser(mgr);
	if(user == NULL){
		UaCoreERR("[uaREFERMsg] user is NULL!\n");
		return NULL;
	}
	/*get cfg object, get configuration information*/
	cfg=uaMgrGetCfg(mgr);
	if(cfg==NULL){
		UaCoreERR("[uaREFERMsg] cfg is NULL!\n");
		return NULL;
	}
	/*create new empty REFER request message*/
	refReq=sipReqNew();
	if(refReq == NULL){
		UaCoreERR("[uaREFERMsg] To create new sipReq is failed!\n");
		return NULL;
	}
	/*check dialog call state, if UACSTATE_IDLE --> 3rd party call control*/
	/*	if UACSTATE_CONNECTED or UACSTATE_ONHELD --> transfer or confierence*/
	dlgstate=uaDlgGetState(_dlg);

	switch(dlgstate){
	case UACSTATE_IDLE:/*3rd party call control, construct a new REFER message*/
		/*request-uri should not be NULL*/
		if(NULL == _requri){
			sipReqFree(refReq);
			UaCoreERR("[uaREFERMsg] request-uri is NULL!\n");
			return NULL;
		}

		/*create a request line*/
		url=sipURLNewFromTxt((char*)_requri);
		line=uaReqNewReqLine(REFER,sipURLToStr(url),SIP_VERSION);
		if(url){
			sipURLFree(url);
			url=NULL;
		}
		if(line != NULL){
			if(sipReqSetReqLine(refReq,line)!=RC_OK){
				uaReqLineFree(line);
				sipReqFree(refReq);
				UaCoreERR("[uaREFERMsg] To set request line is failed!\n");
				return NULL;
			}
			uaReqLineFree(line);
		}else{
			sipReqFree(refReq);
			UaCoreERR("[uaREFERMsg] To create request line is failed!\n");
			return NULL;
		}
		username=(char*)uaUserGetName(user);
		laddr=(char*)uaUserGetLocalAddr(user);
		hostname=(char*)uaUserGetLocalHost(user);
		if((NULL==username)||(NULL==laddr)||(NULL==hostname)){
			sipReqFree(refReq);
			UaCoreERR("[uaREFERMsg] username, localaddr or hostname is NULL!\n");
			return NULL;
		}

		/*copy local ip addres into ip buffer*/
		strcpy(ip,laddr);

		/*To header*/
		sprintf(buf,"To:<%s>\r\n",_requri);
		rCode=sipReqAddHdrFromTxt(refReq,To,buf);

		/*Call-ID header*/
		sprintf(buf,"Call-ID:%s\r\n",sipCallIDGen(hostname,ip));
		rCode=sipReqAddHdrFromTxt(refReq,Call_ID,buf);

		/*Contact header*/
		if(uaDlgGetSecureFlag(_dlg)==TRUE){
			if(uaUserGetSIPSContact(user))
				sprintf(buf,"Contact:%s\r\n",uaUserGetSIPSContact(user));
			else{
				UaCoreERR("[uaREFERMsg] SIPS contact is not found!\n");
				sipReqFree(refReq);
				return NULL;
			}
		}else
			sprintf(buf,"Contact:%s\r\n",uaMgrGetContactAddr(mgr));
		
		rCode=sipReqAddHdrFromTxt(refReq,Contact,buf);

		/*From header*/
		
		/*modify by tyhuang:add displayname to From sprintf(buf,"From:%s\r\n",uaMgrGetPublicAddr(mgr));*/
		priv=uaDlgGetPrivacy(_dlg);
		if(priv){
			sprintf(buf,"Privacy:%s\r\n",priv);
			rCode=sipReqAddHdrFromTxt(refReq,Privacy,buf);
		}
		priv=uaDlgGetPPreferredID(_dlg);
		/* P-Preferred-Identity */
		if(priv){
			sprintf(buf,"P-Preferred-Identity:%s\r\n",priv);
			rCode=sipReqAddHdrFromTxt(refReq,P_Preferred_Identity,buf);
		}
		
		if(uaDlgGetUserPrivacy(_dlg)==TRUE)
			sprintf(buf,"From:\"Anonymous\"<sip:anonymous@anonymous.invalid>\r\n");
		else{
			dname=(char*)uaUserGetDisplayName(user);
			if(dname)
				sprintf(buf,"From:\"%s\"<%s>\r\n",dname,uaMgrGetPublicAddr(mgr));
			else
				sprintf(buf,"From:\"%s\"<%s>\r\n",uaMgrGetUserName(mgr),uaMgrGetPublicAddr(mgr));
		}
		rCode=sipReqAddHdrFromTxt(refReq,From,buf);
		pFrom=(SipFrom*)sipReqGetHdr(refReq,From);
		pFromTag=uaFromDupAddTag(pFrom);
		if(pFromTag != NULL){
			rCode=sipReqAddHdr(refReq,From,(void*)pFromTag);
			uaFromHdrFree(pFromTag);
		}

		/*Max-Forward.modified by tyhuang 2003.05.19*/
		maxfor=uaUserGetMaxForwards(user);
		if(maxfor<70) maxfor=70;
		/*sprintf(buf,"Max-Forwards:70\r\n");*/
		sprintf(buf,"Max-Forwards:%d\r\n",maxfor);
		rCode=sipReqAddHdrFromTxt(refReq,Max_Forwards,buf);

		/*CSeq header*/
		sprintf(buf,"CSeq:%d REFER\r\n",uaMgrGetCmdSeq(mgr));
		rCode=sipReqAddHdrFromTxt(refReq,CSeq,buf);

		/*Refer-To*/
		sprintf(buf,"Refer-To:<%s>\r\n",_referto);
		rCode=sipReqAddHdrFromTxt(refReq,Refer_To,buf);

#ifdef _DEBUG
		/* User-Agent header */
		sprintf(buf,"User-Agent:%s\r\n",USER_AGENT);
		rCode=sipReqAddHdrFromTxt(refReq,User_Agent,buf);
#endif
	
		/*no body*/
		uaReqAddBody(refReq,NULL,NULL);

		break;
	case UACSTATE_CONNECTED:
	case UACSTATE_ONHELD:
		tx=uaDlgGetInviteTx(_dlg,TRUE);
		if(NULL != tx){
			origReq=sipTxGetOriginalReq(tx);
			finalRsp=sipTxGetLatestRsp(tx);

			/*Client transaction*/
			txType=sipTxGetType(tx);
			if(txType == TX_CLIENT_INVITE ){
				if(NULL == origReq){
					sipReqFree(refReq);
					UaCoreERR("[uaREFERMsg] To original Request is NULL!\n");
					return NULL;
				}
				pContact=sipRspGetHdr(finalRsp,Contact);
				if(pContact != NULL)
					bContactOK=TRUE;

				/*set request line*/
				pLine=sipReqGetReqLine(origReq);
				if(bContactOK){
					/*copy Contact header as Request-URI */
					/*uaContactAsReqURI(pContact,uribuf,256,&urilen);*/
					sprintf(uribuf,"%s",uaDlgGetRemoteTarget(_dlg));
					line=uaReqNewReqLine(REFER,uribuf,SIP_VERSION);
					/*line=uaReqNewReqLine(REFER,uribuf,pLine->ptrSipVersion);*/
				}else
					line=uaReqNewReqLine(REFER,pLine->ptrRequestURI,pLine->ptrSipVersion);
				rCode=sipReqSetReqLine(refReq,line);
				uaReqLineFree(line);
				if(rCode != RC_OK){
					sipReqFree(refReq);
					UaCoreERR("[uaREFERMsg] To set request line is failed!\n");
					return NULL;
				}

				/* privacy */
				priv=uaDlgGetPrivacy(_dlg);
				if(priv){
					sprintf(buf,"Privacy:%s\r\n",priv);
					rCode=sipReqAddHdrFromTxt(refReq,Privacy,buf);
				}
				/* P-Preferred-Identity */
				priv=uaDlgGetPPreferredID(_dlg);
				if(priv){
					sprintf(buf,"P-Preferred-Identity:%s\r\n",priv);
					rCode=sipReqAddHdrFromTxt(refReq,P_Preferred_Identity,buf);
				}
				/*copy From header */
				rCode=sipReqAddHdr(refReq,From,sipReqGetHdr(origReq,From));
				
				/*copy Call-ID header*/
				rCode=sipReqAddHdr(refReq,Call_ID,sipReqGetHdr(origReq,Call_ID));

				/*copy Contact header*/
				rCode=sipReqAddHdr(refReq,Contact,sipReqGetHdr(origReq,Contact));
				
				/*Add Route header if exist Reocord-Route header*/
				pRecordRoute=(SipRecordRoute*)sipRspGetHdr(finalRsp,Record_Route);
				if(pRecordRoute != NULL){
					pRoute=uaRecordRouteCovertToRoute(pRecordRoute);
					if(pRoute != NULL){
						sipReqAddHdr(refReq,Route,pRoute);
						uaRouteFree(pRoute);
					}
				}	

				/*copy To header*/
				rCode=sipReqAddHdr(refReq,To,sipRspGetHdr(finalRsp,To));
				
				/*copy CSeq*/
				uaMgrAddCmdSeq(mgr); /*command sequence increase 1*/
				cseq=uaNewCSeq(uaMgrGetCmdSeq(mgr),REFER);
				sipReqAddHdr(refReq,CSeq,cseq);
				uaFreeCSeq(cseq);
				/*copy Max-Forwards*/
				if(sipReqGetHdr(origReq,Max_Forwards))
					sipReqAddHdr(refReq,Max_Forwards,sipReqGetHdr(origReq,Max_Forwards));
				else{
					/*Max-Forward. modified by tyhuang */
					maxfor=uaUserGetMaxForwards(user);
					if(maxfor<0) maxfor=70;
					sprintf(buf,"Max-Forwards:%d\r\n",maxfor);
					if((rCode=sipReqAddHdrFromTxt(refReq,Max_Forwards,buf)) != RC_OK)
						UaCoreERR("[uaREGISTERMsg]add MaxForward failed!\n");
				}
				/*Add Refer-To*/
				sprintf(buf,"Refer-To:<%s>\r\n",_referto);
				rCode=sipReqAddHdrFromTxt(refReq,Refer_To,buf);

				/*Add Refer-By*/
				sprintf(buf,"Referred-By:%s\r\n",uaMgrGetPublicAddr(mgr));
				rCode=sipReqAddHdrFromTxt(refReq,Referred_By,buf);
#ifdef _DEBUG				
				/* User-Agent header */
				sprintf(buf,"User-Agent:%s\r\n",USER_AGENT);
				rCode=sipReqAddHdrFromTxt(refReq,User_Agent,buf);
#endif				
				/*Add Empty Body*/
				uaReqAddBody(refReq,NULL,NULL);
				/*Client transaction finished*/

			}else if(TX_SERVER_INVITE==txType){  
				/*Server transaction*/
				pContact=sipReqGetHdr(origReq,Contact);
				if(NULL != pContact){
					url=sipURLNewFromTxt(pContact->sipContactList->address->addr);
					if(url != NULL)
						bContactOK=TRUE;
				}
				if(bContactOK){/*copy from Contact header*/
					/*uaContactAsReqURI(pContact,uribuf,256,&urilen);*/
					sprintf(uribuf,"%s",uaDlgGetRemoteTarget(_dlg));
					line=uaReqNewReqLine(REFER,uribuf,SIP_VERSION);
					sipURLFree(url);
				}else{/*copy from From header*/
					pFrom=sipRspGetHdr(finalRsp,From);
					if(pFrom != NULL)
						line=uaReqNewReqLine(REFER,pFrom->address->addr,SIP_VERSION);
					else{
						sipReqFree(refReq);
						UaCoreERR("[uaREFERMsg] From header is NULL!\n");
						return NULL;
					}
				}
				rCode=sipReqSetReqLine(refReq,line);
				uaReqLineFree(line);

				/* privacy */
				priv=uaDlgGetPrivacy(_dlg);
				if(priv){
					sprintf(buf,"Privacy:%s\r\n",priv);
					rCode=sipReqAddHdrFromTxt(refReq,Privacy,buf);
				}
				
				/* P-Preferred-Identity */
				priv=uaDlgGetPPreferredID(_dlg);
				if(priv){
					sprintf(buf,"P-Preferred-Identity:%s\r\n",priv);
					rCode=sipReqAddHdrFromTxt(refReq,P_Preferred_Identity,buf);
				}
				/*copy From header*/
				/*notice that copy from To header, since should copy the origin tag*/
				rCode=sipReqAddHdr(refReq,From,sipRspGetHdr(finalRsp,To));
				
				/*copy To header*/
				/*notice that copy from From header, since should copy the origin tag*/
				rCode=sipReqAddHdr(refReq,To,sipRspGetHdr(finalRsp,From));

				/*copy Call-ID header*/
				rCode=sipReqAddHdr(refReq,Call_ID,sipReqGetHdr(origReq,Call_ID));
				
				/*copy CSeq*/
				uaMgrAddCmdSeq(uaDlgGetMgr(_dlg));
				cseq=uaNewCSeq(uaMgrGetCmdSeq(uaDlgGetMgr(_dlg)),REFER);
				rCode=sipReqAddHdr(refReq,CSeq,cseq);
				uaFreeCSeq(cseq);
				
				/*Add Contact header */ 
				if(uaDlgGetSecureFlag(_dlg)==TRUE){
					if(uaUserGetSIPSContact(user))
						sprintf(buf,"Contact:%s\r\n",uaUserGetSIPSContact(user));
					else{
						UaCoreERR("[uaREFERMsg] SIPS contact is not found!\n");
						sipReqFree(refReq);
						return NULL;
					}
				}else
					sprintf(buf,"Contact:%s\r\n",uaMgrGetContactAddr(mgr));
				rCode=sipReqAddHdrFromTxt(refReq,Contact,buf);
				
				/*Add Route header if exist Reocord-Route header*/
				/*pRecordRoute=(SipRecordRoute*)sipReqGetHdr(origReq,Record_Route);
				if(pRecordRoute != NULL){
					pRoute=(SipRoute*)pRecordRoute;
					sipReqAddHdr(refReq,Route,pRoute);
				}*/
				pRecordRoute=(SipRecordRoute*)sipRspGetHdr(finalRsp,Record_Route);
				if(pRecordRoute != NULL){
					pRoute=(SipRoute*)pRecordRoute;
					sipReqAddHdr(refReq,Route,pRoute);
				}
				/*Add Max-Forward.modified by tyhuang 2003.05.19*/
				maxfor=uaUserGetMaxForwards(user);
				if(maxfor<70) maxfor=70;
				/*sprintf(buf,"Max-Forwards:70\r\n");*/
				sprintf(buf,"Max-Forwards:%d\r\n",maxfor);
				rCode=sipReqAddHdrFromTxt(refReq,Max_Forwards,buf);

				/*Add Refer-To*/
				sprintf(buf,"Refer-To:<%s>\r\n",_referto);
				rCode=sipReqAddHdrFromTxt(refReq,Refer_To,buf);

				/*Add Refer-By*/
				sprintf(buf,"Referred-By:%s\r\n",uaMgrGetPublicAddr(mgr));
				rCode=sipReqAddHdrFromTxt(refReq,Referred_By,buf);

#ifdef _DEBUG				
				/* User-Agent header */
				sprintf(buf,"User-Agent:%s\r\n",USER_AGENT);
				rCode=sipReqAddHdrFromTxt(refReq,User_Agent,buf);
#endif
				
				/*Add NULL body*/
				uaReqAddBody(refReq,NULL,NULL);
			}

		}else{  /*transaction NULL*/
			sipReqFree(refReq);
			UaCoreERR("[uaREFERMsg] transaction is NULL!\n");
			return NULL;
		}

		break;
	default:
		
		break;
	}
#ifdef _DEBUG
	TCRPrint(TRACE_LEVEL_API,"[uaREFERMsg]create a REFER message\n%s!\n",sipReqPrint(refReq));
#endif
	return refReq;
}

/*NOTIFY message*/
SipReq uaNOTIFYMsg(IN UaDlg _dlg,IN const char* _dest,IN const char* state,IN UaContent _content)
{
	SipReq notifyMsg=NULL;
	RCODE  rCode;
	UaMgr  mgr;
	UaUser user;
	UaCfg  cfg;
	SipReqLine *line=NULL;
	char buf[MAXBUFFERSIZE]={'\0'};
	char ip[MAXADDRESIZE]={'\0'},*username,*laddr,*hostname;
	SipRsp finalRefRsp=NULL;
	SipCSeq *cseq=NULL;
	SipReq origReq=NULL;
	SdpSess _sdp;
	SipBody *tmpBody=NULL;
	SipURL url=NULL;
	int maxfor,bodylen;
	char *priv;

	SipRoute *pRoute=NULL;
	SipRecordRoute *pRecordRoute=NULL;
	TxStruct tx;
	TXTYPE	txType;
	SipReq invitereq;
	SipRsp invitersp;

	/*check input parameter is NULL or not*/
	if((_dlg == NULL)||(_dest == NULL)){
		UaCoreERR("[uaNOTIFYMsg] dialog or destination is NULL!\n");
		return NULL;
	}
	/*get parent mgr, if NULL : Error */
	mgr=uaDlgGetMgr(_dlg);
	if(mgr==NULL){
		UaCoreERR("[uaNOTIFYMsg] Mgr is NULL!\n");
		return NULL;
	}
	/*get user object, get user information*/
	user=uaMgrGetUser(mgr);
	if(user == NULL){
		UaCoreERR("[uaNOTIFYMsg] user is NULL!\n");
		return NULL;
	}
	/*get cfg object, get configuration information*/
	cfg=uaMgrGetCfg(mgr);
	if(cfg==NULL){
		UaCoreERR("[uaNOTIFYMsg] cfg is NULL!\n");
		return NULL;
	}
	/*Get REFER final response*/
	finalRefRsp=uaGetReferRsp(_dlg);

	/* get original Request */
	tx=uaDlgGetInviteTx(_dlg,FALSE);
	if(tx==NULL)
		tx=uaDlgGetInviteTx(_dlg,TRUE);
	origReq=sipTxGetOriginalReq(tx);

	if(NULL == finalRefRsp ){
		UaCoreERR("[uaNOTIFYMsg] REFER final response is NULL!\n");
		return NULL;
	}
	/*create new empty request message*/
	notifyMsg=sipReqNew();
	if(notifyMsg == NULL){
		UaCoreERR("[uaNOTIFYMsg] To create new request is failed!\n");
		return NULL;
	}
	/*request line*/
	url=sipURLNewFromTxt((char*)_dest);
	line=uaReqNewReqLine(SIP_NOTIFY,sipURLToStr(url),SIP_VERSION);
	if(url){
			sipURLFree(url);
			url=NULL;
	}
	if(NULL != line){
		rCode=sipReqSetReqLine(notifyMsg,line);
		if(RC_OK != rCode){
			uaReqLineFree(line);
			sipReqFree(notifyMsg);
			UaCoreERR("[uaNOTIFYMsg] To set request line is failed!\n");
			return NULL;
		}
		if(NULL != line)
			uaReqLineFree(line);
	}else{
		sipReqFree(notifyMsg);
		UaCoreERR("[uaNOTIFYMsg] To create request line is failed!\n");
		return NULL;
	}

	username=(char*)uaUserGetName(user);
	laddr=(char*)uaUserGetLocalAddr(user);
	hostname=(char*)uaUserGetLocalHost(user);
	if((NULL==username)||(NULL==laddr)||(NULL==hostname)){
		sipReqFree(notifyMsg);
		UaCoreERR("[uaNOTIFYMsg] username, localaddr or hostname is NULL!\n");
		return NULL;
	}

	/*copy local ip address into ip buffer*/
	strcpy(ip,laddr);

	/*Call-ID header*/
	sprintf(buf,"Call-ID:%s\r\n",uaDlgGetCallID(_dlg));
	rCode=sipReqAddHdrFromTxt(notifyMsg,Call_ID,buf);

	/*Event*/
	sprintf(buf,"Event:refer\r\n");
	rCode=sipReqAddHdrFromTxt(notifyMsg,Event,buf);

	/*To header, copy from REFER transaction latest response From*/
	rCode=sipReqAddHdr(notifyMsg,To,sipRspGetHdr(finalRefRsp,From));
	/*From header, copy from REFER transaction latest response To */
	rCode=sipReqAddHdr(notifyMsg,From,sipRspGetHdr(finalRefRsp,To));
	/*Contact header*/
	if(uaDlgGetSecureFlag(_dlg)==TRUE){
		if(uaUserGetSIPSContact(user))
			sprintf(buf,"Contact:%s\r\n",uaUserGetSIPSContact(user));
		else{
			UaCoreERR("[uaINVITEMsg] SIPS contact is not found!\n");
			sipReqFree(notifyMsg);
			return NULL;
		}
	}else
		sprintf(buf,"Contact:%s\r\n",uaMgrGetContactAddr(mgr));

	rCode=sipReqAddHdrFromTxt(notifyMsg,Contact,buf);
	/*CSeq header*/
	uaMgrAddCmdSeq(mgr);
	cseq=uaNewCSeq(uaMgrGetCmdSeq(mgr),SIP_NOTIFY);
	sipReqAddHdr(notifyMsg,CSeq,cseq);
	uaFreeCSeq(cseq);

	/*Max-Forward. modified by tyhuang 2003.05.19*/
	maxfor=uaUserGetMaxForwards(user);
	if(maxfor<70) maxfor=70;
	/*sprintf(buf,"Max-Forwards:70\r\n");*/
	sprintf(buf,"Max-Forwards:%d\r\n",maxfor);
	if((rCode=sipReqAddHdrFromTxt(notifyMsg,Max_Forwards,buf))!=RC_OK)
		UaCoreERR("[uaNOTIFYMsg] add MaxForward failed!\n");

	/* privacy */
	priv=uaDlgGetPrivacy(_dlg);
	if(priv){
		sprintf(buf,"Privacy:%s\r\n",priv);
		rCode=sipReqAddHdrFromTxt(notifyMsg,Privacy,buf);
	}
	
	/* P-Preferred-Identity */
	priv=uaDlgGetPPreferredID(_dlg);
	if(priv){
		sprintf(buf,"P-Preferred-Identity:%s\r\n",priv);
		rCode=sipReqAddHdrFromTxt(notifyMsg,P_Preferred_Identity,buf);
	}
	/*Add Route header if exist Reocord-Route header*/
	tx=uaDlgGetInviteTx(_dlg,TRUE);
	if(tx != NULL){
		invitereq=sipTxGetOriginalReq(tx);
		invitersp=sipTxGetLatestRsp(tx);
		/*Client transaction*/
		txType=sipTxGetType(tx);
		if((TX_CLIENT_INVITE == txType) && invitersp){
			/*Add Route header if exist Reocord-Route header*/
			pRecordRoute=(SipRecordRoute*)sipRspGetHdr(invitersp,Record_Route);
			if(pRecordRoute){
				pRoute=uaRecordRouteCovertToRoute(pRecordRoute);
				if(pRoute != NULL){
					sipReqAddHdr(notifyMsg,Route,pRoute);
					uaRouteFree(pRoute);
				}
			}	
		}else if((TX_SERVER_INVITE==txType) && invitereq) {
			/*Add Route header if exist Reocord-Route header*/
			pRecordRoute=(SipRecordRoute*)sipRspGetHdr(invitersp,Record_Route);
			if(pRecordRoute){
				pRoute=(SipRoute*)pRecordRoute;
				sipReqAddHdr(notifyMsg,Route,pRoute);
			}
			
		}
	}
	/* subscribe-state */
	if(strICmp(state,"terminated")==0)
		sprintf(buf,"Subscription-State:terminated;reason=noresource");
	else
		sprintf(buf,"Subscription-State:active;expires=180");
	rCode=sipReqAddHdrFromTxt(notifyMsg,Subscription_State,buf);

#ifdef _DEBUG
	/* User-Agent header */
	sprintf(buf,"User-Agent:%s\r\n",USER_AGENT);
	rCode=sipReqAddHdrFromTxt(notifyMsg,User_Agent,buf);
	
	/* Content-Language */
	sprintf(buf,"Content-Language:%s\r\n",DEFAULT_LANGUAGE);
	rCode=sipReqAddHdrFromTxt(notifyMsg,Content_Language,buf);
#endif

	bodylen=uaContentGetLength(_content);
	if(bodylen>0){
		sprintf(buf,"Content-Length:%d\r\n",strlen(uaContentGetBody(_content)));
		rCode=sipReqAddHdrFromTxt(notifyMsg,Content_Length,buf);
		sprintf(buf,"Content-Type:%s\r\n",uaContentGetType(_content));
		rCode=sipReqAddHdrFromTxt(notifyMsg,Content_Type,buf);
	}else{
		sprintf(buf,"Content-Length:0\r\n");
		rCode=sipReqAddHdrFromTxt(notifyMsg,Content_Length,buf);
	}
	/* */
	/*Add body .modified by tyhuang 2003.05.20*/
	if(_content != NULL){
		if(strICmp(uaContentGetType(_content),"application/sdp")==0){
			_sdp=sdpSessNewFromText( (char*)uaContentGetBody(_content));
			if(_sdp != NULL){
				tmpBody=uaCreatSipBody(_sdp);
				sdpSessFree(_sdp);
				if(tmpBody != NULL)
					uaReqAddBody(notifyMsg,tmpBody,"application/sdp");
				else
					uaReqAddBody(notifyMsg,NULL,NULL);

			}else{
				uaReqAddBody(notifyMsg,NULL,NULL);
			}
		}else{
			tmpBody=(SipBody*)calloc(1,sizeof(SipBody));
			tmpBody->length=uaContentGetLength(_content);
			tmpBody->content=strDup(uaContentGetBody(_content));
			rCode=sipReqSetBody(notifyMsg,tmpBody);
			if(rCode != RC_OK){
				sipReqFree(notifyMsg);
				notifyMsg=NULL;
			}
		}
	}
	if(tmpBody)
		sipBodyFree(tmpBody);
#ifdef _DEBUG
	TCRPrint(TRACE_LEVEL_API,"[uaNOTIFYMsg]create a NOTIFY message\n%s!\n",sipReqPrint(notifyMsg));
#endif
	return notifyMsg;
}

SipReq	uaINFOMsg(IN UaDlg _dlg,IN const char* _dest, IN UaContent _content)
{

	SipReqLine *pLine=NULL,*pNewLine=NULL;
	SipCSeq *cseq;
	SipFrom *pFrom=NULL,*pFromTag=NULL;
	RCODE rCode;
	SipReq messageMsg=NULL;
	UaMgr mgr=NULL;
	UaUser user;
	UaCfg  cfg;
	SipURL url;
	SipBody	*tmpBody=NULL;
	char buf[MAXBUFFERSIZE]={'\0'};
	char ip[MAXADDRESIZE]={'\0'},*username,*laddr,*hostname,*authz,*callid;
	int maxfor,bodylen;

	SipRoute *pRoute=NULL;
	SipRecordRoute *pRecordRoute=NULL;
	TxStruct tx;
	TXTYPE	txType;
	SipReq invitereq;
	SipRsp invitersp;

	
	/* check dialog */
	if(_dlg == NULL){
		UaCoreERR("[uaINFOMsg] dialog is NULL!\n");
		return NULL;
	}
	/*check input parameter is NULL or not*/
	if(_dest == NULL){
		UaCoreERR("[uaINFOMsg] destination is NULL!\n");
		return NULL;
	}
	mgr=uaDlgGetMgr(_dlg);
	/*get parent mgr, if NULL : Error */
	if(mgr==NULL){
		UaCoreERR("[uaINFOMsg] mgr is NULL!\n");
		return NULL;
	}
	/*get user object, get user information*/
	user=uaMgrGetUser(mgr);
	if(user == NULL){
		UaCoreERR("[uaINFOMsg] user is NULL!\n");
		return NULL;
	}
	/*get cfg object, get configuration information*/
	cfg=uaMgrGetCfg(mgr);
	if(cfg==NULL)
		return NULL;

	/*create new empty request message*/
	messageMsg=sipReqNew();
	if(messageMsg == NULL){
		UaCoreERR("[uaINFOMsg] To create new Request is failed!\n");
		return NULL;
	}

	/*request line*/
	url=sipURLNewFromTxt((char*)_dest);
	pNewLine=uaReqNewReqLine(SIP_INFO,sipURLToStr(url),SIP_VERSION);
	if(url)
		sipURLFree(url);
	if(NULL != pNewLine){
		rCode=sipReqSetReqLine(messageMsg,pNewLine);
		if(RC_OK != rCode){
			uaReqLineFree(pNewLine);
			sipReqFree(messageMsg);
		}
		if(NULL != pNewLine)
			uaReqLineFree(pNewLine);
	}else{
		sipReqFree(messageMsg);
		UaCoreERR("[uaINFOMsg] To create new request line is failed!\n");
		return NULL;
	}
	

	username=(char*)uaUserGetName(user);
	laddr=(char*)uaUserGetLocalAddr(user);
	hostname=(char*)uaUserGetLocalHost(user);
	if((NULL==username)||(NULL==laddr)||(NULL==hostname)){
		sipReqFree(messageMsg);
		UaCoreERR("[uaINFOMsg] username,localaddr or hostname is NULL!\n");
		return NULL;
	}

	/*copy local ip address into ip buffer*/
	strcpy(ip,laddr);


	callid=uaDlgGetCallID(_dlg);
	if(callid==NULL){
		callid=sipCallIDGen(hostname,ip);
		uaDlgSetCallID(_dlg,(const char*)callid);
	}
	sprintf(buf,"Call-ID:%s\r\n",callid);
	rCode=sipReqAddHdrFromTxt(messageMsg,Call_ID,buf);
	
	tx=uaDlgGetInviteTx(_dlg,TRUE);
	if(tx != NULL){
		invitereq=sipTxGetOriginalReq(tx);
		invitersp=sipTxGetLatestRsp(tx);
		/*Client transaction*/
		txType=sipTxGetType(tx);
		if((TX_CLIENT_INVITE == txType) && invitersp && invitereq){
			/*To header*/
			/*Code=sipReqAddHdr(messageMsg,To,sipReqGetHdr(invitereq,To));*/
			rCode=sipReqAddHdr(messageMsg,To,sipRspGetHdr(invitersp,To));
			/*From header */
			rCode=sipReqAddHdr(messageMsg,From,sipReqGetHdr(invitereq,From));
			/*Add Route header if exist Reocord-Route header*/
			pRecordRoute=(SipRecordRoute*)sipRspGetHdr(invitersp,Record_Route);
			if(pRecordRoute){
				pRoute=uaRecordRouteCovertToRoute(pRecordRoute);
				if(pRoute != NULL){
					sipReqAddHdr(messageMsg,Route,pRoute);
					uaRouteFree(pRoute);
				}
			}	
		}else if((TX_SERVER_INVITE==txType) && invitersp) {
			/*To header*/
			rCode=sipReqAddHdr(messageMsg,From,sipRspGetHdr(invitersp,To));
			/*From header */
			rCode=sipReqAddHdr(messageMsg,To,sipRspGetHdr(invitersp,From));
			/*Add Route header if exist Reocord-Route header*/
			pRecordRoute=(SipRecordRoute*)sipRspGetHdr(invitersp,Record_Route);
			if(pRecordRoute){
				pRoute=(SipRoute*)pRecordRoute;
				sipReqAddHdr(messageMsg,Route,pRoute);
			}
			
		}
	}else{
		/*To header*/
		sprintf(buf,"To:<%s>\r\n",_dest);
		rCode=sipReqAddHdrFromTxt(messageMsg,To,buf);

		/*From header and Contact header. modified by tyhuang*/
		if(cfg!=NULL)
			sprintf(buf,"From:\"%s\"<sip:%s@%s>\r\n",uaMgrGetUserName(mgr),uaMgrGetUserName(mgr),uaCfgGetProxyAddr(cfg));
		else
			sprintf(buf,"From:\"%s\"<%s>\r\n",uaMgrGetUserName(mgr),uaMgrGetPublicAddr(mgr));
	}

	/*CSeq header*/
	uaMgrAddCmdSeq(mgr);
	cseq=uaNewCSeq(uaMgrGetCmdSeq(mgr),SIP_INFO);
	sipReqAddHdr(messageMsg,CSeq,cseq);
	uaFreeCSeq(cseq);

	/*Max-Forward*/
	maxfor=uaUserGetMaxForwards(user);
	if(maxfor<70) maxfor=70;
	/*sprintf(buf,"Max-Forwards:70\r\n");*/
	sprintf(buf,"Max-Forwards:%d\r\n",maxfor);
	rCode=sipReqAddHdrFromTxt(messageMsg,Max_Forwards,buf);

	/* privacy */	
	if(uaDlgGetPrivacy(_dlg)){
		sprintf(buf,"Privacy:%s\r\n",uaDlgGetPrivacy(_dlg));
		rCode=sipReqAddHdrFromTxt(messageMsg,Privacy,buf);
	}

	bodylen=uaContentGetLength(_content);
	if(bodylen>0){
		sprintf(buf,"Content-Length:%d\r\n",strlen(uaContentGetBody(_content)));
		rCode=sipReqAddHdrFromTxt(messageMsg,Content_Length,buf);
		sprintf(buf,"Content-Type:%s\r\n",uaContentGetType(_content));
		rCode=sipReqAddHdrFromTxt(messageMsg,Content_Type,buf);
	}else{
		sprintf(buf,"Content-Length:0\r\n");
		rCode=sipReqAddHdrFromTxt(messageMsg,Content_Length,buf);
	}

#ifdef _DEBUG
	/* User-Agent header */
	sprintf(buf,"User-Agent:%s\r\n",USER_AGENT);
	rCode=sipReqAddHdrFromTxt(messageMsg,User_Agent,buf);
#endif 

	/* authz header add by tyhuang 2003.10.1 */
	authz=uaUserGetAuthData(user);
	if(authz){
		sprintf(buf,"%s",authz);
		rCode=sipReqAddHdrFromTxt(messageMsg,Authorization,buf);
	}

	/* add content to sip message body */
	if((messageMsg != NULL)&&(uaContentGetBody(_content)!=NULL)){
		tmpBody=(SipBody*)calloc(1,sizeof(SipBody));
		tmpBody->length=bodylen;
		tmpBody->content=strDup(uaContentGetBody(_content));
		rCode=sipReqSetBody(messageMsg,tmpBody);
		if(rCode != RC_OK){
			sipReqFree(messageMsg);
			messageMsg=NULL;
		}
	}
	if(tmpBody != NULL)
		sipBodyFree(tmpBody);
#ifdef _DEBUG
	TCRPrint(TRACE_LEVEL_API,"[uaINFOMsg]create a INFO message\n%s!\n",sipReqPrint(messageMsg));
#endif
	return messageMsg;	
}

SipReq	uaUPDATEMsg(IN UaDlg _dlg,IN const char* _dest, IN UaContent _content)
{

	SipReqLine *pLine=NULL,*pNewLine=NULL;
	SipCSeq *cseq;
	SipFrom *pFrom=NULL,*pFromTag=NULL;
	RCODE rCode;
	SipReq messageMsg=NULL;
	SipRsp	finalRsp=NULL;
	UaMgr mgr=NULL;
	UaUser user;
	UaCfg  cfg;
	SipURL url;
	SipBody	*tmpBody=NULL;
	char buf[MAXBUFFERSIZE]={'\0'};
	char ip[MAXADDRESIZE]={'\0'},*username,*laddr,*hostname,*authz,*callid;
	int maxfor,bodylen;

	SipRoute *pRoute=NULL;
	SipRecordRoute *pRecordRoute=NULL;
	TxStruct tx;
	TXTYPE	txType;
	SipReq invitereq;
	SipRsp invitersp;

	
	/* check dialog */
	if(_dlg == NULL){
		UaCoreERR("[uaUPDATEMsg] dialog is NULL!\n");
		return NULL;
	}
	/*check input parameter is NULL or not*/
	if(_dest == NULL){
		UaCoreERR("[uaUPDATEMsg] destination is NULL!\n");
		return NULL;
	}
	mgr=uaDlgGetMgr(_dlg);
	/*get parent mgr, if NULL : Error */
	if(mgr==NULL){
		UaCoreERR("[uaUPDATEMsg] mgr is NULL!\n");
		return NULL;
	}
	/*get user object, get user information*/
	user=uaMgrGetUser(mgr);
	if(user == NULL){
		UaCoreERR("[uaUPDATEMsg] user is NULL!\n");
		return NULL;
	}
	/*get cfg object, get configuration information*/
	cfg=uaMgrGetCfg(mgr);
	if(cfg==NULL)
		return NULL;

	/*create new empty request message*/
	messageMsg=sipReqNew();
	if(messageMsg == NULL){
		UaCoreERR("[uaUPDATEMsg] To create new Request is failed!\n");
		return NULL;
	}

	/*request line*/
	url=sipURLNewFromTxt((char*)_dest);
	pNewLine=uaReqNewReqLine(SIP_UPDATE,sipURLToStr(url),SIP_VERSION);
	if(url)
		sipURLFree(url);
	if(NULL != pNewLine){
		rCode=sipReqSetReqLine(messageMsg,pNewLine);
		if(RC_OK != rCode){
			uaReqLineFree(pNewLine);
			sipReqFree(messageMsg);
		}
		if(NULL != pNewLine)
			uaReqLineFree(pNewLine);
	}else{
		sipReqFree(messageMsg);
		UaCoreERR("[uaUPDATEMsg] To create new request line is failed!\n");
		return NULL;
	}
	

	username=(char*)uaUserGetName(user);
	laddr=(char*)uaUserGetLocalAddr(user);
	hostname=(char*)uaUserGetLocalHost(user);
	if((NULL==username)||(NULL==laddr)||(NULL==hostname)){
		sipReqFree(messageMsg);
		UaCoreERR("[uaUPDATEMsg] username,localaddr or hostname is NULL!\n");
		return NULL;
	}

	/*copy local ip address into ip buffer*/
	strcpy(ip,laddr);


	callid=uaDlgGetCallID(_dlg);
	if(callid==NULL){
		callid=sipCallIDGen(hostname,ip);
		uaDlgSetCallID(_dlg,(const char*)callid);
	}
	sprintf(buf,"Call-ID:%s\r\n",callid);
	rCode=sipReqAddHdrFromTxt(messageMsg,Call_ID,buf);

	tx=uaDlgGetInviteTx(_dlg,TRUE);
	if(tx != NULL){
		invitereq=sipTxGetOriginalReq(tx);
		invitersp=sipTxGetLatestRsp(tx);
		/*Client transaction*/
		txType=sipTxGetType(tx);
		if((TX_CLIENT_INVITE == txType) && invitersp){
			/*To header*/
			rCode=sipReqAddHdr(messageMsg,To,sipReqGetHdr(invitereq,To));
			/*From header */
			rCode=sipReqAddHdr(messageMsg,From,sipReqGetHdr(invitereq,From));
			/*Add Route header if exist Reocord-Route header*/
			pRecordRoute=(SipRecordRoute*)sipRspGetHdr(invitersp,Record_Route);
			if(pRecordRoute){
				pRoute=uaRecordRouteCovertToRoute(pRecordRoute);
				if(pRoute != NULL){
					sipReqAddHdr(messageMsg,Route,pRoute);
					uaRouteFree(pRoute);
				}
			}	
		}else if((TX_SERVER_INVITE==txType) && invitereq) {
			/*To header*/
			rCode=sipReqAddHdr(messageMsg,From,sipRspGetHdr(invitersp,To));
			/*From header */
			rCode=sipReqAddHdr(messageMsg,To,sipRspGetHdr(invitersp,From));
			/*Add Route header if exist Reocord-Route header*/
			pRecordRoute=(SipRecordRoute*)sipRspGetHdr(invitersp,Record_Route);
			if(pRecordRoute){
				pRoute=(SipRoute*)pRecordRoute;
				sipReqAddHdr(messageMsg,Route,pRoute);
			}
			
		}
	}
	

	/*Contact header*/
	if(uaDlgGetSecureFlag(_dlg)==TRUE){
		if(uaUserGetSIPSContact(user))
			sprintf(buf,"Contact:%s\r\n",uaUserGetSIPSContact(user));
		else{
			UaCoreERR("[uaUPDATEMsg] SIPS contact is not found!\n");
			sipReqFree(messageMsg);
			return NULL;
		}
	}else
		sprintf(buf,"Contact:%s\r\n",uaMgrGetContactAddr(mgr));

	/*CSeq header*/
	uaMgrAddCmdSeq(mgr);
	cseq=uaNewCSeq(uaMgrGetCmdSeq(mgr),SIP_UPDATE);
	sipReqAddHdr(messageMsg,CSeq,cseq);
	uaFreeCSeq(cseq);

	/*Max-Forward*/
	maxfor=uaUserGetMaxForwards(user);
	if(maxfor<70) maxfor=70;
	/*sprintf(buf,"Max-Forwards:70\r\n");*/
	sprintf(buf,"Max-Forwards:%d\r\n",maxfor);
	rCode=sipReqAddHdrFromTxt(messageMsg,Max_Forwards,buf);
	/* privacy */	
	if(uaDlgGetPrivacy(_dlg)){
		sprintf(buf,"Privacy:%s\r\n",uaDlgGetPrivacy(_dlg));
		rCode=sipReqAddHdrFromTxt(messageMsg,Privacy,buf);
	}

	bodylen=uaContentGetLength(_content);
	if(bodylen>0){
		sprintf(buf,"Content-Length:%d\r\n",strlen(uaContentGetBody(_content)));
		rCode=sipReqAddHdrFromTxt(messageMsg,Content_Length,buf);
		sprintf(buf,"Content-Type:%s\r\n",uaContentGetType(_content));
		rCode=sipReqAddHdrFromTxt(messageMsg,Content_Type,buf);
	}else{
		sprintf(buf,"Content-Length:0\r\n");
		rCode=sipReqAddHdrFromTxt(messageMsg,Content_Length,buf);
	}

	/* session expires header */
	if(uaDlgGetSETimer(_dlg)>0){
		if(uaDlgGetSEType(_dlg)==1)
			sprintf(buf,"Session-Expires:%d;refresher=uac\r\n",uaDlgGetSETimer(_dlg));	
		else if(uaDlgGetSEType(_dlg)==2)
			sprintf(buf,"Session-Expires:%d;refresher=uas\r\n",uaDlgGetSETimer(_dlg));
		rCode=sipReqAddHdrFromTxt(messageMsg,Session_Expires,buf);
	}

#ifdef _DEBUG
	/* User-Agent header */
	sprintf(buf,"User-Agent:%s\r\n",USER_AGENT);
	rCode=sipReqAddHdrFromTxt(messageMsg,User_Agent,buf);
	/* Content-Language */
	sprintf(buf,"Content-Language:%s\r\n",DEFAULT_LANGUAGE);
	rCode=sipReqAddHdrFromTxt(messageMsg,Content_Language,buf);
#endif

	/* authz header add by tyhuang 2003.10.1 */
	authz=uaUserGetAuthData(user);
	if(authz){
		sprintf(buf,"%s",authz);
		rCode=sipReqAddHdrFromTxt(messageMsg,Authorization,buf);
	}

	/* add content to sip message body */
	if((messageMsg != NULL)&&(uaContentGetBody(_content)!=NULL)){
		tmpBody=(SipBody*)calloc(1,sizeof(SipBody));
		tmpBody->length=bodylen;
		tmpBody->content=strDup(uaContentGetBody(_content));
		rCode=sipReqSetBody(messageMsg,tmpBody);
		if(rCode != RC_OK){
			sipReqFree(messageMsg);
			messageMsg=NULL;
		}
	}
	if(tmpBody != NULL)
		sipBodyFree(tmpBody);
#ifdef _DEBUG
	TCRPrint(TRACE_LEVEL_API,"[uaUPDATEMsg]create a UPDATE message\n%s!\n",sipReqPrint(messageMsg));
#endif
	return messageMsg;	
}

SipReq	uaPRACKMsg(IN UaDlg _dlg,IN const char* _dest, IN UaContent _content , IN unsigned int rseq)
{

	SipReqLine *pLine=NULL,*pNewLine=NULL;
	SipCSeq *cseq;
	SipFrom *pFrom=NULL,*pFromTag=NULL;
	RCODE rCode;
	SipReq messageMsg=NULL,invitereq;
	SipRsp finalRsp=NULL;
	UaMgr mgr=NULL;
	UaUser user;
	UaCfg  cfg;
	SipURL url;
	SipBody	*tmpBody=NULL;
	char buf[MAXBUFFERSIZE]={'\0'};
	char ip[MAXADDRESIZE]={'\0'},*username,*laddr,*hostname,*authz,*callid;
	int maxfor,bodylen;

	
	/* check dialog */
	if(_dlg == NULL){
		UaCoreERR("[uaPRACKMsg] dialog is NULL!\n");
		return NULL;
	}
	/*check input parameter is NULL or not*/
	if(_dest == NULL){
		UaCoreERR("[uaPRACKMsg] destination is NULL!\n");
		return NULL;
	}
	mgr=uaDlgGetMgr(_dlg);
	/*get parent mgr, if NULL : Error */
	if(mgr==NULL){
		UaCoreERR("[uaPRACKMsg] mgr is NULL!\n");
		return NULL;
	}
	/*get user object, get user information*/
	user=uaMgrGetUser(mgr);
	if(user == NULL){
		UaCoreERR("[uaPRACKMsg] user is NULL!\n");
		return NULL;
	}
	/*get cfg object, get configuration information*/
	cfg=uaMgrGetCfg(mgr);
	if(cfg==NULL)
		return NULL;

	invitereq=uaDlgGetRequest(_dlg);
	if(uaGetReqMethod(invitereq)!= INVITE){
		UaCoreERR("[uaPRACKMsg] prack only for invite!\n");
		return NULL;
	}
	/*create new empty request message*/
	messageMsg=sipReqNew();
	if(messageMsg == NULL){
		UaCoreERR("[uaPRACKMsg] To create new Request is failed!\n");
		return NULL;
	}

	/*request line*/
	url=sipURLNewFromTxt((char*)_dest);
	pNewLine=uaReqNewReqLine(PRACK,sipURLToStr(url),SIP_VERSION);
	if(url)
		sipURLFree(url);
	if(NULL != pNewLine){
		rCode=sipReqSetReqLine(messageMsg,pNewLine);
		if(RC_OK != rCode){
			uaReqLineFree(pNewLine);
			sipReqFree(messageMsg);
		}
		if(NULL != pNewLine)
			uaReqLineFree(pNewLine);
	}else{
		sipReqFree(messageMsg);
		UaCoreERR("[uaPRACKMsg] To create new request line is failed!\n");
		return NULL;
	}
	

	username=(char*)uaUserGetName(user);
	laddr=(char*)uaUserGetLocalAddr(user);
	hostname=(char*)uaUserGetLocalHost(user);
	if((NULL==username)||(NULL==laddr)||(NULL==hostname)){
		sipReqFree(messageMsg);
		UaCoreERR("[uaPRACKMsg] username,localaddr or hostname is NULL!\n");
		return NULL;
	}

	/*copy local ip address into ip buffer*/
	strcpy(ip,laddr);


	callid=uaDlgGetCallID(_dlg);
	if(callid==NULL){
		callid=sipCallIDGen(hostname,ip);
		uaDlgSetCallID(_dlg,(const char*)callid);
	}
	sprintf(buf,"Call-ID:%s\r\n",callid);
	rCode=sipReqAddHdrFromTxt(messageMsg,Call_ID,buf);
	

	finalRsp=uaDlgGetResponse(_dlg);
	/*To header*/
	rCode=sipReqAddHdr(messageMsg,To,sipRspGetHdr(finalRsp,To));
	/*From header */
	rCode=sipReqAddHdr(messageMsg,From,sipRspGetHdr(finalRsp,From));
	
	/*CSeq header*/
	uaMgrAddCmdSeq(mgr);
	cseq=uaNewCSeq(uaMgrGetCmdSeq(mgr),PRACK);
	sipReqAddHdr(messageMsg,CSeq,cseq);
	uaFreeCSeq(cseq);

	/*Max-Forward*/
	maxfor=uaUserGetMaxForwards(user);
	if(maxfor<70) maxfor=70;
	/*sprintf(buf,"Max-Forwards:70\r\n");*/
	sprintf(buf,"Max-Forwards:%d\r\n",maxfor);
	rCode=sipReqAddHdrFromTxt(messageMsg,Max_Forwards,buf);

	/* privacy */	
	if(uaDlgGetPrivacy(_dlg)){
		sprintf(buf,"Privacy:%s\r\n",uaDlgGetPrivacy(_dlg));
		rCode=sipReqAddHdrFromTxt(messageMsg,Privacy,buf);
	}

	bodylen=uaContentGetLength(_content);
	if(bodylen>0){
		sprintf(buf,"Content-Length:%d\r\n",strlen(uaContentGetBody(_content)));
		rCode=sipReqAddHdrFromTxt(messageMsg,Content_Length,buf);
		sprintf(buf,"Content-Type:%s\r\n",uaContentGetType(_content));
		rCode=sipReqAddHdrFromTxt(messageMsg,Content_Type,buf);
		
	}else{
		sprintf(buf,"Content-Length:0\r\n");
		rCode=sipReqAddHdrFromTxt(messageMsg,Content_Length,buf);
	}

	/* RAck header */
	if(invitereq){
		cseq=(SipCSeq*)sipReqGetHdr(invitereq,CSeq);
		if(cseq){
			sprintf(buf,"RAck:%d %d INVITE\r\n",rseq,cseq->seq);
			rCode=sipReqAddHdrFromTxt(messageMsg,RAck,buf);
		}
	}
#ifdef _DEBUG
	/* User-Agent header */
	sprintf(buf,"User-Agent:%s\r\n",USER_AGENT);
	rCode=sipReqAddHdrFromTxt(messageMsg,User_Agent,buf);
	/* Content-Language */
	sprintf(buf,"Content-Language:%s\r\n",DEFAULT_LANGUAGE);
	rCode=sipReqAddHdrFromTxt(messageMsg,Content_Language,buf);
#endif

	/* authz header add by tyhuang 2003.10.1 */
	authz=uaUserGetAuthData(user);
	if(authz){
		sprintf(buf,"%s",authz);
		rCode=sipReqAddHdrFromTxt(messageMsg,Authorization,buf);
	}

	/* add content to sip message body */
	if((messageMsg != NULL)&&(uaContentGetBody(_content)!=NULL)){
		tmpBody=(SipBody*)calloc(1,sizeof(SipBody));
		tmpBody->length=bodylen;
		tmpBody->content=strDup(uaContentGetBody(_content));
		rCode=sipReqSetBody(messageMsg,tmpBody);
		if(rCode != RC_OK){
			sipReqFree(messageMsg);
			messageMsg=NULL;
		}
	}
	if(tmpBody != NULL)
		sipBodyFree(tmpBody);
#ifdef _DEBUG
	TCRPrint(TRACE_LEVEL_API,"[uaPRACKMsg]create a UPDATE message\n%s!\n",sipReqPrint(messageMsg));
#endif
	return messageMsg;	
}

/*input original register dialog and txStruct, and original sipReq and final 401 response*/
SipReq	uaRegAuthnMsg(IN UaDlg _dlg, 
		      IN TxStruct _tx, 
		      IN SipReq	_origreq,
		      IN SipRsp _finalrsp)
{
	SipReq regMsg=NULL;
	UaMgr   mgr=NULL;
	UaUser  user=NULL;
	UaCfg	cfg=NULL;
	SipCSeq *pCseq=NULL;
	SipVia *pVia=NULL;
	int rspStatus=UnKnown_Status,nc=0;
	UAAuthnType RspType=AUTHN_UNKNOWN;
	UAAuthzType ReqType=AUTHZ_UNKNOWN;
	SipMethodType method;
	RCODE rCode;
	

	if(_dlg == NULL){ /*dialog not exist */
		UaCoreERR("[uaRegAuthnMsg] dialog is NULL!\n");
		return NULL;
	}
	/*can't get right response key, not 401 or 407 response*/
	rspStatus=uaGetRspStatusCode(_finalrsp);
	if(!((rspStatus ==Unauthorized)||(rspStatus == Proxy_Authentication_Required))){
		UaCoreERR("[uaRegAuthnMsg] can't get right response key, not 401 or 407 response!\n");
		return NULL;
	}
	/*get manager object*/
	mgr=uaDlgGetMgr(_dlg);
	if(mgr==NULL){
		UaCoreERR("[uaRegAuthnMsg] mgr is NULL!\n");
		return NULL;
	}
	/*if original request or final response is NULL, Error!*/
	if((_origreq == NULL) || (_finalrsp == NULL)){
		UaCoreERR("[uaRegAuthnMsg] original request or final response is NULL!\n");
		return NULL;
	}
	/*get general information*/
	cfg=uaMgrGetCfg(mgr);
	user=uaMgrGetUser(mgr);
	if((cfg==NULL)||(user==NULL)){
		UaCoreERR("[uaRegAuthnMsg] cfg or user is NULL!\n");
		return NULL;
	}
	/*duplicate new register message from original request*/
	regMsg=sipReqDup(_origreq);

	/* delete old via header */
	pVia=sipReqGetHdr(regMsg,Via);
	while(pVia){
		if(pVia->numParams>0){
			sipReqDelViaHdrTop(regMsg);	
			pVia=sipReqGetHdr(regMsg,Via);
		}else
			break;
	}
	
	if(regMsg == NULL){
		UaCoreERR("[uaRegAuthnMsg] To creat new Request is failed!\n");
		return NULL;
	}
	method=uaGetReqMethod(_origreq);
	/*modify CSeq header*/
	/* mark by tyhuang 2003.9.30
	if(rspStatus ==Unauthorized)
		pCseq=uaNewCSeq(uaGetReqCSeq(_origreq)+1,REGISTER);
	else if(rspStatus ==Proxy_Authentication_Required)
		pCseq=uaNewCSeq(uaGetReqCSeq(_origreq)+1,INVITE);
	*/
	pCseq=uaNewCSeq(uaGetReqCSeq(_origreq)+1,method);
	uaMgrAddCmdSeq(mgr);
	if(pCseq!=NULL){
		sipReqAddHdr(regMsg,CSeq,pCseq);
		uaFreeCSeq(pCseq);
	}

	/*should add WWWW-Authenticate header*/
	/*check response status is WWW-Authenticate or Proxy-Authenticate*/
	(rspStatus == Unauthorized) ? (RspType=WWW_AUTHN):(RspType=PROXY_AUTHN);
	(rspStatus == Unauthorized) ? (ReqType=AUTHZ):(ReqType=PROXY_AUTHZ);

	/*Prepare authorized */
	/*didn't support BASIC Authenticate*/

	/*Do Digest Authenticate*/
	{
		SipDigestWWWAuthn wwwauth;
		SipDigestAuthz author;
	
		int RetCode=-1;
		char *username=NULL,*passwd=NULL,*authzstr=NULL;
		UaAuthz authz=NULL;
		char buf[512]={'\0'};
		int buflen=511,len;
		DigestCB digcb=NULL;
	
#ifdef _WIN32	
		unsigned int ltime;
		ltime=uagettickcount();
#else
		int ltime;
		ltime=rand();
#endif
	
		RetCode=uaRspGetDigestAuthn(_finalrsp,RspType,&wwwauth);
		author.flag_ =0;

		if(wwwauth.flags_ & SIP_DWAFLAG_ALGORITHM){
			strcpy(author.algorithm_,wwwauth.algorithm_); 
			author.flag_ |= SIP_DAFLAG_ALGORITHM;
		}

		/*strcpy(author.cnonce_,"0a4f113b"); 
		author.flag_ |= SIP_DAFLAG_CNONCE;*/

		strcpy(author.digest_uri_,uaGetReqReqURI(_origreq));
		if(wwwauth.flags_ & SIP_DWAFLAG_QOP) {
			strcpy(author.message_qop_,"auth"); 
			author.flag_ |= SIP_DAFLAG_MESSAGE_QOP;
			sprintf(buf,"%08x",ltime);
			/*strcpy(author.cnonce_,"0a4f113b"); */
			strcpy(author.cnonce_,buf);
			author.flag_ |= SIP_DAFLAG_CNONCE;

		}

		if(wwwauth.flags_ & SIP_DWAFLAG_NONCE) {
			strcpy(author.nonce_,wwwauth.nonce_);
			nc=uaMgrGetCmdSeq(mgr);
			sprintf(buf,"%08x",nc);
			/*strcpy(author.nonce_count_,"0000001"); */
			strcpy(author.nonce_count_,buf);
			author.flag_ |= SIP_DAFLAG_NONCE_COUNT;
		}
		if(wwwauth.flags_ &  SIP_DWAFLAG_OPAQUE){
			strcpy(author.opaque_,wwwauth.opaque_); 
			author.flag_ |= SIP_DAFLAG_OPAQUE;
		}
		strcpy(author.realm_,wwwauth.realm_);
		

		authz=uaUserGetMatchAuthz(user,wwwauth.realm_);
			
		if(authz !=NULL){
			username=uaAuthzGetUserName(authz);
			passwd=uaAuthzGetPasswd(authz);
			uaDlgDelUnSupportRealm(_dlg);
		}else{
			uaDlgAddUnSupportRealm(_dlg,wwwauth.realm_);
			sipReqFree(regMsg);
			UaCoreERR("[uaRegAuthnMsg] To get match Authzation is failed!\n");
			return NULL;
		}
			
		strcpy(author.username_,username);
		/*
		digcb=uaUserGetDigestCB(user);
		if(digcb){
			digcb(&author,passwd);
		}else
		*/
		/* debug */	
		if(username != NULL){
			SipAuthz *reqAuth;

			if(ReqType==AUTHZ)
				reqAuth=(SipAuthz *)sipReqGetHdr(_origreq,Authorization);
			else
				reqAuth=(SipAuthz *)sipReqGetHdr(_origreq,Proxy_Authorization);

			while(reqAuth != NULL){
				SipDigestAuthz reqdig;
				RCODE ret=uaGetDigestAuthz(reqAuth,&reqdig);

				if((ret==RC_OK)&&(wwwauth.stale_==FALSE)
					&&(strcmp(wwwauth.realm_,reqdig.realm_)==0)&&(strcmp(username,reqdig.username_)==0)){
					sipReqFree(regMsg);
					TCRPrint(TRACE_LEVEL_API,"can not find username/password!\n");
					return NULL;
				}
				reqAuth=reqAuth->next;
			}
		}
		TCRPrint(TRACE_LEVEL_API,"get username/password for authenticate\n");

		sipDigestAuthzGenRspKey(&author,passwd,(char*)uaSipMethodToString(uaGetReqMethod(_origreq)),NULL);
		
		uaReqAddDigestAuthz(regMsg,ReqType,author);
		/* add auth to user*/
		if(digcb&&(ReqType==AUTHZ)){
			AuthorizationAsStr((SipAuthz*)sipReqGetHdr(regMsg,Authorization),buf,buflen,&len);
			rCode=uaUserSetAuthData(user,buf);
		}

	}
#ifdef _DEBUG
	TCRPrint(TRACE_LEVEL_API,"[uaRegAuthnMsg]create a message with Auth\n%s!\n",sipReqPrint(regMsg));
#endif
	return regMsg;
}


SipReq uaOPTMsg(IN UaDlg);

/*Check Request message contain Replaces or not*/
UaDlg	uaReqCheckReplaceMsg(IN UaMgr _mgr,IN SipReq _req)
{
	UaDlg matchdlg=NULL;
	SipReplaces *pReplace=NULL;

	if(NULL == _mgr)
		return NULL;

	if(NULL == _req)
		return NULL;

	pReplace=sipReqGetHdr(_req,Replaces);
	if(NULL == pReplace){
		return NULL;
	}
	/*get match dialog,by callid compare*/
	if(NULL != pReplace->callid){
		matchdlg=uaMgrGetMatchDlg(_mgr,pReplace->callid);
	}
	return matchdlg;
}


SipRsp uaCreateRspFromReq(IN SipReq _req,IN UAStatusCode rspcode,IN char *_contact, IN SdpSess _sdp)
{
	return uaCreateRspFromReqEx(_req,rspcode,_contact,_sdp,SESSION_TIMER_INTERVAL);
}

SipRsp uaCreateRspFromReqEx(IN SipReq _req,IN UAStatusCode rspcode,IN char *_contact, IN SdpSess _sdp,IN unsigned int _SE)
{
	RCODE rCode;
	SipRsp rsp=NULL;
	SipRspStatusLine* pStatusLine=NULL;
	/*SipReqLine* pLine=NULL;*/
	SipRecordRoute* pRecord;

	SipTo *pNewTo=NULL;
	char buf[MAXBUFFERSIZE]={'\0'};


	if(_req != NULL){
		rsp=sipRspNew();
		pStatusLine=uaRspNewStatusLine(SIP_VERSION,rspcode,NULL);
		if(pStatusLine==NULL){
			sipRspFree(rsp);
			UaCoreERR("[uaCreateRspFromReq] To creat status line is failed!\n");
			return NULL;
		}
		rCode=sipRspSetStatusLine(rsp,pStatusLine);
		uaRspStatusLineFree(pStatusLine);
		/*copy Call-ID*/
		sipRspAddHdr(rsp,Call_ID,sipReqGetHdr(_req,Call_ID));
		/*copy From*/
		sipRspAddHdr(rsp,From,sipReqGetHdr(_req,From));
		/*copy To*/
		sipRspAddHdr(rsp,To,sipReqGetHdr(_req,To));
		/*copy cseq*/
		sipRspAddHdr(rsp,CSeq,sipReqGetHdr(_req,CSeq));
		/*add contact*/
		if(_contact !=NULL){
			char buf[240]={'\0'};
			sprintf(buf,"Contact:%s\r\n",_contact);
			sipRspAddHdrFromTxt(rsp,Contact,buf);
		}
		
		/*copy Via*/
		sipRspAddHdr(rsp,Via,sipReqGetHdr(_req,Via));
		/*reverse Record-Route header*/
		/*response should not add Route header*/
		pRecord=sipReqGetHdr(_req,Record_Route);
		
		if(pRecord != NULL)
			sipRspAddHdr(rsp,Record_Route,pRecord);
			/*sipRspAddHdr(rsp,Route,uaRecordRouteCovertToRoute(pRecord));*/
		/*add body related*/
		if(_sdp !=NULL){
			SipBody *pBody=NULL;
			pBody=uaCreatSipBody(_sdp);
			if(pBody!=NULL){
				uaRspAddBody(rsp,pBody,"application/sdp");
				uaFreeSipBody(pBody);
			}else{
				sipRspFree(rsp);
				UaCoreERR("[uaCreateRspFromReq] To add a sdp to body is failed!\n");
				return NULL;
			}
		}else
			uaRspAddBody(rsp,NULL,NULL);

		/*except 100, all response add To tag*/
		if((rspcode > Trying)&&(uaGetReqToTag(_req)==NULL)){	
			pNewTo=uaToDupAddTag(sipReqGetHdr(_req,To));
			if(pNewTo != NULL){
				sipReqAddHdr(_req,To,pNewTo);
				sipRspAddHdr(rsp,To,pNewTo);
				uaToHdrFree(pNewTo);
			}
		}
		
		/* add header for each response */
		switch(rspcode){
		
		case sipOK:{/*should add To tag*/
			int idx=0,length;
			char buffer[260]={'\0'};
			SipAllow pAllow;
			SipSessionExpires *pSE;
			SipMinSE *pminse;
			SipMethodType method=uaGetReqMethod(_req);
			
			if((method==INVITE)||(method==OPTIONS)){
				/* Allowed Header */
				for(idx=0;idx<SIP_SUBSCRIBE;idx++)
					pAllow.Methods[idx]=1;

				sipRspAddHdr(rsp,Allow,&pAllow);

				/* enable/disable session-timer */
				/* Supported Header */
				if (_SE>0) 
					sprintf(buf,"Supported:%s\r\n",SUPPORT_LIST);
				else
					sprintf(buf,"Supported:%s\r\n",SUPPORT_LIST_NO_TIMER);
				
				sipRspAddHdrFromTxt(rsp,Supported,buf);
			}
			
			/* enable/disable session-timer */
			/* copy session-expire and Min-SE if it exists */
			if (_SE>0) {
				pSE=(SipSessionExpires*)sipReqGetHdr(_req,Session_Expires);
				if(pSE){
					SessionExpiresAsStr(pSE,buffer,255,&length);
					if(strstr(buffer,"refresher="))
						sipRspAddHdr(rsp,Session_Expires,(void*)pSE);
					else{
						sprintf(buffer,"%s;refresher=uas",buffer);
						sipRspAddHdrFromTxt(rsp,Session_Expires,buffer);
					}
				}
				pminse=(SipMinSE*)sipReqGetHdr(_req,Min_SE);
				if(pminse)
					sipRspAddHdr(rsp,Min_SE,(void*)pminse);
			}
			break;
		}
		case Method_Not_Allowed:{/*add Allow header*/
			int idx;
			SipAllow pAllow;

			for(idx=0;idx<SIP_SUBSCRIBE;idx++)
				pAllow.Methods[idx]=1;

			sipRspAddHdr(rsp,Allow,&pAllow);
			break;
		}
		case Unsupported_Media_Type:
			/* Accept */
			sprintf(buf,"Accept:application/sdp\r\n");
			sipRspAddHdrFromTxt(rsp,Accept,buf);
			/* Content-Language */
			sprintf(buf,"Accept-Language:%s\r\n",DEFAULT_LANGUAGE);
			sipRspAddHdrFromTxt(rsp,Accept_Language,buf);
			/* Accept-Encoding */
			sprintf(buf,"Accept-Encoding:identity\r\n");
			sipRspAddHdrFromTxt(rsp,Accept_Encoding,buf);
			break;
		case UA_BUSY_HERE:
			break;
		case Bad_Event:
			sprintf(buf,"Allow-Events:presence,refer\r\n");
			sipRspAddHdrFromTxt(rsp,Allow_Events,buf);
			break;
		case Bad_Extension:{
			SipRequire *pRequire;
			int i,pos;
			SipStr *tmp;

			pRequire = (SipRequire*)sipReqGetHdr(_req,Require);
			pos=sprintf(buf,"Unsupported:");
			tmp=pRequire->sipOptionTagList;
			for(i=0;i<pRequire->numTags;i++){
				if(tmp==NULL) break;
				if(i>0)
					pos+=sprintf(buf+pos,",%s",tmp->str);
				else
					pos+=sprintf(buf+pos,"%s",tmp->str);
				tmp=tmp->next;
			}	
			pos+=sprintf(buf+pos,"\r\n");
			sipRspAddHdrFromTxt(rsp,Unsupported,buf);
			break;
		}
		case Session_Inerval_Too_Small:
			sprintf(buf,"Min-SE:%d\r\n",MINSE_EXPIRES);
			sipRspAddHdrFromTxt(rsp,Min_SE,buf);
			break;
		case Not_Acceptable_Here:
			/* Warning */
			sprintf(buf,"Warning:305 %s \"session description isn't understood\"\r\n",USER_AGENT);
			sipRspAddHdrFromTxt(rsp,Warning,buf);
			break;
		case Internal_Server_Error:
			/* retry-after */
			sprintf(buf,"Retry-After:%d\r\n",rand()%10+1);
			sipRspAddHdrFromTxt(rsp,Retry_After,buf);
		default:
			break;
		}
	}

	return rsp;
}

SipRsp uaCreateRspMsg(IN SipReq _req,IN UAStatusCode rspcode,IN const char *_contact, IN UaContent _content)
{
	return uaCreateRspMsgEx(_req,rspcode,_contact,_content,SESSION_TIMER_INTERVAL);
}

SipRsp uaCreateRspMsgEx(IN SipReq _req,IN UAStatusCode rspcode,IN const char *_contact, IN UaContent _content,IN unsigned int _SE)
{
	RCODE rCode;
	SipRsp rsp=NULL;
	SipRspStatusLine* pStatusLine=NULL;
	/*SipReqLine* pLine=NULL;*/
	SipRecordRoute* pRecord;
	SipTo *pNewTo=NULL;
	int bodylen=0;
	char buf[MAXBUFFERSIZE]={'\0'};
	

	if(_req != NULL){
		rsp=sipRspNew();
		pStatusLine=uaRspNewStatusLine(SIP_VERSION,rspcode,NULL);
		if(pStatusLine==NULL){
			UaCoreERR("[uaCreateRspMsg] To creat status line is failed!\n");
			return NULL;
		}
		rCode=sipRspSetStatusLine(rsp,pStatusLine);
		uaRspStatusLineFree(pStatusLine);
		/*copy Call-ID*/
		sipRspAddHdr(rsp,Call_ID,sipReqGetHdr(_req,Call_ID));
		/*copy From*/
		sipRspAddHdr(rsp,From,sipReqGetHdr(_req,From));
		/*copy To*/
		sipRspAddHdr(rsp,To,sipReqGetHdr(_req,To));
		/*copy cseq*/
		sipRspAddHdr(rsp,CSeq,sipReqGetHdr(_req,CSeq));
		/*add contact*/
		if(_contact !=NULL){
			sprintf(buf,"Contact:%s\r\n",_contact);
			sipRspAddHdrFromTxt(rsp,Contact,buf);
		}
		/*copy Via*/
		sipRspAddHdr(rsp,Via,sipReqGetHdr(_req,Via));
		/*reverse Record-Route header*/
		/*response should not add Route header*/
		pRecord=sipReqGetHdr(_req,Record_Route);
		if(pRecord != NULL)
			sipRspAddHdr(rsp,Record_Route,pRecord);
			/*sipRspAddHdr(rsp,Route,uaRecordRouteCovertToRoute(pRecord));*/
		/* if length of content > 0, add sip Header:content-type and content-length */
		bodylen=uaContentGetLength(_content);
		if(bodylen>0){
			sprintf(buf,"Content-Length:%d\r\n",bodylen);
			rCode=sipRspAddHdrFromTxt(rsp,Content_Length,buf);
			sprintf(buf,"Content-Type:%s\r\n",uaContentGetType(_content));
			rCode=sipRspAddHdrFromTxt(rsp,Content_Type,buf);
		}else{
			sprintf(buf,"Content-Length:0\r\n");
			rCode=sipRspAddHdrFromTxt(rsp,Content_Length,buf);
		}

#ifdef _DEBUG
		/* User-Agent header */
		sprintf(buf,"User-Agent:%s\r\n",USER_AGENT);
		rCode=sipRspAddHdrFromTxt(rsp,User_Agent,buf);
#endif

		/*add body related*/
		if(_content !=NULL){
			SipBody *tmpBody=NULL;
			tmpBody=(SipBody*)calloc(1,sizeof(SipBody));
			
			if(tmpBody!=NULL){
				tmpBody->content=strDup(uaContentGetBody(_content));
				tmpBody->length=bodylen;
				rCode=sipRspSetBody(rsp,tmpBody);
				/* add body to response message */
				if(rCode != RC_OK){
					UaCoreERR("[uaCreateRsp] set body failed!");
					/*sipRspFree(rsp);
					rsp=NULL;*/
					uaRspAddBody(rsp,NULL,NULL);
				}
				sipBodyFree(tmpBody);
			}else{
				sipRspFree(rsp);
				UaCoreERR("[uaCreateRspMsg] To create body is failed!\n");
				return NULL;
			}
		}else
			uaRspAddBody(rsp,NULL,NULL);

		/*except 200, all response add To tag*/
		if(rspcode >= sipOK){
			pNewTo=uaToDupAddTag(sipReqGetHdr(_req,To));
			if(pNewTo != NULL){
				sipRspAddHdr(rsp,To,pNewTo);
				uaToHdrFree(pNewTo);
			}
		}

		switch(rspcode){
		
		case sipOK:{/*should add To tag*/
			int idx=0,length;
			char buffer[260]={'\0'};
			SipAllow pAllow;
			SipSessionExpires *pSE;
			SipMinSE *pminse;
			SipMethodType method=uaGetReqMethod(_req);

			if((method==INVITE)||(method==OPTIONS)){

				for(idx=0;idx<SIP_SUBSCRIBE;idx++)
					pAllow.Methods[idx]=1;

					sipRspAddHdr(rsp,Allow,&pAllow);
			}
			/* enable/disable session-timer */
			/* copy session-expire and Min-SE if _SE >0 and it exists */
			if (_SE>0) {
				pSE=(SipSessionExpires*)sipReqGetHdr(_req,Session_Expires);
				if(pSE){
					SessionExpiresAsStr(pSE,buffer,255,&length);
					if(strstr(buffer,";refresher="))
						sipRspAddHdr(rsp,Session_Expires,(void*)pSE);
					else{
						sprintf(buffer,"%s;refresher=uas",buffer);
						sipRspAddHdrFromTxt(rsp,Session_Expires,buffer);
					}
				}
				pminse=(SipMinSE*)sipReqGetHdr(_req,Min_SE);
				if(pminse)
					sipRspAddHdr(rsp,Min_SE,(void*)pminse);
			}
			break;
			}
		case Method_Not_Allowed:{/*add Allow header*/
			int idx=0;
			SipAllow pAllow;	

			for(idx=0;idx<SIP_SUBSCRIBE;idx++)
				pAllow.Methods[idx]=1;

			sipRspAddHdr(rsp,Allow,&pAllow);
			break;
			}
		case Unsupported_Media_Type:
			/* Accept */
			sprintf(buf,"Accept:application/sdp,\r\n");
			sipRspAddHdrFromTxt(rsp,Accept,buf);
			/* Content-Language */
			sprintf(buf,"Accept-Language:%s\r\n",DEFAULT_LANGUAGE);
			sipRspAddHdrFromTxt(rsp,Accept_Language,buf);
			/* accept-Encoding */
			sprintf(buf,"Accept-Encoding:identity\r\n");
			sipRspAddHdrFromTxt(rsp,Accept_Encoding,buf);
			break;
		case UA_BUSY_HERE:
			break;
		case Bad_Event:
			sprintf(buf,"Allow-Events:presence,refer\r\n");
			sipRspAddHdrFromTxt(rsp,Allow_Events,buf);
			break;
		case Session_Inerval_Too_Small:
			sprintf(buf,"Min-SE:%d\r\n",MINSE_EXPIRES);
			sipRspAddHdrFromTxt(rsp,Min_SE,buf);
			break;
		case Bad_Extension:{
			SipRequire *pRequire;
			int i,pos;
			SipStr *tmp;

			pRequire = (SipRequire*)sipReqGetHdr(_req,Require);
			pos=sprintf(buf,"Unsupported:");
			tmp=pRequire->sipOptionTagList;
			for(i=0;i<pRequire->numTags;i++){
				if(tmp==NULL) break;
				if(i>0)
					pos+=sprintf(buf+pos,",%s",tmp->str);
				else
					pos+=sprintf(buf+pos,"%s",tmp->str);
				tmp=tmp->next;
			}	
			pos+=sprintf(buf+pos,"\r\n");
			sipRspAddHdrFromTxt(rsp,Unsupported,buf);
			break;
			}
		case Not_Acceptable_Here:
			/* Warning */
			sprintf(buf,"Warning: 399 CCL_SIP_UA \"no resource\"\r\n");
			sipRspAddHdrFromTxt(rsp,Warning,buf);
			break;
		default:
			break;
		}
	}

	return rsp;
}

SipFrom* uaFromDupAddTag(IN SipFrom *_from)
{
	if(_from != NULL){
		SipFrom *pNewFrom=(SipFrom*)calloc(1,sizeof(*pNewFrom));
		int num=0;
		SipAddr *pAddr=_from->address;
		pNewFrom->numAddr=_from->numAddr;
		pNewFrom->address=uaAddrDup(pAddr,_from->numAddr);
		pNewFrom->ParamList=uaParameterDupAddTag(pAddr,_from->ParamList,_from->numParam,&num);
		pNewFrom->numParam=num;
		return pNewFrom;
	}else
		return NULL;
}

RCODE uaFromHdrFree(IN SipFrom* _from)
{
	RCODE rCode=RC_OK;
	if(_from != NULL){
		if(_from->address != NULL)
			uaAddrFree(_from->address);
		if(_from->ParamList != NULL)
			uaParameterFree(_from->ParamList);
		free(_from);
	}else
		rCode=UASIPMSG_FROM_NULL;

	return rCode;
}


SipTo* uaToDupAddTag(IN SipTo *_to)
{
	if(_to != NULL){
		SipTo *pNewTo=(SipTo*)calloc(1,sizeof(*pNewTo));
		int num=0;
		SipAddr *pAddr=_to->address;
		pNewTo->numAddr=_to->numAddr;
		pNewTo->address=uaAddrDup(pAddr,_to->numAddr);
		pNewTo->paramList=uaParameterDupAddTag(pAddr,_to->paramList,_to->numAddr,&num);
		pNewTo->numParam=num;
		return pNewTo;
	}else
		return NULL;
}

RCODE uaToHdrFree(IN SipTo* _to)
{
	RCODE rCode=RC_OK;
	if(_to != NULL){
		if(_to->address != NULL)
			uaAddrFree(_to->address);
		if(_to->paramList != NULL)
			uaParameterFree(_to->paramList);
		free(_to);
	}else
		rCode=UASIPMSG_TO_NULL;

	return rCode;
}


SipRoute* uaRecordRouteCovertToRoute(IN SipRecordRoute* _pRecord)
{
	SipRoute *pRoute;
	SipRecAddr *tmpAddr,*newAddr,*preAddr;
	int i=0;

	/* Route Header Copy From Record-Route header and reverse, in the same call-leg */
	if(_pRecord!= NULL){ /*need to copy reverse into Route*/
		pRoute=(SipRoute*) calloc(1,sizeof(SipRoute));
		tmpAddr=_pRecord->sipNameAddrList;
		for(i=0;i<_pRecord->numNameAddrs;i++) {
			if(tmpAddr == NULL) 
				break;
			newAddr=(SipRecAddr*)calloc(1,sizeof(SipRecAddr));
			newAddr->next=NULL;
			if(tmpAddr->addr != NULL)
				newAddr->addr=strDup(tmpAddr->addr);
			else	
				newAddr->addr=NULL;
			if(tmpAddr->display_name != NULL)
				newAddr->display_name=strDup(tmpAddr->display_name);
			else
				newAddr->display_name=NULL;
			if(tmpAddr->sipParamList){
				newAddr->sipParamList=sipParameterDup(tmpAddr->sipParamList,tmpAddr->numParams);
				newAddr->numParams=tmpAddr->numParams;
			}
			(i==0)? (preAddr=newAddr):(newAddr->next=preAddr);
			tmpAddr=tmpAddr->next;
			preAddr=newAddr;
		}
		if(i>0){
			pRoute->numNameAddrs=i;
			pRoute->sipNameAddrList=preAddr;
		}
		return pRoute;
	}else
		return NULL;
}

SipRoute* uaRecordRouteCovertToRouteRev(IN SipRecordRoute* _pRecord)
{
	SipRoute *pRoute;
	SipRecAddr *tmpAddr,*newAddr,*preAddr;
	int i=0;

	/* Route Header Copy From Record-Route header, in the same call-leg */
	if(_pRecord!= NULL){ /*need to copy into Route*/
		pRoute=(SipRoute*) calloc(1,sizeof(SipRoute));
		tmpAddr=_pRecord->sipNameAddrList;
		for(i=0;i<_pRecord->numNameAddrs;i++) {
			if(tmpAddr == NULL) 
				break;
			newAddr=(SipRecAddr*)calloc(1,sizeof(SipRecAddr));
			newAddr->next=NULL;
			if(tmpAddr->addr != NULL)
				newAddr->addr=strDup(tmpAddr->addr);
			else	
				newAddr->addr=NULL;
			if(tmpAddr->display_name != NULL)
				newAddr->display_name=strDup(tmpAddr->display_name);
			else
				newAddr->display_name=NULL;
			if(tmpAddr->sipParamList){
				newAddr->sipParamList=sipParameterDup(tmpAddr->sipParamList,tmpAddr->numParams);
				newAddr->numParams=tmpAddr->numParams;
			}
			(i==0)? (preAddr=newAddr):(newAddr->next=preAddr);
			tmpAddr=tmpAddr->next;
			preAddr=newAddr;
		}
		if(i>0){
			pRoute->numNameAddrs=i;
			pRoute->sipNameAddrList=preAddr;
		}
		return pRoute;
	}else
		return NULL;
}

RCODE uaRouteFree(IN SipRoute* _route)
{
	RCODE rCode=RC_OK;

	SipRecAddr *tmpAddr;

	if(_route!=NULL){
		tmpAddr=_route->sipNameAddrList;
		if(tmpAddr!=NULL){
			sipRecAddrFree(tmpAddr);
			tmpAddr=NULL;
		}
		free(_route);
		_route=NULL;
	}else
		rCode=UASIPMSG_ROUTE_NULL;

	return rCode;
}

/*Create a new refer-to message*/
/*input the 2nd dialg to be replaced*/
RCODE	uaCreateReferToMsg(IN UaDlg _dlg,IN char* _buf,IN OUT int* _buflen)
{
	RCODE rCode=RC_OK;
	DxStr reftostr=NULL;
	TxStruct tx=NULL;
	TXTYPE txType=TX_NON_ASSIGNED;
	char* requri=NULL;
	SipReq origReq=NULL;
	SipRsp finalRsp=NULL;
	SipContact *pContact=NULL;
	SipFrom *pFrom=NULL;
	SipURL url=NULL;
	BOOL bContactOK=FALSE;
	char uribuf[256]={'\0'},*callid=NULL,*fromtag=NULL,*totag=NULL,*pos=NULL;
	int urilen;

	if(_dlg == NULL)
		return UADLG_NULL;

	reftostr=dxStrNew();
	tx=uaDlgGetInviteTx(_dlg,TRUE);
	txType=sipTxGetType(tx);

	origReq=sipTxGetOriginalReq(tx);
	finalRsp=sipTxGetLatestRsp(tx);

	/*Add request-uri into string*/
	if(TX_CLIENT_INVITE == txType ){
		requri=(char*)uaGetReqReqURI(origReq);
		if(NULL !=requri)
			dxStrCat(reftostr,requri);
		else{
			dxStrFree(reftostr);
			return RC_ERROR;
		}

	}else if( TX_SERVER_INVITE == txType){
		pContact=sipReqGetHdr(origReq,Contact);
		if(NULL != pContact  ){
			url=sipURLNewFromTxt(pContact->sipContactList->address->addr);
			if(url != NULL)
				bContactOK=TRUE;
		}
		if(bContactOK){/*copy from Contact */
			uaContactAsReqURI(pContact,uribuf,256,&urilen);
			sipURLFree(url);
		}else{ /*copy from from header*/
			pFrom=sipRspGetHdr(finalRsp,From);
			if(pFrom != NULL)
				strcpy(uribuf,pFrom->address->addr);
			else{
				dxStrFree(reftostr);
				return RC_ERROR;
			}
		}
		dxStrCat(reftostr,uribuf);
		/*dxStrCat(reftostr,requri);*/
	}else{
		/* non-invite transaction ,so return error*/
		dxStrFree(reftostr);
		return RC_ERROR;
	}
	dxStrCat(reftostr,"?Replaces=");
	/*Add Replace=*/
	callid=strDup((char*)sipReqGetHdr(origReq,Call_ID));
	pos=strchr(callid,'@');
	if(pos){
		*pos='\0';
		pos++;
		dxStrCat(reftostr,callid);
		dxStrCat(reftostr,"%40");
		dxStrCat(reftostr,pos);
	}else
		dxStrCat(reftostr,callid);

	if(callid)
		free(callid);

	/*dxStrCat(reftostr,";");*/
	dxStrCat(reftostr,"%3B");

	/*Add to-tag*/
	if(TX_CLIENT_INVITE == txType ){
		totag=uaGetRspToTag(finalRsp);
		fromtag=uaGetRspFromTag(finalRsp);
	}else{
		totag=uaGetRspFromTag(finalRsp);
		fromtag=uaGetRspToTag(finalRsp);
	}
	if(NULL != totag){
		dxStrCat(reftostr,"to-tag");
		dxStrCat(reftostr,"%3D");
		dxStrCat(reftostr,totag);
		/*dxStrCat(reftostr,";");*/
		dxStrCat(reftostr,"%3B");
	}else{
		dxStrFree(reftostr);
		return RC_ERROR;
	}

	/*Add from-tag*/
	/*fromtag=uaGetReqFromTag(origReq);*/
	if(fromtag!=NULL){
		dxStrCat(reftostr,"from-tag");
		dxStrCat(reftostr,"%3D");
		dxStrCat(reftostr,fromtag);
	}else{
		dxStrFree(reftostr);
		return RC_ERROR;
	}
	dxStrCat(reftostr,"\r\n");

	if(dxStrLen(reftostr)< *_buflen){
		strcpy(_buf,(const char*)dxStrAsCStr(reftostr));
		*_buflen=dxStrLen(reftostr);
		dxStrFree(reftostr);
	}else{
		*_buflen=dxStrLen(reftostr);
		dxStrFree(reftostr);
		return RC_ERROR;
	}

	return rCode;

}


/*Refer-To header operation*/
RCODE	uaReferToGetAddr(SipReferTo* _referto,char* _buf,int* _len)
{
	RCODE rCode=RC_OK;
	SipURL url = NULL;
	char tmpbuf[256]={'\0'};
	DxStr str;

	if(_referto == NULL)
		return RC_ERROR;
	if(!_referto->address || !_referto->address->addr)
		return RC_ERROR;

	/*assume there is only one url*/
	url=sipURLNewFromTxt(_referto->address->addr);
	if (!url) 
		return RC_ERROR;
	str=dxStrNew();
	dxStrCat(str,"sip:");
	/*add user@hosr*/
	if(sipURLGetUser(url))
		sprintf(tmpbuf,"%s@%s",sipURLGetUser(url),sipURLGetHost(url));
	else
		sprintf(tmpbuf,"%s",sipURLGetHost(url));
	dxStrCat(str,(const char*)tmpbuf);
	if(sipURLGetPort(url) != 0){
		/*add port*/
		sprintf(tmpbuf,":%d",sipURLGetPort(url));
		dxStrCat(str,(const char*)tmpbuf);
	}
	if((*_len)>dxStrLen(str)){
		strcpy(_buf,(const char*)dxStrAsCStr(str));
	}else{
		rCode=RC_ERROR;
	}
	*_len=dxStrLen(str);
	dxStrFree(str);
	sipURLFree(url);

	return rCode;
}

/*get Refer-To header's Call-ID */
char*	uaReferToGetReplaceID(SipReferTo* _referto)
{
	char *pos=NULL,*buf=NULL,*tmp=NULL,*freeptr;

	if(_referto == NULL)
		return NULL;
	if(!_referto->address || !_referto->address->addr)
		return NULL;

	/*assume there is only one url*/
	tmp=strDup(_referto->address->addr);
	if (!tmp) 
		return NULL;
	freeptr=tmp;
	tmp=strchr(tmp,'?');
	if(tmp)
		tmp++;
	else{
		free(freeptr);
		return NULL;
	}
	pos=strstr(tmp,"Replaces");
	if(pos){
		pos=strchr(pos,'=');
		pos++;
		tmp=strchr(pos,'&');
		if (tmp) 
			*tmp='\0';
		buf=strDup(pos);
	}

	free(freeptr);
		
	
	return buf;
}

/*get Refer-To header's to-tag */
char*	uaReferToGetToTag(SipReferTo* _referto)
{
	char *pos=NULL,*ret=NULL,*buf=NULL,*tmp=NULL,*freeptr=NULL;

	if(_referto == NULL)
		return NULL;
	if(!_referto->address || !_referto->address->addr)
		return NULL;

	/*assume there is only one url*/
	tmp=strDup(_referto->address->addr);
	if (!tmp) 
		return NULL;

	/*find to-tag*/
	freeptr=tmp;
	pos=strstr(tmp,"Replaces");
	if(pos){
		pos=strstr(pos,"=");
		pos++;
		pos=strstr(pos,"to-tag");
		if(pos){
			ret=strstr(pos,"%3D");
			if(ret){
				ret+=3;
				buf=strDup(ret);
				pos=strstr(buf,"%3B");
				if(pos)
					*pos='\0';
			}else{ /*search "=" */
				ret=strchr(pos,'=');
				if(ret){
					ret++;
					buf=strDup(ret);
					pos=strchr(buf,';');
					if(pos)
						*pos='\0';

				}
			}
		}
	}

	free(freeptr);
	
	return buf;
}

/*get Refer-To header's from-tag*/
char*	uaReferToGetFromTag(SipReferTo* _referto)
{
	char *pos=NULL,*ret=NULL,*buf=NULL,*tmp=NULL,*freeptr=NULL;

	if(_referto == NULL)
		return NULL;
	if(!_referto->address || !_referto->address->addr)
		return NULL;

	/*assume there is only one url*/
	tmp=strDup(_referto->address->addr);
	if (!tmp) 
		return NULL;
	freeptr=tmp;
	pos=strstr(tmp,"Replaces");
	if(pos){
		pos=strchr(pos,'=');
		pos++;
		pos=strstr(pos,"from-tag");
		if(pos){
			ret=strstr(pos,"%3D");
			if(ret){
				ret+=3;
				buf=strDup(ret);
				pos=strstr(buf,"%3B");
				if(pos)
					*pos='\0';
			}else{ /*search "=" */
				ret=strchr(pos,'=');
				if(ret){
					ret++;
					buf=strDup(ret);
					pos=strchr(buf,';');
					if(pos)
						*pos='\0';

				}
			}
		}
	}
	free(freeptr);
	return buf;
}

/*transfer contact address into Request-URI */
/*only retrieve the first address and parameters */
RCODE
uaContactAsReqURI(IN SipContact *_contact,IN OUT char *buf, IN OUT int buflen,IN OUT int*length)
{
	int		tmp,i,retvalue;
	SipContactElm	*_contactelm;
	SipParam	*_parameter;
	DxStr		ContactStr;
	char		buffer[512]={'\0'};
  
	ContactStr=dxStrNew();
	_contactelm = _contact->sipContactList;
	/*only retrieve the first contact address*/
	for(i=0; i<1; i++) {
		uaAddrAsStr(_contactelm->address,(unsigned char*) buffer, 512, &tmp);
		dxStrCat(ContactStr,buffer);
		/*didn't retrieve expire time and date*/
		_parameter = _contactelm->ext_attrs;
		while (1) {
			if (_parameter != NULL){
				dxStrCat(ContactStr,";");
				dxStrCat(ContactStr, _parameter->name);
				if (_parameter->value != NULL){
					dxStrCat(ContactStr, "=");/*for equal */
					dxStrCat(ContactStr, _parameter->value);
				}
				_parameter = _parameter->next;
			} else
				break;
		}/*end while loop*/

		/*not add comment*/
	}
	*length=dxStrLen(ContactStr);

	if((*length) > buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(ContactStr));
		retvalue= 1;
	}
	dxStrFree(ContactStr);
	return RC_OK;

} /* ContactAsStr */



void uaAddrFree(SipAddr *addr)
{
	SipAddr *tmpAddr,*nextAddr;

	tmpAddr=addr;
	while(tmpAddr!=NULL){
		if(tmpAddr->display_name != NULL) 
			free(tmpAddr->display_name);
		if(tmpAddr->addr != NULL) {
			if(tmpAddr->with_angle_bracket) 
				free(tmpAddr->addr);
			else free(tmpAddr->addr);
		}
		nextAddr=tmpAddr->next;
		free(tmpAddr);
		tmpAddr=nextAddr;
	}
}

SipAddr* uaAddrDup(SipAddr* addrList,int addrnum)
{
	int idx;
	SipAddr *tmpAddr,*newAddr,*preAddr,*firstAddr;

	tmpAddr=addrList;
	for(idx=0;idx<addrnum;idx++){
		if(tmpAddr==NULL) break;
		newAddr=(SipAddr*)calloc(1,sizeof(SipAddr));
		newAddr->next=NULL;
		(tmpAddr->display_name == NULL)?(newAddr->display_name=NULL):(newAddr->display_name=strDup(tmpAddr->display_name));
		(tmpAddr->addr == NULL)?(newAddr->addr=NULL):(newAddr->addr=strDup(tmpAddr->addr));
		newAddr->with_angle_bracket=tmpAddr->with_angle_bracket;
		(idx==0) ? (firstAddr=newAddr) : (preAddr->next = newAddr);
		preAddr=newAddr;
		tmpAddr=tmpAddr->next;
	}/*end copy Address list*/
	(idx>0) ? (tmpAddr=firstAddr) : (tmpAddr=NULL);

	return tmpAddr;
}

int uaAddrAsStr(SipAddr *_addr, unsigned char *buf, int buflen, int *length)
{
	DxStr AddStr;
	int retvalue;

	AddStr=dxStrNew();
	/* mark by tyhuang 2003.10.6
	if (_addr->display_name != NULL)
		dxStrCat(AddStr, _addr->display_name);
	else
		dxStrCat(AddStr, "");
	*/
	dxStrCat(AddStr, _addr->addr);
	
	*length=dxStrLen(AddStr);
	if((*length) > buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(AddStr));
		retvalue= 1;
	}
	dxStrFree(AddStr);
	return retvalue;
} /* uaAddrAsStr */

int randchar(void)
{
	return (rand()%239+17);
}

SipParam* uaParameterDupAddTag(SipAddr* _addr,SipParam* _param,int numParam,int *num)
{
	int idx,pos;
	BOOL bTag=TRUE;
	SipParam *tmpPara,*newPara,*prePara,*firstPara,*tagPara;

	tmpPara=_param;
	for(idx=0;idx<numParam;idx++){
		if(tmpPara == NULL ) break;
		newPara=(SipParam*)calloc(1,sizeof(SipParam));
		newPara->next=NULL;

		if(strICmp(tmpPara->name,"TAG")==0)
			bTag=FALSE;
		newPara->name=strDup((const char*)tmpPara->name);
		newPara->value=strDup((const char*)tmpPara->value);
		(idx==0) ? (firstPara=newPara) : (prePara->next = newPara);
		prePara=newPara;
		tmpPara=tmpPara->next;
	}
	(idx>0) ? (tmpPara=firstPara) : (tmpPara=NULL);

	if(bTag){ /* Must add tag */
		char tagbuf[128]={"\0"};
		unsigned int ltime;
#ifdef _WIN32	
		/*ltime=clock();*/
		ltime=(unsigned int)uagettickcount();
#else
		ltime=(unsigned int)rand();
#endif
	
		/*if(_addr->addr != NULL)
			base64encode(_addr->addr,tagbuf,1024);
		else	
			base64encode("CCL_SIP_UA_CORE",tagbuf,1024);
	        */
		pos=0;
		
		sprintf(tagbuf+pos,"%x-%04d",randchar(),ltime);

		tagPara=(SipParam*)calloc(1,sizeof(*tagPara));
		tagPara->name=strDup("tag");
		tagPara->value=strDup(tagbuf);
		tagPara->next=NULL;
		idx++;
		if(_param != NULL)
			newPara->next=tagPara;
		else
			tmpPara=tagPara;
	}else{ /*tag had exist */
		
	}
	*num=idx;
	return tmpPara;
}


void uaParameterFree(SipParam* para)
{
	SipParam *tmpPara,*nextPara;

	tmpPara=para;
	while(tmpPara!=NULL){
		if(tmpPara->name != NULL)  free(tmpPara->name);
		if(tmpPara->value!= NULL)  free(tmpPara->value);
		nextPara=tmpPara->next;
		free(tmpPara);
		tmpPara=nextPara;
	}
}

const char* uaGetReqReqURI(SipReq _req)
{
	SipReqLine *pLine=NULL;
	if(_req!=NULL){
		pLine=sipReqGetReqLine(_req);
		if(pLine!=NULL)
			return pLine->ptrRequestURI;
		else 
			return NULL;
	}else
		return NULL;
}

/*Get From and To header tag*/
char*	uaGetReqFromTag(SipReq _req)
{
	char *tag=NULL;
	SipFrom *pFrom=NULL;
	int idx;
	SipParam *pParam=NULL;
	
	if(NULL == _req)
		return NULL;

	pFrom=(SipFrom*)sipReqGetHdr(_req,From);
	if(NULL == pFrom)
		return NULL;

	pParam=pFrom->ParamList;
	for(idx=0;(idx<pFrom->numParam)&&(NULL!=pParam);idx++){
		if(strICmp(pParam->name,"tag")==0)
			return pParam->value;
		else
			pParam=pParam->next;
	}
	return tag;
}

char*	uaGetReqToTag(SipReq _req)
{
	char *tag=NULL;
	SipTo *pTo=NULL;
	int idx;
	SipParam *pParam=NULL;
	
	if(NULL == _req)
		return NULL;

	pTo=(SipTo*)sipReqGetHdr(_req,To);
	if(NULL == pTo)
		return NULL;

	pParam=pTo->paramList;
	for(idx=0;(idx<pTo->numParam)&&(NULL!=pParam);idx++){
		if(strICmp(pParam->name,"tag")==0)
			return pParam->value;
		else
			pParam=pParam->next;
	}
	return tag;
}

/*Get From and To header address*/
char*	uaGetReqFromAddr(SipReq _req)
{
	SipFrom *pFrom=NULL;
	
	if(NULL == _req)
		return NULL;

	pFrom=(SipFrom*)sipReqGetHdr(_req,From);
	if(NULL == pFrom)
		return NULL;
	if(NULL == pFrom->address)
		return NULL;
	if(NULL != pFrom->address->addr)
		return pFrom->address->addr;

	return NULL;
}
char*	uaGetReqToAddr(SipReq _req)
{
	SipTo *pTo=NULL;
	
	if(NULL == _req)
		return NULL;

	pTo=(SipTo*)sipReqGetHdr(_req,To);
	if(NULL == pTo)
		return NULL;
	if(NULL == pTo->address)
		return NULL;
	if(NULL != pTo->address->addr)
		return pTo->address->addr;

	return NULL;
}

/*2003.5.9 add by tyhuang:Get From Displayname */
char*	uaGetReqFromDisplayname(SipReq _req)
{
	SipFrom *pFrom=NULL;
	
	if(NULL == _req)
		return NULL;

	pFrom=(SipFrom*)sipReqGetHdr(_req,From);
	if(NULL == pFrom)
		return NULL;
	if(NULL == pFrom->address)
		return NULL;
	if(NULL != pFrom->address->display_name)
		return pFrom->address->display_name;

	return NULL;
}

/*2003.5.14 add by tyhuang:Get From Displayname */
char*	uaGetReqToDisplayname(SipReq _req)
{
	SipTo *pTo=NULL;
	
	if(NULL == _req)
		return NULL;

	pTo=(SipTo*)sipReqGetHdr(_req,To);
	if(NULL == pTo)
		return NULL;
	if(NULL == pTo->address)
		return NULL;
	if(NULL != pTo->address->display_name)
		return pTo->address->display_name;

	return NULL;
}

char*	uaGetReqContact(SipReq _req)
{
	SipContact *pCont=NULL;
	char *contactbuf=NULL;
	SipURL url;

	if(NULL == _req)
		return NULL;

	pCont=(SipContact*)sipReqGetHdr(_req,Contact);
	if(pCont){
		if(pCont->sipContactList){
			if(pCont->sipContactList->address){
				if(pCont->sipContactList->address->addr){
					contactbuf=pCont->sipContactList->address->addr;
					/* parameter for transport */
				}
				
			}
		}
	}
	/*check address is correct or not */
	url=sipURLNewFromTxt(contactbuf);
	if(url==NULL){
		contactbuf=NULL;
	}else
		sipURLFree(url);
	
	return contactbuf;
}
/*sip Request message processing*/
SipMethodType uaGetReqMethod(SipReq _req)
{
	SipMethodType method=UNKNOWN_METHOD;
	SipReqLine *pLine=NULL;

	if(_req != NULL){
		pLine=sipReqGetReqLine(_req);
		if(pLine != NULL){
			method=pLine->iMethod;
		}
	}
	return method;
}

int uaGetReqCSeq(SipReq _req)
{
	int cseq=-1;
	SipCSeq *pCseq=NULL;

	if(_req!=NULL){
		pCseq=sipReqGetHdr(_req,CSeq);
		if(pCseq!=NULL){
			cseq=pCseq->seq;
		}
	}
	return cseq;
}

/*user get thie SdpSess must free it */
SdpSess uaGetReqSdp(SipReq _req)
{
	SdpSess reqsdp=NULL;
	SipBody *pbody=NULL;

	if(_req != NULL){
		pbody=sipReqGetBody(_req);
		if(pbody!=NULL)
			reqsdp=sdpSessNewFromText((const char*)pbody->content);
	}
	return reqsdp;
}

/*get request message target*/
const char*	uaGetReqCallee(SipReq _req)
{
	SipURL url=NULL;
	SipReqLine *pLine=NULL;
	char* _this; 

	if(_req!=NULL){
		pLine=sipReqGetReqLine(_req);
		if(pLine!= NULL){
			url=sipURLNewFromTxt(pLine->ptrRequestURI);
			if(url==NULL) 
				return NULL;
			/*duplicate callee information*/
			_this = strDup(sipURLGetUser(url)); 
			sipURLFree(url); 
			return _this;
		}else
			return NULL;
	}else
		return NULL;
}

/*check request message, decide response code*/
UAStatusCode uaReqChecking(SipReq _req)
{
	UAStatusCode retCode=sipOK;
	SipReqLine *pLine=NULL;
	SipURL url=NULL;
	SipContType *pContType=NULL;
	SipRequire *pRequire=NULL;
	SipMethodType reqMethod=UNKNOWN_METHOD;
	SipAccept *accept=NULL;
	SipContentLanguage *clang=NULL;
	SipContEncoding *cenc=NULL;
	SipContentDisposition *cdisp=NULL;
	SipBody *pBody=NULL;
	SdpSess pSdp=NULL;
	SipMediaType *media=NULL;
	int acceptflag=0,size,pos,len;
	SipEvent *event=NULL;
	SipReferTo *referto=NULL;
	SipReferredBy *referby=NULL;

	if(_req == NULL)
		return Bad_Request;

	
	/*check request line*/
	pLine=sipReqGetReqLine(_req);
	if(pLine == NULL)
		return Bad_Request;/*400*/
	
	reqMethod=uaGetReqMethod(_req);
	if(reqMethod == UNKNOWN_METHOD)
		return Method_Not_Allowed;/*405*/
	if(strICmp(pLine->ptrSipVersion,SIP_VERSION)!=0)
		return  SIP_Version_not_supported; /*505*/

	if((url=sipURLNewFromTxt(pLine->ptrRequestURI)) == NULL)
		return Unsupported_URI_Scheme;/*416*/
	else{ 
		/*free requets url */
		sipURLFree(url);
		/* check sip: 
		if(strstr(pLine->ptrRequestURI,"sip:")==NULL)
			return Unsupported_URI_Scheme;
		*/
	}
	if(strchr(pLine->ptrRequestURI,'?')!=NULL)
		return Bad_Request;
	/*Check user name*/ 
	/* if not found user, return Not_Found=404*/
	/*this is done when receive the request transaction*/

	/*check Call-ID is NULL*/
	if(sipReqGetHdr(_req,Call_ID)==NULL)
		return Bad_Request; /*400*/
	/*check CSeq is NULL */
	if(sipReqGetHdr(_req,CSeq)==NULL)
		return Bad_Request; /*400*/

	/*check From is NULL */
	if(sipReqGetHdr(_req,From)==NULL)
		return Bad_Request;
	/*check To is NULL */
	if(sipReqGetHdr(_req,To)==NULL)
		return Bad_Request;
	if(sipReqGetHdr(_req,Max_Forwards)==NULL)
		return Bad_Request;
	/*check To is NULL */
	/* Via header is same */
	if(sipReqGetHdr(_req,Via)==NULL)
		return Bad_Request;

	if (reqMethod==INVITE) {
		SipExpires *exp=(SipExpires*)sipReqGetHdr(_req,Expires);
		if (exp!=NULL) 
			if (exp->expireSecond<2) 
				return Request_Terminated;
	}

	/*check content-length modified 2003.9.8*/
	len=*(int*)sipReqGetHdr(_req,Content_Length);
	if(len<0)
		return Bad_Request;
	if (len>0) {
		/*Content-Type, INVITE only accept SDP*/
		pContType=(SipContType*)sipReqGetHdr(_req,Content_Type);
		if((pContType != NULL)&&(reqMethod==INVITE)){
			if((strICmp(pContType->type,"application")!=0)||(strICmp(pContType->subtype,"sdp")!=0)){
					return Unsupported_Media_Type; /*415 */
				}			
		}
		/* content-language 
		clang=(SipContentLanguage*)sipReqGetHdr(_req,Content_Language);
		if(clang){
			if(strICmp(DEFAULT_LANGUAGE,clang->langList->str)!=0)
				return Unsupported_Media_Type;
		}*/
		/* content-encoding */
		cenc=(SipContEncoding*)sipReqGetHdr(_req,Content_Encoding);
		if(cenc){/* exist */
			/* not equal to "" */
			if(strlen(cenc->sipCodingList->coding)>0){
				if(strICmp(DEFAULT_ENCODING,cenc->sipCodingList->coding)!=0)
					if(strICmp("identity",cenc->sipCodingList->coding)!=0)
						return Unsupported_Media_Type;
			}
		}
		/* content-dispoition */
		cdisp=(SipContentDisposition*)sipReqGetHdr(_req,Content_Disposition);
		if(cdisp && cdisp->disptype ){/* exist */
			if(strICmp(cdisp->disptype,"session")==0) ;
			else if (strICmp(cdisp->disptype,"render")==0) ;
			else if(strICmp(cdisp->disptype,"alert")==0) ;
			else
				return Unsupported_Media_Type;
		}
	}
	/* if there is Accept header, check if it exists application/sdp */
	accept=sipReqGetHdr(_req,Accept);
	if(accept&&(reqMethod==INVITE)){
		size=accept->numMediaTypes;
		media=accept->sipMediaTypeList;
		for(pos=0;pos<size;pos++){
			if (!media||!media->type||!media->subtype) 
				break;
			if((strICmp(media->type,"application")==0)&&(strICmp(media->subtype,"sdp")==0))
				acceptflag=1;
			media=media->next;
		}
		if(acceptflag==0)
			return Not_Acceptable_Here; /* 488 */
	}
	/* subscribe/notify check event header.added by tyhuang 2003.05.27 */
	if((reqMethod==SIP_SUBSCRIBE)||(reqMethod==SIP_PUBLISH)||(reqMethod==SIP_NOTIFY)){
		if((event=sipReqGetHdr(_req,Event))==NULL)
			return Bad_Event;
		if(event->pack==NULL)
			return Bad_Event;
	}
	/* Require Header: if didn't understand, reply unsupported */
	if(pLine->iMethod != CANCEL){
		pRequire = (SipRequire*)sipReqGetHdr(_req,Require) ;
		if(pRequire != NULL){
			SipStr *taglist=pRequire->sipOptionTagList;
			size=pRequire->numTags;
			for(pos=0;pos<size;pos++){
				if((strICmp("timer",taglist->str)!=0)&&(strICmp("replaces",taglist->str)!=0)&&(strICmp("100rel",taglist->str)!=0)){
					UaCoreERR("[uaReqChecking] Get non-understand Require Header!\n");
					return Bad_Extension; /* 420 */
				}
				taglist=taglist->next;
			}
		}
	}
	
	/* check refer-to if method = refer */
	if(reqMethod==REFER){
		referto=(SipReferTo*)sipReqGetHdr(_req,Refer_To);
		if(referto==NULL)
			return Bad_Request;
		if(!referto->address || !referto->address->addr)
			return Bad_Request;
		if (referto->address->next)
			return Bad_Request;
		if (referto->numAddr!=1) 
			return Bad_Request;
/*		chech referred-by */
		referby=(SipReferredBy*)sipReqGetHdr(_req,Referred_By);
		if(referby==NULL)
			return Provide_Referrer_Identity;
		if(referby->address==NULL)
			return Provide_Referrer_Identity;

	}
	/*not check media type here*/

	/*check SDP,if SDP Error*/
	if(reqMethod==INVITE){
		pBody=(SipBody*)sipReqGetBody(_req);
		if(pBody==NULL){
			/*accept no SDP INVITE request*/
		}else{
			if(pBody->content == NULL)/*content is NULL */
				return Not_Acceptable_Here; /* 488 */
			else { 
				/*compare length with content-length */
				size=(int)strlen((const char*)pBody->content);
				if(size < len)
					return Bad_Request;
				/*Check it by SDP Parsing */
				pSdp=sdpSessNewFromText((const char*)pBody->content);
				if(pSdp==NULL)
					return Not_Acceptable_Here;/* 488 */
				sdpSessFree(pSdp);
				/*didn't check media type support here*/
			} /*SDP Parsing Correct */
		}
	}
	return retCode;
}


/*create a new SipReqLine, (Method,Req-URI,SIP-Version) */
SipReqLine* uaReqNewReqLine(SipMethodType _method,char* _requri,char* _version)
{
	SipReqLine *pLine=NULL;
	if((_requri != NULL)&&(_version != NULL)){
		pLine=(SipReqLine*)calloc(1,sizeof(*pLine));
		pLine->iMethod=_method;
		pLine->ptrRequestURI=strDup(_requri);
		pLine->ptrSipVersion=strDup(_version);
	}

	return pLine;
}

RCODE uaReqLineFree(SipReqLine *_line)
{
	RCODE rCode=RC_OK;
	if(_line != NULL){
		if(_line->ptrRequestURI != NULL){
			free(_line->ptrRequestURI);
			_line->ptrRequestURI=NULL;
		}
		if(_line->ptrSipVersion != NULL){
			free(_line->ptrSipVersion);
			_line->ptrSipVersion=NULL;
		}
		free(_line);
		_line=NULL;
	}else
		rCode=UASIPMSG_REQLINE_NULL;
	
	return rCode;
}

/*create a new SipRspStatusLine, (SIP-Version,code,reason)*/
/* if reasone is NULL, using  the default */
SipRspStatusLine* uaRspNewStatusLine(const char* _version,UAStatusCode _code,const char* _reason)
{
	SipRspStatusLine *pStatusline=NULL;
	char buf[5]={'\0'};

	if(_version != NULL){
		pStatusline=(SipRspStatusLine*)calloc(1,sizeof(*pStatusline));
		if(_reason == NULL)
			pStatusline->ptrReason=strDup(uaStatusCodeToString(_code));
		else
			pStatusline->ptrReason=strDup(_reason);
		pStatusline->ptrVersion=strDup(_version);
		sprintf(buf,"%d",_code);
		pStatusline->ptrStatusCode=strDup(buf);
		return pStatusline;
	}else
		return NULL;
}

RCODE uaRspStatusLineFree(SipRspStatusLine* _line)
{
	RCODE rCode=RC_OK;

	if(_line!=NULL){
		if(_line->ptrReason!=NULL){
			free(_line->ptrReason);
			_line->ptrReason=NULL;
		}
		if(_line->ptrStatusCode!= NULL){
			free(_line->ptrStatusCode);
			_line->ptrStatusCode=NULL;
		}
		if(_line->ptrVersion != NULL){
			free(_line->ptrVersion);
			_line->ptrVersion=NULL;
		}
		free(_line);
		_line=NULL;
	}else
		rCode=UASIPMSG_RSPLINE_NULL;

	return rCode;
}

/*get REFER resposne*/
SipRsp	uaGetReferRsp(IN UaDlg _dlg)
{
	SipRsp refRsp=NULL;
	TxStruct reftx=NULL;

	reftx=uaDlgGetMethodTx(_dlg,REFER);
	if(NULL != reftx){
		refRsp=sipTxGetLatestRsp(reftx);
	}

	return refRsp;
}


/*create a new CSeq */
SipCSeq* uaNewCSeq(int _cmdseq,SipMethodType _method)
{
	SipCSeq *cseq=NULL;
	cseq=(SipCSeq*)calloc(1,sizeof(SipCSeq));
	if(cseq!=NULL){
		cseq->Method=_method;
		cseq->seq=_cmdseq;
	}
	return cseq;

}

RCODE uaFreeCSeq(SipCSeq* _cseq)
{
	RCODE rCode=RC_OK;
	
	if(_cseq!=NULL)
		free(_cseq);
	else
		rCode=UASIPMSG_CSEQ_NULL;
	
	return rCode;
}


/*sip Response message processing*/
SipMethodType	uaGetRspMethod(SipRsp _rsp)
{
	SipMethodType method=UNKNOWN_METHOD;
	SipCSeq	*pCSeq=NULL;

	if(_rsp != NULL){
		pCSeq=sipRspGetHdr(_rsp,CSeq);
		if(pCSeq != NULL){
			method=pCSeq->Method;
		}
	}
	return method;
}

int uaGetRspStatusCode(SipRsp _rsp)
{
	SipRspStatusLine *pLine=NULL;
	int code=-1;

	if(_rsp!=NULL){
		pLine=sipRspGetStatusLine(_rsp);
		if(pLine != NULL){
			code=a2i(pLine->ptrStatusCode);
		}
	}
	return code;
}

int uaGetRspCSeq(SipRsp _rsp)
{
	int cseq=-1;
	SipCSeq	*pCSeq=NULL;

	if(_rsp != NULL){
		pCSeq=sipRspGetHdr(_rsp,CSeq);
		if(pCSeq != NULL){
			cseq=pCSeq->seq;
		}
	}
	return cseq;
}

SdpSess	 uaGetRspSdp(SipRsp _rsp)
{
	SdpSess rspsdp=NULL;
	SipBody *pbody=NULL;

	if(_rsp != NULL){
		pbody=sipRspGetBody(_rsp);
		if(pbody!=NULL)
			rspsdp=sdpSessNewFromText((const char*)pbody->content);
	}
	return rspsdp;

}

/*Get From and To header tag*/
char*	uaGetRspFromTag(SipRsp _rsp)
{
	char *tag=NULL;
	SipFrom *pFrom=NULL;
	int idx;
	SipParam *pParam=NULL;
	
	if(NULL == _rsp)
		return NULL;

	pFrom=(SipFrom*)sipRspGetHdr(_rsp,From);
	if(NULL == pFrom)
		return NULL;

	pParam=pFrom->ParamList;
	for(idx=0;(idx<pFrom->numParam)&&(NULL!=pParam);idx++){
		if(strICmp(pParam->name,"tag")==0)
			return pParam->value;
		else
			pParam=pParam->next;
	}
	return tag;
}

char*	uaGetRspToTag(SipRsp _rsp)
{
	char *tag=NULL;
	SipTo *pTo=NULL;
	int idx;
	SipParam *pParam=NULL;
	
	if(NULL == _rsp)
		return NULL;

	pTo=(SipTo*)sipRspGetHdr(_rsp,To);
	if(NULL == pTo)
		return NULL;

	pParam=pTo->paramList;
	for(idx=0;(idx<pTo->numParam)&&(NULL!=pParam);idx++){
		if(strICmp(pParam->name,"tag")==0)
			return pParam->value;
		else
			pParam=pParam->next;
	}
	return tag;
}


/*message body, SipBody can be NULL or not*/
RCODE uaReqAddBody(SipReq _req,SipBody *_body,char* _conttype)
{
	RCODE rCode=UASIPMSG_ERROR;
	char buf[128]={'\0'};

	if(_req !=NULL){
		if(_body != NULL){
			sprintf(buf,"Content-Type=%s",_conttype);
			rCode=sipReqAddHdrFromTxt(_req,Content_Type,buf);
			rCode=sipReqAddHdr(_req,Content_Length,&(_body->length));
			rCode=sipReqSetBody(_req,_body);

		}else{
			int len=0;
			rCode=sipReqAddHdr(_req,Content_Length,&len);
		}
	}else
		rCode=UASIPMSG_REQ_NULL;

	return rCode;
}
RCODE uaRspAddBody(SipRsp _rsp,SipBody *_body,char* _conttype)
{
	RCODE rCode=UASIPMSG_ERROR;
	char buf[128]={'\0'};
	
	if(_rsp != NULL){
		if(_body!=NULL){
			sprintf(buf,"Content-Type : %s",_conttype);
			rCode=sipRspAddHdrFromTxt(_rsp,Content_Type,buf);
			rCode=sipRspAddHdr(_rsp,Content_Length,&(_body->length));
			rCode=sipRspSetBody(_rsp,_body);
		}else{
			rCode=sipRspAddHdr(_rsp,Content_Length,0);
		}
	}else
		rCode=UASIPMSG_RSP_NULL;
	
	return rCode;
}


/*sip Body create*/
SipBody* uaCreatSipBody(IN SdpSess _sdp)
{
	SipBody *body=NULL;
	char sdpbuf[1024]={'\0'};
	unsigned int bodylen=1024;

	if(_sdp!=NULL){
		body=(SipBody*)calloc(1,sizeof(SipBody));
		if(RC_ERROR == (sdpSess2Str(_sdp,sdpbuf,&bodylen))){
			free(body);
			return NULL;
		}
		body->content=(unsigned char*)strDup(sdpbuf);
		body->length=bodylen;
	}
	return body;
}

SipBody* uaCreatSipBodyFromContent(IN UaContent _content)
{
	SipBody *body=NULL;

	if(_content != NULL){
		body=(SipBody*)calloc(1,sizeof(SipBody));
		body->content=NULL;
		body->length=uaContentGetLength(_content);
		if(body->length > 0)
			body->content=strDup(uaContentGetBody(_content));
	}
	return body;
}

void	uaFreeSipBody(IN SipBody* _body)
{
	if(_body!=NULL){
		if(_body->content!=NULL)
			free(_body->content);
		free(_body);
	}
}

RCODE	uaRspGetDigestAuthn(SipRsp _rsp,UAAuthnType _type,SipDigestWWWAuthn* _auth)
{			    
	RCODE ret;
	SipAuthnStruct tmpAuth;

	tmpAuth.iType=SIP_DIGEST_CHALLENGE;
	switch(_type){
	case WWW_AUTHN:
		ret=sipRspGetWWWAuthn(_rsp,&tmpAuth);
		break;
	case PROXY_AUTHN:
		ret=sipRspGetPxyAuthn(_rsp,&tmpAuth);
		break;
	}
	_auth->stale_=FALSE;
	if(ret != -1){
		_auth->flags_=tmpAuth.auth_.digest_.flags_;

		if(_auth->flags_ & SIP_DWAFLAG_ALGORITHM)
			strcpy(_auth->algorithm_,tmpAuth.auth_.digest_.algorithm_);
 
		if(_auth->flags_ & SIP_DWAFLAG_DOMAIN)
			strcpy(_auth->domain_,tmpAuth.auth_.digest_.domain_);
		
		if(_auth->flags_ & SIP_DWAFLAG_NONCE)
			strcpy(_auth->nonce_,tmpAuth.auth_.digest_.nonce_);
		if(_auth->flags_ & SIP_DWAFLAG_OPAQUE)
			strcpy(_auth->opaque_,tmpAuth.auth_.digest_.opaque_);
		if(_auth->flags_ & SIP_DWAFLAG_QOP)
			strcpy(_auth->qop_,tmpAuth.auth_.digest_.qop_);
		strcpy(_auth->realm_,tmpAuth.auth_.digest_.realm_);
		if(_auth->flags_ & SIP_DWAFLAG_STALE)
			_auth->stale_=tmpAuth.auth_.digest_.stale_;
	}
	return ret;
}


RCODE	uaGetDigestAuthz(SipAuthz *authz,SipDigestAuthz* auth)
{			    
	int		i = 0;
	SipParam	*piter = NULL;
	int		numParm;


	piter =authz->sipParamList;
	numParm = authz->numParams;

	if(piter == NULL)
		return RC_ERROR;

	for( i=0; (i<numParm)&&piter; i++, piter=piter->next) {
		if( strcmp(piter->name,"username")==0 ) {
			strcpy(auth->username_,unquote(piter->value));
			continue;
		}
		if( strcmp(piter->name,"realm")==0 ) {
			strcpy(auth->realm_,unquote(piter->value));
			continue;
		}
		if( strcmp(piter->name,"nonce")==0 ) {
			strcpy(auth->nonce_,unquote(piter->value));
			continue;
		}

		/*if( strcmp(piter->name,"digest-uri")==0 ) {*/
		if( strcmp(piter->name,"uri")==0 ) {
			strcpy(auth->digest_uri_,unquote(piter->value));
			continue;
		}
		if( strcmp(piter->name,"response")==0 ) {
			strcpy(auth->response_,unquote(piter->value));
			continue;
		}
		if( strcmp(piter->name,"algorithm")==0 ) {
			strcpy(auth->algorithm_,unquote(piter->value));
			auth->flag_ |= SIP_DAFLAG_ALGORITHM;
			continue;
		}
		if( strcmp(piter->name,"cnonce")==0 ) {
			strcpy(auth->cnonce_,unquote(piter->value));
			auth->flag_ |= SIP_DAFLAG_CNONCE;
			continue;
		}
		if( strcmp(piter->name,"opaque")==0 ) {
			strcpy(auth->opaque_,unquote(piter->value));
			auth->flag_ |= SIP_DAFLAG_OPAQUE;
			continue;
		}
		if( strcmp(piter->name,"qop")==0 ) {
			strcpy(auth->message_qop_,unquote(piter->value));
			auth->flag_ |= SIP_DAFLAG_MESSAGE_QOP;
			continue;
		}
		if( strcmp(piter->name,"nc")==0 ) {
			strcpy(auth->nonce_count_,unquote(piter->value));
			auth->flag_ |= SIP_DAFLAG_NONCE_COUNT;
			continue;
		}
	}
	return RC_OK;
}

RCODE uaReqAddDigestAuthz(SipReq _req,UAAuthzType _type,SipDigestAuthz _auth)
{
	RCODE ret;
	SipAuthzStruct tmpAuth;

	tmpAuth.iType=SIP_DIGEST_CHALLENGE;
	strcpy(tmpAuth.auth_.digest_.algorithm_,_auth.algorithm_);
	strcpy(tmpAuth.auth_.digest_.cnonce_,_auth.cnonce_);
	strcpy(tmpAuth.auth_.digest_.digest_uri_,_auth.digest_uri_);
	tmpAuth.auth_.digest_.flag_=_auth.flag_;
	strcpy(tmpAuth.auth_.digest_.message_qop_,_auth.message_qop_);
	strcpy(tmpAuth.auth_.digest_.nonce_,_auth.nonce_);
	strcpy(tmpAuth.auth_.digest_.nonce_count_,_auth.nonce_count_);
	strcpy(tmpAuth.auth_.digest_.opaque_,_auth.opaque_);
	strcpy(tmpAuth.auth_.digest_.realm_,_auth.realm_);
	strcpy(tmpAuth.auth_.digest_.response_,_auth.response_);
	strcpy(tmpAuth.auth_.digest_.username_,_auth.username_);
	switch(_type){
	case AUTHZ:
		ret=sipReqAddAuthz(_req,tmpAuth);
		break;
	case PROXY_AUTHZ:
		ret=sipReqAddPxyAuthz(_req,tmpAuth);
		break;
	}
	return ret;
}

/*add by tyhuang . to get sip message content from request*/
UaContent	uaGetReqContent(IN SipReq _req)
{
	UaContent _content=NULL;
	SipBody	*pbody=NULL;
	SipContType *type=NULL;
	char fulltype[50]={'\0'};
	int pos=0;

	if(_req != NULL){
		pbody=sipReqGetBody(_req);
		type=sipReqGetHdr(_req,Content_Type);
		if(type == NULL)
			return NULL;
		if(type->type==NULL)
			return NULL;
		pos=sprintf(fulltype,"%s",type->type);
		if(type->subtype)
			sprintf(fulltype+pos,"/%s",type->subtype);

		if(pbody)
			_content=uaContentNew(fulltype,pbody->length,pbody->content);
	}
	return _content;
}

/*add by tyhuang . to get sip message content from response*/
UaContent	uaGetRspContent(IN SipRsp _rsp)
{
	UaContent _content=NULL;
	SipBody	*pbody=NULL;
	SipContType *type=NULL;
	char fulltype[50]={'\0'};
	int pos=0;

	if(_rsp != NULL){
		pbody=sipRspGetBody(_rsp);
		type=sipRspGetHdr(_rsp,Content_Type);
		if(type == NULL)
			return NULL;
		if(type->type==NULL)
			return NULL;
		pos=sprintf(fulltype,"%s",type->type);
		if(type->subtype)
			sprintf(fulltype+pos,"/%s",type->subtype);

		if(pbody){
			if(strlen(pbody->content) <= (size_t)pbody->length)
				_content=uaContentNew(fulltype,strlen(pbody->content),pbody->content);
			else
				_content=uaContentNew(fulltype,pbody->length,pbody->content);
		}
	}
	return _content;
}

/*sdp checking */
BOOL	uaCheckHold(IN SdpSess _sdp)
{
	BOOL bHold=FALSE;
	RCODE rCode;
	char conntip[20]={'\0'};
	CodecLst clist=NULL;
	UaCodec rCodec=NULL;
	int count=0,i,size;
	SdpTa ta;
	int iMedia;
	SdpMsess Media;
	SdpTm  MediaAnn;

	/*get connection ip address */
	rCode=uaSDPGetDestAddress(_sdp,conntip,20);
	/*based on RFC2543*/
	if(strICmp("0.0.0.0",conntip)==0)
		bHold= TRUE;

	/*based on 3261 should check media attribute = sendonly */
	/* check session part first */
	size=sdpSessGetTaSize(_sdp);
	for(i=0;i<size;i++){
		sdpSessGetTaAt(_sdp, i, &ta);
		if(strICmp(ta.pAttrib_,"sendonly")==0){
			return TRUE;
		}
		if(strICmp(ta.pAttrib_,"inactive")==0){
			return TRUE;
		}
	}
	/* check audio 
	clist=uaSDPGetCodecList(_sdp,UA_MEDIA_AUDIO);
	if(clist){
		count=uaCodecLstGetCount(clist);
		for(i=0;i<count;i++){
			rCodec=uaCodecLstGetAt(clist,i);
			if((uaCodecGetAttr(rCodec)==UAMEDIA_SENDONLY)||(uaCodecGetAttr(rCodec)==UAMEDIA_INACTIVE)){
				bHold= TRUE;
				break;
			}
		}
		uaCodecLstFree(clist);
	}
	*/

	iMedia=sdpSessGetMsessSize(_sdp);
	for(i=0;i<iMedia;i++){
		Media=sdpSessGetMsessAt(_sdp,i);
		sdpMsessGetTm(Media,&MediaAnn);
		if(MediaAnn.Tm_type_ == SDP_TM_MEDIA_TYPE_AUDIO){
			size=sdpMsessGetTaSize(Media);
			for(i=0;i<size;i++){
				sdpMsessGetTaAt(Media,i,&ta);
				if(strICmp(ta.pAttrib_,"sendonly")==0)
					return TRUE;
				
				if(strICmp(ta.pAttrib_,"inactive")==0)
					return TRUE;
			}
		}/* end of if AUDIO */
		
	}
	
	return bHold;
}

/*
char* GetSipfragFromRsp(IN SipRsp rsp)
{
	static char buf[256];
	SipRspStatusLine *sline=NULL;

	memset(buf,0,sizeof(char));
	if(rsp){
		sline=sipRspGetStatusLine(rsp);
		if(sline){
			sprintf(buf,"%s %s %s",sline->ptrVersion,sline->ptrStatusCode,sline->ptrReason);
		}else
			TCRPrint(TRACE_LEVEL_API,"status line is NULL!\n");
	}else
		TCRPrint(TRACE_LEVEL_API,"response is NULL!\n");
	return buf;
}
*/
RCODE GetSipfragFromRsp(IN SipRsp rsp,OUT char* pbuf,IN int length)
{
	char buf[256];
	SipRspStatusLine *sline=NULL;

	if (!rsp || !pbuf ||(length<1)) {
		TCRPrint(TRACE_LEVEL_API,"[GetSipfragFromRsp]response is NULL!\n");
		return RC_ERROR;
	}
	pbuf[0]='\0';
	memset(buf,0,sizeof(buf));

	sline=sipRspGetStatusLine(rsp);
	if(sline){
		sprintf(buf,"%s %s %s",sline->ptrVersion,sline->ptrStatusCode,sline->ptrReason);
	}else
		TCRPrint(TRACE_LEVEL_API,"[GetSipfragFromRsp]status line is NULL!\n");

	if (strlen(buf)>(unsigned int)length) {
		TCRPrint(TRACE_LEVEL_API,"[GetSipfragFromRsp]input buffer is too small!\n");
		return RC_ERROR;
	}
	strcpy(pbuf,buf);
	return RC_OK;
}

BOOL  BeSIPSURL(IN const char*url)
{
	char *semi,*buf;
	
	if(!url)
		return FALSE;
	buf=strDup(url);
	semi=strchr(buf,':');

	if(!semi)
		return FALSE;
	*semi='\0';
	if(strICmp(buf,"sips")==0){
		free(buf);
		return TRUE;
	}
	free(buf);
	return FALSE;
}

char* ReqGetViaBranch(SipReq req)
{
	SipVia *via;
	SipViaParm *tmpViaPara;

	if(!req)
		return NULL;
	
	via=(SipVia*)sipReqGetHdr(req,Via);
	if(!via)
		return NULL;
	
	tmpViaPara=via->ParmList;
	if(!tmpViaPara)
		return NULL;

	return tmpViaPara->branch;
}

unsigned char* ReqGetSupported(SipReq req)
{
	static unsigned char supstr[256]={'\0'};
	SipSupported *s=NULL;
	SipStr *_str;
	int i;


	if(!req)
		return NULL;

	s=(SipSupported *)sipReqGetHdr(req,Supported);
	if(!s)
		return NULL;

	_str=s->sipOptionTagList;
	if(!_str)
		return NULL;

	memset(supstr,0,sizeof(supstr));
	for(i=0; i<s->numTags; i++){
		if(!_str)
			break;

		if(_str->str){
			strcat(supstr,_str->str);
			if (_str->next != NULL)
				strcat(supstr,",");/*for comma*/
		}
		_str = _str->next;
	}

	return supstr;
}

unsigned char* RspGetSupported(SipRsp rsp)
{
	static unsigned char suppstr[256]={'\0'};
	SipSupported *s=NULL;
	SipStr *_str;
	int i;


	if(!rsp)
		return NULL;

	s=(SipSupported *)sipRspGetHdr(rsp,Supported);
	if(!s)
		return NULL;

	_str=s->sipOptionTagList;
	if(!_str)
		return NULL;

	memset(suppstr,0,sizeof(suppstr));
	for(i=0; i<s->numTags; i++){
		if(!_str)
			break;

		if(_str->str){
			strcat(suppstr,_str->str);
			if (_str->next != NULL)
				strcat(suppstr,",");/*for comma*/
		}
		_str = _str->next;
	}

	return suppstr;
}

