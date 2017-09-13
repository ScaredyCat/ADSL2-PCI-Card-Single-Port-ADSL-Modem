/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * sdp_mses.c
 *
 * $Id: sdp_mses.c,v 1.13 2006/12/05 09:41:33 tyhuang Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <adt/dx_vec.h>
#include "sdp_tok.h"
#include "sdp_sess.h"
#include "sdp_tm.h"
#include "sdp_utl.h"
#include <common/cm_utl.h>
#include <adt/dx_str.h>

#define SDP_STRING_MSESS_LENGTH	256

struct sdpMsessObj
{
	SdpTm		Tm_;		/* mandatory */
	char*		pTi_;		/* optional */
	SdpTb*		pTb_;		/* optional */
	SdpTk*		pTk_;		/* optional */
	DxVector	pTcList_;	/* optional */
	DxVector	pTaList_;	/* optional */
	DxVector	pTbList_;	/* optional */
};


CCLAPI
SdpMsess	sdpMsessNew(void)
{
	SdpMsess	sdp = (SdpMsess)malloc(sizeof(struct sdpMsessObj));

	sdp->pTaList_ = dxVectorNew(SDP_DEFAULT_LIST_SIZE,SDP_DEFAULT_LIST_SIZE/2,DX_VECTOR_POINTER);
	sdp->pTcList_ = dxVectorNew(SDP_DEFAULT_LIST_SIZE,SDP_DEFAULT_LIST_SIZE/2,DX_VECTOR_POINTER);
	sdp->pTbList_ = dxVectorNew(SDP_DEFAULT_LIST_SIZE,SDP_DEFAULT_LIST_SIZE/2,DX_VECTOR_POINTER);	
	sdp->Tm_.fmtSize_ = 0;
	sdp->Tm_.Tm_flags_ = SDP_TM_FLAG_NONE;
	sdp->pTb_ = NULL;
	sdp->pTk_ = NULL;
	sdp->pTi_ = NULL;

	return sdp;
}

SdpTa*	sdpTaDup(SdpTa*	attrib)
{
	SdpTa*	temp;

	if(!attrib)
		return NULL;
		
	temp = (SdpTa*)malloc(sizeof(SdpTa));
	*temp = *attrib;

	return temp;
}

SdpTc*	sdpTcDup(SdpTc*	conndata)
{
	SdpTc*	temp;

	if(!conndata)
		return NULL;
		
	temp = (SdpTc*)malloc(sizeof(SdpTc));
	*temp = *conndata;

	return temp;
}

SdpTb* sdpTbDup(SdpTb* b)
{
	SdpTb* temp;
	if(!b)
		return NULL;
	temp=(SdpTb*)calloc(1,sizeof(SdpTb));
	*temp=*b;

	return temp;
}

CCLAPI
SdpMsess	sdpMsessDup(SdpMsess mediaSess)
{
	SdpMsess	sdp;
		
	if(!mediaSess)
		return NULL;

	sdp = (SdpMsess)calloc(1,sizeof(struct sdpMsessObj));

	if (!sdp) return NULL;

	sdp->pTaList_ = dxVectorDup(mediaSess->pTaList_,(void*(*)(void*))sdpTaDup);
	sdp->pTcList_ = dxVectorDup(mediaSess->pTcList_,(void*(*)(void*))sdpTcDup);
	sdp->pTbList_ = dxVectorDup(mediaSess->pTbList_,(void*(*)(void*))sdpTbDup);
	sdp->Tm_ = mediaSess->Tm_;
/*	sdp->pTb_ = NULL;
	sdp->pTk_ = NULL;
	sdp->pTi_ = NULL;
*/
	if( mediaSess->pTb_ ) {
		sdp->pTb_ = malloc(sizeof(SdpTb));
		(*sdp->pTb_) = (*mediaSess->pTb_);
	}
	if( mediaSess->pTk_ ) {
		sdp->pTk_ = (SdpTk*)malloc(sizeof(SdpTk));
		(*sdp->pTk_) = (*mediaSess->pTk_);
	}
	if( mediaSess->pTi_ ) 
		sdp->pTi_ = strDup(mediaSess->pTi_);

	return sdp;
}

CCLAPI
void		sdpMsessFree(SdpMsess mediaSess)
{
	if(!mediaSess)
		return;

	dxVectorFree(mediaSess->pTaList_, (void(*)(void*))free);
	dxVectorFree(mediaSess->pTcList_, (void(*)(void*))free);
	dxVectorFree(mediaSess->pTbList_, (void(*)(void*))free);
	if(mediaSess->pTb_)
		free(mediaSess->pTb_);
	if(mediaSess->pTk_)
		free(mediaSess->pTk_);
	if(mediaSess->pTi_)
		free(mediaSess->pTi_);

	free(mediaSess);
}

CCLAPI
RCODE		sdpMsessSetTm(SdpMsess mediaSess, SdpTm* media)
{
	if(!mediaSess)
		return RC_E_PARAM;
	if(!media) 
		return RC_E_PARAM;
			
	mediaSess->Tm_ = (*media);
	
	return RC_OK;
}

CCLAPI
RCODE		sdpMsessGetTm(SdpMsess mediaSess, SdpTm* media)
{
	if(!mediaSess)
		return RC_E_PARAM;
	if(!media) 
		return RC_E_PARAM;

	(*media) = mediaSess->Tm_;

	return RC_OK;
}

CCLAPI
RCODE		sdpMsessSetTi(SdpMsess mediaSess, char* title)
{
	if(!mediaSess)
		return RC_E_PARAM;
	if(!title) {
		free(mediaSess->pTi_);
		mediaSess->pTi_ = NULL;
		return RC_OK;
	}
	
	if(mediaSess->pTi_)
		free(mediaSess->pTi_);
	mediaSess->pTi_ = strDup((const char*)title);

	return RC_OK;
}

CCLAPI
RCODE		sdpMsessGetTi(SdpMsess mediaSess, char* strbuf, UINT32* buflen)
{
	int		rlen = 0;

	if(!mediaSess)
		return RC_E_PARAM;
	if(!strbuf||!buflen)
		return RC_E_PARAM;

	if( mediaSess->pTi_ ) {
		rlen = strlen(mediaSess->pTi_);
		if( *buflen<=(UINT32)rlen ) {
			strncpy(strbuf,mediaSess->pTi_,*buflen-1);
			(*buflen) = rlen + 1;
			return RC_SDP_BUFFER_LENGTH_INSUFFICIENT;
		}
		strcpy(strbuf,mediaSess->pTi_);
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
RCODE		sdpMsessSetTb(SdpMsess mediaSess, SdpTb* bandwidth)
{
	if(!mediaSess)
		return RC_E_PARAM;
	if(!bandwidth) {
		free(mediaSess->pTb_);
		mediaSess->pTb_ = NULL;
		return RC_OK;
	}

	if(!mediaSess->pTb_) 
		mediaSess->pTb_ = (SdpTb*)malloc(sizeof(SdpTb));

	*(mediaSess->pTb_) = *bandwidth;

	return RC_OK;	
}

CCLAPI
RCODE		sdpMsessGetTb(SdpMsess mediaSess, SdpTb* bandwidth)
{
	if(!mediaSess)
		return RC_E_PARAM;
	if(!mediaSess->pTb_) 
		return RC_SDP_VALUE_NOT_SET;
	if(!bandwidth)
		return RC_E_PARAM;

	*bandwidth = *(mediaSess->pTb_);

	return RC_OK;	
}

CCLAPI
RCODE		sdpMsessSetTk(SdpMsess mediaSess, SdpTk* encrykey)
{
	if(!mediaSess)
		return RC_E_PARAM;
	if(!encrykey) {
		free(mediaSess->pTk_);
		mediaSess->pTk_ = NULL;
		return RC_OK;
	}

	if(!mediaSess->pTk_) 
		mediaSess->pTk_ = (SdpTk*)malloc(sizeof(SdpTk));

	*(mediaSess->pTk_) = *encrykey;

	return RC_OK;
}

CCLAPI
RCODE		sdpMsessGetTk(SdpMsess mediaSess, SdpTk* encrykey)
{
	if(!mediaSess)
		return RC_E_PARAM;
	if(!mediaSess->pTk_) 
		return RC_SDP_VALUE_NOT_SET;
	if(!encrykey) 
		return RC_E_PARAM;

	*encrykey = *(mediaSess->pTk_);

	return RC_OK;
}

CCLAPI
RCODE		sdpMsessAddTcAt(SdpMsess mediaSess, SdpTc* conndata, int index)
{
	SdpTc*	temp;

	if(!mediaSess)
		return RC_E_PARAM;
	if(!conndata) 
		return RC_E_PARAM;

	temp = (SdpTc*)malloc(sizeof(SdpTc));
	*temp = *conndata;

	if( dxVectorAddElementAt(mediaSess->pTcList_,temp,index)!=RC_OK ) {
		free(temp);
		return RC_ERROR;		
	}
	return RC_OK;
}

CCLAPI
RCODE		sdpMsessDelTcAt(SdpMsess mediaSess, int index)
{
	SdpTc*	temp;

	if(!mediaSess)
		return RC_E_PARAM;

	if( (temp = (SdpTc*)dxVectorRemoveElementAt(mediaSess->pTcList_,index)) ) {
		free(temp);
		return RC_OK;
	}

	return RC_ERROR;
}

CCLAPI
int		sdpMsessGetTcSize(SdpMsess mediaSess)
{
	return (mediaSess)?dxVectorGetSize(mediaSess->pTcList_):0;
}

CCLAPI
RCODE		sdpMsessGetTcAt(SdpMsess mediaSess, int index, SdpTc* conndata)
{
	SdpTc*	temp;

	if(!mediaSess)
		return RC_E_PARAM;
	if(!conndata) 
		return RC_E_PARAM;

	if( (temp = (SdpTc*)dxVectorGetElementAt(mediaSess->pTcList_,index)) ) {
		*conndata = *temp;
		return RC_OK;
	}

	return RC_ERROR;
}

CCLAPI
RCODE		sdpMsessAddTaAt(SdpMsess mediaSess, SdpTa* attrib, int index)
{
	SdpTa*	temp;

	if(!mediaSess)
		return RC_E_PARAM;
	if(!attrib) 
		return RC_E_PARAM;

	temp = (SdpTa*)malloc(sizeof(SdpTa));
	*temp = *attrib;

	if( dxVectorAddElementAt(mediaSess->pTaList_,temp,index)!=RC_OK ) {
		free(temp);
		return RC_ERROR;		
	}
	return RC_OK;
}

CCLAPI
RCODE		sdpMsessDelTaAt(SdpMsess mediaSess, int index)
{
	SdpTc*	temp;

	if(!mediaSess)
		return RC_E_PARAM;

	if( (temp = (SdpTc*)dxVectorRemoveElementAt(mediaSess->pTaList_,index)) ) {
		free(temp);
		return RC_OK;
	}

	return RC_ERROR;
}

CCLAPI
int		sdpMsessGetTaSize(SdpMsess mediaSess)
{
	return (mediaSess)?dxVectorGetSize(mediaSess->pTaList_):0;
}

CCLAPI
RCODE		sdpMsessGetTaAt(SdpMsess mediaSess, int index, SdpTa* attrib)
{
	SdpTa*	temp;

	if(!mediaSess)
		return RC_E_PARAM;
	if(!attrib) 
		return RC_E_PARAM;

	if( (temp = (SdpTa*)dxVectorGetElementAt(mediaSess->pTaList_,index)) ) {
		*attrib = *temp;
		return RC_OK;
	}

	return RC_ERROR;
}

CCLAPI
RCODE		sdpMsessAddTbAt(SdpMsess mediaSess, SdpTb* bindwidth, int index)
{
	SdpTb*	temp;
	
	if(!mediaSess)
		return RC_E_PARAM;
	if(!bindwidth) 
		return RC_E_PARAM;
	
	temp = (SdpTb*)malloc(sizeof(SdpTb));
	*temp = *bindwidth;
	
	if( dxVectorAddElementAt(mediaSess->pTbList_,temp,index)!=RC_OK ) {
		free(temp);
		return RC_ERROR;		
	}
	return RC_OK;
}

CCLAPI
RCODE		sdpMsessDelTbAt(SdpMsess mediaSess, int index)
{
	SdpTb*	temp;
	
	if(!mediaSess)
		return RC_E_PARAM;
	
	if( (temp = (SdpTb*)dxVectorRemoveElementAt(mediaSess->pTbList_,index)) ) {
		free(temp);
		return RC_OK;
	}
	
	return RC_ERROR;
}

CCLAPI
int		sdpMsessGetTbSize(SdpMsess mediaSess)
{
	return (mediaSess)?dxVectorGetSize(mediaSess->pTbList_):0;
}

CCLAPI
RCODE		sdpMsessGetTbAt(SdpMsess mediaSess, int index, SdpTb* bandwidth)
{
	SdpTb*	temp;
	
	if(!mediaSess)
		return RC_E_PARAM;
	if(!bandwidth) 
		return RC_E_PARAM;
	
	if( (temp = (SdpTb*)dxVectorGetElementAt(mediaSess->pTbList_,index)) ) {
		*bandwidth = *temp;
		return RC_OK;
	}
	
	return RC_ERROR;
}

CCLAPI
RCODE		sdpMsess2Str(SdpMsess mediaSess, char* strbuf, UINT32* buflen)
{
	DxStr			MediaSess;
	char*			mtemp;
	UINT32			mlen = SDP_STRING_MSESS_LENGTH;
	UINT32			rlen;
	RCODE			ret;
	int			i = 0;

	if( !mediaSess||!strbuf||!buflen ) 
		return RC_ERROR;

	MediaSess = dxStrNew();
	mtemp = (char*)malloc(mlen);
	mtemp[0] = 0;

	rlen = mlen;
	ret = sdpTm2Str(&(mediaSess->Tm_),(char*)mtemp,&rlen);
	if( ret==RC_ERROR ) {
		goto MSESS_2STR_ERR;
	}
	else if( ret==RC_SDP_BUFFER_LENGTH_INSUFFICIENT ) {
		mtemp = (char*)realloc(mtemp,rlen);
		mlen = rlen;
		ret = sdpTm2Str(&(mediaSess->Tm_),(char*)mtemp,&rlen);
		if( ret!=RC_OK ) {
			goto MSESS_2STR_ERR;
		}
	}
	dxStrCat(MediaSess, mtemp);

	/* media title*/
	if( mediaSess->pTi_ ) {
		dxStrCat(MediaSess,(char*)SDP_TI_TOKEN);	
		dxStrCat(MediaSess,mediaSess->pTi_);
		dxStrCat(MediaSess,(char*)SDP_CRLF);	
	}

	/* connection data*/
	for( i=0; i<dxVectorGetSize(mediaSess->pTcList_); i++) {
		SdpTc	*connData = (SdpTc*)dxVectorGetElementAt(mediaSess->pTcList_,i);

		rlen = mlen;
		ret = sdpTc2Str(connData,(char*)mtemp,&rlen);
		if( ret==RC_ERROR ) {
			goto MSESS_2STR_ERR;
		}
		else if( ret==RC_SDP_BUFFER_LENGTH_INSUFFICIENT ) {
			mtemp = (char*)realloc(mtemp,rlen);
			mlen = rlen;
			ret = sdpTc2Str(connData,(char*)mtemp,&rlen);
			if( ret!=RC_OK ) {
				goto MSESS_2STR_ERR;
			}
		}		
		dxStrCat(MediaSess,(char*)mtemp);
	}

	/* bandwidth
	if( mediaSess->pTb_ ) {
		rlen = mlen;
		ret = sdpTb2Str(mediaSess->pTb_,(char*)mtemp,&rlen);
		if( ret==RC_ERROR ) {
			goto MSESS_2STR_ERR;
		}
		else if( ret==RC_SDP_BUFFER_LENGTH_INSUFFICIENT ) {
			mtemp = (char*)realloc(mtemp,rlen);
			mlen = rlen;
			ret = sdpTb2Str(mediaSess->pTb_,(char*)mtemp,&rlen);
			if( ret!=RC_OK ) {
				goto MSESS_2STR_ERR;
			}
		}
		dxStrCat(MediaSess,(char*)mtemp);
	}
	*/

	for( i=0; i<dxVectorGetSize(mediaSess->pTbList_); i++) {
		SdpTb	*bandwidth = (SdpTb*)dxVectorGetElementAt(mediaSess->pTbList_,i);
		
		rlen = mlen;
		ret = sdpTb2Str(bandwidth,(char*)mtemp,&rlen);
		if( ret==RC_ERROR ) {
			goto MSESS_2STR_ERR;
		}
		else if( ret==RC_SDP_BUFFER_LENGTH_INSUFFICIENT ) {
			mtemp = (char*)realloc(mtemp,rlen);
			mlen = rlen;
			ret = sdpTb2Str(bandwidth,(char*)mtemp,&rlen);
			if( ret!=RC_OK ) {
				goto MSESS_2STR_ERR;
			}
		}		
		dxStrCat(MediaSess,(char*)mtemp);
	}

	/* encrykey*/
	if( mediaSess->pTk_ ) {
		rlen = mlen;
		ret = sdpTk2Str(mediaSess->pTk_,(char*)mtemp,&rlen);
		if( ret==RC_ERROR ) {
			goto MSESS_2STR_ERR;
		}
		else if( ret==RC_SDP_BUFFER_LENGTH_INSUFFICIENT ) {
			mtemp = (char*)realloc(mtemp,rlen);
			mlen = rlen;
			ret = sdpTk2Str(mediaSess->pTk_,(char*)mtemp,&rlen);
			if( ret!=RC_OK ) {
				goto MSESS_2STR_ERR;
			}
		}
		dxStrCat(MediaSess,(char*)mtemp);
	}

	/* attrib*/
	for( i=0; i<dxVectorGetSize(mediaSess->pTaList_); i++) {
		SdpTa	*attrib = (SdpTa*)dxVectorGetElementAt(mediaSess->pTaList_,i);
		rlen = mlen;
		ret = sdpTa2Str(attrib,(char*)mtemp,&rlen);
		if( ret==RC_ERROR ) {
			goto MSESS_2STR_ERR;
		}
		else if( ret==RC_SDP_BUFFER_LENGTH_INSUFFICIENT ) {
			mtemp = (char*)realloc(mtemp,rlen);
			mlen = rlen;
			ret = sdpTa2Str(attrib,(char*)mtemp,&rlen);
			if( ret!=RC_OK ) {
				goto MSESS_2STR_ERR;
			}
		}
		dxStrCat(MediaSess,(char*)mtemp);
	}

	if( (*buflen)<=(UINT32)dxStrLen(MediaSess) ) {
		strncpy(strbuf,dxStrAsCStr(MediaSess),*buflen-1);
		(*buflen) = (UINT32)dxStrLen(MediaSess) + 1;
		dxStrFree(MediaSess);
		free(mtemp);
		return RC_SDP_BUFFER_LENGTH_INSUFFICIENT;
	}
	strcpy(strbuf,dxStrAsCStr(MediaSess));
	(*buflen) = (UINT32)dxStrLen(MediaSess);
	dxStrFree(MediaSess);
	free(mtemp);

	return RC_OK;

MSESS_2STR_ERR:
	dxStrFree(MediaSess);
	free(mtemp);
	return RC_ERROR;
}





