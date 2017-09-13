/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * cx_thrd.h
 *
 * $Id: cx_thrd.h,v 1.9 2004/09/23 04:41:40 tyhuang Exp $
 */
 
#ifndef CX_THRD_H
#define CX_THRD_H

#include <common/cm_def.h>

#ifdef  __cplusplus
extern "C" {
#endif
 
typedef	struct cxThreadObj*	CxThread;
 
/*****************************************************************************
cxThreadCreat()
	use all default attributes to create thread.
****************************************************************************/
CxThread	cxThreadCreate(void*(*thread_routine)(void*), void *arg);

/*****************************************************************************
RCODE cxThreadJoin(thread)
	wait for this thread to terminate.
****************************************************************************/
RCODE cxThreadJoin(CxThread);

/*****************************************************************************
RCODE cxThreadCancel(thread)
	Cancel thr execution of this thread.and cleanup all resources used
	by thread.
	ps.	For WIN32, call _endthreadex(),
		for UNIX, call pthread_cancle().
****************************************************************************/
RCODE cxThreadCancel(CxThread);
 
#ifdef __cplusplus
}
#endif

#endif /* CX_THRD_H*/ 
 
 
