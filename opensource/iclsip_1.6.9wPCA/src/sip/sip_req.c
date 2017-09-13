/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * sip_req.c
 *
 * $Id: sip_req.c,v 1.128 2006/11/02 06:33:13 tyhuang Exp $
 */

#include <stdio.h>
#include <string.h>
#include "sip_req.h"
#include "base64.h"
#include "md5_g.h"
#include "md5.h"
#include "sip_hdr.h"
#include "sip_int.h"
#include <common/cm_utl.h>
#include <common/cm_trace.h>
#include <adt/dx_str.h>
#include <adt/dx_lst.h>

struct sipReqObj {
	SipReqLine		*requestline;

	SipAccept		*accept;  
	SipAcceptEncoding	*acceptencoding;
	SipAcceptLang		*acceptlanguage;
	SipInfo			*alertinfo;	/* add by tyhuang */
	SipAllow		*allow;
	SipAllowEvt		*allowevents;
	SipAuthz		*authorization;
	unsigned char	*callid;
	SipInfo			*callinfo;	/* add by tyhuang */
	SipClass		*classpublish;  /* add by tyhuang */
	SipContact		*contact;
	SipContentDisposition	*contentdisposition;   /* add by tyhuang */
	SipContEncoding		*contentencoding;
	SipContentLanguage	*contentlanguage;	/* add by tyhuang */
	int				contentlength;
	SipContType		*contenttype;
	SipCSeq			*cseq;
	SipDate			*date;
	SipEncrypt		*encryption;
	SipEvent		*event;
	SipExpires		*expires;
	SipFrom			*from;
	SipHideType		hide;
	SipInReplyTo	*inreplyto;
	int				maxforwards;
	unsigned char	*mimeversion;
	SipMinSE		*minse;
	unsigned char	*organization;
	SipIdentity		*passertedidentity;
	SipIdentity		*ppreferredidentity;
	SipPriorityType	priority;
	SipPrivacy		*privacy;
	SipPxyAuthz		*proxyauthorization;
	SipPxyRequire	*proxyrequire;
	SipRAck			*rack;
	SipRecordRoute	*recordroute;
	SipReferTo		*referto;
	SipReferredBy	*referredby;
	SipRemotePartyID	*remotepartyid;
	SipReplaces		*replaces;
	SipReplyTo		*replyto;
	SipRequire		*require;
	SipRspKey		*responsekey;
	SipRtyAft		*retryafter;
	SipRoute		*route;
	SipSessionExpires	*sessionexpires;
	unsigned char	*sipifmatch;
	unsigned char	*subject;
	SipSubState		*subscriptionstate;
	SipSupported	*supported;
	SipTimestamp		*timestamp;
	SipTo			*to;
	SipUserAgent	*useragent;
	SipVia			*via;
	SipBody			*body;
	char			*unHdr;
	BOOL			reqflag;
	char			*reqbuf;

	MsgHdr			hdrlist;
	void*			tx;

#ifdef ICL_IMS 
	unsigned char	*pmediaauthorization;
	SipPath			*path;
	unsigned char	*paccessnetworkinfo;
	unsigned char	*pcalledpartyid;
	unsigned char	*pchargingfunctionaddress;
	unsigned char	*pchargingvector;
	unsigned char	*pvisitednetworkid;
	unsigned char	*securityclient;
	unsigned char	*securityverify;
#endif /* end of ICL_IMS */
};

typedef enum {
	SIP_AUTHZ		= 0x1,
	SIP_PROXY_AUTHZ		= 0x2
} SipAuthzType;

int sipReqAsStr(SipReq, DxStr, int *);
int sipReqParse(SipReq, unsigned char*, int*);
int CountReqLineLen(SipReq);

RCODE	sipReqGetDigestAuthz(SipReq,SipAuthzType,SipDigestAuthz*);
RCODE	sipReqSetDigestAuthz(SipReq,SipAuthzType,SipDigestAuthz);
RCODE	sipReqGetBasicAuthz(SipReq,SipAuthzType,char*, int,char*, int);
RCODE	sipReqSetBasicAuthz(SipReq request,SipAuthzType type,char* user_id,char* password);

CCLAPI
SipReq sipReqNew(void)
{
	SipReq _this;
	
	/*_this = (SipReq)malloc(sizeof *_this);*/

	_this = (SipReq)calloc(1,sizeof(*_this));
	if(!_this){
		TCRPrint(1,"<sipReqDup> memory alloc failed !\n");
		return _this;
	}
	_this->maxforwards = -1; /* represent no this header */
	_this->hide = unknown_hide;
	_this->priority = Unkn_priority;

	return _this;
}

CCLAPI
SipReq sipReqDup(SipReq _source)
{
	SipReq _this=NULL;
	
	
	if(_source){
		_this=sipReqNew();
		if(!_this){
			TCRPrint(1,"<sipReqDup> memory alloc failed !\n");
			return _this;
		}
		
		if(_source->requestline)
			sipReqSetReqLine(_this,_source->requestline);
		
		if(_source->accept)
			AcceptReqDup(_this,_source->accept);
		
		if(_source->acceptencoding)
			AcceptEncodingReqDup(_this,_source->acceptencoding);
	
		if(_source->acceptlanguage)
			AcceptLanguageReqDup(_this,_source->acceptlanguage);
	
		if(_source->alertinfo)
			AlertInfoReqDup(_this,_source->alertinfo);
	
		if(_source->allow)
			AllowReqDup(_this,_source->allow);

		if(_source->allowevents)
			AllowEventsReqDup(_this,_source->allowevents);
	
		if(_source->authorization)
			AuthorizationReqDupLink(_this,_source->authorization);
	
		if(_source->callid)
			CallIDReqDup(_this,_source->callid);
		
		if(_source->callinfo)
			CallInfoReqDup(_this,_source->callinfo);
		
		if(_source->classpublish)
			ClassReqDup(_this,_source->classpublish);
		
		if(_source->contact)
			ContactReqDupLink(_this,_source->contact);
		
		if(_source->contentdisposition)
			ContentDispositionReqDup(_this,_source->contentdisposition);

		if(_source->contentencoding)
			ContentEncodingReqDup(_this,_source->contentencoding);
	
		if(_source->contentlanguage)
			ContentLanguageReqDup(_this,_source->contentlanguage);

			_this->contentlength = _source->contentlength;

		if(_source->contenttype)
			ContentTypeReqDup(_this,_source->contenttype);
	
		if(_source->cseq)
			CSeqReqDup(_this,_source->cseq);
		
		if(_source->date)
			DateReqDup(_this,_source->date);
		
		if(_source->encryption)
			EncryptionReqDup(_this,_source->encryption);

		if(_source->event)
			EventReqDup(_this,_source->event);
		
		if(_source->expires)
			ExpiresReqDup(_this,_source->expires);
			

		if(_source->from)
			FromReqDup(_this,_source->from);
			
		_this->hide =_source->hide; 
			
		if(_source->inreplyto)
			InReplyToReqDup(_this,_source->inreplyto);
			
		_this->maxforwards =_source->maxforwards;
			
		if(_source->mimeversion)
			MIMEVersionReqDup(_this,_source->mimeversion);
			
		if(_source->minse)
			MinSEReqDup(_this,_source->minse);
		
		if(_source->organization)
			OrganizationReqDup(_this,(const char*)_source->organization);
			
		if(_source->passertedidentity)
			PAssertedIdentityReqDupLink(_this,_source->passertedidentity);

		if(_source->ppreferredidentity)
			PPreferredIdentityReqDupLink(_this,_source->ppreferredidentity);
		
		_this->priority = _source->priority; /*sipPriorityType is a enum */
			
		if(_source->privacy)
			PrivacyReqDup(_this,_source->privacy);
		
		if(_source->proxyauthorization)
			ProxyAuthorizationReqDupLink(_this,_source->proxyauthorization);
			
		if(_source->proxyrequire)
			ProxyRequireReqDup(_this,_source->proxyrequire);

		if(_source->rack)
			RAckReqDup(_this,_source->rack);
			
		if(_source->recordroute)
			RecordRouteReqDupLink(_this,_source->recordroute);
			
		if(_source->referto)
			ReferToReqDup(_this,_source->referto);
			
		if(_source->referredby)
			ReferredByReqDup(_this,_source->referredby);
			
		if(_source->remotepartyid)
			RemotePartyIDReqDupLink(_this,_source->remotepartyid);
			
		if(_source->replaces)
			ReplacesReqDupLink(_this,_source->replaces);
			
		if(_source->replyto)
			ReplyToReqDup(_this,_source->replyto);
			
		if(_source->require)
			RequireReqDup(_this,_source->require);
			
		if(_source->responsekey)
			ResponseKeyReqDup(_this,_source->responsekey);
			
		if(_source->retryafter)
			RetryAfterReqDup(_this,_source->retryafter);
			
		if(_source->route)
			RouteReqDupLink(_this,_source->route);
			
		if(_source->sessionexpires)
			SessionExpiresReqDup(_this,_source->sessionexpires);
		
		if(_source->sipifmatch)
			_this->sipifmatch=(unsigned char*)strDup((const char *)_source->sipifmatch);

		if(_source->subject)
			_this->subject = (unsigned char*)strDup((const char *)_source->subject);

		if(_source->subscriptionstate)
			SubscriptionStateReqDup(_this,_source->subscriptionstate);

		if(_source->supported)
			SupportedReqDup(_this,_source->supported);

		if(_source->timestamp)
			TimestampReqDup(_this,_source->timestamp);

		if(_source->to)
			ToReqDup(_this,_source->to);

		if(_source->useragent)
			UserAgentReqDup(_this,_source->useragent);

		if(_source->via)  
			ViaReqDupLink(_this,_source->via);

		if(_source->unHdr)
			_this->unHdr=strDup(_source->unHdr);

		if(_source->body)
			sipReqSetBody(_this,_source->body);

		_this->reqflag= TRUE;

	/*	_this->hdrlist=msgHdrDup(_source->hdrlist);
	*/	
		
#ifdef ICL_IMS
		if(_source->pmediaauthorization)
			PMAReqDup(_this,_source->pmediaauthorization);

		if(_source->path)
			PathReqDupLink(_this,_source->path);

		if(_source->paccessnetworkinfo)
			PANIReqDup(_this,_source->paccessnetworkinfo);

		if(_source->pcalledpartyid)
			PCPIReqDup(_this,_source->pcalledpartyid);

		if(_source->pchargingfunctionaddress)
			PCFAReqDup(_this,_source->pchargingfunctionaddress);

		if(_source->pvisitednetworkid)
			PVNIReqDup(_this,_source->pvisitednetworkid);

		if(_source->securityclient)
			SecurityClientReqDup(_this,_source->securityclient);

		if(_source->securityverify)
			SecurityVerifyReqDup(_this,_source->securityverify);

#endif /* end of ICL_IMS */	
		
	}else
		TCRPrint(1,"<sipReqDup> Request not exist !\n");

	return _this;
}

CCLAPI
void sipReqFree(SipReq request)
{
	if( !request ){
		TCRPrint(1,"<sipReqFree> Request not exist !\n");
		return;
	}

	sipReqLineFree(request->requestline);
	AcceptFree(request->accept);
	AcceptEncodingFree(request->acceptencoding);
	AcceptLanguageFree(request->acceptlanguage);
	AlertInfoFree(request->alertinfo);
	AllowFree(request->allow);
	AllowEventsFree(request->allowevents);
	AuthorizationFree(request->authorization);
	CallIDFree(request->callid);
	CallInfoFree(request->callinfo);
	ClassFree(request->classpublish);
	sipContactFree(request->contact);
	ContentDispositionFree(request->contentdisposition);
	ContentEncodingFree(request->contentencoding);
	ContentLanguageFree(request->contentlanguage);
	ContentTypeFree(request->contenttype);
	CSeqFree(request->cseq);
	DateFree(request->date);
	EncryptionFree(request->encryption);
	EventFree(request->event);
	ExpiresFree(request->expires);
	FromFree(request->from);
	InReplyToFree(request->inreplyto);
	MIMEVersionFree(request->mimeversion);
	MinSEFree(request->minse);
	OrganizationFree(request->organization);
	PAssertedIdentityFree(request->passertedidentity);
	PPreferredIdentityFree(request->ppreferredidentity);
	PrivacyFree(request->privacy);
	ProxyAuthorizationFree(request->proxyauthorization);
	ProxyRequireFree(request->proxyrequire);
	RAckFree(request->rack);
	RecordRouteFree(request->recordroute);
	ReferToFree(request->referto);
	ReferredByFree(request->referredby);
	RemotePartyIDFree(request->remotepartyid);
	ReplacesFree(request->replaces);
	ReplyToFree(request->replyto);
	RequireFree(request->require);
	ResponseKeyFree(request->responsekey);
	RetryAfterFree(request->retryafter);
	RouteFree(request->route);
	SessionExpiresFree(request->sessionexpires);
	SIPIfMatchFree(request->sipifmatch);
	SubjectFree(request->subject);
	SubscriptionStateFree(request->subscriptionstate);
	SupportedFree(request->supported);
	TimestampFree(request->timestamp);
	ToFree(request->to);
	UserAgentFree(request->useragent);
	ViaFree(request->via);
	sipBodyFree(request->body);

	if(request->reqbuf != NULL)
		free(request->reqbuf);
		
	if(request->unHdr != NULL)
		free(request->unHdr);

	/* add hdrlist 2004/11/22 */
	if (request->hdrlist) 
		msgHdrFree(request->hdrlist);

#ifdef ICL_IMS 
	PathFree(request->path);

	if(request->pmediaauthorization) free(request->pmediaauthorization);
	if(request->paccessnetworkinfo) free(request->paccessnetworkinfo);
	if(request->pcalledpartyid) free(request->pcalledpartyid);
	if(request->pchargingfunctionaddress) free(request->pchargingfunctionaddress);
	if(request->pvisitednetworkid) free(request->pvisitednetworkid);
	if(request->securityclient) free(request->securityclient);
	if(request->securityverify) free(request->securityverify);
#endif /* end of ICL_IMS */	

	memset(request,0,sizeof(struct sipReqObj));
	free(request);
}

CCLAPI
RCODE sipReqSetReqLine(SipReq request,SipReqLine *line)
{
	if(!request || !line)
		return RC_ERROR;
	/* setup the Request-Line */
	request->reqflag=TRUE;
	if(request->requestline == NULL){
		request->requestline=(SipReqLine*)calloc(1,sizeof(*line));
		request->requestline->ptrRequestURI=NULL;
		request->requestline->ptrSipVersion=NULL;
	}
	request->requestline->iMethod = line->iMethod;
	if(request->requestline->ptrRequestURI != NULL){
		free(request->requestline->ptrRequestURI);
		request->requestline->ptrRequestURI=NULL;
	}
	request->requestline->ptrRequestURI = strDup(line->ptrRequestURI);
	if(request->requestline->ptrSipVersion != NULL){
		free(request->requestline->ptrSipVersion);
		request->requestline->ptrSipVersion=NULL;
	}
	request->requestline->ptrSipVersion = strDup(line->ptrSipVersion);

	return RC_OK;
}

CCLAPI
SipReqLine* sipReqGetReqLine(SipReq request)
{
	if(request != NULL)
		return request->requestline;
	else {
		TCRPrint(1,"<sipReqGetReqLine> request line not exist !\n");
		return NULL;
	}
}
 
CCLAPI
RCODE sipReqAddHdr(SipReq request, SipHdrType headertype, void* str)
{

   if((request == NULL)|| (NULL ==str))
	   return RC_ERROR;
   
   request->reqflag=TRUE;
   switch(headertype){
	case Accept:
		AcceptReqDup(request,(SipAccept*)str);
		break;
	case Accept_Encoding:
		AcceptEncodingReqDup(request,(SipAcceptEncoding*)str);
		break;
	case Accept_Language:
		AcceptLanguageReqDup(request,(SipAcceptLang *) str);
		break;
	case Alert_Info:
		AlertInfoReqDup(request,(SipInfo*)str);
		break;
	case Allow:
		AllowReqDup(request,(SipAllow*)str);
		break;
	case Allow_Events_Short:
	case Allow_Events:
		AllowEventsReqDup(request,(SipAllowEvt*)str);
		break;
	case Authorization:
		AuthorizationReqDupLink(request,(SipAuthz*)str);
		/*AuthorizationFree((SipAuthz*)str);*/
		break;
	case Call_ID_Short:
	case Call_ID:
		CallIDReqDup(request,(unsigned char*)str);
		break;
	case Call_Info:
		CallInfoReqDup(request,(SipInfo*)str);
		break;
	case Class:
		ClassReqDup(request,(SipClass*)str);
		break;
	case Contact_Short:
	case Contact:
		ContactReqDupLink(request,(SipContact*)str);
		/*sipContactFree((SipContact*)str);*/
		break;
	case Content_Disposition:
		ContentDispositionReqDup(request,(SipContentDisposition*)str);
		break;
	case Content_Encoding_Short:
	case Content_Encoding:
		ContentEncodingReqDup(request,(SipContEncoding*)str);
		break;
	case Content_Language:
		ContentLanguageReqDup(request,(SipContentLanguage*)str);
		break;
	case Content_Length_Short:
	case Content_Length:
		request->contentlength = *(int *)str;
		break;
	case Content_Type_Short:
	case Content_Type:
		ContentTypeReqDup(request,(SipContType*)str);
		break;
	case CSeq:
		CSeqReqDup(request,(SipCSeq*)str);
		break;
	case Date:
		DateReqDup(request,(SipDate*)str);
		break;
	case Encryption:
		EncryptionReqDup(request,(SipEncrypt*)str);
		break;
	case Event:
	case Event_Short:
		EventReqDup(request,(SipEvent*)str);
		break;
	case Expires:
		ExpiresReqDup(request,(SipExpires*)str);
		break;
	case From_Short:
	case From:
		FromReqDup(request,(SipFrom*)str);
		break;
	case Hide:
		request->hide = (SipHideType)((int)str); /*sipHideType is a enum*/
		break;
	case In_Reply_To:
		InReplyToReqDup(request,(SipInReplyTo*)str);
		break;
	case Max_Forwards:
		request->maxforwards = *(int *)str;
		break;
	case MIME_Version:
		MIMEVersionReqDup(request, (unsigned char*)str);
		break;
	case Min_SE:
		MinSEReqDup(request,(SipMinSE*)str);
		break;
	case Organization:
		OrganizationReqDup(request,(const char*)str);
		break;
	case P_Asserted_Identity:
		PAssertedIdentityReqDupLink(request,(SipIdentity*)str);
		break;
	case P_Preferred_Identity:	
		PPreferredIdentityReqDupLink(request,(SipIdentity*)str);
		break;
	case Priority:
		request->priority = (SipPriorityType)((int)str); /*sipPriorityType is a enum */
		break;
	case Privacy:	
		PrivacyReqDup(request,(SipPrivacy*)str);
		break;
	case Proxy_Authorization:
		ProxyAuthorizationReqDupLink(request,(SipPxyAuthz*)str);
		break;
	case Proxy_Require:
		ProxyRequireReqDup(request,(SipPxyRequire*)str);
		break;
	case RAck:
		RAckReqDup(request,(SipRAck*)str);
		break;
	case Record_Route:
		RecordRouteReqDupLink(request,(SipRecordRoute*)str);
		break;
	case Refer_To:
		ReferToReqDup(request,(SipReferTo*)str);
		break;
	case Referred_By:
		ReferredByReqDup(request,(SipReferredBy*)str);
		break;
	case Remote_Party_ID:
		RemotePartyIDReqDupLink(request,(SipRemotePartyID*)str);
		break;
	case Replaces:
		ReplacesReqDupLink(request,(SipReplaces*)str);
		break;
	case Reply_To:
		ReplyToReqDup(request,(SipReplyTo*)str);
		break;
	case Require:
		RequireReqDup(request,(SipRequire*)str);
		break;
	case Response_Key:
		ResponseKeyReqDup(request,(SipRspKey*)str);
		break;
	case Retry_After:
		RetryAfterReqDup(request,(SipRtyAft*)str);
		break;
	case Route:
		RouteReqDupLink(request,(SipRoute*)str);
		/*RouteFree((SipRoute*)str);*/
		break;
	case Session_Expires:
	case Session_Expires_Short:
		SessionExpiresReqDup(request,(SipSessionExpires*)str);
		break;
	case SIP_If_Match:
		request->sipifmatch=(unsigned char*) strDup((const char *)str);
		break;
	case Subject_Short:
	case Subject:
		request->subject =(unsigned char*) strDup((const char *)str);
		break;
	case Subscription_State:
		SubscriptionStateReqDup(request,(SipSubState*)str);
		break;
	case Supported:
	case Supported_Short:
		SupportedReqDup(request,(SipSupported*)str);
		break;
	case Timestamp:
		TimestampReqDup(request,(SipTimestamp*)str);
		break;
	case To_Short:
	case To:
		ToReqDup(request,(SipTo*)str);
		break;
	case User_Agent:
		UserAgentReqDup(request,(SipUserAgent*)str);
		break;
	case Via_Short:
	case Via:  /* In this case, link the new sipVia into request */
		ViaReqDupLink(request,(SipVia*)str);
		/*ViaFree((SipVia*)str);*/
		break;

#ifdef ICL_IMS
	case P_Media_Authorization:
		PMAReqDup(request,(unsigned char*)str);
		break;
	case Path:
		PathReqDupLink(request,(SipPath *)str);
		break;
	case P_Access_Network_Info:
		PANIReqDup(request,(unsigned char *)str);
		break;
	case P_Called_Party_ID:
		PCPIReqDup(request,(unsigned char *)str);
		break;
	case P_Charging_Function_Address:
		PCFAReqDup(request,(unsigned char *)str);
		break;
	case P_Visited_Network_ID:
		PVNIReqDup(request,(unsigned char *)str);
		break;
	case Security_Client:
		SecurityClientReqDup(request,(unsigned char *)str);
		break;
	case Security_Verify:
		SecurityVerifyReqDup(request,(unsigned char *)str);
		break;
#endif /* end of ICL_IMS */	

	default:
		TCRPrint(1,"<sipReqAddHdr> Header not supported\n");

		return 0;
    }
    return RC_OK;
}


CCLAPI
RCODE sipReqAddHdrEx(SipReq request, SipHdrType headertype, void* ret)
{

   if(request == NULL)
	   return RC_ERROR;
   
   request->reqflag=TRUE;
   switch(headertype){
		case Accept:
			if(request->accept != NULL)
				AcceptFree(request->accept);
			request->accept=(SipAccept*)ret;
			break;
		case Accept_Encoding:
			if(request->acceptencoding != NULL)
				AcceptEncodingFree(request->acceptencoding);
			request->acceptencoding=(SipAcceptEncoding*)ret;
			break;
		case Accept_Language:
			if(request->acceptlanguage != NULL)
				AcceptLanguageFree(request->acceptlanguage);
			request->acceptlanguage=(SipAcceptLang*)ret;
			break;
		case Alert_Info:
			if(request->alertinfo != NULL)
				AlertInfoFree(request->alertinfo);
			request->alertinfo=(SipInfo*)ret;
			break;
		case Allow:
			if(request->allow != NULL)
				AllowFree(request->allow);
			request->allow=(SipAllow*)ret;
			break;
		case Allow_Events_Short:
		case Allow_Events:
			if(request->allowevents != NULL)
				AllowEventsFree(request->allowevents);
			request->allowevents=(SipAllowEvt*)ret;
			break;
		case Authorization:
			if(request->authorization != NULL){
				AuthorizationReqDupLink(request,(SipAuthz*)ret);
				AuthorizationFree((SipAuthz*)ret);
			}
			else
				request->authorization=(SipAuthz*)ret;
			break;
		case Call_ID_Short:
		case Call_ID:
			if(request->callid != NULL)
				CallIDFree(request->callid);
			request->callid=(unsigned char*)ret;
			break;
		case Call_Info:
			if(request->callinfo != NULL)
				CallInfoFree(request->callinfo);
			request->callinfo=(SipInfo*)ret;
			break;
		case Class:
			if(request->classpublish != NULL)
				ClassFree(request->classpublish);
			request->classpublish=(SipClass*)ret;
			break;
		case Contact_Short:
	    case Contact:
			if(request->contact == NULL)
				request->contact=(SipContact*)ret;
			else{
				ContactReqDupLink(request,(SipContact*)ret);
				sipContactFree((SipContact*)ret);
			}
			break;
		case Content_Disposition:
			if(request->contentdisposition != NULL)
				ContentDispositionFree(request->contentdisposition);
			request->contentdisposition=(SipContentDisposition*)ret;
			break;
		case Content_Encoding_Short:
		case Content_Encoding:
			if(request->contentencoding != NULL)
				ContentEncodingFree(request->contentencoding);
			request->contentencoding=(SipContEncoding*)ret;
			break;
		case Content_Language:
			if(request->contentlanguage != NULL)
				ContentLanguageFree(request->contentlanguage);
			request->contentlanguage=(SipContentLanguage*)ret;
			break;
		case Content_Length_Short:
		case Content_Length:
			if(!ret) break;
			request->contentlength = *(int *)ret;
			break;
		case Content_Type_Short:
		case Content_Type:
			if(request->contenttype != NULL)
				ContentTypeFree(request->contenttype);
			request->contenttype=(SipContType*)ret;
			break;
		case CSeq:
			if(request->cseq != NULL)
				CSeqFree(request->cseq);
			request->cseq=(SipCSeq*)ret;
			break;
		case Date:
			if(request->date != NULL)
				DateFree(request->date);
			request->date=(SipDate*)ret;
			break;
		case Event:
		case Event_Short:
			if(request->event !=NULL)
				EventFree(request->event);
			
			request->event=(SipEvent*)ret;
			break;
		case Encryption:
			if(request->encryption != NULL)
				EncryptionFree(request->encryption);
			request->encryption=(SipEncrypt*)ret;
			break;
		case Expires:
			if(request->expires != NULL)
				ExpiresFree(request->expires);
			request->expires=(SipExpires*)ret;
			break;
		case From_Short:
		case From:
			if(request->from != NULL)
				FromFree(request->from);
			request->from=(SipFrom*)ret;
			break;
		case Hide:
			request->hide = (SipHideType)((int)ret);
			break;
		case In_Reply_To:
			if(request->inreplyto != NULL)
				InReplyToFree(request->inreplyto);
			request->inreplyto=(SipInReplyTo*)ret;
			break;
		case Max_Forwards:
			if(!ret) break;
			request->maxforwards = *(int *)ret;
			break;
		case MIME_Version:
			if(request->mimeversion != NULL)
				MIMEVersionFree(request->mimeversion);
			request->mimeversion=(unsigned char*)ret;
			break;
		case Min_SE:
			if(request->minse != NULL)
				MinSEFree(request->minse);
			request->minse=(SipMinSE*)ret;
			break;
		case Organization:
			if(request->organization != NULL)
				OrganizationFree(request->organization);
			request->organization=(unsigned char*)ret;
			break;
		case P_Asserted_Identity:
			if(request->passertedidentity !=NULL){
				PAssertedIdentityReqDupLink(request,(SipIdentity*)ret);
				PAssertedIdentityFree((SipIdentity*)ret);
			}else
				request->passertedidentity=(SipIdentity*)ret;
			break;
		case P_Preferred_Identity:
			if(request->ppreferredidentity !=NULL){
				PPreferredIdentityReqDupLink(request,(SipIdentity*)ret);
				PPreferredIdentityFree((SipIdentity*)ret);
			}else
				request->ppreferredidentity=(SipIdentity*)ret;
			break;
		case Priority:
			request->priority = (SipPriorityType)((int)ret);
			break;
		case Privacy:
			if(request->privacy)
				PrivacyFree(request->privacy);
			request->privacy=(SipPrivacy*)ret;
			break;
		case Proxy_Authorization:
			if(request->proxyauthorization != NULL){
				ProxyAuthorizationReqDupLink(request,(SipPxyAuthz*)ret);
				ProxyAuthorizationFree((SipPxyAuthz*)ret);
			}
			else
				request->proxyauthorization=(SipPxyAuthz*)ret;
			break;
		case Proxy_Require:
			if(request->proxyrequire != NULL)
				ProxyRequireFree(request->proxyrequire);
			request->proxyrequire=(SipPxyRequire*)ret;
			break;
		case RAck:
			if(request->rack != NULL)
				RAckFree(request->rack);
			request->rack=(SipRAck*)ret;
			break;
		case Record_Route:
			if(request->recordroute != NULL){
				RecordRouteReqDupLink(request,(SipRecordRoute*)ret);
				RecordRouteFree((SipRecordRoute*)ret);
			}else
				request->recordroute=(SipRecordRoute*)ret;
			break;
		case Refer_To:
		case Refer_To_Short:
			if(request->referto != NULL){
				ReferToReqDup(request,(SipReferTo*)ret);
				ReferToFree((SipReferTo*)ret);
			}else
				request->referto=(SipReferTo*)ret;
			break;
		case Referred_By:
		case Referred_By_Short:
			if(request->referredby != NULL)
				ReferredByFree(request->referredby);
			request->referredby=(SipReferredBy*)ret;
			break;
		case Remote_Party_ID:
			if(request->remotepartyid != NULL){
				RemotePartyIDReqDupLink(request,(SipRemotePartyID*)ret);
				RemotePartyIDFree((SipRemotePartyID*)ret);
			}else
				request->remotepartyid=(SipRemotePartyID*)ret;
			break;
		case Replaces:
			if(request->replaces != NULL){
				ReplacesReqDupLink(request,(SipReplaces*)ret);
				ReplacesFree((SipReplaces*)ret);
			}else
				request->replaces=(SipReplaces*)ret;
			break;
		case Reply_To:
			if(request->replyto != NULL)
				ReplyToFree(request->replyto);
			request->replyto=(SipReplyTo*)ret;
			break;
		case Require:
			if(request->require != NULL)
				RequireFree(request->require);
			request->require=(SipRequire*)ret;
			break;
		case Response_Key:
			if(request->responsekey != NULL)
				ResponseKeyFree(request->responsekey);
			request->responsekey=(SipRspKey*)ret;
			break;
		case Retry_After:
			if(request->retryafter != NULL)
				RetryAfterFree(request->retryafter);
			request->retryafter=(SipRtyAft*)ret;
			break;
		case Route:
			if(request->route != NULL){
				RouteReqDupLink(request,(SipRoute*)ret);
				RouteFree((SipRoute*)ret);
			}else
				request->route=(SipRoute*)ret;
			break;
		case Session_Expires:
		case Session_Expires_Short:
			if(request->sessionexpires != NULL)
				SessionExpiresFree(request->sessionexpires);
			request->sessionexpires=(SipSessionExpires*)ret;
			break;
		case SIP_If_Match:
			if(request->sipifmatch != NULL)
				SIPIfMatchFree(request->sipifmatch);
			request->sipifmatch=(unsigned char*)ret;
			break;
		case Subject_Short:
		case Subject:
			if(request->subject != NULL)
				 SubjectFree(request->subject);
			request->subject=(unsigned char*)ret;
			break;
		case Subscription_State:
			if(request->subscriptionstate != NULL)
				SubscriptionStateFree(request->subscriptionstate);
			request->subscriptionstate=(SipSubState*)ret;
			break;
		case Supported:
		case Supported_Short:
			if(request->supported != NULL)
				SupportedFree(request->supported);
			request->supported=(SipSupported*)ret;
			break;
		case Timestamp:
			if(request->timestamp != NULL)
				TimestampFree(request->timestamp);
			request->timestamp=(SipTimestamp*)ret;
			break;
		case To_Short:
		case To:
			if(request->to != NULL)
				ToFree(request->to);
			request->to=(SipTo*)ret;
			break;
		case User_Agent:
			if(request->useragent != NULL)  
				UserAgentFree(request->useragent);
			request->useragent=(SipUserAgent*)ret;
			break;
		case Via_Short:
		case Via:
			if(request->via == NULL) request->via=(SipVia*)ret;
			else{
				ViaReqDupLink(request,(SipVia*)ret);
				ViaFree((SipVia*)ret);
			}
			break;			
#ifdef ICL_IMS
		case P_Media_Authorization:
			if(request->pmediaauthorization != NULL)
				free(request->pmediaauthorization);
			request->pmediaauthorization=(unsigned char*)ret;
			break;
		case Path:
			if(request->path == NULL) request->path=(SipPath*)ret;
			else{
				PathReqDupLink(request,(SipPath*)ret);
				PathFree((SipPath*)ret);
			}
			break;
		case P_Access_Network_Info:
			if(request->paccessnetworkinfo != NULL)  
				free(request->paccessnetworkinfo);
			request->paccessnetworkinfo=(unsigned char*)ret;
			break;
		case P_Called_Party_ID:
			if(request->pcalledpartyid != NULL)  
				free(request->pcalledpartyid);
			request->pcalledpartyid=(unsigned char*)ret;
			break;
		case P_Charging_Function_Address:
			if(request->pchargingfunctionaddress != NULL)  
				free(request->pchargingfunctionaddress);
			request->pchargingfunctionaddress=(unsigned char*)ret;
			break;
		case P_Visited_Network_ID:
			if(request->pvisitednetworkid != NULL)  
				free(request->pvisitednetworkid);
			request->pvisitednetworkid=(unsigned char*)ret;
			break;
		case Security_Client:
			if(request->securityclient != NULL)  
				free(request->securityclient);
			request->securityclient=(unsigned char*)ret;
			break;
		case Security_Verify:
			if(request->securityverify != NULL)  
				free(request->securityverify);
			request->securityverify=(unsigned char*)ret;
			break;
#endif /* end of ICL_IMS */	

		case UnknSipHdr:
			break;
		default:
			TCRPrint(1,"<sipReqAddHdr> Header not supported\n");
			return RC_ERROR;
    }
    return RC_OK;
}

CCLAPI 
RCODE sipReqAddHdrFromTxt(SipReq request, SipHdrType type, char* header)
{
	MsgHdr hdr,tmp;
	void *ret;
	char *tmpbuf;
	DxStr unHdrStr;
	RCODE rcode=RC_OK;

	if(request == NULL)
		return RC_ERROR;

	request->reqflag=TRUE;
	tmpbuf=(char*)calloc(strlen(header)+2,1);
	strcpy(tmpbuf,header);
	hdr=msgHdrNew();
	rfc822_parseline((unsigned char*)tmpbuf,strlen(header),hdr);
	tmp=hdr;

	if((tmp!=NULL)&&((tmp->name)!=NULL)&&(strlen(tmp->name)>0)){
		SipHdrType hdrtype=sipHdrNameToType(tmp->name);
		if(hdrtype==UnknSipHdr){
			/* add into unhdrbuf, in the switch case (UnknSipHdr) Acer 02/3/28 */
			unHdrStr=dxStrNew();
			if (request->unHdr) {
				dxStrCat(unHdrStr,(const char*)request->unHdr);
				free(request->unHdr);
			}
			dxStrCat(unHdrStr,(const char*)header);
			dxStrCat(unHdrStr,(const char*)"\r\n");
			
			request->unHdr=strDup(dxStrAsCStr(unHdrStr));
			dxStrFree(unHdrStr);
			
		}else{
			if(RFC822ToSipHeader(tmp,hdrtype,&ret)){ 
				rcode=sipReqAddHdrEx(request,hdrtype,ret);
				if((hdrtype==Content_Length)
					||(hdrtype==Max_Forwards)
					||(hdrtype==Min_Expires)
					||(hdrtype==RSeq)
					||(hdrtype==Content_Length_Short))
					free(ret);
			}else
				rcode=RC_ERROR;
		}
		/*
		if(RFC822ToSipHeader(tmp,hdrtype,&ret)==1){
			if(hdrtype==UnknSipHdr){
				TCRPrint(1,"<sipReqAddHdrFromTxt> Header Type Not supported, without parsing! \n");
				if(request->unHdr == NULL)
					request->unHdr=strDup((const char*)header);
				else{
					unHdrStr=dxStrNew();
					dxStrCat(unHdrStr,(const char*)request->unHdr);
					dxStrCat(unHdrStr,(const char*)header);
					free(request->unHdr);
					request->unHdr=strDup(dxStrAsCStr(unHdrStr));
					dxStrFree(unHdrStr);
				}
			}else
				rcode=sipReqAddHdrEx(request,hdrtype,ret);
		}else{
			rcode=RC_ERROR;
		}
		*/
	}else
		rcode = RC_ERROR;

	msgHdrFree(hdr);
	free(tmpbuf);

	return rcode;
}

CCLAPI
void*   sipReqGetHdr(SipReq request, SipHdrType headertype)
{
	if(request == NULL)
	   return NULL;
	switch(headertype){
		case Accept:
			return (void *)request->accept;
		case Accept_Encoding:
			return (void *)request->acceptencoding;
		case Accept_Language:
			return (void *)request->acceptlanguage;
		case Alert_Info:
			return (void *)request->alertinfo;
		case Allow:
			return (void *)request->allow;
		case Allow_Events_Short:
		case Allow_Events:
			return (void *)request->allowevents;
		case Authorization:
			return (void *)request->authorization;
		case Call_ID_Short:      
		case Call_ID:
			return (void *)request->callid;
		case Call_Info:
			return (void *)request->callinfo;
		case Class:
			return (void *)request->classpublish;
		case Contact_Short:
		case Contact:
			return (void *)request->contact;
		case Content_Disposition:
			return (void *)request->contentdisposition;
		case Content_Encoding_Short:
		case Content_Encoding:
			return (void *)request->contentencoding;
		case Content_Language:
			return (void *)request->contentlanguage;
		case Content_Length_Short:
		case Content_Length:
			return (void *)(&request->contentlength);
		case Content_Type_Short:
		case Content_Type:
			return (void *)request->contenttype;
	    case CSeq:
			return (void *)request->cseq;
		case Date:
			return (void *)request->date;
		case Encryption:
			return (void *)request->encryption;
		case Event:
		case Event_Short:
			return (void*)request->event;
		case Expires:
			return (void *)request->expires;
		case From_Short:
		case From:
			return (void *)request->from;
		case Hide:
			return (void *)request->hide;
		case In_Reply_To:
			return (void *)request->inreplyto;
		case Max_Forwards:
			return (void *)(&request->maxforwards);
		case MIME_Version:
			return (void *)request->mimeversion;
		case Min_SE:
			return (void*)request->minse;
		case Organization:
			return (void *)request->organization;
		case P_Asserted_Identity:
			return (void *)request->passertedidentity;
		case P_Preferred_Identity:
			return (void *)request->ppreferredidentity;
		case Priority:
			return (void *)request->priority;
		case Privacy:
			return (void *)request->privacy;
		case Proxy_Authorization:
			return (void *)request->proxyauthorization;
		case Proxy_Require:
			return (void *)request->proxyrequire;
		case RAck:
			return (void *)request->rack;
		case Record_Route:
			return (void *)request->recordroute;
		case Refer_To:
		case Refer_To_Short:
			return (void *)request->referto;
		case Referred_By:
		case Referred_By_Short:
			return (void*)request->referredby;
		case Remote_Party_ID:
			return (void*)request->remotepartyid;
		case Replaces:
			return (void*)request->replaces;
		case Reply_To:
			return (void*)request->replyto;
		case Require:
			return (void *)request->require;
		case Response_Key:
			return (void *)request->responsekey;
		case Retry_After:
			return (void *)request->retryafter;
		case Route:
			return (void *)request->route;
		case Session_Expires:
		case Session_Expires_Short:
			return (void *)request->sessionexpires;
		case SIP_If_Match:
			return (void *)request->sipifmatch;
		case Subject_Short:
		case Subject:
			return (void *)request->subject;
		case Subscription_State:
			return (void *)request->subscriptionstate;
		case Supported_Short:
		case Supported:
			return (void *)request->supported;
		case Timestamp:
			return (void *)request->timestamp;
		case To_Short:
	        case To:
			return (void *)request->to;
	        case User_Agent:
			return (void *)request->useragent;
		case Via_Short:
		case Via:
			return (void *)request->via;
		case UnknSipHdr:
			return (void *)request->unHdr;
#ifdef ICL_IMS
		case P_Media_Authorization:
			return (void *)request->pmediaauthorization;
		case Path:
			return (void *)request->path;
		case P_Access_Network_Info:
			return (void *)request->paccessnetworkinfo;
		case P_Called_Party_ID:
			return (void *)request->pcalledpartyid;
		case P_Charging_Function_Address:
			return (void *)request->pchargingfunctionaddress;
		case P_Visited_Network_ID:
			return (void *)request->pvisitednetworkid;
		case Security_Client:
			return (void *)request->securityclient;
		case Security_Verify:
			return (void *)request->securityverify;
#endif /* end of ICL_IMS */	
		default:
			return (void *)NULL;
	}
}

/* get original header list */
CCLAPI 
MsgHdr		sipReqGetHdrLst(IN SipReq request)
{
	if (request) return request->hdrlist;
	return NULL;
}

CCLAPI
SipBody* sipReqGetBody(SipReq request)
{
	if(request == NULL)
		return NULL;
	else
		return request->body;
}

CCLAPI
RCODE sipReqSetBody(SipReq request,SipBody *content)
{
	SipBody *_body;

	if(request == NULL)
	   return RC_ERROR;
	request->reqflag=TRUE;
	_body = (SipBody*)calloc(1,sizeof(*_body));
	_body->content = (unsigned char*)strDup((const char*)content->content);
	_body->length = content->length;
	if(request->body)
		sipBodyFree(request->body);
	request->body = _body;
	return RC_OK;
}


CCLAPI 
SipReq	sipReqNewFromTxt(char* text)
{
	SipReq	request;
	char*		temp;
	int		length=0;

	length=strlen(text);
	/*temp = (char*)malloc(sizeof(char)*(length+1));*/
	temp = (char*)calloc((length+1),sizeof(char));
	strcpy(temp,text);

	request = sipReqNew();
	request->reqflag=TRUE;
	if( sipReqParse(request, (unsigned char*)temp, &length)!= SIP_OK ) {
		TCRPrint(1,"<SipReqNewFromTxt> Can't parse, input message format error!\n");
		sipReqFree(request);
		free(temp);
		return NULL;
	}

	free(temp);

	return request;
}

CCLAPI 
char*	sipReqPrint(SipReq request)
{
	int retsize;
	DxStr ReqBuf;

	if ( !request )
		return "\0";

	if(request->reqflag==TRUE){
		ReqBuf=dxStrNew();
		sipReqAsStr(request,ReqBuf,&retsize);
		if(request->reqbuf != NULL)
			free(request->reqbuf);
		request->reqbuf=strDup(dxStrAsCStr(ReqBuf));
		dxStrFree(ReqBuf);
		request->reqflag=TRUE;
	}
	return request->reqbuf; 
}

/* to set unkwon header buf in request */
CCLAPI 
RCODE	sipReqAddUnknowHdr(IN SipReq request,IN const char *unknowhdr)
{
	/* check input parameter*/
	if(request==NULL){
		TCRPrint(1,"***[sipReqAddUnknowHdr] input request is NULL!");
		return SIP_REQ_ERROR;
	}
	if(unknowhdr==NULL){
		TCRPrint(1,"***[sipReqAddUnknowHdr] input unknow hrea is NULL!");
		return RC_ERROR;
	}
	/* check request->unhdr */
	if(request->unHdr)
		free(request->unHdr);

	/* set new value for unknown header */
	request->unHdr=strDup(unknowhdr);
	return RC_OK;
}


/*------------------ Internal Duplicate Functions ------------------*/
int AcceptReqDup(SipReq request,SipAccept *accept )
{
	if(request->accept!=NULL)
		AcceptFree(request->accept);
	request->accept=(SipAccept*)calloc(1,sizeof(SipAccept));
	request->accept->numMediaTypes=accept->numMediaTypes;
	request->accept->sipMediaTypeList=NULL;
	if(accept->sipMediaTypeList != NULL)
		request->accept->sipMediaTypeList=sipMediaTypeDup(accept->sipMediaTypeList,accept->numMediaTypes);
	return 1;
}

int AcceptEncodingReqDup(SipReq request,SipAcceptEncoding* coding)
{
	if(request->acceptencoding != NULL)
		AcceptEncodingFree(request->acceptencoding);
	request->acceptencoding=(SipAcceptEncoding*)calloc(1,sizeof(SipAcceptEncoding));
	request->acceptencoding->numCodings=coding->numCodings;
	request->acceptencoding->sipCodingList=NULL;
	if(coding->sipCodingList != NULL)
		request->acceptencoding->sipCodingList=sipCodingDup(coding->sipCodingList,coding->numCodings);
	return 1;
}

int AllowReqDup(SipReq request, SipAllow *allow)
{
	int idx;
	if(request->allow == NULL)
		request->allow=(SipAllow*)calloc(1,sizeof(SipAllow));
	for(idx=0;idx<SIP_SUPPORT_METHOD_NUM;idx++)
		request->allow->Methods[idx]=allow->Methods[idx];
	return 1;
}

int AllowEventsReqDup(SipReq request, SipAllowEvt *allowevt)
{
	int count=0;
	SipAllowEvt *tmpallowevt;
	if(request->allowevents != NULL){
		AllowEventsFree(request->allowevents);
		request->allowevents=NULL;
	}
	while(allowevt != NULL){
		if(count==0){
			request->allowevents=(SipAllowEvt*)calloc(1,sizeof(SipAllowEvt));
			tmpallowevt=request->allowevents;
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

int AcceptLanguageReqDup(SipReq request, SipAcceptLang* lang)
{
	if(request->acceptlanguage == NULL)
		AcceptLanguageFree(request->acceptlanguage);
	request->acceptlanguage=(SipAcceptLang*)calloc(1,sizeof(SipAcceptLang));
	request->acceptlanguage->numLangs=lang->numLangs;
	request->acceptlanguage->sipLangList=NULL;
	if(lang->sipLangList != NULL)
		request->acceptlanguage->sipLangList=sipLanguageDup(lang->sipLangList,lang->numLangs);
	return 1;
}


int AuthorizationReqDupLink(SipReq request, SipAuthz *author)
{
	SipAuthz *newAuthz,*tmpAuthz;

	if(request->authorization == NULL)
		request->authorization = sipAuthzDup(author);
	else{
		newAuthz=sipAuthzDup(author);
		tmpAuthz=request->authorization;
		while(tmpAuthz->next!=NULL)
			tmpAuthz=tmpAuthz->next;
		tmpAuthz->next=newAuthz;
	}
	return 1;

}

int AlertInfoReqDup(SipReq request,SipInfo *Alertinfo)
{
	SipInfo *nowAlertinfo,*tmpAlertinfo;
	

	nowAlertinfo=request->alertinfo;
	if(nowAlertinfo != NULL)
			AlertInfoFree(nowAlertinfo);
	tmpAlertinfo=NULL;
	while(Alertinfo!=NULL){
		nowAlertinfo=(SipInfo*)calloc(1,sizeof(SipInfo));
		if(tmpAlertinfo != NULL) 
			tmpAlertinfo->next=nowAlertinfo;
		else
			request->alertinfo=nowAlertinfo;
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

int CallIDReqDup(SipReq request,unsigned char* str)
{
	if(request->callid != NULL)
		free(request->callid);
	request->callid=(unsigned char*)strDup((char*)str);
	return 1;
}

int CallInfoReqDup(SipReq request,SipInfo *Callinfo)
{
	SipInfo *nowCallinfo,*tmpCallinfo;
	

	nowCallinfo=request->callinfo;
	if(nowCallinfo != NULL)
		CallInfoFree(nowCallinfo);

	tmpCallinfo=NULL;
	while(Callinfo!=NULL){
		nowCallinfo=(SipInfo*)calloc(1,sizeof(SipInfo));
		if(tmpCallinfo != NULL) 
			tmpCallinfo->next=nowCallinfo;
		else
			request->callinfo=nowCallinfo;
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

int ClassReqDup(SipReq request,SipClass *_class)
{
	if(request->classpublish != NULL)
		ClassFree(request->classpublish);
	request->classpublish=(SipClass*)calloc(1,sizeof(SipClass));
	request->classpublish->numClass=_class->numClass;
	request->classpublish->classList=NULL;
	if(_class->classList != NULL)
		request->classpublish->classList=sipStrDup(_class->classList,_class->numClass);
	return 1;
} 

int ContactReqDupLink(SipReq request,SipContact* contact)
{
	SipContactElm *elm,*ptrelm,*lastelm;


	if (!contact) return 1;
	
	elm=sipContactElmDup(contact->sipContactList,contact->numContacts);
	if(request->contact != NULL) {
		ptrelm=request->contact->sipContactList;
		
		if(ptrelm != NULL){
			lastelm=ptrelm;
			while(1){
				ptrelm=ptrelm->next;
				if (ptrelm == NULL) break;
				lastelm=ptrelm;
			}
			request->contact->numContacts=request->contact->numContacts+contact->numContacts;
			lastelm->next=elm;
		}else{
			request->contact->numContacts=contact->numContacts;
			request->contact->sipContactList=elm;
		}
	}else{
		request->contact=(SipContact*)calloc(1,sizeof(SipContact));
		request->contact->numContacts=contact->numContacts;
		request->contact->sipContactList=elm;
	}
	return 1;
}

int ContentDispositionReqDup(SipReq request,SipContentDisposition* contentdisp)
{
	if(request->contentdisposition != NULL)
		ContentDispositionFree(request->contentdisposition);
	request->contentdisposition=(SipContentDisposition*)calloc(1,sizeof(SipContentDisposition));
	request->contentdisposition->numParam=contentdisp->numParam;
	request->contentdisposition->disptype=NULL;
	request->contentdisposition->paramList=NULL;
	if(contentdisp->disptype != NULL)
		request->contentdisposition->disptype=strDup(contentdisp->disptype);
	if(contentdisp->paramList != NULL)
		request->contentdisposition->paramList=sipParameterDup(contentdisp->paramList,contentdisp->numParam);
	return 1;
}

int ContentEncodingReqDup(SipReq request,SipContEncoding* contcoding)
{
	if(request->contentencoding != NULL)
		ContentEncodingFree(request->contentencoding);
	request->contentencoding=(SipContEncoding*)calloc(1,sizeof(SipContEncoding));
	request->contentencoding->numCodings=contcoding->numCodings;
	request->contentencoding->sipCodingList=NULL;
	if(contcoding != NULL)
		request->contentencoding->sipCodingList=sipCodingDup(contcoding->sipCodingList,contcoding->numCodings);
	return 1;
}

int ContentLanguageReqDup(SipReq request,SipContentLanguage* contentlang)
{
	if(request->contentlanguage != NULL)
		ContentLanguageFree(request->contentlanguage);
	request->contentlanguage=(SipContentLanguage*)calloc(1,sizeof(SipContentLanguage));
	request->contentlanguage->numLang=contentlang->numLang;
	request->contentlanguage->langList=NULL;
	if(contentlang->langList != NULL)
		request->contentlanguage->langList=sipStrDup(contentlang->langList,contentlang->numLang);
	return 1;
} 

int ContentTypeReqDup(SipReq request,SipContType* conttype)
{
	if(request->contenttype != NULL)
		ContentTypeFree(request->contenttype);
	request->contenttype=(SipContType*)calloc(1,sizeof(SipContType));
	request->contenttype->next=NULL;
	request->contenttype->numParam=conttype->numParam;
	request->contenttype->type=NULL;
	request->contenttype->subtype=NULL;
	request->contenttype->sipParamList=NULL;
	if(conttype->type!= NULL)
		request->contenttype->type=strDup(conttype->type);
	if(conttype->subtype != NULL)
		request->contenttype->subtype=strDup(conttype->subtype);
	if(conttype->sipParamList != NULL)
		request->contenttype->sipParamList=sipParameterDup(conttype->sipParamList,conttype->numParam);
	return 1;
}

int CSeqReqDup(SipReq request,SipCSeq* seq)
{
	if(request->cseq != NULL)
		CSeqFree(request->cseq);
	request->cseq=(SipCSeq*)calloc(1,sizeof(SipCSeq));
	request->cseq->Method=seq->Method;
	request->cseq->seq=seq->seq;
	return 1;
}

int DateReqDup(SipReq request,SipDate* InDate)
{
	if(request->date != NULL)
		DateFree(request->date);
	request->date=(SipDate*)calloc(1,sizeof(SipDate));
	if(InDate->date!= NULL){
		request->date->date=(SipRfc1123Date*)calloc(1,sizeof(SipRfc1123Date));
		request->date->date->month=InDate->date->month;
		request->date->date->weekday=InDate->date->weekday;
	}
	if(InDate->date->year!=NULL)
		strcpy(request->date->date->year,InDate->date->year);
	if(InDate->date->day != NULL)
		strcpy(request->date->date->day,InDate->date->day);
	if(InDate->date->time != NULL)
		strcpy(request->date->date->time,InDate->date->time);
	return 1;
}

int EncryptionReqDup(SipReq request,SipEncrypt* encrypt)
{
	if(request->encryption != NULL)
		EncryptionFree(request->encryption);
	request->encryption=(SipEncrypt*)calloc(1,sizeof(SipEncrypt));
	request->encryption->numParams=encrypt->numParams;
	request->encryption->scheme=NULL;
	request->encryption->sipParamList=NULL;
	if(encrypt->scheme != NULL)
		request->encryption->scheme=strDup(encrypt->scheme);
	if(encrypt->sipParamList != NULL)
		request->encryption->sipParamList=sipParameterDup(encrypt->sipParamList,encrypt->numParams);
	return 1;
}

int	EventReqDup(SipReq req, SipEvent *event)
{
	SipEvent *finalevent,*newevent;

	if(!req || !event)
		return 0;

	finalevent=req->event;
	if( finalevent != NULL){
		EventFree(finalevent);
		finalevent=NULL;
	}		

	newevent=(SipEvent*)calloc(1,sizeof(*newevent));
	if(newevent)
		req->event=newevent;
	else{
		req->event=NULL;
		return 0;
	}

	newevent->numParam=event->numParam;
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


int ExpiresReqDup(SipReq request,SipExpires* expire)
{
	if( !request || !expire )
		return 0;

	if(request->expires != NULL)
		ExpiresFree(request->expires);
	request->expires=(SipExpires*)calloc(1,sizeof(SipExpires));
	request->expires->expireSecond=expire->expireSecond;
	/*request->expires->expireDate=(rfc1123Date*)malloc(sizeof(rfc1123Date));*/
	/*request->expires->expireDate = expire->expireDate;*/
	/*strcpy(request->expires->expireDate->day,expire->expireDate->day);*/
	/*strcpy(request->expires->expireDate->year,expire->expireDate->year);*/
	/*strcpy(request->expires->expireDate->time,expire->expireDate->time);*/
	/*request->expires->expireDate = ((expire->expireDate)?rfc1123DateDup(expire->expireDate):NULL);*/
	/*request->expires->expireDate->month=expire->expireDate->month;*/
	/*request->expires->expireDate->weekday=expire->expireDate->weekday;*/

	return 1;
}

int FromReqDup(SipReq request,SipFrom* Infrom)
{
	if(request->from != NULL)
		FromFree(request->from);
	request->from=(SipFrom*)calloc(1,sizeof(SipFrom));
	request->from->numParam=Infrom->numParam;
	request->from->numAddr=Infrom->numAddr;
	request->from->address=NULL;
	request->from->ParamList=NULL;
	if(Infrom->address != NULL)
		request->from->address=sipAddrDup(Infrom->address,Infrom->numAddr);
	if(Infrom->ParamList != NULL)
		request->from->ParamList=sipParameterDup(Infrom->ParamList,Infrom->numParam);
	return 1;
}

int InReplyToReqDup(SipReq request,SipInReplyTo *inreplyto)
{
	if(request->inreplyto!=NULL)
		InReplyToFree(request->inreplyto);
	request->inreplyto=(SipInReplyTo*)calloc(1,sizeof(SipInReplyTo));
	request->inreplyto=NULL;
	request->inreplyto->numCallids=inreplyto->numCallids;
	if(inreplyto->CallidList !=NULL )
		request->inreplyto->CallidList=sipStrDup(inreplyto->CallidList,inreplyto->numCallids);
	return 1;
}

int MIMEVersionReqDup(SipReq request,unsigned char *str)
{
	if(request->mimeversion != NULL)
		free(request->mimeversion);
	request->mimeversion=(unsigned char*)strDup((const char*)str);
	return 1;
}

int MinSEReqDup(SipReq req,SipMinSE *_minse)
{
	SipMinSE *newobj;

	if(req->minse != NULL)
		MinSEFree(req->minse);
	if(_minse != NULL){
		newobj=(SipMinSE*)calloc(1,sizeof(*newobj));
		newobj->deltasecond=_minse->deltasecond;
		newobj->numParam=_minse->numParam;
		newobj->paramList=sipParameterDup(_minse->paramList,_minse->numParam);
		req->minse=newobj;
	}else
		req->minse=NULL;

	return 1;
}

int OrganizationReqDup(SipReq request,const char* str)
{
	if(request->organization != NULL)
		OrganizationFree(request->organization);
	request->organization=(unsigned char*)strDup((const char*)str);
	return 1;
}

int	PAssertedIdentityReqDupLink(SipReq request,SipIdentity *pId)
{
	SipIdentity *last,*newid;

	if(request==NULL)
		return 0;
	if(pId==NULL)
		return 0;
	newid=(SipIdentity*)calloc(1,sizeof(SipIdentity));

	if(request->passertedidentity==NULL)
		request->passertedidentity=newid;
	else{
		last=request->passertedidentity;
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

int	PPreferredIdentityReqDupLink(SipReq request,SipIdentity *pId)
{

	SipIdentity *last,*newid;

	if(request==NULL)
		return 0;
	if(pId==NULL)
		return 0;
	newid=(SipIdentity*)calloc(1,sizeof(SipIdentity));

	if(request->ppreferredidentity==NULL)
		request->ppreferredidentity=newid;
	else{
		last=request->ppreferredidentity;
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

int PrivacyReqDup(SipReq request,SipPrivacy *pPrivacy)
{
	if(request){
		if(request->privacy)
			PrivacyFree(request->privacy);
	}else
		return 0;
	if(pPrivacy)
		request->privacy=calloc(1,sizeof(SipPrivacy));
	else{
		request->privacy=NULL;
		return 0;
	}
	if(pPrivacy->privvalue){
		request->privacy->privvalue=sipStrDup(pPrivacy->privvalue,pPrivacy->numPriv);
		request->privacy->numPriv=pPrivacy->numPriv;
	}else{
		request->privacy->privvalue=NULL;
		request->privacy->numPriv=0;
	}
	return 1;
}

int ProxyAuthorizationReqDupLink(SipReq request,SipPxyAuthz* author)
{
	SipPxyAuthz *newPxyAuthz,*tmpPxyAuthz;
	
	if(request->proxyauthorization == NULL)
		request->proxyauthorization=sipPxyAuthzDup(author);
	else{
		newPxyAuthz=sipPxyAuthzDup(author);
		tmpPxyAuthz=request->proxyauthorization;
		while(tmpPxyAuthz->next!=NULL)
			tmpPxyAuthz=tmpPxyAuthz->next;
		tmpPxyAuthz->next=newPxyAuthz;
	}

	return 1;

}

int ProxyRequireReqDup(SipReq request,SipPxyRequire* require)
{
	if(request->proxyrequire != NULL)
		ProxyRequireFree(request->proxyrequire);
	request->proxyrequire=(SipPxyRequire*)calloc(1,sizeof(SipPxyRequire));
	request->proxyrequire->numTags=require->numTags;
	request->proxyrequire->sipOptionTagList=NULL;
	if(require->sipOptionTagList != NULL)
		request->proxyrequire->sipOptionTagList=sipStrDup(require->sipOptionTagList,require->numTags);
	return 1;
}

int RAckReqDup(SipReq request,SipRAck *pRack)
{
	if(request->rack != NULL){
		RAckFree(request->rack);
		request->rack=NULL;
	}
	request->rack=(SipRAck*)calloc(1,sizeof(SipRAck));
	if(request->rack){
		request->rack->rseq=pRack->rseq;
		request->rack->cseq=pRack->cseq;
		request->rack->Method=pRack->Method;
	}
	return 1;
}

int RecordRouteReqDupLink(SipReq request,SipRecordRoute* record)
{
	SipRecordRoute *tmpRR;
	SipRecAddr	*newRRAddr,*oldRRAddr;
	if(request->recordroute == NULL){
		request->recordroute=(SipRecordRoute*)calloc(1,sizeof(SipRecordRoute));
		request->recordroute->numNameAddrs=record->numNameAddrs;
		request->recordroute->sipNameAddrList=NULL;
		if(record->sipNameAddrList != NULL)
			request->recordroute->sipNameAddrList=sipRecAddrDup(record->sipNameAddrList,record->numNameAddrs);
	}else{
		if(record->sipNameAddrList!= NULL){
			newRRAddr=sipRecAddrDup(record->sipNameAddrList,record->numNameAddrs);
			tmpRR=request->recordroute;
			oldRRAddr=tmpRR->sipNameAddrList;
			while(oldRRAddr->next!=NULL)
				oldRRAddr=oldRRAddr->next;
			oldRRAddr->next=newRRAddr;
			request->recordroute->numNameAddrs+=record->numNameAddrs;
		}
	}

	return 1;
}

int ReferToReqDup(SipReq req,SipReferTo* referto)
{

	if(req==NULL)
		return 0;
	if(referto==NULL)
		return 1;

	if(req->referto != NULL)
		ReferToFree(req->referto);
	req->referto=(SipReferTo*)calloc(1,sizeof(SipReferTo));
	req->referto->numAddr=referto->numAddr;
	req->referto->numParam=referto->numParam;
	req->referto->address=NULL;
	req->referto->paramList=NULL;
	if(referto->address != NULL)
		req->referto->address=sipAddrDup(referto->address,referto->numAddr);
	if(referto->paramList != NULL)
		req->referto->paramList=sipParameterDup(referto->paramList,referto->numParam);
	
	return 1;
}

int ReferredByReqDup(SipReq req,SipReferredBy* referredby)
{
	if(req==NULL)
		return 0;
	if(referredby==NULL)
		return 1;
	
	if(req->referredby != NULL)
		ReferredByFree(req->referredby);
	req->referredby=(SipReferredBy*)calloc(1,sizeof(SipReferredBy));
	req->referredby->numAddr=referredby->numAddr;
	req->referredby->numParam=referredby->numParam;
	req->referredby->address=NULL;
	req->referredby->paramList=NULL;
	if(referredby->address != NULL)
		req->referredby->address=sipAddrDup(referredby->address,referredby->numAddr);
	if(referredby->paramList != NULL)
		req->referredby->paramList=sipParameterDup(referredby->paramList,referredby->numParam);
	return 1;
}

int RemotePartyIDReqDupLink(SipReq req,SipRemotePartyID *remotepartyid)
{
	SipRemotePartyID *tmp,*newobj,*last;

	if(req && remotepartyid) ;
	else
		return 0;
		
	tmp=remotepartyid;
	last=req->remotepartyid;
	
	newobj=(SipRemotePartyID*)calloc(1,sizeof(*newobj));
	
	if(last==NULL)
		req->remotepartyid=newobj;
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


int ReplacesReqDupLink(SipReq req,SipReplaces* replaces)
{
	SipReplaces *lastreplaces,*tmp,*newreplaces;

	if(req==NULL)
		return 0;
	if(replaces==NULL)
		return 0;
	newreplaces=(SipReplaces*)calloc(1,sizeof(*newreplaces));
	if (!newreplaces) return 0;
	if(req->replaces==NULL)
		req->replaces=newreplaces;
	else{
		lastreplaces=req->replaces;
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

int ReplyToReqDup(SipReq request,SipReplyTo* _replyto)
{
	if(request->replyto != NULL)
		ReplyToFree(request->replyto);
	request->replyto=(SipReplyTo*)calloc(1,sizeof(SipReplyTo));
	request->replyto->numParam=_replyto->numParam;
	request->replyto->numAddr=_replyto->numAddr;
	request->replyto->address=NULL;
	request->replyto->ParamList=NULL;
	if(_replyto->address != NULL)
		request->replyto->address=sipAddrDup(_replyto->address,_replyto->numAddr);
	if(_replyto->ParamList != NULL)
		request->replyto->ParamList=sipParameterDup(_replyto->ParamList,_replyto->numParam);
	return 1;
}

int RequireReqDup(SipReq request,SipRequire* require)
{
	if(request->require != NULL)
		RequireFree(request->require);
	request->require=(SipRequire*)calloc(1,sizeof(SipRequire));
	request->require->numTags=require->numTags;
	request->require->sipOptionTagList=NULL;
	if(require->sipOptionTagList != NULL)
		request->require->sipOptionTagList=sipStrDup(require->sipOptionTagList,require->numTags);
	return 1;
}

int ResponseKeyReqDup(SipReq request,SipRspKey* key)
{
	if(request->responsekey != NULL)
		ResponseKeyFree(request->responsekey);
	request->responsekey=(SipRspKey*)calloc(1,sizeof(SipRspKey));
	request->responsekey->sipParamList=NULL;
	if(key->scheme != NULL)
		request->responsekey->scheme=strDup(key->scheme);
	request->responsekey->numParams=key->numParams;
	if(key->sipParamList != NULL)
		request->responsekey->sipParamList=sipParameterDup(key->sipParamList,key->numParams);
	return 1;
}

int RetryAfterReqDup(SipReq request,SipRtyAft* after)
{
	if(request->retryafter != NULL)
		RetryAfterFree(request->retryafter);
	request->retryafter=(SipRtyAft*)calloc(1,sizeof(SipRtyAft));
	request->retryafter->afterSecond=after->afterSecond;
	request->retryafter->afterDate=NULL;
	request->retryafter->comment=NULL;
	if(after->afterDate != NULL)
		request->retryafter->afterDate=rfc1123DateDup(after->afterDate);
	if(after->comment != NULL)
		request->retryafter->comment=strDup(after->comment);
	request->retryafter->duration=after->duration;
	return 1;
}

int RouteReqDupLink(SipReq request,SipRoute* route)
{
	SipRoute *tmpR;
	SipRecAddr *newRAddr,*oldRAddr;
	if(request->route == NULL){
		request->route=(SipRoute*)calloc(1,sizeof(SipRoute));
		request->route->numNameAddrs=route->numNameAddrs;
		request->route->sipNameAddrList=NULL;
		if(route->sipNameAddrList != NULL)
			request->route->sipNameAddrList=sipRecAddrDup(route->sipNameAddrList,route->numNameAddrs);
	}else{
		if(route->sipNameAddrList != NULL){
			newRAddr=sipRecAddrDup(route->sipNameAddrList,route->numNameAddrs);
			tmpR=request->route;
			oldRAddr=tmpR->sipNameAddrList;
			while(oldRAddr->next!= NULL)
				oldRAddr=oldRAddr->next;
			oldRAddr->next=newRAddr;
			request->route->numNameAddrs+=route->numNameAddrs;
		}
	}
	return 1;
}
int SessionExpiresReqDup(SipReq req,SipSessionExpires *_sessionexpires)
{
	SipSessionExpires *newobj;

	if(req->sessionexpires != NULL)
		SessionExpiresFree(req->sessionexpires);
	if(_sessionexpires != NULL){
		newobj=(SipSessionExpires*)calloc(1,sizeof(*newobj));
		newobj->deltasecond=_sessionexpires->deltasecond;
		newobj->numParam=_sessionexpires->numParam;
		newobj->paramList=sipParameterDup(_sessionexpires->paramList,_sessionexpires->numParam);
		req->sessionexpires=newobj;
	}
	return 1;
}

int SubjectReqDup(SipReq request,unsigned char* subject)
{
	if(request->subject != NULL)
		free(request->subject);
	request->subject=(unsigned char*)strDup((const char*)subject);
	return 1;
}

int SubscriptionStateReqDup(SipReq request, SipSubState *substate)
{
	if(request->subscriptionstate != NULL)
		SubscriptionStateFree(request->subscriptionstate);
	if(substate != NULL){
		request->subscriptionstate=(SipSubState*)calloc(1,sizeof(SipSubState));
		request->subscriptionstate->expires=substate->expires;
		request->subscriptionstate->retryatfer=substate->retryatfer;
		request->subscriptionstate->state = NULL;
		request->subscriptionstate->reason = NULL;
		request->subscriptionstate->paramList = NULL;
		request->subscriptionstate->numParam = substate->numParam;
		if(substate->state != NULL)
			request->subscriptionstate->state=strDup((const char*)substate->state);
		if(substate->reason != NULL)
			request->subscriptionstate->reason=strDup((const char*)substate->reason);
		if(substate->paramList != NULL)
			request->subscriptionstate->paramList=sipParameterDup(substate->paramList,substate->numParam);
	}
	return 1;
}

int SupportedReqDup(SipReq request,SipSupported *support)
{
	if(request->supported != NULL)
		SupportedFree(request->supported);
	request->supported=(SipSupported*)calloc(1,sizeof(SipSupported));
	request->supported->numTags=support->numTags;
	request->supported->sipOptionTagList=NULL;
	if(support->sipOptionTagList != NULL)
		request->supported->sipOptionTagList=sipStrDup(support->sipOptionTagList,support->numTags);
	return 1;
}

int TimestampReqDup(SipReq request, SipTimestamp* stamp)
{
	if(request->timestamp != NULL)
		TimestampFree(request->timestamp);
	request->timestamp=(SipTimestamp*)calloc(1,sizeof(SipTimestamp));
	request->timestamp->delay=NULL;
	request->timestamp->time=NULL;
	if(stamp->delay != NULL)
		request->timestamp->delay=strDup(stamp->delay);
	if(stamp->time != NULL)
		request->timestamp->time=strDup(stamp->time);
	return 1;
}

int ToReqDup(SipReq request,SipTo* toList)
{
	if(request->to != NULL)
		ToFree(request->to);
	request->to=(SipTo*)calloc(1,sizeof(SipTo));
	request->to->numAddr=toList->numAddr;
	request->to->numParam=toList->numParam;
	request->to->address=NULL;
	request->to->paramList=NULL;
	if(toList->address != NULL)
		request->to->address=sipAddrDup(toList->address,toList->numAddr);
	if(toList->paramList != NULL)
		request->to->paramList=sipParameterDup(toList->paramList,toList->numParam);
	return 1;
}

int UserAgentReqDup(SipReq request,SipUserAgent* agent)
{
	if(request->useragent != NULL)
		UserAgentFree(request->useragent);
	request->useragent=(SipUserAgent*)calloc(1,sizeof(SipUserAgent));
	request->useragent->data=NULL;
	request->useragent->numdata=agent->numdata;
	if(agent->data != NULL)
		request->useragent->data=sipStrDup(agent->data,agent->numdata);
	return 1;
}

int ViaReqDupLink(SipReq request,SipVia* visList)
{
	SipViaParm *tmpViaPara,*firstViaPara,*lastViaPara;
/*	This will make via header sequence reverse */
/*	origViaPara=request->via->ParmList;
	origViaLast=request->via->last;

	tmpViaPara=sipViaParmDup(visList->ParmList, visList->numParams);
	firstViaPara=tmpViaPara;
	while(tmpViaPara != NULL){
		lastViaPara=tmpViaPara;
		tmpViaPara=tmpViaPara->next;
	}

	if(origViaPara != NULL){
		request->via->numParams=request->via->numParams+ visList->numParams;
		request->via->ParmList=firstViaPara;
		request->via->last=origViaLast;
		lastViaPara->next=origViaPara;
	}else{
		request->via->ParmList=firstViaPara;
		request->via->numParams=visList->numParams;
		request->via->last=lastViaPara;
	}*/

	tmpViaPara=sipViaParmDup(visList->ParmList, visList->numParams);
	firstViaPara=tmpViaPara;
	lastViaPara=tmpViaPara;
	
	while(tmpViaPara != NULL){
		lastViaPara=tmpViaPara;
		tmpViaPara=tmpViaPara->next;
	}
	
	if (request->via != NULL){
		request->via->numParams=request->via->numParams+ visList->numParams;
		request->via->last->next=firstViaPara;
		request->via->last=lastViaPara;
	}else{
		request->via=(SipVia*)calloc(1,sizeof(SipVia));
		request->via->numParams = visList->numParams;
		request->via->ParmList=firstViaPara;
		request->via->last=lastViaPara;
	}

	return 1;
}


#ifdef ICL_IMS
/* P-Media-Authorization */
int	PMAReqDup(SipReq request,unsigned char* str)
{
	if(request->pmediaauthorization != NULL)
		free(request->pmediaauthorization);
	request->pmediaauthorization=(unsigned char*)strDup((char*)str);
	return 1;
}
 
/* Path */
int	PathReqDupLink(SipReq request,SipPath* path)
{
	SipPath *tmpR;
	SipRecAddr *newRAddr,*oldRAddr;
	if(request->path == NULL){
		request->path=(SipPath*)calloc(1,sizeof(SipPath));
		request->path->numNameAddrs=path->numNameAddrs;
		request->path->sipNameAddrList=NULL;
		if(path->sipNameAddrList != NULL)
			request->path->sipNameAddrList=sipRecAddrDup(path->sipNameAddrList,path->numNameAddrs);
	}else{
		if(path->sipNameAddrList != NULL){
			newRAddr=sipRecAddrDup(path->sipNameAddrList,path->numNameAddrs);
			tmpR=request->path;
			oldRAddr=tmpR->sipNameAddrList;
			while(oldRAddr->next!= NULL)
				oldRAddr=oldRAddr->next;
			oldRAddr->next=newRAddr;
			request->path->numNameAddrs+=path->numNameAddrs;
		}
	}
	return 1;
}

/* P-Access-Network-Info */
int	PANIReqDup(SipReq request,unsigned char* str)
{
	if(request->paccessnetworkinfo != NULL)
		free(request->paccessnetworkinfo);
	request->paccessnetworkinfo=(unsigned char*)strDup((char*)str);
	return 1;
}

/* P-Called-Party-ID */
int	PCPIReqDup(SipReq request,unsigned char* str)
{
	if(request->pcalledpartyid != NULL)
		free(request->pcalledpartyid);
	request->pcalledpartyid=(unsigned char*)strDup((char*)str);
	return 1;
}

/* P-Charging-Function-Address */
int	PCFAReqDup(SipReq request,unsigned char* str)
{
	if(request->pchargingfunctionaddress != NULL)
		free(request->pchargingfunctionaddress);
	request->pchargingfunctionaddress=(unsigned char*)strDup((char*)str);
	return 1;
}

/* P-Charging-Vector */
int	PCVReqDup(SipReq request,unsigned char* str)
{
	if(request->pchargingvector != NULL)
		free(request->pchargingvector);
	request->pchargingvector=(unsigned char*)strDup((char*)str);
	return 1;
}

/* P-Visited-Network-ID */
int	PVNIReqDup(SipReq request,unsigned char* str)
{
	if(request->pvisitednetworkid != NULL)
		free(request->pvisitednetworkid);
	request->pvisitednetworkid=(unsigned char*)strDup((char*)str);
	return 1;
}

/* Security-Client */
int	SecurityClientReqDup(SipReq request,unsigned char* str)
{
	if(request->securityclient != NULL)
		free(request->securityclient);
	request->securityclient=(unsigned char*)strDup((char*)str);
	return 1;
}

/* Security-Verify */
int	SecurityVerifyReqDup(SipReq request,unsigned char* str)
{
	if(request->securityverify != NULL)
		free(request->securityverify);
	request->securityverify=(unsigned char*)strDup((char*)str);
	return 1;
}

#endif /* end of ICL_IMS */

/*---------------------------------------------------------------*/
/* This function is input a string buffer, parsing the buffer and filled into request struct */
int sipReqParse(SipReq request, unsigned char* msg, int* length)
{
	MsgHdr hdr,tmp;
	MsgBody _pby;
	int hdrlength,linelength=0;
	unsigned char *requestline, *request_URI, *version, *lineend;
	void *ret;
	SipReqLine *reqline;
	DxStr dxBuf;
	SipRetMsg sipCode=SIP_OK;

	hdrlength=0;
	/* parse Request-URI */
	lineend = (unsigned char*)strstr((const char*)msg,"\n");
	if (lineend == NULL)
		return SIP_REQUESTLINE_ERROR;
	else linelength = lineend - msg + 1;
		*lineend = '\0';	
	*(lineend-1) = ( *(lineend-1)=='\r' )?0:*(lineend-1);

	/*requestline = (unsigned char *)malloc(linelength); *//* temporary use */
	requestline = (unsigned char *)calloc(linelength,sizeof(char));

	strncpy((char*)requestline,(const char*) msg, linelength);
	request_URI = (unsigned char*)strstr((const char*)requestline," ");
	if (request_URI == NULL)
		return SIP_REQUESTURL_NULL;
	else {
		*request_URI = '\0';
		request_URI++;
	};
	version = (unsigned char*)strstr((const char*)request_URI, " ");
	if (version == NULL)
		return SIP_VERSION_ERROR;
	else{
		*version = '\0';
		version++;
	};

	reqline=(SipReqLine*)calloc(1,sizeof(SipReqLine));
	reqline->iMethod=(SipMethodType)((int)sipMethodNameToType((char*)requestline));
	reqline->ptrRequestURI = strDup((const char*)request_URI);
	reqline->ptrSipVersion = strDup((const char*)version);

	sipReqSetReqLine(request,reqline);
	free(reqline->ptrRequestURI);
	free(reqline->ptrSipVersion);
	free(reqline);
	free(requestline);

	/* parse header */
	hdr = msgHdrNew(); /* temporary create, free later */
	msg = lineend + 1;
	if( rfc822_parse(msg, &hdrlength, hdr)==0 )
		return SIP_RFC822PARSE_ERROR;
	tmp = hdr;

	/* set request hdrlist 2004/11/22 */
#ifdef CCLSIP_HDRLIST
	request->hdrlist=hdr;
#endif

	while (1){
		if ((tmp == NULL) || ((tmp->name)==NULL) ){
			/*TCRPrint(1,"<sipReqParse> Request Message Parse Error!");
			bcorrect=FALSE;*/
			break;
		}else{
			SipHdrType hdrtype=sipHdrNameToType(tmp->name);
			/* save hdrtype to hdrlist */
			tmp->hdrtype=hdrtype;
			if(hdrtype==UnknSipHdr){
				/*unknown header, keep it in unhdr buffer */
				/*sipCode=SIP_UNKNOWN_HEADER;*/
				TCRPrint(1,"<sipReqParse> Get a unKnown header:%s\n!",tmp->name);
				dxBuf=dxStrNew();
				if(request->unHdr == NULL){
					msgHdrAsStr(tmp,dxBuf);
					request->unHdr=strDup((const char*)dxStrAsCStr(dxBuf));
				}else{
					dxStrCat(dxBuf,(const char*)request->unHdr);
					free(request->unHdr);
					msgHdrAsStr(tmp,dxBuf);
					request->unHdr=strDup((const char*)dxStrAsCStr(dxBuf));
				}
				dxStrFree(dxBuf);

			}else if( RFC822ToSipHeader(tmp, hdrtype, &ret)==0 ){
				/*should not return Parse Error, keep it in original, and give an error code */
				sipCode= sipParseErrMsg(hdrtype);
				dxBuf=dxStrNew();
				if(request->unHdr == NULL){
					msgHdrAsStr(tmp,dxBuf);
					request->unHdr=strDup((const char*)dxStrAsCStr(dxBuf));
				}else{
					dxStrCat(dxBuf,(const char*)request->unHdr);
					free(request->unHdr);
					msgHdrAsStr(tmp,dxBuf);
					request->unHdr=strDup((const char*)dxStrAsCStr(dxBuf));
				}
				dxStrFree(dxBuf);
				/*goto REQUEST_PARSE_ERROR; */
			}else {
				sipReqAddHdrEx(request,hdrtype,ret);
				if((hdrtype==Content_Length)
					||(hdrtype==Max_Forwards)
					||(hdrtype==Min_Expires)
					||(hdrtype==RSeq)
					||(hdrtype==Content_Length_Short))
					free(ret);
			}
			tmp = tmp->next;
		}/*end else loop */
	}/*end while loop */

  /* extract body */
  /*if(bcorrect)*/
	if((request->contentlength > 0)&&(hdrlength>0)){
		_pby = (MsgBody)calloc(1,sizeof(*_pby));
		rfc822_extractbody(msg+hdrlength, hdr, _pby);
		/*sipReqSetBody(request,(SipBody*)_pby);*/
		request->reqflag=TRUE;
		request->body = (SipBody*)_pby;
		*length = hdrlength+_pby->length;
		/* mark by tyhuang 2005/7/21
		free(_pby->content);
		free(_pby);
		*/
	}
	/*msgHdrFree(hdr); mark by tyhuang 2004/11/22 */
#ifndef CCLSIP_HDRLIST
	msgHdrFree(hdr);
#endif
	return sipCode;

/*REQUEST_PARSE_ERROR:
	msgHdrFree(hdr);
	return 0;*/
}

int CountReqLineLen(SipReq request)
{
	int reqLineLength=0;

	/* We had add the two white space & CRCF space, so....+4 */
	if(request->requestline!=NULL)
		reqLineLength=strlen(request->requestline->ptrRequestURI)+\
				strlen(request->requestline->ptrSipVersion)+\
				strlen(sipMethodTypeToName(request->requestline->iMethod))+6;
	return reqLineLength;

}


int sipReqAsStr(SipReq request, DxStr ReqBuf, int *length)
{
  /*unsigned char *tmpbuf;*/
  int tmp, lenstr;
  SipReqLine *reqline;
  unsigned char *HeaderBuf;
  DxStr RequestStr;
  int Len=2048;
  
  /*HeaderBuf=(unsigned char*)malloc(Len);*/
  HeaderBuf=(unsigned char*)calloc(Len,sizeof(unsigned char));
  RequestStr=ReqBuf;
  reqline=sipReqGetReqLine(request);
  if(reqline != NULL){
	  dxStrCat(RequestStr,(const char*)sipMethodTypeToName(reqline->iMethod));
	  dxStrCat(RequestStr," ");
	  dxStrCat(RequestStr,reqline->ptrRequestURI);
	  dxStrCat(RequestStr," ");
	  dxStrCat(RequestStr,reqline->ptrSipVersion);
	  dxStrCat(RequestStr,"\r\n");
  }

  *length=dxStrLen(RequestStr);
  /*tmpbuf=dxStrAsCStr(RequestStr);*/
  lenstr=*length;
  if (request->accept != NULL) {
	AcceptAsStr(request->accept,HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(const char*)HeaderBuf);
	lenstr = lenstr + tmp;
  }
  if (request->acceptencoding != NULL) {
	AcceptEncodingAsStr(request->acceptencoding, HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if (request->acceptlanguage != NULL) {
	AcceptLanguageAsStr(request->acceptlanguage, HeaderBuf, Len, &tmp);
      	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if(request->allow != NULL){
	  AllowAsStr(request->allow,HeaderBuf,Len,&tmp);
	  dxStrCat(RequestStr,(char*)HeaderBuf);
	  lenstr=lenstr+tmp;
  }
  if(request->allowevents != NULL){
	  AllowEventsAsStr(request->allowevents,HeaderBuf,Len,&tmp);
	  dxStrCat(RequestStr,(char*)HeaderBuf);
	  lenstr+=tmp;
  }
  if (request->authorization != NULL) {
	AuthorizationAsStr(request->authorization, HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if(request->alertinfo != NULL){
	AlertInfoAsStr(request->alertinfo,HeaderBuf,Len,&tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if (request->callid != NULL) {
	CallIDAsStr(request->callid, HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if(request->callinfo != NULL){
	CallInfoAsStr(request->callinfo,HeaderBuf,Len,&tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if(request->classpublish != NULL){
	ClassAsStr(request->classpublish,HeaderBuf,Len,&tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if (request->contact != NULL) {
	sipContactAsStr(request->contact, HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if (request->contentdisposition!= NULL) {
	ContentDispositionAsStr(request->contentdisposition, HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if (request->contentencoding != NULL) {
	ContentEncodingAsStr(request->contentencoding, HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if (request->contentlanguage != NULL) {
	ContentLanguageAsStr(request->contentlanguage, HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if (request->contentlength != -1) {
	ContentLengthAsStr(request->contentlength,HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if (request->contenttype != NULL) {
	ContentTypeAsStr(request->contenttype,HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if (request->cseq != NULL) {
	CSeqAsStr(request->cseq,HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if (request->date != NULL) {
	DateAsStr(request->date,HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if (request->encryption != NULL) {
	EncryptionAsStr(request->encryption,HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if (request->event !=NULL){
	  EventAsStr(request->event,HeaderBuf,Len,&tmp);
	  dxStrCat(RequestStr,(char*)HeaderBuf);
	  lenstr=lenstr+tmp;
  }
  if (request->expires != NULL) {
	ExpiresAsStr(request->expires,HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if (request->from != NULL) {
	FromAsStr(request->from,HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if (request->hide != unknown_hide) {
	HideAsStr(request->hide,HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if (request->inreplyto != NULL) {
	InReplyToAsStr(request->inreplyto,HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if (request->maxforwards != -1) {
	MaxForwardsAsStr(request->maxforwards, HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if (request->minse != NULL) {
	MinSEAsStr(request->minse,  HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if (request->organization != NULL) {
	OrganizationAsStr(request->organization,HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if(NULL != request->passertedidentity){
	PAssertedIdentityAsStr(request->passertedidentity,HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if(NULL != request->ppreferredidentity){
	PPreferredIdentityAsStr(request->ppreferredidentity,HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if (request->priority != Unkn_priority) {
	PriorityAsStr(request->priority,HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if(request->privacy != NULL) {
	PrivacyAsStr(request->privacy,HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if (request->proxyauthorization != NULL) {
	ProxyAuthorizationAsStr(request->proxyauthorization,HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if (request->proxyrequire != NULL) {
	ProxyRequireAsStr(request->proxyrequire,HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if(request->rack != NULL){
	RAckAsStr(request->rack,HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if (request->recordroute != NULL) {
	RecordRouteAsStr(request->recordroute,HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if (request->require != NULL) {
	RequireAsStr(request->require,HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if(request->referto != NULL){
	  ReferToAsStr(request->referto,HeaderBuf,Len, &tmp);
	  dxStrCat(RequestStr,(char*)HeaderBuf);
	  lenstr=lenstr+tmp;
  }
  if(request->referredby != NULL){
	  ReferredByAsStr(request->referredby,HeaderBuf,Len, &tmp);
	  dxStrCat(RequestStr,(char*)HeaderBuf);
	  lenstr=lenstr+tmp;
  }
  if(request->remotepartyid != NULL){
	  RemotePartyIDAsStr(request->remotepartyid,HeaderBuf,Len,&tmp);
	  dxStrCat(RequestStr,(char*)HeaderBuf);
	  lenstr=lenstr+tmp;
  }
  if(request->replaces != NULL){
	  ReplacesAsStr(request->replaces,HeaderBuf,Len,&tmp);
	  dxStrCat(RequestStr,(char*)HeaderBuf);
	  lenstr=lenstr+tmp;
  }
  if(request->replyto != NULL){
	  ReplyToAsStr(request->replyto,HeaderBuf,Len,&tmp);
	  dxStrCat(RequestStr,(char*)HeaderBuf);
	  lenstr=lenstr+tmp;
  }
  if (request->responsekey != NULL) {
	ResponseKeyAsStr(request->responsekey,HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if (request->retryafter != NULL) {
	RetryAfterAsStr(request->retryafter,HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if (request->route != NULL) {
	RouteAsStr(request->route, HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if(request->sessionexpires != NULL){
	  SessionExpiresAsStr(request->sessionexpires,HeaderBuf,Len,&tmp);
	  dxStrCat(RequestStr,(char*)HeaderBuf);
	  lenstr=lenstr+tmp;
  }
  if(request->sipifmatch != NULL){
	  SIPIfMatchAsStr(request->sipifmatch,HeaderBuf,Len,&tmp);
	  dxStrCat(RequestStr,(char*)HeaderBuf);
	  lenstr=lenstr+tmp;
  }
  if (request->subject != NULL) {
	SubjectAsStr(request->subject,HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if (request->subscriptionstate != NULL){
	  SubscriptionStateAsStr(request->subscriptionstate,HeaderBuf,Len,&tmp);
	  dxStrCat(RequestStr,(char*)HeaderBuf);
	  lenstr+=tmp;
  }
  if (request->supported != NULL){
	  SupportedAsStr(request->supported,HeaderBuf,Len,&tmp);
	  dxStrCat(RequestStr,(char*)HeaderBuf);
	  lenstr+=tmp;
  }
  if (request->timestamp != NULL) {
	TimestampAsStr(request->timestamp,HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if (request->to != NULL) {
	ToAsStr(request->to, HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if (request->useragent != NULL) {
	UserAgentAsStr(request->useragent, HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }
  if (request->via != NULL) {
	ViaAsStr(request->via,HeaderBuf, Len, &tmp);
	dxStrCat(RequestStr,(char*)HeaderBuf);
	lenstr = lenstr+tmp;
  }

#ifdef ICL_IMS
  if(request->pmediaauthorization != NULL){
	  if(PMAAsStr(request->pmediaauthorization,HeaderBuf, Len, &tmp)!=0){
		dxStrCat(RequestStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	  }
  }
  if (request->path != NULL) {
	  if(PathAsStr(request->path,HeaderBuf, Len, &tmp)!=0){
		dxStrCat(RequestStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	  }
  }
  if (request->paccessnetworkinfo != NULL) {
	  if(PANIAsStr(request->paccessnetworkinfo,HeaderBuf, Len, &tmp)!=0){
		dxStrCat(RequestStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	  }
  }
  if (request->pcalledpartyid != NULL) {
	  if(PCPIAsStr(request->pcalledpartyid,HeaderBuf, Len, &tmp)!=0){
		dxStrCat(RequestStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	  }
  }
  if (request->pchargingfunctionaddress != NULL) {
	  if(PCFAAsStr(request->pchargingfunctionaddress,HeaderBuf, Len, &tmp)!=0){
		dxStrCat(RequestStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	  }
  }
  if (request->pvisitednetworkid != NULL) {
	  if(PVNIAsStr(request->pvisitednetworkid,HeaderBuf, Len, &tmp)!=0){
		dxStrCat(RequestStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	  }
  }
  if (request->securityclient != NULL) {
	  if(SecurityClientAsStr(request->securityclient,HeaderBuf, Len, &tmp)!=0){
		dxStrCat(RequestStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	  }
  }
  if (request->securityverify != NULL) {
	  if(SecurityVerifyAsStr(request->securityverify,HeaderBuf, Len, &tmp)!=0){
		dxStrCat(RequestStr,(char*)HeaderBuf);
		lenstr = lenstr+tmp;
	  }
  }
#endif /* end of ICL_IMS */	

  if(request->unHdr != NULL){
	  dxStrCat(RequestStr,(const char*)request->unHdr);
	  lenstr=lenstr+strlen(request->unHdr);
  }
  
  /* print  a new line */
  dxStrCat(RequestStr,"\r\n");

  /* print body */
  if(request->body != NULL) {
	  if(request->body->length > 0){
		  dxStrCat(RequestStr,(char*)request->body->content);
		  /*dxStrCat(RequestStr,"\r\n"); marked by ljchuang 08/19/2003 */
	  }
  }

  dxStrCat(RequestStr,"\0");
  *length=dxStrLen(RequestStr);
  free(HeaderBuf);

  return 1;
}/*end sipRequestAsStr */


/* Add/delete  Via header for request message */
CCLAPI 
RCODE sipReqAddViaHdrTop(IN SipReq _this, char* _via)
{
	MsgHdr hdr,tmp;
	void *ret;
	char *tmpbuf;
	SipVia *newVia;
	SipViaParm *origViaPara,*origViaLast,*tmpViaPara,*firstViaPara,*lastViaPara;

	RCODE rCode=RC_OK;

	_this->reqflag=TRUE;
	/* parse new via header */
	newVia=NULL;
	/*tmpbuf=(char*)malloc(strlen(_via)+2);*/
	tmpbuf=(char*)calloc(strlen(_via)+2,sizeof(char));
	strcpy(tmpbuf,_via);
	hdr=msgHdrNew();
	rfc822_parseline((unsigned char*)tmpbuf,strlen(_via),hdr);
	tmp=hdr;
	if((tmp!=NULL)&&((tmp->name)!=NULL)){
		SipHdrType hdrtype=sipHdrNameToType(tmp->name);
		if(hdrtype!=Via)
			rCode=RC_ERROR;
		
		if(RFC822ToSipHeader(tmp,hdrtype,&ret) !=1){
			rCode=RC_ERROR;
		}else{
			newVia=(SipVia*)ret;
		}
	}else
		rCode=RC_ERROR;

	msgHdrFree(hdr);
	free(tmpbuf);

	/* add new via header into Top most via header*/
	if(newVia != NULL){
		if(_this->via == NULL){
			_this->via=newVia;
		}else{
			origViaPara=_this->via->ParmList;
			origViaLast=_this->via->last;

			tmpViaPara=newVia->ParmList;
			firstViaPara=tmpViaPara;
			while(tmpViaPara != NULL){
				lastViaPara=tmpViaPara;
				tmpViaPara=tmpViaPara->next;
			}
			if(origViaPara != NULL){
				_this->via->numParams=_this->via->numParams+ newVia->numParams;
				_this->via->ParmList=firstViaPara;
				_this->via->last=origViaLast;
				lastViaPara->next=origViaPara;
			}else{
				_this->via->ParmList=firstViaPara;
				_this->via->numParams=newVia->numParams;
				_this->via->last=lastViaPara;
			}
			/*sam add:*/
			newVia->ParmList = NULL;
			newVia->last = NULL;
			free(newVia);
		}
	}else
		rCode = RC_ERROR;

	return rCode;
}

CCLAPI 
RCODE sipReqDelViaHdrTop(IN SipReq _this)
{
	RCODE rCode = RC_OK;
	SipViaParm *tmpViaPara;

	if(_this->via != NULL){
		if(_this->via->ParmList != NULL){
			_this->reqflag=TRUE;
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

/* Add/delete Route header for request message */
/* pos = 0 : top; pos = 1:Last */
CCLAPI 
RCODE sipReqAddRouteHdr(IN SipReq _this,int pos, char* _route)
{
	MsgHdr hdr,tmp;
	void *ret;
	char *tmpbuf;
	SipRoute *newRoute;
	RCODE rCode=RC_OK;
	SipRoute *origRoute;
	SipRecAddr *origAddr,*tmpAddr;

	_this->reqflag=TRUE;
	/* parse new Route header */
	newRoute=NULL;
	/*tmpbuf=(char*)malloc(strlen(_route)+2);*/
	tmpbuf=(char*)calloc(strlen(_route)+2,sizeof(char));
	strcpy(tmpbuf,_route);
	hdr=msgHdrNew();
	rfc822_parseline((unsigned char*)tmpbuf,strlen(_route),hdr);
	tmp=hdr;
	if((tmp!=NULL)&&((tmp->name)!=NULL)){
		SipHdrType hdrtype=sipHdrNameToType(tmp->name);
		/* modified by tyhuang 2004.2.27 */
		if(hdrtype!= Route){
			rCode=RC_ERROR;
		}
		if(RFC822ToSipHeader(tmp,hdrtype,&ret) != 1){
			rCode=RC_ERROR;
		}else{
			newRoute=(SipRoute*)ret;
		}
	}else
		rCode=RC_ERROR;

	msgHdrFree(hdr);
	free(tmpbuf);

	/* Add into Route header */
	if(rCode != RC_ERROR){
		if(newRoute != NULL){
			if(_this->route == NULL){
				/* top and last is same */
				_this->route=newRoute;
			}else{ /* there exist a Route header */
				origRoute=_this->route;
				origAddr=origRoute->sipNameAddrList;
				origRoute->numNameAddrs=origRoute->numNameAddrs+newRoute->numNameAddrs;
				if(pos==0){ /*mean top */
					_this->route->sipNameAddrList=newRoute->sipNameAddrList;
					tmpAddr=newRoute->sipNameAddrList;
					/* get the last Addr */
					while(tmpAddr->next != NULL){
						tmpAddr=tmpAddr->next;
					}
					tmpAddr->next=origAddr;

				}else if(pos==1){ /*mean last */
					tmpAddr=origRoute->sipNameAddrList;
					if(tmpAddr){
						while(tmpAddr->next!=NULL){
							tmpAddr=tmpAddr->next;
						}
					}
					tmpAddr->next=newRoute->sipNameAddrList;
				}else
					rCode=RC_ERROR;

				free(newRoute);
			}		
			/* free(newRoute);*/
		}else
			rCode=RC_ERROR;
	}
	return rCode;
}

CCLAPI 
RCODE sipReqDelRouteHdrTop(IN SipReq _this)
{
	RCODE rCode = RC_OK;
	SipRoute *origRoute;
	SipRecAddr *origAddr,*tmpAddr;

	if(_this->route != NULL){
		if(_this->route->sipNameAddrList != NULL){
			_this->reqflag = TRUE;
			origRoute=_this->route;
			origRoute->numNameAddrs--;
			origAddr=origRoute->sipNameAddrList;
			tmpAddr=origRoute->sipNameAddrList->next;
			origRoute->sipNameAddrList=tmpAddr;
			origAddr->next=NULL;
			sipRecAddrFree(origAddr);
			/* remove empty route */
			if (0==origRoute->numNameAddrs) {
				RouteFree(_this->route);
				_this->route=NULL;
			}
		}else
			rCode=RC_ERROR;

	}else
		rCode=RC_ERROR;

	return rCode;
}

CCLAPI 
RCODE sipReqSetMaxForward(IN SipReq request,IN unsigned int maxforward)
{
	if (!request) 
		return RC_ERROR;
	request->maxforwards=maxforward;
	return RC_OK;
}


CCLAPI 
RCODE sipReqDelHdr(IN SipReq request,IN SipHdrType hdrtype)
{
	if (!request || (hdrtype>UnknSipHdr)) 
		return RC_ERROR;

	switch(hdrtype) {
	case Authorization:
		AuthorizationFree(request->authorization);
		request->authorization=NULL;
		break;
	case Contact_Short:
	case Contact:
		sipContactFree(request->contact);
		request->contact = NULL;
		break;
	case Content_Length:
	case Content_Length_Short:
		request->contentlength=0;
		break;
	case Max_Forwards:
		request->maxforwards=0;
		break;
	case P_Asserted_Identity:
		PAssertedIdentityFree(request->passertedidentity);
		request->passertedidentity=NULL;
		break;
	case P_Preferred_Identity:
		PPreferredIdentityFree(request->ppreferredidentity);
		request->ppreferredidentity=NULL;
		break;
	case Priority:
		request->priority = Unkn_priority;
		break;
	case Proxy_Authorization:
		ProxyAuthorizationFree(request->proxyauthorization);
		request->proxyauthorization=NULL;
		break;
	case Record_Route:
		RecordRouteFree(request->recordroute);
		request->recordroute=NULL;
		break;
	case Remote_Party_ID:
		RemotePartyIDFree(request->remotepartyid);
		request->remotepartyid=NULL;
		break;
	case Replaces:
		ReplacesFree(request->replaces);
		request->replaces=NULL;
		break;
	case Route:
		RouteFree(request->route);
		request->route=NULL;
		break;;
	case Via_Short:
	case Via:
		ViaFree(request->via);
		request->via=NULL;
		break;
	case UnknSipHdr:
		free(request->unHdr);
		request->unHdr=NULL;
		break;
	default:
		sipReqAddHdrEx(request,hdrtype,NULL);
		break;
	}
	return RC_OK;
}

/***********************************************************************
*
*	Authorization  ( Digest and Basic )
*							
***********************************************************************/
RCODE			
sipReqSetBasicAuthz(SipReq request,SipAuthzType type,char* user_id,char* password)
{
	/*int		i = 0;*/
	char		source[1024] = {"\0"};
	char		target[1024] = {"\0"};
	SipParam	*piter = NULL;
	SipParam	*ptemp = NULL;

	if( !request || !user_id || !password )	return RC_ERROR;

	if( type==SIP_AUTHZ ) {
		if( !request->authorization ) {
			request->authorization = (SipAuthz*)calloc(1,sizeof(SipAuthz));
			request->authorization->numParams = 0;
			request->authorization->scheme = NULL;
			request->authorization->sipParamList = NULL;
		}
		if( request->authorization->scheme ) 
			free(request->authorization->scheme);
		request->authorization->scheme = strDup("Basic");
	}
	else if( type==SIP_PROXY_AUTHZ ) {
		if( !request->proxyauthorization ) {
			request->proxyauthorization = (SipPxyAuthz*)calloc(1,sizeof(SipPxyAuthz));
			request->proxyauthorization->numParams = 0;
			request->proxyauthorization->scheme = NULL;
			request->proxyauthorization->sipParamList = NULL;
		}
		if( request->proxyauthorization->scheme ) 
			free(request->proxyauthorization->scheme);
		request->proxyauthorization->scheme = strDup("Basic");
	}else
		return RC_ERROR;

	strcat(source,user_id); strcat(source,":"); strcat(source,password);
	base64encode( source, target, 1024);

	if( type==SIP_AUTHZ ) 
		for( piter=request->authorization->sipParamList; (piter); ) {
			ptemp = piter->next;	free(piter);
			piter = ptemp;
		}
	else 
		for( piter=request->proxyauthorization->sipParamList; (piter); ) {
			ptemp = piter->next;	free(piter);
			piter = ptemp;
		}
	piter = (SipParam*)calloc(1,sizeof(SipParam));
	/*piter->value = strDup(target);*/
	piter->name = strDup(target);
	piter->value = NULL;
	piter->next = NULL;

	if( type==SIP_AUTHZ ) {
		request->authorization->sipParamList = piter;
		request->authorization->numParams = 1;
	}
	else {
		request->proxyauthorization->sipParamList = piter;
		request->proxyauthorization->numParams = 1;
	}
	return RC_OK;
}

RCODE			
sipReqGetBasicAuthz(SipReq request,SipAuthzType type,char* user_id, int ulen,char* password, int plen)
{
	char	target[1024] = {"\0"};
	char	user[1024] = {"\0"};
	char	pwd[1024] = {"\0"};
	char	temp[1024];

	if( !request || !user_id || !password )		return RC_ERROR;
	if( type==SIP_AUTHZ ) {
		if( !request->authorization )			return RC_ERROR;
		if( !request->authorization->sipParamList )	return RC_ERROR;
		if( !request->authorization->scheme )		return RC_ERROR;
		if( strICmp(request->authorization->scheme,"Basic")!=0 )
			return RC_ERROR;

		base64decode(request->authorization->sipParamList->name,target,1024);
	}
	else if( type==SIP_PROXY_AUTHZ ) {
		if( !request->proxyauthorization )			return RC_ERROR;
		if( !request->proxyauthorization->sipParamList )	return RC_ERROR;
		if( !request->proxyauthorization->scheme )		return RC_ERROR;
		if( strICmp(request->proxyauthorization->scheme,"Basic")!=0 )
			return RC_ERROR;

		base64decode(request->proxyauthorization->sipParamList->name,target,1024);
	}
	else
		return RC_ERROR;

	strcpy(temp,target);
	strcpy(user,strtok(temp,":"));
	strcpy(pwd,strtok(NULL,":"));

	if( (int)strlen(user)>=ulen && (int)strlen(pwd)>=plen ) 
		return RC_ERROR;
	strcpy(user_id,user);
	strcpy(password,pwd);
	
	return RC_OK;
}


RCODE			
sipReqSetDigestAuthz(SipReq request,SipAuthzType type,SipDigestAuthz auth)
{
	/*int		i = 0;*/
	SipPxyAuthz	*lastPxyAuthz,*newPxyAuthz;
	SipAuthz	*lastAuthz, *newAuthz;
	SipParam	*piter = NULL;
	SipParam	*ptemp = NULL;
	int		*numParm;
	char		*tmp;

	if( !request )	return RC_ERROR;

	if( type==SIP_AUTHZ ) {
		lastAuthz =request->authorization;
		newAuthz=(SipAuthz*)calloc(1,sizeof(SipAuthz));
		newAuthz->next=NULL;
		newAuthz->numParams=0;
		newAuthz->scheme=NULL;
		newAuthz->sipParamList=NULL;
		if(lastAuthz != NULL){
			while(lastAuthz->next != NULL){
				lastAuthz=lastAuthz->next;
			}
			lastAuthz->next=newAuthz;

		}else{
			request->authorization=newAuthz;
			lastAuthz=newAuthz;
		}
		newAuthz->scheme = strDup("Digest");

		newAuthz->numParams = 0;
		numParm = &(newAuthz->numParams);
	}
	else if( type==SIP_PROXY_AUTHZ ) {
		lastPxyAuthz=request->proxyauthorization;
		newPxyAuthz = (SipPxyAuthz*)calloc(1,sizeof(SipPxyAuthz));
		newPxyAuthz->numParams = 0;
		newPxyAuthz->scheme = NULL;
		newPxyAuthz->sipParamList = NULL;
		newPxyAuthz->next = NULL;
		if(lastPxyAuthz != NULL){
			while(lastPxyAuthz->next !=NULL){
				lastPxyAuthz=lastPxyAuthz->next;
			}
			lastPxyAuthz->next=newPxyAuthz;
		}else{
			request->proxyauthorization=newPxyAuthz;
			lastPxyAuthz=newPxyAuthz;
		}
		newPxyAuthz->scheme = strDup("Digest");

		newPxyAuthz->numParams = 0;
		numParm = &(newPxyAuthz->numParams);
	}
	else
		return RC_ERROR;

	/* set username */
	piter = (SipParam*)calloc(1,sizeof(SipParam));
	piter->name = strDup("username");
	/*piter->value = strDup(quote(auth.username_)); modified by tyhuang 2003.09.23 */
	tmp=strDup(auth.username_);
	tmp=realloc(tmp,strlen(tmp)+3);
	piter->value = quote(tmp);
	piter->next = NULL;
	if( type==SIP_AUTHZ ) 
		/*request->authorization->sipParamList = piter;*/
		newAuthz->sipParamList =piter;
	else
		newPxyAuthz->sipParamList = piter;
	/*request->authorization->numParams++; */
	(*numParm)++;

	/* set realm */
	ptemp = (SipParam*)calloc(1,sizeof(SipParam));
	ptemp->name = strDup("realm");
	/*ptemp->value = strDup(quote(auth.realm_));modified by tyhuang 2003.09.23 */
	tmp=strDup(auth.realm_);
	tmp=realloc(tmp,strlen(tmp)+3);
	ptemp->value=quote(tmp);
	ptemp->next = NULL;
	piter->next = ptemp;
	piter = piter->next;
	/*request->authorization->numParams++;*/
	(*numParm)++;

	/* set nonce */
	ptemp = (SipParam*)calloc(1,sizeof(SipParam));
	ptemp->name = strDup("nonce");
	/*ptemp->value = strDup(quote(auth.nonce_));modified by tyhuang 2003.09.23 */
	tmp=strDup(auth.nonce_);
	tmp=realloc(tmp,strlen(tmp)+3);
	ptemp->value=quote(tmp);
	ptemp->next = NULL;
	piter->next = ptemp;
	piter = piter->next;
	/*request->authorization->numParams++; */
	(*numParm)++;

	/* set digest-uri */
	ptemp = (SipParam*)calloc(1,sizeof(SipParam));
	/*ptemp->name = strDup("digest-uri");*/
	ptemp->name = strDup("uri");
	/*ptemp->value = strDup(quote(auth.digest_uri_));modified by tyhuang 2003.09.23 */
	tmp=strDup(auth.digest_uri_);
	tmp=realloc(tmp,strlen(tmp)+3);
	ptemp->value=quote(tmp);
	ptemp->next = NULL;
	piter->next = ptemp;
	piter = piter->next;
	/*request->authorization->numParams++; */
	(*numParm)++;

	/* set response */
	ptemp = (SipParam*)calloc(1,sizeof(SipParam));
	ptemp->name = strDup("response");
	/*ptemp->value = strDup(quote(auth.response_));modified by tyhuang 2003.09.23 */
	tmp=strDup(auth.response_);
	tmp=realloc(tmp,strlen(tmp)+3);
	ptemp->value=quote(tmp);
	ptemp->next = NULL;
	piter->next = ptemp;
	piter = piter->next;
	/*request->authorization->numParams++; */
	(*numParm)++;

	/* set algorithm */
	if( auth.flag_&SIP_DAFLAG_ALGORITHM ) {
		ptemp = (SipParam*)calloc(1,sizeof(SipParam));
		ptemp->name = strDup("algorithm");
		ptemp->value = strDup(auth.algorithm_);
		ptemp->next = NULL;
		piter->next = ptemp;
		piter = piter->next;
		/*request->authorization->numParams++; */
		(*numParm)++;
	}

	/* set cnonce */
	if( auth.flag_&SIP_DAFLAG_CNONCE ) {
		ptemp = (SipParam*)calloc(1,sizeof(SipParam));
		ptemp->name = strDup("cnonce");
		/*ptemp->value = strDup(quote(auth.cnonce_));modified by tyhuang 2003.09.23 */
		tmp=strDup(auth.cnonce_);
		tmp=realloc(tmp,strlen(tmp)+3);
		ptemp->value=quote(tmp);
		ptemp->next = NULL;
		piter->next = ptemp;
		piter = piter->next;
		/*request->authorization->numParams++; */
		(*numParm)++;
	}

	/* set opaque */
	if( auth.flag_&SIP_DAFLAG_OPAQUE ) {
		ptemp = (SipParam*)calloc(1,sizeof(SipParam));
		ptemp->name = strDup("opaque");
		/*ptemp->value = strDup(quote(auth.opaque_));modified by tyhuang 2003.09.23 */
		tmp=strDup(auth.opaque_);
		tmp=realloc(tmp,strlen(tmp)+3);
		ptemp->value=quote(tmp);
		ptemp->next = NULL;
		piter->next = ptemp;
		piter = piter->next;
		/* request->authorization->numParams++;*/
		(*numParm)++;
	}

	/* set message-qop */
	if( auth.flag_&SIP_DAFLAG_MESSAGE_QOP ) {
		ptemp = (SipParam*)calloc(1,sizeof(SipParam));
		ptemp->name = strDup("qop");
		ptemp->value = strDup(auth.message_qop_);
		/*ptemp->value = strDup(quote(auth.message_qop_));modified by tyhuang 2003.09.23 */
		tmp=strDup(auth.message_qop_);
		tmp=realloc(tmp,strlen(tmp)+3);
		ptemp->value=quote(tmp);
		ptemp->next = NULL;
		piter->next = ptemp;
		piter = piter->next;
		/*request->authorization->numParams++; */
		(*numParm)++;
	}

	/* set nonce-count */
	if( auth.flag_ & SIP_DAFLAG_NONCE_COUNT ) {
		ptemp = (SipParam*)calloc(1,sizeof(SipParam));
		ptemp->name = strDup("nc");
		/*ptemp->value = strDup(quote(auth.nonce_count_));modified by tyhuang 2003.09.23 */
		tmp=strDup(auth.nonce_count_);
		tmp=realloc(tmp,strlen(tmp)+3);
		ptemp->value=quote(tmp);
		ptemp->next = NULL;
		piter->next = ptemp;
		piter = piter->next;
		/*request->authorization->numParams++; */
		(*numParm)++;
	}

	return RC_OK;
}

RCODE			
sipReqGetDigestAuthz(SipReq request,SipAuthzType type,SipDigestAuthz* auth)
{
	int		i = 0;
	SipParam	*piter = NULL;
	int		numParm;

	if( !request && !auth )				return RC_ERROR;
	if( type==SIP_AUTHZ ) {
		if( !request->authorization )			return RC_ERROR;
		if( !request->authorization->sipParamList )	return RC_ERROR;
		if( !request->authorization->scheme )		return RC_ERROR;
		if( strICmp(request->authorization->scheme,"Digest")!=0 )
			return RC_ERROR;

		auth->flag_ = SIP_DAFLAG_NONE;
		piter = request->authorization->sipParamList;
		numParm = request->authorization->numParams;
	}
	else if( type==SIP_PROXY_AUTHZ ) {
		if( !request->proxyauthorization )			return RC_ERROR;
		if( !request->proxyauthorization->sipParamList )	return RC_ERROR;
		if( !request->proxyauthorization->scheme )		return RC_ERROR;
		if( strICmp(request->proxyauthorization->scheme,"Digest")!=0 )
			return RC_ERROR;

		auth->flag_ = SIP_DAFLAG_NONE;
		piter = request->proxyauthorization->sipParamList;
		numParm = request->proxyauthorization->numParams;
	}
	else
		return RC_ERROR;

	for( i=0; (i<numParm)&&piter; i++, piter=piter->next) {
		if( strcmp(piter->name,"username")==0 ) {
			strcpy(auth->username_,unquote(piter->value));
			continue;
		}
		if( strcmp(piter->name,"realm")==0 ) {
			strcpy(auth->realm_,unquote(piter->value));
			continue;
		}
		if( strcmp(piter->name,"nonce")==0 ) {
			strcpy(auth->nonce_,unquote(piter->value));
			continue;
		}

		/*if( strcmp(piter->name,"digest-uri")==0 ) {*/
		if( strcmp(piter->name,"uri")==0 ) {
			strcpy(auth->digest_uri_,unquote(piter->value));
			continue;
		}
		if( strcmp(piter->name,"response")==0 ) {
			strcpy(auth->response_,unquote(piter->value));
			continue;
		}
		if( strcmp(piter->name,"algorithm")==0 ) {
			strcpy(auth->algorithm_,unquote(piter->value));
			auth->flag_ |= SIP_DAFLAG_ALGORITHM;
			continue;
		}
		if( strcmp(piter->name,"cnonce")==0 ) {
			strcpy(auth->cnonce_,unquote(piter->value));
			auth->flag_ |= SIP_DAFLAG_CNONCE;
			continue;
		}
		if( strcmp(piter->name,"opaque")==0 ) {
			strcpy(auth->opaque_,unquote(piter->value));
			auth->flag_ |= SIP_DAFLAG_OPAQUE;
			continue;
		}
		if( strcmp(piter->name,"qop")==0 ) {
			strcpy(auth->message_qop_,unquote(piter->value));
			auth->flag_ |= SIP_DAFLAG_MESSAGE_QOP;
			continue;
		}
		if( strcmp(piter->name,"nc")==0 ) {
			strcpy(auth->nonce_count_,unquote(piter->value));
			auth->flag_ |= SIP_DAFLAG_NONCE_COUNT;
			continue;
		}
	}
	return RC_OK;
}

/* marked by tyhuang
char* KD(char* a, char* b)
{
	unsigned char		md5_KD[16];
	static unsigned char	KD_Hex[33];
	MD5_CTX			context;
	unsigned int		len;
	int			i;
	char			temp[1024];

	if(!a||!b)	return NULL;

	temp[0] = 0;
	strcat(temp,unquote(a)); strcat(temp,":"); strcat(temp,unquote(b));

	len = strlen (temp);
	MD5Init (&context);
	MD5Update (&context,(unsigned char*)temp, len);
	MD5Final (md5_KD, &context);

	for( i=0; i<16; i++)
		sprintf((char*)(KD_Hex+2*i), "%02x", md5_KD[i]);
	KD_Hex[32] = 0;
	return (char*)KD_Hex;
}
*/

RCODE KD2(char* a, char* b,char* KD_Hex,int length)
{
	unsigned char		md5_KD[16];
	MD5_CTX			context;
	unsigned int		len;
	int			i;
	char			temp[1024];

	if(!a||!b)	return RC_ERROR;

	if(!KD_Hex ||(length<33)) return RC_ERROR;

	temp[0] = 0;
	strcat(temp,unquote(a)); strcat(temp,":"); strcat(temp,unquote(b));

	len = strlen (temp);
	MD5Init (&context);
	MD5Update (&context,(unsigned char*)temp, len);
	MD5Final (md5_KD, &context);

	for( i=0; i<16; i++)
		sprintf((char*)(KD_Hex+2*i), "%02x", md5_KD[i]);
	KD_Hex[32] = 0;
	return RC_OK;
}

/* marked by tyhuang
char* H(char* a)
{
	unsigned char	md5_H[16];
	static char	H_Hex[33];
	MD5_CTX		context;
	unsigned int	len;
	int		i;

	if( !a )	return NULL;

	len = strlen (a);
	MD5Init (&context);
	MD5Update (&context,(unsigned char*) a, len);
	MD5Final (md5_H, &context);

	for( i=0; i<16; i++)
		sprintf( (H_Hex+2*i), "%02x", md5_H[i]);
	H_Hex[32] = 0;
	return H_Hex;
}
*/

RCODE H2(char* a,char* H_Hex,int length)
{
	unsigned char	md5_H[16];
	MD5_CTX		context;
	unsigned int	len;
	int		i;

	if (!a) return RC_ERROR; 
	if( !H_Hex ||(length<33) )	return RC_ERROR;

	len = strlen (a);
	MD5Init (&context);
	MD5Update (&context,(unsigned char*) a, len);
	MD5Final (md5_H, &context);

	for( i=0; i<16; i++)
		sprintf( (H_Hex+2*i), "%02x", md5_H[i]);
	H_Hex[32] = 0;
	return RC_OK;
}

/* PS. The entity_body field is only needed when (auth->message_qop_ == "auth-init") */
CCLAPI RCODE
sipDigestAuthzGenRspKey(
	SipDigestAuthz* auth, 
	char*	password,
	char*	method,		/* request method */
	char*	entity_body	/* 32-Byte char array */
	)
{
	char	A1[1024];
	char	A2[1024];
	char	temp[1024],temp1[33]={'\0'};

	if( !auth || !password || !method )	return RC_ERROR;

	/* A1 */
	if( auth->algorithm_ && strcmp(auth->algorithm_,"MD5-sess")==0 ) {
		temp[0] = 0;	A1[0] = 0;
		strcat(temp,unquote(auth->username_));	strcat(temp,":");
		strcat(temp,unquote(auth->realm_));	strcat(temp,":");
		strcat(temp,password);
		
		/*strcpy(A1,H(temp));			*/
		H2(temp1,A1,33);
		strcat(A1,temp1);

		strcat(A1,":");
		strcat(A1,unquote(auth->nonce_));	strcat(A1,":");
		strcat(A1,unquote(auth->cnonce_));
	}
	else {	/* auth->algorithm_ == "MD5" */
		A1[0] = 0;
		strcat(A1,unquote(auth->username_));	strcat(A1,":");
		strcat(A1,unquote(auth->realm_));	strcat(A1,":");
		strcat(A1,password);
	}

	/* A2 */
	if( auth->message_qop_ && strcmp(auth->message_qop_,"auth-int")==0 ) {
		if( !entity_body )
			return RC_ERROR;
		A2[0] = 0;
		strcat(A2,method);			strcat(A2,":");
		strcat(A2,unquote(auth->digest_uri_));	strcat(A2,":");
		/*strcat(A2,H(entity_body));*/
		H2(entity_body,temp1,33);
		strcat(A1,temp1);
	}
	else {	/* auth->message_qop_ == "auth" */
		A2[0] = 0;
		strcat(A2,method);			strcat(A2,":");
		strcat(A2,unquote(auth->digest_uri_));
	}

	/* request-digest */
	if( auth->message_qop_ &&
	    ( strcmp(auth->message_qop_,"auth-int")==0 ||
	      strcmp(auth->message_qop_,"auth")==0 )) {
		temp[0] = 0;
		strcat(temp,unquote(auth->nonce_));	strcat(temp,":");
		
		if(auth->flag_ & SIP_DAFLAG_NONCE_COUNT) 
			strcat(temp,auth->nonce_count_);	
		strcat(temp,":");

		if(auth->flag_&SIP_DAFLAG_CNONCE)
			strcat(temp,unquote(auth->cnonce_));	
		strcat(temp,":");

		if( auth->flag_&SIP_DAFLAG_MESSAGE_QOP )
			strcat(temp,unquote(auth->message_qop_));	
		strcat(temp,":");
		/* mark by tyhuang
		strcat(temp,H(A2));
		memcpy( auth->response_, KD(H(A1),temp), 32); 
		auth->response_[32] = 0;
		*/
		
	}else {
		temp[0] = 0;
		strcat(temp,unquote(auth->nonce_));	strcat(temp,":");
		/*
		strcat(temp,H(A2));
		memcpy( auth->response_, KD(H(A1),temp), 32); 
		*/
	}
	

	H2(A2,temp1,33);
	strcat(temp,temp1);
	H2(A1,temp1,33);
	KD2(temp1,temp,auth->response_,33);
	
	auth->response_[32] = 0;
	TCRPrint(TRACE_LEVEL_API,"auth_response=%s\n",auth->response_);

	return RC_OK;
}

/*---------------------------------------------*/
/* New Interface for request Authorization	   */
/*---------------------------------------------*/
CCLAPI RCODE 
sipReqAddAuthz(SipReq request,SipAuthzStruct authoriz)
{
	RCODE  ret=RC_ERROR;
	switch(authoriz.iType){
	case SIP_BASIC_CHALLENGE:
		ret=sipReqSetBasicAuthz(request,SIP_AUTHZ,authoriz.auth_.basic_.username,authoriz.auth_.basic_.passwd);
		break;
	case SIP_DIGEST_CHALLENGE:
		ret=sipReqSetDigestAuthz(request,SIP_AUTHZ,authoriz.auth_.digest_);
		break;
	}
	return ret;
}

CCLAPI RCODE
sipReqAddPxyAuthz(SipReq request,SipAuthzStruct authoriz)
{
	RCODE  ret=RC_ERROR;
	switch(authoriz.iType){
	case SIP_BASIC_CHALLENGE:
		ret=sipReqSetBasicAuthz(request,SIP_PROXY_AUTHZ,authoriz.auth_.basic_.username,authoriz.auth_.basic_.passwd);
		break;
	case SIP_DIGEST_CHALLENGE:
		ret=sipReqSetDigestAuthz(request,SIP_PROXY_AUTHZ,authoriz.auth_.digest_);
		break;
	}

	return ret;
}

CCLAPI RCODE
sipReqGetAuthz(SipReq request,SipAuthzStruct* authoriz)
{
	RCODE  ret=RC_ERROR;
	switch(authoriz->iType){
	case SIP_BASIC_CHALLENGE:
		ret=sipReqGetBasicAuthz(request,SIP_AUTHZ,authoriz->auth_.basic_.username,64,authoriz->auth_.basic_.passwd,128);
		break;
	case SIP_DIGEST_CHALLENGE:
		ret=sipReqGetDigestAuthz(request,SIP_AUTHZ,&(authoriz->auth_.digest_));
		break;
	}
	return ret;
}

CCLAPI RCODE
sipReqGetPxyAuthz(SipReq request,SipAuthzStruct* authoriz)
{
	RCODE  ret=RC_ERROR;
	switch(authoriz->iType){
	case SIP_BASIC_CHALLENGE:
		ret=sipReqGetBasicAuthz(request,SIP_PROXY_AUTHZ,authoriz->auth_.basic_.username,64,authoriz->auth_.basic_.passwd,128);
		break;
	case SIP_DIGEST_CHALLENGE:
		ret=sipReqGetDigestAuthz(request,SIP_PROXY_AUTHZ,&(authoriz->auth_.digest_));
		break;
	}

	return ret;
}
/*---------------------------------------------*/
CCLAPI RCODE			
sipRequestSetPGPAuthorization(void)
{

	return RC_OK;
}

CCLAPI RCODE			
sipRequestGetPGPAuthorization(void)
{

	return RC_OK;
}

