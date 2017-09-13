/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sip_int.h
 *
 * $Id: sip_int.h,v 1.40 2006/11/02 06:33:13 tyhuang Exp $
 */

#ifndef SIP_INT_H
#define SIP_INT_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "sip_cm.h"
#include "rfc822.h"
#include "sip_bdy.h"
#include "sip_hdr.h"
#include "sip_req.h"
#include "sip_rsp.h"

#define strlen(A)	strlen((const char*)A)
#define strcat(A,B)	strcat((char*)A,(const char*)B)
#define strcpy(A,B)	strcpy((char*)A,(const char*)B)
#define strstr(A,B)	strstr((const char*)A,(const char*)B)
#define strcmp(A,B)	strcmp((const char*)A,(const char*)B)

/*--------------------------------------------*/
SipHdrType	sipHdrNameToType(char*);
const char* sipHdrTypeToName(SipHdrType);
SipMethodType	sipMethodNameToType(char*);

/*------------------------------------------*/
/* General Function */
int		RFC822ToSipHeader(MsgHdr, SipHdrType, void **);
BOOL		CheckHeaderName(MsgHdr, SipHdrType);
int		AnalyzeParameter(unsigned char *, SipParam*);
int		AnalyzeTimeParameter(unsigned char *, SipRfc1123Date **, int *);
int		CollectDateData(unsigned char **, unsigned char *, MsgHdrElm *);
int		CollectCommentData(unsigned char **, unsigned char *, MsgHdrElm *);
int		CollectQuotedString(unsigned char **, unsigned char *, MsgHdrElm *);
SipMsgType	sipMsgIdentify(unsigned char *);
SipRetMsg	sipParseErrMsg(SipHdrType);
/* name-addr | addr-spec */
int	AddrParse(unsigned char*, SipAddr*);
int	AddrAsStr(SipAddr*, unsigned char *, int, int *);
int	ExtractComment(unsigned char *, int *);

int	sipReqLineFree(SipReqLine*);
int	sipRspStatusLineFree(SipRspStatusLine*);

/* Accept */
int	AcceptParse(MsgHdr, void **);
int	AcceptAsStr(SipAccept*, unsigned char*, int, int*);
int	AcceptFree(SipAccept*);
int	AcceptReqDup(SipReq ,SipAccept*);
int	AcceptRspDup(SipRsp,SipAccept*);

/* Accept-Encoding */
int	AcceptEncodingParse(MsgHdr, void **);
int	AcceptEncodingAsStr(SipAcceptEncoding*, unsigned char*, int, int*);
int	AcceptEncodingFree(SipAcceptEncoding*);
int	AcceptEncodingReqDup(SipReq,SipAcceptEncoding*);
int	AcceptEncodingRspDup(SipRsp,SipAcceptEncoding*);

/* Accept-Language */
int	AcceptLanguageParse(MsgHdr, void **);
int	AcceptLanguageAsStr(SipAcceptLang*, unsigned char*, int, int*);
int	AcceptLanguageFree(SipAcceptLang*);
int	AcceptLanguageReqDup(SipReq, SipAcceptLang*);
int	AcceptLanguageRspDup(SipRsp,SipAcceptLang*);

/* Allow */
int	AllowParse(MsgHdr, void **);
int	AllowAsStr(SipAllow*, unsigned char *, int, int *);
int	AllowFree(SipAllow*);
int	AllowRspDup(SipRsp,SipAllow*);
int	AllowReqDup(SipReq,SipAllow*);

/* Allow-Events */
int	AllowEventsParse(MsgHdr, void **);
int	AllowEventsAsStr(SipAllowEvt*, unsigned char *, int, int *);
int	AllowEventsFree(SipAllowEvt*);
int	AllowEventsRspDup(SipRsp,SipAllowEvt*);
int	AllowEventsReqDup(SipReq,SipAllowEvt*);

/* Authorization */
int	AuthorizationParse(MsgHdr, void **);
int	AuthorizationAsStr(SipAuthz*, unsigned char*, int, int*);
int	AuthorizationFree(SipAuthz*);
int	AuthorizationReqDupLink(SipReq, SipAuthz*);

/* Call-ID */
int	CallIDParse(MsgHdr, void **);
int	CallIDAsStr(unsigned char *,unsigned char *, int, int *);
int	CallIDFree(unsigned char *);
int	CallIDReqDup(SipReq,unsigned char*);
int	CallIDRspDup(SipRsp,unsigned char*);

/* Class */
int	ClassParse(MsgHdr, void **);
int	ClassAsStr(SipClass*,unsigned char *, int, int *);
int	ClassFree(SipClass*);
int	ClassReqDup(SipReq,SipClass*);

/* Contact */
int			ContactParse(MsgHdr, void **);
CCLAPI SipContact*	sipContactNewFromText(char* text);
CCLAPI int		sipContactAsStr(SipContact *, unsigned char *, int, int *);
CCLAPI int		sipContactFree(SipContact *);
int			ContactReqDupLink(SipReq,SipContact*);
int			ContactRspDupLink(SipRsp,SipContact*);

/* Content-Encoding */
int	ContentEncodingParse(MsgHdr, void **);
int	ContentEncodingAsStr(SipContEncoding*, unsigned char*, int, int*);
int	ContentEncodingFree(SipContEncoding*);
int	ContentEncodingReqDup(SipReq, SipContEncoding*);
int	ContentEncodingRspDup(SipRsp,SipContEncoding*);

/* Content-Length */
int	ContentLengthParse(MsgHdr, void **);
int	ContentLengthAsStr(int, unsigned char*, int, int*);
/*ContentLength's type is int, free is needless */

/* Content-Type */
int	ContentTypeParse(MsgHdr, void **);
int	ContentTypeAsStr(SipContType*, unsigned char*, int, int*);
int	ContentTypeFree(SipContType*);
int	ContentTypeReqDup(SipReq,SipContType*);
int	ContentTypeRspDup(SipRsp,SipContType*);

/* CSeq */
int	CSeqParse(MsgHdr, void **);
int	CSeqAsStr(SipCSeq*, unsigned char *, int, int *);
int	CSeqFree(SipCSeq*);
int	CSeqReqDup(SipReq,SipCSeq*);
int	CSeqRspDup(SipRsp,SipCSeq*);

/* Date */
int	RFCDateParse(unsigned char*, SipRfc1123Date*);
int	RFCDateAsStr(SipRfc1123Date*, unsigned char *, int, int *);
int	DateParse(MsgHdr, void **);
int	DateAsStr(SipDate*, unsigned char *, int, int *);
int	DateFree(SipDate*);
int	DateReqDup(SipReq,SipDate*);
int	DateRspDup(SipRsp,SipDate*);

/* Encryption */
int	EncryptionParse(MsgHdr, void **);
int	EncryptionAsStr(SipEncrypt*, unsigned char *, int, int *);
int	EncryptionFree(SipEncrypt*);
int	EncryptionReqDup(SipReq, SipEncrypt*);
int	EncryptionRspDup(SipRsp,SipEncrypt*);

/* Event  */
int	EventParse(MsgHdr,void**);
int	EventAsStr(SipEvent*, unsigned char*, int, int*);
int	EventFree(SipEvent*);
int	EventReqDup(SipReq, SipEvent*);
int	EventRspDup(SipRsp,SipEvent*);

/* Expires */
int	ExpiresParse(MsgHdr, void **);
int	ExpiresAsStr(SipExpires*, unsigned char *, int, int *);
int	ExpiresFree(SipExpires*);
int	ExpiresReqDup(SipReq,SipExpires*);
int	ExpiresRspDup(SipRsp,SipExpires*);

/* From */
int	FromParse(MsgHdr, void **);
int	FromAsStr(SipFrom*, unsigned char *, int, int *);
int	FromFree(SipFrom*);
int	FromReqDup(SipReq,SipFrom*);
int	FromRspDup(SipRsp,SipFrom*);


/* Hide */
int	HideParse(MsgHdr, void **);
int	HideAsStr(SipHideType, unsigned char *, int, int *);
/*free is needless */

/* Max-Forwards */
int	MaxForwardsParse(MsgHdr, void **);
int	MaxForwardsAsStr(int, unsigned char *, int, int *);
/*free is needless */

/* Session-Expires */
int	MinSEParse(MsgHdr, void **);
int	MinSEAsStr(SipMinSE*,unsigned char*,int,int*);
int	MinSEFree(SipMinSE*);
int	MinSEReqDup(SipReq,SipMinSE*);
int	MinSERspDup(SipRsp,SipMinSE*);

/* Organization */
int	OrganizationParse(MsgHdr, void **);
int	OrganizationAsStr(unsigned char *, unsigned char *, int, int *);
int	OrganizationFree(unsigned char *);
int	OrganizationReqDup(SipReq,const char*);
int	OrganizationRspDup(SipRsp,const char*);

/* P-Asserted-Identity */
int	PAssertedIdentityParse(MsgHdr, void **);
int	PAssertedIdentityAsStr(SipIdentity*, unsigned char*, int, int *);
int	PAssertedIdentityFree(SipIdentity*);
int	PAssertedIdentityReqDupLink(SipReq,SipIdentity*);
int	PAssertedIdentityRspDupLink(SipRsp,SipIdentity*);

/* P-Preferred-Identity */
int	PPreferredIdentityParse(MsgHdr, void **);
int	PPreferredIdentityAsStr(SipIdentity*, unsigned char*, int, int *);
int	PPreferredIdentityFree(SipIdentity*);
int	PPreferredIdentityReqDupLink(SipReq,SipIdentity*);
int	PPreferredIdentityRspDupLink(SipRsp,SipIdentity*);

/* Priority */
int	PriorityParse(MsgHdr, void **);
int	PriorityAsStr(SipPriorityType, unsigned char *, int, int *);
/*free is needless */

/* Privacy */
int	PrivacyParse(MsgHdr, void **);
int	PrivacyAsStr(SipPrivacy*, unsigned char*, int, int *);
int	PrivacyFree(SipPrivacy*);
int	PrivacyReqDup(SipReq,SipPrivacy*);
int	PrivacyRspDup(SipRsp,SipPrivacy*);

/* Proxy-Authenticate */
int	ProxyAuthenticateParse(MsgHdr, void **);
int	ProxyAuthenticateAsStr(SipPxyAuthn*, unsigned char *, int, int *);
int	ProxyAuthenticateFree(SipPxyAuthn*);
int	ProxyAuthenticateRspDupLink(SipRsp,SipPxyAuthn*);

/* Proxy-Authorization */
int	ProxyAuthorizationParse(MsgHdr, void **);
int	ProxyAuthorizationAsStr(SipPxyAuthz*, unsigned char *, int, int *);
int	ProxyAuthorizationFree(SipPxyAuthz*);
int	ProxyAuthorizationReqDupLink(SipReq,SipPxyAuthz*);

/* Proxy-Require */
int	ProxyRequireParse(MsgHdr, void **);
int	ProxyRequireAsStr(SipPxyRequire*, unsigned char *, int, int *);
int	ProxyRequireFree(SipPxyRequire*);
int	ProxyRequireReqDup(SipReq,SipPxyRequire*);

/* RAck */
int	RAckParse(MsgHdr, void **);
int	RAckAsStr(SipRAck*, unsigned char *, int, int *);
int	RAckFree(SipRAck*);
int	RAckReqDup(SipReq,SipRAck*);

/* Record-Route */
int	RecordRouteParse(MsgHdr, void **);
int	RecordRouteAsStr(SipRecordRoute*, unsigned char *, int, int *);
int	RecordRouteFree(SipRecordRoute*);
int	RecordRouteReqDupLink(SipReq,SipRecordRoute*);
int	RecordRouteRspDupLink(SipRsp,SipRecordRoute*);

/*Refer-To */
int	ReferToParse(MsgHdr, void **);
int	ReferToAsStr(SipReferTo*, unsigned char*, int, int*);
int	ReferToReqDup(SipReq,SipReferTo*);
int	ReferToRspDup(SipRsp,SipReferTo*);
int	ReferToFree(SipReferTo*);

/*Referred-By */
int	ReferredByParse(MsgHdr, void **);
int	ReferredByAsStr(SipReferredBy*, unsigned char*, int, int*);
int	ReferredByReqDup(SipReq,SipReferredBy*);
int	ReferredByRspDup(SipRsp,SipReferredBy*);
int	ReferredByFree(SipReferredBy*);

/* Replaces */
int	ReplacesParse(MsgHdr, void**);
int	ReplacesAsStr(SipReplaces*,unsigned char*, int, int*);
int	ReplacesFree(SipReplaces*);
int	ReplacesReqDupLink(SipReq,SipReplaces*);
int	ReplacesRspDupLink(SipRsp,SipReplaces*);

/* Remote-Party-ID */
int	RemotePartyIDParse(MsgHdr,void**);
int	RemotePartyIDAsStr(SipRemotePartyID*,unsigned char*, int, int*);
int	RemotePartyIDFree(SipRemotePartyID*);
int	RemotePartyIDReqDupLink(SipReq,SipRemotePartyID*);
int	RemotePartyIDRspDupLink(SipRsp,SipRemotePartyID*);

/* Require */
int	RequireParse(MsgHdr, void **);
int	RequireAsStr(SipRequire*, unsigned char *, int, int *);
int	RequireFree(SipRequire*);
int	RequireReqDup(SipReq,SipRequire*);
int	RequireRspDup(SipRsp,SipRequire*);

/* Response-Key */
int	ResponseKeyParse(MsgHdr, void **);
int	ResponseKeyAsStr(SipRspKey*, unsigned char *, int, int *);
int	ResponseKeyFree(SipRspKey*);
int	ResponseKeyReqDup(SipReq,SipRspKey*);

/* Retry-After */
int	RetryAfterParse(MsgHdr, void **);
int	RetryAfterAsStr(SipRtyAft*, unsigned char *, int, int *);
int	RetryAfterFree(SipRtyAft*);
int	RetryAfterReqDup(SipReq,SipRtyAft*);
int	RetryAfterRspDup(SipRsp,SipRtyAft*);

/* Route */
int	RouteParse(MsgHdr, void **);
int	RouteAsStr(SipRoute*, unsigned char *, int, int *);
int	RouteFree(SipRoute*);
int	RouteReqDupLink(SipReq,SipRoute*);

/* RSeq */
int	RSeqParse(MsgHdr, void **);
int	RSeqAsStr(int, unsigned char*, int, int*);
/*RSeq's type is int, free is needless */

/* Session-Expires */
int	SessionExpiresParse(MsgHdr, void **);
int	SessionExpiresAsStr(SipSessionExpires*,unsigned char*,int,int*);
int	SessionExpiresFree(SipSessionExpires*);
int	SessionExpiresReqDup(SipReq,SipSessionExpires*);
int	SessionExpiresRspDup(SipRsp,SipSessionExpires*);

/* Server */
int	ServerParse(MsgHdr, void **);
int	ServerAsStr(SipServer*, unsigned char *, int, int *);
int	ServerFree(SipServer*);
int	ServerRspDup(SipRsp,SipServer*);

/* SIP-ETag */
int	SIPETagParse(MsgHdr, void **);
int	SIPETagAsStr(unsigned char *,unsigned char *, int, int *);
int	SIPETagFree(unsigned char *);

/* SIP-If-Match */
int	SIPIfMatchParse(MsgHdr, void **);
int	SIPIfMatchAsStr(unsigned char *,unsigned char *, int, int *);
int	SIPIfMatchFree(unsigned char *);

/* Subject */
int	SubjectParse(MsgHdr, void **);
int	SubjectAsStr(unsigned char *, unsigned char *, int, int *);
int	SubjectFree(unsigned char *);
int	SubjectReqDup(SipReq,unsigned char*);

/* Supported */
int	SupportedParse(MsgHdr, void **);
int	SupportedAsStr(SipSupported*, unsigned char *, int, int *);
int	SupportedFree(SipSupported*);
int SupportedReqDup(SipReq,SipSupported *);
int	SupportedRspDup(SipRsp,SipSupported *);

/* Timestamp */
int	TimestampParse(MsgHdr, void **);
int	TimestampAsStr(SipTimestamp*, unsigned char *, int, int *);
int	TimestampFree(SipTimestamp*);
int	TimestampReqDup(SipReq, SipTimestamp*);
int	TimestampRspDup(SipRsp,SipTimestamp*);

/* To */
int	ToParse(MsgHdr, void **);
int	ToAsStr(SipTo*, unsigned char *, int, int *);
int	ToFree(SipTo*);
int	ToReqDup(SipReq,SipTo*);
int	ToRspDup(SipRsp,SipTo*);

/* Unsupported */
int	UnsupportedParse(MsgHdr, void **);
int	UnsupportedAsStr(SipUnsupported*, unsigned char *, int, int *);
int	UnsupportedFree(SipUnsupported*);
int	UnsupportedRspDup(SipRsp,SipUnsupported *);

/* User-Agent */
int	UserAgentParse(MsgHdr, void **);
int	UserAgentAsStr(SipUserAgent*, unsigned char *, int, int *);
int	UserAgentFree(SipUserAgent*);
int	UserAgentReqDup(SipReq,SipUserAgent*);
int	UserAgentRspDup(SipRsp,SipUserAgent*);

/* Via */
int	ProcessViaParm(SipViaParm* , unsigned char*);
int	ViaParse(MsgHdr, void **);
int	ViaAsStr(SipVia*, unsigned char *, int, int *);
int	ViaFree(SipVia*);
int	ViaReqDupLink(SipReq,SipVia*);
int	ViaRspDupLink(SipRsp,SipVia*);

/* Warning */
int	WarningParse(MsgHdr, void **);
int	WarningAsStr(SipWarning*, unsigned char *, int, int *);
int	WarningFree(SipWarning*);
int	WarningRspDupLink(SipRsp,SipWarning*);

/* WWW-Authenticate */
int	WWWAuthenticateParse(MsgHdr, void **);
int	WWWAuthenticateAsStr(SipWWWAuthn*, unsigned char *, int, int *);
int	WWWAuthenticateFree(SipWWWAuthn*);
int	WWWAuthenticateRspDupLink(SipRsp,SipWWWAuthn*);

/*-------------------add by tyhuang------------------------------------*/
/* Alert-Info */
int	AlertInfoParse(MsgHdr, void **);
int	AlertInfoAsStr(SipInfo *, unsigned char *, int , int *);
int	AlertInfoFree(SipInfo*);
int	AlertInfoReqDup(SipReq,SipInfo*);
int	AlertInfoRspDup(SipRsp,SipInfo*);

/* Authentication-Info */
int	AuthenticationInfoParse(MsgHdr, void **);
int	AuthenticationInfoAsStr(SipAuthInfo *, unsigned char *, int , int *);
int	AuthenticationInfoFree(SipAuthInfo*);
int	AuthenticationInfoRspDup(SipRsp,SipAuthInfo*);

/* Call-Info */
int	CallInfoParse(MsgHdr, void **);
int	CallInfoAsStr(SipInfo *, unsigned char *, int , int *);
int	CallInfoFree(SipInfo*);
int	CallInfoReqDup(SipReq,SipInfo*);
int	CallInfoRspDup(SipRsp,SipInfo*);

/* Content-Disposition */
int	ContentDispositionParse(MsgHdr, void **);
int	ContentDispositionAsStr(SipContentDisposition *, unsigned char *, int , int *);
int	ContentDispositionFree(SipContentDisposition*);
int	ContentDispositionReqDup(SipReq,SipContentDisposition*);
int	ContentDispositionRspDup(SipRsp,SipContentDisposition*);

/* Content-Language */
int	ContentLanguageParse(MsgHdr, void **);
int	ContentLanguageAsStr(SipContentLanguage *, unsigned char *, int , int *);
int	ContentLanguageFree(SipContentLanguage*);
int	ContentLanguageReqDup(SipReq,SipContentLanguage*);
int	ContentLanguageRspDup(SipRsp,SipContentLanguage*);

/* Error-Info */
int	ErrorInfoParse(MsgHdr, void **);
int	ErrorInfoAsStr(SipInfo *, unsigned char *, int , int *);
int	ErrorInfoFree(SipInfo*);
int	ErrorInfoRspDup(SipRsp,SipInfo*);

/* In-Reply-To */
int	InReplyToParse(MsgHdr, void **);
int	InReplyToAsStr(SipInReplyTo*, unsigned char *, int, int *);
int	InReplyToFree(SipInReplyTo*);
int	InReplyToReqDup(SipReq,SipInReplyTo*);

/* MIME-Version */
int	MIMEVersionParse(MsgHdr, void **);
int	MIMEVersionAsStr(unsigned char *, unsigned char *, int , int *);
int	MIMEVersionFree(unsigned char *);
int	MIMEVersionReqDup(SipReq,unsigned char*);
int	MIMEVersionRspDup(SipRsp,unsigned char*);

/* Min-Expires */
int	MinExpiresParse(MsgHdr, void **);
int	MinExpiresAsStr(int, unsigned char *, int, int *);
/*free is needless */

/* Reply-To */
int	ReplyToParse(MsgHdr, void **);
int	ReplyToAsStr(SipReplyTo*, unsigned char *, int, int *);
int	ReplyToFree(SipReplyTo*);
int	ReplyToReqDup(SipReq,SipReplyTo*);
int	ReplyToRspDup(SipRsp,SipReplyTo*);

/* Subscription-State */
int	SubscriptionStateParse(MsgHdr, void **);
int	SubscriptionStateAsStr(SipSubState*, unsigned char *, int, int *);
int	SubscriptionStateFree(SipSubState*);
int	SubscriptionStateReqDup(SipReq,SipSubState*);

#ifdef ICL_IMS
/* P-Media-Authorization */
int	PMAParse(MsgHdr, void **);
int	PMAAsStr(unsigned char*, unsigned char *, int, int *);
int	PMAReqDup(SipReq,unsigned char*);
int	PMARspDup(SipRsp,unsigned char*);
 
/* Path */
int	PathParse(MsgHdr, void **);
int	PathAsStr(SipPath*, unsigned char *, int, int *);
int	PathFree(SipPath*);
int	PathReqDupLink(SipReq,SipPath*);
int	PathRspDupLink(SipRsp,SipPath*);

/* P-Access-Network-Info */
int	PANIParse(MsgHdr, void **);
int	PANIAsStr(unsigned char*, unsigned char *, int, int *);
int	PANIReqDup(SipReq,unsigned char*);
int	PANIRspDup(SipRsp,unsigned char*);

/* P-Associated-URI */
int	PAUParse(MsgHdr, void **);
int	PAUAsStr(unsigned char*, unsigned char *, int, int *);
int	PAURspDup(SipRsp,unsigned char*);

/* P-Called-Party-ID */
int	PCPIParse(MsgHdr, void **);
int	PCPIAsStr(unsigned char*, unsigned char *, int, int *);
int	PCPIReqDup(SipReq,unsigned char*);

/* P-Charging-Function-Address */
int	PCFAParse(MsgHdr, void **);
int	PCFAAsStr(unsigned char*, unsigned char *, int, int *);
int	PCFAReqDup(SipReq,unsigned char*);
int	PCFARspDup(SipRsp,unsigned char*);

/* P-Charging-Vector */
int	PCVParse(MsgHdr, void **);
int	PCVAsStr(unsigned char*, unsigned char *, int, int *);
int	PCVReqDup(SipReq,unsigned char*);
int	PCVRspDup(SipRsp,unsigned char*);

/* P-Visited-Network-ID */
int	PVNIParse(MsgHdr, void **);
int	PVNIAsStr(unsigned char*, unsigned char *, int, int *);
int	PVNIReqDup(SipReq,unsigned char*);

/* Security-Client */
int	SecurityClientParse(MsgHdr, void **);
int	SecurityClientAsStr(unsigned char*, unsigned char *, int, int *);
int	SecurityClientReqDup(SipReq,unsigned char*);

/* Security-Server */
int	SecurityServerParse(MsgHdr, void **);
int	SecurityServerAsStr(unsigned char*, unsigned char *, int, int *);
int	SecurityServerRspDup(SipRsp,unsigned char*);

/* Security-Verify */
int	SecurityVerifyParse(MsgHdr, void **);
int	SecurityVerifyAsStr(unsigned char*, unsigned char *, int, int *);
int	SecurityVerifyReqDup(SipReq,unsigned char*);

/* Service-Route */
int	ServiceRouteParse(MsgHdr, void **);
int	ServiceRouteAsStr(SipServiceRoute*, unsigned char *, int, int *);
int	ServiceRouteFree(SipServiceRoute*);
int	ServiceRouteRspDupLink(SipRsp,SipServiceRoute*);

#endif /* end of ICL_IMS */

/*---------------------------------------------------------------------*/

/* Body and Buffer */
int	sipBodyFree(SipBody*);

/*parameter duplication function */
SipParam*	sipParameterDup(SipParam*,int);
SipAddr*	sipAddrDup(SipAddr*,int);
SipRecAddr* sipRecAddrDup(SipRecAddr*,int);
SipAuthz*	sipAuthzDup(SipAuthz*);
SipPxyAuthz*	sipPxyAuthzDup(SipPxyAuthz*);
SipStr*		sipStrDup(SipStr* ,int );
SipCoding*	sipCodingDup(SipCoding* ,int);
SipLang*	sipLanguageDup(SipLang*,int);
SipMediaType*	sipMediaTypeDup(SipMediaType*,int);
SipContactElm*	sipContactElmDup(SipContactElm*,int);
SipRfc1123Date*	rfc1123DateDup(SipRfc1123Date *);
SipViaParm*	sipViaParmDup(SipViaParm*,int);
SipChllng*	sipChallengeDup(SipChllng*,int);
SipWrnVal*	sipWarningDup(SipWrnVal*,int);

/*Memory free function */
int	sipParameterFree(SipParam *);
int	sipMediaTypeFree(SipMediaType *);
int	sipCodingFree(SipCoding *);
int	sipLanguageFree(SipLang *);
int	sipAddrFree(SipAddr *);
int	sipRecAddrFree(SipRecAddr *);
int	sipContactElmFree(SipContactElm *);
int	sipStrFree(SipStr *);
int	sipViaParmFree(SipViaParm *);
int	sipChallengeFree(SipChllng *);

#ifdef  __cplusplus
}
#endif

#endif /* SIP_INT_H */
