/*
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * cx_thrd.c
 *
 * $Id: cx_thrd.c,v 1.11 2004/12/16 08:45:52 ljchuang Exp $
 */
#define _POSIX_C_SOURCE 199309L /* struct timespec */

#include <stdlib.h>
#ifdef UNIX
#include <time.h>
#include <sys/types.h>
#include <pthread.h>
#elif defined(_WIN32_WCE)
#include <Winbase.h>
#elif defined(_WIN32)
#include <windows.h>
#include <process.h>
#endif

#include <common/cm_def.h>
#include "cx_thrd.h"

struct cxThreadObj {
#if defined(UNIX)
	pthread_t	threadID_;
	pthread_attr_t	attr_;
#elif defined(_WIN32)
	HANDLE	 	hthread_;
	UINT32		threadID_;
#if !defined(_WIN32_WCE)
	void*           (*threadRoutine_)(void*);
	void*           parm_;
#endif /* !defined(_WIN32_WCE) */
#endif /* defined(UNIX) */
};

#if defined(_WIN32) && !defined(_WIN32_WCE)
void* cxThreadRoutine(void* parm)
{
	CxThread       this_ = (CxThread)parm;
	void*          ret = NULL;

	if( !this_ )
		goto END_THREAD;

	ret = this_->threadRoutine_( this_->parm_ );

END_THREAD:
	_endthread();
	return ret;
}
#endif /*_WIN32*/

CxThread cxThreadCreate(void*(*thread_routine)(void*),void *arg)
{
	CxThread	this_ = (CxThread)malloc(sizeof(struct cxThreadObj));
	int		ret;

	if( !this_ )
		return NULL;
#if defined(UNIX)
	/* initialize attr with default attributes */
        pthread_attr_init(&this_->attr_);
        ret = pthread_create(&this_->threadID_, &this_->attr_, thread_routine, arg);
        if( ret!=0 ) {
        	free(this_);
        	/*set return code here*/
        	return NULL;
        }
#else
#ifdef _WIN32_WCE
	this_->hthread_ = CreateThread(NULL, 0,
			(unsigned long(__stdcall*)(void*))thread_routine,
			arg, CREATE_SUSPENDED, &this_->threadID_);
#elif defined(_WIN32)
	this_->threadRoutine_ = thread_routine;
	this_->parm_ = arg;
	this_->hthread_ = (HANDLE)_beginthreadex(NULL,0,
			(unsigned (__stdcall*)(void*))cxThreadRoutine,
			this_,CREATE_SUSPENDED,&this_->threadID_);
#endif /*_WIN32_WCE*/
	if( this_->hthread_==INVALID_HANDLE_VALUE ) {
		free(this_);
		return NULL;
	}
	ret = ResumeThread(this_->hthread_);
	if( ret==-1 ) {
		CloseHandle(this_->hthread_);
		free(this_);
		return NULL;
	}
#endif	
	return this_;
}


RCODE cxThreadJoin(CxThread this_)
{
	int 	ret;
#if defined(UNIX)
	int* 	rot;
#endif	
	
	if(!this_)	return -1;
	
#if defined(UNIX)
	ret = pthread_join(this_->threadID_,(void**)&rot);
	if( ret!=0 ) {
		/*if( ret==ESRCH )
			return -2;*/
		return -1;
	}
#elif defined(_WIN32)
	ret = WaitForSingleObject(this_->hthread_,INFINITE);
	if( ret==WAIT_FAILED ) {
		return -1;
	}
	CloseHandle(this_->hthread_);
#endif	
	free(this_);
	return 0;	
}

RCODE cxThreadCancel(CxThread this_)
{
	int 	ret;

	if(!this_)	return -1;
	
#if defined(UNIX)
	ret = pthread_cancel(this_->threadID_);
	if( ret!=0 ) {
		return -1;
	}
#elif defined(_WIN32)
	ret = TerminateThread(this_->hthread_,0);
	if( ret==FALSE ) {
		return -1;
	}
	CloseHandle(this_->hthread_);
#endif
	free(this_);
	return 0;
}


