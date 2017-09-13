/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * ua_cm.h
 *
 * $Id: ua_cm.h,v 1.130 2006/03/23 11:57:49 tyhuang Exp $
 */

#ifndef UA_CM_H
#define UA_CM_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <sip/sip_cm.h>
#include <adt/dx_lst.h>

#define TCRPrintEx(X,Y)	TCRPrint ( X, "[UACORE] %s \n", Y );
#define TCRPrintEx1(X,Y,Z)	TCRPrint ( X, "[UACORE] %s \n" Y, Z );

#define UaCoreWARN(W) TCRPrint ( TRACE_LEVEL_WARNING, "[UACORE] WARNING: %s",W )
#define UaCoreERR(W) TCRPrint ( TRACE_LEVEL_ERROR, "[UACORE] ERROR: %s (at %s, %d)\n",W, __FILE__, __LINE__)
/*#define UaCoreERR1(W,Y) TCRPrint ( TRACE_LEVEL_ERROR, "[UACORE] ERROR: %s (at %s, %d)\n" W,Y, __FILE__, __LINE__)*/

#define SIP_VERSION	"SIP/2.0"

/*
#define USER_AGENT	"CCL_SIP_SOFTPHONE_1.6.7"
*/
#define USER_AGENT	"CCLSP"
	
#define DEFAULT_EXPIRES 180 /* sec, default expire for invite if 0 means off*/
#define T1		500 /* t1:500msec */
#define	EXPIRE_DEFAULT  360 /* default expire for register if 0 means off */
#define MAX_FORWARDS	70

#define SESSION_TIMER_INTERVAL 1800 /* default session expire and recommended value is 1800 if 0 means off */
#define MINSE_EXPIRES 150  /* Min-SE */

#define EXPIRE_DURATION  10
	
#define HOLD_VERISON 1 /* 0:rfc2543,1:rfc3261 */

#define DEFAULT_LANGUAGE "en"
#define DEFAULT_ENCODING ""
#define SUPPORT_LIST "replaces,timer"
#define SUPPORT_LIST_NO_TIMER "replaces" /* without "timer" */

#ifndef SIP_SUPPORT_METHOD_NUM	
#define SIP_SUPPORT_METHOD_NUM	UNKNOWN_METHOD
#endif

#ifndef MAXADDRESIZE
#define MAXADDRESIZE 128
#endif

#ifndef MAXBUFFERSIZE
#define MAXBUFFERSIZE 256
#endif
#ifndef MAXDNSNAMESIZE
#define MAXDNSNAMESIZE 1024
#endif

#define MAX_TX_COUNT 1024

#define AutoCreateMgr 0 /* add by tyhuang 2003.5.30 */
typedef DxLst CodecLst;
typedef DxLst uaSDPMediaLst;

typedef struct uaDlgObj		UaDlgS;
typedef struct uaDlgObj*	UaDlg;
typedef struct uaMgrObj*	UaMgr;
typedef struct uaCfgObj*	UaCfg;
typedef struct uaUserObj*	UaUser;
typedef struct uaMsgObj*	UaMsg;
typedef struct uaAuthzObj*	UaAuthz;
typedef struct uaCodecObj*	UaCodec;
typedef struct uaContentObj*	UaContent;
typedef struct evtpkgObj*	UaEvtpkg;
typedef struct uaclassObj*	UaClass;
typedef struct substateObj*	SubscriptionState;
typedef struct uasubObj*	UaSub;
typedef struct uaMediaObj*	UaSDPMedia;

typedef enum{
	UAMSG_INFO,		/* Automatic generate message, e.q. reINVITE, */
	UAMSG_CALLSTATE,	/* Call State Change */
	UAMSG_REPLY,		/* For Asynchronous response */
	UAMSG_REFER,		/* New REFER message coming*/
	UAMSG_REFER_FAIL,
	UAMSG_TX_TIMEOUT,	/* Transaction time(Timer B,F,H)out and enter terminated state*/
	UAMSG_TX_TERMINATED,	/* Transaction terminated */
	UAMSG_TX_TRANSPORTERR,   /* Transport error */
	UAMSG_MESSAGE,
	UAMSG_MESSAGE_FAIL,
	UAMSG_SUBSCRIBE,
	UAMSG_SUBSCRIBE_FAIL,
	UAMSG_PUBLISH,
	UAMSG_PUBLISH_FAIL,
	UAMSG_PUBLISH_CONDITIONAL_FAIL,
	UAMSG_NOTIFY,
	UAMSG_NOTIFY_FAIL,
	UAMSG_REGISTER,
	UAMSG_REGISTER_EXPIRED,
	UAMSG_SIP_INFO,		/* receive sip INFO message */
	UAMSG_SIP_INFO_RSP,
	UAMSG_SIP_INFO_FAIL,
	UAMSG_SIP_UPDATE,
	UAMSG_SIP_UPDATE_RSP,
	UAMSG_SIP_UPDATE_FAIL,
	UAMSG_SIP_PRACK,
	UAMSG_SIP_PRACK_RSP,
	UAMSG_SIP_PRACK_FAIL,
	UAMSG_REQUIRE_PRACK,		/* Receive reliable provision ack */
	UAMSG_UNATTENDED_TRANSFER,/*unattended transfer*/
	UAMSG_ATTENDED_TRANSFER,  /*attended transfer call*/
	UAEVT_NULL
}UAEvtType;

typedef enum {
	WWW_AUTHN = 0x1,
	PROXY_AUTHN = 0x2,
	AUTHN_UNKNOWN
}UAAuthnType;

typedef enum {
	AUTHZ	= 0x1,
	PROXY_AUTHZ	= 0x2,
	AUTHZ_UNKNOWN
}UAAuthzType;

typedef enum {
	UA_REGISTER=0,
	UA_UNREGISTER,
	UA_UNREGISTERALL,/* add by tyhuang */
	UA_REGISTER_QUERY,
	UA_REGISTERSIPS,
	UA_UNREGISTERSIPS,
	UA_UNREGISTERALLSIPS,
	UA_REGISTERSIPS_QUERY
}UARegisterType;


typedef enum{
	UACSTATE_IDLE,
	UACSTATE_DIALING,
	UACSTATE_PROCEEDING,
	UACSTATE_RINGBACK,
	UACSTATE_BUSY,
	UACSTATE_REJECT,	/*Occured after send Reject*/
	UACSTATE_CANCEL,
	UACSTATE_CONNECTED,
	UACSTATE_PROXYAUTHN_FAIL,  /*proxy authenticate error*/
	UACSTATE_DISCONNECT,
	UACSTATE_OFFERING,	   /*a new coming call, application should send 180 and ring user */
	UACSTATE_OFFERING_REPLACE, /*contain replace heaer, user should not ring*/
	UACSTATE_ACCEPT,
	UACSTATE_REJECTED,	/*Receive Reject from Callee*/
	UACSTATE_SDPCHANGING,	/*on SDP-CHANGING procedure */
	UACSTATE_ONHOLDING,	/*on holding,waitfor response*/
	UACSTATE_ONHELD,	/*hold finished, on hold state*/
	UACSTATE_RETRIEVING,	/*to retrieve a call */
	UACSTATE_UNHOLD,	/*Get reINVITE, Unhold dlg*/
	UACSTATE_ONHOLDPENDCONF,
	UACSTATE_ONHOLDPENDTRANSFER,
	UACSTATE_CONFERENCE,
	UACSTATE_SPECIALINFO,
	UACSTATE_REGISTER,	/*doing register*/
	UACSTATE_REGISTER_AUTHN,/*doing authorization, since receive WWW-Authenticate*/
	UACSTATE_REGISTERED,	/*finished registration*/
	UACSTATE_REGISTERFAIL,
	UACSTATE_REGISTERTIMEOUT,
	UACSTATE_REGISTERERR,
	UACSTATE_TIMEOUT,
	UACSTATE_TRANSPORTERR, 
	UACSTATE_UNKNOWN
}UACallStateType;

typedef enum{
	UA_MEDIA_AUDIO,
	UA_MEDIA_VIDEO,
/*	UA_MEDIA_APPLICATION,
	UA_MEDIA_DATA,
*/
	UA_MEDIA_UNKNOWN
}UAMediaType;

typedef enum {
	/*UA_ALL_MEDIA= -1,*/

	UA_WAV_PCMU = 0, 
	UA_WAV_PCMA = 8, 
	UA_WAV_GSM  = 3, 
	UA_WAV_723  = 4, 
	UA_WAV_729  =18, 
	UA_WAV_UNKNOWN=1024
}UAAudioType;

typedef enum {
	
	UA_VIDEO_H261= 31,
	UA_VIDEO_H263= 34,
	UA_VIDEO_UNKNOWN = 1024
}UAVideoType;


typedef enum{
	UAMEDIA_SENDRECV,	/*default*/
	UAMEDIA_INACTIVE,
	UAMEDIA_RECVONLY,
	UAMEDIA_SENDONLY,
	UAMEDIA_UNKNOWN
}UAMediaAttr;

typedef enum {			/*RC_BASE_SIPUA = 7000*/
	UAMGR_ERROR		=RC_BASE_SIPUA,
	UAMGR_NULL,
	UAMGR_SET_CFG_ERROR,
	UAMGR_INIT_CB_FUNC_NULL,
	UAMGR_DLG_NOT_FOUND,
	UAMGR_DLGLST_CREATE_ERROR,
	UAMGR_DLGLST_NULL,
	UADLG_ERROR		=RC_BASE_SIPUA+100,
	UADLG_NULL,
	UADLG_PARENT_MGR_NULL,
	UADLG_CALLID_NULL,
	UADLG_USERID_NULL,
	UADLG_REMOTEID_NULL,
	UADLG_REMOTEADDR_NULL,
	UADLG_REMOTETARGET_NULL,
	UADLG_CALLSTATE_UNMATCHED,
	UADLG_CALLSTATE_UNHELD,
	UADLG_ORIGSDP_NULL,
	UADLG_GEN_BYEREQ_FAIL,
	UADLG_GEN_REINV_FAIL,
	UADLG_GEN_REFREQ_FAIL,
	UADLG_TXMSG_NOT_FOUND,
	UADLG_TXMSG_REQ_NULL,
	UADLG_TXCOUNT_ZERO,
	UAUSER_ERROR		=RC_BASE_SIPUA+200,
	UAUSER_NULL,
	UAUSER_LOCAL_ADDR_NULL,
	UAUSER_HOST_NULL,
	UAUSER_CONTACT_NULL,
	UAUSER_MAXFORWARD_TOO_SMALL,
	UAUSER_AUTHZ_NULL,
	UACFG_ERROR		=RC_BASE_SIPUA+300,
	UACFG_NULL,
	UASIPMSG_ERROR		=RC_BASE_SIPUA+400,
	UASIPMSG_REQ_NULL,
	UASIPMSG_RSP_NULL,
	UASIPMSG_REQLINE_NULL,
	UASIPMSG_RSPLINE_NULL,
	UASIPMSG_CSEQ_NULL,
	UASIPMSG_FROM_NULL,
	UASIPMSG_TO_NULL,
	UASIPMSG_ROUTE_NULL,
	UASIPMSG_WWWAUTHN_NULL,
	UASIPMSG_GENACK_FAIL,
	UASDP_ERROR		=RC_BASE_SIPUA+500,
	UASDP_NULL,
	UACODEC_LST_NULL,
	UACODEC_TYPE_ERR,
	UACODEC_MEDIA_NOT_FOUND,
	UATX_ERROR		=RC_BASE_SIPUA+600,
	UATX_NULL,
	UATX_USER_PROFILE_NULL,
	UATX_TX_NOT_FOUND,/*transaction can't find */
	UATX_PARENTDLG_NOT_FOUND,/*can't find transaction's parent dialog*/
	UATX_REQ_NOT_FOUND,/*transaction can't find a request message*/
	UATX_RSP_NOT_FOUND,/*transaction can't find a response message*/
	UATX_ACK_NOT_FOUND,/*transaction can't find a ACK message*/
	UATX_REQ_MSG_ERROR,/*transaction message contain a error req msg*/
	UATX_RSP_MSG_ERROR,/*transaction message contain a error rsp msg*/
	UATX_AUTHZ_MSG_GEN_ERROR,/*generate register authenticate message error */
	UATX_REQMETHOD_UNKNOWN,
	UATX_RSPMETHOD_UNKNOWN,
	UATX_DLG_NULL,
	UATX_MGR_NULL,
	UAMSG_NULL =RC_BASE_SIPUA+700,
	UAMSG_DLG_NULL,
	UAMSG_DLG_TX_NULL,	/*dialog didn't contain any transaction*/
	UAMSG_DLG_TX_MSG_NULL,	/*transaction didn't contain sip message*/
	UAMSG_CONTACT_NULL,
	UAMSG_URL_NULL,
	UACORE_ERROR =RC_BASE_SIPUA+800,
	UACORE_USER_NOT_FOUND,
	UACODEC_ERROR,
	UABUFFER_TOO_SMALL,
	UAUSER_PROFILE_NULL
}UARetCode;

/* Define return status code */
typedef enum
{
    Trying=100,
    Ringing=180,
    Call_Is_Being_Forwarded=181,
    Queued=182,
    Session_Progress=183,
    sipOK=200,		/*Since conflict with RTP definition */
    Accepted=202,
    Multiple_Choice=300,
    Moved_Permanently=301,
    Moved_Temporarily=302,
    User_Proxy=305,
    Alternative_Service=380,
    Bad_Request=400,
    Unauthorized=401,  
    Payment_Required =402,  
    Forbidden=403, 
    Not_Found=404,  
    Method_Not_Allowed=405,  
    Client_Not_Acceptable=406,  
    Proxy_Authentication_Required=407,  
    Request_Timeout=408, 
    Conflict=409,  
    Gone=410,  
    Length_Required=411,
	Conditional_Request_Failed=412,
    Request_Entity_Too_Large=413,  
    Request_URI_Too_Large=414, 
    Unsupported_Media_Type=415,
    Unsupported_URI_Scheme=416,
    Bad_Extension=420,
    Extension_Required=421,
	Session_Inerval_Too_Small=422,
    Interval_Too_Brief=423,
	Provide_Referrer_Identity=429,
    Temporarily_not_available=480,  
    Call_Leg_Transaction_Does_Not_Exist=481, 
    Loop_Detected=482,  
    Too_Many_Hops=483,  
    Address_Incomplete=484,  
    Ambiguous=485,  
    Busy_Here=486,
    Request_Terminated=487,  
    Not_Acceptable_Here=488,
    Bad_Event=489,
    Request_Pending=491,
    Undecipherable=493,
    Internal_Server_Error=500,  
    Not_Implemented=501,  
    Bad_Gateway=502,  
    Service_Unavailable=503,  
    Gateway_Time_out=504,  
    SIP_Version_not_supported=505,
    Message_Too_Large=513,
    Busy_Everywhere=600,  
    Decline=603,  
    Does_not_exist_anywhere=604,  
    Not_Acceptable=606,
    UnKnown_Status=999
}UAStatusCode;

typedef enum {
	UASUB_NULL = -1,
	UASUB_ACTIVE,
	UASUB_PENDING,
	UASUB_TERMINATED
}UASubState;

/*translate Error Code into Reason-Phase*/
CCLAPI char* uaCMRet2Phase(UARetCode);

/*get sdp attribute type*/
UAMediaAttr uaSdpGetMediaAttr(IN char*);

/*transfer codec into string, input buf and buf-size*/
RCODE MediaCodectoStr(IN CodecLst,IN OUT char*,IN int);

/*transfer media attribute into string*/
char* MediaAttributetoStr(IN UAMediaAttr);

/*transfer media attribute into string*/
char* MediaTypetoStr(IN UAMediaType);
/*get sdp attribute type*/
UAMediaType uaSdpGetMediaType(IN char*);
 
/*input UAAudioType or UAVideoType*/
char*   MediaCodectoPayload(IN int);

/* input codec specified number to inner codec typecode */
int	MediaCodectoNum(IN const char*);

/*input UAAudioType or UAVideoType, return descriptin*/
char*	MediaCodectoExplan(IN int);

/*transfer codec string into type */
UAAudioType StrtoAutioCodec(IN const char*);

/*get tickcount() */
unsigned long uagettickcount(void);

#ifdef  __cplusplus
}
#endif

#endif /* UA_CM_H */
