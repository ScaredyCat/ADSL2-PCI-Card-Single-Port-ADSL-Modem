/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sip_rsp.c
 *
 * $Id: sip_rsp.c,v 1.111 2006/11/02 06:33:13 tyhuang Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sip_rsp.h"
#include "sip_hdr.h"
#include "sip_int.h"
#include <common/cm_utl.h>
#include <common/cm_trace.h>
#include <adt/dx_str.h>

struct sipRspObj {
	/* Response Status line */
	SipRspStatusLine	*statusline;

	SipAccept		*accept;  
	SipAcceptEncoding	*acceptencoding;
	SipAcceptLang		*acceptlanguage;
	SipAllow		*allow;
	SipAllowEvt		*allowevents;
	SipInfo			*alertinfo; /* added by tyhuang */
	SipAuthInfo		*authenticationinfo;
	unsigned char		*callid;
	SipInfo			*callinfo;
	SipContact		*contact;
	SipContentDisposition	*contentdisposition;
	SipContEncoding		*contentencoding;
	SipContentLanguage	*contentlanguage;
	int				contentlength;
	SipContType		*contenttype;
	SipCSeq			*cseq;
	SipDate			*date;
	SipEncrypt		*encryption;
	SipEvent		*event;
	SipInfo			*errorinfo;   /* added by tyhuang */
	SipExpires		*expires;
	SipFrom			*from;
	unsigned char	*mimeversion; /* added by tyhuang */
	int				minexpires;   /* added by tyhuang */
	SipMinSE		*minse;
	char			*organization;
	SipIdentity		*passertedidentity; /* RFC 3325 */
	SipIdentity		*ppreferredidentity;/* RFC 3325 */
	SipPrivacy		*privacy;			/* RFC 3323 */
	SipPxyAuthn		*proxyauthenticate;
	SipRecordRoute	*recordroute;
	SipReferTo		*referto;
	SipReferredBy	*referredby;
	SipReplaces		*replaces;
	SipReplyTo		*replyto;
	SipRequire		*require;
	SipRemotePartyID	*remotepartyid;
	SipRtyAft		*retryafter;
	int				rseq;
	SipSessionExpires	*sessionexpires;
	SipServer		*server;
	unsigned char	*sipetag;
	SipSupported	*supported;
	SipTimestamp		*timestamp;
	SipTo			*to;
	SipUnsupported		*unsupported;
	SipUserAgent		*useragent;
	SipVia			*via;
	SipWarning		*warning;
	SipWWWAuthn		*wwwauthenticate;
	SipBody			*body;
	BOOL			rspflag;
	char			*rspbuf;
	char			*unHdr;

	MsgHdr			hdrlist;

#ifdef ICL_IMS
	unsigned char	*pmediaauthorization;
	SipPath			*path;
	unsigned char	*paccessnetworkinfo;
	unsigned char	*passociateduri;
	unsigned char	*pchargingfunctionaddress;
	unsigned char	*pchargingvector;
	unsigned char	*securityserver;
	SipServiceRoute	*serviceroute;
#endif /* end of ICL_IMS */
	
};

typedef enum {
	SIP_WWW_AUTHN = 0x1,
	SIP_PXY_AUTHN = 0x2
} SipAuthnType;

/*int CountResponseHeaderLength(sipResponse, int *);*/
int sipRspAsStr(SipRsp, DxStr, int *);
int CountRspStatusLineLen(SipRsp);

int	sipRspGetAuthnSize(SipRsp response,SipAuthnType type);
RCODE	sipRspAddBasicAuthnAt(SipRsp response,SipAuthnType type,int position,char* realm);
RCODE	sipRspGetBasicAuthnAt(SipRsp response,SipAuthnType type,int position,char* realm, int rlen);
RCODE	sipRspAddDigestAuthnAt(SipRsp response,SipAuthnType type,int position,SipDigestWWWAuthn auth);
RCODE	sipRspGetDigestAuthnAt(SipRsp response,SipAuthnType type,int position,SipDigestWWWAuthn* auth);
RCODE	sipRspAddPGPWWWAuthnAt(void);
RCODE	sipRspGetPGPWWWAuthnAt(void);

CCLAPI 
SipRsp sipRspNew(void)
{
	SipRsp _this;
	
	/*_this =(SipRsp)malloc(sizeof *_this);*/
	_this =(SipRsp)calloc(1,sizeof(*_this));
	if(!_this){
		TCRPrint(1,"<sipRspDup> memory alloc failed !\n");
		return _this;
	}
	/* initialize the headers of request */
	_this->minexpires = -1;
	_this->rseq = -1;
	_this->rspflag=FALSE;

	return _this;
}

CCLAPI
SipRsp sipRspDup(SipRsp _source)
{
	SipRsp _this=NULL;

	

	if(_source){
		_this=sipRspNew();
		if(!_this){
			TCRPrint(1,"<sipRspDup> memory alloc failed !\n");
			return _this;
		}

		if(_source->statusline)
			sipRspSetStatusLine(_this,_source->statusline);

		
		if(_source->accept)
			AcceptRspDup(_this,_source->accept);  
			
		if(_source->acceptencoding)
			AcceptEncodingRspDup(_this,_source->acceptencoding);
			
		if(_source->acceptlanguage)
			AcceptLanguageRspDup(_this,_source->acceptlanguage);
			
		if(_source->alertinfo)
			AlertInfoRspDup(_this,_source->alertinfo);
			
		if(_source->allow)
			AllowRspDup(_this,_source->allow);
			
		if(_source->allowevents)
			AllowEventsRspDup(_this,_source->allowevents);
			
		if(_source->authenticationinfo)
			AuthenticationInfoRspDup(_this,_source->authenticationinfo);

		if(_source->callid)
			CallIDRspDup(_this,_source->callid);

		if(_source->callinfo)
			CallInfoRspDup(_this,_source->callinfo);

		if(_source->contact)
			ContactRspDupLink(_this,_source->contact);
			
		if(_source->contentdisposition)
			ContentDispositionRspDup(_this,_source->contentdisposition);

		if(_source->contentlanguage)
			ContentLanguageRspDup(_this,_source->contentlanguage);
			
		if(_source->contentencoding)
			ContentEncodingRspDup(_this,_source->contentencoding);
			
		_this->contentlength=_source->contentlength;

		if(_source->contenttype)
			ContentTypeRspDup(_this,_source->contenttype);
			
		if(_source->cseq)
			CSeqRspDup(_this,_source->cseq);
			
		if(_source->date)
			DateRspDup(_this,_source->date);
			
		if(_source->encryption)
			EncryptionRspDup(_this,_source->encryption);
			
		if(_source->event)
			EventRspDup(_this,_source->event);
			
		if(_source->errorinfo)
			ErrorInfoRspDup(_this,_source->errorinfo);

		if(_source->expires)
			ExpiresRspDup(_this,_source->expires);
			
		if(_source->from)
			FromRspDup(_this,_source->from);
			
		if(_source->mimeversion)
			MIMEVersionRspDup(_this,_source->mimeversion);
			
		_this->minexpires=_source->minexpires;

		if(_source->minse)
			MinSERspDup(_this,_source->minse);
			
		if(_source->organization)
			OrganizationRspDup(_this,(const char*)_source->organization);

		if(_source->passertedidentity) 
			PAssertedIdentityRspDupLink(_this,_source->passertedidentity);

		if(_source->ppreferredidentity)
			PPreferredIdentityRspDupLink(_this,_source->ppreferredidentity);

		if (_source->privacy) {
			PrivacyRspDup(_this,_source->privacy);
		}
			
		if(_source->proxyauthenticate)
			ProxyAuthenticateRspDupLink(_this,_source->proxyauthenticate);
			
		if(_source->recordroute)
			RecordRouteRspDupLink(_this,_source->recordroute);
			
		if(_source->referto)
			ReferToRspDup(_this,_source->referto);
			
		if(_source->referredby)
			ReferredByRspDup(_this,_source->referredby);
			
		if(_source->remotepartyid)
			RemotePartyIDRspDupLink(_this,_source->remotepartyid);
			
		if(_source->replaces)
			ReplacesRspDupLink(_this,_source->replaces);
			
		if(_source->replyto)
			ReplyToRspDup(_this,_source->replyto);
			
		if(_source->require)
			RequireRspDup(_this,_source->require);
		
		if(_source->retryafter)
			RetryAfterRspDup(_this,_source->retryafter);
			
		_this->rseq=_source->rseq;
		
		if(_source->sessionexpires)
			SessionExpiresRspDup(_this,_source->sessionexpires);
			
		if(_source->server)
			ServerRspDup(_this,_source->server);
			
		if(_source->sipetag)
			_this->sipetag=(unsigned char*)strDup((const char*)_source->sipetag);
		
		if(_source->supported)
			SupportedRspDup(_this,_source->supported);
			
		if(_source->timestamp)
			TimestampRspDup(_this,_source->timestamp);
			
		if(_source->to)
			ToRspDup(_this,_source->to);
			
		if(_source->unsupported)
			UnsupportedRspDup(_this,_source->unsupported);
			
		if(_source->useragent)
			UserAgentRspDup(_this,_source->useragent);
			
		if(_source->via)
			ViaRspDupLink(_this,_source->via);
			
		if(_source->warning)
			WarningRspDupLink(_this,_source->warning);
			
		if(_source->wwwauthenticate)
			WWWAuthenticateRspDupLink(_this,_source->wwwauthenticate);
			
		if(_source->body)
			sipRspSetBody(_this,_source->body);
			
		_this->rspflag=TRUE;
	
		if(_source->unHdr)
			_this->unHdr=strDup(_source->unHdr);

#ifdef ICL_IMS 
		if(_source->pmediaauthorization)
			PMARspDup(_this,_source->pmediaauthorization);
		
		if(_source->path)
			PathRspDupLink(_this,_source->path);

		if(_source->paccessnetworkinfo)
			PANIRspDup(_this,_source->paccessnetworkinfo);

		if(_source->passociateduri)
			PAURspDup(_this,_source->passociateduri);

		if(_source->pchargingfunctionaddress)
			PCFARspDup(_this,_source->pchargingfunctionaddress);

		if(_source->pchargingvector)
			PCVRspDup(_this,_source->pchargingvector);

		if(_source->securityserver)
			SecurityServerRspDup(_this,_source->securityserver);

		if(_source->serviceroute)
			ServiceRouteRspDupLink(_this,_source->serviceroute);
		
#endif /* end of ICL_IMS */
		
	}else
		TCRPrint(1,"<sipRspDup>Response message is NULL!\n");
	return _this;
}

CCLAPI 
void sipRspFree(SipRsp response)
{
	if( !response ){
		TCRPrint(1,"<sipRspFree>Response message Free error!\n");
		return ;
	}

	sipRspStatusLineFree(response->statusline);
	AcceptFree(response->accept);
	AcceptEncodingFree(response->acceptencoding);
	AcceptLanguageFree(response->acceptlanguage);
	AlertInfoFree(response->alertinfo);
	AllowFree(response->allow);
	AllowEventsFree(response->allowevents);
  	AuthenticationInfoFree(response->authenticationinfo);
	CallIDFree(response->callid);
	CallInfoFree(response->callinfo);
	sipContactFree(response->contact);
	ContentDispositionFree(response->contentdisposition);
	ContentEncodingFree(response->contentencoding);
	ContentLanguageFree(response->contentlanguage);
	ContentTypeFree(response->contenttype);
 	CSeqFree(response->cseq);
  	DateFree(response->date);
	EncryptionFree(response->encryption);
	ErrorInfoFree(response->errorinfo);
	EventFree(response->event);
	ExpiresFree(response->expires);
	FromFree(response->from);
	MIMEVersionFree(response->mimeversion);
	MinSEFree(response->minse);
	OrganizationFree((unsigned char*)response->organization);
	PAssertedIdentityFree(response->passertedidentity);
	PPreferredIdentityFree(response->ppreferredidentity);
	PrivacyFree(response->privacy);
    ProxyAuthenticateFree(response->proxyauthenticate);
  	RecordRouteFree(response->recordroute);
	ReferToFree(response->referto);
	ReferredByFree(response->referredby);
	RemotePartyIDFree(response->remotepartyid);
	ReplacesFree(response->replaces);
	ReplyToFree(response->replyto);
	RequireFree(response->require);
	RetryAfterFree(response->retryafter);
	SessionExpiresFree(response->sessionexpires);
	ServerFree(response->server);
	SIPETagFree(response->sipetag);
	SupportedFree(response->supported);
  	TimestampFree(response->timestamp);
	ToFree(response->to);
	UnsupportedFree(response->unsupported);
	UserAgentFree(response->useragent);
	ViaFree(response->via);

	WarningFree(response->warning);
	if (response->wwwauthenticate != NULL)
		WWWAuthenticateFree(response->wwwauthenticate);
	
	sipBodyFree(response->body);

	if(response->rspbuf != NULL)
		free(response->rspbuf);
	
	if(response->unHdr != NULL)
		free(response->unHdr);

	if (response->hdrlist) 
		msgHdrFree(response->hdrlist);

#ifdef ICL_IMS 
	
	PathFree(response->path);
	
	if(response->pmediaauthorization) free(response->pmediaauthorization);
	if(response->paccessnetworkinfo) free(response->paccessnetworkinfo);
	if(response->passociateduri) free(response->passociateduri);
	if(response->pchargingfunctionaddress) free(response->pchargingfunctionaddress);
	if(response->pchargingvector) free(response->pchargingvector);
	if(response->securityserver) free(response->securityserver);
	
	ServiceRouteFree(response->serviceroute);
	
#endif /* end of ICL_IMS */

	memset(response,0,sizeof(*response));

	free(response);
}


CCLAPI 
RCODE sipRspSetStatusLine(SipRsp response,SipRspStatusLine *statusline)
{
	if(response == NULL)
		return RC_ERROR;
	response->rspflag=TRUE;
	if(response->statusline == NULL)
		response->statusline=(SipRspStatusLine*)calloc(1,sizeof(SipRspStatusLine));
	response->statusline->ptrVersion = strDup(statusline->ptrVersion);
	response->statusline->ptrStatusCode= strDup(statusline->ptrStatusCode);
	response->statusline->ptrReason	 = strDup(statusline->ptrReason);
	return RC_OK;
}

CCLAPI 
SipRspStatusLine* sipRspGetStatusLine(SipRsp response)
{
	if(response == NULL)
		return NULL;
	else
		return response->statusline;
}

int CountRspStatusLineLen(SipRsp response)
{
	int length=0;
	if(response != NULL)
		length = strlen(response->statusline->ptrVersion)+ \
				 strlen(response->statusline->ptrStatusCode)+\
				 strlen(response->statusline->ptrReason)+6;
	return length;
}


CCLAPI 
void* sipRspGetHdr(SipRsp response, SipHdrType headertype)
{
	if(response == NULL)
		return NULL;

	switch(headertype){
	case Accept:
		return (void *)response->accept;
	case Accept_Encoding:
		return (void *)response->acceptencoding;
	case Accept_Language:
		return (void *)response->acceptlanguage;
	case Alert_Info:
		return (void *)response->alertinfo;
	case Allow:
		return (void *)response->allow;
	case Allow_Events_Short:
	case Allow_Events:
		return (void *)response->allowevents;
	case Authentication_Info:
		return (void *)response->authenticationinfo;
	case Call_ID_Short:    
	case Call_ID:
		return (void *)response->callid;
	case Call_Info:
		return (void *)response->callinfo;
	case Contact_Short:
	case Contact:
		return (void *)response->contact;
	case Content_Disposition:
		return (void *)response->contentdisposition;
	case Content_Encoding_Short:
	case Content_Encoding:
		return (void *)response->contentencoding;
	case Content_Language:
		return (void *)response->contentlanguage;
	case Content_Length_Short:
	case Content_Length:
		return (void *)(&response->contentlength);
	case Content_Type_Short:
	case Content_Type:
		return (void *)response->contenttype;
	case CSeq:
		return (void *)response->cseq;
	case Date:
		return (void *)response->date;
	case Encryption:
		return (void *)response->encryption;
	case Error_Info:
		return (void *)response->errorinfo;
	case Event:
	case Event_Short:
		return (void*)response->event;
	case Expires:
		return (void *)response->expires;
	case From_Short:
	case From:
		return (void *)response->from;
	case MIME_Version:
		return (void *)response->mimeversion;
	case Min_Expires:
		return (void *)(&response->minexpires);
	case Min_SE:
		return (void*)response->minse;
	case Organization:
		return (void *)response->organization;
	case P_Asserted_Identity:
		return (void *)response->passertedidentity;
	case P_Preferred_Identity:
		return (void *)response->ppreferredidentity;
	case Privacy:
		return (void *)response->privacy;
	case Proxy_Authenticate:
		return (void *)response->proxyauthenticate;
	case Record_Route:
		return (void *)response->recordroute;
	case Refer_To:
	case Refer_To_Short:
		return (void*)response->referto;
	case Referred_By:
	case Referred_By_Short:
		return (void*)response->referredby;
	case Remote_Party_ID:
		return (void *)response->remotepartyid;
	case Replaces:
		return (void*)response->replaces;
	case Reply_To:
		return (void*)response->replyto;
	case Require:
		return (void*)response->require;
	case Retry_After:
		return (void *)response->retryafter;
	case RSeq:
		return (void*)(&response->rseq);
	case Session_Expires:
	case Session_Expires_Short:
		return (void*)response->sessionexpires;
	case Server:
		return (void *)response->server;
	case SIP_ETag:
		return (void *)response->sipetag;
	case Supported:
	case Supported_Short:
		return (void *)response->supported;
	case Timestamp:
		return (void *)response->timestamp;
	case To_Short:
	case To:
		return (void *)response->to;
	case Unsupported:
		return (void *)response->unsupported;
	case User_Agent:
		return (void *)response->useragent;
	case Via_Short:
	case Via:
		return (void *)response->via;
	case Warning:
		return (void *)response->warning;
	case WWW_Authenticate:
		return (void *)response->wwwauthenticate;
	case UnknSipHdr:
		return (void *)response->unHdr;
#ifdef ICL_IMS
	case P_Media_Authorization:
		return (void *)response->pmediaauthorization;
	case Path:	
		return (void *)response->path;
	case P_Access_Network_Info:	
		return (void *)response->paccessnetworkinfo;
	case P_Associated_URI:
		return (void *)response->passociateduri;
	case P_Charging_Function_Address:
		return (void *)response->pchargingfunctionaddress;
	case P_Charging_Vector:
		return (void *)response->pchargingvector;
	case Security_Server:
		return (void *)response->securityserver;	
	case Service_Route:
		return (void *)response->serviceroute;		
#endif /* end of ICL_IMS */
	default:
		return (void *)NULL;
  }
}

/* make a copy */
CCLAPI 
RCODE sipRspAddHdr(SipRsp response, SipHdrType headertype, void *data)
{
	if((response == NULL) || (NULL==data))
		return RC_ERROR;

	response->rspflag=TRUE;
	if(data != NULL){
		switch(headertype){
		case Accept:
			AcceptRspDup(response,(SipAccept*)data);
			break;
		case Accept_Encoding:
			AcceptEncodingRspDup(response,(SipAcceptEncoding*)data);
			break;
		case Accept_Language:
			AcceptLanguageRspDup(response,(SipAcceptLang*)data);
			break;
		case Alert_Info:
			AlertInfoRspDup(response,(SipInfo*)data);
			break;
		case Allow:
			AllowRspDup(response,(SipAllow*)data);
			break;
		case Allow_Events_Short:
		case Allow_Events:
			AllowEventsRspDup(response,(SipAllowEvt*)data);
			break;
		case Authentication_Info:
			AuthenticationInfoRspDup(response,(SipAuthInfo*)data);
			break;
		case Call_ID_Short:    
		case Call_ID:
			CallIDRspDup(response,(unsigned char*)data);
			break;
		case Call_Info:
			CallInfoRspDup(response,(SipInfo*)data);
			break;
		case Contact_Short:
		case Contact:
			ContactRspDupLink(response,(SipContact*)data);
			break;
		case Content_Disposition:
			ContentDispositionRspDup(response,(SipContentDisposition*)data);
			break;
		case Content_Encoding_Short:
		case Content_Encoding:
			ContentEncodingRspDup(response,(SipContEncoding*)data);
			break;
		case Content_Language:
			ContentLanguageRspDup(response,(SipContentLanguage*)data);
			break;
		case Content_Length_Short:
		case Content_Length:
			response->contentlength = *(int *)data;
			break;
		case Content_Type_Short:
		case Content_Type:
			ContentTypeRspDup(response,(SipContType*)data);
			break;
		case CSeq:
			CSeqRspDup(response,(SipCSeq*)data);
			break;
		case Date:
			DateRspDup(response,(SipDate*)data);
			break;
		case Error_Info:
			ErrorInfoRspDup(response,(SipInfo*)data);
		case Event:
		case Event_Short:
			EventRspDup(response,(SipEvent*)data);
			break;
		case Encryption:
			EncryptionRspDup(response,(SipEncrypt*)data);
			break;
		case Expires:
			ExpiresRspDup(response,(SipExpires*)data);
			break;
		case From_Short:
		case From:
			FromRspDup(response,(SipFrom*)data);
			break;
		case MIME_Version:
			MIMEVersionRspDup(response,(unsigned char*)data);
			break;
		case Min_Expires:
			response->minexpires = *(int *)data;
			break;
		case Min_SE:
			MinSERspDup(response,(SipMinSE*)data);
			break;
		case Organization:
			OrganizationRspDup(response,(const char*)data);
			break;
		case P_Asserted_Identity:
			PAssertedIdentityRspDupLink(response,(SipIdentity*)data);
			break;
		case P_Preferred_Identity:
			PPreferredIdentityRspDupLink(response,(SipIdentity*)data);
			break;
		case Privacy:
			PrivacyRspDup(response,(SipPrivacy*)data);
			break;
		case Proxy_Authenticate:
			ProxyAuthenticateRspDupLink(response,(SipPxyAuthn*)data);
			break;
		case Record_Route:
			RecordRouteRspDupLink(response,(SipRecordRoute*)data);
			break;
		case Refer_To:
		case Refer_To_Short:
			ReferToRspDup(response,(SipReferTo*)data);
			break;
		case Referred_By:
		case Referred_By_Short:
			ReferredByRspDup(response,(SipReferredBy*)data);
			break;
		case Remote_Party_ID:
			RemotePartyIDRspDupLink(response,(SipRemotePartyID*)data);
			break;
		case Replaces:
			ReplacesRspDupLink(response,(SipReplaces*)data);
			break;
		case Reply_To:
			ReplyToRspDup(response,(SipReplyTo*)data);
			break;
		case Require:
			RequireRspDup(response,(SipRequire*)data);
			break;
		case Retry_After:
			RetryAfterRspDup(response,(SipRtyAft*)data);
			break;
		case RSeq:
			response->rseq=*(int *)data;
			break;
		case Session_Expires:
		case Session_Expires_Short:
			SessionExpiresRspDup(response,(SipSessionExpires*)data);
			break;
		case Server:
			ServerRspDup(response,(SipServer*)data);
			break;
		case SIP_ETag:
			if(response->sipetag) free(response->sipetag);
			response->sipetag=(unsigned char*)strDup((const char*)data);
			break;
		case Supported:
		case Supported_Short:
			SupportedRspDup(response,(SipSupported*)data);
			break;
		case Timestamp:
			TimestampRspDup(response,(SipTimestamp*)data);
			break;
		case To_Short:
		case To:
			ToRspDup(response,(SipTo*)data);
			break;
		case Unsupported:
			UnsupportedRspDup(response,(SipUnsupported*)data);
			break;
		case User_Agent:
			UserAgentRspDup(response,(SipUserAgent*)data);
			break;
		case Via_Short:
		case Via:
			ViaRspDupLink(response,(SipVia*)data);
			break;
		case Warning:
			WarningRspDupLink(response,(SipWarning*)data);
			break;
		case WWW_Authenticate:
			WWWAuthenticateRspDupLink(response,(SipWWWAuthn*)data);
			break;
#ifdef ICL_IMS
		case P_Media_Authorization:
			PMARspDup(response,(unsigned char*)data);
			break;
		case Path:
			PathRspDupLink(response,(SipPath*)data);
			break;
		case P_Access_Network_Info:	
			PANIRspDup(response,(unsigned char*)data);
			break;
		case P_Associated_URI:
			PAURspDup(response,(unsigned char*)data);
			break;
		case P_Charging_Function_Address:	
			PCFARspDup(response,(unsigned char*)data);
			break;
		case P_Charging_Vector:
			PCVRspDup(response,(unsigned char*)data);
			break;
		case Security_Server:
			SecurityServerRspDup(response,(unsigned char*)data);
			break;
		case Service_Route:
			ServiceRouteRspDupLink(response,(SipServiceRoute*)data);
			break;
#endif /* end of ICL_IMS */
		default:
			TCRPrint(1,"<sipRspAddHdr> Header Not supported! \n");
			return RC_ERROR;
		}/*end switch case */
	}
	return RC_OK;
}

/* add header only */
RCODE sipRspAddHdrEx(SipRsp response, SipHdrType headertype, void *ret)
{
	if(response == NULL)
		return RC_ERROR;
	
	response->rspflag=TRUE;
	if(ret != NULL)
		switch(headertype){
	     case Accept:
			if(response->accept != NULL)
				AcceptFree(response->accept);
			response->accept=(SipAccept*)ret;
			break;
		case Accept_Encoding:
			if(response->acceptencoding != NULL)
				AcceptEncodingFree(response->acceptencoding);
			response->acceptencoding=(SipAcceptEncoding*)ret;
			break;
		case Accept_Language:
			if(response->acceptlanguage != NULL)
				AcceptLanguageFree(response->acceptlanguage);
			response->acceptlanguage=(SipAcceptLang*)ret;
		    break;
		case Alert_Info:
			if(response->alertinfo != NULL)
				AlertInfoFree(response->alertinfo);
			response->alertinfo=(SipInfo*)ret;
		case Allow:
			if(response->allow != NULL)
				AllowFree(response->allow);
			response->allow=(SipAllow*)ret;
			break;
		case Allow_Events_Short:
		case Allow_Events:
			if(response->allowevents != NULL)
				AllowEventsFree(response->allowevents);
			response->allowevents=(SipAllowEvt*)ret;
			break;
		case Authentication_Info:
			if(response->authenticationinfo != NULL)
				AuthenticationInfoFree(response->authenticationinfo);
			response->authenticationinfo=(SipAuthInfo*)ret;
			break;
		case Call_ID_Short:
		case Call_ID:
			if(response->callid != NULL)
				CallIDFree(response->callid);
			response->callid=(unsigned char*)ret;
			break;
		case Call_Info:
			if(response->callinfo != NULL)
				CallInfoFree(response->callinfo);
			response->callinfo=(SipInfo*)ret;
			break;
		case Contact_Short:
		case Contact:			
			if(response->contact == NULL)
				response->contact=(SipContact*)ret;
			else{
				ContactRspDupLink(response,(SipContact*)ret);
				sipContactFree((SipContact*)ret);
			}
			break;
		case Content_Disposition:
			if(response->contentdisposition != NULL)
				ContentDispositionFree(response->contentdisposition);
			response->contentdisposition=(SipContentDisposition*)ret;
			break;
		case Content_Encoding_Short:
		case Content_Encoding:
			if(response->contentencoding != NULL)
				ContentEncodingFree(response->contentencoding);
			response->contentencoding=(SipContEncoding*)ret;
			break;
		case Content_Language:
			if(response->contentlanguage != NULL)
				ContentLanguageFree(response->contentlanguage);
			response->contentlanguage=(SipContentLanguage*)ret;
			break;
		case Content_Length_Short:
		case Content_Length:
			if(!ret) break;
			response->contentlength = *(int *)ret;
			break;
		case Content_Type_Short:
		case Content_Type:
			if(response->contenttype != NULL)
				ContentTypeFree(response->contenttype);
			response->contenttype=(SipContType*)ret;
			break;
		case CSeq:
			if(response->cseq != NULL)
				CSeqFree(response->cseq);
			response->cseq=(SipCSeq*)ret;
			break;
		case Date:
			if(response->date != NULL)
				DateFree(response->date);
			response->date=(SipDate*)ret;
			break;
		case Encryption:
			if(response->encryption != NULL)
				EncryptionFree(response->encryption);
			response->encryption=(SipEncrypt*)ret;
			break;
		case Error_Info:
			if(response->errorinfo != NULL)
				ErrorInfoFree(response->errorinfo);
			response->errorinfo=(SipInfo*)ret;
			break;
		case Event:
		case Event_Short:
			if(response->event != NULL)
				EventFree(response->event);
			
			response->event=(SipEvent*)ret;
			break;
		case Expires:
			if(response->expires != NULL)
				ExpiresFree(response->expires);
			response->expires=(SipExpires*)ret;
			break;
		case From_Short:
		case From:
			if(response->from != NULL)
				FromFree(response->from);
			response->from=(SipFrom*)ret;
			break;
		case MIME_Version:
			if(response->mimeversion != NULL)
				MIMEVersionFree((unsigned char*)response->mimeversion);
			response->mimeversion=(unsigned char*)ret;
			break;
		case Min_Expires:
			if(!ret) break;
			response->minexpires = *(int *) ret;
			break;
		case Min_SE:
			if(response->minse != NULL)
				MinSEFree(response->minse);
			response->minse=(SipMinSE*)ret;
			break;
		case Organization:
			if(response->organization != NULL)
				OrganizationFree((unsigned char*)response->organization);
			response->organization=(char*)ret;
			break;
		case P_Asserted_Identity:
			if(response->passertedidentity != NULL){
				PAssertedIdentityRspDupLink(response,(SipIdentity*)ret);
				PAssertedIdentityFree((SipIdentity*)ret);
			}
			else
				response->passertedidentity=(SipIdentity*)ret;
			break;
		case P_Preferred_Identity:
			if(response->ppreferredidentity != NULL){
				PPreferredIdentityRspDupLink(response,(SipIdentity*)ret);
				PPreferredIdentityFree((SipIdentity*)ret);
			}
			else
				response->ppreferredidentity=(SipIdentity*)ret;
			break;
		case Privacy:
			if (response->privacy != NULL) {
				PrivacyFree(response->privacy);
			}
			response->privacy=(SipPrivacy*)ret;
			break;
		case Proxy_Authenticate:
			if(response->proxyauthenticate != NULL){
				ProxyAuthenticateRspDupLink(response,(SipPxyAuthn*)ret);
				ProxyAuthenticateFree((SipPxyAuthn*)ret);
			}
			else
				response->proxyauthenticate=(SipPxyAuthn*)ret;
			break;
		case Record_Route:
			if(response->recordroute != NULL){
				RecordRouteRspDupLink(response,(SipRecordRoute*)ret);
				RecordRouteFree((SipRecordRoute*)ret);
			}else
				response->recordroute=(SipRecordRoute*)ret;
			break;
		case Refer_To:
		case Refer_To_Short:
			if(response->referto != NULL){
				ReferToRspDup(response,(SipReferTo*)ret);
				ReferToFree((SipReferTo*)ret);
			}else
				response->referto=(SipReferTo*)ret;
			break;
		case Referred_By:
		case Referred_By_Short:
			if(response->referredby != NULL)
				ReferredByFree((SipReferredBy*)ret);
			else
				response->referredby=(SipReferredBy*)ret;
			break;
		case Remote_Party_ID:
			if(response->remotepartyid != NULL){
				RemotePartyIDRspDupLink(response,(SipRemotePartyID *)ret);
				RemotePartyIDFree((SipRemotePartyID*)ret);
			}else
				response->remotepartyid=(SipRemotePartyID*)ret;
			break;
		case Replaces:
			if(response->replaces != NULL){
				ReplacesRspDupLink(response,(SipReplaces*)ret);
				ReplacesFree((SipReplaces*)ret);
			}else
				response->replaces=(SipReplaces*)ret;
			break;
		case Reply_To:
			if(response->replyto != NULL)
				ReplyToFree(response->replyto);
			response->replyto=(SipReplyTo*)ret;
			break;
		case Require:
			if(response->require != NULL)
				RequireFree(response->require);
			response->require=(SipRequire*)ret;
			break;
		case Retry_After:
			if(response->retryafter != NULL)
				RetryAfterFree(response->retryafter);
			response->retryafter=(SipRtyAft*)ret;
			break;
		case RSeq:
			response->rseq=*(int *)ret;
			break;
		case Session_Expires:
		case Session_Expires_Short:
			if(response->sessionexpires != NULL)
				SessionExpiresFree(response->sessionexpires);
			response->sessionexpires=(SipSessionExpires*)ret;
			break;
		case Server:
			if(response->server != NULL)
				ServerFree(response->server);
			response->server=(SipServer*)ret;
			break;
		case SIP_ETag:
			if(response->sipetag != NULL)
				SIPETagFree(response->sipetag);
			response->sipetag=(unsigned char*)ret;
			break;
		case Supported:
		case Supported_Short:
			if(response->supported != NULL)
				SupportedFree(response->supported);
			response->supported=(SipSupported*)ret;
			break;
		case Timestamp:
			if(response->timestamp != NULL)
				TimestampFree(response->timestamp);
			response->timestamp=(SipTimestamp*)ret;
			break;
		case To_Short:
		case To:
			if(response->to != NULL)
				ToFree(response->to);
			response->to=(SipTo*)ret;
			break;
		case Unsupported:
			if(response->unsupported != NULL)
				UnsupportedFree(response->unsupported);
			response->unsupported=(SipUnsupported*)ret;
			break;
		case User_Agent:
			if(response->useragent != NULL)  
				free(response->useragent);
			response->useragent=(SipUserAgent*)ret;
			break;
		case Via_Short:
		case Via:
			if(response->via == NULL) response->via=(SipVia*)ret;
			else{
				ViaRspDupLink(response,(SipVia*)ret);
				ViaFree((SipVia*)ret);
			}
		        break;
		case Warning:
			if(response->warning != NULL)
				WarningFree(response->warning);
			response->warning=(SipWarning*)ret;
			break;
		case WWW_Authenticate:
			if(response->wwwauthenticate != NULL){
				WWWAuthenticateRspDupLink(response,(SipWWWAuthn*)ret);
				WWWAuthenticateFree((SipWWWAuthn*)ret);
			}else
				response->wwwauthenticate=(SipWWWAuthn*)ret;
			break;
#ifdef ICL_IMS
		case P_Media_Authorization:
			if(response->pmediaauthorization != NULL)
				free(response->pmediaauthorization);
			response->pmediaauthorization=(unsigned char*)ret;
			break;
		case Path:
			if(response->path != NULL){
				PathRspDupLink(response,(SipPath*)ret);
				PathFree((SipPath*)ret);
			}else
				response->path=(SipPath*)ret;
			break;
		case P_Access_Network_Info:	
			if(response->paccessnetworkinfo != NULL)
				free(response->paccessnetworkinfo);
			response->paccessnetworkinfo=(unsigned char*)ret;
			break;
		case P_Associated_URI:
			if(response->passociateduri != NULL)
				free(response->passociateduri);
			response->passociateduri=(unsigned char*)ret;
			break;
		case P_Charging_Function_Address:	
			if(response->pchargingfunctionaddress != NULL)
				free(response->pchargingfunctionaddress);
			response->pchargingfunctionaddress=(unsigned char*)ret;
			break;
		case P_Charging_Vector:
			if(response->pchargingvector != NULL)
				free(response->pchargingvector);
			response->pchargingvector=(unsigned char*)ret;
			break;
		case Security_Server:
			if(response->securityserver != NULL)
				free(response->securityserver);
			response->securityserver=(unsigned char*)ret;
			break;
		case Service_Route:
			if(response->path != NULL){
				ServiceRouteRspDupLink(response,(SipServiceRoute*)ret);
				ServiceRouteFree((SipServiceRoute*)ret);
			}else
				response->serviceroute=(SipServiceRoute*)ret;
			break;
#endif /* end of ICL_IMS */
		default:
			TCRPrint(1,"<sipRspAddHdrFromTxt> Response Header Not Supported!\n");
			break;
		}/*end switch case */
	return RC_OK;
}

CCLAPI 
RCODE sipRspAddHdrFromTxt(SipRsp response, SipHdrType type, char* header)
{
	MsgHdr	hdr,tmp;
	void	*ret;
	char	*tmpbuf;
	DxStr	unHdrStr;
	RCODE	rcode=RC_OK;

	if(response == NULL)
		return RC_ERROR;
	response->rspflag=TRUE;
	/*tmpbuf=(char*)malloc(strlen(header)+2);*/
	tmpbuf=(char*)calloc(strlen(header)+2,sizeof(char));
	strcpy(tmpbuf,header);
	hdr=msgHdrNew();
	rfc822_parseline((unsigned char*)tmpbuf,strlen(header),hdr);
	tmp=hdr;
	if((tmp!=NULL)&&((tmp->name)!=NULL)){
		SipHdrType hdrType=sipHdrNameToType(tmp->name);
		if(hdrType==UnknSipHdr){
			TCRPrint(1,"<sipRspAddHdrFromTxt> Response Header Not Supported!%s\n",tmp->name);
			
			unHdrStr=dxStrNew();
			if (response->unHdr) {
				dxStrCat(unHdrStr,(const char*)response->unHdr);
				free(response->unHdr);
			}
			dxStrCat(unHdrStr,(const char*)header);
			dxStrCat(unHdrStr,(const char*)"\r\n");
			response->unHdr=strDup(dxStrAsCStr(unHdrStr));
			dxStrFree(unHdrStr);
		}else{
			if (RFC822ToSipHeader(tmp,hdrType,&ret)) {
				rcode=sipRspAddHdrEx(response,hdrType,ret);
				if((hdrType==Content_Length)
					||(hdrType==Max_Forwards)
					||(hdrType==Min_Expires)
					||(hdrType==RSeq)
					||(hdrType==Content_Length_Short))
					free(ret);
			}else
				rcode=RC_ERROR;
		}

		/*
		if(RFC822ToSipHeader(tmp,hdrType,&ret)==1){
		
		
			if(hdrType==UnknSipHdr) {
				TCRPrint(1,"<SipRspAddHdrFromTxt> Header Type Not supported, without parsing \n");
				if(response->unHdr == NULL)
					response->unHdr=strDup((const char*)header);
				else{
					unHdrStr=dxStrNew();
					dxStrCat(unHdrStr,(const char*)response->unHdr);
					dxStrCat(unHdrStr,(const char*)header);
					free(response->unHdr);
					response->unHdr=strDup(dxStrAsCStr(unHdrStr));
					dxStrFree(unHdrStr);
				}
			}else
				rcode=sipRspAddHdrEx(response,hdrType,ret);
			
		}else{
			rcode=RC_ERROR;
		}
		*/
	}else
		rcode=RC_ERROR;

	msgHdrFree(hdr);
	free(tmpbuf);
	return rcode;
}

/* get original header list */
CCLAPI 
MsgHdr	sipRspGetHdrLst(IN SipRsp response)
{
	if (response) {
		return response->hdrlist;
	}else
		return NULL;
}

CCLAPI 
SipBody* sipRspGetBody(SipRsp response)
{
	if(response == NULL)
		return NULL;
	else
		return response->body;
}

CCLAPI 
RCODE sipRspSetBody(SipRsp response,SipBody *data)
{
	SipBody *_body;

	if(response == NULL)
		return RC_ERROR;
	response->rspflag=TRUE;
	_body = (SipBody*)calloc(1,sizeof(*_body));
	_body->content =(unsigned char*) strDup((const char*)data->content);
	_body->length = data->length;
	response->body = _body;
	return RC_OK;
}

/* temporatory change to internal function */
int sipRspParse(SipRsp response,unsigned char* msg,int* length)
{
	MsgHdr hdr,tmp;
	MsgBody _pby;
	int hdrlength,linelength;
	unsigned char *statusline, *code, *reason, *lineend;
	void *ret;
	DxStr dxBuf;
	SipRetMsg  sipCode=SIP_OK;
	SipRspStatusLine *tmpStatusLine=NULL;
  
	response->rspflag=TRUE;
	/* parse Status-Line */
	lineend = (unsigned char*)strchr((const char*)msg,'\n');
	if (lineend == NULL)
		return 0;
	else 
		linelength = lineend - msg + 1;
	*lineend = '\0';	
	*(lineend-1) = ( *(lineend-1)=='\r' )?0:*(lineend-1);

	statusline = (unsigned char *)calloc(linelength,sizeof(unsigned char)); /* temporary use */
	if (!statusline) return 0;
	strncpy((char*)statusline,(const char*)msg, linelength);
	code = (unsigned char*)strchr((const char*)statusline,' ');
	if (code == NULL)
		return 0;
	else{
		*code = '\0';
		code++;
	};
	reason = (unsigned char*)strchr((const char*)code, ' ');
	if (reason == NULL)
		return 0;
	else{
		*reason = '\0';
		reason++;
	};
	tmpStatusLine=(SipRspStatusLine*)calloc(1,sizeof(SipRspStatusLine));
	if (!tmpStatusLine) return 0;
	tmpStatusLine->ptrVersion=strDup((const char*)statusline);
	tmpStatusLine->ptrStatusCode=strDup((const char*)code);
	tmpStatusLine->ptrReason=strDup((const char*)reason);
	free(statusline);
	/*
	sipRspSetStatusLine(response,tmpStatusLine);
	free(tmpStatusLine->ptrReason);
	free(tmpStatusLine->ptrStatusCode);
	free(tmpStatusLine->ptrVersion);
	free(tmpStatusLine);
	*/
	response->rspflag=TRUE;
	response->statusline=tmpStatusLine;

	/* parse header */
	hdr = msgHdrNew(); /* temporary create, free later */
	msg = lineend + 1;
	if( rfc822_parse(msg, &hdrlength, hdr)==0 ){
		TCRPrint(1,"<sipResponseParse> Resposne Message Parse Error!\n");
		return SIP_RFC822PARSE_ERROR;
	}

	tmp = hdr;

#ifdef CCLSIP_HDRLIST
	/* set response hdrlist 2004/11/22 */
	response->hdrlist=hdr;
#endif
	
	while (1){
			if ((tmp == NULL) ||(tmp->name == NULL)){
				break;
		}else{
			SipHdrType hdrType =sipHdrNameToType(tmp->name); 
			/* save hdrtype to hdrlist */
			tmp->hdrtype=hdrType;
			if(hdrType == UnknSipHdr){
				/*sipCode=SIP_UNKNOWN_HEADER;*/
				TCRPrint(1,"<sipResponseParse> Get a unKnown header:%s\n!",tmp->name);
				dxBuf=dxStrNew();
				if(response->unHdr == NULL) {
					msgHdrAsStr(tmp,dxBuf);
					response->unHdr=strDup((const char*)dxStrAsCStr(dxBuf));
				}else{
					dxStrCat(dxBuf,(const char*)response->unHdr);
					free(response->unHdr);
					msgHdrAsStr(tmp,dxBuf);
					response->unHdr=strDup((const char*)dxStrAsCStr(dxBuf));
				}
				dxStrFree(dxBuf);
			}else if( RFC822ToSipHeader(tmp, hdrType, &ret)==0 ){
				sipCode=sipParseErrMsg(hdrType);
				dxBuf=dxStrNew();
				if(response->unHdr == NULL){
					msgHdrAsStr(tmp,dxBuf);
					response->unHdr=strDup((const char*)dxStrAsCStr(dxBuf));
				}else{
					dxStrCat(dxBuf,(const char*)response->unHdr);
					free(response->unHdr);
					msgHdrAsStr(tmp,dxBuf);
					response->unHdr=strDup((const char*)dxStrAsCStr(dxBuf));
				}
				dxStrFree(dxBuf);
				/*goto RESPONSE_PARSE_ERROR;*/
			}else{
				sipRspAddHdrEx(response,hdrType,ret);
				/* for int type,it need free*/
				if((hdrType==Content_Length)
					||(hdrType==Max_Forwards)
					||(hdrType==Min_Expires)
					||(hdrType==RSeq)
					||(hdrType==Content_Length_Short))
					free(ret);
			}
			tmp = tmp->next;
		}
	}

	/* extract body */
	if(response->contentlength > 0){
		_pby = (MsgBody)calloc(1,sizeof(*_pby));
		rfc822_extractbody(msg+hdrlength, hdr, _pby);
		/*sipRspSetBody(response,(SipBody*)_pby);*/
		response->rspflag=TRUE;
		response->body = (SipBody*)_pby;
		*length = hdrlength+_pby->length;
		/*
		free(_pby->content);
		free(_pby);
		*/
	}

#ifndef CCLSIP_HDRLIST
	msgHdrFree(hdr);
#endif
	
	return sipCode;

/*RESPONSE_PARSE_ERROR:
	msgHdrFree(hdr);
	return 0;*/
}

int sipRspAsStr(SipRsp response, DxStr RspBuf, int *length)
{
	int tmp, lenstr;
	SipRspStatusLine *rspline;
	DxStr ResponseStr;
	unsigned char *HeaderBuf;
	int Len=SIP_MAX_CONTENTLENGTH;

	if(response == NULL)
		return 0;

	HeaderBuf=(unsigned char*) calloc(Len,sizeof(unsigned char));
	ResponseStr=RspBuf;
	rspline=sipRspGetStatusLine(response);
	if(rspline != NULL){
		dxStrCat(ResponseStr,(char*)rspline->ptrVersion);
		dxStrCat(ResponseStr," ");
		dxStrCat(ResponseStr,(char*)rspline->ptrStatusCode);
		dxStrCat(ResponseStr," ");
		dxStrCat(ResponseStr,(char*)rspline->ptrReason);
		dxStrCat(ResponseStr,"\r\n");
	}

	*length=dxStrLen(ResponseStr);

	lenstr=*length;
	if (response->accept != NULL) {
		AcceptAsStr(response->accept, HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr + tmp;
	}
	if (response->acceptencoding != NULL) {
		AcceptEncodingAsStr(response->acceptencoding,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if (response->acceptlanguage != NULL) {
		AcceptLanguageAsStr(response->acceptlanguage,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if(response->alertinfo != NULL) {
		AlertInfoAsStr(response->alertinfo,HeaderBuf,Len,&tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if (response->allow != NULL) {
		AllowAsStr(response->allow,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if(response->allowevents != NULL){
		AllowEventsAsStr(response->allowevents,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if(response->authenticationinfo != NULL) {
		AuthenticationInfoAsStr(response->authenticationinfo,HeaderBuf,Len,&tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if (response->callid != NULL) {
		CallIDAsStr(response->callid,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if(response->callinfo != NULL) {
		CallInfoAsStr(response->callinfo,HeaderBuf,Len,&tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if (response->contact != NULL) {
		sipContactAsStr(response->contact,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if (response->contentdisposition != NULL) {
		ContentDispositionAsStr(response->contentdisposition,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if (response->contentencoding != NULL) {
		ContentEncodingAsStr(response->contentencoding,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if (response->contentlanguage != NULL) {
		ContentLanguageAsStr(response->contentlanguage,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if (response->contentlength != -1) {
		ContentLengthAsStr(response->contentlength,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if (response->contenttype != NULL) {
		ContentTypeAsStr(response->contenttype,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if (response->cseq != NULL) {
		CSeqAsStr(response->cseq,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if (response->date != NULL) {
		DateAsStr(response->date,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if (response->encryption != NULL) {
		EncryptionAsStr(response->encryption,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if (response->errorinfo != NULL) {
		ErrorInfoAsStr(response->errorinfo,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if (response->event != NULL){
		EventAsStr(response->event,HeaderBuf,Len,&tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr +tmp;
	}
	if (response->expires != NULL) {
		ExpiresAsStr(response->expires,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if (response->from != NULL) {
		FromAsStr(response->from, HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if (response->mimeversion != NULL) {
		MIMEVersionAsStr((unsigned char*)response->mimeversion,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if (response->minexpires != -1) {
		MinExpiresAsStr(response->minexpires,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if (response->minse != NULL) {
		MinSEAsStr(response->minse,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if (response->organization != NULL) {
		OrganizationAsStr((unsigned char*)response->organization,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if(NULL != response->passertedidentity){
		PAssertedIdentityAsStr(response->passertedidentity,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if(NULL != response->ppreferredidentity){
		PPreferredIdentityAsStr(response->ppreferredidentity,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if (NULL != response->privacy) {
		PrivacyAsStr(response->privacy,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if (response->proxyauthenticate != NULL) {
		ProxyAuthenticateAsStr(response->proxyauthenticate,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if (response->recordroute != NULL) {
		RecordRouteAsStr(response->recordroute,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}

	if(response->referto != NULL){
		ReferToAsStr(response->referto,HeaderBuf,Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr=lenstr+tmp;
	}
	if(response->referredby != NULL){
		ReferredByAsStr(response->referredby,HeaderBuf,Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr=lenstr+tmp;
	}
	if(response->remotepartyid != NULL){
		RemotePartyIDAsStr(response->remotepartyid,HeaderBuf,Len,&tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr=lenstr+tmp;
	}
	if(response->replaces != NULL){
		ReplacesAsStr(response->replaces,HeaderBuf,Len,&tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr=lenstr+tmp;
	}
	if(response->replyto != NULL){
		ReplyToAsStr(response->replyto,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	
	if (response->require != NULL) {
		RequireAsStr(response->require,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}

	if (response->retryafter != NULL) {
		RetryAfterAsStr(response->retryafter,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	
	if (response->rseq > -1) {
		RSeqAsStr(response->rseq,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}

	if(response->sessionexpires != NULL){
		SessionExpiresAsStr(response->sessionexpires,HeaderBuf,Len,&tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr=lenstr+tmp;
	}
	if (response->server != NULL) {
		ServerAsStr(response->server,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if (response->sipetag != NULL) {
		SIPETagAsStr(response->sipetag,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if (response->supported != NULL){
		SupportedAsStr(response->supported,HeaderBuf,Len,&tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if (response->timestamp != NULL) {
		TimestampAsStr(response->timestamp,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if (response->to != NULL) {
		ToAsStr(response->to,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if (response->unsupported != NULL) {
		UnsupportedAsStr(response->unsupported,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if (response->useragent != NULL) {
		UserAgentAsStr(response->useragent,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if (response->warning != NULL) {
		WarningAsStr(response->warning,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if (response->via != NULL) {
		ViaAsStr(response->via,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
	if (response->wwwauthenticate != NULL) {
		WWWAuthenticateAsStr(response->wwwauthenticate,HeaderBuf, Len, &tmp);
		dxStrCat(ResponseStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	}
#ifdef ICL_IMS
	if (response->pmediaauthorization != NULL) {
		if(PMAAsStr(response->pmediaauthorization,HeaderBuf, Len, &tmp)==1){
			dxStrCat(ResponseStr,(char*)HeaderBuf);
			lenstr = lenstr+tmp;
		}
	}
	if (response->path != NULL) {
		if(PathAsStr(response->path,HeaderBuf, Len, &tmp)==1){
			dxStrCat(ResponseStr,(char*)HeaderBuf);
			lenstr = lenstr+tmp;
		}
	}
	if (response->paccessnetworkinfo != NULL) {
		if(PANIAsStr(response->paccessnetworkinfo,HeaderBuf, Len, &tmp)==1){
			dxStrCat(ResponseStr,(char*)HeaderBuf);
			lenstr = lenstr+tmp;
		}
	}
	if (response->passociateduri != NULL) {
		if(PAUAsStr(response->passociateduri,HeaderBuf, Len, &tmp)==1){
			dxStrCat(ResponseStr,(char*)HeaderBuf);
			lenstr = lenstr+tmp;
		}
	}
	if (response->pchargingfunctionaddress != NULL) {
		if(PCFAAsStr(response->pchargingfunctionaddress,HeaderBuf, Len, &tmp)==1){
			dxStrCat(ResponseStr,(char*)HeaderBuf);
			lenstr = lenstr+tmp;
		}
	}
	if (response->pchargingvector != NULL) {
		if(PCVAsStr(response->pchargingvector,HeaderBuf, Len, &tmp)==1){
			dxStrCat(ResponseStr,(char*)HeaderBuf);
			lenstr = lenstr+tmp;
		}
	}
	if (response->securityserver != NULL) {
		if(SecurityServerAsStr(response->securityserver,HeaderBuf, Len, &tmp)==1){
			dxStrCat(ResponseStr,(char*)HeaderBuf);
			lenstr = lenstr+tmp;
		}
	}
	if (response->serviceroute != NULL) {
		if(ServiceRouteAsStr(response->serviceroute,HeaderBuf, Len, &tmp)==1){
			dxStrCat(ResponseStr,(char*)HeaderBuf);
			lenstr = lenstr+tmp;
		}
	}
#endif /* end of ICL_IMS */
	if(response->unHdr != NULL){
		dxStrCat(ResponseStr,(const char*)response->unHdr);
		lenstr=lenstr+strlen(response->unHdr);
	}


	dxStrCat(ResponseStr,"\r\n");
	if(response->body != NULL){
		if(response->body->length > 0){
			dxStrCat(ResponseStr,(char*)response->body->content);
		}
	}
	dxStrCat(ResponseStr,"\0");
	*length=dxStrLen(ResponseStr);
	free(HeaderBuf);

	return 1;
}


CCLAPI 
SipRsp sipRspNewFromTxt(char *text)
{
	SipRsp response;
	char*		temp;
	int length=0;

	length=strlen(text);
	temp = (char*)calloc((length+1),sizeof(char));
	strcpy(temp,text);

	response=sipRspNew();
	response->rspflag=TRUE;
	if( sipRspParse(response,(unsigned char*)temp,&length)==0 ){
		sipRspFree(response);
		free(temp);
		return NULL;
	}

	free(temp);

	return response;
}

CCLAPI 
char*	sipRspPrint(SipRsp response)
{
	int retsize;
	DxStr RspBuf;
	
	if ( !response )
		return "\0";

	if(response->rspflag==TRUE){
		RspBuf=dxStrNew();
		sipRspAsStr(response,RspBuf,&retsize);
		if(response->rspbuf != NULL)
			free(response->rspbuf);
		response->rspbuf=strDup(dxStrAsCStr(RspBuf));
		dxStrFree(RspBuf);
		response->rspflag=TRUE;
	}

	return response->rspbuf;
}

/* to set unkwon header buf in response */
CCLAPI 
RCODE	sipRspAddUnknowHdr(IN SipRsp response,IN const char *unknowhdr)
{
	/* check input parameter*/
	if(response==NULL){
		TCRPrint(1,"***[sipRspAddUnknowHdr] input response is NULL!");
		return SIP_REQ_ERROR;
	}
	if(unknowhdr==NULL){
		TCRPrint(1,"***[sipRspAddUnknowHdr] input unknow hrea is NULL!");
		return RC_ERROR;
	}
	/* check request->unhdr */
	if(response->unHdr)
		free(response->unHdr);

	response->unHdr=strDup(unknowhdr);
	return RC_OK;
}

/*-----------------------------------------------------------------*/
/*                      Internal Functions                         */
/*-----------------------------------------------------------------*/
int AcceptRspDup(SipRsp response,SipAccept *accept )
{
	if(response->accept != NULL)
		AcceptFree(response->accept);
	response->accept=(SipAccept*)calloc(1,sizeof(SipAccept));
	response->accept->numMediaTypes=accept->numMediaTypes;
	response->accept->sipMediaTypeList=NULL;
	if(accept->sipMediaTypeList != NULL)
		response->accept->sipMediaTypeList=sipMediaTypeDup(accept->sipMediaTypeList,accept->numMediaTypes);
	return 1;
}

int AcceptEncodingRspDup(SipRsp response,SipAcceptEncoding* coding)
{
	if(response->acceptencoding != NULL)
		AcceptEncodingFree(response->acceptencoding);
	response->acceptencoding=(SipAcceptEncoding*)calloc(1,sizeof(SipAcceptEncoding));
	response->acceptencoding->numCodings=coding->numCodings;
	response->acceptencoding->sipCodingList=NULL;
	if(coding->sipCodingList != NULL)
		response->acceptencoding->sipCodingList=sipCodingDup(coding->sipCodingList,coding->numCodings);
	return 1;
}

int AcceptLanguageRspDup(SipRsp response, SipAcceptLang* lang)
{
	if(response->acceptlanguage != NULL)
		AcceptLanguageFree(response->acceptlanguage);
	response->acceptlanguage=(SipAcceptLang*)calloc(1,sizeof(SipAcceptLang));
	response->acceptlanguage->numLangs=lang->numLangs;
	response->acceptlanguage->sipLangList=NULL;
	if(lang->sipLangList != NULL)
		response->acceptlanguage->sipLangList=sipLanguageDup(lang->sipLangList,lang->numLangs);
	return 1;
}

/**add by tyhuang**************/
int AlertInfoRspDup(SipRsp response,SipInfo *Alertinfo)
{
	SipInfo *nowAlertinfo,*tmpAlertinfo;

	if(response->alertinfo != NULL)
		AlertInfoFree(response->alertinfo);

	nowAlertinfo=NULL;
	tmpAlertinfo=NULL;
	while(Alertinfo!=NULL){
		nowAlertinfo=(SipInfo*)calloc(1,sizeof(SipInfo));
		if(tmpAlertinfo != NULL) 
			tmpAlertinfo->next=nowAlertinfo;
		else
			response->alertinfo=nowAlertinfo;
		nowAlertinfo->absoluteURI=NULL;
		nowAlertinfo->next=NULL;
		nowAlertinfo->numParams=Alertinfo->numParams;
		nowAlertinfo->SipParamList=NULL;
		if(Alertinfo->absoluteURI!=NULL)
			nowAlertinfo->absoluteURI=strDup((const char *)Alertinfo->absoluteURI);
		if(Alertinfo->SipParamList!=NULL)
			nowAlertinfo->SipParamList=sipParameterDup(Alertinfo->SipParamList,Alertinfo->numParams);
		Alertinfo=Alertinfo->next;
		tmpAlertinfo=nowAlertinfo;
	}
	return 1;

}
/******************************/

int AllowRspDup(SipRsp response, SipAllow *allow)
{
	int idx;
	if(response->allow == NULL)
		response->allow=(SipAllow*)calloc(1,sizeof(SipAllow));
	for(idx=0;idx<SIP_SUPPORT_METHOD_NUM;idx++)
		response->allow->Methods[idx]=allow->Methods[idx];
	return 1;
}

int AllowEventsRspDup(SipRsp response, SipAllowEvt *allowevt)
{
	int count=0;
	SipAllowEvt *tmpallowevt;
	if(response->allowevents != NULL){
		AllowEventsFree(response->allowevents);
		response->allowevents=NULL;
	}
	while(allowevt != NULL){
		if(count==0){
			response->allowevents=(SipAllowEvt*)calloc(1,sizeof(SipAllowEvt));
			tmpallowevt=response->allowevents;
		}else{
			tmpallowevt->next=(SipAllowEvt*)calloc(1,sizeof(SipAllowEvt));
			tmpallowevt=tmpallowevt->next;
		}
		tmpallowevt->next=NULL;
		tmpallowevt->pack=NULL;
		tmpallowevt->tmplate=NULL;
		if(allowevt->pack != NULL)
			tmpallowevt->pack = strDup(allowevt->pack);
		if(allowevt->tmplate != NULL)
			tmpallowevt->tmplate = sipStrDup(allowevt->tmplate,-1);
		allowevt=allowevt->next;
		count++;
	}/* end of while */
	return 1;
}
/**add by tyhuang**************/
int AuthenticationInfoRspDup(SipRsp response,SipAuthInfo *authinfo)
{
	if(response->authenticationinfo != NULL)
		AuthenticationInfoFree(response->authenticationinfo);
	if(authinfo != NULL){
		response->authenticationinfo=(SipAuthInfo*)calloc(1,sizeof(SipAuthInfo));
		response->authenticationinfo->numAinfos=authinfo->numAinfos;
		response->authenticationinfo->ainfoList=NULL;
		if(authinfo->ainfoList != NULL)
			response->authenticationinfo->ainfoList=sipParameterDup(authinfo->ainfoList,authinfo->numAinfos);
	}
	return 1;
}
/******************************/
int CallIDRspDup(SipRsp response,unsigned char* str)
{
	if(response->callid != NULL)
		free(response->callid);
	if(str != NULL)
		response->callid=(unsigned char*)strDup((char*)str);
	return 1;
}

/**add by tyhuang**************/
int CallInfoRspDup(SipRsp response,SipInfo *Callinfo)
{
	SipInfo *nowCallinfo,*tmpCallinfo;

	if(response->callinfo != NULL)
		CallInfoFree(response->callinfo);

	nowCallinfo=NULL;
	tmpCallinfo=NULL;
	while(Callinfo!=NULL){
		nowCallinfo=(SipInfo*)calloc(1,sizeof(SipInfo));
		if(tmpCallinfo != NULL) 
			tmpCallinfo->next=nowCallinfo;
		else
			response->callinfo=nowCallinfo;
		nowCallinfo->absoluteURI=NULL;
		nowCallinfo->next=NULL;
		nowCallinfo->numParams=Callinfo->numParams;
		nowCallinfo->SipParamList=NULL;
		if(Callinfo->absoluteURI!=NULL)
			nowCallinfo->absoluteURI=strDup((const char *)Callinfo->absoluteURI);
		if(Callinfo->SipParamList!=NULL)
			nowCallinfo->SipParamList=sipParameterDup(Callinfo->SipParamList,Callinfo->numParams);
		Callinfo=Callinfo->next;
		tmpCallinfo=nowCallinfo;
	}
	return 1;

}
/******************************/

int ContactRspDupLink(SipRsp response,SipContact* contact)
{
	SipContactElm *elm,*ptrelm,*firstelm,*lastelm;

	elm=sipContactElmDup(contact->sipContactList,contact->numContacts);

	if(response->contact != NULL) {
		ptrelm=response->contact->sipContactList;
		firstelm=lastelm=ptrelm;
		while(ptrelm != NULL){
			lastelm=ptrelm;
			ptrelm=ptrelm->next;
		}
		response->contact->numContacts=response->contact->numContacts+contact->numContacts;
		lastelm->next=elm;
	}else{
		response->contact=(SipContact*)calloc(1,sizeof(SipContact));
		response->contact->numContacts=contact->numContacts;
		response->contact->sipContactList=NULL;
		response->contact->sipContactList=elm;

	}
	return 1;
}

int ContentDispositionRspDup(SipRsp response,SipContentDisposition* contentdisp)
{
	if(response->contentdisposition != NULL)
		ContentDispositionFree(response->contentdisposition);
	response->contentdisposition=(SipContentDisposition*)calloc(1,sizeof(SipContentDisposition));
	response->contentdisposition->disptype = NULL;
	response->contentdisposition->paramList = NULL;

	if(contentdisp->disptype != NULL)
		response->contentdisposition->disptype=strDup(contentdisp->disptype);
	if(contentdisp->paramList != NULL)
		response->contentdisposition->paramList=sipParameterDup(contentdisp->paramList,contentdisp->numParam);
	return 1;
}

int ContentEncodingRspDup(SipRsp response,SipContEncoding* contcoding)
{
	if(response->contentencoding != NULL)
		ContentEncodingFree(response->contentencoding);
	response->contentencoding=(SipContEncoding*)calloc(1,sizeof(SipContEncoding));
	response->contentencoding->numCodings=contcoding->numCodings;
	response->contentencoding->sipCodingList=NULL;
	if(contcoding->sipCodingList != NULL)
		response->contentencoding->sipCodingList=sipCodingDup(contcoding->sipCodingList,contcoding->numCodings);
	return 1;
}

int ContentLanguageRspDup(SipRsp response,SipContentLanguage* contentlang)
{
	if(response->contentlanguage != NULL)
		ContentLanguageFree(response->contentlanguage);
	response->contentlanguage=(SipContentLanguage*)calloc(1,sizeof(SipContentLanguage));
	response->contentlanguage->numLang=contentlang->numLang;
	response->contentlanguage->langList=NULL;
	if(contentlang->langList != NULL)
		response->contentlanguage->langList=sipStrDup(contentlang->langList,contentlang->numLang);
	return 1;
}

int ContentTypeRspDup(SipRsp response,SipContType* conttype)
{
	if(response->contenttype != NULL)
		ContentTypeFree(response->contenttype);
	response->contenttype=(SipContType*)calloc(1,sizeof(SipContType));
	response->contenttype->next=NULL;
	response->contenttype->numParam=conttype->numParam;
	response->contenttype->type=NULL;
	response->contenttype->subtype=NULL;
	response->contenttype->sipParamList=NULL;
	if(conttype->type != NULL)
		response->contenttype->type=strDup(conttype->type);
	if(conttype->subtype != NULL)
		response->contenttype->subtype=strDup(conttype->subtype);
	if(conttype->sipParamList != NULL)
		response->contenttype->sipParamList=sipParameterDup(conttype->sipParamList,conttype->numParam);
	return 1;
}

int CSeqRspDup(SipRsp response,SipCSeq* seq)
{
	if(response->cseq != NULL)
		CSeqFree(response->cseq);
	response->cseq=(SipCSeq*)calloc(1,sizeof(SipCSeq));
	response->cseq->Method=seq->Method;
	response->cseq->seq=seq->seq;
	return 1;
}

int DateRspDup(SipRsp response,SipDate* InDate)
{
	if(response->date != NULL)
		DateFree(response->date);
	response->date=(SipDate*)calloc(1,sizeof(SipDate));
	if(InDate->date != NULL){
		response->date->date=(SipRfc1123Date*)calloc(1,sizeof(SipRfc1123Date));
		response->date->date->month=InDate->date->month;
		response->date->date->weekday=InDate->date->weekday;
	}
	if(InDate->date->year != NULL)
		strcpy(response->date->date->year,InDate->date->year);
	if(InDate->date->day != NULL)
		strcpy(response->date->date->day,InDate->date->day);
	if(InDate->date->time != NULL)
		strcpy(response->date->date->time,InDate->date->time);
	return 1;
}

int EncryptionRspDup(SipRsp response,SipEncrypt* encrypt)
{
	if(response->encryption != NULL)
		EncryptionFree(response->encryption);
	response->encryption=(SipEncrypt*)calloc(1,sizeof(SipEncrypt));
	response->encryption->numParams=encrypt->numParams;
	response->encryption->scheme=NULL;
	response->encryption->sipParamList=NULL;
	if(encrypt->scheme != NULL)
		response->encryption->scheme=strDup(encrypt->scheme);
	if(encrypt->sipParamList != NULL)
		response->encryption->sipParamList=sipParameterDup(encrypt->sipParamList,encrypt->numParams);
	return 1;
}

/**add by tyhuang**************/
int ErrorInfoRspDup(SipRsp response,SipInfo *Errortinfo)
{
	SipInfo *nowErrorinfo,*tmpErrorinfo;

	if(response->errorinfo != NULL)
		ErrorInfoFree(response->errorinfo);

	nowErrorinfo=NULL;
	tmpErrorinfo=NULL;
	while(Errortinfo!=NULL){
		nowErrorinfo=(SipInfo*)calloc(1,sizeof(SipInfo));
		if(tmpErrorinfo != NULL) 
			tmpErrorinfo->next=nowErrorinfo;
		else
			response->errorinfo=nowErrorinfo;
		nowErrorinfo->absoluteURI=NULL;
		nowErrorinfo->next=NULL;
		nowErrorinfo->numParams=Errortinfo->numParams;
		nowErrorinfo->SipParamList=NULL;
		if(Errortinfo->absoluteURI!=NULL)
			nowErrorinfo->absoluteURI=strDup((const char *)Errortinfo->absoluteURI);
		if(Errortinfo->SipParamList!=NULL)
			nowErrorinfo->SipParamList=sipParameterDup(Errortinfo->SipParamList,Errortinfo->numParams);
		Errortinfo=Errortinfo->next;
		tmpErrorinfo=nowErrorinfo;
	}
	return 1;

}

int EventRspDup(SipRsp rsp,SipEvent *event)
{

	SipEvent *newevent,*finalevent;

	if(!rsp || !event)
		return 0;

	finalevent=rsp->event;
	
	if(finalevent!= NULL)
		EventFree(finalevent);
	
	newevent=(SipEvent*)calloc(1,sizeof(*newevent));
	if(newevent)
		rsp->event=newevent;
	else{
		rsp->event=NULL;
		return 0;
	}

	if(event->paramList)
		newevent->paramList=sipParameterDup(event->paramList,event->numParam);
	else{
		newevent->paramList=NULL;
		newevent->numParam=0;
		
	}
	if(event->tmplate)
		newevent->tmplate=sipStrDup(event->tmplate,-1);
	else
		newevent->tmplate=NULL;
	if(event->pack)
		newevent->pack=strDup(event->pack);
	else
		newevent->pack=NULL;

	return 1;
}


int ExpiresRspDup(SipRsp response,SipExpires* expire)
{
	if(response->expires != NULL)
		ExpiresFree(response->expires);
	response->expires=(SipExpires*)calloc(1,sizeof(SipExpires));
	response->expires->expireSecond=expire->expireSecond;
	/*response->expires->expireDate=(rfc1123Date*)malloc(sizeof(rfc1123Date)); */
	/*response->expires->expireDate = ((expire->expireDate)?rfc1123DateDup(expire->expireDate):NULL);*/

	/*strcpy(response->expires->expireDate->day,expire->expireDate->day);*/
	/*strcpy(response->expires->expireDate->year,expire->expireDate->year);*/
	/*strcpy(response->expires->expireDate->time,expire->expireDate->time);*/
	/*response->expires->expireDate->month=expire->expireDate->month;*/
	/*response->expires->expireDate->weekday=expire->expireDate->weekday;*/

	return 1;
}

int FromRspDup(SipRsp response,SipFrom* Infrom)
{
	if(response->from != NULL)
		FromFree(response->from);
	response->from=(SipFrom*)calloc(1,sizeof(SipFrom));
	response->from->numParam=Infrom->numParam;
	response->from->numAddr=Infrom->numAddr;
	response->from->address=NULL;
	response->from->ParamList=NULL;
	if(Infrom->address != NULL)
		response->from->address=sipAddrDup(Infrom->address,Infrom->numAddr);
	if(Infrom->ParamList != NULL)
		response->from->ParamList=sipParameterDup(Infrom->ParamList,Infrom->numParam);
		/* response->from->tagList=sipStrDup(Infrom->tagList,Infrom->numTags); */
	return 1;
}

int MIMEVersionRspDup(SipRsp response,unsigned char* str)
{
	if(response->mimeversion != NULL)
		free(response->mimeversion);
	response->mimeversion=(unsigned char*)strDup((const char*)str);
	return 1;
}

int MinSERspDup(SipRsp rsp,SipMinSE *minse)
{
	SipMinSE *newobj;

	if(rsp->minse != NULL)
		MinSEFree(rsp->minse);
	if(minse != NULL){
		newobj=(SipMinSE*)calloc(1,sizeof(*newobj));
		newobj->deltasecond=minse->deltasecond;
		newobj->numParam=minse->numParam;
		newobj->paramList=sipParameterDup(minse->paramList,minse->numParam);
		rsp->minse=newobj;
	}else
		rsp->minse=NULL;
	return 1;
}

int OrganizationRspDup(SipRsp response,const char* str)
{
	if(response->organization != NULL)
		free(response->organization);
	response->organization=strDup((const char*)str);
	return 1;
}

int	PAssertedIdentityRspDupLink(SipRsp response,SipIdentity *pId)
{
	SipIdentity *last,*newid;

	if(response==NULL)
		return 0;
	if(pId==NULL)
		return 0;
	newid=(SipIdentity*)calloc(1,sizeof(SipIdentity));

	if(response->passertedidentity==NULL)
		response->passertedidentity=newid;
	else{
		last=response->passertedidentity;
		while (last) {
			if(last->next==NULL) break;
			last=last->next;
		}
		last->next=newid;
	}

	while(pId){
		newid->numAddr=pId->numAddr;
		newid->address=sipAddrDup(pId->address,pId->numAddr);
		newid->next=NULL;
		pId=pId->next;
		if(pId){
			newid->next=(SipIdentity*)calloc(1,sizeof(SipIdentity));
			newid=newid->next;
		}
	}
	return 1;
}

int	PPreferredIdentityRspDupLink(SipRsp response,SipIdentity *pId)
{

	SipIdentity *last,*newid;

	if(response==NULL)
		return 0;
	if(pId==NULL)
		return 0;
	newid=(SipIdentity*)calloc(1,sizeof(SipIdentity));

	if(response->ppreferredidentity==NULL)
		response->ppreferredidentity=newid;
	else{
		last=response->ppreferredidentity;
		while (last) {
			if(last->next==NULL) break;
			last=last->next;
		}
		last->next=newid;
	}

	while(pId){
		newid->numAddr=pId->numAddr;
		newid->address=sipAddrDup(pId->address,pId->numAddr);
		newid->next=NULL;
		pId=pId->next;
		if(pId){
			newid->next=(SipIdentity*)calloc(1,sizeof(SipIdentity));
			newid=newid->next;
		}
	}
	return 1;
}

int PrivacyRspDup(SipRsp response,SipPrivacy *pPrivacy)
{
	if(response){
		if(response->privacy)
			PrivacyFree(response->privacy);
	}else
		return 0;

	if(pPrivacy)
		response->privacy=calloc(1,sizeof(SipPrivacy));
	else{
		response->privacy=NULL;
		return 0;
	}
	if(pPrivacy->privvalue){
		response->privacy->privvalue=sipStrDup(pPrivacy->privvalue,pPrivacy->numPriv);
		response->privacy->numPriv=pPrivacy->numPriv;
	}else{
		response->privacy->privvalue=NULL;
		response->privacy->numPriv=0;
	}
	return 1;
}

int ProxyAuthenticateRspDupLink(SipRsp response,SipPxyAuthn *authen)
{
	SipChllng *origChllng, *newChllng;
	if(response->proxyauthenticate == NULL){
		response->proxyauthenticate=(SipPxyAuthn*)calloc(1,sizeof(SipPxyAuthn));
		response->proxyauthenticate->numChllngs = authen->numChllngs;
		response->proxyauthenticate->ChllngList=NULL;
		if(authen->ChllngList != NULL)
			response->proxyauthenticate->ChllngList=sipChallengeDup(authen->ChllngList,authen->numChllngs);
	}else{
		if(authen->ChllngList != NULL)
			newChllng=sipChallengeDup(authen->ChllngList,authen->numChllngs);
		response->proxyauthenticate->numChllngs+=authen->numChllngs;
		origChllng=response->proxyauthenticate->ChllngList;
		while(origChllng->next != NULL)
			origChllng=origChllng->next;
		origChllng->next=newChllng;
	}
	return 1;
}

int RecordRouteRspDupLink(SipRsp response,SipRecordRoute* record)
{
	SipRecordRoute *tmpRR;
	SipRecAddr *newRRAddr, *oldRRAddr;
	if(response->recordroute == NULL){
		response->recordroute=(SipRecordRoute*)calloc(1,sizeof(SipRecordRoute));
		response->recordroute->numNameAddrs=record->numNameAddrs;
		response->recordroute->sipNameAddrList=NULL;
		if(record->sipNameAddrList != NULL)
			response->recordroute->sipNameAddrList=sipRecAddrDup(record->sipNameAddrList,record->numNameAddrs);
	}else{
		if(record->sipNameAddrList != NULL){
			newRRAddr=sipRecAddrDup(record->sipNameAddrList,record->numNameAddrs);
			tmpRR=response->recordroute;
			oldRRAddr=tmpRR->sipNameAddrList;
			while(oldRRAddr->next!=NULL)
				oldRRAddr=oldRRAddr->next;
			oldRRAddr->next=newRRAddr;
			response->recordroute->numNameAddrs+=record->numNameAddrs;
		}
	}
	return 1;
}

int ReferToRspDup(SipRsp rsp,SipReferTo* referto)
{
	if(rsp==NULL)
		return 0;
	if(referto==NULL)
		return 1;

	if(rsp->referto != NULL)
		ReferToFree(rsp->referto);
	rsp->referto=(SipReferTo*)calloc(1,sizeof(SipReferTo));
	rsp->referto->numAddr=referto->numAddr;
	rsp->referto->numParam=referto->numParam;
	rsp->referto->address=NULL;
	rsp->referto->paramList=NULL;
	if(referto->address != NULL)
		rsp->referto->address=sipAddrDup(referto->address,referto->numAddr);
	if(referto->paramList != NULL)
		rsp->referto->paramList=sipParameterDup(referto->paramList,referto->numParam);
	
	return 1;

}


int ReferredByRspDup(SipRsp rsp,SipReferredBy* referredby)
{
	if(rsp->referredby != NULL)
		ReferredByFree(rsp->referredby);
	rsp->referredby=(SipReferredBy*)calloc(1,sizeof(SipReferredBy));
	rsp->referredby->numAddr=referredby->numAddr;
	rsp->referredby->numParam=referredby->numParam;
	rsp->referredby->address=NULL;
	rsp->referredby->paramList=NULL;
	if(referredby->address != NULL)
		rsp->referredby->address=sipAddrDup(referredby->address,referredby->numAddr);
	if(referredby->paramList != NULL)
		rsp->referredby->paramList=sipParameterDup(referredby->paramList,referredby->numParam);
	return 1;

}

int RemotePartyIDRspDupLink(SipRsp rsp,SipRemotePartyID* remotepartyid)
{
	SipRemotePartyID *tmp,*newobj,*last;

	if(rsp==NULL)
		return 0;
	if(remotepartyid==NULL)
		return 0;

	tmp=remotepartyid;
	last=rsp->remotepartyid;
	
	newobj=(SipRemotePartyID*)calloc(1,sizeof(*newobj));
	
	if(last==NULL)
		rsp->remotepartyid=newobj;
	else{
		while(last!=NULL){
			if(last->next == NULL) break;
			last=last->next;
		}
		last->next=newobj;
	}

	while(tmp != NULL){
		
		newobj->addr=NULL;
		newobj->numParam=0;
		newobj->next=NULL;
		newobj->paramList=NULL;
		if(tmp->addr!=NULL)
			newobj->addr=sipAddrDup(tmp->addr,-1);
		if(tmp->paramList!=NULL){
			newobj->paramList=sipParameterDup(tmp->paramList,tmp->numParam);
			newobj->numParam=tmp->numParam;
		}	
		tmp=tmp->next;
		if(tmp){
			newobj->next=(SipRemotePartyID*)calloc(1,sizeof(*newobj));
			newobj=newobj->next;
		}
	}
		
	return 1;
}

int ReplacesRspDupLink(SipRsp rsp,SipReplaces* replaces)
{
	SipReplaces *lastreplaces,*tmp,*newreplaces;

	if(rsp==NULL)
		return 0;
	if(replaces==NULL)
		return 0;
	newreplaces=(SipReplaces*)calloc(1,sizeof(*newreplaces));
	if(rsp->replaces==NULL)
		rsp->replaces=newreplaces;
	else{
		lastreplaces=rsp->replaces;
		while(lastreplaces!=NULL){
			if(lastreplaces->next == NULL) break;
			lastreplaces=lastreplaces->next;
		}
		lastreplaces->next=newreplaces;
	}
	
	/* Duplicate replaces object */
	tmp=replaces;
	while(tmp!=NULL){
		
		newreplaces->callid=NULL;
		newreplaces->next=NULL;
		newreplaces->numParam=0;
		newreplaces->paramList=NULL;
		if(tmp->callid != NULL)
			newreplaces->callid=strDup(tmp->callid);
		newreplaces->numParam=tmp->numParam;
		if(tmp->paramList != NULL)
			newreplaces->paramList=sipParameterDup(tmp->paramList,tmp->numParam);
		tmp=tmp->next;
		if(tmp){
			newreplaces->next=(SipReplaces*)calloc(1,sizeof(*newreplaces));
			newreplaces=newreplaces->next;
		}
	}

	return 1;
}

/* ReplyToRspDup tyhuang*/
int ReplyToRspDup(SipRsp response,SipReplyTo* _replyto)
{
	if(response->replyto != NULL)
		ReplyToFree(response->replyto);
	response->replyto=(SipReplyTo*)calloc(1,sizeof(SipReplyTo));
	response->replyto->numParam=_replyto->numParam;
	response->replyto->numAddr=_replyto->numAddr;
	response->replyto->address=NULL;
	response->replyto->ParamList=NULL;
	if(_replyto->address != NULL)
		response->replyto->address=sipAddrDup(_replyto->address,_replyto->numAddr);
	if(_replyto->ParamList != NULL)
		response->replyto->ParamList=sipParameterDup(_replyto->ParamList,_replyto->numParam);
	
	return 1;
}

int RequireRspDup(SipRsp response,SipRequire* require)
{
	if(response->require != NULL)
		RequireFree(response->require);
	response->require=(SipRequire*)calloc(1,sizeof(SipRequire));
	response->require->numTags=require->numTags;
	response->require->sipOptionTagList=NULL;
	if(require->sipOptionTagList != NULL)
		response->require->sipOptionTagList=sipStrDup(require->sipOptionTagList,require->numTags);
	return 1;
}

int RetryAfterRspDup(SipRsp response,SipRtyAft* after)
{
	if(response->retryafter != NULL)
		RetryAfterFree(response->retryafter);
	response->retryafter=(SipRtyAft*)calloc(1,sizeof(SipRtyAft));
	response->retryafter->afterSecond=after->afterSecond;
	response->retryafter->afterDate=NULL;
	response->retryafter->comment=NULL;
	if(after->afterDate != NULL)
		response->retryafter->afterDate=rfc1123DateDup(after->afterDate);
	if(after->comment != NULL)
		response->retryafter->comment=strDup(after->comment);
	response->retryafter->duration=after->duration;
	return 1;
}

int SessionExpiresRspDup(SipRsp rsp,SipSessionExpires *sessionexpires)
{
	SipSessionExpires *newobj;

	if(rsp->sessionexpires != NULL)
		SessionExpiresFree(rsp->sessionexpires);
	if(sessionexpires != NULL){
		newobj=(SipSessionExpires*)calloc(1,sizeof(*newobj));
		newobj->deltasecond=sessionexpires->deltasecond;
		newobj->numParam=sessionexpires->numParam;
		newobj->paramList=sipParameterDup(sessionexpires->paramList,sessionexpires->numParam);
		rsp->sessionexpires=newobj;
	}
	return 1;
}

int ServerRspDup(SipRsp response,SipServer* server)
{
	if(response->server != NULL)
		ServerFree(response->server);
	response->server=(SipServer*)calloc(1,sizeof(SipServer));
	response->server->numServer=server->numServer;
	response->server->data=NULL;
	if(server->data != NULL)
		response->server->data=sipStrDup(server->data,server->numServer);
	return 1;
}

int SupportedRspDup(SipRsp response,SipSupported *support)
{
	if(response->supported != NULL)
		SupportedFree(response->supported);
	response->supported=(SipSupported*)calloc(1,sizeof(SipSupported));
	response->supported->numTags=support->numTags;
	response->supported->sipOptionTagList=NULL;
	if(support->sipOptionTagList != NULL)
		response->supported->sipOptionTagList=sipStrDup(support->sipOptionTagList,support->numTags);
	return 1;
}

int TimestampRspDup(SipRsp response, SipTimestamp* stamp)
{
	if(response->timestamp != NULL)
		TimestampFree(response->timestamp);
	response->timestamp=(SipTimestamp*)calloc(1,sizeof(SipTimestamp));
	response->timestamp->delay=NULL;
	response->timestamp->time=NULL;
	if(stamp->delay != NULL)
		response->timestamp->delay=strDup(stamp->delay);
	if(stamp->time != NULL)
		response->timestamp->time=strDup(stamp->time);
	return 1;
}

int ToRspDup(SipRsp response,SipTo* toList)
{
	if(response->to != NULL)
		ToFree(response->to);
	response->to=(SipTo*)calloc(1,sizeof(SipTo));
	response->to->numAddr=toList->numAddr;
	response->to->numParam=toList->numParam;
	response->to->address=NULL;
	response->to->paramList=NULL;
	if(toList->address != NULL)
		response->to->address=sipAddrDup(toList->address,toList->numAddr);
	if(toList->paramList != NULL)
		response->to->paramList=sipParameterDup(toList->paramList,toList->numParam);
	return 1;
}

int UnsupportedRspDup(SipRsp response,SipUnsupported *unsupport)
{
	if(response->unsupported != NULL)
		UnsupportedFree(response->unsupported);
	response->unsupported=(SipUnsupported*)calloc(1,sizeof(SipUnsupported));
	response->unsupported->numTags=unsupport->numTags;
	response->unsupported->sipOptionTagList=NULL;
	if(unsupport->sipOptionTagList != NULL)
		response->unsupported->sipOptionTagList=sipStrDup(unsupport->sipOptionTagList,unsupport->numTags);
	return 1;
}

int UserAgentRspDup(SipRsp response,SipUserAgent* agent)
{
	if(response->useragent != NULL)
		UserAgentFree(response->useragent);
	response->useragent=(SipUserAgent*)calloc(1,sizeof(SipUserAgent));
	response->useragent->numdata=agent->numdata;
	response->useragent->data=NULL;
	if(agent->data != NULL)
		response->useragent->data=sipStrDup(agent->data,agent->numdata);
	return 1;
}

int ViaRspDupLink(SipRsp response,SipVia* visList)
{
	SipViaParm *tmpViaPara,*firstViaPara,*lastViaPara;

	tmpViaPara=sipViaParmDup(visList->ParmList, visList->numParams);
	firstViaPara=tmpViaPara;
	lastViaPara=tmpViaPara;
	while(tmpViaPara->next != NULL){
		lastViaPara=tmpViaPara;
		tmpViaPara=tmpViaPara->next;
	}
	if (response->via != NULL){
		response->via->numParams=response->via->numParams+ visList->numParams;
		response->via->last->next=firstViaPara;
		response->via->last=lastViaPara;
	}else{
		response->via=(SipVia*)calloc(1,sizeof(SipVia));
		response->via->numParams = visList->numParams;
		response->via->ParmList=firstViaPara;
		response->via->last=lastViaPara;
	}
	return 1;
}

int WarningRspDupLink(SipRsp response,SipWarning* warning)
{
	SipWrnVal *tmpValue,*firstValue,*lastValue;

	tmpValue=sipWarningDup(warning->WarningValList,warning->numVal);
	firstValue=tmpValue;
	lastValue=tmpValue;
	while(tmpValue->next != NULL){
		lastValue=tmpValue;
		tmpValue=tmpValue->next;
	}
	if(response->warning != NULL){
		response->warning->numVal=response->warning->numVal + warning->numVal;
		response->warning->last->next=firstValue;
		response->warning->last=lastValue;
	}else{
		response->warning=(SipWarning*)calloc(1,sizeof(SipWarning));
		response->warning->numVal=warning->numVal;
		response->warning->WarningValList=firstValue;
		response->warning->last=lastValue;
	}
	return 1;

}

int WWWAuthenticateRspDupLink(SipRsp response,SipWWWAuthn *authen)
{
	SipChllng *origChllng,*newChllng;

	if(response->wwwauthenticate == NULL){
		response->wwwauthenticate=(SipWWWAuthn*)calloc(1,sizeof(SipWWWAuthn));
		response->wwwauthenticate->numChallenges=authen->numChallenges;
		response->wwwauthenticate->ChllngList=NULL;
		if(authen->ChllngList != NULL)
			response->wwwauthenticate->ChllngList=sipChallengeDup(authen->ChllngList,authen->numChallenges);
	}else {
		if(authen->ChllngList != NULL)
			newChllng=sipChallengeDup(authen->ChllngList,authen->numChallenges);
		response->wwwauthenticate->numChallenges+=authen->numChallenges;
		origChllng=response->wwwauthenticate->ChllngList;
		while(origChllng->next != NULL)
			origChllng=origChllng->next;
		origChllng->next=newChllng;
	}

	return 1;
}

#ifdef ICL_IMS
/* P-Media-Authorization */
int	PMARspDup(SipRsp response,unsigned char* str)
{
	if(response->pmediaauthorization != NULL)
		free(response->pmediaauthorization);
	response->pmediaauthorization=(unsigned char*)strDup((char*)str);
	return 1;
}
 
/* Path */
int	PathRspDupLink(SipRsp response,SipPath* path)
{
	SipPath *tmpR;
	SipRecAddr *newRAddr,*oldRAddr;
	if(response->path == NULL){
		response->path=(SipPath*)calloc(1,sizeof(SipPath));
		response->path->numNameAddrs=path->numNameAddrs;
		response->path->sipNameAddrList=NULL;
		if(path->sipNameAddrList != NULL)
			response->path->sipNameAddrList=sipRecAddrDup(path->sipNameAddrList,path->numNameAddrs);
	}else{
		if(path->sipNameAddrList != NULL){
			newRAddr=sipRecAddrDup(path->sipNameAddrList,path->numNameAddrs);
			tmpR=response->path;
			oldRAddr=tmpR->sipNameAddrList;
			while(oldRAddr->next!= NULL)
				oldRAddr=oldRAddr->next;
			oldRAddr->next=newRAddr;
			response->path->numNameAddrs+=path->numNameAddrs;
		}
	}
	return 1;
}

/* P-Access-Network-Info */
int	PANIRspDup(SipRsp response,unsigned char* str)
{
	if(response->paccessnetworkinfo != NULL)
		free(response->paccessnetworkinfo);
	response->paccessnetworkinfo=(unsigned char*)strDup((char*)str);
	return 1;
}

/* P-Associated-URI */
int	PAURspDup(SipRsp response,unsigned char* str)
{
	if(response->passociateduri != NULL)
		free(response->passociateduri);
	response->passociateduri=(unsigned char*)strDup((char*)str);
	return 1;
}

/* P-Charging-Function-Address */
int	PCFARspDup(SipRsp response,unsigned char* str)
{
	if(response->pchargingfunctionaddress != NULL)
		free(response->pchargingfunctionaddress);
	response->pchargingfunctionaddress=(unsigned char*)strDup((char*)str);
	return 1;
}

/* P-Charging-Vector */
int	PCVRspDup(SipRsp response,unsigned char* str)
{
	if(response->pchargingvector != NULL)
		free(response->pchargingvector);
	response->pchargingvector=(unsigned char*)strDup((char*)str);
	return 1;
}

/* Security-Server */
int	SecurityServerRspDup(SipRsp response,unsigned char* str)
{
	if(response->securityserver != NULL)
		free(response->securityserver);
	response->securityserver=(unsigned char*)strDup((char*)str);
	return 1;
}

/* Service-Route */
int	ServiceRouteRspDupLink(SipRsp response,SipServiceRoute* sr)
{
	SipPath *tmpR;
	SipRecAddr *newRAddr,*oldRAddr;
	if(response->serviceroute == NULL){
		response->serviceroute=(SipServiceRoute*)calloc(1,sizeof(SipServiceRoute));
		response->serviceroute->numNameAddrs=sr->numNameAddrs;
		response->serviceroute->sipNameAddrList=NULL;
		if(sr->sipNameAddrList != NULL)
			response->serviceroute->sipNameAddrList=sipRecAddrDup(sr->sipNameAddrList,sr->numNameAddrs);
	}else{
		if(sr->sipNameAddrList != NULL){
			newRAddr=sipRecAddrDup(sr->sipNameAddrList,sr->numNameAddrs);
			tmpR=response->serviceroute;
			oldRAddr=tmpR->sipNameAddrList;
			while(oldRAddr->next!= NULL)
				oldRAddr=oldRAddr->next;
			oldRAddr->next=newRAddr;
			response->serviceroute->numNameAddrs+=sr->numNameAddrs;
		}
	}
	return 1;
}

#endif /* end of ICL_IMS */

/************************************************************************/
CCLAPI 
RCODE sipRspDelViaHdrTop(IN SipRsp _this)
{
	RCODE rCode = RC_OK;
	SipViaParm *tmpViaPara;

	if(_this->via != NULL){
		if(_this->via->ParmList != NULL){
			_this->rspflag=TRUE;
			tmpViaPara=_this->via->ParmList;
			_this->via->ParmList=tmpViaPara->next;
			_this->via->numParams--;
			tmpViaPara->next=NULL;
			sipViaParmFree(tmpViaPara);
		}else
			rCode=RC_ERROR;
		if (0==_this->via->numParams) {
			free(_this->via);
			_this->via=NULL;
		}
	}else
		rCode = RC_ERROR;

	return rCode;
}

CCLAPI 
RCODE sipRspDelAllViaHdr(IN SipRsp _this)
{
	if (!_this || !_this->via ) return RC_ERROR;
	ViaFree(_this->via);
	_this->via=NULL;
	return RC_OK;

}

/* Add/delete Record Route header for request message */
/* pos = 0 : top; pos = 1:Last */
CCLAPI 
RCODE sipRspAddRecRouteHdr(IN SipRsp _this,int pos, char* _recroute)
{
	MsgHdr hdr,tmp;
	void *ret;
	char *tmpbuf;
	SipRecordRoute *newRecRoute;
	RCODE rCode=RC_OK;
	SipRecordRoute *origRecRoute;
	SipRecAddr *origAddr,*tmpAddr;

	_this->rspflag=TRUE;
	/* parse new Route header */
	newRecRoute=NULL;
	/*tmpbuf=(char*)malloc(strlen(_route)+2);*/
	tmpbuf=(char*)calloc(strlen(_recroute)+2,sizeof(char));
	strcpy(tmpbuf,_recroute);
	hdr=msgHdrNew();
	rfc822_parseline((unsigned char*)tmpbuf,strlen(_recroute),hdr);
	tmp=hdr;
	if((tmp!=NULL)&&((tmp->name)!=NULL)){
		SipHdrType hdrtype=sipHdrNameToType(tmp->name);
		if(hdrtype!=Record_Route){
			rCode=RC_ERROR;
		}
	
		if(RFC822ToSipHeader(tmp,hdrtype,&ret) != 1){
			rCode=RC_ERROR;
		}else{
			newRecRoute=(SipRecordRoute*)ret;
		}
	}else
		rCode = RC_ERROR;

	msgHdrFree(hdr);
	free(tmpbuf);

	/* Add into Route header */
	if(rCode != RC_ERROR){
		if(newRecRoute != NULL){
			if(_this->recordroute == NULL){
				/* top and last is same */
				_this->recordroute=newRecRoute;
			}else{ /* there exist a Record-Route header */
				origRecRoute=_this->recordroute;
				origAddr=origRecRoute->sipNameAddrList;
				origRecRoute->numNameAddrs=origRecRoute->numNameAddrs+newRecRoute->numNameAddrs;
				if(pos==0){ /*mean top */
					_this->recordroute->sipNameAddrList=newRecRoute->sipNameAddrList;
					tmpAddr=newRecRoute->sipNameAddrList;
					/* get the last Addr */
					while(tmpAddr->next != NULL){
						tmpAddr=tmpAddr->next;
					}
					tmpAddr->next=origAddr;

				}else if(pos==1){ /*mean last */
					tmpAddr=origRecRoute->sipNameAddrList;
					if(tmpAddr){
						while(tmpAddr->next!=NULL){
							tmpAddr=tmpAddr->next;
						}
					}
					tmpAddr->next=newRecRoute->sipNameAddrList;
				}else
					rCode=RC_ERROR;

				free(newRecRoute);
			}		
			/* free(newRoute);*/
		}else
			rCode=RC_ERROR;
	}
	return rCode;
}

CCLAPI 
RCODE sipRspDelHdr(IN SipRsp response,IN SipHdrType hdrtype)
{
	switch(hdrtype) {
	case Contact_Short:
	case Contact:
		sipContactFree(response->contact);
		response->contact = NULL;
		break;
	case Content_Length:
	case Content_Length_Short:
		response->contentlength=0;
		break;
	case Min_Expires:
		response->minexpires=-1;
		break;
	case Proxy_Authenticate:
		ProxyAuthenticateFree(response->proxyauthenticate);
		response->proxyauthenticate=NULL;
		break;
	case Record_Route:
		RecordRouteFree(response->recordroute);
		response->recordroute=NULL;
		break;
	case Remote_Party_ID:
		RemotePartyIDFree(response->remotepartyid);
		response->remotepartyid=NULL;
		break;
	case Replaces:
		ReplacesFree(response->replaces);
		response->replaces=NULL;
		break;
	case RSeq:
		response->rseq=-1;
		break;
	case Via_Short:
	case Via:
		ViaFree(response->via);
		response->via=NULL;
		break;
	case WWW_Authenticate:
		WWWAuthenticateFree(response->wwwauthenticate);
		response->wwwauthenticate=NULL;
		break;
#ifdef ICL_IMS 
	case Path:	
		PathFree(response->path);
		response->path = NULL;
		break;
	case Service_Route:
		ServiceRouteFree(response->serviceroute);
		response->serviceroute = NULL;
		break;
#endif /* end of ICL_IMS */
	case UnknSipHdr:
		free(response->unHdr);
		response->unHdr=NULL;
		break;

	default:
		sipRspAddHdrEx(response,hdrtype,NULL);
		break;
	}
	return RC_OK;	
}

/***********************************************************************
*
*	WWW Authenticate  ( Digest and Basic )
*
***********************************************************************/
int			
sipRspGetAuthnSize(SipRsp response,SipAuthnType type)
{
	/*int i = 0;*/
	if( !response )	return 0;
	if( type==SIP_WWW_AUTHN ) {
		if( !response->wwwauthenticate ) 
			return 0;
		return response->wwwauthenticate->numChallenges;
	}
	else if( type==SIP_PXY_AUTHN ) {
		if( !response->proxyauthenticate ) 
			return 0;
		return response->proxyauthenticate->numChllngs;
	}else
		return 0;
}


RCODE			
sipRspAddBasicAuthnAt(SipRsp response,SipAuthnType type,int position,char* realm)

{
	int		i = 0;
	SipChllng	*challenge;
	SipChllng	*iter;
	SipChllng	*temp;

	if( !response || !realm )	
		return RC_ERROR;
	if( type==SIP_WWW_AUTHN ) {
		if( !response->wwwauthenticate ) 
			response->wwwauthenticate = (SipWWWAuthn*)calloc(1,sizeof(SipWWWAuthn));
		if( !response->wwwauthenticate )	
			return RC_ERROR;
	}else if( type==SIP_PXY_AUTHN ) {
		if( !response->proxyauthenticate ) 
			response->proxyauthenticate = (SipPxyAuthn*)calloc(1,sizeof(SipPxyAuthn));
		if( !response->proxyauthenticate )	return RC_ERROR;
	}else 
		return RC_ERROR;


	challenge = (SipChllng*)calloc(1,sizeof(SipChllng));
	challenge->scheme = strDup("Basic");
	challenge->numParams = 1;
	challenge->sipParamList = (SipParam*)calloc(1,sizeof(SipParam));
	challenge->sipParamList->name = strDup("realm");
	challenge->sipParamList->value = strDup(realm);
	challenge->sipParamList->next = NULL;

	if( type==SIP_WWW_AUTHN ) 
		temp = iter = response->wwwauthenticate->ChllngList;
	else 
		temp = iter = response->proxyauthenticate->ChllngList;
	for(i=0; (iter)&&(i<position); i++) {
		temp = iter;
		iter = iter->next;
	}
	if( i!=0 ) {
		temp->next = challenge;
		challenge->next = iter;
	}
	else {
		challenge->next = iter;
		if( type==SIP_WWW_AUTHN ) 
			response->wwwauthenticate->ChllngList = challenge;
		else 
			response->proxyauthenticate->ChllngList = challenge;
	}

	return RC_OK;
}

RCODE			
sipRspGetBasicAuthnAt(SipRsp response,SipAuthnType type,int position,char* realm, int rlen)
{
	int		i = 0;
	SipChllng	*iter;

	if( !response || !realm )		return RC_ERROR;
	if( type==SIP_WWW_AUTHN ) {
		if( !response->wwwauthenticate )	return RC_ERROR;
		iter = response->wwwauthenticate->ChllngList;
	}
	else if( type==SIP_PXY_AUTHN ) {
		if( !response->proxyauthenticate )	return RC_ERROR;
		iter = response->proxyauthenticate->ChllngList;
	}
	else 
		return RC_ERROR;
	
	for( i=0; (iter)&&(i<position); i++)
		iter = iter->next;

	if( !iter->scheme || strICmp("Basic",(const char*)iter->scheme)!=0 )
		return RC_ERROR;
	if( iter->numParams<0 ) 		
		return RC_ERROR;
	if( !(iter->sipParamList) ) 	
		return RC_ERROR;
	if( rlen<=(int)strlen(iter->sipParamList->value) )
		return RC_ERROR;
	strcpy(realm,iter->sipParamList->value);

	return RC_OK;
}

RCODE			
sipRspAddDigestAuthnAt(SipRsp response,SipAuthnType type,int position,SipDigestWWWAuthn auth)
{
	int		i = 0;
	SipChllng	*challenge;
	SipChllng	*citer;
	SipChllng	*ctemp;
	SipParam	*piter;
	SipParam	*ptemp;
	char		*tmp;

	if( !response )	return RC_ERROR;
	if( type==SIP_WWW_AUTHN ) {
		if( !response->wwwauthenticate ) {
			response->wwwauthenticate = (SipWWWAuthn*)calloc(1,sizeof(SipWWWAuthn));
			response->wwwauthenticate->numChallenges = 0;
			response->wwwauthenticate->ChllngList = NULL;
		}
		if( !response->wwwauthenticate )	
			return RC_ERROR;
	}else if( type==SIP_PXY_AUTHN ) {
		if( !response->proxyauthenticate ) {
			response->proxyauthenticate = (SipPxyAuthn*)calloc(1,sizeof(SipPxyAuthn));
			response->proxyauthenticate->numChllngs = 0;
			response->proxyauthenticate->ChllngList = NULL;
		}
		if( !response->proxyauthenticate )	
			return RC_ERROR;
	}else 
		return RC_ERROR;

	challenge = (SipChllng*)calloc(1,sizeof(SipChllng));
	challenge->scheme = strDup("Digest");
	challenge->numParams = 0;

	challenge->sipParamList = piter = (SipParam*)calloc(1,sizeof(SipParam));
	piter->name = strDup("realm");
	/*piter->value = strDup(quote(auth.realm_));modified by tyhuang 2003.09.23 */
	tmp=strDup(auth.realm_);
	tmp=realloc(tmp,strlen(tmp)+3);
	piter->value=quote(tmp);
	piter->next = NULL;
	challenge->numParams++;

	if( auth.flags_&SIP_DWAFLAG_DOMAIN ) {
		ptemp = (SipParam*)calloc(1,sizeof(SipParam));
		ptemp->name = strDup("domain");
		/*ptemp->value = strDup(quote(auth.domain_));modified by tyhuang 2003.09.23 */
		tmp=strDup(auth.domain_);
		tmp=realloc(tmp,strlen(tmp)+3);
		ptemp->value=quote(tmp);
		ptemp->next = NULL;
		piter->next = ptemp;
		piter = ptemp;
		challenge->numParams++;
	}
	if( auth.flags_&SIP_DWAFLAG_NONCE ) {
		ptemp = (SipParam*)calloc(1,sizeof(SipParam));
		ptemp->name = strDup("nonce");
		/*ptemp->value = strDup(quote(auth.nonce_));modified by tyhuang 2003.09.23 */
		tmp=strDup(auth.nonce_);
		tmp=realloc(tmp,strlen(tmp)+3);
		ptemp->value=quote(tmp);

		ptemp->next = NULL;
		piter->next = ptemp;
		piter = ptemp;
		challenge->numParams++;
	}
	if( auth.flags_&SIP_DWAFLAG_OPAQUE ) {
		ptemp = (SipParam*)calloc(1,sizeof(SipParam));
		ptemp->name = strDup("opaque");
		/*ptemp->value = strDup(quote(auth.opaque_));modified by tyhuang 2003.09.23 */
		tmp=strDup(auth.opaque_);
		tmp=realloc(tmp,strlen(tmp)+3);
		ptemp->value=quote(tmp);

		ptemp->next = NULL;
		piter->next = ptemp;
		piter = ptemp;
		challenge->numParams++;
	}
	if( auth.flags_&SIP_DWAFLAG_STALE ) {
		ptemp = (SipParam*)calloc(1,sizeof(SipParam));
		ptemp->name = strDup("stale");
		ptemp->value = (auth.stale_==TRUE)?strDup("true"):strDup("false");
		ptemp->next = NULL;
		piter->next = ptemp;
		piter = ptemp;
		challenge->numParams++;
	}
	if( auth.flags_&SIP_DWAFLAG_ALGORITHM ) {
		ptemp = (SipParam*)calloc(1,sizeof(SipParam));
		ptemp->name = strDup("algorithm");
		ptemp->value = strDup(auth.algorithm_);
		ptemp->next = NULL;
		piter->next = ptemp;
		piter = ptemp;
		challenge->numParams++;
	}
	if( auth.flags_&SIP_DWAFLAG_QOP ) {
		ptemp = (SipParam*)calloc(1,sizeof(SipParam));
		ptemp->name = strDup("qop");
		/*ptemp->value = strDup(quote(auth.qop_));modified by tyhuang 2003.09.23 */
		tmp=strDup(auth.qop_);
		tmp=realloc(tmp,strlen(tmp)+3);
		ptemp->value=quote(tmp);
		ptemp->value=tmp;
		ptemp->next = NULL;
		piter->next = ptemp;
		piter = ptemp;
		challenge->numParams++;
	}

	if( type==SIP_WWW_AUTHN ) 
		ctemp = citer = response->wwwauthenticate->ChllngList;
	else
		ctemp = citer = response->proxyauthenticate->ChllngList;
	for(i=0; (citer)&&(i<position); i++) {
		ctemp = citer;
		citer = citer->next;
	}
	if( i!=0 ) {
		ctemp->next = challenge;
		challenge->next = citer;
		if( type==SIP_WWW_AUTHN ) 
			response->wwwauthenticate->numChallenges++;
		else
			response->proxyauthenticate->numChllngs++;
	}else {
		challenge->next = citer;
		if( type==SIP_WWW_AUTHN ) {
			response->wwwauthenticate->ChllngList = challenge;
			response->wwwauthenticate->numChallenges++;
		}else {
			response->proxyauthenticate->ChllngList = challenge;
			response->proxyauthenticate->numChllngs++;
		}
	}	
	
	return RC_OK;
}

RCODE			
sipRspGetDigestAuthnAt(SipRsp response,SipAuthnType type,int position,SipDigestWWWAuthn* auth)
{
	int		i = 0;
	SipChllng	*citer;
	SipParam	*piter;

	if( !response || !auth )		
		return RC_ERROR;
	if( type==SIP_WWW_AUTHN ) {
		if( !response->wwwauthenticate )	
			return RC_ERROR;
		citer = response->wwwauthenticate->ChllngList;
	}
	else if( type==SIP_PXY_AUTHN ) {
		if( !response->proxyauthenticate )	
			return RC_ERROR;
		citer = response->proxyauthenticate->ChllngList;
	}
	else
		return RC_ERROR;

	for( i=0; (citer)&&(i<position); i++)
		citer = citer->next;

	if( !citer->scheme || strICmp("Digest",(const char*)citer->scheme)!=0 )
		return RC_ERROR;
	if( citer->numParams<0 ) 		return RC_ERROR;
	if( !(citer->sipParamList) ) 	return RC_ERROR;

	auth->flags_ = 0;
	piter = citer->sipParamList;
	for( i=0; (i<citer->numParams)&&piter; i++, piter=piter->next) {
		if( strcmp(piter->name,"realm")==0 ) {
			strcpy(auth->realm_,unquote(piter->value));
			continue;
		}
		if( strcmp(piter->name,"domain")==0 ) {
			strcpy(auth->domain_,unquote(piter->value));
			auth->flags_ |= SIP_DWAFLAG_DOMAIN;
			continue;
		}
		if( strcmp(piter->name,"nonce")==0 ) {
			strcpy(auth->nonce_,unquote(piter->value));
			auth->flags_ |= SIP_DWAFLAG_NONCE;
			continue;
		}		
		if( strcmp(piter->name,"opaque")==0 ) {
			strcpy(auth->opaque_,unquote(piter->value));
			auth->flags_ |= SIP_DWAFLAG_OPAQUE;
			continue;
		}		
		if( strcmp(piter->name,"stale")==0 ) {
			auth->stale_ = (strcmp(piter->value,"true")==0)?TRUE:FALSE;
			auth->flags_ |= SIP_DWAFLAG_STALE;
			continue;
		}		
		if( strcmp(piter->name,"algorithm")==0 ) {
			strcpy(auth->algorithm_,piter->value);
			auth->flags_ |= SIP_DWAFLAG_ALGORITHM;
			continue;
		}		
		if( strcmp(piter->name,"qop")==0 ) {
			strcpy(auth->qop_,unquote(piter->value));
			auth->flags_ |= SIP_DWAFLAG_QOP;
			continue;
		}
	}

	return RC_OK;
}

RCODE			
sipRspAddPGPWWWAuthnAt(void)
{
	return RC_OK;
}

RCODE
sipRspGetPGPWWWAuthnAt(void)
{
	return RC_OK;
}
/*--------------------------------------------*/
/* Since Authenticate API's Interface changed */
/* Acer Modified it May 2001				  */
/*--------------------------------------------*/
CCLAPI RCODE			
sipRspAddWWWAuthn(SipRsp response,SipAuthnStruct auth)
{
	RCODE ret=RC_ERROR;
	switch(auth.iType){
	case SIP_BASIC_CHALLENGE:
			ret=sipRspAddBasicAuthnAt(response,SIP_WWW_AUTHN,0,auth.auth_.basic_.realm_);
		break;
	case SIP_DIGEST_CHALLENGE:
			ret=sipRspAddDigestAuthnAt(response,SIP_WWW_AUTHN,0,auth.auth_.digest_);
		break;
	}
	return ret;
}

CCLAPI RCODE
sipRspAddPxyAuthn(SipRsp response,SipAuthnStruct auth)
{
	RCODE ret=RC_ERROR;
	switch(auth.iType){
	case SIP_BASIC_CHALLENGE:
			ret=sipRspAddBasicAuthnAt(response,SIP_PXY_AUTHN,0,auth.auth_.basic_.realm_);
		break;
	case SIP_DIGEST_CHALLENGE:
			ret=sipRspAddDigestAuthnAt(response,SIP_PXY_AUTHN,0,auth.auth_.digest_);
		break;
	}
	return ret;
}

/* Get WWW_Authenticate & Proxy_Authenticate */
CCLAPI RCODE 
sipRspGetWWWAuthn(SipRsp response,SipAuthnStruct *Auth)
{
	RCODE  ret=RC_ERROR;
	switch(Auth->iType)
	{
	case SIP_BASIC_CHALLENGE:
			ret=sipRspGetBasicAuthnAt(response,SIP_WWW_AUTHN,0,Auth->auth_.basic_.realm_,128);
		break;
	case SIP_DIGEST_CHALLENGE:
			/*ret=sipRspAddDigestAuthnAt(response,SIP_WWW_AUTHN,0,Auth->auth_.digest_);*/
			ret=sipRspGetDigestAuthnAt(response,SIP_WWW_AUTHN,0,&(Auth->auth_.digest_));
		break;
	}
	return ret;
}

CCLAPI RCODE
sipRspGetPxyAuthn(SipRsp response,SipAuthnStruct *Auth)
{
	RCODE  ret=RC_ERROR;
	switch(Auth->iType)
	{
	case SIP_BASIC_CHALLENGE:
			ret=sipRspGetBasicAuthnAt(response,SIP_PXY_AUTHN,0,Auth->auth_.basic_.realm_,128);
		break;
	case SIP_DIGEST_CHALLENGE:
			ret=sipRspGetDigestAuthnAt(response,SIP_PXY_AUTHN,0,&(Auth->auth_.digest_));
		break;
	}
	return ret;
}

