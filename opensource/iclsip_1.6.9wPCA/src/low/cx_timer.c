/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* Copyright Telcordia Technologies 1999 */
/* 
 * cx_timer.c
 *
 * $Id: cx_timer.c,v 1.61 2006/05/12 08:56:13 tyhuang Exp $
 */

#define _POSIX_SOURCE 1	/* POSIX signals */
#define __EXTENSIONS__	/* Solaris struct timeval*/

#include <stdlib.h>
#include <stdio.h>

#ifndef _WIN32_WCE
#include <assert.h>
#endif

#ifdef UNIX
#include <signal.h>
#include <sys/time.h>
#elif defined(_WIN32_WCE)
#include <winsock.h>	/* struct timeval */
#include <mmsystem.h>
/*#pragma comment(lib, "winmm.lib")*/ /* timeSetEvent() */ 
#elif defined(_WIN32)
#include <winsock2.h>	/* struct timeval */
#include <sys/types.h>	/* time_t */
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib") /* timeSetEvent() */ 
#endif


#include <common/cm_def.h>
#include "cx_timer.h"
#include "cx_mutex.h"
#include "cx_thrd.h"
#include "cx_misc.h"

#ifdef _WIN32
#define TIME_RESOLUTION_MINISEC		100
CRITICAL_SECTION	CSHold;
UINT			nTimeID;
#endif

#ifdef UNIX
  typedef void (TIMECALLBACK)(int);
  static TIMECALLBACK doCB;
#define TIME_RESOLUTION_MINISEC		100
#elif defined(_WIN32_WCE)
  #define MAX_CE_TIMER_THREAD		1000
  static unsigned int	gTimerHadle=0;
  HANDLE		gCEThreadHandle[MAX_CE_TIMER_THREAD];
  unsigned int		gCETimerDelay[MAX_CE_TIMER_THREAD];
  void			ceTimer_thread(unsigned int);
#endif

#if defined(_WIN32_WCE) && (_WIN32_WCE < 400)
  typedef void (TIMECALLBACK)(void);
  unsigned int		timeSetEvent(unsigned int);
  void			timeKillEvent(unsigned int);
#endif

static TIMECALLBACK doCB;

#define MILLION		1000000
int			tickSize;
static int              tmr_refcnt = 0;

typedef struct event 
{
	CxTimerCB	cb;
	void*		data;
	struct timeval	delay;
	int		heapIdx;
} Event;

static Event*		events;
static int*		heap;
int			eventCount, toDo, MAXEVENTS;

/* events[1..MAXEVENTS-1] are event structures that can represent
 * events in the queue.
 *
 * heap[1..eventCount] represent a heap, ordered by events[heap[i]].delay.
 *     CB!=NULL
 * heap[eventCount+1..toDo-1] represent free events.
 *     CB==NULL
 * heap[toDo..MAXEVENTS-1] are events that need to occur in this time 
 *     CB!=NULL
 * interval (only used in doCB)
 * heap[1..MAXEVENTS-1] are a permutation of 1..MAXEVENTS-1, so all 
 * events are known.
 *
 * events[heap[i]].heapIdx == i
 * This lets us go from an index in events (used as the event id) to the
 * heap.
 *
 * N.B. The 0th element of each array is not used because the heap algorithm
 * assumes the root of the heap is element 1.
 */

/* indices are all into heap */
static void heapSwap(int a, int b);
static void heapUp(int i);
static void heapDown(int i);
static void heapCheck(char* msg);
static int cxTimer_debug = 0;

/* Delay/timer invariant
 * Active events have cb!=NULL
 * delay is the time *after* the timer expires when the event 
 * should occur.
 * Outside of the scheduling functions (cxTimerSet and cxTimerDel)
 * the timer must be set if there are any active events.
 * Complexity of the code is due to the possibility of callbacks 
 * invoking scheduling functions.
 *
 * PROBLEM: RESOLVED use "timerSet" to know whether the timer is set.
 * What if the clock goes from non-zero to zero while processing a 
 * request?  We can suppress the signal, but the zero timer will make it 
 * look like there is no pending event and we will decrement all the clocks.
 * SOLUTION: 
 *  (1) freeze the clock, resetting it appropriately
 *  (2) use a more sensitive means of determining if there is a 
 *      scheduled event.
 *
 * PROBLEM RESOLVED -- use inDoCB to protect inner call
 * nested external functions (doCB calling cxTimer{Set,Del}) will 
 * unmask the SIGALRM on the inner call
 * SECOND RESOLUTION -- use cxTimerHold/Release to handle nested 
 * timer holding.  This allows external blocking as well.
 *
 */

static int timerSet;
static struct timeval	TV_ZERO={0,0};

static void decrTimer(struct timeval *delta, struct timeval *nextTime);
static int resetTimer(int e, struct timeval delay, struct timeval *deltaP); 
static void scheduleTimer(int e, struct timeval delay);
static void findNextEvent(void);

/* block/release occurrences of timer. these are cumulative and may be nested */
void cxTimerHold(void);
void cxTimerRelease(void);

/* sjtsai 2003.11.12 */
/*#define TV_CMP(a,b)	(a.tv_sec==b.tv_sec ? a.tv_usec-b.tv_usec : a.tv_sec-b.tv_sec)*/
#define TV_CMP(a,b)	( a.tv_sec*MILLION + a.tv_usec ) - ( b.tv_sec*MILLION + b.tv_usec )


static int		timerHolds = 0;

#ifdef UNIX
CxMutex unixTimerMutex;
CxThread unixTimerThread;
int unix_timer_delay;
BOOL unix_timer_set;

/*void unixTimer_thread(unsigned int timer_id); */
void* unixTimer_thread(void* data);
unsigned int timeSetEvent(unsigned int time_delay);
void timeKillEvent(unsigned int timer_id);
#endif


RCODE cxTimerInit(int maxEvents) 
{
	int i;
#if defined(UNIX)
	struct itimerval minTime;
	struct sigaction act, oact;    
#endif
	if(++tmr_refcnt>1)
		return RC_OK;

#if defined(UNIX)

	unixTimerMutex = cxMutexNew(CX_MUTEX_INTERTHREAD, 0);
	unixTimerThread = cxThreadCreate(unixTimer_thread, NULL);

	act.sa_handler = SIG_IGN;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGALRM, &act, &oact);
	
	/* try to get tick size for several times,
	   this prevent some slow machine always 
	   get a zero tick size. */
	for( i=0; i<10; i++ ) {
		minTime.it_value.tv_sec = 0;
		minTime.it_value.tv_usec = 1;
		minTime.it_interval = TV_ZERO;
	
		/* get minimum setable interval */
		setitimer(ITIMER_REAL, &minTime, NULL);
		getitimer(ITIMER_REAL, &minTime);
		
		if( minTime.it_value.tv_usec!=0 )
			break;
			
		if( i==9 && minTime.it_value.tv_usec==0 ) {
			TCRPrint(101,"cxTimerInit(): fail to get tick size!\n");
			return RC_ERROR;
		}
		minTime.it_value = TV_ZERO;
		setitimer(ITIMER_REAL, &minTime, NULL);		
	}
	/* this rounds tickSize to a factor of MILLION */
	tickSize = MILLION/(MILLION/minTime.it_value.tv_usec);

#ifdef _MARKBYLJCHUANG
	minTime.it_value = TV_ZERO;
	setitimer(ITIMER_REAL, &minTime, NULL);

	act.sa_handler = doCB;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGALRM, &act, &oact);
#endif

#elif defined(_WIN32)
	InitializeCriticalSection(&CSHold);
	nTimeID	= 0;
	tickSize = 1000; /* 1000 usec */
   #ifdef _WIN32_WCE
	memset(gCEThreadHandle, (int)NULL, sizeof(gCEThreadHandle));
	memset(gCETimerDelay, 0, sizeof(gCETimerDelay));
   #endif
#endif

	MAXEVENTS=maxEvents+1;
	timerSet = 0;
	eventCount = 0;
	toDo = MAXEVENTS;

	events = (Event*)calloc(MAXEVENTS, sizeof(struct event));
	if( !events )
		return RC_ERROR;
	
	heap  = (int*)calloc(MAXEVENTS, sizeof(*heap));
	if( !heap ) {
		free( events );
		return RC_ERROR;
	}

	for( i=1; i<MAXEVENTS; i++ ) {
		events[i].cb=NULL;
		events[i].heapIdx = i;
		heap[i] = i;
	}

	return RC_OK;
}

RCODE cxTimerClean(void)
{
	int	i;

	if(--tmr_refcnt>0)
		return RC_OK;

	if(tmr_refcnt<0){
		tmr_refcnt =0;
		return RC_ERROR;
	}

	for( i=0; i<eventCount; i++) {
		cxTimerDel(heap[i]);
	}

	free(events);
	free(heap);
#ifdef _WIN32
	DeleteCriticalSection(&CSHold);
   #ifdef _WIN32_WCE
	for(i=0; i<MAX_CE_TIMER_THREAD; i++)
	{
		if (gCEThreadHandle[i] != NULL)
			CloseHandle(gCEThreadHandle[i]);
	}
   #endif
#endif

#ifdef UNIX
	cxMutexFree(unixTimerMutex);
	cxThreadCancel(unixTimerThread);
#endif 

	return RC_OK;
}

int cxTimerSet(struct timeval delay, CxTimerCB cb, void* data)
{
	int e;
	
	if( cb==NULL )
		return -1;

	if( TV_CMP(delay, TV_ZERO) <= 0 )
		return -1;

	if( delay.tv_usec>=MILLION || delay.tv_usec<0 || delay.tv_sec<0 )
		return -1;

	TCRPrint(101, "<cxTimerSet> (%d.%06d)\n", delay.tv_sec, delay.tv_usec);

	cxTimerHold();

	if (cxTimer_debug) heapCheck((char*)"enter Set"); 

	/* find empty event */
	eventCount++;
	if( eventCount>=toDo ) {
		/* ErrorCheck */
		eventCount--;
		cxTimerRelease();
		return -1;
	}

	e = heap[eventCount];
	events[e].cb = cb;
	events[e].data = data;

	scheduleTimer(e,delay);
	heapUp(eventCount);

	if (cxTimer_debug) heapCheck((char*)"exit Set"); 

	cxTimerRelease();

	return e;
}

void* cxTimerDel(int e)
{
	int oldHeapPlace;
	
	if( e<0 || e>=MAXEVENTS )
		return NULL;

	cxTimerHold();

	if (cxTimer_debug) heapCheck((char*)"enter Del"); 

	if( events[e].cb == NULL ) {
		cxTimerRelease();
		return NULL;	
	}
	events[e].cb = NULL;
	/* setting cb==NULL removes the event from the queue 
	 * if this event was scheduled for the next timer, 
	 * find the new next event.
	 * otherwise everything is already set-up and may be left alone 
	 */
	oldHeapPlace = events[e].heapIdx;
	heapSwap(eventCount, oldHeapPlace);
	eventCount--;

	/* the new value is not necessarily a decendant of the
	 * original value, so oldHeapPlace may be too big or too small
	 * need to protect against the case where it was already eventCount
	 */
	if( oldHeapPlace <= eventCount ) {
		heapDown(oldHeapPlace);
		heapUp(oldHeapPlace);
	}

	if( events[e].delay.tv_sec==0 && events[e].delay.tv_usec==0 ) {
		/* this was scheduled for next timer */
		findNextEvent();
	}

	if (cxTimer_debug) heapCheck((char*)"exit Del"); 

	cxTimerRelease();

	return events[e].data;
}

void cxTimerHold(void)
{
#if defined(UNIX)
	sigset_t set, oset;

	if( timerHolds == 0 ) {
		/* delay any incoming timers */
		sigemptyset(&set);
		sigaddset(&set, SIGALRM);
		sigprocmask(SIG_BLOCK, &set, &oset);
	}
	timerHolds++;

#elif defined(_WIN32)
	EnterCriticalSection(&CSHold);
#endif

	TCRPrint(101,"cxTimerHold()\n");
}

void cxTimerRelease(void)
{
#if defined(UNIX)
	sigset_t set, oset;

	timerHolds--;    
	if( timerHolds==0 ) {
		sigemptyset(&set);
		sigaddset(&set, SIGALRM);
		sigprocmask(SIG_UNBLOCK, &set, &oset);		
	}
#elif defined(_WIN32)
	LeaveCriticalSection(&CSHold);
#endif

	TCRPrint(101,"cxTimerRelease()\n");
}

/* subtract nextTime from delta. make 0<=usec<MILLION */
/* if the result is negtive delta->tv_sec will < 0 */
static void decrTimer(struct timeval *delta, struct timeval *nextTime)
{
	delta->tv_sec  -= nextTime->tv_sec;
	delta->tv_usec -= nextTime->tv_usec;
	while( delta->tv_usec < 0 ) {
		delta->tv_usec += MILLION;
		delta->tv_sec--;
	} 
	while( delta->tv_usec >= MILLION ) {
		delta->tv_usec -= MILLION;
		delta->tv_sec++;
	}
}

/* round to multiple of tickSize                               */
/* For example, when tickSize = 1000	                       */
/* 1 - 1000 will round to 1000, 1001 - 2000 will round to 2000 */
static void tickRound(time_t * t) 
{
	*t = ((*t+tickSize-1)/tickSize)*tickSize;
	if( *t==MILLION ) *t -= 1;	/* avoid rounding up to MILLION */
}

/* set the timer for delay
 * if timer already set for longer period, reset for delay
 * return *delta == delay-timer  
 * return value: 1 if timer was reset, 0 otherwise
 *
 * Problem: RESOLVED by returning value to indicate timer change
 * when timer == 0, but there are events  already scheduled
 * because we had used delta==delay to indicate this is the next event.
 */
static int resetTimer(int e, struct timeval delay, struct timeval *deltaP) 
{
#if defined(UNIX)
	struct itimerval nextTime;
	double time_delay = 0;
#elif defined(_WIN32)
	static DWORD     time_delay = 0;
	static DWORD     start_t = 0, end_t = 0;
	struct timeval   nextTime;
#endif
    
	if( TV_CMP(delay, TV_ZERO) == 0 ){	/* Turn-off timer */
#if defined(UNIX)
		/* LJCHUANG: 2005/06/27
		nextTime.it_interval = TV_ZERO;
		nextTime.it_value = TV_ZERO;
		if (setitimer(ITIMER_REAL, &nextTime, NULL) != 0)
			TCRPrint( 1 , "setitimer() ERROR!! nextTime=%d.%06d\n"
			            , nextTime.it_value.tv_sec
				    , nextTime.it_value.tv_usec);
		*/
		timeKillEvent(0);
#elif defined(_WIN32)
		if (nTimeID) {
			timeKillEvent(nTimeID);
			nTimeID = 0;
		}
#endif
		timerSet = 0;
		*deltaP = TV_ZERO;
		return 0;
	}

	tickRound(&delay.tv_usec);
	*deltaP = delay;
#if defined(UNIX)
	if (getitimer(ITIMER_REAL, &nextTime) != 0)
		TCRPrint(101, "getitimer() ERROR!! nextTime=%d.%06d\n", nextTime.it_value.tv_sec, nextTime.it_value.tv_usec);

	/*
	nextTime.it_value.tv_sec = (time_delay * 1000) / 1000000;
	nextTime.it_value.tv_usec = time_delay % 1000000;
	*/

	/* round nextTime and delay to nearest tickSize */
	tickRound(&nextTime.it_value.tv_usec);
	tickRound(&delay.tv_usec);
	decrTimer(deltaP, &nextTime.it_value);

	TCRPrint(101, "<resetTimer> (%d) delta(%d.%06d) = delay(%d.%06d) - nextTime(%d.%06d)\n",
		e,
		deltaP->tv_sec, deltaP->tv_usec,
		delay.tv_sec, delay.tv_usec,
		nextTime.it_value.tv_sec, nextTime.it_value.tv_usec);

#elif defined(_WIN32)
	if( start_t!=0 ) {
		/* time_delay = delay represented by milliseconds */

		end_t = GetTickCount();
		nextTime.tv_sec = ( (long)(time_delay - (end_t - start_t)) * 1000 ) / 1000000;
		nextTime.tv_usec = ( (long)(time_delay - (end_t - start_t)) * 1000) % 1000000;
		
		tickRound(&nextTime.tv_usec);
		
		if ( nextTime.tv_sec<0 || nextTime.tv_usec<0 ) {
			TCRPrint( 100 , "nextTime(%d.%06d) < 0\n"
			              , nextTime.tv_sec
				      , nextTime.tv_usec);
			TCRPrint( 100 , "time_delay=%u, end_t=%u, start_t=%u \n"
			              , time_delay, end_t, start_t );
			nextTime.tv_sec = 0;
			nextTime.tv_usec = 0;
		}

		decrTimer(deltaP, &nextTime);

		TCRPrint( 100, "<resetTimer> (%d) delta(%d.%06d) = delay(%d.%06d) - nextTime(%d.%06d)\n",
			e,
			deltaP->tv_sec, deltaP->tv_usec,
			delay.tv_sec, delay.tv_usec,
			nextTime.tv_sec, nextTime.tv_usec);
		
	}
#endif

	if( deltaP->tv_sec < 0	/* this is sooner than next event */
		|| !timerSet) {	/* timer not set, adjust time */	
		/* re-set timer, this is next */
#if defined(UNIX)
		TCRPrint(101, "%s: (%d) = %d.%06d\n", "next was", e, 
			nextTime.it_value.tv_sec, nextTime.it_value.tv_usec);
		
		nextTime.it_interval = TV_ZERO;
		nextTime.it_value = delay;
		
		/* Convert to milliseconds */
		time_delay = delay.tv_sec * 1000 + delay.tv_usec / 1000;

		timeSetEvent(time_delay);

		if (setitimer(ITIMER_REAL, &nextTime, NULL) != 0)
			TCRPrint(101, "setitimer() ERROR!! nextTime=%d.%06d\n", nextTime.it_value.tv_sec, nextTime.it_value.tv_usec);

#elif defined(_WIN32)
		if (nTimeID) {
			timeKillEvent(nTimeID);
			nTimeID = 0;
		}
		
		/* Convert to milliseconds */
		time_delay = delay.tv_sec * 1000 + delay.tv_usec / 1000;

	#if defined(_WIN32_WCE) && (_WIN32_WCE < 400)
		nTimeID = timeSetEvent(time_delay);
	#else	 
		nTimeID = timeSetEvent(time_delay, TIME_RESOLUTION_MINISEC, doCB, 0, TIME_ONESHOT | TIME_CALLBACK_FUNCTION);
	#endif
		
		TCRPrint(101, "%s: (%d) = %d.%06d\n", "next was", e, 
			delay.tv_sec, delay.tv_usec);
		
		if( nTimeID==0 ) {
			TCRPrint(101, "resetTimer(): timeSetEvent() error!\n");
		#ifndef _WIN32_WCE
			assert(0);
		#endif
		}

		start_t = GetTickCount();
#endif
		timerSet = 1;
		return 1;
	}
	return 0;
}

/* If delay is zero, timer is already set, leave alone 
 * Determine delta for event, relative to current timer 
 * If delta is positive, set events[e].delay = delta 
 * If delta is negative (sooner than timer), reset timer to delay 
 * and adjust all other delays 
 * If delta=delay, there is no previously scheduled event, so this is next
 */
static void scheduleTimer(int e, struct timeval delay) 
{
	struct timeval delta;
	int changed;

	TCRPrint(101,"<scheduleTimer> (%d) delay = %d.%06d\n", e, delay.tv_sec, delay.tv_usec);

	if( TV_CMP(delay, TV_ZERO) == 0 ) return;

	/* delta = delay - current Timer */
	changed = resetTimer(e, delay, &delta);
	
	events[e].delay = delta;
	TCRPrint(101, "%s: (%d) delay = %d.%06d\n", "setting", e, events[e].delay.tv_sec, events[e].delay.tv_usec);

	if( changed ) {	/* adjust all relative times */
		int i;
		for( i=1; i<=eventCount; i++ ) {
			TCRPrint(101, "changed: decrement delta(%d.%06d) from (%d)delay(%d.%06d)\n",
				delta.tv_sec, delta.tv_usec,
				heap[i],
				events[heap[i]].delay.tv_sec,events[heap[i]].delay.tv_usec);
			
			decrTimer(&events[heap[i]].delay, &delta);
			if( events[heap[i]].delay.tv_sec<0 
				||  events[heap[i]].delay.tv_usec<0 ) {
				TCRPrint(101, "%s: (%d) = %d.%06d\n", "after ", i, events[heap[i]].delay.tv_sec, events[heap[i]].delay.tv_usec);
				TCRPrint(101, "%s: (%d) = %d.%06d\n", "delta", e, delta.tv_sec, delta.tv_usec);
			}
		}
	}
}

/* this code must be re-entrant in case the callback creates or
 * destroys events. 
 */
#if defined(UNIX)
static void doCB(int dummy)
#elif defined(_WIN32_WCE) && (_WIN32_WCE < 400)
static void doCB(void)
#elif defined(_WIN32_WCE) && (_WIN32_WCE >= 400)
static void CALLBACK doCB(UINT uTimerID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
#elif defined(_WIN32)
static void CALLBACK doCB(UINT uTimerID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
#endif
{
	CxTimerCB cb;
	void *data;

	if( tmr_refcnt==0 )
		return;

	cxTimerHold();

	if (cxTimer_debug) heapCheck((char*)"enter doCB"); 

	timerSet = 0;
	while( 1<=eventCount 
		&& ( ( events[heap[1]].delay.tv_sec==0 
		       && events[heap[1]].delay.tv_usec==0 )
		     || ( events[heap[1]].delay.tv_sec<0 
		          || events[heap[1]].delay.tv_usec<0 )
		   ) ) {
		TCRPrint(101, "<doCB> (%d) delay.sec=%d, delay.usec=%d, eventCount = %d\n"
			   , heap[1]
		           , events[heap[1]].delay.tv_sec
			   , events[heap[1]].delay.tv_usec
			   , eventCount);
		/* this event should occur.  */
		heapSwap(1,eventCount); 
		heapSwap(eventCount, toDo-1);
		eventCount--; toDo--;
		heapDown(1);
	}

	findNextEvent();	/* re-establish invariant */

	for( ; toDo < MAXEVENTS; toDo++ ) {
		/* now do scheduled events */
		cb = events[heap[toDo]].cb;
		data = events[heap[toDo]].data;
#ifdef	_WIN32
		cxTimerRelease();
#endif
		if( !cb ) {
			TCRPrint(101, "cx_timer.c: id=%d: "
			            "doCB() error: cb==NULL\n"
			          , heap[toDo]);
			continue;
		}
		(*cb)(heap[toDo], data);
#ifdef	_WIN32
		cxTimerHold();
#endif
		events[heap[toDo]].cb = NULL;
	}
	
	if (cxTimer_debug) heapCheck((char*)"exit doCB"); 

	cxTimerRelease();
}

/* determine next event and reset timer. */
static void findNextEvent(void) 
{
	if (cxTimer_debug) heapCheck((char*)"findNext");

	if( eventCount > 0 ) {
		scheduleTimer(heap[1], events[heap[1]].delay);
	}
	
	if (cxTimer_debug) heapCheck((char*)"findNext exit"); 
}

/* indices are all into heap */
static void heapSwap(int a, int b)
{
	int t;
	
	t = heap[a];
	heap[a] = heap[b];
	heap[b] = t;

	events[heap[a]].heapIdx = a;
	events[heap[b]].heapIdx = b;
}

static void heapUp(int i) 
{
	while( i > 1 && TV_CMP(events[heap[i]].delay, events[heap[i/2]].delay) < 0 ) {
		heapSwap(i,i/2);
		i = i/2;
	}
}

static void heapDown(int i)
{
	int child;
	int smallest;

	for(;;) {
		child = 2*i;
		smallest = i;
		if( child <= eventCount
			&& (TV_CMP(events[heap[smallest]].delay, 
			events[heap[child]].delay) > 0 ) ) {
			smallest = child;
		}
		if( child+1 <= eventCount
			&& (TV_CMP(events[heap[smallest]].delay, 
			events[heap[child+1]].delay) > 0 ) ) {
			smallest = child+1;
		}
		if( smallest == i ) break;
		heapSwap(i,smallest);
		i = smallest;
	}
	
	if (cxTimer_debug) heapCheck((char*)"down"); 
}

void cxTimerSetDebug(void)
{
	cxTimer_debug = 1;
}

void cxTimerResetDebug(void)
{
	cxTimer_debug = 0;
}

static void heapCheck(char* msg)
{
	int i;

	for( i=1;i<MAXEVENTS; i++) {
		if(i!=events[heap[i]].heapIdx)
			TCRPrint(101, "bad back ref %d %d\n", i, events[heap[i]].heapIdx );
	}

	for( i=1;  i*2 <= eventCount; i++ ) {
		if( TV_CMP(events[heap[i]].delay, events[heap[i*2]].delay) > 0 ) {
			TCRPrint(101, "%s: out of order %d=%d.06%d  %d=%d.06%d\n",
				msg,
				i, events[heap[i]].delay.tv_sec, 
				events[heap[i]].delay.tv_usec,
				i*2, events[heap[i*2]].delay.tv_sec, 
				events[heap[i*2]].delay.tv_usec);
		}
		if( i*2+1 <= eventCount &&
			TV_CMP(events[heap[i]].delay, events[heap[i*2+1]].delay) > 0 ) {
			TCRPrint(101, "%s: out of order %d=%d.06%d  %d=%d.06%d\n",
				msg,
				i, events[heap[i]].delay.tv_sec, 
				events[heap[i]].delay.tv_usec,
				i*2+1, events[heap[i*2+1]].delay.tv_sec, 
				events[heap[i*2+1]].delay.tv_usec);
		}
	}
}


#if defined(_WIN32_WCE) && (_WIN32_WCE < 400)
/* timer for WinCE */
/* time_delay : milliseconds */
/* this function will call back the callback function ONE time after the timer expire */
unsigned int timeSetEvent(unsigned int time_delay)
{
	int	timer_id;

	timer_id = gTimerHadle;
	gCEThreadHandle[timer_id] = CreateThread(NULL, 0, 
			(unsigned long(__stdcall*)(void*))ceTimer_thread, 
			(void*)timer_id, 0, NULL);
	gCETimerDelay[timer_id] = time_delay;
	if (gTimerHadle+1 < MAX_CE_TIMER_THREAD)
		gTimerHadle++;
	else gTimerHadle = 0;

	return timer_id;
}

void ceTimer_thread(unsigned int timer_id)
{
	Sleep(gCETimerDelay[timer_id]);
	doCB();
	CloseHandle(gCEThreadHandle[timer_id]);
	gCETimerDelay[timer_id] = 0;
	gCEThreadHandle[timer_id] = 0;
}

void timeKillEvent(unsigned int timer_id)
{
	TerminateThread(gCEThreadHandle[timer_id], (unsigned long)NULL);
	CloseHandle(gCEThreadHandle[timer_id]);
	gCEThreadHandle[timer_id] = 0;
}
#endif

#if defined(UNIX)
/* timer for UNIX */
/* time_delay : milliseconds */
/* this function will call back the callback function ONE time after the timer expire */
unsigned int timeSetEvent(unsigned int time_delay)
{
	int timer_id = 0;
	
	cxMutexLock(unixTimerMutex);

	unix_timer_delay = time_delay;
	unix_timer_set = TRUE;

	cxMutexUnlock(unixTimerMutex);
	
	return timer_id;
}

/*void unixTimer_thread(unsigned int timer_id)*/
void* unixTimer_thread(void* data)
{
	while (1) {
		cxMutexLock(unixTimerMutex);
		if ( unix_timer_set == TRUE ) {
			while ( unix_timer_delay > 0 ) {
				sleeping(TIME_RESOLUTION_MINISEC);
				unix_timer_delay -= TIME_RESOLUTION_MINISEC;

				TCRPrint(101, "delay = %d\n", unix_timer_delay);

				if ( unix_timer_delay <= 0 ) {
					doCB(0);
					unix_timer_set = FALSE;
				}
			}
		}
		cxMutexUnlock(unixTimerMutex);
		sleeping(10);
	}
}

void timeKillEvent(unsigned int timer_id)
{
	cxMutexLock(unixTimerMutex);
	
	unix_timer_set = FALSE;
	unix_timer_delay = 0;
	
	cxMutexUnlock(unixTimerMutex);
}
#endif
