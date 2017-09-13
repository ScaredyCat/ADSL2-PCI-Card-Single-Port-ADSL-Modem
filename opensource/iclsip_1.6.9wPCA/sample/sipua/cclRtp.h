/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * cclRtp.h
 *
 * $Id: cclRtp.h,v 1.9 2006/08/04 02:50:54 shinjan Exp $
 */

#ifndef CCLRTP_H
#define CCLRTP_H

#include <rtp/rtp.h>
#include <common/cm_def.h>

#ifdef  __cplusplus
extern "C" {
#endif


#define		DEFAULT_READ_BUFFER_SIZ	512

struct Jitter_Buffer {
	int		FrameType;
	int		count;
	unsigned int	ts;
	unsigned short	seq;
	char	payload[DEFAULT_READ_BUFFER_SIZ];
	struct	Jitter_Buffer *next;
};



int		cclPutRTP2JB(char *payload,int size,int type,unsigned short seq,int frameNumber,unsigned int ts, unsigned int sample, int debug);
void	cclGetRTPFromJB(char *buf,int *type,int *length);
int		cclClearJB(void);
void	cclDropRTP(struct Jitter_Buffer *inHead);
int		cclGetJBLength(void);
int		cclRemoveJB(int count);

RCODE	cclRTPStartup(int maxRtpChans);

void	cclRTPCleanup(void);

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
		   int nTos);

RCODE	cclRTPClose(int channel);

UINT32	cclRTPGetPort(int channel);

RCODE	cclRTPGetRR(int channel, rtcp_rr_t* rr);

void	cclRTPSetEventHandler( int(*rtpReadCB)(int,const char*,int) );

void	cclRTPSetEventHandlerEx( int(*rtpReadCBEx)(int,rtp_hdr_t*,const char*,int) );

void	cclRTPSetEventHandlerPkt( int(*rtpReadCBPkt)(int,rtp_pkt_t*) );

#define	cclRTPWrite(a,b,c) cclRTPWriteEx(a,NULL,b,c)

RCODE	cclRTPWriteEx(int channel, rtp_pkt_param_t* parm, const char* buff, int len);

RCODE	cclRTPAddReceiver(int channel, const char* addr, unsigned short port, short af);

unsigned long gettickcount(int channel);

RCODE	cclRTPStat(int channel);

#ifdef  __cplusplus
}
#endif

#endif /* CCLRTP_H */

