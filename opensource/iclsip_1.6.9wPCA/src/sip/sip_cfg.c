/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sip_cfg.c
 *
 * $Id: sip_cfg.c,v 1.90 2006/03/31 08:16:26 tyhuang Exp $
 */

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32_WCE /*WinCE didn't support time.h*/
#elif defined(_WIN32)
#include <time.h>
#include <sys/timeb.h>
#elif defined(UNIX)
#include <sys/time.h>
#endif

#include <string.h>

#ifdef CCL_CX_MEM
#include <low/cx_mem.h>
#endif

#include <low/cx_mutex.h>
#include "sip_cfg.h"
#include "sip_tx.h"
#include "sip_hdr.h"
#include "sip_int.h"
#include "rfc822.h"

#ifdef _WIN32_WCE_EVC3
#include <low_vc3/cx_sock.h>
#else
#include <low/cx_sock.h>
#endif

#include <low/cx_event.h>
#include <low/cx_timer.h>
#include <common/cm_trace.h>

#ifdef CCL_TLS

#include <openssl/ssl.h>
#include <openssl/err.h>

#define CERTF   "root.pem"
#define KEYF    "root.pem"

#endif

#ifndef	INADDR_NONE
#define	INADDR_NONE	0xffffffff
#endif

CxMutex CallIDGENMutex=NULL;

extern CxEvent		g_sipEvt;
extern CxSock		g_sipTCPSrv;

#ifdef CCL_TLS
extern CxSock		g_sipTLSSrv;
extern SipTLSEvtCB	g_sipTLSEvtCB;
extern SSL_CTX*		g_SERVERTLSCTX;
extern SSL_CTX*		g_CLIENTTLSCTX;
#endif

extern SipTransType	g_sipAppType;
extern SipTCPEvtCB	g_sipTCPEvtCB;
extern SipUDPEvtCB	g_sipUDPEvtCB;


void sipTCPServerAccept(CxSock sock, CxEventType event, int err, void* context);

#ifdef CCL_TLS
void sipTLSServerAccept(CxSock sock, CxEventType event, int err, void* context);
#endif

void sipInit(void) {} /* just for DLL forcelink */

void sipConfigInit(SipConfigData c)
{
	if( c.log_flag == LOG_FLAG_NONE )
		return;

	TCRBegin();

	TCRSetTraceLevel(c.trace_level);

	if(c.log_flag & LOG_FLAG_CONSOLE) 
		TCRConsoleLoggerON();

	if(c.log_flag & LOG_FLAG_FILE)
		TCRFileLoggerON(c.fname);

	if(c.log_flag & LOG_FLAG_SOCK) 
		TCRSockLoggerON(c.raddr, c.rport);
}

void sipConfigClean(void)
{
	TCREnd();
}

CCLAPI 
RCODE sipLibInit(SipTransType type, SipTCPEvtCB tcpEvtCB, SipUDPEvtCB udpEvtCB, SipConfigData config)
{
	int port;
	RCODE rc;

	if(config.tcp_port)
		port=config.tcp_port;
	else{
		port=SIP_TCP_DEFAULT_PORT; /*default TCP port number */
	}

	g_sipAppType = type;

	if (!tcpEvtCB || !udpEvtCB){
		printf("<sipLibInit>:TCP or UDP Callback function NULL! \n");
		return RC_ERROR;
	}else {
		g_sipTCPEvtCB = tcpEvtCB;
		g_sipUDPEvtCB = udpEvtCB;
	}

#ifdef CCL_CX_MEM

	/* initial cx_mem .add by tyhuang 2005/3/8 */
	if(RC_ERROR==cxMemInit())
		return RC_ERROR;
#endif

	rc=cxSockInit();
	if(rc ==RC_ERROR) return rc;

	if(RC_ERROR==cxTimerInit(SIP_MAX_TIMER_COUNT))
		return RC_ERROR;

	g_sipEvt = cxEventNew();

	
	sipConfigInit(config);

	config.laddr[15]='\0';
	TCRPrint(TRACE_LEVEL_API,"<sipLibInit>:using local ip=%s\n",config.laddr);

	if (type & SIP_TRANS_TCP_SERVER) {
		CxSockAddr laddr=NULL;

		/* get server ipaddr & port from config file */
		if(config.laddr&&(strlen(config.laddr) == 0))
			laddr = cxSockAddrNew(NULL, (UINT16)port); /* auto-fill my ip */
		else {
			if (inet_addr((const char*)config.laddr)!=INADDR_NONE)
				laddr=cxSockAddrNew(config.laddr,(UINT16)port);
		}
		if(!laddr)
			laddr = cxSockAddrNew(NULL,(UINT16)port);

		g_sipTCPSrv = cxSockNew(CX_SOCK_SERVER, laddr);

		if(g_sipTCPSrv != NULL)
			rc=cxEventRegister(g_sipEvt, g_sipTCPSrv, CX_EVENT_RD, sipTCPServerAccept, NULL);
		else
			rc=RC_ERROR;

		cxSockAddrFree(laddr);
	}
	
	/*HdrSetFlag=(config.limithdrset!=1)?0:1;*/

	if( rc == RC_ERROR){
		TCRPrint(TRACE_LEVEL_ERROR,"<sipLibInit>:SIP stack init failed!\n");
		sipLibClean();
		return RC_ERROR;
	}else{
		TCRPrint(TRACE_LEVEL_API,"<sipLibInit>:SIP stack 1.6.8 is running!\n");
		TCRPrint(TRACE_LEVEL_API,"<sipLibInit>:TCP port=%d\n",port);
		return RC_OK;
	}
}

#ifdef CCL_TLS

int passwd_cb( char *buf, int size, int flag, void *password)
{
        strncpy(buf, (char *)(password), size);
        buf[size - 1] = '\0';    
 
        return(strlen(buf));
}

CCLAPI 
RCODE sipLibInitWithTLS(IN SipTransType type, 
				   IN SipTCPEvtCB tcpEvtCB, 
				   IN SipUDPEvtCB udpEvtCB,
				   IN SipTLSEvtCB tlsEvtCB,
				   IN int tlsport ,
				   IN SipConfigData config)
{
	int port;
	RCODE rc;

	if(config.tcp_port)
		port=config.tcp_port;
	else{
		port=SIP_TCP_DEFAULT_PORT; /*default TCP port number */
	}
	if(port==tlsport){
		printf("<sipLibInit>:TCP and TLS listen port conflit! \n");
		return RC_ERROR;
	}

	g_sipAppType = type;

	if (!tcpEvtCB || !udpEvtCB){
		printf("<sipLibInit>:TCP or UDP Callback function NULL! \n");
		return RC_ERROR;
	}else {
		g_sipTCPEvtCB = tcpEvtCB;
		g_sipUDPEvtCB = udpEvtCB;
	}

	rc=cxSockInit();
	if(rc ==RC_ERROR) return rc;

	if(RC_ERROR==cxTimerInit(SIP_MAX_TIMER_COUNT))
		return RC_ERROR;

	g_sipEvt = cxEventNew();

	config.laddr[15]='\0';
	if (type & SIP_TRANS_TCP_SERVER) {
		CxSockAddr laddr=NULL;

		/* get server ipaddr & port from configfile */
		if(config.laddr&&(strlen(config.laddr) == 0))
			laddr = cxSockAddrNew(NULL, (UINT16)port); /* auto-fill my ip */
		else {
			if (inet_addr(config.laddr)!=-1)
				laddr=cxSockAddrNew(config.laddr,(UINT16)port);
		}
		if(!laddr){
			laddr = cxSockAddrNew(NULL,(UINT16)port);
		}

		g_sipTCPSrv = cxSockNew(CX_SOCK_SERVER, laddr);

		rc=cxEventRegister(g_sipEvt, g_sipTCPSrv, CX_EVENT_RD, sipTCPServerAccept, NULL);

		cxSockAddrFree(laddr);
	}
	
	/* if tlsEvtCB is not NULL */
	if( tlsEvtCB ) {
		g_sipTLSEvtCB = tlsEvtCB;

		if (type & SIP_TRANS_TLS_SERVER) {
			
			CxSockAddr laddr=NULL;
			SSL_CTX         *ctx,*clctx;
			
			/* get server ipaddr & port from configfile */
			if(config.laddr&&(strlen(config.laddr) == 0))
				laddr = cxSockAddrNew(NULL, (UINT16)tlsport); /* auto-fill my ip */
			else{
				if ((!laddr)&&(inet_addr(config.laddr)!=INADDR_NONE))
					laddr=cxSockAddrNew(config.laddr,(UINT16)tlsport);
			}
			if(!laddr){
				laddr = cxSockAddrNew(NULL,(UINT16)tlsport);
			}

			SSL_library_init();
			SSL_load_error_strings();
			clctx = SSL_CTX_new (TLSv1_client_method());
			ctx = SSL_CTX_new (TLSv1_server_method());
			
			if(!ctx || !clctx ){
				TCRPrint(TRACE_LEVEL_ERROR,"new SSL_CTX is failed\n ");		
			}else{
				/* set client ctx*/
				/* SSL_CTX_set_cipher_list(clctx,"ALL:!ADH:!AES256:+RC4:@STRENGTH");*/
				g_CLIENTTLSCTX=clctx;
				/* set server ctx parameter */
				SSL_CTX_set_default_passwd_cb_userdata(ctx, (void *)"1111"); 
				SSL_CTX_set_default_passwd_cb(ctx, passwd_cb);                       
				
				/*if(SSL_CTX_load_verify_locations(ctx,"cacert.pem","c:\\")<1)
					TCRPrint(TRACE_LEVEL_ERROR,"cacert.pem is failed\n ");
				*/
				if(SSL_CTX_use_certificate_file(ctx, CERTF, SSL_FILETYPE_PEM)<1){
					TCRPrint(TRACE_LEVEL_ERROR,"SSL_CTX use_certificate_file is failed\n ");
					SSL_CTX_free(ctx);
					cxSockClean();
					return RC_ERROR;
				}
				if(SSL_CTX_use_PrivateKey_file(ctx, KEYF, SSL_FILETYPE_PEM)<1){
					TCRPrint(TRACE_LEVEL_ERROR,"SSL_CTX use_PrivateKey_file is failed\n ");
					SSL_CTX_free(ctx);
					cxSockClean();
					return RC_ERROR;
				}
				if(SSL_CTX_check_private_key(ctx)<1){
					TCRPrint(TRACE_LEVEL_ERROR,"SSL_CTX_check_private_key is failed\n ");
					SSL_CTX_free(ctx);
					cxSockClean();
					return RC_ERROR;
				}
				g_SERVERTLSCTX = ctx;
				
				g_sipTLSSrv = cxSockNew(CX_SOCK_SERVER, laddr);

				cxSockAddrFree(laddr);
				rc|=cxEventRegister(g_sipEvt, g_sipTLSSrv, CX_EVENT_RD, sipTLSServerAccept, NULL);
			}

		}
	}/* end of tls */

	/*HdrSetFlag=(config.limithdrset!=1)?0:1;*/

	sipConfigInit(config);

	if (!CallIDGENMutex) 
		CallIDGENMutex = cxMutexNew(CX_MUTEX_INTERTHREAD, 0);

	if( rc == RC_ERROR){
		TCRPrint(TRACE_LEVEL_ERROR,"<sipLibInit>:SIP stack init failed!\n");
		sipLibClean();
		return RC_ERROR;
	}else{
		TCRPrint(TRACE_LEVEL_API,"<sipLibInit>:SIP stack 1.6.7 is running!\n");
		TCRPrint(TRACE_LEVEL_API,"<sipLibInit>:TCP port=%d\n",port);
		TCRPrint(TRACE_LEVEL_API,"<sipLibInit>:TLS port=%d\n",tlsport);
		return RC_OK;
	}
	return RC_OK;
}

#endif

CCLAPI 
RCODE sipLibClean()
{
	void* context = NULL;
	RCODE rcode=RC_OK;

	if (g_sipTCPSrv) {
		cxEventUnregister(g_sipEvt, g_sipTCPSrv, &context);
		cxSockFree(g_sipTCPSrv);
		if( context )
			free(context);

		g_sipTCPSrv = NULL;
	}

#ifdef CCL_TLS
	if(g_SERVERTLSCTX){ 
		cxEventUnregister(g_sipEvt, g_sipTLSSrv,  &context);
		cxSockFree(g_sipTLSSrv);
		SSL_CTX_free(g_SERVERTLSCTX);
		g_SERVERTLSCTX = NULL;
	}

	if( g_CLIENTTLSCTX ){
		SSL_CTX_free(g_CLIENTTLSCTX);
		g_CLIENTTLSCTX=NULL;
	}
#endif

	rcode|=cxEventFree(g_sipEvt);
	g_sipEvt = NULL;

	rcode|=cxSockClean();

	rcode|=cxTimerClean();

	sipConfigClean();

	if (CallIDGENMutex) {
		cxMutexFree(CallIDGENMutex);
		CallIDGENMutex = NULL;
	}

#ifdef CCL_CX_MEM
	/* clean cx_mem .add by tyhuang 2005/3/8 */
	cxMemClean();
#endif

	return rcode;
}

CCLAPI 
RCODE sipConfig(SipConfigData* c)
{

	return RC_OK;
}

int rchar(void)
{
	return (rand()%239+17);
}

int rint(int yy)
{
	return (yy%239+17);
}
int ipResol(char *ip,char hex[10])
{
	char seps[]= ".";
	char *token,buf[3];
	int count=0;
	
	hex[0]='\0';
	token=strtok(ip,seps);
	while(token!= NULL){
		count++;
		sprintf(buf,"%X",rint(atoi(token)));
		strcat(hex,buf);
		if(count%4 == 2) strcat(hex,"-");
		token=strtok(NULL,seps);
	}
	return RC_OK;
}

CCLAPI
unsigned char* sipCallIDGen(char* host,char* ipaddr)
{
	static unsigned char callidstr[128]={'\0'};
	static unsigned int  gencount;
	char ipbuf[12]={'\0'},buf[64]={'\0'},tmpip[20]={'\0'};
	int pos;

#ifdef _WIN32
	unsigned long ltime;
	ltime=(unsigned long) GetTickCount();

#elif defined(UNIX)
	unsigned long ltime;
	struct timeval	tv;
	gettimeofday(&tv, NULL);
	ltime=(unsigned long) (tv.tv_sec * 1000 + tv.tv_usec / 1000);

#else
	int ltime;
	ltime=rand();
#endif
	if(gencount==0)
		gencount++;

	if(ipaddr!=NULL){
		strcpy(tmpip,ipaddr);

		ipResol(tmpip,ipbuf);
		
		pos=sprintf(buf,"%lu-",ltime);
/*	for mtu
		sprintf(buf+pos,"%X%X%X%X%X%X",rchar(),rchar(),rchar(),rchar(),rchar(),rchar());
		sprintf((char*)callidstr,"%s-%s-%04d",ipbuf,buf,gencount);

		sprintf((char*)callidstr,"%lu-%04d",ltime,gencount);
*/
		sprintf(buf+pos,"%X%X",rchar(),rchar());
		sprintf((char*)callidstr,"%s-%04d",buf,gencount);
		gencount=(gencount+1)%60000;
		/* add 2003-03-06 by tyhuang 
		if(host){
			if(strlen(host)>0){
				strcat(callidstr,"@");
				strcat(callidstr,host);
			}
		}else{	*//*HANK2007/2/27 05:34¤U¤È*/
			if(strlen(ipaddr)>0){
				strcat(callidstr,"@");
				strcat(callidstr,ipaddr);
			}
		
	}else{
		callidstr[0]='\0';
	}
	return callidstr;
} /* sipCallIdGenerate */


CCLAPI
RCODE CallIDGenerator(IN char* host,IN char* ipaddr,OUT char* buffer,IN OUT int* length)
{
	char callidstr[256]={'\0'};
	static unsigned int  gencount;
	char buf[128]={'\0'};
	int pos;

#ifdef _WIN32
	unsigned long ltime;
	ltime=(unsigned long) GetTickCount();

#elif defined(UNIX)
	unsigned long ltime;
	struct timeval	tv;
	gettimeofday(&tv, NULL);
	ltime=(unsigned long) (tv.tv_sec * 1000 + tv.tv_usec / 1000);

#else
	int ltime;
	ltime=rand();
#endif
	if(gencount==0)
			gencount++;

	if ((NULL==buffer)||(NULL==length)||(*length<1)) return RC_ERROR;

	if(ipaddr||host){

		pos=sprintf(buf,"%lu-",ltime);
		sprintf(buf+pos,"%X%X",rchar(),rchar());
		
		cxMutexLock(CallIDGENMutex);
		
		sprintf((char*)callidstr,"%s-%04d",buf,gencount);
		gencount=(gencount+1)%60000;
		cxMutexUnlock(CallIDGENMutex);
		
		/* add 2003-03-06 by tyhuang */
		if(host){
			if(strlen(host)>0){
				strcat(callidstr,"@");
				strcat(callidstr,host);
			}
		}else{
			if(strlen(ipaddr)>0){
				strcat(callidstr,"@");
				strcat(callidstr,ipaddr);
			}
		}

	}else{
		callidstr[0]='\0';
	}
	
	strncpy(buffer,callidstr,*length);
	return RC_OK;
} /* sipCallIdGenerate */

CCLAPI 
void* sipMMalloc(int size)
{
#ifdef CCL_CX_MEM
	return cxMemMalloc(size);
#else
	return malloc(size);
#endif
}	

CCLAPI 
void* sipMCalloc(int n, int size)
{
#ifdef CCL_CX_MEM
	return cxMemCalloc(n,size);
#else
	return calloc(n,size);
#endif	
}

CCLAPI 
void* sipMRalloc(void *p, int size)
{
#ifdef CCL_CX_MEM
	return cxMemRealloc(p,size);
#else
	return realloc(p,size);
#endif
}

CCLAPI 
void sipMFree(void *p)
{
#ifdef CCL_CX_MEM
	cxMemFree(p);
#else
	free(p);
#endif
}

