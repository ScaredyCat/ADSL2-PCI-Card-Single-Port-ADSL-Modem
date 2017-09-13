/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * sdp_tok.c
 *
 * $Id: sdp_tok.c,v 1.20 2005/03/22 06:21:39 tyhuang Exp $
 */

#include <string.h>
#include <stdlib.h>

#include <adt/dx_str.h>
#include "sdp_tok.h"
#include <common/cm_utl.h>

static char SDP_FIELDS[16][3] =	{ "v=","o=","s=","i=","u=","e=","p=","b=",
				  "c=","r=","z=","t=","k=","a=","m=","\0" };

/* modified for IPv6 */
static char SDP_TOKEN_SEPERATOR[3][2] =	{ ":","/","\0" };
static char SDP_TOKEN_SEPERATOR_C[2][2] =	{ "/","\0" };

static char SDP_SPACE[3][2] = { " ","\t","\0" }; 

int isCRLF(const char* str)
{
	if( !str ) return 0;
	if( strlen(str)<1 )	return 0;


	if( strlen(str)>1 && strncmp(SDP_CRLF,str,strlen(SDP_CRLF))==0 )
		return 2;
	/*
	if( strlen(str)>=1 && 
	    ( strncmp("\r",str,1)==0 ||
	      strncmp("\n",str,1)==0 )
	  )
		return 1;
	*/

	if( strlen(str)>=1 && ( *str=='\r' || *str=='\n'))
		return 1;
	return 0;
}

int isSDPField(const char* field)
{
	int i = 0;

	if(!field) return 0;

/*
	for( i=0; *SDP_FIELDS[i]!='\0'; i++) {
		if( strncmp(SDP_FIELDS[i],field,strlen(SDP_FIELDS[i]))==0 )
			return 1;
	}
*/
	if (!strchr(field,'=')) return 0;
	for( i=0; *SDP_FIELDS[i]!='\0'; i++) {
		if( *field==*SDP_FIELDS[i] )
			return 1;
	}

	return 0;
}

int isSDPTokenSep(const char* field)
{
	int i = 0;

	if(!field) return 0;

	for( i=0; *SDP_TOKEN_SEPERATOR[i]!='\0'; i++) {
		if( strncmp(SDP_TOKEN_SEPERATOR[i],field,strlen(SDP_TOKEN_SEPERATOR[i]))==0 )
			return 1;
	}

	return 0;
}

int isSDPTokenSepC(const char* field)
{
	int i = 0;

	if(!field) return 0;

	for( i=0; *SDP_TOKEN_SEPERATOR_C[i]!='\0'; i++) {
		if( strncmp(SDP_TOKEN_SEPERATOR_C[i],field,strlen(SDP_TOKEN_SEPERATOR[i]))==0 )
			return 1;
	}

	return 0;
}

int isSDPSpace(const char* field)
{
	int i = 0;

	if(!field) return 0;

	for( i=0; *SDP_SPACE[i]!='\0'; i++) {
		if( strncmp(SDP_SPACE[i],field,strlen(SDP_SPACE[i]))==0 )
			return 1;
	}

	return 0;
}

SdpList sdpTokParse(const char* pSdpStr)
{
	char*		temp = NULL;
	char*		p;
	SdpList		tokList,nexttok;
	int 		i = -1;
	int 		j = -1;

	if(!pSdpStr||*pSdpStr=='\0')	
		return NULL;
/*	
	while( 1 ) {
		i++;
		if( isSDPSpace( (pSdpStr+i) ) )
			continue;
		else
			break;
	}
	p = (char*)(pSdpStr+i);
	if( *p=='\0' )
		return NULL;

	if( i>0 ) {
		tokList = (SdpList)malloc(sizeof(SdpListElm));
		tokList->pToken = strDup(" ");
		tokList->pNext = sdpTokParse(p);

		return tokList;
	}

	if( isSDPField(p) ) {
		j = 2;
	} 
	else if( isCRLF(p) ) {
		j = isCRLF(p);
	}
	else if( isSDPTokenSep(p) ) {
		j = 1;
	}
	else {
		while(1) {
			j++;
			if( p[j]!='\0' &&
				!isCRLF( (p+j) ) &&
				!isSDPField( (p+j) ) &&
				!isSDPTokenSep( (p+j) ) &&
				!isSDPSpace( (p+j) ) ) {
				continue;
			} 
			else 
				break;
		}
	}

	temp = (char*)malloc(sizeof(char)*(j+1));
	strncpy(temp,p,j);	temp[j] = 0;

	tokList = (SdpList)malloc(sizeof(SdpListElm));
	tokList->pToken = temp;
	tokList->pNext = sdpTokParse(p+j);
*/
	
	tokList = (SdpList)calloc(1,sizeof(SdpListElm));
	if (!tokList) return NULL;
	nexttok=tokList;

	/* parse header part */
	p=strchr(pSdpStr,'=');
	if (!p) {
		free(tokList);
		return NULL;
	}

	j=p-pSdpStr+1;
	temp = (char*)calloc(1,sizeof(char)*(j+1));
	strncpy(temp,pSdpStr,j);
	nexttok->pToken=temp;
	
	/* parse value */
	pSdpStr=p+1;
	while (pSdpStr) {
		nexttok->pNext = (SdpList)calloc(1,sizeof(SdpListElm));
		nexttok=nexttok->pNext;
		if (!nexttok) break;
		p=strchr(pSdpStr,' ');
		if (p){
			j=p-pSdpStr;
			temp = (char*)calloc(1,sizeof(char)*(j+1));
			strncpy(temp,pSdpStr,j);
			nexttok->pToken=temp;
		}else{
			p=strchr(pSdpStr,'\r');
			if (!p) p=strchr(pSdpStr,'\n'); 
			if (p) {
				j=p-pSdpStr;
				temp = (char*)calloc(1,sizeof(char)*(j+1));
				memcpy(temp,pSdpStr,j);
				nexttok->pToken=temp;
			}else
				nexttok->pToken=strDup(pSdpStr);
			break;
		}
		pSdpStr=p+1;
	}
	return tokList;
}

/*
SdpList sdpTokParseC(const char* pSdpStr)
{
	char*		temp = NULL;
	char*		p;
	SdpList		tokList;
	int 		i = -1;
	int 		j = -1;

	if(!pSdpStr||*pSdpStr=='\0')	
		return NULL;
	
	while( 1 ) {
		i++;
		if( isSDPSpace( (pSdpStr+i) ) )
			continue;
		else
			break;
	}
	p = (char*)(pSdpStr+i);
	if( *p=='\0' )
		return NULL;

	if( i>0 ) {
		tokList = (SdpList)malloc(sizeof(SdpListElm));
		tokList->pToken = strDup(" ");
		tokList->pNext = sdpTokParseC(p);

		return tokList;
	}

	if( isSDPField(p) ) {
		j = 2;
	} 
	else if( isCRLF(p) ) {
		j = isCRLF(p);
	}
	else if( isSDPTokenSepC(p) ) {
		j = 1;
	}
	else {
		while(1) {
			j++;
			if( p[j]!='\0' &&
				!isCRLF( (p+j) ) &&
				!isSDPField( (p+j) ) &&
				!isSDPTokenSepC( (p+j) ) &&
				!isSDPSpace( (p+j) ) ) {
				continue;
			} 
			else 
				break;
		}
	}

	temp = (char*)malloc(sizeof(char)*(j+1));
	strncpy(temp,p,j);	temp[j] = 0;

	tokList = (SdpList)malloc(sizeof(SdpListElm));
	tokList->pToken = temp;
	tokList->pNext = sdpTokParseC(p+j);
	
	return tokList;
}
*/

SdpList sdpLineParse(const char* pSdpStr)
{
	char*	temp;
	char	*p;
	SdpList	tokList,nexttok;
	int	i = -1;
	int	j = -1;

	int len;
	if(!pSdpStr||*pSdpStr=='\0')	
		return NULL;

/*
	while( 1 ) {
		i++;
		if( isSDPSpace( (pSdpStr+i) ) )
			continue;
		else
			break;
	}
	p = (char*)(pSdpStr+i);
	if( *p=='\0' )
		return NULL;

	if( i>0 ) {
		tokList = (SdpList)malloc(sizeof(SdpListElm));
		tokList->pToken = strDup(" ");
		tokList->pNext = sdpTokParse(p);

		return tokList;
	}

	while( 1 ) {
		j++;
		if( isCRLF(p+j) ) {
			j += isCRLF(p+j);
			break;
		}	
		if( p[j]=='\0' )
			break;
	}

	temp = (char*)malloc(sizeof(char)*(j+1));
	strncpy(temp,p,j);	temp[j] = 0;

	tokList = (SdpList)malloc(sizeof(SdpListElm));
	tokList->pToken = temp;
	tokList->pNext = sdpLineParse(p+j);
*/
	tokList=(SdpList)calloc(1,sizeof(struct sdpToken));
	if (!tokList) return NULL;
	
	nexttok=tokList;

	/*temp=(char*)strDup(pSdpStr);
	if (!temp) return tokList;*/
	temp=(char*)pSdpStr;
	
	/*freeptr=temp;*/
	while(1){
		/*
		while (1) {
		
			p=strchr(temp,'\r');
			if (p) {
				if (*(p+1)=='\n'){ 
					// *p='\0'; 
					len=p-temp;
					break;
				}else
					p++;
			}else
				break;	
		}*/
	
		/* 
		 * search line break '\n' (0x0a)
		 */
		p=strchr(temp,'\n');
		if (p) {
			/* if len=0 */
			if (p==temp) 
				p=NULL;
			else{
				/* check '\r'(0x0d),if exist,skip it */
				if (*(p-1)=='\r') 
					p--;
				len=p-temp;
			}
		}

		if (!p) {
			nexttok->pToken = strDup(temp);
			break;
		}
		/*nexttok->pToken = strDup(temp);*/
		nexttok->pToken = calloc(1,len+1);
		strncpy(nexttok->pToken,temp,len);
		len=0;
		if(*p=='\r')
			temp=p+2;
		else
			temp=p+1;
		if (*temp!='\0') {
			nexttok->pNext=calloc(1,sizeof(struct sdpToken));
			nexttok=nexttok->pNext;
		}else
			break;
	}

	/*free(freeptr);*/
	return tokList;
}


void sdpListFree(SdpList tokList)
{
	SdpList	pNext = tokList;

	if(!tokList)	return;

	while(pNext) {
		tokList = tokList->pNext;

		if(pNext->pToken)	free(pNext->pToken);
		free(pNext);

		pNext = tokList;
	}

}
