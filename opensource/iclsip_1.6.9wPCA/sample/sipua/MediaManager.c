// MediaManager.cpp: implementation of the CMediaManager class.
//
//////////////////////////////////////////////////////////////////////
#include <UACore/ua_core.h>

#include "CallManager.h"
#include "MediaManager.h"
#include "UACommon.h"
//#include "UAProfile.h"
//#include "UAControl.h"
#include "math.h"
//#include "IniFile.H"

#include "ifx_common.h"
#include "../../../ifx/ifx_httpd/ifx_amazon_cfg.h"

#ifdef UACOM_USE_DIRECTX
#include "DxInOut.h"
#endif

//! media codec
#define USE_MIDDLEWARE 0

// for test/debug, sjhuang 2006/02
//#define DEVELOP_JITTER

#ifdef DEBUG_DEBUG
#define DEBUG_AUDIO2
#define SAVE_STREAMING
#endif

//#ifdef DEBUG_AUDIO
BOOL start_play=FALSE;
//#endif

// wave IO Module
#undef UACOM_WAVETHREAD
//#include "WaveThread.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define  L_FRAME_COMPRESSED 10
#define  L_FRAME            80
#define  L_FRAME_MAX_G723_BIN 24

unsigned int LastPutTimestamp = 0;
unsigned int LastBufferTimestamp = 0;

unsigned char nullbuf[160];

int blankframe[24];
int totalblank;
int rtpbuflen;
int rtpthreshold;


unsigned int RTPGetPort()
{
	return cclRTPGetPort(m_RtpChan);
}

RCODE RTPOpenPort(WAV_FORMAT wavetype)
{
	
	int nTos = 0;
	
	return cclRTPOpen(m_RtpChan, m_LocalAddr, m_RTPPacketsize, m_RTPPacketizePeriod, wavetype, PF_UNSPEC, nTos);
	//return cclRTPOpen(m_RtpChan, m_RTPPacketsize, m_RTPPacketizePeriod, wavetype, AF_INET, nTos);
}
// reset statistic buffer with expected data rate
void _ClearStatBuf()
{
	int datarate;
	switch ( m_WaveType)
	{
	case WAV_G723:	datarate = 800;		// fixed in 800 Bps
		break;
	case WAV_G729:	datarate = 1000;	// fixed in 1000 Bps
		break;
	case WAV_PCMU:
	case WAV_PCMA:	datarate = 8000;	// fixed in 8000 Bps
		break;
	default:
		datarate = 0;	// not support type (NOTE: not support GSM)
	}

	time(&m_StatBufTimestamp);
	m_StatBuf[0] = 0;
	m_StatBuf[1] = datarate;
	m_StatBuf[2] = datarate;
	m_StatBuf[3] = datarate;	// this should be faster?
}
// shift m_StatBuf if current time > m_StatBufTimestamp
void _ShiftStatBuf()		
{
	time_t now;
	time(&now);
	int i,n,diff;

	diff = now - m_StatBufTimestamp;
	if ( diff == 0)
		return;

	m_StatBufTimestamp = now;

	// if last statistic record is before 3 seconds, or time changed
	if ( diff > 3 || diff < 0)
	{
		// clear all statistic buffer
		m_StatBuf[0] = 0;
		m_StatBuf[1] = 0;
		m_StatBuf[2] = 0;
		m_StatBuf[3] = 0;	// this should be faster?
		return;
	}

	// shift statistic buffer
	for ( i=3; i>=1; i--)
	{
		n = i-diff;
		m_StatBuf[i] = (n>=0)? m_StatBuf[n] : 0;
	}
	m_StatBuf[0] = 0;
}

// log received bytes
void _IncStatBuf( int n)
{
	_ShiftStatBuf();

	m_StatBuf[0] += n;
}
#if 0
// static operation
unsigned long int DSPThreadFunc(void* pParam)
{
	
	unsigned long int dwWait=0,lastts,curts,lastW2R,lastR2W,curtick;
	int framesize,ret;
	// 10ms 8000Hz, Mono, 16bit  [BYTESPERSAMPLE * SAMPLESPERFRAME]
	framesize = BYTESPERSAMPLE * cmediamanager->m_CodecPacketizePeriod * 8;

	// The code below have bugs, will cause system crash, by sjhuang 2006/07/28
//	if( cmediamanager->m_pRecord==NULL )
//	cmediamanager->m_pRecord = (unsigned char*) malloc (/*sizeof(unsigned char) **/ framesize*2);
	
//	if( cmediamanager->m_pPlay==NULL )
//	cmediamanager->m_pPlay = (unsigned char*) malloc (/*sizeof(unsigned char) **/ framesize*2);

	if( framesize>640 )
		framesize = 640;

	static char _record[640];
	static char _play[640];

	lastts=0;
	curts=0;
	lastW2R=0;
	lastR2W=0;
	curtick=0;

	while ( (dwWait = WaitForSingleObject(cmediamanager->m_hEventKill, 0)) != 0 )
	{

		if ( waveIn.ReadData(/*cmediamanager->m_pRecord*/(unsigned char*)_record, framesize) ) {

			Wav2RTP((char*)/*cmediamanager->m_pRecord*/_record, framesize);

			//channelControl();
			curtick = GetTickCount();
			lastW2R = curtick;


#ifdef DEBUG_AUDIO2
			if(printOutStream)
				fprintf(printOutStream,"w2R curtick:%d interval:%d size:%d \n",curtick,curtick-lastW2R,framesize);
#endif
			
			curtick = GetTickCount();


			if ( (ret=cmediamanager->RTP2Wav((char*)/*cmediamanager->m_pPlay*/_play, framesize)) > 0 ) {
				
#ifdef DEBUG_AUDIO2
				if(printOutStream)
				fprintf(printOutStream,"read -> R2W curtick:%d interval:%d ret:%d \n",curtick,curtick-lastR2W,ret);
#endif
				waveOut.WriteData(/*cmediamanager->m_pPlay*/(unsigned char*)_play, framesize);
				
				lastR2W = curtick;
			}
			else
			{
#ifdef DEBUG_AUDIO2
				if(printOutStream)
				fprintf(printOutStream,"\n\n Error read -> R2W curtick:%d interval:%d ret:%d \n",curtick,curtick-lastR2W,ret);
#endif
			}

			
		} // Dose it have enough interval between previous and this audio packet ? 
		
		else Sleep(2);

	}

	CloseHandle(cmediamanager->m_hDSPThread);
	cmediamanager->m_hDSPThread = NULL;

	return 0;
	
}
#endif
int RTPEventHandlerEx(int channel,rtp_hdr_t* param, const char* buffer,int buflen)
{

	int samples;
	
	// detect packet lost
	static unsigned int first_packet=0;
	static unsigned int last_packet=0;
	static unsigned int should_packet=0;
	static int total_packet=0,lost_packet=0;
	static unsigned long lastts=0,curts=0;
	static char debugMsg[500];

	// get sample
	if ( param->pt == 19)
	{
		//time( &cmediamanager->m_SilenceTimestamp);
		//cmediamanager->m_DataTimestamp = 0;
		return 0;
	}


	if( !start_play ) {
		switch ( param->pt ) {
			case 0: // 711U
				samples = buflen; // 80 samples = 80bytes;
			break;
			case 3: // GSM
				samples = buflen; // 80 samples = 80bytes;
			break;
			case 4: // G723
				samples = buflen * 10; // 240samples = 24bytes;
			break;
			case 8: // 711A
				samples = buflen; // 80 samples = 80bytes;
			break;
			case 18: // G729
				samples = buflen * 8; // 80 samples = 10bytes;
			break;
		}

		start_play=TRUE;
		first_packet=0;
		last_packet=0;
		should_packet=0;
		total_packet=0;
		lost_packet=0;
		lastts=0,curts=0;
		LastPutTimestamp=0;
		LastPutTimestamp = ntohl(param->ts) - samples;
	}
	
	last_packet = (ntohl(param->seq)>>16 & 0x0000ffff);
	total_packet++;

	// to make null data for RTP2Wav generate NO_TRANSFER command to MiddleWare...
	if( (should_packet+1) != last_packet && first_packet!=0) {
		lost_packet += (last_packet-(should_packet+1));

#ifdef DEBUG_AUDIO2
		if( printOutStream )
		fprintf(printOutStream, "\n\nError:(packet lost)==> should_packet:%u, this pacekt:%u, packet lost:%d <== \n\n",
									(should_packet+1), last_packet, lost_packet);
#endif

	}

	should_packet = last_packet;
	
	if( first_packet==0 )
		should_packet = first_packet = (ntohl(param->seq)>>16 & 0xffff);
	
#ifdef DEBUG_AUDIO2	
	curts = GetTickCount();
	if( lastts!=0 && curts-lastts >= buflen*rtpthreshold ) // maybe should be threshold value
	{

		if( LastPutTimestamp+samples != ntohl(param->ts)) // timestamp should be according to payload type
		{
			sprintf(debugMsg,"Notics:(RTP Packet interval %d ms)==>last:%u cur:%u => RTP log: len=%d header=ver:%d,p=%d,x=%d,cc=%d,m=%d,pt=%d,seq=%u,ts=%u,first=%u,last=%u,l-f=%u,total=%d,rtp buflen:%d",
								curts-lastts,LastPutTimestamp,ntohl(param->ts),buflen, param->ver, param->p, param->x, param->cc,
								param->m, param->pt, (ntohl(param->seq)>>16 & 0xffff), ntohl(param->ts),first_packet,last_packet,last_packet-first_packet,total_packet,cclGetJBLength());
			if( printOutStream )
				fprintf(printOutStream,"\n\n %s \n\n",debugMsg);
	
			DebugMsg2UI(debugMsg);
		}
		else
		{
			
			sprintf(debugMsg, "Error:(Emergency RTP Packet interval %d ms)==> RTP log: len=%d header=ver:%d,p=%d,x=%d,cc=%d,m=%d,pt=%d,seq=%u,ts=%u,first=%u,last=%u,l-f=%u,total=%d,rtp buflen:%d",
								curts-lastts,buflen, param->ver, param->p, param->x, param->cc,
								param->m, param->pt, (ntohl(param->seq)>>16 & 0xffff), ntohl(param->ts),first_packet,last_packet,last_packet-first_packet,total_packet,cclGetJBLength() );

			if( printOutStream )
				fprintf(printOutStream,"\n\n %s \n\n",debugMsg);

			DebugMsg2UI(debugMsg);
		}

	} else {
		if( printOutStream )
		fprintf(printOutStream, "RTP log: len=%d header=ver:%d,p=%d,x=%d,cc=%d,m=%d,pt=%d,seq=%u,ts=%u,first=%u,last=%u,l-f=%u,total=%d rtp buflen:%d\n",
		buflen, param->ver, param->p, param->x, param->cc,
		param->m, param->pt, (ntohl(param->seq)>>16 & 0xffff), ntohl(param->ts),first_packet,last_packet,last_packet-first_packet,total_packet,cclGetJBLength() );
	}

	LastPutTimestamp = ntohl(param->ts);
	lastts = curts;
#endif

#ifdef DEVELOP_JITTER
	CIniFile m_ConfigFile(cmediamanager->m_strAppPath+"config.ini");	
	return RTPSequence( channel, param,  buffer, buflen, atoi(m_ConfigFile.ReadString("DEBUG", "JITTERBUFFER", "0")));
#else
	{

		dxBufferWrite(m_BufRTP2IO,(char*)buffer,buflen);
		if( dxBufferGetCurrLen(m_BufRTP2IO)>rtpthreshold && !bMyFlag)
		{
			bMyFlag = TRUE;
		}
		// sam add: calc data receive rate in bytes per seconds
		_IncStatBuf( buflen);
		time( m_DataTimestamp);
	}
#endif // end of DEVELOP_JITTER

}

void CMediaManager_init()
{
	if (m_bInitialized)
		return;
		
	int ret = cclRTPStartup(2);
	
	if (RC_OK == ret) {
		printf("[MediaManager] RTP initialize successful.\n");
	} else {
		printf("[MediaManager] RTP initialize fail!");
		printf("error code = %d\n", ret);
	}
	
	cclRTPSetEventHandlerEx(RTPEventHandlerEx);
	//cclRTPSetEventHandlerPkt(RTPEventHandlerPkt);

	m_BufRTP2IO = dxBufferNew(2000);
	//m_BufIO2RTP = dxBufferNew(4096);
	//m_HashRTP2IO = dxHashNew(10, FALSE, DX_HASH_POINTER);

	//m_WaveIOObj = WavInOut_instance();
	
	/* Associate WAVE stream with RTP stream */
	//m_WaveIOObj->init(Wav2RTP,RTP2Wav);

#if !USE_MIDDLEWARE	
	//g729_init_encoder();
	//g729_init_decoder();

	//g723_initial_encoder();
	//g723_initial_decoder();
	

	//Audio_EncodeInit(AUDIO_G723);
	//Audio_DecodeInit(AUDIO_G723);

	//iLBC_initEncoder(&Enc_Inst, 20);
	//iLBC_initDecoder(&Dec_Inst, 20, 1);
#endif

	// marked by sjhuang, 2006/11/14, when change network interface, it will cause
	// rtp packet can't receive success.
	RTPOpenPort(WAV_PCMU);

#ifndef _WIN32_WCE
//	ReadTonePCMRes();
#endif

#ifdef UACOM_USE_DIRECTX
	if (FAILED(DxInOutInit( CUAControl_GetControl()->m_hWnd ))) {
		DebugMsg("[MediaManager] DxInOut initialize fail!");
	}
#endif

#if USE_MIDDLEWARE

	t_dspDebugerCfg dsp_debuger;
    
	dsp_debuger.vdb_recoder_op      = 0;
	dsp_debuger.codecs_recoder_op   = 0;
	dsp_debuger.ec_recoder_op       = 0;
	dsp_debuger.PDB_debuger_op      = 0;
	dsp_debuger.TDB_debuger_op      = 0;

	channelControl_Init();

	dspDebugerCfg(0, &dsp_debuger);
#endif
	
	//DxInOutInit( CUAControl_GetControl()->m_hWnd );

	m_bInitialized = TRUE;
}
