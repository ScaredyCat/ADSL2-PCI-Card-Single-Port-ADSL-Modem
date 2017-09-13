/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * sdp_tm.c
 *
 * $Id: sdp_tm.c,v 1.14 2006/12/05 09:41:33 tyhuang Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sdp_tok.h"
#include "sdp_sess.h"
#include "sdp_tm.h"
#include "sdp_utl.h"
#include <common/cm_utl.h>
#include <adt/dx_str.h>

char MEDIA_TYPE_TOKEN[6][12] = {
	"NONE",
	"audio",
	"video",
	"application",
	"data",
	"\0"
};

char MEDIA_PROTOCOL_TOKEN[5][8] = {
	"NONE",
	"RTP/AVP",
	"udp",
	"tcp",
	"\0"
};

/*---------------------------------------------
// Media Announcement
*/
CCLAPI
RCODE		sdpTmParse(const char* mediaText, SdpTm* media)
{
	SdpList	tokList;
	SdpList	tokListIter;
	char *slash;

	if(!mediaText)          return RC_E_PARAM;
	if(!media)              return RC_E_PARAM;

	tokList = tokListIter = sdpTokParse(mediaText);

	/* m=*/
	if( tokListIter==NULL )
		return RC_SDP_TM_ERR;
	if( strcmp(SDP_TM_TOKEN,tokListIter->pToken)!=0 )
		return RC_SDP_TM_TOKEN_ERR;
/*	else
		media = (SdpTm*)malloc(sizeof(SdpTm));*/
	tokListIter = tokListIter->pNext;

	media->Tm_flags_ = SDP_TM_FLAG_NONE;

	/* media type*/
	if( tokListIter==NULL )
		return RC_SDP_TM_ERR;
	if( !tokListIter->pToken )
		return RC_SDP_TM_MEDIATYPE_ERR;
	else {
		int i = 0, l = 0;
		media->unknown_tm_type_[0] = '\0';
		
		while( *MEDIA_TYPE_TOKEN[i]!='\0' ) {
			if (*tokListIter->pToken==MEDIA_TYPE_TOKEN[i][0]){
				if( strcmp(MEDIA_TYPE_TOKEN[i],tokListIter->pToken)==0 ) {
					media->Tm_type_ = i;
					goto mediaType_success;
				}
			}
			i++;
		}
		
		/* media type unknown */
		media->Tm_type_ = SDP_TM_MEDIA_TYPE_UNKNOWN;
		l = strlen( tokListIter->pToken );
		if ( l < SDP_MAX_TM_TOKEN_SIZE ) {
			strcpy( media->unknown_tm_type_, tokListIter->pToken);
		}
		else {
			strncpy( media->unknown_tm_type_
			         , tokListIter->pToken
			         , SDP_MAX_TM_TOKEN_SIZE - 1 );
		}
	}
mediaType_success:
	tokListIter = tokListIter->pNext;	

	/*check space
	if( !tokListIter || !tokListIter->pToken )
		return RC_SDP_TM_ERR;
	if( strncmp(" ",tokListIter->pToken,1)!=0 )
		return RC_SDP_TM_ERR;
	tokListIter = tokListIter->pNext;
	*/
	/* port*/
	if( tokListIter==NULL )
		return RC_SDP_TM_ERR;
	if( !tokListIter->pToken )
		return RC_SDP_TM_PORT_ERR;
	else {
		if( *(tokListIter->pToken)=='$' )
			media->Tm_flags_ |= SDP_TM_FLAG_PORT_CHOOSE;

		if ((slash=strchr(tokListIter->pToken,'/'))) {
			/* num of port */
			*slash='\0';
			media->numOfPort_ = atoi(slash+1);
			media->Tm_flags_ |= SDP_TM_FLAG_NUMOFPORT;
		}
		media->port_ = atoi(tokListIter->pToken);
	}
	tokListIter = tokListIter->pNext;

	/* num of port 
	if( tokListIter==NULL )
		return RC_SDP_TM_ERR;
	if( !tokListIter->pToken )
		return RC_SDP_TM_NUMOFPORT_ERR;
	else if( strcmp("/",tokListIter->pToken)==0 ) {
		tokListIter = tokListIter->pNext;
		if( tokListIter==NULL )
			return RC_SDP_TM_ERR;
		if( !tokListIter->pToken )
			return RC_SDP_TM_NUMOFPORT_ERR;

		media->numOfPort_ = atoi(tokListIter->pToken);
		tokListIter = tokListIter->pNext;
	}
	*/
	
	/*check space
	if( !tokListIter || !tokListIter->pToken )
		return RC_SDP_TM_ERR;
	if( strncmp(" ",tokListIter->pToken,1)!=0 )
		return RC_SDP_TM_ERR;
	tokListIter = tokListIter->pNext;
	*/
	/* protocol*/
	if( tokListIter==NULL )
		return RC_SDP_TM_ERR;
	if( !tokListIter->pToken )
		return RC_SDP_TM_PROTOCOL_ERR;
	else  {
		int i = 0;
	/*	char temp[128] = {"\0"};

		while( strcmp(" ",tokListIter->pToken)!=0 ) {
			strcat(temp,tokListIter->pToken);

			tokListIter = tokListIter->pNext;
			if( tokListIter==NULL )
				return RC_SDP_TM_ERR;
		}

		strcat(temp,tokListIter->pToken);	
	*/
		while( *MEDIA_PROTOCOL_TOKEN[i]!='\0' ) {

			if (*tokListIter->pToken==MEDIA_PROTOCOL_TOKEN[i][0]) {
				if( strcmp(MEDIA_PROTOCOL_TOKEN[i],tokListIter->pToken)==0 ) {
					media->protocol_  = i;
					goto protocol_success;
				}
			}
			i++;
		}
		return RC_SDP_TM_PROTOCOL_ERR;
	}
protocol_success:

	tokListIter = tokListIter->pNext;
	/* fmt*/
	if( tokListIter==NULL )
		return RC_SDP_TM_ERR;
	if( !tokListIter->pToken )
		return RC_SDP_TM_FMT_ERR;
	else  {
		UINT32 i = 0;

		while( /*!isCRLF(tokListIter->pToken) && */i<SDP_MAX_TM_FMT_SIZE )
		{
			int fmtlen = 0;
			
			/*check space
			if( strncmp(" ",tokListIter->pToken,1)!=0 )
				return RC_SDP_TM_FMT_ERR;
			tokListIter = tokListIter->pNext;
			*/
			if( !tokListIter || !tokListIter->pToken )
				return RC_SDP_TM_FMT_ERR;
			if( !tokListIter->pToken )
				return RC_SDP_TM_FMT_ERR;

			fmtlen = strlen( tokListIter->pToken );
			if( fmtlen < SDP_MAX_TM_TOKEN_SIZE ) 
				strcpy( media->fmtList_[i], tokListIter->pToken );
			else 
				strncpy( media->fmtList_[i]
				         , tokListIter->pToken
				         , SDP_MAX_TM_TOKEN_SIZE-1 );
			i++;
			tokListIter = tokListIter->pNext;
			if( !tokListIter || !tokListIter->pToken )
				break;

			/*lost add last one 
			i++;*/
		}
		media->fmtSize_ = i;
	}

	sdpListFree(tokList);

	return RC_OK;
}

CCLAPI
RCODE			sdpTm2Str(SdpTm* media, char* strbuf, UINT32* buflen)
{
	DxStr 		temp = NULL;
	char		i2atemp[12];
	UINT32 	i = 0;

	if(!media||!strbuf||!buflen)				
		return RC_ERROR;

	if( media->Tm_type_ > SDP_TM_MEDIA_TYPE_UNKNOWN ||
	    media->Tm_type_ < SDP_TM_MEDIA_TYPE_NONE )	
		return RC_ERROR;

	for(i=0; *MEDIA_PROTOCOL_TOKEN[i]!='\0'; i++);
	if( media->protocol_>=i )	
		return RC_ERROR;

	temp = dxStrNew();
	if( !temp )
		return RC_ERROR;
		
	/* m= */	
	dxStrCat(temp,(char*)SDP_TM_TOKEN);
	
	/* media type */
	if ( media->Tm_type_ != SDP_TM_MEDIA_TYPE_UNKNOWN ) {
		dxStrCat(temp,MEDIA_TYPE_TOKEN[media->Tm_type_]);
	}
	else {
		dxStrCat(temp, media->unknown_tm_type_);			
	}
	dxStrCat(temp," ");

	if( media->Tm_flags_&SDP_TM_FLAG_PORT_CHOOSE ) 
		dxStrCat(temp,(char*)"$");
	else 
		dxStrCat(temp, i2a(media->port_,i2atemp,12));

	if( media->Tm_flags_&SDP_TM_FLAG_NUMOFPORT ) {
		dxStrCat(temp,(char*)"/");	dxStrCat(temp,i2a(media->numOfPort_,i2atemp,12));
	}

	dxStrCat(temp,(char*)" ");	dxStrCat(temp, MEDIA_PROTOCOL_TOKEN[media->protocol_]);

	for(i=0; i<media->fmtSize_; i++) {
		dxStrCat(temp,(char*)" ");	
		dxStrCat(temp, media->fmtList_[i] );
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

