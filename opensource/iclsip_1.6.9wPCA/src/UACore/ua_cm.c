/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 *
 * 
 * ua_cm.c
 *
 * $Id: ua_cm.c,v 1.52 2005/10/27 01:05:06 tyhuang Exp $
 */

#include <sip/sip_cm.h>
#include <sip/sip.h>
#include "ua_cm.h"
#include <adt/dx_lst.h>
#include "ua_sdp.h"
#ifdef UNIX
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#else
#include <windows.h>
#endif

/*translate Error Code into Reason-Phase*/
CCLAPI char* uaCMRet2Phase(UARetCode rCode)
{
	char* result=NULL;
	switch(rCode){
	case UAMGR_ERROR:
		return "UA Manager Object Error!";
	case UAMGR_NULL:
		return "UA Manager NULL!";
	case UAMGR_SET_CFG_ERROR:
		return "UA Manager object configuration Error!";
	case UAMGR_INIT_CB_FUNC_NULL:
		return "UA initialize didn't contain callback function!";
	case UAMGR_DLG_NOT_FOUND:
		return "UA Manager can't find Dialog!";
	case UAMGR_DLGLST_CREATE_ERROR:
		return "UA Dialog create List Error!";
	case UAMGR_DLGLST_NULL:
		return "UA Dialog list didn't exist!";
	case UADLG_ERROR:	
		return "UA Dialog error!";
	case UADLG_NULL:
		return "UA Dialog NULL!";
	case UADLG_PARENT_MGR_NULL:
		return "UA Dialog can't find parent manager!";
	case UADLG_CALLID_NULL:
		return "UA Dialog Call-ID didn't exist!";
	case UADLG_USERID_NULL:
		return "UA Dialog can't find User!";
	case UADLG_REMOTEID_NULL:
		return "UA Dialog remote displayname is not found!";
	case UADLG_REMOTEADDR_NULL:
		return "UA Dialog remote address is not found!";
	case UADLG_REMOTETARGET_NULL:
		return "UA Dialog remote contact address is not found!";
	case UADLG_CALLSTATE_UNMATCHED:
		return "UA CallState didn't match!";
	case UADLG_CALLSTATE_UNHELD:
		return "UA CallState is not Held!";
	case UADLG_ORIGSDP_NULL:
		return "UA Dialog didn't contain SDP!";
	case UADLG_GEN_BYEREQ_FAIL:
		return "UA Dialog generate BYE message error!";
	case UADLG_GEN_REINV_FAIL:
		return "UA Dialog generate re-INVITE message error!";
	case UADLG_GEN_REFREQ_FAIL:
		return "UA Dialog generate REFER message error!";
	case UADLG_TXMSG_NOT_FOUND:
		return "UA Dialog can't find transaction!";
	case UADLG_TXMSG_REQ_NULL:
		return "UA Dialog can't find transaction Request message!";
	case UADLG_TXCOUNT_ZERO:
		return "UA Dialog can't access transaction!";
	case UAUSER_ERROR:
		return "UA User Error!";
	case UAUSER_NULL:
		return "UA User NULL!";
	case UAUSER_LOCAL_ADDR_NULL:
		return "UA User local address NULL!";
	case UAUSER_HOST_NULL:
		return "UA User host NULL!";
	case UAUSER_CONTACT_NULL:
		return "UA User Contact address NULL!";
	case UAUSER_MAXFORWARD_TOO_SMALL:
		return "UA User Max-Forwards setting too small!";
	case UAUSER_AUTHZ_NULL:
		return "UA User authorization data NULL!";
	case UACFG_ERROR:
		return "UA Cfg error!";
	case UACFG_NULL:
		return "UA Cfg object NULL!";
	case UASIPMSG_ERROR:
		return "UA SIP message error!";
	case UASIPMSG_REQ_NULL:
		return "UA SIP request message NULL!";
	case UASIPMSG_RSP_NULL:
		return "UA SIP resposne message NULL!";
	case UASIPMSG_REQLINE_NULL:
		return "UA SIP request-line NULL!";
	case UASIPMSG_RSPLINE_NULL:
		return "UA SIP status-line NULL!";
	case UASIPMSG_CSEQ_NULL:
		return "UA SIP CSeq header NULL!";
	case UASIPMSG_FROM_NULL:
		return "UA SIP From header NULL!";
	case UASIPMSG_TO_NULL:
		return "UA SIP To header NULL!";
	case UASIPMSG_ROUTE_NULL:
		return "UA SIP Route header NULL!";
	case UASIPMSG_WWWAUTHN_NULL:
		return "UA SIP WWW-Authenticate NULL!";
	case UASIPMSG_GENACK_FAIL:
		return "UA generate ACK message failure!";
	case UASDP_ERROR:
		return "UA SDP error!";
	case UASDP_NULL:
		return "UA SDP NULL!";
	case UACODEC_LST_NULL:
		return "UA Codec list NULL!";
	case UACODEC_TYPE_ERR:
		return "UA Codec type error!";
	case UACODEC_MEDIA_NOT_FOUND:
		return "UA media type not supported!";
	case UATX_ERROR:
	case UATX_NULL:
	case UATX_USER_PROFILE_NULL:
	case UATX_TX_NOT_FOUND:
	case UATX_PARENTDLG_NOT_FOUND:/*can't find transaction's parent dialog*/
	case UATX_REQ_NOT_FOUND:/*transaction can't find a request message*/
	case UATX_RSP_NOT_FOUND:/*transaction can't find a response message*/
	case UATX_ACK_NOT_FOUND:/*transaction can't find a ACK message*/
	case UATX_REQ_MSG_ERROR:/*transaction message contain a error req msg*/
	case UATX_RSP_MSG_ERROR:/*transaction message contain a error rsp msg*/
	case UATX_AUTHZ_MSG_GEN_ERROR:/*generate register authenticate message error */
	case UATX_REQMETHOD_UNKNOWN:
	case UATX_RSPMETHOD_UNKNOWN:
	case UATX_DLG_NULL:
	case UATX_MGR_NULL:
		return "UA Transaction Error!";
	case UAMSG_NULL:
	case UAMSG_DLG_NULL:
	case UAMSG_DLG_TX_NULL:	/*dialog didn't contain any transaction*/
	case UAMSG_DLG_TX_MSG_NULL:	/*transaction didn't contain sip message*/
	case UAMSG_CONTACT_NULL:
	case UAMSG_URL_NULL:
		return "UA callback message error!";
	case UACORE_ERROR:
	case UACORE_USER_NOT_FOUND:
	case UACODEC_ERROR:
	case UABUFFER_TOO_SMALL:
	case UAUSER_PROFILE_NULL:
		return "UA core error!";
	default:
		return "UA unknown error!";

	}


	return result;
}


/*input UAAudioType or UAVideoType*/
char*   MediaCodectoPayload(IN int type)
{
	switch(type){
	case UA_WAV_PCMU:	
		return	"0";
	case UA_WAV_PCMA:	
		return	"8";
	case UA_WAV_GSM:
		return	"3";
	case UA_WAV_723:
		return	"4";
	case UA_WAV_729:
		return	"18";

	case UA_VIDEO_H261:
		return	"31";
	case UA_VIDEO_H263:
		return	"34";
	default:		
		return	NULL;
	}
}

int	MediaCodectoNum(IN const char* type)
{
	/*switch(type){
	case 0:
		return UA_WAV_PCMU;
	case 8:
		return UA_WAV_PCMA;
	case 3:
		return UA_WAV_GSM;
	case 4:
		return UA_WAV_723;
	case 18:
		return UA_WAV_729;
	case 31:
		return UA_VIDEO_H261;
	case 34:
		return UA_VIDEO_H263;
	default:
		return UA_VIDEO_UNKNOWN;
	}*/

	if(strcmp(type,"0") == 0)
		return UA_WAV_PCMU;
	else if(strcmp(type,"8") == 0)
		return UA_WAV_PCMA;
	else if(strcmp(type,"3") == 0)
		return UA_WAV_GSM;
	else if(strcmp(type,"4") == 0)
		return UA_WAV_723;
	else if(strcmp(type,"18") == 0)
		return UA_WAV_729;
	else if(strcmp(type,"31") == 0)
		return UA_VIDEO_H261;
	else if(strcmp(type,"34") == 0)
		return UA_VIDEO_H263;
	else
		return atoi(type);
	
	/*if(strcmp(type,"0") == 0)
		return 0;
	else if(strcmp(type,"8") == 0)
		return 8;
	else if(strcmp(type,"3") == 0)
		return 3;
	else if(strcmp(type,"4") == 0)
		return 4;
	else if(strcmp(type,"18") == 0)
		return 18;
	else if(strcmp(type,"31") == 0)
		return 31;
	else if(strcmp(type,"34") == 0)
		return 34;
	else
		return atoi(type);*/
}

char*	MediaCodectoExplan(IN int type)
{
	switch(type){
	case UA_WAV_PCMU:	
		return	"PCMU";
	case UA_WAV_PCMA:	
		return	"PCMA";
	case UA_WAV_GSM:
		return	"GSM";
	case UA_WAV_723:
		return	"G723";
	case UA_WAV_729:
		return	"G729";

	case UA_VIDEO_H261:
		return	"H261";
	case UA_VIDEO_H263:
		return	"H263";
	default:		
		return	NULL;
	}
}

/*transfer media attribute into string*/
char* MediaAttributetoStr(IN UAMediaAttr _attr)
{

	switch(_attr){
	case 	UAMEDIA_SENDRECV:
		return "sendrecv";
	case	UAMEDIA_INACTIVE:
		return "inactive";
	case	UAMEDIA_RECVONLY:
		return "recvonly";
	case 	UAMEDIA_SENDONLY:
		return "sendonly";
	default:
		UaCoreERR("[MediaAttributetoStr] unKnow Media attribute\n");
		return NULL;
	}
}

/*get sdp attribute type*/
UAMediaAttr uaSdpGetMediaAttr(IN char* _attr)
{

	if(strICmp("recvonly",_attr)==0)
		return UAMEDIA_RECVONLY;
	if(strICmp("sendonly",_attr)==0)
		return UAMEDIA_SENDONLY;
	if(strICmp("inactive",_attr)==0)
		return UAMEDIA_INACTIVE;
	if(strICmp("sendrecv",_attr)==0)
		return UAMEDIA_SENDRECV;

	return UAMEDIA_UNKNOWN;

}

/*transfer media attribute into string*/
char* MediaTypetoStr(IN UAMediaType _type)
{

	if(_type==UA_MEDIA_AUDIO)
		return "audio";
	else if(_type==UA_MEDIA_VIDEO)
		return "video";
	else
		return "application";

}

/*get sdp attribute type*/
UAMediaType uaSdpGetMediaType(IN char* _type)
{

	if(strICmp("audio",_type)==0)
		return UA_MEDIA_AUDIO;
	if(strICmp("video",_type)==0)
		return UA_MEDIA_VIDEO;
	/*
	 	if(strICmp("application",_type)==0)
		return UA_MEDIA_APPLICATION;
		if(strICmp("data",_type)==0)
		return UA_MEDIA_DATA;
	 */

	return UA_MEDIA_AUDIO;

}

/*transfer codec into string*/
RCODE MediaCodectoStr(CodecLst _lst,char *buf, int buflen)
{
	RCODE rCode=RC_OK;
	DxStr tmp;
	int codecnum=0,pos,c;
	char *payload,ctype[4]="\0";
	UaCodec codec=NULL;


	if(_lst == NULL)
		return UACODEC_LST_NULL;

	/*get total number in list*/
	codecnum=dxLstGetSize(_lst);
	if(codecnum <= 0)
		return UACODEC_LST_NULL;

	tmp=dxStrNew();
	/*transfer each codec into string*/
	for(pos=0;pos<codecnum;pos++){
		codec=(UaCodec)dxLstPeek(_lst,pos);
		if(NULL == codec)
			break;
		payload=MediaCodectoPayload(uaCodecGetType(codec));
		if(payload != NULL){
			if(pos !=0 ) 
				dxStrCat(tmp," ");
			dxStrCat(tmp,payload);
		}else{ /* modify by tyhuang */
			c=uaCodecGetType(codec);
			if((c>0)&&(c<1000))
				i2a(c,ctype,10);
			if(pos !=0 ) 
				dxStrCat(tmp," ");
			dxStrCat(tmp,ctype);
		}
		codec=NULL;
	}/*finished add all codec string*/

	if(dxStrLen(tmp) > buflen){
		rCode=UABUFFER_TOO_SMALL;
	}else{
		strcpy(buf,dxStrAsCStr(tmp));
	}

	dxStrFree(tmp);
	return rCode;
}


const char* uaStatusCodeToString(IN UAStatusCode code)
{
	switch(code){
	case Trying : 
		return "Trying";	
		break;
	case Ringing:
		return "Ringing";	
		break;
	case Call_Is_Being_Forwarded:
		return "Call is being Forwarded"; 
		break;
	case Queued:
		return "Queued";	
		break;
	case Session_Progress:
		return "Session in progress"; 
		break;
	case sipOK:
		return "OK";		
		break;
	case Accepted:
		return "Accepted";
		break;
	case Multiple_Choice:
		return "Multiple Choice"; 
		break;
	case Moved_Permanently:
		return "Move Permanently"; 
		break;
	case Moved_Temporarily:
		return "Move Temporarily"; 
		break;
	case User_Proxy:
		return "User Proxy";	
		break;
	case Alternative_Service:
		return "Alternative Server"; 
		break;
	case Bad_Request:
		return "Bad Request";	
		break;
	case Unauthorized:
		return "Unauthorized";	
		break;
	case Payment_Required:
		return "Payment Required";	
		break;
	case Forbidden:
		return "Forbidden";	
		break;
	case Not_Found:
		return "Not Found";	
		break;
	case Method_Not_Allowed:
		return "Method Not Allowed";	
		break;
	case Client_Not_Acceptable:
		return "Client Not Acceptable";	
		break;
	case Proxy_Authentication_Required:
		return "Proxy Authenticate Required";	
		break;
	case Request_Timeout:
		return "Request Time Out";	
		break;
	case Conflict:
		return "Conflict";	
		break;
	case Gone:
		return "Gone";		
		break;
	case Length_Required:
		return "Length Required";		
		break;
	case Conditional_Request_Failed:
		return "Conditional Request Failed";
		break;
	case Request_Entity_Too_Large:
		return "Request Entity Too Large";	
		break;
	case Request_URI_Too_Large:
		return "Request URI Too large";		
		break;
	case Unsupported_Media_Type:
		return "Unsupported Media Type";	
		break;
	case Unsupported_URI_Scheme:
		return "Unsupported URI Scheme";
		break;
	case Bad_Extension:
		return "Bad Extension";	
		break;
	case Extension_Required:
		return "Extension Required";
		break;
	case Session_Inerval_Too_Small:
		return "Session Inerval Too Small";
		break;
	case Interval_Too_Brief:
		return "Interval Too Brief";
		break;
	case Provide_Referrer_Identity:
		return "Provide Referrer Identity";
		break;
	case Temporarily_not_available:
		return "Temporarily not available";	
		break;
	case Call_Leg_Transaction_Does_Not_Exist:
		return "Call Leg/Transaction Does Not Exist";	
		break;
	case Loop_Detected:
		return "Loop detected";		
		break;
	case Too_Many_Hops:
		return "Too many hops";		
		break;
	case Address_Incomplete:
		return "Address Incomplete";	
		break;
	case Ambiguous:
		return	"Ambiguous";		
		break;
	case Busy_Here:
		return  "Busy here";		
		break;
	case Request_Terminated:
		return "Request Terminated";	
		break;
	case Not_Acceptable_Here:
		return "Not Acceptable Here";	
		break;
	case Bad_Event:
		return "Bad Event";
		break;
	case Request_Pending:
		return "Request Pending";
		break;
	case Undecipherable:
		return "Undecipherable";
		break;
	case Internal_Server_Error:
		return "Internal Server Error";	
		break;
	case Not_Implemented:
		return	"Not Implemented";	
		break;
	case Bad_Gateway:
		return	"Bad Gateway";		
		break;
	case Service_Unavailable:
		return "Server Unavailable";	
		break;
	case Gateway_Time_out:
		return "Gateway Time Out";	
		break;
	case SIP_Version_not_supported:
		return "SIP Version NOT Supported"; 
		break;
	case Busy_Everywhere:
		return "Busy_Everywhere";	
		break;
	case Decline:
		return "Decline";		
		break;
	case Does_not_exist_anywhere:
		return "Does not exist anywhere";
		break;
	case Not_Acceptable:
		return "Not Acceptable";	
		break;
	default:
		if(code<200)
			return "trying";
		else if((code>200)&&(code<300))
			return "ok";
		else if((code>300)&&(code<400))
			return "redirect";
		else 
			return "failure";
		break;
	}
}

/*transfer sip method into string*/
const char* uaSipMethodToString(IN SipMethodType _type)
{
	switch(_type){
	case INVITE:
		return "INVITE";
		break;
	case ACK:
		return "ACK";
		break;
	case OPTIONS:
		return "OPTIONS";
		break;
	case BYE:
		return "BYE";
		break;
	case CANCEL:
		return "CANCEL";
		break;
	case REGISTER:
		return "REGISTER";
		break;
	case SIP_INFO:
		return "INFO";
		break;
	case PRACK:
		return "PRACK";
		break;
/* due to being expired. 2003.10.24
	case COMET:
		return "COMET";
		break;
*/
	case REFER:
		return "REFER";
		break;
	case SIP_SUBSCRIBE:
		return "SUBSCRIBE";
		break;
	case SIP_NOTIFY:
		return "NOTIFY";
		break;
	case SIP_MESSAGE:
		return "MESSAGE";
		break;
	case SIP_PUBLISH:
		return "PUBLISH";
		break;
	default:
		return "";
		break;
	}
}

unsigned long uagettickcount(void)
{
#ifdef	UNIX		/* [ for UNIX ] */
	struct timeval	tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
#elif	defined(WIN32)  /* [ for Windows ] */
	return GetTickCount();
#endif			/* [ #ifdef UNIX ] */

}
