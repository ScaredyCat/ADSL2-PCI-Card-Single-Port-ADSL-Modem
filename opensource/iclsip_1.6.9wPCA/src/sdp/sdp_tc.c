/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * sdp_tc.c
 *
 * $Id: sdp_tc.c,v 1.9 2004/12/01 07:37:47 tyhuang Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sdp_tok.h"
#include "sdp_sess.h"
#include "sdp_tc.h"
#include "sdp_utl.h"
#include <common/cm_utl.h>
#include <adt/dx_str.h>


extern char SDP_NETTYPE_TOKEN[3][5];
extern char SDP_ADDRTYPE_TOKEN[4][5];

/*--------------------------------------------------------------------------
// Connection Data
*/
CCLAPI
RCODE		sdpTcParse(const char* conndataText, SdpTc* conndata)
{
	SdpList	tokList;
	SdpList	tokListIter;

	if(!conndataText)		return RC_E_PARAM;
	if(!conndata)			return RC_E_PARAM;

	/*tokList = tokListIter = sdpTokParseC(conndataText);*/
	tokList = tokListIter = sdpTokParse(conndataText);

	/* c=*/
	if( tokListIter==NULL )
		return RC_SDP_TC_ERR;
	if( strcmp(SDP_TC_TOKEN,tokListIter->pToken)!=0 )
		return RC_SDP_TC_TOKEN_ERR;
/*	else
//		conndata = (SdpTc*)malloc(sizeof(SdpTc));*/
	tokListIter = tokListIter->pNext;

	conndata->conn_flags_ = SDP_TC_FLAG_NONE; /* set flags to none first*/

	/* network type*/
	if( tokListIter==NULL )
		return RC_SDP_TC_ERR;
	if( !tokListIter->pToken )
		return RC_SDP_TC_NETWORKTYPE_TOKEN_ERR;
	else {
		int i = 0;
		while( *SDP_NETTYPE_TOKEN[i]!='\0' ) {
			if( strcmp(SDP_NETTYPE_TOKEN[i],tokListIter->pToken)==0 ) {
				conndata->networkType_ = i;
				goto networkType_success;
			}
			i++;
		}
		return RC_SDP_TC_NETWORKTYPE_TOKEN_ERR;
	}
networkType_success:
	tokListIter = tokListIter->pNext;

	/*check space
	if( !tokListIter || !tokListIter->pToken )
		return RC_SDP_TC_ERR;
	if( strncmp(" ",tokListIter->pToken,1)!=0 )
		return RC_SDP_TC_ERR;
	tokListIter = tokListIter->pNext;
	*/
	/* address type*/
	if( tokListIter==NULL )
		return RC_SDP_TC_ERR;
	if( !tokListIter->pToken )
		return RC_SDP_TC_ADDRESSTYPE_TOKEN_ERR;
	else {
		int i = 0;
		while( *SDP_ADDRTYPE_TOKEN[i]!='\0' ) {
			if( strcmp(SDP_ADDRTYPE_TOKEN[i],tokListIter->pToken)==0 ) {
				conndata->addressType_ = i;
				goto addressType_success;
			}
			i++;
		}
		return RC_SDP_TC_ADDRESSTYPE_TOKEN_ERR;
	}
addressType_success:
	tokListIter = tokListIter->pNext;

	/*check space
	if( !tokListIter || !tokListIter->pToken )
		return RC_SDP_TC_ERR;
	if( strncmp(" ",tokListIter->pToken,1)!=0 )
		return RC_SDP_TC_ERR;
	tokListIter = tokListIter->pNext;
	*/
	/* address*/
	if( tokListIter==NULL )
		return RC_SDP_TC_ERR;
	if( !tokListIter->pToken )
		return RC_SDP_TC_ADDRESS_TOKEN_ERR;
	else
		strcpy(conndata->pConnAddr_, tokListIter->pToken);
	tokListIter = tokListIter->pNext;

	/* ttl*/
	if( tokListIter==NULL )
		goto conndata_success;
	if( !tokListIter->pToken )
		return RC_SDP_TC_ADDRESS_TOKEN_ERR;
	else {
		if( strcmp("/",tokListIter->pToken)!=0 )
			goto conndata_success;

		tokListIter = tokListIter->pNext;
		if( tokListIter==NULL )
			return RC_SDP_TC_ERR;
		if( !tokListIter->pToken )
			return RC_SDP_TC_TTL_TOKEN_ERR;

		conndata->ttl_ = a2ui(tokListIter->pToken);
		conndata->conn_flags_ |= SDP_TC_FLAG_TTL;
	}
	tokListIter = tokListIter->pNext;

	/* number of address*/
	if( tokListIter==NULL )
		goto conndata_success;
	if( !tokListIter->pToken )
		return RC_SDP_TC_NUMADDR_TOKEN_ERR;
	else {
		if( strcmp("/",tokListIter->pToken)!=0 )
			goto conndata_success;

		tokListIter = tokListIter->pNext;
		if( tokListIter==NULL )
			return RC_SDP_TC_ERR;
		if( !tokListIter->pToken )
			return RC_SDP_TC_NUMADDR_TOKEN_ERR;

		conndata->numOfAddr_ = a2ui(tokListIter->pToken);
		conndata->conn_flags_ |= SDP_TC_FLAG_NUMOFADDR;
	}
	tokListIter = tokListIter->pNext;
	
	
conndata_success:	
	sdpListFree(tokList);


	return RC_OK;
}

CCLAPI
RCODE			sdpTc2Str(SdpTc* conndata, char* strbuf, UINT32* buflen)
{
	DxStr	temp;
	char	i2atemp[12];
	int	i = 0;

	if(!conndata||!strbuf||!buflen)				
		return RC_ERROR;

	for(i=0; *SDP_NETTYPE_TOKEN[i]!='\0'; i++);
	if( conndata->networkType_>=i )	
		return RC_ERROR;

	for(i=0; *SDP_ADDRTYPE_TOKEN[i]!='\0'; i++);
	if( conndata->addressType_>=i )	
		return RC_ERROR;

	temp = dxStrNew();
	dxStrCat(temp,(char*)SDP_TC_TOKEN);
	dxStrCat(temp,SDP_NETTYPE_TOKEN[conndata->networkType_]);
	dxStrCat(temp,(char*)" ");
	dxStrCat(temp,SDP_ADDRTYPE_TOKEN[conndata->addressType_]);
	dxStrCat(temp,(char*)" ");
	dxStrCat(temp,conndata->pConnAddr_);

	/* check TTL*/
	if( conndata->conn_flags_&SDP_TC_FLAG_TTL ) {
		dxStrCat(temp,(char*)"/"); dxStrCat(temp, i2a(conndata->ttl_,i2atemp,12));
	}
	/* check num of address*/
	if( conndata->conn_flags_&SDP_TC_FLAG_NUMOFADDR ) {
		dxStrCat(temp,(char*)"/"); dxStrCat(temp, i2a(conndata->numOfAddr_,i2atemp,12));
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

