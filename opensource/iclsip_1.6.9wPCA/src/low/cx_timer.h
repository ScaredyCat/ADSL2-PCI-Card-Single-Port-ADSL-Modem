/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* Copyright Telcordia Technologies 1999 */
/*
 * cx_timer.h
 *
 * $Id: cx_timer.h,v 1.15 2005/06/27 13:34:59 ljchuang Exp $
 */

#ifndef CX_TIMER_H
#define CX_TIMER_H

#include <common/cm_trace.h>

#ifdef  __cplusplus
extern "C" {
#endif

#ifdef UNIX
#include <sys/time.h>
#elif defined(_WIN32)
#include <common\cm_def.h> /* added by Arlene*/
/* Arlene marked #include <winsock.h>*/ /* timeval */
#endif

typedef void (*CxTimerCB)(int i, void*);


/*  Fun: cxTimerInit
 *
 *  Desc: MUST be called before any other CxTimer routines.
 *        Allocates room for maxEvents events waiting at one time.
 *
 *  Ret: Returns an timer id used by cxTimerDel to cancel an timer.
 */
RCODE	cxTimerInit(int maxEvents);
RCODE   cxTimerClean(void);

/*  Fun: cxTimerSet
 *
 *  Desc: Create an timer to occur after delay.  On the timer, call $cb($data).
 *        $data is the responsibility of the creator of the timer.
 *        It must either be deallocated by cb or when the event is deleted with
 *        cxTimerDel() .
 *
 *  Ret: Returns an timer id used by cxTimerDel to cancel an timer.
 */
int	cxTimerSet(struct timeval delay, CxTimerCB cb, void* data);

/*  Fun: cxTimerDel
 *
 *  Desc: Remove timer e from future events.
 *
 *  Ret: Returns associated data, in case it needs to be managed.
 */
void*	cxTimerDel(int e);

/*  Fun: cxTimerHold
 *
 *  Desc: Lock up this timer module.
 *
 *  Ret: void
 */
void    cxTimerHold(void);

/*  Fun: cxTimerRelease
 *
 *  Desc: Unlock this timer module.
 *
 *  Ret: void
 */
void    cxTimerRelease(void);

void	cxTimerSetDebug(void);
void	cxTimerResetDebug(void);

#ifdef  __cplusplus
}
#endif

#endif /* CX_TIMER_H */

