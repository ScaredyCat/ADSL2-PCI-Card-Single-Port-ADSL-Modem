/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 *
 * 
 * ua_msg.c
 *
 * $Id: ua_msg.c,v 1.49 2006/07/13 03:27:06 tyhuang Exp $
 */

#include "ua_msg.h"
#include "ua_dlg.h"
#include "ua_mgr.h"
#include "ua_cm.h"
#include "ua_int.h"
#include "ua_sipmsg.h"
#include <sip/sip_cm.h>


/*create a new UA message*/
/* this sdp will not free until uaMsgFree */
/*CCLAPI 
UaMsg uaMsgNew(IN UaDlg _dlg,UAEvtType evttype,int _cseq,SdpSess _sdp)*/
UaMsg uaMsgNew(IN UaDlg _dlg,UAEvtType evttype,int _cseq,UaContent _content,void* reserved)
{
	UaMsg uamsg=NULL;

	if(_dlg!=NULL){
		uamsg=(UaMsg)calloc(1,sizeof(struct uaMsgObj));
		if(uamsg == NULL)
			return NULL;

		uamsg->dlg=_dlg;
		uaDlgAddReference( _dlg, &(uamsg->dlg));
		uamsg->evtType=evttype;
		uamsg->callid=strDup(uaDlgGetCallID(_dlg));
		
		if(uamsg->callid == NULL){
			free(uamsg);
			UaCoreERR("[uaMsgNew] To callid is failed!!!\n");
			return NULL;
		}
		uamsg->cmdseq=_cseq;
		uamsg->callstate=uaDlgGetState(_dlg);
		uamsg->responsecode=uaGetRspStatusCode(uaDlgGetResponse(_dlg));
		uamsg->reserved=reserved;
		if(_content!=NULL){
			uamsg->bBody = TRUE;
			uamsg->rcontent = uaContentDup(_content);
			if(uamsg->rcontent == NULL){
				free(uamsg->callid);
				free(uamsg);
				UaCoreERR("[uaMsgNew] To new content is failed!!!\n");
				return NULL;
			}
		}else{
			uamsg->bBody = FALSE;
			uamsg->rcontent = NULL;
		}
		uamsg->rsubscribe=NULL;
	}
	return uamsg;
}

/*dulicate a uaMsg*/
CCLAPI 
UaMsg uaMsgDup(IN UaMsg _this)
{
	UaMsg uamsg=NULL;

	/*check input UaMsg*/
	if(_this == NULL)
		return NULL;
	else{
		/*duplicate each part of UaMsg*/
		uamsg=(UaMsg)calloc(1,sizeof(struct uaMsgObj));
		if(uamsg == NULL)
			return NULL;
		uamsg->dlg=_this->dlg;
		uaDlgAddReference( _this->dlg, &(uamsg->dlg));
		uamsg->evtType=_this->evtType;
		uamsg->responsecode=_this->responsecode;
		uamsg->reserved=_this->reserved;
		uamsg->callid=strDup(_this->callid);
		if(uamsg->callid == NULL){
			free(uamsg);
			UaCoreERR("[uaMsgDup] To callid is failed!!!\n");
			return NULL;
		}
		uamsg->cmdseq=_this->cmdseq;
		uamsg->callstate=_this->callstate;
		uamsg->bBody=_this->bBody;
		if(uamsg->bBody){
			/*contain sdp object*/
			uamsg->rcontent=uaContentDup(_this->rcontent);
			if(uamsg->rcontent == NULL){
				free(uamsg->callid);
				free(uamsg);
				UaCoreERR("[uaMsgDup] To duplicate content is failed!!!\n");
				return NULL;
			}
		}else{
			/*no contain sdp object*/
			uamsg->rcontent=NULL;
		}
	
		if(_this->rsubscribe != NULL)
			uamsg->rsubscribe=uaSubDup(_this->rsubscribe);
		else
			uamsg->rsubscribe=NULL;
	}
	return uamsg;
}

RCODE uaMsgFree(IN UaMsg _this)
{
	RCODE rCode = UAMSG_NULL;

	if(_this != NULL){
		if(_this->callid != NULL){
			free(_this->callid);
			_this->callid=NULL;
		}
		if((_this->bBody==TRUE)&&(_this->rcontent != NULL)){
			uaContentFree(_this->rcontent);
			_this->bBody=FALSE;
		}
	
		if(_this->rsubscribe != NULL)
			uaSubFree(_this->rsubscribe);
		
		uaDlgRemoveReference( _this->dlg, &(_this->dlg));
		free(_this);
		_this=NULL;
		rCode=RC_OK;
	}

	return rCode;
}

/*Dialog operation*/
CCLAPI 
UaDlg uaMsgGetDlg(IN UaMsg _this)
{
	if(_this != NULL){
		return _this->dlg;
	}else
		return NULL;
}

/*msg information*/
CCLAPI 
UACallStateType uaMsgGetCallState(IN UaMsg _this)
{
	UACallStateType statetype=UACSTATE_UNKNOWN;
	if(_this != NULL)
		statetype= _this->callstate;

	return statetype;
}
#ifdef CCL_DISABLE_AUTOSEND 
/* msg get request */
CCLAPI
SipReq uaMsgGetRequest(IN UaMsg _this)
{	
	SipReq req=NULL;
	TxStruct _tx;
	if(_this){
		_tx=(TxStruct)_this->reserved;
		if(_tx)
			req=sipTxGetOriginalReq(_tx);
	}
	return req;
}
/*
 *	send a response message via uamsg
 *	it may fail if the handle doesn't exist!
 */
CCLAPI RCODE uaMsgSendResponse(IN UaMsg _this,IN UAStatusCode _statusCode, IN UaContent _content)
{
	TxStruct _tx;
	SipReq req;
	SipRsp rsp;
	UaDlg dlg=NULL;
	RCODE rCode=RC_OK;
	/* check input parameter */
	if(!_this)
		return UAMSG_NULL;
	
	/* get handle */
	_tx=(TxStruct)_this->reserved;
	if(!_tx)
		return UATX_NULL;
	dlg=_this->dlg;
	req=sipTxGetOriginalReq(_tx);
	rsp=uaCreateRspMsg(req,_statusCode,NULL,_content);
	if(rsp==NULL){
		/*discard the message */
		rCode=UATX_REQ_MSG_ERROR;
		UaCoreERR("[uaMsgSendResponse] Create Response NULL!\n");
	}else{
		uaDlgSetResponse(dlg,rsp);
		sipTxSendRsp(_tx,rsp);
	}
	return rCode;
}
#endif

/*msg Response status code */
CCLAPI 
UAStatusCode uaMsgGetResponseCode(IN UaMsg _this)
{
	UAStatusCode code=UnKnown_Status;
	if(_this != NULL)
		/*code= _this->callstate;*/
		code=_this->responsecode;

	return code;
}

CCLAPI 
char*	uaMsgGetCallID(IN UaMsg _this)
{
	if(_this != NULL)
		return _this->callid;
	else
		return NULL;
}

CCLAPI 
int	uaMsgGetCmdSeq(IN UaMsg _this)
{
	if(_this != NULL){
		return _this->cmdseq;
	}else
		return -1;
}

CCLAPI 
BOOL	uaMsgContainSDP(IN UaMsg _this)
{
	if(_this != NULL && _this->rcontent){
		if(strICmp("application/sdp",uaContentGetType(_this->rcontent))==0)
			return _this->bBody;
	}
	else
		return FALSE;
	return FALSE;
}

CCLAPI 
SdpSess	uaMsgGetSdp(IN UaMsg _this)
{
	if(_this != NULL && _this->rcontent){
		if(strICmp("application/sdp",uaContentGetType(_this->rcontent))==0)
			return sdpSessNewFromText( (char*)uaContentGetBody(_this->rcontent));
	}else
		return NULL;
	return NULL;
}

CCLAPI UaContent uaMsgGetContent(IN UaMsg _this)
{
	if(_this != NULL && _this->rcontent){
		return uaContentDup(_this->rcontent);
	}else
		return NULL;
}

CCLAPI 
UAEvtType uaMsgGetEvtType(IN UaMsg _this)
{
	if(_this != NULL){
		return _this->evtType;
	}else
		return UAEVT_NULL;
}

/* add by tyhuang 2003.6.9*/
CCLAPI BOOL	uaMsgContainContent(IN UaMsg _this)
{
	if(_this!=NULL)
		return _this->bBody;
	return FALSE;
}

CCLAPI UaSub	 uaMsgGetSub(IN UaMsg _this)
{
	if(_this != NULL){
		if(_this->rsubscribe != NULL)
			return _this->rsubscribe;
	}
	return NULL;
}

RCODE	 uaMsgSetSubscription(IN UaMsg _this,IN UaSub _sub)
{
	RCODE rCode=RC_ERROR;
	if((_this != NULL) &&(_sub != NULL)){
		_this->rsubscribe=uaSubDup(_sub);
		rCode=RC_OK;
	}
	return rCode;

}

/*extract information*/
CCLAPI 
const char* uaMsgGetUser(IN UaMsg _this)
{
	char *user=NULL;
	if(_this != NULL){
		user=uaDlgGetUser(_this->dlg);
	}
	return user;
}

/*retrieve from From and To header*/
CCLAPI 
const char* uaMsgGetRemoteParty(IN UaMsg _msg)
{
	char *user=NULL;
	TxStruct tx=NULL;
	TXTYPE txType=TX_NON_ASSIGNED;

	if(NULL == _msg){
		UaCoreERR("[uaMsgGetRemoteParty] msg is NULL!\n");
		return NULL;
	}
	if(NULL == _msg->dlg){
		UaCoreERR("[uaMsgGetRemoteParty] msg->dlg is NULL!\n");
		return NULL;
	}
	/*check INVITE transaction*/
	tx=uaDlgGetInviteTx(_msg->dlg,TRUE);
	if(NULL!=tx){
		txType=sipTxGetType(tx);
		if(TX_CLIENT_INVITE == txType){
			/*get from To header*/
			user=uaGetReqToAddr(sipTxGetOriginalReq(tx));
		}else if(TX_SERVER_INVITE == txType){
			/*get from From header*/
			user=uaGetReqFromAddr(sipTxGetOriginalReq(tx));
		}
		if(user != NULL)
			return user;
	}
	/*check NO-INVITE Transaction*/
	tx=uaDlgGetInviteTx(_msg->dlg,FALSE);
	if(NULL != tx){
		txType=sipTxGetType(tx);
		if(TX_CLIENT_NON_INVITE == txType){
			/*get from To header*/
			user=uaGetReqToAddr(sipTxGetOriginalReq(tx));
		}else if(TX_SERVER_NON_INVITE == txType){
			/*get from From header*/
			user=uaGetReqFromAddr(sipTxGetOriginalReq(tx));
		}
		if(user != NULL)
			return user;
	}

	return user;
}

/*input buffer and buffer length to put Contact information*/
CCLAPI 
RCODE uaMsgGetRspContactAddr(IN UaMsg _this,IN OUT char* _buf, IN OUT int* _buflen)
{
	RCODE rCode=RC_OK;
	UaDlg dlg=NULL;
	TxStruct finaltx=NULL;
	SipRsp rsp=NULL;
	SipContact *contact=NULL;
	DxLst list=NULL;
	int size=0,pos=0;

	if(_this != NULL){
		/*check dialog exist or not*/
		dlg=_this->dlg;
		if(dlg==NULL)
			return UAMSG_DLG_NULL;

		/*get the latest transaction*/
		list=uaDlgGetTxList(dlg);
		size=dxLstGetSize(list);
		(size>0)?(pos=size-1):(pos=0);

		finaltx=dxLstPeek(list,pos);
		if(finaltx==NULL)
			return UAMSG_DLG_TX_NULL;

		/*get final response message*/
		rsp=sipTxGetLatestRsp(finaltx);
		if(rsp == NULL)
			return UAMSG_DLG_TX_MSG_NULL;
		
		/*get Contact message*/
		contact=sipRspGetHdr(rsp,Contact);
		if(contact != NULL){
			sipContactAsStr(contact,(unsigned char*)_buf,*_buflen,_buflen);
		}else
			return UAMSG_CONTACT_NULL;
	}else
		rCode=UAMSG_NULL;

	return rCode;
}

/*get REFER request contain Refer-To header*/
CCLAPI 
SipReferTo* uaMsgGetReqReferTo(IN UaDlg _dlg)
{
	SipReferTo *refto=NULL;
	TxStruct tx=NULL;
	SipReq	refReq=NULL;

	tx=uaDlgGetMethodTx(_dlg,REFER);
	if(NULL==tx)
		return refto;

	refReq=sipTxGetOriginalReq(tx);
	if(NULL != refReq){
		refto=sipReqGetHdr(refReq,Refer_To);
	}
	return refto;
}

CCLAPI 
RCODE uaMsgGetReqReferToAddr(IN UaDlg _dlg, IN OUT char* buf, IN OUT int* len)
{
	RCODE rCode=RC_OK;
	SipReferTo * refto;
	char* str;

	refto=uaMsgGetReqReferTo(_dlg);
	if(NULL != refto){
		if(!refto->address || !refto->address->addr)
			return UAMSG_URL_NULL;
		str=refto->address->addr;
		if(strlen(str) > (unsigned int)*len){
			*len=strlen(str);
			return UABUFFER_TOO_SMALL;
		}else{
			strcpy(buf,str);
			*len=strlen(str);
		}

	}
	return rCode;
}

/*get REFER request contain Referred-By header*/
CCLAPI 
SipReferredBy* uaMsgGetReqReferredBy(IN UaDlg _dlg)
{
	SipReferredBy *refby=NULL;
	TxStruct tx=NULL;
	SipReq	refReq=NULL;

	tx=uaDlgGetMethodTx(_dlg,REFER);
	if(NULL==tx)
		return refby;

	refReq=sipTxGetOriginalReq(tx);
	if(NULL != refReq){
		refby=sipReqGetHdr(refReq,Referred_By);
	}

	return refby;
}

CCLAPI 
RCODE uaMsgGetReqReferredByAddr(IN UaDlg _dlg,IN OUT char* buf, IN OUT int* buflen)
{
	RCODE rCode=RC_OK;
	SipReferredBy *refby=NULL;

	refby=uaMsgGetReqReferredBy(_dlg);
	if(NULL != refby){
		if(NULL != refby->address){
			if(strlen(refby->address) > (unsigned int)*buflen){
				*buflen=strlen(refby->address->addr);
				return UABUFFER_TOO_SMALL;
			}else{
				strcpy(buf,refby->address->addr);
				*buflen=strlen(refby->address->addr);
			}
		}

	}else
		return UAMSG_DLG_NULL;

	return rCode;

}

CCLAPI 
unsigned char* uaMsgGetSupported(IN UaMsg _this)
{
	if((NULL==_this)|| (NULL == _this->reserved))
		return NULL;
	else
		return (unsigned char *)_this->reserved;

}