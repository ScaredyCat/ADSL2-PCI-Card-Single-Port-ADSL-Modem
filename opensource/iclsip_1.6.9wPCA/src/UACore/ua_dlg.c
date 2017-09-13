/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * ua_dlg.c
 *
 * $Id: ua_dlg.c,v 1.222 2006/07/13 03:27:06 tyhuang Exp $
 */
#include <stdio.h>
#include <sip/sip.h>
#include <adt/dx_vec.h>
#include <adt/dx_lst.h>

#include "ua_dlg.h"
#include "ua_cm.h"
#include "ua_mgr.h"
#include "ua_sipmsg.h"
#include "ua_int.h"

char* taggen(void);

/*dialog correspond to a call concept*/
struct uaDlgObj{
	UaMgr		parent;		/*keep parent UaMgr object*/
	DxLst		txStruct;	/*transaction list*/
	UACallStateType	callState;	/*call state*/
	char*		callid;		/*callid*/
	char*		userid;		/*keep locat user identification*/
	SipMethodType	method;
	UINT32		remotecseq;	/*keep remote cseq */
	char*		remotedisplayname; /* keep remote identification */
	char*		remoteaddr; /* keep remote uri */
	char*		remotetarget; /* keep remote contact address */
	BOOL		totag;
	BOOL		flag_SDP;	/* flag for invite SDP */
	int			txCount;	/*keep transaction alive count, not in Terminated State*/
	BOOL		bRelease;	/*release flag, if TRUE, Free it*/
	BOOL		bTimer;		/* timer setting flag for internal use-2003.8.13 tyhuang*/
	int			hTimer;		/* handle for timer */
	int			hRegTimer;
	DxStr		unSupportRealm; /*keep unsupport Realm data*/
	SipReq		originalrequest;
	SipRsp		finalresponse;
	BOOL		bServer;
	char*		ReqRecRoute;
	char*		RspRecRoute;
	DxVector	refDlg;	/* other reference to this dialog object, 
						   use to clear dangling pointer after uaDlg freed */
	int			refresher; /* 0:nono/1:UPDATE-uac/2:UPDATE:uas/3:BYE */
	int			sessiontimerhandle;
	unsigned int	sessionexpires;
	SipMethodType	refreshmethod;  /* 0 for re-Invite,others for Update */
	char		*privacy;
	BOOL		privacyflag;	/* user privacy */
	char		*ppreferredid;

	unsigned int rseq; /* RSeq use in 1xx response */
	char		*localtag;
	char		*remotetag;

	unsigned char* sipifmatch;

	UaDlg		pRefDlg;
	BOOL		bRefDlg;
	char		*referevent;
	BOOL		Flag_Secure;
	unsigned char	*supported;
};

extern UAEvtCB g_uaEvtCB;

/* Dialog Initialize function and free */
/*create a dialog to attach parent */
CCLAPI UaDlg uaDlgNew(IN UaMgr _parent)
{
	UaDlg _this;
	UaUser _user;

	_user=uaMgrGetUser(_parent);
	if(_user == NULL){
		UaCoreERR("[uaDlgNew] user is NULL!\n");
		return NULL;
	}
	_this=(UaDlg)calloc(1,sizeof(struct uaDlgObj));
	if(_this != NULL){
		_this->parent=_parent;
		_this->callState=UACSTATE_IDLE;
		_this->txStruct=dxLstNew(DX_LST_POINTER);
		_this->userid=NULL;
		_this->txCount=0;
		_this->bRelease=FALSE;
		_this->bTimer=FALSE; /* 2003.8.13 tyhuang*/
		_this->hTimer=0;
		_this->hRegTimer=-1;
		_this->method=UNKNOWN_METHOD;
		_this->remotecseq=0;
		_this->callid=NULL;
		_this->unSupportRealm=NULL;
		_this->remotedisplayname=NULL;
		_this->remoteaddr=NULL;
		_this->remotetarget=NULL;
		_this->totag=FALSE;
		_this->flag_SDP=FALSE;
		_this->originalrequest=NULL;
		_this->finalresponse=NULL;
		_this->bServer=FALSE;
		_this->ReqRecRoute=NULL;
		_this->RspRecRoute=NULL;
		_this->refDlg=dxVectorNew(4,1,DX_VECTOR_POINTER);
		uaDlgSetUserID(_this,uaUserGetName(_user));
		_this->refresher=0;
		_this->sessiontimerhandle=-1;
		_this->sessionexpires=SESSION_TIMER_INTERVAL;
		_this->refreshmethod=SIP_UPDATE;
		_this->privacy=NULL;
		_this->privacyflag=FALSE;
		_this->ppreferredid=NULL;
		_this->rseq=0;

		_this->localtag=strDup(taggen());
		_this->remotetag=NULL;
		_this->pRefDlg=NULL;
		/* add for transfer 2004/8/25 */
		_this->bRefDlg=FALSE;
		_this->Flag_Secure=FALSE;

		_this->referevent=NULL;
		_this->supported=NULL;
		return _this;
	}else
		return NULL;
}

/*destroy a dialog */
void uaDlgFree(IN UaDlg _this)
{
	RCODE rCode=RC_OK;
	UaMgr mgr;
	

	if(_this != NULL){
		if(_this->callid != NULL){
			free(_this->callid);
			_this->callid=NULL;
		}
		/*
		Acer: modify as uaDlgRelease, check transaction is terminated or not
		uaMgrDelDlgLink(mgr,_this); 
		_this->parent=NULL;*/
		mgr=_this->parent;
		uaMgrDelDlgLink(mgr,_this); 
		
		if(NULL != _this->userid ){
			free(_this->userid);
			_this->userid = NULL;
		}
		if(NULL != _this->unSupportRealm){
			dxStrFree(_this->unSupportRealm);
			_this->unSupportRealm= NULL;
		}
		dxLstFree(_this->txStruct,(void (*)(void *)) sipTxFree);
		/* remote & remote */
		if(_this->remotedisplayname != NULL){
			free(_this->remotedisplayname);
			_this->remotedisplayname=NULL;
		}
		if(_this->remoteaddr != NULL){
			free(_this->remoteaddr);
			_this->remoteaddr=NULL;
		}
		if(_this->remotetarget != NULL){
			free(_this->remotetarget);
			_this->remotetarget=NULL;
		}
		if(_this->originalrequest!=NULL){
			sipReqFree(_this->originalrequest);
			_this->originalrequest=NULL;
		}
		if(_this->finalresponse != NULL){
			sipRspFree(_this->finalresponse);
			_this->finalresponse=NULL;
		}
		if(_this->bTimer==TRUE)
			uaDelTimer(_this->hTimer);

		if(_this->hRegTimer>-1)
			uaDelTimer(_this->hRegTimer);

		if((_this->refresher>0)&&(_this->refresher<4))
			uaDelTimer(_this->sessiontimerhandle);

		if(_this->privacy){
			free(_this->privacy);
			_this->privacy=NULL;
		}

		if(_this->ppreferredid){
			free(_this->ppreferredid);
			_this->ppreferredid=NULL;
		}

		if(_this->ReqRecRoute != NULL){
			free(_this->ReqRecRoute);
			_this->ReqRecRoute=NULL;
		}
		if(_this->RspRecRoute != NULL){
			free(_this->RspRecRoute);
			_this->RspRecRoute=NULL;
		}
		if(_this->localtag){
			free(_this->localtag);
			_this->localtag=NULL;
		}
		if(_this->remotetag){
			free(_this->remotetag);
			_this->remotetag=NULL;
		}

		if(_this->referevent)
			free(_this->referevent);

		{
			int i;
			int n = dxVectorGetSize(_this->refDlg);
			for ( i=0; i<n; i++)
			{
				UaDlg* ppDlg = dxVectorGetElementAt(_this->refDlg,i);
				if ( ppDlg)
					*ppDlg = NULL;	/* clear dangling pointer */
			}
			dxVectorFree(_this->refDlg,NULL);
		}

		if(_this->supported){
			free(_this->supported);
			_this->supported=NULL;
		}
		
		memset(_this,0,sizeof(UaDlgS));
		free(_this);
	}else
		rCode = UADLG_NULL;
}

/*Release dialog is to release control handle*/
/*will clean all resource when all transaction is terminated*/
CCLAPI 
void uaDlgRelease(IN UaDlg _this)
{
	int txpos, txnum;
	TxStruct targTx;
	DxLst txList;

	/*if (_this->bRelease)
		return;
	*/
	if(!_this)
		return;

	if (_this->bRelease!=TRUE) {
		_this->bRelease=TRUE;
	}

	if (_this->bRefDlg==TRUE) 
		return;
	/*check all transaction is terminated or not*/
	/*
	if(_this->txCount ==0){
		uaDlgFree(_this);
	}
	*/
	txList=uaDlgGetTxList(_this);
	if (txList)
	{
		txnum=dxLstGetSize(txList);
		for(txpos=0;txpos < txnum;txpos++){
			targTx=dxLstPeek(txList,txpos);
			if (sipTxGetState(targTx) != TX_TERMINATED)
				return;
		}
	}
	TCRPrintEx(TRACE_LEVEL_API,"[uaDlgRelease] all tx in list is terminated,so fun dlgfree!");
	uaDlgFree(_this);
}


/*get parent - uaMgr */	
CCLAPI 
UaMgr uaDlgGetMgr(IN UaDlg _this)
{
	if(_this != NULL)
		return _this->parent;
	else
		return NULL;
}

/* change parent -uaMgr ;both two must not be NULL*/
CCLAPI 
RCODE uaDlgSetMgr(IN UaDlg _this,IN UaMgr _mgr)
{
	RCODE rCode=RC_ERROR;


	if(_this && _mgr){
		_this->parent=_mgr;
		
		rCode=RC_OK;
	}
	return rCode;
}

CCLAPI 
char* uaDlgGetUser(IN UaDlg _this)
{
	if(_this != NULL)
		return _this->userid;
	else
		return NULL;
}

/* Dialog get a callstate */
CCLAPI 
UACallStateType uaDlgGetState(IN UaDlg _this)
{
	if(_this != NULL)
		return _this->callState;
	else
		return UACSTATE_UNKNOWN;
}

 
RCODE uaDlgSetState(IN UaDlg _this,IN UACallStateType _type)
{
	RCODE rCode=UADLG_ERROR;
	if(_this != NULL){
		_this->callState = _type;
		rCode=RC_OK;
/*		if(_type==UACSTATE_DISCONNECT){
			if(_this->originalrequest!=NULL){
				sipReqFree(_this->originalrequest);
				_this->originalrequest=NULL;
			}
			if(_this->finalresponse != NULL){
				sipRspFree(_this->finalresponse);
				_this->finalresponse=NULL;
			}
		}
*/
	}else
		rCode =UADLG_NULL;

	return rCode;
}

/*dialog callid*/
CCLAPI 
char* uaDlgGetCallID(IN UaDlg _this)
{
	if(_this != NULL)
		return _this->callid;
	else 
		return NULL;
}

 
RCODE uaDlgSetCallID(IN UaDlg _this,IN const char* _callid)
{
	RCODE rCode=UADLG_ERROR;
	char *pAt;
	if(_this != NULL){
		if(_this->callid != NULL){
			free(_this->callid);
			_this->callid=NULL;
		}
		if(_callid != NULL){
			_this->callid=strDup(_callid);
			if(_this->privacy){
				pAt=strchr(_this->callid,'@');
				if(pAt)
					*pAt='\0';
			}
			rCode=RC_OK;
		}else
			rCode=UADLG_CALLID_NULL;
	}else
		rCode=UADLG_NULL;
	return rCode;
}

/*dialog user id*/
CCLAPI 
char* uaDlgGetUserID(IN UaDlg _this)
{
	if(_this != NULL)
		return _this->userid;
	else
		return NULL;
}

/*dialog initial method 2003.6.24.tyhuang */
CCLAPI	SipMethodType uaDlgGetMethod(IN UaDlg _this)
{
	if(_this)
		return _this->method;
	else
		return UNKNOWN_METHOD;
}

CCLAPI 
RCODE uaDlgSetPrivacy(IN UaDlg _this,IN const char *priv)
{
	if(_this && priv){
		if(_this->privacy)
			free(_this->privacy);
		_this->privacy=strDup(priv);
	}else
		return RC_ERROR;

	return RC_OK;
}

CCLAPI 
RCODE uaDlgSetUserPrivacy(IN UaDlg _this,IN BOOL flag)
{
	if(!_this)
		return RC_ERROR;
	_this->privacyflag=flag;
	return RC_OK;

}

CCLAPI 
RCODE uaDlgSetPPreferredID(IN UaDlg _this,IN const char *ppid)
{
	if(_this && ppid){
		if(_this->ppreferredid)
			free(_this->ppreferredid);
		_this->ppreferredid=strDup(ppid);
	}else
		return RC_ERROR;

	return RC_OK;

}

char* uaDlgGetPrivacy(IN UaDlg _this)
{
	if(_this)
		return _this->privacy;
	return NULL;
}

BOOL uaDlgGetUserPrivacy(IN UaDlg _this)
{
	if(_this)
		return _this->privacyflag;
	return FALSE;
}

char* uaDlgGetPPreferredID(IN UaDlg _this)
{
	if(_this)
		return _this->ppreferredid;
	return NULL;
}

RCODE	uaDlgGetInviteSDP(IN UaDlg _this)
{
	if(_this==NULL){
		UaCoreERR("[uaDlgGetInviteSDP] dlg is NULL!\n");
		return UADLG_NULL;
	}
	_this->flag_SDP=TRUE;
	return RC_OK;
}

BOOL  isuaDlgInviteSDP(IN UaDlg _this)
{
	if(_this==NULL){
		UaCoreERR("[isuaDlgInviteSDP] dlg is NULL!\n");
		return FALSE;
	}
	return _this->flag_SDP;
}

RCODE	uaDlgSetMethod(IN UaDlg _this, IN SipMethodType _method)
{
	RCODE rCode=UADLG_ERROR;
	if(_this){
		_this->method=_method;
		rCode=RC_OK;
	}else
		rCode=UADLG_NULL;
	return rCode;

}

/*add unsupport Realm*/
RCODE uaDlgAddUnSupportRealm(IN UaDlg _dlg, IN char* _realm)
{
	RCODE rCode=RC_OK;

	if(NULL == _dlg)
		return UADLG_NULL;
	if(NULL == _dlg->unSupportRealm ){
		_dlg->unSupportRealm=dxStrNew();
	}

	if(NULL == _dlg->unSupportRealm)
		return RC_ERROR;

	dxStrCat(_dlg->unSupportRealm," ");
	dxStrCat(_dlg->unSupportRealm,_realm);

	return rCode;
}

RCODE uaDlgDelUnSupportRealm(IN UaDlg _dlg)
{
	RCODE rCode=RC_OK;

	if(NULL == _dlg)
		return UADLG_NULL;
	if(NULL != _dlg->unSupportRealm){
		dxStrFree(_dlg->unSupportRealm);
		_dlg->unSupportRealm=NULL;
	}
	return rCode;
}
 
RCODE uaDlgSetUserID(IN UaDlg _dlg, IN const char* _userid)
{
	RCODE rCode = UADLG_ERROR;
	if(_dlg != NULL){
		if(_dlg->userid != NULL){
			free(_dlg->userid);
			_dlg->userid=NULL;
		}
		if(_userid != NULL){
			_dlg->userid=strDup(_userid);
			rCode=RC_OK;
		}else
			rCode=UADLG_USERID_NULL;
	}else
		rCode=UADLG_NULL;
	return rCode;
}

RCODE uaDlgSetRemoteDisplayname(IN UaDlg _dlg, IN const char* _remotedisplayname)
{
	RCODE rCode = UADLG_ERROR;

	if (!_remotedisplayname) {
		return UADLG_REMOTEID_NULL;
	}

	if(_dlg != NULL){
		if(_dlg->remotedisplayname != NULL){
			free(_dlg->remotedisplayname);
			_dlg->remotedisplayname=NULL;
		}		
		_dlg->remotedisplayname=strDup(_remotedisplayname);
		rCode=RC_OK;
	}else
		rCode=UADLG_NULL;
	return rCode;
}

RCODE uaDlgSetRemoteAddr(IN UaDlg _dlg, IN const char* _remoteaddr)
{
	RCODE rCode = UADLG_ERROR;

	if (!_remoteaddr) {
		return UADLG_REMOTEADDR_NULL;
	}

	if(_dlg != NULL){
		if(_dlg->remoteaddr != NULL){
			free(_dlg->remoteaddr);
			_dlg->remoteaddr=NULL;
		}
		_dlg->remoteaddr=strDup(_remoteaddr);
		rCode=RC_OK;
	}else
		rCode=UADLG_NULL;
	return rCode;
}

RCODE uaDlgSetRemoteTarget(IN UaDlg _dlg, IN const char* _remotetarget)
{
	RCODE rCode = UADLG_ERROR;

	if (!_remotetarget) {
		return UADLG_REMOTETARGET_NULL;
	}

	if(_dlg != NULL){
		if(_dlg->remotetarget != NULL){
			free(_dlg->remotetarget);
			_dlg->remotetarget=NULL;
		}
		_dlg->remotetarget=strDup(_remotetarget);
		rCode=RC_OK;
	}else
		rCode=UADLG_NULL;
	return rCode;
}

const char * uaDlgGetRemoteTarget(IN UaDlg _dlg)
{
	if(_dlg)
		return _dlg->remotetarget;
	
	return NULL;	
}
DxLst uaDlgGetTxList(IN UaDlg _this)
{
	if(_this!=NULL)
		return _this->txStruct;
	else
		return NULL;
}


RCODE uaDlgNewTxList(IN UaDlg _this)
{
	RCODE rCode=RC_OK;
	int txnum=0,txpos=0;

	if(_this!= NULL){
		if(_this->txStruct != NULL){
			dxLstFree(_this->txStruct,(void (*)(void *))sipTxFree);
			_this->txStruct=NULL;
		}
		_this->txStruct=dxLstNew(DX_LST_POINTER);
		_this->txCount=0;
		_this->bRelease=FALSE;
	}else
		rCode=UADLG_NULL;

	return rCode;
}


/*delete transaction from TxList*/
RCODE uaDlgDelTxFromTxList(IN UaDlg _dlg,IN TxStruct _tx)
{
	RCODE rCode=UATX_TX_NOT_FOUND;
	int txnum=0,txpos=0;
	TxStruct tmptx;

	if(_dlg!=NULL){
		if(_dlg->txStruct ){
			txnum=dxLstGetSize(_dlg->txStruct);
			/*search a exist tx*/
			for(txpos=0;txpos<txnum;txpos++){
				tmptx=dxLstPeek(_dlg->txStruct,txpos);
				if(tmptx == _tx){
					/*remove the found tx*/
					tmptx=dxLstGetAt(_dlg->txStruct,txpos);
					if(_dlg->txCount >0)
						_dlg->txCount--;
					rCode=RC_OK;
					break;
				}
			}/*end for loop*/
		}/* end of if */
	}else{
		rCode=UADLG_NULL;
	}
	return rCode;
}

/*add transaction into TxList, since Incoming call*/
RCODE uaDlgAddTxIntoTxList(IN UaDlg _this,IN TxStruct _tx)
{
	RCODE rCode=RC_OK;

	if(_this==NULL)
		return UADLG_NULL;
	if(_tx==NULL)
		return UATX_NULL;

	rCode=dxLstPutTail(_this->txStruct,_tx);
	if(rCode ==RC_OK)
		_this->txCount ++;

	sipTxSetUserData(_tx,(void*)_this);
	return rCode;
}

void uaDlgDelInviteTx(IN UaDlg _dlg)
{
	/*
	int pos,num;
	TxStruct inviteTx;
	*/
	if(_dlg == NULL){
		UaCoreERR("[uaDlgDelInviteTx] dlg is NULL!\n");
		return;
	}
	/*
	num=uaDlgGetTxCount(_dlg);
	for(pos=0;pos<num;pos++){
		inviteTx=uaDlgGetInviteTx(_dlg,TRUE);
		if(inviteTx != NULL){	
			uaDlgDelTxFromTxList(_dlg, inviteTx);
			
			sipTxFree(inviteTx);
			inviteTx = NULL;
			
		}
	}*/
}
/*decrease txCount number*/
RCODE uaDlgDecreaseTxCount(IN UaDlg _this)
{
	RCODE rCode =RC_OK;

	if(_this !=NULL){
		if(_this->txCount >0)
			_this->txCount--;
		else
			rCode=UADLG_TXCOUNT_ZERO;
	}else
		rCode=UADLG_NULL;

	return rCode;
}

/*increase txCount number*/
RCODE uaDlgIncreaseTxCount(IN UaDlg _this)
{
	RCODE rCode=RC_OK;
	if(_this !=NULL){
		_this->txCount++;
	}else
		rCode=UADLG_NULL;

	return rCode;
}

/*get txCount number*/
int 
uaDlgGetTxCount(IN UaDlg _this)
{
	if(_this !=NULL){
		return _this->txCount;
	}else/*fail to return value*/
		return -1;
}

/*get bRelease flag, if TRUE: release*/
BOOL 
uaDlgGetReleaseFlag(IN UaDlg _this)
{
	return _this->bRelease;
}

/*get local sdp, when play as Client, retrieve from request message*/
/*		 when paly as Server, retrieve from response message*/
SdpSess uaDlgGetLocalSdp(IN UaDlg _this)
{
	SdpSess sdp=NULL;
	TxStruct tx=NULL;
	int txnum=0,txpos=0;
	SipReq reqmsg=NULL,ackmsg=NULL;
	SipRsp rspmsg=NULL;

	if(_this == NULL)
		return NULL;

	txnum=dxLstGetSize(_this->txStruct);
	for(txpos=0;txpos<txnum;txpos++){
		tx=dxLstPeek(_this->txStruct,txpos);
		if(sipTxGetType(tx) == TX_CLIENT_INVITE){
			/*Client transaction, maybe contain sdp in INVITE or ACK*/
			reqmsg=sipTxGetOriginalReq(tx);
			if(reqmsg != NULL){
				sdp=uaGetReqSdp(reqmsg);
				if(sdp != NULL){
					/*find it, contained in original INVITE message*/
					return sdp;
				}
			}
			ackmsg=sipTxGetAck(tx);
			if(ackmsg !=NULL){
				sdp=uaGetReqSdp(ackmsg);
				if(sdp != NULL){
					/*find sdp, contain in ACK message*/
					return sdp;
				}
			}
		}/*client INVITE transaction*/

		if(sipTxGetType(tx) == TX_SERVER_INVITE){
			/*Server transaction, maybe contain sdp in provisional response or final response*/
			rspmsg=sipTxGetLatestRsp(tx);
			if(rspmsg != NULL){
				sdp=uaGetRspSdp(rspmsg);
				if(sdp!=NULL){
					/*find it, containted in final response*/
					return sdp;
				}
			}
		}/*server INVITE transaction*/
	}/*end for loop, check all transactions contained in dialog*/
	return sdp;
}

char* UnEscapeURI(const char* strUri)
{

    int len,j=0,i=0 ;
    char *szBuf,*stopstring;
	char buf[3];

	if (!strUri) 
		return NULL;

	len=strlen(strUri);
	szBuf=calloc(1,len+1);
 
    while ( i < len){
        if (strUri[i] != '%')
            szBuf[j++] = strUri[i++];
        else{
			buf[0]=strUri[i+1];
			buf[1]=strUri[i+2];
			buf[2]=0;
            sprintf(szBuf+j,"%c",strtol(buf,&stopstring,16));
			j++;
            i += 3;
        }

    }

     return szBuf;

}

/* dialog,REFER dialog, destination, refer-by, spd*/
CCLAPI RCODE uaDlgDialRefer(IN UaDlg _this, IN const char* _target,IN UaDlg _refdlg,IN SdpSess _sdp)
/*CCLAPI RCODE uaDlgDialRefer(IN UaDlg _this, IN const char *_dest,IN const char* _refby, IN SdpSess _sdp)*/
{
	RCODE rCode=RC_OK;
	SipReq _req=NULL,refReq=NULL;
	TxStruct inviteTx=NULL;
	UserProfile pfile=NULL;
	UaMgr	mgr=NULL,refmgr=NULL;
	UaUser  user=NULL,refuser=NULL;
	UaCfg	cfg=NULL,refcfg=NULL;
	UaContent  _content=NULL;
	char *_refby=NULL;
	/*SipReferTo *pReferTo=NULL;*/
	SipReferredBy *pReferredBy=NULL;
	TxStruct refTx=NULL;
	char _dest[256]={'\0'},buf[128]={'\0'};
	int  _destlen=256;
	char *replaceid=NULL,*uncapreplaceid=NULL;

	/*new dialog should not be NULL*/
	if((NULL==_this  )){
		UaCoreERR("[uaDlgDialRefer] dlg is NULL!\n");
		return UADLG_NULL;
	}
	/*target address and refer dialog can't NULL simtinuous*/
	if((NULL == _target) &&(NULL==_refdlg)){
		UaCoreERR("[uaDlgDialRefer] target address and refer dialog can't NULL simtinuous!\n");
		return UADLG_NULL;
	}
	mgr=uaDlgGetMgr(_this);
	if((NULL==mgr)){
		UaCoreERR("[uaDlgDialRefer] parent manager is NULL!\n");
		return UAMGR_NULL;
	}
	if(_refdlg != NULL){
		refmgr=uaDlgGetMgr(_refdlg);
		if(NULL == refmgr)
			return UAMGR_NULL;
		refuser=uaMgrGetUser(refmgr);
		if(NULL == refuser)
			return UAUSER_NULL;
		refcfg=uaMgrGetCfg(refmgr);
		if(NULL == refcfg)
			return UACFG_NULL;
	}

	user=uaMgrGetUser(mgr);
	if(( NULL==user )){
		UaCoreERR("[uaDlgDialRefer] user is NULL!\n");
		return UAUSER_NULL;
	}
	cfg=uaMgrGetCfg(mgr);
	if((NULL==cfg)){
		UaCoreERR("[uaDlgDialRefer] cfg is NULL!\n");
		return UACFG_NULL;
	}
	if(NULL == _refdlg){
		strcpy(_dest,_target);

	}else{
		if(RC_OK != uaDlgGetReferTo(_refdlg,_dest,&_destlen)){
			UaCoreERR("[uaDlgDialRefer] get referto is failed!\n");
			return UADLG_TXMSG_NOT_FOUND;
		}
		/*get replace-id, from-tag, to-tag from SipReferTo header*/

		replaceid=uaDlgGetReferToReplaceID(_refdlg);
		if(replaceid){
			/* if replace parameter in refer-to*/
			uncapreplaceid=UnEscapeURI(replaceid);
			free(replaceid);
		}
		/*find REFER transaction*/
		refTx=uaDlgGetMethodTx(_refdlg,REFER);
		if(NULL == refTx)
			return UADLG_TXMSG_NOT_FOUND;
		/*find REFER request message*/
		refReq=sipTxGetOriginalReq(refTx);
		if(NULL == refReq)
			return UADLG_TXMSG_REQ_NULL;

		pReferredBy=(SipReferredBy*)sipReqGetHdr(refReq,Referred_By);
/*		
		add check by tyhuang 200
		if(NULL == pReferredBy)
			return UADLG_TXMSG_REQ_NULL;
		_refby=pReferredBy->address->addr; 
*/

		if (!pReferredBy && !pReferredBy->address) 
			_refby=pReferredBy->address->addr;
	}

	/* construct INVITE requets message */
	if(BeSIPSURL(_dest)==TRUE)
		_this->Flag_Secure=TRUE;

	_req=uaINVITEMsg(_this,_dest,_refby,_sdp);
	if(_sdp)
		uaDlgGetInviteSDP(_this);
	else
		_this->flag_SDP=FALSE;
	/*should add replace header*/
	if(uncapreplaceid){
		sprintf(_dest,"Replaces:%s\r\n",uncapreplaceid);
		rCode=sipReqAddHdrFromTxt(_req,Replaces,_dest);
		free(uncapreplaceid);
	}

	if(_req==NULL){
		UaCoreERR("[uaDlgDialRefer] UASIPMSG_REQ_NULL");
		return UASIPMSG_REQ_NULL;
	}
	pfile=uaCreateUserProfile(cfg);
	if(pfile == NULL){
		UaCoreERR("[uaDlgDialRefer] user profile is NULL!\n");
		return UAUSER_PROFILE_NULL;
	}
	/* set dialog parameters */
	if(uaDlgGetRemoteParty(_this)==NULL)
		uaDlgSetRemoteAddr(_this,uaGetReqToAddr(_req));
	uaDlgSetRemoteTarget(_this,_dest);
	/* uaDlgSetCallID(_this,(char*)sipReqGetHdr(_req,Call_ID));*/
	uaDlgSetRequest(_this,_req);
	uaDlgSetState(_this,UACSTATE_DIALING);

	/*get message body */
	_content=uaGetReqContent(_req);

	/*uaMgrPostNewMsg(mgr,_this,UAMSG_CALLSTATE,uaGetReqCSeq(_req),uaGetReqSdp(_req));*/
	uaMgrPostNewMessage(mgr,_this,UAMSG_CALLSTATE,uaGetReqCSeq(_req),_content,NULL);
	if(_content){
		uaContentFree(_content);
		_content=NULL;
	}
	/*create a new client transaction */
	inviteTx=sipTxClientNew(_req,NULL,pfile,(void*)_this);
	uaUserProfileFree(pfile);

	if(inviteTx == NULL){
		sipReqFree(_req);
		UaCoreERR("[uaDlgDialRefer] create invite Tx fail!");
		return UATX_NULL;
	}
	/*add INVITE transaction into dialog*/
	rCode=uaDlgAddTxIntoTxList(_this,inviteTx);
	if(rCode != RC_OK)
		UaCoreERR("[uaDlgDialRefer] uaDlgAddTxIntoTxList fail!");
	if(_this->method==UNKNOWN_METHOD)
		_this->method=INVITE;
	/* set referdlg*/
	_this->pRefDlg=_refdlg;
	/* add for transfer 2004/8/25 */
	if(_refdlg){
		_refdlg->bRefDlg=TRUE;
		uaDlgAddReference( _refdlg, &(_this->pRefDlg));
	}
	/*add dialog into mgr*/
	dxLstPutTail(uaMgrGetDlgLst(mgr),_this);
	TCRPrint(TRACE_LEVEL_API,"[uaDlgDialRefer] uaDlg count = %d\n",dxLstGetSize(uaMgrGetDlgLst(mgr)));
 
	return rCode;

}

CCLAPI
RCODE uaDlgGetReferTo(IN UaDlg _refdlg,IN OUT char* _dest, IN OUT int* _destlen)
{
	RCODE rCode=RC_OK;
	TxStruct refTx=NULL;
	SipReq refReq=NULL;
	SipReferTo *pReferTo=NULL;

	/*find REFER transaction*/
	refTx=uaDlgGetMethodTx(_refdlg,REFER);
	if(NULL == refTx)
		return UADLG_TXMSG_NOT_FOUND;
	/*find REFER request message*/
	refReq=sipTxGetOriginalReq(refTx);
	if(NULL == refReq)
		return UADLG_TXMSG_REQ_NULL;
	
	/*Refer-To header*/
	pReferTo=sipReqGetHdr(refReq,Refer_To);
	if(NULL == pReferTo)
		return UADLG_TXMSG_REQ_NULL;

	if(RC_OK != uaReferToGetAddr(pReferTo,_dest,_destlen))
		return UADLG_TXMSG_NOT_FOUND;

	return rCode;
}

/* type 1: replace-id, 2: from-tag, 3: to-tag*/
char* uaDlgGetReferParameter(IN UaDlg _refdlg, IN int type)
{
	TxStruct refTx=NULL;
	SipReq refReq=NULL;
	SipReferTo *pReferTo=NULL;
	char *target=NULL;

	/*find REFER transaction*/
	refTx=uaDlgGetMethodTx(_refdlg,REFER);
	if(NULL == refTx)
		return NULL;
	/*find REFER request message*/
	refReq=sipTxGetOriginalReq(refTx);
	if(NULL == refReq)
		return NULL;
	
	/*Refer-To header*/
	pReferTo=sipReqGetHdr(refReq,Refer_To);
	if(NULL == pReferTo)
		return NULL;

	switch(type){
	case 1:/*replace-id*/
		target= uaReferToGetReplaceID(pReferTo);
		break;
	case 2:/*from-tag*/
		target = uaReferToGetFromTag(pReferTo);
		break;
	case 3:/*to-tag*/
		target = uaReferToGetToTag(pReferTo);
		break;
	default:
		break;
	}

	return target;
}

/*get replaceid*/
char* uaDlgGetReferToReplaceID(IN UaDlg _refdlg)
{
	return uaDlgGetReferParameter(_refdlg,1);
}

/*get from-tag*/
char* uaDlgGetReferToFromTag(IN UaDlg _refdlg)                                   
{
	return uaDlgGetReferParameter(_refdlg,2);
}

/*get to-tag*/
char* uaDlgGetReferToToTag(IN UaDlg _refdlg)
{
	return uaDlgGetReferParameter(_refdlg,3);
}

/* make a call to destination */
CCLAPI RCODE uaDlgDial(IN UaDlg _this, const char* _dest, SdpSess _sdp)
{
	return uaDlgDialRefer(_this,_dest,NULL,_sdp);
}

/* cancel a active call */
CCLAPI 
RCODE uaDlgCANCEL(IN UaDlg _dlg)
{
	RCODE rCode=RC_OK;
	UaMgr mgr=NULL;
	UaContent _content=NULL;
	TxStruct inviteTx=NULL,cancelTx=NULL;
	int num=0,pos=0;
	TXSTATE txState;
	SipReq cancelmsg=NULL;
	UserProfile pProfile=NULL;
	SipMethodType method;

	if (!_dlg) 
		return UADLG_NULL;

	/* modified by tyhuang 2003.6.5 .add (_dilg->callState != UACSTATE_RETRIEVING)*/
	if((_dlg->callState != UACSTATE_PROCEEDING)
	  &&(_dlg->callState != UACSTATE_RINGBACK)
	  &&(_dlg->callState != UACSTATE_RETRIEVING))
	  return UADLG_CALLSTATE_UNMATCHED;

	mgr=uaDlgGetMgr(_dlg);
	if(mgr==NULL)
		return UAMGR_NULL;

	num=dxLstGetSize(_dlg->txStruct);
	for(pos=0;pos<num;pos++){
		inviteTx=dxLstPeek((_dlg->txStruct),pos);
		if(inviteTx != NULL){
			txState=sipTxGetState(inviteTx);
			method=sipTxGetMethod(inviteTx); /*ask mac to add this function 7/30 */
			if((txState <TX_COMPLETED)&&(method==INVITE)){
				cancelmsg=uaCANCELMsg(_dlg,sipTxGetOriginalReq(inviteTx),sipTxGetLatestRsp(inviteTx));
				if(cancelmsg == NULL){
					rCode=UATX_NULL;
				}
				break;
			}
		}
	}/*end for loop*/
	
	/*create a new transaction for cancel */
	if((inviteTx!=NULL)&&(cancelmsg!=NULL)){
		pProfile=sipTxGetUserProfile(inviteTx);
		if(pProfile == NULL)
			return UATX_USER_PROFILE_NULL;
		cancelTx=sipTxClientNew(cancelmsg,(char*)sipTxGetTxid(inviteTx),pProfile,_dlg);
		if(cancelTx==NULL) 
			return UATX_NULL;
		/*add cancel transaction into dialog */
		uaDlgAddTxIntoTxList(_dlg,cancelTx);
		uaDlgSetState(_dlg,UACSTATE_CANCEL);
		/*uaMgrPostNewMsg(mgr,_dlg,UAMSG_CALLSTATE,uaGetReqCSeq(cancelmsg),uaGetReqSdp(cancelmsg));*/
		/*get message body */
		_content=uaGetReqContent(cancelmsg);
		/* post message */
		uaMgrPostNewMessage(mgr,_dlg,UAMSG_CALLSTATE,uaGetReqCSeq(cancelmsg),_content,NULL);
		if(_content)
			uaContentFree(_content);
		if(IsDlgTimerd(_dlg)==TRUE){
			uaDelTimer(uaDlgGetTimerHandle(_dlg));
			SetDlgTimerd(_dlg,FALSE);
		}
		if(IsDlgTimerd(_dlg)==FALSE)
			uaDlgSetTimerHandle(_dlg,uaAddTimer(_dlg,CANCEL_EXPIRE,64*T1));
			
	}
	return rCode;
}

/* disconnect a active call */
CCLAPI 
RCODE uaDlgDisconnect(IN UaDlg _dlg)
{
	RCODE rCode=RC_OK;
	UaMgr mgr=NULL;
	UACallStateType dlgState=UACSTATE_UNKNOWN;
	SipReq byeReq=NULL;
	UserProfile pProfile=NULL;
	TxStruct byeTx=NULL;

	/*check dialog*/
	if(_dlg==NULL)
		return UADLG_NULL;
	
	/*check call state can be answer or not */
	/* tyhuang remove (dlgState != UACSTATE_ACCEPT) 2004/6/18*/
	dlgState=uaDlgGetState(_dlg);
	if((dlgState != UACSTATE_CONNECTED)&&
	   (dlgState != UACSTATE_ONHELD)&&
	   ( dlgState !=UACSTATE_ONHOLDING)&&
	   (dlgState != UACSTATE_ACCEPT))
		return UADLG_CALLSTATE_UNMATCHED;

	/*get mgr*/
	mgr=uaDlgGetMgr(_dlg);
	if(mgr==NULL)
		return UADLG_PARENT_MGR_NULL;

	/*create a BYE request message*/
	byeReq=uaBYEMsg(_dlg,NULL);
	if(byeReq!=NULL){

		/*create UserProfile*/
		pProfile=uaCreateUserProfile(uaMgrGetCfg(mgr));
		if(pProfile == NULL)
			return UAUSER_PROFILE_NULL;

		/*create a new transaction for BYE */
		byeTx=sipTxClientNew(byeReq,NULL,pProfile,_dlg);
		uaUserProfileFree(pProfile);
		if(byeTx == NULL)
			return UATX_NULL;
		uaDlgAddTxIntoTxList(_dlg,byeTx);
		/*uaDlgSetState(_dlg,UACSTATE_DISCONNECT);*/
		
	}else
		rCode = UADLG_GEN_BYEREQ_FAIL;

	return rCode;
}

/* disconnect a active call with message body */
CCLAPI 
RCODE uaDlgDisconnectEx(IN UaDlg _dlg,IN UaContent _content)
{
	RCODE rCode=RC_OK;
	UaMgr mgr=NULL;
	UACallStateType dlgState=UACSTATE_UNKNOWN;
	SipReq byeReq=NULL;
	UserProfile pProfile=NULL;
	TxStruct byeTx=NULL;

	/*check dialog*/
	if(_dlg==NULL)
		return UADLG_NULL;
	
	/*check call state can be answer or not */
	/* tyhuang remove (dlgState != UACSTATE_ACCEPT) 2004/6/18*/
	dlgState=uaDlgGetState(_dlg);
	if((dlgState != UACSTATE_CONNECTED)&&
	   (dlgState != UACSTATE_ONHELD)&&
	   (dlgState != UACSTATE_ONHOLDING))
		return UADLG_CALLSTATE_UNMATCHED;

	/*get mgr*/
	mgr=uaDlgGetMgr(_dlg);
	if(mgr==NULL)
		return UADLG_PARENT_MGR_NULL;

	/*create a BYE request message*/
	byeReq=uaBYEMsg(_dlg,_content);
	if(byeReq!=NULL){
		/* add message body */

		/*create UserProfile*/
		pProfile=uaCreateUserProfile(uaMgrGetCfg(mgr));
		if(pProfile == NULL)
			return UAUSER_PROFILE_NULL;

		/*create a new transaction for BYE */
		byeTx=sipTxClientNew(byeReq,NULL,pProfile,_dlg);
		uaUserProfileFree(pProfile);
		if(byeTx == NULL)
			return UATX_NULL;
		uaDlgAddTxIntoTxList(_dlg,byeTx);
		/*uaDlgSetState(_dlg,UACSTATE_DISCONNECT);*/
		
	}else
		rCode = UADLG_GEN_BYEREQ_FAIL;

	return rCode;
}

/* Anser/reject/busy call */
CCLAPI 
RCODE uaDlgAnswer(IN UaDlg _dlg,IN UAAnswerType _type, IN UAStatusCode _reason, IN SdpSess _sdp )
{
	RCODE rCode=RC_OK;
	UaMgr mgr=NULL;
	UaContent _content=NULL;
	UACallStateType dlgState=UACSTATE_UNKNOWN;
	SipReq req=NULL;
	SipRsp rsp=NULL;
	TxStruct reqTx=NULL;
	SipTo *pNewTo=NULL;
	SipSessionExpires *pSE=NULL;
	int idx=0,length;
	char SEbuf[256],*contacturl=NULL;

#ifdef CCL_SERVER
	char *recroute=NULL;
	char buf[255];
		
#endif

	/*check dialog*/
	if(_dlg==NULL)
		return UADLG_NULL;
	
	/*check call state can be answer or not */
	dlgState=uaDlgGetState(_dlg);
	switch (dlgState) {
	case UACSTATE_OFFERING:
	case UACSTATE_OFFERING_REPLACE:
	case UACSTATE_UNHOLD:
	case UACSTATE_SDPCHANGING:
		break;
	default:
		return UADLG_CALLSTATE_UNMATCHED;
	}

	/*get mgr*/
	mgr=uaDlgGetMgr(_dlg);
	if(mgr==NULL)
		return UADLG_PARENT_MGR_NULL;

	/*uaDlgPrintAllTx(_dlg);*/

	/*get request message*/
	/*reqTx=uaDlgGetSpecTx(_dlg,TX_SERVER_INVITE,INVITE);*/
	/*state==offering or unhelod, it can get alive Invite Tx*/
	/*transaction is alive in 32 seconds*/
	reqTx=uaDlgGetAliveInviteTx(_dlg,TRUE);
	if(reqTx==NULL)
		return UADLG_TXMSG_NOT_FOUND;
	
	req=sipTxGetOriginalReq(reqTx);
	if(req==NULL)
		return UADLG_TXMSG_REQ_NULL;

	/*if(_dlg->totag==FALSE){
		pNewTo=uaToDupAddTag(sipReqGetHdr(req,To));
		if(pNewTo != NULL){
			sipReqAddHdr(req,To,pNewTo);
			uaToHdrFree(pNewTo);
		}
		
	}*/
	/* check if timer is set*/
	if(_reason>=sipOK){
		_dlg->totag=TRUE;
		if(IsDlgTimerd(_dlg)==TRUE){
			uaDelTimer(uaDlgGetTimerHandle(_dlg));
			SetDlgTimerd(_dlg,FALSE);	
		}
	}
	/*create a response message*/
	switch(_type){
	case UA_ANSWER_CALL:
		/* enable/disable session-timer */
		if(uaDlgGetSecureFlag(_dlg)==TRUE){
			contacturl=(char*)uaUserGetSIPSContact(uaMgrGetUser(mgr));
			if(contacturl)
				rsp=uaCreateRspFromReqEx(req,sipOK,contacturl,_sdp,uaDlgGetSETimer(_dlg));
			else{
				UaCoreERR("[uaREFERMsg] SIPS contact is not found!\n");
				rsp=NULL;
			}
		}else
			rsp=uaCreateRspFromReqEx(req,sipOK,(char*)uaMgrGetContactAddr(mgr),_sdp,uaDlgGetSETimer(_dlg));
#ifdef CCL_SERVER
		/* add record-route header if it exists*/
		recroute=uaDlgGetRspRecordRoute(_dlg);
		if(recroute && rsp){
			sprintf(buf,"Record-Route:%s",recroute);
			sipRspAddRecRouteHdr(rsp,0,buf);
		}
#endif

		if(rsp==NULL){
			rCode=UASIPMSG_RSP_NULL;
			UaCoreERR("[uaDlgAnswer] UA_ANSWER_CALL, can't create response message!\n");
		}else{
			/* set suppported */
			if(uaDlgGetSupported(_dlg)){
				unsigned char tempbuf[256];
				memset(tempbuf,0,sizeof(tempbuf));
				sprintf(tempbuf,"Supported:%s\r\n",uaDlgGetSupported(_dlg));
				sipRspAddHdrFromTxt(rsp,Supported,tempbuf);			
			}

			/* check session-timer */
			pSE=(SipSessionExpires*)sipRspGetHdr(rsp,Session_Expires);
			if(pSE){
				/* remove old timer if it exists */
				if(uaDlgGetSEType(_dlg)>0)
					uaDelTimer(uaDlgGetSEHandle(_dlg));
				/* enable timer */
				if(pSE->deltasecond>0){
					SessionExpiresAsStr(pSE,SEbuf,255,&length);
					if(strstr(SEbuf,"refresher=uas")){
						/* set timer to send update*/
						uaDlgSetSEHandle(_dlg,2,uaAddTimer(_dlg,SESSION_TIMER,pSE->deltasecond*500));
					}else{
						/* set timer to send BYE*/
						uaDlgSetSEHandle(_dlg,3,uaAddTimer(_dlg,SESSION_TIMER,pSE->deltasecond * 1000));
					}
					/* set session timer to dialog */
					uaDlgSetSETimer(_dlg,pSE->deltasecond);
				}
			}
			/* send 200 ok */
			sipTxSendRsp(reqTx,rsp);
			/* store 200ok in dialog */
			uaDlgSetResponse(_dlg,rsp);
			/* set a timer for 200 ok */
			if(IsDlgTimerd(_dlg)==FALSE)
				uaDlgSetTimerHandle(_dlg,uaAddTimer(_dlg, OK200_EXPIRE, 64*T1));
				
			/* change call state */
			uaDlgSetState(_dlg,UACSTATE_ACCEPT);
			_content=uaGetRspContent(rsp);
			uaMgrPostNewMessage(uaDlgGetMgr(_dlg),_dlg,UAMSG_CALLSTATE,uaGetRspCSeq(rsp),_content,NULL);
		}
		break;
	case UA_REJECT_CALL:
		if(_reason >=Bad_Request)
			rsp=uaCreateRspFromReq(req,_reason,NULL,_sdp);
		else
			rsp=uaCreateRspFromReq(req,Not_Acceptable_Here,NULL,_sdp);
		if(rsp == NULL){
			rCode=UASIPMSG_RSP_NULL;
			UaCoreERR("[uaDlgAnswer] UA_REJECT_CALL, can't create response message!\n");
		}else{
			uaDlgSetState(_dlg,UACSTATE_REJECT);
			_content=uaGetRspContent(rsp);
			uaMgrPostNewMessage(uaDlgGetMgr(_dlg),_dlg,UAMSG_CALLSTATE,uaGetRspCSeq(rsp),_content,rsp);
			sipTxSendRsp(reqTx,rsp);
		}
		break;
	case UA_BUSY_HERE:
		if(_reason >=Bad_Request)
			rsp=uaCreateRspFromReq(req,_reason,NULL,_sdp);
		else
			rsp=uaCreateRspFromReq(req,Busy_Here,NULL,_sdp);
		if(rsp == NULL){
			rCode=UASIPMSG_RSP_NULL;
			UaCoreERR("[uaDlgAnswer] UA_BUSY_HERE, can't create response message!\n");
		}else{
			uaDlgSetState(_dlg,UACSTATE_REJECT);
			_content=uaGetRspContent(rsp);
			uaMgrPostNewMessage(uaDlgGetMgr(_dlg),_dlg,UAMSG_CALLSTATE,uaGetRspCSeq(rsp),_content,rsp);
			sipTxSendRsp(reqTx,rsp);
		}
		break;
	/* add a answer 180 for sending a ringing -2003.6.13 tyhuang*/
	case UA_RINGING:
		rsp=uaCreateRspFromReq(req,_reason,(char*)uaMgrGetContactAddr(mgr),_sdp);
		if(rsp==NULL){
			/*discard the message */
			rCode=UASIPMSG_RSP_NULL;
			UaCoreERR("[uaProcessReqTx] Create Response NULL!\n");
		}else
			sipTxSendRsp(reqTx,rsp);
		break;
	case UA_100rel:
	{
		char buffer[20]={'\0'};

		if((_reason<200)&&(_reason>=100)){
			/* provisional response ,add require:100rel and RSeq: */
			rsp=uaCreateRspFromReq(req,_reason,(char*)uaMgrGetContactAddr(mgr),_sdp);
			
#ifdef CCL_100REL
			sprintf(buffer,"Require:100rel");
			sipRspAddHdrFromTxt(rsp,Require,buffer);
			sprintf(buffer,"RSeq:%d",uaDlgGetRSeq(_dlg));
			sipRspAddHdrFromTxt(rsp,RSeq,buffer);
#endif /* CCL_100REL */
			
		}
		if(rsp==NULL){
			/*discard the message */
			rCode=UASIPMSG_RSP_NULL;
			UaCoreERR("[uaDlgAnswerCall] Create Response NULL!\n");
		}else
			sipTxSendRsp(reqTx,rsp);
		break;
	}
	default:
		 UaCoreERR ("[uaDlgAnswer] Error! Unknow UAAnswerType!\n");
		break;
	}

	if(_content)
		uaContentFree(_content);
	return rCode;
}

/* Anser/reject/busy call with specific */
CCLAPI 
RCODE uaDlgAnswerCall(IN UaDlg _dlg,IN UAAnswerType _type, IN UAStatusCode _reason, IN UaContent _content ,IN const char* _contactaddr)
{
	RCODE rCode=RC_OK;
	UaMgr mgr=NULL;
	UACallStateType dlgState=UACSTATE_UNKNOWN;
	SipReq req=NULL;
	SipRsp rsp=NULL;
	TxStruct reqTx=NULL;
	int idx=0,length;
	char SEbuf[256];
	SipSessionExpires *pSE;

	/*check dialog*/
	if(_dlg==NULL)
		return UADLG_NULL;
	/* check contact address */
	if((_contactaddr == NULL)&&(_type==UA_REDIRECT))
		return UAMSG_CONTACT_NULL;
		
	/*check call state can be answer or not */
	dlgState=uaDlgGetState(_dlg);
	switch (dlgState) {
	case UACSTATE_OFFERING:
	case UACSTATE_OFFERING_REPLACE:
	case UACSTATE_UNHOLD:
	case UACSTATE_SDPCHANGING:
		break;
	default:
		return UADLG_CALLSTATE_UNMATCHED;
	}

	/*get mgr*/
	mgr=uaDlgGetMgr(_dlg);
	if(mgr==NULL)
		return UADLG_PARENT_MGR_NULL;

	/*uaDlgPrintAllTx(_dlg);*/

	/*get request message*/
	/*reqTx=uaDlgGetSpecTx(_dlg,TX_SERVER_INVITE,INVITE);*/
	/*state==offering or unhelod, it can get alive Invite Tx*/
	/*transaction is alive in 32 seconds*/
	reqTx=uaDlgGetAliveInviteTx(_dlg,TRUE);
	if(reqTx==NULL)
		return UADLG_TXMSG_NOT_FOUND;
	
	req=sipTxGetOriginalReq(reqTx);
	if(req==NULL)
		return UADLG_TXMSG_REQ_NULL;

	/* check if timer is set*/
	if(_reason>=sipOK){
		if(IsDlgTimerd(_dlg)==TRUE){
			uaDelTimer(uaDlgGetTimerHandle(_dlg));
			SetDlgTimerd(_dlg,FALSE);	
		}
	}

	/*create a response message*/
	switch(_type){
	case UA_ANSWER_CALL:
		/* enable/disable session-timer */
		rsp=uaCreateRspMsgEx(req,sipOK,_contactaddr,_content,uaDlgGetSETimer(_dlg));			
		if(rsp==NULL){
			rCode=UASIPMSG_RSP_NULL;
			UaCoreERR("[uaDlgAnswerCall] UA_ANSWER_CALL, can't create response message!\n");
		}else{
			/* check session-timer */
			pSE=(SipSessionExpires*)sipRspGetHdr(rsp,Session_Expires);
			if(pSE){
				/* remove old timer if it exists */
				if(uaDlgGetSEType(_dlg)>0)
					uaDelTimer(uaDlgGetSEHandle(_dlg));
				/* enable timer */
				SessionExpiresAsStr(pSE,SEbuf,255,&length);
				if(strstr(SEbuf,"refresher=uas")){
					/* set timer to send update*/
					uaDlgSetSEHandle(_dlg,2,uaAddTimer(_dlg,SESSION_TIMER,pSE->deltasecond *500));
				}else{
					/* set timer to send BYE*/
					uaDlgSetSEHandle(_dlg,3,uaAddTimer(_dlg,SESSION_TIMER,pSE->deltasecond * 1000));
				}
			}

			sipTxSendRsp(reqTx,rsp);
			uaDlgSetState(_dlg,UACSTATE_ACCEPT);
			uaMgrPostNewMessage(uaDlgGetMgr(_dlg),_dlg,UAMSG_CALLSTATE,uaGetRspCSeq(rsp),_content,NULL);
			/* store 200ok in dialog */
			uaDlgSetResponse(_dlg,rsp);
			/* set a timer 200 ok */
			if(IsDlgTimerd(_dlg)==FALSE)
				uaDlgSetTimerHandle(_dlg,uaAddTimer(_dlg, OK200_EXPIRE, 64*T1));
				
		}
		
		break;
	case UA_REJECT_CALL:
		if(_reason >=Bad_Request)
			rsp=uaCreateRspMsg(req,_reason,NULL,_content);
		else
			rsp=uaCreateRspMsg(req,Not_Acceptable_Here,NULL,_content);
		if(rsp == NULL){
			rCode=UASIPMSG_RSP_NULL;
			UaCoreERR("[uaDlgAnswerCall] UA_REJECT_CALL, can't create response message!\n");
		}else{
			sipTxSendRsp(reqTx,rsp);
			uaDlgSetState(_dlg,UACSTATE_REJECT);
			uaMgrPostNewMessage(uaDlgGetMgr(_dlg),_dlg,UAMSG_CALLSTATE,uaGetRspCSeq(rsp),_content,rsp);
		}
		break;
	case UA_BUSY_HERE:
		if(_reason >=Bad_Request)
			rsp=uaCreateRspMsg(req,_reason,NULL,_content);
		else
			rsp=uaCreateRspMsg(req,Busy_Here,NULL,_content);
		if(rsp == NULL){
			rCode=UASIPMSG_RSP_NULL;
			UaCoreERR("[uaDlgAnswerCall] UA_BUSY_HERE, can't create response message!\n");
		}else{
			sipTxSendRsp(reqTx,rsp);
			uaDlgSetState(_dlg,UACSTATE_REJECT);
			uaMgrPostNewMessage(uaDlgGetMgr(_dlg),_dlg,UAMSG_CALLSTATE,uaGetRspCSeq(rsp),_content,rsp);
		}
		break;
	
	case UA_REDIRECT:
		rsp=uaCreateRspMsg(req,_reason,_contactaddr,_content);
		uaDlgSetState(_dlg,UACSTATE_REJECT);
		uaMgrPostNewMessage(uaDlgGetMgr(_dlg),_dlg,UAMSG_CALLSTATE,uaGetRspCSeq(rsp),_content,rsp);
		if(rsp==NULL){
			/*discard the message */
			rCode=UASIPMSG_RSP_NULL;
			UaCoreERR("[uaDlgAnswerCall] Create Response NULL!\n");
		}else
			sipTxSendRsp(reqTx,rsp);
		break;
	/* add a answer 180 for sending a ringing -2003.6.13 tyhuang*/	
	case UA_RINGING:
		rsp=uaCreateRspMsg(req,_reason,_contactaddr,_content);
		if(rsp==NULL){
			/*discard the message */
			rCode=UASIPMSG_RSP_NULL;
			UaCoreERR("[uaDlgAnswerCall] Create Response NULL!\n");
		}else
			sipTxSendRsp(reqTx,rsp);
		break;
	case UA_100rel:
	{
		char buffer[20]={'\0'};
		if((_reason<200)&&(_reason>=100)){
			/* provisional response ,add require:100rel and RSeq: */
			rsp=uaCreateRspMsg(req,_reason,_contactaddr,_content);

#ifdef CCL_100REL
			sprintf(buffer,"Require:100rel");
			sipRspAddHdrFromTxt(rsp,Require,buffer);
			sprintf(buffer,"RSeq:%d",uaDlgGetRSeq(_dlg));
			sipRspAddHdrFromTxt(rsp,RSeq,buffer);
#endif /* CCL_100REL */
		}
		if(rsp==NULL){
			/*discard the message */
			rCode=UASIPMSG_RSP_NULL;
			UaCoreERR("[uaDlgAnswerCall] Create Response NULL!\n");
		}else
			sipTxSendRsp(reqTx,rsp);
		
		break;
	}
	default:
		UaCoreERR ("[uaDlgAnswerCall] Error! Unknow UAAnswerType!\n");
		break;
	}

	
	return rCode;
}

/* send a Info message */
CCLAPI RCODE uaDlgInfo(IN UaDlg _dlg, IN const char* _destination, IN UaContent _content)
{
	UaMgr mgr=NULL;
	RCODE rCode=RC_OK;
	TxStruct infoTx=NULL;
	UaUser	user=NULL;
	UaCfg	cfg=NULL;
	SipReq infoReq=NULL;
	UserProfile pProfile=NULL;

	/*new dialog should not be NULL*/
	if(NULL==_dlg  ){
		UaCoreERR("[uaDlgInfo] dlg is NULL!\n");
		return UADLG_NULL;
	}
	/*target address should not be NULL*/
	/*if(NULL == _dest){ 
		UaCoreERR("[uaDlgPublish] destination is NULL!\n");
		return UADLG_NULL;
	}*/
	mgr=uaDlgGetMgr(_dlg);
	if((NULL==mgr)){
		UaCoreERR("[uaDlgInfo] parent manager is NULL!\n");
		return UAMGR_NULL;
	}
	user=uaMgrGetUser(mgr);
	if(( NULL==user )){
		UaCoreERR("[uaDlgInfo] user is NULL!\n");
		return UAUSER_NULL;
	}
	cfg=uaMgrGetCfg(mgr);
	if((NULL==cfg)){
		UaCoreERR("[uaDlgInfo] cfg is NULL!\n");
		return UACFG_NULL;
	}
	/*create info message */
	infoReq=uaINFOMsg(_dlg,_destination,_content); 
	if(infoReq==NULL)
		return UASIPMSG_REQ_NULL;

	/*set remote information*/
	uaDlgSetRemoteAddr(_dlg,uaGetReqToAddr(infoReq));
	uaDlgSetRemoteTarget(_dlg,_destination);
	/*create UserProfile*/
	pProfile=uaCreateUserProfile(cfg);
	if(pProfile == NULL)
		return UAUSER_PROFILE_NULL;

	/*create new client transaction*/
	/*infoTx=sipTxClientNew(infoReq,NULL,pProfile,NULL);*/
	/*dialog is null,found by sam */
	infoTx=sipTxClientNew(infoReq,NULL,pProfile,(void*)_dlg);
	uaUserProfileFree(pProfile);
	if(infoTx==NULL)
		return UATX_NULL;
	
	if(rCode==RC_OK){
		uaDlgAddTxIntoTxList(_dlg,infoTx);
		uaDlgSetUserID(_dlg,uaUserGetName(uaMgrGetUser(mgr)));
	}
	/* set method */
	if(uaDlgGetMethod(_dlg)==UNKNOWN_METHOD)
		uaDlgSetMethod(_dlg,sipTxGetMethod(infoTx));
	/*add dialog into mgr if it doesnt exist*/
	if(uaMgrGetMatchDlg(mgr,uaDlgGetCallID(_dlg))==NULL)
		dxLstPutTail(uaMgrGetDlgLst(mgr),_dlg);

	return rCode;


}

/* send a Update message */
CCLAPI RCODE uaDlgUpdate(IN UaDlg _dlg, IN const char* _destination, IN UaContent _content)
{
	UaMgr mgr=NULL;
	RCODE rCode=RC_OK;
	TxStruct infoTx=NULL;
	UaUser	user=NULL;
	UaCfg	cfg=NULL;
	SipReq infoReq=NULL;
	UserProfile pProfile=NULL;

	/*new dialog should not be NULL*/
	if(NULL==_dlg  ){
		UaCoreERR("[uaDlgUpdate] dlg is NULL!\n");
		return UADLG_NULL;
	}
	/*target address should not be NULL*/
	if(NULL == _destination){ 
		UaCoreERR("[uaDlgPublish] destination is NULL!\n");
		return UADLG_NULL;
	}
	mgr=uaDlgGetMgr(_dlg);
	if((NULL==mgr)){
		UaCoreERR("[uaDlgUpdate] parent manager is NULL!\n");
		return UAMGR_NULL;
	}
	user=uaMgrGetUser(mgr);
	if(( NULL==user )){
		UaCoreERR("[uaDlgUpdate] user is NULL!\n");
		return UAUSER_NULL;
	}
	cfg=uaMgrGetCfg(mgr);
	if((NULL==cfg)){
		UaCoreERR("[uaDlgUpdate] cfg is NULL!\n");
		return UACFG_NULL;
	}
	/*create update message */
	infoReq=uaUPDATEMsg(_dlg,_destination,_content);
	if(infoReq==NULL)
		return UASIPMSG_REQ_NULL;

	/*set remote information*/
	uaDlgSetRemoteAddr(_dlg,uaGetReqToAddr(infoReq));
	/*create UserProfile*/
	pProfile=uaCreateUserProfile(cfg);
	if(pProfile == NULL)
		return UAUSER_PROFILE_NULL;

	/*create new client transaction*/
	/*infoTx=sipTxClientNew(infoReq,NULL,pProfile,NULL);*/
	/*dialog is null,found by sam */
	infoTx=sipTxClientNew(infoReq,NULL,pProfile,(void*)_dlg);
	uaUserProfileFree(pProfile);
	if(infoTx==NULL)
		return UATX_NULL;
	
	if(rCode==RC_OK){
		uaDlgAddTxIntoTxList(_dlg,infoTx);
		uaDlgSetUserID(_dlg,uaUserGetName(uaMgrGetUser(mgr)));
	}

	return rCode;
}

/* send a PRAck message */
CCLAPI RCODE uaDlgPRAck(IN UaDlg _dlg, IN const char* _destination, IN UaContent _content,IN unsigned int rseq)
{
	UaMgr mgr=NULL;
	RCODE rCode=RC_OK;
	TxStruct infoTx=NULL;
	UaUser	user=NULL;
	UaCfg	cfg=NULL;
	SipReq infoReq=NULL;
	UserProfile pProfile=NULL;

	/*new dialog should not be NULL*/
	if(NULL==_dlg  ){
		UaCoreERR("[uaDlgPRAck] dlg is NULL!\n");
		return UADLG_NULL;
	}
	/*target address should not be NULL*/
	if(NULL == _destination){ 
		UaCoreERR("[uaDlgPRAck] destination is NULL!\n");
		return UADLG_NULL;
	}
	mgr=uaDlgGetMgr(_dlg);
	if((NULL==mgr)){
		UaCoreERR("[uaDlgPRAck] parent manager is NULL!\n");
		return UAMGR_NULL;
	}
	user=uaMgrGetUser(mgr);
	if(( NULL==user )){
		UaCoreERR("[uaDlgPRAck] user is NULL!\n");
		return UAUSER_NULL;
	}
	cfg=uaMgrGetCfg(mgr);
	if((NULL==cfg)){
		UaCoreERR("[uaDlgPRAck] cfg is NULL!\n");
		return UACFG_NULL;
	}
	/*create PRACK message */
	infoReq=uaPRACKMsg(_dlg,_destination,_content,rseq); 
	if(infoReq==NULL)
		return UASIPMSG_REQ_NULL;

	/*set remote information*/
	uaDlgSetRemoteAddr(_dlg,uaGetReqToAddr(infoReq));
	/*create UserProfile*/
	pProfile=uaCreateUserProfile(cfg);
	if(pProfile == NULL)
		return UAUSER_PROFILE_NULL;

	/*create new client transaction*/
	/*infoTx=sipTxClientNew(infoReq,NULL,pProfile,NULL);*/
	/*dialog is null,found by sam */
	infoTx=sipTxClientNew(infoReq,NULL,pProfile,(void*)_dlg);
	uaUserProfileFree(pProfile);
	if(infoTx==NULL)
		return UATX_NULL;
	
	if(rCode==RC_OK){
		uaDlgAddTxIntoTxList(_dlg,infoTx);
		uaDlgSetUserID(_dlg,uaUserGetName(uaMgrGetUser(mgr)));
	}

	return rCode;
}
/* send a ack message */
RCODE uaDlgAck(IN UaDlg _dlg, IN const char* _destination, IN SdpSess _content)
{
	UaMgr mgr=NULL;
	TxStruct ackTx=NULL;
	UaUser	user=NULL;
	UaCfg	cfg=NULL;
	SipReq ackMsg=NULL;
	UserProfile pProfile=NULL;

	/*new dialog should not be NULL*/
	if(NULL==_dlg  ){
		UaCoreERR("[uaDlgACK] dlg is NULL!\n");
		return UADLG_NULL;
	}
	/*if(NULL==_content){
		UaCoreERR("[uaDlgACK] SDP is NULL!\n");
		return UASDP_NULL;
	}*/
	
	mgr=uaDlgGetMgr(_dlg);
	if((NULL==mgr)){
		UaCoreERR("[uaDlgACK] parent manager is NULL!\n");
		return UAMGR_NULL;
	}
	user=uaMgrGetUser(mgr);
	if(( NULL==user )){
		UaCoreERR("[uaDlgACK] user is NULL!\n");
		return UAUSER_NULL;
	}
	cfg=uaMgrGetCfg(mgr);
	if((NULL==cfg)){
		UaCoreERR("[uaDlgACK] cfg is NULL!\n");
		return UACFG_NULL;
	}
	/*create registration message */
	ackMsg=uaACKMsgEx(_dlg,_content);
	if(ackMsg == NULL){
		UaCoreERR("[uaDlgACK] ack message is NULL!\n");
		return UASIPMSG_GENACK_FAIL;
	}
	/*construct ack message */
	
	pProfile=uaCreateUserProfile(uaMgrGetCfg(mgr));
	/*create a new client transaction */
	ackTx=sipTxClientNew(ackMsg,NULL,pProfile,(void*)_dlg);
	uaUserProfileFree(pProfile);

	if(ackTx == NULL){
		sipReqFree(ackMsg);
		UaCoreERR("[uaDlgACK] create ack Tx fail!");
		return UATX_NULL;
	}
	sipTxFree(ackTx);

	/* reset flag */
	if(_content)
		_dlg->flag_SDP=FALSE;

	return RC_OK;

}

CCLAPI 
RCODE uaDlgHold(IN UaDlg _dlg)
{
	RCODE rCode=RC_OK;
	SdpSess origsdp=NULL;
	SipReq holdReq=NULL;
	SdpSess holdSdp=NULL;
	UserProfile pfile=NULL;
	TxStruct reInvTx=NULL;
	UaMgr	mgr=NULL;
	UaUser  user=NULL;
	UaCfg	cfg=NULL;
	UaContent _content=NULL;
	int	tmpsetimer;

	/*check current state is UACSTATE_CONNECTED or not*/
	if(uaDlgGetState(_dlg) != UACSTATE_CONNECTED)
		return UADLG_CALLSTATE_UNMATCHED;

	/*get mgr*/
	mgr=uaDlgGetMgr(_dlg);
	if(mgr==NULL)
		return UAMGR_NULL;

	/*get user*/
	user=uaMgrGetUser(mgr);
	if(user == NULL)
		return UAUSER_NULL;

	/*get config*/
	cfg=uaMgrGetCfg(mgr);
	if(cfg==NULL)
		return UACFG_NULL;

	/*get original sdp,if Client-->From INVITE, Server-->Final Response*/
	origsdp=uaDlgGetLocalSdp(_dlg);

	/*create a new holdSdp,here based on rfc2543*/
	/*holdSdp=uaSDPHoldSDP(origsdp,0);*/
	/* create a new holdSdp,here based on rfc3261 */

	holdSdp=uaSDPHoldSDP(origsdp,uaUserGetHoldVersion(user)); 
	if(origsdp)
		uaSDPFree(origsdp);
	if(holdSdp==NULL)
		return UADLG_ORIGSDP_NULL;
	 
	/*create a new re-INVITE message*/
	tmpsetimer=uaDlgGetSETimer(_dlg);
	uaDlgSetSETimer(_dlg,0);
	holdReq=uaReINVITEMsg(_dlg,holdSdp);
	uaDlgSetSETimer(_dlg,tmpsetimer);
	if(holdSdp)
		uaDlgGetInviteSDP(_dlg);
	else
		_dlg->flag_SDP=FALSE;

	if(holdReq==NULL){
		
		uaSDPFree(holdSdp);
		return UADLG_GEN_REINV_FAIL;
	}

	/*create user profile*/
	pfile=uaCreateUserProfile(cfg);
	if(pfile == NULL){
		sipReqFree(holdReq);
		uaSDPFree(holdSdp);
		return UAUSER_PROFILE_NULL;
	}
	/*create a new transaction and re-send invite message*/
	reInvTx=sipTxClientNew(holdReq,NULL,pfile,(void*)_dlg);
	uaUserProfileFree(pfile);

	if(reInvTx == NULL){
		sipReqFree(holdReq);
		uaSDPFree(holdSdp);
		return UATX_NULL;
	}

	/*add re-INVITE transaction into dialog*/
	rCode=uaDlgAddTxIntoTxList(_dlg,reInvTx);

	uaDlgSetState(_dlg,UACSTATE_ONHOLDING);
	/*uaMgrPostNewMsg(mgr,_dlg,UAMSG_CALLSTATE,uaGetReqCSeq(holdReq),uaGetReqSdp(holdReq));*/
	/*get message body */
	_content=uaGetReqContent(holdReq);
	/* post message */
	uaMgrPostNewMessage(mgr,_dlg,UAMSG_CALLSTATE,uaGetReqCSeq(holdReq),_content,NULL);
	if(_content){
		uaContentFree(_content);
		_content=NULL;
	}
	if(holdSdp){
		uaSDPFree(holdSdp);
		holdSdp=NULL;
	}
	return rCode;
}

/*retrieve a held call, call state == UACSTATE_ONHELD*/
CCLAPI 
RCODE uaDlgUnHold(IN UaDlg _dlg,IN SdpSess _sdp)
{
	RCODE rCode=RC_OK;
	SipReq unHoldReq=NULL;
	UserProfile pfile=NULL;
	TxStruct reInvTx=NULL;
	UaMgr	mgr=NULL;
	UaUser  user=NULL;
	UaCfg	cfg=NULL;
	UaContent _content=NULL;


	/*check current call state, should be held*/
	if(uaDlgGetState(_dlg)!=UACSTATE_ONHELD)
		return UADLG_CALLSTATE_UNHELD;

	/*get mgr*/
	mgr=uaDlgGetMgr(_dlg);
	if(mgr==NULL)
		return UAMGR_NULL;

	/*get user*/
	user=uaMgrGetUser(mgr);
	if(user == NULL)
		return UAUSER_NULL;

	/*get config*/
	cfg=uaMgrGetCfg(mgr);
	if(cfg==NULL)
		return UACFG_NULL;

	/*create a new INVITE request message*/
	unHoldReq=uaReINVITEMsg(_dlg,_sdp);
	if(_sdp)
		uaDlgGetInviteSDP(_dlg);
	else
		_dlg->flag_SDP=FALSE;

	if(unHoldReq == NULL){
		return UADLG_GEN_REINV_FAIL;
	}
	/*create user profile*/
	pfile=uaCreateUserProfile(cfg);
	if(pfile == NULL)
		return UAUSER_PROFILE_NULL;

	/*create new transaction and send INVITE message*/
	reInvTx=sipTxClientNew(unHoldReq,NULL,pfile,(void*)_dlg);
	uaUserProfileFree(pfile);

	if(reInvTx == NULL){
		sipReqFree(unHoldReq);
		return UATX_NULL;
	}
	/*add re-INVITE transaction into dialog*/
	rCode=uaDlgAddTxIntoTxList(_dlg,reInvTx);

	/*uaDlgSetState(_dlg,UACSTATE_DIALING);modified by tyhuang 2003.6.5 change call state to re-Invite*/
	uaDlgSetState(_dlg,UACSTATE_RETRIEVING);
	/*uaMgrPostNewMsg(mgr,_dlg,UAMSG_CALLSTATE,uaGetReqCSeq(unHoldReq),uaGetReqSdp(unHoldReq));*/
	/*get message body */
	_content=uaGetReqContent(unHoldReq);
	/* post message */
	uaMgrPostNewMessage(mgr,_dlg,UAMSG_CALLSTATE,uaGetReqCSeq(unHoldReq),_content,NULL);
	if(_content)
		uaContentFree(_content);
	return rCode;
}


/*send a reinvite messgae, call state == UACSTATE_CONNECTED*/
CCLAPI 
RCODE uaDlgChangeSDP(IN UaDlg _dlg,IN SdpSess _sdp)
{
	RCODE rCode=RC_OK;
	SipReq ReInvreq=NULL;
	UserProfile pfile=NULL;
	TxStruct reInvTx=NULL;
	UaMgr	mgr=NULL;
	UaUser  user=NULL;
	UaCfg	cfg=NULL;

	/*check current call state, should be connected*/
	if(uaDlgGetState(_dlg)!=UACSTATE_CONNECTED)
		return UADLG_CALLSTATE_UNMATCHED;

	/*get mgr*/
	mgr=uaDlgGetMgr(_dlg);
	if(mgr==NULL)
		return UAMGR_NULL;

	/*get user*/
	user=uaMgrGetUser(mgr);
	if(user == NULL)
		return UAUSER_NULL;

	/*get config*/
	cfg=uaMgrGetCfg(mgr);
	if(cfg==NULL)
		return UACFG_NULL;

	/*create a new INVITE request message*/
	ReInvreq=uaReINVITEMsg(_dlg,_sdp);
	if(_sdp)
		uaDlgGetInviteSDP(_dlg);
	else
		_dlg->flag_SDP=FALSE;

	if(ReInvreq == NULL){
		return UADLG_GEN_REINV_FAIL;
	}
	/*create user profile*/
	pfile=uaCreateUserProfile(cfg);
	if(pfile == NULL)
		return UAUSER_PROFILE_NULL;

	/*create new transaction and send INVITE message*/
	reInvTx=sipTxClientNew(ReInvreq,NULL,pfile,(void*)_dlg);
	uaUserProfileFree(pfile);

	if(reInvTx == NULL){
		sipReqFree(ReInvreq);
		return UATX_NULL;
	}
	/*add re-INVITE transaction into dialog*/
	rCode=uaDlgAddTxIntoTxList(_dlg,reInvTx);

	return rCode;
}

/* UnAttend transfer call */
CCLAPI 
RCODE uaDlgUnAttendTransfer(IN UaDlg _dlg, IN const char* _dest)
{
	RCODE rCode=RC_OK;
	UACallStateType dlgState=UACSTATE_UNKNOWN;
	SipReq refReq=NULL;
	TxStruct refTx=NULL;
	UserProfile pfile=NULL;
	UaMgr	mgr=NULL;
	UaUser  user=NULL;
	UaCfg	cfg=NULL;
	UaContent _content=NULL;

	if(NULL == _dlg)
		return UADLG_NULL;
	if(NULL == _dest)
		return RC_ERROR;
	
	mgr=uaDlgGetMgr(_dlg);
	if(NULL == mgr)
		return UAMGR_NULL;

	user = uaMgrGetUser(mgr);
	if(NULL == user)
		return UAUSER_NULL;

	cfg=uaMgrGetCfg(mgr);
	if(NULL == cfg)
		return UACFG_NULL;

	/*check call state is on  UACSTATE_CONNECTED*/
	dlgState = uaDlgGetState(_dlg);
	if((UACSTATE_CONNECTED != dlgState) && (UACSTATE_ONHELD != dlgState))
		return UADLG_CALLSTATE_UNMATCHED;

	/*create a new REFER message to destination */
	refReq=uaREFERMsg(_dlg,NULL,_dest);
	if(NULL == refReq   )
		return UADLG_GEN_REFREQ_FAIL;

	pfile = uaCreateUserProfile(cfg);
	if(NULL == pfile)
		return UAUSER_PROFILE_NULL;

	/*create a new transaction to terminate the call*/
	refTx = sipTxClientNew(refReq,NULL,pfile,(void*)_dlg);
	uaUserProfileFree(pfile);

	if(NULL == refTx){
		sipReqFree(refReq);
		return UATX_NULL;
	}
	/* Add REFER transaction into dialog*/
	rCode=uaDlgAddTxIntoTxList(_dlg,refTx);

	/*uaMgrPostNewMsg(mgr,_dlg,UAMSG_CALLSTATE,uaGetReqCSeq(refReq),uaGetReqSdp(refReq));*/
	/*get message body */
	_content=uaGetReqContent(refReq);
	/* post message */
	uaMgrPostNewMessage(mgr,_dlg,UAMSG_UNATTENDED_TRANSFER,uaGetReqCSeq(refReq),_content,NULL);
	if(_content)
		uaContentFree(_content);

	/*disconnect the original call*/
	/* rCode=uaDlgDisconnect(_dlg); marked by ljchuang 2003/8/20 */

	return rCode;
}

/* Attend Transfer call */
/* A <--orig dialog--> B <-- second dialog -->C */
/* A <--new dialog---> C */
/* both orig and target dialg is in UACSTATE_ONHELD */
/* orig dialog will be replace with a new dialog */
CCLAPI 
RCODE uaDlgAttendTransfer(IN UaDlg _origdlg, IN UaDlg _secdlg,IN UaDlg _refdlg)
{
	RCODE rCode=RC_OK;
	UACallStateType origState=UACSTATE_UNKNOWN,secState=UACSTATE_UNKNOWN;
	SipReq refReq=NULL;
	TxStruct refTx=NULL;
	UserProfile pfile=NULL;
	UaMgr	origmgr=NULL,secmgr=NULL,refmgr=NULL;
	UaUser  origuser=NULL,secuser=NULL,refuser=NULL;
	UaCfg	origcfg=NULL,seccfg=NULL,refcfg=NULL;
	UaContent _content=NULL;
	int  buflen=512;
	char buf[512]={'\0'};

	if((NULL == _origdlg) ||(NULL==_secdlg) || (NULL==_refdlg) )
		return UADLG_NULL;

	origmgr=uaDlgGetMgr(_origdlg);
	secmgr=uaDlgGetMgr(_secdlg);
	refmgr=uaDlgGetMgr(_refdlg);
	if((NULL == origmgr)||(NULL==secmgr)||(NULL==refmgr))
		return UAMGR_NULL;

	origuser = uaMgrGetUser(origmgr);
	secuser = uaMgrGetUser(secmgr);
	refuser = uaMgrGetUser(refmgr);
	if((NULL == origuser)||(NULL==secuser) || (NULL ==refuser))
		return UAUSER_NULL;

	origcfg=uaMgrGetCfg(origmgr);
	seccfg =uaMgrGetCfg(secmgr);
	refcfg =uaMgrGetCfg(refmgr);
	if((NULL == origcfg) || (NULL== seccfg)||(NULL==refcfg))
		return UACFG_NULL;

	/*check call state is on  UACSTATE_CONNECTED*/
	origState = uaDlgGetState(_origdlg);
	secState  = uaDlgGetState(_secdlg);
	if((UACSTATE_ONHELD != origState) ||(UACSTATE_ONHELD != secState))
		return UADLG_CALLSTATE_UNMATCHED;

	/*create a new REFER message to destination */
	rCode=uaCreateReferToMsg(_secdlg,buf,&buflen);
	if(RC_OK != rCode){
		return UADLG_GEN_REFREQ_FAIL;
	}
	refReq=uaREFERMsg(_origdlg,NULL,buf);/*refer-to should not be NULL*/
	if(NULL == refReq   )
		return UADLG_GEN_REFREQ_FAIL;

	pfile = uaCreateUserProfile(origcfg);
	if(NULL == pfile)
		return UAUSER_PROFILE_NULL;

	/*change into a new Call-ID */
	/*VONTEL didn't accept a new dialog for REFER and NOTIFY transaction*/
/*	laddr=(char*)uaUserGetLocalAddr(origuser);
	strcpy(ip,laddr);
	sprintf(buf,"Call-ID:%s\r\n",sipCallIDGen((char*)uaUserGetLocalHost(origuser),ip));
	sipReqAddHdrFromTxt(refReq,Call_ID,buf);
*/

	/*create a new transaction and add new REFER dialog*/
	refTx = sipTxClientNew(refReq,NULL,pfile,(void*)_refdlg);
	uaUserProfileFree(pfile);

	if(NULL == refTx){
		sipReqFree(refReq);
		return UATX_NULL;
	}

	/* Add REFER transaction into dialog*/
	rCode=uaDlgAddTxIntoTxList(_refdlg,refTx);

	/*uaDlgSetState(_refdlg,UACSTATE_REFER);*/
	/*uaMgrPostNewMsg(refmgr,_refdlg,UAMSG_CALLSTATE,uaGetReqCSeq(refReq),uaGetReqSdp(refReq));*/
	/*get message body */
	_content=uaGetReqContent(refReq);
	/* post message */
	uaMgrPostNewMessage(uaDlgGetMgr(_origdlg),_origdlg,UAMSG_UNATTENDED_TRANSFER,uaGetReqCSeq(refReq),_content,NULL);
	/*uaMgrPostNewMessage(refmgr,_refdlg,UAMSG_REFER,uaGetReqCSeq(refReq),_content,NULL); marked by tyhuang 2004/6/29 */
	if(_content)
		uaContentFree(_content);

	/*disconnect the original call*/
	/*rCode=uaDlgDisconnect(_origdlg); marked by ljchuang 2003/8/20 */

	return rCode;
}

/*Get remote party address, get from From ,To header or contact*/
CCLAPI 
const char* uaDlgGetRemoteParty(IN UaDlg _dlg)
{
	char *user=NULL;
/*	TxStruct tx=NULL;
	TXTYPE txType=TX_NON_ASSIGNED;
*/
	if(NULL == _dlg)
		return NULL;
	if(_dlg->remoteaddr)
		return _dlg->remoteaddr;
	/*check INVITE transaction*/
/*	tx=uaDlgGetInviteTx(_dlg,TRUE);
	if(NULL!=tx){
		txType=sipTxGetType(tx);
		if(TX_CLIENT_INVITE == txType){
*/			/*get from request-URI */
/*			user = (char*)uaGetReqReqURI(sipTxGetOriginalReq(tx));
*/			/*get from To header*/
			/*user=uaGetReqToAddr(sipTxGetOriginalReq(tx));*/
/*		}else if(TX_SERVER_INVITE == txType){
*/			/*get from From header*/
/*			user=uaGetReqFromAddr(sipTxGetOriginalReq(tx));
		}
		if(user != NULL)
			return user;
	}
*/	/*check NO-INVITE Transaction*/
/*	tx=uaDlgGetInviteTx(_dlg,FALSE);
	if(NULL != tx){
		txType=sipTxGetType(tx);
		if(TX_CLIENT_NON_INVITE == txType){
*/			/*get from To header*/
/*			user=uaGetReqToAddr(sipTxGetOriginalReq(tx));
		}else if(TX_SERVER_NON_INVITE == txType){
*/			/*get from From header*/
/*			user=uaGetReqFromAddr(sipTxGetOriginalReq(tx));
		}
		if(user != NULL)
			return user;
	}

*/	return user;

}

/*2003.5.9 add by tyhuang: Get remote party displayname, get from From header*/
CCLAPI 
const char* uaDlgGetRemotePartyDisplayname(IN UaDlg _dlg)
{
	char *displayname=NULL;
	TxStruct tx=NULL;
	TXTYPE txType=TX_NON_ASSIGNED;

	if(NULL == _dlg)
		return NULL;
	
	if(_dlg->remotedisplayname)
		return _dlg->remotedisplayname;

	/*check INVITE transaction*/
	tx=uaDlgGetInviteTx(_dlg,TRUE);
	if(NULL!=tx){
		txType=sipTxGetType(tx);
		if(TX_CLIENT_INVITE == txType){
			/*get from To header */
			displayname=(char*)uaGetReqToDisplayname(sipTxGetOriginalReq(tx));
		}else if(TX_SERVER_INVITE == txType){
			/*get from From header*/
			displayname=(char*)uaGetReqFromDisplayname(sipTxGetOriginalReq(tx));
		}
		if(displayname != NULL)
			return displayname;
	}
	/*check NO-INVITE Transaction*/
	tx=uaDlgGetInviteTx(_dlg,FALSE);
	if(NULL != tx){
		txType=sipTxGetType(tx);
		if(TX_CLIENT_NON_INVITE == txType){
			/*get from To header */
			displayname=(char*)uaGetReqToDisplayname(sipTxGetOriginalReq(tx));
		}else if(TX_SERVER_NON_INVITE == txType){
			/*get from From header*/
			displayname=(char*)uaGetReqFromDisplayname(sipTxGetOriginalReq(tx));
		}
		if(displayname != NULL)
			return displayname;
	}
	return displayname;

}

/* if TRUE --> remote tag, FALSE --> local tag */
CCLAPI 
const char* uaDlgGetRemoteTag(IN UaDlg _dlg,IN BOOL _bremote)
{
	char *tag=NULL;
	TxStruct tx=NULL;
	TXTYPE txType=TX_NON_ASSIGNED;


	if(NULL == _dlg)
		return NULL;
	
	/*check INVITE transaction*/
	tx=uaDlgGetInviteTx(_dlg,TRUE);
	if(NULL!=tx){
		txType=sipTxGetType(tx);
		if(TX_CLIENT_INVITE == txType){
			if(_bremote){
				/*get remote Tag --> To tag*/
				tag=uaGetRspToTag(sipTxGetLatestRsp(tx));
			}else{
				/*get local Tag --> From tag*/
				tag=uaGetReqFromTag(sipTxGetOriginalReq(tx));
			}

		}else if(TX_SERVER_INVITE == txType){
			if(_bremote){
				/*get remote Tag --> From tag*/
				tag=uaGetReqFromTag(sipTxGetOriginalReq(tx));
			}else{
				/*get local Tag --> From tag*/
				tag=uaGetRspToTag(sipTxGetLatestRsp(tx));
			}
		}
		if(tag != NULL)
			return tag;
	}
	/*check NO-INVITE Transaction*/
	tx=uaDlgGetInviteTx(_dlg,FALSE);
	if(NULL != tx){
		txType=sipTxGetType(tx);
		if(TX_CLIENT_NON_INVITE == txType){
			if(_bremote){ 
				/*get remote Tag --> To tag*/
				tag=uaGetRspToTag(sipTxGetLatestRsp(tx));
			}else{
				/*get local Tag --> From tag*/
				tag=uaGetReqFromTag(sipTxGetOriginalReq(tx));
			}
			if(NULL != tag)
				return tag;
			
		}else if(TX_SERVER_NON_INVITE == txType){
			if(_bremote){
				/*get remote Tag --> From tag*/
				tag=uaGetReqFromTag(sipTxGetOriginalReq(tx));
			}else{
				/*get local Tag --> From tag*/
				tag=uaGetRspToTag(sipTxGetLatestRsp(tx));
			}
		}
		if(tag != NULL)
			return tag;
	}

	return tag;
}

/*Get local party address, get from From or To header*/
CCLAPI 
const char* uaDlgGetLocalParty(IN UaDlg _dlg)
{
	char *user=NULL;
	TxStruct tx=NULL;
	TXTYPE txType=TX_NON_ASSIGNED;

	if(NULL == _dlg)
		return NULL;

	/*check INVITE transaction*/
	tx=uaDlgGetInviteTx(_dlg,TRUE);
	if(NULL!=tx){
		txType=sipTxGetType(tx);
		if(TX_CLIENT_INVITE == txType){
			/*get from From header*/
			user=uaGetReqFromAddr(sipTxGetOriginalReq(tx));
		}else if(TX_SERVER_INVITE == txType){
			/*get from request-URI */
			/*user = (char*)uaGetReqReqURI(sipTxGetOriginalReq(tx));*/
			/*get from To header*/
			user=uaGetReqToAddr(sipTxGetOriginalReq(tx));
		}
		if(user != NULL)
			return user;
	}
	/*check NO-INVITE Transaction*/
	tx=uaDlgGetInviteTx(_dlg,FALSE);
	if(NULL != tx){
		txType=sipTxGetType(tx);
		if(TX_CLIENT_NON_INVITE == txType){
			/*get from From header*/
			user=uaGetReqFromAddr(sipTxGetOriginalReq(tx));
		}else if(TX_SERVER_NON_INVITE == txType){
			/*get from To header*/
			user=uaGetReqToAddr(sipTxGetOriginalReq(tx));
			
		}
		if(user != NULL)
			return user;
	}

	return user;
}


CCLAPI BOOL IsDlgReleased(IN UaDlg _dlg)
{
	if (_dlg != NULL)
		return _dlg->bRelease;
	return TRUE;
}

CCLAPI 
RCODE uaDlgSetRSeq(IN UaDlg _dlg,IN unsigned int rseq)
{
	if(_dlg==NULL)
		return RC_ERROR;
	_dlg->rseq=rseq;
	return RC_OK;

}
CCLAPI 
unsigned int uaDlgGetRSeq(IN UaDlg _dlg)
{
	if(_dlg){
		_dlg->rseq=_dlg->rseq+1;
		return _dlg->rseq;
	}
	return 0;
}


BOOL IsDlgTimerd(IN UaDlg _dlg)
{
	if (_dlg )
		return _dlg->bTimer;
	return FALSE;
}

RCODE SetDlgTimerd(IN UaDlg _dlg,BOOL flag)
{
	if (_dlg){
		_dlg->bTimer=flag;
		return RC_OK;
	}
	return RC_ERROR;
}

int uaDlgGetTimerHandle(IN UaDlg _dlg)
{
	if (_dlg )
		return _dlg->hTimer;
	return -1;
}

RCODE uaDlgSetTimerHandle(IN UaDlg _dlg,int handle)
{
	if (_dlg){
		if(handle>-1) _dlg->bTimer=TRUE;
		else
			_dlg->bTimer=FALSE;
			_dlg->hTimer=handle;
			return RC_OK;
	}
	return RC_ERROR;
}

RCODE uaDlgSetRegTimerHandle(IN UaDlg _dlg,int handle)
{
	if (_dlg){
		_dlg->hRegTimer=handle;
		return RC_OK;
	}
	return RC_ERROR;
}

int uaDlgGetSEHandle(IN UaDlg _dlg)
{
	if (_dlg )
		return _dlg->sessiontimerhandle;
	return -1;
}

int uaDlgGetSEType(IN UaDlg _dlg)
{
	if (_dlg )
		return _dlg->refresher;
	return -1;
}

unsigned int uaDlgGetSETimer(IN UaDlg _dlg)
{
	if (_dlg )
		return _dlg->sessionexpires;
	return 0;
}

RCODE uaDlgSetSETimer(IN UaDlg _dlg,unsigned int timer)
{
	unsigned int tse;
	if(timer>1800) tse=1800;
	else if(timer < 1 ) tse=0;
	else
		tse=timer;
	if (_dlg ){
		 _dlg->sessionexpires=tse;
		 return RC_OK;
	}
	return RC_ERROR;
}
RCODE uaDlgSetSEHandle(IN UaDlg _dlg,int setype,int handle)
{
	if (_dlg){
		if (_dlg->refresher>-1) 
			_dlg->refresher=setype;
		_dlg->sessiontimerhandle=handle;
		return RC_OK;
	}
	return RC_ERROR;
}

CCLAPI 
RCODE uaDlgSetRefreshMethod(IN UaDlg _dlg,IN SipMethodType method)
{
	if(!_dlg)
		return RC_ERROR;
	if(method==INVITE)
		_dlg->refreshmethod=method;
	else
		_dlg->refreshmethod=SIP_UPDATE;
	return RC_OK;
}

SipMethodType uaDlgGetRefreshMethod(IN UaDlg _dlg)
{
	if(!_dlg)
		return UNKNOWN_METHOD;
	else
		return _dlg->refreshmethod;
}
/*get dialog status code --> for INVITE Transaction*/
CCLAPI UAStatusCode uaDlgGetLastestStatusCode(IN UaDlg _dlg)
{
	UAStatusCode statusCode=UnKnown_Status;
	SipRsp rsp=NULL;
/*	TxStruct tx=NULL;
	DxLst txList=NULL;
	int txnum=0;

	tx=uaDlgGetAliveInviteTx(_dlg,TRUE);

	if(NULL == _dlg)
		return statusCode;

	txList=uaDlgGetTxList(_dlg);
	if(txList == NULL)
		return statusCode;
	
	if(tx==NULL){
		txnum=dxLstGetSize(txList);
		tx=dxLstPeek(txList,txnum-1);
	}
	if(NULL != tx){
		statusCode=uaGetRspStatusCode(sipTxGetLatestRsp(tx));
	}
*/
	if(_dlg){
		rsp=uaDlgGetResponse(_dlg);
		if(rsp)
			statusCode=uaGetRspStatusCode(rsp);

	}
	return statusCode;
}


RCODE uaDlgSetReqRecordRoute(IN UaDlg _this,IN const char *routeset)
{
	if(_this==NULL)
		return UADLG_NULL;
	if(routeset==NULL)
		return RC_ERROR;

	if(_this->ReqRecRoute)
		free(_this->ReqRecRoute);
	_this->ReqRecRoute=strDup(routeset);
	return RC_OK;
}


const char* uaDlgGetReqRecordRoute(IN UaDlg _this)
{
	if(_this == NULL)
		return NULL;
	return _this->ReqRecRoute;
}


RCODE uaDlgSetRspRecordRoute(IN UaDlg _this,IN const char *routeset)
{
	if(_this==NULL)
		return UADLG_NULL;
	if(routeset==NULL)
		return RC_ERROR;

	if(_this->RspRecRoute)
		free(_this->RspRecRoute);
	_this->RspRecRoute=strDup(routeset);
	return RC_OK;
}


const char* uaDlgGetRspRecordRoute(IN UaDlg _this)
{
	if(_this == NULL)
		return NULL;
	return _this->RspRecRoute;
}

char* 
uaDlgGetLocalTag(IN UaDlg _this)
{
	if(_this)
		return _this->localtag;
	else
		return NULL;
}

char* 
uaDlgGetRemoveTag(IN UaDlg _this)
{
	if(_this)
		return _this->remotetag;
	else
		return NULL;
}

RCODE 
uaDlgSetRemoveTag(IN UaDlg _this,IN const char* tag)
{
	if( NULL==_this)
		return RC_ERROR;

	if( NULL==tag)
		return RC_ERROR;

	if(_this->remotetag != NULL)
		return RC_ERROR;
	_this->remotetag=strDup(tag);
	return RC_OK;

}

RCODE 
uaDlgSetResponse(IN UaDlg _this, IN SipRsp _rsp)
{
	/* check parameter*/
	if(_this == NULL){
		UaCoreERR("[uaDlgSetResponse] dialog is null!\n");
		return UADLG_NULL;
	}
	if(_rsp == NULL){
		UaCoreERR("[uaDlgSetResponse] input response message is null!\n" );
		return UASIPMSG_RSP_NULL;
	}
	if(_this->finalresponse!=NULL)
		sipRspFree(_this->finalresponse);
	/* duplicate and set */
	_this->finalresponse=sipRspDup(_rsp);
	/* return */
	return RC_OK;

}
/* it is a duplicate function */
SipRsp uaDlgGetResponse(IN UaDlg _this)
{
	/* check parameter*/
	if(_this == NULL){
		UaCoreERR("[uaDlgGetResponse] dialog is null!\n");
		return NULL;
	}

	/* return */
	return _this->finalresponse;

}

RCODE 
uaDlgSetRequest(IN UaDlg _this, IN SipReq _req)
{
	/* check parameter*/
	if(_this == NULL){
		UaCoreERR("[uaDlgSetRequest] dialog is null!\n");
		return UADLG_NULL;
	}
	if(_req == NULL){
		UaCoreERR("[uaDlgSetRequest] input request message is null!\n" );
		return UASIPMSG_REQ_NULL;
	}
	if(_this->originalrequest!=NULL)
		sipReqFree(_this->originalrequest);
	/* duplicate and set */
	_this->originalrequest=sipReqDup(_req);
	/* return */
	return RC_OK;

}
/* to get the original request */
SipReq uaDlgGetRequest(IN UaDlg _this)
{
	/* check parameter*/
	if(_this == NULL){
		UaCoreERR("[uaDlgGetReqeust] dialog is null!\n");
		return NULL;
	}

	/* return */
	return _this->originalrequest;

}

BOOL IsDlgServer(IN UaDlg _this)
{
	if(_this)
		return _this->bServer;

	return FALSE;
}

RCODE SetDlgServer(IN UaDlg _this,BOOL flag){
	
	if(_this){
		_this->bServer=flag;
		return RC_OK;
	}
	return RC_ERROR;
}


UINT32 uaDlgGetRemoteCseq(IN UaDlg _this)
{
	if(_this)
		return _this->remotecseq;

	return 0;
}

RCODE uaDlgSetRemoteCseq(IN UaDlg _this,UINT32 cseq)
{
	
	if(_this){
		_this->remotecseq=cseq;
		return RC_OK;
	}
	return RC_ERROR;
}

void uaDlgAddReference(IN UaDlg _this, IN UaDlg* _ref)
{
	if ( _this != NULL)
		dxVectorAddElement( _this->refDlg, _ref);
}

void uaDlgRemoveReference(IN UaDlg _this, IN UaDlg* _ref)
{
	if ( _this != NULL)
		dxVectorRemoveElement( _this->refDlg, _ref);
}

UaDlg uaDlgGetRefDlg(IN UaDlg _this)
{
	if(_this)
		return _this->pRefDlg;

	return NULL;
}

void uaDlgRemoveRef(IN UaDlg _this)
{
	if(_this)
		_this->bRefDlg=FALSE;
}

RCODE uaDlgSetREFERID(IN UaDlg _this,IN char *eventid)
{
	if(!_this)
		return RC_ERROR;

	if(_this->referevent)
		free(_this->referevent);
	
	_this->referevent=strDup(eventid);
	return RC_OK;
}

BOOL  uaDlgGetSecureFlag(IN UaDlg _this)
{
	if (!_this) 
		return FALSE;
	return _this->Flag_Secure;

}

RCODE uaDlgSetSecureFlag(IN UaDlg _this)
{
	if(!_this)
		return RC_ERROR;

	_this->Flag_Secure=TRUE;
	return RC_OK;

}
/* set SIP-If-Match */
RCODE uaDlgSetSIPIfMatch(IN UaDlg _this,IN unsigned char *sipetag)
{
	if(!_this)
		return RC_ERROR;
	if(_this->sipifmatch != NULL)
		free(_this->sipifmatch);
	if(sipetag==NULL)
		_this->sipifmatch=NULL;
	else
		_this->sipifmatch=strDup(sipetag);
	return RC_OK;
}

unsigned char* uaDlgGetSIPIfMatch(IN UaDlg _this)
{
	return _this==NULL?NULL:_this->sipifmatch;
}


/* set supported */
RCODE uaDlgSetSupported(IN UaDlg _this,IN unsigned char *newsupp)
{
	if(NULL==_this)
		return RC_ERROR;

	if(_this->supported)
		free(_this->supported);

	if(NULL==newsupp)
		_this->supported=NULL;
	else
		_this->supported=strDup(newsupp);

	return RC_OK;
}

unsigned char* uaDlgGetSupported(IN UaDlg _this)
{
	if(NULL==_this)
		return NULL;
	else
		return _this->supported;
}

char* 
taggen(void)
{
	static char buf[128]={'\0'};
	static unsigned int count;
	unsigned int ltime;

	ltime=rand();
	
	sprintf(buf,"%s-%u-%u",USER_AGENT,ltime,count);
	count++;
	return buf;
}

CCLAPI RCODE uaDlgConference(IN UaDlg, const char* destination);

