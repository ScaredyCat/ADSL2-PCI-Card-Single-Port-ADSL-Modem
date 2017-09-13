/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * sdp_to.c
 *
 * $Id: sdp_to.c,v 1.10 2004/12/01 07:37:47 tyhuang Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <adt/dx_str.h>
#include "sdp_tok.h"
#include "sdp_sess.h"
#include "sdp_to.h"
#include "sdp_utl.h"
#include <common/cm_utl.h>

extern char SDP_NETTYPE_TOKEN[3][5];
extern char SDP_ADDRTYPE_TOKEN[4][5];

/*--------------------------------------------------------------------------
// Origin
*/
CCLAPI
RCODE	sdpToParse(const char* originText, SdpTo* origin)
{
	SdpList	tokList;
	SdpList	tokListIter;

	if(!originText)		return RC_E_PARAM;
	if(!origin)			return RC_E_PARAM;

	/*tokList = tokListIter = sdpTokParseC(originText);*/
	tokList = tokListIter = sdpTokParse(originText);

	
	/* o=*/
	if( tokListIter==NULL )
		return RC_SDP_TO_ERR;
	if( strcmp(SDP_TO_TOKEN,tokListIter->pToken)!=0 )
		return RC_SDP_TO_TOKEN_ERR;
/*	else
//		origin = (SdpTo*)malloc(sizeof(SdpTo));*/
	tokListIter = tokListIter->pNext;

	
	/* username*/
	if( tokListIter==NULL )
		return RC_SDP_TO_ERR;
	if( !tokListIter->pToken )
		return RC_SDP_TO_USERNAME_TOKEN_ERR;
	else {
		/*
		origin->pUsername_[0] = 0;
		while( strncmp(" ",tokListIter->pToken,1)!=0 ) {
			strcat(origin->pUsername_,tokListIter->pToken);

			tokListIter = tokListIter->pNext;
			if( !tokListIter || !tokListIter->pToken )
				return RC_SDP_TO_USERNAME_TOKEN_ERR;
		}
		*/
		strcpy(origin->pUsername_,tokListIter->pToken);
	}
	tokListIter = tokListIter->pNext;

	/*check space
	if( !tokListIter || !tokListIter->pToken )
		return RC_SDP_TO_ERR;
	if( strncmp(" ",tokListIter->pToken,1)!=0 )
		return RC_SDP_TO_ERR;
	tokListIter = tokListIter->pNext;
	*/
	/* sessionID*/
	if( tokListIter==NULL )
		return RC_SDP_TO_ERR;
	if( !tokListIter->pToken )
		return RC_SDP_TO_SESSIONID_TOKEN_ERR;
	else 
		origin->sessionID_ = a2ui(tokListIter->pToken);

	tokListIter = tokListIter->pNext;
	/*check space
	if( !tokListIter || !tokListIter->pToken )
		return RC_SDP_TO_ERR;
	if( strncmp(" ",tokListIter->pToken,1)!=0 )
		return RC_SDP_TO_ERR;
	tokListIter = tokListIter->pNext;
	*/
	/* version*/
	if( tokListIter==NULL )
		return RC_SDP_TO_ERR;
	if( !tokListIter->pToken )
		return RC_SDP_TO_VERSION_TOKEN_ERR;
	else
		origin->version_ = a2ui(tokListIter->pToken);

	tokListIter = tokListIter->pNext;
	
	/*check space
	if( !tokListIter || !tokListIter->pToken )
		return RC_SDP_TO_ERR;
	if( strncmp(" ",tokListIter->pToken,1)!=0 )
		return RC_SDP_TO_ERR;
	tokListIter = tokListIter->pNext;
	*/
	/* network type*/
	if( tokListIter==NULL )
		return RC_SDP_TO_ERR;
	if( !tokListIter->pToken )
		return RC_SDP_TO_NETWORKTYPE_TOKEN_ERR;
	else {
		int i = 0;
		while( *SDP_NETTYPE_TOKEN[i]!='\0' ) {
			if( strcmp(SDP_NETTYPE_TOKEN[i],tokListIter->pToken)==0 ) {
				origin->networkType_ = i;
				goto networkType_success;
			}
			i++;
		}
		return RC_SDP_TO_NETWORKTYPE_TOKEN_ERR;
	}
networkType_success:
	tokListIter = tokListIter->pNext;

	/*check space
	if( !tokListIter || !tokListIter->pToken )
		return RC_SDP_TO_ERR;
	if( strncmp(" ",tokListIter->pToken,1)!=0 )
		return RC_SDP_TO_ERR;
	tokListIter = tokListIter->pNext;
	*/
	/* address type*/
	if( tokListIter==NULL )
		return RC_SDP_TO_ERR;
	if( !tokListIter->pToken )
		return RC_SDP_TO_ADDRESSTYPE_TOKEN_ERR;
	else {
		int i = 0;
		while( *SDP_ADDRTYPE_TOKEN[i]!='\0' ) {
			if( strcmp(SDP_ADDRTYPE_TOKEN[i],tokListIter->pToken)==0 ) {
				origin->addressType_ = i;
				goto addressType_success;
			}
			i++;
		}
		return RC_SDP_TO_ADDRESSTYPE_TOKEN_ERR;
	}
addressType_success:
	tokListIter = tokListIter->pNext;

	/*check space
	if( !tokListIter || !tokListIter->pToken )
		return RC_SDP_TO_ERR;
	if( strncmp(" ",tokListIter->pToken,1)!=0 )
		return RC_SDP_TO_ERR;
	tokListIter = tokListIter->pNext;
	*/
	/* address*/
	if( tokListIter==NULL )
		return RC_SDP_TO_ERR;
	if( !tokListIter->pToken )
		return RC_SDP_TO_ADDRESS_TOKEN_ERR;
	else
		strcpy(origin->pAddress_, tokListIter->pToken);

	sdpListFree(tokList);


	return RC_OK;
}

CCLAPI
RCODE sdpTo2Str(SdpTo* origin, char* strbuf, UINT32* buflen)
{
	DxStr	temp = NULL;
	char	num[128];
	int	i = 0;

	if(!origin||!strbuf||!buflen)				
		return RC_ERROR;

	for(i=0; *SDP_NETTYPE_TOKEN[i]!='\0'; i++);
	if( origin->networkType_>=i )	
		return RC_ERROR;

	for(i=0; *SDP_ADDRTYPE_TOKEN[i]!='\0'; i++);
	if( origin->addressType_>=i )	
		return RC_ERROR;

	temp = dxStrNew();
	dxStrCat(temp,(char*)SDP_TO_TOKEN);
	dxStrCat(temp,origin->pUsername_);
	dxStrCat(temp," ");
	num[0] = 0;
	sprintf(num,"%lu %u ",origin->sessionID_,origin->version_);
	dxStrCat(temp,num);
	dxStrCat(temp,SDP_NETTYPE_TOKEN[origin->networkType_]);
	dxStrCat(temp," ");
	dxStrCat(temp,SDP_ADDRTYPE_TOKEN[origin->addressType_]);
	dxStrCat(temp," ");
	dxStrCat(temp,origin->pAddress_);
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


