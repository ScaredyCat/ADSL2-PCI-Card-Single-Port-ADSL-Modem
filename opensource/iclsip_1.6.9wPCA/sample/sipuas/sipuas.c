/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * acc_sip.c
 *
 * $Id: sipuas.c,v 1.6 2005/11/10 09:54:39 tyhuang Exp $
 */


#include <stdio.h>

#ifdef _WIN32
#include <winsock.h>
#include <conio.h>
#else
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define _snprintf snprintf
#endif

#include <sip/sip.h>

#define DEFAULTIP	"127.0.0.1"

SipConfigData	g_ConfigStr;


void	FillSdp(SipReq request, SipRsp response)
{
	SipBody *body;
	int length;

	body=sipReqGetBody(request);

	length=strlen(body->content);
	sipRspSetBody(response,body);
	sipRspAddHdr(response,Content_Length,&length);
}

void	ResponseAddStatusLine(SipReq request,SipRsp response,int RespCode)
{
	SipRspStatusLine *line=(SipRspStatusLine*)malloc(sizeof(SipRspStatusLine));
	SipReqLine	*reqline;
	char buf[5],Reason[20];

	sprintf(buf,"%d",RespCode);
	reqline=sipReqGetReqLine(request);
	line->ptrVersion=strdup("SIP/2.0");
	line->ptrStatusCode=strdup(buf);
	switch(RespCode){
	case 100:
		strcpy(Reason,"Trying");	break;
	case 180:
		strcpy(Reason,"Ringing");	break;
	case 200:
		strcpy(Reason,"OK");
		if(reqline->iMethod == INVITE)
			FillSdp(request,response);	
		break;
	default:
		strcpy(Reason,"  ");		break;
	}
		
	line->ptrReason=strdup(Reason);

	sipRspSetStatusLine(response,line);
	free(line->ptrReason);
	free(line->ptrStatusCode);
	free(line->ptrVersion);
	free(line);
}

void ResponseAddHeader(SipReq request,SipRsp response)
{
	SipFrom *from;
	SipTo	*to;
	char	*ID;
	SipCSeq	*seq;
	/*int		length;*/
	/*sipBody *body;*/
	SipContType    *type;
	
	from=(SipFrom*)sipReqGetHdr(request,From);
	sipRspAddHdr(response,From,from);
	to=(SipTo*)sipReqGetHdr(request,To);
	sipRspAddHdr(response,To,to);
	ID=(char*)sipReqGetHdr(request,Call_ID);
	sipRspAddHdr(response,Call_ID,ID);
	seq=(SipCSeq*)sipReqGetHdr(request,CSeq);
	sipRspAddHdr(response,CSeq,seq);
	type=sipReqGetHdr(request,Content_Type);
	sipRspAddHdr(response,Content_Type,type);
}

void ResponseAddViaHeader(SipReq request,SipRsp response)
{
	SipVia *via;

	via=(SipVia*)sipReqGetHdr(request,Via);
	if(via!= NULL)

	sipRspAddHdr(response,Via,via);
}

int	SendUDPResponseFunc(SipUDP udp,SipReq request,char *radr,int rport,int RespCode)
{
	SipRsp response;
	SipVia *via;

	response=sipRspNew();

	ResponseAddStatusLine(request,response,RespCode);
	ResponseAddViaHeader(request,response);
	ResponseAddHeader(request,response);
	via=sipReqGetHdr(request,Via);
	
	if(radr != NULL)
		sipUDPSendRsp(udp, response,radr,(UINT16)rport);

	sipRspFree(response);
	return 1;

}

/* SIP UDP CallBack Function */
void CBSipUDPEventHandle(SipUDP udp, SipUDPEvtType event, SipMsgType msgtype,void* msg)
{
	SipReq req;
	SipReqLine *reqLine;
	char* raddr;
	unsigned short rport;

	switch(event) {
	case SIP_UDPEVT_DATA:
		switch(msgtype) {
		case SIP_MSG_REQ:
				req=(SipReq)msg;
				reqLine=sipReqGetReqLine(req);
				
				raddr =(char*) sipUDPGetRaddr(udp);
				sipUDPGetRport(udp,&rport);

				switch(reqLine->iMethod){
				case INVITE:
					SendUDPResponseFunc(udp,(SipReq)msg,raddr,rport,100);
					SendUDPResponseFunc(udp,(SipReq)msg,raddr,rport,200);
					break;
				case REGISTER:
				case BYE:
					SendUDPResponseFunc(udp,(SipReq)msg,raddr,rport,200);
					sipUDPFree(udp);
					break;
				default:
					break;
				}
				sipReqFree(req);
			break;
		case SIP_MSG_RSP:
				sipRspFree((SipRsp)msg);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}


void CBSipTCPEventHandle(SipTCP tcp, SipTCPEvtType event, SipMsgType msgtype,void* msg)
{

	switch(event) {
	case SIP_UDPEVT_DATA:
		switch(msgtype) {
		case SIP_MSG_REQ:
			
			break;
		case SIP_MSG_RSP:
			
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

const char *getMyAddr()
{
	static char buf[64];
	struct hostent *hp;
	char hostname[128];
	struct in_addr addr;
	FILE *fp;

	buf[0] = '\0';
	_snprintf(buf, sizeof(buf), "%s", DEFAULTIP);
	
	buf[sizeof(buf) - 1] = '\0';
	
//	fp = fopen("/tmp/wan_ip","w");
//	system("/usr/sbin/status_oper GET Wan1_IF_Info IP > /tmp/wan_ip");
//  fclose(fp);
#if 1	
	if (gethostname(hostname, sizeof(hostname)) < 0)
			return buf;
			
	if ((hp = gethostbyname(hostname)) == NULL)
		return buf;
		
	memcpy(&addr.s_addr, hp->h_addr, sizeof(addr.s_addr));
	_snprintf(buf, sizeof(buf), "%s", inet_ntoa(addr));
#endif
//	fp = fopen("/tmp/wan_ip","r");
//	if (fp!=NULL)
//			{
//				fscanf(fp,"%s",&buf);
//				fclose(fp);
//			}
	
	buf[sizeof(buf) - 1] = '\0';
	
	return buf;
}


/* sip protocol stack initialization */
RCODE sipAccInit(void)
{
	RCODE result;
	char *localaddr;
	

#ifdef _WIN32
	WSADATA wsadata;
	WSAStartup(0x0101, &wsadata);
	localaddr = (char*)getMyAddr();
	WSACleanup();
#else
	localaddr = (char*)getMyAddr();
#endif

	g_ConfigStr.tcp_port=5060;
	g_ConfigStr.log_flag = LOG_FLAG_CONSOLE;
	strcpy(g_ConfigStr.fname,"/tmp/sipuas.log");
	g_ConfigStr.trace_level = TRACE_LEVEL_ALL;
	strcpy(g_ConfigStr.laddr ,localaddr);

	printf("local ip:%s",localaddr);
	result=sipLibInit(SIP_TRANS_UDP |SIP_TRANS_TCP_SERVER| SIP_TRANS_TCP_CLIENT, 
			  CBSipTCPEventHandle, 
			  CBSipUDPEventHandle, 
			  g_ConfigStr);
			  
	
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


#ifdef _WIN32
int KeyboardProcessing(void)
{
    int keyin;
    char input[101];

    keyin = 0 ;

    if (kbhit()) {
        gets(input);
        keyin = 1;
    }

	if(keyin>0)
		if (input[0]=='q')
				return 1;
	return 0;
}
#endif

int main(void)
{
	/*RCODE rCodeTmp;*/
	SipUDP udp;
	
	sipAccInit();
	udp = sipUDPNew(NULL,5060);
	while (1) 
	{
		sipEvtDispatch(500);
#ifdef _WIN32	
		if(KeyboardProcessing()) break;
#endif
	}
	sipAccClean();
	return 0;
}

