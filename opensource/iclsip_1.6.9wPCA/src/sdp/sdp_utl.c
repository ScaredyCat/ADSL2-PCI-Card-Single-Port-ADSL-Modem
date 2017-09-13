/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * sdp_utl.c
 *
 * $Id: sdp_utl.c,v 1.8 2004/12/06 09:31:42 tyhuang Exp $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <common/cm_utl.h>
#include "sdp_tok.h"
#include "sdp_sess.h"
#include "sdp_utl.h"

/*---------------------------------------------*/
/* PhoneNum*/
/**/
RCODE	sdpTpNewFromText(char* phonenumText, char** phonenum)
{
/*	SdpList	tokList = NULL;
	SdpList	tokListIter = NULL;

	if(!phonenumText)		return RC_E_PARAM;
	if(!phonenum)			return RC_E_PARAM;

	tokList = tokListIter = sdpTokParse(phonenumText);
*/
	char *semi;
	char temp;

	if(!phonenumText)	return RC_E_PARAM;
	if(!phonenum)	return RC_E_PARAM;

	/* e= */
	semi=strchr(phonenumText,'=');
	if (semi){
		temp=*(semi-1);
		semi++;
	}else
		return RC_SDP_TE_EMAIL_ERR;

	if( 'p'!=temp)
		return RC_SDP_TE_TOKEN_ERR;

	/* e-mail */
	(*phonenum) = strDup(semi);

	/* p=
	if( tokListIter==NULL )
		return RC_SDP_TP_ERR;
	if( strcmp(SDP_TP_TOKEN,tokListIter->pToken)!=0 )
		return RC_SDP_TP_TOKEN_ERR;
	tokListIter = tokListIter->pNext;	
	*/
	/* phone number
	if( tokListIter==NULL )
		return RC_SDP_TP_ERR;
	if( !tokListIter->pToken )
		return RC_SDP_TP_TOKEN_ERR;
	else {
		char temp[1024] = {"\0"};

		while( !isCRLF(tokListIter->pToken) ) {
			strcat(temp,tokListIter->pToken);
			tokListIter = tokListIter->pNext;	

			if( tokListIter==NULL )
				break;
			if( !tokListIter->pToken )
				break;
		}
		(*phonenum) = strDup(temp);
	}

	sdpListFree(tokList);
	*/

	return RC_OK;
}

/*---------------------------------------------*/
/* Email*/
/**/
RCODE	sdpTeNewFromText(char* emailText, char** email)
{
/*
	SdpList	tokList = NULL;
	SdpList	tokListIter = NULL;
*/
	char *semi;
	char temp;

	if(!emailText)	return RC_E_PARAM;
	if(!email)	return RC_E_PARAM;

	/* e= */
	semi=strchr(emailText,'=');
	if (semi){
		temp=*(semi-1);
		semi++;
	}else
		return RC_SDP_TE_EMAIL_ERR;

	if( 'e'!=temp )
		return RC_SDP_TE_TOKEN_ERR;

	/* e-mail */
	(*email) = strDup(semi);


	/*
	tokList = tokListIter = sdpTokParse(emailText);
	*/
	/* e=
	if( tokListIter==NULL )
		return RC_SDP_TE_ERR;
	if( strcmp(SDP_TE_TOKEN,tokListIter->pToken)!=0 )
		return RC_SDP_TE_TOKEN_ERR;
	tokListIter = tokListIter->pNext;	
	*/
	/* email
	if( tokListIter==NULL )
		return RC_SDP_TE_ERR;
	if( !tokListIter->pToken )
		return RC_SDP_TE_EMAIL_ERR;
	else {
		char temp[1024] = {"\0"};

		while( !isCRLF(tokListIter->pToken) ) {
			strcat(temp,tokListIter->pToken);
			tokListIter = tokListIter->pNext;	

			if( tokListIter==NULL )
				break;
			if( !tokListIter->pToken )
				break;
		}
		(*email) = strDup(temp);
	}

	sdpListFree(tokList);
	*/
	
	return RC_OK;
}


/*---------------------------------------------*/
/* URI*/
/**/
RCODE	sdpTuNewFromText(char* uriText, char** uri)
{
/*	SdpList	tokList = NULL;
	SdpList	tokListIter = NULL;

	if(!uriText)	return RC_E_PARAM;
	if(!uri)	return RC_E_PARAM;


	tokList = tokListIter = sdpTokParse(uriText);
*/
	char *semi;
	char temp;

	if(!uriText)	return RC_E_PARAM;
	if(!uri)	return RC_E_PARAM;


	/* u= */
	semi=strchr(uriText,'=');
	if (semi){
		temp=*(semi-1);
		semi++;
	}else
		return RC_SDP_TUURI_ERR;

	if( 'u'!=temp)
		return RC_SDP_TU_TOKEN_ERR;

	/* uri */
	(*uri) = strDup(semi);

	/* u=
	if( tokListIter==NULL )
		return RC_SDP_TUERR;
	if( strcmp(SDP_TU_TOKEN,tokListIter->pToken)!=0 )
		return RC_SDP_TU_TOKEN_ERR;
	tokListIter = tokListIter->pNext;	
	*/
	/* uri
	if( tokListIter==NULL )
		return RC_SDP_TUERR;
	if( !tokListIter->pToken )
		return RC_SDP_TUURI_ERR;
	else {
		char temp[1024] = {"\0"};

		while( !isCRLF(tokListIter->pToken) ) {
			strcat(temp,tokListIter->pToken);
			tokListIter = tokListIter->pNext;	

			if( tokListIter==NULL )
				break;
			if( !tokListIter->pToken )
				break;
		}
		(*uri) = strDup(temp);
	}

	sdpListFree(tokList);
	*/
	return RC_OK;
}

/*---------------------------------------------*/
/* Session name*/
/**/
RCODE	sdpTsNewFromText(char* nameText, char** name)
{
/*	SdpList	tokList = NULL;
	SdpList	tokListIter = NULL;

	if(!nameText)	return RC_E_PARAM;
	if(!name)	return RC_E_PARAM;

	tokList = tokListIter = sdpTokParse(nameText);
*/
	char *semi;
	char temp;

	if(!nameText)	return RC_E_PARAM;
	if(!name)	return RC_E_PARAM;

	/* s= */
	semi=strchr(nameText,'=');
	if (semi) {
		temp=*(semi-1);
		semi++;
	}else
		return RC_SDP_TS_NAME_ERR;

	if( 's'!=temp )
		return RC_SDP_TS_TOKEN_ERR;

	/* name */
	(*name) = strDup(semi);


	/* s=
	if( tokListIter==NULL )
		return RC_SDP_TS_ERR;
	if( strcmp(SDP_TS_TOKEN,tokListIter->pToken)!=0 )
		return RC_SDP_TS_TOKEN_ERR;
	tokListIter = tokListIter->pNext;	
	*/
	/* name
	if( tokListIter==NULL )
		return RC_SDP_TS_ERR;
	if( !tokListIter->pToken )
		return RC_SDP_TS_NAME_ERR;
	else {
		char temp[1024] = {"\0"};

		while( !isCRLF(tokListIter->pToken) ) {
			strcat(temp,tokListIter->pToken);
			tokListIter = tokListIter->pNext;	

			if( tokListIter==NULL )
				break;
			if( !tokListIter->pToken )
				break;
		}
		(*name) = strDup(temp);
	}

	sdpListFree(tokList);
	*/
	return RC_OK;
}

/*---------------------------------------------*/
/* Session infomation*/
/**/
RCODE		sdpSiNewFromText(char* infoText, char** info)
{
/*	SdpList	tokList = NULL;
	SdpList	tokListIter = NULL;

	if(!infoText)		return RC_E_PARAM;
	if(!info)		return RC_E_PARAM;

	tokList = tokListIter = sdpTokParse(infoText);
*/
	char *semi;
	char temp;

	if(!infoText)	return RC_E_PARAM;
	if(!info)	return RC_E_PARAM;

	/* i= */
	semi=strchr(infoText,'=');
	if (semi) {
		temp=*(semi-1);
		semi++;
	}else
		return RC_SDP_TI_INFO_ERR;

	if( 'i'!=temp )
		return RC_SDP_TI_TOKEN_ERR;

	/* name */
	(*info) = strDup(semi);

	/* i=
	if( tokListIter==NULL )
		return RC_SDP_TI_ERR;
	if( strcmp(SDP_TI_TOKEN,tokListIter->pToken)!=0 )
		return RC_SDP_TI_TOKEN_ERR;
	tokListIter = tokListIter->pNext;	
	*/
	/* info
	if( tokListIter==NULL )
		return RC_SDP_TI_ERR;
	if( !tokListIter->pToken )
		return RC_SDP_TI_INFO_ERR;
	else {
		char *equal;
	
		char temp[1024] = {"\0"};
		
		while( 1!isCRLF(tokListIter->pToken) ) {
			strcat(temp,tokListIter->pToken);
			if (tokListIter->pNext) strcat(temp," ");
			tokListIter = tokListIter->pNext;	

			if( tokListIter==NULL )
				break;
			if( !tokListIter->pToken )
				break;
		}
		(*info) = strDup(temp);
	}

	sdpListFree(tokList);
	*/

	return RC_OK;
}

