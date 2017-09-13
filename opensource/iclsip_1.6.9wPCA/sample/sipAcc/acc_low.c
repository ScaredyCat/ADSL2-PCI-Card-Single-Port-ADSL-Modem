/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * acc_low.c
 *
 * $Id: acc_low.c,v 1.7 2006/05/09 03:17:47 tyhuang Exp $
 */

#include <stdio.h>
#include <time.h>

#include <common/cm_def.h>
#include <low/cx_thrd.h>
#include <low/cx_mutex.h>
#include <low/cx_sock.h>
#include <low/cx_event.h>
#include <low/cx_thrd.h>
#include <low/cx_timer.h>
#include <low/cx_misc.h>
#include <low/cx_mem.h>

RCODE	accMisc(void)
{
	int	ret = RC_OK;

	sleeping(1);

	return ret;
}

int	thrd_stop = 0;
int	thrd_i = 0;
void*	thrd_tst(void* arg)
{
	int type = (arg)?(*(int*)arg):0;

	if( type==0 )
		while( thrd_i<100000 ) 
			thrd_i++;
	else
		while(1)
			thrd_i++;

	return NULL;
}

RCODE	accThread(void)
{
	int		ret = RC_OK;
	int		type = 0;
	CxThread	t;

	t = cxThreadCreate(thrd_tst,&type);
	if( !t ) {
		printf( "\n!!ERROR!! %s:%d: cxThreadCreate() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto THRD_ERR;
	}
	if( cxThreadJoin(t)!=RC_OK ) {
		printf( "\n!!ERROR!! %s:%d: cxThreadJoin() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto THRD_ERR;		
	}

	type = 1;
	t = cxThreadCreate(thrd_tst,&type);
	if( !t ) {
		printf( "\n!!ERROR!! %s:%d: cxThreadCreate() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto THRD_ERR;
	}
	if( cxThreadCancel(t)!=RC_OK ) {
		printf( "\n!!ERROR!! %s:%d: cxThreadCancel() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto THRD_ERR;		
	}

THRD_ERR:
	return ret;
}

CxMutex	CS = NULL;
int	mutex_tst_data = 0;
int	mutex_tst_stop = 0;
void*	mutex_tst(void* arg)
{
	while( !mutex_tst_stop ) { 
		if( cxMutexLock(CS)!=0 ) {
			printf( "\n!!ERROR!! %s:%d: cxMutexLock() error.\n",
				__CMFILE__,__LINE__);
			goto MTHRD_ERR;	
		}
		mutex_tst_data = 1;
		if( cxMutexUnlock(CS)!=0 ) {
			printf( "\n!!ERROR!! %s:%d: cxMutexUnlock() error.\n",
				__CMFILE__,__LINE__);
			goto MTHRD_ERR;	
		}
		sleeping(10);
	}
	
MTHRD_ERR:
	return NULL;
}

RCODE	accMutex(void)
{
	int		ret = RC_OK;
	CxThread	t;

	CS = cxMutexNew(CX_MUTEX_INTERTHREAD,1123);
	t = cxThreadCreate(mutex_tst,NULL);
	sleeping(1000);
	if( cxMutexLock(CS)!=0 ) {
		printf( "\n!!ERROR!! %s:%d: cxMutexLock() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto MUTEX_ERR;	
	}
	mutex_tst_data = 0;
	sleeping(1000);
	if( mutex_tst_data!=0 ) {
		printf( "\n!!ERROR!! %s:%d: Function of CxMutex error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto MUTEX_ERR;		
	}
	if( cxMutexUnlock(CS)!=0 ) {
		printf( "\n!!ERROR!! %s:%d: cxMutexUnlock() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto MUTEX_ERR;	
	}
	sleeping(1000);
	if( mutex_tst_data!=1 ) {
		printf( "\n!!ERROR!! %s:%d: Function of CxMutex error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto MUTEX_ERR;		
	}

	mutex_tst_stop = 1;
	cxThreadJoin(t);
	cxMutexFree(CS);

MUTEX_ERR:
	return ret;
}


#define SOCK_TST_STRING		"0123456789"
#define SOCK_TST_PORT		2220

CxEvent	g_evn;
CxSock	newsock;
CxSock	c_tcp;
CxSock	c_udp;

int	sock_tcp_msgcnt = 0;
int	sock_udp_msgcnt = 0;
void*	sock_client_tst(void* arg)
{
	CxSockAddr	raddr_tcp;
	CxSockAddr	raddr_udp;
	int		i = 0;
	int		n;
	char		buf[256];

	raddr_tcp=cxSockAddrNew("127.0.0.1",SOCK_TST_PORT);
	raddr_udp=cxSockAddrNew("127.0.0.1",SOCK_TST_PORT);
	c_tcp = cxSockNew(CX_SOCK_STREAM_C,NULL);
	cxSockConnect(c_tcp,raddr_tcp);

	strcpy(buf,SOCK_TST_STRING);
	for( i=0; i<10; i++) {
		if( i<5 ) {
			n = cxSockSend(c_tcp,buf,strlen(buf));
			if( n<0 ) {
				printf( "\n!!ERROR!! %s:%d: cxSockSend() error.\n",
					__CMFILE__,__LINE__);
				break;
			}
		}
		else {
			n = cxSockTpktSend(c_tcp,buf,strlen(buf));
			if( n<0 ) {
				printf( "\n!!ERROR!! %s:%d: cxSockTpktSend() error.\n",
					__CMFILE__,__LINE__);
				break;
			}
		}
	}

	c_udp = cxSockNew(CX_SOCK_DGRAM,NULL);
	for( i=0; i<10; i++) {
		n = cxSockSendto(c_udp,buf,strlen(buf),raddr_udp);
		if( n<0 ) {
			printf( "\n!!ERROR!! %s:%d: cxSockSendto() error.\n",
				__CMFILE__,__LINE__);
			break;
		}
	}

	cxSockAddrFree(raddr_tcp);
	cxSockAddrFree(raddr_udp);

	return NULL;
}

void sock_tcp_tst_cb(CxSock sock, CxEventType event, int err, void* context)
{
	char	buf[256];
	int	n;
	static int i = -1;

	switch( event ) {
	case CX_EVENT_RD:
		i++;
		if( i<5 ) {
			memset(buf,0,256);			
			n = cxSockRecv(sock,buf,10);
			if( n==0 )
				break;			
			if( n<0 || strncmp(buf,SOCK_TST_STRING,10)!=0 ) {
				printf( "\n!!ERROR!! %s:%d: cxSockRecv() error.\n",
					__CMFILE__,__LINE__);
				break;
			}
		}
		else {
			memset(buf,0,256);
			n = cxSockTpktRecv(sock,buf,10);
			if( n==0 )
				break;
			if( n<0 || strncmp(buf,SOCK_TST_STRING,10)!=0 ) {
				printf( "\n!!ERROR!! %s:%d: cxSockTpktRecv() error.\n",
					__CMFILE__,__LINE__);
				break;
			}
		}
		sock_tcp_msgcnt++;
		break;
	case CX_EVENT_WR:
	case CX_EVENT_UNKNOWN:
	case CX_EVENT_EX:
		printf( "\n!!ERROR!! %s:%d: CxEvent callback function error.\n",
			__CMFILE__,__LINE__);
		break;	
	}
	return;
}

void sock_tcp_accept(CxSock sock, CxEventType event, int err, void* context)
{
	newsock = cxSockAccept(sock);
	if (newsock == NULL) {
		printf( "\n!!ERROR!! %s:%d: cxSockAccept() error.\n",
			__CMFILE__,__LINE__);
		return;	
	};

	cxEventRegister(g_evn, newsock, CX_EVENT_RD, sock_tcp_tst_cb, NULL);

	return;
}

void sock_udp_tst_cb(CxSock sock, CxEventType event, int err, void* context)
{
	char	buf[256];
	int	n;

	switch( event ) {
	case CX_EVENT_RD:
		n = cxSockRecvfrom(sock,buf,10);
		if( n<0 || strncmp(buf,SOCK_TST_STRING,10)!=0 ) {
			printf( "\n!!ERROR!! %s:%d: cxSockRecvfrom() error.\n",
				__CMFILE__,__LINE__);
			break;
		}
		sock_udp_msgcnt++;
		break;
	case CX_EVENT_WR:
	case CX_EVENT_UNKNOWN:
	case CX_EVENT_EX:
		printf( "\n!!ERROR!! %s:%d: CxEvent callback function error.\n",
			__CMFILE__,__LINE__);
		break;	
	}

	return;
}

RCODE	accSockEvent(void)
{
	int		i,ret = RC_OK;
	char		myip[10] = {"127.0.0.1"};
	char		buff[100];
	CxThread	t;
	CxSock		s_tcp;
	CxSock		s_udp;
	CxSockAddr	addr_tcp;
	CxSockAddr	addr_udp;

	if( cxSockInit()!=0 ) {
		printf( "\n!!ERROR!! %s:%d: cxSockInit() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto SOCKEVENT_ERR;	
	}
	if( !(addr_tcp=cxSockAddrNew("127.0.0.1",SOCK_TST_PORT)) ) {
		printf( "\n!!ERROR!! %s:%d: cxSockAddrNew() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto SOCKEVENT_ERR;	
	}
	if( strcmp(cxSockAddrGetAddr(addr_tcp),myip)!=0 ) {
		printf( "\n!!ERROR!! %s:%d: cxSockAddrGetAddr() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto SOCKEVENT_ERR;
	}
	if( cxSockAddrGetPort(addr_tcp)!=SOCK_TST_PORT ) {
		printf( "\n!!ERROR!! %s:%d: cxSockAddrGetPort() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto SOCKEVENT_ERR;
	}

	/* open TCP socket */
	s_tcp=cxSockNew(CX_SOCK_SERVER,addr_tcp);
	if( !s_tcp ) {
		printf( "\n!!ERROR!! %s:%d: cxSockNew() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto SOCKEVENT_ERR;

	}

	/* open UDP socket */
	if( !(addr_udp=cxSockAddrNew("127.0.0.1",SOCK_TST_PORT)) ) {
		printf( "\n!!ERROR!! %s:%d: cxSockAddrNew() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto SOCKEVENT_ERR;	
	}
	s_udp = cxSockNew(CX_SOCK_DGRAM, addr_udp);
	if( !s_udp ) {
		printf( "\n!!ERROR!! %s:%d: cxSockNew() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto SOCKEVENT_ERR;
	}

	if( !(g_evn = cxEventNew()) ) {
		printf( "\n!!ERROR!! %s:%d: cxEventNew() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto SOCKEVENT_ERR;
	}
	if( cxEventRegister(g_evn, s_tcp, CX_EVENT_RD, sock_tcp_accept, NULL)!=RC_OK ) {
		printf( "\n!!ERROR!! %s:%d: cxEventRegister() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto SOCKEVENT_ERR;
	}
	if( cxEventRegister(g_evn, s_udp, CX_EVENT_RD, sock_udp_tst_cb, NULL)!=RC_OK ) {
		printf( "\n!!ERROR!! %s:%d: cxEventRegister() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto SOCKEVENT_ERR;
	}

	t = cxThreadCreate(sock_client_tst,NULL);

	/* do cxEventDispatch() for 21 times,
	   one for sock_tcp_accept(), ten for TCP data, and ten for UDP data. */
	for(i=0;i<21;i++) 
		cxEventDispatch(g_evn, 1000);

	if( sock_tcp_msgcnt<10 ) { /* receive TCP data tem times */
		printf( "\n!!ERROR!! %s:%d: TCP receive error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
	}
	if( sock_udp_msgcnt<10 ) { /* receive UDP data tem times */
		printf( "\n!!ERROR!! %s:%d: UDP receive loss.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
	}

	/* test cxSockRecvEx() timeout */ 
	if( cxSockRecvEx(newsock, buff, 100, 2000 )!=-2 ) {
		printf( "\n!!ERROR!! %s:%d: cxSockRecvEx() error %d.\n",
			__CMFILE__,__LINE__,i);
		ret = RC_ERROR;
		goto SOCKEVENT_ERR;
	}

	cxThreadJoin(t);
	cxSockFree(c_tcp);
	cxSockFree(c_udp);
	
	cxSockFree(s_tcp);
	cxSockFree(newsock);
	cxSockFree(s_udp);
	cxEventFree(g_evn);
	cxSockAddrFree(addr_tcp);
	cxSockAddrFree(addr_udp);

	if( cxSockClean()!=0 ) {
		printf( "\n!!ERROR!! %s:%d: cxSockClean() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
		goto SOCKEVENT_ERR;	
	}

SOCKEVENT_ERR:
	return ret;
}


RCODE	timer_ret = RC_OK;
int	timer_cb_flag = 0;
void	timer_tst_cb(int i, void* arg)
{
	struct timeval	*delay = (struct timeval*)arg;
	timer_cb_flag = 1;
	
	if(delay->tv_sec!=1||delay->tv_usec!=0) {
		printf( "\n!!ERROR!! %s:%d: CxTimer callback parameter error.\n",
			__CMFILE__,__LINE__);
		timer_ret = RC_ERROR;
	}
}

RCODE	accTimer(void)
{
	int		e;
	struct timeval	delay;
	struct timeval*	prm;

	if( cxTimerInit(10)!=RC_OK ) {
		printf( "\n!!ERROR!! %s:%d: cxTimerInit() error.\n",
			__CMFILE__,__LINE__);
		timer_ret = RC_ERROR;
		goto TIMER_ERR;
	}

	delay.tv_sec = 1;
	delay.tv_usec = 0;

	e = cxTimerSet(delay, timer_tst_cb, &delay);
	if( e==-1 ) {
		printf( "\n!!ERROR!! %s:%d: cxTimerSet() error.\n",
			__CMFILE__,__LINE__);
		timer_ret = RC_ERROR;
		goto TIMER_ERR;
	}
	sleeping(1100);
	if( timer_cb_flag!=1 ) {
		printf( "\n!!ERROR!! %s:%d: cxTimer callback error.\n",
			__CMFILE__,__LINE__);
		timer_ret = RC_ERROR;
		goto TIMER_ERR;
	}

	timer_cb_flag = 0;
	e = cxTimerSet(delay, timer_tst_cb, &delay);
	if( e==-1 ) {
		printf( "\n!!ERROR!! %s:%d: cxTimerSet() error.\n",
			__CMFILE__,__LINE__);
		timer_ret = RC_ERROR;
		goto TIMER_ERR;
	}
	prm = cxTimerDel(e);
	if( prm==NULL ) {
		printf( "\n!!ERROR!! %s:%d: cxTimerDel() error.\n",
			__CMFILE__,__LINE__);
		timer_ret = RC_ERROR;
		goto TIMER_ERR;
	}
	sleeping(1500);
	if( timer_cb_flag==1 ) {
		printf( "\n!!ERROR!! %s:%d: cxTimerDel() error.\n",
			__CMFILE__,__LINE__);
		timer_ret = RC_ERROR;
		goto TIMER_ERR;
	}

	if( cxTimerClean()!=RC_OK ) {
		printf( "\n!!ERROR!! %s:%d: cxTimerClean() error.\n",
			__CMFILE__,__LINE__);
		timer_ret = RC_ERROR;
		goto TIMER_ERR;
	}
TIMER_ERR:
	return timer_ret;
}

#ifndef  _WIN32_WCE

RCODE accMem()
{
	RCODE mem_ret=RC_OK;
	unsigned long iAllocationCount=0;
	double duration;
	int i=0;
	void * arrayVoidPointer [1024];
	clock_t starttime,endtime;

	/* Seed the random-number generator with current time so that
	* the numbers will be different every time we run.
	*/
	srand((unsigned int)clock());

	cxMemInit();
	cxMemDisplayInfo ();

	for ( i = 0; i < 1024; ++i ) { 
		arrayVoidPointer [i] = NULL;
	}

	starttime=clock();
	do {
		int j = rand () % 1024;
		
		if ( arrayVoidPointer [j] ) {
			if (j%2) {
				cxMemFree ( arrayVoidPointer [j] );
				arrayVoidPointer [j] = NULL;
			}else{
				arrayVoidPointer [j] = cxMemRealloc(arrayVoidPointer [j],rand () % 961 + 40);
			}
		}
		else {
			arrayVoidPointer [j] = cxMemMalloc ( rand () % 961 + 40 );
			if ( arrayVoidPointer [j] ) {
				++iAllocationCount;
			}
			else {
				mem_ret=RC_ERROR;
				break;
			}
		}
	} while ( iAllocationCount < 100000 );
	endtime=clock();
	duration = (double)(endtime - starttime) / CLOCKS_PER_SEC;
	printf( "memcpy:%f seconds\n", duration );
	cxMemDisplayInfo ();
	for ( i = 0; i < 1024; ++i ) {
		if ( arrayVoidPointer [i] ) {
			cxMemFree ( arrayVoidPointer [i] );
			arrayVoidPointer [i] = NULL;
		}
	}
	printf("after cxMemfree!\n");
	cxMemDisplayInfo ();
	cxMemClean ();
	return mem_ret;
}

#endif /* _WIN32_WCE */

RCODE	accLow(void)
{
	RCODE	ret = RC_OK;

	printf( "\n-------------------------------------------------------------\n"
		"<low> module test\n"
		"-------------------------------------------------------------\n"); 

	printf( "\nCASE cx_1: test <cx_misc> module ... ");
	if( accMisc()==RC_OK )
		printf("!!SUCCESS!!\n");
	else
		ret = RC_ERROR;
	
	printf( "\nCASE cx_2: test <cx_thrd> module ... "); 
	if( accThread()==RC_OK )
		printf("!!SUCCESS!!\n");
	else
		ret = RC_ERROR;

	printf( "\nCASE cx_3: test <cx_mutex> module ... "); 
	if( accMutex()==RC_OK )
		printf("!!SUCCESS!!\n");
	else
		ret = RC_ERROR;

	printf( "\nCASE cx_4: test <cx_sock> and <cx_event> module ... "); 
	if( accSockEvent()==RC_OK )
		printf("!!SUCCESS!!\n");
	else
		ret = RC_ERROR;

	printf("\nCASE cx_5: test <cx_timer> module ... "); 
	if( accTimer()==RC_OK )
		printf("!!SUCCESS!!\n");
	else
		ret = RC_ERROR;

#ifndef  _WIN32_WCE
	printf("\nCASE cx_6: test <cx_mem> module ... "); 
	if( accMem()==RC_OK )
		printf("!!SUCCESS!!\n");
	else
		ret = RC_ERROR;
#endif /* _WIN32_WCE */

	return ret;
}

