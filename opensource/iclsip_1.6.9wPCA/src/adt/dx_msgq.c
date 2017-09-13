/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *                         Industrial Technology Research Institute
 */
/*
 * dx_msgq.c
 *
 * $Id: dx_msgq.c,v 1.22 2004/12/20 03:12:11 tyhuang Exp $ 
 */

#include <stdlib.h>
#include <memory.h>
#ifndef _WIN32_WCE
#include <sys/timeb.h>
#endif
#include <common/cm_def.h>
#include <low/cx_misc.h>
#include <low/cx_mutex.h>
#include "dx_msgq.h"
#include <common/cm_trace.h>

struct dxMsgQObj {
	DxVector	msgq_;	
	CxMutex		r_cs_;

	TraceFlag	traceFlag_;
};

DxMsgQ dxMsgQNew(int initSize)
{
	DxMsgQ mq = malloc(sizeof(struct dxMsgQObj));
	if( !mq )
		return NULL;

	mq->msgq_ = dxVectorNew(initSize,initSize, DX_VECTOR_POINTER);
	if( !mq->msgq_ ) {
		free( mq );
		return NULL;
	}
	mq->r_cs_ = cxMutexNew(CX_MUTEX_INTERTHREAD,0);
	if( !mq->r_cs_ ) {
		dxVectorFree(mq->msgq_,NULL);
		free( mq );
		return NULL;
	}

	return mq;
}

void dxMsgQFree(DxMsgQ mq)
{
	if( !mq )
		return;

	cxMutexFree(mq->r_cs_);
	dxVectorFree(mq->msgq_, (void(*)(void*))free);
	free(mq);
}

int dxMsgQGetLen(DxMsgQ mq)
{
	return (mq)?dxVectorGetSize(mq->msgq_):0;
}

/* [dxMsgQSendMsg]
*	Send message to message queue.
*  return:
*	>=0	if success, return value is the length of message. 
*	<0	if error.
*/
int dxMsgQSendMsg(DxMsgQ mq,char* msg,unsigned int len)
{
	char* buff = NULL;
	unsigned int ulen;

	if(!mq||!msg)	return -1;

	ulen = sizeof(unsigned int);
	buff = malloc(len+ulen);
	if( !buff )
		return -1;

	memcpy(buff+ulen,msg,len);
	*(unsigned int*)buff = len;

	if( dxVectorAddElement(mq->msgq_,buff)<0 ) {
		free(buff);
		return -1;
	}	
		
	return len;
}

/* [dxMsgQGetMsg]
*	Get message from message queue. 
*	Set $timeout==-1 to block until get message.
*  return:
*	>0              if success, return value is the length of message.
*	=0              if time-out
*	0xFFFFFFFF      if error
*/
#ifndef _WIN32_WCE
unsigned int dxMsgQGetMsg(DxMsgQ mq,char* msg,unsigned int len,long timeout)
{
	char* buff = NULL;
	unsigned int bufflen = 0;
	unsigned long start, now;
	struct timeb timebuf;

	if(!mq||!msg)	return -1;

	ftime(&timebuf);
	start = timebuf.time * 1000 + timebuf.millitm;

	while(1) {
		if( cxMutexLock(mq->r_cs_)!=0 )
			return -1;
		if( dxMsgQGetLen(mq)>0 ) 
			break;
		if( cxMutexUnlock(mq->r_cs_)!=0 )
			return -1;
		
		if( timeout==0 )
			return 0;
		sleeping(1);

		ftime(&timebuf);
		now = timebuf.time * 1000 + timebuf.millitm;

		if( timeout>0 && (now-start)>=(unsigned long)timeout ) 
			return 0;
	}
	
	buff = dxVectorRemoveElementAt(mq->msgq_,0);
	
	if( cxMutexUnlock(mq->r_cs_)!=0 )
		return -1;
	
	if( buff && bufflen<=len ) {
		bufflen = *(unsigned int*)buff;
		memcpy(msg,buff+sizeof(unsigned int),bufflen);
		free(buff);

		return bufflen;
	}
	
	free(buff);
	return -1;
}
#endif

/*========================================================================
// set this DxMsgQ's trace flag ON or OFF*/
void dxMsgQSetTraceFlag(DxMsgQ _this, TraceFlag traceflag)
{
	if ( _this==NULL ) 
		return;
	_this->traceFlag_ = traceflag;
}
