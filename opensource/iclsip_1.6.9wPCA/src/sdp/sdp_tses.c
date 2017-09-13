/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * sdp_tses.c
 *
 * $Id: sdp_tses.c,v 1.10 2004/09/24 05:47:14 tyhuang Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <adt/dx_vec.h>
#include <adt/dx_str.h>
#include "sdp_sess.h"
#include "sdp_tok.h"
#include "sdp_tses.h"
#include "sdp_utl.h"
#include <common/cm_utl.h>

#define SDP_STRING_TIMESESS_LENGTH	128

struct sdpTsessObj {
	SdpTt			Tt_;		/* mandatory */
	DxVector		pTrList_;	/* optional */
};

CCLAPI 
SdpTsess sdpTsessNew(void)
{
	SdpTsess	timeSess = (SdpTsess)malloc(sizeof(struct sdpTsessObj));

	timeSess->pTrList_ = dxVectorNew(SDP_DEFAULT_LIST_SIZE,SDP_DEFAULT_LIST_SIZE/2,DX_VECTOR_POINTER);
	strcpy(timeSess->Tt_.Endtime_,"0");
	strcpy(timeSess->Tt_.StartTime_,"0");

	return timeSess;
}

SdpTr* sdpTrDup(SdpTr* reptime)
{
	SdpTr*	dup;
	if(!reptime)
		return NULL;

	dup = (SdpTr*)malloc(sizeof(SdpTr));
	(*dup) = (*reptime);

	return dup;
}

CCLAPI 
SdpTsess sdpTsessDup(SdpTsess timeSess)
{
	SdpTsess	dup;
	if(!timeSess)
		return	NULL;

	dup = (SdpTsess)malloc(sizeof(struct sdpTsessObj));
	dup->pTrList_ = dxVectorDup(timeSess->pTrList_,(void*(*)(void*))sdpTrDup);
	dup->Tt_ = timeSess->Tt_;

	return dup;
}

CCLAPI 
void sdpTsessFree(SdpTsess timeSess)
{
	if(!timeSess)
		return;

	dxVectorFree(timeSess->pTrList_, (void(*)(void*))free);
	free(timeSess);
}

CCLAPI 
RCODE	sdpTsessSetTt(SdpTsess timeSess, SdpTt* time)
{
	if(!timeSess)	
		return RC_E_PARAM;
	if(!time)	
		return RC_E_PARAM;

	timeSess->Tt_ = (*time);

	return RC_OK;
}

CCLAPI 
RCODE	sdpTsessGetTt(SdpTsess timeSess, SdpTt* time)
{
	if(!timeSess)	
		return RC_E_PARAM;
	if(!time)	
		return RC_E_PARAM;

	(*time) = timeSess->Tt_;

	return RC_OK;
}

CCLAPI 
RCODE	sdpTsessAddTrAt(SdpTsess timeSess, SdpTr* reptime, int index)
{
	SdpTr*		temp;
	if(!timeSess)	
		return RC_E_PARAM;
	if(!reptime)	
		return RC_E_PARAM;

	temp = (SdpTr*)malloc(sizeof(SdpTr));
	(*temp) = (*reptime);

	if( dxVectorAddElementAt(timeSess->pTrList_,(void*)temp,index)!=RC_OK ) {
		free(temp);
		return RC_ERROR;	
	}
	return RC_OK;
}

CCLAPI 
RCODE	sdpTsessDelTrAt(SdpTsess timeSess, int index)
{
	SdpTr*		temp;
	if(!timeSess)	
		return RC_E_PARAM;

	temp = (SdpTr*)dxVectorRemoveElementAt(timeSess->pTrList_,index);
	if(temp) {
		free(temp);
		return RC_OK;
	}

	return RC_ERROR;
}

CCLAPI 
int sdpTsessGetTrSize(SdpTsess timeSess)
{
	return (timeSess)?dxVectorGetSize(timeSess->pTrList_):0;
}

CCLAPI 
RCODE	sdpTsessGetTrAt(SdpTsess timeSess, int index, SdpTr* reptime)
{
	SdpTr*		temp;

	if(!timeSess)	
		return RC_E_PARAM;
	if(!reptime)	
		return RC_E_PARAM;

	temp = (SdpTr*)dxVectorGetElementAt(timeSess->pTrList_,index);
	if(temp) {
		(*reptime) = (*temp);
		return RC_OK;
	}

	return RC_ERROR;
}

CCLAPI 
RCODE sdpTsess2Str(SdpTsess timeSess, char* strbuf, UINT32* buflen)
{
	DxStr		TimeSess;
	int		i = 0;
	char*		temp;
	UINT32		tlen = SDP_STRING_TIMESESS_LENGTH;
	UINT32		rlen;
	RCODE		ret;
	SdpTr*		pr = NULL;

	if(!timeSess||!strbuf||!buflen)	
		return RC_ERROR;

	TimeSess = dxStrNew();
	temp = (char*)malloc(tlen);

	rlen = tlen;
	ret = sdpTt2Str(&(timeSess->Tt_),temp,&rlen);
	if( ret==RC_ERROR ) {
		goto TSESS_2STR_ERR;
	}
	else if ( ret==RC_SDP_BUFFER_LENGTH_INSUFFICIENT ) {
		temp = (char*)realloc(temp,rlen);
		tlen = rlen;
		ret = sdpTt2Str(&(timeSess->Tt_),temp,&rlen);
		if( ret!=RC_OK ) {
			goto TSESS_2STR_ERR;
		}
	}
	dxStrCat(TimeSess,temp);

	for(i=0; i<dxVectorGetSize(timeSess->pTrList_); i++) {
		pr = (SdpTr*)dxVectorGetElementAt(timeSess->pTrList_,i);
		rlen = tlen;
		ret = sdpTr2Str(pr,temp,&rlen);
		if( ret==RC_ERROR ) {
			goto TSESS_2STR_ERR;
		}
		else if ( ret==RC_SDP_BUFFER_LENGTH_INSUFFICIENT ) {
			temp = (char*)realloc(temp,rlen);
			tlen = rlen;
			ret = sdpTr2Str(pr,temp,&rlen);
			if( ret!=RC_OK ) {
				goto TSESS_2STR_ERR;
			}
		}
		dxStrCat(TimeSess,temp);
	}

	if( (*buflen)<=(UINT32)dxStrLen(TimeSess) ) {
		strncpy(strbuf,dxStrAsCStr(TimeSess),*buflen-1);
		(*buflen) = (UINT32)dxStrLen(TimeSess) + 1;
		dxStrFree(TimeSess);
		free(temp);
		return RC_SDP_BUFFER_LENGTH_INSUFFICIENT;
	}
	strcpy(strbuf,dxStrAsCStr(TimeSess));
	(*buflen) = (UINT32)dxStrLen(TimeSess);
	dxStrFree(TimeSess);
	free(temp);

	return RC_OK;

TSESS_2STR_ERR:
	dxStrFree(TimeSess);
	free(temp);
	return RC_ERROR;
}
