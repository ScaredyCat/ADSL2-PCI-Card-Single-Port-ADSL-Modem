/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * sdp_tz.c
 *
 * $Id: sdp_tz.c,v 1.8 2004/12/01 05:32:43 tyhuang Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <adt/dx_str.h>
#include "sdp_tok.h"
#include "sdp_sess.h"
#include "sdp_tz.h"
#include "sdp_utl.h"
#include <common/cm_utl.h>

/*---------------------------------------------------------------------------
// Time zone adjustment
*/
CCLAPI
RCODE	sdpTzParse(const char* timeadjText, SdpTz* timeadj)
{
	SdpList	tokList;
	SdpList	tokListIter;
	UINT32	i = 0;

	if(!timeadjText)		return RC_E_PARAM;
	if(!timeadj)			return RC_E_PARAM;

	tokList = tokListIter = sdpTokParse(timeadjText);

	/* z=*/
	if( tokListIter==NULL )
		return RC_SDP_TZ_ERR;
	if( strcmp(SDP_TZ_TOKEN,tokListIter->pToken)!=0 )
		return RC_SDP_TZ_TOKEN_ERR;
/*	else
//		timeadj = (SdpTz*)malloc(sizeof(SdpTz));*/
	tokListIter = tokListIter->pNext;

	/* adjustment*/
	if( tokListIter==NULL )
		return RC_SDP_TZ_ERR;
	else if( !tokListIter->pToken )
		return RC_SDP_TZ_ADJUSTMENT_ERR;
	else {
		for( i=0; (tokListIter)!=NULL ; i++) {
			if( isCRLF(tokListIter->pToken) )
				break;
			if( i>0 ) {
				/*check space
				if( !tokListIter || !tokListIter->pToken )
					return RC_SDP_TZ_ERR;
				if( strncmp(" ",tokListIter->pToken,1)!=0 )
					return RC_SDP_TZ_ERR;
				tokListIter = tokListIter->pNext;
				*/
			}

			if( !tokListIter || tokListIter->pToken==NULL )
				return RC_SDP_TZ_ADJUSTMENT_ERR;
			if( isCRLF(tokListIter->pToken) )
				break;

			strcpy(timeadj->adjustment_[i].pAdjTime_, tokListIter->pToken);
			tokListIter = tokListIter->pNext;

			/*check space
			if( !tokListIter || !tokListIter->pToken )
				return RC_SDP_TZ_ERR;
			if( strncmp(" ",tokListIter->pToken,1)!=0 )
				return RC_SDP_TZ_ERR;
			tokListIter = tokListIter->pNext;
			*/
			if( !tokListIter || !tokListIter->pToken)
				return RC_SDP_TZ_ADJUSTMENT_ERR;
			if( isCRLF(tokListIter->pToken) )
				return RC_SDP_TZ_ADJUSTMENT_ERR;

			strcpy(timeadj->adjustment_[i].pOffset_, tokListIter->pToken);
			tokListIter = tokListIter->pNext;			
		}
	}

	sdpListFree(tokList);
	timeadj->adjustment_size_ = i;

	return RC_OK;
}

CCLAPI
RCODE		sdpTz2Str(SdpTz* timeadj, char* strbuf, UINT32* buflen)
{
	DxStr temp = NULL;
	UINT32 i = 0;

	if(!timeadj||!strbuf||!buflen)
		return RC_ERROR;

	if( timeadj->adjustment_size_<1 )
		return RC_ERROR;

	temp = dxStrNew();
	dxStrCat(temp,SDP_TZ_TOKEN);
	dxStrCat(temp,timeadj->adjustment_[0].pAdjTime_);
	dxStrCat(temp," ");
	dxStrCat(temp,timeadj->adjustment_[0].pOffset_);

	/* check offset*/
	for( i=1; i<timeadj->adjustment_size_; i++) {
		dxStrCat(temp," "); 
		dxStrCat(temp, timeadj->adjustment_[i].pAdjTime_);
		dxStrCat(temp," "); 
		dxStrCat(temp, timeadj->adjustment_[i].pOffset_);
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



