/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * cm_trace.h
 *
 * $Id: cm_trace.h,v 1.11 2001/12/26 01:59:31 yjliao Exp $
 */

#ifndef CM_TRACE_H
#define CM_TRACE_H

#include "cm_def.h"

#ifdef  __cplusplus
extern "C" {
#endif

#define	TCRMAXBUFLEN	10000
#define	TRACE_ON	1
#define	TRACE_OFF	0

typedef int		TraceFlag;

/* TCRBegin()
	By default, all loggers are OFF. trace_level = 0
*/
void		TCRBegin(void);
void		TCREnd(void);
RCODE		TCRSetTraceLevel(TraceLevel trace_level);
TraceLevel	TCRGetTraceLevel(void);	
	
/* TCRSetMsgCB()
	TCR will callback #msgCB when it gets any message.
	Set #msgCB=NULL to disable callback function.
*/
RCODE		TCRSetMsgCB(void(*msgCB)(const char*));

/* TCRSockLoggerON()
	Log to remote via UDP.
*/
RCODE		TCRSockLoggerON(const char* raddr, UINT16 rport);
RCODE		TCRSockLoggerOFF(void);

/* TCRSockServerON()
	when SockServer is ON, tracer will receive UDP packet from
	#port, and log it to any logger that is active. 
*/
RCODE		TCRSockServerON(UINT16 port);
RCODE		TCRSockServerOFF(void);

RCODE		TCRConsoleLoggerON(void);
RCODE		TCRConsoleLoggerOFF(void);

RCODE		TCRFileLoggerON(const char* fname);
RCODE		TCRFileLoggerOFF(void);

/* TCRPrint()
	this function acts just like printf().
*/
int		TCRPrint(int level, const char* format, ...);

#ifdef  __cplusplus
}
#endif

#endif /* CM_TRACE_H */
