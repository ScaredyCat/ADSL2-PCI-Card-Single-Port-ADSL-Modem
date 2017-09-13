/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * sdp_tt.c
 *
 * $Id: sdp_tt.c,v 1.8 2004/12/01 05:32:43 tyhuang Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <adt/dx_str.h>
#include "sdp_tok.h"
#include "sdp_sess.h"
#include "sdp_tt.h"
#include "sdp_utl.h"
#include <common/cm_utl.h>

/*--------------------------------------------------------------------------
// time
*/
CCLAPI 
RCODE	sdpTtParse(const char* timeText, SdpTt* time)
{
	SdpList	tokList;
	SdpList	tokListIter;

	if(!timeText)		return RC_E_PARAM;
	if(!time)			return RC_E_PARAM;

	tokList = tokListIter = sdpTokParse(timeText);

	/* t=*/
	if( tokListIter==NULL )
		return RC_SDP_TT_ERR;
	if( strcmp(SDP_TT_TOKEN,tokListIter->pToken)!=0 )
		return RC_SDP_TT_TOKEN_ERR;
/*	else
//		time = (SdpTt*)malloc(sizeof(SdpTt));*/
	tokListIter = tokListIter->pNext;

	/* start time*/
	if( tokListIter==NULL )
		return RC_SDP_TT_ERR;
	if( !tokListIter->pToken )
		return RC_SDP_TT_STARTTIME_TOKEN_ERR;
	else 
		strcpy(time->StartTime_, tokListIter->pToken);
	tokListIter = tokListIter->pNext;

	/*check space
	if( !tokListIter || !tokListIter->pToken )
		return RC_SDP_TR_ERR;
	if( strncmp(" ",tokListIter->pToken,1)!=0 )
		return RC_SDP_TR_ERR;
	tokListIter = tokListIter->pNext;
	*/
	/* end time*/
	if( tokListIter==NULL )
		return RC_SDP_TT_ERR;
	if( !tokListIter->pToken )
		return RC_SDP_TT_ENDTIME_TOKEN_ERR;
	else 
		strcpy(time->Endtime_,tokListIter->pToken);
	tokListIter = tokListIter->pNext;
	
	
	sdpListFree(tokList);

	return RC_OK;
}

CCLAPI 
RCODE sdpTt2Str(SdpTt* time, char* strbuf, UINT32* buflen)
{
	DxStr temp = NULL;

	if(!time||!strbuf||!buflen)
		return RC_ERROR;

	temp = dxStrNew();
	dxStrCat(temp,SDP_TT_TOKEN);
	dxStrCat(temp,time->StartTime_);
	dxStrCat(temp," ");
	dxStrCat(temp,time->Endtime_);
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

