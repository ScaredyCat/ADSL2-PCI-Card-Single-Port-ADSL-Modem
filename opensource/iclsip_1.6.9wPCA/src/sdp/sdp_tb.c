/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * sdp_tb.c
 *
 * $Id: sdp_tb.c,v 1.9 2006/12/05 09:41:33 tyhuang Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sdp_tok.h"
#include "sdp_sess.h"
#include "sdp_tb.h"
#include "sdp_utl.h"
#include <common/cm_utl.h>
#include <adt/dx_str.h>

/*
static char BANDWIDTH_MODIFIER_TOKEN[7][5] = {
	"NONE",
	"CT",
	"AS",
	"X-",
	"RR",
	"RS",
	"\0"
};
*/
/*--------------------------------------------------------------------------
// Bandwidth Data
*/
CCLAPI
RCODE		sdpTbParse(const char* bandwidthText, SdpTb* bandwidth)
{
	SdpList	tokList;
	SdpList	tokListIter;
	char *semi;

	if(!bandwidthText)		return RC_E_PARAM;
	if(!bandwidth)			return RC_E_PARAM;

	tokList = tokListIter = sdpTokParse(bandwidthText);

	/* b=*/
	if( tokListIter==NULL )
		return RC_SDP_TB_ERR;
	if( strcmp(SDP_TB_TOKEN,tokListIter->pToken)!=0 )
		return RC_SDP_TB_TOKEN_ERR;
/*	else
		bandwidth = (SdpTb*)malloc(sizeof(SdpTb));*/
	tokListIter = tokListIter->pNext;

	/* modifier*/
	if( tokListIter==NULL )
		return RC_SDP_TB_ERR;
	if( !tokListIter->pToken )
		return RC_SDP_TB_MODIFIER_ERR;
	else {
		int i = 0;
		semi=strchr(tokListIter->pToken,':');
		if (semi) *semi='\0';
		while( *BANDWIDTH_MODIFIER_TOKEN[i]!='\0' ) {
			if( strncmp(BANDWIDTH_MODIFIER_TOKEN[i],tokListIter->pToken,strlen(BANDWIDTH_MODIFIER_TOKEN[i]))==0 ) {
				bandwidth->modifier_ = i;
				if(i==SDP_TB_MODIFIER_EXTENSION)
					strcpy(bandwidth->pExtentionName, (tokListIter->pToken+strlen(BANDWIDTH_MODIFIER_TOKEN[i])) );
				goto modifier_success;
			}
			i++;
		}
		return RC_SDP_TB_MODIFIER_ERR;
	}
modifier_success:
	/*tokListIter = tokListIter->pNext;*/

	/* skip ":"
	if( tokListIter==NULL )
		return RC_SDP_TB_ERR;
	if( strcmp(tokListIter->pToken,":")!=0 )
		return RC_SDP_TB_MODIFIER_ERR;
	tokListIter = tokListIter->pNext;
	*/

	/* bandwidth
	if( tokListIter==NULL )
		return RC_SDP_TB_ERR;
	if( !tokListIter->pToken )
		return RC_SDP_TB_BANDWIDTH_ERR;
	else 
		bandwidth->bandwidth_ = a2ui(tokListIter->pToken);
	tokListIter = tokListIter->pNext;
	*/

	if (semi) bandwidth->bandwidth_ = a2ui(semi+1);

	sdpListFree(tokList);

	return RC_OK;
}

CCLAPI
RCODE			sdpTb2Str(SdpTb* bandwidth, char* strbuf, UINT32* buflen)
{
	DxStr		temp = NULL;
	char		i2atemp[12];
	UINT32	i = 0;

	if(!bandwidth||!strbuf||!buflen)				
		return RC_ERROR;

	for(i=0; *BANDWIDTH_MODIFIER_TOKEN[i]!='\0'; i++);
	if( bandwidth->modifier_>=i )	
		return RC_ERROR;

	temp = dxStrNew();

	dxStrCat( temp, (char*)SDP_TB_TOKEN);
	dxStrCat( temp, BANDWIDTH_MODIFIER_TOKEN[bandwidth->modifier_]);
	
	if( bandwidth->modifier_==SDP_TB_MODIFIER_EXTENSION ) 
		dxStrCat(temp,bandwidth->pExtentionName);
	
	dxStrCat(temp,(char*)":");
	dxStrCat(temp,i2a(bandwidth->bandwidth_,i2atemp,12));
	dxStrCat(temp,(char*)SDP_CRLF);

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

