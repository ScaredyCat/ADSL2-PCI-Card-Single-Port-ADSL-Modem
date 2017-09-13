/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sip_tx.c
 *
 * $Id: sip_tx.c,v 1.87 2006/04/04 08:07:25 tyhuang Exp $
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sip_tx.h"

#ifdef _WIN32_WCE_EVC3
#include <low_vc3/cx_sock.h>
#else
#include <low/cx_sock.h>
#endif

#include <low/cx_event.h>
#include <low/cx_timer.h>
#include "rfc822.h"
#include "sip_int.h"
#include <common/cm_trace.h>

#ifdef CCL_TLS
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

typedef struct  
{
	SipTCP			tcp;
	SipUDP			udp;
	SipTLS			tls;
	int			tcptype;/* 0=client, 1=server */
} sipEvtCBParam;

struct sipTCPObj
{
	CxSock			sock_;
	sipEvtCBParam*		ptrTCPPara;
};

struct sipUDPObj
{
	CxSock			sock_;
	sipEvtCBParam*		ptrUDPPara;
	unsigned short		refcount;
};

/* add tlsobj by tyhuang */
#ifdef CCL_TLS
struct sipTLSObj
{
	CxSock				sock_;
	SSL*				ssl;
	sipEvtCBParam*		ptrTLSPara;
};
#endif

CxEvent				g_sipEvt=NULL; 
CxSock				g_sipTCPSrv=NULL; 
CxSock				g_sipTLSSrv=NULL;

SipTransType		g_sipAppType = SIP_TRANS_UNKN;
SipTCPEvtCB			g_sipTCPEvtCB = NULL; 
SipUDPEvtCB			g_sipUDPEvtCB = NULL;
SipTLSEvtCB			g_sipTLSEvtCB = NULL;

#ifdef CCL_TLS
SSL_CTX*			g_SERVERTLSCTX = NULL;
SSL_CTX*			g_CLIENTTLSCTX = NULL;

#define CHK_SSL(err) if ((err)==-1) { ERR_print_errors_fp(stderr); exit(2); }
#endif

#define SIP_TX_MAXBUFLEN		20000 /*Acer Modified SipIt 10th*/ /*original 4096*/
#define SIP_INFINITE_CONT_LEN		100000000
/*#define EOF -1 */

SipTCP sipTCPServerNew(CxSock sock);

#ifdef CCL_TLS
SipTLS sipTLSServerNew(CxSock sock);
#endif

int getc_socket(CxSock f) 
{
	char cstr[512];
	char c;
	int ret;
/* Fun: cxSockRecvEx
 * Ret: Return number of byets actually received when success, 
 *      0, connection closed.
 *      -1, error, 
 *      -2, timeout.
 */
	ret=cxSockRecvEx(f, cstr, 1,100);
	if (ret >0 ) {
		memcpy(&c, cstr, 1);
		return c;
	}else if(ret == -2){ /* Time out */
		return SIP_SOCKET_TIMEOUT;
	}else{		   /* socket error & connection closed */
		return EOF;
	}
}

void sipTCPCallback(CxSock sock, CxEventType event, int err, void* context)
{
	sipEvtCBParam* param = (sipEvtCBParam*)context;
	enum {Header, CR, CRNL, CRNLCR, NL, Body} state = Header;
	int content_length;
	int cl = 0;              
	int c;
	char ch;
	SipMsgType msgtype = SIP_MSG_UNKNOWN; /* ljchuang modified 0607 */
	void* message = NULL; /* ljchuang modified 0607 */
	/*char *header, *body;* mark tyhuang 2004/12/27 */
	/*static char header[SIP_TX_MAXBUFLEN],body[SIP_TX_MAXBUFLEN];*/
	char header[SIP_TX_MAXBUFLEN],body[SIP_TX_MAXBUFLEN];
	int hcount, bcount;
	int x=0;

	c = EOF;

	if ( event & CX_EVENT_EX ) {
		g_sipTCPEvtCB(param->tcp, SIP_TCPEVT_END, msgtype, NULL);
		return;
	}

	if ( event & CX_EVENT_WR) {
		g_sipTCPEvtCB(param->tcp, SIP_TCPEVT_CONNECTED, msgtype, NULL);
		cxEventRegister(g_sipEvt, sock, CX_EVENT_RD, sipTCPCallback, (void*)param);
		return;
	}

	/* Once around the loop for each message. */
	do {
		x++;
/*		header = (char*)malloc(SIP_TX_MAXBUFLEN);
		body = (char*)malloc(SIP_TX_MAXBUFLEN);
*/
		memset(header, 0, SIP_TX_MAXBUFLEN*sizeof(char));
		memset(body, 0, SIP_TX_MAXBUFLEN*sizeof(char));
		hcount = 0;
		bcount = 0;
		state          = Header;
		content_length = SIP_INFINITE_CONT_LEN;
		cl             = 0;
		while ((cl < content_length) && ((c = getc_socket(sock)) != EOF ) && (c != SIP_SOCKET_TIMEOUT)) {
			ch = c;
			if (state == Body) {
				memcpy(body + bcount, &ch, 1);
				bcount++;
			} else {
				memcpy(header + hcount, &ch, 1);
				hcount++;
			}
			switch (state) {
			case Header:
				if (c == '\r') state = CR;
				else if (c == '\n') state = NL;
				break;

			case CR:
				if (c == '\n') state = CRNL;
				else state = Header;
				break;

			case CRNL:
				if (c == '\r') state = CRNLCR;
				else state = Header;
				break;

			case NL:
			case CRNLCR:
				if (c == '\n') {
					if (hcount > 4) {
						MsgHdr hdr, tmp;
						int hdrlen;
						int* len;
						unsigned char* lineend;
						int found=0;
						char msg[SIP_TX_MAXBUFLEN];
						/*
						char* msg;
						msg = (char*)malloc(SIP_TX_MAXBUFLEN);
						memset(msg, 0, SIP_TX_MAXBUFLEN);
						*/
						msgtype = sipMsgIdentify((unsigned char*)header);
						if (msgtype == SIP_MSG_REQ || msgtype == SIP_MSG_RSP) {
							hdr = msgHdrNew();
							/*memcpy(msg, header, hcount);*/
							strncpy(msg,header,hcount);

							/* find and drop start line */
							lineend = (unsigned char*)strstr((const char*)msg,"\n");

							rfc822_parse(lineend+1, &hdrlen, hdr);

							tmp = hdr;
							while (!found && tmp!=NULL)
								if (CheckHeaderName(tmp, Content_Length)
									||CheckHeaderName(tmp, Content_Length_Short)){

									ContentLengthParse(tmp, (void**) &len);
									content_length = *(int *)len;
									free(len);
									found=1;
								}
								/*if (RFC822ToSipHeader(tmp, Content_Length,(void**) &len)==1) {
										content_length = *(int *)len;
										found=1;
								}*/else
									tmp = tmp->next;	
							msgHdrFree(hdr);
						}
/*						free(msg);
*/						
						if( content_length==0 )
							break;
					} else 
						content_length = -1;
					state = Body;
				} else 
					state = Header;
				break;
				
			case Body:
				cl++;
				break;
			}
		}

		if ((c != EOF)&&(c != SIP_SOCKET_TIMEOUT)) {
			/* We now have a complete message, with header and content.*/
			if (content_length > 0)
				memcpy(header + hcount, body, bcount);
#ifdef _DEBUG
			/* print buffer from cx.added by tyhuang 2004/7/15 */
			TCRPrint (TRACE_LEVEL_API, "[Rev from TCP]\n%s\n",header);
#endif

#ifdef CCL_SIP_NON_PARSE
			/* post message received from cx */
			message = header;
#else
			if (msgtype == SIP_MSG_REQ || msgtype == SIP_MSG_UNKNOWN){ 
				message = sipReqNewFromTxt(header);
				/* print request after parse.added by tyhuang 2004/7/15 */
#ifdef _DEBUG
				TCRPrint (TRACE_LEVEL_API, "[sipRev request]\n%s\n",sipReqPrint(message));
#endif
			}else if (msgtype == SIP_MSG_RSP){ 
				message = sipRspNewFromTxt(header);
				/* print response after parse.added by tyhuang 2004/7/15 */
#ifdef _DEBUG
				TCRPrint (TRACE_LEVEL_API, "[sipRev response]\n%s\n",sipRspPrint(message));
#endif
			}else
				message = NULL;		
#endif

/*			free(header);
			free(body);
*/
#ifdef CCL_IP_CHECK
			if (message && (msgtype == SIP_MSG_REQ)){
				SipVia *viahdr=NULL;
				char *viarec=NULL,*socksource=NULL;
				socksource=cxSockAddrGetAddr(cxSockGetRaddr(sock));
				viahdr=(SipVia*)sipReqGetHdr((SipReq)message,Via);
				if (viahdr) 
					if (viahdr->ParmList) 
						viarec=viahdr->ParmList->sentby;
				if (viarec && socksource) {			
					if (strcmp(viarec,socksource)!=0){
						sipReqFree((SipReq)message);
						message=NULL;
						TCRPrint(TRACE_LEVEL_API, "[sipRev response]remote ip address and via sendby is not match\n");
					}
				}
			}
#endif
			if(message)
				g_sipTCPEvtCB(param->tcp, SIP_TCPEVT_DATA, msgtype, (void*)message);
			else
				TCRPrint(TRACE_LEVEL_API, "[sipRev response]receive non-SIP message\n");
		} else {
/*			free(header);
			free(body);
*/			break;
		}

		break; /*??? */
	} while ((c != EOF)&&(c!=SIP_SOCKET_TIMEOUT));

	if (c == EOF){
#ifdef WIN32
		WSASetLastError(0);
#endif
		if (param->tcptype == 0) {
			/* ljchuang modified 0607 */
			g_sipTCPEvtCB(param->tcp, SIP_TCPEVT_SERVER_CLOSE, msgtype, (void*)message);
			/*g_sipTCPEvtCB(param->tcp, SIP_TCPEVT_SERVER_CLOSE, ((msgtype==SIP_MSG_UNKNOWN)? SIP_MSG_REQ:msgtype), (void*)message); */
			/*sipTCPFree(param->tcp);*/
			/*param->tcp->ptrTCPPara=NULL;*/
			/*free(param);*/

		} else if (param->tcptype == 1) {
			/* ljchuang modified 0607 */
			g_sipTCPEvtCB(param->tcp, SIP_TCPEVT_CLIENT_CLOSE, msgtype, (void*)message);
			/*g_sipTCPEvtCB(param->tcp, SIP_TCPEVT_CLIENT_CLOSE, ((msgtype==SIP_MSG_UNKNOWN)?SIP_MSG_REQ:msgtype), (void*)message); */
			/*sipTCPServerFree(param->tcp);*/
			/*param->tcp->ptrTCPPara=NULL;
			free(param);*/
		}
	}

	/* Q:WHAT IF c == SIP_SOCKET_TIMEOUT? (ljchuang 2004/06/07) */
}

#ifdef CCL_TLS
void sipTLSCallback(CxSock sock, CxEventType event, int err, void* context)
{
	sipEvtCBParam* param = (sipEvtCBParam*)context;
	enum {Header, CR, CRNL, CRNLCR, NL, Body} state = Header;
	int content_length;
	int cl = 0;              
	char c;
	char ch;
	SipMsgType msgtype = SIP_MSG_UNKNOWN; /* ljchuang modified 0607 */
	void* message = NULL; /* ljchuang modified 0607 */
	/*char *header, *body;* mark tyhuang 2004/12/27 */
	/*static char header[SIP_TX_MAXBUFLEN],body[SIP_TX_MAXBUFLEN];*/
	char header[SIP_TX_MAXBUFLEN],body[SIP_TX_MAXBUFLEN];
	int hcount, bcount;
	int x = 0;
	int err0 = 0;
	int err1 = 0;
	SSL* ssl = param->tls->ssl;

	c = EOF;

	if ( event & CX_EVENT_EX ) {
		g_sipTLSEvtCB(param->tls, SIP_TLSEVT_END, msgtype, NULL);
		return;
	}

	if ( event & CX_EVENT_WR) {
		TCRPrint (TRACE_LEVEL_API, "[sipTLSCallback] try SSL_connect\n");

		err0 = SSL_set_fd (ssl, cxSockGetSock(sock) );
		CHK_SSL(err0);

		err0 = SSL_connect (ssl);
		while (err0 < 0) {
			err1 = SSL_get_error(ssl, err0);
			if ( (err1 == SSL_ERROR_WANT_READ) || (err1 == SSL_ERROR_WANT_WRITE)) {
				TCRPrint(TRACE_LEVEL_ERROR,"[SSL_connect]SSL_ERROR_WANT_READ/SSL_ERROR_WANT_WRITE\n");
				Sleep(100);
				err0 = SSL_connect (ssl);
			}
		}
		TCRPrint (TRACE_LEVEL_API, "[sipTLSCallback] SSL_connect successful\n");
		
		g_sipTLSEvtCB(param->tls, SIP_TLSEVT_CONNECTED, msgtype, NULL);
		cxEventRegister(g_sipEvt, sock, CX_EVENT_RD, sipTLSCallback, (void*)param);
		return;
	}

	/* Once around the loop for each message. */
	do {
		x++;
/*		header = (char*)malloc(SIP_TX_MAXBUFLEN);
		body = (char*)malloc(SIP_TX_MAXBUFLEN);
*/
		memset(header, 0, SIP_TX_MAXBUFLEN*sizeof(char));
		memset(body, 0, SIP_TX_MAXBUFLEN*sizeof(char));

		hcount = 0;
		bcount = 0;
		state          = Header;
		content_length = SIP_INFINITE_CONT_LEN;
		cl             = 0;
		while ((cl < content_length) && (SSL_read(ssl,&c,1)!=EOF) && (c != SIP_SOCKET_TIMEOUT)) {
			ch = c;
			if(c==EOF) break;
			if (state == Body) {
				memcpy(body + bcount, &ch, 1);
				bcount++;
			} else {
				memcpy(header + hcount, &ch, 1);
				hcount++;
			}
			switch (state) {
			case Header:
				if (c == '\r') state = CR;
				else if (c == '\n') state = NL;
				break;

			case CR:
				if (c == '\n') state = CRNL;
				else state = Header;
				break;

			case CRNL:
				if (c == '\r') state = CRNLCR;
				else state = Header;
				break;

			case NL:
			case CRNLCR:
				if (c == '\n') {
					if (hcount > 4) {
						MsgHdr hdr, tmp;
						int hdrlen;
						int* len;
						unsigned char* lineend;
						int found=0;
						char msg[SIP_TX_MAXBUFLEN];
						/*
						char* msg;
						msg = (char*)calloc(1,SIP_TX_MAXBUFLEN);
						*/
						msgtype = sipMsgIdentify((unsigned char*)header);
						if (msgtype == SIP_MSG_REQ || msgtype == SIP_MSG_RSP) {
							hdr = msgHdrNew();
							/*memcpy(msg, header, hcount);*/
							strncpy(msg,header,hcount);			
							/* find and drop start line */
							lineend = (unsigned char*)strstr((const char*)msg,"\n");

							rfc822_parse(lineend+1, &hdrlen, hdr);

							tmp = hdr;
							while (!found && tmp!=NULL)
								
								if (RFC822ToSipHeader(tmp, Content_Length,(void**) &len)==1) {
										content_length = *(int *)len;
										found=1;
								}else
									tmp = tmp->next;
								
							msgHdrFree(hdr);
						}
/*						free(msg);
*/						
						if( content_length==0 )
							break;
					} else 
						content_length = -1;
					state = Body;
				} else 
					state = Header;
				break;
				
			case Body:
				cl++;
				break;
			}
		}

		if ((c != EOF)&&(c != SIP_SOCKET_TIMEOUT)) {
			/* We now have a complete message, with header and content.*/
			if (content_length > 0)
				memcpy(header + hcount, body, bcount);
#ifdef _DEBUG
			/* print buffer from cx.added by tyhuang 2004/7/15 */
			TCRPrint (TRACE_LEVEL_API, "[Rev from TLS]\n%s\n",header);
#endif

#ifdef CCL_SIP_NON_PARSE 
			/* post message received from cx */
			message = header;
#else
			if (msgtype == SIP_MSG_REQ || msgtype == SIP_MSG_UNKNOWN){ 
				message = sipReqNewFromTxt(header);
				/* print request after parse.added by tyhuang 2004/7/15 */
#ifdef _DEBUG
				TCRPrint (TRACE_LEVEL_API, "[sipRev request]\n%s\n",sipReqPrint(message));
#endif
			}else if (msgtype == SIP_MSG_RSP){ 
				message = sipRspNewFromTxt(header);
				/* print response after parse.added by tyhuang 2004/7/15 */
#ifdef _DEBUG
				TCRPrint (TRACE_LEVEL_API, "[sipRev response]\n%s\n",sipRspPrint(message));
#endif
			}else
				message = NULL;
#endif

/*			free(header);
			free(body);
*/			
#ifdef CCL_IP_CHECK
			if (message && (msgtype == SIP_MSG_REQ)){
				SipVia *viahdr=NULL;
				char *viarec=NULL,*socksource=NULL;
				socksource=cxSockAddrGetAddr(cxSockGetRaddr(sock));
				viahdr=(SipVia*)sipReqGetHdr((SipReq)message,Via);
				if (viahdr) 
					if (viahdr->ParmList) 
						viarec=viahdr->ParmList->sentby;
				if (viarec && socksource) {			
					if (strcmp(viarec,socksource)!=0){
						sipReqFree((SipReq)message);
						message=NULL;
						TCRPrint(TRACE_LEVEL_API, "[sipRev response]remote ip address and via sendby is not match\n");
					}
				}
			}
#endif
			if(message)
				g_sipTLSEvtCB(param->tls, SIP_TLSEVT_DATA, msgtype, (void*)message);
			else{
				g_sipTLSEvtCB(param->tls, SIP_TLSEVT_DATA, msgtype, NULL);
				TCRPrint(TRACE_LEVEL_ERROR, "[sipRev response]receive non-SIP message\n");
			}
		} else {
/*			free(header);
			free(body);
*/			break;
		}

		break; /*??? */
	} while ((c != EOF)&&(c!=SIP_SOCKET_TIMEOUT));

	if (c == EOF){
#ifdef WIN32
		WSASetLastError(0);
#endif
		if (param->tcptype == 0) {
			/* ljchuang modified 0607 */
			g_sipTLSEvtCB(param->tls, SIP_TLSEVT_SERVER_CLOSE, msgtype, (void*)message);
			/*g_sipTCPEvtCB(param->tcp, SIP_TCPEVT_SERVER_CLOSE, ((msgtype==SIP_MSG_UNKNOWN)? SIP_MSG_REQ:msgtype), (void*)message); */
	
		} else if (param->tcptype == 1) {
			/* ljchuang modified 0607 */
			g_sipTLSEvtCB(param->tls, SIP_TLSEVT_SERVER_CLOSE, msgtype, (void*)message);
	
		}
	}
}

#endif
void sipUDPCallback(CxSock sock, CxEventType event, int err, void* context)
{
	sipEvtCBParam* param = (sipEvtCBParam*)context;
	int nrecv;
	SipMsgType msgtype = SIP_MSG_UNKNOWN; /* ljchuang modified 0607 */
	void* message = NULL; /* ljchuang modified 0607 */

#ifdef CCL_IP_CHECK
	SipVia *viahdr = NULL;
	char *socksource = NULL;
	char* viarec = NULL;
#endif
	
	/*static char buf[SIP_TX_MAXBUFLEN];*/
	char buf[SIP_TX_MAXBUFLEN];
	/* added by mac, since cxSockRecvfrom may return -1 ! Feb. 16, 2002 */
	buf[0] = '\0';

	/* memset(buf,0,sizeof(char)*SIP_TX_MAXBUFLEN);*/
	/* One UDP datagram may contain several SIP messages!! */
	nrecv = cxSockRecvfrom(sock, buf, SIP_TX_MAXBUFLEN);

	/* check added by mac, since cxSockRecvfrom may return -1 ! Feb. 16, 2002 */
	if ( nrecv >= 0 ) {
		buf[nrecv] = '\0';
	}

	if (nrecv > 0) {
#ifdef _DEBUG
 		TCRPrint (TRACE_LEVEL_API, "[Rev from cx]\n%s\n",buf);
#endif
		msgtype = sipMsgIdentify((unsigned char*)buf);

#ifdef CCL_SIP_NON_PARSE
		/* post message received from cx */
		message = buf;
#else
		if (msgtype == SIP_MSG_REQ || msgtype == SIP_MSG_UNKNOWN){ 
			message = sipReqNewFromTxt(buf);
#ifdef _DEBUG
			TCRPrint (TRACE_LEVEL_API, "[sipRev request]\n%s\n",sipReqPrint(message));
#endif
		}else if (msgtype == SIP_MSG_RSP){ 

			message = sipRspNewFromTxt(buf);
			/* check content-length and actual body size */
			if (message) {
				int len=*(int*)sipRspGetHdr((SipRsp)message,Content_Length);
				SipBody *body=sipRspGetBody((SipRsp)message);
				if ((len>0)&& (NULL!=body)) {
					if (body->length < len){
						TCRPrint (TRACE_LEVEL_ERROR, "[sipstack]drop msg without complete body\n%s\n",sipReqPrint(message));
						sipRspFree((SipRsp)message);
						message=NULL;
					}
				}
			}
#ifdef _DEBUG
			TCRPrint (TRACE_LEVEL_API, "[sipRev response]\n%s\n",sipRspPrint(message));
#endif
		}else
			message = NULL;
#endif
	}
#ifdef CCL_IP_CHECK
	if (message && (msgtype == SIP_MSG_REQ)) {
		socksource = cxSockAddrGetAddr(cxSockGetRaddr(sock));
		viahdr = (SipVia*)sipReqGetHdr((SipReq)message,Via);
		if (viahdr) {
			if (viahdr->ParmList->rport == -1)
				viahdr->ParmList->rport = cxSockAddrGetPort(cxSockGetRaddr(sock));
			
			viarec = viahdr->ParmList->sentby;

			if ( viarec && socksource) {			
				if (strcmp(viarec,socksource)!=0){
					if (viahdr->ParmList->received)
						free(viahdr->ParmList->received);
					viahdr->ParmList->received = strDup( socksource );
				}
			}
		}
	}
#endif
	/* ljchuang modified 0607 */
	/* check message if parse failed or NULL */
	if(message)
		g_sipUDPEvtCB(param->udp, SIP_UDPEVT_DATA, msgtype, (void*)message); 
	else{
		g_sipUDPEvtCB(param->udp, SIP_UDPEVT_DATA, msgtype, NULL); 
		TCRPrint(TRACE_LEVEL_ERROR, "[sipRev]receive non-SIP message\n");
	}
	/*g_sipUDPEvtCB(param->udp, SIP_UDPEVT_DATA, ((msgtype==SIP_MSG_UNKNOWN)?SIP_MSG_REQ:msgtype), (void*)message); */
}

void sipTCPServerAccept(CxSock sock, CxEventType event, int err, void* context)
{
	SipTCP newtcp;

	newtcp = sipTCPServerNew(sock);
	if (newtcp != NULL)
		g_sipTCPEvtCB(newtcp, SIP_TCPEVT_CLIENT_CONN, SIP_MSG_UNKNOWN, NULL);
}

#ifdef CCL_TLS
void sipTLSServerAccept(CxSock sock, CxEventType event, int err, void* context)
{
	SipTLS newtls;

	TCRPrint(TRACE_LEVEL_ERROR,"sipTLSServerAccept\n");

	newtls = sipTLSServerNew(sock);
	if (newtls != NULL)
		g_sipTLSEvtCB(newtls, SIP_TLSEVT_CLIENT_CONN, SIP_MSG_UNKNOWN, NULL);
}
#endif

/* add sipTCPNew() and rewrite sipTCPNEW(char*laddr,UINT16 lport) */
CCLAPI 
SipTCP sipTCPNew(const char* laddr,UINT16 lport)
{
	SipTCP		_this;
	CxSock		sock;
	CxSockAddr	lsockaddr=NULL;

	_this = malloc(sizeof *_this);
	/* error check */
	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"TCP object is NULL!\n");
		return NULL;
	}

	if(laddr){
		if(strlen(laddr) == 0)
			lsockaddr = cxSockAddrNew(NULL, lport); /* auto-fill my ip */
		else 
			if (inet_addr(laddr)!=-1)
				lsockaddr = cxSockAddrNew(laddr,lport);
		if(!lsockaddr)
			lsockaddr = cxSockAddrNew(NULL,lport);
	}else
		lsockaddr = cxSockAddrNew(NULL,lport);
	
	_this->ptrTCPPara=NULL;
	/* lsockaddr = NULL; */
	/* error check */

	sock = cxSockNew(CX_SOCK_STREAM_C, lsockaddr);	
	if (!sock) {
		cxSockAddrFree(lsockaddr);
		free(_this);
		return NULL;
	}

	_this->sock_ = sock;
	cxSockAddrFree(lsockaddr);

	return _this;
}

CCLAPI 
SipTCP sipTCPSrvNew(const char* laddr, UINT16 port)
{
	SipTCP		_this;
	CxSock		sock;
	CxSockAddr	lsockaddr=NULL;

	_this = (SipTCP) malloc(sizeof *_this);
	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"TCP object is NULL!\n");
		return NULL;
	}
	_this->ptrTCPPara = NULL;

	/* get server ipaddr & port from configfile */
	if ( laddr && (strlen(laddr) == 0 ))
		lsockaddr = cxSockAddrNew(NULL, (UINT16)port); /* auto-fill my ip */
	else {
		if ( inet_addr(laddr)!=-1 ) 
			lsockaddr = cxSockAddrNew(laddr,(UINT16)port);
	} 
	
	if(!lsockaddr){
		lsockaddr = cxSockAddrNew(NULL,(UINT16)port);
	}

	sock = cxSockNew(CX_SOCK_SERVER, lsockaddr);
	if (!sock) {
		cxSockAddrFree(lsockaddr);
		free(_this);
		return NULL;
	}

	_this->sock_ = sock;
	cxEventRegister(g_sipEvt, _this->sock_, CX_EVENT_RD, sipTCPServerAccept, NULL);
	cxSockAddrFree(lsockaddr);

	return _this;
}

CCLAPI RCODE sipTCPConnect(SipTCP _this,const char* raddr, UINT16 rport)
{
	CxSockAddr	rsockaddr;
	sipEvtCBParam*	param;

	if ((raddr == NULL) || (rport == 0)||(_this==NULL)) 
		return RC_ERROR;

	rsockaddr = cxSockAddrNew(raddr, rport);
	if(rsockaddr==NULL)
		return RC_ERROR;
	/* error check*/

	if (cxSockConnect(_this->sock_, rsockaddr) < 0) {
		cxSockAddrFree(rsockaddr);
		return RC_ERROR;
	}
	cxSockAddrFree(rsockaddr);

	/* register this new sipTCP for incoming event if callback is set*/
	if (g_sipTCPEvtCB) {
		param = calloc(1,sizeof *param);
		param->tcp = _this;
		param->udp = NULL;
		param->tcptype = 0; /* TCP client */
		_this->ptrTCPPara=param;

		cxEventRegister(g_sipEvt, _this->sock_, CX_EVENT_WR|CX_EVENT_EX, sipTCPCallback, (void*)param);
	}

	return RC_OK;
}

CCLAPI 
const char*	sipTCPGetRaddr(SipTCP _this)
{
	CxSockAddr raddr;
	char* address;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"TCP object is NULL!\n");
		return NULL;
	}
	raddr=cxSockGetRaddr(_this->sock_);
	if(raddr !=NULL)
		address=cxSockAddrGetAddr(raddr);
	else
		address=NULL;
	return address;
}

CCLAPI 
RCODE	sipTCPGetRport(SipTCP _this,UINT16 *port)
{
	CxSockAddr raddr;
	RCODE rc=RC_OK;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"TCP object is NULL!\n");
		return RC_ERROR;
	}
	raddr=cxSockGetRaddr(_this->sock_);
	(raddr != NULL)?(*port=cxSockAddrGetPort(raddr)):(rc=RC_ERROR);

	return rc;

}

CCLAPI
const char*	sipTCPGetLaddr(SipTCP _this)
{
	CxSockAddr laddr;
	char* address;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"TCP object is NULL!\n");
		return NULL;
	}
	laddr=cxSockGetLaddr(_this->sock_);
	if(laddr !=NULL)
		address=cxSockAddrGetAddr(laddr);
	else
		address=NULL;
	return address;
}


CCLAPI 
void sipTCPFree(SipTCP _this)
{
	void* context = NULL;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"TCP object is NULL!\n");
		return ;
	}
	cxEventUnregister(g_sipEvt, _this->sock_, &context);

	/*Since tcp parameter have been free when callback*/
	if(_this->ptrTCPPara != NULL){
		free(_this->ptrTCPPara);
		_this->ptrTCPPara = NULL;
	}

	if(_this->sock_ != NULL) {
		cxSockFree(_this->sock_);
		_this->sock_ = NULL;
	}

	/* Context has been freed alreay!
	if( context )
		free(context);
	*/

	free(_this);
}


SipTCP sipTCPServerNew(CxSock sock)
{
	SipTCP _this;
	sipEvtCBParam* param;
	CxSock	newsock;

	newsock = cxSockAccept(sock);
	if (newsock == NULL) return NULL;

	_this = calloc(1,sizeof *_this);
	/* error check*/
	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"TCP object is NULL!\n");
		return NULL;
	}

	_this->sock_ = newsock;
	_this->ptrTCPPara=NULL;

	/* memory leak while active close!! */
	param = calloc(1,sizeof *param);
	param->tcp = _this;
	param->udp = NULL;
	param->tcptype = 1; /* TCP server */
	_this->ptrTCPPara=param;

	cxEventRegister(g_sipEvt, newsock, CX_EVENT_RD, sipTCPCallback, (void*)param);

	return _this;
}

CCLAPI
void sipTCPServerFree(SipTCP _this)
{
	void* context = NULL;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"TCP object is NULL!\n");
		return ;
	}
	cxEventUnregister(g_sipEvt, _this->sock_, &context);

	/*Since tcp parameter have been free when callback*/
	if(_this->ptrTCPPara != NULL){
		free(_this->ptrTCPPara);
		_this->ptrTCPPara = NULL;
	}
	
	if(_this->sock_ != NULL) {
		cxSockFree(_this->sock_);
		_this->sock_ = NULL;
	}
	/*
	Modified by Mac on May 22, 2003
	no idea why this code is here, which causes double free (context == _this->ptrTCPPara)
	This code seems was added by Acer on July 17, 2002
	if( context )
		free(context);
	*/
	free(_this);
}

CCLAPI 
int sipTCPSendTxt(SipTCP _this, char* msgtext)
{
	unsigned int nsent = 0;
	
	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"TCP object is NULL!\n");
		return RC_ERROR;
	}

	if(!msgtext)
		return RC_ERROR;

	while (nsent < strlen(msgtext) ) {
		nsent += cxSockSend(_this->sock_, msgtext, strlen(msgtext));
	}
	return nsent;
}

CCLAPI 
int sipTCPSendReq(SipTCP _this, SipReq request)
{
	unsigned int nsent = 0;
	char* msgtext;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"TCP object is NULL!\n");
		return RC_ERROR;
	}
	/* SIP message body could contain null character??? */
	msgtext = sipReqPrint(request);
	if(_this->sock_ == NULL)
		nsent =0;
	else {
		while (nsent < strlen(msgtext) ) {
			nsent += cxSockSend(_this->sock_, msgtext, strlen(msgtext));
		}
	}
	return nsent;
}

CCLAPI 
int sipTCPSendRsp(SipTCP _this, SipRsp response)
{
	unsigned int nsent = 0,len;
	char* msgtext;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"TCP object is NULL!\n");
		return RC_ERROR;
	}
	msgtext = sipRspPrint(response);

	TCRPrint(TRACE_LEVEL_ERROR,"[TCP sende]\n%s\n",msgtext);

	len=strlen(msgtext);
	if(_this->sock_ == NULL)
		nsent = 0 ;
	else {
		/*while (nsent < strlen(msgtext) ) {
			nsent += cxSockSend(_this->sock_, msgtext, strlen(msgtext));
		}*/
		while (nsent < len ) 
			nsent += cxSockSend(_this->sock_, msgtext, len);
	}
	return nsent;
}

CCLAPI 
SipUDP sipUDPNew(const char* laddr, UINT16 lport)
{
	SipUDP		_this;
	CxSock		sock;
	CxSockAddr	lsockaddr;
	sipEvtCBParam* param;

	_this = calloc(1,sizeof *_this);
	if(_this == NULL){
		TCRPrint(TRACE_LEVEL_ERROR,"alloc memory for UDP object is failed!\n");
		return NULL;
	}
	/* error check*/

	_this->ptrUDPPara=NULL;
	lsockaddr = cxSockAddrNew(laddr, lport);
	if(lsockaddr == NULL)
		return NULL;
	/* error check*/

	sock = cxSockNew(CX_SOCK_DGRAM, lsockaddr);	
	if (!sock) {
		cxSockAddrFree(lsockaddr);
		free(_this);
		return NULL;
	}
	cxSockAddrFree(lsockaddr);

	_this->sock_ = sock;

	/* register this new sipUDP for incoming event if callback is set*/
	if (g_sipUDPEvtCB) {
		param = calloc(1,sizeof *param);
		param->tcp = NULL;
		param->udp = _this;
		_this->ptrUDPPara=param;

		cxEventRegister(g_sipEvt, _this->sock_, CX_EVENT_RD, sipUDPCallback, (void*)param);
	}

	_this->refcount = 1;

	return _this;
}


CCLAPI 
void sipUDPFree(SipUDP _this)
{
	void* context = NULL;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"UDP object is NULL!\n");
		return ;
	}
	cxEventUnregister(g_sipEvt, _this->sock_, &context);

	if(_this->ptrUDPPara != NULL){
		free(_this->ptrUDPPara);
		_this->ptrUDPPara = NULL;
	}
	if(_this->sock_ != NULL) {
		cxSockFree(_this->sock_);
		_this->sock_=NULL;
	}
	/* context has been freed alreay!
	if( context )
		free(context);
	*/

	free(_this);

}

CCLAPI
unsigned short sipUDPGetRefCount(SipUDP _this)
{
	return 	_this->refcount;
}

CCLAPI
unsigned short sipUDPIncRefCount(SipUDP _this)
{
	return 	++(_this->refcount);
}

CCLAPI
unsigned short sipUDPDecRefCount(SipUDP _this)
{
	if (_this->refcount > 0)
		return --(_this->refcount);
	else
		return 0;
}

CCLAPI 
const char* sipUDPGetRaddr(SipUDP _this)
{
	CxSockAddr raddr;
	char* address;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"UDP object is NULL!\n");
		return NULL;
	}
	raddr = cxSockGetRaddr(_this->sock_);
	if(raddr!=NULL)
		address = cxSockAddrGetAddr(raddr);
	else
		address = NULL;

	return address;
}

CCLAPI 
RCODE sipUDPGetRport(SipUDP _this,UINT16 *port)
{
	CxSockAddr raddr;
	RCODE rc=RC_OK;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"UDP object is NULL!\n");
		return RC_ERROR;
	}
	raddr = cxSockGetRaddr(_this->sock_);
	if(raddr!=NULL)
		*port = cxSockAddrGetPort(raddr);
	else
		rc=RC_ERROR;

	return rc;
}

CCLAPI 
int sipUDPSendTxt(SipUDP _this, char* msgtext, char* raddr, UINT16 rport)
{
	int nsent;
	CxSockAddr rsockaddr;


	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"UDP object is NULL!\n");
		return RC_ERROR;
	}
	if(!msgtext)
		return RC_ERROR;

	/* modified by txyu, to solve the problem that one domain name mapping to
	   IPv4 and IPv6 address, 2004/5/20 */
	/* rsockaddr = cxSockAddrNew(raddr, rport); */

	/* Use cxSockAddrNewCxSock to force the application sends packets 
	   to the same IP family. In other words, send packets to remote machine's
	   IPv4 address if we bind at IPv4 address, or send packets to remote 
	   machine's IPv6 address if we bind at IPv6 address.
	*/
	rsockaddr = cxSockAddrNewCxSock(raddr, rport, _this->sock_);

	if(rsockaddr == NULL) 
		return RC_ERROR;

	if(_this->sock_ != NULL)
		nsent = cxSockSendto(_this->sock_, msgtext, strlen(msgtext), rsockaddr);
	else
		nsent = RC_ERROR;

	cxSockAddrFree(rsockaddr);

	return nsent;
}

CCLAPI 
int sipUDPSendReq(SipUDP _this, SipReq request,const char* raddr, UINT16 rport)
{
	int nsent;
	CxSockAddr rsockaddr;
	char* msgtext;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"UDP object is NULL!\n");
		return RC_ERROR;
	}
	rsockaddr = cxSockAddrNew(raddr, rport);
	if(rsockaddr==NULL)
		return RC_ERROR;
	msgtext = sipReqPrint(request);

	nsent = cxSockSendto(_this->sock_, msgtext, strlen(msgtext), rsockaddr);

	cxSockAddrFree(rsockaddr);

	return nsent;
}

CCLAPI 
int sipUDPSendRsp(SipUDP _this, SipRsp response,const char* raddr, UINT16 rport)
{
	int nsent;
	CxSockAddr rsockaddr;
	char* msgtext;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"UDP object is NULL!\n");
		return RC_ERROR;
	}
	rsockaddr = cxSockAddrNew(raddr, rport);
	if(rsockaddr == NULL)
		return RC_ERROR;
	msgtext = sipRspPrint(response);

	nsent = cxSockSendto(_this->sock_, msgtext, strlen(msgtext), rsockaddr);
	
	cxSockAddrFree(rsockaddr);

	return nsent;
}

CCLAPI 
int sipEvtDispatch(int timeout)
{
	return cxEventDispatch(g_sipEvt, timeout);
}

CCLAPI 
int sipTimerSet(int delay, sipTimerCB cb, void* data)
{
	struct timeval d = {0, 0};
	int timerIdx;

	d.tv_sec = delay / 1000;
	d.tv_usec = (delay % 1000)*1000;
	timerIdx = cxTimerSet(d, cb, data);

	return timerIdx;
}

CCLAPI 
void* sipTimerDel(int e)
{
	return cxTimerDel(e);
}

#ifdef CCL_TLS
/* Create a new TLS object */
CCLAPI 
SipTLS sipTLSNew(const char* laddr, UINT16 lport)
{
	SipTLS		_this;
	CxSock		sock;
	CxSockAddr	lsockaddr=NULL;

	if(!g_CLIENTTLSCTX){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSNew]Client TLS ctx is NULL!\n");
		return NULL;
	}

	_this = calloc(1,sizeof *_this);
	/* error check*/
	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSNew]TLS object is NULL!\n");
		return NULL;
	}

	if(laddr){
		if(strlen(laddr) == 0)
			lsockaddr = cxSockAddrNew(NULL, lport); /* auto-fill my ip */
		else{ 
			if (inet_addr(laddr)!=-1)
				lsockaddr = cxSockAddrNew(laddr,lport);
		}
		if(!lsockaddr)
			lsockaddr = cxSockAddrNew(NULL,lport);
	}else
		lsockaddr = cxSockAddrNew(NULL,lport);

	_this->ptrTLSPara=NULL;

	/* error check*/
	sock = cxSockNew(CX_SOCK_STREAM_C, lsockaddr);	
	if (!sock) {
		cxSockAddrFree(lsockaddr);
		free(_this);
		return NULL;
	}

	_this->sock_ = sock;
	cxSockAddrFree(lsockaddr);	

	_this->ssl=SSL_new(g_CLIENTTLSCTX);

	if(_this->ssl==NULL){
		cxSockFree(_this->sock_);
		free(_this);
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSNew]new ssl is failed!\n");
		return NULL;
	}

	return _this;
}

CCLAPI 
SipTLS sipTLSSrvNew(const char* laddr, UINT16 port)
{
	SipTLS		_this;
	CxSock		sock;
	CxSockAddr	lsockaddr=NULL;

	_this = calloc(1,sizeof *_this);
	/* error check*/
	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"TLS object is NULL!\n");
		return NULL;
	}

	_this->ptrTLSPara = NULL;
	_this->ssl = NULL;

	/* get server ipaddr & port from configfile */
	if ( laddr && (strlen(laddr) == 0 ))
		lsockaddr = cxSockAddrNew(NULL, (UINT16)port); /* auto-fill my ip */
	else{
		if ( inet_addr(laddr)!=-1 ) 
			lsockaddr = cxSockAddrNew(laddr,(UINT16)port);
	} 
	if(!lsockaddr){
		lsockaddr = cxSockAddrNew(NULL,(UINT16)port);
	}

	sock = cxSockNew(CX_SOCK_SERVER, lsockaddr);
	cxSockAddrFree(lsockaddr);
	if (!sock) {
		free(_this);
		return NULL;
	}

	_this->sock_ = sock;
	cxEventRegister(g_sipEvt, _this->sock_, CX_EVENT_RD, sipTLSServerAccept, NULL);

	return _this;
}

CCLAPI
void sipTLSServerFree(SipTLS _this)
{
	void* context = NULL;


	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"TLS object is NULL!\n");
		return ;
	}
	cxEventUnregister(g_sipEvt, _this->sock_, &context);

	if(_this->ptrTLSPara!=NULL){
		free(_this->ptrTLSPara);
		_this->ptrTLSPara=NULL;
	}
	
	if(_this->sock_ != NULL){
		cxSockFree(_this->sock_);
		_this->sock_ =NULL;
	}

	if( _this->ssl ){
		SSL_free(_this->ssl);
		_this->ssl=NULL;
	}
	
	free(_this);
}

/* Establish a TLS connection */
/* input a tcp object pointer and IP address/port for remote side */
SipTLS sipTLSServerNew(CxSock sock)
{
	SipTLS _this;
	sipEvtCBParam* param;
	CxSock	newsock;
	int err, err0;

	if(!g_SERVERTLSCTX){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSServerNew]TLS ctx is NULL!\n");
		return NULL;
	}
	
	newsock = cxSockAccept(sock);
	if (newsock == NULL) return NULL;

	_this = calloc(1,sizeof *_this);
	/* error check*/
	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSServerNew]TLS object is NULL!\n");
		return NULL;
	}
	_this->sock_ = newsock;
	_this->ptrTLSPara=NULL;

	_this->ssl = SSL_new (g_SERVERTLSCTX);

	err = SSL_set_fd (_this->ssl, cxSockGetSock(newsock));
	CHK_SSL(err);

	err = SSL_accept (_this->ssl);
	while (err < 0) {
		err0 = SSL_get_error(_this->ssl, err);
		if ( (err0 == SSL_ERROR_WANT_READ) || (err0 == SSL_ERROR_WANT_WRITE)) {
			TCRPrint(TRACE_LEVEL_ERROR,"[SSL_accept]SSL_ERROR_WANT_READ/SSL_ERROR_WANT_WRITE\n");
			Sleep(100);
			err = SSL_accept (_this->ssl);
		}
	}
	
	/* memory leak while active close!! */
	param = calloc(1,sizeof *param);
	param->tcp = NULL;
	param->udp = NULL;
	param->tls = _this;
	param->tcptype = 1; /* TLS server */
	_this->ptrTLSPara=param;

	cxEventRegister(g_sipEvt, newsock, CX_EVENT_RD, sipTLSCallback, (void*)param);

	return _this;	
}

CCLAPI 
RCODE sipTLSConnect(IN SipTLS _this,IN const char* raddr,IN UINT16 rport)
{
	CxSockAddr	rsockaddr;
	sipEvtCBParam*	param;
	/*int err; */

	if(!g_CLIENTTLSCTX){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSConnect]TLS ctx is NULL!\n");
		return RC_ERROR;
	}

	if ((raddr == NULL) || (rport == 0)||(_this==NULL)){ 
		TCRPrint(TRACE_LEVEL_API,"[sipTLSConnect]input parameters is NULL!\n");
		return RC_ERROR;
	}

	if((_this->sock_==NULL)||(_this->ssl==NULL)){
		TCRPrint(TRACE_LEVEL_WARNING,"[sipTLSConnect]NULL pointer: sock_ or ssl\n");
		return RC_ERROR;
	}

	rsockaddr = cxSockAddrNew(raddr, rport);
	if(rsockaddr==NULL){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSConnect]new sockaddr is NULL!\n");
		return RC_ERROR;
	}
	/* error check*/

	if (cxSockConnect(_this->sock_, rsockaddr) < 0) {
		cxSockAddrFree(rsockaddr);
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSConnect]connect failed!\n");
		return RC_ERROR;
	}
	cxSockAddrFree(rsockaddr);

	/* register this new sipTCP for incoming event if callback is set*/
	if (g_sipTLSEvtCB) {
		param = calloc(1,sizeof *param);
		param->tcp = NULL;
		param->tls = _this;
		param->udp = NULL;
		param->tcptype = 0; /* TCP client */
		_this->ptrTLSPara=param;

		cxEventRegister(g_sipEvt, _this->sock_, CX_EVENT_WR|CX_EVENT_EX, sipTLSCallback, (void*)param);
	}

	/* tls connect */
	/*
	SSL_set_fd (_this->ssl, cxSockGetSock(_this->sock_));
    err=SSL_connect (_this->ssl);

    CHK_SSL(err);
*/
	/* register this new sipTCP for incoming event if callback is set*/
/*
	if (g_sipTLSEvtCB) {
		param = calloc(1,sizeof *param);
		param->tcp = NULL;
		param->tls = _this;
		param->udp = NULL;
		param->tcptype = 0;
		_this->ptrTLSPara=param;

		cxEventRegister(g_sipEvt, _this->sock_, CX_EVENT_RD, sipTLSCallback, (void*)param);
	}
*/

	return RC_OK;
}

/* free a TLS object */
CCLAPI 
void sipTLSFree(IN SipTLS _this)
{

	void* context = NULL;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSFree]TLS object is NULL!\n");
		return ;
	}
	cxEventUnregister(g_sipEvt, _this->sock_, &context);

	if(_this->ptrTLSPara != NULL){
		free(_this->ptrTLSPara);
		_this->ptrTLSPara = NULL;
	}

	if(_this->sock_ != NULL) {
		cxSockFree(_this->sock_);
		_this->sock_=NULL;
	}
	
	if( _this->ssl ){
		SSL_free(_this->ssl);
		_this->ssl=NULL;
	}

	memset(_this,0,sizeof *_this);

	free(_this);
}

/* Send out a buffer of character string using a exist TLS connect*/
/* return value = how many bytes had been sent */
CCLAPI 
int	sipTLSSendTxt(IN SipTLS _this, IN char* msgtext)
{
	int nsent;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSSendTxt]TLS object is NULL!\n");
		return RC_ERROR;
	}

	if(_this->ssl==NULL){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSSendTxt]ssl is NULL!\n");
		return RC_ERROR;
	}

	if(!msgtext){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSSendTxt]message is NULL!\n");
		return RC_ERROR;
	}
	nsent = SSL_write(_this->ssl, msgtext, strlen(msgtext));

	return nsent;
}

			/* Send out a sip request message using a exist TLS connection */
			/* return value = how many bytes had been sent */
CCLAPI 
int		sipTLSSendReq(IN SipTLS _this, IN SipReq req)
{
	int nsent;
	char* msgtext;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSSendReq]TCP object is NULL!\n");
		return RC_ERROR;
	}
	if(_this->ssl==NULL){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSSendReq]ssl is NULL!\n");
		return RC_ERROR;
	}
	if( !req ) {
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSSendReq]request is NULL!\n");
		return RC_ERROR;
	}
	/* SIP message body could contain null character??? */
	msgtext = sipReqPrint(req);
	if( msgtext == NULL )
		nsent =0;
	else
		nsent = SSL_write (_this->ssl, msgtext, strlen(msgtext));

	return nsent;
}

			/* Send out a sip response message using a exist TLS connection */
			/* return value = how many bytes had been sent */
CCLAPI 
int	sipTLSSendRsp(IN SipTLS _this, IN SipRsp rsp)
{
	int nsent;
	char* msgtext;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSSendRsp]TCP object is NULL!\n");
		return RC_ERROR;
	}
	if(_this->ssl==NULL){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSSendRsp]ssl is NULL!\n");
		return RC_ERROR;
	}
	if( !rsp ) {
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSSendRsp]response is NULL!\n");
		return RC_ERROR;
	}

	/* SIP message body could contain null character??? */
	msgtext = sipRspPrint(rsp);
	if( msgtext == NULL )
		nsent =0;
	else
		nsent = SSL_write (_this->ssl, msgtext, strlen(msgtext));

	return nsent;
}

			/* Get remote side address from a TLS object */
			/* return value: remote side IP address */
CCLAPI 
const char*	sipTLSGetRaddr(IN SipTLS _this)
{
	CxSockAddr raddr;
	char* address;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSGetRaddr]TLS object is NULL!\n");
		return NULL;
	}
	raddr=cxSockGetRaddr(_this->sock_);
	if(raddr !=NULL)
		address=cxSockAddrGetAddr(raddr);
	else
		address=NULL;
	return address;	
}

			/* Get remote side port number from a TLS object */
			/* return value: remote side port number */
CCLAPI 
RCODE sipTLSGetRport(SipTLS _this,UINT16 *port)
{
	CxSockAddr raddr;
	RCODE rc=RC_OK;

	if(!_this){
		TCRPrint(TRACE_LEVEL_ERROR,"[sipTLSGetRport]TLS object is NULL!\n");
		return RC_ERROR;
	}
	raddr=cxSockGetRaddr(_this->sock_);
	(raddr != NULL)?(*port=cxSockAddrGetPort(raddr)):(rc=RC_ERROR);

	return rc;
}

#endif
