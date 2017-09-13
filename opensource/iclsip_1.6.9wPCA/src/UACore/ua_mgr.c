/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * ua_mgr.c
 *
 * $Id: ua_mgr.c,v 1.185 2006/03/31 08:50:12 tyhuang Exp $
 */
#include "stdio.h"
#include "ua_mgr.h"
#include "ua_dlg.h"
#include "ua_int.h"
#include <adt/dx_vec.h>
#include <adt/dx_lst.h>
#include <sipTx/sipTx.h>
#include "ua_sipmsg.h"
#include "ua_evtpkg.h"


struct uaMgrObj{
	DxLst		uadlg;
	UaUser		users;
	UaCfg		config;
	UaDlg		regdlg;
	int		cmdseq;
	char*		contactaddr;
	char*		publicaddr; 
	char*		lastregisteraddr;/* for internal use */
	DxLst		UaEvent;/* add by tyhuang.2003.05.20 */
};

UAEvtCB g_uaEvtCB=NULL;
DxLst MgrList; /*keep all mgr objects*/

void UaCoreSipTxFree(UaDlg tmpdlg,TxStruct txData)
{
	if (tmpdlg) 
		uaDlgDelTxFromTxList(tmpdlg, txData);
	sipTxFree(txData);
}
/* Tx layer call back function */
void TxEvtCB(TxStruct txData, SipTxEvtType event)
{
	UaDlg tmpdlg;
	UaMgr tmpmgr=NULL;
	SipReq tmpReq=NULL;
	SipMethodType method=sipTxGetMethod(txData);
	RCODE rcode;
	int statuscode;
	/*  Application should take the responsibility of the
	    birth and death of the TxData		*/
	tmpdlg=(UaDlg)sipTxGetUserData(txData);
	switch (event) {
		case TX_TRANSPORT_ERROR:
			if(tmpdlg != NULL){
				/*
				rCode=uaDlgDelTxFromTxList(tmpdlg,txData);
				if(rCode!=RC_OK)
					UaCoreERR("[TxEvtCB] TX_TRANSPORT_ERROR, can't delete tx from dialog\n");
				*/

				/* for a regist message should be set as registererr */
				if(method==REGISTER)
					uaDlgSetState(tmpdlg,UACSTATE_REGISTERERR);
				else
					uaDlgSetState(tmpdlg,UACSTATE_TRANSPORTERR);

				tmpmgr=uaDlgGetMgr(tmpdlg);
				if((NULL != tmpmgr) && !IsDlgReleased(tmpdlg)){
					tmpReq=sipTxGetOriginalReq(txData);
					rcode=uaMgrPostNewMsg(tmpmgr,tmpdlg,UAMSG_TX_TRANSPORTERR,uaGetReqCSeq(tmpReq),NULL);
				}
			}
			break;

		case TX_TIMER_A:
		case TX_TIMER_E:
			
			/* mark by tyhuang 2003.12.10 */
			/*
			tmpmgr=uaDlgGetMgr(tmpdlg);
			if((NULL != tmpmgr) && !IsDlgReleased(tmpdlg)){
				tmpReq=NULL;
				tmpReq=sipTxGetOriginalReq(txData);
				rcode=uaMgrPostNewMsg(tmpmgr,tmpdlg,UAMSG_CALLSTATE,uaGetReqCSeq(tmpReq),NULL);
				if (rcode !=RC_OK) {
						UaCoreERR("[TxEvtCB] Error, Post Message Fail:%s!\n",uaCMRet2Phase(rcode));
				}
			}*/
			break;

		case TX_TIMER_B:
		case TX_TIMER_F:
		case TX_TIMER_H:
			if(NULL != tmpdlg){
				/* add for transfer 2004/8/25 */
				uaDlgRemoveRef(	tmpdlg );
				/* for a regist message should be set as registertimeout */
				if(method==REGISTER)
					uaDlgSetState(tmpdlg,UACSTATE_REGISTERTIMEOUT);
				else
					uaDlgSetState(tmpdlg,UACSTATE_TIMEOUT);

				tmpmgr=uaDlgGetMgr(tmpdlg);
				if((NULL != tmpmgr) && !IsDlgReleased(tmpdlg)){
					tmpReq=NULL;
					tmpReq=sipTxGetOriginalReq(txData);
					rcode=uaMgrPostNewMsg(tmpmgr,tmpdlg,UAMSG_TX_TIMEOUT,uaGetReqCSeq(tmpReq),NULL);
					if(sipTxGetType(txData)==TX_CLIENT_2XX){
						TCRPrintEx(TRACE_LEVEL_API,"[TxEvtCB] Not Receive ACK,so diconnect!\n");
						uaDlgDisconnect(tmpdlg);
					}
				}
			}

			break;
		case TX_BADREQ_RECEIVED:
			/* callid is null */
			tmpmgr=uaDlgGetMgr(tmpdlg);
			SendResponseEx(uaCreateRspFromReq(sipTxGetOriginalReq(txData),Bad_Request,NULL,NULL),uaCreateUserProfile(uaMgrGetCfg(tmpmgr)));
			sipTxFree(txData);
			break;
		case TX_REQ_RECEIVED:/*receive a request transaction*/
			if(sipTxGetMethod(txData)==ACK ){
				TCRPrintEx(TRACE_LEVEL_API,"[TxEvtCB] Receive ACK\n");
				rcode=uaProcessAckTx(txData);
				if(rcode!=RC_OK)
					/*UaCoreERR1 ( "[TxEvtCB] ERROR:uaProcessAckTx Fail:%s!\n",uaCMRet2Phase(rcode));*/
					UaCoreERR ( "[TxEvtCB] ERROR:uaProcessReqTx Fail!\n");
				sipTxFree(txData);
				txData=NULL;
			}else{
				TCRPrintEx(TRACE_LEVEL_API,"[TxEvtCB] Receive REQ\n");
				rcode=uaProcessReqTx(txData);
				if(rcode!=RC_OK)
					/*UaCoreERR1 ( "[TxEvtCB] ERROR:uaProcessReqTx Fail:%s!\n",uaCMRet2Phase(rcode));*/
					UaCoreERR ( "[TxEvtCB] ERROR:uaProcessReqTx Fail!\n");
				/*sipTxFree(txData);*/
			}
			break;
		case TX_RSP_RECEIVED:
			rcode=uaProcessRspTx(txData);
			if(rcode!=RC_OK)
				UaCoreERR ( "[TxEvtCB] ERROR:uaProcessRspTx Fail!\n");
			break;
		/* handle no match rsp-receive 200OK retransmit must be handle*/
		case TX_NOMATCH_RSP_RECEIVED :
			method=uaGetRspMethod(sipTxGetLatestRsp(txData));
			if (method==INVITE) {		
				rcode=uaProcessRspTx(txData);
				if(rcode!=RC_OK)
					UaCoreERR ( "[TxEvtCB] ERROR:uaProcessRspTx Fail!\n");
				if (NULL != tmpdlg) 
					uaDlgDelTxFromTxList(tmpdlg,txData);
			}
			sipTxFree(txData);
			break;
		case TX_ACK_RECEIVED:
			TCRPrintEx(TRACE_LEVEL_API,"[TxEvtCB] Receive ACK\n");
			rcode=uaProcessAckTx(txData);
			if(rcode!=RC_OK)
				UaCoreERR ("[TxEvtCB] ERROR:uaProcessAckTx Fail!\n");
			statuscode=uaGetRspStatusCode(sipTxGetLatestRsp(txData));
			if ((statuscode>199)&&(statuscode<300))
				sipTxFree(txData);
			break;
		case TX_CANCEL_RECEIVED:
			/* handle tx event for cancel,use inviteTx to send 487 */
			TCRPrintEx(TRACE_LEVEL_API,"[TxEvtCB] Receive Cancel\n");
			if (RC_OK!=uaProcessCancel(txData)) {
				UaCoreERR ("[TxEvtCB] ERROR:uaProcessAckTx Fail!\n");
			}
			break;
		case TX_TERMINATED_EVENT:
			/*
			MDlg=uaDlgMatchTx(uaMgrMatchTx(txData),txData);
			if(MDlg)
				tmpdlg=MDlg;
			*/
			tmpdlg=(UaDlg)sipTxGetUserData(txData);
			if(NULL != tmpdlg){
				tmpmgr=uaDlgGetMgr(tmpdlg);
				if((NULL != tmpmgr) && !IsDlgReleased(tmpdlg)){
					tmpReq=sipTxGetOriginalReq(txData);
					rcode=uaMgrPostNewMsg(tmpmgr,tmpdlg,UAMSG_TX_TERMINATED,uaGetReqCSeq(tmpReq),NULL);
				}
			}else{/* if dialog is null,free Tx */
				/*uaDlgDelTxFromTxList(tmpdlg, txData);*/
				sipTxFree(txData);
				break;
			}

			switch ( uaDlgGetState(tmpdlg) ) {
			case UACSTATE_DIALING:
			/*
			case UACSTATE_PROCEEDING:
			case UACSTATE_RINGBACK:
			case UACSTATE_OFFERING:
			*/
			 case UACSTATE_TIMEOUT:
			case UACSTATE_TRANSPORTERR:
			case UACSTATE_BUSY:
			case UACSTATE_REJECT:
			/*case UACSTATE_DISCONNECT:	mark by tyhuang 2004.02.25 */
				uaDlgSetState(tmpdlg,UACSTATE_DISCONNECT);
				tmpmgr=uaDlgGetMgr(tmpdlg);
				if((NULL != tmpmgr) && !IsDlgReleased(tmpdlg)){
					tmpReq=NULL;
					tmpReq=sipTxGetOriginalReq(txData);
					rcode=uaMgrPostNewMsg(tmpmgr,tmpdlg,UAMSG_CALLSTATE,uaGetReqCSeq(tmpReq),NULL);
				}
				break;
			default:
				break;
			}

			/*if((method!=REGISTER)&&(method!=INVITE)&&(method!=SIP_SUBSCRIBE)){*/
			if((method!=INVITE)&&(method!=SIP_SUBSCRIBE)){
				TCRPrint(TRACE_LEVEL_API,"[TxEvtCB] ref count : %d\n", sipTxGetRefCount(txData));
				TCRPrintEx(TRACE_LEVEL_API,"[TxEvtCB] Destroy transaction\n");
				uaDlgDelTxFromTxList(tmpdlg, txData);
				sipTxFree(txData);
				/*uaDlgDecreaseTxCount(tmpdlg);*/
			}

			if (uaDlgGetState(tmpdlg) == UACSTATE_DISCONNECT) {
				if(uaDlgGetReleaseFlag(tmpdlg)==TRUE)
					uaDlgRelease(tmpdlg);
				/* if state is equal to  UACSTATE_DISCONNECT and release flag  is 
				 * set by user, free invite transaction 
				 */
				/*if(txData && uaDlgGetReleaseFlag(tmpdlg) ){
					uaDlgDelTxFromTxList(tmpdlg, txData);
					sipTxFree(txData);
					txData = NULL;
				}*/
			}

			/*when dialog is not using any more, free dialog*/
		
			/*if((uaDlgGetTxCount(tmpdlg)==1)&&(uaDlgGetReleaseFlag(tmpdlg) == TRUE)&&(uaDlgGetState(tmpdlg) == UACSTATE_DISCONNECT)){
				uaDlgFree(tmpdlg);
			}*/
			
			break;
	}
}
/* UA SDK Initialize */
/* input callback function */
CCLAPI 
RCODE 
uaLibInit(IN UAEvtCB eventCB,IN SipConfigData config)
{
	RCODE rCode;

	if(eventCB != NULL)
		g_uaEvtCB=eventCB;
	else
		return UAMGR_INIT_CB_FUNC_NULL;

	/*rCode = uaLibInitWithStun(eventCB,config,NULL);*/
	rCode = sipTxLibInit(TxEvtCB,config,FALSE,MAX_TX_COUNT);

	if(RC_OK == rCode){
		MgrList = dxLstNew(DX_LST_POINTER);
	}

	return rCode;
}

/* UA SDK Initialize */
/* input callback function and Stun Server address */
CCLAPI 
RCODE uaLibInitWithStun(IN UAEvtCB eventCB,IN SipConfigData config,IN const char* STUNserver)
{
	RCODE rCode;

	if(eventCB != NULL)
		g_uaEvtCB=eventCB;
	else
		return UAMGR_INIT_CB_FUNC_NULL;

	/*rCode = sipTxLibInit(TxEvtCB,config,TRUE); */
	/* modified by tyhuang 2003.9.15 
	 * uacore will handle 200ok of invite
	 */
	rCode = sipTxLibInitEx(TxEvtCB,config,NULL,FALSE,MAX_TX_COUNT,STUNserver);

	if(RC_OK == rCode){
		MgrList = dxLstNew(DX_LST_POINTER);
	}

	return rCode;
}

/* clear ua core stack */
CCLAPI 
RCODE uaLibClean(void)
{
	/* change from dxLstFree.2003.07.10 by tyhuang*/
	int size,pos;
	size=dxLstGetSize(MgrList);
	for(pos=0;pos<size;pos++)
		uaMgrFree(dxLstPeek(MgrList,0));
	dxLstFree(MgrList,NULL);
	MgrList=NULL;
 	/*dxLstFree(MgrList,(void (*)(void *))uaMgrFree);*/
	/* found by Quantway */
	TCRPrintEx	(TRACE_LEVEL_API,"[uaLibClean] uaLibClean is done!start sipTxLibEnd!\n");
	return sipTxLibEnd();
}

/* dispatch ua events */
CCLAPI 
RCODE uaEvtDispatch(const int timeout)
{
	return sipTxEvtDispatch(timeout);
}

/* Manager */
/*create a new Mgr, similar to create a new device */
CCLAPI 
UaMgr uaMgrNew(void)
{
	UaMgr _this;

	_this=(UaMgr)calloc(1,sizeof(struct uaMgrObj));
	if(_this != NULL){
		_this->config=NULL;
		_this->uadlg=dxLstNew(DX_LST_POINTER);
		_this->UaEvent=NULL;
		_this->UaEvent=dxLstNew(DX_LST_POINTER);
		_this->users=NULL;
		_this->cmdseq=0;
		_this->regdlg=NULL;
		_this->contactaddr=NULL; 
		_this->publicaddr=NULL;
		dxLstPutTail(MgrList,_this);
		_this->lastregisteraddr=NULL;
		return _this;
	}else
		return NULL;

}

/*destruct a Mgr object, similar to destruct a old device */
CCLAPI 
void uaMgrFree(IN UaMgr _this)
{
	int mgrnum=0,mgrpos=0,dlgnum=0,dlgpos=0;
	UaMgr mgr=NULL;
	UaDlg dlg=NULL;
	
	if(_this != NULL){
		/*remove mgr object from list */
		mgrnum=dxLstGetSize(MgrList);
		for(mgrpos=0;mgrpos<mgrnum;mgrpos++){
			mgr=dxLstPeek(MgrList,mgrpos);
			if(mgr==_this){
				mgr=dxLstGetAt(MgrList,mgrpos);
				break;
			}
		}
		/*free dialog*/
		if(_this->uadlg != NULL){
			dlgnum=dxLstGetSize(_this->uadlg);
			TCRPrint(TRACE_LEVEL_API,"[uaMgrFree] uaDlg count = %d\n",dlgnum);
			/*get each dialog and free it*/
			for(dlgpos=0;dlgpos<dlgnum;dlgpos++){
				dlg=(UaDlg)dxLstGetAt(_this->uadlg,dlgpos);
				if(dlg !=NULL)
					uaDlgFree(dlg);
			}
			
			dxLstFree(_this->uadlg, NULL);
			/*dxLstFree(_this->uadlg,(void (*)(void *))uaDlgFree);*/
			_this->uadlg = NULL;

		}

		/* free event*/
		if(_this->UaEvent != NULL){
			dxLstFree(_this->UaEvent, (void (*)(void *))uaEvtpkgFree);
			_this->UaEvent = NULL;
		}

		/*free cfg object*/
		if(_this->config != NULL){
			uaCfgFree(_this->config);
			_this->config = NULL;
		}
		/*free regdlg, for registration dialog
		if(_this->regdlg != NULL){
			uaDlgFree(_this->regdlg);
			_this->regdlg = NULL;
		}*/
		/*free user object*/
		if(_this->users != NULL){
			uaUserFree(_this->users);
			_this->users = NULL;
		}
		/*free contactaddress parameter*/
		if(_this->contactaddr != NULL){
			free(_this->contactaddr);
			_this->contactaddr=NULL;
		}
		if(_this->publicaddr != NULL){
			free(_this->publicaddr);
			_this->publicaddr=NULL;
		}
		if(_this->lastregisteraddr!=NULL){
			free(_this->lastregisteraddr);
			_this->lastregisteraddr=NULL;
		}
		free(_this);
		_this=NULL;
	}
}

/*get dialog list */
CCLAPI 
DxLst uaMgrGetDlgLst(IN UaMgr _this)
{
	if(_this != NULL)
		return _this->uadlg;
	else
		return NULL;
}

/*get dialog list */
CCLAPI 
DxLst uaMgrGetEventLst(IN UaMgr _this)
{
	if(_this != NULL)
		return _this->UaEvent;
	else
		return NULL;
}

/*get user object */
CCLAPI 
UaUser uaMgrGetUser(IN UaMgr _this)
{
	if(_this != NULL)
		return _this->users;
	else
		return NULL;
}

/*set user setting */
CCLAPI 
RCODE uaMgrSetUser(IN UaMgr _this, IN UaUser user)
{
	RCODE rCode =RC_OK;
	char buffer[256];

	if(_this == NULL){
		UaCoreWARN("[uaMgrSetUser] mgr is  NULL!\n");
		return UAMGR_NULL;
	}
	if(user == NULL){
		UaCoreWARN("[uaMgrSetUser] user is  NULL!\n");
		return UAUSER_NULL;
	}

	if(_this->users != NULL)
		rCode=uaUserFree(_this->users);
	/*user is create by usUserNew(), not need to duplicate
	_this->users=user;*/
	/* add default sips url */
	if (uaUserGetName(user)&&uaUserGetLocalAddr(user)) {
		sprintf(buffer,"<sips:%s@%s:%d;transport=tcp",uaUserGetName(user),uaUserGetLocalAddr(user),uaUserGetLocalPort(user));
		uaUserSetContactAddr(user,buffer);
	}else
		return UAUSER_NULL;
	_this->users=uaUserDup(user);
	
	 if(_this->contactaddr != NULL){
	  free(_this->contactaddr);
	  _this->contactaddr=NULL;
	 }
	 if(_this->publicaddr != NULL){
	  free(_this->publicaddr);
	  _this->publicaddr=NULL;
	 }
	 if(_this->lastregisteraddr!=NULL){
	  free(_this->lastregisteraddr);
	  _this->lastregisteraddr=NULL;
	 }

	return rCode;
}

/*delete a user object*/
CCLAPI 
RCODE uaMgrDelUser(IN UaMgr _this)
{
	RCODE rCode;

	if(_this !=NULL){
		rCode=uaUserFree(_this->users);
		_this->users = NULL;
	}else
		rCode=UAMGR_NULL;
		
	return rCode;
}

/*get mgr user name,user object user, user->nameid */
CCLAPI 
char* uaMgrGetUserName(IN UaMgr _this)
{
	UaUser user=NULL;
	if(_this != NULL){
		user=uaMgrGetUser(_this);
		if(user != NULL)
			return (char*)uaUserGetName(user);
		else
			return NULL;
	}else
		return NULL;
}

/*Manager-config functions */
/*get config data about this manager */
CCLAPI 
UaCfg uaMgrGetCfg(IN UaMgr _this)
{
	if(_this != NULL){
		return _this->config;
	}else
		return NULL;
}

/*set config data */
CCLAPI 
RCODE uaMgrSetCfg(IN UaMgr _this, IN UaCfg cfg)
{
	RCODE rCode =RC_OK;

	if(_this == NULL){
		UaCoreWARN("[uaMgrSetCfg] mgr is  NULL!\n");
		return UAMGR_NULL;
	}
	if(cfg == NULL){
		UaCoreWARN("[uaMgrSetCfg] cfg is  NULL!\n");
		return UACFG_NULL;
	}


	if(_this->config != NULL){
		rCode=uaCfgFree(_this->config);
		_this->config=NULL;
	}
		/*since cfg is create by uaCfgNew(), not need to duplicate
		_this->config=cfg;*/
	_this->config=uaCfgDup(cfg);
	

	return rCode;
}

/*registration function */
/*register single user to registrar */
CCLAPI 
RCODE uaMgrRegister(IN UaMgr _this,IN UARegisterType _regtype,IN const char* _addr)
{
	RCODE rCode=RC_OK;
	TxStruct regTx=NULL;
	UaDlg regdlg=NULL,tmpdlg;
	int txnum=0,txpos=0;
	SipReq origReg=NULL,regReq=NULL;
	UserProfile pProfile=NULL;
	BOOL	bDone=TRUE;

	/*if original registration message exist */
	if (!_this) 
		return UAMGR_NULL;

	tmpdlg=_this->regdlg;

	if(tmpdlg==NULL){
		/*first time registration */
		regdlg=uaDlgNew(_this);
		dxLstPutTail(uaMgrGetDlgLst(_this),regdlg);
		regTx=NULL;
		if(regdlg==NULL)
			return UADLG_NULL;
		tmpdlg=regdlg;
	}else{
		/*if(uaDlgGetState(tmpdlg)==UACSTATE_REGISTERED){
			txnum=dxLstGetSize(uaDlgGetTxList(tmpdlg));
			for(txpos=0;(txpos<txnum)&&(bDone);txpos++){
				regTx=dxLstPeek(uaDlgGetTxList(tmpdlg),txpos);
			*/	/*if((sipTxGetMethod(regTx)==REGISTER)&&(sipTxGetState(regTx)==TX_TERMINATED)){*/
			/*	if((sipTxGetMethod(regTx)==REGISTER)){
					origReg=sipTxGetOriginalReq(regTx);
					bDone=FALSE;
					break;
				}
				if(bDone){
					regTx=NULL;
					tmpdlg=NULL;
				}
			
			}
			

		}*/
		origReg=uaDlgGetRequest(tmpdlg);
		regdlg=tmpdlg;
	}

	/*create registration message */
	regReq=uaREGMsg(tmpdlg,regTx,origReg,_regtype,_addr); /*dialog,original tx, original registration message*/
	if(regReq==NULL)
		return UASIPMSG_REQ_NULL;

	/*create UserProfile*/
	pProfile=uaCreateUserProfile(_this->config);
	if(pProfile == NULL)
		return UAUSER_PROFILE_NULL;

	
	/*create new client transaction*/
	regTx=sipTxClientNew(regReq,NULL,pProfile,regdlg);
	uaUserProfileFree(pProfile);
	if(regTx==NULL)
		return UATX_NULL;

	/*free original transactions contained in registration dialog */
	rCode=uaDlgNewTxList(regdlg);
	if(rCode==RC_OK){
		uaDlgAddTxIntoTxList(regdlg,regTx);
		uaDlgSetUserID(regdlg,uaUserGetName(uaMgrGetUser(_this)));
		uaDlgSetState(regdlg,UACSTATE_REGISTER);
		uaMgrPostNewMsg(uaDlgGetMgr(regdlg),regdlg,UAMSG_CALLSTATE,uaGetReqCSeq(regReq),NULL);
		/*assign the new registration dialog into transaction*/
		_this->regdlg=regdlg;
		if(_this->lastregisteraddr){
			free(_this->lastregisteraddr);
			_this->lastregisteraddr=NULL;
		}
		if(_addr)
			_this->lastregisteraddr=strDup(_addr);
		
	}
	return rCode;
}

/* Manager Dlg */
/*get total dialog number */
CCLAPI 
int uaMgrGetDlgCount(IN UaMgr _this)
{
	if(_this->uadlg!= NULL){
		return  dxLstGetSize(_this->uadlg);
	}else
		return -1;
}

/*get a dialog handle */
CCLAPI 
UaDlg uaMgrGetDlg(IN UaMgr _this,IN int pos)
{
	UaDlg dlg=NULL;
	if(_this->uadlg != NULL){
		dlg=(UaDlg)dxLstPeek(_this->uadlg,pos);
		if(dlg != NULL)
			return dlg;
	}
	return dlg;
}

/*get registration dialog*/
CCLAPI 
UaDlg uaMgrGetRegDlg(IN UaMgr _this)
{
	UaDlg dlg=NULL;
	if(_this!=NULL){
		if(_this->regdlg != NULL)
			dlg=_this->regdlg;
	}
	return dlg;
}


/*create a new dialog*/ /* not implement */
CCLAPI 
UaDlg uaMgrAddNewDlg(IN UaMgr _this)
{
	UaDlg dlg=NULL;

	dlg=uaDlgNew(_this);

	return dlg;
}

/*not destroy a dialog,cut the link to Mgr object */
CCLAPI 
RCODE uaMgrDelDlgLink(IN UaMgr _this, IN UaDlg _target)
{
	RCODE rCode=UAMGR_DLG_NOT_FOUND;
	UaDlg dlg;
	int pos;

	if (!_this || !_target) {
		return RC_ERROR;
	}

	for(pos=0;pos<dxLstGetSize(_this->uadlg);pos++){
		dlg=(UaDlg)dxLstPeek(_this->uadlg,pos);
		if(_target == dlg){
			dlg=dxLstGetAt(_this->uadlg,pos);
			TCRPrint(TRACE_LEVEL_API,"[uaMgrDelDlgLink] uaDlg count = %d\n",dxLstGetSize(_this->uadlg));
			/*uaDlgRelease(dlg);*/
			/*if(_flag)
			     uaDlgFree(dlg);*/
			rCode=RC_OK;
			break;
		}
	}
	return rCode;
}

/*destory the dialog list, free all dialogs and construct a new dialog list */
CCLAPI 
RCODE uaMgrDelAllDlg(IN UaMgr _this)
{
	RCODE rCode=UAMGR_DLG_NOT_FOUND;

	if (!_this) {
		return RC_ERROR;
	}

	if(_this->uadlg != NULL){
	/*	dxLstFree(_this->uadlg,(void (*)(void *))uaDlgFree); */
	/* tyhuang modified 2004/6/15 */
		dxLstFree(_this->uadlg,(void (*)(void *))uaDlgRelease);
		_this->uadlg = NULL;
		rCode=RC_OK;
	}
	_this->uadlg=dxLstNew(DX_LST_POINTER);
	if(_this->uadlg == NULL)
		rCode=UAMGR_DLGLST_CREATE_ERROR;

	return rCode;
}


/*get current dialog state */
CCLAPI 
UACallStateType uaMgrGetDlgState(IN UaMgr _this, IN UaDlg target)
{
	UACallStateType state;
	UaDlg dlg;
	int pos;

	if (!_this || !target) {
		return RC_ERROR;
	}
	state=UACSTATE_UNKNOWN;
	for(pos=0;pos<dxLstGetSize(_this->uadlg);pos++){
		dlg=(UaDlg)dxLstPeek(_this->uadlg,pos);
		if(dlg == target){
			state = uaDlgGetState(dlg);
			break;
		}
	}
	return state;
}
/*make a call directly */
CCLAPI 
UaDlg uaMgrMakeCall(IN UaMgr _mgr, IN const char* _dest, IN SdpSess _sdp)
{
	UaDlg dlg=NULL;
	RCODE rCode=UAMGR_ERROR;

	if(NULL == _mgr)
		return NULL;

	if(NULL == _dest)
		return NULL;

	dlg=uaDlgNew(_mgr);
	if(NULL == dlg)
		return NULL;
	else{
		rCode=uaDlgDial(dlg,_dest,_sdp);
	}

	if(RC_OK != rCode){
		uaDlgRelease(dlg);
		return NULL;
	}

	return dlg;

}

/*command sequence */
CCLAPI 
int uaMgrGetCmdSeq(IN UaMgr _mgr)
{
	if(_mgr != NULL)
		return _mgr->cmdseq;
	else
		return -1;
}

CCLAPI 
RCODE uaMgrAddCmdSeq(IN UaMgr _this)
{
	RCODE rCode =UAMGR_NULL;

	if(_this != NULL){
		_this->cmdseq++;
		rCode=RC_OK;
	}
	return rCode;
}

/*post call status to application*/
/*CCLAPI 
void uaMgrPostMsg(IN UaMgr _this, IN UaMsg _msg)
{
	g_uaEvtCB(_msg->evtType,_msg);
}*/

/*post call status to applicatoin, another method type*/
/*CCLAPI 
void uaMgrPostNewMsg(IN UaMgr _this,
		     IN UaDlg _dlg,
		     IN UAEvtType _type,
		     IN int _cseq,
		     IN SdpSess _sdp) modified by tyhuang*/
RCODE uaMgrPostNewMsg(IN UaMgr _this,
		     IN UaDlg _dlg,
		     IN UAEvtType _type,
		     IN int _cseq,
		     IN SdpSess _sdp)
{
	UaMsg newMsg=NULL;
	UaContent _content=NULL;	
	char sdpbuf[1025]={'\0'};
	unsigned int bodylen=1024;
	RCODE rcode;

	rcode=sdpSess2Str(_sdp,sdpbuf,&bodylen);
	if(rcode==RC_OK)
		_content=uaContentNew("application/sdp",bodylen,(void *)sdpbuf);
	newMsg=uaMsgNew(_dlg,_type,_cseq,_content,NULL);
	if(_content!=NULL)
		uaContentFree(_content);

	if((_this!=NULL)&&(newMsg!=NULL))
		g_uaEvtCB(newMsg->evtType,newMsg);
	else 
		rcode=UAMSG_NULL;
		
	/*assume user will not use it after callback*/
	/*if user want to keep this, should duplicate it */
	if(newMsg)
		uaMsgFree(newMsg); 
	/*if (rcode != RC_OK) {
		UaCoreERR("[TxEvtCB] Error, Post Message Fail:%s!\n",uaCMRet2Phase(rcode));
	}*/
	return rcode;
}

/* new report function */
RCODE uaMgrPostNewMessage(IN UaMgr _this,
		     IN UaDlg _dlg,
		     IN UAEvtType _type,
		     IN int _cseq,
		     IN UaContent _content,
		     IN void* reserved)
{
	UaMsg newMsg=NULL;
	RCODE rCode=RC_OK;

	newMsg=uaMsgNew(_dlg,_type,_cseq,_content,reserved);
	if((_this!=NULL)&&(newMsg!=NULL))
		g_uaEvtCB(newMsg->evtType,newMsg);
	else
		rCode=UAMSG_NULL;
	/*assume user will not use it after callback*/
	/*if user want to keep this, should duplicate it */
	if(newMsg)
		uaMsgFree(newMsg);
	/*if (rCode != RC_OK) {
		UaCoreERR("[TxEvtCB] Error, Post Message Fail:%s!\n",uaCMRet2Phase(rCode));
	}*/
	return rCode;
}

RCODE uaMgrPostNewSubMsg(IN UaMgr _this,
		     IN UaDlg _dlg,
		     IN UAEvtType _type,
		     IN int _cseq,
		     IN UaContent _content,
		     IN UaSub	_sub)
{
	UaMsg newMsg=NULL;
	RCODE rCode=RC_OK;

	newMsg=uaMsgNew(_dlg,_type,_cseq,_content,NULL);
	uaMsgSetSubscription(newMsg,_sub);
	if((_this!=NULL)&&(newMsg!=NULL))
		g_uaEvtCB(newMsg->evtType,newMsg);
	else
		rCode=UAMSG_NULL;
	/*assume user will not use it after callback*/
	/*if user want to keep this, should duplicate it */
	if(newMsg)
		uaMsgFree(newMsg);
	/*if (rCode != RC_OK) {
		UaCoreERR("[TxEvtCB] Error, Post Message Fail:%s!\n",uaCMRet2Phase(rCode));
	}*/
	return rCode;
}

/*get Contact address*/
CCLAPI 
const char* uaMgrGetContactAddr(IN UaMgr _this)
{
	UaCfg cfg;
	UaUser user;
	char buf[512]={'\0'};
	char *userContactAddr=NULL;
	char *contactaddr=NULL,*tmp=NULL;
	int size,pos,flag=0;
	RCODE rcode;

	if(_this != NULL){
		cfg=uaMgrGetCfg(_this);
		user=uaMgrGetUser(_this);
		if((cfg==NULL)||(user==NULL)){
			UaCoreERR("[uaMgrGetContactAddr] user or cfg is NULL!\n");
			return NULL;
		}
		if(_this->contactaddr == NULL){
			/*generate contact address automatic*/
			tmp=(char*)uaUserGetName(user);
			if(tmp && (tmp!=""))
				size=sprintf(buf,"%s <sip:%s@%s",uaUserGetName(user),uaUserGetName(user),uaUserGetLocalAddr(user));
			else
				size=sprintf(buf,"supermgr <sip:%s",uaUserGetLocalAddr(user));
			
			/*if(uaCfgGetListenPort(cfg)!=5060)*/
			/*size+=sprintf(buf+size,":%d",uaCfgGetListenPort(cfg));*/
			if(uaUserGetLocalPort(user)>0)
				size+=sprintf(buf+size,":%d",uaUserGetLocalPort(user));
			else
				size+=sprintf(buf+size,":%d",uaCfgGetListenPort(cfg));
			if(uaCfgGetRegistrarTXP(cfg)==TCP)
				sprintf(buf+size,";transport=tcp>");
			else
				sprintf(buf+size,">");

		}else
			return _this->contactaddr;
		/* set mgr contact*/
		contactaddr=strDup(buf);
		_this->contactaddr=contactaddr;
		/* update contact in UaUser */
		size=uaUserGetNumOfContactAddr(user);
		for(pos=0;pos<size;pos++){
			userContactAddr=(char*)uaUserGetContactAddr(user,pos);
			/* if the  contactaddr not in user, should add it*/
			if(strICmp(userContactAddr,contactaddr)==0)
				flag=1;
		}
		if(flag==0)
			rcode=uaUserSetContactAddr(user,contactaddr);

	}else
		return NULL;

	return contactaddr;
}

/*get Poblic address*/
CCLAPI 
const char* uaMgrGetPublicAddr(IN UaMgr _this)
{
	UaCfg cfg;
	UaUser user;
	char buf[1024]={'\0'};

	if(_this != NULL){
		cfg=uaMgrGetCfg(_this);
		user=uaMgrGetUser(_this);
		if((cfg==NULL)||(user==NULL)){
			UaCoreERR("[uaMgrGetPublicAddr] user or cfg is NULL!\n");
			return NULL;
		}
		/*check using proxy or not*/
/*		if(uaCfgUsingProxy(cfg)){
			if(uaCfgGetProxyAddr(cfg) != NULL)
				sprintf(buf,"sip:%s@%s:%d",uaUserGetName(user),uaCfgGetProxyAddr(cfg),uaCfgGetProxyPort(cfg));
			else
				sprintf(buf,"sip:%s@%s:%d",uaUserGetName(user),uaUserGetLocalAddr(user),uaCfgGetListenPort(cfg));*/
		/* check using registrar or not */
		if(uaCfgUsingRegistrar(cfg)){
			if(uaCfgGetRegistrarAddr(cfg)!=NULL){
			/*using registrar*/
				/* modified bt tyhuang: add case TCP, with transport=tcp and add <>*/
				switch(uaCfgGetRegistrarTXP(cfg)){
					case TCP:
						/*
						sprintf(buf,"sip:%s@%s;transport=tcp",uaUserGetName(user),uaCfgGetRegistrarAddr(cfg));
						break;
						*/
					case UDP:
					default:
						sprintf(buf,"sip:%s@%s",uaUserGetName(user),uaCfgGetRegistrarAddr(cfg));
						break;		
				}
			}
		}else
			sprintf(buf,"sip:%s@%s",uaUserGetName(user),uaUserGetLocalAddr(user));
		
		if(_this->publicaddr != NULL){
			free(_this->publicaddr);
			_this->publicaddr=NULL;
		}
		_this->publicaddr=strDup(buf);

	}else
		return NULL;

	return _this->publicaddr;
}

char* uaMgrNewPublicSIPS(IN UaMgr _this)
{
	UaCfg cfg;
	UaUser user;
	char buf[256]={'\0'},*returl=NULL;

	if(_this != NULL){
		cfg=uaMgrGetCfg(_this);
		user=uaMgrGetUser(_this);
		if((cfg==NULL)||(user==NULL)){
			UaCoreERR("[uaMgrGetPublicAddr] user or cfg is NULL!\n");
			return NULL;
		}

		if(uaCfgUsingRegistrar(cfg)){
			if(uaCfgGetRegistrarAddr(cfg)!=NULL)
				sprintf(buf,"sips:%s@%s",uaUserGetName(user),uaCfgGetRegistrarAddr(cfg));
			else
				sprintf(buf,"sips:%s@%s",uaUserGetName(user),uaUserGetLocalAddr(user));
		}else
			sprintf(buf,"sips:%s@%s",uaUserGetName(user),uaUserGetLocalAddr(user));
		
		if(_this->publicaddr != NULL){
			free(_this->publicaddr);
			_this->publicaddr=NULL;
		}
		returl=strDup(buf);

	}

	return returl;
}

/*get match dialog,input call-id*/
CCLAPI 
UaDlg uaMgrGetMatchDlg(UaMgr _mgr, IN const char* _callid)
{
	UaDlg target=NULL;
	int idx=0,dlgnum=0;
	DxLst dlgLst=NULL;
	

	if(NULL == _mgr){
		UaCoreERR("[uaMgrGetMatchDlg] mgr is NULL!\n");
		return NULL;
	}
	if(NULL == _callid){
		UaCoreERR("[uaMgrGetMatchDlg] callid is NULL!\n");
		return NULL;
	}
	dlgLst=uaMgrGetDlgLst(_mgr);
	if(NULL == dlgLst)
		return NULL;

	dlgnum=dxLstGetSize(dlgLst);
	for(idx=0;idx<dlgnum;idx++){
		target=(UaDlg)dxLstPeek(dlgLst,idx);
		if(strICmp(uaDlgGetCallID(target),_callid)==0)
			return target;
		target=NULL;
	}

	return target;
}

/* user shold free it after using*/
char* uaMgrGetLastRegisterAddr(IN UaMgr mgr)
{

	if(mgr)
		if(mgr->lastregisteraddr)
			return strDup(mgr->lastregisteraddr);
	return NULL;
}
