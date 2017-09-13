/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sip_hdr.c
 *
 * $Id: sip_hdr.c,v 1.170 2006/11/02 06:33:13 tyhuang Exp $
 */

#include <stdio.h>
#include <string.h>
#include "sip_hdr.h"
#include "sip_int.h"
#include <common/cm_utl.h>
#include <common/cm_trace.h>
#include <adt/dx_str.h>


const char* sipMethodName[] =
{
  "INVITE",
  "ACK",
  "OPTIONS",
  "BYE",
  "CANCEL",
  "REGISTER",
  "INFO",
  "PRACK",
  "REFER",
  "SUBSCRIBE",
  "NOTIFY",
  "PUBLISH",
  "MESSAGE",
  "UPDATE",
  "UNKN_METHOD"
};

const char* sipHeaderName[] = 
{
  "Accept",
  "Accept-Encoding",
  "Accept-Language",
  "Alert-Info", /* add by tyhuang*/
  "Allow",
  "Allow-Events",
  "Authentication-Info",
  "Authorization",
  "Call-ID",
  "Call-Info", /* add by tyhuang */
  "Class",
  "Contact",
  "Content-Disposition", /* add by tyhuang */
  "Content-Encoding",
  "Content-Language",	/* add by tyhuang */
  "Content-Length",
  "Content-Type",
  "CSeq",
  "Date",
  "Encryption",
  "Error-Info",
  "Event",
  "Expires",
  "From",
  "Hide",
  "In-Reply-To",  /* add by tyhuang */
  "Max-Forwards",
  "MIME-Version", /* add by tyhuang */
  "Min-Expires",  /* add by tyhuang */
  "Min-SE",
  "Organization",
  "P-Asserted-Identity",
  "P-Preferred-Identity",
  "Priority",
  "Privacy",
  "Proxy-Authenticate",
  "Proxy-Authorization",
  "Proxy-Require",
  "RAck",
  "Record-Route",
  "Refer-To",
  "Referred-By",
  "Remote-Party-ID",
  "Replaces",
  "Reply-To",
  "Require",
  "Response-Key",
  "Retry-After",
  "Route",
  "RSeq",
  "Server",
  "Session-Expires",
  "SIP-ETag",
  "SIP-If-Match",
  "Subject",
  "Subscription-State",
  "Supported",
  "Timestamp",
  "To",
  "Unsupported",
  "User-Agent",
  "Via",
  "Warning",
  "WWW-Authenticate",
  "u",
  "i",
  "m",
  "e",
  "l",
  "c",
  "f",
  "o",
  "r",	/* refer-to */
  "b",
  "x",
  "s",
  "k",
  "t",
  "v",
#ifdef ICL_IMS
  "P-Media-Authorization",
  "Path",
  "P-Access-Network-Info",
  "P-Associated-URI",
  "P-Called-Party-ID",
  "P-Charging-Function-Address",
  "P-Charging-Vector",
  "P-Visited-Network-ID",
  "Security-Client",
  "Security-Server",
  "Security-Verify",
  "Service-Route",
#endif /* end of ICL_IMS */
  "Unknown"
};

const char* wkdayName[] =
{
  "Mon",
  "Tue",
  "Wed",
  "Thu",
  "Fri",
  "Sat",
  "Sun"
};

const char* monthName[] = 
{
  "Jan",
  "Feb",
  "Mar",
  "Apr",
  "May",
  "Jun",
  "Jul",
  "Aug",
  "Sep",
  "Oct",
  "Nov",
  "Dec"
};

const char* priorityName[] = 
{
  "emergency",
  "urgent",
  "normal",
  "non-urgent"
};

const char* hideName[] = 
{
  "route",
  "hop"
};

char *sipHdrChunk = "ACCEPT%ACCEPT-ENCODING%ACCEPT-LANGUAGE%ALERT-INFO%ALLOW%ALLOW-EVENTS%AUTHENTICATION-INFO%AUTHORIZATION%CALL-ID%CALL-INFO%CLASS%CONTACT%CONTENT-DISPOSITION%CONTENT-ENCODING%CONTENT-LANGUAGE%CONTENT-LENGTH%CONTENT-TYPE%CSEQ%DATE%ENCRYPTION%ERROR-INFO%EVENT%EXPIRES%FROM%HIDE%IN-REPLY-TO%MAX-FORWARDS%MIME-VERSION%MIN-EXPIRES%MIN-SE%ORGANIZATION%P-ASSERTED-IDENTITY%P-PREFERRED-IDENTITY%PRIORITY%PRIVACY%PROXY-AUTHENTICATE%PROXY-AUTHORIZATION%PROXY-REQUIRE%RACK%RECORD-ROUTE%REFER-TO%REFERRED-BY%REMOTE-PARTY-ID%REPLACES%REPLY-TO%REQUIRE%RESPONSE-KEY%RETRY-AFTER%ROUTE%RSEQ%SERVER%SESSION-EXPIRES%SIP-ETAG%SIP-IF-MATCH%SUBJECT%SUBSCRIPTION-STATE%SUPPORTED%TIMESTAMP%TO%UNSUPPORTED%USER-AGENT%VIA%WARNING%WWW-AUTHENTICATE%U%I%M%E%L%C%F%O%R%B%X%S%K%T%V%";
char hdr_idx[748] = {
1, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 5, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 9, 0, 0, 0, 0, 0, 0, 0, 10, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 11, 0, 0, 0, 0, 0, 12, 
0, 0, 0, 0, 0, 0, 0, 13, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 15, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 17, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 18, 0, 0, 0, 0, 19, 0, 
0, 0, 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 21, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 22, 0, 0, 0, 0, 0, 23, 
0, 0, 0, 0, 0, 0, 0, 24, 0, 0, 0, 0, 25, 0, 0, 0, 
0, 26, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 27, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 28, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 29, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 30, 0, 0, 0, 0, 0, 0, 31, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 33, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
34, 0, 0, 0, 0, 0, 0, 0, 0, 35, 0, 0, 0, 0, 0, 0, 
0, 36, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 37, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 38, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 39, 0, 0, 0, 0, 40, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 41, 0, 0, 0, 0, 0, 0, 0, 
0, 42, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 43, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 44, 0, 0, 
0, 0, 0, 0, 0, 0, 45, 0, 0, 0, 0, 0, 0, 0, 0, 46, 
0, 0, 0, 0, 0, 0, 0, 47, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 48, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
49, 0, 0, 0, 0, 0, 50, 0, 0, 0, 0, 51, 0, 0, 0, 0, 
0, 0, 52, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 53, 0, 0, 0, 0, 0, 0, 0, 0, 54, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 55, 0, 0, 0, 0, 0, 0, 0, 
56, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 57, 0, 0, 0, 0, 0, 0, 0, 0, 0, 58, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 59, 0, 0, 60, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 61, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 62, 0, 0, 0, 63, 0, 0, 0, 0, 0, 0, 0, 64, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 65, 0, 
66, 0, 67, 0, 68, 0, 69, 0, 70, 0, 71, 0, 72, 0, 73, 0, 
74, 0, 75, 0, 76, 0, 77, 0, 78, 0, 79, 0};

const char* sipMethodTypeToName(SipMethodType methodtype)
{
	if((methodtype>=INVITE)&&(methodtype < UNKNOWN_METHOD))
		return sipMethodName[methodtype];
	else
		return sipMethodName[UNKNOWN_METHOD];
	
} /* sipMethodTypeToName */

/* For Method */
SipMethodType sipMethodNameToType(char* methodname)
{
  int idx=0;
  const char* method;

  if(!methodname)
	  return UNKNOWN_METHOD;

  methodname=trimWS((char*)methodname);
  for(idx=INVITE;idx<UNKNOWN_METHOD;idx++)  {
	method=sipMethodName[idx];
	/*if (toupper((int)*methodname)!=toupper((int)*method)) continue; */
	if (!strICmp((const char*)methodname,method))
		return (SipMethodType)idx;
  }
  return UNKNOWN_METHOD;
} /* sipMethodNameToType */

SipHdrType sipHdrNameToType(char* headername)
{
	int index,i=0;
	SipHdrType target=UnknSipHdr;
	char buf[64];
	char *ptr = NULL,*s;

	if(!headername || *headername=='\0' || strlen(headername)>62) 
		return UnknSipHdr;

	/*memset(buf,0,sizeof(buf));*/
	while (1) {
		buf[i++]=(char)toupper((int)*headername++);
		if (*headername=='\0') { 
			buf[i++]='%';
			buf[i]='\0';
			break;
		}
	}

	s=sipHdrChunk;
	while ((ptr =strstr(s,buf))) {
		index = hdr_idx[ptr - sipHdrChunk];
		if( index > 0){
			target=(SipHdrType)(index-1);
			break;
		}
		s=ptr+1;
	}

#ifdef ICL_IMS 
	if(UnknSipHdr==target){
		buf[i-1]='\0';
		for(i=P_Media_Authorization;i<=Service_Route;i++)  {	
			if (strICmp((const char*)buf,(const char*)sipHeaderName[i])==0){
				target=i;
				break;
			}
		}
	}
#endif /* end of ICL_IMS */

	/* real compare */
	if(target>UnknSipHdr) target=UnknSipHdr;
	return target;
}


/* For Header 
SipHdrType sipHdrNameToType(char* headername)
{
	int idx;
	SipMethodType target=Accept;
	int firstc;

	if(!headername || *headername=='\0') return UnknSipHdr;

	headername=trimWS(headername);

	firstc=toupper((int)*headername);
	switch(firstc) {
	case 'A':
		target=Accept;
		break;
	case 'C':
		target=Call_ID;
		break;
	case 'D':
		target=Date;
		break;
	case 'E':
		target=Encryption;
		break;
	case 'F':
		target=From;
		break;
	case 'H':
		target=Hide;
		break;
	case 'I':
		target=In_Reply_To;
		break;
	case 'M':
		target=Max_Forwards;
		break;
	case 'O':
		target=Organization;
		break;
	case 'P':
		target=P_Asserted_Identity;
		break;
	case 'R':
		target=RAck;
		break;
	case 'S':
		target=Server;
		break;
	case 'T':
		target=Timestamp;
		break;
	case 'U':
		target=Unsupported;
		break;
	case 'V':
		target=Via;
		break;
	case 'W':
		target=Warning;
		break;
	default:
		break;
	}
	
	for(idx=target;idx<UnknSipHdr;idx++)  {

	  if (!strICmp((const char*)headername,(const char*)sipHeaderName[idx])){

		if(HdrSetFlag==1){ 
			switch((SipHdrType)idx){
				case Accept_Encoding:
				case Accept_Language:       																								
				case Alert_Info:
				case Allow_Events:
				case Authentication_Info:    																								
				case Call_Info:      
				case Class:
				case Content_Disposition:   																								
				case Content_Encoding:      																								
				case Content_Language:      																								
				case Encryption:            																								
				case Error_Info:            																								
				case Hide:                  																								
				case In_Reply_To:           																								
				case MIME_Version:          																								
				case Min_Expires:  
				case Min_SE:
				case P_Asserted_Identity:
				case P_Preferred_Identity:
				case Priority:
				case Privacy:
				case Remote_Party_ID:       																								
				case Reply_To:             																								
				case Response_Key:          																								
				case Retry_After:           																								
				case Session_Expires:
				case Subscription_State:
				case Timestamp:             																								
				case Warning:
				case Allow_Events_Short:
				case Content_Encoding_Short:																								
				case Session_Expires_Short: 																								
				case Supported:             																								
				case Supported_Short:
					return UnknSipHdr;
					break;
				default:
					return (SipHdrType)idx;
			}
		}
		else
			return (SipHdrType)idx;
	  }
	}
	return UnknSipHdr;
} *//* sipHeaderNameToType */

const char* sipHdrTypeToName(SipHdrType hdrtype)
{
	if(hdrtype>UnknSipHdr)
		hdrtype=UnknSipHdr;
	return sipHeaderName[hdrtype];
}

SipRetMsg sipParseErrMsg(SipHdrType hdrtype)
{
	switch(hdrtype){
	case Accept:
		return SIP_ACCEPT_ERROR;
	case Accept_Encoding:	
		return SIP_ACCEPTENCODING_ERROR ;
	case Accept_Language:
		return SIP_ACCEPTLANGUAGE_ERROR;
	case Alert_Info: /* add by tyhuang*/
		return SIP_ALERTINFO_ERROR; 
	case Allow:
		return SIP_ALLOW_ERROR;
	case Allow_Events_Short:
	case Allow_Events:
		return SIP_ALLOWEVENTS_ERROR;
	case Authentication_Info:
		return SIP_AUTHENTICATIONINFO_ERROR;
	case Authorization:
		return SIP_AUTHORIZATION_ERROR;
	case Call_ID_Short:
	case Call_ID:
		return SIP_CALLID_ERROR;
	case Call_Info:/* add by tyhuang*/
		return SIP_CALLID_ERROR;
	case Class:/* add by tyhuang*/
		return SIP_CLASS_ERROR;
	case Contact_Short:
	case Contact:
		return SIP_CONTACT_ERROR;
	case Content_Disposition:
		return SIP_CONTENTDISPOSITION_ERROR;
	case Content_Encoding_Short:
	case Content_Encoding:
		return SIP_CONTENTENCODING_ERROR;
	case Content_Language:
		return SIP_CONTENTLANGUAGE_ERROR;
	case Content_Length_Short:
	case Content_Length:
		return SIP_CONTENTLENGTH_ERROR;
	case Content_Type_Short:
	case Content_Type:
		return SIP_CONTENTTYPE_ERROR;
	case CSeq:
		return SIP_CSEQ_ERROR;
	case Date:
		return SIP_DATE_ERROR;
	case Encryption:
		return SIP_ENCRYPTION_ERROR;
	case Error_Info:/* add by tyhuang*/
		return SIP_ERRORINFO_ERROR;
	case Expires:
		return SIP_EXPIRES_ERROR;
	case From_Short:
	case From:
		return SIP_FROM_ERROR;
	case Hide:
		return SIP_HIDE_ERROR;
	case In_Reply_To:/* add by tyhuang*/
		return SIP_INREPLYTO_ERROR;
	case Max_Forwards:
		return SIP_MAXFORWARD_ERROR;
	case MIME_Version: /* add by tyhuang*/
		return SIP_MIMEVERSION_ERROR;
	case Min_Expires: /* add by tyhuang*/
		return SIP_MINEXPIRES_ERROR;
	case Min_SE:
		return SIP_MINSE_ERROR;
	case Organization:
		return SIP_ORGANIZATION_ERROR;
	case P_Asserted_Identity:
		return SIP_PASSERTEDIDENTITY_ERROR;
	case P_Preferred_Identity:
		return SIP_PPREFERREDIDENTITY_ERROR;
	case Priority:
		return SIP_PRIORITY_ERROR;
	case Privacy:
		return SIP_PRIVACY_ERROR;
	case Proxy_Authenticate:
		return SIP_PROXYAUTHENTICATE_ERROR;
	case Proxy_Authorization:
		return SIP_PROXYAUTHORIZATION_ERROR;
	case Proxy_Require:
		return SIP_PROXYREQUIRE_ERROR;
	case Record_Route:
		return SIP_RECORDROUTE_ERROR;
	case Reply_To:/* add by tyhuang*/
		return SIP_REPLYTO_ERROR;
	case Require:
		return SIP_REQUIRE_ERROR;
	case Response_Key:
		return SIP_RESPONSEKEY_ERROR;
	case Retry_After:
		return SIP_RETRYAFTER_ERROR;
	case Route:
		return SIP_ROUTE_ERROR;
	case Server:
		return SIP_SERVER_ERROR;
	case Subject_Short:
	case Subject:
		return SIP_SUBJECT_ERROR;
	case Subscription_State:
		return SIP_SUBSCRIPTIONSTATE_ERROR;
	case Timestamp:
		return SIP_TIMESTAMP_ERROR;
	case To_Short:
	case To:
		return SIP_TO_ERROR;
	case Unsupported:
		return SIP_UNSUPPORTED_ERROR;
	case User_Agent:
		return SIP_USERAGENT_ERROR;
	case Via_Short:
	case Via:
		return SIP_VIA_ERROR;
	case Warning:
		return SIP_WARNING_ERROR;
	case WWW_Authenticate:
		return SIP_WWWAUTHENTICATE_ERROR;		
#ifdef ICL_IMS
	case P_Media_Authorization:
		SIP_P_MEDIA_AUTHENTICATION_ERROR;
	case Path:
		return SIP_PATH_ERROR;
	case P_Access_Network_Info:
		return SIP_P_ACCESS_NETWORK_INFO_ERROR;
	case P_Associated_URI:
		return SIP_P_ASSOCIATED_URI_ERROR;
	case P_Called_Party_ID:
		return SIP_P_CALLED_PARTY_ID_ERROR;
	case P_Charging_Function_Address:
		return SIP_P_CHARGING_FUNCTION_ADDRESS_ERROR;
	case P_Charging_Vector:
		return SIP_P_CHARGING_VECTOR_ERROR;
	case P_Visited_Network_ID:
		return SIP_P_VISITED_NETWORK_ID_ERROR;
	case Security_Client:
		return SIP_SECURITY_CLIENT_ERROR;
	case Security_Server:
		return SIP_SECURITY_SERVER_ERROR;
	case Security_Verify:
		return SIP_SECURITY_VERIFY_ERROR;
	case Service_Route:
		return SIP_SERVICE_ROUTE_ERROR;
#endif /* end of ICL_IMS */
	default:
		return SIP_UNKNOWN_HEADER;
	}

}

BOOL CheckHeaderName(MsgHdr ph, SipHdrType headertype)
{
  /* check the name is or not same as user's input */
	if (!ph || !ph->name) return FALSE;
	if (!strICmp(sipHeaderName[headertype],(const char*)ph->name))
		return TRUE;
	else
		return FALSE;
  
  /*if (!strICmp(sipHeaderName[headertype],trimWS((char*)ph->name)))
	return TRUE;
  else{
	return FALSE;
  }*/
} /* CheckHeaderName */

int RFC822ToSipHeader(MsgHdr ph, SipHdrType headertype, void **ret)
{
 /*if (CheckHeaderName(ph, headertype)){*/
    switch (headertype){
	case Accept:
		return AcceptParse(ph, ret);
	case Accept_Encoding:
		return AcceptEncodingParse(ph, ret);
	case Accept_Language:
		return AcceptLanguageParse(ph, ret);
	case Alert_Info:/* add by tyhuang*/
		return AlertInfoParse(ph,ret);
	case Allow:
		return AllowParse(ph, ret);
	case Allow_Events_Short:
	case Allow_Events:
		return AllowEventsParse(ph, ret);
	case Authentication_Info:
		return AuthenticationInfoParse(ph, ret);
	case Authorization:
		return AuthorizationParse(ph, ret);
	case Call_ID_Short:
	case Call_ID:
		return CallIDParse(ph, ret);
	case Call_Info:/* add by tyhuang*/
		return CallInfoParse(ph, ret);
	case Class:/* add by tyhuang */
		return ClassParse(ph, ret);
	case Contact_Short:
	case Contact:
		return ContactParse(ph, ret);
	case Content_Disposition:
		return ContentDispositionParse(ph, ret);
	case Content_Encoding_Short:
	case Content_Encoding:
		return ContentEncodingParse(ph, ret);
	case Content_Language:
		return ContentLanguageParse(ph, ret);
	case Content_Length_Short:
	case Content_Length:
		return ContentLengthParse(ph, ret);
	case Content_Type_Short:
	case Content_Type:
		return ContentTypeParse(ph, ret);
	case CSeq:
		return CSeqParse(ph, ret);
	case Date:
		return DateParse(ph, ret);
	case Encryption:
		return EncryptionParse(ph, ret);
	case Error_Info:/* add by tyhuang*/
		return ErrorInfoParse(ph, ret);
	case Event:
	case Event_Short:
		return EventParse(ph,ret);
	case Expires:
		return ExpiresParse(ph, ret);
	case From_Short:
	case From:
		return FromParse(ph, ret);
	case Hide:
		return HideParse(ph, ret);
	case In_Reply_To:
		return InReplyToParse(ph, ret);
	case Max_Forwards:
		return MaxForwardsParse(ph, ret);
	case MIME_Version: /* add by tyhuang*/
		return MIMEVersionParse(ph, ret);
	case Min_Expires: /* add by tyhuang*/
		return MinExpiresParse(ph, ret);
	case Min_SE:
		return MinSEParse(ph,ret);
	case Organization:
		return OrganizationParse(ph, ret);
	case P_Asserted_Identity:
		return PAssertedIdentityParse(ph, ret);
	case P_Preferred_Identity:
		return PPreferredIdentityParse(ph, ret);
	case Priority:
		return PriorityParse(ph, ret);
	case Privacy:
		return PrivacyParse(ph, ret);
	case Proxy_Authenticate:
		return ProxyAuthenticateParse(ph, ret);
	case Proxy_Authorization:
		return ProxyAuthorizationParse(ph, ret);
	case Proxy_Require:
		return ProxyRequireParse(ph, ret);
	case RAck:
		return RAckParse(ph, ret);
	case Record_Route:
		return RecordRouteParse(ph, ret);
	case Refer_To:
	case Refer_To_Short:
		return ReferToParse(ph,ret);
	case Referred_By:
	case Referred_By_Short:
		return ReferredByParse(ph,ret);
	case Remote_Party_ID:
		return RemotePartyIDParse(ph,ret);
	case Replaces:
		return ReplacesParse(ph,ret);
	case Reply_To:
		return ReplyToParse(ph,ret);
	case Require:
		return RequireParse(ph, ret);
	case Response_Key:
		return ResponseKeyParse(ph, ret);
	case Retry_After:
		return RetryAfterParse(ph, ret);
	case Route:
		return RouteParse(ph, ret);
	case RSeq:
		return RSeqParse(ph, ret);
	case Server:
		return ServerParse(ph, ret);
	case Session_Expires:
	case Session_Expires_Short:
		return SessionExpiresParse(ph,ret);
	case SIP_ETag:
		return SIPETagParse(ph,ret);
	case SIP_If_Match:
		return SIPIfMatchParse(ph,ret);
	case Subject_Short:
	case Subject:
		return SubjectParse(ph, ret);
	case Subscription_State:
		return SubscriptionStateParse(ph, ret);
	case Supported:
	case Supported_Short:
		return SupportedParse(ph, ret);
	case Timestamp:
		return TimestampParse(ph, ret);
	case To_Short:
	case To:
		return ToParse(ph, ret);
	case Unsupported:
		return UnsupportedParse(ph, ret);
	case User_Agent:
		return UserAgentParse(ph, ret);
	case Via_Short:
	case Via:
		return ViaParse(ph, ret);
	case Warning:
		return WarningParse(ph, ret);
	case WWW_Authenticate:
		return WWWAuthenticateParse(ph, ret);
#ifdef ICL_IMS
	case P_Media_Authorization:
		return PMAParse(ph, ret);
	case Path:
		return PathParse(ph, ret);
	case P_Access_Network_Info:
		return PANIParse(ph, ret);
	case P_Associated_URI:
		return PAUParse(ph, ret);
	case P_Called_Party_ID:
		return PCPIParse(ph, ret);
	case P_Charging_Function_Address:
		return PCFAParse(ph, ret);
	case P_Charging_Vector:
		return PCVParse(ph, ret);
	case P_Visited_Network_ID:
		return PVNIParse(ph, ret);
	case Security_Client:
		return SecurityClientParse(ph, ret);
	case Security_Server:
		return SecurityServerParse(ph, ret);
	case Security_Verify:
		return SecurityVerifyParse(ph, ret);
	case Service_Route:
		return ServiceRouteParse(ph, ret);
#endif /* end of ICL_IMS */
	default:
		TCRPrint(1,"<RFC822ToSipHeader>: Header Not Supported! \n ");
		return 0;
    }/*end switch case */
/*  }
  else if (headertype==UnknSipHdr)
	  return 1;	
  else
	  return 0; *//*  modified by tyhuang 2004.2.27,tmp->name and hdrtype is not match */
} /* RFC822ToSipHeader */

int AnalyzeParameter(unsigned char *buf, SipParam* _oneparm)
{
	char *equal;

	if(*buf == 0) return -1;
	equal = strChr(buf, '=');
	if (equal == NULL){
		_oneparm->name = strDup((const char*)trimWS((char*)buf));
		_oneparm->value = NULL;
	} else{
		*equal = '\0';
		_oneparm->name = strDup((const char*)trimWS((char*)buf));
		_oneparm->value = strDup((const char*)trimWS((char*)equal+1));
	}
	_oneparm->next = NULL;
	return 1;
} /* AnalyzeParameter */

int AnalyzeTimeParameter(unsigned char *buf, SipRfc1123Date **_date, int *_seconds)
{
	int iszero = 1;
	char *tmp;
	unsigned int idx;
  
	buf = (unsigned char*)trimWS((char*)strChr(buf, '=') + 1);
	if (*buf == '"')
		buf++;
	/* rfc1123date with a comma */
	tmp = strChr(buf, ',');
	if (tmp == NULL){ /* assume seconds */
		for(idx=0;idx<strlen((const char*)buf);idx++) {
			if (*(buf+idx) != '0'){
				iszero = 0 ;
				break;
			}
		}

		if (iszero) /* zero seconds */
			*_seconds = 0;
		else{
			*_seconds = atoi((const char*)buf);
			if (*_seconds == 0) /* error transfer */
				return 0;
		}
	}else {
		*_date =(SipRfc1123Date*) calloc(1,sizeof (**_date));
		RFCDateParse(buf, *_date);
	}
	return 1;
} /* AnalyzeTimeParameter */

int CollectDateData(unsigned char **buf, unsigned char *tmp, MsgHdrElm *nowElm)
{
	int i;
  
	tmp = (unsigned char*)trimWS((char*)tmp);
	/* collect Date */
	for(i=Mon; i<UnknWeek; i++){
		if (tmp == (unsigned char*)strstr((const char*)tmp, wkdayName[i])){
			if ((*nowElm)->next != NULL){
				*buf =(unsigned char*)realloc(*buf, strlen((const char*)*buf)+strlen((const char*)(*nowElm)->next->content)+2);
				strcat(*buf, ",");
				strcat(*buf, (*nowElm)->next->content);
				*nowElm = (*nowElm)->next;
				break;
			}
			else
				return 0;
		}
	}
	return 1;
}

int CollectCommentData(unsigned char **buf, unsigned char *tmp, MsgHdrElm *nowElm)
{
	int depth = 1;
	int slash = 0;
	int length;

	/* collect comment */
	while(1){
		if (*tmp == '\0'){
			if ((*nowElm)->next != NULL){
				length = strlen((const char*)*buf);
				*buf = (unsigned char *)realloc(*buf, length+strlen((const char*)(*nowElm)->next->content)+2);
				strcpy((*buf)+length, ",");
				length++;
				strcpy((*buf)+length, (*nowElm)->next->content);
				*nowElm = (*nowElm)->next;
			}else
				return 0;
		} else{
			if (!slash){
				switch (*tmp){
					case '(':depth++;
						break;
					case ')':depth--;
						if (depth==0)
							return 1;
						break;
					case '\\':
						slash = !slash;
						break;
				}/*end switch case */
			}/*end if !slash */
			else
				slash = 0;
			tmp++;
		}
	} /*end while loop */
} /* CollectCommentData */

int CollectQuotedString(unsigned char **buf, unsigned char *tmp, MsgHdrElm *nowElm)
{
	int length;

	if (strstr(tmp, "\"") == NULL) {
		while (strstr((*nowElm)->next->content, "\"") == NULL){
			length = strlen((const char*)*buf);
			*buf = (unsigned char*)realloc(*buf, length+strlen((const char*)(*nowElm)->next->content)+2);
			strcpy((*buf)+length, ",");
			length++;
			strcpy((*buf)+length, (*nowElm)->next->content);
			*nowElm = (*nowElm)->next;
		}
		length = strlen((const char*)*buf);
		*buf = (unsigned char*)realloc(*buf, length+strlen((const char*)(*nowElm)->next->content)+2);
		strcpy((*buf)+length, ",");
		length++;
		strcpy((*buf)+length, (*nowElm)->next->content);
		*nowElm = (*nowElm)->next;
		return 1;
	}else
		return 1;
} /* CollectQuotedString */

SipMsgType sipMsgIdentify(unsigned char *data)
{
	unsigned char buf[4],*tmpBuf;
	int method;
	SipMsgType msgtype=SIP_MSG_UNKNOWN;

	memcpy(buf, data, 3);
	buf[3] = '\0';
	if (!strcmp((const char*)buf, "SIP"))
		msgtype=SIP_MSG_RSP;
	else{
		tmpBuf=(unsigned char*)strDup((const char*)data);
		for(method=INVITE; method<UNKNOWN_METHOD; method++){
			if ((unsigned char*)strstr(data, sipMethodName[method]) == data) {
				if (tmpBuf[strlen((const char*)sipMethodName[method])] == ' ')
					msgtype=SIP_MSG_REQ;
				break; /* Quantway: Once identified, break the loop. -- ysc */
			}
		}
		free(tmpBuf);
	}
	return msgtype;
} /* sipMsgIdentify */

int AddrParse(unsigned char* addrstr, SipAddr *_addr)
{
	unsigned char* tmp;
	char* colon;

	if( !addrstr )
		return 1;
	addrstr = (unsigned char*)trimWS((char*)addrstr);
	tmp = (unsigned char*)strChr(addrstr, '<');
	_addr->next = NULL;
	if (tmp == NULL){
		_addr->display_name = NULL;
		_addr->addr = strDup((const char*)addrstr);
		_addr->with_angle_bracket = 0;
		return 1;
	} else {
		_addr->with_angle_bracket = 1;
		if (strChr(addrstr,'>') != NULL){      
			if (addrstr == tmp)
				_addr->display_name = NULL;
			else{
				*tmp = '\0';
				_addr->display_name = strDup((const char*)trimWS((char*)addrstr));
			} 
			tmp++;
			colon=strChr(tmp, '>');
			if(colon != NULL)
				*colon='\0';
#ifdef CCL_SIP_IGNORE
			colon=strChr(tmp,';');
			if(colon != NULL)
				*colon='\0';
#endif
			_addr->addr = strDup((const char*)trimWS((char*)tmp));
			return 1;
		}else{
			TCRPrint(1, "*** [sipStack] AddrParse error! \n");
			return 0;
		}
	}/*end else */
} /* AddrParse */

int AddrAsStr(SipAddr *_addr, unsigned char *buf, int buflen, int *length)
{
	DxStr AddStr;
	int retvalue;

	AddStr=dxStrNew();
	if (_addr->display_name != NULL)
		dxStrCat(AddStr, _addr->display_name);
	else
		dxStrCat(AddStr, "");

	if (_addr->with_angle_bracket)
		dxStrCat(AddStr, "<");

	dxStrCat(AddStr, _addr->addr);
	if (_addr->with_angle_bracket)
		dxStrCat(AddStr, ">");
	
	*length=dxStrLen(AddStr);
	if((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(AddStr));
		retvalue= 1;
	}
	dxStrFree(AddStr);
	return retvalue;
} /* AddrAsStr */

int ExtractComment(unsigned char *startpos, int *length)
{
	int depth = 1;
	int slash = 0;

	(*length) = 1;
	while (1){
		if (*startpos == '\0')
			return 0;
		else{
			if (!slash){
				switch (*startpos){
					case '(':
						depth++;
						break;
					case ')':
						depth--;
						if (depth==0){
							*startpos = '\0';
							return 1;
						}
						break;
					case '\\':
						slash = !slash;
						break;
				}/*end swich case */
			}else
				slash = 0;
			startpos++;
			(*length)++;
		}/*end else */
	}/*end while loop */
} /* ExtractComment */

int sipReqLineFree(SipReqLine *requestline)
{
	if(requestline != NULL){
		if(requestline->ptrRequestURI != NULL){
			free(requestline->ptrRequestURI);
			requestline->ptrRequestURI=NULL;
		}
		if(requestline->ptrSipVersion != NULL){
			free(requestline->ptrSipVersion);
			requestline->ptrSipVersion=NULL;
		}
		free(requestline);
		requestline=NULL;
	}
	return 1;
}

int sipRspStatusLineFree(SipRspStatusLine *responseline)
{
	if(responseline != NULL){
		if(responseline->ptrVersion != NULL){
			free(responseline->ptrVersion);
			responseline->ptrVersion=NULL;
		}
		if(responseline->ptrStatusCode != NULL){
			free(responseline->ptrStatusCode);
			responseline->ptrStatusCode=NULL;
		}
		if(responseline->ptrReason != NULL){
			free(responseline->ptrReason);
			responseline->ptrReason=NULL;
		}
		free(responseline);
		responseline=NULL;
	}
	return 1;
}
/* memory free by user */
int AcceptParse(MsgHdr ph, void **ret)
{
	unsigned char *strbuf,*freeptr,*ptmp;
	SipAccept* _accept=NULL;
	SipMediaType* _mediatype;
	SipParam* _parameter;
	MsgHdrElm nowElm;
	

	*ret = NULL;
	/* chech input data */
	if(!ph->first)
		return 0;
	/*_accept =(SipAccept*) malloc(sizeof(*_accept));*/
	_accept =(SipAccept*) calloc(1,sizeof(*_accept));
	if(_accept){
		_accept->numMediaTypes = ph->size;
		_accept->sipMediaTypeList = NULL;
	}else{
		TCRPrint(1, "*** [sipStack] AcceptParse memory alloc error! \n");
		return 0;
	}
	nowElm = ph->first;
	if(nowElm != NULL){
		/*_mediatype = (SipMediaType*)malloc(sizeof(*_mediatype));*/
		_mediatype = (SipMediaType*)calloc(1,sizeof(*_mediatype));
		_accept->sipMediaTypeList = _mediatype;
		_mediatype->numParam = 0;
		_mediatype->sipParamList = NULL;
		_mediatype->next = NULL;
	}else {
		TCRPrint(1, "*** [sipStack] AcceptParse error! \n");
		return 0;
	}
	while (1){
		strbuf=(unsigned char*)trimWS((char*)nowElm->content);
		if(!strbuf || (strlen(strbuf)==0)) break;
		strbuf =(unsigned char*)strDup((const char*)strbuf);
		freeptr=strbuf;
		ptmp = (unsigned char*)strChr(strbuf,'/');
		if (ptmp == NULL){
			/* free memory */
			if(freeptr) free(freeptr);
			if(_accept) AcceptFree(_accept);
			TCRPrint(1, "*** [sipStack] Accept header Parse error! \n");
			return 0;
		}
		else{
			*ptmp = '\0';
			_mediatype->type = strDup((const char*)strbuf);
			strbuf = ptmp;
			strbuf++;
		}

		ptmp = (unsigned char*)strChr(strbuf,';');
		if (ptmp == NULL){
/*			*ptmp='\0'; */
			_mediatype->subtype = strDup((const char*)strbuf);
		}
		else{
			*ptmp = '\0';
			_mediatype->subtype =strDup((const char*)strbuf);
			strbuf = ptmp;
			strbuf++;

			/*_parameter = (SipParam*)malloc(sizeof(*_parameter));*/
			_parameter = (SipParam*)calloc(1,sizeof(*_parameter));
			_parameter->next = NULL;
			_mediatype->sipParamList = _parameter;
			while(1){
				strbuf = (unsigned char*)trimWS((char*)strbuf);
				ptmp =(unsigned char*)strChr(strbuf,'=');
				if (ptmp == NULL){
					/* free memory */
					if(freeptr) free(freeptr);
					if(_accept) AcceptFree(_accept);
					TCRPrint(1, "*** [sipStack] Accept header Parse error in parameter! \n");
					return 0;
				}
				else{
					*ptmp = '\0';
					_parameter->name =strDup((const char*)strbuf);
					strbuf = ptmp;
					strbuf++;
				}
      
				strbuf =(unsigned char*) trimWS((char*)strbuf);
				ptmp = (unsigned char*)strChr(strbuf,';');
				if (ptmp == NULL)		{
					_mediatype->numParam++;
					/* *ptmp='\0'; */
					_parameter->value =strDup((const char*)strbuf);
					break;
				}
				else{
					_mediatype->numParam++;
					*ptmp = '\0';
					_parameter->value =strDup((const char*)strbuf);
					strbuf = ptmp + 1;
					/*_parameter->next = (SipParam*)malloc(sizeof(*_parameter));*/
					_parameter->next = (SipParam*)calloc(1,sizeof(*_parameter));
					_parameter = _parameter->next;
					_parameter->next = NULL;
				}/*end else*/
			}/*end while(1) */
		}/*end else*/
		if(freeptr) free(freeptr);

		if (nowElm == ph->last)
			break;
		else{
			nowElm = nowElm->next;
			/*_mediatype->next = (SipMediaType*)malloc(sizeof(*_mediatype));*/
			_mediatype->next = (SipMediaType*)calloc(1,sizeof(*_mediatype));
			_mediatype = _mediatype->next;
			_mediatype->next = NULL;
			_mediatype->numParam = 0;
			_mediatype->sipParamList = NULL;
		}
	}/*end while (1)*/
	*ret = (void*)_accept;
	return 1;
} /* AcceptParse */

int AcceptAsStr(SipAccept* _accept, unsigned char *buf, int buflen, int *length)
{
	int i,j;
	SipMediaType* _mediatype;
	SipParam* _parameter;
	DxStr AcceptStr;
	int retvalue;

	AcceptStr=dxStrNew();
	dxStrCat(AcceptStr,"Accept:");
	_mediatype = _accept->sipMediaTypeList;

	for(i=0;i<_accept->numMediaTypes;i++){
		dxStrCat(AcceptStr,_mediatype->type);
		dxStrCat(AcceptStr, "/");
		dxStrCat(AcceptStr, _mediatype->subtype);
    
		_parameter = _mediatype->sipParamList;
		for(j=0;j<_mediatype->numParam;j++){
			dxStrCat(AcceptStr, ";");
			dxStrCat(AcceptStr, _parameter->name);
			dxStrCat(AcceptStr, "=");
			dxStrCat(AcceptStr, _parameter->value);
      
			_parameter = _parameter->next;
		} /*end for loop j*/
		_mediatype = _mediatype->next;
		if (_mediatype != NULL)
			dxStrCat(AcceptStr, ",");
	} /*end fore loop i*/
	dxStrCat(AcceptStr, "\r\n");
	*length=dxStrLen(AcceptStr);

	if ((*length) >= buflen){
		retvalue= 0;
	}else{
		strcpy(buf,(const char*)dxStrAsCStr(AcceptStr));
		retvalue= 1;
	}
	dxStrFree(AcceptStr);
	return retvalue;
} /* AcceptAsStr */

int AcceptFree(SipAccept* _accept)
{
	if(_accept != NULL){
		if(_accept->sipMediaTypeList != NULL){
			sipMediaTypeFree(_accept->sipMediaTypeList);
			_accept->sipMediaTypeList=NULL;
		}
		free(_accept);
		_accept=NULL;
	}
	return 1;
} /* AcceptFree */


int AcceptEncodingParse(MsgHdr ph, void **ret)
{
	char              *strbuf,*freeptr, *ptmp;
	SipAcceptEncoding *_acceptencoding=NULL;
	SipCoding	  *_coding;
	MsgHdrElm         nowElm;
  
	*ret = NULL;
	/* chech input data */
	if(!ph->first)
		return 0;
	/*_acceptencoding = (SipAcceptEncoding *)malloc(sizeof(*_acceptencoding));*/
	_acceptencoding = (SipAcceptEncoding *)calloc(1,sizeof(*_acceptencoding));
	_acceptencoding->numCodings = ph->size;
	nowElm = ph->first;
	if(nowElm != NULL){
		/*_coding = (SipCoding*)malloc(sizeof(*_coding));*/
		_coding = (SipCoding*)calloc(1,sizeof(*_coding));
		_coding->next = NULL;
		_coding->coding = NULL;
		_coding->qvalue = NULL;
		_acceptencoding->sipCodingList = _coding;
	}else{
		TCRPrint(1, "*** [sipStack] AcceptEncoding memory alloc error! \n");
		return 0;
	}
	while (1){
		strbuf=trimWS((char*)nowElm->content);
		if(!strbuf || (strlen(strbuf)==0)) break;
		strbuf = strDup((const char*)strbuf);
		freeptr=strbuf;

		ptmp = strchr(strbuf,';');
		if (ptmp == NULL)
			_coding->coding = strDup((const char*)strbuf);
		else{
			*ptmp = '\0';
			_coding->coding = strDup((const char*)strbuf);
			strbuf = ptmp;
			strbuf++;

			ptmp = strchr((const char*)strbuf,'=');
			if (ptmp == NULL){
				/* free memory*/
				if(freeptr) free(freeptr);
				TCRPrint(1, "*** [sipStack] AcceptEncoding Parse error! \n");
				break;
			}
			else{
				if (*(ptmp-1)!='q'){
					/* free memory*/
					if(freeptr) free(freeptr);
					TCRPrint(1, "*** [sipStack] AcceptEncoding Parse error with q value! \n");
					break;
				}
				else{
					ptmp++;
					_coding->qvalue = strDup((const char*)ptmp);
				}
			}
		}/*end else ptmp= NULL*/
		if(freeptr) free(freeptr);

		if (nowElm == ph->last)
			break;
		else{
			nowElm = nowElm->next;
			/*_coding->next = (SipCoding*)malloc(sizeof(*_coding));*/
			_coding->next = (SipCoding*)calloc(1,sizeof(*_coding));
			_coding = _coding->next;
			_coding->next = NULL;
			_coding->coding = NULL;
			_coding->qvalue = NULL;
		}
	}/*end while(1)*/
	*ret = (void*)_acceptencoding;
	return 1;
} /* AcceptEncodingParse */

int AcceptEncodingAsStr(SipAcceptEncoding *_acceptencoding, unsigned char *buf, int buflen, int *length)
{
	int		i,retvalue;
	SipCoding	*_coding;
	DxStr		AcceptEncodingStr;

	AcceptEncodingStr=dxStrNew();

	dxStrCat(AcceptEncodingStr, "Accept-Encoding:");  
	_coding = _acceptencoding->sipCodingList;
	for(i=0;i<_acceptencoding->numCodings;i++){
		dxStrCat(AcceptEncodingStr,  _coding->coding);
		if (_coding->qvalue != NULL){
			dxStrCat(AcceptEncodingStr, ";q=");
			dxStrCat(AcceptEncodingStr, _coding->qvalue);
		}
		_coding = _coding->next;
		if (_coding != NULL)
			dxStrCat(AcceptEncodingStr, ",");
	}/*end for loop i*/
	dxStrCat(AcceptEncodingStr, "\r\n");/* Add CRLF */
	*length=dxStrLen(AcceptEncodingStr);

	if ((*length) >= buflen){
		retvalue= 0;
	}else{
		strcpy(buf,(const char*)dxStrAsCStr(AcceptEncodingStr));
		retvalue= 1;
	}
	dxStrFree(AcceptEncodingStr);
	return retvalue;

} /* AcceptEncodingAsStr */

int AcceptEncodingFree(SipAcceptEncoding *_acceptencoding)
{
	if(_acceptencoding != NULL){
		if(_acceptencoding->sipCodingList){
			sipCodingFree(_acceptencoding->sipCodingList);
			_acceptencoding->sipCodingList=NULL;
		}
		free(_acceptencoding);
		_acceptencoding=NULL;
	}
	return 1;
} /* AcceptEncodingFree */

int AcceptLanguageParse(MsgHdr ph, void **ret)
{
	unsigned char *strbuf,*freeptr, *ptmp;
	SipAcceptLang *_acceptlanguage=NULL;
	SipLang *_language;
	MsgHdrElm nowElm;

	*ret = NULL;
	/* chech input data */
	if(!ph->first)
		return 0;
	/*_acceptlanguage = (SipAcceptLang*)malloc(sizeof(*_acceptlanguage)); */
	_acceptlanguage = (SipAcceptLang*)calloc(1,sizeof(*_acceptlanguage));
	if(_acceptlanguage != NULL){
		_acceptlanguage->numLangs = ph->size;
		_acceptlanguage->sipLangList=NULL;
	}else{
		TCRPrint(1, "*** [sipStack] AcceptLanguage memory alloc error! \n");
		return 0;
	}
	nowElm = ph->first;
	if(nowElm != NULL){
		/*_language = (SipLang*)malloc(sizeof(*_language));*/
		_language = (SipLang*)calloc(1,sizeof(*_language));
		_language->qvalue = NULL;
		_language->next = NULL;
		_acceptlanguage->sipLangList = _language;
	}else{
		TCRPrint(1, "*** [sipStack] AcceptLanguage Parse error! \n");
		return 0;
	}
	while (1){
		if(strlen(trimWS((char*)nowElm->content))==0) break;
		strbuf = (unsigned char*)strDup((const char*)trimWS((char*)nowElm->content));
		freeptr=strbuf;

		ptmp = (unsigned char*)strChr(strbuf,';');
		if (ptmp == NULL)
			_language->lang = strDup((const char*)strbuf);
		else{
			*ptmp = '\0';
			_language->lang = strDup((const char*)strbuf);
			strbuf = ptmp;
			strbuf++;

			strbuf = (unsigned char*)trimWS((char*)strbuf);
			ptmp = (unsigned char*)strstr(strbuf,"=");
			if (ptmp == NULL){
				/* free memory*/
				if(freeptr) free(freeptr);
				if(_acceptlanguage) AcceptLanguageFree(_acceptlanguage);
				TCRPrint(1, "*** [sipStack] AcceptLanguage Parse error! \n");
				return 0;
			}
			else{
				ptmp++;
				_language->qvalue = strDup((const char*)trimWS((char*)ptmp));
			}
		}
		free(freeptr);
		if (nowElm == ph->last)
			break;
		else{
			nowElm = nowElm->next;
			_language->next =(SipLang*)calloc(1,sizeof(*_language));
			_language = _language->next;
			_language->next = NULL;
			_language->qvalue = NULL;
		}
	}/*end while loop*/
	*ret = (void*)_acceptlanguage;
	return 1;
} /* AcceptLanguageParse */

int AcceptLanguageAsStr(SipAcceptLang *_acceptlanguage, unsigned char *buf, int buflen, int *length)
{
	int i,retvalue;
	SipLang *_language;
	DxStr AcceptLangStr;

	AcceptLangStr=dxStrNew();

	dxStrCat(AcceptLangStr,"Accept-Language:");  
	_language = _acceptlanguage->sipLangList;
	for(i=0;i<_acceptlanguage->numLangs;i++){
		dxStrCat(AcceptLangStr,  _language->lang);
		if (_language->qvalue != NULL){
			dxStrCat(AcceptLangStr, ";q=");
			dxStrCat(AcceptLangStr, _language->qvalue);
		}
		_language = _language->next;
		if (_language != NULL)
			dxStrCat(AcceptLangStr, ",");
	}
	dxStrCat(AcceptLangStr,"\r\n");  /* Add CRLF */
	*length=dxStrLen(AcceptLangStr);

	if ((*length) >= buflen){
		retvalue= 0;
	} else{
		strcpy(buf,(const char*)dxStrAsCStr(AcceptLangStr));
		retvalue= 1;
	}
	dxStrFree(AcceptLangStr);
	return retvalue;
} /* AcceptLanguageAsStr */

int AcceptLanguageFree(SipAcceptLang *_acceptlanguage)
{
	if(_acceptlanguage != NULL){
		if(_acceptlanguage->sipLangList != NULL){
			sipLanguageFree(_acceptlanguage->sipLangList);
			_acceptlanguage->sipLangList=NULL;
		}
		free(_acceptlanguage);
		_acceptlanguage=NULL;
	}
	return 1;
} /* AcceptLanguageFree */

int AlertInfoParse(MsgHdr ph, void **ret)
{
	SipInfo	*_alertinfo=NULL,*tmpalertinfo;
	SipParam	*_parameter;
	MsgHdrElm	nowElm;
	unsigned char	*buf ,*freeptr,*tmp,*parms;
	int		count,retvalue=1;	
	
	*ret = NULL;
	/* chech input data */
	if(!ph->first)
		return 0;
	
	_alertinfo = (SipInfo*)calloc(1,sizeof(*_alertinfo));
	if(_alertinfo!=NULL){
		_alertinfo->absoluteURI=NULL;
		_alertinfo->numParams=0;
		_alertinfo->SipParamList=NULL;
		_alertinfo->next=NULL;
	}else{
		TCRPrint(1, "*** [sipStack] AlertInfo memory alloc error! \n");
		return 0;
	}
	tmpalertinfo=_alertinfo;
	_parameter=NULL;
	nowElm = ph->first;

	while(1){
	/* parse absoluteURI */
		buf = (unsigned char *)strDup((const char*)trimWS((char*)nowElm->content));
		freeptr=buf;
		tmp=(unsigned char *)strChr(buf,'<');
		if(tmp != NULL){
			buf=tmp+1;
			tmp=(unsigned char *)strChr(buf,'>');
			if(tmp != NULL){
				*tmp='\0';
				_alertinfo->absoluteURI=strDup((const char*)buf);
				buf=tmp+1;
				buf=(unsigned char*)trimWS((char*)buf);
				
			}
		}else {
			_alertinfo->absoluteURI=NULL;
			retvalue=0;
			if(freeptr) free(freeptr);
			TCRPrint(1, "*** [sipStack] AlertInfo Parse error with URI=NULL! \n");
			break;
		}
	/* parse generic-param  */
		
		parms=(unsigned char *)strChr(buf,';');
		count=0;
		if(parms != NULL){
			*parms='\0';
			parms++;
			
			while(1){
				count++;
				tmp = (unsigned char *)strChr(parms, ';');
				if(_parameter==NULL)	
						_parameter =(SipParam*) calloc(1,sizeof(*_parameter));
				if(count==1)
						_alertinfo->SipParamList=_parameter;
				if (tmp == NULL){ /* only one parameter*/
					_parameter->next = NULL;
					AnalyzeParameter(parms,_parameter);
					break;
				}else{
					*tmp = '\0';
			
					AnalyzeParameter(parms,_parameter);
					parms=tmp+1;
					_parameter->next=(SipParam*) calloc(1,sizeof(*_parameter));
					_parameter=_parameter->next;
					_parameter->name=NULL;
					_parameter->value=NULL;
					_parameter->next=NULL;
				}
				
			}
			
		}
		_alertinfo->numParams=count;
		if(freeptr!=NULL)
			free(freeptr);
		if (nowElm->next != NULL){ 
			nowElm = nowElm->next;
			
			_alertinfo->next=(SipInfo*)calloc(1,sizeof(*_alertinfo));
			_alertinfo=_alertinfo->next;
			_alertinfo->numParams=0;
			_alertinfo->absoluteURI=NULL;
			_parameter=NULL;
			_alertinfo->SipParamList=NULL;
		}
		else
			break;
	}/*end while */
	*ret=(void *)tmpalertinfo;
	return 1;
} /* AlertInfoParse tyhuang*/

int AlertInfoAsStr(SipInfo *_alertinfo, unsigned char *buf, int buflen, int *length)
{
	DxStr AlertInfoStr;
	int retvalue;
	SipParam* _parameter;

	AlertInfoStr=dxStrNew();
	while(1){
		if(_alertinfo==NULL) break;
		dxStrCat(AlertInfoStr, "Alert-Info: <");
		dxStrCat(AlertInfoStr,(const char*)_alertinfo->absoluteURI);
		dxStrCat(AlertInfoStr,">");
		_parameter=_alertinfo->SipParamList;
		while (1) {
			if (_parameter != NULL){
				dxStrCat(AlertInfoStr,";");
				dxStrCat(AlertInfoStr, _parameter->name);
				if (_parameter->value != NULL){
					dxStrCat(AlertInfoStr, "=");/*for equal */
					dxStrCat(AlertInfoStr, _parameter->value);
				}
				_parameter = _parameter->next;
			} else
				break;
		}/*end while loop*/
		
		if(_alertinfo->next==NULL) break;
		else{
			_alertinfo=_alertinfo->next;
			dxStrCat(AlertInfoStr,",");
		}
	}/*end while loop */

	dxStrCat(AlertInfoStr, "\r\n");
	*length=dxStrLen(AlertInfoStr);

	if ((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(AlertInfoStr));
		retvalue= 1;
	}
	dxStrFree(AlertInfoStr);
	return retvalue;
}/*AlertInfoAsStr tyhuang*/

int AlertInfoFree(SipInfo *_alertinfo)
{
	SipInfo *tmpalertinfo;

	while(_alertinfo!=NULL){
		if(_alertinfo->absoluteURI!=NULL){ 
			free(_alertinfo->absoluteURI);
			_alertinfo->absoluteURI=NULL;
		}
		if(_alertinfo->SipParamList!=NULL){
			sipParameterFree(_alertinfo->SipParamList);
			_alertinfo->SipParamList=NULL;
		}
		tmpalertinfo=_alertinfo->next;
		free(_alertinfo);
		_alertinfo=tmpalertinfo;
	}
	return 1;
}/* AlertInfoFree tyhuang*/


int AllowParse(MsgHdr ph, void **ret)
{
	SipAllow  *_allow=NULL;
	MsgHdrElm nowElm;
	unsigned char *buf;
	int i;

	*ret = NULL;
	/* chech input data */
	if(!ph->first)
		return 0;
	
	_allow = (SipAllow*)calloc(1,sizeof(*_allow));
	for(i=INVITE; i<UNKNOWN_METHOD; i++)
		_allow->Methods[i] = 0;
	nowElm = ph->first;
	while (1){
		buf = (unsigned char*)strDup((const char*)trimWS((char*)nowElm->content));
		for(i=INVITE; i<UNKNOWN_METHOD; i++) {
			if (!strcmp(buf, sipMethodName[i])){
				_allow->Methods[i] = 1;
				break;
			}
		}
		free(buf);
		if (nowElm == ph->last)
			break;
		else
			nowElm = nowElm->next;
	}
	*ret = (void*)_allow;
	return 1;
} /* AllowParse */

int AllowAsStr(SipAllow *_allow, unsigned char *buf, int buflen, int *length)
{
	int i,retvalue;
	DxStr AllowStr;
	BOOL first=TRUE;

	AllowStr=dxStrNew();
	dxStrCat(AllowStr, "Allow:");
	for(i=INVITE; i<UNKNOWN_METHOD; i++){
		if (_allow->Methods[i] == 1) {
			if(!first)
				dxStrCat(AllowStr, ","); /*comma */
			dxStrCat(AllowStr,(char*) sipMethodName[i]);
			first=FALSE;
		}
	}
	dxStrCat(AllowStr,"\r\n");
	*length = dxStrLen(AllowStr);

	if((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(AllowStr));
		retvalue= 1;
	}
	dxStrFree(AllowStr);
	return retvalue;
} /* AllowAsStr */

int AllowFree(SipAllow *_allow)
{
	if(_allow != NULL){
		free(_allow);
		_allow=NULL;
	}
	return 1;
} /* AllowFree */

int AllowEventsParse(MsgHdr ph, void **ret)
{
	SipAllowEvt	*_allowevt=NULL,*tmpallowevt;
	MsgHdrElm	nowElm;
	unsigned char	*buf;
	SipStr		*str,*prestr;
	char		*dot,*c;
	int		strcount=0;
	BOOL		btmp=TRUE;
	
	*ret = NULL;
	/* chech input data */
	if(!ph->first)
		return 0;
	nowElm=ph->first;
	_allowevt=(SipAllowEvt*)calloc(1,sizeof(*_allowevt));
	if(_allowevt!=NULL){
		_allowevt->pack=NULL;
		_allowevt->tmplate=NULL;
		_allowevt->next=NULL;
		tmpallowevt=_allowevt;
	}else{
		TCRPrint(1, "*** [sipStack] AllowEvents memory alloc error! \n");
		return 0;
	}

	if(nowElm->content != NULL)
		buf=(unsigned char*)strDup((const char*)trimWS(nowElm->content));
	while(1){	
		dot=(char*)strChr(buf,'.');
		if(dot==NULL) /* without template */
			_allowevt->pack=strDup((const char*)buf);
		else{ /* with template */
			*dot=0;
			_allowevt->pack=strDup((const char*)buf);
			while(btmp){
				str=(SipStr*)calloc(1,sizeof(*str));
				str->next=NULL;
				if((c=strChr((unsigned char*)dot+1,'.'))!=0)
					*c='\0';
				else
					btmp=FALSE;

				str->str=strDup(dot+1);
				if(strcount==0)
					_allowevt->tmplate=str;
				else
					prestr->next=str;
				prestr=str;
				dot=c;
				strcount++;
			}
		}
		if( buf != NULL ) free(buf);
		if (nowElm->next != NULL){ 
			nowElm = nowElm->next;
			_allowevt->next=(SipAllowEvt*)calloc(1,sizeof(*_allowevt));
			_allowevt=_allowevt->next;
			_allowevt->pack=NULL;
			_allowevt->tmplate=NULL;
			_allowevt->next=NULL;
			if(nowElm->content != NULL)
				buf=(unsigned char*)strDup((const char*)trimWS(nowElm->content));
		}else
			break;
	}/* end while */
	*ret = (void*)tmpallowevt;
	return 1;
} /* Allow-Events Parse */

int AllowEventsAsStr(SipAllowEvt *_allowevt, unsigned char *buf, int buflen, int *length)
{
	int retvalue;
	SipStr *str;
	DxStr AllowEventsStr;

	AllowEventsStr=dxStrNew();
	dxStrCat(AllowEventsStr, "Allow-Events:");
	while(1){
		if(_allowevt->pack != NULL){
			dxStrCat(AllowEventsStr,_allowevt->pack);
			str=_allowevt->tmplate;
			while(str != NULL){
				dxStrCat(AllowEventsStr,".");
				dxStrCat(AllowEventsStr,str->str);
				str=str->next;
			}/* end while str */
		}
		if(_allowevt->next != NULL){
			dxStrCat(AllowEventsStr,",");
			_allowevt=_allowevt->next;
		}else
			break;
	}/* end while */
	dxStrCat(AllowEventsStr, "\r\n");
	*length=dxStrLen(AllowEventsStr);
	if((*length) >buflen)
		retvalue=0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(AllowEventsStr));
		retvalue=1;
	}
	dxStrFree(AllowEventsStr);
	return retvalue;
} /* Allow-Events AsStr */

int AllowEventsFree(SipAllowEvt *_allowevt)
{
	SipAllowEvt	*tmp;

	while(_allowevt != NULL){
		if(_allowevt->pack!= NULL ){
			free(_allowevt->pack);
			_allowevt->pack=NULL;
		}
		if(_allowevt->tmplate != NULL){
			sipStrFree(_allowevt->tmplate);
			_allowevt->tmplate=NULL;
		}
		tmp=_allowevt;
		_allowevt=_allowevt->next;
		free(tmp);
		tmp=NULL;
	}
	return 1;
} /* Allow-Events Free */


int AuthenticationInfoParse(MsgHdr ph, void **ret)
{
	SipAuthInfo	*_authinfo=NULL;
	SipParam	*_parameter;
	MsgHdrElm	nowElm;
	unsigned char	*buf,*freeptr;
	int		count=0;	
	
	*ret = NULL;
	/* chech input data */
	if(!ph->first)
		return 0;
	
	_authinfo = (SipAuthInfo*)calloc(1,sizeof(*_authinfo));
	
	if(_authinfo != NULL){
		_authinfo->numAinfos=0;
		_authinfo->ainfoList=NULL;
	}else{
		TCRPrint(1, "*** [sipStack] Authenticate-Info memory alloc error! \n");
		return 0;
	}
	_parameter=NULL;
	nowElm = ph->first;

	while(1){
		buf = (unsigned char*)strDup((const char*)trimWS((char*)nowElm->content));
		freeptr=buf;
		/* parse generic-param  */
		if(count==0){
			_parameter=(SipParam*) calloc(1,sizeof(*_parameter));	
			_authinfo->ainfoList=_parameter;
		}
		AnalyzeParameter(buf,_parameter);
		_authinfo->numAinfos++;
		free(freeptr);
		/* check parameter type */
		/*if((strICmp(_parameter->name,"nextnonce")==0)||(strICmp(_parameter->name,"rspauth")==0)){
			_parameter->value=trimWS((char *)_parameter->value);
			freeparm=_parameter->value;
			_parameter->value++;
			tmp=(unsigned char *)strstr(_parameter->value,"\"");
			if(tmp == NULL){
				TCRPrint(1, "*** [sipStack] AuthenticationInfo Parse error in parameter! \n");
				break;
			}
			*tmp='\0';
			tmp=strDup(_parameter->value);
			free(freeparm);
			_parameter->value=tmp;
		}*/
		if(nowElm->next != NULL){
			_parameter->next=(SipParam*) calloc(1,sizeof(*_parameter));
			_parameter=_parameter->next;
			_parameter->name=NULL;
			_parameter->value=NULL;
			_parameter->next=NULL;
			count++;
			nowElm=nowElm->next;
		}else
			break;
	}/* end while */
	_parameter->next=NULL;
	*ret=(void *)_authinfo;
	return 1;
} /* AuthenticationInfoParse tyhuang */

int AuthenticationInfoAsStr(SipAuthInfo *_authinfo, unsigned char *buf, int buflen, int *length)
{
	DxStr AuthInfoStr;
	int retvalue;
	SipParam *_parameter;

	AuthInfoStr=dxStrNew();

		if(_authinfo==NULL) return 0;
		dxStrCat(AuthInfoStr, "Authentication-Info:");
		_parameter=_authinfo->ainfoList;
		while (1) {
			if (_parameter != NULL){
				dxStrCat(AuthInfoStr, _parameter->name);
				dxStrCat(AuthInfoStr, "=");/*for equal 	*/
				
				if((strcmp(_parameter->name,"nc")==0)||(strcmp(_parameter->name,"qop")==0))
					dxStrCat(AuthInfoStr,unquote(_parameter->value));
				else
					dxStrCat(AuthInfoStr, _parameter->value);
				 _parameter = _parameter->next;
				if(_parameter != NULL ) dxStrCat(AuthInfoStr,",");
			} else
				break;
		}/* end while loop */
		
	

	dxStrCat(AuthInfoStr, "\r\n");
	*length=dxStrLen(AuthInfoStr);

	if ((*length) > buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(AuthInfoStr));
		retvalue= 1;
	}
	dxStrFree(AuthInfoStr);
	return retvalue;
}/* AuthenticationInfoAsStr tyhuang */

int AuthenticationInfoFree(SipAuthInfo *_authinfo)
{

	if(_authinfo != NULL){
		if(_authinfo->ainfoList != NULL){
			sipParameterFree(_authinfo->ainfoList);
			_authinfo->ainfoList=NULL;
		}
		free(_authinfo);
		_authinfo=NULL;
	}
	return 1;
}/* AuthenticationInfoFree tyhuang */

int AuthorizationParse(MsgHdr ph, void **ret)
{
	SipAuthz	*_authorization=NULL;
	SipParam	*_parameter;
	MsgHdrElm	nowElm;
	unsigned char	*buf,*freeptr, *tmp;
	int		count=0;
	int		doing = TRUE;

	*ret = NULL;
	/* chech input data */
	if(!ph->first)
		return 0;

	_authorization = (SipAuthz*)calloc(1,sizeof(*_authorization));
	if(_authorization != NULL){
		_authorization->numParams = ph->size;
		_authorization->next=NULL;
	}else{
		TCRPrint(1, "*** [sipStack] Authorization memory alloc error! \n");
		return 0;
	}
	nowElm = ph->first;
	buf = (unsigned char*)strDup((const char*)trimWS((char*)nowElm->content));
	freeptr=buf;

	/* scheme */
	tmp =(unsigned char*) strChr(buf, ' ');
	if (tmp == NULL){
		TCRPrint(1, "*** [sipStack] Authorization Parse error! \n");
		if(buf)	free(buf);
		if(_authorization) free(_authorization);
		return 0;
	}else {
		*tmp = '\0';
		_authorization->scheme = strDup((const char*)buf);
		_parameter =(SipParam*) calloc(1,sizeof(*_parameter));
		_authorization->sipParamList = _parameter;
		buf=(tmp+1);
		if(buf!=NULL){
			count++;
			AnalyzeParameter(buf,_parameter);
			if(ph->size ==1){
				_parameter->next=NULL;
				doing=FALSE;
			}else{
				doing=TRUE;
				_parameter->next=(SipParam*)calloc(1,sizeof(*_parameter));
				_parameter=_parameter->next;
				_parameter->name=NULL;
				_parameter->value=NULL;
				_parameter->next=NULL;
			}
		}
	}
	free(freeptr);

	/* parameter */
	nowElm = nowElm->next;

	buf = (nowElm)? ((unsigned char*)strDup((const char*)trimWS((char*)nowElm->content))):((unsigned char*)NULL);
	freeptr=buf;
	while (doing){
		count++;
		AnalyzeParameter(buf, _parameter);
		if(nowElm==ph->last) 
			break;
		nowElm = nowElm->next;
		_parameter->next =(SipParam*) calloc(1,sizeof(*_parameter));
		_parameter = _parameter->next;
		/*buf = (unsigned char*)strDup((const char*)trimWS((char*)nowElm->content));*/
		buf = (unsigned char*)trimWS((char*)nowElm->content);
	}
	if(freeptr != NULL)
		free(freeptr);
	_authorization->numParams = count;
	*ret = (void *)_authorization;
	return 1;
}/* AuthorizationParse */

int AuthorizationAsStr(SipAuthz *_authorization, unsigned char *buf, int buflen, int *length)
{
	SipParam* _parameter;
	SipAuthz *tmpAuthz;
	int i,retvalue;
	DxStr AuthorStr;

	AuthorStr=dxStrNew();

	tmpAuthz=_authorization;
	while(tmpAuthz!=NULL){
		dxStrCat(AuthorStr,"Authorization:");
		dxStrCat(AuthorStr, tmpAuthz->scheme);
		dxStrCat(AuthorStr, " ");
		_parameter = tmpAuthz->sipParamList;
		if (_parameter) {
			for(i=0; i<tmpAuthz->numParams; i++) {
				if (_parameter->name)
					dxStrCat(AuthorStr, _parameter->name);
				if( _parameter->value ) {
					dxStrCat(AuthorStr,"=");
					if((strcmp(_parameter->name,"nc")==0)||(strcmp(_parameter->name,"qop")==0))
						dxStrCat(AuthorStr,unquote(_parameter->value));
					else
						dxStrCat(AuthorStr, _parameter->value);
				}
				if (_parameter->next != NULL)
					dxStrCat(AuthorStr,",");
				_parameter = _parameter->next;
			}
		}
		dxStrCat(AuthorStr, "\r\n");
		tmpAuthz=tmpAuthz->next;
	}
	*length=dxStrLen(AuthorStr);
	if((*length)>buflen){
		retvalue= 0;
	}else{
		strcpy(buf,(const char*)dxStrAsCStr(AuthorStr));
		retvalue= 1;
	}
	dxStrFree(AuthorStr);
	return retvalue;

} /* AuthorizationAsStr */

int AuthorizationFree(SipAuthz *_authorization)
{
	SipAuthz	*tmpAuthz,*nowAuthz;
	nowAuthz=_authorization;
	while(nowAuthz!=NULL){
		if(nowAuthz->scheme!=NULL){
			free(nowAuthz->scheme);
			nowAuthz->scheme=NULL;
		}
		if(nowAuthz->sipParamList){
			sipParameterFree(nowAuthz->sipParamList);
			nowAuthz->sipParamList=NULL;
		}
		tmpAuthz=nowAuthz->next;
		free(nowAuthz);
		nowAuthz=tmpAuthz;
	}
	return 1;
} /* AuthorizationFree */

int CallIDParse(MsgHdr ph, void **ret)
{
	unsigned char *callid;
	
	*ret=NULL;
	/* check input data */
	if(!ph->first || !ph->first->content)
		return 0;
	callid = (unsigned char*)strDup((const char*)trimWS((char*)ph->first->content));
	*ret = (void *)callid;
	return 1;
} /* CallIDParse */


int CallIDAsStr(unsigned char *callid, unsigned char *buf, int buflen, int *length)
{
	DxStr CallIDStr;
	int retvalue;

	if(!callid || !buf || !length)
		return 0;
	
	CallIDStr=dxStrNew();
	dxStrCat(CallIDStr, "Call-ID:");
	dxStrCat(CallIDStr,(const char*)callid);
	dxStrCat(CallIDStr, "\r\n");
	*length=dxStrLen(CallIDStr);

	if ((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(CallIDStr));
		retvalue= 1;
	}
	dxStrFree(CallIDStr);
	return retvalue;
} /* CallIDAsStr */

int CallIDFree(unsigned char *callid)
{
	if(callid != NULL){
		free(callid);
		callid=NULL;
	}
	return 1;
} /* CallIDFree */

int CallInfoParse(MsgHdr ph, void **ret)
{
	SipInfo	*_callinfo=NULL,*tmpinfo;
	SipParam	*_parameter;
	MsgHdrElm	nowElm;
	unsigned char	*buf,*freeptr=NULL,*tmp,*parms;
	int		count;	
	
	*ret = NULL;
	/* chech input data */
	if(!ph->first)
		return 0;
	nowElm = ph->first;
	if(nowElm != NULL){
		_callinfo = (SipInfo*)calloc(1,sizeof(*_callinfo));
		if(_callinfo != NULL){
			_callinfo->absoluteURI=NULL;
			_callinfo->numParams=0;
			_callinfo->SipParamList=NULL;
			_callinfo->next=NULL;
		}else{
			TCRPrint(1, "*** [sipStack] CallInfo memory alloc error! \n");
			return 0;
		}
	}else
		return 0;
	tmpinfo=_callinfo;
	_parameter=NULL;

	while(1){
	/* parse absoluteURI */
		buf = (unsigned char*)strDup((const char*)trimWS((char*)nowElm->content));
		freeptr=buf;
		tmp=(unsigned char*)strChr(buf,'<');
		if(tmp != NULL){
			buf=tmp+1;
			tmp=(unsigned char*)strChr(buf,'>');
			if(tmp != NULL){
				*tmp='\0';
				_callinfo->absoluteURI=strDup((const char*)buf);
				buf=tmp+1;
				buf=(unsigned char*)trimWS((char*)buf);
				
			}
		}else {
			_callinfo->absoluteURI=NULL;
			if(freeptr) free(freeptr);
			TCRPrint(1, "*** [sipStack] CallInfo Parse error! \n");
			break;
		}
	/* parse generic-param  */
		
		parms=(unsigned char*)strChr(buf,';');
		count=0;
		if(parms != NULL){
			*parms='\0';
			parms++;
			
			while(1){
				count++;
				tmp = (unsigned char*)strChr(parms, ';');
				if(_parameter==NULL)	
						_parameter =(SipParam*) calloc(1,sizeof(*_parameter));
				if(count==1)
						_callinfo->SipParamList=_parameter;
				if (tmp == NULL){ /* only one parameter*/
					_parameter->next = NULL;
					AnalyzeParameter(parms,_parameter);
					break;
				}else{
					*tmp = '\0';
			
					AnalyzeParameter(parms,_parameter);
					parms=tmp+1;
					_parameter->next=(SipParam*) calloc(1,sizeof(*_parameter));
					_parameter=_parameter->next;
					_parameter->name=NULL;
					_parameter->value=NULL;
					_parameter->next=NULL;
				}
				
				
			}
			
		}
		_callinfo->numParams=count;
		if(freeptr!=NULL)
			free(freeptr);
		if (nowElm->next != NULL){ 
			nowElm = nowElm->next;
			/* new next callinfo and initialize new parameter of callinfo*/
			_callinfo->next=(SipInfo*)calloc(1,sizeof(*_callinfo));
			_callinfo=_callinfo->next;
			_callinfo->numParams=0;
			_callinfo->absoluteURI=NULL;
			_callinfo->SipParamList=NULL;
			_callinfo->next=NULL;
			_parameter=NULL;
		}
		else
			break;
	}/*end while */

	*ret=(void *)tmpinfo;
	return 1;
} /* CallInfoParse tyhuang*/

int CallInfoAsStr(SipInfo *_callinfo, unsigned char *buf, int buflen, int *length)
{
	DxStr CallInfoStr;
	int retvalue;
	SipParam* _parameter;

	CallInfoStr=dxStrNew();
	while(1){
		if(_callinfo==NULL) break;
		dxStrCat(CallInfoStr, "Call-Info: <");
		dxStrCat(CallInfoStr,(const char*)_callinfo->absoluteURI);
		dxStrCat(CallInfoStr,">");
		_parameter=_callinfo->SipParamList;
		while (1) {
			if (_parameter != NULL){
				dxStrCat(CallInfoStr,";");
				dxStrCat(CallInfoStr, _parameter->name);
				if (_parameter->value != NULL){
					dxStrCat(CallInfoStr, "=");/*for equal */
					dxStrCat(CallInfoStr, _parameter->value);
				}
				_parameter = _parameter->next;
			} else
				break;
		}/*end while loop*/
		
		if(_callinfo->next==NULL) break;
		else{
			_callinfo=_callinfo->next;
			dxStrCat(CallInfoStr,",");
		}
	}/*end while loop */

	dxStrCat(CallInfoStr, "\r\n");
	*length=dxStrLen(CallInfoStr);

	if ((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(CallInfoStr));
		retvalue= 1;
	}
	dxStrFree(CallInfoStr);
	return retvalue;
}/*CallInfoAsStr tyhuang*/

int CallInfoFree(SipInfo *_callinfo)
{
	SipInfo *tmpinfo;

	while(_callinfo!=NULL){
		if(_callinfo->absoluteURI!=NULL){ 
			free(_callinfo->absoluteURI);
			_callinfo->absoluteURI=NULL;
		}
		if(_callinfo->SipParamList!=NULL){
			sipParameterFree(_callinfo->SipParamList);
			_callinfo->SipParamList=NULL;
		}
			
		tmpinfo=_callinfo->next;
		free(_callinfo);
		_callinfo=tmpinfo;
	}
	return 1;
}/* CallInfoFree tyhuang*/

int ClassParse(MsgHdr ph, void **ret)
{
	SipClass	*_class=NULL;
	MsgHdrElm	nowElm;
	SipStr		*celm=NULL;
	
	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;

	nowElm=ph->first;
	/* parse language-tag=  primary-tag *( "-" subtag ) */
	if(nowElm != NULL){
		_class=(SipClass*)calloc(1,sizeof(*_class));
		if(_class != NULL){
			_class->classList=NULL;
			_class->numClass=ph->size;
		}else{
			TCRPrint(1, "*** [sipStack] ClassParse memory alloc error! \n");
			return 0;
		}
		celm=(SipStr*)calloc(1,sizeof(*celm));
		celm->str=strDup((const char*)trimWS((char*)nowElm->content));
		celm->next=NULL;
		_class->classList=celm;
		nowElm=nowElm->next;
	}
	while(nowElm != NULL){
		celm->next=(SipStr*)calloc(1,sizeof(*celm));
		celm=celm->next;
		celm->str=strDup((const char*)trimWS((char*)nowElm->content));
		celm->next=NULL;
		nowElm=nowElm->next;
	}

	*ret=(void*)_class;
	return 1;
} /* ClassParse */

int ClassAsStr(SipClass *_class,unsigned char* buf, int buflen, int* length)
{	
	DxStr ClassStr;
	int i,retvalue;
	SipStr *celm;

	ClassStr=dxStrNew();
	dxStrCat(ClassStr, "Class:");
	celm=_class->classList;
	for(i=0;i<_class->numClass;i++){
		if(i>0)  dxStrCat(ClassStr,",");
		dxStrCat(ClassStr,celm->str);
		celm=celm->next;
	}
	dxStrCat(ClassStr, "\r\n");
	*length=dxStrLen(ClassStr);
	if((*length) >= buflen)
		retvalue= 0;
	else {
		strcpy(buf,(const char*)dxStrAsCStr(ClassStr));
		retvalue= 1;
	}
	dxStrFree(ClassStr);
	return retvalue;
}/* ClassAsStr */

int ClassFree(SipClass *_class)
{
	if(_class!= NULL){
		if(_class->classList != NULL){
			sipStrFree(_class->classList);
			_class->classList=NULL;
		}
		free(_class);
		_class=NULL;
	}
	return 1;
} /* ClassFree */

CCLAPI SipContact* sipContactNewFromText(char* text)
{
	SipContact	*_contact;
	MsgHdr hdr;
	char *tmpbuf;

	if(!text)
		return NULL;
	tmpbuf=strDup((const char*)text);

	hdr=msgHdrNew();
	rfc822_parseline((unsigned char*)tmpbuf,strlen((const char*)tmpbuf),hdr);

	if( ContactParse(hdr,(void**)&_contact)==0 ) {
		free(tmpbuf);
		return NULL;
	}

	free(tmpbuf);
	return _contact;
}

int ContactParse(MsgHdr ph, void **ret)
{
	SipContact	*_contact=NULL;
	SipContactElm	*_contactelm;
	SipAddr		*_addr;
	SipParam*	_parameter, *_prePara;
	unsigned char	*buf,*freeptr, *parms, *comm, *tmp;
	MsgHdrElm	nowElm;
	int		length, i = 0;
	/*msgHdr		tmpHr = ph;*/
	
	*ret = NULL;
	/* chech input data */
	if(!ph->first)
		return 0;
	
	nowElm = ph->first;
	_contact =(SipContact*) calloc(1,sizeof(*_contact));
	if(_contact != NULL){
		_contact->numContacts = 1;
		_contact->sipContactList=NULL;
	}else{
		TCRPrint(1, "*** [sipStack] Authorization memory alloc error! \n");
		return 0;
	}
	_contactelm = (SipContactElm*)calloc(1,sizeof(*_contactelm));
	_contact->sipContactList = _contactelm;
	_contactelm->expireDate = NULL;
	_contactelm->expireSecond = -1;
	_contactelm->comment = NULL;
	_contactelm->numaddress=0;
	_contactelm->address = NULL;
	_contactelm->numext_attrs=0;
	_contactelm->ext_attrs = NULL;
	_contactelm->next = NULL;
	

	while(1) {
		/* check nowElm and nowElm->content */
		if(!nowElm || !nowElm->content) break;
		/* if not : ,means not an URL */
		if(!strchr(nowElm->content,':') && !strchr(nowElm->content,'*')){
			nowElm=nowElm->next;
			continue;
		}

		buf = (unsigned char*)strDup((const char*)trimWS((char*)nowElm->content));
		freeptr=buf;

		tmp = (unsigned char*)strstr(buf, "expires");
		if (tmp != NULL){
			tmp = (unsigned char*)strChr(tmp, '=');
			if (*tmp == '='){
				CollectDateData(&buf, tmp+1, &nowElm);
				freeptr=buf;
			}
		}

		while( buf[i]!='\0' ) {
			if( buf[i]=='<' ) {
				while( buf[i]!='>' && buf[i]!='\0' )	i++;
				continue;
			}
			else if( buf[i]==';')
				break;
			else
				i++;
		}/* end while  buf[i] != \0 */
		parms = buf + i;	
		comm = (unsigned char*)strChr(buf, '(');
    
		/* parse comment */
		if (comm != NULL){
			CollectCommentData(&buf, comm+1, &nowElm);
			freeptr=buf;
			comm = (unsigned char*)strChr(buf, '(');
			if (comm) {
				*comm = '\0';
				comm++;
			}
			
			if(ExtractComment(comm, &length)) { /* extract comment */
				if (*(comm+length) == '\0')
					_contactelm->comment=strDup((const char*)comm);
				else{	
					if(buf) free(buf);
					TCRPrint(1, "*** [sipStack] Contact Parse error in comment! \n");
					return 0;
				}
			}else{ 
				if(buf) free(buf);
				TCRPrint(1, "*** [sipStack] Contact Parse error in comment! \n");
				return 0;
			}
		}else
			_contactelm->comment = NULL;

		/* parse parameter */
		/* if (parms != NULL) */
		if( *parms!='\0' ){
			int countPara=0;
			*parms = '\0';
			parms++;
			_prePara = NULL;
			while(1){
				tmp = (unsigned char*)strChr(parms, ';');
				if (tmp == NULL){ /* only one parameter*/
					if (strstr(parms,"expires") == NULL){
						_parameter = (SipParam*)calloc(1,sizeof(*_parameter));
						_parameter->name = NULL;
						_parameter->value = NULL;
						_parameter->next = NULL;
						if(_prePara == NULL)
							_contactelm->ext_attrs = _prePara = _parameter;
						else{
							_prePara->next=_parameter;
							_prePara=_parameter;
						}

						if(!_parameter) {
							_parameter =(SipParam*) calloc(1,sizeof(*_parameter));
							_parameter->next = NULL;
							if(_prePara) {
								_prePara->next = _parameter;
								_prePara = _parameter;
							}else
								_prePara = _parameter;
						}
						countPara++;
						AnalyzeParameter(parms, _parameter);
						_parameter = _parameter->next;
					} /*end if(strstr(parms,"expires") == NULL) */
					else /*no expires parameters*/
						AnalyzeTimeParameter(parms, &_contactelm->expireDate, &_contactelm->expireSecond);
					break;
				} /*end if tmp==NULL) */
				else{ /* more than one parameters */
					*tmp = '\0';

					if (strstr(parms, "expires") == NULL) {
						_parameter =(SipParam*) calloc(1,sizeof(*_parameter));
						_parameter->next = NULL;
						if(_prePara) {
							_prePara->next = _parameter;
							_prePara = _parameter;
						}else
							_prePara = _parameter;
						AnalyzeParameter(parms, _parameter);
						_parameter = _parameter->next;
						if((countPara==0)&&(_prePara!=NULL))/*Acer Modify 4/22 SipIt */
							_contactelm->ext_attrs=_prePara;
						countPara++;
					}else
						AnalyzeTimeParameter(parms, &_contactelm->expireDate, &_contactelm->expireSecond);
					parms = tmp + 1;
				}
			}/*end while */
			_contactelm->numext_attrs=countPara;
			
		}/*end if *parms!='\0'*/

		/* parse address */
		_addr = (SipAddr*)calloc(1,sizeof(*_addr));
		if (!AddrParse(buf, _addr)){
			if(buf) free(buf);
			if(_addr) sipAddrFree(_addr);
			TCRPrint(1, "*** [sipStack] Contact Parse error in address part! \n");
			return 0;
		}
		_contactelm->address = _addr;
		_contactelm->numaddress++;

		/* if not : ,means not an URL */
		while(nowElm->next){
			if(nowElm->next->content && !strchr(nowElm->next->content,':'))
				break;
			else
				nowElm=nowElm->next;
		}

		if (nowElm->next != NULL) 
			nowElm = nowElm->next;
		else{
			if(freeptr!=NULL)
				free(freeptr);
			break;
		}

		_contact->numContacts++;
		_contactelm->next = (SipContactElm*)calloc(1,sizeof(*_contactelm));
		_contactelm = _contactelm->next;
		_contactelm->expireDate = NULL;
		_contactelm->expireSecond = -1;
		_contactelm->comment = NULL;
		_contactelm->numaddress=0;
		_contactelm->address = NULL;
		_contactelm->numext_attrs=0;
		_contactelm->ext_attrs = NULL;
		_contactelm->next = NULL;
		free(freeptr);
	}/*end while(1) */
	*ret = (void *)_contact;
	return 1;
} /* ContactParse */

int sipContactAsStr(SipContact *_contact, unsigned char *buf, int buflen, int *length)
{
	int		tmp,i,retvalue;
	SipContactElm	*_contactelm;
	SipParam	*_parameter;
	unsigned char	tmpbuf[12];
	char		*tmpchar;
	DxStr		ContactStr;
	char		buffer[512]={'\0'};
	char		Datebuf[64]={'\0'};
  
	ContactStr=dxStrNew();
	dxStrCat(ContactStr,"Contact:");
	_contactelm = _contact->sipContactList;
	if(_contactelm==NULL)
		return RC_ERROR;
	for(i=0; i<_contact->numContacts; i++) {
		if(_contactelm->address){
			AddrAsStr(_contactelm->address,(unsigned char*) buffer, 512, &tmp);
			dxStrCat(ContactStr,buffer);
		}
		if (_contactelm->expireSecond != -1) {
			dxStrCat(ContactStr,";expires=");
			memset(tmpbuf, ' ', 12);
			tmpchar=i2a(_contactelm->expireSecond,(char*)tmpbuf,12);
			dxStrCat(ContactStr,(char*)tmpbuf);
		}
		else if (_contactelm->expireDate != NULL) {
			dxStrCat(ContactStr,";expires=\"");
			RFCDateAsStr(_contactelm->expireDate,(unsigned char*)Datebuf, 64, &tmp);
			dxStrCat(ContactStr,Datebuf);
			dxStrCat(ContactStr,"\"");
		}/*end if */

		_parameter = _contactelm->ext_attrs;
		while (1) {
			if (_parameter != NULL){
				dxStrCat(ContactStr,";");
				dxStrCat(ContactStr, _parameter->name);
				if (_parameter->value != NULL){
					dxStrCat(ContactStr, "=");/*for equal */
					dxStrCat(ContactStr, _parameter->value);
				}
				_parameter = _parameter->next;
			} else
				break;
		}/*end while loop*/

		if (_contactelm->comment != NULL)  {
			dxStrCat(ContactStr, "(");
			dxStrCat(ContactStr, _contactelm->comment);
			dxStrCat(ContactStr, ")");
		}

		if (_contactelm->next != NULL) {
			dxStrCat(ContactStr,"\r\n");
			dxStrCat(ContactStr,"Contact:");
		}
		_contactelm = _contactelm->next;
	}
	dxStrCat(ContactStr,"\r\n");
	*length=dxStrLen(ContactStr);

	if((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(ContactStr));
		retvalue= 1;
	}
	dxStrFree(ContactStr);
	return retvalue;

} /* ContactAsStr */

int sipContactFree(SipContact *_contact)
{
	if(_contact != NULL){
		if(_contact->sipContactList){
			sipContactElmFree(_contact->sipContactList);
			_contact->sipContactList=NULL;
		}
		free(_contact);
		_contact=NULL;
	}
	return 1;
}

int ContentDispositionParse(MsgHdr ph, void **ret)
{
	SipContentDisposition	*_contentdisposition=NULL,*tmpcontentdisposition;
	SipParam	*_parameter;
	MsgHdrElm	nowElm;
	unsigned char	*buf,*freeptr,*tmp,*parms;
	int		count=0;	
	
	*ret = NULL;
	/* chech input data */
	if(!ph->first)
		return 0;
	_contentdisposition = (SipContentDisposition*)calloc(1,sizeof(*_contentdisposition));
	if(_contentdisposition != NULL){
		_contentdisposition->disptype = NULL;
		_contentdisposition->numParam = 0;
		_contentdisposition->paramList = NULL;
	}else{
		TCRPrint(1, "*** [sipStack] contentdisposition memory alloc error! \n");
		return 0;
	}
	tmpcontentdisposition=_contentdisposition;
	_parameter=NULL;
	nowElm = ph->first;

	/* parse absoluteURI */
	if(nowElm->content != NULL){
		buf = (unsigned char*)strDup((const char*)trimWS((char*)nowElm->content));
		freeptr=buf;
	}
	tmp=(unsigned char*)strChr(buf,';');
	if(tmp != NULL){ /* there are parameters */
		*tmp='\0';
		tmpcontentdisposition->disptype=strDup((const char*)buf);
		buf=tmp+1;
		parms=(unsigned char*)trimWS((char*)buf);
		/* parse parameters  */
				
		while(1){
			count++;
			tmp = (unsigned char*)strChr(parms, ';');
			if(_parameter==NULL)	
				_parameter =(SipParam*) calloc(1,sizeof(*_parameter));
			if(count==1)
				tmpcontentdisposition->paramList=_parameter;
			if (tmp == NULL){ /* only one or last parameter */
				_parameter->next = NULL;
				AnalyzeParameter(parms,_parameter);
				break;
			}else{
				*tmp = '\0';
			
				AnalyzeParameter(parms,_parameter);
				parms=tmp+1;
				_parameter->next=(SipParam*) calloc(1,sizeof(*_parameter));
				_parameter=_parameter->next;
				_parameter->name=NULL;
				_parameter->value=NULL;
				_parameter->next=NULL;
			}
				
		}/*end of while */
				
	}else {

		tmpcontentdisposition->disptype=strDup((const char*)buf);
	}
	free(freeptr);
	tmpcontentdisposition->numParam = count;
	*ret=(void *)_contentdisposition;
	return 1;
} /* ContentDispParse tyhuang*/

int ContentDispositionAsStr(SipContentDisposition *_contentdisposition, unsigned char *buf, int buflen, int *length)
{
	DxStr ContentDispositionStr;
	int retvalue;
	SipParam* _parameter;

	if(_contentdisposition==NULL) return 0;

	ContentDispositionStr=dxStrNew();
	dxStrCat(ContentDispositionStr, "Content-Disposition:");
	dxStrCat(ContentDispositionStr,(const char*)_contentdisposition->disptype);
	_parameter=_contentdisposition->paramList;
	while (1) {
		if (_parameter != NULL){
			dxStrCat(ContentDispositionStr,";");
			dxStrCat(ContentDispositionStr, _parameter->name);
			if (_parameter->value != NULL){
				dxStrCat(ContentDispositionStr, "=");/*for equal */
				dxStrCat(ContentDispositionStr, _parameter->value);
			}
				_parameter = _parameter->next;
			} else
				break;
	}/*end while loop*/
		

	dxStrCat(ContentDispositionStr, "\r\n");
	*length=dxStrLen(ContentDispositionStr);

	if ((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(ContentDispositionStr));
		retvalue= 1;
	}
	dxStrFree(ContentDispositionStr);
	return retvalue;
}/*ContentDispAsStr tyhuang*/

int ContentDispositionFree(SipContentDisposition *_contentdisp)
{
	if(_contentdisp != NULL){
		if(_contentdisp->disptype != NULL){ 
			free(_contentdisp->disptype);
			_contentdisp->disptype=NULL;
		}
		if(_contentdisp->paramList!=NULL){
			sipParameterFree(_contentdisp->paramList);
			_contentdisp->paramList=NULL;
		}
		free(_contentdisp);
		_contentdisp=NULL;
	}
	return 1;
}/* ContentDispositionFree tyhuang*/

int ContentEncodingParse(MsgHdr ph, void **ret)
{
	char		*strbuf,*freeptr;
	SipContEncoding	*_contentencoding=NULL;
	SipCoding	*_coding=NULL;
	MsgHdrElm	nowElm;
  
	*ret = NULL;
	/* chech input data */
	if(!ph->first)
		return 0;
	_contentencoding = (SipContEncoding*)calloc(1,sizeof(*_contentencoding));
	if(_contentencoding != NULL){
		_contentencoding->sipCodingList=NULL;
		_contentencoding->numCodings = ph->size;
	}else{
		TCRPrint(1, "*** [sipStack] contentencoding memory alloc error! \n");
		return 0;
	}
	nowElm = ph->first;
	_coding =(SipCoding*) calloc(1,sizeof(*_coding));
	if(_coding){
		_coding->next = NULL;
		_coding->coding = NULL;
		_coding->qvalue = NULL;
	}else{
		TCRPrint(1, "*** [sipStack] contentencoding memory alloc error! \n");
		return 0;
	}
	_contentencoding->sipCodingList = _coding;

	while (1) {
		char *tmp;

		strbuf = strDup((const char*)trimWS((char*)nowElm->content));
		freeptr=strbuf;
		tmp=strchr((const char*)strbuf,';');
		if(tmp!=NULL) {
			*tmp='\0';
			_coding->coding=strDup((const char*)strbuf);
			strbuf=tmp+1;
			/*strbuf=strDup((const char*)trimWS((char*)strbuf));*/
			strbuf=(char*)trimWS((char*)strbuf);
			tmp=strchr((const char*)strbuf,'=');
			if(tmp!=NULL)
				_coding->qvalue=strDup((const char*)(tmp+1));
			else
				_coding->qvalue=NULL;
		}else {/* no parameter*/
			_coding->coding=strDup((const char*)strbuf);
			_coding->qvalue=NULL;
		}
		free(freeptr);
		/*_coding->coding = strbuf; */
    
		if (nowElm == ph->last)
			break;
		else{
			nowElm = nowElm->next;
			_coding->next = (SipCoding*)calloc(1,sizeof(*_coding));
			_coding = _coding->next;
			_coding->next = NULL;
			_coding->coding = NULL;
			_coding->qvalue = NULL;
		}
	}/*end while */
	*ret = (void*)_contentencoding;
	return 1;
} /* ContentEncodingParse */

int ContentEncodingAsStr(SipContEncoding *_contentencoding, unsigned char *buf, int buflen, int *length)
{
	int i,retvalue;
	SipCoding *_coding;
	DxStr ContentEnCodStr;

	ContentEnCodStr=dxStrNew();
	dxStrCat(ContentEnCodStr, "Content-Encoding:");  
	_coding = _contentencoding->sipCodingList;
	for(i=0;i<_contentencoding->numCodings;i++){
		if(_coding != NULL){
			dxStrCat(ContentEnCodStr,_coding->coding);
			if(_coding->qvalue!=NULL){
				dxStrCat(ContentEnCodStr,";q=");
				dxStrCat(ContentEnCodStr,_coding->qvalue);
			}
			_coding = _coding->next;
		}else
			break;
		if (_coding != NULL)
			dxStrCat(ContentEnCodStr, ",");
	}/*end for loop*/
	dxStrCat(ContentEnCodStr, "\r\n");
	*length=dxStrLen(ContentEnCodStr);
	if((*length) >= buflen)
		retvalue= 0;
	else {
		strcpy(buf,(const char*)dxStrAsCStr(ContentEnCodStr));
		retvalue= 1;
	}
	dxStrFree(ContentEnCodStr);
	return retvalue;
} /* ContentEncodingAsStr */

int ContentEncodingFree(SipContEncoding *_contentencoding)
{
	if(_contentencoding != NULL){
		if(_contentencoding->sipCodingList){
			sipCodingFree(_contentencoding->sipCodingList);
			_contentencoding->sipCodingList=NULL;
		}
		free(_contentencoding);
		_contentencoding=NULL;
	}
	return 1;
} /* ContentEncodingFree */

int ContentLanguageParse(MsgHdr ph, void **ret)
{
	SipContentLanguage	*contentlang=NULL;
	MsgHdrElm		nowElm;
	SipStr			*lang;
	
	*ret = NULL;
	/* chech input data */
	if(!ph->first)
		return 0;
	contentlang=(SipContentLanguage*)calloc(1,sizeof(*contentlang));
	if(contentlang!=NULL){
		contentlang->langList=NULL;
		contentlang->numLang=ph->size;
	}else{
		TCRPrint(1, "*** [sipStack] contentlanguage memory alloc error! \n");
		return 0;
	}
	nowElm=ph->first;
	lang=NULL;
	/* parse language-tag=  primary-tag *( "-" subtag ) */
	if(nowElm != NULL){
		lang=(SipStr*)calloc(1,sizeof(*lang));
		lang->str=strDup((const char*)trimWS((char*)nowElm->content));
		lang->next=NULL;
		contentlang->langList=lang;
		nowElm=nowElm->next;
	}
	while(nowElm != NULL){
		lang->next=(SipStr*)calloc(1,sizeof(*lang));
		lang=lang->next;
		lang->str=strDup((const char*)trimWS((char*)nowElm->content));
		lang->next=NULL;
		nowElm=nowElm->next;
	}

	*ret=(void*)contentlang;
	return 1;
} /* ContentLanguageParse */

int ContentLanguageAsStr(SipContentLanguage *contentlang,unsigned char* buf, int buflen, int* length)
{	
	DxStr ContentLangStr;
	int i,retvalue;
	SipStr *lang;
	
	ContentLangStr=dxStrNew();
	dxStrCat(ContentLangStr, "Content-Language:");
	lang=contentlang->langList;
	for(i=0;i<contentlang->numLang;i++){
		if(i>0)  dxStrCat(ContentLangStr,",");
		dxStrCat(ContentLangStr,lang->str);
		lang=lang->next;
	}
	dxStrCat(ContentLangStr, "\r\n");
	if((*length) >= buflen)
		retvalue= 0;
	else {
		strcpy(buf,(const char*)dxStrAsCStr(ContentLangStr));
		retvalue= 1;
	}
	dxStrFree(ContentLangStr);
	return retvalue;
}/* ContentLanguageAsStr */

int ContentLanguageFree(SipContentLanguage *_contentlang)
{
	if(_contentlang != NULL){
		if(_contentlang->langList != NULL){
			sipStrFree(_contentlang->langList);
			_contentlang->langList=NULL;
		}
		free(_contentlang);
		_contentlang=NULL;
	}
	return 1;
} /* ContentEncodingFree */

int ContentLengthParse(MsgHdr ph, void **ret)
{
	int contentlength;
	int *ct;
	
	/* check input data */
	if(!ph->first || !ph->first->content)
		return 0;
	ct=(int*)calloc(1,sizeof(int));
	contentlength = atoi(ph->first->content);
	/* added by tyhuang  */
	if(contentlength < 0) contentlength=0;
	if(contentlength > SIP_MAX_CONTENTLENGTH) contentlength=SIP_MAX_CONTENTLENGTH;
	/**ret = &contentlength;*/
	*ct=contentlength;
	*ret=(void*)ct;
	return 1;
} /* ContentLengthParse */

int ContentLengthAsStr(int value, unsigned char* buf, int buflen, int* length)
{
	char tmp[15]={'\0'};
	char *tmpstr;
	DxStr ContentLengStr;
	int retvalue;

	ContentLengStr=dxStrNew();
	memset(tmp, ' ', 15);
	tmpstr=i2a(value,(char*)tmp,12);
	dxStrCat(ContentLengStr,"Content-Length:");
	dxStrCat(ContentLengStr,tmp);
	dxStrCat(ContentLengStr,"\r\n");
	*length=dxStrLen(ContentLengStr);
	
	if((*length) >= buflen)
		retvalue= 0;
	else {
		strcpy(buf,(const char*)dxStrAsCStr(ContentLengStr));
		retvalue= 1;
	}
	dxStrFree(ContentLengStr);
	return retvalue;
} /* ContentLengthAsStr */

/* Content-Type = ("Content-Type" | "c" ) ":" media-type */
int ContentTypeParse(MsgHdr ph, void **ret)
{
	char          *strbuf,*freeptr, *ptmp;
	SipContType  *_mediatype;
	SipParam*  _parameter;

	*ret = NULL;
	if (ph->first != ph->last)
		return 0;
	else{
		_mediatype = (SipContType*)calloc(1,sizeof(*_mediatype));
		_mediatype->next = NULL;
		_mediatype->numParam = 0;
		_mediatype->sipParamList = NULL;

		strbuf = strDup((const char*)trimWS((char*)ph->first->content));
		freeptr=strbuf;
		ptmp = strchr((const char*)strbuf,'/');
		if (ptmp == NULL){
			if(_mediatype) free(_mediatype);
			if(freeptr) free(freeptr);
			TCRPrint(1, "*** [sipStack] ContenType Parse error ! \n");
			return 0;
		}else{
			*ptmp = '\0';
			_mediatype->type = strDup((const char*)strbuf);
			strbuf = ptmp;
			strbuf++;
			strbuf =(char*) trimWS((char*)strbuf);
		}

		ptmp = strchr((const char*)strbuf,';');
		if (ptmp == NULL)
			_mediatype->subtype = strDup((const char*)trimWS((char*)strbuf));
		else{
			*ptmp = '\0';
			_mediatype->subtype = strDup((const char*)trimWS((char*)strbuf));
			strbuf = ptmp;
			strbuf++;
			strbuf =(char*) trimWS(strbuf);

			_parameter = (SipParam*)calloc(1,sizeof(*_parameter));
			_mediatype->sipParamList = _parameter;
			while(1){
				ptmp =strchr((const char*)strbuf,';');
				if (ptmp == NULL){
					AnalyzeParameter((unsigned char*)strbuf, _parameter);
					_mediatype->numParam++;
					break;
				}else{
					*ptmp = '\0';
					AnalyzeParameter((unsigned char*)strbuf, _parameter);
					_mediatype->numParam++;
					strbuf =(char*) trimWS(ptmp+1);
					_parameter->next = (SipParam*)calloc(1,sizeof(*_parameter));
					_parameter = _parameter->next;
					_parameter->next = NULL;
				}       
			}/*end while loop*/
		}/*end else */
		free(freeptr);
	}
	*ret = (void*)_mediatype;
	return 1;
} /* ContentTypeParse */

int ContentTypeAsStr(SipContType *_contenttype, unsigned char* buf, int buflen, int* length)
{
	int i,retvalue;
	SipParam* _parameter;
	DxStr	ContentTypeStr;

	ContentTypeStr=dxStrNew();
	dxStrCat(ContentTypeStr, "Content-Type:");
	if(_contenttype){
		if(_contenttype->type)
			dxStrCat(ContentTypeStr,  _contenttype->type);
		if(_contenttype->subtype){
			dxStrCat(ContentTypeStr, "/");
			dxStrCat(ContentTypeStr, _contenttype->subtype);
		}
		_parameter = _contenttype->sipParamList;
		for(i=0;i<_contenttype->numParam;i++){
			dxStrCat(ContentTypeStr, ";");
			dxStrCat(ContentTypeStr, _parameter->name);
			dxStrCat(ContentTypeStr, "=");
			dxStrCat(ContentTypeStr, _parameter->value);
			_parameter = _parameter->next;
		}
	}
	dxStrCat(ContentTypeStr, "\r\n");
	*length=dxStrLen(ContentTypeStr);

	if((*length) >= buflen)
		retvalue= 0;
	else {
		strcpy(buf,(const char*)dxStrAsCStr(ContentTypeStr));
		retvalue= 1;
	}
	dxStrFree(ContentTypeStr);
	return retvalue;
} /* ContentTypeAsStr */

int ContentTypeFree(SipContType *_contenttype)
{
	/* Assume there is only one media type obj */
	/* and this media type obj had one parameter list */
	if(_contenttype != NULL){
		if(_contenttype->type){
			free(_contenttype->type);/*free data */
			_contenttype->type=NULL;
		}
		if(_contenttype->subtype){
			free(_contenttype->subtype);
			_contenttype->subtype=NULL;
		}
		if(_contenttype->sipParamList){
			sipParameterFree(_contenttype->sipParamList);
			_contenttype->sipParamList=NULL;
		}
		free(_contenttype);
		_contenttype=NULL;
	}
	return 1;
} /* ContentTypeFree */

int CSeqParse(MsgHdr ph, void **ret)
{
	SipCSeq *_cseq;
	unsigned char *buf, *tmp;

	*ret = NULL;
	/* chech input data */
	if(!ph->first || !ph->first->content)
		return 0;
	_cseq = (SipCSeq*)calloc(1,sizeof (*_cseq));
	buf = (unsigned char*)strDup((const char*)trimWS(ph->first->content));
	tmp = (unsigned char*)strChr(buf, ' ');
	if (tmp){ 
		*tmp = '\0';
		_cseq->seq = (unsigned int)atof((const char*)buf);
		tmp++;
		tmp = (unsigned char*)trimWS((char*)tmp);
		_cseq->Method = sipMethodNameToType((char*)tmp);
	}
	
	free(buf);

	*ret = (void *)_cseq;
	return 1;
} /* CSeqParse */

int CSeqAsStr(SipCSeq *_cseq, unsigned char *buf, int buflen, int *length)
{
	char tmp[16]={'\0'};
	char *tmpstr;
	DxStr CSeqStr;
	int retvalue;

	CSeqStr=dxStrNew();

	tmpstr=i2a(_cseq->seq,(char*)tmp,15);

	dxStrCat(CSeqStr, "CSeq:");
	dxStrCat(CSeqStr, tmp);
	dxStrCat(CSeqStr, " ");
	dxStrCat(CSeqStr, (char*)sipMethodName[_cseq->Method]);
	dxStrCat(CSeqStr, "\r\n");
	*length=dxStrLen(CSeqStr);

	if((*length) >= buflen)
		retvalue =0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(CSeqStr));
		retvalue= 1;
	}
	dxStrFree(CSeqStr);
	return retvalue;
} /* CSeqAsStr */

int CSeqFree(SipCSeq *_cseq)
{
	if(_cseq != NULL){
		free(_cseq);
		_cseq=NULL;
	}
	return 1;
} /* CSeqFree */

int RFCDateParse(unsigned char *Datestr, SipRfc1123Date *date)
{
	unsigned char *tmp, *buf, *iter, *p;
	int wkday;
	int month;
  
	buf = (unsigned char*)strDup((const char*)trimWS((char*)Datestr));

	tmp = (unsigned char*)strChr(buf,',');
	if (tmp) 
		*tmp = '\0';
	else{
		free(buf);
		return 0;
	}
	for(wkday=Mon;wkday<=UnknWeek;wkday=wkday+1) {
		if (!strICmp(wkdayName[wkday],(const char*)buf))
			break;
	}
	if (wkday == UnknWeek){
		free(buf);
		return 0;
	}else
		date->weekday = (SipwkdayType)wkday;

	/*tmp = tmp + 2; */
	tmp = tmp + 1;	
	trimWS((char*)tmp);

	strncpy(date->day,(char*)tmp, 2);
 
	/*tmp = tmp + 3; */
	/*tmp = (unsigned char*)strChr(tmp,' ') + 1;*/
	tmp = (unsigned char*)strChr(tmp,' ');
	if (tmp) {
		tmp=tmp+1;
	}else{
		free(buf);
		return 0;
	}

	date->day[2] = '\0';
  
	p = (unsigned char*)strChr(tmp,' ');
	if(p) {
		*p = 0;
	}

	for(month=Jan;month<=UnknMonth;month++){
		if (!strICmp(monthName[month],(const char*)tmp))
			break;
	}
	if (month == UnknMonth){
		free(buf);
		return 0;
	}else
		date->month = (SipMonthType)month;

	iter = tmp = tmp + 4;	
	iter = (unsigned char*)strChr(tmp,' ');
	if( iter )
		*iter = 0;
  
	if(strlen(tmp)<=4)
		strcpy(date->year, tmp);
	else
		strncpy(date->year,(char*)tmp, 4);

	/*tmp = tmp + 5; */
	tmp = ++iter;		
	iter = (unsigned char*)strChr(tmp,' ');
	if( iter )
		*iter = 0;
	else {
		date->year[0] = 0;
		date->time[0] = 0;
		return 0;
	}

	date->year[4] = '\0';
  
	if(strlen(tmp)<=8)
		strcpy(date->time, tmp);
	else
		strncpy(date->time,(char*)tmp, 8);

	/*tmp = tmp + 8; */
	tmp = ++iter;	
	date->time[8] = '\0';
  
	if( strcmp("GMT",tmp)==0 ) {
		free(buf);
		return 1;
	}else {
		free(buf);
		return 0;
	}
} /* RFCDateParse */

int RFCDateAsStr(SipRfc1123Date *_date, unsigned char *buf, int buflen, int *length)
{
	DxStr DateStr;
	int retvalue;

	if (!_date) return 0;

	DateStr=dxStrNew();
	dxStrCat(DateStr,(char*)wkdayName[_date->weekday]);
	dxStrCat(DateStr, ", ");
	dxStrCat(DateStr, _date->day);
	dxStrCat(DateStr, " ");
	dxStrCat(DateStr,(char*)monthName[_date->month]);
	dxStrCat(DateStr, " ");
	dxStrCat(DateStr, _date->year);
	dxStrCat(DateStr, " ");
	dxStrCat(DateStr, _date->time);
	dxStrCat(DateStr, " GMT");
	*length=dxStrLen(DateStr);

	if ((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(DateStr));
		retvalue= 1;
	}
	dxStrFree(DateStr);
	return retvalue;
} /* RFCDateAsStr */

int DateParse(MsgHdr ph, void **ret)
{
	unsigned char rfc1123_date[30];


	SipRfc1123Date *_date;
	SipDate *_sipdate;

	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;

	/* combine the data in msgHeader to become the rfc1123 date format */
	strncpy((char*)rfc1123_date, (const char*)trimWS(ph->first->content), 3);
	*(rfc1123_date + 3) = '\0';
	strcat(rfc1123_date, ",");

	/* 
	modified by Mac on May 22, 2003
	Please use str"n"cat instead of strcat as much as possible
	*/
	strncat((char*)rfc1123_date, (const char*)ph->last->content, sizeof(rfc1123_date)-strlen(rfc1123_date)-1 );

	_date =(SipRfc1123Date*) calloc(1,sizeof (*_date));
	RFCDateParse(rfc1123_date, _date);
	_sipdate = (SipDate*)calloc(1,sizeof (*_sipdate));
	_sipdate->date = _date;
	*ret = (void *)_sipdate;
	return 1;
} /* DateParse */

int DateAsStr(SipDate *_date, unsigned char *buf, int buflen, int *length)
{
	int itmp,retvalue;
	DxStr DateStr;
	char tmpbuf[512]={'\0'};

	DateStr=dxStrNew();
	dxStrCat(DateStr, "Date:");
	RFCDateAsStr(_date->date,(unsigned char*)tmpbuf,512,&itmp);
	dxStrCat(DateStr,tmpbuf);
	dxStrCat(DateStr, "\r\n");
	*length=dxStrLen(DateStr);

	if((*length) >= buflen)
		retvalue= 0;
	else {
		strcpy(buf,(const char*)dxStrAsCStr(DateStr));
		retvalue= 1;
	}
	dxStrFree(DateStr);
	return retvalue;
} /* DateAsStr */

int DateFree(SipDate *_date)
{
	if(_date != NULL){
		if(_date->date){
			free(_date->date);
			_date->date=NULL;
		}
		free(_date);
		_date=NULL;
	}
	return 1;
} /* DateFree */

int EncryptionParse(MsgHdr ph, void **ret)
{
	unsigned char *buf,*freeptr, *ptmp;
	SipEncrypt *_encryption;
	SipParam  *_parameter;
	MsgHdrElm nowElm;

	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;
	_encryption = (SipEncrypt*)calloc(1,sizeof(*_encryption));
	_encryption->numParams = 0;
	nowElm = ph->first;
	buf = (unsigned char*)strDup((const char*)trimWS(nowElm->content));
	freeptr=buf;
	ptmp = (unsigned char*)strChr(buf, ' ');
	if (ptmp == NULL)
		_encryption->scheme =strDup((const char*) buf);
	else {
		int countPara=0;

		*ptmp = '\0';
		_encryption->scheme = strDup((const char*)trimWS((char*)buf));
		buf = ptmp+1;
    
		_parameter = (SipParam*)calloc(1,sizeof(*_parameter));
		_encryption->sipParamList = _parameter;
		if (*buf != '\0') {
			countPara++;
			AnalyzeParameter(buf, _parameter);
			_encryption->numParams++;
			nowElm = nowElm->next;
			while (1) {
				if (nowElm != NULL){
					countPara++;
					/*buf = (unsigned char*)strDup((const char*)trimWS(nowElm->content));*/
					buf = (unsigned char*)trimWS(nowElm->content);
					_parameter->next = (SipParam*)calloc(1,sizeof(*_parameter));
					_parameter = _parameter->next;
					AnalyzeParameter(buf, _parameter);
					_encryption->numParams++;
					nowElm = nowElm->next;
				}else
					break;
			}/*end while loop*/
		}/*end if */
		_encryption->numParams=countPara;
	}
	free(freeptr);
	*ret = (void*)_encryption;
	return 1;
} /* EncryptionParse */

int EncryptionAsStr(SipEncrypt *_encryption, unsigned char *buf, int buflen, int *length)
{
	SipParam *_parameter;
	int i,retvalue;
	DxStr EncryptStr;

	EncryptStr=dxStrNew();
	dxStrCat(EncryptStr, "Encryption:");
	dxStrCat(EncryptStr, _encryption->scheme);
	dxStrCat(EncryptStr, " ");
	_parameter = _encryption->sipParamList;
	for(i=0; i<_encryption->numParams; i++) {
		dxStrCat(EncryptStr, _parameter->name);
		dxStrCat(EncryptStr, "=");
		dxStrCat(EncryptStr, _parameter->value);
		if (_parameter->next != NULL)
			dxStrCat(EncryptStr, ",");
		_parameter = _parameter->next;
	}
	dxStrCat(EncryptStr, "\r\n");
	*length=dxStrLen(EncryptStr);

	if((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(EncryptStr));
		retvalue= 1;
	}
	dxStrFree(EncryptStr);
	return retvalue;
} /* EncryptionAsStr */

int EncryptionFree(SipEncrypt *_encryption)
{
	if(_encryption != NULL){
		if(_encryption->scheme){
			free(_encryption->scheme);/*free scheme and first parameter data*/
			_encryption->scheme=NULL;
		}
		if(_encryption->sipParamList){
			sipParameterFree(_encryption->sipParamList);
			_encryption->sipParamList=NULL;
		}
		free(_encryption);
		_encryption=NULL;
	}
	return 1;
} /* EncryptionFree */

int EventParse(MsgHdr ph,void **ret)
{
	SipEvent	*_event=NULL;
	SipParam	*_param,*firstparam,*preparam;
	unsigned char	*buf;
	MsgHdrElm	nowElm;
	char		*a,*b,*c;
	SipStr		*str,*first,*prestr;
	int		strcount=0,paracount=0;
	BOOL		doing=TRUE,btmp=TRUE;

	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;

	nowElm=ph->first;
	if(nowElm != NULL){
		_event=(SipEvent*)calloc(1,sizeof(*_event));
		_event->numParam=0;
		_event->pack=NULL;
		_event->paramList=NULL;
		_event->tmplate=NULL;
		if(nowElm->content != NULL)
			buf=(unsigned char*)strDup((const char*)trimWS(nowElm->content));
		else{
			free(_event);
			TCRPrint(1, "*** [sipStack] Event Parse error! \n");
			return 0;
		}
		a=strChr(buf,';');
		if(a!=0)/*with parameter*/ 
			*a=0;
		b=strChr(buf,'.');
		if(b==NULL) /* without template */
			_event->pack=strDup((const char*)buf);
		else{ /* with template */
			*b=0;
			_event->pack=strDup((const char*)buf);
			while(btmp){
				str=(SipStr*)calloc(1,sizeof(*str));
				if((c=(char*)strchr((const char*)b+1,'.'))!=0)
					*c=0;
				else
					btmp=FALSE;

				str->str=strDup(b+1);
				str->next=NULL;
				if(strcount==0){
					first=str;
					_event->tmplate=str;
				}else{
					prestr->next=str;
				}
				prestr=str;
				b=c;
				strcount++;
			}
		}

		/*with parameter */
		if(a!=NULL){ 
			a++;
			while(doing==TRUE){
				_param=(SipParam*)calloc(1,sizeof(*_param));
				if((b=strchr((const char*)a,';'))!=0){
					*b=0;
					doing=TRUE;
					AnalyzeParameter((unsigned char*)a,_param);
					a=b+1;
				}else{
					AnalyzeParameter((unsigned char*)a,_param);
					doing=FALSE;
				}
				if(paracount==0){
					_event->paramList=_param;
					firstparam=_param;
				}else{
					preparam->next=_param;
				}
				preparam=_param;
				paracount++;
			}
			_event->numParam=paracount;
		}
		free(buf);
	}
	*ret=(void*)_event;
	return 1;
}/* Event Parse */

int EventAsStr(SipEvent *_event, unsigned char *buf, int buflen, int *length)
{
	SipStr *_str;
	SipParam *_param;
	DxStr EventStr;
	SipEvent *tmpevent;
	int retvalue;

	EventStr=dxStrNew();
	tmpevent=_event;
	if(tmpevent != NULL){ 
		dxStrCat(EventStr,"Event:");
		if(_event->pack != NULL)
			dxStrCat(EventStr,tmpevent->pack);
		_str=tmpevent->tmplate;
		while(_str!=NULL){
			dxStrCat(EventStr,".");
			dxStrCat(EventStr,_str->str);
			_str=_str->next;
		}
		_param=tmpevent->paramList;
		while(_param!=NULL){
			if(_param->name!=NULL){
				dxStrCat(EventStr,";");
				dxStrCat(EventStr,_param->name);
			}
			if(_param->value!=NULL){
				dxStrCat(EventStr,"=");
				dxStrCat(EventStr,_param->value);
			}
			_param=_param->next;
		}
		dxStrCat(EventStr,"\r\n");
	}
	*length=dxStrLen(EventStr);

	if((*length) >= buflen)
		retvalue=0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(EventStr));
		retvalue=1;
	}
	dxStrFree(EventStr);
	return retvalue;
}/* EventAsStr */

int EventFree(SipEvent *_event)
{
	SipEvent *tmpevent;

	tmpevent=_event;
	if(tmpevent!=NULL){
		if(tmpevent->pack != NULL){
			free(tmpevent->pack);
			tmpevent->pack=NULL;
		}
		if(tmpevent->tmplate != NULL){
			sipStrFree(tmpevent->tmplate);
			tmpevent->tmplate=NULL;
		}
		if(tmpevent->paramList != NULL){
			sipParameterFree(tmpevent->paramList);
			tmpevent->paramList=NULL;
		}
		free(tmpevent);
	}

	return 1;
}/* EventFree */

int ExpiresParse(MsgHdr ph, void **ret)
{
	SipExpires *_expires;
	unsigned char *buf,*freeptr;
	unsigned int idx;
	int iszero;
/*	unsigned char rfc1123_date[30];
	SipRfc1123Date *_date;
*/	int retvalue;

	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;
	_expires = (SipExpires*)calloc(1,sizeof(*_expires));
/*	_expires->expireDate = NULL;*/
	_expires->expireSecond = -1;

	/* check the string is zero */
	buf = (unsigned char*)strDup((const char*)trimWS((char*)ph->first->content));
	freeptr=buf;
	/*buf = (unsigned char*)trimWS((char*)buf);*/
	iszero = 1;
	for(idx=0;idx<strlen(buf);idx++) {
		if (*(buf+idx) != '0'){
			iszero = 0 ;
			break;
		}
	}
	if (iszero){
		_expires->expireSecond = 0;
		*ret = (void *)_expires;
		retvalue= 1;
	}else {
		if (ph->first == ph->last){
			_expires->expireSecond = atoi((const char*)buf);
			if (_expires->expireSecond == 0)
				retvalue= 0;
			else {
				*ret = (void *)_expires;
				retvalue= 1;
			}
		}/*else{
			int r = 0;
			strncpy((char*)rfc1123_date, (const char*)trimWS(ph->first->content), 3);
			*(rfc1123_date + 3) = '\0';
			strcat(rfc1123_date, ",");
			strcat(rfc1123_date, ph->last->content);

			_date =(SipRfc1123Date*) calloc(1,sizeof (*_date));

			r = RFCDateParse(rfc1123_date, _date);
			_expires->expireDate = _date;
			*ret = (void *)_expires;
			retvalue= r;
		}*/
	}
	free(freeptr);
	return retvalue;
} /* ExpiresParse */

int ExpiresAsStr(SipExpires *_expires, unsigned char *buf, int buflen, int *length)
{
	unsigned char tmp[12];
	/*char tmpbuf[512]={'\0'};should be remove this parameter, July 27 2001 Acer */
	int /*itmp,*/retvalue;
	char *tmpstr;
	DxStr ExpireStr;

	if (!_expires) return 0;
	/*ExpireStr=dxStrNew();
	dxStrCat(ExpireStr, "Expires:");*/

	if(_expires->expireSecond > -1) {
		ExpireStr=dxStrNew();
		dxStrCat(ExpireStr, "Expires:");
		memset(tmp, ' ', 12);
		tmpstr=i2a(_expires->expireSecond,(char*)tmp,12);
		dxStrCat(ExpireStr,(char*) tmp);
	}else 
		return 0;
	/*else{
		if (1 == RFCDateAsStr(_expires->expireDate,(unsigned char*)tmpbuf,512,&itmp));
			dxStrCat(ExpireStr,tmpbuf);
	}*/

	dxStrCat(ExpireStr, "\r\n");
	*length=dxStrLen(ExpireStr);

	if((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(ExpireStr));
		retvalue= 1;
	}
	dxStrFree(ExpireStr);
	return retvalue;
} /* ExpiresAsStr */

int ExpiresFree(SipExpires *_expires)
{
	if(_expires != NULL){
		/*if (_expires->expireDate != NULL){
		    free(_expires->expireDate);
		    _expires->expireDate=NULL;
		}*/
		memset((void*)_expires,0,sizeof(SipExpires));
		free(_expires); 
		/*_expires=NULL;*/
	}
	return 1;
} /* ExpiresFree */

int ErrorInfoParse(MsgHdr ph, void **ret)
{
	SipInfo	*_errorinfo,*tmperrorinfo;
	SipParam	*_parameter;
	MsgHdrElm	nowElm;
	unsigned char	*buf,*freeptr, *tmp,*parms;
	int		count;	
	
	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;
	_errorinfo = (SipInfo*)calloc(1,sizeof(*_errorinfo));
	_errorinfo->absoluteURI=NULL;
	_errorinfo->numParams=0;
	_errorinfo->SipParamList=NULL;
	_errorinfo->next=NULL;
	tmperrorinfo=_errorinfo;
	_parameter=NULL;
	nowElm = ph->first;

	while(1){
	/* parse absoluteURI */
		buf = (unsigned char*)strDup((const char*)trimWS((char*)nowElm->content));
		freeptr=buf;
		tmp=(unsigned char*)strChr(buf,'<');
		if(tmp != NULL){
			buf=tmp+1;
			tmp=(unsigned char*)strChr(buf,'>');
			if(tmp != NULL){
				*tmp='\0';
				_errorinfo->absoluteURI=strDup((const char*)buf);
				buf=tmp+1;
				buf=(unsigned char*)trimWS((char*)buf);
				
			}
		}else {
			_errorinfo->absoluteURI=NULL;
			if(freeptr) free(freeptr);
			TCRPrint(1, "*** [sipStack] ErrorInfo Parse error with URI=NULL! \n");
			break;
		}
	/* parse generic-param  */
		
		parms=(unsigned char*)strChr(buf,';');
		count=0;
		if(parms != NULL){
			*parms='\0';
			parms++;
			
			while(1){
				count++;
				tmp = (unsigned char*)strChr(parms, ';');
				if(_parameter==NULL)	
						_parameter =(SipParam*) calloc(1,sizeof(*_parameter));
				if(count==1)
						_errorinfo->SipParamList=_parameter;
				if (tmp == NULL){ /* only one parameter*/
					_parameter->next = NULL;
					AnalyzeParameter(parms,_parameter);
					break;
				}else{
					*tmp = '\0';
			
					AnalyzeParameter(parms,_parameter);
					parms=tmp+1;
					_parameter->next=(SipParam*) calloc(1,sizeof(*_parameter));
					_parameter=_parameter->next;
					_parameter->name=NULL;
					_parameter->value=NULL;
					_parameter->next=NULL;
				}
				
			}
			
		}
		_errorinfo->numParams=count;
		if(freeptr!=NULL)
			free(freeptr);
		if (nowElm->next != NULL){ 
			nowElm = nowElm->next;
			_errorinfo->next=(SipInfo*)calloc(1,sizeof(*_errorinfo));
			_errorinfo=_errorinfo->next;
			_errorinfo->numParams=0;
			_errorinfo->absoluteURI=NULL;
			_errorinfo->next=NULL;
			_errorinfo->SipParamList=NULL;
		}
		else
			break;
	}/*end while */
	*ret=(void *)tmperrorinfo;
	return 1;
} /* ErrorInfoParse tyhuang*/

int ErrorInfoAsStr(SipInfo *_errorinfo, unsigned char *buf, int buflen, int *length)
{
	DxStr ErrorInfoStr;
	int retvalue;
	SipParam* _parameter;

	ErrorInfoStr=dxStrNew();
	while(1){
		if(_errorinfo==NULL) break;
		dxStrCat(ErrorInfoStr, "Error-Info: <");
		dxStrCat(ErrorInfoStr,(const char*)_errorinfo->absoluteURI);
		dxStrCat(ErrorInfoStr,">");
		_parameter=_errorinfo->SipParamList;
		while (1) {
			if (_parameter != NULL){
				dxStrCat(ErrorInfoStr,";");
				dxStrCat(ErrorInfoStr, _parameter->name);
				if (_parameter->value != NULL){
					dxStrCat(ErrorInfoStr, "=");/*for equal */
					dxStrCat(ErrorInfoStr, _parameter->value);
				}
				_parameter = _parameter->next;
			} else
				break;
		}/*end while loop*/
		
		if(_errorinfo->next==NULL) break;
		else{
			_errorinfo=_errorinfo->next;
			dxStrCat(ErrorInfoStr,",");
		}
	}/*end while loop */

	dxStrCat(ErrorInfoStr, "\r\n");
	*length=dxStrLen(ErrorInfoStr);

	if ((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(ErrorInfoStr));
		retvalue= 1;
	}
	dxStrFree(ErrorInfoStr);
	return retvalue;
}/* ErrorInfoAsStr tyhuang*/

int ErrorInfoFree(SipInfo *_errorinfo)
{
	SipInfo *tmperrorinfo;

	while(_errorinfo!=NULL){
		if(_errorinfo->absoluteURI!=NULL){ 
			free(_errorinfo->absoluteURI);
			_errorinfo->absoluteURI=NULL;
		}
		if(_errorinfo->SipParamList!=NULL){
			sipParameterFree(_errorinfo->SipParamList);
			_errorinfo->SipParamList=NULL;
		}
		tmperrorinfo=_errorinfo->next;
		free(_errorinfo);
		_errorinfo=tmperrorinfo;
	}
	return 1;
}/* ErrorInfoFree tyhuang*/

int FromParse(MsgHdr ph, void **ret)
{
	SipFrom *_from;
	SipAddr *_addr;
	SipParam *_param,*tmpPara;
	unsigned char *tmp, *buf,*freeptr, *semicolon=NULL;
	int count=0;

	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;
	_from =(SipFrom*) calloc(1,sizeof (*_from));
	_from->numAddr=1;
	_from->numParam = 0;
	_from->ParamList = NULL;

	buf = (unsigned char*)strDup((const char*)trimWS(ph->first->content));
	freeptr=buf;
	/* drop headers ,find '?' */
	tmp=(unsigned char*)strChr(buf,'?');
	if (tmp){
		*tmp++='>';
		*tmp='\0';
	}else
		tmp=(unsigned char*)strChr(buf,'>');

	/* Parse addr */
	_addr = (SipAddr*)calloc(1,sizeof (*_addr));
	_from->address = _addr;

	/*
	while( buf[i]!='\0' ) {
		if( buf[i]=='<' ) {
			while( buf[i]!='>' && buf[i]!='\0' )	i++;
				continue;
		}
		else if( buf[i]==';' )
			break;
		else
			i++;
	}
	tmp = buf + i;
	*/
	
	if(tmp != NULL)
		tmp = (unsigned char*)strChr (tmp+1, ';');
	else
		tmp = (unsigned char*)strChr(buf , ';');

	if((tmp == NULL)||( *tmp=='\0' )||(*(tmp+1)=='\0'))
		AddrParse(buf, _addr);
	else{
		/* have parameter, parse */
		*tmp = '\0';
		tmp++;
		AddrParse(buf, _addr);
		
		while (1){
			semicolon  =(unsigned char*)strChr(tmp, ';');

			_param = (SipParam*)calloc(1,sizeof *_param);
			if (semicolon == NULL){
				if(*tmp == '\0'){
					free(_param);
					break;
				}
				
				AnalyzeParameter(tmp,_param);
						
				if(count==0){
					_from->ParamList = _param;
					tmpPara=_param;
				}else{
					tmpPara->next=_param;
					tmpPara=_param;
				}
				
				count++;
				break;
			}else{
				*semicolon = '\0';
				AnalyzeParameter(tmp,_param);
				if(count==0){
					_from->ParamList = _param;
					tmpPara=_param;
				}else{
					tmpPara->next=_param;
					tmpPara=_param;
				}
				count++;

				tmp = semicolon + 1;
			}
		}
	}


	_from->numParam=count;
	if(freeptr) free(freeptr);
	*ret = (void *)_from;
	return 1;
} /* FromParse*/

int FromAsStr(SipFrom *_from, unsigned char *buf, int buflen, int *length)
{
	int itmp, i,retvalue;
	/*SipStr *_str;*/
	SipParam *_para;
	DxStr FromStr;
	char tmpbuf[512]={'\0'};

	FromStr=dxStrNew();
	dxStrCat(FromStr, "From:");
	AddrAsStr(_from->address,(unsigned char*)tmpbuf,512,&itmp);
	dxStrCat(FromStr,tmpbuf);
	/*_str = _from->tagList;*/
	_para=_from->ParamList;
	for(i=0; (i<_from->numParam)&&(_para != NULL); i++) {
		dxStrCat(FromStr, ";");
		dxStrCat(FromStr,_para->name);
		dxStrCat(FromStr,"=");
		dxStrCat(FromStr,_para->value);
		_para=_para->next;
	}
	dxStrCat(FromStr, "\r\n");
	*length=dxStrLen(FromStr);

	if((*length) >= buflen)
		retvalue= 0;
	else {
		strcpy(buf,(const char*) dxStrAsCStr(FromStr));
		retvalue= 1;
	}
	dxStrFree(FromStr);
	return retvalue;

} /* FromAsStr */

int FromFree(SipFrom *_from)
{
	if(_from != NULL){
		if(_from->address!=NULL){
			sipAddrFree(_from->address);
			_from->address=NULL;
		}
		if(_from->ParamList != NULL){
			sipParameterFree(_from->ParamList);
			_from->ParamList=NULL;
		}
		free(_from);
		_from=NULL;
	}
	return 1;
} /* FromFree */

int HideParse(MsgHdr ph, void **ret)
{
	int _hide;
	unsigned char *buf;

	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;
	buf = (unsigned char*)strDup((const char*)trimWS(ph->first->content));
	for(_hide=route; _hide<=unknown_hide; _hide++)
		if (!strICmp((const char*)buf,hideName[_hide]))
			break;
	free(buf);
	*ret = (void *)_hide;
	return 1;
} /* HideParse */

int HideAsStr(SipHideType _hide, unsigned char *buf, int buflen, int *length)
{
	DxStr HideStr;
	int retvalue;

	HideStr=dxStrNew();
	dxStrCat(HideStr, "Hide:");
	dxStrCat(HideStr,(char*) hideName[_hide]);
	dxStrCat(HideStr, "\r\n");
	*length=dxStrLen(HideStr);

	if((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(HideStr));
		retvalue= 1;
	}
	dxStrFree(HideStr);
	return retvalue;
} /* HideAsStr */

int InReplyToParse(MsgHdr ph, void **ret)
{
	SipInReplyTo	*_inreplyto;
	SipStr		*_str;
	MsgHdrElm	nowElm;
	
	*ret=NULL;

	/* check input data */
	if(!ph->first)
		return 0;
	nowElm=ph->first;
	_inreplyto = (SipInReplyTo*)calloc(1,sizeof(*_inreplyto));
	_str = (SipStr*)calloc(1,sizeof(*_str));
	_inreplyto->CallidList = _str;
	_inreplyto->numCallids = ph->size;
	_str->str =strDup((const char*)trimWS(nowElm->content));
	nowElm = nowElm->next;
	while(1){
		if (nowElm != NULL){
			_str->next = (SipStr*)calloc(1,sizeof(*_str));
			_str = _str->next;
			_str->str = strDup((const char*)trimWS(nowElm->content));
			nowElm = nowElm->next;
		}else
			break;
	}
	_str->next = NULL;
	*ret = (void *)_inreplyto;
	return 1;
} /* InReplyToDParse */


int InReplyToAsStr(SipInReplyTo *_inreplyto, unsigned char *buf, int buflen, int *length)
{
	DxStr INREPLYTOStr;
	int retvalue;
	SipStr *_callid;

	_callid=_inreplyto->CallidList;
	INREPLYTOStr=dxStrNew();
	dxStrCat(INREPLYTOStr, "In-Reply-To:");
	dxStrCat(INREPLYTOStr,(const char*)_callid->str);
	_callid=_callid->next;
	while(_callid != NULL){
	dxStrCat(INREPLYTOStr, ",");
	dxStrCat(INREPLYTOStr, (const char*)_callid->str);
	_callid=_callid->next;
	}
	dxStrCat(INREPLYTOStr, "\r\n");
	*length=dxStrLen(INREPLYTOStr);

	if ((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(INREPLYTOStr));
		retvalue= 1;
	}
	dxStrFree(INREPLYTOStr);
	return retvalue;
} /* InReplyToAsStr */

int InReplyToFree(SipInReplyTo *inreplyto)
{

	if( inreplyto != NULL){
		if(inreplyto->CallidList){
			sipStrFree(inreplyto->CallidList);
			inreplyto->CallidList=NULL;
		}
		free(inreplyto);
		inreplyto=NULL;
	}
	return 1;
} /* InReplyToFree */

int MaxForwardsParse(MsgHdr ph, void **ret)
{
	/*static int _maxforwards;*/
	int _maxforwards,*ct;
	unsigned int i;
	int iszero=1;

	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;

	for(i=0; i<strlen(ph->first->content); i++){
		if (ph->first->content[i] != '0'){
			iszero=0;
			break;
		}
	}
	if (iszero){
		_maxforwards = 0;
	}else {
		_maxforwards = atoi(ph->first->content);
		
	}
	ct=(int*)calloc(1,sizeof(int));
	*ct=_maxforwards;
	/**ret = &_maxforwards;*/
	*ret=(void*)ct;
	return 1;

} /* MaxForwardsParse */

int MaxForwardsAsStr(int value, unsigned char *buf, int buflen, int *length)
{
	char tmp[12]={'\0'};
	char* tmpstr;
	DxStr MaxForwardStr;
	int retvalue;

	MaxForwardStr=dxStrNew();
	tmpstr=i2a(value,(char*)tmp,12);
	dxStrCat(MaxForwardStr, "Max-Forwards:");
	dxStrCat(MaxForwardStr, tmp);
	dxStrCat(MaxForwardStr, "\r\n");
	*length = dxStrLen(MaxForwardStr);

	if((*length) >= buflen)
		retvalue= 0;
	else {
		strcpy(buf,(const char*)dxStrAsCStr(MaxForwardStr));
		retvalue= 1;
	}
	dxStrFree(MaxForwardStr);
	return retvalue;
} /* MaxForwardsAsStr */

int MIMEVersionParse(MsgHdr ph, void **ret)
{
	unsigned char *buf,*dot,*tmp;

	buf=(unsigned char*)strDup((const char*)trimWS((char*)ph->first->content));
	tmp=buf;

	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;
	
	dot=(unsigned char*)strChr(tmp, '.');
	if((dot==NULL)||(dot==buf)) 
	{
		if(buf!=NULL)
			free(buf);
		return 0;
	}
	dot++;
	if(*dot=='\0') {
		if(buf!=NULL)
			free(buf);
		return 0;
	}
	tmp=dot;
	dot=(unsigned char*)strChr(tmp, '.');
	
	if(dot != NULL) {
		if(buf!=NULL)
			free(buf);
		return 0;
	}
	*ret = (void *)buf;
	return 1;	


} /* MIMEVersionParse */

int MIMEVersionAsStr(unsigned char *mimeversion, unsigned char *buf, int buflen, int *length)
{
	DxStr MimeStr;
	int retvalue;

	MimeStr=dxStrNew();

	dxStrCat(MimeStr, "MIME-Version:");
	dxStrCat(MimeStr,(char*) mimeversion);
	dxStrCat(MimeStr, "\r\n");
	*length=dxStrLen(MimeStr);

	if((*length) >= buflen)
		retvalue= 0;
	else {
		strcpy(buf,(const char*)dxStrAsCStr(MimeStr));
		retvalue= 1;
	}
	dxStrFree(MimeStr);
	return  retvalue;
} /* MIMEVersionAsStr */

int MIMEVersionFree(unsigned char *_mimeversion)
{
	if(_mimeversion != NULL){
		free(_mimeversion);
		_mimeversion=NULL;
	}
	return 1;
} /* MIMEVersionFree */

int MinExpiresParse(MsgHdr ph, void **ret)
{
	/*static int _minexpires;*/
	int _minexpires,*ct;
	unsigned int i;
	int iszero=1;

	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;

	for(i=0; i<strlen(ph->first->content); i++){
		if (ph->first->content[i] != '0'){
			iszero=0;
			break;
		}
	}
	if (iszero)
		_minexpires = 0;
	else {
		_minexpires = atoi(ph->first->content);
	
		/**ret = &_minexpires;
		return 1;*/
	}
	ct=(int*)calloc(1,sizeof(int));
	*ct=_minexpires;
	/**ret = &_maxforwards;*/
	*ret=(void*)ct;
	return 1;
} /* MinExpiresParse */

int MinExpiresAsStr(int value, unsigned char *buf, int buflen, int *length)
{
	char tmp[12]={'\0'};
	char* tmpstr;
	DxStr MinExpiresStr;
	int retvalue;

	MinExpiresStr=dxStrNew();
	tmpstr=i2a(value,(char*)tmp,12);
	dxStrCat(MinExpiresStr, "Min-Expires:");
	dxStrCat(MinExpiresStr, tmp);
	dxStrCat(MinExpiresStr, "\r\n");
	*length = dxStrLen(MinExpiresStr);

	if((*length) >= buflen)
		retvalue= 0;
	else {
		strcpy(buf,(const char*)dxStrAsCStr(MinExpiresStr));
		retvalue= 1;
	}
	dxStrFree(MinExpiresStr);
	return retvalue;
} /* MaxForwardsAsStr */

/* Min-SE*/
int MinSEParse(MsgHdr ph, void **ret)
{
	int paracount=0;
	SipParam *_para,*firstpara,*preparam;
	unsigned char *buf,*a,*b;
	BOOL bdoing=TRUE;
	MsgHdrElm nowElm;
	SipMinSE *minse;

	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;
	
	nowElm = ph->first;
	while(nowElm != NULL){
		minse=(SipMinSE*)calloc(1,sizeof(*minse));
		minse->deltasecond=-1;
		minse->numParam=0;
		minse->paramList=NULL;

		if(nowElm->content != NULL)
			buf=(unsigned char*)strDup((const char*)trimWS(nowElm->content));
		else{
			free(minse);
			break;
		}

		a=(unsigned char*)strChr(buf,';');
		if(a==0){ /* only delta-second */
			minse->deltasecond=atoi((const char*)buf);
		}else{ /*with parameter */
			*a=0;
			minse->deltasecond=atoi((const char*)buf);
			a++;
			while(bdoing){
				_para=(SipParam*)calloc(1,sizeof(*_para));
				_para->name=NULL;
				_para->value=NULL;
				_para->next=NULL;
				if((b=(unsigned char*)strChr(a,';'))!=0){
					*b=0;
					bdoing=TRUE;
					if(AnalyzeParameter(a,_para) == -1){
						free(_para);
						break;
					}
					a=b+1;
				}else{
					if(AnalyzeParameter(a,_para)==-1){
						free(_para);
						break;
					}
					bdoing=FALSE;
				}
				if(paracount ==0){
					minse->paramList=_para;
					firstpara=_para;
				}else{
					preparam->next=_para;
				}
				preparam=_para;
				paracount++;

			}
			minse->numParam=paracount;
		}
		nowElm=nowElm->next;
		free(buf);
	}

	*ret = (void*)minse;
	return 1;
}

/* MinSEAsStr */
int MinSEAsStr(SipMinSE *minse,unsigned char *buf,int buflen,int *length)
{
	DxStr MinSEStr;
	char buff[64]={'\0'};
	SipParam *_para;
	int retvalue;
	int i;

	MinSEStr=dxStrNew();
	if(minse != NULL){
		dxStrCat(MinSEStr,"Min-SE:");
		sprintf(buff,"%d",minse->deltasecond);
		dxStrCat(MinSEStr,buff);
		_para=minse->paramList;
		for(i=0;(i<minse->numParam)&&(_para != NULL);i++){
			dxStrCat(MinSEStr,";");
			dxStrCat(MinSEStr,_para->name);
			if(_para->value != NULL){
				dxStrCat(MinSEStr,"=");
				dxStrCat(MinSEStr,_para->value);
			}
			_para=_para->next;
		}
	}
	dxStrCat(MinSEStr,"\r\n");
	*length=dxStrLen(MinSEStr);
	if((*length) >= buflen)
		retvalue= 0;
	else {
		strcpy(buf,(const char*) dxStrAsCStr(MinSEStr));
		retvalue= 1;
	}
	dxStrFree(MinSEStr);
	return retvalue;

	return 1;

}

/* MinSEFree */
int MinSEFree(SipMinSE *minse)
{
	if(minse != NULL){
		if(minse->paramList != NULL){
			sipParameterFree(minse->paramList);
			minse->paramList=NULL;
		}
		free(minse);
		minse=NULL;
	}
	return 1;
}

int OrganizationParse(MsgHdr ph, void **ret)
{
	unsigned char* buf;
	int count;
	MsgHdrElm nowElm;

	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;

	nowElm = ph->first;
	count = strlen(nowElm->content);
	nowElm = nowElm->next;
	while(1){
		if (nowElm != NULL){
			count = count + strlen(nowElm->content);
			nowElm = nowElm->next;
		}else
			break;
	}
	buf = (unsigned char*)calloc(1,count+1);
	nowElm = ph->first;
	strcpy(buf, trimWS(nowElm->content));
	nowElm = nowElm->next;
	while(1){
		if (nowElm != NULL){
			strcat(buf, ",");
			strcat(buf, nowElm->content);
			nowElm = nowElm->next;
		}else
			break;
	}
	*ret = (void *)buf;
	return 1;
} /* OrganizationParse */

int OrganizationAsStr(unsigned char *organization, unsigned char *buf, int buflen, int *length)
{
	DxStr OrganStr;
	int retvalue;

	OrganStr=dxStrNew();

	dxStrCat(OrganStr, "Organization:");
	dxStrCat(OrganStr,(char*) organization);
	dxStrCat(OrganStr, "\r\n");
	*length=dxStrLen(OrganStr);

	if((*length) >= buflen)
		retvalue= 0;
	else {
		strcpy(buf,(const char*)dxStrAsCStr(OrganStr));
		retvalue= 1;
	}
	dxStrFree(OrganStr);
	return  retvalue;
} /* OrganizationAsStr */

int OrganizationFree(unsigned char *_organization)
{
	if(_organization != NULL){
		free(_organization);
		_organization=NULL;
	}
	return 1;
} /* OrganizationFree */

int	PAssertedIdentityParse(MsgHdr ph, void **ret)
{
	SipIdentity *pId;
	unsigned char *buf,*freeptr;
	MsgHdrElm	nowElm;
	SipAddr *tmpaddr=NULL;
	int num=0;

	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;
	
	nowElm = ph->first;
	pId=(SipIdentity*)calloc(1,sizeof(SipIdentity));
	if(!pId)
		return 0;
	tmpaddr=(SipAddr*)calloc(1,sizeof(SipAddr));
	if(!tmpaddr){
		free(pId);
		return 0;
	}
	pId->address=tmpaddr;
	tmpaddr->display_name=NULL;
	tmpaddr->addr=NULL;
	tmpaddr->with_angle_bracket=0;
	tmpaddr->next=NULL;
	while(nowElm){
		if(nowElm->content){
			buf=(unsigned char*)strDup((const char*)trimWS(nowElm->content));
			freeptr=buf;
			AddrParse(buf,tmpaddr);
			free(freeptr);
			num++;
		}
		nowElm=nowElm->next;
		if(nowElm){
			tmpaddr->next=(SipAddr*)calloc(1,sizeof(SipAddr));
			tmpaddr=tmpaddr->next;
			tmpaddr->display_name=NULL;
			tmpaddr->addr=NULL;
			tmpaddr->with_angle_bracket=0;
			tmpaddr->next=NULL;
		}
	}
	pId->numAddr=num;
	pId->next=NULL;
	*ret=(void*)pId;

	return 1;
}
int	PAssertedIdentityAsStr(SipIdentity *pId, unsigned char *buf, int buflen, int *length)
{
	DxStr OutStr;
	SipAddr *tmpaddr;
	int retvalue;

	OutStr=dxStrNew();
	while(pId){
		tmpaddr=pId->address;

		dxStrCat(OutStr, "P-Asserted-Identity:");
		while(tmpaddr){
			if(tmpaddr->display_name){
				dxStrCat(OutStr,"\"");
				dxStrCat(OutStr,(char*) tmpaddr->display_name);
				dxStrCat(OutStr,"\"");
			}
			if(tmpaddr->addr){
				if (tmpaddr->with_angle_bracket)
					dxStrCat(OutStr, "<");

				dxStrCat(OutStr, tmpaddr->addr);

				if (tmpaddr->with_angle_bracket)
					dxStrCat(OutStr, ">");
			}
			tmpaddr=tmpaddr->next;
			if(tmpaddr)
				dxStrCat(OutStr,",");
		}
		
		dxStrCat(OutStr, "\r\n");
		pId=pId->next;
	}

	*length=dxStrLen(OutStr);
	if((*length) >= buflen)
		retvalue= 0;
	else {
		strcpy(buf,(const char*)dxStrAsCStr(OutStr));
		retvalue= 1;
	}
	dxStrFree(OutStr);
	return  retvalue;
}

int	PAssertedIdentityFree(SipIdentity *pId)
{
	SipIdentity *tmp;
	while(pId){
		if(pId->address){
			sipAddrFree(pId->address);
			pId->address=NULL;
		}
		tmp=pId->next;
		free(pId);
		pId=tmp;
	}
	return 1;
}

int	PPreferredIdentityParse(MsgHdr ph, void **ret)
{
	return PAssertedIdentityParse(ph,ret);
}

int	PPreferredIdentityAsStr(SipIdentity *pId, unsigned char *buf, int buflen, int *length)
{
	DxStr OutStr;
	SipAddr *tmpaddr;
	int retvalue;

	OutStr=dxStrNew();
	while(pId){
		tmpaddr=pId->address;

		dxStrCat(OutStr, "P-Preferred-Identity:");
		while(tmpaddr){
			if(tmpaddr->display_name){
				dxStrCat(OutStr,"\"");
				dxStrCat(OutStr,(char*) tmpaddr->display_name);
				dxStrCat(OutStr,"\"");
			}
			if(tmpaddr->addr){
				if (tmpaddr->with_angle_bracket)
					dxStrCat(OutStr, "<");

				dxStrCat(OutStr, tmpaddr->addr);

				if (tmpaddr->with_angle_bracket)
					dxStrCat(OutStr, ">");
			}
			tmpaddr=tmpaddr->next;
			if(tmpaddr)
				dxStrCat(OutStr,",");
		}
		
		dxStrCat(OutStr, "\r\n");
		pId=pId->next;
	}
	*length=dxStrLen(OutStr);

	if((*length) >= buflen)
		retvalue= 0;
	else {
		strcpy(buf,(const char*)dxStrAsCStr(OutStr));
		retvalue= 1;
	}
	dxStrFree(OutStr);
	return  retvalue;
}

int	PPreferredIdentityFree(SipIdentity *pId)
{
	return PAssertedIdentityFree(pId);
}

int PriorityParse(MsgHdr ph, void **ret)
{
	int _priority;
	unsigned char *buf;

	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;

	buf = (unsigned char*)strDup((const char*)trimWS(ph->first->content));
	for(_priority=emergency; _priority<=Unkn_priority; _priority++)
		if (!strICmp((const char*)buf,priorityName[_priority]))
			break;
	*ret = (void *)_priority;
	free(buf);
	return 1;
} /* PriorityParse */

int PriorityAsStr(SipPriorityType _priority, unsigned char *buf, int buflen, int *length)
{
	DxStr PriorityStr;
	int retvalue;

	PriorityStr=dxStrNew();
	dxStrCat(PriorityStr, "Priority:");
	dxStrCat(PriorityStr, (char*)priorityName[_priority]);
	dxStrCat(PriorityStr, "\r\n");
	*length=dxStrLen(PriorityStr);

	if((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*) dxStrAsCStr(PriorityStr));
		retvalue= 1;
	}
	dxStrFree(PriorityStr);
	return retvalue;
} /* PriorityAsStr */

int	PrivacyParse(MsgHdr ph, void **ret)
{
	MsgHdrElm	nowElm;
	SipPrivacy	*pPriv=NULL;
	unsigned char	*buf,*freeptr;
	char *semi;
	SipStr	*tmpstr;
	int	num=0;

	*ret=NULL;
	nowElm = ph->first;
	if(!nowElm)
		return 0;
	if(!nowElm->content)
		return 0;
	buf = (unsigned char*)strDup((const char*)trimWS(nowElm->content));
	freeptr = buf;
	pPriv=(SipPrivacy*)calloc(1,sizeof(*pPriv));
	if(!pPriv){
		free(freeptr);
		return 0;
	}
	tmpstr=(SipStr*)calloc(1,sizeof(*tmpstr));
	if(!tmpstr){
		free(freeptr);
		free(pPriv);
		return 0;
	}
	pPriv->privvalue=tmpstr;
	tmpstr->next=NULL;
	while(1){
		semi=strChr(buf,';');
		if(!semi) break;
		else
			*semi='\0';	
		
		tmpstr->str=strDup((const char*)buf);
		num++;
		tmpstr->next=(SipStr*)calloc(1,sizeof(*tmpstr));
		tmpstr=tmpstr->next;
		tmpstr->next=NULL;
		/* move buf point */
		buf=(unsigned char*)semi+1;
	}
	if(*buf!='\0'){
		tmpstr->str=strDup((const char*)buf);
		num++;
	}else
		tmpstr->str=NULL;

	if(freeptr)
		free(freeptr);
	/* set number of privacy */
	pPriv->numPriv=num;
	*ret=(void*)pPriv;
	return 1;
}

int	PrivacyAsStr(SipPrivacy *pPrivacy, unsigned char *buf, int buflen, int *length)
{
	DxStr OutStr;
	SipStr *tmpstr;
	int retvalue;

	OutStr=dxStrNew();
	if(pPrivacy)
		tmpstr=pPrivacy->privvalue;

	dxStrCat(OutStr, "Privacy:");
	while(tmpstr){
		if(tmpstr->str)
			dxStrCat(OutStr,(char*) tmpstr->str);
		tmpstr=tmpstr->next;
		if(tmpstr)
			dxStrCat(OutStr,";");
	}
	
	dxStrCat(OutStr, "\r\n");
	*length=dxStrLen(OutStr);

	if((*length) >= buflen)
		retvalue= 0;
	else {
		strcpy(buf,(const char*)dxStrAsCStr(OutStr));
		retvalue= 1;
	}
	dxStrFree(OutStr);
	return  retvalue;
}

int	PrivacyFree(SipPrivacy *pPrivacy)
{
	if(pPrivacy){
		if(pPrivacy->privvalue){
			sipStrFree(pPrivacy->privvalue);
			pPrivacy->privvalue=NULL;
		}
		free(pPrivacy);
	}else
		return 0;
	return 1;
}


int ProxyAuthenticateParse(MsgHdr ph, void **ret)
{
	SipPxyAuthn	*_proxyauthenticate;
	SipChllng	*_challenge;
	SipParam	*_parameter;
	MsgHdrElm       nowElm;
	unsigned char   *buf,*buffer,*freeptr, *tmp;

	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;
	
	_proxyauthenticate = (SipPxyAuthn*)calloc(1,sizeof(*_proxyauthenticate));
	_proxyauthenticate->numChllngs = 0;
	nowElm = ph->first;
	buf = (unsigned char*)strDup((const char*)trimWS(nowElm->content));
	freeptr=buf;
	buf=(unsigned char*)trimWS((char*)buf);
	_challenge = (SipChllng*)calloc(1,sizeof(*_challenge));
	_challenge->numParams = 0;
	_challenge->next = NULL;
	_proxyauthenticate->ChllngList = _challenge;

	while (1){
		/* scheme */
		tmp = (unsigned char*)strChr(buf, ' ');
		if (tmp == NULL){
			if(freeptr!=NULL)
				free(freeptr);
			if(_proxyauthenticate) 
				ProxyAuthenticateFree(_proxyauthenticate);
			TCRPrint(1, "*** [sipStack] ProxyAuthenticate Parse error! \n");
			return 0;
		}
		else{
			int pos;
			unsigned char tmpbuf[12];
			
			_proxyauthenticate->numChllngs++;
			pos=strlen(buf)-strlen(tmp);
			memset(tmpbuf,0,12);
			strncpy((char*)tmpbuf,(char*)buf,pos);
			_challenge->scheme=strDup((const char*)tmpbuf);
			_parameter = (SipParam*)calloc(1,sizeof(*_parameter));
			_challenge->sipParamList = _parameter;
			*tmp = '\0';
			AnalyzeParameter(tmp+1, _parameter);
			_challenge->numParams++;
		}

		/* parameter */
		while (1){
			if (nowElm == ph->last){
				*ret = (void*)_proxyauthenticate;
				if(freeptr!=NULL) 
					free(freeptr);
				return 1;
			}else{
				nowElm = nowElm->next;
				buffer = (unsigned char*)strDup((const char*)trimWS(nowElm->content));
				/*tmp = (unsigned char*)strstr(buffer, " ");*/ /* Acer Modified 4/21 SipIt*/
				tmp=NULL;
				if (tmp != NULL){
					_challenge->next =(SipChllng*) calloc(1,sizeof(*_challenge));
					_challenge = _challenge->next;
					_challenge->numParams = 0;
					_challenge->next = NULL;
					break;
				}else{
					_parameter->next = (SipParam*)calloc(1,sizeof(*_parameter));
					_parameter = _parameter->next;
					AnalyzeParameter(buffer, _parameter);
					_challenge->numParams++;
				}
				free(buffer);
			}
		}
	}/*end while loop*/
	if(freeptr!= NULL)
		free(freeptr);
} /* ProxyAuthenticateParse */

int ProxyAuthenticateAsStr(SipPxyAuthn *_proxyauthenticate, unsigned char *buf, int buflen, int *length)
{
	SipChllng *_challenge;
	SipParam* _parameter;
	int i,j,retvalue;
	DxStr ProxyAuthenStr;

	ProxyAuthenStr=dxStrNew();
	_challenge = _proxyauthenticate->ChllngList;
	for(i=0; i<_proxyauthenticate->numChllngs; i++) {
		dxStrCat(ProxyAuthenStr, "Proxy-Authenticate:");
		dxStrCat(ProxyAuthenStr, _challenge->scheme);
		dxStrCat(ProxyAuthenStr, " ");
		_parameter = _challenge->sipParamList;
		for(j=0; j<_challenge->numParams; j++){
			dxStrCat(ProxyAuthenStr, _parameter->name);
			dxStrCat(ProxyAuthenStr, "=");
			dxStrCat(ProxyAuthenStr, _parameter->value);
			if (_parameter->next != NULL)
				dxStrCat(ProxyAuthenStr, ",");/*for comma*/
			_parameter = _parameter->next;
		}
		/*if (_challenge->next != NULL)
			dxStrCat(ProxyAuthenStr, ",");*/
		dxStrCat(ProxyAuthenStr, "\r\n");
		_challenge = _challenge->next;
	}
	*length = dxStrLen(ProxyAuthenStr);

	if((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*) dxStrAsCStr(ProxyAuthenStr));
		retvalue= 1;
	}
	dxStrFree(ProxyAuthenStr);
	return retvalue;
} /* ProxyAuthenticateAsStr */

int ProxyAuthenticateFree(SipPxyAuthn *_proxyauthenticate)
{
	if(_proxyauthenticate != NULL){
		if(_proxyauthenticate->ChllngList){
			sipChallengeFree(_proxyauthenticate->ChllngList);
			_proxyauthenticate->ChllngList=NULL;
		}
		free(_proxyauthenticate);
		_proxyauthenticate=NULL;
	}
	return 1;
} /* ProxyAuthenticateFree */

int ProxyAuthorizationParse(MsgHdr ph, void **ret)
{
	SipPxyAuthz *_proxyauthorization;
	SipParam*      _parameter;
	MsgHdrElm         nowElm;
	unsigned char     *buf,*freeptr, *tmp;
	int count=0;
	int doing=TRUE;

	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;

	_proxyauthorization = (SipPxyAuthz*)calloc(1,sizeof(*_proxyauthorization));
	_proxyauthorization->numParams = ph->size;
	_proxyauthorization->next=NULL;
	nowElm = ph->first;
	buf = (unsigned char*)strDup((const char*)trimWS(nowElm->content));
	freeptr=buf;
	/* scheme */
	tmp = (unsigned char*)strChr(buf, ' ');
	if (tmp == NULL){
		if(freeptr) free(freeptr);
		if(_proxyauthorization) free(_proxyauthorization);
		TCRPrint(1, "*** [sipStack] ProxyAuthorization Parse error! \n");
		return 0;
	}else{
		*tmp = '\0';
		_proxyauthorization->scheme =strDup((const char*) buf);
		_parameter = (SipParam*)calloc(1,sizeof(*_parameter));
		_proxyauthorization->sipParamList = _parameter;
		buf=(tmp+1);
		if(buf!=NULL){
			count++;
			AnalyzeParameter(buf, _parameter);
			if(ph->size == 1){
				_parameter->next=NULL;
				doing =FALSE;
			}else{
				doing = TRUE;
				_parameter->next=(SipParam*)calloc(1,sizeof(*_parameter));
				_parameter=_parameter->next;
				_parameter->name=NULL;
				_parameter->value=NULL;
				_parameter->next=NULL;
			}
		}/*end if*/
	}/*end else*/
	free(freeptr);
	
	/* parameter */
	nowElm=nowElm->next;
	buf = (nowElm)? ((unsigned char*)strDup((const char*)trimWS((char*)nowElm->content))):((unsigned char*)NULL);
	freeptr=buf;

	while (doing){
		count++;
		AnalyzeParameter(buf, _parameter);
		if(nowElm==ph->last) 
			break;
		nowElm = nowElm->next;
		_parameter->next =(SipParam*) calloc(1,sizeof(*_parameter));
		_parameter = _parameter->next;
		if(freeptr!= NULL)
			free(freeptr);
		buf = (unsigned char*)strDup((const char*)trimWS((char*)nowElm->content));
		freeptr=buf;
	}
	_proxyauthorization->numParams= count;
	*ret = (void *)_proxyauthorization;
	if(freeptr != NULL)
		free(freeptr);
	return 1;
} /* ProxyAuthorizationParse */

int ProxyAuthorizationAsStr(SipPxyAuthz *_proxyauthorization, unsigned char *buf, int buflen, int *length)
{
	SipParam* _parameter;
	SipPxyAuthz *tmpPxyAuthz;
	int i,retvalue;
	DxStr ProxyAuthorStr;

	ProxyAuthorStr=dxStrNew();
	tmpPxyAuthz=_proxyauthorization;
	while(tmpPxyAuthz != NULL){
		dxStrCat(ProxyAuthorStr, "Proxy-Authorization:");
		dxStrCat(ProxyAuthorStr, tmpPxyAuthz->scheme);
		dxStrCat(ProxyAuthorStr, " ");/*for space*/
		_parameter = tmpPxyAuthz->sipParamList;
		for(i=0; i<tmpPxyAuthz->numParams; i++)  {
			dxStrCat(ProxyAuthorStr, _parameter->name);
			if( _parameter->value ) {
				dxStrCat(ProxyAuthorStr, "=");
				/* if nc or qop ,unqote it's value */
				if((strcmp(_parameter->name,"nc")==0)||(strcmp(_parameter->name,"qop")==0))
					dxStrCat(ProxyAuthorStr,unquote(_parameter->value));
				else
					dxStrCat(ProxyAuthorStr, _parameter->value);
				/*dxStrCat(ProxyAuthorStr, _parameter->value);*/
			}
			_parameter=_parameter->next;
			if (_parameter != NULL)
				dxStrCat(ProxyAuthorStr, ",");/*for comma*/
		}
		dxStrCat(ProxyAuthorStr, "\r\n");
		tmpPxyAuthz=tmpPxyAuthz->next;
	}
	*length=dxStrLen(ProxyAuthorStr);
	if((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(ProxyAuthorStr));
		retvalue= 1;
	}
	dxStrFree(ProxyAuthorStr);
	return retvalue;
} /* ProxyAuthorizationAsStr */

int ProxyAuthorizationFree(SipPxyAuthz *_proxyauthorization)
{
	SipPxyAuthz *tmpAuthz,*nowAuthz;
	nowAuthz=_proxyauthorization;
	while(nowAuthz!=NULL){
		if(nowAuthz->scheme!=NULL){
			free(nowAuthz->scheme);
			nowAuthz->scheme=NULL;
		}
		if(nowAuthz->sipParamList){
			sipParameterFree(nowAuthz->sipParamList);
			nowAuthz->sipParamList=NULL;
		}
		tmpAuthz=nowAuthz->next;
		free(nowAuthz);
		nowAuthz=tmpAuthz;
	}
	return 1;
} /* ProxyAuthorizationFree */

int ProxyRequireParse(MsgHdr ph, void **ret)
{
	SipPxyRequire *_proxyrequire;
	MsgHdrElm nowElm;
	SipStr *_str;

	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;
	
	_proxyrequire = (SipPxyRequire*)calloc(1,sizeof(*_proxyrequire));
	_proxyrequire->numTags = ph->size;
	nowElm = ph->first;
	_str = (SipStr*)calloc(1,sizeof(*_str));
	_proxyrequire->sipOptionTagList = _str;
	_str->str =strDup((const char*)trimWS(nowElm->content));
	nowElm = nowElm->next;

	while(1){
		if (nowElm != NULL){
			_str->next = (SipStr*)calloc(1,sizeof(*_str));
			_str = _str->next;
			_str->str = strDup((const char*)trimWS(nowElm->content));
			nowElm = nowElm->next;
		}else
			break;
	}
	_str->next = NULL;
	*ret = (void *)_proxyrequire;
	return 1;
} /* ProxyRequireParse */

int ProxyRequireAsStr(SipPxyRequire *_proxyrequire, unsigned char *buf, int buflen, int *length)
{
	SipStr *_str;
	int i,retvalue;
	DxStr ProxyRequireStr;

	ProxyRequireStr=dxStrNew();
	dxStrCat(ProxyRequireStr, "Proxy-Require:");
	_str = _proxyrequire->sipOptionTagList;
	for(i=0; i<_proxyrequire->numTags; i++) {
		dxStrCat(ProxyRequireStr, _str->str);
		if (_str->next != NULL)
			dxStrCat(ProxyRequireStr, ",");/*for comma*/
		_str = _str->next;
	}
	dxStrCat(ProxyRequireStr, "\r\n");
	*length=dxStrLen(ProxyRequireStr);

	if((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(ProxyRequireStr));
		retvalue= 1;
	}
	dxStrFree(ProxyRequireStr);
	return retvalue;
} /* ProxyRequireAsStr */

int ProxyRequireFree(SipPxyRequire *_proxyrequire)
{
	if(_proxyrequire != NULL){
		if(_proxyrequire->sipOptionTagList != NULL){
			sipStrFree(_proxyrequire->sipOptionTagList);
			_proxyrequire->sipOptionTagList=NULL;
		}
		free(_proxyrequire);
		_proxyrequire=NULL;
	}
	return 1;
} /* ProxyRequireFree */

int	RAckParse(MsgHdr ph, void **ret)
{
	SipRAck *pRack;
	MsgHdrElm nowElm;
	unsigned char *buf,*freeptr;
	char *ws;

	*ret=NULL;
	if(ph)
		nowElm = ph->first;
	else
		return 0;

	if(nowElm){
		buf=(unsigned char*)strDup((const char*)trimWS(nowElm->content));
		freeptr=buf;
		pRack=(SipRAck*)calloc(1,sizeof(*pRack));
		if(pRack==NULL)
			return 0;
		pRack->rseq=0;
		pRack->cseq=0;
		pRack->Method=INVITE; /* default method */
	}else
		return 0;

	/* rseq */
	ws=strChr(buf,' ');
	if(ws){
		*ws='\0';
		pRack->rseq=atoi((const char*)buf);
		buf=(unsigned char*)ws+1;
	}
	/* cseq */
	ws=strChr(buf,' ');
	if(ws){
		*ws='\0';
		pRack->cseq=atoi((const char*)buf);
		buf=(unsigned char*)ws+1;
	}
	/* method*/
	buf = (unsigned char*)trimWS((char*)buf);
	pRack->Method = sipMethodNameToType((char*)buf);
	
	if(freeptr)
		free(freeptr);
	*ret=(void *)pRack;

	return 1;
}
int	RAckAsStr(SipRAck *pRack, unsigned char *buf, int buflen, int *length)
{

	char tmp[16]={'\0'};
	char *tmpstr;
	DxStr PAckStr;
	int retvalue;

	PAckStr=dxStrNew();

	dxStrCat(PAckStr, "RAck:");
	tmpstr=i2a(pRack->rseq,(char*)tmp,15);
	dxStrCat(PAckStr, tmp);
	dxStrCat(PAckStr, " ");
	
	tmpstr=i2a(pRack->cseq,(char*)tmp,15);
	dxStrCat(PAckStr, tmp);
	dxStrCat(PAckStr, " ");

	dxStrCat(PAckStr, (char*)sipMethodName[pRack->Method]);
	dxStrCat(PAckStr, "\r\n");
	*length=dxStrLen(PAckStr);

	if((*length) >= buflen)
		retvalue =0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(PAckStr));
		retvalue= 1;
	}
	dxStrFree(PAckStr);
	return retvalue;

}
int	RAckFree(SipRAck *pRack)
{
	if(pRack)
		free(pRack);
	return 1;
}

int RecordRouteParse(MsgHdr ph, void **ret)
{
	SipRecordRoute *_recordroute;
	SipRecAddr  *_addr;
	SipAddr *_addrtmp;
	MsgHdrElm nowElm;
	unsigned char *buf,*freeptr,*semicolon,*tmp;
	int i,retvalue=1,count=0;
	SipParam *_param,*tmpPara;

	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;
	
	_recordroute =(SipRecordRoute*)calloc(1,sizeof(*_recordroute));
	if(_recordroute){
		_recordroute->numNameAddrs = ph->size;
		_recordroute->sipNameAddrList = NULL;
	}else
		return RC_ERROR;

	_addr = (SipRecAddr*)calloc(1,sizeof(*_addr));
	if(_addr){
		_addr->addr=NULL;
		_addr->display_name=NULL;
		_addr->sipParamList=NULL;
		_addr->next=NULL;
		_addr->numParams=0;
		_recordroute->sipNameAddrList = _addr;
	}else{
		free(_recordroute);
		return RC_ERROR;
	}
	
	nowElm = ph->first;
	for(i=0; i<ph->size; i++){
		buf = (unsigned char*)strDup((const char*)trimWS(nowElm->content));
		freeptr=buf;
		count=0;
		/* check if parameter exist */
		tmp =(unsigned char*)strChr(buf,'>');
		if(tmp)
			tmp=(unsigned char*)strChr(tmp, ';');
		else
			tmp=(unsigned char*)strChr(buf, ';');

		if(tmp){
			*tmp++='\0';
			while (1){
				semicolon  =(unsigned char*)strChr(tmp, ';');
				_param = (SipParam*)calloc(1,sizeof *_param);
				if (semicolon == NULL){
					if(*tmp == '\0') 
						break;
					AnalyzeParameter(tmp,_param);
					if(count==0)
						_addr->sipParamList = _param;
					else
						tmpPara->next=_param;
					count++;
					break;
				}else{
					*semicolon = '\0';
					AnalyzeParameter(tmp,_param);
					if(count==0){
						_addr->sipParamList = _param;
						tmpPara=_param;
					}else{
						tmpPara->next=_param;
						tmpPara=_param;
					}
					count++;
					tmp = semicolon + 1;
				}
			}
		}
		_addr->numParams=count;
		_addrtmp=(SipAddr*)calloc(1,sizeof(*_addrtmp));
		if (AddrParse(buf, _addrtmp)){
			_addr->addr=strDup(_addrtmp->addr);
			_addr->display_name=strDup(_addrtmp->display_name);
			sipAddrFree(_addrtmp);
			if ((nowElm != ph->last) && nowElm->next){
				nowElm = nowElm->next;
				_addr->next = (SipRecAddr*)calloc(1,sizeof(*_addr));
				_addr = _addr->next;
				_addr->addr=NULL;
				_addr->display_name=NULL;
				_addr->sipParamList=NULL;
				_addr->next=NULL;
				_addr->numParams=0;
			}else{
				if(freeptr!=NULL)
					free(freeptr);
				break;
			}
		}else
			retvalue= 0;

		if(freeptr!=NULL)
			free(freeptr);
	}
	*ret = (void *)_recordroute;
	return retvalue;
} /* RecordRouteParse */

int RecordRouteAsStr(SipRecordRoute *_recordroute, unsigned char *buf, int buflen, int *length)
{
	int  i,j,retvalue;
	SipRecAddr *_addr;
	DxStr RecordRouteStr;
	SipParam *_param;

	RecordRouteStr=dxStrNew();
	dxStrCat(RecordRouteStr, "Record-Route:");
	_addr = _recordroute->sipNameAddrList;
	for(i=0; i<_recordroute->numNameAddrs; i++){
		/*AddrAsStr(_addr,(unsigned char*) tmpbuf, 1024, &itmp);*/
		if(_addr->display_name){
			dxStrCat(RecordRouteStr,"\"");
			dxStrCat(RecordRouteStr,_addr->display_name);
			dxStrCat(RecordRouteStr,"\"");
		}
		if(_addr->addr){
			dxStrCat(RecordRouteStr,"<");
			dxStrCat(RecordRouteStr,_addr->addr);
			dxStrCat(RecordRouteStr,">");
		}
		
		_param=_addr->sipParamList;
		for(j=0;j<_addr->numParams;j++){
			if(_param){
				dxStrCat(RecordRouteStr,";");
				dxStrCat(RecordRouteStr,_param->name);
				if(_param->value){
					dxStrCat(RecordRouteStr,"=");
					dxStrCat(RecordRouteStr,_param->value);
				}
			}
		}
		if (_addr->next != NULL)  {
			dxStrCat(RecordRouteStr, ",");
		}
		_addr = _addr->next;
		if(_addr==NULL) break;
	}
	dxStrCat(RecordRouteStr, "\r\n");
	*length=dxStrLen(RecordRouteStr);

	if((*length) >= buflen)
		retvalue= 0;
	else {
		strcpy(buf,(const char*)dxStrAsCStr(RecordRouteStr));
		retvalue= 1;
	}
	dxStrFree(RecordRouteStr);
	return retvalue;
} /* RecordRouteAsStr */


int RecordRouteFree(SipRecordRoute *_recordroute)
{
	if(_recordroute != NULL){
		if(_recordroute->sipNameAddrList != NULL){
			sipRecAddrFree(_recordroute->sipNameAddrList);
			_recordroute->sipNameAddrList=NULL;
		}
		free(_recordroute);
		_recordroute=NULL;
	}
	return 1;
} /* RecordRouteFree */

/*Refer-To */
int ReferToParse(MsgHdr ph, void **ret)
{
	SipReferTo *_referto;
	SipAddr *_addr;
	SipParam *_param, *tmpPara;
	unsigned char *tmp, *buf,*freeptr, *semicolon;
	int i = 0,count=0;

	*ret=NULL;
	/* chech input data */
	if(!ph->first || !ph->first->content)
		return 0;
	
	_referto =(SipReferTo*) calloc(1,sizeof (*_referto));
	if (!_referto) 
		return RC_ERROR;
	
	_referto->numAddr=1;
	_referto->numParam = 0;
	_referto->paramList = NULL;

	buf = (unsigned char*)strDup((const char*)trimWS(ph->first->content));
	freeptr=buf;
	/* Parse addr */
	_addr = (SipAddr*)calloc(1,sizeof (*_addr));
	_referto->address = _addr;

	while( buf[i]!='\0' ) {
		if( buf[i]=='<' ) {
			while( buf[i]!='>' && buf[i]!='\0' )	i++;
				continue;
		}
		else if( buf[i]==';' )
			break;
		else
			i++;
	}
	tmp = buf + i;

	/*tmp = strstr(buf, ";");*/
	
	if(( *tmp=='\0' )||(*(tmp+1)=='\0'))
		AddrParse(buf, _addr);
	else{
		/* have parameter, parse */
		*tmp = '\0';
		tmp++;
		AddrParse(buf, _addr);
		while (1){
			semicolon  =(unsigned char*)strChr(tmp, ';');

			_param = (SipParam*)calloc(1,sizeof *_param);
			if (semicolon == NULL){
				if(*tmp == '\0') 
					break;
				AnalyzeParameter(tmp,_param);
				if(count==0){
					_referto->paramList = _param;
					tmpPara=_param;
				}else{
					tmpPara->next=_param;
					tmpPara=_param;
				}
				count++;
				break;
			}else{
				*semicolon = '\0';
				AnalyzeParameter(tmp,_param);
				if(count==0){
					_referto->paramList = _param;
					tmpPara=_param;
				}else{
					tmpPara->next=_param;
					tmpPara=_param;
				}
				count++;

				tmp = semicolon + 1;
			}
		}
	}
	_referto->numParam=count;
	free(freeptr);

	/* add number of address to notify up-layer,more than one address */
	if(ph->first->next)
		_referto->numAddr++;
		
	*ret = (void *)_referto;
	return 1;
}

int ReferToAsStr(SipReferTo *_refer, unsigned char *buf, int buflen, int *length)
{
	DxStr ReferToStr;
	int itmp, i,retvalue;
	/*SipStr *_str;*/
	SipParam *_para;
	char tmpbuf[512]={'\0'};

	ReferToStr=dxStrNew();
	dxStrCat(ReferToStr, "Refer-To:");
	AddrAsStr(_refer->address,(unsigned char*)tmpbuf,512,&itmp);
	dxStrCat(ReferToStr,tmpbuf);
	/*_str = _from->tagList;*/
	_para=_refer->paramList;
	for(i=0; (i<_refer->numParam)&&(_para != NULL); i++) {
		dxStrCat(ReferToStr, ";");
		dxStrCat(ReferToStr,_para->name);
		if(_para->value != NULL){
			dxStrCat(ReferToStr,"=");
			dxStrCat(ReferToStr,_para->value);
		}
		_para=_para->next;
	}
	dxStrCat(ReferToStr, "\r\n");
	*length=dxStrLen(ReferToStr);

	if((*length) >= buflen)
		retvalue= 0;
	else {
		strcpy(buf,(const char*) dxStrAsCStr(ReferToStr));
		retvalue= 1;
	}
	dxStrFree(ReferToStr);

	return retvalue;
}

int ReferToFree(SipReferTo *_refer)
{

	if(_refer != NULL){
		if(_refer->address != NULL){
			sipAddrFree(_refer->address);
			_refer->address=NULL;
		}
		if(_refer->paramList != NULL){
			sipParameterFree(_refer->paramList);
			_refer->paramList=NULL;
		}
		free(_refer);
	}
	
	return 1;
}

/*Referred-By */
int	ReferredByParse(MsgHdr ph, void **ret)
{
	SipReferredBy *_referredby;
	SipAddr *_addr;
	SipParam *_param, *tmpPara;
	unsigned char *tmp, *buf,*freeptr, *semicolon;
	int i = 0,count=0;

	*ret=NULL;
	/* chech input data */
	if(!ph->first || !ph->first->content)
		return 0;
	
	_referredby =(SipReferredBy*) calloc(1,sizeof (*_referredby));
	if (!_referredby) 
		return RC_ERROR;
	_referredby->numAddr=1;
	_referredby->numParam = 0;
	_referredby->paramList = NULL;

	buf = (unsigned char*)strDup((const char*)trimWS(ph->first->content));
	freeptr=buf;
	/* Parse addr */
	_addr = (SipAddr*)calloc(1,sizeof (*_addr));
	_referredby->address = _addr;

	while( buf[i]!='\0' ) {
		if( buf[i]=='<' ) {
			while( buf[i]!='>' && buf[i]!='\0' )	i++;
				continue;
		}
		else if( buf[i]==';' )
			break;
		else
			i++;
	}
	tmp = buf + i;

	/*tmp = strstr(buf, ";");*/
	
	if(( *tmp=='\0' )||(*(tmp+1)=='\0'))
		AddrParse(buf, _addr);
	else{
		/* have parameter, parse */
		*tmp = '\0';
		tmp++;
		AddrParse(buf, _addr);
		while (1){
			semicolon  =(unsigned char*)strChr(tmp, ';');

			_param = (SipParam*)calloc(1,sizeof *_param);
			if (semicolon == NULL){
				if(*tmp == '\0') 
					break;
				AnalyzeParameter(tmp,_param);
				if(count==0){
					_referredby->paramList = _param;
					tmpPara=_param;
				}else{
					tmpPara->next=_param;
					tmpPara=_param;
				}
				count++;
				break;
			}else{
				*semicolon = '\0';
				AnalyzeParameter(tmp,_param);
				if(count==0){
					_referredby->paramList = _param;
					tmpPara=_param;
				}else{
					tmpPara->next=_param;
					tmpPara=_param;
				}
				count++;

				tmp = semicolon + 1;
			}
		}
	}
	_referredby->numParam=count;

	/* add number of address to notify up-layer,more than one address */
	if (ph->first->next)
		_referredby->numAddr++;
	free(freeptr);
	*ret = (void *)_referredby;

	return 1;
}

int	ReferredByAsStr(SipReferredBy *_referredby, unsigned char *buf, int buflen, int *length)
{
	int itmp, i,retvalue;
	/*SipStr *_str;*/
	SipParam *_para;
	DxStr ReferredBy;
	char tmpbuf[512]={'\0'};

	ReferredBy=dxStrNew();
	dxStrCat(ReferredBy, "Referred-By:");
	AddrAsStr(_referredby->address,(unsigned char*)tmpbuf,512,&itmp);
	dxStrCat(ReferredBy,tmpbuf);
	/*_str = _from->tagList;*/
	_para=_referredby->paramList;
	for(i=0; (i<_referredby->numParam)&&(_para != NULL); i++) {
		dxStrCat(ReferredBy, ";");
		dxStrCat(ReferredBy,_para->name);
		if(_para->value != NULL){
			dxStrCat(ReferredBy,"=");
			dxStrCat(ReferredBy,_para->value);
		}
		_para=_para->next;
	}
	dxStrCat(ReferredBy, "\r\n");
	*length=dxStrLen(ReferredBy);

	if((*length) >= buflen)
		retvalue= 0;
	else {
		strcpy(buf,(const char*) dxStrAsCStr(ReferredBy));
		retvalue= 1;
	}
	dxStrFree(ReferredBy);
	return retvalue;
}
int	ReferredByFree(SipReferredBy *_referredby)
{
	
	if(_referredby != NULL){
		if(_referredby->address != NULL){
			sipAddrFree(_referredby->address);
			_referredby->address=NULL;
		}
		if(_referredby->paramList != NULL){
			sipParameterFree(_referredby->paramList);
			_referredby->paramList=NULL;
		}
		free(_referredby);
	}

	return 1;
}
int	RemotePartyIDParse(MsgHdr ph,void **ret)
{	
	SipRemotePartyID *_remotepartyid;
	SipAddr *_addr;
	SipParam *_param,*tmpPara;
	char *buf,*freeptr,*semicolon,*tmp;
	int i=0,count=0;

	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;

	_remotepartyid=(SipRemotePartyID*)calloc(1,sizeof(*_remotepartyid));
	_remotepartyid->addr=NULL;
	_remotepartyid->next=NULL;
	_remotepartyid->paramList=NULL;
	_remotepartyid->numParam=0;

	buf=(char*)strDup((const char*)trimWS(ph->first->content));
	freeptr=buf;
	/*parse addr */
	_addr=(SipAddr*)calloc(1,sizeof(*_addr));
	_addr->addr=NULL;
	_addr->display_name=NULL;
	_addr->next=NULL;
	_addr->with_angle_bracket=0;


	while(buf[i] != '\0'){
		if( buf[i]=='<' ) {
			while( buf[i]!='>' && buf[i]!='\0' )	i++;
				continue;
		}
		else if( buf[i]==';' )
			break;
		else
			i++;
	}
	tmp = buf + i;

	if(( *tmp=='\0' )||(*(tmp+1)=='\0')){
		AddrParse((unsigned char*)buf, _addr);
		_remotepartyid->addr=_addr;
	}else{
		/* have parameter, parse */
		*tmp = '\0';
		tmp++;
		AddrParse((unsigned char*)buf, _addr);
		_remotepartyid->addr=_addr;
		while (1){
			semicolon  =(char*)strChr((unsigned char*)tmp, ';');

			_param = (SipParam*)calloc(1,sizeof *_param);
			if (semicolon == NULL){
				if(*tmp == '\0') 
					break;
				AnalyzeParameter((unsigned char*)tmp,_param);
				if(count==0){
					_remotepartyid->paramList = _param;
					tmpPara=_param;
				}else{
					tmpPara->next=_param;
					tmpPara=_param;
				}
				count++;
				break;
			}else{
				*semicolon = '\0';
				AnalyzeParameter((unsigned char*)tmp,_param);
				if(count==0){
					_remotepartyid->paramList = _param;
					tmpPara=_param;
				}else{
					tmpPara->next=_param;
					tmpPara=_param;
				}
				count++;

				tmp = semicolon + 1;
			}
		}
	}

	_remotepartyid->numParam=count;
	free(freeptr);
	*ret=(void*)_remotepartyid;
	return 1;
}
int	RemotePartyIDAsStr(SipRemotePartyID *_remotepartyid,unsigned char *buf,int buflen, int *length)
{
	DxStr RemotePartyIDStr;
	SipRemotePartyID *next;
	SipParam *_para;
	int i,itmp,retvalue;
	char tmpbuf[512]={'\0'};

	RemotePartyIDStr=dxStrNew();
	*length=0;
	if(_remotepartyid != NULL){
		next=_remotepartyid;
		while(next!=NULL){
			dxStrCat(RemotePartyIDStr,"Remote-Party-ID:");
			if(next->addr!= NULL){
				AddrAsStr(next->addr,(unsigned char*)tmpbuf,512,&itmp);
				dxStrCat(RemotePartyIDStr,tmpbuf);
			}
			_para=next->paramList;
			for(i=0;(i<next->numParam)&&(_para != NULL);i++){
				dxStrCat(RemotePartyIDStr,";");
				dxStrCat(RemotePartyIDStr,_para->name);
				if(_para->value!=NULL){
					dxStrCat(RemotePartyIDStr,"=");
					dxStrCat(RemotePartyIDStr,_para->value);
				}
				_para=_para->next;
			}
			dxStrCat(RemotePartyIDStr,"\r\n");
			next=next->next;
		}
	}

	*length=dxStrLen(RemotePartyIDStr);
	if((*length) >= buflen)
		retvalue=0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(RemotePartyIDStr));
		retvalue =1;
	}
	dxStrFree(RemotePartyIDStr);
	return retvalue;
}

int	RemotePartyIDFree(SipRemotePartyID *_remotepartyid)
{
	SipRemotePartyID *next,*tmp;

	tmp=_remotepartyid;
	while(tmp !=NULL){
		if(tmp->addr!= NULL){
			sipAddrFree(tmp->addr);
			tmp->addr=NULL;
		}
		if(tmp->paramList!=NULL){
			sipParameterFree(tmp->paramList);
			tmp->paramList=NULL;
		}
		next=tmp->next;
		free(tmp);
		tmp=next;
	}
	return 1;
}

int ReplacesParse(MsgHdr ph, void **ret)
{
	SipReplaces *_replace,*tmpreplace;
	SipParam *_param,*firstpara,*preparam;
	MsgHdrElm nowElm;
	char *a,*b;
	int paracount=0;
	BOOL	bdoing=TRUE;
	unsigned char *buf;

	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;
	
	nowElm=ph->first;
	if (nowElm) {
		_replace=(SipReplaces*)calloc(1,sizeof(*_replace));
		_replace->callid=NULL;
		_replace->next=NULL;
		_replace->numParam=0;
		_replace->paramList=NULL;
		tmpreplace=_replace;
	}
	while(nowElm != NULL){
		if(nowElm->content != NULL)
			buf=(unsigned char*)strDup((const char*)trimWS(nowElm->content));
		else{
			free(_replace);
			break;
		}
		a=(char*)strstr(buf,";");
		if(a==0){/*only call id */
			_replace->callid=strDup((const char*)buf);
		}else{
			*a=0;
			_replace->callid=strDup((const char*)buf);
			while(bdoing){
				_param=(SipParam*)calloc(1,sizeof(*_param));
				if((b=(char*)strstr(a+1,";"))!=0){
					*b=0;
					bdoing=TRUE;
					AnalyzeParameter((unsigned char*)(a+1),_param);
					a=b+1;
				}else{
					AnalyzeParameter((unsigned char*)a,_param);
					bdoing=FALSE;
				}

				if(paracount ==0){
					_replace->paramList=_param;
					firstpara=_param;
				}else{
					preparam->next=_param;
				}
				preparam=_param;
				paracount++;

			}
			_replace->numParam=paracount;
		}
		free(buf);
		nowElm=nowElm->next;
		if (nowElm) {
			_replace->next=(SipReplaces*)calloc(1,sizeof(*_replace));
			if (_replace->next) 
				_replace=_replace->next;
			else
				break;
			_replace->callid=NULL;
			_replace->next=NULL;
			_replace->numParam=0;
			_replace->paramList=NULL;
		}
	}/*end while nowElm != NULL */
	*ret=(void*)tmpreplace;
	return 1;

}

int	ReplacesAsStr(SipReplaces* _replaces,unsigned char *buf, int buflen, int *length)
{
	DxStr ReplacesStr;
	SipReplaces *next;
	SipParam *_para;
	int i,retvalue;

	ReplacesStr=dxStrNew();
	*length=0;
	if(_replaces != NULL){
		next=_replaces;
		while(next != NULL){
			dxStrCat(ReplacesStr,"Replaces:");
			dxStrCat(ReplacesStr,next->callid);
			_para=next->paramList;
			for(i=0;i<next->numParam;i++){
				dxStrCat(ReplacesStr,";");
				dxStrCat(ReplacesStr,_para->name);
				if(_para->value != NULL){
					dxStrCat(ReplacesStr,"=");
					dxStrCat(ReplacesStr,_para->value);
				}
				_para=_para->next;
			}
			dxStrCat(ReplacesStr,"\r\n");
			next=next->next;
		}/*end while */
	}

	*length=dxStrLen(ReplacesStr);

	if((*length) >= buflen)
		retvalue =0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(ReplacesStr));
		retvalue =1;
	}
	dxStrFree(ReplacesStr);

	return retvalue;
}

int	ReplacesFree(SipReplaces *_replace)
{
	SipReplaces *tmp,*next;

	tmp=_replace;
	while(tmp!=NULL){
		if(tmp->callid!=NULL){
			free(tmp->callid);
			tmp->callid=NULL;
		}
		if(tmp->paramList!= NULL){
			sipParameterFree(tmp->paramList);
			tmp->paramList=NULL;
		}
		next=tmp->next;
		free(tmp);
		tmp=next;
	}
	return 1;
}

int ReplyToParse(MsgHdr ph, void **ret)
{
	SipReplyTo *_replyto;
	SipAddr *_addr;
/*	SipStr     *_str;*/
	SipParam *_param,*tmpPara;
	unsigned char *tmp, *buf,*freeptr, *semicolon;
	int i = 0,count=0;

	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;

	_replyto =(SipReplyTo*) calloc(1,sizeof (*_replyto));
	_replyto->numAddr=1;
	_replyto->numParam = 0;
	_replyto->ParamList = NULL;

	buf = (unsigned char*)strDup((const char*)trimWS(ph->first->content));
	freeptr=buf;
	/* Parse addr */
	_addr = (SipAddr*)calloc(1,sizeof (*_addr));
	_replyto->address = _addr;

	while( buf[i]!='\0' ) {
		if( buf[i]=='<' ) {
			while( buf[i]!='>' && buf[i]!='\0' )	i++;
				continue;
		}
		else if( buf[i]==';' )
			break;
		else
			i++;
	}
	tmp = buf + i;

	/*tmp = strstr(buf, ";");*/
	
	if(( *tmp=='\0' )||(*(tmp+1)=='\0'))
		AddrParse(buf, _addr);
	else{
		/* have parameter, parse */
		*tmp = '\0';
		tmp++;
		AddrParse(buf, _addr);
		while (1){
			semicolon  =(unsigned char*)strChr(tmp, ';');

			_param = (SipParam*)calloc(1,sizeof *_param);
			if (semicolon == NULL){
				if(*tmp == '\0'){
					free(_param);
					break;
				}
				AnalyzeParameter(tmp,_param);
				if(count==0){
					_replyto->ParamList = _param;
					tmpPara=_param;
				}else{
					tmpPara->next=_param;
					tmpPara=_param;
				}
				count++;
				break;
			}else{
				*semicolon = '\0';
				AnalyzeParameter(tmp,_param);
				if(count==0){
					_replyto->ParamList = _param;
					tmpPara=_param;
				}else{
					tmpPara->next=_param;
					tmpPara=_param;
				}
				count++;

				tmp = semicolon + 1;
			}
		}
	}
	_replyto->numParam=count;
	free(freeptr);
	*ret = (void *)_replyto;
	return 1;
} /* replytoParse*/

int ReplyToAsStr(SipReplyTo *_replyto, unsigned char *buf, int buflen, int *length)
{
	int itmp, i,retvalue;
	/*SipStr *_str;*/
	SipParam *_para;
	DxStr ReplyToStr;
	char tmpbuf[512]={'\0'};

	ReplyToStr=dxStrNew();
	dxStrCat(ReplyToStr, "Reply-To:");
	AddrAsStr(_replyto->address,(unsigned char*)tmpbuf,512,&itmp);
	dxStrCat(ReplyToStr,tmpbuf);
	/*_str = _from->tagList;*/
	_para=_replyto->ParamList;
	for(i=0; (i<_replyto->numParam)&&(_para != NULL); i++) {
		dxStrCat(ReplyToStr, ";");
		dxStrCat(ReplyToStr,_para->name);
		dxStrCat(ReplyToStr,"=");
		dxStrCat(ReplyToStr,_para->value);
		_para=_para->next;
	}
	dxStrCat(ReplyToStr, "\r\n");
	*length=dxStrLen(ReplyToStr);

	if((*length) >= buflen)
		retvalue= 0;
	else {
		strcpy(buf,(const char*) dxStrAsCStr(ReplyToStr));
		retvalue= 1;
	}
	dxStrFree(ReplyToStr);
	return retvalue;

} /* ReplayToAsStr */

int ReplyToFree(SipReplyTo *_replyto)
{
	if(_replyto != NULL){
		if(_replyto->address!=NULL){
			sipAddrFree(_replyto->address);
			_replyto->address=NULL;
		}
		if(_replyto->ParamList != NULL){
			sipParameterFree(_replyto->ParamList);
			_replyto->ParamList=NULL;
		}
		free(_replyto);
		_replyto=NULL;
	}
	return 1;
} /* ReplayToFree */

int RequireParse(MsgHdr ph, void **ret)
{
	SipRequire *_require;
	MsgHdrElm nowElm;
	SipStr *_str;

	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;

	_require = (SipRequire*)calloc(1,sizeof(*_require));
	_require->numTags = ph->size;
	nowElm = ph->first;
	_str = (SipStr*)calloc(1,sizeof(*_str));
	_require->sipOptionTagList = _str;
	_str->str = strDup((const char*)trimWS(nowElm->content));
	nowElm = nowElm->next;

	while(1){
		if (nowElm != NULL){
			_str->next = (SipStr*)calloc(1,sizeof(*_str));
			_str = _str->next;
			_str->str = strDup((const char*)trimWS(nowElm->content));
			nowElm = nowElm->next;
		}else
			break;
	}
	_str->next = NULL;
	*ret = (void *)_require;
	return 1;
} /* RequireParse */

int RequireAsStr(SipRequire *_require, unsigned char *buf, int buflen, int *length)
{
	SipStr *_str;
	int i,retvalue;
	DxStr RequireStr;

	RequireStr=dxStrNew();
	dxStrCat(RequireStr, "Require:");
	_str = _require->sipOptionTagList;
	for(i=0; i<_require->numTags; i++){
		dxStrCat(RequireStr, _str->str);
		if (_str->next != NULL)
			dxStrCat(RequireStr, ",");/*for comma*/
		_str = _str->next;
	}
	dxStrCat(RequireStr, "\r\n");
	*length=dxStrLen(RequireStr);

	if((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(RequireStr));
		retvalue= 1;
	}
	dxStrFree(RequireStr);
	return retvalue;
} /* RequireAsStr */

int RequireFree(SipRequire *_require)
{
	if(_require != NULL){
		if(_require->sipOptionTagList){
			sipStrFree(_require->sipOptionTagList);
			_require->sipOptionTagList=NULL;
		}
		free(_require);
		_require=NULL;
	}
	return 1;
} /* RequireFree */

int ResponseKeyParse(MsgHdr ph, void **ret)
{
	SipRspKey *_responsekey;
	unsigned char *buf,*freeptr, *ptmp;
	SipParam* _parameter;
	MsgHdrElm nowElm;

	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;

	_responsekey =(SipRspKey*) calloc(1,sizeof(*_responsekey));
	_responsekey->numParams = 0;
	nowElm = ph->first;
	buf = (unsigned char*)strDup((const char*)trimWS(nowElm->content));
	freeptr=buf;
	ptmp =(unsigned char*)strstr(buf, " ");

	if (ptmp == NULL)
		_responsekey->scheme =strDup((const char*)buf);
	else{
		*ptmp = '\0';
		_responsekey->scheme =strDup((const char*)trimWS((char*)buf));
		buf = ptmp;
		buf++;
    
		_parameter =(SipParam*) calloc(1,sizeof(*_parameter));
		_parameter->next = NULL;
		_responsekey->sipParamList = _parameter;

		while (1){
			ptmp =(unsigned char*) strChr(buf,'=');
			if (ptmp == NULL){
				/*return 0; modified by tyhuang 2003.7.24*/
				TCRPrint(1, "*** [sipStack] ResponseKeyParse Parse error in parameter part! \n");
				break;
			}else{
				*ptmp = '\0';
				_parameter->name =strDup((const char*)trimWS((char*)buf));
				ptmp++;
				_parameter->value = strDup((const char*)trimWS((char*)ptmp));
				_responsekey->numParams++;
			}
			if (nowElm == ph->last)
				break;
			else{
				nowElm = nowElm->next;
				/*buf = (unsigned char*)strDup((const char*)trimWS(nowElm->content));*/
				buf = (unsigned char*)trimWS(nowElm->content);
				_parameter->next =(SipParam*) calloc(1,sizeof(*_parameter));
				_parameter = _parameter->next;
				_parameter->next = NULL;
			}
		}
	}
	free(freeptr);
	*ret = (void *)_responsekey;
	return 1;
} /* ResponseKeyParse */

int ResponseKeyAsStr(SipRspKey *_responsekey, unsigned char *buf, int buflen, int *length)
{
	SipParam* _parameter;
	int i,retvalue;
	DxStr RspKeyStr;

	RspKeyStr=dxStrNew();
	dxStrCat(RspKeyStr, "Response-Key:");
	dxStrCat(RspKeyStr, _responsekey->scheme);
	dxStrCat(RspKeyStr, " ");
	_parameter = _responsekey->sipParamList;
	for(i=0; i<_responsekey->numParams; i++){
		dxStrCat(RspKeyStr, _parameter->name);
		dxStrCat(RspKeyStr, "=");
		dxStrCat(RspKeyStr, _parameter->value);
		if (_parameter->next != NULL)
			dxStrCat(RspKeyStr, ",");
		_parameter = _parameter->next;
	}
	dxStrCat(RspKeyStr, "\r\n");
	*length=dxStrLen(RspKeyStr);

	if((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(RspKeyStr));
		retvalue= 1;
	}
	dxStrFree(RspKeyStr);
	return retvalue;
} /* ResponseKeyAsStr */

int ResponseKeyFree(SipRspKey *_responsekey)
{
	if(_responsekey != NULL){
		if(_responsekey->scheme){
			free(_responsekey->scheme);
			_responsekey->scheme=NULL;
		}
		if(_responsekey->sipParamList){
			sipParameterFree(_responsekey->sipParamList);
			_responsekey->sipParamList=NULL;
		}
		free(_responsekey);
		_responsekey=NULL;
	}
	return 1;
} /* ResponseKeyFree */

int RetryAfterParse(MsgHdr ph, void **ret)
{
	SipRtyAft *_retryafter;
	char *buf, *tmp, *tmptmp, *pbuf;
	MsgHdrElm nowElm;
	SipRfc1123Date *_date;
	int count;

	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;
	
	_retryafter =(SipRtyAft*) calloc(1,sizeof(*_retryafter));
	_retryafter->afterSecond = -1;
	_retryafter->afterDate = NULL;
	_retryafter->comment = NULL;
	_retryafter->duration = -1;
	/* count allocate size*/
	nowElm = ph->first;
	count = strlen(nowElm->content);
	nowElm = nowElm->next;
	while(nowElm != NULL){
		count++;
		count = count + strlen(nowElm->content);
		nowElm = nowElm->next;
	}

	/*buf =(char*) malloc(count + 1);*/ /* NULL  */
	buf =(char*) calloc(count + 1,sizeof(char));
	pbuf = buf;
	nowElm = ph->first;
	strcpy(buf, nowElm->content);
	nowElm = nowElm->next;
	while(nowElm != NULL){
		strcat(buf, ",");
		strcat(buf, nowElm->content);
		nowElm = nowElm->next;
	}

	buf = trimWS(buf);
	/* parse duration */
	tmp=strchr((const char*)buf,';');
	if(tmp!=NULL){
		*tmp='\0';
		tmp=strchr((const char*)trimWS(tmp+1),'=');
		_retryafter->duration = (tmp != NULL) ? atoi((const char*)trimWS(tmp+1)):-1;
	}
	/* parse comment */
	tmp=strchr((const char*)buf,'(');
	if(tmp!=NULL) {
		tmptmp=strchr((const char*)tmp,')');
		if(tmptmp!=NULL) 
			*tmptmp='\0';
		_retryafter->comment=(tmptmp!=NULL) ? strDup((const char*)(tmp+1)):NULL;
		*tmp='\0';
	}
	/* parse date */
	tmp=strchr((const char*)buf, ',');
	if(tmp==NULL)
		_retryafter->afterSecond=atoi(buf);
	else {
		_date =(SipRfc1123Date*) calloc(1,sizeof (*_date));
		if (!RFCDateParse((unsigned char*)buf, _date)){
			/* modified by tyhuang 2003.7.24 */
			if(buf) free(buf);
			if(_retryafter) RetryAfterFree(_retryafter);
			TCRPrint(1, "*** [sipStack] RetryAfter Parse error in RFCDATE parse! \n");
			return 0;
		}
		_retryafter->afterDate = _date;
	}
	free(pbuf);
	*ret = (void *)_retryafter;
	return 1;
} /* RetryAfterParse */


int RetryAfterAsStr(SipRtyAft *_retryafter, unsigned char *buf, int buflen, int *length)
{
	unsigned char tmp[12], tmp2[12];
	int itmp,retvalue;
	DxStr RetryAfStr;
	char *tmpstr;
	char tmpbuf[128]={'\0'};

	RetryAfStr=dxStrNew();
	dxStrCat(RetryAfStr, "Retry-After:");
	if (_retryafter->afterSecond > -1){
		tmpstr=i2a(_retryafter->afterSecond,(char*)tmp,12);
		dxStrCat(RetryAfStr,(char*) tmp);
	}else{
		RFCDateAsStr(_retryafter->afterDate, (unsigned char*)tmpbuf,128, &itmp);
		dxStrCat(RetryAfStr,tmpbuf);
	}

	if (_retryafter->comment != NULL){
		dxStrCat(RetryAfStr, "(");
		dxStrCat(RetryAfStr, _retryafter->comment);
		dxStrCat(RetryAfStr, ")");
	}

	if(_retryafter->duration > -1) {
		tmpstr=i2a(_retryafter->duration,(char*)tmp2,12);
		dxStrCat(RetryAfStr, ";duration=");
		dxStrCat(RetryAfStr,(char*) tmp2);
	}
	dxStrCat(RetryAfStr, "\r\n");
	*length=dxStrLen(RetryAfStr);

	if((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(RetryAfStr));
		retvalue= 1;
	}
	dxStrFree(RetryAfStr);
	return retvalue;
} /* RetryAfterAsStr */

int RetryAfterFree(SipRtyAft *_retryafter)
{
	if(_retryafter != NULL)	{
		if (_retryafter->afterDate != NULL){    
			free(_retryafter->afterDate);
			_retryafter->afterDate=NULL;
		}
		if (_retryafter->comment != NULL){      
			free(_retryafter->comment);
			_retryafter->comment=NULL;
		}
		free(_retryafter);
		_retryafter=NULL;
	}
	return 1;
} /* RetryAfterFree */

int RouteParse(MsgHdr ph, void **ret)
{
	SipRoute *_route;
	SipRecAddr  *_addr;
	SipAddr *_addrtmp;
	MsgHdrElm nowElm;
	unsigned char *buf,*freeptr;
	char *semicolon,*tmp;
	int i,retvalue=1,count=0;
	SipParam *_param,*tmpPara;

	*ret=NULL;
	/* check input data */
	if(!ph->first || !ph->first->content)
		return 0;
	
	_route =(SipRoute*)calloc(1,sizeof(*_route));
	if(_route){
		_route->numNameAddrs = ph->size;
		_route->sipNameAddrList = NULL;
	}else
		return RC_ERROR;

	_addr = (SipRecAddr*)calloc(1,sizeof(*_addr));
	if(_addr){
		_addr->addr=NULL;
		_addr->display_name=NULL;
		_addr->sipParamList=NULL;
		_addr->next=NULL;
		_addr->numParams=0;
		_route->sipNameAddrList = _addr;
	}else{
		free(_route);
		return RC_ERROR;
	}
	
	nowElm = ph->first;
	for(i=0; i<ph->size; i++){
		buf = (unsigned char*)strDup((const char*)trimWS(nowElm->content));
		freeptr=buf;
		count=0;
		/* check if parameter exist */
		tmp =strChr(buf,'>');
		if(tmp)
			tmp=strChr((unsigned char*)tmp, ';');
		else
			tmp=strChr(buf, ';');
		if(tmp){
			*tmp++='\0';
			while (1){
				semicolon  =strChr((unsigned char*)tmp, ';');
				_param = (SipParam*)calloc(1,sizeof *_param);
				if (semicolon == NULL){
					if(*tmp == '\0') 
						break;
					AnalyzeParameter((unsigned char*)tmp,_param);
					if(count==0)
						_addr->sipParamList = _param;
					else
						tmpPara->next=_param;
					count++;
					break;
				}else{
					*semicolon = '\0';
					AnalyzeParameter((unsigned char*)tmp,_param);
					if(count==0){
						_addr->sipParamList = _param;
						tmpPara=_param;
					}else{
						tmpPara->next=_param;
						tmpPara=_param;
					}
					count++;
					tmp = semicolon + 1;
				}
			}
		}
		_addr->numParams=count;
		_addrtmp=(SipAddr*)calloc(1,sizeof(*_addrtmp));;
		if (AddrParse(buf, _addrtmp)){
			if(_addrtmp->addr)
				_addr->addr=strDup((const char*)_addrtmp->addr);
			else
				_addr->addr=NULL;
			if(_addr->display_name)
				_addr->display_name=_addrtmp->display_name;
			else
				_addr->display_name=NULL;

			sipAddrFree(_addrtmp);
			if ((nowElm != ph->last) && nowElm->next){
				nowElm = nowElm->next;
				_addr->next = (SipRecAddr*)calloc(1,sizeof(*_addr));
				_addr = _addr->next;
				_addr->addr=NULL;
				_addr->display_name=NULL;
				_addr->sipParamList=NULL;
				_addr->next=NULL;
				_addr->numParams=0;
			}else{
				if(freeptr!=NULL)
					free(freeptr);
				break;
			}
		}else
			retvalue= 0;

		if(freeptr!=NULL)
			free(freeptr);
	}
	*ret = (void *)_route;
	return retvalue;
} /* RouteParse */

int RouteAsStr(SipRoute *_route, unsigned char *buf, int buflen, int *length)
{
	int i,j,retvalue;
	SipRecAddr *_addr;
	SipParam *_param;
	DxStr RouteStr;

	RouteStr=dxStrNew();
	dxStrCat(RouteStr, "Route:");
	_addr = _route->sipNameAddrList;
	for(i=0; i<_route->numNameAddrs; i++){
		if(_addr->display_name){
			dxStrCat(RouteStr,"\"");
			dxStrCat(RouteStr,_addr->display_name);
			dxStrCat(RouteStr,"\"");
		}
		if(_addr->addr){
			dxStrCat(RouteStr,"<");
			dxStrCat(RouteStr,_addr->addr);
			dxStrCat(RouteStr,">");
		}
		
		_param=_addr->sipParamList;
		for(j=0;j<_addr->numParams;j++){
			if(_param){
				dxStrCat(RouteStr,";");
				dxStrCat(RouteStr,_param->name);
				if(_param->value){
					dxStrCat(RouteStr,"=");
					dxStrCat(RouteStr,_param->value);
				}
			}
		}
		if (_addr->next != NULL){
				dxStrCat(RouteStr, ",");
			}
		_addr = _addr->next;
		if(_addr==NULL) break;
	}
	dxStrCat(RouteStr, "\r\n");
	*length=dxStrLen(RouteStr);

	if((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(RouteStr));
		retvalue= 1;
	}
	dxStrFree(RouteStr);
	return retvalue;
} /* RouteAsStr */

int RouteFree(SipRoute *_route)
{
	if(_route != NULL){
		if(_route->sipNameAddrList != NULL){
			sipRecAddrFree(_route->sipNameAddrList);
			_route->sipNameAddrList=NULL;
		}
		free(_route);
		_route=NULL;
	}
	return 1;
} /* RouteFree */

/* RSeq */
int	RSeqParse(MsgHdr ph, void **ret)
{
	/*static int rseq=0;*/
	int rseq=0,*ct;
	
	*ret=NULL;
	if(ph)
		if(ph->first)
			if(ph->first->content)
				rseq = (unsigned int)atoi(ph->first->content);
	ct=(int*)calloc(1,sizeof(int));
	*ct=rseq;
	*ret = (void*)ct;
	return 1;
}
int	RSeqAsStr(int value, unsigned char *buf, int buflen, int *length)
{
	char tmp[16]={'\0'};
	char *tmpstr;
	DxStr RseqStr;
	int retvalue;

	RseqStr=dxStrNew();
	memset(tmp, ' ', 16);
	tmpstr=i2a(value,(char*)tmp,15);
	dxStrCat(RseqStr,"RSeq:");
	dxStrCat(RseqStr,tmp);
	dxStrCat(RseqStr,"\r\n");
	*length=dxStrLen(RseqStr);
	
	if((*length) >= buflen)
		retvalue= 0;
	else {
		strcpy(buf,(const char*)dxStrAsCStr(RseqStr));
		retvalue= 1;
	}
	dxStrFree(RseqStr);
	return retvalue;
}

int ServerParse(MsgHdr ph, void **ret)
{
	SipServer *_server;
	SipStr *_str;
	unsigned char *buf,*freeptr, *space, *bracket;
	unsigned int tail;
	MsgHdrElm nowElm;
	int count=0;

	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;
	
	_server = (SipServer*)calloc(1,sizeof(*_server));
	buf = (unsigned char*)strDup((const char*)ph->first->content);
	freeptr=buf;
	_str =(SipStr*) calloc(1,sizeof(*_str));
	_server->data = _str;
	nowElm = ph->first;
	if (strChr(buf, '(') != NULL){
		CollectCommentData(&buf, (unsigned char*)strChr(buf, '('), &nowElm);
		freeptr=buf;
	}

	buf = (unsigned char*)trimWS((char*)buf);
	while(1){
		bracket = (unsigned char*)strChr(buf, '(');
		space = (unsigned char*)strstr(buf, " ");
		if ((bracket == NULL) && (space == NULL)) {
			count++;
			_str->str = strDup((const char*)buf);
			_str->next = NULL;
			break;
		}
		if ((bracket == NULL) && (space != NULL)) {
			count++;
			*space = '\0';
			_str->str = strDup((const char*)buf);
			_str->next =(SipStr*) calloc(1,sizeof(*_str));
			_str = _str->next;
			buf = space+1;
		}
		if ((bracket != NULL) && (space == NULL)) {
			count++;
			_str->str = strDup((const char*)buf);
			_str->next = NULL;
			break;
		}
		if ((bracket != NULL) && (space != NULL)){
			if (space<bracket){
				count++;
				*space = '\0';
				_str->str = strDup((const char*)buf);
				_str->next =(SipStr*) calloc(1,sizeof(*_str));
				_str = _str->next;
				buf = space+1;
			}else{
				count++;
				_str->str = strDup((const char*)buf);
				ExtractComment(bracket+1,(int*) &tail);
				*(buf+tail) = ')';
				*(buf+tail+1) = '\0';
				_str->next =(SipStr*) calloc(1,sizeof(*_str));
				_str = _str->next;
				buf = buf+tail+2;
			}
		}
	}/*end while loop*/
	_server->numServer=count;
	*ret = (void *)_server;
	free(freeptr);
	return 1;
} /* ServerParse */

int ServerAsStr(SipServer *_server, unsigned char *buf, int buflen, int *length)
{
	SipStr *_str;
	DxStr ServerStr;
	int retvalue;

	ServerStr=dxStrNew();
	dxStrCat(ServerStr, "Server:");
	_str = _server->data;
	while (1){
		dxStrCat(ServerStr, _str->str);
		if (_str->next != NULL)
			dxStrCat(ServerStr, " ");/*for space*/
		else
			break;
		_str = _str->next;
	}
	dxStrCat(ServerStr,"\r\n");
	*length=dxStrLen(ServerStr);

	if((*length) >= buflen) 
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(ServerStr));
		retvalue= 1;
	}
	dxStrFree(ServerStr);
	return retvalue;
} /* ServerAsStr */

int ServerFree(SipServer *_server)
{
	if(_server != NULL){
		if(_server->data){
			sipStrFree(_server->data);
			_server->data=NULL;
		}
		free(_server);
		_server=NULL;
	}
	return 1;
} /* ServerFree */

int SessionExpiresParse(MsgHdr ph, void **ret)
{
	int paracount=0;
	SipParam *_para,*firstpara,*preparam;
	unsigned char *buf,*a,*b;
	BOOL bdoing=TRUE;
	MsgHdrElm nowElm;
	SipSessionExpires *sessionexpires;

	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;

	nowElm = ph->first;
	while(nowElm != NULL){
		sessionexpires=(SipSessionExpires*)calloc(1,sizeof(*sessionexpires));
		sessionexpires->deltasecond=-1;
		sessionexpires->numParam=0;
		sessionexpires->paramList=NULL;

		if(nowElm->content != NULL)
			buf=(unsigned char*)strDup((const char*)trimWS(nowElm->content));
		else{
			free(sessionexpires);
			break;
		}

		a=(unsigned char*)strChr(buf,';');
		if(a==0){ /* only delta-second */
			sessionexpires->deltasecond=atoi((const char*)buf);
		}else{ /*with parameter */
			*a=0;
			sessionexpires->deltasecond=atoi((const char*)buf);
			a++;
			while(bdoing){
				_para=(SipParam*)calloc(1,sizeof(*_para));
				_para->name=NULL;
				_para->value=NULL;
				_para->next=NULL;
				if((b=(unsigned char*)strChr(a,';'))!=0){
					*b=0;
					bdoing=TRUE;
					if(AnalyzeParameter(a,_para) == -1){
						free(_para);
						break;
					}
					a=b+1;
				}else{
					if(AnalyzeParameter(a,_para)==-1){
						free(_para);
						break;
					}
					bdoing=FALSE;
				}
				if(paracount ==0){
					sessionexpires->paramList=_para;
					firstpara=_para;
				}else{
					preparam->next=_para;
				}
				preparam=_para;
				paracount++;

			}
			sessionexpires->numParam=paracount;
		}
		nowElm=nowElm->next;
		free(buf);
	}

	*ret = (void*)sessionexpires;
	return 1;
}

int SessionExpiresAsStr(SipSessionExpires *sessionexpires,unsigned char *buf,int buflen,int *length)
{
	DxStr SessionStr;
	char buff[64]={'\0'};
	SipParam *_para;
	int retvalue;
	int i;

	SessionStr=dxStrNew();
	if(sessionexpires != NULL){
		dxStrCat(SessionStr,"Session-Expires:");
		sprintf(buff,"%d",sessionexpires->deltasecond);
		dxStrCat(SessionStr,buff);
		_para=sessionexpires->paramList;
		for(i=0;(i<sessionexpires->numParam)&&(_para != NULL);i++){
			dxStrCat(SessionStr,";");
			dxStrCat(SessionStr,_para->name);
			if(_para->value != NULL){
				dxStrCat(SessionStr,"=");
				dxStrCat(SessionStr,_para->value);
			}
			_para=_para->next;
		}
	}
	dxStrCat(SessionStr,"\r\n");
	*length=dxStrLen(SessionStr);
	if((*length) >= buflen)
		retvalue= 0;
	else {
		strcpy(buf,(const char*) dxStrAsCStr(SessionStr));
		retvalue= 1;
	}
	dxStrFree(SessionStr);
	return retvalue;

	return 1;

}

int SessionExpiresFree(SipSessionExpires* _sessionexpires)
{
	if(_sessionexpires != NULL){
		if(_sessionexpires->paramList != NULL){
			sipParameterFree(_sessionexpires->paramList);
			_sessionexpires->paramList=NULL;
		}
		free(_sessionexpires);
		_sessionexpires=NULL;
	}
	return 1;
}

/* SIP-ETag */
int SIPETagParse(MsgHdr ph, void **ret)
{
	unsigned char *s;
	
	*ret=NULL;
	/* check input data */
	if(!ph->first || !ph->first->content)
		return 0;
	s = (unsigned char*)strDup((const char*)trimWS((char*)ph->first->content));
	*ret = (void *)s;
	return 1;
} /* SIPETagParse */

int SIPETagAsStr(unsigned char *s, unsigned char *buf, int buflen, int *length)
{
	DxStr Str;
	int retvalue;

	if(!s || !buf || !length )
		return 0;

	Str=dxStrNew();
	dxStrCat(Str, "SIP-ETag:");
	dxStrCat(Str,(const char*)s);
	dxStrCat(Str, "\r\n");

	*length=dxStrLen(Str);

	if ((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(Str));
		retvalue= 1;
	}
	dxStrFree(Str);
	return retvalue;
} /* SIPETagAsStr */

int SIPETagFree(unsigned char *s)
{
	if(s != NULL)
		free(s);	
	return 1;
} /* SIPETagFree */

/* SIP-If-Match */
int	SIPIfMatchParse(MsgHdr ph, void **ret)
{
	unsigned char *s;
	
	*ret=NULL;
	/* check input data */
	if(!ph->first || !ph->first->content)
		return 0;
	s = (unsigned char*)strDup((const char*)trimWS((char*)ph->first->content));
	*ret = (void *)s;
	return 1;
}/* SIPIfMatchParse */

int	SIPIfMatchAsStr(unsigned char *s, unsigned char *buf, int buflen, int *length)
{
	DxStr Str;
	int retvalue;

	Str=dxStrNew();
	if( s != NULL){
		dxStrCat(Str, "SIP-If-Match:");
		dxStrCat(Str,(const char*)s);
		dxStrCat(Str, "\r\n");
	}
	*length=dxStrLen(Str);

	if ((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(Str));
		retvalue= 1;
	}
	dxStrFree(Str);
	return retvalue;
}/* SIPIfMatchAsStr */

int	SIPIfMatchFree(unsigned char *s)
{
	if(s != NULL)
		free(s);	
	return 1;
}/* SIPIfMatchFree */


int SubjectParse(MsgHdr ph, void **ret)
{
	unsigned char *buf;
	int count=0;
	MsgHdrElm nowElm;

	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;
	
	nowElm = ph->first;
	while (1){
		count = count+strlen(nowElm->content);
		if (nowElm->next != NULL){
			count++;
			nowElm = nowElm->next;
		}else
			break;
	}

	/*buf =(unsigned char*) malloc(count+1);*//*NULL*/
	buf =(unsigned char*) calloc(count+1,sizeof(char));
	nowElm = ph->first;
	strcpy(buf, trimWS(nowElm->content));
	nowElm = nowElm->next;
	while (1){
		if (nowElm != NULL){
			strcat(buf, ",");
			strcat(buf, nowElm->content);
		}else
			break;
		nowElm = nowElm->next;
	}
	*ret = (void *)buf;
	return 1;
} /* SubjectParse */

int SubjectAsStr(unsigned char *subject, unsigned char *buf, int buflen, int *length)
{
	DxStr SubjectStr;
	int retvalue;

	SubjectStr=dxStrNew();
	dxStrCat(SubjectStr, "Subject:");
	dxStrCat(SubjectStr,(char*) subject);
	dxStrCat(SubjectStr, "\r\n");
	*length=dxStrLen(SubjectStr);

	if((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(SubjectStr));
		retvalue= 1;
	}
	dxStrFree(SubjectStr);
	return retvalue;
} /* SubjectAsStr */

int SubjectFree(unsigned char *_subject)
{
	if(_subject != NULL){
		free(_subject);
		_subject=NULL;
	}
	return 1;
} /* SubjectFree */

int SubscriptionStateParse(MsgHdr ph, void **ret)
{
	SipSubState	*_substate;
	unsigned char	*buf,*parms,*tmp;
	MsgHdrElm	nowElm;
	int		count=0;
	SipParam	*_parameter;

	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;
	
	nowElm = ph->first;
	_parameter=NULL;
	if(nowElm != NULL){
		_substate=(SipSubState*) calloc(1,sizeof(*_substate));
		_substate->expires=-1;
		_substate->retryatfer=-1;
		_substate->state=NULL;
		_substate->reason=NULL;
		_substate->paramList=NULL;
		_substate->numParam=0;
	}else
		return 0;
	if(nowElm->content != NULL)
		buf=(unsigned char*)strDup((const char*)trimWS(nowElm->content));
	else{
		free(_substate);
		TCRPrint(1, "*** [sipStack] SubscriptionState Parse error! \n");
		return 0;
	}
	parms=(unsigned char*)strChr(buf,';');
	if(parms!=NULL) {
	/* with parameter(s) */
		*parms='\0';
		parms++;
		/* parse parameters  */			
		while(1){
			tmp = (unsigned char*)strChr(parms, ';');
			count++;
			if(_parameter==NULL){
				_parameter =(SipParam*) calloc(1,sizeof(*_parameter));
				_parameter->name=NULL;
				_parameter->next=NULL;
				_parameter->value=NULL;
			}
			if(count==1)
				_substate->paramList=_parameter;
			if (tmp == NULL){ /* only one or last parameter */
				AnalyzeParameter(parms,_parameter);
				if(strICmp(_parameter->name,"reason")==0)
					_substate->reason=strDup(_parameter->value);
				if(strICmp(_parameter->name,"expires")==0)
					_substate->expires=atoi((const char*)_parameter->value);
				if(strICmp(_parameter->name,"retry-after")==0)
					_substate->retryatfer=atoi((const char*)_parameter->value);
			}
			else{
				*tmp = '\0';
			
				AnalyzeParameter(parms,_parameter);
				parms=tmp+1;
				
				if(strICmp(_parameter->name,"reason")==0)
					_substate->reason=strDup(_parameter->value);
				if(strICmp(_parameter->name,"expires")==0)
					_substate->expires=atoi((const char*)_parameter->value);
				if(strICmp(_parameter->name,"retry-after")==0)
					_substate->retryatfer=atoi((const char*)_parameter->value);
				_parameter->next=(SipParam*) calloc(1,sizeof(*_parameter));
				_parameter=_parameter->next;
				_parameter->name=NULL;
				_parameter->value=NULL;
				_parameter->next=NULL;
			}
			
			if(tmp==NULL)
				break;

		}/*end of while */
		_substate->numParam=count;
	}
	_substate->state=(char *)strDup((const char*)buf);
	if(buf != NULL)	
		free(buf);
	*ret=(void *)_substate;
	return 1;
}/* SubscriptionStateParse */

int SubscriptionStateAsStr(SipSubState *_substate, unsigned char *buf, int buflen, int *length)
{	
	DxStr SubstateStr;
	int retvalue;
	SipParam *_parameter;
	
	SubstateStr=dxStrNew();
	dxStrCat(SubstateStr, "Subscription-State:");
	dxStrCat(SubstateStr,_substate->state);
	_parameter=_substate->paramList;
	while(_parameter != NULL){
		if(_parameter->name!=NULL){
			dxStrCat(SubstateStr,";");
			dxStrCat(SubstateStr,_parameter->name);
		}
		if(_parameter->value!=NULL){
			dxStrCat(SubstateStr,"=");
			dxStrCat(SubstateStr,_parameter->value);
		}
		_parameter=_parameter->next;
	}
	dxStrCat(SubstateStr, "\r\n");
	*length=dxStrLen(SubstateStr);

	if((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(SubstateStr));
		retvalue= 1;
	}
	dxStrFree(SubstateStr);
	return retvalue;
}/*  SubscriptionStateAsStr*/

int SubscriptionStateFree(SipSubState *substate){
	
	if(substate != NULL){
		if(substate->state != NULL){
			free(substate->state);
			substate->state=NULL;
		}
		if(substate->reason != NULL){
			free(substate->reason);
			substate->reason=NULL;
		}
		if(substate->paramList != NULL){
			sipParameterFree(substate->paramList);
			substate->paramList=NULL;
		}
		free(substate);
		substate=NULL;
	}
	return 1;
}

int SupportedParse(MsgHdr ph, void **ret)
{
	SipSupported *_Supported;
	MsgHdrElm nowElm;
	SipStr *_str;

	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;
	
	nowElm = ph->first;
	if(nowElm){
		_Supported =(SipSupported*)calloc(1,sizeof(*_Supported));
		_Supported->numTags = ph->size;
		_str =(SipStr*) calloc(1,sizeof(*_str));
		_str->str=NULL;
		_Supported->sipOptionTagList = _str;
	}
	if(nowElm->content)
		_str->str = strDup((const char*)trimWS((char*)nowElm->content));
	else
		_str->str = NULL;

	nowElm = nowElm->next;
	while(1){
		if (nowElm != NULL){
			_str->next = (SipStr*)calloc(1,sizeof(*_str));
			_str = _str->next;
			_str->str = strDup((const char*)trimWS((char*)nowElm->content));
			nowElm = nowElm->next;
		}else
			break;
	}
	_str->next = NULL;
	*ret = (void *)_Supported;
	return 1;
} /* SupportedParse */

int SupportedAsStr(SipSupported *_Supported, unsigned char *buf, int buflen, int *length)
{
	SipStr *_str;
	int i,retvalue;
	DxStr SupportedStr;

	SupportedStr=dxStrNew();
	dxStrCat(SupportedStr, "Supported:");
	_str = _Supported->sipOptionTagList;
	for(i=0; i<_Supported->numTags; i++){
		dxStrCat(SupportedStr, _str->str);
		if (_str->next != NULL)
			dxStrCat(SupportedStr,",");/*for comma*/
		_str = _str->next;
	}
	dxStrCat(SupportedStr, "\r\n");
	*length=dxStrLen(SupportedStr);

	if((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(SupportedStr));
		retvalue= 1;
	}
	dxStrFree(SupportedStr);
	return retvalue;
} /* SupportedAsStr */

int SupportedFree(SipSupported *_Supported)
{
	SipStr *_str, *tmp_str;

	if(_Supported != NULL){
		_str = _Supported->sipOptionTagList;
		while (1){
			if (_str != NULL){
				tmp_str = _str->next;
				if(_str->str != NULL){
					free(_str->str);
					_str->str=NULL;
				}
				free(_str);
				_str = tmp_str;
			}else
				break;
		}
		free(_Supported);
		_Supported=NULL;
	}
	return 1;
} /* SupportedFree */

int TimestampParse(MsgHdr ph, void **ret)
{
	SipTimestamp *_timestamp;
	unsigned char *buf,*freeptr, *tmp;

	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;
	
	_timestamp =(SipTimestamp*) calloc(1,sizeof(*_timestamp));
	_timestamp->time = NULL;
	_timestamp->delay = NULL;
	buf = (unsigned char*)strDup((const char*)trimWS(ph->first->content));
	freeptr=buf;
	tmp = (unsigned char*)strstr(buf, " ");
	if (tmp != NULL)
		if ((*(tmp-1) == '.') || (*(tmp+1) == '.')){
			if(freeptr) free(freeptr);
			if(_timestamp) free(_timestamp);
			TCRPrint(1, "*** [sipStack] Timestamp Parse error! \n");
			return 0;
		}else{
			*tmp = '\0';
			_timestamp->time =strDup((const char*)buf);
			tmp++;
			_timestamp->delay = strDup((const char*)trimWS((char*)tmp));
		}
	else
		_timestamp->time = strDup((const char*)buf);
	*ret = (void *)_timestamp;
	free(freeptr);
	return 1;
} /* TimestampParse */

int TimestampAsStr(SipTimestamp *_timestamp, unsigned char *buf, int buflen, int *length)
{
	DxStr TimestampStr;
	int retvalue;

	TimestampStr=dxStrNew();
	dxStrCat(TimestampStr, "Timestamp:");
	dxStrCat(TimestampStr, _timestamp->time);
	if (_timestamp->delay != NULL){
		dxStrCat(TimestampStr, " ");
		dxStrCat(TimestampStr, _timestamp->delay);
	}
	dxStrCat(TimestampStr, "\r\n");
	*length=dxStrLen(TimestampStr);

	if((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(TimestampStr));
		retvalue= 1;
	}
	dxStrFree(TimestampStr);
	return retvalue;
} /* TimestampAsStr */

int TimestampFree(SipTimestamp *_timestamp)
{
	if(_timestamp != NULL){
		if(_timestamp->time != NULL){ 
			free(_timestamp->time);
			_timestamp->time=NULL;
		}
		if(_timestamp->delay != NULL){
			free(_timestamp->delay);
			_timestamp->delay=NULL;
		}
		free(_timestamp);
		_timestamp=NULL;
	}
	return 1;
} /* TimestampFree */

int ToParse(MsgHdr ph, void **ret)
{
	SipTo *_to;
	SipAddr *_addr;
	SipParam *_param,*tmpPara;
	unsigned char *tmp, *buf,*freeptr, *semicolon=NULL;
	int count=0;

	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;

	_to = (SipTo*)calloc(1,sizeof (*_to));
	_to->numAddr=1;
	_to->numParam = 0;
	_to->paramList = NULL;

	buf = (unsigned char*)strDup((const char*)trimWS((char*)ph->first->content));
	freeptr=buf;
	/* remove headers-> find '?'*/
	tmp=(unsigned char*)strChr(buf,'?');
	if(tmp) {
		*tmp++='>';
		*tmp='\0';
	}else
		tmp=(unsigned char*)strChr(buf,'>');

	/* Parse addr */
	_addr =(SipAddr*) calloc(1,sizeof (*_addr));
	_to->address = _addr;

	/*
	while( buf[i]!='\0' ) {
		if( buf[i]=='<' ) {
			while( buf[i]!='>' && buf[i]!='\0' )	i++;
				continue;
		}else if( buf[i]==';' )
			break;
		else
			i++;
	}
	tmp = buf + i;
	*/

	if(tmp != NULL)
		tmp = (unsigned char*)strChr (tmp+1, ';');
	else
		tmp = (unsigned char*)strChr(buf , ';');

	if((tmp ==NULL)||( *tmp=='\0' )||(*(tmp+1) == '\0'))
		AddrParse(buf, _addr);
	else{
		/* have parameter, parse */
		*tmp = '\0';
		tmp++;
		AddrParse(buf, _addr);
		while (1){
			semicolon  =(unsigned char*) strChr(tmp, ';');
			_param = (SipParam*)calloc(1,sizeof *_param);
			if (semicolon == NULL){
				if(*tmp == '\0')
					break;
				AnalyzeParameter(tmp,_param);
				if(count==0){
					_to->paramList=_param;
					tmpPara=_param;
				}else{
					tmpPara->next=_param;
					tmpPara=_param;
				}
				count++;
				break;
			}else{
				*semicolon = '\0';
				AnalyzeParameter(tmp,_param);
				if(count==0){
					_to->paramList = _param;
					tmpPara=_param;
				}else{
					tmpPara->next=_param;
					tmpPara=_param;
				}
				count++;
				tmp=semicolon+1;
			}
		}
	}
	_to->numParam=count;

	*ret = (void *)_to;
	free(freeptr);
	return 1;
} /* ToParse*/

int ToAsStr(SipTo *_to, unsigned char *buf, int buflen, int *length)
{
	int itmp, i,retvalue;
	/*SipStr *_str;*/
	SipParam *_param;
	DxStr ToStr;
	char tmpbuf[512]={'\0'};

	ToStr=dxStrNew();
	dxStrCat(ToStr, "To:");
	AddrAsStr(_to->address,(unsigned char*) tmpbuf, 512, &itmp);
	dxStrCat(ToStr,tmpbuf);
	_param = _to->paramList;
	for(i=0; (i<_to->numParam) && (_param != NULL); i++){
		dxStrCat(ToStr, ";");
		dxStrCat(ToStr,_param->name);
		dxStrCat(ToStr, "=");
		dxStrCat(ToStr,_param->value);
		_param = _param->next;
	}
	dxStrCat(ToStr, "\r\n");
	*length=dxStrLen(ToStr);

	if((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(ToStr));
		retvalue= 1;
	}
	dxStrFree(ToStr);
	return retvalue;
} /* ToAsStr */

int ToFree(SipTo *_to)
{
	if(_to != NULL){
		if(_to->address){
			sipAddrFree(_to->address);
			_to->address=NULL;
		}
		/*sipStrFree(_to->tagList);*/
		if(_to->paramList){
			sipParameterFree(_to->paramList);
			_to->paramList=NULL;
		}
		free(_to);
		_to=NULL;
	}
	return 1;
} /* ToFree */

int UnsupportedParse(MsgHdr ph, void **ret)
{
	SipUnsupported *_unsupported;
	MsgHdrElm nowElm;
	SipStr *_str;

	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;
	
	nowElm = ph->first;
	if(nowElm){
		_unsupported =(SipUnsupported*)calloc(1,sizeof(*_unsupported));
		_unsupported->numTags = ph->size;
		_str =(SipStr*) calloc(1,sizeof(*_str));
		_unsupported->sipOptionTagList = _str;
	}
	if(nowElm->content)
		_str->str = strDup((const char*)trimWS((char*)nowElm->content));
	else
		_str->str=NULL;

	nowElm = nowElm->next;
	while(1){
		if (nowElm != NULL){
			_str->next = (SipStr*)calloc(1,sizeof(*_str));
			_str = _str->next;
			_str->str = strDup((const char*)trimWS((char*)nowElm->content));
			nowElm = nowElm->next;
		}else
			break;
	}
	_str->next = NULL;
	*ret = (void *)_unsupported;
	return 1;
} /* UnsupportedParse */

int UnsupportedAsStr(SipUnsupported *_unsupported, unsigned char *buf, int buflen, int *length)
{
	SipStr *_str;
	int i,retvalue;
	DxStr UnsupportedStr;

	UnsupportedStr=dxStrNew();
	dxStrCat(UnsupportedStr, "Unsupported:");
	_str = _unsupported->sipOptionTagList;
	for(i=0; i<_unsupported->numTags; i++){
		dxStrCat(UnsupportedStr, _str->str);
		if (_str->next != NULL)
			dxStrCat(UnsupportedStr,",");/*for comma*/
		_str = _str->next;
	}
	dxStrCat(UnsupportedStr, "\r\n");
	*length=dxStrLen(UnsupportedStr);

	if((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(UnsupportedStr));
		retvalue= 1;
	}
	dxStrFree(UnsupportedStr);
	return retvalue;
} /* UnsupportedAsStr */

int UnsupportedFree(SipUnsupported *_unsupported)
{
	SipStr *_str, *tmp_str;

	if(_unsupported != NULL){
		_str = _unsupported->sipOptionTagList;
		while (1){
			if (_str != NULL){
				tmp_str = _str->next;
				if(_str->str != NULL){
					free(_str->str);
					_str->str=NULL;
				}
				free(_str);
				_str = tmp_str;
			}else
				break;
		}
		free(_unsupported);
		_unsupported=NULL;
	}
	return 1;
} /* UnsupportedFree */

int UserAgentParse(MsgHdr ph, void **ret)
{
	SipUserAgent *_useragent;
	SipStr *_str;
	unsigned char *buf,*freeptr, *space, *bracket;
	unsigned int tail;
	MsgHdrElm nowElm;
	int count=0;

	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;

	_useragent =(SipUserAgent*) calloc(1,sizeof(*_useragent));
	buf = (unsigned char*)strDup((const char*)ph->first->content);
	freeptr=buf;
	_str = (SipStr*)calloc(1,sizeof(*_str));
	_useragent->data = _str;
	_useragent->numdata=0;
	nowElm = ph->first;
	if (strChr(buf, '(') != NULL){
		CollectCommentData(&buf,(unsigned char*)strChr(buf, '('), &nowElm);
		freeptr=buf;
	}

	buf = (unsigned char*)trimWS((char*)buf);
	while(1){
		bracket =(unsigned char*) strChr(buf, '(');
		space = (unsigned char*)strstr(buf, " ");
		if ((bracket == NULL) && (space == NULL)){
			count++;
			_str->str = strDup((const char*)buf);
			_str->next = NULL;
			break;
		}
		if ((bracket == NULL) && (space != NULL)){
			count++;
			*space = '\0';
			_str->str = strDup((const char*)buf);
			_str->next =(SipStr*) calloc(1,sizeof(*_str));
			_str = _str->next;
			buf = space+1;
		}
		if ((bracket != NULL) && (space == NULL)){
			count++;
			_str->str = strDup((const char*)buf);
			_str->next = NULL;
			break;
		}
		if ((bracket != NULL) && (space != NULL)){
			count++;
			if (space<bracket){
				*space = '\0';
				_str->str = strDup((const char*)buf);
				_str->next =(SipStr*) calloc(1,sizeof(*_str));
				_str = _str->next;
				buf = space+1;
			}else{
				_str->str = strDup((const char*)buf);
				ExtractComment(bracket+1,(int*) &tail);
				*(buf+tail) = ')';
				*(buf+tail+1) = '\0';
				_str->next =(SipStr*) calloc(1,sizeof(*_str));
				_str = _str->next;
				buf = buf+tail+2;
			}
		}
	}
	_useragent->numdata=count;
	*ret = (void *)_useragent;
	free(freeptr);
	return 1;
} /* UserAgentParse */

int UserAgentAsStr(SipUserAgent *_useragent, unsigned char *buf, int buflen, int *length)
{
	SipStr *_str;
	DxStr UserAgentStr;
	int retvalue;

	UserAgentStr=dxStrNew();
	dxStrCat(UserAgentStr, "User-Agent:");
	_str = _useragent->data;
	while (1){
		dxStrCat(UserAgentStr, _str->str);
		if (_str->next != NULL)
			dxStrCat(UserAgentStr, " ");/*for space*/
		else
			break;
		_str = _str->next;
	}
	dxStrCat(UserAgentStr, "\r\n");
	*length=dxStrLen(UserAgentStr);

	if((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(UserAgentStr));
		retvalue= 1;
	}
	dxStrFree(UserAgentStr);
	return retvalue;
} /* UserAgentAsStr */

int UserAgentFree(SipUserAgent *_useragent)
{
	if(_useragent != NULL){
		if(_useragent->data){
			sipStrFree(_useragent->data);
			_useragent->data=NULL;
		}
		free(_useragent);
		_useragent=NULL;
	}
	return 1;
} /* UserAgentFree */

int ProcessViaParm(SipViaParm *_viaparm, unsigned char* buf)
{
	char* eq;
	SipParam *_param=NULL,*tmpparam=NULL;

	if( !_viaparm || !buf )
		return 0;

	buf = (unsigned char*) trimWS((char*)buf);
	eq = strChr(buf, '=');
	if (eq == NULL){
		/*if (!stricmp("hidden",(char*)buf))*/
		if(!strICmp("hidden",(const char*)buf)){
			_viaparm->hidden = 1;
			return 1;
		}else if(!strICmp("rport",(const char*)buf)){/* add by tyhuang 2003.7.22 */
			_viaparm->rport=-1;
			return 1;	
		}
		else{
			TCRPrint(1, "*** [sipStack] unsupport via parameter<%s> is found!\n",buf);
			/* add to unsupport list 
			 * add by tyhuang 2003.9.4 */
			_param=(SipParam*)calloc(1,sizeof *_param);
			if(_param){
				_param->name=strDup((const char*)buf);
				_param->value=NULL;
				_param->next=NULL;
				_viaparm->numofUnSupport++;
				if(_viaparm->UnSupportparamList==NULL)
					_viaparm->UnSupportparamList=_param;
				else{ /* paramlist exist so need to find the last one */
					tmpparam=_viaparm->UnSupportparamList;
					while(tmpparam->next!=NULL){
						tmpparam=tmpparam->next;
					}
					tmpparam->next=_param;
				}
			}else{
				TCRPrint(1, "*** [sipStack] memory alloc error in via parameter is found!\n");
				return 0;
			}
			return 1;
		}
	}else {
		*eq = '\0';
		if (!strICmp("ttl",(const char*)buf)){
			_viaparm->ttl = atoi((const char*)trimWS((char*)(eq+1)));
			return 1;
		}
		if (!strICmp("maddr",(const char*)buf)){
			_viaparm->maddr = strDup((const char*)trimWS((char*)(eq+1)));
			return 1;
		}
		if (!strICmp("received",(const char*)buf)){
			_viaparm->received = strDup((const char*)trimWS((char*)(eq+1)));
			return 1;
		}
		if (!strICmp("branch",(const char*)buf)){
			_viaparm->branch = strDup((const char*)trimWS((char*)(eq+1)));
			return 1;
		}/* add by tyhuang 2003.7.22 */
		if(!strICmp("rport",(const char*)buf)){
			_viaparm->rport=atoi((const char*)trimWS((char*)(eq+1)));
			return 1;
		}
		TCRPrint(1, "*** [sipStack] unsupport via parameter<%s> is found!\n",buf);
		/* add to unsupport list 
		 * add by tyhuang 2003.9.4 */
		_param=(SipParam*)calloc(1,sizeof *_param);
		if(_param){
			_param->name=strDup((const char*)buf);
			_param->value=strDup((const char*)trimWS((char*)(eq+1)));
			_param->next=NULL;
			_viaparm->numofUnSupport++;
			if(_viaparm->UnSupportparamList==NULL)
				_viaparm->UnSupportparamList=_param;
			else{ /* paramlist exist so need to find the last one */
				tmpparam=_viaparm->UnSupportparamList;
				while(tmpparam->next!=NULL){
					tmpparam=tmpparam->next;
				}
				tmpparam->next=_param;
			}
			
		}else{
			TCRPrint(1, "*** [sipStack] memory alloc error in via parameter is found!\n");
			return 0;
		}
		return 1;
	}
} /* ProcessViaParm */

int ViaParse(MsgHdr ph, void **ret)
{
	SipVia *_via;
	SipViaParm *_viaparm;
	int length;
	unsigned char *buf,*freeptr, *tmp, *sep;
	MsgHdrElm nowElm;
	char	temp[256];

	*ret=NULL;
	/* check input data */
	if(!ph->first)
		return 0;
	
	_via =(SipVia*) calloc(1,sizeof(*_via));
	if (!_via) return 0;
	_via->numParams = 0;
  
	nowElm = ph->first;
	_viaparm =(SipViaParm*) calloc(1,sizeof(*_viaparm));
	if (!_via) {
		free(_via);
		return 0;
	}
	_via->ParmList = _viaparm;
	_via->last = _viaparm;
	_viaparm->protocol=NULL;
	_viaparm->version=NULL;
	_viaparm->transport=NULL;
	_viaparm->sentby=NULL;
	_viaparm->hidden = 0;
	_viaparm->ttl = -1; /* represent no ttl value */
	_viaparm->maddr = NULL;
	_viaparm->received = NULL;
	_viaparm->branch = NULL;
	_viaparm->comment = NULL;
	_viaparm->next = NULL;
	_viaparm->rport = 0; /* add by tyhuang 2003.7.22 */
	_viaparm->UnSupportparamList = NULL;/* add by tyhuang 2003.9.4 */
	_viaparm->numofUnSupport=0;

	while (1){
		buf = (unsigned char*)strDup((const char*)nowElm->content);
		freeptr=buf;
		tmp = (unsigned char*)strstr(buf, "(");
		if (tmp != NULL) /* check if there is contained "Comment" */
			if (!CollectCommentData(&buf, tmp+1, &nowElm)){
				TCRPrint(1, "*** [sipStack] Via Parse error ! \n");
				return 0;
			}
		freeptr=buf;
		buf = (unsigned char*)trimWS((char*)buf);
		
		/*tmp = strstr(buf, " "); */
		/*tmp = '\0';*/

		/* parse sent-protocol */
		sep =(unsigned char*)strstr(buf, "/");
		if (sep == NULL){
			if(freeptr) free(freeptr);
			if(_via) ViaFree(_via);
			TCRPrint(1, "*** [sipStack] Via Parse error in sent-protocol! \n");
			return 0; /*Error*/
		}
		*sep = '\0';
		_viaparm->protocol = strDup((const char*)buf); /*protocol-name*/

		buf = sep + 1;
		sep = (unsigned char*)strstr(buf, "/");
		if (sep == NULL){
			if(freeptr) free(freeptr);
			if(_via) ViaFree(_via);
			TCRPrint(1, "*** [sipStack] Via Parse error in protocol-name! \n");
			return 0; /*Error*/
		}
		*sep = '\0';
		_viaparm->version = strDup((const char*)buf); /*protocol-version*/
    
		tmp = sep + 1;
		while( *tmp==' ' )	
			tmp++;
		while( *tmp!=0 && *tmp!=' ' && *tmp!='(' )	
			tmp++;
		temp[0] = 0;
		strncpy(temp,(const char*)(sep+1),(tmp-sep-1));
		temp[(tmp-sep-1)] = 0;
		_viaparm->transport = strDup((const char*)temp); /*transport*/

		/* parse sent-by */
		buf = tmp;

		sep = (unsigned char*)strstr(buf, ";");
		if (sep == NULL){ /* no parameter */
			sep =(unsigned char*) strstr(buf, "(");
			if (sep != NULL){ /* with comment*/
				*sep='\0';
				_viaparm->sentby=strDup((const char*)buf);
				if (!ExtractComment(sep+1, &length)){
					if(freeptr) free(freeptr);
					if(_via) ViaFree(_via);
					TCRPrint(1, "*** [sipStack] Via Parse error in sent-by! \n");
					return 0;
				}
			}else
				_viaparm->sentby=strDup((const char*)buf);
		}else {/*with parameter*/
			*sep = '\0';
			_viaparm->sentby=strDup((const char*)buf);
			buf = (unsigned char*)trimWS((char*)(sep + 1));
			sep = (unsigned char*)strstr(buf, ";");
			while (1) {
				if (sep == NULL){ /* no other parameter */
					sep =(unsigned char*)strstr(buf, "(");
					if (sep != NULL) {/* with comment */
						*sep = '\0';
						if (!ExtractComment(sep+1, &length)){
							if(freeptr) free(freeptr);
							if(_via) ViaFree(_via);
							TCRPrint(1, "*** [sipStack] Via Parse error in parameter! \n");
							return 0;
						}else{
							*(sep+length)='\0';
							_viaparm->comment =strDup((const char*)(sep+1)) ;
						}
					}
					if (ProcessViaParm(_viaparm, buf))
						break;
					else
						return 0;
				} else{
					*sep = '\0';
					if (!ProcessViaParm(_viaparm, buf))
						return 0;
					buf = (unsigned char*)trimWS((char*)(sep + 1) );
					sep = (unsigned char*)strstr(buf, ";");
				}
			}/*end while loop */
		}/*end else */
		_via->numParams++;
		nowElm = nowElm->next;
		if (nowElm != NULL){
			_viaparm->next =(SipViaParm*) calloc(1,sizeof(*_viaparm));
			_viaparm = _viaparm->next;
			_via->last = _viaparm;
			_viaparm->protocol=NULL;
			_viaparm->version=NULL;
			_viaparm->transport=NULL;
			_viaparm->sentby=NULL;
			_viaparm->hidden = 0;
			_viaparm->ttl = -1; /* represent no ttl value */
			_viaparm->maddr = NULL;
			_viaparm->received = NULL;
			_viaparm->branch = NULL;
			_viaparm->comment = NULL;
			_viaparm->next = NULL;
			_viaparm->rport = 0;/* add by tyhuang 2003.7.22 */
			_viaparm->UnSupportparamList = NULL; /* add by tyhuang 2003.9.4 */
			_viaparm->numofUnSupport=0;
		}else{
			if(freeptr!=NULL)
				free(freeptr);
			break;	
		}
		if(freeptr!=NULL)
			free(freeptr);
	} /*end while loop */
	*ret = (void*)_via;
	return 1;
} /* ViaParse */

int ViaAsStr(SipVia *_via, unsigned char *buf, int buflen, int *length)
{
	SipViaParm *_viaparm;
	SipParam *_param=NULL;
	int i,retvalue;
	unsigned char tmp[12];
	char *charstr;
	DxStr ViaStr;

	ViaStr=dxStrNew();
	_viaparm = _via->ParmList;
	for(i=0; i<_via->numParams&&_viaparm; i++){
		dxStrCat(ViaStr, "Via:");
		dxStrCat(ViaStr, _viaparm->protocol);
		dxStrCat(ViaStr, "/");
		dxStrCat(ViaStr, _viaparm->version);
		dxStrCat(ViaStr, "/");
		dxStrCat(ViaStr, _viaparm->transport);
		if (*(_viaparm->sentby)!=' ') 
			dxStrCat(ViaStr, " ");
		dxStrCat(ViaStr, _viaparm->sentby);
		if (_viaparm->hidden)
			dxStrCat(ViaStr, ";hidden");
		if (_viaparm->ttl >= 0){
			charstr=i2a(_viaparm->ttl,(char*)tmp,12);
			dxStrCat(ViaStr, ";ttl=");
			dxStrCat(ViaStr,(char*) tmp);
		}
		/* rport:add by tyhuang 2003.7.22 */
		if (_viaparm->rport > 0){
			charstr=i2a(_viaparm->rport,(char*)tmp,12);
			dxStrCat(ViaStr, ";rport=");
			dxStrCat(ViaStr,(char*) tmp);
		}else if(_viaparm->rport < 0)
			dxStrCat(ViaStr, ";rport");
		
		if (_viaparm->maddr != NULL){
			dxStrCat(ViaStr, ";maddr=");
			dxStrCat(ViaStr, _viaparm->maddr);
		}
		if (_viaparm->received != NULL){
			dxStrCat(ViaStr, ";received=");
			dxStrCat(ViaStr, _viaparm->received);
		}
		if (_viaparm->branch != NULL){
			dxStrCat(ViaStr, ";branch=");
			dxStrCat(ViaStr, _viaparm->branch);
		}
		/* add by tyhuang 2003.9.4 */
		_param=_viaparm->UnSupportparamList;
		while(_param!=NULL){
			dxStrCat(ViaStr,";");
			dxStrCat(ViaStr,_param->name);
			/* _param with value,should add "=" and value */
			if(_param->value!=NULL){
				dxStrCat(ViaStr,"=");
				dxStrCat(ViaStr,_param->value);
			}
			_param=_param->next;
		}
		if (_viaparm->comment != NULL){
			dxStrCat(ViaStr, "(");
			dxStrCat(ViaStr, _viaparm->comment);
			dxStrCat(ViaStr, ")");
		}
		_viaparm = _viaparm->next;
		dxStrCat(ViaStr, "\r\n");
	}
	*length=dxStrLen(ViaStr);

	if((*length) >= buflen)
		retvalue= 0;
	else {
		strcpy(buf,(const char*)dxStrAsCStr(ViaStr));
		retvalue= 1;
	}
	dxStrFree(ViaStr);
	return retvalue;
} /* ViaAsStr */

int ViaFree(SipVia *_via)
{
	if(_via != NULL){
		if(_via->ParmList){
			sipViaParmFree(_via->ParmList);
			_via->ParmList=NULL;
		}
		free(_via);
		_via=NULL;
	}
	return 1;
} /* ViaFree */

int WarningParse(MsgHdr ph, void **ret)
{
	SipWarning *_warning;
	SipWrnVal *_warningvalue;
	unsigned char *buf,*freeptr, *tmp;
	MsgHdrElm nowElm;

	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;
	
	_warning =(SipWarning*) calloc(1,sizeof(*_warning));
	_warning->numVal = 0;
	nowElm = ph->first;
	buf = (unsigned char*)strDup((const char*)nowElm->content);
	freeptr=buf;
	
	_warningvalue =(SipWrnVal*) calloc(1,sizeof(*_warningvalue));
	_warningvalue->code = NULL;
	_warningvalue->agent = NULL;
	_warningvalue->text = NULL;
	_warningvalue->next = NULL;

	_warning->WarningValList = _warningvalue;
	_warning->last = _warningvalue;

	while (1){
		tmp = (unsigned char*)strstr(buf, "\"");
		if (tmp == NULL){
			if(freeptr) free(freeptr);
			if(_warning) WarningFree(_warning);
			TCRPrint(1, "*** [sipStack] Warning Parse error! \n");
			return 0;
		}else{
			CollectQuotedString(&buf, tmp, &nowElm);
			buf = (unsigned char*)trimWS((char*)(buf));
			tmp = (unsigned char*)strstr(buf, " ");
			*tmp = '\0';
			_warningvalue->code = (unsigned char*)strDup((const char*)buf);
			buf=tmp+1;
			tmp = (unsigned char*)strstr(buf, " ");
			*tmp = '\0';
			_warningvalue->agent =(unsigned char*) strDup((const char*)buf);
			_warningvalue->text =(unsigned char*) strDup((const char*)(tmp+1));
			_warning->numVal++;
			if (nowElm == ph->last)
				break;
			else {
				nowElm = nowElm->next;
				_warningvalue->next = (SipWrnVal*)calloc(1,sizeof(*_warningvalue));
				_warningvalue = _warningvalue->next;
				_warningvalue->next = NULL;
				_warning->last = _warningvalue;
				free(freeptr);
				buf = (unsigned char*)strDup((const char*)nowElm->content);
				freeptr=buf;
			}
		}
	} /*end while loop */
	*ret = (void *)_warning;
	if(freeptr != NULL)
		free(freeptr);
	return 1;
} /* WarningParse */

int WarningAsStr(SipWarning *_warning, unsigned char *buf, int buflen, int *length)
{
	SipWrnVal *_warningvalue;
	int i,retvalue;
	DxStr WarningStr;

	WarningStr=dxStrNew();
	dxStrCat(WarningStr, "Warning:");
	_warningvalue = _warning->WarningValList;
	for(i=0; i<_warning->numVal; i++) {
		dxStrCat(WarningStr,(char*) _warningvalue->code);
		dxStrCat(WarningStr, " ");
		dxStrCat(WarningStr,(char*) _warningvalue->agent);
		dxStrCat(WarningStr, " ");
		dxStrCat(WarningStr,(char*) _warningvalue->text);
		if(_warningvalue->next != NULL){
			dxStrCat(WarningStr, ",");
			_warningvalue=_warningvalue->next;
		}
	}
	dxStrCat(WarningStr, "\r\n");
	*length=dxStrLen(WarningStr);

	if((*length) >= buflen)
		retvalue= 0;
	else {
		strcpy(buf,(const char*)dxStrAsCStr(WarningStr));
		retvalue= 1;
	}
	dxStrFree(WarningStr);
	return retvalue;
} /* WarningAsStr */

int WarningFree(SipWarning *_warning)
{
	SipWrnVal *_warningvalue, *tmp_warningvalue;

	if(_warning != NULL){
		_warningvalue = _warning->WarningValList;
		while (_warningvalue != NULL){
			tmp_warningvalue = _warningvalue->next;
			if(_warningvalue->code != NULL){ 
				free(_warningvalue->code);
				_warningvalue->code=NULL;
			}
			if(_warningvalue->agent != NULL){ 
				free(_warningvalue->agent);
				_warningvalue->agent=NULL;
			}
			if(_warningvalue->text != NULL) {
				free(_warningvalue->text);
				_warningvalue->text=NULL;
			}
			free(_warningvalue);
			_warningvalue = tmp_warningvalue;
		}
		free(_warning);
		_warning=NULL;
	}
	return 1;
} /* WarningFree */

int WWWAuthenticateParse(MsgHdr ph, void **ret)
{
	SipWWWAuthn	*_wwwauthenticate;
	SipChllng	*_challenge;
	SipParam	*_parameter;
	MsgHdrElm       nowElm;
	unsigned char   *buf,*buffer,*freeptr, *tmp;

	*ret=NULL;
	/* chech input data */
	if(!ph->first)
		return 0;
	
	_wwwauthenticate = (SipWWWAuthn*)calloc(1,sizeof(*_wwwauthenticate));
	_wwwauthenticate->numChallenges = 0;
	nowElm = ph->first;
	buf = (unsigned char*)strDup((const char*)nowElm->content);
	freeptr=buf;
	buf = (unsigned char*)trimWS((char*)buf);
	_challenge = (SipChllng*)calloc(1,sizeof(*_challenge));
	_challenge->numParams = 0;
	_challenge->next = NULL;
	_wwwauthenticate->ChllngList = _challenge;

	while (1){
		/* scheme */

		tmp = (unsigned char*)strstr(buf," ");
		if (tmp == NULL){
			if(freeptr!= NULL)
				free(freeptr);
			if(_wwwauthenticate) WWWAuthenticateFree(_wwwauthenticate);
			TCRPrint(1, "*** [sipStack] WWWAuthenticate Parse error! \n");
			return 0;
		}
		else{
			unsigned char tmpbuf[12];
			int pos,len1,len2;

			len1=strlen(buf);
			len2=strlen(tmp);
			pos=strlen(buf)-strlen(tmp);
			memset(tmpbuf,0,12);
			strncpy((char*)tmpbuf,(char*)buf,pos);
			_wwwauthenticate->numChallenges++;
			_challenge->scheme =strDup((const char*) tmpbuf);
			_parameter = (SipParam*)calloc(1,sizeof(*_parameter));
			_challenge->sipParamList = _parameter;
			*tmp = '\0';
			AnalyzeParameter(tmp+1, _parameter);
			_challenge->numParams++;
		}

		/* parameter */
		while (1){
			if (nowElm == ph->last){
				*ret = (void*)_wwwauthenticate;
				if(freeptr!=NULL)
					free(freeptr);
				return 1;
			}else{
				nowElm = nowElm->next;
				buffer = (unsigned char*)trimWS((char*)strDup((const char*)nowElm->content));
				/*tmp = (unsigned char*)strstr(buffer, " ");*/ /*Acer Modify SipIt 10th*/
				tmp=NULL;
				if(tmp != NULL){
					_challenge->next =(SipChllng*) calloc(1,sizeof(*_challenge));
					_challenge = _challenge->next;
					_challenge->numParams = 0;
					_challenge->next = NULL;
					break;
				}else{
					_parameter->next =(SipParam*) calloc(1,sizeof(*_parameter));
					_parameter = _parameter->next;
					AnalyzeParameter(buffer, _parameter);
					_challenge->numParams++;
				}
				free(buffer);
			}
		}/*end while loop(parameter) */
	}/*end while loop */
	if(freeptr!=NULL)
		free(freeptr);
} /* WWWAuthenticateParse */

int WWWAuthenticateAsStr(SipWWWAuthn *_wwwauthenticate, unsigned char *buf, int buflen, int *length)
{
	SipChllng *_challenge;
	SipParam* _parameter;
	int i,j,retvalue;
	DxStr WWWAuthenStr;

	WWWAuthenStr=dxStrNew();
	_challenge = _wwwauthenticate->ChllngList;
	for(i=0; i<_wwwauthenticate->numChallenges; i++){
		dxStrCat(WWWAuthenStr, "WWW-Authenticate:");
		dxStrCat(WWWAuthenStr, _challenge->scheme);
		dxStrCat(WWWAuthenStr, " ");
		_parameter = _challenge->sipParamList;
		for(j=0; j<_challenge->numParams; j++) {
			if(_parameter->name)
				dxStrCat(WWWAuthenStr, _parameter->name);
			if(_parameter->value) {
				dxStrCat(WWWAuthenStr, "=");
				dxStrCat(WWWAuthenStr, _parameter->value);
			}
			if (_parameter->next != NULL)
				dxStrCat(WWWAuthenStr, ",");/*for comma*/
			_parameter = _parameter->next;
		}
		/*if(_challenge->next != NULL)*/
		/*	dxStrCat(WWWAuthenStr, ",");*/
		dxStrCat(WWWAuthenStr, "\r\n");
		_challenge = _challenge->next;
	}
	*length=dxStrLen(WWWAuthenStr);

	if((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(WWWAuthenStr));
		retvalue= 1;
	}
	dxStrFree(WWWAuthenStr);
	return retvalue;

} /* WWWAuthenticateAsStr */

int WWWAuthenticateFree(SipWWWAuthn *_wwwauthenticate)
{
	if(_wwwauthenticate != NULL){
		if(_wwwauthenticate->ChllngList){
			sipChallengeFree(_wwwauthenticate->ChllngList);
			_wwwauthenticate->ChllngList=NULL;
		}
		free(_wwwauthenticate);
		_wwwauthenticate=NULL;
	}
	return 1;
} /* WWWAuthenticateFree */


#ifdef ICL_IMS 
/* P-Media-Authorization */
int	PMAParse(MsgHdr ph, void **ret)
{
	return CallIDParse(ph,ret);
}/* PMAParse */

int	PMAAsStr(unsigned char* hdr, unsigned char *buf, int buflen, int *length)
{
	DxStr HdrStr;
	int retvalue;
	
	if(!hdr || !buf || !length )
		return 0;
	
	if(strlen(hdr)<1)
		return 0;

	HdrStr=dxStrNew();
	dxStrCat(HdrStr, "P-Media-Authorization:");
	dxStrCat(HdrStr,(const char*)hdr);
	dxStrCat(HdrStr, "\r\n");
	
	*length=dxStrLen(HdrStr);
	
	if ((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(HdrStr));
		retvalue= 1;
	}
	dxStrFree(HdrStr);
	return retvalue;
} /* PMAAsStr */

/* Path */
int	PathParse(MsgHdr ph, void **ret)
{
	return RouteParse(ph,ret);
}/* PathParse */

int	PathAsStr(SipPath* path, unsigned char  *buf, int buflen, int *length)
{
	int i,j,retvalue;
	SipRecAddr *_addr;
	SipParam *_param;
	DxStr HdrStr;
	
	HdrStr=dxStrNew();
	dxStrCat(HdrStr, "Path:");
	_addr = path->sipNameAddrList;
	for(i=0; i<path->numNameAddrs; i++){
		if(!_addr) continue;
		if(_addr->display_name){
			dxStrCat(HdrStr,"\"");
			dxStrCat(HdrStr,_addr->display_name);
			dxStrCat(HdrStr,"\"");
		}
		if(_addr->addr){
			dxStrCat(HdrStr,"<");
			dxStrCat(HdrStr,_addr->addr);
			dxStrCat(HdrStr,">");
		}
		
		_param=_addr->sipParamList;
		for(j=0;j<_addr->numParams;j++){
			if(_param){
				dxStrCat(HdrStr,";");
				dxStrCat(HdrStr,_param->name);
				if(_param->value){
					dxStrCat(HdrStr,"=");
					dxStrCat(HdrStr,_param->value);
				}
			}
		}
		if (_addr->next != NULL){
			dxStrCat(HdrStr, ",");
		}
		_addr = _addr->next;
		if(_addr==NULL) break;
	}
	dxStrCat(HdrStr, "\r\n");
	*length=dxStrLen(HdrStr);
	
	if((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(HdrStr));
		retvalue= 1;
	}
	dxStrFree(HdrStr);
	return retvalue;
}/* PathAsStr */

int	PathFree(SipPath* path)
{
	return RouteFree((SipRoute *)path);
}/* PathFree */

/* P-Access-Network-Info */
int	PANIParse(MsgHdr ph, void **ret)
{
	return CallIDParse(ph,ret);
}/* PANIParse */

int	PANIAsStr(unsigned char* hdr, unsigned char *buf, int buflen, int *length)
{
	DxStr HdrStr;
	int retvalue;

	if(!hdr || !buf || !length )
		return 0;

	if(strlen(hdr)<1)
		return 0;

	HdrStr=dxStrNew();
	dxStrCat(HdrStr, "P-Access-Network-Info:");
	dxStrCat(HdrStr,(const char*)hdr);
	dxStrCat(HdrStr, "\r\n");

	*length=dxStrLen(HdrStr);

	if ((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(HdrStr));
		retvalue= 1;
	}
	dxStrFree(HdrStr);
	return retvalue;
} /* PANIAsStr */

/* P-Associated-URI */
int	PAUParse(MsgHdr ph, void **ret)
{
	return CallIDParse(ph,ret);
}/* PAUParse */

int	PAUAsStr(unsigned char* hdr, unsigned char *buf, int buflen, int *length)
{
	DxStr HdrStr;
	int retvalue;
	
	if(!hdr || !buf || !length )
		return 0;
	
	HdrStr=dxStrNew();
	dxStrCat(HdrStr, "P-Associated-URI:");
	dxStrCat(HdrStr,(const char*)hdr);
	dxStrCat(HdrStr, "\r\n");
	
	*length=dxStrLen(HdrStr);
	
	if ((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(HdrStr));
		retvalue= 1;
	}
	dxStrFree(HdrStr);
	return retvalue;
} /* PAUAsStr */

/* P-Called-Party-ID */
int	PCPIParse(MsgHdr ph, void **ret)
{
	return CallIDParse(ph,ret);
}/* PCPIParse */

int	PCPIAsStr(unsigned char* hdr, unsigned char *buf, int buflen, int *length)
{
	DxStr HdrStr;
	int retvalue;
	
	if(!hdr || !buf || !length )
		return 0;
	
	if(strlen(hdr)<1)
		return 0;

	HdrStr=dxStrNew();
	dxStrCat(HdrStr, "P-Called-Party-ID:");
	dxStrCat(HdrStr,(const char*)hdr);
	dxStrCat(HdrStr, "\r\n");
	
	*length=dxStrLen(HdrStr);
	
	if ((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(HdrStr));
		retvalue= 1;
	}
	dxStrFree(HdrStr);
	return retvalue;
} /* PCPIAsStr */

/* P-Charging-Function-Address */
int	PCFAParse(MsgHdr ph, void **ret)
{
	return CallIDParse(ph,ret);
}/* PCFAParse */

int	PCFAAsStr(unsigned char* hdr, unsigned char *buf, int buflen, int *length)
{
	DxStr HdrStr;
	int retvalue;
	
	if(!hdr || !buf || !length )
		return 0;

	if(strlen(hdr)<1)
		return 0;

	HdrStr=dxStrNew();
	dxStrCat(HdrStr, "P-Charging-Function-Address:");
	dxStrCat(HdrStr,(const char*)hdr);
	dxStrCat(HdrStr, "\r\n");
	
	*length=dxStrLen(HdrStr);
	
	if ((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(HdrStr));
		retvalue= 1;
	}
	dxStrFree(HdrStr);
	return retvalue;
} /* PCFAAsStr */

/* P-Charging-Vector */
int	PCVParse(MsgHdr ph, void **ret)
{
	return CallIDParse(ph,ret);
}/* PCVParse */

int	PCVAsStr(unsigned char* hdr, unsigned char *buf, int buflen, int *length)
{
	DxStr HdrStr;
	int retvalue;
	
	if(!hdr || !buf || !length )
		return 0;
	
	if(strlen(hdr)<1)
		return 0;

	HdrStr=dxStrNew();
	dxStrCat(HdrStr, "P-Charging-Vector:");
	dxStrCat(HdrStr,(const char*)hdr);
	dxStrCat(HdrStr, "\r\n");
	
	*length=dxStrLen(HdrStr);
	
	if ((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(HdrStr));
		retvalue= 1;
	}
	dxStrFree(HdrStr);
	return retvalue;
} /* PCVAsStr */

/* P-Visited-Network-ID */
int	PVNIParse(MsgHdr ph, void **ret)
{
	return CallIDParse(ph,ret);
}/* PVNIParse */

int	PVNIAsStr(unsigned char* hdr, unsigned char *buf, int buflen, int *length)
{
	DxStr HdrStr;
	int retvalue;
	
	if(!hdr || !buf || !length )
		return 0;
	
	if(strlen(hdr)<1)
		return 0;

	HdrStr=dxStrNew();
	dxStrCat(HdrStr, "P-Visited-Network-ID:");
	dxStrCat(HdrStr,(const char*)hdr);
	dxStrCat(HdrStr, "\r\n");
	
	*length=dxStrLen(HdrStr);
	
	if ((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(HdrStr));
		retvalue= 1;
	}
	dxStrFree(HdrStr);
	return retvalue;
} /* PVNIAsStr */

/* Security-Client */
int	SecurityClientParse(MsgHdr ph, void **ret)
{
	return CallIDParse(ph,ret);
}/* SecurityClientParse */

int	SecurityClientAsStr(unsigned char* hdr, unsigned char *buf, int buflen, int *length)
{
	DxStr HdrStr;
	int retvalue;
	
	if(!hdr || !buf || !length )
		return 0;
	
	if(strlen(hdr)<1)
		return 0;

	HdrStr=dxStrNew();
	dxStrCat(HdrStr, "Security-Client:");
	dxStrCat(HdrStr,(const char*)hdr);
	dxStrCat(HdrStr, "\r\n");
	
	*length=dxStrLen(HdrStr);
	
	if ((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(HdrStr));
		retvalue= 1;
	}
	dxStrFree(HdrStr);
	return retvalue;
} /* SecurityClientAsStr */

/* Security-Server */
int	SecurityServerParse(MsgHdr ph, void **ret)
{
	return CallIDParse(ph,ret);
}/* SecurityServerParse */

int	SecurityServerAsStr(unsigned char* hdr, unsigned char *buf, int buflen, int *length)
{
	DxStr HdrStr;
	int retvalue;
	
	if(!hdr || !buf || !length )
		return 0;
	
	if(strlen(hdr)<1)
		return 0;

	HdrStr=dxStrNew();
	dxStrCat(HdrStr, "Security-Server:");
	dxStrCat(HdrStr,(const char*)hdr);
	dxStrCat(HdrStr, "\r\n");
	
	*length=dxStrLen(HdrStr);
	
	if ((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(HdrStr));
		retvalue= 1;
	}
	dxStrFree(HdrStr);
	return retvalue;
} /* SecurityServerAsStr */

/* Security-Verify */
int	SecurityVerifyParse(MsgHdr ph, void **ret)
{
	return CallIDParse(ph,ret);
}/* SecurityVerifyParse */

int	SecurityVerifyAsStr(unsigned char* hdr, unsigned char *buf, int buflen, int *length)
{
	DxStr HdrStr;
	int retvalue;
	
	if(!hdr || !buf || !length )
		return 0;
	
	if(strlen(hdr)<1)
		return 0;

	HdrStr=dxStrNew();
	dxStrCat(HdrStr, "Security-Verify:");
	dxStrCat(HdrStr,(const char*)hdr);
	dxStrCat(HdrStr, "\r\n");
	
	*length=dxStrLen(HdrStr);
	
	if ((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(HdrStr));
		retvalue= 1;
	}
	dxStrFree(HdrStr);
	return retvalue;
} /* SecurityVerifyAsStr */

/* Service-Route */
int	ServiceRouteParse(MsgHdr ph, void **ret)
{
	return RouteParse(ph,ret);
}/* ServiceRouteParse */

int	ServiceRouteAsStr(SipServiceRoute* sr, unsigned char  *buf, int buflen, int *length)
{
	int i,j,retvalue;
	SipRecAddr *_addr;
	SipParam *_param;
	DxStr HdrStr;
	
	HdrStr=dxStrNew();
	dxStrCat(HdrStr, "Service-Route:");
	_addr = sr->sipNameAddrList;
	for(i=0; i<sr->numNameAddrs; i++){
		if(!_addr) continue;
		if(_addr->display_name){
			dxStrCat(HdrStr,"\"");
			dxStrCat(HdrStr,_addr->display_name);
			dxStrCat(HdrStr,"\"");
		}
		if(_addr->addr){
			dxStrCat(HdrStr,"<");
			dxStrCat(HdrStr,_addr->addr);
			dxStrCat(HdrStr,">");
		}
		
		_param=_addr->sipParamList;
		for(j=0;j<_addr->numParams;j++){
			if(_param){
				dxStrCat(HdrStr,";");
				dxStrCat(HdrStr,_param->name);
				if(_param->value){
					dxStrCat(HdrStr,"=");
					dxStrCat(HdrStr,_param->value);
				}
			}
		}
		if (_addr->next != NULL){
			dxStrCat(HdrStr, ",");
		}
		_addr = _addr->next;
		if(_addr==NULL) break;
	}
	dxStrCat(HdrStr, "\r\n");
	*length=dxStrLen(HdrStr);
	
	if((*length) >= buflen)
		retvalue= 0;
	else{
		strcpy(buf,(const char*)dxStrAsCStr(HdrStr));
		retvalue= 1;
	}
	dxStrFree(HdrStr);
	return retvalue;
}/* ServiceRouteAsStr */

int	ServiceRouteFree(SipServiceRoute* sr)
{
	return RouteFree((SipRoute *)sr);
}/* ServiceRouteFree*/
		
#endif /* end of ICL_IMS */

/*----------------------------------------------*/
int sipBodyFree(SipBody *body)
{
	if(body != NULL){
		if(body->content != NULL) 
			free(body->content);
		free(body);
	}
	return 1;
}

/*-------------------------------------------------*/
/*------- Duplicate functions ---------------------*/
/*-------------------------------------------------*/
SipParam* sipParameterDup(SipParam* paralist, int listlength)
{
	int idx;
	SipParam *tmpPara,*newPara,*prePara,*firstPara;

	tmpPara=paralist;
	for(idx=0;idx<listlength;idx++){
		if(tmpPara == NULL ) break;
		newPara=(SipParam*)calloc(1,sizeof(SipParam));
		newPara->name=NULL;
		newPara->value=NULL;
		newPara->next=NULL;
		if(tmpPara->name != NULL)
			newPara->name=strDup((const char*)tmpPara->name);
		if(tmpPara->value != NULL)
			newPara->value=strDup((const char*)tmpPara->value);
		(idx==0) ? (firstPara=newPara) : (prePara->next = newPara);
		prePara=newPara;
		tmpPara=tmpPara->next;
	}
	(idx>0) ? (tmpPara=firstPara) : (tmpPara=NULL);
	
	return tmpPara;
}

SipAddr* sipAddrDup(SipAddr* addrList,int addrnum)
{
	int idx;
	SipAddr *tmpAddr,*newAddr,*preAddr,*firstAddr;

	tmpAddr=addrList;
	if(addrnum == -1) addrnum=65536;
	for(idx=0;(idx<addrnum)&&(tmpAddr!=NULL);idx++){
		if(tmpAddr==NULL) break;
		newAddr=(SipAddr*)calloc(1,sizeof(SipAddr));
		newAddr->next=NULL;
		if(tmpAddr->display_name != NULL)
			newAddr->display_name=strDup((const char*)tmpAddr->display_name);
		else
			newAddr->display_name=NULL;
		newAddr->addr=strDup((const char*)tmpAddr->addr);
		newAddr->with_angle_bracket=tmpAddr->with_angle_bracket;
		(idx==0) ? (firstAddr=newAddr) : (preAddr->next = newAddr);
		preAddr=newAddr;
		tmpAddr=tmpAddr->next;
	}/*end copy Address list*/
	(idx>0) ? (tmpAddr=firstAddr) : (tmpAddr=NULL);

	return tmpAddr;
}

SipRecAddr* sipRecAddrDup(SipRecAddr* addrList,int addrnum)
{
	int idx;
	SipRecAddr *tmpAddr,*newAddr,*preAddr,*firstAddr;

	tmpAddr=addrList;
	if(addrnum == -1) addrnum=65536;
	for(idx=0;(idx<addrnum)&&(tmpAddr!=NULL);idx++){
		if(tmpAddr==NULL) break;
		newAddr=(SipRecAddr*)calloc(1,sizeof(SipRecAddr));
		newAddr->next=NULL;
		if(tmpAddr->display_name != NULL)
			newAddr->display_name=strDup((const char*)tmpAddr->display_name);
		else
			newAddr->display_name=NULL;
		if(tmpAddr->addr)
			newAddr->addr=strDup((const char*)tmpAddr->addr);
		else
			newAddr->addr=NULL;
		if(tmpAddr->sipParamList){
			newAddr->sipParamList=sipParameterDup(tmpAddr->sipParamList,tmpAddr->numParams);
			newAddr->numParams=tmpAddr->numParams;
		}else{
			newAddr->sipParamList=NULL;
			newAddr->numParams=0;
		}
		(idx==0) ? (firstAddr=newAddr) : (preAddr->next = newAddr);
		preAddr=newAddr;
		tmpAddr=tmpAddr->next;
	}/*end copy Address list*/
	(idx>0) ? (tmpAddr=firstAddr) : (tmpAddr=NULL);

	return tmpAddr;
}

SipAuthz* sipAuthzDup(SipAuthz* authzList){

	SipAuthz *newauthz,*nowauthz,*tmpAuthz,*firstAuthz,*preAuthz;
	int idx=0;

	nowauthz=authzList;
	while(nowauthz!=NULL){
		newauthz=(SipAuthz*)calloc(1,sizeof(*newauthz));
		newauthz->numParams=nowauthz->numParams;
		newauthz->next=NULL;
		if(nowauthz->scheme != NULL)
			newauthz->scheme=strDup((const char*)nowauthz->scheme);
		else
			newauthz->scheme=NULL;
		newauthz->sipParamList=sipParameterDup(nowauthz->sipParamList,nowauthz->numParams);
		(idx==0) ? (firstAuthz=newauthz):(preAuthz->next=newauthz);
		preAuthz=newauthz;
		nowauthz=nowauthz->next;
		idx++;
	}
	(idx>0) ? ( tmpAuthz=firstAuthz) : (tmpAuthz=NULL);
	return tmpAuthz;
}


SipStr* sipStrDup(SipStr* strList,int strnum)
{
	int idx;
	SipStr *tmpStr,*newStr,*preStr,*firstStr;

	tmpStr=strList;
	if(strnum == -1) strnum=65536;
	for(idx=0;((idx<strnum)&&(tmpStr != NULL));idx++){
		if(tmpStr==NULL) break;
		newStr=(SipStr*)calloc(1,sizeof(SipStr));
		newStr->next=NULL;
		newStr->str=strDup((const char*)tmpStr->str);
		(idx==0) ? (firstStr=newStr) : ( preStr->next=newStr);
		preStr=newStr;
		tmpStr=tmpStr->next;
	}
	(idx>0) ? (tmpStr=firstStr) : (tmpStr=NULL);

	return tmpStr;
}

SipCoding* sipCodingDup(SipCoding* codingList,int codingnum)
{
	int idx;
	SipCoding *tmpCoding,*newCoding,*preCoding,*firstCoding;

	tmpCoding=codingList;
	for(idx=0;idx<codingnum;idx++){
		if(tmpCoding == NULL) break;
		newCoding=(SipCoding*)calloc(1,sizeof(SipCoding));
		newCoding->next=NULL;
		newCoding->coding=strDup((const char*)tmpCoding->coding);
		newCoding->qvalue=strDup((const char*)tmpCoding->qvalue);
		(idx==0) ? (firstCoding=newCoding):(preCoding->next=newCoding);
		preCoding=newCoding;
		tmpCoding=tmpCoding->next;
	}
	(idx) > 0 ? (tmpCoding = firstCoding) : (tmpCoding=NULL);

	return tmpCoding;
}

SipLang* sipLanguageDup(SipLang* langList,int langnum)
{
	int idx;
	SipLang *tmpLang,*newLang,*preLang,*firstLang;

	tmpLang=langList;
	for(idx=0;idx<langnum;idx++){
		if(tmpLang==NULL) break;
		newLang=(SipLang*)calloc(1,sizeof(SipLang));
		newLang->next=NULL;
		newLang->lang=strDup((const char*)tmpLang->lang);
		newLang->qvalue=strDup((const char*)tmpLang->qvalue);
		(idx==0) ? (firstLang=newLang):(preLang->next=newLang);
		preLang=newLang;
		tmpLang=tmpLang->next;
	}
	(idx>0) ? (tmpLang = firstLang) : (tmpLang=NULL);
	return tmpLang;
}

SipMediaType* sipMediaTypeDup(SipMediaType* mediaList,int medianum)
{
	int idx;
	SipMediaType *tmpMedia,*newMedia,*preMedia,*firstMedia;

	tmpMedia=mediaList;
	for(idx=0;idx<medianum;idx++){
		if(tmpMedia == NULL) break;
		newMedia=(SipMediaType*)calloc(1,sizeof(SipMediaType));
		newMedia->next=NULL;
		newMedia->type=strDup((const char*)tmpMedia->type);
		newMedia->subtype=strDup((const char*)tmpMedia->subtype);
		newMedia->numParam=tmpMedia->numParam;
		(idx==0) ? (firstMedia=newMedia): (preMedia->next=newMedia);
		preMedia=newMedia;
		(newMedia->sipParamList)=sipParameterDup(tmpMedia->sipParamList,tmpMedia->numParam);
		tmpMedia=tmpMedia->next;
	} 
	(idx > 0) ? (tmpMedia = firstMedia) : (tmpMedia=NULL);

	return tmpMedia;
}

SipContactElm* sipContactElmDup(SipContactElm* elmList,int elmnum)
{
	int idx=0;
	SipContactElm *tmpElm,*newElm,*preElm,*firstElm=NULL;

	if(elmList==NULL)
		return NULL;
	tmpElm=elmList;
	for(idx=0;idx<elmnum;idx++){
		if (!tmpElm) break;
		newElm=(SipContactElm*)calloc(1,sizeof(SipContactElm));
		newElm->next=NULL;
		newElm->expireSecond=tmpElm->expireSecond;
		if(tmpElm->comment != NULL)
			newElm->comment=strDup((const char*)tmpElm->comment);
		else
			newElm->comment=NULL;
		
		(idx==0) ? (firstElm=newElm):(preElm->next=newElm);
		preElm=newElm;
		
		/* first add address element */
		newElm->numaddress=tmpElm->numaddress;
		newElm->address = sipAddrDup(tmpElm->address,tmpElm->numaddress);

		/*Second add expireDate element */
		newElm->expireSecond=tmpElm->expireSecond;
		newElm->expireDate=rfc1123DateDup(tmpElm->expireDate);

		/*third add parameter */
		newElm->numext_attrs=tmpElm->numext_attrs;
		newElm->ext_attrs=sipParameterDup(tmpElm->ext_attrs,tmpElm->numext_attrs);

		tmpElm=tmpElm->next;
	}/* end idx=0; contact list */
	/*(idx > 0) ? (tmpElm = firstElm) : (tmpElm=NULL);*/
	tmpElm = firstElm;
	return tmpElm;
}

SipPxyAuthz* sipPxyAuthzDup(SipPxyAuthz* authzList)
{
	SipPxyAuthz *newauthz,*nowauthz,*tmpAuthz,*firstAuthz,*preAuthz;
	int idx=0;

	nowauthz=authzList;
	while(nowauthz!=NULL){
		newauthz=(SipPxyAuthz*)calloc(1,sizeof(*newauthz));
		newauthz->numParams=nowauthz->numParams;
		newauthz->next=NULL;
		if(nowauthz->scheme != NULL)
			newauthz->scheme=strDup((const char*)nowauthz->scheme);
		else
			newauthz->scheme=NULL;
		newauthz->sipParamList=sipParameterDup(nowauthz->sipParamList,nowauthz->numParams);
		(idx==0) ? (firstAuthz=newauthz):(preAuthz->next=newauthz);
		preAuthz=newauthz;
		nowauthz=nowauthz->next;
		idx++;
	}
	(idx>0) ? ( tmpAuthz=firstAuthz) : (tmpAuthz=NULL);
	return tmpAuthz;
}


SipRfc1123Date* rfc1123DateDup(SipRfc1123Date *date)
{
	SipRfc1123Date *tmpDate,*newDate;

	tmpDate=date;
	if(tmpDate != NULL){
		newDate=(SipRfc1123Date*)calloc(1,sizeof(SipRfc1123Date));
		*newDate = *tmpDate;
		
		strncpy(newDate->year,tmpDate->year,5);
		newDate->month=tmpDate->month;
		newDate->weekday=tmpDate->weekday;
		strncpy(newDate->day,tmpDate->day,3);
		strncpy(newDate->time,tmpDate->time,9);
	}
	else
		newDate=NULL;

	return newDate;
}

SipViaParm* sipViaParmDup(SipViaParm* viaPara,int numPara)
{
	int idx;
	SipViaParm *tmpVia,*newVia,*preVia,*firstVia;

	tmpVia=viaPara;
	for(idx=0;idx<numPara;idx++){
		if(tmpVia==NULL) break;
		newVia=(SipViaParm*)calloc(1,sizeof(SipViaParm));
		newVia->next=NULL;

		(tmpVia->protocol == NULL)?(newVia->protocol= NULL):(newVia->protocol=strDup((const char*)tmpVia->protocol));
		(tmpVia->version  == NULL)?(newVia->version = NULL):(newVia->version =strDup((const char*)tmpVia->version));
		(tmpVia->transport== NULL)?(newVia->transport=NULL):(newVia->transport=strDup((const char*)tmpVia->transport));
		(tmpVia->sentby   == NULL)?(newVia->sentby  = NULL):(newVia->sentby=strDup((const char*) tmpVia->sentby));
		newVia->hidden	=tmpVia->hidden;
		newVia->ttl	=tmpVia->ttl;
		newVia->rport =tmpVia->rport;
		(tmpVia->maddr    == NULL)?(newVia->maddr    = NULL):(newVia->maddr   =strDup((const char*)tmpVia->maddr));
		(tmpVia->received == NULL)?(newVia->received = NULL):(newVia->received=strDup((const char*)tmpVia->received));
		(tmpVia->branch   == NULL)?(newVia->branch   = NULL):(newVia->branch  =strDup((const char*)tmpVia->branch));
		(tmpVia->comment  == NULL)?(newVia->comment  = NULL):(newVia->comment =strDup((const char*)tmpVia->comment));
		/* add by tyhuang 2003.9.4 */
		if(tmpVia->UnSupportparamList)
			newVia->UnSupportparamList=sipParameterDup(tmpVia->UnSupportparamList,tmpVia->numofUnSupport);
		else
			newVia->UnSupportparamList=NULL;
		newVia->numofUnSupport=tmpVia->numofUnSupport;
		(idx==0) ? (firstVia=preVia=newVia):(preVia->next=newVia);
		preVia=newVia;
		tmpVia=tmpVia->next;
	}
	(idx>0) ? (tmpVia = firstVia) : (tmpVia=NULL);

	return tmpVia;
}

SipChllng* sipChallengeDup(SipChllng* challPara,int numChalleng)
{
	int idx;
	SipChllng *tmpChall,*newChall,*preChall,*firstChall;

	tmpChall=challPara;
	for(idx=0;idx<numChalleng;idx++){
		if(tmpChall == NULL) break;

		newChall=(SipChllng*)calloc(1,sizeof(SipChllng));
		newChall->next=NULL;
		newChall->scheme=strDup((const char*)tmpChall->scheme);
		newChall->numParams=tmpChall->numParams;
		newChall->sipParamList=sipParameterDup(tmpChall->sipParamList,tmpChall->numParams);
		(idx==0) ? (firstChall=newChall) : (preChall->next=newChall);
		preChall=newChall;
		tmpChall=tmpChall->next;
	}
	(idx >0 ) ? (tmpChall=firstChall) : (tmpChall=NULL);
	return tmpChall;

}
SipWrnVal* sipWarningDup(SipWrnVal *warning,int numWarning)
{
	int idx;
	SipWrnVal *tmpWarn,*newWarn,*preWarn,*firstWarn;

	tmpWarn=warning;
	for(idx=0;idx<numWarning;idx++){
		if(tmpWarn== NULL) 
			break;

		newWarn=(SipWrnVal*)calloc(1,sizeof(SipWrnVal));
		newWarn->next=NULL;

		newWarn->code=(unsigned char*)strDup((const char*)tmpWarn->code);
		newWarn->agent=(unsigned char*)strDup((const char*)tmpWarn->agent);
		newWarn->text=(unsigned char*)strDup((const char*)tmpWarn->text);
		(idx==0) ? (firstWarn=newWarn) : (preWarn->next = newWarn);
		preWarn=newWarn;
		tmpWarn=tmpWarn->next;
	}
	(idx>0) ? (tmpWarn = firstWarn) : (tmpWarn=NULL);
	return tmpWarn;
}
/*----------------------------------------------*/
/*-------------- Memory Free Functions ---------*/
/*----------------------------------------------*/
int sipParameterFree(SipParam* para)
{
	SipParam *tmpPara,*nextPara;

	tmpPara=para;
	while(tmpPara!=NULL){
		if(tmpPara->name != NULL){  
			free(tmpPara->name);
			tmpPara->name=NULL;
		}
		if(tmpPara->value!= NULL){
			free(tmpPara->value);
			tmpPara->name=NULL;
		}
		nextPara=tmpPara->next;
		free(tmpPara);
		tmpPara=nextPara;
	}
	para=NULL;
	return 1;
}

int sipMediaTypeFree(SipMediaType *media)
{
	SipMediaType *tmpMedia,*nextMedia;

	tmpMedia=media;
	while(tmpMedia!=NULL){
		if(tmpMedia->type != NULL) 
			free(tmpMedia->type);
		if(tmpMedia->subtype != NULL) 
			free(tmpMedia->subtype); 
		sipParameterFree(tmpMedia->sipParamList);
		nextMedia=tmpMedia->next;
		free(tmpMedia);
		tmpMedia=nextMedia;
	}
	return 1;
}

int sipAcceptFree(SipAccept *accept)
{
	if(accept!= NULL){
		sipMediaTypeFree(accept->sipMediaTypeList);
		free(accept);
	}
	return 1;
}

int sipCodingFree(SipCoding *coding)
{
	SipCoding *tmpCoding,*nextCoding;

	tmpCoding=coding;
	while(tmpCoding != NULL){
		if(tmpCoding->coding != NULL) 
			free(tmpCoding->coding);
		if(tmpCoding->qvalue != NULL) 
			free(tmpCoding->qvalue);
		nextCoding=tmpCoding->next;
		free(tmpCoding);
		tmpCoding=nextCoding;
	}
	return 1;
}

int sipLanguageFree(SipLang *lang)
{
	SipLang *tmpLang,*nextLang;

	tmpLang=lang;
	while(tmpLang != NULL){
		if(tmpLang->lang != NULL) free(tmpLang->lang);
		if(tmpLang->qvalue != NULL) free(tmpLang->qvalue);
		nextLang=tmpLang->next;
		free(tmpLang);
		tmpLang=nextLang;
	}
	return 1;
}

int sipAddrFree(SipAddr *addr)
{
	SipAddr *tmpAddr,*nextAddr;

	tmpAddr=addr;
	while(tmpAddr!=NULL){
		if(tmpAddr->display_name != NULL) 
			free(tmpAddr->display_name);
		if(tmpAddr->addr != NULL) {
			/*
			if(tmpAddr->with_angle_bracket) 
				free(tmpAddr->addr);
			else free(tmpAddr->addr);
			*/
			free(tmpAddr->addr);
		}
		nextAddr=tmpAddr->next;
		free(tmpAddr);
		tmpAddr=nextAddr;
	}
	return 1;
}

int sipRecAddrFree(SipRecAddr *addr)
{
	SipRecAddr *tmpAddr,*nextAddr;

	tmpAddr=addr;
	while(tmpAddr!=NULL){
		if(tmpAddr->display_name != NULL){
			free(tmpAddr->display_name);
			tmpAddr->display_name=NULL;
		}
		if(tmpAddr->addr != NULL) {
			free(tmpAddr->addr);
			tmpAddr->addr=NULL;
		}
		if(tmpAddr->sipParamList){
			sipParameterFree(tmpAddr->sipParamList);
			tmpAddr->sipParamList=NULL;
		}
		nextAddr=tmpAddr->next;
		free(tmpAddr);
		tmpAddr=nextAddr;
	}
	return 1;
}

int sipContactElmFree(SipContactElm *contact)
{
	SipContactElm *tmpElm,*nextElm;

	tmpElm=contact;
	while(tmpElm != NULL){
		if(tmpElm->comment != NULL){ 
			free(tmpElm->comment);
			tmpElm->comment=NULL;
		}
		if(tmpElm->address != NULL) {
			sipAddrFree(tmpElm->address);
			tmpElm->address=NULL;
		}
		if(tmpElm->ext_attrs != NULL){ 
			sipParameterFree(tmpElm->ext_attrs);
			tmpElm->ext_attrs=NULL;
		}
		/*tmpElm->expireDate didn't need free, since it is a static memory allocate*/
		nextElm=tmpElm->next;
		free(tmpElm);
		tmpElm=nextElm;
	}
	return 1;
}

int sipStrFree(SipStr *str)
{
	SipStr *tmpStr,*nextStr;

	tmpStr=str;
	while(tmpStr != NULL){
		if(tmpStr->str != NULL) {
			free(tmpStr->str);
			tmpStr->str=NULL;
		}
		nextStr=tmpStr->next;
		free(tmpStr);
		tmpStr=nextStr;
	}
	return 1;
}

int sipViaParmFree(SipViaParm *para)
{
	SipViaParm *tmpPara,*nextPara;

	tmpPara=para;
	while(tmpPara!=NULL){
		if(tmpPara->protocol != NULL) {
			free(tmpPara->protocol);
			tmpPara->protocol=NULL;
		}
		if(tmpPara->version  != NULL){
			free(tmpPara->version);
			tmpPara->version=NULL;
		}
		if(tmpPara->transport!= NULL){
			free(tmpPara->transport);
			tmpPara->transport=NULL;
		}
		if(tmpPara->sentby   != NULL) {
			free(tmpPara->sentby);
			tmpPara->sentby=NULL;
		}
		if(tmpPara->maddr    != NULL) {
			free(tmpPara->maddr);
			tmpPara->maddr=NULL;
		}
		if(tmpPara->received != NULL) {
			free(tmpPara->received);
			tmpPara->received=NULL;
		}
		if(tmpPara->branch   != NULL){
			free(tmpPara->branch);
			tmpPara->branch=NULL;
		}
		if(tmpPara->comment  != NULL){
			free(tmpPara->comment);
			tmpPara->comment=NULL;
		}
		/* add by tyhuang 2003.7.22 */
		tmpPara->rport=0;
		if(tmpPara->UnSupportparamList){
			sipParameterFree(tmpPara->UnSupportparamList);
			tmpPara->UnSupportparamList=NULL;
		}
		nextPara=tmpPara->next;
		free(tmpPara);
		tmpPara=nextPara;
	}
	return 1;
}

int sipChallengeFree(SipChllng *challeng)
{
	SipChllng *tmpChg,*nextChg;

	tmpChg=challeng;
	while(tmpChg != NULL){
		if(tmpChg->scheme != NULL){
			free(tmpChg->scheme);
			tmpChg->scheme=NULL;
		}
		if(tmpChg->sipParamList != NULL) {
			sipParameterFree(tmpChg->sipParamList);
			tmpChg->sipParamList=NULL;
		}
		nextChg=tmpChg->next;
		free(tmpChg);
		tmpChg=nextChg;
	}
	return 1;
}

