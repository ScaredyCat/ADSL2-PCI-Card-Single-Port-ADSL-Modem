/*
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * cm_trace.c
 *
 * $Id: cm_trace.c,v 1.28 2005/02/24 09:40:59 ljchuang Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#ifndef _WIN32_WCE
#include <time.h>
#else
#include <Winbase.h>
#endif

#ifdef _WIN32_WCE_EVC3
#include <low_vc3/cx_sock.h>
#else
#include <low/cx_sock.h>
#endif

#include <low/cx_event.h>
#include <low/cx_mutex.h>
#include <low/cx_thrd.h>
#include "cm_trace.h"

#define LOG2FILE(f, lbuf)	if (f!=NULL) {\
					fwrite(lbuf, strlen(lbuf), 1, f);\
					fflush(f);\
				}

struct TCRObj {
	TraceLevel	trace_level_;
	LogFlag		log_flag_;
	UINT32		sequence_no_;

	CxSock		sock_;		/* sock parameters */
	CxSockAddr	addr_;		/* sock parameters */
	FILE*		fd_;		/* file parameters */
	CxMutex		cs_;		/* critical section */

	CxSock		servSock_;	/* sock server parameters */
	CxSockAddr	servAddr_;	/* sock server parameters */
	CxThread	servThread_;	/* sock server parameters */
	int		servThreadON_;	/* sock server parameters */
	
	void		(*msgCB_)(const char*);	/* message callback function */
};
typedef struct TCRObj	*TCR;	/* tracer */

typedef enum
{
	TCR_SOCKSERV,
	TCR_PRINT
} FromType;

TCR g_tracer = NULL;
int g_tcr_refcnt = 0;

char* gettime(void)
{
	static char logtime[128];

#ifndef _WIN32_WCE
	time_t ltime;
	struct tm *now;

	logtime[0]='\0';

	time( &ltime );
	now = localtime( &ltime );
	strftime(logtime, 128, "%m/%d/%H:%M:%S", now);
#else
	SYSTEMTIME lpSystemTime;

	GetLocalTime(&lpSystemTime);
	sprintf(logtime, "%d/%d/%d:%d:%d", 
		lpSystemTime.wMonth, lpSystemTime.wDay,
		lpSystemTime.wHour, lpSystemTime.wMinute,
		lpSystemTime.wSecond);
#endif

	return logtime;
}

void TCRLogToAllLogger(FromType from, char* logbuf)
{
	if( cxMutexLock(g_tracer->cs_)!=0 )
		return;

	/* log to console */
	if( (g_tracer->log_flag_&LOG_FLAG_CONSOLE) ) 
		printf("%s",logbuf);
	
	/* log to file */
	if( (g_tracer->log_flag_&LOG_FLAG_FILE)&&g_tracer->fd_ ) 
		LOG2FILE(g_tracer->fd_, logbuf);
	
	/* log to sock */
	if( (g_tracer->log_flag_&LOG_FLAG_SOCK)&&g_tracer->sock_&& from!=TCR_SOCKSERV ) 
		cxSockSendto(g_tracer->sock_,logbuf,strlen(logbuf)+1,g_tracer->addr_);

	if( g_tracer->msgCB_ )
		(g_tracer->msgCB_)(logbuf);

	cxMutexUnlock(g_tracer->cs_);
}

void TCRBegin(void)
{
	if(++g_tcr_refcnt>1||g_tracer) 
		return;
	g_tracer = (TCR)malloc(sizeof(struct TCRObj));
	if( !g_tracer )
		return;

	g_tracer->cs_ = cxMutexNew(CX_MUTEX_INTERTHREAD,0);
	if( cxMutexLock(g_tracer->cs_)!=0 ) 
		return;	

	g_tracer->trace_level_ = TRACE_LEVEL_NONE;
	g_tracer->log_flag_ = LOG_FLAG_NONE;
	g_tracer->sock_ = NULL;
	g_tracer->addr_ = NULL;
	g_tracer->fd_ = NULL;
	g_tracer->sequence_no_ = 0;
	g_tracer->servSock_ = NULL;
	g_tracer->servAddr_ = NULL;
	g_tracer->servThread_ = NULL;
	g_tracer->servThreadON_ = 0;
	g_tracer->msgCB_ = NULL;
		
	cxMutexUnlock(g_tracer->cs_);
	return;
}

void TCREnd(void)
{
	if(--g_tcr_refcnt>0)
		return;
	if(!g_tracer)
		return;
	
	TCRSockLoggerOFF();
	TCRFileLoggerOFF();
	TCRSockServerOFF();
	cxMutexFree(g_tracer->cs_);
	free(g_tracer);
	g_tracer = NULL;
	g_tcr_refcnt = 0;
}

RCODE TCRSetMsgCB(void(*msgCB)(const char*))
{
	if(!g_tracer)
		return RC_ERROR;
	if( cxMutexLock(g_tracer->cs_)!=0 ) 
		return RC_ERROR;	
		
	g_tracer->msgCB_ = msgCB;
	
	if( cxMutexUnlock(g_tracer->cs_)!=0 )
		return RC_ERROR;	
	return RC_OK;
}


RCODE TCRSockLoggerON(const char* raddr, UINT16 rport)
{
	int 		ret = RC_OK;
	CxSock		s;	
	CxSockAddr	addr;	

	if(!g_tracer)
	 	return RC_ERROR;
	if( (g_tracer->log_flag_&LOG_FLAG_SOCK) )
		return RC_ERROR;
	
	cxSockInit();
	s = cxSockNew(CX_SOCK_DGRAM, NULL);
	if( !s ) {
		ret = RC_ERROR;
		goto end;
	}
	addr = cxSockAddrNew(raddr,rport);
	if( !addr ) {
		ret = RC_ERROR;
		cxSockFree(s);
		goto end;
	}
	
	if( cxMutexLock(g_tracer->cs_)!=0 ) 
		return RC_ERROR;	
	
	g_tracer->sock_ = s;
	g_tracer->addr_ = addr;
	g_tracer->log_flag_ |= LOG_FLAG_SOCK;

	if( cxMutexUnlock(g_tracer->cs_)!=0)
		return RC_ERROR;
end:		
	return ret;
}

RCODE TCRSockLoggerOFF(void)
{
	CxSock		s;	/* sock server parameters */
	CxSockAddr	addr;	/* sock server parameters */

	if(!g_tracer)
		return RC_ERROR;
	if( !(g_tracer->log_flag_&LOG_FLAG_SOCK) )
		return RC_OK;
		
	if( cxMutexLock(g_tracer->cs_)!=0 ) 
		return RC_ERROR;
		
	g_tracer->log_flag_ &= ~(LOG_FLAG_SOCK);
	s = g_tracer->sock_;
	g_tracer->sock_ = NULL;
	addr = g_tracer->addr_;
	g_tracer->addr_ = NULL;
	
	if( cxMutexUnlock(g_tracer->cs_)!=0)
		return RC_ERROR;	
			
	cxSockFree(s);	
	cxSockAddrFree(addr);	
	
	return RC_OK;
}

void servEvnCB(CxSock sock, CxEventType event, int err, void* context)
{
	char		buff[TCRMAXBUFLEN];
	int 		len;
	
	len = cxSockRecvfrom(sock, buff, TCRMAXBUFLEN);
	if( len<0 ) {
		printf("msgSockRecvfrom error.\n");
	}
	else {
		buff[len] = 0;
		TCRLogToAllLogger(TCR_SOCKSERV,buff);		
	}
}

void* servThread(void* data)
{
	TCR 	tcr = (TCR)data;
	CxEvent evn = cxEventNew();

	cxEventRegister(evn, tcr->servSock_, CX_EVENT_RD, servEvnCB, NULL);

	while( tcr->servThreadON_ ) {
		cxEventDispatch(evn,100);
	}
	cxEventUnregister(evn,tcr->servSock_,NULL);
	cxEventFree(evn);

	return NULL;
}

RCODE TCRSockServerON(UINT16 port)
{
	int	ret = RC_OK;
	
	if(!g_tracer)
		return RC_ERROR;
	if( g_tracer->servSock_ ) 	/* already on */
		return RC_ERROR;
	if( cxMutexLock(g_tracer->cs_)!=0 ) 
		return RC_ERROR;	

	g_tracer->servAddr_ = cxSockAddrNew(NULL,port); /* auto fill my ip */
	if( !g_tracer->servAddr_ ) {
		ret = RC_ERROR;
		goto end;
	}	
	g_tracer->servSock_ = cxSockNew(CX_SOCK_DGRAM, g_tracer->servAddr_);
	if( !g_tracer->servSock_ ) {
		ret = RC_ERROR;
		cxSockAddrFree(g_tracer->servAddr_);
		g_tracer->servAddr_ = NULL;
		goto end;
	}
	g_tracer->servThreadON_ = 1;
	g_tracer->servThread_ = cxThreadCreate(servThread,(void*)g_tracer);
	if( !g_tracer->servThread_ ) {
		ret = RC_ERROR;
		cxSockAddrFree(g_tracer->servAddr_);
		g_tracer->servAddr_ = NULL;
		cxSockFree(g_tracer->servSock_);
		g_tracer->servSock_ = NULL;
		g_tracer->servThreadON_ = 0;
	}
	
end:
	if( cxMutexUnlock(g_tracer->cs_)!=0)
		return RC_ERROR;
	return ret;
}

RCODE TCRSockServerOFF(void)
{
	if(!g_tracer)
		return RC_ERROR;
	if(!g_tracer->servSock_)
		return RC_OK;
	if( cxMutexLock(g_tracer->cs_)!=0 ) 
		return RC_ERROR;	

	g_tracer->servThreadON_ = 0;
	cxThreadJoin(g_tracer->servThread_);
	g_tracer->servThread_ = NULL;
	cxSockFree(g_tracer->servSock_);
	g_tracer->servSock_ = NULL;
	cxSockAddrFree(g_tracer->servAddr_);
	g_tracer->servAddr_ = NULL;
	
	cxSockClean();

	if( cxMutexUnlock(g_tracer->cs_)!=0)
		return RC_ERROR;
		
	return RC_OK;
}

RCODE TCRConsoleLoggerON(void)
{
	if(!g_tracer)
		return RC_ERROR;
	if( cxMutexLock(g_tracer->cs_)!=0 ) 
		return RC_ERROR;	

	g_tracer->log_flag_ |= LOG_FLAG_CONSOLE;

	if( cxMutexUnlock(g_tracer->cs_)!=0)
		return RC_ERROR;
		
	return RC_OK;
}

RCODE TCRConsoleLoggerOFF(void)
{
	if(!g_tracer)
		return  RC_ERROR;
	if( cxMutexLock(g_tracer->cs_)!=0 ) 
		return RC_ERROR;	

	g_tracer->log_flag_ &= ~(LOG_FLAG_CONSOLE);

	if( cxMutexUnlock(g_tracer->cs_)!=0)
		return RC_ERROR;

	return RC_OK;
}

RCODE TCRFileLoggerON(const char* fname)
{
	if(!g_tracer||!fname)
		return RC_ERROR;
	if(g_tracer->fd_)
		return RC_ERROR;

	if( cxMutexLock(g_tracer->cs_)!=0 ) 
		return RC_ERROR;	

	g_tracer->fd_ = fopen(fname,"w"); 
	if (g_tracer->fd_ == NULL) { 
		printf("Log file %s open error.\n", fname);
		if( cxMutexUnlock(g_tracer->cs_)!=0)
			return RC_ERROR;
	
		return RC_ERROR;
	} 
	g_tracer->log_flag_ |= LOG_FLAG_FILE;

	if( cxMutexUnlock(g_tracer->cs_)!=0)
		return RC_ERROR;

	return RC_OK;
}

RCODE TCRFileLoggerOFF(void)
{
	if(!g_tracer)
		return RC_ERROR;
	if( cxMutexLock(g_tracer->cs_)!=0 ) 
		return RC_ERROR;	

	if (g_tracer->fd_ != NULL) { 
		fclose(g_tracer->fd_);
		g_tracer->fd_ = NULL;
		g_tracer->log_flag_ &= ~(LOG_FLAG_FILE);
	} 

	if( cxMutexUnlock(g_tracer->cs_)!=0)
		return RC_ERROR;
	return RC_OK;
}

#ifdef  __cplusplus
extern "C" {
#endif

int TCRPrint(int level, const char* format, ...)
{
	char	logbuf[TCRMAXBUFLEN];
	va_list v;
	int     len;
	int     ret;

	if(!g_tracer)
		return RC_ERROR;
	if( g_tracer->trace_level_<level )
		return RC_OK;
	if( g_tracer->log_flag_ == LOG_FLAG_NONE && g_tracer->msgCB_ == NULL )
		return RC_OK;
	if( cxMutexLock(g_tracer->cs_)!=0 )
		return RC_ERROR;

	memset(logbuf,0,TCRMAXBUFLEN);
	sprintf(logbuf, "[%u %s] ", ++g_tracer->sequence_no_,gettime());
	len = strlen(logbuf);
	va_start(v,format);
#if defined(_WIN32) 
	ret = _vsnprintf( logbuf+len, TCRMAXBUFLEN-len, format, v);
#elif defined(_WIN32_WCE)
	ret = vsprintf( logbuf+len, format, v);
#else 
	ret = vsnprintf( logbuf+len, TCRMAXBUFLEN-len, format, v);
#endif
	va_end(v);

	if( cxMutexUnlock(g_tracer->cs_)!=0)
		return RC_ERROR;
		
	TCRLogToAllLogger(TCR_PRINT,logbuf);

	if( ret<0 ) 
		return ret;
	else
		return strlen(logbuf);
}

#ifdef  __cplusplus
}
#endif

RCODE TCRSetTraceLevel(TraceLevel trace_level)
{
	if(!g_tracer)
		return RC_ERROR;
	if( cxMutexLock(g_tracer->cs_)!=0 ) 
		return RC_ERROR;
			
	g_tracer->trace_level_ = trace_level;
	
	if( cxMutexUnlock(g_tracer->cs_)!=0)
		return RC_ERROR;
	return RC_OK;
}

TraceLevel TCRGetTraceLevel(void)
{
	return g_tracer->trace_level_;
}

