/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * acc_sip.c
 *
 * $Id: acc_sip.c,v 1.46 2007/01/12 02:26:10 tyhuang Exp $
 */


#ifdef _WIN32
#include <conio.h>
#else
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sip/sip.h>
#include <low/cx_thrd.h>
#include <low/cx_misc.h>
#include "sipAcc.h"

#ifdef CCL_TLS

#define localaddr "140.96.102.205"
#define remoteaddr "140.96.102.77"

#else
#define localaddr "127.0.0.1"
#endif


SipReq		g_ReqMsg;
SipRsp		g_RspMsg;
SipConfigData	g_ConfigStr;
int		g_WhileDoing,g_TLSCONNECT,g_TCPCONNECT;
char pReqBuf[4096];
char pRspBuf[4096];

const char*	ReqMethod="INVITE";
const char*	ReqURI="sip:acer@140.96.102.238";
const char*	ReqSIPVersion="SIP/2.0";

const char*	RspSIPVersion="SIP/2.0";
const char*	RspStatusCode="404";
const char*	RspReason="Not Found";

const char*	AcceptHdr="Accept:application/sdp;level=1;q=0.5,application/x-private,text/html";
const char*	AcceptEncodingHdr="Accept-Encoding:gzip;q=1.0,identity;q=0.5,*;q=0";
const char*	AcceptLangHdr="Accept-Language:da,en-gb;q=0.8,en;q=0.7";
const char*	AcceptAuthzHdr="Authorization:Digest username=\"John\",\
		realm=\"MCI WorldCom SIP\",\
		nonce=\"ea9c8e88df84f1cec4341ae6cbe5a359\",opaque=\"\",\
		uri=\"sip:ss2.wcom.com\",response=\"dfe56131d1958046689cd83306477ecc\"";
const char*	CallIDHdr="Call-ID:f81d4fae-7dec-11d0-a765-00a0c91e6bf6@PUNTO.ccl.itri.org.tw";
const char*	ContactHdr="Contact:Acer<sip:client@user.com>";
const char*	ContactHdr1="Contact:<sip:+88234234@user1.com;user=phone>";
const char*	ContactHdr2="Contact:\"Mr. Watson\"<mailto:watson@bell-telephone.com>;q=0.1";

const char*	ContentEncodingHdr="Content-Encoding:gzip;q=1.0,identity;q=0.5,*;q=0";
const char*	ContentLengHdr="Content-Length:116";
const char*	ContentTypeHdr="Content-Type:application/html;charset=ISO-8859-4";
const char*	CSeqHdr="CSeq:1722 INVITE";
const char*	DateHdr="Date:Thu, 31 Aug 2000 11:23:05 GMT";
const char*	EncryptionHdr="Encryption:PGP version=2.6.2,encoding=ascii";
const char*	ExpiresHdr="Expires:3600";
const char*	FromHdr="From:<sip:hchsu@spring.ccl.itri.org.tw>;tag=123456789-111";
const char*	HideHdr="Hide:route";
const char*	MaxForwardsHdr="Max-Forwards:6";
const char*	OrganizationHdr="Organization: No example...";
const char*	PriorityHdr="Priority:non-urgent";
const char*	ProxyAuthzHdr="Proxy-Authorization:Digest username=\"John\",realm=\"MCI WorldCom SIP\",\
			nonce=\"ea9c8e88df84f1cec4341ae6cbe5a359\",opaque=\"\",\
			uri=\"sip:ss2.wcom.com\",response=\"dfe56131d1958046689cd83306477ecc\"";
const char*	ProxyRequireHdr="Proxy-Require:com.example.billing,saturn.bell-tel.com";
const char*	RecordRouteHdr="Record-Route:<sip:a.g.bell@bell-telephone.com>,<sip:a.bell@ieee.org>,sip:nothing@itri.org.tw,sip:acer@csie.nctu.edu.tw";
const char* ReferToHdr="Refer-To:<sip:dave@denver.example.org?Replaces=12345%40192.168.118.3%3Bto-tag%3D12345%3Bfrom-tag%3D5FFE-3994>";
const char*	RequireHdr="Require:com.example.billing,testing.example";
const char*	ResponseKeyHdr="Response-Key:pgp version=\"2.6.2\",encoding=\"ascii\"";
const char*	RetryAfterHdr="Retry-After:Mon, 01 Jan 2001 07:12:54 GMT(Dear John: Don't call me back, ever);duration=3600";
const char*	RouteHdr="Route:<sip:a.g.bell@bell-telephone.com>,<sip:a.bell@ieee.org>";
const char*	SessionExpiresHdr="Session-Expires:1800";
const char*	SubjectHdr="Subject: Tune in - they are talking about your work!";
const char*	TimestampHdr="Timestamp:123.456 0.789";
const char*	ToHdr="To:<sip:acer@ferio.ccl.itri.org.tw;tag=998373-9283>";
const char*	UserAgentHdr="User-Agent:CERN-LineMode/2.15 libwww/2.17b3";
const char*	ViaHdr="Via:SIP/2.0/TCP 140.96.120.222:5060";

const char*	ReqMsgBody="v=0\r\n\
o=Acer 2890844526 IN IP4 140.96.102.174\r\n\
s=Session SDP\r\n\
t=0 0\r\n\
m=audio -1 RTP/AVP 0 8 3\r\n\
a=rtpmap:0 PCMU/8000\r\n";

const char*	RspAccept="Accept:application/sdp;level=1;q=0.5,application/x-private,text/html";
const char*	RspAcceptEncoding="Accept-Encoding:gzip;q=1.0,identity;q=0.5,*;q=0";
const char*	RspAccpetLanguage="Accept-Language:da,en-gb;q=0.8,en;q=0.7";
const char*	RspAllow="Allow:INVITE,ACK,OPTIONS";
const char*	RspCallID="Call-ID:f81d4fae-7dec-11d0-a765-00a0c91e6bf6@PUNTO.ccl.itri.org.tw";
const char*	RspContact="Contact:\"Mr. Watson\"<sip:watson@worcester.bell-telephone.com>";
const char*	RspContentEncoding="Content-Encoding:gzip;q=1.0,identity;q=0.5,*;q=0";
const char*	RspContentLength="Content-Length:66";
const char*	RspContentType="Content-Type:text/html;charset=ISO-8859-4";
const char*	RspCSeq="CSeq:4711 INVITE";
const char*	RspDate="Date:Thu, 31 Aug 2000 11:23:05 GMT";
const char*	RspEncryption="Encryption:PGP version=2.6.2,encoding=ascii";
const char*	RspExpires="Expires:3600";
const char*	RspFrom="From:\"A. G. Bell\"<sip:agb@bell-telephone.com>;tag=9883742";
const char* RspMinSE="Min-SE:30";
const char*	RspOrganization="Organization: No example...";
const char*	RspProxyAutheticate="Proxy-Authenticate:Digest realm=\"MCI WorldCom SIP\",domain=\"wcom.com\",nonce=\"123\"";
const char*	RspRecordRoute="Record-Route:<sip:a.g.bell@bell-telephone.com>,<sip:a.bell@ieee.org>";
const char*	RspRetryAfter="Retry-After:Mon, 01 Jan 9999 00:00:00 GMT(Dear John: Don't call me back, ever);duration=3600";
const char*	RspServer="Server:CERN/3.0 libwww/2.17";
const char*	RspTimestamp="Timestamp:123.456 0.789";
const char*	RspTo="To:Anonymous<sip:+12125551212@server.phone2net.com>;tag=7654321";
const char*	RspUnsupported="Unsupported:com.example.billing";
const char*	RspUserAgent="User-Agent:CERN-LineMode/2.15 libwww/2.17b3";
const char*	RspWarning="Warning:307 isi.edu \"Session parameter 'foo' not understood\",309 acer.itri \"Another parameter\"";
const char*	RspVia="Via:SIP/2.0/UDP  first.example.com:4000;ttl=16;maddr=224.2.0.1;branch=a7c6a8d1ze(Example)";
const char*	RspWWWAutheticate="WWW-Authenticate:Digest realm=\"MCI WorldCom SIP\",domain=\"wcom.com\",nonce=\"ea9c8e88df84f1cec4341ae6cbe5a359\",opaque=\"\",stale=FALSE,algorithm=MD5";

const char*	RspBody="v=0\r\n\
o=userl 53655765 2353687637 IN IP4 128.3.4.5\r\n\
s=Mbone Audio\r\n";

/* IMS SIP headers */
#ifdef ICL_IMS

const char* PathHdr="Path:<sip:P3.EXAMPLEHOME.COM;lr>,<sip:P1.EXAMPLEVISITED.COM;lr>";
const char* RspPAU="P-Associated-URI:";
const char* RspServiceRouteHdr="Service-Route:<sip:P2.HOME.EXAMPLE.COM;lr>,<sip:HSP.HOME.EXAMPLE.COM;lr>";	

#endif
	
const char*	errMsgText = "";

/* SIP TCP CallBack Function */
void CBSipTCPEventHandle(SipTCP tcp, SipTCPEvtType event, SipMsgType msgtype, void* msg)
{
	char* tmp;

	switch(event) {
	case SIP_TCPEVT_DATA:{
		switch(msgtype) {
			case SIP_MSG_REQ:
				if(msg!=NULL){
					g_WhileDoing++;
					tmp=sipReqPrint((SipReq)msg);
					if(strncmp(tmp,pReqBuf,strlen(pReqBuf))!=0){
						printf("\n!!ERROR!! %s:%d: sipTCPSendReq() error.\n",__CMFILE__,__LINE__);
					}
					else
						printf("\n\t(sip protocol stack Receive TCP Request Message)........!!SUCCESS!!\n");
					sipReqFree((SipReq)msg);
				}
				break;
			case SIP_MSG_RSP:
				if(msg!=NULL){
					g_WhileDoing++;
					tmp=sipRspPrint((SipRsp)msg);
					if(strcmp(tmp,pRspBuf)!=0){
						printf("\n!!ERROR!! %s:%d: sipTCPSendRsp() error.\n",__CMFILE__,__LINE__);
					}
					else
						printf("\n\t(sip protocol stack Receive TCP Response Message).......!!SUCCESS!!\n");
					sipRspFree((SipRsp)msg);
				}
				break;
			default:
				break;
		}
		break;
	}
	case SIP_TCPEVT_CONNECTED:
		g_TCPCONNECT=1;
		break;
	case SIP_TCPEVT_SERVER_CLOSE:
		sipTCPFree(tcp);
		break;
	case SIP_TCPEVT_CLIENT_CLOSE:
		sipTCPServerFree(tcp);
	default:
		break;
	}
}

#ifdef CCL_TLS
/* SIP TLS CallBack Function */
void CBSipTLSEventHandle(SipTLS tls, SipTLSEvtType event, SipMsgType msgtype, void* msg)
{
	char* tmp;


	switch(event) {
	case SIP_TLSEVT_DATA:{
		if( !msg ) {
			sipTLSFree(tls);
			break;
		}
		switch(msgtype) {
			case SIP_MSG_REQ:
				if(msg!=NULL){
					g_WhileDoing++;
					tmp=sipReqPrint((SipReq)msg);
					printf("\nreq:%s\n",tmp);
					if(strncmp(tmp,pReqBuf,strlen(pReqBuf))!=0){
						printf("\n!!ERROR!! %s:%d: sipTLSSendReq() error.\n",__CMFILE__,__LINE__);
					}
					else
						printf("\n\t(sip protocol stack Receive TLS Request Message)........!!SUCCESS!!\n");
					sipReqFree((SipReq)msg);
				}
				break;
			case SIP_MSG_RSP:
				if(msg!=NULL){
					g_WhileDoing++;
					tmp=sipRspPrint((SipRsp)msg);
					if(strcmp(tmp,pRspBuf)!=0){
						printf("\n!!ERROR!! %s:%d: sipTLSSendRsp() error.\n",__CMFILE__,__LINE__);
					}
					else
						printf("\n\t(sip protocol stack Receive TLS Response Message).......!!SUCCESS!!\n");
					sipRspFree((SipRsp)msg);
				}
				break;
			default:
				break;
		}
		break;
	}

	case SIP_TLSEVT_CONNECTED:
		g_TLSCONNECT=1;
		break;
	case SIP_TLSEVT_SERVER_CLOSE:
	case SIP_TLSEVT_CLIENT_CLOSE:
	default:
		break;
	}
}

#endif

/* SIP UDP CallBack Function */
void CBSipUDPEventHandle(SipUDP udp, SipUDPEvtType event, SipMsgType msgtype,void* msg)
{
	char *tmp;

	switch(event) {
	case SIP_UDPEVT_DATA:
		switch(msgtype) {
		case SIP_MSG_REQ:
			if(msg!=NULL){
				g_WhileDoing++;
				tmp=sipReqPrint((SipReq)msg);
				if(strncmp(tmp,pReqBuf,strlen(pReqBuf))!=0){
					printf("\n!!ERROR!! %s:%d: sipUDPSendReq() error.\n",__CMFILE__,__LINE__);
				}
				else
					printf("\n\t(sip protocol stack Receive UDP Request Message).......!!SUCCESS!!\n");
				sipReqFree((SipReq)msg);
			}
			break;
		case SIP_MSG_RSP:
			if(msg!=NULL){
				g_WhileDoing++;
				tmp=sipRspPrint((SipRsp)msg);
				if(strcmp(tmp,pRspBuf)!=0){
					printf("\n!!ERROR!! %s:%d: sipTCPSendRsp() error.\n",__CMFILE__,__LINE__);
				}
				else
					printf("\n\t(sip protocol stack Receive UDP Response Message).......!!SUCCESS!!\n");
				sipRspFree((SipRsp)msg);
			}
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}


/* sip protocol stack initialization */
RCODE sipAccInit(void)
{
	RCODE result;

	g_TLSCONNECT=0;
	g_TCPCONNECT=0;
	g_ConfigStr.tcp_port=5060;
	//g_ConfigStr.log_flag = LOG_FLAG_CONSOLE;
	g_ConfigStr.log_flag = LOG_FLAG_FILE;
	//ConfigStr.log_flag = LOG_FLAG_FILE;
	strcpy(g_ConfigStr.fname, "sipacclog.txt");
	g_ConfigStr.trace_level = TRACE_LEVEL_API;
	strcpy(g_ConfigStr.laddr ,localaddr);


#ifndef CCL_TLS

	result=sipLibInit(SIP_TRANS_UDP |SIP_TRANS_TCP_SERVER| SIP_TRANS_TCP_CLIENT, 
			  CBSipTCPEventHandle, 
			  CBSipUDPEventHandle, 
			  g_ConfigStr);
#else
	result=sipLibInitWithTLS(SIP_TRANS_UDP |SIP_TRANS_TCP_SERVER| SIP_TRANS_TCP_CLIENT|SIP_TRANS_TLS_SERVER|SIP_TRANS_TLS_CLIENT, 
			  CBSipTCPEventHandle, 
			  CBSipUDPEventHandle,
			  CBSipTLSEventHandle,
			  5061,
			  g_ConfigStr);
#endif
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipLibInit() error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	else{
		printf("\n\t(sip protocol stack Initiated)..................!!SUCCESS!!\n");
		return RC_OK;
	}
}

/* sip protocol stack clean */
RCODE sipAccClean(void)
{
	RCODE result;


	result=sipLibClean();
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipLibClean() error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	else{
		printf("\n\t(sip protocol stack clean)......................!!SUCCESS!!\n");
		return RC_OK;
	}

}

RCODE sipReqNew_Test(void)
{
	/* sipReqNew is OK or not */
	g_ReqMsg=sipReqNew();
	if(g_ReqMsg == NULL){
		printf( "\n!!ERROR!! %s:%d: sipReqNew() error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	sipReqFree(g_ReqMsg);
	return RC_OK;
}

RCODE sipReqNewFromTxt_Test(void)
{
	FILE *pf;
	char* pMsg=NULL;

	/* sipReqMsg New From Txt */
	g_ReqMsg= NULL;

#ifdef ICL_IMS
	pf = fopen("IMSReqMsg","rb");
#else
	pf = fopen("ReqMsg","rb");
#endif
	
	memset(pReqBuf,0,4096);
	if (pf!=NULL){
		fread(pReqBuf,sizeof(char),4096,pf);
		/*printf("\n---------------Original-----------\n%s",pReqBuf);*/
		g_ReqMsg = sipReqNewFromTxt(pReqBuf);
		if(g_ReqMsg == NULL){
			printf( "\n!!ERROR!! %s:%d: sipReqNewFromTxt() error.\n",__CMFILE__,__LINE__);
			fclose(pf);
			return RC_ERROR;
		}
		fclose(pf);
	}
	else{
		printf( "\n!!ERROR!! %s:%d: sipReqNewFromTxt() reading request file error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}

	pMsg=sipReqPrint(g_ReqMsg);
	/*printf("\n\%sn",pMsg);*/
	if(strncmp(pMsg,pReqBuf,strlen(pReqBuf))!=0){
		printf( "\n!!ERROR!! %s:%d: sipReqNewFromTxt() Compare with orginal message error.\n",__CMFILE__,__LINE__);
		printf("origianl:\n%s\n",pReqBuf );
		printf("after   :\n%s\n",pMsg);
		return RC_ERROR;
	}
	sipReqFree(g_ReqMsg);

	printf("\n\t(sip protocol stack New Request Msg From Text)..........!!SUCCESS!!\n");
	return RC_OK;
}

RCODE sipReqModify_Test(void)
{
	RCODE result;
	SipReqLine *reqLine,*getLine;
	SipBody *pBody;
	char *pMsg;

	/* New a Request message, and add request line & headers */
	g_ReqMsg=sipReqNew();
	if(g_ReqMsg == NULL){
		printf( "\n!!ERROR!! %s:%d: sipReqNew() error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}

	reqLine=(SipReqLine*)malloc(sizeof(SipReqLine));
	reqLine->iMethod=INVITE;
	reqLine->ptrRequestURI=strDup(ReqURI);
	reqLine->ptrSipVersion=strDup(ReqSIPVersion);
	result=sipReqSetReqLine(g_ReqMsg,reqLine);
	free(reqLine->ptrRequestURI);
	free(reqLine->ptrSipVersion);
	free(reqLine);
	if(result!= RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqSetReqLine() error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	getLine=sipReqGetReqLine(g_ReqMsg);
	if((getLine->iMethod != INVITE) || 
	   (strcmp(getLine->ptrRequestURI,ReqURI)!=0) ||
	   (strcmp(getLine->ptrSipVersion,ReqSIPVersion)!=0)){
		printf( "\n!!ERROR!! %s:%d: sipReqGetReqLine() compare error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}

	printf("\n\t(sip protocol stack Add/Get Request Line)...............!SUCCESS!!\n");

	/* Add Message Header */
	result=sipReqAddHdrFromTxt(g_ReqMsg,Accept,(char*)AcceptHdr);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqAddHdrFromTxt() Accept Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}

	result=sipReqAddHdrFromTxt(g_ReqMsg,Accept_Encoding,(char*)AcceptEncodingHdr);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqAddHdrFromTxt() Accept_Encoding Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}

	result=sipReqAddHdrFromTxt(g_ReqMsg,Accept_Language,(char*)AcceptLangHdr);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqAddHdrFromTxt() Accept_Language Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}

	result=sipReqAddHdrFromTxt(g_ReqMsg,Authorization,(char*)AcceptAuthzHdr);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqAddHdrFromTxt() Authorization Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}

	result=sipReqAddHdrFromTxt(g_ReqMsg,Call_ID,(char*)CallIDHdr);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqAddHdrFromTxt() CallIDHdr Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipReqAddHdrFromTxt(g_ReqMsg,Contact,(char*)ContactHdr);
	result=sipReqAddHdrFromTxt(g_ReqMsg,Contact,(char*)ContactHdr1);
	result=sipReqAddHdrFromTxt(g_ReqMsg,Contact,(char*)ContactHdr2);

	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqAddHdrFromTxt() Contact Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipReqAddHdrFromTxt(g_ReqMsg,Content_Encoding,(char*)ContentEncodingHdr);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqAddHdrFromTxt() Content_Encoding Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipReqAddHdrFromTxt(g_ReqMsg,Content_Length,(char*)ContentLengHdr);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqAddHdrFromTxt() Content_Length Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}

	result=sipReqAddHdrFromTxt(g_ReqMsg,Content_Type,(char*)ContentTypeHdr);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqAddHdrFromTxt() Content_Type Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipReqAddHdrFromTxt(g_ReqMsg,CSeq,(char*)CSeqHdr);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqAddHdrFromTxt() CSeq Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipReqAddHdrFromTxt(g_ReqMsg,Date,(char*)DateHdr);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqAddHdrFromTxt() Date Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipReqAddHdrFromTxt(g_ReqMsg,Encryption,(char*)EncryptionHdr);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqAddHdrFromTxt() Encryption Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipReqAddHdrFromTxt(g_ReqMsg,Expires,(char*)ExpiresHdr);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqAddHdrFromTxt() Expires Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipReqAddHdrFromTxt(g_ReqMsg,From,(char*)FromHdr);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqAddHdrFromTxt() From Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipReqAddHdrFromTxt(g_ReqMsg,Hide,(char*)HideHdr);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqAddHdrFromTxt() Hide Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipReqAddHdrFromTxt(g_ReqMsg,Max_Forwards,(char*)MaxForwardsHdr);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqAddHdrFromTxt() Max_Forwards Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipReqAddHdrFromTxt(g_ReqMsg,Organization,(char*)OrganizationHdr);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqAddHdrFromTxt() Organization Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipReqAddHdrFromTxt(g_ReqMsg,Priority,(char*)PriorityHdr);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqAddHdrFromTxt() Priority Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipReqAddHdrFromTxt(g_ReqMsg,Proxy_Authorization,(char*)ProxyAuthzHdr);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqAddHdrFromTxt() Proxy_Authorization Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipReqAddHdrFromTxt(g_ReqMsg,Proxy_Require,(char*)ProxyRequireHdr);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqAddHdrFromTxt() Proxy_Require Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipReqAddHdrFromTxt(g_ReqMsg,Record_Route,(char*)RecordRouteHdr);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqAddHdrFromTxt() Record_Route Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipReqAddHdrFromTxt(g_ReqMsg,Refer_To,(char*)ReferToHdr);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqAddHdrFromTxt() Refer_To Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipReqAddHdrFromTxt(g_ReqMsg,Require,(char*)RequireHdr);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqAddHdrFromTxt() Require Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipReqAddHdrFromTxt(g_ReqMsg,Response_Key,(char*)ResponseKeyHdr);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqAddHdrFromTxt() Response_Key Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipReqAddHdrFromTxt(g_ReqMsg,Retry_After,(char*)RetryAfterHdr);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqAddHdrFromTxt() Retry_After Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipReqAddHdrFromTxt(g_ReqMsg,Route,(char*)RouteHdr);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqAddHdrFromTxt() Route Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipReqAddHdrFromTxt(g_ReqMsg,Session_Expires,(char*)SessionExpiresHdr);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqAddHdrFromTxt() Session-Expires Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipReqAddHdrFromTxt(g_ReqMsg,Subject,(char*)SubjectHdr);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqAddHdrFromTxt() Subject Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipReqAddHdrFromTxt(g_ReqMsg,Timestamp,(char*)TimestampHdr);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqAddHdrFromTxt() Timestamp Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipReqAddHdrFromTxt(g_ReqMsg,To,(char*)ToHdr);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqAddHdrFromTxt() To Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipReqAddHdrFromTxt(g_ReqMsg,User_Agent,(char*)UserAgentHdr);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqAddHdrFromTxt() User_Agent Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipReqAddHdrFromTxt(g_ReqMsg,Via,(char*)ViaHdr);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqAddHdrFromTxt() Via Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}

#ifdef ICL_IMS
	result=sipReqAddHdrFromTxt(g_ReqMsg,Path,(char*)PathHdr);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqAddHdrFromTxt() Path Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
#endif

	pBody=(SipBody*)malloc(sizeof(SipBody));
	if (NULL!=pBody) {
		pBody->length=116;
		pBody->content=(unsigned char*)strDup(ReqMsgBody);
		result=sipReqSetBody(g_ReqMsg,pBody);
		sipBodyFree(pBody);
	}
	
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipReqSetBody() error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}

	pMsg=sipReqPrint(g_ReqMsg);
	if(strncmp(pMsg,pReqBuf,strlen(pReqBuf))!=0){
		printf("\n!!ERROR!! %s:%d: sipReqNewFromTxt() Compare with orginal message error.\n",__CMFILE__,__LINE__);
		printf("origianl:\n%s\n",pReqBuf);
		printf("after   :\n%s\n",pMsg);
		return RC_ERROR;
	}

	printf("\n\t(sip protocol stack Add/Get Request Header/Body ).......!!SUCCESS!!\n");

	sipReqFree(g_ReqMsg);
	return RC_OK;
}

RCODE sipRspNew_Test(void)
{
	/* sipReqNew is OK or not */
	g_RspMsg=sipRspNew();
	if(g_RspMsg == NULL){
		printf( "\n!!ERROR!! %s:%d: sipRspNew() error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	sipRspFree(g_RspMsg);

	return RC_OK;
}
	
RCODE sipRspNewFromTxt_Test(void)
{
	FILE *pf;
	char* pMsg;

	/* sipReqMsg New From Txt */
	g_RspMsg= NULL;
#ifdef ICL_IMS
	pf = fopen("IMSRspMsg","rb");
#else
	pf = fopen("RspMsg","rb");
#endif	
	memset(pRspBuf,0,4096);
	if (pf!=NULL){
		fread(pRspBuf,sizeof(char),4096,pf);
		g_RspMsg = sipRspNewFromTxt(pRspBuf);
		if(g_RspMsg == NULL){
			printf( "\n!!ERROR!! %s:%d: sipRspNewFromTxt() error.\n",__CMFILE__,__LINE__);
			fclose(pf);
			return RC_ERROR;
		}
		fclose(pf);
	}
	else{
		printf( "\n!!ERROR!! %s:%d: sipRspNewFromTxt() reading request file error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}

	pMsg=sipRspPrint(g_RspMsg);

	if(strcmp(pMsg,pRspBuf)!=0){
		printf("\n!!ERROR!! %s:%d: sipRspNewFromTxt() Compare with orginal message error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	sipRspFree(g_RspMsg);

	printf("\n\t(sip protocol stack New Response From Text).............!!SUCCESS!!\n");
	return RC_OK;
}
	
RCODE sipRspModify_Test(void)
{
	RCODE result;
	SipRspStatusLine *rspLine,*getLine;
	SipBody *pBody;
	char *pMsg;

	/* sipReqNew is OK or not */
	g_RspMsg=sipRspNew();
	if(g_RspMsg == NULL){
		printf( "\n!!ERROR!! %s:%d: sipRspNew() error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}

	rspLine =(SipRspStatusLine*)malloc(sizeof(SipRspStatusLine));
	rspLine->ptrReason=strDup(RspReason);
	rspLine->ptrStatusCode=strDup(RspStatusCode);
	rspLine->ptrVersion=strDup(RspSIPVersion);

	result=sipRspSetStatusLine(g_RspMsg,rspLine);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipRspSetStatusLine() error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	free(rspLine->ptrReason);
	free(rspLine->ptrStatusCode);
	free(rspLine->ptrVersion);
	free(rspLine);

	getLine=sipRspGetStatusLine(g_RspMsg);
	if(getLine == NULL){
		printf( "\n!!ERROR!! %s:%d: sipRspGetStatusLine() error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	if((strcmp(getLine->ptrReason,RspReason)!=0) ||
	   (strcmp(getLine->ptrStatusCode,RspStatusCode)!=0) ||
	   (strcmp(getLine->ptrVersion,RspSIPVersion)!=0)){
		printf( "\n!!ERROR!! %s:%d: sipRspGetStatusLine() error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	printf("\n\t(sip protocol stack Set/Get Response Status Line )..... !!SUCCESS!!\n");

	/* Add Response Header */
	result=sipRspAddHdrFromTxt(g_RspMsg,Accept,(char*)RspAccept);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipRspAddHdrFromTxt() Accept Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipRspAddHdrFromTxt(g_RspMsg,Accept_Encoding,(char*)RspAcceptEncoding);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipRspAddHdrFromTxt() Accept_Encoding Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipRspAddHdrFromTxt(g_RspMsg,Accept_Language,(char*)RspAccpetLanguage);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipRspAddHdrFromTxt() Accept_Language Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipRspAddHdrFromTxt(g_RspMsg,Allow,(char*)RspAllow);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipRspAddHdrFromTxt() Allow Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipRspAddHdrFromTxt(g_RspMsg,Call_ID,(char*)RspCallID);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipRspAddHdrFromTxt() Call_ID Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipRspAddHdrFromTxt(g_RspMsg,Contact,(char*)RspContact);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipRspAddHdrFromTxt() Contact Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipRspAddHdrFromTxt(g_RspMsg,Content_Encoding,(char*)RspContentEncoding);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipRspAddHdrFromTxt() Content_Encoding Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipRspAddHdrFromTxt(g_RspMsg,Content_Length,(char*)RspContentLength);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipRspAddHdrFromTxt() Content_Length Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipRspAddHdrFromTxt(g_RspMsg,Content_Type,(char*)RspContentType);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipRspAddHdrFromTxt() Content_Type Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipRspAddHdrFromTxt(g_RspMsg,CSeq,(char*)RspCSeq);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipRspAddHdrFromTxt() CSeq Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipRspAddHdrFromTxt(g_RspMsg,Date,(char*)RspDate);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipRspAddHdrFromTxt() Date Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipRspAddHdrFromTxt(g_RspMsg,Encryption,(char*)RspEncryption);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipRspAddHdrFromTxt() Encryption Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipRspAddHdrFromTxt(g_RspMsg,Expires,(char*)RspExpires);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipRspAddHdrFromTxt() Expires Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipRspAddHdrFromTxt(g_RspMsg,From,(char*)RspFrom);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipRspAddHdrFromTxt() From Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipRspAddHdrFromTxt(g_RspMsg,Min_SE,(char*)RspMinSE);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipRspAddHdrFromTxt() Min-SE Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipRspAddHdrFromTxt(g_RspMsg,Organization,(char*)RspOrganization);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipRspAddHdrFromTxt() Organization Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipRspAddHdrFromTxt(g_RspMsg,Proxy_Authenticate,(char*)RspProxyAutheticate);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipRspAddHdrFromTxt() Proxy_Authenticate Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipRspAddHdrFromTxt(g_RspMsg,Record_Route,(char*)RspRecordRoute);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipRspAddHdrFromTxt() Record_Route Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipRspAddHdrFromTxt(g_RspMsg,Retry_After,(char*)RspRetryAfter);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipRspAddHdrFromTxt() Retry_After Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}

	result=sipRspAddHdrFromTxt(g_RspMsg,Server,(char*)RspServer);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipRspAddHdrFromTxt() Server Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipRspAddHdrFromTxt(g_RspMsg,Timestamp,(char*)RspTimestamp);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipRspAddHdrFromTxt() Timestamp Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipRspAddHdrFromTxt(g_RspMsg,To,(char*)RspTo);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipRspAddHdrFromTxt() To Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipRspAddHdrFromTxt(g_RspMsg,Unsupported,(char*)RspUnsupported);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipRspAddHdrFromTxt() Unsupported Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipRspAddHdrFromTxt(g_RspMsg,User_Agent,(char*)RspUserAgent);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipRspAddHdrFromTxt() User_Agent Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipRspAddHdrFromTxt(g_RspMsg,Warning,(char*)RspWarning);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipRspAddHdrFromTxt() Warning Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipRspAddHdrFromTxt(g_RspMsg,Via,(char*)RspVia);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipRspAddHdrFromTxt() Via Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	result=sipRspAddHdrFromTxt(g_RspMsg,WWW_Authenticate,(char*)RspWWWAutheticate);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipRspAddHdrFromTxt() WWW_Authenticate Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}

#ifdef ICL_IMS
	result=sipRspAddHdrFromTxt(g_RspMsg,P_Associated_URI,(char*)RspPAU);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipRspAddHdrFromTxt() Service-Route Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}

	result=sipRspAddHdrFromTxt(g_RspMsg,Service_Route,(char*)RspServiceRouteHdr);
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipRspAddHdrFromTxt() Service-Route Header error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
#endif

	pBody=(SipBody*)malloc(sizeof(SipBody));
	if (NULL!=pBody) {
		pBody->length=66;
		pBody->content=(unsigned char*)strDup(RspBody);
		result=sipRspSetBody(g_RspMsg,pBody);
		sipBodyFree(pBody);
	}
	if(result != RC_OK){
		printf( "\n!!ERROR!! %s:%d: sipRspSetBody() error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}

	pMsg=sipRspPrint(g_RspMsg);
	if(strcmp(pMsg,pRspBuf)!=0){
		printf("\n!!ERROR!! %s:%d: sipRspAdd/GetHdr() error.\n",__CMFILE__,__LINE__);
		printf("origianl:\n%s\n",pRspBuf);
		printf("after   :\n%s\n",pMsg);
		return RC_ERROR;
	}

	printf("\n\t(sip protocol stack Add/Get Response Header/Body )..... !!SUCCESS!!\n");

	sipRspFree(g_RspMsg);
	return RC_OK;
}


RCODE sipReqMsg(void)
{
	RCODE rCode=RC_OK;

	rCode|=sipReqNew_Test();
	rCode|=sipReqNewFromTxt_Test();
	rCode|=sipReqModify_Test();

	return rCode;
}
	
RCODE sipRspMsg(void)
{
	RCODE rCode=RC_OK;

	rCode|=sipRspNew_Test();
	rCode|=sipRspNewFromTxt_Test();
	rCode|=sipRspModify_Test();

	return rCode;
}

RCODE sipTransport(void)
{	
	SipTCP	tcpc;
	SipUDP	udpc;

#ifdef CCL_TLS
	SipTLS	tlsc;
#endif

	g_ReqMsg=sipReqNewFromTxt(pReqBuf);
	if(g_ReqMsg==NULL){
		printf("\n!!ERROR!! %s:%d: sipReqNewFromTxt() error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	g_RspMsg=sipRspNewFromTxt(pRspBuf);
	if(g_ReqMsg==NULL){
		printf("\n!!ERROR!! %s:%d: sipRspNewFromTxt() error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}

	/*udpc = sipUDPNew("127.0.0.1",5060);*/
	tcpc = sipTCPNew(NULL,0);
	if(tcpc == NULL){
		printf("\n!!ERROR!! %s:%d: sipTCPNew() error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	if(sipTCPConnect(tcpc,localaddr,5060) == -1){
		printf("\n!!ERROR!! %s:%d: sipTCPConnect() error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}

	/* wait for connect */
	while (g_TCPCONNECT<1) {
		sipEvtDispatch(500);
	}
	sipTCPSendRsp(tcpc,g_RspMsg);
	sipTCPSendReq(tcpc,g_ReqMsg);

	g_WhileDoing=0;
	while(g_WhileDoing<2){
		sipEvtDispatch(500);
	}
	sipTCPFree(tcpc);

	udpc=sipUDPNew(NULL,5060);
	if(udpc==NULL){
		printf("\n!!ERROR!! %s:%d: sipUDPNew() error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	sipUDPSendReq(udpc,g_ReqMsg,localaddr,5060);
	sipUDPSendRsp(udpc,g_RspMsg,localaddr,5060);
	g_WhileDoing=0;
	while(g_WhileDoing<2){
		sipEvtDispatch(500);
	}

	sipUDPFree(udpc);

#ifdef CCL_TLS_SERVER
	g_WhileDoing=0;
	while(g_WhileDoing<2){
		sipEvtDispatch(500);
	}
	tlsc = sipTLSNew(NULL,0);
	if(tlsc == NULL){
		printf("\n!!ERROR!! %s:%d: sipTLSNew() error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}

	if(sipTLSConnect(tlsc,remoteaddr,5061) == -1){
		printf("\n!!ERROR!! %s:%d: sipTLSConnect() error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}

	while (g_TLSCONNECT<1) {
		sipEvtDispatch(500);
	}

	sipTLSSendRsp(tlsc,g_RspMsg);
	sipTLSSendReq(tlsc,g_ReqMsg);
	sipTLSFree(tlsc);
#elif defined(CCL_TLS_CLIENT)
	
	tlsc = sipTLSNew(NULL,0);
	if(tlsc == NULL){
		printf("\n!!ERROR!! %s:%d: sipTLSNew() error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}

	if(sipTLSConnect(tlsc,remoteaddr,5061) == -1){
		printf("\n!!ERROR!! %s:%d: sipTLSConnect() error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	while (g_TLSCONNECT<1) {
		sipEvtDispatch(500);
	}
	sipTLSSendRsp(tlsc,g_RspMsg);
	sipTLSSendReq(tlsc,g_ReqMsg);

	g_WhileDoing=0;
	while(g_WhileDoing<2){
		sipEvtDispatch(500);
	}

	sipTLSFree(tlsc);
#endif
	sipReqFree(g_ReqMsg);
	sipRspFree(g_RspMsg);
	g_ReqMsg=NULL;
	g_RspMsg=NULL;
	
	return RC_OK;
}

#define threadcycle 100

void* sipReqNewFromTxt_Thrd1(void* agr)
{
	char *pBuf;	
	char *pMsg=NULL;
	SipReq req=NULL;
	int ret=RC_OK,i,length=255;
	char buffer[256];
	
	pBuf=strDup(pReqBuf);
	
	for(i=0;i<threadcycle;i++){
		/* printf("sipReqNewFromTxt_Thrd1 run%d!\n",i); */
		/* callid gen */
		CallIDGenerator("xxx",NULL,buffer,&length);
		/* printf("%s\n",buffer); */
		if (pBuf)
			req = sipReqNewFromTxt(pBuf);
		else{
			*(int*)agr=RC_ERROR;
			return NULL;
		}
		if (NULL==req) {
			free(pBuf);
			*(int*)agr=RC_ERROR;
			return NULL;
		}
		pMsg=sipReqPrint(req);
		if(strncmp(pMsg,pBuf,strlen(pBuf))!=0){
			printf("compare fail!\n");
			free(pBuf);
			sipReqFree(req);
			*(int*)agr=RC_ERROR;
			return NULL;
		}
		
		sipReqFree(req);
	}
	free(pBuf);
	*(int*)agr=ret;
	return NULL;
}


void* sipReqNewFromTxt_Thrd2(void* agr)
{
	char *pBuf;	
	char *pMsg=NULL;
	SipReq req=NULL;
	int ret=RC_OK,i,length=255;
	char buffer[256];
	
	pBuf=strDup(pRspBuf);
	
	for(i=0;i<threadcycle;i++){
		/* printf("sipReqNewFromTxt_Thrd2 run%d!\n",i); */
		/* callid gen */
		CallIDGenerator("xxx",NULL,buffer,&length); 
		/* printf("%s\n",buffer); */
		if (pBuf)
			req = sipReqNewFromTxt(pBuf);
		else{
			*(int*)agr=RC_ERROR;
			return NULL;
		}
		if (NULL==req) {
			free(pBuf);
			*(int*)agr=RC_ERROR;
			return NULL;
		}
		pMsg=sipReqPrint(req);
		if(strncmp(pMsg,pBuf,strlen(pBuf))!=0){
			printf("compare fail!\n");
			free(pBuf);
			sipReqFree(req);
			*(int*)agr=RC_ERROR;
			return NULL;
		}
		
		sipReqFree(req);
	}
	free(pBuf);
	*(int*)agr=ret;
	return NULL;
}

/*
 *	detect key 'q'
 */
int KeyboardProcessing(void)
{
    int keyin;
    char input[101];

    keyin = 0 ;

#ifdef _WIN32
    if (kbhit()) {
        gets(input);
        keyin = 1;
    }
#else           
    keyin = read(0, input, 80) ;  // read keyboard
#endif
	if(keyin>0)
		if (input[0]=='q')
				return 1;
	return 0;
}


RCODE sipThrdSafeTest(void)
{

	int rcode=RC_ERROR;
	FILE *pf;
	RCODE ret=RC_OK;
	CxThread tpool[3];
		
	/* set input string */
	pf = fopen("ReqMsg","rb");	
	memset(pReqBuf,0,4096);
	if (pf!=NULL){
		fread(pReqBuf,sizeof(char),4096,pf);
		fclose(pf);
	}
	else{
		printf( "\n!!ERROR!! %s:%d: sipThrdSafeTest() reading request file error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}

	pf = fopen("testmsg","rb");	
	memset(pRspBuf,0,4096);
	if (pf!=NULL){
		fread(pRspBuf,sizeof(char),4096,pf);
		/*printf("\n---------------Original-----------\n%s",pReqBuf);*/
	
		fclose(pf);
	}
	else{
		printf( "\n!!ERROR!! %s:%d: sipThrdSafeTest() reading request file error.\n",__CMFILE__,__LINE__);
		return RC_ERROR;
	}

	tpool[0] = cxThreadCreate(sipReqNewFromTxt_Thrd1,&rcode);
	if( !tpool[0] ) 
		ret = RC_ERROR;
	
	tpool[1] = cxThreadCreate(sipReqNewFromTxt_Thrd2,&rcode);
	if( !tpool[1] ) 
		ret = RC_ERROR;

	cxThreadJoin(tpool[0]);
	cxThreadJoin(tpool[1]);
	

	return ret;
}

RCODE accSip(void)
{
	/*RCODE rCodeTmp;*/
	RCODE rCode = RC_OK;

	printf("\n-------------------------------------------------------------\n");
	printf("<Sip> module test...\n");
	printf("-------------------------------------------------------------\n");

	rCode|=sipAccInit();
	rCode|=sipReqMsg();
	rCode|=sipRspMsg();
	rCode|=sipTransport();
	rCode|=sipThrdSafeTest();
	rCode|=sipAccClean();
	

	return rCode;
}

