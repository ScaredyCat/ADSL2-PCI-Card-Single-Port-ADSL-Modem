/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * cclRtp.h
 *
 * $
 */

#ifndef CCLRTP_H
#define CCLRTP_H

#include <rtp/rtp.h>
#include <common/cm_def.h>

#ifdef  __cplusplus
extern "C" {
#endif


//	cclRTPVideoStartup(int maxRtpChans)
//	cclRTPVideoOpen(int channel,int rtpPacketSiz/*bytes*/,int packetizePeriod/*msec*/,UINT8 payloadType);				//For RTP Packet Initial
//	RCODE	cclRTPVideoWrite(int channel, const char* buff, int len);				//For Encoder Net out	
//	void	cclRTPVideoSetEventHandler( int(*rtpReadCB)(int,const char*,int) );		//For Decoder Net in
//	RCODE	cclRTPVideoClose(int channel);
//	void	cclRTPVideoCleanup(void);

RCODE	cclRTPVideoStartup(int maxRtpChans);

void	cclRTPVideoCleanup(void);

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
			const char* localadddr,
		   int rtpPacketSiz,          /* bytes */
		   int packetizePeriod,       /* msec */
		   UINT8 payloadType,
			short addressFamily,
		   int nTos);

RCODE	cclRTPVideoClose(int channel);

UINT32	cclRTPVideoGetPort(int channel);

RCODE	cclRTPVideoGetRR(int channel, rtcp_rr_t* rr);

void	cclRTPVideoSetEventHandler( int(*rtpReadCB)(int,const char*,int) );

RCODE	cclRTPVideoWrite(int channel, const char* buff, int len);

int	cclRTPVideoWriteEx(int channel, rtp_pkt_param_t* parm, const char* buff, int len);

RCODE	cclRTPVideoAddReceiver(int channel, char* addr, unsigned short port, short af);

#ifdef  __cplusplus
}
#endif

#endif /* CCLRTP_H */

