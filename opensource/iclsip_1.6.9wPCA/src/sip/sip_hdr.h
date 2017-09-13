/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sip_hdr.h
 *
 * $Id: sip_hdr.h,v 1.58 2006/11/02 06:33:13 tyhuang Exp $
 */

#ifndef SIP_HDR_H
#define SIP_HDR_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <common/cm_def.h>
#include "sip_cm.h"
#include "sip_bdy.h"
#include "sip_url.h"

/* SIP message type */
typedef enum {
	SIP_MSG_UNKNOWN		=0,
	SIP_MSG_REQ		=0x01,	
	SIP_MSG_RSP		=0x02
} SipMsgType;	

/* Support SIP Request Methods */
typedef enum {
	INVITE=0,
	ACK,
	OPTIONS,
	BYE,
	CANCEL,
	REGISTER,  /*RFC 2543 standard methods*/
	SIP_INFO,	   /*RFC 2976 standar methods*/
	PRACK,	   /* RFC 3262 */
	/*COMET,	due to being expired*/
	REFER,		/*RFC 3515 */
	/* new methods for simple api*/
	SIP_SUBSCRIBE,
	SIP_NOTIFY,
	SIP_PUBLISH,
	SIP_MESSAGE,
	/*------------------------*/
	SIP_UPDATE,		/* RFC 3311 */
	UNKNOWN_METHOD /* unknown method */
} SipMethodType;

#define SIP_SUPPORT_METHOD_NUM	UNKNOWN_METHOD	

/* support SIP headers */
typedef enum {		
	Accept = 0,
	Accept_Encoding,
	Accept_Language,
	Alert_Info,  /* added by tyhuang */
	Allow,
	Allow_Events,
	Authentication_Info,/* added by tyhuang */
	Authorization,
	Call_ID,
	Call_Info,  /* added by tyhuang */
	Class, /* added by tyhuang */
	Contact,
	Content_Disposition, /* added by tyhuang */
	Content_Encoding,
	Content_Language, /* added by tyhuang */
	Content_Length,
	Content_Type,
	CSeq,
	Date,
	Encryption,
	Error_Info,  /* added by tyhuang */
	Event,
	Expires,
	From,
	Hide,
	In_Reply_To, /* added by tyhuang */
	Max_Forwards,
	MIME_Version, /* added by tyhuang */
	Min_Expires,  /* added by tyhuang */
	Min_SE,			/* mini session expire */
	Organization,
	P_Asserted_Identity, /* RFC 3325 */
	P_Preferred_Identity, /* RFC 3325 */
	Priority,
	Privacy,	/* RFC 3323 */
	Proxy_Authenticate,
	Proxy_Authorization,
	Proxy_Require,
	RAck,	/* RFC 3262 */
	Record_Route,
	Refer_To, /* RFC3515 */
	Referred_By,
	Remote_Party_ID,
	Replaces,
	Reply_To, /* added by tyhuang */
	Require,
	Response_Key,
	Retry_After,
	Route,
	RSeq,	/* RFC 3262 */
	Server,
	Session_Expires,
	SIP_ETag, /* RFC 3903 */
	SIP_If_Match, /* RFC 3903 */
	Subject,
	Subscription_State,
	Supported,
	Timestamp,
	To,
	Unsupported,
	User_Agent,
	Via,
	Warning,
	WWW_Authenticate,
	Allow_Events_Short,
	Call_ID_Short,
	Contact_Short,
	Content_Encoding_Short,
	Content_Length_Short,
	Content_Type_Short,
	From_Short,
	Event_Short,
	Refer_To_Short,
	Referred_By_Short,
	Session_Expires_Short,
	Subject_Short,
	Supported_Short,
	To_Short,
	Via_Short,
#ifdef ICL_IMS
	P_Media_Authorization, /* RFC3313 */
	Path,	/* RFC3327 */
	P_Access_Network_Info,	/* RFC3455 */
	P_Associated_URI,
	P_Called_Party_ID,
	P_Charging_Function_Address,
	P_Charging_Vector,
	P_Visited_Network_ID,
	Security_Client,	/* RFC3329 */
	Security_Server,
	Security_Verify,
	Service_Route,	/* RFC3608 */
#endif /* end of ICL_IMS */
	UnknSipHdr
} SipHdrType ;   

typedef enum {
	route = 0,
	hop,
	unknown_hide
} SipHideType;

typedef enum {
	Mon = 0,
	Tue,
	Wed,
	Thu,
	Fri,
	Sat,
	Sun,
	UnknWeek
} SipwkdayType;       

typedef enum {
	Jan = 0,
	Feb,
	Mar,
	Apr,
	May,
	Jun,
	Jul,
	Aug,
	Sep,
	Oct,
	Nov,
	Dec,
	UnknMonth
} SipMonthType;  

typedef enum {
	emergency = 0,
	urgent,
	normal,
	non_urgent,
	Unkn_priority
} SipPriorityType;

typedef enum {
	SIP_BASIC_CHALLENGE	=0x1,
	SIP_DIGEST_CHALLENGE	=0x2
} SipChallengeType;		 

/*-----------------------------------*/
typedef struct {
	char*		ptrVersion;
	char*		ptrStatusCode;
	char*		ptrReason;
} SipRspStatusLine; 

typedef struct {
	SipMethodType	iMethod;
	char*		ptrRequestURI;
	char*		ptrSipVersion;
} SipReqLine;

typedef struct sipParam_t	SipParam; 
struct sipParam_t {
	char*		name;
	char*		value;
	SipParam*	next;
};

typedef struct sipMediaType_t	SipMediaType;
struct sipMediaType_t {
	char*		type;
	char*		subtype;
	int		numParam;
	SipParam*	sipParamList;
	SipMediaType*	next;
};

typedef struct sipMediaType_t	SipContType;

/* Header Accept */
typedef struct sipAccept_t {
	int		numMediaTypes;
	SipMediaType*	sipMediaTypeList;
} SipAccept; 

/* Original type define structure */
typedef struct sipCoding_t	SipCoding;
struct sipCoding_t {
	char*		coding;
	char*		qvalue;
	SipCoding*	next;
}; 

/* Header Accept-Encoding */
typedef struct sipAcceptEncoding_t {
	int		numCodings;
	SipCoding*	sipCodingList;
} SipAcceptEncoding;

typedef struct sipLang_t	SipLang;
struct sipLang_t {
	char*		lang;
	char*		qvalue;
	SipLang*	next;
};

/* Header Accept-Language */
typedef struct sipAcceptLanguage_t {
	int		numLangs;
	SipLang*	sipLangList;
} SipAcceptLang;

/* Header Allow */
typedef struct sipAllow_t {
	int		Methods[SIP_SUPPORT_METHOD_NUM];
} SipAllow;

typedef struct Str_t	SipStr;
struct Str_t {
	char*		str;
	SipStr*		next;
};

/* Header Allow-Events */
typedef struct sipAllowEvt_t SipAllowEvt;
struct sipAllowEvt_t{
	char*		pack;
	SipStr*		tmplate;
	SipAllowEvt*	next;
};
/* Header Authentication-Info */
typedef struct sipAuthinfo_t{
	int		numAinfos;
	SipParam*	ainfoList;
}SipAuthInfo;

/* Header Authorization */
typedef struct sipAuthz_t	SipAuthz;
struct sipAuthz_t {
	char*		scheme;
	int		numParams;
	SipParam*	sipParamList;
	SipAuthz*	next;
};

/*******************add by tyhuang***/
/* Info */
typedef struct sipInfo_t SipInfo;
struct sipInfo_t{
	char*		absoluteURI;
	int		numParams;
	SipParam*	SipParamList;
	SipInfo*	next;
};

typedef struct sipClass_t{
	int		numClass;
	SipStr*		classList;	
}SipClass;
/***********************************/
typedef struct rfc1123Date_t {
	char		year[5];
	SipMonthType	month;
	SipwkdayType	weekday;
	char		day[3];
	char		time[9];
} SipRfc1123Date;

typedef struct sipAddr_t	SipAddr;
struct sipAddr_t {
	char*		display_name;
	char*		addr;
	int		with_angle_bracket;
	SipAddr*	next;
};

typedef struct sipContactElm_t	SipContactElm;
struct sipContactElm_t {
	int		numaddress;
	SipAddr*	address;
	int		expireSecond;
	SipRfc1123Date*	expireDate;  /*if empty, please fill NULL */
	int		numext_attrs;
	SipParam*	ext_attrs;
	char*		comment;
	SipContactElm*	next;
};

/* Header Contact */
typedef struct sipContact_t {
	int		numContacts;
	SipContactElm*	sipContactList;
} SipContact;

/* Header Content-Disposition */
typedef struct sipContentDisposition_t{
	char*		disptype;
	int		numParam;
	SipParam*	paramList;
}SipContentDisposition;

/* Header Content-Encoding */
typedef struct sipContEncoding_t {
	int		numCodings;
	SipCoding*	sipCodingList;
} SipContEncoding;

typedef struct sipContentLanguage_t{
	int		numLang;
	SipStr*		langList;	
}SipContentLanguage;

/* Header CSeq */
typedef struct sipCSeq_t {
	UINT32		seq;
	SipMethodType	Method;
} SipCSeq;

/* Header Date */
typedef struct sipDate_t {
	SipRfc1123Date*	date;
} SipDate;

/* Header Event */
typedef struct sipEvent_t SipEvent;
struct sipEvent_t{
	char*		pack;
	SipStr*		tmplate;
	int		numParam;
	SipParam*	paramList;
};

/* Header Encryption */
typedef struct sipEncrypt_t {
	char*		scheme;
	int		numParams;
	SipParam*	sipParamList;
} SipEncrypt;

/* Header Expires */
typedef struct sipExpires_t {
	int		expireSecond;
/*	SipRfc1123Date*	expireDate; mark by tyhuang 2004/12/7 */
} SipExpires;

/* Header From */
typedef struct sipFrom_t {
	int		numAddr;
	SipAddr*	address;
	int		numParam;
	SipParam*	ParamList;
} SipFrom; /*based on new RFC2543bis, generic parameter, not only tag */
/*typedef struct sipFrom_t {
	int		numAddr;
	SipAddr*	address;
	int		numTags;
	SipStr*		tagList;
} SipFrom; */ /* based on RFC 2543, only tag parameters*/

typedef struct sipPrivacy_t {
	SipStr *privvalue;
	int numPriv;
} SipPrivacy;

typedef struct sipIdentity_t SipIdentity;
struct sipIdentity_t{
	SipAddr*	address;
	int			numAddr;
	SipIdentity *next;
};

typedef struct Challenge_t	SipChllng; 
struct Challenge_t {
	char*		scheme;
	int		numParams;
	SipParam*	sipParamList;
	SipChllng*	next;
};

/* Header Proxy-Authenticate */
typedef struct sipProxyAuthenticate_t {
	int		numChllngs;
	SipChllng*	ChllngList;
} SipPxyAuthn;

/* Header Proxy-Authorization */
typedef struct sipProxyAuthorization_t  SipPxyAuthz;
struct sipProxyAuthorization_t {
	char*		scheme;
	int		numParams;
	SipParam*	sipParamList;
	SipPxyAuthz*	next;
};

/* Header Proxy-Require */
typedef struct sipProxyRequire_t {
	int		numTags;
	SipStr*		sipOptionTagList;
} SipPxyRequire;

typedef struct sipRecAddr_t	SipRecAddr;
struct sipRecAddr_t {
	char*		display_name;
	char*		addr;
	int			numParams;
	SipParam*	sipParamList;		
	SipRecAddr*	next;
};

/* Header Record-Route */
typedef struct sipRecordRoute_t {
	int		numNameAddrs;
	SipRecAddr*	sipNameAddrList;
} SipRecordRoute;

/* Header Refer-To */
typedef	struct sipReferTo_t SipReferTo;
struct sipReferTo_t{
	int		numAddr;
	SipAddr*	address;
	int		numParam;
	SipParam*	paramList;
};

/* Header Referred-By */
typedef struct sipReferredBy_t{
	int		numAddr;
	SipAddr*	address;
	int		numParam;
	SipParam*	paramList;
}SipReferredBy;

/* Header Remote-Party-ID */
typedef struct sipRemotePartyID_t SipRemotePartyID;
struct sipRemotePartyID_t{
	SipAddr*	addr;
	int		numParam;
	SipParam*	paramList;
	SipRemotePartyID* next;
};

/* Header Replaces */
typedef struct sipReplaces_t SipReplaces;
struct sipReplaces_t{
	char*		callid;
	int		numParam;
	SipParam*	paramList;
	SipReplaces*	next;
};

/* Header In-Reply-To tyhuang*/
typedef struct sipInReplyTo_t{
	int		numCallids;
	SipStr*		CallidList;
}SipInReplyTo;

/* Header ReplyTo by tyhuang */
typedef struct sipReplyTo_t {
	int		numAddr;
	SipAddr*	address;
	int		numParam;
	SipParam*	ParamList;
} SipReplyTo;


/* Header Require */
typedef struct sipRequire_t {
	int		numTags;
	SipStr*		sipOptionTagList;
} SipRequire;

/* Header Response-Key */
typedef struct sipResponseKey_t {
	char*		scheme;
	int		numParams;
	SipParam*	sipParamList;
} SipRspKey;

/* Header Retry-After */
typedef struct sipRetryAfter_t {
	int		afterSecond;	
	SipRfc1123Date*	afterDate;
	char*		comment;
	int		duration;
} SipRtyAft; 

/* Header Route */
typedef struct sipRoute_t {
	int		numNameAddrs;
	SipRecAddr*	sipNameAddrList;
} SipRoute;

/* Header RAck */
typedef struct sipRAck_t {
	UINT32			rseq;
	UINT32			cseq;
	SipMethodType	Method;
} SipRAck;

/* Header Session-Expires */
typedef struct sipSessionExpires_t SipSessionExpires;
struct sipSessionExpires_t{
	int		deltasecond;
	int		numParam;
	SipParam*	paramList;
};

/* Header Min-SE*/
typedef struct sipMinSE_t {
	int		deltasecond;
	int		numParam;
	SipParam*	paramList;
}SipMinSE;

/* Header Server */
typedef struct sipServer_t {
	int		numServer;
	SipStr*		data;
} SipServer;

/* Header Subscription-State */
typedef struct sipSubscriptionState_t{
	char*		state;
	int		expires;
	int		retryatfer;
	char*		reason;
	int		numParam;
	SipParam*	paramList;
}SipSubState;

/* Header supported */
typedef struct sipSupported_t {
	int		numTags;
	SipStr*		sipOptionTagList;
} SipSupported;

/* Header Timestamp */
typedef struct sipTimestamp_t {
	char*		time;
	char*		delay;
} SipTimestamp;

/* Header To */
typedef struct sipTo_t {
	int		numAddr;
	SipAddr*	address;
	int		numParam;
	SipParam*	paramList;
} SipTo;

/*typedef struct sipTo_t {
	int		numAddr;
	SipAddr*	address;
	int		numTags;
	SipStr*		tagList;
} SipTo; */ /* Based on RFC 2543, only tag exist */

/* Header Unsupported */
typedef struct sipUnsupported_t {
	int		numTags;
	SipStr*		sipOptionTagList;
} SipUnsupported;

/* Header User-Agent */
typedef struct sipUserAgent_t {
	int		numdata;
	SipStr*		data;
} SipUserAgent;

typedef struct sipViaParm_t	SipViaParm;
struct sipViaParm_t {
	char*		protocol;
	char*		version;
	char*		transport;
	char*		sentby;
	int		hidden; /*From here can be duplicate several items*/
	int		ttl;    /*Acer Modified SipIt10th */
	char*		maddr;
	char*		received;
	char*		branch;
	char*		comment; /*comment should be as parameter structure*/
	int		rport; /* add by tyhuang 2003.7.22 */
	SipParam*	UnSupportparamList; /* add by tyhuang 2003.9.4 */
	int		numofUnSupport;
	SipViaParm*	next;
};

/* Header Via */
typedef struct sipVia_t {
	int		numParams;
	SipViaParm*	ParmList;
	SipViaParm*	last;
} SipVia;

/* Header Warning */
typedef struct sipWarningValue_t	SipWrnVal;
struct sipWarningValue_t {
	unsigned char*	code;
	unsigned char*	agent;
	unsigned char*	text;
	SipWrnVal*	next;
};

typedef struct sipWarning_t {
	int		numVal;
	SipWrnVal*	WarningValList;
	SipWrnVal*	last;
} SipWarning;

/* Header WWW-Authenticate */
typedef struct sipWWWAuthenticate_t {
	int		numChallenges;
	SipChllng*	ChllngList;
} SipWWWAuthn;


#ifdef ICL_IMS
 
/* Header Path */
typedef struct sipRoute_t		SipPath;

/* Header P_Access_Network_Info,
P_Associated_URI,
P_Called_Party_ID,
P_Charging_Function_Address,
P_Charging_Vector,
P_Visited_Network_ID,
Security_Client,
Security_Server,
Security_Verify,*/

/* Header Service_Route */
typedef struct sipRoute_t		SipServiceRoute;
#endif /* end of ICL_IMS */

#ifdef  __cplusplus
}
#endif

#endif /* SIP_HDR_H */
