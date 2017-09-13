/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * cclRtp.c
 *
 * $Id: cclRtp.c,v 1.11 2006/11/01 03:07:08 shinjan Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(UNIX)
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include "cclRtp.h"
#include <low/cx_thrd.h>
#include <low/cx_mutex.h>
#include <low/cx_misc.h>
#include <common/cm_trace.h>

#define FT_NO_TRANSMIT  0
#define FT_VOICE        1
#define FT_SID          2
#define FT_TONE         3


FILE	*ptr = NULL;
int		g_debug=0;
int		m_JBLength = 0;
BOOL	m_start = FALSE;
int		m_frameNumber = 0;
int		m_tolerance = 0;

CxMutex*	jb = NULL;

struct Jitter_Buffer	*JB_Head = NULL;
struct Jitter_Buffer	*JB_Tail = NULL;
unsigned short			m_cur_seq = 0;
unsigned int			m_cur_ts = 0;
unsigned int			m_cur_sample = 0;


RTP**		rtp = NULL;
RTCP**		rtcp = NULL;
UINT8*		g_payload_type  = NULL; /* payload type */
int*		g_rtp_packet_siz = NULL;
int*            g_packetize_period = NULL;
int*            g_timestamp = NULL;
CxMutex*	cs = NULL;
int		MAX_RTP_CHANS = 0;
CxThread	hthreadRead = NULL;
int		threadReadON = 0;
int		(*rtp_read_cb)(int channel
                               , const char* buff
                               , int len);
int		(*rtp_read_cb_ex)(int channel
                                  , rtp_hdr_t* hdr
                                  , const char* buff
                                  , int len);
int		(*rtp_read_cb_pkt)(int channel, rtp_pkt_t* pkt);

void*		_threadRead(void* data)
{
	fd_set		rtpfdset;
	SOCKET		rtpsock;
	rtp_pkt_t	pkt_recv;
	rtp_pkt_param_t	param;
	UINT32		maxfd = 0;
	struct timeval	timeout;
	int		i = 0;
	int		read_buff_siz = DEFAULT_READ_BUFFER_SIZ;
	
	rtp_pkt_init(&pkt_recv, read_buff_siz );
	memset(&param, 0, sizeof(param));

	while(threadReadON) {
		int ready;

		FD_ZERO(&rtpfdset);
		for( i = 0; i<MAX_RTP_CHANS; i++) {
			if( !rtp[i] )
				continue;

			rtpsock = rtp_getsocket(rtp[i]);
			FD_SET(rtpsock, &rtpfdset);
			if( maxfd<(int)rtpsock )
				maxfd = rtpsock;
		}
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		ready = select( maxfd+1, &rtpfdset, (fd_set *) 0,
				(fd_set *) 0, &timeout);
		
		if( ready<=0 ) {
			/* no rtp sock is open, just continue */
			sleeping(1);
			continue;
		}

		if( rtp_read_cb==NULL && rtp_read_cb_ex==NULL)
			continue;
		for( i = 0; i<MAX_RTP_CHANS; i++) {
			if( !rtp[i] )
				continue;
			if( g_rtp_packet_siz[i]>(read_buff_siz/2) ) {
				rtp_pkt_free(&pkt_recv);
				read_buff_siz = g_rtp_packet_siz[i] * 2;
				rtp_pkt_init(&pkt_recv, read_buff_siz );
			}

			rtpsock = rtp_getsocket(rtp[i]);
			if (FD_ISSET(rtpsock, &rtpfdset)) {
				int	rlen = 0;
				/* receive RTP packets */
				rlen = rtp_read(rtp[i], &pkt_recv);
				if( rlen<=0 )
					continue;
				
				if( rtp_read_cb ) {
					rtp_read_cb( i
					             , pkt_recv.data
					             , pkt_recv.datalen);
				}
				if( rtp_read_cb_ex ) {
					rtp_read_cb_ex( i
					                , (rtp_hdr_t*)(pkt_recv.buf)
					                , pkt_recv.data
					                , pkt_recv.datalen);					
				}
				if ( rtp_read_cb_pkt ) {
					rtp_read_cb_pkt( i, &pkt_recv );
				}
			}
		}
		sleeping(5); // added by sjhuang 2006/03/14
	}
	printf("End RTP Read thread\n");
	rtp_pkt_free(&pkt_recv);
	
	return NULL;
}

RCODE	cclRTPStartup(int maxRtpChans)
{
	int	ret = 0;
	int	i = 0;

	if( rtp )
		return RC_ERROR;

	ret = rtp_startup();
	if( ret!=0 )
		return ret;

	MAX_RTP_CHANS = maxRtpChans;

	g_rtp_packet_siz = (int*)malloc( (MAX_RTP_CHANS * sizeof(int)) );
	g_packetize_period = (int*)malloc( (MAX_RTP_CHANS * sizeof(int)) );
	g_timestamp = (int*)malloc( (MAX_RTP_CHANS * sizeof(int)) );
	rtp = (RTP**)malloc( (MAX_RTP_CHANS * sizeof(void*)) );
	rtcp = (RTCP**)malloc( (MAX_RTP_CHANS * sizeof(void*)) );
	cs = (CxMutex*)malloc( (MAX_RTP_CHANS * sizeof(void*)) );
	g_payload_type = (UINT8*)malloc( (MAX_RTP_CHANS * sizeof(UINT8)) );
	for( i=0; i<MAX_RTP_CHANS; i++) {
		g_rtp_packet_siz[i] = 160;            /* default */
		g_packetize_period[i] = 20;           /* default */
		g_timestamp[i] = 0;                   /* default */
		rtp[i] = NULL;                      
		rtcp[i] = NULL;
		g_payload_type[i] = 8;
		cs[i] = cxMutexNew(CX_MUTEX_INTERTHREAD,0);
	}

	return RC_OK;
}

void	cclRTPCleanup(void)
{
	int i;

	if( !rtp )
		return;
	if( hthreadRead ) {
		threadReadON = 0;
		sleep(1);
		//cxThreadJoin(hthreadRead);
		cxThreadCancel(hthreadRead);
		hthreadRead = NULL;
	}

	for( i=0; i<MAX_RTP_CHANS; i++) {
		if( rtp[i]!=NULL ) 
			cclRTPClose(i);
		cxMutexFree(cs[i]);
	}
	
	rtp_cleanup();
	free(g_rtp_packet_siz); g_rtp_packet_siz = NULL;
	free(g_packetize_period); g_packetize_period = NULL;
	free(g_timestamp); g_timestamp = NULL;
	free(rtp); rtp = NULL;
	free(rtcp); rtcp = NULL;
	free(g_payload_type); g_payload_type = NULL;
	free(cs); cs = NULL;

	MAX_RTP_CHANS = 0;
}

/*  Fun: cclRTPOpen
 *
 *  Desc: Open a RTP channel.
 *
 *  Ret: return = 0, success,
 *       return < 0, error,
 *       return > 0, already opened, new #rtpPacketSiz will
 *		     take effort.
 */
RCODE	cclRTPOpen(int channel,
			const char* localaddr,
		   int rtpPacketSiz,          /* bytes */
		   int packetizePeriod,       /* msec */
		   UINT8 payloadType,
			short addressFamily,
		   int nTos)
{
	char	name[64];
	char	temp[64];

	if( channel<0 || channel>=MAX_RTP_CHANS )
		return -1;

	cxMutexLock(cs[channel]);
	g_rtp_packet_siz[channel] = rtpPacketSiz;
	g_packetize_period[channel] = packetizePeriod;
	g_timestamp[channel] = 0;
	g_payload_type[channel] = payloadType;
	if( rtp[channel] ) {
		cxMutexUnlock(cs[channel]);
		return 1;
	}

	if( gethostname(temp,64)==SOCKET_ERROR ) {
		TCRPrint(TRACE_LEVEL_APP,"<cclRTPOpen>:gethostname() error!");
		sprintf(name,"RTP_localhost_%d",channel);
	}
	else
		sprintf(name,"RTP_%s_%d",temp,channel);
	rtp[channel] = rtp_open(localaddr, addressFamily,0,0,(const char*)name);
	if( !rtp[channel] ) {
		cxMutexUnlock(cs[channel]);
		return -1;
	}

	// specify ToS bit for audio RTP stream
	rtp_settos( rtp[channel], nTos);
	cxMutexUnlock(cs[channel]);

	return 0;
}

RCODE	cclRTPClose(int channel)
{
	int	ret = 0;
	RTP*	rtp_temp;
	RTCP*	rtcp_temp;

	if( channel<0 || channel>=MAX_RTP_CHANS )
		return -1;

	cxMutexLock(cs[channel]);
	if( !rtp[channel] ) {
		cxMutexUnlock(cs[channel]);
		return 0;
	}

	rtp_temp = rtp[channel];	rtcp_temp = rtcp[channel];
	rtp[channel] = NULL;		rtcp[channel] = NULL;

	ret = rtp_close(rtp_temp);
	if( ret!=0 ) {
		cxMutexUnlock(cs[channel]);
		return -1; 
	}
	cxMutexUnlock(cs[channel]);
	
	return 0;
}

UINT32	cclRTPGetPort(int channel)
{
	if( channel<0 || channel>=MAX_RTP_CHANS )
		return -1;

	return rtp_get_port( rtp[channel] );
}


RCODE	cclRTPGetRR(int channel, rtcp_rr_t* rr)
{
	if( channel<0 || channel>=MAX_RTP_CHANS )
		return -1;

	return 0;
}

void	cclRTPSetEventHandler( int(*rtpReadCB)(int,const char*,int) )
{
	rtp_read_cb = rtpReadCB;

	if( hthreadRead ) {
		threadReadON = 0;
		cxThreadJoin(hthreadRead);
		hthreadRead = NULL;
	}
	threadReadON = 1;
	hthreadRead = cxThreadCreate(_threadRead,NULL);
}

void	cclRTPSetEventHandlerEx( int(*rtpReadCBEx)(int,rtp_hdr_t*,const char*,int) )
{
	rtp_read_cb_ex = rtpReadCBEx;

	if( hthreadRead ) {
		threadReadON = 0;
		cxThreadJoin(hthreadRead);
		hthreadRead = NULL;
	}
	threadReadON = 1;
	hthreadRead = cxThreadCreate(_threadRead,NULL);
}

void	cclRTPSetEventHandlerPkt( int(*rtpReadCBPkt)(int,rtp_pkt_t*) )
{
	rtp_read_cb_pkt = rtpReadCBPkt;

	if( hthreadRead ) {
		threadReadON = 0;
		cxThreadJoin(hthreadRead);
		hthreadRead = NULL;
	}
	threadReadON = 1;
	hthreadRead = cxThreadCreate(_threadRead,NULL);
}

unsigned long gettickcount(int channel)
{
	if( g_timestamp[channel]==0 ) {

#ifdef	UNIX		/* [ for UNIX ] */
		struct timeval	tv;
		gettimeofday(&tv, NULL);
		g_timestamp[channel] = (tv.tv_sec * 1000 + tv.tv_usec / 1000);
		return g_timestamp[channel];
#elif	defined(WIN32)  /* [ for Windows ] */
		return g_timestamp[channel] = GetTickCount();
#endif			/* [ #ifdef UNIX ] */

	}
	else
		return (g_timestamp[channel] += g_packetize_period[channel]); 
}

int	cclRTPWriteEx(int channel, rtp_pkt_param_t* parm, const char* buff, int len)
{

	rtp_pkt_t	pkt;
	rtp_pkt_param_t	param;
	int		retlen = 0;

	if( channel<0 || channel>=MAX_RTP_CHANS || len <0 )
		return RC_ERROR;

	cxMutexLock(cs[channel]);
	if( !rtp[channel] ) {
		cxMutexUnlock(cs[channel]);
		return RC_ERROR;
	}

	rtp_pkt_init(&pkt, len * 2);
	pkt.datalen = len;
	memset(&param, 0, sizeof(param));

	if( !parm ) {
		param.ts = gettickcount(channel) * 8;
		param.pt = g_payload_type[channel];
		rtp_pkt_setparam(&pkt, &param);
	}
	else {
		rtp_pkt_setparam(&pkt, parm);
	}
	memcpy(pkt.data,(void*)buff,len);
	




	retlen = rtp_write(rtp[channel], &pkt);
	if( retlen>(int)pkt.datalen ) 
		retlen = pkt.datalen;
	
	rtp_pkt_free(&pkt);
	cxMutexUnlock(cs[channel]);

	return retlen;
}

RCODE	cclRTPAddReceiver(int channel, const char* addr, unsigned short port, short af)
{
	ADDRINFO *ai,hints;
	char Port[16];
	int	ret = RC_OK;

	memset(&hints,0,sizeof(hints));
	hints.ai_family = af;
	sprintf(Port,"%d",port);

	if( channel<0 || channel>=MAX_RTP_CHANS || !addr )
		return -1;
	cxMutexLock(cs[channel]);
	if( !rtp[channel] ) {
		cxMutexUnlock(cs[channel]);
		return RC_ERROR;
	}

	if(getaddrinfo(addr,Port,&hints,&ai) != 0){
		cxMutexUnlock(cs[channel]);
		return RC_ERROR;
	}

	ret = rtp_add_receiver(rtp[channel], ai->ai_addr, ai->ai_addrlen);
	cxMutexUnlock(cs[channel]);
	freeaddrinfo(ai);

	return ret;
}

RCODE	cclRTPStat(int channel)
{
	rtp_stat_t mystat;
	RCODE ret = RC_ERROR;

	if( channel<0 || channel>=MAX_RTP_CHANS)
		return RC_ERROR;
	
	cxMutexLock(cs[channel]);
	if( !rtp[channel] ) {
		cxMutexUnlock(cs[channel]);
		return RC_ERROR;
	}

	if ( rtp_stat(rtp[channel], &mystat, 1) > 0 ) {
		ret = RC_OK;

		TCRPrint(TRACE_LEVEL_API, "[RTP]\tSSRC = %d\n\
						\t\t\tBYTES_SENT = %d\n\
						\t\t\tBYTES_RECV = %d\n\
						\t\t\tPKTS_SENT = %d\n\
						\t\t\tPKTS_RECV = %d\n\
						\t\t\tJitter = %d\n\
						\t\t\tPKT_LOST = %d\n\
						\t\t\tLOST_FRACTION = %d\n",
						mystat.ssrc,
						mystat.bytes_sent,
						mystat.bytes_recv,
						mystat.pkt_sent,
						mystat.pkt_recv,
						mystat.jitter,
						mystat.pkt_lost,
						mystat.lost_fraction);
	}

	cxMutexUnlock(cs[channel]);
	return ret;
}


int	cclPutRTP2JB(char *payload,int size,int type,unsigned short seq,int frameNumber,unsigned int ts, unsigned int sample, int debug)
{
//	char debugMsg[255];
	int drop=0;
	int i;
	struct Jitter_Buffer *tmp=NULL;
	struct Jitter_Buffer *inHead=NULL,*inTail=NULL,*inTmp=NULL;
	struct Jitter_Buffer *tmp2=NULL;


	if( JB_Head==NULL )
	{
		jb = (CxMutex*)malloc( (sizeof(void*)) );
		jb[0] = cxMutexNew(CX_MUTEX_INTERTHREAD,0);

		JB_Head = (struct Jitter_Buffer*)malloc(sizeof(struct Jitter_Buffer));
		JB_Head->count = 0;
		JB_Head->FrameType = -1;
		JB_Head->seq = 0;
		JB_Head->ts = 0;
		JB_Head->next = NULL;
		memset(JB_Head->payload,0x00,DEFAULT_READ_BUFFER_SIZ);

		JB_Tail = (struct Jitter_Buffer*)malloc(sizeof(struct Jitter_Buffer));
		JB_Tail->count = 0;
		JB_Tail->FrameType = -1;
		JB_Tail->seq = 0;
		JB_Tail->ts = 0;
		JB_Tail->next = NULL;
		memset(JB_Tail->payload,0x00,DEFAULT_READ_BUFFER_SIZ);

		m_JBLength = 0;
		m_cur_seq = 0;
		m_cur_ts = ts - sample;
		m_cur_sample = sample;
		m_start = FALSE;
		ptr = NULL;
		g_debug = debug;

#ifdef DEBUG_DEBUG
		if( g_debug )
			ptr = fopen("C:\\zz_jitter_buffer.txt","w");
#endif
		
		if( ptr )
			fprintf(ptr,"JB_Head==NULL, new it \n");
	}

/*
	// sjhuang test
	// check local wish sequence & RTP packet sequence
	if( type!=FT_NO_TRANSMIT && m_start ) 
	if( m_cur_seq > seq )
	{
		if( (m_cur_ts+m_cur_sample)!=ts )
		{
			if( ptr )
				fprintf(ptr,"\n\n==>[PUT Attention] ==> (m_cur_seq > seq) && (m_cur_ts+m_cur_sample)!=ts ==> m_cur_seq:%d this seq:%d m_cur_ts:%u this ts:%u \n\n",m_cur_seq,seq,m_cur_ts,ts);
		}
		else
		{
			// tolerance delay
			// ToDo: tolerance design ?
			if( cclGetJBLength()>=10 )
			{
				if( ptr )
					fprintf(ptr,"\n\n==>[PUT Error] ==> (m_cur_seq > seq) && (m_cur_ts+m_cur_sample)==ts ==> m_cur_seq:%d this seq:%d \n\n",m_cur_seq,seq);
			
				//return 0;
			}
			else
			{
				if( ptr )
					fprintf(ptr,"\n\n==>[PUT Attention] ==> JBLength:%d \n\n",cclGetJBLength());
			}
		}		
	}
*/

	if( type==FT_VOICE ) 
	{
		m_frameNumber = frameNumber;
	}
	else if( type==FT_SID ) 
	{
		if( ptr )
			fprintf(ptr,"\n\n ==>[PUT] get SID Frame ==> seq:%d  \n\n",seq);
		
		m_frameNumber = 1;
	}

	// seperate RTP payload to temp link-list
	inTmp=NULL;
	for( i=0;i<m_frameNumber;i++)
	{
		inTmp = (struct Jitter_Buffer*)malloc(sizeof(struct Jitter_Buffer));
		inTmp->count = size/m_frameNumber;
		inTmp->FrameType = type;
		inTmp->seq = seq;
		inTmp->ts = ts;
		inTmp->next = NULL;
	

		if( type==FT_VOICE || type==FT_SID )
			memcpy(inTmp->payload,payload+i*inTmp->count,inTmp->count);
		
		if( i==0 ) 
		{
			inHead = inTmp;
			inTail = inTmp;
		}
		else
		{
			inTail->next = inTmp;
			inTail = inTmp;
		}

		//if( ptr )
		//	fprintf(ptr,"==>[PUT] seperate RTP payload to temp link-list %d seq:%d type:%d \n",i,seq,type);
	
	}
	
	// combine temp link-list to jitter buffer link-list
	tmp = inHead;
	if( tmp==NULL ) return 0;
	// add to link list
	
	cxMutexLock(jb[0]);

	// first time
	if( JB_Head==NULL )
	{
		cxMutexUnlock(jb[0]);
		return 0;	
	}

	if( JB_Head->next==NULL ) {
		JB_Head->next = tmp;
		JB_Tail->next = inTail;
		
		// initial it
		m_JBLength=0;
		m_start=FALSE;

		if( ptr )
			fprintf(ptr,"==>[PUT] JB_Head->next==NULL, first RTP payload ==> seq:%d  \n",JB_Head->next->seq);
	} else {

		// quick search
		if( seq > JB_Tail->next->seq )
		{
			JB_Tail->next->next = tmp;
			JB_Tail->next = inTail;
			m_JBLength += frameNumber;

			
			if( ptr )
				fprintf(ptr,"==>[PUT] cclPutRTP2JB, quick search buffer length: %d JB_Head seq:%d JB_Tail seq:%d \n",m_JBLength,JB_Head->next->seq,JB_Tail->next->seq);
			
			cxMutexUnlock(jb[0]);

			return size;	
		}

		
		for(tmp2=JB_Head->next;tmp2!=NULL;tmp2 = tmp2->next)
		{
			// if smaller then first
			// if bigger then first
			//   if only one item, insert after.
			//   if two item, caculator property position
			if( seq < tmp2->seq )
			{
				if( seq > m_cur_seq /*&& !m_start*/ )
				{
					inTail->next = tmp2;
					JB_Head->next = inHead;

					if( ptr )
						fprintf(ptr,"\n\n ==>[PUT] ==> smaller then first ==> seq:%d type:%d \n\n",JB_Head->next->seq,JB_Head->next->FrameType);
					break;
				}
				else
				{
					// too late, drop it.
					if( ptr )
						fprintf(ptr,"\n\n [PUT Error] ==> too late, drop it ==> seq:%d \n\n",seq);

					cclDropRTP(tmp);
					drop=1;
					size=0;
					break;
				}
			}

			if( seq > tmp2->seq )
			{
				if( tmp2->next==NULL )
				{
					tmp2->next = tmp;
					if( ptr )
						fprintf(ptr,"==>[PUT] insert into tail ==> seq:%d \n",tmp2->next->seq);
					break;
				} 
				else
				{
					if( seq < tmp2->next->seq )
					{
						tmp->next = tmp2->next;
						tmp2->next = tmp;
						if( ptr )
							fprintf(ptr,"\n\n ==>[PUT]==> insert %d before %d  \n\n",seq,tmp->next->seq);
						break;
					}
				}

			}
			
		}
	}

	if( !drop ) m_JBLength += frameNumber;
	
	if( ptr )
		fprintf(ptr,"==>[PUT] cclPutRTP2JB, buffer length: %d JB_Head seq:%d JB_Tail seq:%d\n",m_JBLength,JB_Head->next->seq,JB_Tail->next->seq);

	cxMutexUnlock(jb[0]);
	return size;

}
void cclGetRTPFromJB(char *buf,int *type,int *length)
{
	struct Jitter_Buffer *tmp;

	if( JB_Head==NULL ) 
	{
		*type=-2;
		return ;
	}
	if( JB_Head->next==NULL /*&& m_start*/) 
	{
		// no RTP Data, update sequence and timestamp
		*type=-1;

		if(m_start) m_cur_seq++;
		
		return ;
	}

	if( !m_start ) 
	{
		m_start = TRUE;
		m_cur_seq = JB_Head->next->seq-1;
	}
	
	if( (m_cur_seq+1) != JB_Head->next->seq && m_cur_seq!=JB_Head->next->seq ) // may be have packet lost, insert NO_TRANSFER
	{
		if( ptr )
			fprintf(ptr,"\n\n <==[GET Error] ==> FT_NO_TRANSMIT ==> should seq:%d this seq:%d \n\n",m_cur_seq+1,JB_Head->next->seq);
		
		// ToDO:
		/*if( cclGetJBLength() > rtpThreshold*2 )
		{
			// buffer to big, don't need to put FT_NO_TRANSMIT packet..
		}*/

		cclPutRTP2JB(NULL,0,FT_NO_TRANSMIT,m_cur_seq+1,m_frameNumber,0,m_cur_sample,g_debug);
	}

	cxMutexLock(jb[0]);

	// may be critical section
	m_cur_ts = JB_Head->next->ts;// m_cur_sample;
	m_cur_seq = JB_Head->next->seq;
	tmp = JB_Head->next;
	memcpy(buf,tmp->payload,tmp->count);
	*type = tmp->FrameType;
	*length = tmp->count;
	JB_Head->next = JB_Head->next->next;

	free(tmp);
	
	m_JBLength--;

	if( ptr )
		fprintf(ptr,"<==[GET] cclGetRTPFromJB, type:%d seq:%d buffer length: %d \n",*type,m_cur_seq,m_JBLength);

	cxMutexUnlock(jb[0]);
}

int cclRemoveJB(int size)
{
	struct Jitter_Buffer *tmp;
	int count=0;
	int i=0;

	if( JB_Head==NULL ) {
		if( ptr )
			fprintf(ptr,"\n\n Error [Remove] ==> Remove Jitter Buffer:%d <== \n\n",count);
		return count;
	}
	if( size<=0 )
	{
		if( ptr )
			fprintf(ptr,"\n\n Error [Remove] ==> Remove Jitter Buffer:%d <== \n\n",count);
		return count;
	}

	cxMutexLock(jb[0]);

	i=0;
	for(tmp=JB_Head->next;tmp!=NULL && i<size;tmp=JB_Head->next,i++)
	{
		JB_Head->next = tmp->next;
		free(tmp);
		count++;
	}
	m_start = FALSE;
	m_JBLength -= count;

	cxMutexUnlock(jb[0]);

	if( ptr )
		fprintf(ptr,"\n\n [Remove] ==> Remove Jitter Buffer:%d <== \n\n",count);
	
	
	return count;
}

int	cclClearJB(void)
{
	struct Jitter_Buffer *tmp;
	int count=0;

	if( JB_Head==NULL ) {
		if( ptr )
			fprintf(ptr,"\n\n [Clean JB_Head==NULL] ==> clean Jitter Buffer:%d <== \n\n",count);
		return count;
	}

	cxMutexLock(jb[0]);

	for(tmp=JB_Head->next;tmp!=NULL;tmp=JB_Head->next)
	{
		JB_Head->next = tmp->next;
		free(tmp);
		count++;
	}
	if( JB_Head )
		free(JB_Head); 
	JB_Head = NULL;
	if( JB_Tail )
		free(JB_Tail); 
	JB_Tail = NULL;
	
	m_JBLength = 0;
	m_cur_seq = 0;
	m_start = FALSE;
	ptr = NULL;

	cxMutexUnlock(jb[0]);


	cxMutexFree(jb[0]);
	free(jb); jb = NULL;

	if( ptr )
		fprintf(ptr,"\n\n [Clean] ==> clean Jitter Buffer:%d <== \n\n",count);

	if( ptr )
	{
		fclose(ptr);
		ptr = NULL;
	}
	
	
	
	return count;
}

void cclDropRTP(struct Jitter_Buffer *inHead)
{
	unsigned short seq = inHead->seq;
	int count=0;
	struct Jitter_Buffer *tmp;

	for(tmp=inHead;tmp!=NULL;tmp=inHead)
	{
		inHead = tmp->next;
		free(tmp);
		count++;
	}

	if( ptr )
		fprintf(ptr,"\n\n [Drop] ==> Drop RTP seq:%d count:%d <== \n\n",seq,count);
}

int cclGetJBLength(void)
{
	int length=-1;

	length = m_JBLength;

	return length;
}
