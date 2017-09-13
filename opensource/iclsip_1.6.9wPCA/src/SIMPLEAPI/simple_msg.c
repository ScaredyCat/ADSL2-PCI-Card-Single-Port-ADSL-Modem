/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * simple_msg.c
 *
 * $Id: simple_msg.c,v 1.58 2006/03/31 08:33:39 tyhuang Exp $
 */
#include <stdio.h>
#include <uacore/ua_sipmsg.h>
#include <uacore/ua_mgr.h>
#include <uacore/ua_dlg.h>
#include <uacore/ua_int.h>
#include <siptx/siptx.h>
#include <adt/dx_str.h>
#include <uacore/ua_evtpkg.h>
#include <uacore/ua_user.h>
#include "simple_msg.h"


//add by alan for group subscribe 
SipReq uaSUBSCRIBEMsgWithEventList(IN UaDlg _dlg, IN const char* _dest , IN const char* eventname ,IN int expires ,IN UaContent content)
{
	SipReq subscribeMsg=NULL;
	SipReq origReq=NULL;
	SipRsp finalRsp=NULL;
	TxStruct tx=NULL;
	SipRecordRoute *pRecordRoute=NULL;
	SipRoute *pRoute=NULL;
	SipReqLine *pLine=NULL,*pNewLine=NULL;
	SipCSeq *cseq;
	SipFrom *pFrom=NULL;
	char *pToTag=NULL,*pFromTag=NULL;
	RCODE rCode;
	SipContact *pContact=NULL;
	SipURL url=NULL;
	BOOL bContactOK=FALSE;
	UaMgr  mgr;
	UaUser user;
	UaCfg  cfg;
	SipBody	*tmpBody=NULL;
	char buf[1024]={'\0'};
	char ip[MAXADDRESIZE]={'\0'},*username,*laddr,*hostname,*authz;
	int maxfor,bodylen;
	TXTYPE txType;
	char uribuf[128]={'\0'};


	/*check input parameter is NULL or not*/
	if((_dlg == NULL)||(_dest == NULL)){
		TCRPrint(1,"*** [uaSUBSCRIBEMsg] dialog or destination is NULL!\n");
		return NULL;
	}
	/*get parent mgr, if NULL : Error */
	mgr=uaDlgGetMgr(_dlg);
	if(mgr==NULL){
		TCRPrint(1,"*** [uaSUBSCRIBEMsg] mgr is NULL!\n");
		return NULL;
	}
	/*get user object, get user information*/
	user=uaMgrGetUser(mgr);
	if(user == NULL){
		TCRPrint(1,"*** [uaSUBSCRIBEMsg] user is NULL!\n");
		return NULL;
	}
	/*get cfg object, get configuration information*/
	cfg=uaMgrGetCfg(mgr);
	if(cfg==NULL){
		TCRPrint(1,"*** [uaSUBSCRIBEMsg] cfg is NULL!\n");
		return NULL;
	}
	/*create new empty request message*/
	subscribeMsg=sipReqNew();
	if(subscribeMsg == NULL){
		TCRPrint(1,"*** [uaSUBSCRIBEMsg] To create new Request is failed!\n");
		return NULL;
	}
	tx=uaDlgGetInviteTx(_dlg,FALSE);
	if(tx==NULL)
		tx=uaDlgGetInviteTx(_dlg,TRUE);
	if(tx != NULL){
		
		origReq=sipTxGetOriginalReq(tx);
		finalRsp=sipTxGetLatestRsp(tx);
		
		txType=sipTxGetType(tx);
		/*Client transaction*/
		if(((TX_CLIENT_NON_INVITE == txType)||(TX_CLIENT_INVITE == txType)) && finalRsp){
			if(origReq == NULL){
				sipReqFree(subscribeMsg);
				TCRPrint(1,"*** [uaSUBSCRIBEMsg] original request is NULL!\n");
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
				/*uaContactAsReqURI(pContact,uribuf,256,&urilen); 2003.8.22-tyhuang*/
				sprintf(uribuf,"%s",uaDlgGetRemoteTarget(_dlg));
				pNewLine=uaReqNewReqLine(SIP_SUBSCRIBE,uribuf,pLine->ptrSipVersion);
			}else
				pNewLine=uaReqNewReqLine(SIP_SUBSCRIBE,pLine->ptrRequestURI,pLine->ptrSipVersion);
			rCode=sipReqSetReqLine(subscribeMsg,pNewLine);

			if(rCode != RC_OK){
				if(pNewLine)
					uaReqLineFree(pNewLine);
				sipReqFree(subscribeMsg);
				TCRPrint(1,"*** [uaSUBSCRIBEMsg] To set request line is NULL!\n");
				return NULL;
			}
			
		
			/*Add Route header if exist Reocord-Route header*/
			pRecordRoute=(SipRecordRoute*)sipRspGetHdr(finalRsp,Record_Route);
			if(pRecordRoute != NULL){
				pRoute=uaRecordRouteCovertToRoute(pRecordRoute);
				if(pRoute != NULL){
					sipReqAddHdr(subscribeMsg,Route,pRoute);
					uaRouteFree(pRoute);
				}
			}
		}else if((TX_SERVER_NON_INVITE==txType)||(TX_SERVER_INVITE==txType)){
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
				pNewLine=uaReqNewReqLine(SIP_SUBSCRIBE,uribuf,SIP_VERSION);
				sipURLFree(url);
			}else{ /*copy from from header*/
				pFrom=sipRspGetHdr(finalRsp,From);
				if(pFrom != NULL)
					pNewLine=uaReqNewReqLine(SIP_SUBSCRIBE,pFrom->address->addr,SIP_VERSION);
				else{
					sipReqFree(subscribeMsg);
					TCRPrint(1,"*** [uaSUBSCRIBEMsg] From header gotten is NULL!\n");
					return NULL;
				}
			}
			rCode=sipReqSetReqLine(subscribeMsg,pNewLine);
					
			/*Add Route header if exist Reocord-Route header*/
			pRecordRoute=(SipRecordRoute*)sipReqGetHdr(origReq,Record_Route);
			if(pRecordRoute != NULL){
				pRoute=uaRecordRouteCovertToRoute(pRecordRoute);
				if(pRoute != NULL){
					sipReqAddHdr(subscribeMsg,Route,pRoute);
					uaRouteFree(pRoute);
				}
			}
		}
	}
	if(pNewLine==NULL){
		/*request line*/
		pNewLine=uaReqNewReqLine(SIP_SUBSCRIBE,(char*)_dest,SIP_VERSION);
		if(NULL != pNewLine){
			rCode=sipReqSetReqLine(subscribeMsg,pNewLine);
			if(RC_OK != rCode){
				uaReqLineFree(pNewLine);
				sipReqFree(subscribeMsg);
			}
			if(NULL != pNewLine)
				uaReqLineFree(pNewLine);
		}else{
			sipReqFree(subscribeMsg);
			TCRPrint(1,"*** [uaSUBSCRIBEMsg] To create new request line is failed!\n");
			return NULL;
		}
	}else
		uaReqLineFree(pNewLine);

	username=(char*)uaUserGetName(user);
	laddr=(char*)uaUserGetLocalAddr(user);
	hostname=(char*)uaUserGetLocalHost(user);
	if((NULL==username)||(NULL==laddr)||(NULL==hostname)){
		sipReqFree(subscribeMsg);
		TCRPrint(1,"*** [uaSUBSCRIBEMsg] username, localaddr or hostname is NULL!\n");
		return NULL;
	}

	/*copy local ip address into ip buffer*/
	strcpy(ip,laddr);

	/*Call-ID header*/
	if(uaDlgGetCallID(_dlg))
		sprintf(buf,"Call-ID:%s\r\n",uaDlgGetCallID(_dlg));
	else
		sprintf(buf,"Call-ID:%s\r\n",sipCallIDGen(hostname,ip));

	rCode=sipReqAddHdrFromTxt(subscribeMsg,Call_ID,buf);
	/* Accept header */
	sprintf(buf,"Accept:application/cpim-pidf+xml\r\n");
	rCode=sipReqAddHdrFromTxt(subscribeMsg,Accept,buf);

	/* Accept header */
	sprintf(buf,"Accept:application/rmli+xml\r\n");
	rCode=sipReqAddHdrFromTxt(subscribeMsg,UnknSipHdr,buf);  

	/* Accept header */
	sprintf(buf,"Accept:multipart/related\r\n");
	rCode=sipReqAddHdrFromTxt(subscribeMsg,UnknSipHdr,buf);
	
	/*Event header*/
	sprintf(buf,"Event:%s\r\n",eventname);
	rCode=sipReqAddHdrFromTxt(subscribeMsg,Event,buf);

	/*To header*/
	pToTag=uaDlgGetRemoveTag(_dlg);
	if(pToTag != NULL)
		sprintf(buf,"To:<%s>;tag=%s\r\n",_dest,pToTag);
	else
		sprintf(buf,"To:<%s>\r\n",_dest);

	rCode=sipReqAddHdrFromTxt(subscribeMsg,To,buf);

	/*From header and Contact header. modified by tyhuang*/	
	pFromTag=uaDlgGetLocalTag(_dlg);
	sprintf(buf,"From:<%s>;tag=%s\r\n",uaMgrGetPublicAddr(mgr),pFromTag);
	rCode=sipReqAddHdrFromTxt(subscribeMsg,From,buf);

	/*Contact header*/
	sprintf(buf,"Contact:%s\r\n",uaMgrGetContactAddr(mgr));
	rCode=sipReqAddHdrFromTxt(subscribeMsg,Contact,buf);

	/*CSeq header*/
	uaMgrAddCmdSeq(mgr);
	cseq=uaNewCSeq(uaMgrGetCmdSeq(mgr),SIP_SUBSCRIBE);
	sipReqAddHdr(subscribeMsg,CSeq,cseq);
	uaFreeCSeq(cseq);

	/*Max-Forward*/
	maxfor=uaUserGetMaxForwards(user);
	if(maxfor<0) maxfor=70;
	/*sprintf(buf,"Max-Forwards:70\r\n");*/
	sprintf(buf,"Max-Forwards:%d\r\n",maxfor);
	rCode=sipReqAddHdrFromTxt(subscribeMsg,Max_Forwards,buf);

	/* expires .default to 600 */
	if(expires >= 0)
		sprintf(buf,"Expires:%d\r\n",expires);
	else
		sprintf(buf,"Expires:3600\r\n");

	rCode=sipReqAddHdrFromTxt(subscribeMsg,Expires,buf);
	
	bodylen=uaContentGetLength(content);
	if(bodylen>0){
		sprintf(buf,"Content-Length:%d\r\n",strlen(uaContentGetBody(content)));
		rCode=sipReqAddHdrFromTxt(subscribeMsg,Content_Length,buf);
		sprintf(buf,"Content-Type:%s\r\n",uaContentGetType(content));
		rCode=sipReqAddHdrFromTxt(subscribeMsg,Content_Type,buf);
	}else{
		sprintf(buf,"Content-Length:0\r\n");
		rCode=sipReqAddHdrFromTxt(subscribeMsg,Content_Length,buf);
	}
	/* User-Agent header */
	sprintf(buf,"User-Agent:%s\r\n",USER_AGENT);
	rCode=sipReqAddHdrFromTxt(subscribeMsg,User_Agent,buf);

	/* authz header */
	authz=uaUserGetAuthData(user);
	if(authz){
		sprintf(buf,"%s",authz);
		rCode=sipReqAddHdrFromTxt(subscribeMsg,Authorization,buf);
	}

	/* Supported eventlist header */
	sprintf(buf,"Supported:eventlist\r\n");
	rCode=sipReqAddHdrFromTxt(subscribeMsg,Supported,buf);

	
	/* add content to sip message body */
	if(subscribeMsg&&uaContentGetBody(content)){
		tmpBody=(SipBody*)calloc(1,sizeof(SipBody));
		tmpBody->length=bodylen;
		tmpBody->content=uaContentGetBody(content);
		rCode=sipReqSetBody(subscribeMsg,tmpBody);
		if(rCode != RC_OK){
			sipReqFree(subscribeMsg);
			subscribeMsg=NULL;
		}
	}
	if(tmpBody != NULL)
		free(tmpBody);
	TCRPrint(TRACE_LEVEL_API,"*** [uaSUBSCRIBEMsg]create a SUBSCRIBE message\n%s!\n",sipReqPrint(subscribeMsg));
	return subscribeMsg;
}


/** Function to create a Subscribe message
 *  \param _dlg a dialog will be used to send the message
 *  \param _dest the destination url 
 *  \param _eventname type of event for this subscribe message
 *  \retval SipReq object of subcribe method or NULL
 */
SipReq uaSUBSCRIBEMsg(IN UaDlg _dlg, IN const char* _dest , IN const char* eventname ,IN int expires ,IN UaContent content)
{
	SipReq subscribeMsg=NULL;
	SipReq origReq=NULL;
	SipRsp finalRsp=NULL;
	TxStruct tx=NULL;
	SipRecordRoute *pRecordRoute=NULL;
	SipRoute *pRoute=NULL;
	SipReqLine *pLine=NULL,*pNewLine=NULL;
	SipCSeq *cseq;
	SipFrom *pFrom=NULL;
	char *pToTag=NULL,*pFromTag=NULL;
	RCODE rCode;
	SipContact *pContact=NULL;
	SipURL url=NULL;
	BOOL bContactOK=FALSE;
	UaMgr  mgr;
	UaUser user;
	UaCfg  cfg;
	SipBody	*tmpBody=NULL;
	char buf[1024]={'\0'};
	char ip[MAXADDRESIZE]={'\0'},*username,*laddr,*hostname,*authz;
	int maxfor,bodylen;
	TXTYPE txType;
	char uribuf[128]={'\0'};


	/*check input parameter is NULL or not*/
	if((_dlg == NULL)||(_dest == NULL)){
		TCRPrint(1,"*** [uaSUBSCRIBEMsg] dialog or destination is NULL!\n");
		return NULL;
	}
	/*get parent mgr, if NULL : Error */
	mgr=uaDlgGetMgr(_dlg);
	if(mgr==NULL){
		TCRPrint(1,"*** [uaSUBSCRIBEMsg] mgr is NULL!\n");
		return NULL;
	}
	/*get user object, get user information*/
	user=uaMgrGetUser(mgr);
	if(user == NULL){
		TCRPrint(1,"*** [uaSUBSCRIBEMsg] user is NULL!\n");
		return NULL;
	}
	/*get cfg object, get configuration information*/
	cfg=uaMgrGetCfg(mgr);
	if(cfg==NULL){
		TCRPrint(1,"*** [uaSUBSCRIBEMsg] cfg is NULL!\n");
		return NULL;
	}
	/*create new empty request message*/
	subscribeMsg=sipReqNew();
	if(subscribeMsg == NULL){
		TCRPrint(1,"*** [uaSUBSCRIBEMsg] To create new Request is failed!\n");
		return NULL;
	}
	tx=uaDlgGetInviteTx(_dlg,FALSE);
	if(tx==NULL)
		tx=uaDlgGetInviteTx(_dlg,TRUE);
	if(tx != NULL){
		
		origReq=sipTxGetOriginalReq(tx);
		finalRsp=sipTxGetLatestRsp(tx);
		
		txType=sipTxGetType(tx);
		/*Client transaction*/
		if(((TX_CLIENT_NON_INVITE == txType)||(TX_CLIENT_INVITE == txType)) && finalRsp){
			if(origReq == NULL){
				sipReqFree(subscribeMsg);
				TCRPrint(1,"*** [uaSUBSCRIBEMsg] original request is NULL!\n");
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
				/*uaContactAsReqURI(pContact,uribuf,256,&urilen); 2003.8.22-tyhuang*/
				sprintf(uribuf,"%s",uaDlgGetRemoteTarget(_dlg));
				pNewLine=uaReqNewReqLine(SIP_SUBSCRIBE,uribuf,pLine->ptrSipVersion);
			}else
				pNewLine=uaReqNewReqLine(SIP_SUBSCRIBE,pLine->ptrRequestURI,pLine->ptrSipVersion);
			rCode=sipReqSetReqLine(subscribeMsg,pNewLine);

			if(rCode != RC_OK){
				if(pNewLine)
					uaReqLineFree(pNewLine);
				sipReqFree(subscribeMsg);
				TCRPrint(1,"*** [uaSUBSCRIBEMsg] To set request line is NULL!\n");
				return NULL;
			}
			
		
			/*Add Route header if exist Reocord-Route header*/
			pRecordRoute=(SipRecordRoute*)sipRspGetHdr(finalRsp,Record_Route);
			if(pRecordRoute != NULL){
				pRoute=uaRecordRouteCovertToRoute(pRecordRoute);
				if(pRoute != NULL){
					sipReqAddHdr(subscribeMsg,Route,pRoute);
					uaRouteFree(pRoute);
				}
			}
		}else if((TX_SERVER_NON_INVITE==txType)||(TX_SERVER_INVITE==txType)){
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
				pNewLine=uaReqNewReqLine(SIP_SUBSCRIBE,uribuf,SIP_VERSION);
				sipURLFree(url);
			}else{ /*copy from from header*/
				pFrom=sipRspGetHdr(finalRsp,From);
				if(pFrom != NULL)
					pNewLine=uaReqNewReqLine(SIP_SUBSCRIBE,pFrom->address->addr,SIP_VERSION);
				else{
					sipReqFree(subscribeMsg);
					TCRPrint(1,"*** [uaSUBSCRIBEMsg] From header gotten is NULL!\n");
					return NULL;
				}
			}
			rCode=sipReqSetReqLine(subscribeMsg,pNewLine);
					
			/*Add Route header if exist Reocord-Route header*/
			pRecordRoute=(SipRecordRoute*)sipReqGetHdr(origReq,Record_Route);
			if(pRecordRoute != NULL){
				pRoute=uaRecordRouteCovertToRoute(pRecordRoute);
				if(pRoute != NULL){
					sipReqAddHdr(subscribeMsg,Route,pRoute);
					uaRouteFree(pRoute);
				}
			}
		}
	}
	if(pNewLine==NULL){
		/*request line*/
		pNewLine=uaReqNewReqLine(SIP_SUBSCRIBE,(char*)_dest,SIP_VERSION);
		if(NULL != pNewLine){
			rCode=sipReqSetReqLine(subscribeMsg,pNewLine);
			if(RC_OK != rCode){
				uaReqLineFree(pNewLine);
				
			}	
			uaReqLineFree(pNewLine);
		}else{
			sipReqFree(subscribeMsg);
			TCRPrint(1,"*** [uaSUBSCRIBEMsg] To create new request line is failed!\n");
			return NULL;
		}
	}else
		uaReqLineFree(pNewLine);

	username=(char*)uaUserGetName(user);
	laddr=(char*)uaUserGetLocalAddr(user);
	hostname=(char*)uaUserGetLocalHost(user);
	if((NULL==username)||(NULL==laddr)||(NULL==hostname)){
		sipReqFree(subscribeMsg);
		TCRPrint(1,"*** [uaSUBSCRIBEMsg] username, localaddr or hostname is NULL!\n");
		return NULL;
	}

	/*copy local ip address into ip buffer*/
	strcpy(ip,laddr);

	/*Call-ID header*/
	if(uaDlgGetCallID(_dlg))
		sprintf(buf,"Call-ID:%s\r\n",uaDlgGetCallID(_dlg));
	else
		sprintf(buf,"Call-ID:%s\r\n",sipCallIDGen(hostname,ip));

	rCode=sipReqAddHdrFromTxt(subscribeMsg,Call_ID,buf);

	/* Accept header */
#ifdef SIMPLE_Accept_Type
	sprintf(buf,"Accept:%s\r\n",SIMPLE_Accept_Type);
#else
	sprintf(buf,"Accept:application/pidf+xml\r\n");
#endif
	
	rCode=sipReqAddHdrFromTxt(subscribeMsg,Accept,buf);
	/*Event header*/
	sprintf(buf,"Event:%s\r\n",eventname);
	rCode=sipReqAddHdrFromTxt(subscribeMsg,Event,buf);
	
	/*To header*/
	pToTag=uaDlgGetRemoveTag(_dlg);
	if(pToTag != NULL)
		sprintf(buf,"To:<%s>;tag=%s\r\n",_dest,pToTag);
	else
		sprintf(buf,"To:<%s>\r\n",_dest);

	rCode=sipReqAddHdrFromTxt(subscribeMsg,To,buf);

	/*From header and Contact header. modified by tyhuang*/	
	pFromTag=uaDlgGetLocalTag(_dlg);
	sprintf(buf,"From:<%s>;tag=%s\r\n",uaMgrGetPublicAddr(mgr),pFromTag);
	rCode=sipReqAddHdrFromTxt(subscribeMsg,From,buf);
	
	/*Contact header*/
	sprintf(buf,"Contact:%s\r\n",uaMgrGetContactAddr(mgr));
	rCode=sipReqAddHdrFromTxt(subscribeMsg,Contact,buf);

	/*CSeq header*/
	uaMgrAddCmdSeq(mgr);
	cseq=uaNewCSeq(uaMgrGetCmdSeq(mgr),SIP_SUBSCRIBE);
	sipReqAddHdr(subscribeMsg,CSeq,cseq);
	uaFreeCSeq(cseq);

	/*Max-Forward*/
	maxfor=uaUserGetMaxForwards(user);
	if(maxfor<0) maxfor=70;
	/*sprintf(buf,"Max-Forwards:70\r\n");*/
	sprintf(buf,"Max-Forwards:%d\r\n",maxfor);
	rCode=sipReqAddHdrFromTxt(subscribeMsg,Max_Forwards,buf);

	/* expires .default to 600 */
	if(expires >= 0)
		sprintf(buf,"Expires:%d\r\n",expires);
	else
		sprintf(buf,"Expires:600\r\n");

	rCode=sipReqAddHdrFromTxt(subscribeMsg,Expires,buf);
	
	bodylen=uaContentGetLength(content);
	if(bodylen>0){
		sprintf(buf,"Content-Length:%d\r\n",strlen(uaContentGetBody(content)));
		rCode=sipReqAddHdrFromTxt(subscribeMsg,Content_Length,buf);
		sprintf(buf,"Content-Type:%s\r\n",uaContentGetType(content));
		rCode=sipReqAddHdrFromTxt(subscribeMsg,Content_Type,buf);
	}else{
		sprintf(buf,"Content-Length:0\r\n");
		rCode=sipReqAddHdrFromTxt(subscribeMsg,Content_Length,buf);
	}
	/* User-Agent header */
	sprintf(buf,"User-Agent:%s\r\n",USER_AGENT);
	rCode=sipReqAddHdrFromTxt(subscribeMsg,User_Agent,buf);

	/* authz header */
	authz=uaUserGetAuthData(user);
	if(authz){
		sprintf(buf,"%s",authz);
		rCode=sipReqAddHdrFromTxt(subscribeMsg,Authorization,buf);
	}
	/* add content to sip message body */
	if(subscribeMsg&&uaContentGetBody(content)){
		tmpBody=(SipBody*)calloc(1,sizeof(SipBody));
		tmpBody->length=bodylen;
		tmpBody->content=uaContentGetBody(content);
		rCode=sipReqSetBody(subscribeMsg,tmpBody);
		if(rCode != RC_OK){
			sipReqFree(subscribeMsg);
			subscribeMsg=NULL;
		}
	}
	if(tmpBody != NULL)
		free(tmpBody);
	TCRPrint(TRACE_LEVEL_API,"*** [uaSUBSCRIBEMsg]create a SUBSCRIBE message\n%s!\n",sipReqPrint(subscribeMsg));
	return subscribeMsg;
}

/** Function to create a Publish message 
 *  \param _dlg a dialog will be used to send the message
 *  \param _dest the destination url 
 *  \param eventname type of event for this Publish message
 *  \param classlist the list for class header
 *  \param _content the message body with content-type 
 *  \retval SipReq object of publish method or NULL
 */
SipReq	uaPUBLISHMsg(IN UaDlg _dlg, IN const char* _dest, IN const char* eventname,IN const char* classlist,IN UaContent _content,IN int expires)
{
	SipReq publishMsg=NULL;
	SipReq origReq=NULL;
	SipRsp finalRsp=NULL;
	TxStruct tx=NULL;
	SipRecordRoute *pRecordRoute=NULL;
	SipRoute *pRoute=NULL;
	SipReqLine *pLine=NULL,*pNewLine=NULL;
	SipCSeq *cseq;
	SipFrom *pFrom=NULL,*pFromTag=NULL;
	RCODE rCode;
	SipContact *pContact=NULL;
	SipURL url=NULL;
	BOOL bContactOK=FALSE;
	UaMgr  mgr;
	UaUser user;
	UaCfg  cfg;
	SipBody	*tmpBody=NULL;
	char buf[1024]={'\0'};
	char ip[MAXADDRESIZE]={'\0'},*username,*laddr,*hostname,*authz;
	char *sipifmatch=NULL;
	int maxfor,bodylen;
	TXTYPE txType;
	char uribuf[256]={'\0'};
	
	/*check input parameter is NULL or not*/
	if((_dlg == NULL)||(_dest == NULL)){
		TCRPrint(1,"*** [uaPUBLISHMsg] dialog or destination is NULL!\n");
		return NULL;
	}
	/*get parent mgr, if NULL : Error */
	mgr=uaDlgGetMgr(_dlg);
	if(mgr==NULL){
		TCRPrint(1,"*** [uaPUBLISHMsg] mgr is NULL!\n");
		return NULL;
	}
	/*get user object, get user information*/
	user=uaMgrGetUser(mgr);
	if(user == NULL){
		TCRPrint(1,"*** [uaPUBLISHMsg] user is NULL!\n");
		return NULL;
	}
	/*get cfg object, get configuration information*/
	cfg=uaMgrGetCfg(mgr);
	if(cfg==NULL){
		TCRPrint(1,"*** [uaPUBLISHMsg] cfg is NULL!\n");
		return NULL;
	}
	/*create new empty request message*/
	publishMsg=sipReqNew();
	if(publishMsg == NULL){
		TCRPrint(1,"*** [uaPUBLISHMsg] To create new Request is failed!\n");
		return NULL;
	}
	tx=uaDlgGetInviteTx(_dlg,FALSE);
	if(tx==NULL)
		tx=uaDlgGetInviteTx(_dlg,TRUE);
	if(tx != NULL){
		origReq=sipTxGetOriginalReq(tx);
		finalRsp=sipTxGetLatestRsp(tx);
		
		txType=sipTxGetType(tx);
		/*Client transaction*/
		if(((TX_CLIENT_NON_INVITE == txType)||(TX_CLIENT_INVITE == txType)) && finalRsp){
			if(origReq == NULL){
				sipReqFree(publishMsg);
				TCRPrint(1,"*** [uaPUBLISHMsg] original request is NULL!\n");
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
				/*uaContactAsReqURI(pContact,uribuf,256,&urilen); 2003.8.22-tyhuang*/
				sprintf(uribuf,"%s",uaDlgGetRemoteTarget(_dlg));
				pNewLine=uaReqNewReqLine(SIP_PUBLISH,uribuf,pLine->ptrSipVersion);
			}else
				pNewLine=uaReqNewReqLine(SIP_PUBLISH,pLine->ptrRequestURI,pLine->ptrSipVersion);
			rCode=sipReqSetReqLine(publishMsg,pNewLine);

			if(rCode != RC_OK){
				if(pNewLine)
					uaReqLineFree(pNewLine);
				sipReqFree(publishMsg);
				TCRPrint(1,"*** [uaPUBLISHMsg] To set request line is failed!\n");
				return NULL;
			}
			
		
			/*Add Route header if exist Reocord-Route header*/
			pRecordRoute=(SipRecordRoute*)sipRspGetHdr(finalRsp,Record_Route);
			if(pRecordRoute != NULL){
				pRoute=uaRecordRouteCovertToRoute(pRecordRoute);
				if(pRoute != NULL){
					sipReqAddHdr(publishMsg,Route,pRoute);
					uaRouteFree(pRoute);
				}
			}
		}else if((TX_SERVER_NON_INVITE==txType)||(TX_SERVER_INVITE==txType)){
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
				pNewLine=uaReqNewReqLine(SIP_PUBLISH,uribuf,SIP_VERSION);
				sipURLFree(url);
			}else{ /*copy from from header*/
				pFrom=sipRspGetHdr(finalRsp,From);
				if(pFrom != NULL)
					pNewLine=uaReqNewReqLine(SIP_PUBLISH,pFrom->address->addr,SIP_VERSION);
				else{
					sipReqFree(publishMsg);
					TCRPrint(1,"*** [uaPUBLISHMsg] From Header gotten is NULL!\n");
					return NULL;
				}
			}
			rCode=sipReqSetReqLine(publishMsg,pNewLine);
					
			/*Add Route header if exist Reocord-Route header*/
			pRecordRoute=(SipRecordRoute*)sipReqGetHdr(origReq,Record_Route);
			if(pRecordRoute != NULL){
				pRoute=uaRecordRouteCovertToRoute(pRecordRoute);
				if(pRoute != NULL){
					sipReqAddHdr(publishMsg,Route,pRoute);
					uaRouteFree(pRoute);
				}
			}
		}
	}
	if(pNewLine==NULL){
		/*request line*/
		pNewLine=uaReqNewReqLine(SIP_PUBLISH,(char*)_dest,SIP_VERSION);
		if(NULL != pNewLine){
			rCode=sipReqSetReqLine(publishMsg,pNewLine);
			if(RC_OK != rCode){
				uaReqLineFree(pNewLine);
				sipReqFree(publishMsg);
			}
			if(NULL != pNewLine)
				uaReqLineFree(pNewLine);
		}else{
			sipReqFree(publishMsg);
			return NULL;
		}
	}else
		uaReqLineFree(pNewLine);

	username=(char*)uaUserGetName(user);
	laddr=(char*)uaUserGetLocalAddr(user);
	hostname=(char*)uaUserGetLocalHost(user);
	if((NULL==username)||(NULL==laddr)||(NULL==hostname)){
		sipReqFree(publishMsg);
		TCRPrint(1,"*** [uaPUBLISHMsg] username,localaddr,or hostname is NULL!\n");
		return NULL;
	}

	/*copy local ip address into ip buffer*/
	strcpy(ip,laddr);

	/*Call-ID header*/
	if(uaDlgGetCallID(_dlg))
		sprintf(buf,"Call-ID:%s\r\n",uaDlgGetCallID(_dlg));
	else
		sprintf(buf,"Call-ID:%s\r\n",sipCallIDGen(hostname,ip));

	rCode=sipReqAddHdrFromTxt(publishMsg,Call_ID,buf);
	/*To header*/
	sprintf(buf,"To:<%s>\r\n",_dest);
	rCode=sipReqAddHdrFromTxt(publishMsg,To,buf);

	/*From header and Contact header. modified by tyhuang*/

	sprintf(buf,"From:\"%s\"<%s>\r\n",uaMgrGetUserName(mgr),uaMgrGetPublicAddr(mgr));
	rCode=sipReqAddHdrFromTxt(publishMsg,From,buf);
	pFrom=(SipFrom*)sipReqGetHdr(publishMsg,From);
	pFromTag=uaFromDupAddTag(pFrom);
	if(pFromTag != NULL){
		rCode=sipReqAddHdr(publishMsg,From,(void*)pFromTag);
		uaFromHdrFree(pFromTag);
	}

	/*CSeq header*/
	uaMgrAddCmdSeq(mgr);
	cseq=uaNewCSeq(uaMgrGetCmdSeq(mgr),SIP_PUBLISH);
	sipReqAddHdr(publishMsg,CSeq,cseq);
	uaFreeCSeq(cseq);
	
	/*Event*/
	sprintf(buf,"Event:%s\r\n",eventname);
	rCode=sipReqAddHdrFromTxt(publishMsg,Event,buf);
	
	/*class for publish content */
	if(classlist){
		sprintf(buf,"Class:%s\r\n",classlist);
		rCode=sipReqAddHdrFromTxt(publishMsg,Class,buf);
	}

	/* expires .default to 600 */
	if(expires >= 0)
		sprintf(buf,"Expires:%d\r\n",expires);
	else
		sprintf(buf,"Expires:3600\r\n");

	rCode=sipReqAddHdrFromTxt(publishMsg,Expires,buf);
	
	/*Max-Forward*/
	maxfor=uaUserGetMaxForwards(user);
	if(maxfor<0) maxfor=70;
	/*sprintf(buf,"Max-Forwards:70\r\n");*/
	sprintf(buf,"Max-Forwards:%d\r\n",maxfor);
	rCode=sipReqAddHdrFromTxt(publishMsg,Max_Forwards,buf);

	bodylen=uaContentGetLength(_content);
	if(bodylen>0){
		sprintf(buf,"Content-Length:%d\r\n",bodylen);
		rCode=sipReqAddHdrFromTxt(publishMsg,Content_Length,buf);
		sprintf(buf,"Content-Type:%s\r\n",uaContentGetType(_content));
		rCode=sipReqAddHdrFromTxt(publishMsg,Content_Type,buf);
	}else{
		sprintf(buf,"Content-Length:0\r\n");
		rCode=sipReqAddHdrFromTxt(publishMsg,Content_Length,buf);
	}

	/* SIP-If-Match */
	sipifmatch=uaDlgGetSIPIfMatch(_dlg);
	if(sipifmatch != NULL){
		sprintf(buf,"SIP-If-Match:%s\r\n",sipifmatch);
		rCode=sipReqAddHdrFromTxt(publishMsg,SIP_If_Match,buf);
	}

	/* User-Agent header */
	sprintf(buf,"User-Agent:%s\r\n",USER_AGENT);
	rCode=sipReqAddHdrFromTxt(publishMsg,User_Agent,buf);
	
	/* authz header */
	authz=uaUserGetAuthData(user);
	if(authz){
		sprintf(buf,"%s",authz);
		rCode=sipReqAddHdrFromTxt(publishMsg,Authorization,buf);
	}
	/* add content to sip message body */
	if((publishMsg != NULL)&&(uaContentGetBody(_content)!=NULL)){
		tmpBody=(SipBody*)calloc(1,sizeof(SipBody));
		tmpBody->length=bodylen;
		tmpBody->content=uaContentGetBody(_content);
		rCode=sipReqSetBody(publishMsg,tmpBody);
		if(rCode != RC_OK){
			sipReqFree(publishMsg);
			publishMsg=NULL;
		}
	}
	if(tmpBody != NULL)
		free(tmpBody);
	TCRPrint(TRACE_LEVEL_API,"*** [uaPUBLISHMsg]create a PUBLISH message\n%s!\n",sipReqPrint(publishMsg));
	return publishMsg;
}

/** Function to create a Messgae message 
 *  \param _dlg a dialog will be used to send the message
 *  \param _dest the destination url 
 *  \param _content the message body with content-type 
 *  \retval SipReq object of message method or NULL
 */
SipReq	uaMESSAGEMsg(IN UaDlg _dlg, IN const char* _dest, IN UaContent _content)
{
	SipReq origReq=NULL;
	SipRsp finalRsp=NULL;
	TxStruct tx=NULL;
	SipRecordRoute *pRecordRoute=NULL;
	SipRoute *pRoute=NULL;
	SipReqLine *pLine=NULL,*pNewLine=NULL;
	SipCSeq *cseq;
	SipFrom *pFrom=NULL,*pFromTag=NULL;
	RCODE rCode;
	SipContact *pContact=NULL;
	SipURL url=NULL;
	BOOL bContactOK=FALSE;
	SipReq messageMsg=NULL;
	UaMgr  mgr;
	UaUser user;
	UaCfg  cfg;
	SipBody	*tmpBody=NULL;
	char buf[1024]={'\0'};
	char ip[MAXADDRESIZE]={'\0'},*username,*laddr,*hostname,*authz;
	int maxfor,bodylen;
	TXTYPE txType;
	char uribuf[256]={'\0'};

	/*check input parameter is NULL or not*/
	if((_dlg == NULL)||(_dest == NULL)){
		TCRPrint(1,"*** [uaMESSAGEMsg] dialog or destination is NULL!\n");
		return NULL;
	}
	/*get parent mgr, if NULL : Error */
	mgr=uaDlgGetMgr(_dlg);
	if(mgr==NULL){
		TCRPrint(1,"*** [uaMESSAGEMsg] mgr is NULL!\n");
		return NULL;
	}
	/*get user object, get user information*/
	user=uaMgrGetUser(mgr);
	if(user == NULL){
		TCRPrint(1,"*** [uaMESSAGEMsg] user is NULL!\n");
		return NULL;
	}
	/*get cfg object, get configuration information*/
	cfg=uaMgrGetCfg(mgr);
	if(cfg==NULL)
		return NULL;

	/*create new empty request message*/
	messageMsg=sipReqNew();
	if(messageMsg == NULL){
		TCRPrint(1,"*** [uaMESSAGEMsg] To create new Request is failed!\n");
		return NULL;
	}
	tx=uaDlgGetInviteTx(_dlg,FALSE);
	if(tx==NULL)
		tx=uaDlgGetInviteTx(_dlg,TRUE);
	if(tx != NULL){
		origReq=sipTxGetOriginalReq(tx);
		finalRsp=sipTxGetLatestRsp(tx);
		
		txType=sipTxGetType(tx);
		/*Client transaction*/
		if(  ((TX_CLIENT_NON_INVITE == txType)||(TX_CLIENT_INVITE == txType)) && finalRsp){
			if(origReq == NULL){
				sipReqFree(messageMsg);
				TCRPrint(1,"*** [uaMESSAGEMsg] original request is NULL!\n");
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
				pNewLine=uaReqNewReqLine(SIP_MESSAGE,uribuf,pLine->ptrSipVersion);
			}else
				pNewLine=uaReqNewReqLine(SIP_MESSAGE,pLine->ptrRequestURI,pLine->ptrSipVersion);
			rCode=sipReqSetReqLine(messageMsg,pNewLine);
			
			if(rCode != RC_OK){
				if(pNewLine)
					uaReqLineFree(pNewLine);
				sipReqFree(messageMsg);
				TCRPrint(1,"*** [uaMESSAGEMsg] To set request line is failed!\n");
				return NULL;
			}
			
		
			/*Add Route header if exist Reocord-Route header*/
			pRecordRoute=(SipRecordRoute*)sipRspGetHdr(finalRsp,Record_Route);
			if(pRecordRoute != NULL){
				pRoute=uaRecordRouteCovertToRoute(pRecordRoute);
				if(pRoute != NULL){
					sipReqAddHdr(messageMsg,Route,pRoute);
					uaRouteFree(pRoute);
				}
			}
		}else if((TX_SERVER_NON_INVITE==txType)||(TX_SERVER_INVITE==txType)){
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
				pNewLine=uaReqNewReqLine(SIP_MESSAGE,uribuf,SIP_VERSION);
				sipURLFree(url);
			}else{ /*copy from from header*/
				pFrom=sipRspGetHdr(finalRsp,From);
				if(pFrom != NULL)
					pNewLine=uaReqNewReqLine(SIP_MESSAGE,pFrom->address->addr,SIP_VERSION);
				else{
					sipReqFree(messageMsg);
					TCRPrint(1,"*** [uaMESSAGEMsg] From header gotten is NULL!\n");
					return NULL;
				}
			}
			rCode=sipReqSetReqLine(messageMsg,pNewLine);
			/*Add Route header if exist Reocord-Route header*/
			pRecordRoute=(SipRecordRoute*)sipReqGetHdr(origReq,Record_Route);
			if(pRecordRoute != NULL){
				pRoute=uaRecordRouteCovertToRoute(pRecordRoute);
				if(pRoute != NULL){
					sipReqAddHdr(messageMsg,Route,pRoute);
					uaRouteFree(pRoute);
				}
			}
		}
	}
	
	if(pNewLine==NULL){
		/*request line*/
		pNewLine=uaReqNewReqLine(SIP_MESSAGE,(char*)_dest,SIP_VERSION);
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
			TCRPrint(1,"*** [uaMESSAGEMsg] To create new request line is failed!\n");
			return NULL;
		}
	}else
		uaReqLineFree(pNewLine);

	username=(char*)uaUserGetName(user);
	laddr=(char*)uaUserGetLocalAddr(user);
	hostname=(char*)uaUserGetLocalHost(user);
	if((NULL==username)||(NULL==laddr)||(NULL==hostname)){
		sipReqFree(messageMsg);
		TCRPrint(1,"*** [uaMESSAGEMsg] username,localaddr or hostname is NULL!\n");
		return NULL;
	}

	/*copy local ip address into ip buffer*/
	strcpy(ip,laddr);

	/*Call-ID header*/
	if(uaDlgGetCallID(_dlg))
		sprintf(buf,"Call-ID:%s\r\n",uaDlgGetCallID(_dlg));
	else
		sprintf(buf,"Call-ID:%s\r\n",sipCallIDGen(hostname,ip));
	
	rCode=sipReqAddHdrFromTxt(messageMsg,Call_ID,buf);
	

	/*To header*/
	sprintf(buf,"To:<%s>\r\n",_dest);
	rCode=sipReqAddHdrFromTxt(messageMsg,To,buf);

	/*From header and Contact header. modified by tyhuang*/
//	dname=(char*)uaUserGetDisplayName(user);
//	if(dname)
//		sprintf(buf,"From:\"%s\"<%s>\r\n",dname,uaMgrGetPublicAddr(mgr));
//	else

// add by alan->add SIMPLE port number
//	UaCfg cfg;
//	cfg=uaMgrGetCfg(mgr);
	if(cfg!=NULL)
		sprintf(buf,"From:\"%s\"<sip:%s@%s:%d>\r\n",uaMgrGetUserName(mgr),uaMgrGetUserName(mgr),uaCfgGetSIMPLEProxyAddr(cfg),uaCfgGetSIMPLEProxyPort(cfg));
	else
		sprintf(buf,"From:\"%s\"<%s>\r\n",uaMgrGetUserName(mgr),uaMgrGetPublicAddr(mgr));

	rCode=sipReqAddHdrFromTxt(messageMsg,From,buf);
	pFrom=(SipFrom*)sipReqGetHdr(messageMsg,From);
	pFromTag=uaFromDupAddTag(pFrom);
	if(pFromTag != NULL){
		rCode=sipReqAddHdr(messageMsg,From,(void*)pFromTag);
		uaFromHdrFree(pFromTag);
	}
	/*Contact header*/
	/* not allow contact 
	sprintf(buf,"Contact:%s\r\n",uaMgrGetContactAddr(mgr));
	rCode=sipReqAddHdrFromTxt(messageMsg,Contact,buf);
	*/

	/*CSeq header*/
	uaMgrAddCmdSeq(mgr);
	cseq=uaNewCSeq(uaMgrGetCmdSeq(mgr),SIP_MESSAGE);
	sipReqAddHdr(messageMsg,CSeq,cseq);
	uaFreeCSeq(cseq);

	/*Max-Forward*/
	maxfor=uaUserGetMaxForwards(user);
	if(maxfor<0) maxfor=70;
	/*sprintf(buf,"Max-Forwards:70\r\n");*/
	sprintf(buf,"Max-Forwards:%d\r\n",maxfor);
	rCode=sipReqAddHdrFromTxt(messageMsg,Max_Forwards,buf);

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

	/* User-Agent header */
	sprintf(buf,"User-Agent:%s\r\n",USER_AGENT);
	rCode=sipReqAddHdrFromTxt(messageMsg,User_Agent,buf);
	
	/* authz header */
	authz=uaUserGetAuthData(user);
	if(authz){
		sprintf(buf,"%s",authz);
		rCode=sipReqAddHdrFromTxt(messageMsg,Authorization,buf);
	}
	/* add content to sip message body */
	if((messageMsg != NULL)&&(uaContentGetBody(_content)!=NULL)){
		tmpBody=(SipBody*)calloc(1,sizeof(SipBody));
		tmpBody->length=bodylen;
		tmpBody->content=uaContentGetBody(_content);
		rCode=sipReqSetBody(messageMsg,tmpBody);
		if(rCode != RC_OK){
			sipReqFree(messageMsg);
			messageMsg=NULL;
		}
	}
	if(tmpBody != NULL)
		free(tmpBody);
	TCRPrint(TRACE_LEVEL_API,"*** [uaMESSAGEMsg]create a MESSAGE message\n%s!\n",sipReqPrint(messageMsg));
	return messageMsg;	
}

/** Function to create a NOTIFY message from a UaSub object 
 *  \param _dlg a dialog will be used to send the message
 *  \param _dest the destination url 
 *  \param _sub the UaSub object contains the subscription information and state
 *  \param _content the message body with content-type 
 *  \retval SipReq object of notify method or NULL
 */
SipReq uaSubNOTIFYMsg(IN UaDlg _dlg,IN UaSub _sub,IN const char* _dest,IN UaContent _content)
{
	SipReq notifyMsg=NULL;
	SipReq origReq=NULL;
	SipRsp finalRsp=NULL;
	TxStruct tx=NULL;
	SipRecordRoute *pRecordRoute=NULL;
	SipRoute *pRoute=NULL;
	SipReqLine *pLine=NULL,*pNewLine=NULL;
	SipCSeq *cseq;
	SipFrom *pFrom=NULL,*pFromTag=NULL;
	RCODE rCode;
	SipContact *pContact=NULL;
	SipURL url=NULL;
	BOOL bContactOK=FALSE;
	UaMgr  mgr;
	UaUser user;
	UaCfg  cfg;
	SipBody	*tmpBody=NULL;
	char buf[1024]={'\0'};
	char ip[MAXADDRESIZE]={'\0'},*username,*laddr,*hostname;
	int maxfor,pos=0;
	TXTYPE txType;
	char uribuf[256]={'\0'};
	int  bodylen=0;

	/*check input parameter is NULL or not*/
	if((_dlg == NULL)||(_dest == NULL)){
		TCRPrint(1,"*** [uaSubNOTIFYMsg] dialog or destination is NULL!\n");
		return NULL;
	}
	/*get parent mgr, if NULL : Error */
	mgr=uaDlgGetMgr(_dlg);
	if(mgr==NULL){
		TCRPrint(1,"*** [uaSubNOTIFYMsg] mgr is NULL!\n");
		return NULL;
	}
	/*get user object, get user information*/
	user=uaMgrGetUser(mgr);
	if(user == NULL){
		TCRPrint(1,"*** [uaSubNOTIFYMsg] user is NULL!\n");
		return NULL;
	}
	/*get cfg object, get configuration information*/
	cfg=uaMgrGetCfg(mgr);
	if(cfg==NULL){
		TCRPrint(1,"*** [uaSubNOTIFYMsg] cfg is NULL!\n");
		return NULL;
	}
	/*create new empty request message*/
	notifyMsg=sipReqNew();
	if(notifyMsg == NULL){
		TCRPrint(1,"*** [uaSubNOTIFYMsg] To create new Request is failed!\n");
		return NULL;
	}
	tx=uaDlgGetInviteTx(_dlg,FALSE);
	if(tx==NULL)
		tx=uaDlgGetInviteTx(_dlg,TRUE);
	if(tx != NULL){
		origReq=sipTxGetOriginalReq(tx);
		finalRsp=sipTxGetLatestRsp(tx);
		
		txType=sipTxGetType(tx);
		/*Client transaction*/
		if( ((TX_CLIENT_NON_INVITE == txType)||(TX_CLIENT_INVITE == txType)) && finalRsp){
			if(origReq == NULL){
				sipReqFree(notifyMsg);
				TCRPrint(1,"*** [uaSubNOTIFYMsg] original request is NULL!\n");
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
				/*uaContactAsReqURI(pContact,uribuf,256,&urilen);2003.8.22-tyhuang*/
				sprintf(uribuf,"%s",uaDlgGetRemoteTarget(_dlg));
				pNewLine=uaReqNewReqLine(SIP_NOTIFY,uribuf,pLine->ptrSipVersion);
			}else
				pNewLine=uaReqNewReqLine(SIP_NOTIFY,pLine->ptrRequestURI,pLine->ptrSipVersion);
			rCode=sipReqSetReqLine(notifyMsg,pNewLine);
		
			if(rCode != RC_OK){
				if(pNewLine)
					uaReqLineFree(pNewLine);
				sipReqFree(notifyMsg);
				TCRPrint(1,"*** [uaSubNOTIFYMsg] To set request line is failed!\n");
				return NULL;
			}
			
		
			/*Add Route header if exist Reocord-Route header*/
			pRecordRoute=(SipRecordRoute*)sipRspGetHdr(finalRsp,Record_Route);
			if(pRecordRoute != NULL){
				pRoute=uaRecordRouteCovertToRoute(pRecordRoute);
				if(pRoute != NULL){
					sipReqAddHdr(notifyMsg,Route,pRoute);
					uaRouteFree(pRoute);
				}
			}
		}else if((TX_SERVER_NON_INVITE==txType)||(TX_SERVER_INVITE==txType)){
			/*Server transaction*/
				pContact=sipReqGetHdr(origReq,Contact);
			if(pContact != NULL){
				url=sipURLNewFromTxt(pContact->sipContactList->address->addr);
				if(url != NULL)
					bContactOK=TRUE;
			}
			if(bContactOK){/*copy from Contact */
				/*uaContactAsReqURI(pContact,uribuf,256,&urilen); 2003.8.21-tyhuang*/
				sprintf(uribuf,"%s",uaDlgGetRemoteTarget(_dlg));
				pNewLine=uaReqNewReqLine(SIP_NOTIFY,uribuf,SIP_VERSION);
				sipURLFree(url);
			}else{ /*copy from from header*/
				pFrom=sipRspGetHdr(finalRsp,From);
				if(pFrom != NULL)
					pNewLine=uaReqNewReqLine(SIP_NOTIFY,pFrom->address->addr,SIP_VERSION);
				else{
					sipReqFree(notifyMsg);
					TCRPrint(1,"*** [uaSubNOTIFYMsg] From header gotten is NULL!\n");
					return NULL;
				}
			}
			rCode=sipReqSetReqLine(notifyMsg,pNewLine);
					
			/*Add Route header if exist Reocord-Route header*/
			pRecordRoute=(SipRecordRoute*)sipReqGetHdr(origReq,Record_Route);
			if(pRecordRoute != NULL){
				pRoute=uaRecordRouteCovertToRoute(pRecordRoute);
				if(pRoute != NULL){
					sipReqAddHdr(notifyMsg,Route,pRoute);
					uaRouteFree(pRoute);
				}
			}
		}
	}
	if(pNewLine==NULL){
		/*request line*/
		pNewLine=uaReqNewReqLine(SIP_NOTIFY,(char*)_dest,SIP_VERSION);
		if(NULL != pNewLine){
			rCode=sipReqSetReqLine(notifyMsg,pNewLine);
			if(RC_OK != rCode){
				uaReqLineFree(pNewLine);
				sipReqFree(notifyMsg);
			}
			if(NULL != pNewLine)
				uaReqLineFree(pNewLine);
		}else{
			sipReqFree(notifyMsg);
			TCRPrint(1,"*** [uaSubNOTIFYMsg] To create new request line is failed!\n");
			return NULL;
		}
	}else
		uaReqLineFree(pNewLine);


	username=(char*)uaUserGetName(user);
	laddr=(char*)uaUserGetLocalAddr(user);
	hostname=(char*)uaUserGetLocalHost(user);
	if((NULL==username)||(NULL==laddr)||(NULL==hostname)){
		sipReqFree(notifyMsg);
		TCRPrint(1,"*** [uaSubNOTIFYMsg] username,localaddr or hostname is NULL!\n");
		return NULL;
	}

	/*copy local ip address into ip buffer*/
	strcpy(ip,laddr);

	/*Call-ID header*/
	if(uaDlgGetCallID(_dlg))
		sprintf(buf,"Call-ID:%s\r\n",uaDlgGetCallID(_dlg));
	else
		sprintf(buf,"Call-ID:%s\r\n",sipCallIDGen(hostname,ip));

	rCode=sipReqAddHdrFromTxt(notifyMsg,Call_ID,buf);
	/*Event*/
	sprintf(buf,"Event:%s\r\n",uaEvtpkgGetName(uaSubGetEvtpkg(_sub)));
	rCode=sipReqAddHdrFromTxt(notifyMsg,Event,buf);

	/*To header*/
	/*sprintf(buf,"To:%s\r\n",_dest);
	rCode=sipReqAddHdrFromTxt(notifyMsg,To,buf);*/

	/*From header and Contact header. modified by tyhuang*/
	/*sprintf(buf,"From:%s<%s>\r\n",uaMgrGetUserName(mgr),uaMgrGetPublicAddr(mgr));
	rCode=sipReqAddHdrFromTxt(notifyMsg,From,buf);
	pFrom=(SipFrom*)sipReqGetHdr(notifyMsg,From);
	pFromTag=uaFromDupAddTag(pFrom);
	if(pFromTag != NULL){
		rCode=sipReqAddHdr(notifyMsg,From,(void*)pFromTag);
		uaFromHdrFree(pFromTag);
	}*/
	if(finalRsp){
		/*To header, copy from subscribe transaction latest response From*/
		rCode=sipReqAddHdr(notifyMsg,To,sipRspGetHdr(finalRsp,From));
		/*From header, copy from subscribe transaction latest response To */
		rCode=sipReqAddHdr(notifyMsg,From,sipRspGetHdr(finalRsp,To));
	}else{
		/*To header*/
		sprintf(buf,"To:<%s>\r\n",_dest);
		rCode=sipReqAddHdr(notifyMsg,To,buf);
		rCode=sipReqAddHdrFromTxt(notifyMsg,To,buf);

		/*From header and Contact header. modified by tyhuang*/
//		dname=(char*)uaUserGetDisplayName(user);
//		if(dname)
//			sprintf(buf,"From:\"%s\"<%s>\r\n",dname,uaMgrGetPublicAddr(mgr));
//		else
//			sprintf(buf,"From:\"%s\"<%s>\r\n",uaMgrGetUserName(mgr),uaMgrGetPublicAddr(mgr));
		rCode=sipReqAddHdrFromTxt(notifyMsg,From,buf);
		pFrom=(SipFrom*)sipReqGetHdr(notifyMsg,From);
		pFromTag=uaFromDupAddTag(pFrom);
		if(pFromTag != NULL){
			rCode=sipReqAddHdr(notifyMsg,From,(void*)pFromTag);
			uaFromHdrFree(pFromTag);
		}
	}
	/*Contact header*/
	sprintf(buf,"Contact:%s\r\n",uaMgrGetContactAddr(mgr));
	rCode=sipReqAddHdrFromTxt(notifyMsg,Contact,buf);

	/*CSeq header*/
	uaMgrAddCmdSeq(mgr);
	cseq=uaNewCSeq(uaMgrGetCmdSeq(mgr),SIP_NOTIFY);
	sipReqAddHdr(notifyMsg,CSeq,cseq);
	uaFreeCSeq(cseq);

	/*Max-Forward. modified by tyhuang 2003.05.19*/
	maxfor=uaUserGetMaxForwards(user);
	if(maxfor<0) maxfor=70;
	/*sprintf(buf,"Max-Forwards:70\r\n");*/
	sprintf(buf,"Max-Forwards:%d\r\n",maxfor);
	rCode=sipReqAddHdrFromTxt(notifyMsg,Max_Forwards,buf);

	/* subscribe-state */
	if(uaSubGetSubstate(_sub)==UASUB_TERMINATED){
		pos=sprintf(buf,"Subscription-State:%s;reason=",uaSubStateToStr(uaSubGetSubstate(_sub)));
		if(uaSubGetReason(_sub))
			sprintf(buf+pos,"%s\r\n",uaSubGetReason(_sub));
		else
			sprintf(buf+pos,"deactivated\r\n");
	}else
		sprintf(buf,"Subscription-State:%s;expires=%d\r\n",uaSubStateToStr(uaSubGetSubstate(_sub)),uaSubGetExpires(_sub));;
	rCode=sipReqAddHdrFromTxt(notifyMsg,Subscription_State,buf);
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
	/* User-Agent header */
	sprintf(buf,"User-Agent:%s\r\n",USER_AGENT);
	rCode=sipReqAddHdrFromTxt(notifyMsg,User_Agent,buf);
	
	/*Add body .modified by tyhuang 2003.05.20*/
	if((notifyMsg != NULL)&&(uaContentGetBody(_content)!=NULL)){
		tmpBody=(SipBody*)calloc(1,sizeof(SipBody));
		tmpBody->length=bodylen;
		tmpBody->content=uaContentGetBody(_content);
		rCode=sipReqSetBody(notifyMsg,tmpBody);
		if(rCode != RC_OK){
			sipReqFree(notifyMsg);
			notifyMsg=NULL;
		}
	}
	if(tmpBody != NULL)
		free(tmpBody);
	TCRPrint(TRACE_LEVEL_API,"*** [uaSubNOTIFYMsg]create a NOTIFY message\n%s!\n",sipReqPrint(notifyMsg));
	return notifyMsg;
}
