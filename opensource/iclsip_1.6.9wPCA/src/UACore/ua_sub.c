/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * ua_sub.c
 *
 * $Id: ua_sub.c,v 1.7 2005/05/04 09:03:07 tyhuang Exp $
 */
#include <stdio.h>
#include "ua_sub.h"

/** 
 *  \brief A C-structure for a Subscription
 *
 */

struct uasubObj{
	char* URI;		/* of subscription */
	unsigned short Expires; /* Expires of of subscription */
	UASubState State;	/* current state of subscription */
	char*	reason;		/* reason used when terminate a subscription */	 
	UaDlg Parent;		/* associating dialog of subscription */
	UaEvtpkg Evtpkg;	/* event type of of subscription */
	BOOL bBlock;		/* if block or not */	
	BOOL bIn;		/* bIn specify in IN-List or OUT-List */
};

/* create a subscription object*/
/** 
 *  \brief create a subscription object.
 *  \param _url is the request url for this subscription.
 *  \param _expires is the time of expiration.
 *  \param _state is the subscription state.
 *  \param _parent is the dialog associated with. 
 *  \param _evtpak is the event belonged to.
 *  \param _bBlock is the flag if it's block.
 *  \param _bIn is the flag if it's in in-list or out-list.
 *	\retval UaSub object or NULL
 */
UaSub uaSubNew(const char* _url,unsigned short _expires,UASubState _state,UaDlg _parent,UaEvtpkg _evtpkg,BOOL _bBlock, BOOL _bIn)
{
	UaSub _this;
	/* check input value of parameters */
	if(!_url || strlen(_url)==0){
		UaCoreERR("[uaSubNew] url is NULL!\n");
		return NULL;
	}
	if(_evtpkg==NULL){
		UaCoreERR("[uaSubNew] url is NULL!\n");
		return NULL;
	}
	/*
	if(_expires<0){
		UaCoreERR("[uaSubNew] expires is smaller than zero!\n");
		return NULL;
	}
	*/
	/*alloc memory */
	_this=(UaSub)calloc(1,sizeof(struct uasubObj));
	if(_this != NULL){
		_this->URI=strDup(_url);
		_this->Expires=_expires;
		_this->State=_state;
		_this->reason=NULL;
		_this->Parent=_parent;
		_this->Evtpkg=_evtpkg;
		_this->bBlock=_bBlock;
		_this->bIn=_bIn;
		return _this;
	}
	return NULL;
}

/** Duplicate a uaSub 
 *	\param _uasub the UaSub will be duplicated
 *	\retval UaSub object or NULL
 */
UaSub uaSubDup(IN UaSub _uasub)
{
	UaSub _this;

	/*check input UaSub */
	if(_uasub == NULL){
		UaCoreERR("[uaSubDup] sub is NULL!\n");
		return NULL;
	}
	if(_uasub->URI == NULL){
		UaCoreERR("[uaSubDup] url in sub is NULL!\n");
		return NULL;
	}
	/*alloc memory */
	_this=(UaSub)calloc(1,sizeof(struct uasubObj));
	if(_this != NULL){
		_this->URI=strDup(_uasub->URI);
		_this->Expires=_uasub->Expires;
		_this->State=_uasub->State;
		_this->Parent=_uasub->Parent;
		_this->Evtpkg=_uasub->Evtpkg;
		_this->bBlock=_uasub->bBlock;
		_this->bIn=_uasub->bIn;
		_this->reason=strDup(_uasub->reason);
		return _this;
	}
	return NULL;

}

/** Free a uaSub 
 *	\param _this UaSub object we want to free
 */
void	uaSubFree(IN UaSub _this)
{
	/*check input UaSub */
	if(_this != NULL){
		/*check input UaSub->URL */
		if(_this->URI != NULL){
			/* free URL */
			free(_this->URI);
			_this->URI=NULL;
		}
		_this->Expires=-1;
		_this->Evtpkg=NULL;
		_this->Parent=NULL;
		_this->bBlock=FALSE;
		_this->bIn=FALSE;
		_this->State=UASUB_TERMINATED;
		if(_this->reason != NULL){
			free(_this->reason);
			_this->reason=NULL;
		}
		memset(_this,0,sizeof(struct uasubObj));
		free(_this);
		_this=NULL;
	
	}
}

/** Get uaSub's dialog 
 *	\param _this UaSub object
 *	\retval UaDlg 
 */
UaDlg	uaSubGetDlg(IN UaSub _this)
{
	if(_this != NULL)
		return _this->Parent;
	return NULL;
}

/** Set URI of uaSub 
 *	\param _this UaSub object
 *	\param _url SIP URL will be set in \a _this 
 *	\retval RCODE : RC_OK if success,or RC_ERROR if error occurs
 */
RCODE	uaSubSetURI(IN UaSub _this,const char* _url)
{
	int rcode=RC_ERROR;
	
	/* check inputs  */
	if((_this != NULL)&&(_url != NULL)){
		_this->URI=strDup(_url);
		rcode=RC_OK;
	}
	return rcode;
}
/** get URI of uaSub 
 *	\param _this UaSub object 
 *	\retval uri a char pointer
 */
char*	uaSubGetURI(IN UaSub _this)
{
	if(_this != NULL){
		return _this->URI;
	}
	return NULL;
}

/** set expires of uaSub 
 *	\param _this UaSub object
 *	\param _expires will be set in \a _this 
 *	\retval RCODE : RC_OK if success,or RC_ERROR if error occurs
 */
RCODE uaSubSetExpires(IN UaSub _this,unsigned short _expires)
{
	int rcode=RC_ERROR;
	
	if(_this!=NULL){
		_this->Expires=_expires;
		rcode=RC_OK;
	}
	return rcode;
}

/** get expires of uaSub 
 *	\param _this UaSub object
 *	\retval unsigned short integer
 */
unsigned short uaSubGetExpires(IN UaSub _this)
{
	unsigned int exp=0;
	
	if(_this != NULL)
		exp=_this->Expires;
	return exp; 
}

/** set state of subscription 
 *	\param _this UaSub object
 *	\param _state subscription state will be set in \a _this 
 *	\param _reason reason for this subscription
 *	\retval RCODE : RC_OK if success,or RC_ERROR if error occurs
 */
RCODE	uaSubSetSubstate(IN UaSub _this,IN UASubState _state, IN const char* _reason)
{
	int rcode=RC_ERROR;

	if(_this != NULL){
		_this->State=_state;
		
		if(_state==UASUB_TERMINATED){
			if (_this->reason) free(_this->reason);
			_this->reason=strDup(_reason);
		}
		rcode=RC_OK;
	}

	return rcode;
}

/** get state of subscription 
 *	\param _this UaSub object
 *	\retval UASub
 */
UASubState uaSubGetSubstate(IN UaSub _this)
{
	UASubState st=UASUB_TERMINATED;

	if(_this != NULL)
		st=_this->State;
	return st;
}

/** get reason of a terminated ubscription 
 *	\param _this UaSub object
 *	retval a char pointer to reason
 */
char* uaSubGetReason(IN UaSub _this)
{
	char *rReason=NULL;
	
	if(_this != NULL){
		if(_this->State == UASUB_TERMINATED)
			rReason=_this->reason;
	}
	return rReason;
}

RCODE uaSubSetReason(IN UaSub _this,IN const char*reason)
{

	if (!_this || !reason) return RC_ERROR;

	if (_this->reason) free(_this->reason);

	_this->reason=strDup(_this->reason);
	return RC_OK;

}

/** get eventpackage of UaSub 
 *	\param _this UaSub object
 *	\retval a pointer to UaEvtpkg
 */
UaEvtpkg uaSubGetEvtpkg(IN UaSub _this)
{
	if(_this != NULL)
		return _this->Evtpkg;
	return NULL;
}

/** check if uaSub is block 
 *	\param _this UaSub object
 *	\retval TRUE for block or FALSE for non-block
 */
BOOL	isuaSubBlock(IN UaSub _this)
{
	if(_this != NULL)
		return _this->bBlock;
	return FALSE;
}

/** set uaSub block or unblokc 
 *	\param _this UaSub object
 *	\param block TRUE for block this subscription;FALSE for unblock it
 *	\retval RCODE :RC_OK if success,or RC_ERROR if error occurs
 */
RCODE	uaSubSetBlock(IN UaSub _this,IN BOOL block)
{
	RCODE rCode=RC_ERROR;

	if(_this != NULL){
		_this->bBlock=block;
		rCode=RC_OK;
	}
	return rCode;
}

/** check if uaSub is in in/out-list 
 *	\param _this UaSub object
 *	\retval TRUE for in-list or FALSE for out-list
 */
BOOL	isuaSubIn(IN UaSub _this)
{
	if(_this != NULL)
		return _this->bIn;
	return FALSE;
}

/** covert subscription state to string 
 *	\param _state to be convert to a text string
 *	\retval a string for status."active","pending","terminated"
 */
const char *uaSubStateToStr(IN UASubState _state)
{
	switch(_state)
	{
	case UASUB_ACTIVE:
		return "active";
	case UASUB_PENDING:
		return "pending";
	case UASUB_TERMINATED:
		return "terminated";
	default:
		UaCoreERR("[uaSubStateToStr] unknow subscriptio-state is input!\n");
		break;
	}
	return NULL;
}

/** covert subscription state from string 
 *	\param _state is a text string and will be converted to UASubState
 *	\retval UASubState :UASUB_ACTIVE,UASUB_PENDING,UASUB_TERMINATED
 */
UASubState uaSubStateFromStr(IN const char* _state)
{
	if (!_state) return UASUB_NULL;
	
	if(strcmp(_state,"active")==0)
		return UASUB_ACTIVE;
	else if (strcmp(_state,"pending")==0)
		return UASUB_PENDING;
	else if (strcmp(_state,"terminated")==0)
		return UASUB_TERMINATED;

	return UASUB_TERMINATED;
}