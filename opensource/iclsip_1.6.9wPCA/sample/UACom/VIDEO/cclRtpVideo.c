/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * cclRtp.c
 *
 * $Id: cclRtpVideo.c,v 1.8 2006/08/04 02:50:54 shinjan Exp $
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

#include "cclRtpVideo.h"
#include <low/cx_thrd.h>
#include <low/cx_mutex.h>
#include <low/cx_misc.h>
#include <common/cm_trace.h>

#define		DEFAULT_READ_BUFFER_SIZ	1800

RTP**		videortp;
RTCP**		videortcp;
UINT8*		g_video_payload_type;
int*		g_video_rtp_packet_siz;
int*		g_video_packetize_period;
int*		g_video_timestamp ;
CxMutex*	videocs;
int		MAX_VIDEO_RTP_CHANS;
CxThread	hvideothreadRead;
int		videothreadReadON;

int	(*rtp_video_read_cb)(int channel, const char* buff, int len);


void*	_threadReadVideo(void* data)
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

	while(videothreadReadON) {
		int ready;

		FD_ZERO(&rtpfdset);

		for( i = 0; i<MAX_VIDEO_RTP_CHANS; i++) {
			if( !videortp[i] )
				continue;

			rtpsock = rtp_getsocket(videortp[i]);
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

		if( rtp_video_read_cb==NULL )		
			continue;
		for( i = 0; i<MAX_VIDEO_RTP_CHANS; i++) {

			if( !videortp[i] )
				continue;
			if( g_video_rtp_packet_siz[i]>(read_buff_siz/2) ) {
				rtp_pkt_free(&pkt_recv);
				read_buff_siz = g_video_rtp_packet_siz[i] * 2;
				rtp_pkt_init(&pkt_recv, read_buff_siz );
			}

			rtpsock = rtp_getsocket(videortp[i]);
			if (FD_ISSET(rtpsock, &rtpfdset)) {
				int	rlen = 0;
				/* receive RTP packets */
				rlen = rtp_read(videortp[i], &pkt_recv);
				if( rlen<=0 )
					continue;
				
				if( pkt_recv.datalen<=1 )
					continue;
				 

				rtp_video_read_cb(i, pkt_recv.data-12, pkt_recv.datalen+12);
				//rtp_video_read_cb(i, pkt_recv.data, pkt_recv.datalen);
				
			}
		}
		Sleep(5);
	}
	printf("End RTP Read thread\n");
	rtp_pkt_free(&pkt_recv);

	return NULL;
}

RCODE	cclRTPVideoStartup(int maxRtpChans)
{
	int	ret = 0;
	int	i = 0;

	if( videortp )
		return RC_ERROR;

	ret = rtp_startup();
	if( ret!=0 )
		return RC_ERROR;

	MAX_VIDEO_RTP_CHANS = maxRtpChans;

	g_video_rtp_packet_siz = (int*)malloc( (MAX_VIDEO_RTP_CHANS * sizeof(int)) );
	g_video_packetize_period = (int*)malloc( (MAX_VIDEO_RTP_CHANS * sizeof(int)) );
	g_video_timestamp = (int*)malloc( (MAX_VIDEO_RTP_CHANS * sizeof(int)) );
	videortp = (RTP**)malloc( (MAX_VIDEO_RTP_CHANS * sizeof(void*)) );
	videortcp = (RTCP**)malloc( (MAX_VIDEO_RTP_CHANS * sizeof(void*)) );
	videocs = (CxMutex*)malloc( (MAX_VIDEO_RTP_CHANS * sizeof(void*)) );
	g_video_payload_type = (UINT8*)malloc( (MAX_VIDEO_RTP_CHANS * sizeof(UINT8)) );
	for( i=0; i<MAX_VIDEO_RTP_CHANS; i++) {
		g_video_rtp_packet_siz[i] = 160;            /* default */
		g_video_packetize_period[i] = 20;           /* default */
		g_video_payload_type[i] = 34;
		//g_video_rtp_packet_siz[i] = 0;				//CCKAO            
		//g_video_packetize_period[i] =30;
		//g_video_payload_type[i] = 8;
		g_video_timestamp[i] = 0;                   /* default */
		videortp[i] = NULL;                      
		videortcp[i] = NULL;
		videocs[i] = cxMutexNew(CX_MUTEX_INTERTHREAD,0);
	}
	return RC_OK;
}

void	cclRTPVideoCleanup(void)
{
	int i;

	if( !videortp )
		return;
	if( hvideothreadRead ) {
		videothreadReadON = 0;
		Sleep(1000);
		//cxThreadJoin(hvideothreadRead);
		cxThreadCancel(hvideothreadRead);
		hvideothreadRead = NULL;
	}

	for( i=0; i<MAX_VIDEO_RTP_CHANS; i++) {
		if( videortp[i]!=NULL ) 
			cclRTPVideoClose(i);
		cxMutexFree(videocs[i]);
	}
	
	rtp_cleanup();
	free(g_video_rtp_packet_siz); g_video_rtp_packet_siz = NULL;
	free(g_video_packetize_period); g_video_packetize_period = NULL;
	free(g_video_timestamp); g_video_timestamp = NULL;
	free(videortp); videortp = NULL;
	free(videortcp); videortcp = NULL;
	free(g_video_payload_type); g_video_payload_type = NULL;
	free(videocs); videocs = NULL;

	MAX_VIDEO_RTP_CHANS = 0;
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
RCODE	cclRTPVideoOpen(int channel,
			const char* localaddr,
		   int rtpPacketSiz,          /* bytes */
		   int packetizePeriod,       /* msec */
		   UINT8 payloadType,
			short addressFamily,
			int nTos)
{
	char	name[64];
	char	temp[64];

	if( channel<0 || channel>=MAX_VIDEO_RTP_CHANS )
		return -1;

	cxMutexLock(videocs[channel]);
	
	g_video_rtp_packet_siz[channel] = rtpPacketSiz;
	g_video_packetize_period[channel] = packetizePeriod;
	g_video_timestamp[channel] = 0;
	g_video_payload_type[channel] = payloadType;
	
	if( videortp[channel] ) {
		cxMutexUnlock(videocs[channel]);
		return 1;
	}

	if( gethostname(temp,64)==SOCKET_ERROR ) {
		TCRPrint(1,"<cclRTPOpen>:gethostname() error!");
		sprintf(name,"RTP_localhost_%d",channel);
	}
	else
		sprintf(name,"RTP_%s_%d",temp,channel);
	videortp[channel] = rtp_open(localaddr, addressFamily, 0,0,(const char*)name);
	if( !videortp[channel] ) {
		cxMutexUnlock(videocs[channel]);
		return -1;
	}
	
	// specify ToS bit for audio RTP stream
	rtp_settos( videortp[channel], nTos);
	cxMutexUnlock(videocs[channel]);

	return 0;
}

RCODE	cclRTPVideoClose(int channel)
{
	int	ret = 0;
	RTP*	rtp_temp;
	RTCP*	rtcp_temp;

	if( channel<0 || channel>=MAX_VIDEO_RTP_CHANS )
		return -1;

	cxMutexLock(videocs[channel]);
	if( !videortp[channel] ) {
		cxMutexUnlock(videocs[channel]);
		return 0;
	}

	rtp_temp = videortp[channel];	rtcp_temp = videortcp[channel];
	videortp[channel] = NULL;		videortcp[channel] = NULL;

	ret = rtp_close(rtp_temp);
	if( ret!=0 ) {
		cxMutexUnlock(videocs[channel]);
		return -1; 
	}
	cxMutexUnlock(videocs[channel]);
	
	return 0;
}

UINT32	cclRTPVideoGetPort(int channel)
{
	if( channel<0 || channel>=MAX_VIDEO_RTP_CHANS )
		return -1;

	return rtp_get_port( videortp[channel] );
	//return (videortp[channel])?ntohs(((struct sockaddr_in*)&(videortp[channel]->laddr))->sin_port):0;
}

RCODE	cclRTPVideoGetRR(int channel, rtcp_rr_t* rr)
{
	if( channel<0 || channel>=MAX_VIDEO_RTP_CHANS )
		return -1;

	return 0;
}

void	cclRTPVideoSetEventHandler( int(*rtpReadCB)(int,const char*,int) )
{
	rtp_video_read_cb = rtpReadCB;

	if( hvideothreadRead ) {
		videothreadReadON = 0;
		cxThreadJoin(hvideothreadRead);
		hvideothreadRead = NULL;
	}
	videothreadReadON = 1;
	hvideothreadRead = cxThreadCreate(_threadReadVideo,NULL);
}

unsigned long _gettickcount(int channel)
{
	if( g_video_timestamp[channel]==0 ) {

#ifdef	UNIX		/* [ for UNIX ] */
		struct timeval	tv;
		gettimeofday(&tv, NULL);
		g_video_timestamp[channel] = (tv.tv_sec * 1000 + tv.tv_usec / 1000);
		return g_video_timestamp[channel];
#elif	defined(WIN32)  /* [ for Windows ] */
		return g_video_timestamp[channel] = GetTickCount();
#endif			/* [ #ifdef UNIX ] */

	}
	else
		return (g_video_timestamp[channel] += g_video_packetize_period[channel]); 
}

int	cclRTPVideoWrite(int channel, const char* buff, int len)
{
	rtp_pkt_t	pkt;
	static test =1;
	int		retlen = 0;

	if( channel<0 || channel>=MAX_VIDEO_RTP_CHANS || len <0 )
		return RC_ERROR;

	cxMutexLock(videocs[channel]);
	if( !videortp[channel] ) {
		cxMutexUnlock(videocs[channel]);
		return RC_ERROR;
	}

	pkt.buf		= (char*)buff;		//|RTP Header|	<- buf    		
	pkt.buflen	= len;			//|Payload H.|	<- data
	pkt.data	= (char*)buff+12;	//|H263Stream|
	pkt.datalen	= len-12;	
		
	retlen = rtp_write(videortp[channel], &pkt);
	if( retlen>(int)pkt.datalen ) 
		retlen = pkt.datalen;
	
	//rtp_pkt_free(&pkt);
	cxMutexUnlock(videocs[channel]);

	return retlen;
}

int	cclRTPVideoWriteEx(int channel, rtp_pkt_param_t* parm, const char* buff, int len)
{
	rtp_pkt_t	pkt;
	rtp_pkt_param_t	param;
	int		retlen = 0;

	if( channel<0 || channel>=MAX_VIDEO_RTP_CHANS || len <0 )
		return RC_ERROR;

	cxMutexLock(videocs[channel]);
	if( !videortp[channel] ) {
		cxMutexUnlock(videocs[channel]);
		return RC_ERROR;
	}

	rtp_pkt_init(&pkt, len * 2);
	pkt.datalen = len;
	memset(&param, 0, sizeof(param));

	if( !parm ) {
		param.ts = _gettickcount(channel) * 8;
		param.pt = g_video_payload_type[channel];
		rtp_pkt_setparam(&pkt, &param);
	}
	else {
		rtp_pkt_setparam(&pkt, parm);
	}
	memcpy(pkt.data,(void*)buff,len);
	
	retlen = rtp_write(videortp[channel], &pkt);
	if( retlen>(int)pkt.datalen ) 
		retlen = pkt.datalen;
	
	rtp_pkt_free(&pkt);
	cxMutexUnlock(videocs[channel]);

	return retlen;
}

RCODE	cclRTPVideoAddReceiver(int channel, char* addr, unsigned short port, short af)
{
	int			ret = RC_OK;
	
	char Port[16];
	ADDRINFO *ai, hints;

	memset(&hints,0,sizeof(hints));
	hints.ai_family = af;

	sprintf(Port,"%d",port);

	if( channel<0 || channel>=MAX_VIDEO_RTP_CHANS || !addr )
		return -1;
	cxMutexLock(videocs[channel]);
	if( !videortp[channel] ) {
		cxMutexUnlock(videocs[channel]);
		return RC_ERROR;
	}

	if(getaddrinfo(addr,Port,&hints,&ai) != 0){
		cxMutexUnlock(videocs[channel]);
		return RC_ERROR;
	}

	ret = rtp_add_receiver(videortp[channel],(const struct sockaddr*)(ai->ai_addr),ai->ai_addrlen);
	
	cxMutexUnlock(videocs[channel]);
	freeaddrinfo(ai);

	return ret;
}

