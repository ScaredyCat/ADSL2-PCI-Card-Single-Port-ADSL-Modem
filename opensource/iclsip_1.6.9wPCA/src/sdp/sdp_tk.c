/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * sdp_tk.c
 *
 * $Id: sdp_tk.c,v 1.11 2004/12/01 07:37:47 tyhuang Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sdp_tok.h"
#include "sdp_sess.h"
#include "sdp_tk.h"
#include "sdp_utl.h"
#include <common/cm_utl.h>
#include <adt/dx_str.h>

char ENCRYKEY_METHOD_TOKEN[6][7] = {
	"NONE",
	"prompt",
	"clear",
	"base64",
	"uri",
	"\0"
};

/*--------------------------------------------------------------------------
// Encry key
*/
CCLAPI
RCODE		sdpTkParse(const char* encrykeyText, SdpTk* encrykey)
{
	SdpList	tokList;
	SdpList	tokListIter;
	UINT32	i = 0;
	char	*semi;

	if(!encrykeyText)		return RC_E_PARAM;
	if(!encrykey)			return RC_E_PARAM;

	tokList = tokListIter = sdpTokParse(encrykeyText);

	/* k=*/
	if( tokListIter==NULL )
		return RC_SDP_TK_ERR;
	if( strcmp(SDP_TK_TOKEN,tokListIter->pToken)!=0 )
		return RC_SDP_TK_TOKEN_ERR;
/*	else
//		encrykey = (SdpTk*)malloc(sizeof(SdpTk));*/
	tokListIter = tokListIter->pNext;

	/* method*/
	if( tokListIter==NULL )
		return RC_SDP_TK_ERR;
	if( !tokListIter->pToken )
		return RC_SDP_TK_METHOD_ERR;
	else {
		semi=strchr(tokListIter->pToken,':');
		if (semi) *semi='\0';
/*
		for( i=0; *ENCRYKEY_METHOD_TOKEN[i]!='\0'; i++) {
			if( strncmp(ENCRYKEY_METHOD_TOKEN[i],tokListIter->pToken,strlen(ENCRYKEY_METHOD_TOKEN[i]))==0 ) {
				encrykey->pMethod_ = i;
				goto next_token;
			}
		}
*/
		for( i=0; i<5 ; i++) {
			if( strcmp(ENCRYKEY_METHOD_TOKEN[i],tokListIter->pToken)==0 ) {
				encrykey->pMethod_ = i;
				goto next_token;
			}
		}
		return RC_SDP_TK_METHOD_ERR;
	}
next_token:
/*	tokListIter = tokListIter->pNext; */

	/* key*/
	if( tokListIter==NULL )
		return RC_SDP_TK_ERR;
	if( !tokListIter->pToken )
		return RC_SDP_TK_KEY_ERR;
	else if( i!=SDP_TK_METHOD_PROMPT ) {
		/*char temp[1024] = {"\0"};*/
		char *temp;
		/* skip ":"*/
		/*
		if( strcmp(tokListIter->pToken,":")!=0 )
			return RC_SDP_TK_KEY_ERR;
		
		tokListIter = tokListIter->pNext;
		if( tokListIter==NULL )
			return RC_SDP_TK_ERR;
		if( !tokListIter->pToken )
			return RC_SDP_TK_KEY_ERR;
		*/
	
		temp=encrykey->pKey_;
		memset(temp,0,sizeof(SDP_MAX_TK_KEY_SIZE));
		if (semi){
			strcat(temp,semi+1);
			tokListIter = tokListIter->pNext;
		}
		
		/*
		while( tokListIter && tokListIter->pToken && !isCRLF(tokListIter->pToken) ) {
			strcat(temp,tokListIter->pToken);
			tokListIter = tokListIter->pNext;
		}*/
		while( tokListIter && tokListIter->pToken ) {
			strcat(temp,tokListIter->pToken);
			tokListIter = tokListIter->pNext;
		}
		/*strcpy(encrykey->pKey_,temp);*/
	}
	
	/*
	if (tokListIter) 
		tokListIter = tokListIter->pNext;
	*/

	sdpListFree(tokList);

	return RC_OK;
}

CCLAPI
RCODE			sdpTk2Str(SdpTk* encrykey, char* strbuf, UINT32* buflen)
{
	DxStr		temp = NULL;
	UINT32	i = 0;

	if(!encrykey||!strbuf||!buflen)				
		return RC_ERROR;

	for(i=0; *ENCRYKEY_METHOD_TOKEN[i]!='\0'; i++);
	if( encrykey->pMethod_>=i )	
		return RC_ERROR;

	temp = dxStrNew();
	dxStrCat(temp,(char*)SDP_TK_TOKEN);
	dxStrCat(temp,ENCRYKEY_METHOD_TOKEN[encrykey->pMethod_]);

	if( encrykey->pMethod_!=SDP_TK_METHOD_PROMPT ) {
		dxStrCat(temp,(char*)":");	dxStrCat(temp,encrykey->pKey_);
	}

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


