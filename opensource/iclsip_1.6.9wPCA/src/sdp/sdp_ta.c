/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * sdp_ta.c
 *
 * $Id: sdp_ta.c,v 1.13 2004/12/23 09:03:26 tyhuang Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sdp_tok.h"
#include "sdp_sess.h"
#include "sdp_ta.h"
#include <adt/dx_str.h>

/*---------------------------------------------------------------------------
// Session Attribute
*/
CCLAPI
RCODE	sdpTaParse(const char* attText, SdpTa* att)
{
	SdpList	tokList;
	SdpList	tokListIter;
	char *semic=NULL;

	if(!attText)		return RC_E_PARAM;
	if(!att)			return RC_E_PARAM;

	tokList = tokListIter = sdpTokParse(attText);

	/* z=*/
	if( tokListIter==NULL )
		return RC_SDP_TA_ERR;
	if( strcmp(SDP_TA_TOKEN,tokListIter->pToken)!=0 )
		return RC_SDP_TA_TOKEN_ERR;
	tokListIter = tokListIter->pNext;

	att->sessionatt_flag_ = SDP_TA_FLAG_NONE;
	/* attrib*/
	if( tokListIter==NULL )
		return RC_SDP_TA_ERR;
	if( !tokListIter->pToken )
		return RC_SDP_TA_ATTRIB_ERR;
	else if ( strlen(tokListIter->pToken)<SDP_MAX_TA_ATTRIB_SIZE ){
		semic=strchr(tokListIter->pToken,':');
		if (semic) {
			*semic++='\0';
			/* due to upperlayer need ':' */
			strcpy(att->pValue_,":");
			strcat(att->pValue_,semic);
			att->sessionatt_flag_ |= SDP_TA_FLAG_VALUE;
		}
		strcpy(att->pAttrib_,tokListIter->pToken);
	}else
		goto sessionatt_error;
	tokListIter = tokListIter->pNext;	

	/* value*/
	if( tokListIter==NULL )
		goto sessionatt_success;
	if( !tokListIter->pToken )
		return RC_SDP_TA_ATTRIB_ERR;
	else  {
		if (semic) strcat(att->pValue_," ");
		while( tokListIter && tokListIter->pToken /*&& !isCRLF(tokListIter->pToken)*/ ) {
			if( (strlen(tokListIter->pToken)+strlen(att->pValue_))<SDP_MAX_TA_VALUE_SIZE ){
				strcat(att->pValue_,tokListIter->pToken);
			}else
				goto sessionatt_error;
			if((tokListIter = tokListIter->pNext)!=NULL)
				strcat(att->pValue_," ");
		}
		att->sessionatt_flag_ |= SDP_TA_FLAG_VALUE;
	}


sessionatt_success:
	sdpListFree(tokList);

	return RC_OK;

sessionatt_error:
	sdpListFree(tokList);

	return RC_ERROR;
}

CCLAPI
RCODE sdpTa2Str(SdpTa* att, char* strbuf, UINT32* buflen)
{
	DxStr	temp = NULL;

	if(!att||!strbuf||!buflen)	
		return RC_ERROR;

	temp = dxStrNew();

	dxStrCat(temp, (char*)SDP_TA_TOKEN);
	dxStrCat(temp, att->pAttrib_);

	/* check offset*/
	if( att->sessionatt_flag_&SDP_TA_FLAG_VALUE ){
		dxStrCat(temp, att->pValue_);
	}
	dxStrCat(temp,(char*)SDP_CRLF);

	if( (*buflen)<=(UINT32)dxStrLen(temp) ) {
		strncpy( strbuf, dxStrAsCStr(temp), (*buflen)-1);
		(*buflen) = (UINT32)dxStrLen(temp) + 1;
		dxStrFree(temp);
		return RC_SDP_BUFFER_LENGTH_INSUFFICIENT;
	}

	strcpy( strbuf, dxStrAsCStr(temp));
	(*buflen) = (UINT32)dxStrLen(temp);
	dxStrFree(temp);

	return RC_OK;
}

