/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * sdp_tr.c
 *
 * $Id: sdp_tr.c,v 1.8 2004/12/01 05:31:04 tyhuang Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <adt/dx_str.h>
#include "sdp_tok.h"
#include "sdp_sess.h"
#include "sdp_tr.h"
#include "sdp_utl.h"
#include <common/cm_utl.h>

/*--------------------------------------------------------------------------
// repeat time
*/
CCLAPI
RCODE		sdpTrParse(const char* reptimeText, SdpTr* reptime)
{
	SdpList	tokList;
	SdpList	tokListIter;
	UINT32	i = 0;

	if(!reptimeText)		return RC_E_PARAM;
	if(!reptime)			return RC_E_PARAM;

	tokList = tokListIter = sdpTokParse(reptimeText);

	/* r=*/
	if( tokListIter==NULL )
		return RC_SDP_TR_ERR;
	if( strcmp(SDP_TR_TOKEN,tokListIter->pToken)!=0 )
		return RC_SDP_TR_TOKEN_ERR;
/*	else
//		reptime = (SdpTr*)malloc(sizeof(SdpTr));*/
	tokListIter = tokListIter->pNext;

	/* repeate-interval*/
	if( tokListIter==NULL )
		return RC_SDP_TR_ERR;
	if( !tokListIter->pToken )
		return RC_SDP_TR_REPEATINTERVAL_SIZE_TOKEN_ERR;
	else 
		strcpy(reptime->repeatInterval_, tokListIter->pToken);
	tokListIter = tokListIter->pNext;

	/*check space
	if( !tokListIter || !tokListIter->pToken )
		return RC_SDP_TR_ERR;
	if( strncmp(" ",tokListIter->pToken,1)!=0 )
		return RC_SDP_TR_ERR;
	tokListIter = tokListIter->pNext;
	*/
	/* active-duration*/
	if( tokListIter==NULL )
		return RC_SDP_TR_ERR;
	if( !tokListIter->pToken )
		return RC_SDP_TR_ACTIVEDURATION_SIZE_TOKEN_ERR;
	else 
		strcpy(reptime->activeDuration_,tokListIter->pToken);
	tokListIter = tokListIter->pNext;

	/*offsets*/
	if( tokListIter==NULL )
		return RC_SDP_TR_ERR;
	else if( !tokListIter->pToken )
		return RC_SDP_TR_OFFSET_SIZE_ERR;
	else {
		for( i=0; (tokListIter)!=NULL ; i++) {
			if( isCRLF(tokListIter->pToken) )
				break;
			/*check space
			if( !tokListIter || !tokListIter->pToken )
				return RC_SDP_TR_ERR;
			if( strncmp(" ",tokListIter->pToken,1)!=0 )
				return RC_SDP_TR_ERR;
			tokListIter = tokListIter->pNext;
			*/
			if( tokListIter->pToken==NULL )
				return RC_SDP_TR_OFFSET_SIZE_ERR;
			if( isCRLF(tokListIter->pToken) )
				break;

			strcpy(reptime->pOffsetList_[i],tokListIter->pToken);
			tokListIter = tokListIter->pNext;
		}
	}

	sdpListFree(tokList);
	reptime->offsetlist_size_ = i;

	return RC_OK;
}

CCLAPI
RCODE			sdpTr2Str(SdpTr* reptime, char* strbuf, UINT32* buflen)
{
	DxStr temp = NULL;
	UINT32 i = 0;

	if(!reptime||!strbuf||!buflen)				
		return RC_ERROR;

	temp = dxStrNew();
	dxStrCat(temp,SDP_TR_TOKEN);
	dxStrCat(temp,reptime->repeatInterval_);
	dxStrCat(temp," ");
	dxStrCat(temp,reptime->activeDuration_);

	/* check offset*/
	if( reptime->offsetlist_size_<1 ) {	/* one or more offsets*/
		dxStrFree(temp);
		return RC_ERROR;
	}
	else {
		for( i=0; i<reptime->offsetlist_size_; i++) {
			dxStrCat(temp," "); dxStrCat(temp, reptime->pOffsetList_[i]);
		}
	}
	dxStrCat(temp,SDP_CRLF);

	if( (*buflen)<=(UINT32)dxStrLen(temp) ) {
		strncpy( strbuf, dxStrAsCStr(temp), (*buflen)-1);
		(*buflen) = (UINT32)dxStrLen(temp) + 1;
		dxStrFree(temp);
		return RC_SDP_BUFFER_LENGTH_INSUFFICIENT;
	}
	strcpy(strbuf,dxStrAsCStr(temp));
	(*buflen) = (UINT32)dxStrLen(temp);
	dxStrFree(temp);

	return RC_OK;
}

