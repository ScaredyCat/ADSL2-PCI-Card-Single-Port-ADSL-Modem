/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sip_url.c
 *
 * $Id: sip_url.c,v 1.38 2006/08/23 09:46:26 tyhuang Exp $
 */

/*#include <windows.h> */
#include <string.h>
#include <adt/dx_vec.h>
#include <adt/dx_str.h>
#include "sip_url.h"
#include <common/cm_utl.h>
#include <common/cm_trace.h>

void InitializeURLParam ( SipURLParam * pParam ) {
	if ( NULL != pParam ) {
		pParam->flag_ = SIPURLPARAM_NONE;
		memset ( pParam->pname_, 0, MAX_PARAM_LENGTH );
		memset ( pParam->pvalue_, 0, MAX_PARAM_LENGTH );
	}
}

void InitializeURLHdr ( SipURLHdr * pHeader ) {
	if ( NULL != pHeader ) {
		memset ( pHeader->hname_, 0, MAX_HEADER_LENGTH );
		memset ( pHeader->hvalue_, 0, MAX_HEADER_LENGTH );
	}
}

#define SIPURL_NONE		0x0
#define SIPURL_USER		0x1
#define SIPURL_PASSWD		0x2
#define SIPURL_PORT		0x4

struct sipURLObj
{
	unsigned short	flag_;
	char*		asStr_;
	char*		scheme;
	char*		userinfo_user_;		/* must set flag */
	char*		userinfo_passwd_;	/* must set flag*/
	char*		host_;			
	unsigned int	port_;			/* must set flag*/
	unsigned short	ipflag_;		/* defalut 0:ipv4; 1:ipv6*/
	DxVector	parameters_;		/* must set flag*/
	DxVector	headers_;		/* must set flag*/
};

void sipURLInit(void)
{}

CCLAPI
SipURL	sipURLNew(void)
{
	SipURL url = calloc(1,sizeof(struct sipURLObj));
	
	url->scheme = NULL;
	url->flag_ = SIPURL_NONE;
	url->headers_ = dxVectorNew(10,10,DX_VECTOR_POINTER);
	url->host_ = NULL;
	url->parameters_ = dxVectorNew(10,10,DX_VECTOR_POINTER);
	url->port_ = 0;
	url->ipflag_=0;
	url->userinfo_passwd_ = NULL;
	url->userinfo_user_ = NULL;
	url->asStr_ = NULL;

	return url;
}

CCLAPI
void	sipURLFree(SipURL url)
{
	if( !url )
		return;
	if( url->flag_&SIPURL_USER ){
		free(url->userinfo_user_);
		url->userinfo_user_=NULL;
	}
	if( url->flag_&SIPURL_PASSWD ){
		free(url->userinfo_passwd_);
		url->userinfo_passwd_=NULL;
	}
	if( url->host_ ){
		free(url->host_);
		url->host_=NULL;
	}
	if( url->asStr_!=NULL ){
		free(url->asStr_);
		url->asStr_=NULL;
	}
	if(url->scheme){
		free(url->scheme);
		url->scheme=NULL;
	}
	dxVectorFree(url->headers_,free);
	url->headers_=NULL;
	dxVectorFree(url->parameters_,free);
	url->parameters_=NULL;
	free(url);
	url=NULL;
}

SipURLParam* sipURLParameterDup(SipURLParam* parm)
{
	SipURLParam* tmp = NULL;

	if( parm ) {
		tmp = (SipURLParam*)calloc(1,sizeof(SipURLParam));	
		*tmp=*parm;
	
	}
	
	return tmp;
}

SipURLHdr* sipURLHeaderDup(SipURLHdr* header)
{
	SipURLHdr* tmp = NULL;

	if( header ) {
		tmp = (SipURLHdr*)calloc(1,sizeof(SipURLHdr));	
		*tmp = *header; 
		
	}
	
	return tmp;
}

CCLAPI
SipURL	sipURLDup(SipURL url)
{
	SipURL tmp = calloc(1,sizeof(struct sipURLObj));

	tmp->scheme=(url->scheme)?strDup(url->scheme):NULL;
	tmp->flag_ = url->flag_;
	tmp->headers_ = dxVectorDup(url->headers_,(void*(*)(void*))sipURLHeaderDup);
	tmp->host_ = (url->host_)?strDup(url->host_):NULL;
	tmp->parameters_ = dxVectorDup(url->parameters_,(void*(*)(void*))sipURLParameterDup);
	tmp->port_ = url->port_;
	tmp->userinfo_passwd_ = (url->userinfo_passwd_)?strDup(url->userinfo_passwd_):NULL;
	tmp->userinfo_user_ = (url->userinfo_user_)?strDup(url->userinfo_user_):NULL;
	tmp->asStr_ = NULL;
	tmp->ipflag_ = url->ipflag_;
	return tmp;
}

int	parseURLParam(SipURL url, char* param)
{
	char	*a,*b,*c;
	SipURLParam	p;

	if( !url || !param )
		return 1;

	a = param;

	while(1) {
		InitializeURLParam ( &p );

		b = strchr(a,';');
		if( b ) {
			*b = 0;
			b++;
		}
		c = strchr(a,'=');
		if( c ) {
			*c = 0;	c++;
			strcpy(p.pvalue_,c);
			p.flag_ |= SIPURLPARAM_VALUE;
		}
		strcpy(p.pname_,a);
		if( sipURLAddParamAt(url,p,sipURLGetParamSize(url))!=0 )
			return 1;
		
		if( !b || *b == 0 )
			return 0;
		a = b;
	} /*end while */
}


int	parseURLHeader(SipURL url, char* header)
{
	char		*a,*b,*c;
	SipURLHdr	h;
	InitializeURLHdr ( &h );

	if( !url || !header )
		return 1;

	a = header;

	while(1) {
		b = strchr(a,'&');
		if( b ) {
			*b = 0;
			b++;
		}
		c = strchr(a,'=');
		if( c ) {
			*c = 0;	c++;
			strcpy(h.hvalue_,c);
		}
		strcpy(h.hname_,a);
		if( sipURLAddHdrAt(url,h,sipURLGetHdrSize(url))!=0 )
			return 1;
		
		if( !b || *b == 0 )
			return 0;
		a = b;
	}/*end while */
}


CCLAPI
SipURL	sipURLNewFromTxt(const char* uristr)
{
	SipURL tmp;
	char*	buf;
	char	*a,*b,*c,*d,*leftc=NULL,*rightc=NULL,*s;
	int	isParm = 0;
	/*int	isHdr=0;*/

	/*if( !uristr || strlen(uristr)<=strlen(SIPURL_TOKEN) )	*/
	if(!uristr)
		return NULL;
	
	/*buf = strDup((const char*)trimWS((char*)uristr));*/
	buf = trimWS(strDup(uristr));

	/*	mark by tyhuang 2003.10.1
	if( strICmpN(SIPURL_TOKEN,buf,strlen(SIPURL_TOKEN))!=0 ) {
		TCRPrint(TRACE_LEVEL_ERROR,"*** [sipURLNewFromTxt] \"sip:\" is not found\n");
	
		free(buf);
		return NULL;
	
	}
	*/
	
	s=strchr(buf,':');
	if(s==NULL){
		TCRPrint(TRACE_LEVEL_API,"*** [sipURLNewFromTxt] \":\" is not found\n");
		free(buf);
		return NULL;
	}else 
		*s++='\0';

#ifdef CCL_TLS
	if(( strICmp(buf,"sip")!=0 )&&(strICmp(buf,"sips")!=0)) {
		TCRPrint(TRACE_LEVEL_API,"*** [sipURLNewFromTxt] \"sip:\" or sips: is not found\n");
		free(buf);
		return NULL;
	}
#else
	if( strICmp(buf,"sip")!=0 ) {
		TCRPrint(TRACE_LEVEL_API,"*** [sipURLNewFromTxt] \"sip:\" is not found\n");
		free(buf);
		return NULL;
	}
#endif
	tmp = sipURLNew();	
	tmp->scheme=strDup(buf);
	/*a = buf + strlen(SIPURL_TOKEN);*/
	a = s;
	b = strchr(a,'@');
	/* mark by tyhuang 2005/01/17,moved down
	leftc = strchr(a,'[');
	if(leftc)
		rightc= strchr(a,']');
	*/
	/* userinfo */
	if( b ) {
		*b = 0;	b++;
		c = strchr(a,':');

		if( c ) {
			*c = 0;	c++;
			if( sipURLSetPasswd(tmp,c)!=0 )
				goto URL_PARSE_ERROR;
		}
		if( sipURLSetUser(tmp,trimWS(a))!=0 )
			goto URL_PARSE_ERROR;
		a = b;
	}	
	b = strchr(a,';');

	/* hostport */
	if( b ) {
		*b = 0; 
		b++;
		isParm = 1;
	}
	else {
		b = strchr(a,'?');
		if( b ) {
			*b = 0;
			b++;
			isParm = 0;
		}
	}
	d = strchr(a,'?');
	if(d){
		*d=0;
		if( parseURLHeader(tmp,d+1)!=0 )
			goto URL_PARSE_ERROR;
	}
	/* if ipv6 url */
	leftc = strchr(a,'[');
	if(leftc)
		rightc= strchr(a,']');
	if(leftc && rightc)
		c = strchr(rightc,':');
	else
		c = strchr(a,':');

	if( c ) {
		*c = 0;	c++;
		if( sipURLSetPort(tmp,a2i(c))!=0 )
			goto URL_PARSE_ERROR;
	}

	/* modified by Mac July 14, 2003 */
	/* if ipv6 url 
	if(leftc && rightc){
		*rightc=0;
		leftc++;
		//set flag to ipv6 ..
		tmp->ipflag_=1;

		if( sipURLSetHost(tmp,leftc)!=0 )
			goto URL_PARSE_ERROR;
	}
	else{ 
		if( sipURLSetHost(tmp,a)!=0 )
			goto URL_PARSE_ERROR;
	}*/
	if( sipURLSetHost(tmp,a)!=0 )
		goto URL_PARSE_ERROR;


	if( !b )
		goto URL_PARSE_SUCCESS;
	a = b;
	
	/*/ Parameter and Header */
	if( isParm ) {
		b = strchr(a,'?');
		if( b ) {
			*b = 0; 
			b++;
		}
		if( parseURLParam(tmp,a)!=0 )
			goto URL_PARSE_ERROR;
		if( !b )
			goto URL_PARSE_SUCCESS;
		a = b;
	}
	if( parseURLHeader(tmp,a)!=0 )
		goto URL_PARSE_ERROR;

URL_PARSE_SUCCESS:
	free(buf);

	return tmp;

URL_PARSE_ERROR:
	free(buf);
	sipURLFree(tmp);

	return NULL;
}


CCLAPI
char*	sipURLToStr(SipURL url)
{
	DxStr		urlStr;
	int		i = 0;
	SipURLParam	*p;
	SipURLHdr	*h;
	char 		tmp[12];

	if( !url )
		return NULL;
	if( url->asStr_ ) 
		free(url->asStr_);

	urlStr = dxStrNew();
	dxStrClr(urlStr);

	/* url scheme */	
	/*dxStrCat(urlStr,(char*)SIPURL_TOKEN);*/
	dxStrCat(urlStr,(char*)url->scheme);
	dxStrCat(urlStr,(char*)":");
	/* user and passwd */
	if( url->flag_&SIPURL_USER ) {
		dxStrCat(urlStr,(char*)url->userinfo_user_);
	}
	if( url->flag_&SIPURL_PASSWD ) {
		dxStrCat(urlStr,(char*)":");
		dxStrCat(urlStr,(char*)url->userinfo_passwd_);
	}
	if( url->flag_&SIPURL_USER || url->flag_&SIPURL_PASSWD )
		dxStrCat(urlStr,(char*)"@");

	/* host and port */
	if(!url->host_)
		goto URL_2STR_ERROR;
	if( url->ipflag_==1){
		/* if ipv6 address ,should enclose url with [ ]*/
		dxStrCat(urlStr,"[");
		dxStrCat(urlStr,(char*)url->host_);
		dxStrCat(urlStr,"]");
	}else
		dxStrCat(urlStr,(char*)url->host_);

	if( url->flag_&SIPURL_PORT ) {
		dxStrCat(urlStr,(char*)":");
		i2a(url->port_,tmp,12);
		dxStrCat(urlStr,tmp);
	}

	/* parameter */
	i = 0;
	while( i < sipURLGetParamSize(url) ) {
		if( (p = sipURLGetParamAt(url,i))==0 )
			goto URL_2STR_ERROR;
		dxStrCat(urlStr,(char*)";");
		dxStrCat(urlStr,(char*)p->pname_);
		/*if((p->flag_&SIPURLPARAM_VALUE)&&(p->pvalue_ != NULL)) {*/
		if((p->flag_&SIPURLPARAM_VALUE)&&(strlen(p->pvalue_) > 0)){
			dxStrCat(urlStr,(char*)"=");
			dxStrCat(urlStr,(char*)p->pvalue_);
		}
		i++;
	}	/* header */
	i = 0;
	while( i < sipURLGetHdrSize(url) ) {
		if( (h = sipURLGetHdrAt(url,i))==0 )
			goto URL_2STR_ERROR;
		if( i == 0 )
			dxStrCat(urlStr,(char*)"?");
		else
			dxStrCat(urlStr,(char*)"&");
		dxStrCat(urlStr,(char*)h->hname_);
		dxStrCat(urlStr,(char*)"=");
		dxStrCat(urlStr,(char*)h->hvalue_);
		i++;
	}

	url->asStr_ = strDup(dxStrAsCStr(urlStr));
	dxStrFree(urlStr);
	return url->asStr_;

URL_2STR_ERROR:

	return NULL;
}

CCLAPI 
RCODE	sipURLSetScheme(IN SipURL url, IN const char* scheme)
{
	if(!url || !scheme)
		return RC_E_PARAM;
	if(url->scheme){
		free(url->scheme);
		url->scheme=NULL;
	}
	url->scheme=strDup(scheme);

	return RC_OK;

}
CCLAPI 
char*	sipURLGetScheme(IN SipURL url)
{
	if(!url)
		return NULL;
	return url->scheme;
}

CCLAPI
RCODE	sipURLSetUser(SipURL url, const char* user)
{
	if( !url || !user )
		return RC_E_PARAM;

	if( url->userinfo_user_ )
		free(url->userinfo_user_);
	url->userinfo_user_ = strDup(user);
	url->flag_ |= SIPURL_USER;

	return RC_OK;
}

CCLAPI
char*	sipURLGetUser(SipURL url)
{
	if( !url )
		return NULL;

	if( url->flag_&SIPURL_USER ) {
		return url->userinfo_user_;
	}

	return NULL;
}

CCLAPI
RCODE	sipURLDelUser(SipURL url)
{
	if( !url )
		return RC_E_PARAM;

	if( url->flag_&SIPURL_USER ) {
		free(url->userinfo_user_);
		url->userinfo_user_ = NULL;
		url->flag_ &= ~(SIPURL_USER);
	}

	return RC_OK;
}

CCLAPI
RCODE	sipURLSetPasswd(SipURL url,const char* passwd)
{
	if( !url || !passwd )
		return RC_E_PARAM;

	if( url->userinfo_passwd_ )
		free(url->userinfo_passwd_);
	url->userinfo_passwd_ = strDup(passwd);
	url->flag_ |= SIPURL_PASSWD;

	return RC_OK;
}

CCLAPI
char*	sipURLGetPasswd(SipURL url)
{
	if( !url )
		return NULL;

	if( url->flag_&SIPURL_PASSWD ) {
		return url->userinfo_passwd_;
	}

	return NULL;
}

CCLAPI
int	sipURLDelPasswd(SipURL url)
{
	if( !url )
		return RC_E_PARAM;

	if( url->flag_&SIPURL_PASSWD ) {
		free(url->userinfo_passwd_);
		url->userinfo_passwd_ = NULL;
		url->flag_ &= ~(SIPURL_PASSWD);
	}

	return RC_OK;
}

CCLAPI
RCODE	sipURLSetHost(SipURL url,const char* host)
{
	if( !url || !host )
		return RC_E_PARAM;

	if( url->host_ )
		free(url->host_);
	url->host_ = strDup(host);

	return RC_OK;
}

CCLAPI
char*	sipURLGetHost(SipURL url)
{
	if( !url )
		return NULL;

	if(url->host_) {
		return url->host_;
	}

	return NULL;
}

CCLAPI
RCODE	sipURLDelHost(SipURL url)
{
	if( !url )
		return RC_E_PARAM;

	if( url->host_ ) {
		free(url->host_);
		url->host_ = NULL;
	}

	return RC_OK;
}

CCLAPI
RCODE	sipURLSetPort(SipURL url, unsigned int port)
{
	if( !url )
		return RC_E_PARAM;

	url->port_ = port;
	url->flag_ |= SIPURL_PORT;

	return RC_OK;
}

CCLAPI
unsigned int	sipURLGetPort(SipURL url)
{
	if( !url )
		return 0;

	if( url->flag_&SIPURL_PORT ) {
		return url->port_;
	}

	return 0;
}

CCLAPI
RCODE	sipURLDelPort(SipURL url)
{
	url->flag_ &= ~(SIPURL_PORT);
	return RC_OK;
}


CCLAPI
RCODE	sipURLAddParamAt(SipURL url, SipURLParam parm, int position)
{
	SipURLParam	*p;
	if( !url )
		return RC_E_PARAM;

	p = (SipURLParam*)calloc(1,sizeof(SipURLParam));
	*p = parm;
	if( dxVectorAddElementAt(url->parameters_,p,position)>=0 ) {
		return RC_OK;
	}

	free(p);
	return RC_ERROR;
}

CCLAPI
SipURLParam*	sipURLGetParamAt(SipURL url, int position)
{
	if( !url )
		return NULL;
	
	return dxVectorGetElementAt(url->parameters_,position);
}

CCLAPI
RCODE	sipURLDelParamAt(SipURL url, int position)
{
	SipURLParam	*p;
	if( !url )
		return RC_E_PARAM;

	p = dxVectorRemoveElementAt(url->parameters_,position);
	if( !p )
		return RC_E_PARAM;
	
	free(p);
	return RC_OK;
}

CCLAPI
int	sipURLGetParamSize(SipURL url)
{
	return (url)?dxVectorGetSize(url->parameters_):0;
}

CCLAPI
RCODE	sipURLAddHdrAt(SipURL url, SipURLHdr head, int position)
{
	SipURLHdr	*h;
	if( !url )
		return RC_E_PARAM;

	h = (SipURLHdr*)malloc(sizeof(SipURLHdr));
	*h = head;
	if( dxVectorAddElementAt(url->headers_,h,position)>=0 ) 
		return RC_OK;

	free(h);
	return RC_E_PARAM;
}

CCLAPI
SipURLHdr*	sipURLGetHdrAt(SipURL url, int position)
{
	if( !url )
		return NULL;
	
	return dxVectorGetElementAt(url->headers_,position);
}

CCLAPI
RCODE	sipURLDelHdrAt(SipURL url, int position)
{
	SipURLHdr	*h;
	if( !url )
		return RC_E_PARAM;

	h = dxVectorRemoveElementAt(url->headers_,position);
	if( !h )
		return RC_E_PARAM;
	
	free(h);
	return RC_OK;
}

CCLAPI
int	sipURLGetHdrSize(SipURL url)
{
	return (url)?dxVectorGetSize(url->headers_):0;
}

/* added by Mac July,14,2003*/
CCLAPI 
int	sipURLGetSquareQuote(SipURL url)
{
	if( !url )
		return RC_E_PARAM;

	if ( url->ipflag_ ) {
		return 1;
	}
	return 0;
}
