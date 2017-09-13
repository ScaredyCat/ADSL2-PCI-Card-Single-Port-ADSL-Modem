/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * sdp_sess.c
 *
 * $Id: sdp_sess.c,v 1.26 2006/12/05 09:41:33 tyhuang Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <adt/dx_vec.h>
#include <adt/dx_str.h>
#include <common/cm_utl.h>
#include "sdp_utl.h"
#include "sdp_tok.h"
#include "sdp_sess.h"
#include <common/cm_trace.h>

#define SDP_STRING_SDPSESS_LENGTH		512

/*---------------------------------------------
// SDP session
*/
#define SDP_FLAG_NONE				0x0
#define SDP_FLAG_ORIGIN				0x1
#define SDP_FLAG_SESSIONNAME			0x2
#define SDP_FLAG_SESSIONINFO			0x4
#define SDP_FLAG_URI				0x8
#define SDP_FLAG_EMAIL				0x10
#define SDP_FLAG_PHONENUM			0x20
#define SDP_FLAG_CONNDATA			0x40
#define SDP_FLAG_BANDWIDTH			0x80
#define SDP_FLAG_TIMESESS			0x100
#define SDP_FLAG_TIMEADJ			0x200
#define SDP_FLAG_ENCRYKEY			0x400
#define SDP_FLAG_ATTRIB				0x800
#define SDP_FLAG_MEDIASESS			0x1000

#define SDP_STRING_SESSIONNAME_LENGTH		256
#define SDP_STRING_SESSIONINFO_LENGTH		256
#define SDP_STRING_URI_LENGTH			256
#define SDP_STRING_EMAIL_LENGTH			256
#define SDP_STRING_PHONENUM_LENGTH		256

struct sdpSessObj {
	UINT32		sdp_flags_;

	UINT16		Tv_;
	SdpTo*		pTo_;		/*must set sdp_flags_*/
	char*		pTs_;		/*must set sdp_flags_*/
	char*		pTi_;		/*must set sdp_flags_*/
	char*		pTu_;		/*must set sdp_flags_*/

	DxVector	pTeList_;	/*must set sdp_flags_*/
	DxVector	pTpList_;	/*must set sdp_flags_*/
	SdpTc*		pTc_;		/*must set sdp_flags_*/
	SdpTk*		pTk_;		/*must set sdp_flags_*/
	DxVector	pTbList_;	/*must set sdp_flags_*/
	SdpTz*		pTz_;		/*must set sdp_flags_*/
	DxVector	pTaList_;	/*must set sdp_flags_*/

	DxVector	pTsessList_;	/*must set sdp_flags_*/
	DxVector	pMsessList_;	/*must set sdp_flags_*/
};

char SDP_NETTYPE_TOKEN[3][5] = {
	"NONE",
	"IN",
	"\0"
};

char SDP_ADDRTYPE_TOKEN[4][5] = {
	"NONE",
	"IP4",
	"IP6",
	"\0"
};

void sdpInit(void) {} /* just for WIN32 DLL forcelink */

/*--------------------------------------------------------------------------
// SDP
*/
CCLAPI 
SdpSess	sdpSessNew(void)
{
	SdpSess	sdp;

	sdp = (SdpSess)malloc(sizeof(struct sdpSessObj));

	sdp->sdp_flags_ = SDP_FLAG_NONE;

	sdp->pTaList_ = dxVectorNew(SDP_DEFAULT_LIST_SIZE,SDP_DEFAULT_LIST_SIZE/2,DX_VECTOR_POINTER);
	sdp->pTbList_ = dxVectorNew(SDP_DEFAULT_LIST_SIZE,SDP_DEFAULT_LIST_SIZE/2,DX_VECTOR_POINTER);
	sdp->pTk_ = NULL;
	sdp->pTc_ = NULL;
	sdp->pTeList_ = dxVectorNew(SDP_DEFAULT_LIST_SIZE,SDP_DEFAULT_LIST_SIZE/2,DX_VECTOR_STRING);
	sdp->pMsessList_ = dxVectorNew(SDP_DEFAULT_LIST_SIZE,SDP_DEFAULT_LIST_SIZE/2,DX_VECTOR_POINTER);
	sdp->pTo_ = NULL;
	sdp->pTpList_ = dxVectorNew(SDP_DEFAULT_LIST_SIZE,SDP_DEFAULT_LIST_SIZE/2,DX_VECTOR_STRING);
	sdp->pTi_ = NULL;
	sdp->pTs_ = NULL;
	sdp->pTz_ = NULL;
	sdp->pTsessList_ = dxVectorNew(SDP_DEFAULT_LIST_SIZE,SDP_DEFAULT_LIST_SIZE/2,DX_VECTOR_POINTER);
	sdp->pTu_ = NULL;
	sdp->Tv_ = 0;

	return sdp;
}

CCLAPI 
void	sdpSessFree(SdpSess this_)
{
	if(!this_)
		return;

	dxVectorFree(this_->pTaList_, (void(*)(void*))free);
	dxVectorFree(this_->pTbList_, (void(*)(void*))free);
	free(this_->pTc_);
	dxVectorFree(this_->pTeList_, (void(*)(void*))free);
	free(this_->pTk_);
	dxVectorFree(this_->pMsessList_,(void(*)(void*))sdpMsessFree);
	free(this_->pTo_);
	dxVectorFree(this_->pTpList_, (void(*)(void*))free);
	free(this_->pTi_);
	free(this_->pTs_);
	free(this_->pTz_);
	dxVectorFree(this_->pTsessList_,(void(*)(void*))sdpTsessFree);
	free(this_->pTu_);

	free(this_);
	return;
}

SdpTa*	sdpsessTaDup(SdpTa*	attrib)
{
	SdpTa*	temp;

	if(!attrib)
		return NULL;
		
	temp = (SdpTa*)malloc(sizeof(SdpTa));
	*temp = *attrib;

	return temp;
}

SdpTb*	sdpsessTbDup(SdpTb*	attrib)
{
	SdpTb*	temp;

	if(!attrib)
		return NULL;
		
	temp = (SdpTb*)malloc(sizeof(SdpTb));
	*temp = *attrib;

	return temp;
}

CCLAPI 
SdpSess	sdpSessDup(SdpSess this_)
{
	SdpSess		sdp;
/*	char*		sdpText;
	RCODE		ret; */
	UINT32		len = SDP_STRING_SDPSESS_LENGTH;


	if(!this_)
		return NULL;

/*
	sdpText = (char*)malloc(len);

	ret = sdpSess2Str(this_,sdpText,&len);
	if( ret==RC_ERROR ) {
		TCRPrint(1,"<sdpSessDup()>:sdpSess2Str() error.\n");
		free(sdpText);
		return NULL;
	}
	else if( ret==RC_SDP_BUFFER_LENGTH_INSUFFICIENT ) {
		sdpText = (char*)realloc(sdpText,len);
		ret = sdpSess2Str(this_,(char*)sdpText,&len);
		if( ret!=RC_OK ) {
			TCRPrint(1,"<sdpSessDup()>:sdpSess2Str() error.\n");
			free(sdpText);
			return NULL;
		}
	}
	
	sdp = sdpSessNewFromText(sdpText);
	if( !sdp )
		TCRPrint(1,"<sdpSessDup()>:sdpSessNewFromText() error.\n");

	free(sdpText);
*/
	sdp = (SdpSess)calloc(1,sizeof(struct sdpSessObj));
	if (!sdp) return NULL;

	/* copy or duplicate value */
	sdp->sdp_flags_ = SDP_FLAG_NONE ;
	sdpSessSetTv(sdp,this_->Tv_);

	sdpSessSetTi(sdp,this_->pTi_);
	sdpSessSetTs(sdp,this_->pTs_);
	sdpSessSetTu(sdp,this_->pTu_);

	sdpSessSetTo(sdp,this_->pTo_);
	sdpSessSetTz(sdp,this_->pTz_);
	sdpSessSetTk(sdp,this_->pTk_);
	sdpSessSetTc(sdp,this_->pTc_);
	
	sdp->pTeList_ = dxVectorDup(this_->pTeList_, (void *(*)(void *))strDup);
	sdp->pTpList_ = dxVectorDup(this_->pTpList_, (void *(*)(void *))strDup);

	sdp->pTaList_ = dxVectorDup(this_->pTaList_,(void*(*)(void*))sdpsessTaDup);
	sdp->pTbList_ = dxVectorDup(this_->pTbList_,(void*(*)(void*))sdpsessTbDup);
	sdp->pMsessList_ = dxVectorDup(this_->pMsessList_,(void*(*)(void*))sdpMsessDup);
	sdp->pTsessList_ = dxVectorDup(this_->pTsessList_,(void*(*)(void*))sdpTsessDup);

	sdp->sdp_flags_ = this_->sdp_flags_ ;
	return sdp;
}

CCLAPI 
RCODE	sdpSessSetTv(SdpSess this_, UINT16 version)
{
	if(!this_)
		return	RC_E_PARAM;

	this_->Tv_ = version;
	return RC_OK;
}

CCLAPI 
RCODE	sdpSessGetTv(SdpSess this_, UINT16 *version)
{
	if(!this_)
		return RC_E_PARAM;
	if(!version)
		return RC_E_PARAM;

	(*version) = this_->Tv_;

	return RC_OK;
}

CCLAPI 
RCODE	sdpSessSetTo(SdpSess this_, SdpTo* origin)
{
	if(!this_)
		return	RC_E_PARAM;
	if(!origin) { /* set origin==NULL to clear this->pOrigin */
		free(this_->pTo_);
		this_->pTo_ = NULL;
		this_->sdp_flags_ &= ~(SDP_FLAG_ORIGIN);
		return RC_OK;
	}

	if( !(this_->sdp_flags_&SDP_FLAG_ORIGIN) )
		this_->pTo_ = (SdpTo*)malloc(sizeof(SdpTo));
	
	*(this_->pTo_) = (*origin);		/* copy */
	this_->sdp_flags_ |= SDP_FLAG_ORIGIN;

	return RC_OK;
}

CCLAPI 
RCODE	sdpSessGetTo(SdpSess this_, SdpTo* origin)
{
	if(!this_)
		return RC_E_PARAM;
	if(!origin)
		return RC_E_PARAM;

	if( this_->sdp_flags_&SDP_FLAG_ORIGIN ) {
		(*origin) = *(this_->pTo_);
		return RC_OK;
	}
	else
		return RC_SDP_VALUE_NOT_SET;
}

CCLAPI 
RCODE	sdpSessSetTs(SdpSess this_, char* name)
{
	if(!this_)
		return RC_E_PARAM;
	if(!name) { /* set name==NULL to clear this_->pTs_ */
		if( this_->sdp_flags_&SDP_FLAG_SESSIONNAME ) 
			free(this_->pTs_);
		this_->pTs_ = NULL;
		this_->sdp_flags_ &= ~(SDP_FLAG_SESSIONNAME);
		return RC_OK;
	}

	if( this_->sdp_flags_&SDP_FLAG_SESSIONNAME ) 
		free(this_->pTs_);
	this_->pTs_ = strDup(name);
	this_->sdp_flags_ |= SDP_FLAG_SESSIONNAME;

	return RC_OK;
}

/*
*    Fun:	sdpSessGetTs
*    Desc:	Get Ts into $strbuf
*    Ret:	RC_OK, 
*			Success, set acture length filled to $buflen.
*		RC_SDP_VALUE_NOT_SET,
*			Fail, because the value of Ts is not set.
*		RC_SDP_BUFFER_LENGTH_INSUFFICIENT,
*			Fail, buffer length is not enough, set length needed
*			to $buflen.
*/
CCLAPI
RCODE	sdpSessGetTs(SdpSess this_, char* strbuf, UINT32* buflen)
{
	int		rlen = 0;

	if(!this_)
		return RC_E_PARAM;
	if(!strbuf||!buflen)
		return RC_E_PARAM;

	if( this_->sdp_flags_&SDP_FLAG_SESSIONNAME ) {
		rlen = strlen(this_->pTs_);
		if( *buflen<=(UINT32)rlen ) {
			strncpy(strbuf,this_->pTs_,*buflen-1);
			(*buflen) = rlen + 1;
			return RC_SDP_BUFFER_LENGTH_INSUFFICIENT;
		}
		strcpy(strbuf,this_->pTs_);
		(*buflen) = rlen;
		return RC_OK;
	}
	else {
		strbuf[0] = 0;
		(*buflen) = 0;
		return RC_SDP_VALUE_NOT_SET;
	}
}

CCLAPI 
RCODE	sdpSessSetTi(SdpSess this_, char* info)
{
	if(!this_)
		return RC_E_PARAM;
	if(!info) { /* set info==NULL to clear this_->pTi_*/
		if( this_->sdp_flags_&SDP_FLAG_SESSIONINFO ) 
			free(this_->pTi_);
		this_->pTi_ = NULL;
		this_->sdp_flags_ &= ~(SDP_FLAG_SESSIONINFO);
		return RC_OK;
	}

	if( this_->sdp_flags_&SDP_FLAG_SESSIONINFO ) 
		free(this_->pTi_);
	this_->pTi_ = strDup(info);
	this_->sdp_flags_ |= SDP_FLAG_SESSIONINFO;

	return RC_OK;
}

/*
*    Fun:	sdpSessGetTi
*    Desc:	Get Ti into $strbuf
*    Ret:	RC_OK, 
*			Success, set acture length filled to $buflen.
*		RC_SDP_VALUE_NOT_SET,
*			Fail, because the value of Ts is not set.
*		RC_SDP_BUFFER_LENGTH_INSUFFICIENT,
*			Fail, buffer length is not enough, set length needed
*			to $buflen.
*/
CCLAPI 
RCODE	sdpSessGetTi(SdpSess this_, char* strbuf, UINT32* buflen)
{
	int rlen = 0;

	if(!this_)
		return RC_E_PARAM;
	if(!strbuf||!buflen)
		return RC_E_PARAM;

	if( this_->sdp_flags_&SDP_FLAG_SESSIONNAME ) {
		rlen = strlen(this_->pTi_);
		if( *buflen<=(UINT32)rlen ) {
			strncpy(strbuf,this_->pTi_,*buflen-1);
			(*buflen) = rlen + 1;
			return RC_SDP_BUFFER_LENGTH_INSUFFICIENT;
		}
		strcpy(strbuf,this_->pTi_);
		(*buflen) = rlen;
		return RC_OK;
	}
	else {
		strbuf[0] = 0;
		(*buflen) = 0;
		return RC_SDP_VALUE_NOT_SET;
	}
}

CCLAPI 
RCODE	sdpSessSetTu(SdpSess this_, char* uri)
{
	if(!this_)
		return RC_E_PARAM;
	if(!uri) { /* set uri==NULL to clear this_->pTu_*/
		if( this_->sdp_flags_&SDP_FLAG_URI ) 
			free(this_->pTu_);
		this_->pTu_ = NULL;
		this_->sdp_flags_ &= ~(SDP_FLAG_URI);
		return RC_OK;
	}

	if( this_->sdp_flags_&SDP_FLAG_URI ) 
		free(this_->pTu_);
	this_->pTu_ = strDup(uri);
	this_->sdp_flags_ |= SDP_FLAG_URI;

	return RC_OK;
}

/*
*    Fun:	sdpSessGetTu
*    Desc:	Get Tu into $strbuf
*    Ret:	RC_OK, 
*			Success, set acture length filled to $buflen.
*		RC_SDP_VALUE_NOT_SET,
*			Fail, because the value of Ts is not set.
*		RC_SDP_BUFFER_LENGTH_INSUFFICIENT,
*			Fail, buffer length is not enough, set length needed
*			to $buflen.
*/
CCLAPI 
RCODE	sdpSessGetTu(SdpSess this_, char* strbuf, UINT32* buflen)
{
	int		rlen = 0;

	if(!this_)
		return RC_E_PARAM;
	if(!strbuf||!buflen)
		return RC_E_PARAM;

	if( this_->sdp_flags_&SDP_FLAG_SESSIONNAME ) {
		rlen = strlen(this_->pTu_);
		if( *buflen<=(UINT32)rlen ) {
			strncpy(strbuf,this_->pTu_,*buflen-1);
			(*buflen) = rlen + 1;
			return RC_SDP_BUFFER_LENGTH_INSUFFICIENT;
		}
		strcpy(strbuf,this_->pTu_);
		(*buflen) = rlen;
		return RC_OK;
	}
	else {
		strbuf[0] = 0;
		(*buflen) = 0;
		return RC_SDP_VALUE_NOT_SET;
	}
}

CCLAPI 
RCODE	sdpSessAddTeAt(SdpSess this_, char* email, int index)
{
	if(!this_)
		return RC_E_PARAM;
	if(!email)
		return RC_E_PARAM;

	if( dxVectorAddElementAt(this_->pTeList_,(void*)email,index)==RC_OK ) {
		this_->sdp_flags_ |= SDP_FLAG_EMAIL;
		return RC_OK;
	}
	else
		return RC_ERROR;
}

/*
*    Fun:	sdpSessGetTeAt
*    Desc:	Get Te into $strbuf
*    Ret:	RC_OK, 
*			Success, set acture length filled to $buflen.
*		RC_SDP_VALUE_NOT_SET,
*			Fail, because the value of Ts is not set.
*		RC_SDP_BUFFER_LENGTH_INSUFFICIENT,
*			Fail, buffer length is not enough, set length needed
*			to $buflen.
*/
CCLAPI 
RCODE	sdpSessGetTeAt(SdpSess this_, int index, char* strbuf, UINT32* buflen)
{
	int		rlen = 0;
	char*		temp;

	if(!this_)
		return RC_E_PARAM;
	if(!strbuf||!buflen)
		return RC_E_PARAM;

	if( this_->sdp_flags_&SDP_FLAG_SESSIONNAME ) {
		temp = dxVectorGetElementAt(this_->pTeList_, index);
		rlen = strlen(temp);
		if( *buflen<=(UINT32)rlen ) {
			strncpy(strbuf,temp,*buflen-1);
			(*buflen) = rlen + 1;
			return RC_SDP_BUFFER_LENGTH_INSUFFICIENT;
		}
		strcpy(strbuf,temp);
		(*buflen) = rlen;
		return RC_OK;
	}
	else {
		strbuf[0] = 0;
		(*buflen) = 0;
		return RC_SDP_VALUE_NOT_SET;
	}
}

CCLAPI 
RCODE	sdpSessDelTeAt(SdpSess this_, int index)
{
	char* temp;

	if( !this_ )
		return RC_E_PARAM;

	if( this_->sdp_flags_&SDP_FLAG_EMAIL ) {
		temp = dxVectorRemoveElementAt(this_->pTeList_,index);
		if( temp ) {
			free(temp);
			if( dxVectorGetSize(this_->pTeList_)<=0 )
				this_->sdp_flags_ &= ~(SDP_FLAG_EMAIL);
			return RC_OK;
		}
	}

	return RC_ERROR;
}

CCLAPI 
int	sdpSessGetTeSize(SdpSess this_)
{
	return (this_)?dxVectorGetSize(this_->pTeList_):0;
}

CCLAPI 
RCODE	sdpSessAddTpAt(SdpSess this_, char* phnnum, int index)
{
	if(!this_)
		return RC_E_PARAM;
	if(!phnnum)
		return RC_E_PARAM;

	if( dxVectorAddElementAt(this_->pTpList_,(void*)phnnum,index)==RC_OK ) {
		this_->sdp_flags_ |= SDP_FLAG_PHONENUM;
		return RC_OK;
	}
	else
		return RC_ERROR;
}

/*
*    Fun:	sdpSessGetTpAt
*    Desc:	Get Tp into $strbuf
*    Ret:	RC_OK, 
*			Success, set acture length filled to $buflen.
*		RC_SDP_VALUE_NOT_SET,
*			Fail, because the value of Ts is not set.
*		RC_SDP_BUFFER_LENGTH_INSUFFICIENT,
*			Fail, buffer length is not enough, set length needed
*			to $buflen.
*/
CCLAPI 
RCODE	sdpSessGetTpAt(SdpSess this_, int index, char* strbuf, UINT32* buflen)
{
	int		rlen = 0;
	char*		temp;

	if(!this_)
		return RC_E_PARAM;
	if(!strbuf||!buflen)
		return RC_E_PARAM;

	if( this_->sdp_flags_&SDP_FLAG_SESSIONNAME ) {
		temp = dxVectorGetElementAt(this_->pTpList_, index);
		rlen = strlen(temp);
		if( *buflen<=(UINT32)rlen ) {
			strncpy(strbuf,temp,*buflen-1);
			(*buflen) = rlen + 1;
			return RC_SDP_BUFFER_LENGTH_INSUFFICIENT;
		}
		strcpy(strbuf,temp);
		(*buflen) = rlen;
		return RC_OK;
	}
	else {
		strbuf[0] = 0;
		(*buflen) = 0;
		return RC_SDP_VALUE_NOT_SET;
	}
}

CCLAPI 
RCODE		sdpSessDelTpAt(SdpSess this_, int index)
{
	char* temp;

	if( !this_ )
		return RC_E_PARAM;

	if( this_->sdp_flags_&SDP_FLAG_PHONENUM ) {
		temp = dxVectorRemoveElementAt(this_->pTpList_,index);
		if( temp ) {
			free(temp);
			if( dxVectorGetSize(this_->pTpList_)<=0 )
				this_->sdp_flags_ &= ~(SDP_FLAG_PHONENUM);
			return RC_OK;
		}
	}

	return RC_ERROR;
}

CCLAPI 
int sdpSessGetTpSize(SdpSess this_)
{
	return (this_)?dxVectorGetSize(this_->pTpList_):0;
}

CCLAPI 
RCODE sdpSessSetTc(SdpSess this_, SdpTc* connData)
{
	if(!this_)
		return	RC_E_PARAM;
	if(!connData) { /* set connData==NULL to clear this->pTc_*/
		free(this_->pTc_);
		this_->pTc_ = NULL;
		this_->sdp_flags_ &= ~(SDP_FLAG_CONNDATA);
		return RC_OK;
	}

	if( !(this_->sdp_flags_&SDP_FLAG_CONNDATA) )
		this_->pTc_ = (SdpTc*)malloc(sizeof(SdpTc));
	
	*(this_->pTc_) = (*connData);		/* copy */
	this_->sdp_flags_ |= SDP_FLAG_CONNDATA;

	return RC_OK;
}

CCLAPI 
RCODE sdpSessGetTc(SdpSess this_, SdpTc* connData)
{
	if(!this_)
		return RC_E_PARAM;
	if(!connData)
		return RC_E_PARAM;

	if( this_->sdp_flags_&SDP_FLAG_CONNDATA ) {
		(*connData) = *(this_->pTc_);
		return RC_OK;
	}
	else
		return RC_ERROR;
}

CCLAPI 
RCODE sdpSessSetTk(SdpSess this_, SdpTk* encryKey)
{
	if(!this_)
		return	RC_E_PARAM;
	if(!encryKey) { /* set encryKey==NULL to clear this_->pTk_*/
		free(this_->pTk_);
		this_->pTk_ = NULL;
		this_->sdp_flags_ &= ~(SDP_FLAG_ENCRYKEY);
		return RC_OK;
	}

	if( !(this_->sdp_flags_&SDP_FLAG_ENCRYKEY) )
		this_->pTk_ = (SdpTk*)malloc(sizeof(SdpTk));
	
	*(this_->pTk_) = (*encryKey);		/* copy*/
	this_->sdp_flags_ |= SDP_FLAG_ENCRYKEY;

	return RC_OK;
}

CCLAPI 
RCODE sdpSessGetTk(SdpSess this_, SdpTk* encryKey)
{
	if(!this_)
		return RC_E_PARAM;
	if(!encryKey)
		return RC_E_PARAM;

	if( this_->sdp_flags_&SDP_FLAG_ENCRYKEY ) {
		(*encryKey) = *(this_->pTk_);
		return RC_OK;
	}
	else
		return RC_ERROR;
}

CCLAPI 
RCODE sdpSessAddTbAt(SdpSess this_, SdpTb* bandwidth, int index)
{
	SdpTb* temp;
	
	if(!this_)
		return RC_E_PARAM;
	if(!bandwidth)
		return RC_E_PARAM;
	if(!this_->pTbList_) 
		return RC_ERROR;
	
	temp = (SdpTb*)malloc(sizeof(SdpTb));		
	if(!temp)
		return RC_ERROR;
		
	(*temp) = (*bandwidth);
	if( dxVectorAddElementAt(this_->pTbList_,(void*)temp,index)!=RC_OK ) {
		free(temp);
		return RC_ERROR;
	}

	this_->sdp_flags_ |= SDP_FLAG_BANDWIDTH;
	return RC_OK;
}

CCLAPI 
RCODE sdpSessGetTbAt(SdpSess this_, int index, SdpTb* bandwidth)
{
	SdpTb		*temp;

	if( !this_ )
		return RC_E_PARAM;
	if( !bandwidth )
		return RC_E_PARAM;

	if( this_->sdp_flags_&SDP_FLAG_BANDWIDTH &&
	    (temp=dxVectorGetElementAt(this_->pTbList_, index)) ) {
		(*bandwidth) = (*temp);
		return RC_OK;
	}
	else
		return RC_ERROR;
}

CCLAPI 
RCODE sdpSessDelTbAt(SdpSess this_, int index)
{
	SdpTb	*temp;

	if( !this_ )
		return RC_E_PARAM;

	if( this_->sdp_flags_&SDP_FLAG_BANDWIDTH ) {
		temp = dxVectorRemoveElementAt(this_->pTbList_,index);
		if( temp ) {
			free(temp);
			if( dxVectorGetSize(this_->pTbList_)<=0 )
				this_->sdp_flags_ &= ~(SDP_FLAG_BANDWIDTH);
			return RC_OK;
		}
	}

	return RC_ERROR;
}

CCLAPI 
int sdpSessGetTbSize(SdpSess this_)
{
	return (this_)?dxVectorGetSize(this_->pTbList_):0;
}

CCLAPI 
RCODE sdpSessAddTsessAt(SdpSess this_, SdpTsess timeSess, int index)
{
	SdpTsess temp;

	if(!this_)
		return RC_E_PARAM;
	if(!timeSess)
		return RC_E_PARAM;
	if(!this_->pTsessList_ ) 
		return RC_ERROR;
		
	temp = sdpTsessDup(timeSess);	
	if(!temp)
		return RC_ERROR;
		
	if( dxVectorAddElementAt(this_->pTsessList_,(void*)temp,index)!=RC_OK ) {
		sdpTsessFree(temp);
		return RC_ERROR;
	}
	
	this_->sdp_flags_ |= SDP_FLAG_TIMESESS;
	return RC_OK;
}

CCLAPI 
SdpTsess sdpSessGetTsessAt(SdpSess this_, int index)
{
	SdpTsess		temp;

	if( !this_ )
		return NULL;

	if( this_->sdp_flags_&SDP_FLAG_TIMESESS &&
	    (temp=dxVectorGetElementAt(this_->pTsessList_, index)) ) {
		return temp;
	}
	else
		return NULL;
}

CCLAPI 
RCODE	sdpSessDelTsessAt(SdpSess this_, int index)
{
	SdpTsess		temp;

	if( !this_ )
		return RC_E_PARAM;

	if( this_->sdp_flags_&SDP_FLAG_TIMESESS ) {
		temp = dxVectorRemoveElementAt(this_->pTsessList_,index);
		if( temp ) {
			free(temp);
			if( dxVectorGetSize(this_->pTsessList_)<=0 )
				this_->sdp_flags_ &= ~(SDP_FLAG_TIMESESS);
			return RC_OK;
		}
	}

	return RC_ERROR;
}

CCLAPI 
int					sdpSessGetTsessSize(SdpSess this_)
{
	return (this_)?dxVectorGetSize(this_->pTsessList_):0;
}

CCLAPI 
RCODE sdpSessSetTz(SdpSess this_, SdpTz* timeAdj)
{
	if(!this_)
		return	RC_E_PARAM;
	if(!timeAdj) { /* set connData==NULL to clear this->pTc_*/
		free(this_->pTz_);
		this_->pTz_ = NULL;
		this_->sdp_flags_ &= ~(SDP_FLAG_TIMEADJ);
		return RC_OK;
	}

	if( !(this_->sdp_flags_&SDP_FLAG_TIMEADJ) )
		this_->pTz_ = (SdpTz*)malloc(sizeof(SdpTz));
	
	*(this_->pTz_) = (*timeAdj);		/* copy*/
	this_->sdp_flags_ |= SDP_FLAG_TIMEADJ;

	return RC_OK;
}

CCLAPI 
RCODE sdpSessGetTz(SdpSess this_, SdpTz* timeAdj)
{
	if(!this_)
		return RC_E_PARAM;
	if(!timeAdj)
		return RC_E_PARAM;

	if( this_->sdp_flags_&SDP_FLAG_TIMEADJ ) {
		(*timeAdj) = *(this_->pTz_);
		return RC_OK;
	}
	else
		return RC_ERROR;
}

CCLAPI 
RCODE sdpSessAddTaAt(SdpSess this_, SdpTa* sessionAtt, int index)
{
	SdpTa* temp;
	
	if(!this_)
		return RC_E_PARAM;
	if(!sessionAtt)
		return RC_E_PARAM;
	if(!this_->pTaList_) 
		return RC_ERROR;
	
	temp = (SdpTa*)malloc(sizeof(SdpTa));
	if(!temp)
		return RC_ERROR;
	
	(*temp) = (*sessionAtt);
	if( dxVectorAddElementAt(this_->pTaList_,(void*)temp,index)!=RC_OK ) {
		free(temp);
		return RC_ERROR;
	}
	
	this_->sdp_flags_ |= SDP_FLAG_ATTRIB;
	return RC_OK;
}

CCLAPI 
RCODE sdpSessGetTaAt(SdpSess this_, int index, SdpTa* sessionAtt)
{
	SdpTa		*temp;

	if( !this_ )
		return RC_E_PARAM;
	if( !sessionAtt )
		return RC_E_PARAM;

	if( this_->sdp_flags_&SDP_FLAG_ATTRIB &&
	    (temp=dxVectorGetElementAt(this_->pTaList_, index)) ) {
		(*sessionAtt) = (*temp);
		return RC_OK;
	}
	else
		return RC_ERROR;
}

CCLAPI 
RCODE sdpSessDelTaAt(SdpSess this_, int index)
{
	SdpTa	*temp;

	if( !this_ )
		return RC_E_PARAM;

	if( this_->sdp_flags_&SDP_FLAG_ATTRIB ) {
		temp = dxVectorRemoveElementAt(this_->pTaList_,index);
		if( temp ) {
			free(temp);
			if( dxVectorGetSize(this_->pTaList_)<=0 )
				this_->sdp_flags_ &= ~(SDP_FLAG_ATTRIB);
			return RC_OK;
		}
	}

	return RC_ERROR;
}

CCLAPI 
int sdpSessGetTaSize(SdpSess this_)
{
	return (this_)?dxVectorGetSize(this_->pTaList_):0;
}

CCLAPI 
RCODE sdpSessAddMsessAt(SdpSess this_, SdpMsess mediaSess, int index)
{
	SdpMsess temp;
	
	if(!this_)
		return RC_E_PARAM;
	if(!mediaSess)
		return RC_E_PARAM;

	if(!this_->pMsessList_ ) 
		return RC_ERROR;
	
	
	temp = sdpMsessDup(mediaSess);
	if(!temp)
		return RC_ERROR;
	if( dxVectorAddElementAt(this_->pMsessList_,(void*)temp,index)!=RC_OK ) {
		sdpMsessFree(temp);
		return RC_ERROR;
	}
	
	this_->sdp_flags_ |= SDP_FLAG_MEDIASESS;
	return RC_OK;
}

CCLAPI 
SdpMsess sdpSessGetMsessAt(SdpSess this_, int index)
{
	SdpMsess		temp;

	if( !this_ )
		return NULL;

	if( this_->sdp_flags_&SDP_FLAG_MEDIASESS &&
	    (temp=dxVectorGetElementAt(this_->pMsessList_, index)) ) {
		return temp;
	}
	else
		return NULL;
}

CCLAPI 
RCODE	sdpSessDelMsessAt(SdpSess this_, int index)
{
	SdpMsess		*temp;

	if( !this_ )
		return RC_E_PARAM;

	if( this_->sdp_flags_&SDP_FLAG_MEDIASESS ) {
		temp = dxVectorRemoveElementAt(this_->pMsessList_,index);
		if( temp ) {
			free(temp);
			if( dxVectorGetSize(this_->pMsessList_)<=0 )
				this_->sdp_flags_ &= ~(SDP_FLAG_MEDIASESS);
			return RC_OK;
		}
	}

	return RC_ERROR;
}

CCLAPI 
int sdpSessGetMsessSize(SdpSess this_)
{
	return (this_)?dxVectorGetSize(this_->pMsessList_):0;
}

CCLAPI 
SdpSess sdpSessNewFromText(const char* sdpText)
{
	SdpList	lineList = NULL;
	SdpList	lineListIter = NULL;
	SdpSess	sdp;
	int		i = 0;

	if(!sdpText)		return NULL;

	lineList = lineListIter = sdpLineParse(sdpText);

	/* version*/
	if( lineListIter==NULL ) 
		return NULL;
	if( strncmp(SDP_TV_TOKEN,lineListIter->pToken,strlen(SDP_TV_TOKEN))!=0 ) {
		TCRPrint(1,"<sdpSessNewFromText()>:\"v=\" error.\n");
		return NULL;
	}
	else {
		char*	version = lineListIter->pToken+strlen(SDP_TV_TOKEN);
		sdp = sdpSessNew();
		sdpSessSetTv(sdp,(UINT16)a2i(version));
	}
	lineListIter = lineListIter->pNext;
	if(!lineListIter)
		goto sdp_success;

	/* origin*/
	if( strncmp(SDP_TO_TOKEN,lineListIter->pToken,strlen(SDP_TO_TOKEN))==0 ) {
		SdpTo	origin;
		if( sdpToParse( lineListIter->pToken, &origin)!=RC_OK )
			goto sdp_parse_err;
		sdpSessSetTo(sdp,&origin);

		lineListIter = lineListIter->pNext;
		if(!lineListIter)
			goto sdp_success;
	}

	/* session name*/
	if( strncmp(SDP_TS_TOKEN,lineListIter->pToken,strlen(SDP_TS_TOKEN))==0 ) {
		char* sessName = NULL;
		if( sdpTsNewFromText( lineListIter->pToken, &(sessName))!=RC_OK )
			goto sdp_parse_err;
		sdpSessSetTs(sdp,sessName);
		free(sessName);

		lineListIter = lineListIter->pNext;
		if(!lineListIter)
			goto sdp_success;
	}
	
	/* session info*/
 	if( strncmp(SDP_TI_TOKEN,lineListIter->pToken,strlen(SDP_TI_TOKEN))==0 ) {
		char* sessInfo = NULL;
		if( sdpSiNewFromText( lineListIter->pToken, &(sessInfo))!=RC_OK )
			goto sdp_parse_err;
		sdpSessSetTi(sdp,sessInfo);
		free(sessInfo);

		lineListIter = lineListIter->pNext;
		if(!lineListIter)
			goto sdp_success;
	}

	/* URI*/
	if( strncmp(SDP_TU_TOKEN,lineListIter->pToken,strlen(SDP_TU_TOKEN))==0 ) {
		char* uri = NULL;
		if( sdpTuNewFromText( lineListIter->pToken, &(uri))!=RC_OK )
			goto sdp_parse_err;
		sdpSessSetTu(sdp,uri);
		free(uri);

		lineListIter = lineListIter->pNext;
		if(!lineListIter)
			goto sdp_success;
	}

	/* email*/
	i = 0;
	while( strncmp(SDP_TE_TOKEN,lineListIter->pToken,strlen(SDP_TE_TOKEN))==0 ) {
		char* email;
		if( sdpTeNewFromText( lineListIter->pToken, (&email))!=RC_OK )
			goto sdp_parse_err;
		sdpSessAddTeAt(sdp,email,i);
		free(email);
		
		i++;
		lineListIter = lineListIter->pNext;
		if(!lineListIter)
			goto sdp_success;
	}

	/* phone num*/
	i = 0;
	while( strncmp(SDP_TP_TOKEN,lineListIter->pToken,strlen(SDP_TP_TOKEN))==0 ) {
		char* phonenum;
		if( sdpTpNewFromText( lineListIter->pToken, (&phonenum))!=RC_OK )
			goto sdp_parse_err;
		sdpSessAddTpAt(sdp,phonenum,i);
		free(phonenum);
		
		i++;
		lineListIter = lineListIter->pNext;
		if(!lineListIter)
			goto sdp_success;
	}

	/* Connection Data*/
	if( strncmp(SDP_TC_TOKEN,lineListIter->pToken,strlen(SDP_TC_TOKEN))==0 ) {
		SdpTc	connData;
		if( sdpTcParse( lineListIter->pToken, &(connData) )!=RC_OK )
			goto sdp_parse_err;
		sdpSessSetTc(sdp,&connData);

		lineListIter = lineListIter->pNext;
		if(!lineListIter)
			goto sdp_success;
	}

	/* Bandwidth*/
	i = 0;
	while( strncmp(SDP_TB_TOKEN,lineListIter->pToken,strlen(SDP_TB_TOKEN))==0 ) {
		SdpTb bandWidth;
		if( sdpTbParse( lineListIter->pToken, &(bandWidth) )!=RC_OK )
			goto sdp_parse_err;
		sdpSessAddTbAt(sdp,&bandWidth,i);

		i++;
		lineListIter = lineListIter->pNext;
		if(!lineListIter)
			goto sdp_success;
	}

	/* Time Session*/
	i = 0;
	while( strncmp(SDP_TT_TOKEN,lineListIter->pToken,strlen(SDP_TT_TOKEN))==0 ) {
		int		j = 0;
		SdpTsess	timeSess = sdpTsessNew();
		SdpTt		time;
		if( sdpTtParse( lineListIter->pToken, &(time) )!=RC_OK )
			goto timesess_err;
		sdpTsessSetTt(timeSess,&time);

		i++;
		lineListIter = lineListIter->pNext;
		if(!lineListIter)
			goto timesess_success;

		j = 0;
		while( strncmp(SDP_TR_TOKEN,lineListIter->pToken,strlen(SDP_TR_TOKEN))==0 ) {
			SdpTr	tr;
			if( sdpTrParse(lineListIter->pToken, (&tr))!=RC_OK ) 
				goto timesess_err;
			sdpTsessAddTrAt(timeSess,&tr,j);

			j++;
			lineListIter = lineListIter->pNext;
			if(!lineListIter)
				goto timesess_success;
		}

timesess_success:
		sdpSessAddTsessAt(sdp,timeSess,i-1);
		sdpTsessFree(timeSess);
		if(!lineListIter)
			goto sdp_success; /* fix at SIPit 14th */
		continue;

timesess_err:
		sdpTsessFree(timeSess);
		goto sdp_parse_err;
	}
	

	/* Time Zone Adjustment*/
	if( strncmp(SDP_TZ_TOKEN,lineListIter->pToken,strlen(SDP_TZ_TOKEN))==0 ) {
		SdpTz	timeAdj;
		if( sdpTzParse( lineListIter->pToken, &(timeAdj) )!=RC_OK )
			goto sdp_parse_err;
		sdpSessSetTz(sdp,&timeAdj);

		lineListIter = lineListIter->pNext;
		if(!lineListIter)
			goto sdp_success;
	}

	/* Encry Key*/
	if( strncmp(SDP_TK_TOKEN,lineListIter->pToken,strlen(SDP_TK_TOKEN))==0 ) {
		SdpTk	encryKey;
		if( sdpTkParse( lineListIter->pToken, &(encryKey) )!=RC_OK )
			goto sdp_parse_err;
		sdpSessSetTk(sdp,&encryKey);

		lineListIter = lineListIter->pNext;
		if(!lineListIter)
			goto sdp_success;
	}

	/* Session attribution*/
	i = 0;
	while( strncmp(SDP_TA_TOKEN,lineListIter->pToken,strlen(SDP_TA_TOKEN))==0 ) {
		SdpTa	attrib;
		if( sdpTaParse( lineListIter->pToken, &(attrib) )!=RC_OK )
			goto sdp_parse_err;
		sdpSessAddTaAt(sdp,&attrib,i);

		i++;
		lineListIter = lineListIter->pNext;
		if(!lineListIter)
			goto sdp_success;
	}

	/* Media Announcment*/
	i = 0;
	while( strncmp(SDP_TM_TOKEN,lineListIter->pToken,strlen(SDP_TM_TOKEN))==0 ) {
		int j = 0;
		SdpMsess	mediaSess = sdpMsessNew();
		SdpTm		media;

		if ( !mediaSess )
			break;

		if( sdpTmParse( lineListIter->pToken, &(media) )!=RC_OK ) {
			sdpMsessFree( mediaSess );

			/* skip to next Media Announcment Session */
			lineListIter = lineListIter->pNext;
			if(!lineListIter)
				break;	
			while( strncmp(SDP_TM_TOKEN,lineListIter->pToken,strlen(SDP_TM_TOKEN))!=0 ) {
				lineListIter = lineListIter->pNext;
				if(!lineListIter)
					break;			
			}

			if ( !lineListIter )
				break;
			continue;
		}
		sdpMsessSetTm(mediaSess,&media);

		i++;
		lineListIter = lineListIter->pNext;
		if(!lineListIter)
			goto mediasess_success;

		/*Media Title*/
		if( strncmp(SDP_TI_TOKEN,lineListIter->pToken,strlen(SDP_TI_TOKEN))==0 ) {
			char* sessInfo = NULL;
			if( sdpSiNewFromText( lineListIter->pToken, &(sessInfo))==RC_OK ) {
				sdpMsessSetTi(mediaSess,sessInfo);
				free(sessInfo);
			}

			lineListIter = lineListIter->pNext;
			if( !lineListIter )
				goto mediasess_success;
		}

		/* Connection Data*/
		j = 0;
		while( strncmp(SDP_TC_TOKEN,lineListIter->pToken,strlen(SDP_TC_TOKEN))==0 ) {
			SdpTc	connData;
			if( sdpTcParse( lineListIter->pToken, &(connData) )==RC_OK ) {
				sdpMsessAddTcAt(mediaSess,&connData,j);
				j++;
			}
			
			lineListIter = lineListIter->pNext;
			if(!lineListIter)
				goto mediasess_success;
		}

		/* Bandwidth
		if( strncmp(SDP_TB_TOKEN,lineListIter->pToken,strlen(SDP_TB_TOKEN))==0 ) {
			SdpTb bandWidth;
			if( sdpTbParse( lineListIter->pToken, &(bandWidth) )==RC_OK ) {
				sdpMsessSetTb(mediaSess,&bandWidth);
			}

			lineListIter = lineListIter->pNext;
			if( !lineListIter )
				goto mediasess_success;
		}
		*/
		j = 0;
		while( strncmp(SDP_TB_TOKEN,lineListIter->pToken,strlen(SDP_TB_TOKEN))==0 ) {
			SdpTb bandWidth;
			if( sdpTbParse( lineListIter->pToken, &(bandWidth) )==RC_OK ) {
				sdpMsessAddTbAt(mediaSess,&bandWidth,j);
				j++;
			}
			
			lineListIter = lineListIter->pNext;
			if(!lineListIter)
				goto mediasess_success;
		}

		/* Encrypt Key*/
		if( strncmp(SDP_TK_TOKEN,lineListIter->pToken, strlen(SDP_TK_TOKEN))==0 ) {
			SdpTk	encryKey;
			if( sdpTkParse( lineListIter->pToken, &(encryKey) )==RC_OK ) {
				sdpMsessSetTk(mediaSess,&encryKey);
			}

			lineListIter = lineListIter->pNext;
			if( !lineListIter )
				goto mediasess_success;
		}

		/* Session attribution*/
		j = 0;
		while( strncmp(SDP_TA_TOKEN,lineListIter->pToken,strlen(SDP_TA_TOKEN))==0 ) {
			SdpTa	attrib;
			if( sdpTaParse( lineListIter->pToken, &(attrib) )==RC_OK ) {
				sdpMsessAddTaAt(mediaSess,&attrib,j);
				j++;
			}

			lineListIter = lineListIter->pNext;
			if(!lineListIter)
				goto mediasess_success;
		}

mediasess_success:
		sdpSessAddMsessAt(sdp,mediaSess,i-1);
		sdpMsessFree(mediaSess);
		if(!lineListIter)
			break;
		continue;
	}

sdp_success:
	sdpListFree(lineList);
	return sdp;

sdp_parse_err:
	sdpListFree(lineList);
	sdpSessFree(sdp);
	return NULL;
}

CCLAPI 
RCODE sdpSess2Str(SdpSess this_, char* strbuf, UINT32* buflen)
{
	DxStr			s;
	int			i = 0;
	char*			temp;
	UINT32			tlen = SDP_STRING_SDPSESS_LENGTH;
	UINT32			rlen;
	int			ret = -1;
	char*			p;

	if(!this_||!strbuf||!buflen)				
		return RC_ERROR;

	s = dxStrNew();
	temp = (char*)malloc(tlen);

	/* Version*/
	sprintf(temp,"%s%u%s",
			SDP_TV_TOKEN, this_->Tv_,SDP_CRLF);
	dxStrCat(s,temp);	
		
	if( this_->pTo_ ) {
		rlen = tlen;
		ret = sdpTo2Str(this_->pTo_,(char*)temp,&rlen);
		if( ret==RC_ERROR ) {
			goto SESS_2STR_ERR;
		}
		else if( ret==RC_SDP_BUFFER_LENGTH_INSUFFICIENT ) {
			temp = (char*)realloc(temp,rlen);
			tlen = rlen;
			ret = sdpTo2Str(this_->pTo_,(char*)temp,&rlen);
			if( ret!=RC_OK ) {
				goto SESS_2STR_ERR;
			}
		}
		dxStrCat(s,temp);
	}

	if( this_->pTs_ ) {
		dxStrCat(s,SDP_TS_TOKEN);	
		dxStrCat(s,this_->pTs_);
		dxStrCat(s,SDP_CRLF);	
	}

	if( this_->pTi_ ) {
		dxStrCat(s,SDP_TI_TOKEN);	
		dxStrCat(s,this_->pTi_);
		dxStrCat(s,SDP_CRLF);	
	}

	if( this_->pTu_ ) {
		dxStrCat(s,SDP_TU_TOKEN);	
		dxStrCat(s,this_->pTu_);
		dxStrCat(s,SDP_CRLF);	
	}

	for(i=0; i<dxVectorGetSize(this_->pTeList_); i++) {
		p = (char*)dxVectorGetElementAt(this_->pTeList_,i);
		dxStrCat(s,SDP_TE_TOKEN);	
		dxStrCat(s,p);
		dxStrCat(s,SDP_CRLF);	
	}

	for(i=0; i<dxVectorGetSize(this_->pTpList_); i++) {
		p = (char*)dxVectorGetElementAt(this_->pTpList_,i);
		dxStrCat(s,SDP_TP_TOKEN);	
		dxStrCat(s,p);
		dxStrCat(s,SDP_CRLF);	
	}

	if( this_->pTc_ ) {
		rlen = tlen;
		ret = sdpTc2Str(this_->pTc_,(char*)temp,&rlen);
		if( ret==RC_ERROR ) {
			goto SESS_2STR_ERR;
		}
		else if( ret==RC_SDP_BUFFER_LENGTH_INSUFFICIENT ) {
			temp = (char*)realloc(temp,rlen);
			tlen = rlen;
			ret = sdpTc2Str(this_->pTc_,(char*)temp,&rlen);
			if( ret!=RC_OK ) {
				goto SESS_2STR_ERR;
			}
		}
		dxStrCat(s,temp);
	}

	for(i=0; i<dxVectorGetSize(this_->pTbList_); i++) {
		SdpTb*	bandwidth = (SdpTb*)dxVectorGetElementAt(this_->pTbList_,i);
		rlen = tlen;
		ret = sdpTb2Str(bandwidth,(char*)temp,&rlen);
		if( ret==RC_ERROR ) {
			goto SESS_2STR_ERR;
		}
		else if( ret==RC_SDP_BUFFER_LENGTH_INSUFFICIENT ) {
			temp = (char*)realloc(temp,rlen);
			tlen = rlen;
			ret = sdpTb2Str(bandwidth,(char*)temp,&rlen);
			if( ret!=RC_OK ) {
				goto SESS_2STR_ERR;
			}
		}
		dxStrCat(s,temp);
	}

	for(i=0; i<dxVectorGetSize(this_->pTsessList_); i++) {
		SdpTsess	timeSess = (SdpTsess)dxVectorGetElementAt(this_->pTsessList_,i);
		rlen = tlen;
		ret = sdpTsess2Str(timeSess,(char*)temp,&rlen);
		if( ret==RC_ERROR ) {
			goto SESS_2STR_ERR;
		}
		else if( ret==RC_SDP_BUFFER_LENGTH_INSUFFICIENT ) {
			temp = (char*)realloc(temp,rlen);
			tlen = rlen;
			ret = sdpTsess2Str(timeSess,(char*)temp,&rlen);
			if( ret!=RC_OK ) {
				goto SESS_2STR_ERR;
			}
		}
		dxStrCat(s,temp);
	}

	if( this_->pTz_ ) {
		rlen = tlen;
		ret = sdpTz2Str(this_->pTz_,(char*)temp,&rlen);
		if( ret==RC_ERROR ) {
			goto SESS_2STR_ERR;
		}
		else if( ret==RC_SDP_BUFFER_LENGTH_INSUFFICIENT ) {
			temp = (char*)realloc(temp,rlen);
			tlen = rlen;
			ret = sdpTz2Str(this_->pTz_,(char*)temp,&rlen);
			if( ret!=RC_OK ) {
				goto SESS_2STR_ERR;
			}
		}
		dxStrCat(s,temp);
	}

	if( this_->pTk_ ) {
		rlen = tlen;
		ret = sdpTk2Str(this_->pTk_,(char*)temp,&rlen);
		if( ret==RC_ERROR ) {
			goto SESS_2STR_ERR;
		}
		else if( ret==RC_SDP_BUFFER_LENGTH_INSUFFICIENT ) {
			temp = (char*)realloc(temp,rlen);
			tlen = rlen;
			ret = sdpTk2Str(this_->pTk_,(char*)temp,&rlen);
			if( ret!=RC_OK ) {
				goto SESS_2STR_ERR;
			}
		}
		dxStrCat(s,temp);
	}

	for(i=0; i<dxVectorGetSize(this_->pTaList_); i++) {
		SdpTa*	attrib = (SdpTa*)dxVectorGetElementAt(this_->pTaList_,i);
		rlen = tlen;
		ret = sdpTa2Str(attrib,(char*)temp,&rlen);
		if( ret==RC_ERROR ) {
			goto SESS_2STR_ERR;
		}
		else if( ret==RC_SDP_BUFFER_LENGTH_INSUFFICIENT ) {
			temp = (char*)realloc(temp,rlen);
			tlen = rlen;
			ret = sdpTa2Str(attrib,(char*)temp,&rlen);
			if( ret!=RC_OK ) {
				goto SESS_2STR_ERR;
			}
		}
		dxStrCat(s,temp);
	}

	for(i=0; i<dxVectorGetSize(this_->pMsessList_); i++) {
		SdpMsess	mediaSess = (SdpMsess)dxVectorGetElementAt(this_->pMsessList_,i);
		rlen = tlen;
		ret = sdpMsess2Str(mediaSess,(char*)temp,&rlen);
		if( ret==RC_ERROR ) {
			goto SESS_2STR_ERR;
		}
		else if( ret==RC_SDP_BUFFER_LENGTH_INSUFFICIENT ) {
			temp = (char*)realloc(temp,rlen);
			tlen = rlen;
			ret = sdpMsess2Str(mediaSess,(char*)temp,&rlen);
			if( ret!=RC_OK ) {
				goto SESS_2STR_ERR;
			}
		}
		dxStrCat(s,temp);
	}

	if( (*buflen)<=(UINT32)dxStrLen(s) ) {
		strncpy(strbuf,dxStrAsCStr(s),*buflen-1);
		(*buflen) = (UINT32)dxStrLen(s) + 1;
		dxStrFree(s);
		free(temp);
		return RC_SDP_BUFFER_LENGTH_INSUFFICIENT;
	}
	strcpy(strbuf,dxStrAsCStr(s));
	(*buflen) = (UINT32)dxStrLen(s);
	dxStrFree(s);
	free(temp);

	return RC_OK;

SESS_2STR_ERR:
	dxStrFree(s);
	free(temp);
	return RC_ERROR;
}
