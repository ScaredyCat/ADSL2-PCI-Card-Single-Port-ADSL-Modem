// MediaManager.cpp: implementation of the CMediaManager class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MediaManager.h"
#include "UACommon.h"
#include "UAProfile.h"
#include "UAControl.h"
#include "math.h"
#include "IniFile.H"

//#ifdef UACOM_USE_DIRECTX
#include "DxInOut.h"
//#endif

//! media codec
#define USE_MIDDLEWARE 0

// for test/debug, sjhuang 2006/02
#define DEVELOP_JITTER

#ifdef DEBUG_DEBUG
#define DEBUG_AUDIO2
#define SAVE_STREAMING
#endif

//#ifdef DEBUG_AUDIO
BOOL start_play=FALSE;
//#endif

// wave IO Module
#define UACOM_WAVETHREAD
#include "WaveThread.h"

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

CWaveInThread  waveIn;
CWaveOutThread waveOut;

// Used to reset the data buffer on BUFFER_FULL events
void CALLBACK waveInEventCB(WAVE_IN_EVENT waveInEvent)
{
   switch (waveInEvent)
   {
      case WAVE_IN_EVENT_BUFFER_FULL:
         waveIn.ResetBuffer();
         break;
      case WAVE_IN_EVENT_NONE:
      case WAVE_IN_EVENT_NEW_BUFFER:
      default:
         break;
   }
}


// Used to reset the data buffer on BUFFER_EMPTY events
void CALLBACK waveOutEventCB(WAVE_OUT_EVENT waveOutEvent)
{
   switch (waveOutEvent)
   {
      case WAVE_OUT_EVENT_BUFFER_EMPTY:
         waveOut.ResetBuffer();
         break;
      case WAVE_OUT_EVENT_NONE:
      case WAVE_OUT_EVENT_BUFFER_PLAYED:
      default:
         break;
   }
}

struct Tone_t {
	char buf[2048];
	int	buflen;
} G723Tone[12],G729Tone[12], ToneA[12], ToneU[12];

t_event event_g729[] = {
//	{timeStamp, commandID,             serialNumber,    channelID,      param0,             param1};
	{   0,      CMD_CHAN_DEACTIVATE,         5,             0,          0,                  0     },
	{   0,      CMD_CHAN_CONFIGURE,         10,             0,          VPM_CODEC_G729AB,   0     },
	{   0,      CMD_CHAN_ACTIVATE,          20,             0,          0,                  0     },
	{  -1,	     0,                          0,             0,          0,                  0     } // timeStamp : -1 mean last event
};

t_event event_g723[] = {
//	{timeStamp, commandID,             serialNumber,    channelID,      param0,             param1};
	{   0,      CMD_CHAN_DEACTIVATE,         5,             0,          0,                  0     },
	{   0,      CMD_CHAN_CONFIGURE,         10,             0,          VPM_CODEC_G723_1A,  0     },
	{   0,      CMD_SET_CODING_RATE,        20,             0,          0,                  0     },
	{   0,      CMD_CHAN_ACTIVATE,          30,             0,          0,                  0     },
	{  -1,	     0,                          0,             0,          0,                  0     } // timeStamp : -1 mean last event
};

t_event event_g711u[] = {
//	{timeStamp, commandID,             serialNumber,    channelID,      param0,             param1};
	{   0,      CMD_CHAN_DEACTIVATE,         5,             0,          0,                  0     },
	{   0,      CMD_CHAN_CONFIGURE,         10,             0,          VPM_CODEC_G711_ULAW,0     },
	{   0,      CMD_CHAN_ACTIVATE,          20,             0,          0,                  0     },
	{  -1,	     0,                          0,             0,          0,                  0     } // timeStamp : -1 mean last event
};

t_event event_g711a[] = {
//	{timeStamp, commandID,             serialNumber,    channelID,      param0,             param1};
	{   0,      CMD_CHAN_DEACTIVATE,         5,             0,          0,                  0     },
	{   0,      CMD_CHAN_CONFIGURE,         10,             0,          VPM_CODEC_G711_ALAW,0     },
	{   0,      CMD_CHAN_ACTIVATE,          20,             0,          0,                  0     },
	{  -1,	     0,                          0,             0,          0,                  0     } // timeStamp : -1 mean last event
};


typedef struct _rtpframe {
	unsigned int ts;
	char buf[160];
} * rtpframe;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

static CMediaManager* cmediamanager = NULL;

#ifdef UACOM_USE_G711
static G711Codec* g_G711Codec = NULL;
#endif

static WavInOut* pWaveIOObj = NULL;
unsigned int CMediaManager::RTPLen = 0;
unsigned int CMediaManager::TotalPacket = 0;
char CMediaManager::RTPBuffer[4096];
BOOL CMediaManager::bPacketReceived = FALSE;
BOOL CMediaManager::bRetry = FALSE;
BOOL CMediaManager::bMyFlag = FALSE;

iLBC_Enc_Inst_t Enc_Inst;
iLBC_Dec_Inst_t Dec_Inst;

CMediaManager* CMediaManager::s_pMediaManager = NULL;


typedef struct {
	UINT32 event		:8;       /* event type */
#ifdef	WORDS_BIGENDIAN
	UINT32 e		:1;       /* end bit */
	UINT32 r		:1;       /* reserved bit */
	UINT32 volume		:6;       /* volume */
#else	/* little endian */
	UINT32 volume		:6;       /* volume */
	UINT32 r		:1;       /* reserved bit */
	UINT32 e		:1;       /* end bit */
#endif
	UINT32 duration		:16;      /* duration */
} DTMFPayloadFormat;

FILE *printOutStream = NULL;

CMediaManager::CMediaManager()
{
	cmediamanager = this;
	s_pMediaManager = this;
	m_OwnerDlg = NULL;

#ifdef UACOM_USE_G711
	m_G711Codec = new G711Codec;
	g_G711Codec = m_G711Codec;
#endif
	m_RtpChan = 0;
	m_WaveType = 0;
	m_ToneType = -1;
	m_idx = 0;
	m_IsPlayingRTP = FALSE;
	m_bInitialized = FALSE;
	m_WaveIOObj = NULL;
	m_BufRTP2IO = NULL;
	m_BufIO2RTP = NULL;
	m_HashRTP2IO = NULL;
	m_RTPPacketsize = 0;
	m_RTPPacketizePeriod = 0;
	m_CodecPacketizePeriod = 0;
	m_BitsPerSample = 0;

	m_DTMFState = DTMF_IDLE; 
	m_DTMFDuration = 0;

	m_hDSPThread = NULL;

	//memset((void*)RTPBuffer, 0, 4096);
	
	// sam add: calc data receive rate in bytes per seconds
	_ClearStatBuf();
	m_StatBufTimestamp = 0;
	m_DataTimestamp = 0;
	m_SilenceTimestamp = 0;

	bPacketReceived = FALSE;

	memset(nullbuf, 0, 160);

	totalblank = 0;

	for (int i = 0; i < 24; i++)
		blankframe[i] = -1;

	// sjhuang 2006/02/21
	m_hEventKill = CreateEvent(NULL, FALSE, FALSE, NULL);
	printOutStream=NULL;

	// get the running module path
	GetModuleFileName( GetModuleHandle(NULL), m_strAppPath.GetBuffer(1024), 1024);
	m_strAppPath.ReleaseBuffer();
	int pos = m_strAppPath.ReverseFind('\\') + 1;
	m_strAppPath.Delete( pos, m_strAppPath.GetLength() - pos);
}

CMediaManager::~CMediaManager()
{
	
	// Destroy events
   CloseHandle(m_hEventKill);
  

	cclRTPCleanup();
	
#ifdef UACOM_USE_G711
	if (m_G711Codec != NULL)
		delete m_G711Codec;
#endif

	if (m_WaveIOObj != NULL)
		delete m_WaveIOObj;
	
	if (m_BufRTP2IO != NULL)
		dxBufferFree(m_BufRTP2IO);

	if (m_BufIO2RTP != NULL)
		dxBufferFree(m_BufIO2RTP);

	/*
	if (m_HashRTP2IO ! = NULL)
		dxHashFree(m_HashRTP2IO, free);
	*/

	
	
}

#define SAMPLESPERFRAME 80
#define BYTESPERSAMPLE 2


// static operation
DWORD CMediaManager::DSPThreadFunc(LPVOID pParam)
{
	
	DWORD dwWait=0;

	// 10ms 8000Hz, Mono, 16bit  [BYTESPERSAMPLE * SAMPLESPERFRAME]
	int framesize = BYTESPERSAMPLE * cmediamanager->m_CodecPacketizePeriod * 8;

	// The code below have bugs, will cause system crash, by sjhuang 2006/07/28
//	if( cmediamanager->m_pRecord==NULL )
//	cmediamanager->m_pRecord = (unsigned char*) malloc (/*sizeof(unsigned char) **/ framesize*2);
	
//	if( cmediamanager->m_pPlay==NULL )
//	cmediamanager->m_pPlay = (unsigned char*) malloc (/*sizeof(unsigned char) **/ framesize*2);

	if( framesize>640 )
	{
		AfxMessageBox("DSPThreadFunc has error, framesize>640");
		framesize = 640;
	}

	static char _record[640];
	static char _play[640];

	DWORD lastts=0,curts=0;
	DWORD lastW2R=0,lastR2W=0,curtick=0;
	int ret;
	

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

// sjhuang
BOOL CMediaManager::StopWaveThread()
{
//   WORD i;

   if (m_hDSPThread == NULL)
   {
      return FALSE;
   }

    
   SetThreadPriority(m_hDSPThread, THREAD_PRIORITY_NORMAL);

   
   SetEvent(m_hEventKill);

 
   // This is ugly, wait for 10 seconds, then kill the thread
   DWORD res = WaitForSingleObject(m_hDSPThread, 500);

   if (res == WAIT_TIMEOUT)
   {
      if (m_hDSPThread)
      {
         // OK, do it the hard way
         TerminateThread(m_hDSPThread, 0);

         CloseHandle(m_hDSPThread);

         m_hDSPThread = NULL;
      }

	  return TRUE;
   }
	
   return TRUE;
}


// static operation
int CMediaManager::Wav2RTP(char* buff, int len)
{
	int xlen = 0;
	int ToneType;
	int retlen;
	int smpno_put = 0, smpno_get = 0, codecType, frameType;
	unsigned int packetlen, RtpChan, RTPPacketsize, WaveType, idx, CodecPacketsize;

	static unsigned char serial711a[G711_ALAW_BYTES_PER_FRAME];
	static unsigned char serial711u[G711_ULAW_BYTES_PER_FRAME];
	static unsigned char serial723[G723_1A_BYTES_PER_FRAME];
	static unsigned char serial729[G729AB_BYTES_PER_FRAME];
	
	short encoded_data[ILBCNOOFWORDS_MAX];
	int extpara;//ret;

	
#ifdef UACOM_USE_G711
	char pData[160];
#endif

	//fprintf(printOutStream, "%d [WAV2RTP] \tIO2RTP:\t\tsample data length = %d\n",GetTickCount(),len);

	RtpChan = cmediamanager->m_RtpChan;
	WaveType = cmediamanager->m_WaveType;
	ToneType = cmediamanager->m_ToneType;
	RTPPacketsize = cmediamanager->m_RTPPacketsize;
	idx = cmediamanager->m_idx;

	MyCString tmpStr;

	if ( RTPPacketsize == 0 )
		return 0;

	if (ToneType < 0) {
	
		len = ( len < 4096 )? len:4096;
		switch (WaveType) {
		
		case WAV_G723:

#if !USE_MIDDLEWARE		
			
			memset(serial723, 0, L_FRAME_MAX_G723_BIN);
			retlen = g723_encoder((short*)buff, (char*)serial723, 1);
			//retlen = Audio_Encode((short*)buff, serial, 0);
			
			// modified by ljchuang 08/10/2003
			memcpy((void*)(CMediaManager::RTPBuffer + CMediaManager::RTPLen), (const void*)serial723, retlen);
			CMediaManager::RTPLen += retlen;
						
			if (CMediaManager::RTPLen >= RTPPacketsize) {
				
				cmediamanager->bPacketReceived = TRUE;
				
				xlen = cclRTPWrite(RtpChan,(const char *)CMediaManager::RTPBuffer,CMediaManager::RTPLen);
				CMediaManager::RTPLen = 0;
				//memset((void*)CMediaManager::RTPBuffer, 0, 4096);
				CMediaManager::TotalPacket++;
			} else {
				
				//AtlTrace( "buffered\n");
				
				return 0;
			}
			break;

#else // use middle ware
			
			smpno_put = putPCMFrame(0, G723_1A_SAMPLES_PER_FRAME, (short*)buff);

			channelControl();

			//fprintf(printOutStream, "%d [WAV2RTP] \tputPCMFrame:\t\trqstSamples = %d, gotSamples = %d\n",GetTickCount(), G729AB_SAMPLES_PER_FRAME,smpno_put);

			smpno_get = getPacket(0, &codecType, serial723, &frameType, &extpara);

			//fprintf(printOutStream, "%d [WAV2RTP] \tgetPacket:\t\tpacketSize = %d\textpara = %d\n",GetTickCount(), smpno_get, extpara);

			if (smpno_get <= 0)
				return 0;
	

			memcpy((void*)(CMediaManager::RTPBuffer + CMediaManager::RTPLen), (const void*)serial723, smpno_get);
			CMediaManager::RTPLen += smpno_get;
						
			if (CMediaManager::RTPLen >= RTPPacketsize) {
				//fprintf(printOutStream, "%d [WAV2RTP] \tpayload size = %d\n", GetTickCount(), CMediaManager::RTPLen);
				cmediamanager->bPacketReceived = TRUE;
				xlen = cclRTPWrite(RtpChan,(const char *)CMediaManager::RTPBuffer,CMediaManager::RTPLen);
				CMediaManager::RTPLen = 0;
				CMediaManager::TotalPacket++;
			} else {
				return 0;
			}
			break;
#endif

		case WAV_G729:
			// Do compression

#if !USE_MIDDLEWARE				
			
			memset(serial729, 0, L_FRAME_COMPRESSED);
			g729_encoder((short*)buff, serial729);
			memcpy((void*)(CMediaManager::RTPBuffer + CMediaManager::RTPLen), (const void*)serial729, L_FRAME_COMPRESSED);
			CMediaManager::RTPLen += L_FRAME_COMPRESSED;
						
			if (CMediaManager::RTPLen >= RTPPacketsize) {
				cmediamanager->bPacketReceived = TRUE;

				xlen = cclRTPWrite(RtpChan,(const char *)CMediaManager::RTPBuffer,CMediaManager::RTPLen);
				CMediaManager::RTPLen = 0;
				//memset((void*)CMediaManager::RTPBuffer, 0, 4096);
				CMediaManager::TotalPacket++;
			} else {
				return 0;
			}
			break;
			
#else // use middle ware

			smpno_put = putPCMFrame(0, G729AB_SAMPLES_PER_FRAME, (short*)buff);

			channelControl();

			//fprintf(printOutStream, "%d [WAV2RTP] \tputPCMFrame:\t\trqstSamples = %d, gotSamples = %d\n",GetTickCount(), G729AB_SAMPLES_PER_FRAME,smpno_put);

			smpno_get = getPacket(0, &codecType, serial729, &frameType, &extpara);

			channelControl();

			//fprintf(printOutStream, "%d [WAV2RTP] \tgetPacket:\t\tpacketSize = %d\textpara = %d\n",GetTickCount(), smpno_get, extpara);

			if (smpno_get <= 0)
				return 0;

			memcpy((void*)(CMediaManager::RTPBuffer + CMediaManager::RTPLen), (const void*)serial729, smpno_get);
			CMediaManager::RTPLen += smpno_get;
						
			if (CMediaManager::RTPLen >= RTPPacketsize) {
				//fprintf(printOutStream, "%d [WAV2RTP] \tpayload size = %d\n", GetTickCount(), CMediaManager::RTPLen);
				cmediamanager->bPacketReceived = TRUE;
				xlen = cclRTPWrite(RtpChan,(const char *)CMediaManager::RTPBuffer,CMediaManager::RTPLen);
				CMediaManager::RTPLen = 0;
				CMediaManager::TotalPacket++;
			} else {
				return 0;
			}
			break;
#endif
		case WAV_GSM:
			memcpy((void*)(CMediaManager::RTPBuffer + CMediaManager::RTPLen), (const void*)buff, len);
			CMediaManager::RTPLen += len;
						
			if (CMediaManager::RTPLen >= RTPPacketsize) {
				xlen = cclRTPWrite(RtpChan,(const char *)CMediaManager::RTPBuffer,CMediaManager::RTPLen);
				CMediaManager::RTPLen = 0;
				//memset((void*)CMediaManager::RTPBuffer, 0, 4096);
				CMediaManager::TotalPacket++;
			} else {
				//AtlTrace( "buffered\n");
				return 0;
			}
			break;
			
		case WAV_iLBC:

			memset(encoded_data, 0, ILBCNOOFWORDS_MAX);
		
			retlen = iLBC_encode(&Enc_Inst, encoded_data, (short*)buff);
				
			memcpy((void*)(CMediaManager::RTPBuffer + CMediaManager::RTPLen), (const void*)encoded_data, retlen);
			CMediaManager::RTPLen += retlen;
						
			if (CMediaManager::RTPLen >= RTPPacketsize) {
				xlen = cclRTPWrite(RtpChan,(const char *)CMediaManager::RTPBuffer,CMediaManager::RTPLen);
				CMediaManager::RTPLen = 0;
				//memset((void*)CMediaManager::RTPBuffer, 0, 4096);
				CMediaManager::TotalPacket++;
			} else {
				//AtlTrace( "buffered\n");
				return 0;
			}
			break;

		case WAV_PCMA:

#if !USE_MIDDLEWARE	
		
		#ifdef UACOM_USE_G711			
			retlen = len / 2;
			g_G711Codec->Encode(buff, len, (char *)serial711a, &retlen);
			
			memcpy((void*)(CMediaManager::RTPBuffer + CMediaManager::RTPLen), (const void*)serial711a, retlen);
			CMediaManager::RTPLen += retlen;
			
			if (CMediaManager::RTPLen >= RTPPacketsize) {
				//fprintf(printOutStream, "%d [WAV2RTP] \tpayload size = %d\n", GetTickCount(), CMediaManager::RTPLen);
				cmediamanager->bPacketReceived = TRUE;
				xlen = cclRTPWrite(RtpChan,(const char *)CMediaManager::RTPBuffer,RTPPacketsize);
				CMediaManager::RTPLen = 0;
				CMediaManager::TotalPacket++;
			} else {
				return 0;
			}
			
			//memcpy((void*)(CMediaManager::RTPBuffer + CMediaManager::RTPLen), (const void*)serial711a, retlen);
			//CMediaManager::RTPLen += retlen;
		#else
			if ( (CMediaManager::RTPLen + len) > RTPPacketsize )
				return 0;
			memcpy((void*)(CMediaManager::RTPBuffer + CMediaManager::RTPLen), (const void*)buff, len);
			CMediaManager::RTPLen += len;
		#endif

		break;

#else // use middle ware

			smpno_put = putPCMFrame(0, G711_ALAW_SAMPLES_PER_FRAME, (short*)buff);

			channelControl();

			//fprintf(printOutStream, "%d [WAV2RTP] \tputPCMFrame:\t\trqstSamples = %d, gotSamples = %d\n",GetTickCount(), G729AB_SAMPLES_PER_FRAME,smpno_put);

			smpno_get = getPacket(0, &codecType, serial711a, &frameType, &extpara);

			//fprintf(printOutStream, "%d [WAV2RTP] \tgetPacket:\t\tpacketSize = %d\textpara = %d\n",GetTickCount(), smpno_get, extpara);

			if (smpno_get <= 0)
				return 0;

			memcpy((void*)(CMediaManager::RTPBuffer + CMediaManager::RTPLen), (const void*)serial711a, smpno_get);
			CMediaManager::RTPLen += smpno_get;

			if (CMediaManager::RTPLen >= RTPPacketsize) {
				//fprintf(printOutStream, "%d [WAV2RTP] \tpayload size = %d\n", GetTickCount(), CMediaManager::RTPLen);
				cmediamanager->bPacketReceived = TRUE;
				xlen = cclRTPWrite(RtpChan,(const char *)CMediaManager::RTPBuffer,RTPPacketsize);
				CMediaManager::RTPLen = 0;
				CMediaManager::TotalPacket++;
			} else {
				return 0;
			}
			break;

#endif // use middle ware or not
			
		case WAV_PCMU:
		default:
			
#if !USE_MIDDLEWARE	

		#ifdef UACOM_USE_G711			
			retlen = len / 2;
			g_G711Codec->Encode(buff, len, (char *)serial711u, &retlen);

			memcpy((void*)(CMediaManager::RTPBuffer + CMediaManager::RTPLen), (const void*)serial711u, retlen);
			CMediaManager::RTPLen += retlen;

			if (CMediaManager::RTPLen >= RTPPacketsize) {
				//fprintf(printOutStream, "%d [WAV2RTP] \tpayload size = %d\n", GetTickCount(), CMediaManager::RTPLen);
				cmediamanager->bPacketReceived = TRUE;
				xlen = cclRTPWrite(RtpChan,(const char *)CMediaManager::RTPBuffer,RTPPacketsize);
				CMediaManager::RTPLen = 0;
				CMediaManager::TotalPacket++;
			} else {
				return 0;
			}
			
			//memcpy((void*)(CMediaManager::RTPBuffer + CMediaManager::RTPLen), (const void*)serial711u, retlen);
			//CMediaManager::RTPLen += retlen;
		#else
			if ( (CMediaManager::RTPLen + len) > RTPPacketsize )
				return 0;
			memcpy((void*)(CMediaManager::RTPBuffer + CMediaManager::RTPLen), (const void*)buff, len);
			CMediaManager::RTPLen += len;
		#endif

		break;
		
#else // use middle ware

			smpno_put = putPCMFrame(0, G711_ULAW_SAMPLES_PER_FRAME, (short*)buff);

			channelControl();

			//fprintf(printOutStream, "%d [WAV2RTP] \tputPCMFrame:\t\trqstSamples = %d, gotSamples = %d\n",GetTickCount(), G729AB_SAMPLES_PER_FRAME,smpno_put);

			smpno_get = getPacket(0, &codecType, serial711u, &frameType, &extpara);

			//fprintf(printOutStream, "%d [WAV2RTP] \tgetPacket:\t\tpacketSize = %d\textpara = %d\n",GetTickCount(), smpno_get, extpara);

			if (smpno_get <= 0)
				return 0;


			memcpy((void*)(CMediaManager::RTPBuffer + CMediaManager::RTPLen), (const void*)serial711u, smpno_get);
			CMediaManager::RTPLen += smpno_get;

			if (CMediaManager::RTPLen >= RTPPacketsize) {
				//fprintf(printOutStream, "%d [WAV2RTP] \tpayload size = %d\n", GetTickCount(), CMediaManager::RTPLen);
				cmediamanager->bPacketReceived = TRUE;
				xlen = cclRTPWrite(RtpChan,(const char *)CMediaManager::RTPBuffer,RTPPacketsize);
				CMediaManager::RTPLen = 0;
				CMediaManager::TotalPacket++;
			} else {
				return 0;
			}
			break;
#endif

		}
	} else { // send DTMF tone
		switch (WaveType) {
		case WAV_PCMA:
			packetlen=ToneA[ToneType].buflen;
			CodecPacketsize = cmediamanager->m_CodecPacketizePeriod * 8;
			break;
		case WAV_PCMU:
			packetlen=ToneU[ToneType].buflen;
			CodecPacketsize = cmediamanager->m_CodecPacketizePeriod * 8;
			break;
		case WAV_G729:
			packetlen=G729Tone[ToneType].buflen;
			CodecPacketsize = cmediamanager->m_CodecPacketizePeriod;
			break;
		// add by tyhuang 2005/12/2
		case WAV_G723:
			packetlen=G723Tone[ToneType].buflen;
			CodecPacketsize = cmediamanager->m_CodecPacketizePeriod;
			break;
		default:
			break;
		}

		if ((packetlen - idx) <= CodecPacketsize) {
			len = packetlen - idx;  
			cmediamanager->m_ToneType = -1; // No more DTMF data
		} else {
			if(WaveType == WAV_G723)
				len = 24;
			else if(WaveType == WAV_G729)
				len = 10;
			else 
				len = CodecPacketsize;	
			cmediamanager->m_idx += len;	
		}

		switch (WaveType) {
		case WAV_PCMA:
			memcpy((void*)(CMediaManager::RTPBuffer + CMediaManager::RTPLen), &(ToneA[ToneType].buf[idx]), len);
			break;
		case WAV_PCMU:
			memcpy((void*)(CMediaManager::RTPBuffer + CMediaManager::RTPLen), &(ToneU[ToneType].buf[idx]), len);
			break;
		case WAV_G729:
			memcpy((void*)(CMediaManager::RTPBuffer + CMediaManager::RTPLen), &(G729Tone[ToneType].buf[idx]), len);
			break;
		// add by tyhuang 2005/12/2
		case WAV_G723:
			memcpy((void*)(CMediaManager::RTPBuffer + CMediaManager::RTPLen), &(G723Tone[ToneType].buf[idx]), len);
			break;
		default:
			break;
		}

		CMediaManager::RTPLen += len;

		if (CMediaManager::RTPLen >= RTPPacketsize) {
			xlen = cclRTPWrite(RtpChan,(const char *)CMediaManager::RTPBuffer,RTPPacketsize);
			CMediaManager::RTPLen = 0;
			//memset((void*)CMediaManager::RTPBuffer, 0, 4096);
			CMediaManager::TotalPacket++;
		} else {
			return 0;
		}
	}

	/* Send out-of-band DTMF tone */
	if ( cmediamanager->m_DTMFState != DTMF_IDLE ) {

		if ( cmediamanager->m_DTMFState == DTMF_PREPARE ) {
			memset(&(cmediamanager->m_DTMFparam), 0, sizeof(cmediamanager->m_DTMFparam));
			cmediamanager->m_DTMFparam.ts = gettickcount(RtpChan) * 8;
			cmediamanager->m_DTMFparam.pt = 101;
			cmediamanager->m_DTMFparam.m = 1;
			cmediamanager->m_DTMFState = DTMF_SENDING;
			cmediamanager->m_DTMFDuration = 0x0050; /* 800 units : 100ms * 8 */
		} else {
			cmediamanager->m_DTMFparam.m = 0;
			if ( cmediamanager->m_DTMFState == DTMF_SENDING )
				cmediamanager->m_DTMFDuration += 0x00A0; /* 1600 units : 200ms * 8 */
		}

		/* TEST */
		if ( (cmediamanager->m_DTMFDuration > 0x0320) && 
			 (cmediamanager->m_DTMFState == DTMF_SENDING) ) {
			cmediamanager->m_DTMFState = DTMF_LAST1;
		}
		/* TEST */

		DTMFPayloadFormat payload;
		
		payload.event = cmediamanager->m_DTMFEvent;
		payload.r = 0;
		payload.volume = 10;
		payload.duration = htons(cmediamanager->m_DTMFDuration);

		switch ( cmediamanager->m_DTMFState ) {
		case DTMF_LAST1:
			cmediamanager->m_DTMFState = DTMF_LAST2;
			payload.e = 1;
			break;
		case DTMF_LAST2:
			cmediamanager->m_DTMFState = DTMF_LAST3;
			payload.e = 1;
			break;
		case DTMF_LAST3:
			cmediamanager->m_DTMFState = DTMF_IDLE;
			payload.e = 1;
			break;
		default:
			payload.e = 0;
			break;
		}

		cclRTPWriteEx(RtpChan, &(cmediamanager->m_DTMFparam), (const char *)(&payload), 4);
	}
	
	return ( len < xlen )? len:xlen;
}

// static operation
int CMediaManager::RTP2Wav(char* buff, int len)
{
	short encoded_data[ILBCNOOFWORDS_MAX];
	
	unsigned char serial729[L_FRAME_COMPRESSED];
	unsigned char serial723[L_FRAME_MAX_G723_BIN];
	unsigned char serial711u[G711_ULAW_BYTES_PER_FRAME];
	unsigned char serial711a[G711_ALAW_BYTES_PER_FRAME];

	//static unsigned char bin1[L_FRAME];
	//static BOOL bBufferFull = FALSE;
	short WaveType = cmediamanager->m_WaveType;
	short ret;
	int smpno_put = 0, smpno_get = 0;
//	int i;

	MyCString tmpStr;

#ifdef UACOM_USE_G711
	char* pData;
	int retlen = len;
#endif

	switch (WaveType) {
	case WAV_G723:

#if !USE_MIDDLEWARE

		if (cmediamanager->bMyFlag) { // receive RTP Packet
			int type,length;
			cclGetRTPFromJB((char*)serial723,&type, &length);
			if( type==-1 ) {
					
					//putPacket(0, VPM_CODEC_G723_1A, NULL, FT_NO_TRANSMIT, 0);
					//channelControl();
			}else if(type==FT_VOICE) {
				
				memset(buff, 0, 480);
				g723_decoder((short*)buff, (char*)serial723);
				return 480;
				//smpno_put = putPacket(0, VPM_CODEC_G723_1A, serial723, FT_VOICE, 0);
				//channelControl();
			}else if(type==FT_SID) {
				//smpno_put = putPacket(0, VPM_CODEC_G723_1A, serial723, FT_SID, length);
				//channelControl();
			}else if(type==FT_NO_TRANSMIT){
				//putPacket(0, VPM_CODEC_G723_1A, NULL, FT_NO_TRANSMIT, 0);
				//channelControl();
			}
		} else {
			//putPacket(0, VPM_CODEC_G723_1A, NULL, FT_NO_TRANSMIT, 0);
			//channelControl();
		}

		//bufsize = dxBufferGetCurrLen(cmediamanager->m_BufRTP2IO);
		//ret = dxBufferRead(cmediamanager->m_BufRTP2IO,(char*)line,bufsize);	
		/* marked by ljchuang 2005/08/16
		ret = dxBufferRead(cmediamanager->m_BufRTP2IO,(char*)bin,24);
		if (ret <=0 )
			return ret;
		*/
		/* marked by ljchuang 2005/08/16
		memset(buff, 0, 480);
		g723_decoder((short*)buff, (char*)bin);
		//Audio_Decode((short*)buff, bin, 0);
		*/
		return 0;
		

#else //! use middle ware

#ifdef DEVELOP_JITTER
		if (cmediamanager->bMyFlag) { // receive RTP Packet
			int type,length;
			cclGetRTPFromJB((char*)serial723,&type, &length);
			if( type==-1 ) {
					//cmediamanager->bMyFlag=FALSE;
					putPacket(0, VPM_CODEC_G723_1A, NULL, FT_NO_TRANSMIT, 0);
					channelControl();
			}else if(type==FT_VOICE) {
				smpno_put = putPacket(0, VPM_CODEC_G723_1A, serial723, FT_VOICE, 0);
				channelControl();
			}else if(type==FT_SID) {
				smpno_put = putPacket(0, VPM_CODEC_G723_1A, serial723, FT_SID, length);
				channelControl();
			}else if(type==FT_NO_TRANSMIT){
				putPacket(0, VPM_CODEC_G723_1A, NULL, FT_NO_TRANSMIT, 0);
				channelControl();
			}
		} else {
			putPacket(0, VPM_CODEC_G723_1A, NULL, FT_NO_TRANSMIT, 0);
			channelControl();
		}
#else

		// sjhuang 2006/03/03, refine MiddleWare API.
		if (cmediamanager->bMyFlag) { // receive RTP Packet
			ret = dxBufferRead(cmediamanager->m_BufRTP2IO,(char*)serial723,L_FRAME_MAX_G723_BIN);
						
			if ( ret > 0 ) {

				unsigned char tmp[L_FRAME_MAX_G723_BIN]; // to get null data
				memset(tmp,0x99,L_FRAME_MAX_G723_BIN);
				if( memcmp(serial723,tmp,L_FRAME_MAX_G723_BIN)==0 )
				{

#ifdef DEBUG_AUDIO
					if(printOutStream)
					fprintf(printOutStream,"\n\n ==> suppose null data <== \n\n");
#endif					
					putPacket(0, VPM_CODEC_G723_1A, NULL, FT_NO_TRANSMIT, 0);
					channelControl();
				} 
				else 
				{
					smpno_put = putPacket(0, VPM_CODEC_G723_1A, serial723, FT_VOICE, 0);
					channelControl();	
				}
			} else {
				putPacket(0, VPM_CODEC_G723_1A, NULL, FT_NO_TRANSMIT, 0);
				channelControl();
			}
			
		} else {
			putPacket(0, VPM_CODEC_G723_1A, NULL, FT_NO_TRANSMIT, 0);
			channelControl();
		}

#endif
		//fprintf(printOutStream, "%d [RTP2WAV] \tputPacket:\t\tpacketSize = %d\n",GetTickCount(),smpno_put);
		
		smpno_get = getPCMFrame(0, G723_1A_SAMPLES_PER_FRAME, (short*)buff);

		//fprintf(printOutStream, "%d [RTP2WAV] \tgetPCMFrame:\t\trqstSamples = %d, gotSamples = %d\n",GetTickCount(), G723_1A_SAMPLES_PER_FRAME,smpno_get);


		return (smpno_get * sizeof(short));

#endif // use middle ware or not

	case WAV_G729:

#if !USE_MIDDLEWARE

		if (cmediamanager->bMyFlag) { // receive RTP Packet
			int type,length;
			cclGetRTPFromJB((char*)serial729,&type, &length);
			
			if( type==-1 ) {
				
			}else if(type==FT_VOICE) {
				
				memset(buff, 0, L_FRAME * sizeof(short));
				g729_decoder(serial729, (short*)buff);
				return (L_FRAME * sizeof(short));
				
			}else if(type==FT_SID) {

				
			}else if(type==FT_NO_TRANSMIT){
				
			}
		} else {
			
		}

		return 0;
	// Do decompression
		/*
		ret = dxBufferRead(cmediamanager->m_BufRTP2IO,(char*)serial,L_FRAME_COMPRESSED);

		if (ret <=0 )
			return ret;

		memset(buff, 0, L_FRAME * sizeof(short));
		g729_decoder(serial, (short*)buff);
		return (L_FRAME * sizeof(short));
		*/
		

#else //! use middle ware

#ifdef DEVELOP_JITTER
		if (cmediamanager->bMyFlag) { // receive RTP Packet
			int type,length;
			cclGetRTPFromJB((char*)serial729,&type, &length);
			
			if( type==-1 ) {


				CIniFile m_ConfigFile(cmediamanager->m_strAppPath+"config.ini");	
				CString str_slowStart;
				str_slowStart = m_ConfigFile.ReadString("SlowStart", "WaitPacket", "0");
				
				if( atoi(str_slowStart)>0 )
				{
					cmediamanager->bMyFlag = FALSE;
					rtpthreshold = atoi(str_slowStart) ;

#ifdef DEBUG_AUDIO2
					if(printOutStream)
						fprintf(printOutStream,"\n\n ==> Slow Start , threshold:%d , type==-1, no RTP data <== \n\n",rtpthreshold);
#endif	
				}
				else
				{

#ifdef DEBUG_AUDIO2
					if(printOutStream)
						fprintf(printOutStream,"\n\n ==> No Slow Start, type==-1, no RTP data <== \n\n");
#endif		
				}

				putPacket(0, VPM_CODEC_G729AB, NULL, FT_NO_TRANSMIT, 0);
				channelControl();
				
			}else if(type==FT_VOICE) {

				//DebugMsg2UI("VOICE Frame\n");

				smpno_put = putPacket(0, VPM_CODEC_G729AB, serial729, FT_VOICE, 0);
				channelControl();
			}else if(type==FT_SID) {

				DebugMsg2UI("SID Frame\n");

				smpno_put = putPacket(0, VPM_CODEC_G729AB, serial729, FT_SID, length);
				channelControl();
			}else if(type==FT_NO_TRANSMIT){
				putPacket(0, VPM_CODEC_G729AB, NULL, FT_NO_TRANSMIT, 0);
				channelControl();
			}
		} else {
			putPacket(0, VPM_CODEC_G729AB, NULL, FT_NO_TRANSMIT, 0);
			channelControl();
		}
#else

		if (cmediamanager->bMyFlag) { // receive RTP Packet
			ret = dxBufferRead(cmediamanager->m_BufRTP2IO,(char*)serial729,L_FRAME_COMPRESSED);


			
			if ( ret > 0 ) {

				unsigned char tmp[L_FRAME_COMPRESSED]; // to get null data
				memset(tmp,0x99,L_FRAME_COMPRESSED);
				if( memcmp(serial729,tmp,L_FRAME_COMPRESSED)==0 )
				{
#ifdef DEBUG_AUDIO2
					if(printOutStream)
					fprintf(printOutStream,"\n\n ==> suppose null data <== \n\n");
#endif					
					putPacket(0, VPM_CODEC_G729AB, NULL, FT_NO_TRANSMIT, 0);
					channelControl();
				} 
				else 
				{
					smpno_put = putPacket(0, VPM_CODEC_G729AB, serial729, FT_VOICE, 0);
					channelControl();

				}
			} else {

#ifdef DEBUG_AUDIO2
					if(printOutStream)
					fprintf(printOutStream,"\n\n ==> FT_NO_TRANSMIT, dxBufferRead no Data  <== \n\n");
#endif

					putPacket(0, VPM_CODEC_G729AB, NULL, FT_NO_TRANSMIT, 0);
					channelControl();
		
			}
			
		} else { // wait buffer up2 threshold value, then start play
#ifdef DEBUG_AUDIO2
					if(printOutStream)
					fprintf(printOutStream,"\n\n ==> wait buffer up2 threshold value, then start play <== \n\n");
#endif	

			putPacket(0, VPM_CODEC_G729AB, NULL, FT_NO_TRANSMIT, 0);
			channelControl();
		}

#endif


		smpno_get = getPCMFrame(0, G729AB_SAMPLES_PER_FRAME, (short*)buff);

		channelControl();

		//fprintf(printOutStream, "%d [RTP2WAV] \tgetPCMFrame:\t\trqstSamples = %d, gotSamples = %d\n",GetTickCount(), G729AB_SAMPLES_PER_FRAME,smpno_get);

		return (smpno_get * sizeof(short));

#endif // use middle ware or not

	case WAV_GSM:

		ret = dxBufferRead(cmediamanager->m_BufRTP2IO,buff,len);

		return ret;

	case WAV_iLBC:
		// Do decompression
		ret = dxBufferRead(cmediamanager->m_BufRTP2IO,(char*)encoded_data,38);

		if (ret <=0 )
			return ret;

		memset(buff, 0, 160 * sizeof(short));

		ret = iLBC_decode(&Dec_Inst, (short*)buff, encoded_data, 1);
		return (ret * sizeof(short));
		
	case WAV_PCMA:

#if !USE_MIDDLEWARE
	if (cmediamanager->bMyFlag) { // receive RTP Packet
			int type,length;
			cclGetRTPFromJB((char*)serial711a,&type, &length);
			if( type==-1 ) {
				
			}else if(type==FT_VOICE) {
				#ifdef UACOM_USE_G711
					g_G711Codec->Decode(serial711a, len/2, buff, &retlen);
				#endif
					
				return retlen;

			}else if(type==FT_SID) {
				
			}else if(type==FT_NO_TRANSMIT){
			
			}
		} else {
			
		}
		return 0;

#else //! use middle ware

#ifdef DEVELOP_JITTER
		if (cmediamanager->bMyFlag) { // receive RTP Packet
			int type,length;
			cclGetRTPFromJB((char*)serial711a,&type, &length);
			if( type==-1 ) {
					//cmediamanager->bMyFlag=FALSE;
					putPacket(0, VPM_CODEC_G711_ALAW, NULL, FT_NO_TRANSMIT, 0);
					channelControl();
			}else if(type==FT_VOICE) {
				smpno_put = putPacket(0, VPM_CODEC_G711_ALAW, serial711a, FT_VOICE, L_FRAME);
				channelControl();	
				//smpno_put = putPacket(0, VPM_CODEC_G711_ALAW, serial711a, FT_VOICE, 0);
				//channelControl();
			}else if(type==FT_SID) {
				smpno_put = putPacket(0, VPM_CODEC_G711_ALAW, serial711a, FT_SID, length);
				channelControl();
			}else if(type==FT_NO_TRANSMIT){
				putPacket(0, VPM_CODEC_G711_ALAW, NULL, FT_NO_TRANSMIT, 0);
				channelControl();
			}
		} else {
			putPacket(0, VPM_CODEC_G711_ALAW, NULL, FT_NO_TRANSMIT, 0);
			channelControl();
		}
#else

		// sjhuang 2006/03/03, refine MiddleWare API.
		if (cmediamanager->bMyFlag) { // receive RTP Packet
			ret = dxBufferRead(cmediamanager->m_BufRTP2IO,(char*)serial711a,G711_ALAW_SAMPLES_PER_FRAME);
						
			if ( ret > 0 ) {

				unsigned char tmp[G711_ALAW_SAMPLES_PER_FRAME]; // to get null data
				memset(tmp,0x99,G711_ALAW_SAMPLES_PER_FRAME);
				if( memcmp(serial711a,tmp,G711_ALAW_SAMPLES_PER_FRAME)==0 )
				{
#ifdef DEBUG_AUDIO
					if(printOutStream)
					fprintf(printOutStream,"\n\n ==> suppose null data <== \n\n");
#endif					
					putPacket(0, VPM_CODEC_G711_ALAW, NULL, FT_NO_TRANSMIT, 0);
					channelControl();
				} 
				else 
				{
					smpno_put = putPacket(0, VPM_CODEC_G711_ALAW, serial711a, FT_VOICE, L_FRAME);
					channelControl();	
				}
			} else {
				putPacket(0, VPM_CODEC_G711_ALAW, NULL, FT_NO_TRANSMIT, 0);
				channelControl();
			}
			
		} else {
			putPacket(0, VPM_CODEC_G711_ALAW, NULL, FT_NO_TRANSMIT, 0);
			channelControl();
		}

#endif

		smpno_get = getPCMFrame(0, G711_ALAW_SAMPLES_PER_FRAME, (short*)buff);

		//fprintf(printOutStream, "%d [RTP2WAV] \tgetPCMFrame:\t\trqstSamples = %d, gotSamples = %d\n",GetTickCount(), G711_ULAW_SAMPLES_PER_FRAME,smpno_get);

		return (smpno_get * sizeof(short));

#endif // use middle ware or not		

	case WAV_PCMU:
	default:

#if !USE_MIDDLEWARE
	
		if (cmediamanager->bMyFlag) { // receive RTP Packet
			int type,length;
			cclGetRTPFromJB((char*)serial711u,&type,&length);
			if( type==-1 ) {
					
			}else if(type==FT_VOICE) {

				#ifdef UACOM_USE_G711
					g_G711Codec->Decode(serial711u, len/2, buff, &retlen);
				#endif

				return retlen;

			}else if(type==FT_SID) {
				
			}else if(type==FT_NO_TRANSMIT){
				
			}
		} else {
			
		}
		
		return 0;
	/*
	#ifdef UACOM_USE_G711
		pData = (char*)malloc(sizeof(char) * len / 2);
		ret = dxBufferRead(cmediamanager->m_BufRTP2IO,pData,len/2);
	#else
		ret = dxBufferRead(cmediamanager->m_BufRTP2IO,buff,len);
	#endif

		if (ret <=0 )
			return ret;

	#ifdef UACOM_USE_G711
		g_G711Codec->Decode(pData, len/2, buff, &retlen);
		if (pData != NULL)
			free(pData);
	#endif
		return ret;
	*/

#else // use middle ware

#ifdef DEVELOP_JITTER
		if (cmediamanager->bMyFlag) { // receive RTP Packet
			int type,length;
			cclGetRTPFromJB((char*)serial711u,&type,&length);
			if( type==-1 ) {
					//cmediamanager->bMyFlag=FALSE;
					putPacket(0, VPM_CODEC_G711_ULAW, NULL, FT_NO_TRANSMIT, 0);
					channelControl();
			}else if(type==FT_VOICE) {
				smpno_put = putPacket(0, VPM_CODEC_G711_ULAW, serial711u, FT_VOICE, L_FRAME);
				channelControl();	
				//smpno_put = putPacket(0, VPM_CODEC_G711_ULAW, serial711u, FT_VOICE, 0);
				//channelControl();
			}else if(type==FT_SID) {
				smpno_put = putPacket(0, VPM_CODEC_G711_ULAW, serial711u, FT_SID, length);
				channelControl();
			}else if(type==FT_NO_TRANSMIT){
				putPacket(0, VPM_CODEC_G711_ULAW, NULL, FT_NO_TRANSMIT, 0);
				channelControl();
			}
		} else {
			putPacket(0, VPM_CODEC_G711_ULAW, NULL, FT_NO_TRANSMIT, 0);
			channelControl();
		}
#else

		// sjhuang 2006/03/03, refine MiddleWare API.
		if (cmediamanager->bMyFlag) { // receive RTP Packet
			ret = dxBufferRead(cmediamanager->m_BufRTP2IO,(char*)serial711u,G711_ULAW_SAMPLES_PER_FRAME);
						
			if ( ret > 0 ) {

				unsigned char tmp[G711_ULAW_SAMPLES_PER_FRAME]; // to get null data
				memset(tmp,0x99,G711_ULAW_SAMPLES_PER_FRAME);
				if( memcmp(serial711u,tmp,G711_ULAW_SAMPLES_PER_FRAME)==0 )
				{
#ifdef DEBUG_AUDIO2
					if(printOutStream)
					fprintf(printOutStream,"\n\n ==> suppose null data <== \n\n");
#endif					
					putPacket(0, VPM_CODEC_G711_ULAW, NULL, FT_NO_TRANSMIT, 0);
					channelControl();
				} 
				else 
				{
					smpno_put = putPacket(0, VPM_CODEC_G711_ULAW, serial711u, FT_VOICE, L_FRAME);
					channelControl();	
				}
			} else {
				putPacket(0, VPM_CODEC_G711_ULAW, NULL, FT_NO_TRANSMIT, 0);
				channelControl();
			}
			
		} else {
			putPacket(0, VPM_CODEC_G711_ULAW, NULL, FT_NO_TRANSMIT, 0);
			channelControl();
		}

#endif

		smpno_get = getPCMFrame(0,  G711_ULAW_SAMPLES_PER_FRAME, (short*)buff);

		//fprintf(printOutStream, "%d [RTP2WAV] \tgetPCMFrame:\t\trqstSamples = %d, gotSamples = %d\n",GetTickCount(), G711_ULAW_SAMPLES_PER_FRAME,smpno_get);

		channelControl();
		return (smpno_get * sizeof(short));

#endif // use middle ware or not
		
	}
}

// static operation
int CMediaManager::RTPEventHandler(int channel,const char* buffer,int buflen)
{
	return dxBufferWrite(cmediamanager->m_BufRTP2IO,((char *) buffer),buflen);
}

// static operation
int CMediaManager::RTPEventHandlerPkt(int channel,rtp_pkt_t* pkt)
{
	return dxBufferWrite(cmediamanager->m_BufRTP2IO, pkt->buf, pkt->buflen);

}

int CMediaManager::RTPSequence(int channel,rtp_hdr_t* param, const char* buffer,int buflen,int debug)
{

	rtpframe tmpframe = NULL;
	unsigned int ts;
	unsigned int samples;
	int retval;
	int type;
	int frameNumber=0;
	int bytePerSample=0;

	if (!cmediamanager->m_IsPlayingRTP)
	{
		if(printOutStream)
			fprintf(printOutStream,"\n\n ==>Recieve RTP Packet, but not start playing RTP <== \n\n");
		return 0;
	}
	
	if ( param->pt == 19)
	{
		time( &cmediamanager->m_SilenceTimestamp);
		cmediamanager->m_DataTimestamp = 0;
		return 0;
	}

	ts = ntohl(param->ts);

	switch ( param->pt ) {
		case 0: // 711U
			samples = buflen; // 80 samples = 80bytes;
			frameNumber = buflen/80;
			bytePerSample=80;
			break;
		case 3: // GSM
			samples = buflen; // 80 samples = 80bytes;
			frameNumber = buflen/80;
			bytePerSample=80;
			break;
		case 4: // G723
			samples = buflen * 10; // 240samples = 24bytes;
			frameNumber = buflen*10/240;
			bytePerSample=24;
			break;
		case 8: // 711A
			samples = buflen; // 80 samples = 80bytes;
			frameNumber = buflen/80;
			bytePerSample=80;
			break;
		case 18: // G729
			samples = buflen * 8; // 80 samples = 10bytes;
			frameNumber = buflen*8/80;
			bytePerSample=10;
			break;
	}

	// check Jitter Buffer length, if too bigger, then drop it
	CUAProfile *pProfile = CUAProfile::GetProfile();
	if( cclGetJBLength() > cmediamanager->m_maxJB ) {
		cclRemoveJB( cmediamanager->m_dropJB );
		//cmediamanager->bMyFlag=FALSE;
		if( printOutStream )
			fprintf(printOutStream,"\n\n cclGetJBLength() too big \n\n");
		return 0;
	}

	// put RTP to property position, attention to check if comfort noise
	// insert never before first item.
	// ToDo: check RTP packet data if complete !! sjhuang 2006/02/23
	// Shall be support Comfort Noise RFC 3389
	type = FT_VOICE;
	if( buflen <= 0 ) return 0;
	if( buflen%10!=0 && buflen%24!=0) 
	{
		char debugMsg[255];
		sprintf(debugMsg,"Get SID Frame, buflen:%d \n",buflen);
		DebugMsg2UI(debugMsg);
		type = FT_SID;
	}

	retval = cclPutRTP2JB( (char*)buffer, buflen, type, (unsigned short)(ntohl(param->seq)>>16)&0x0000ffff, ( (type==FT_SID)?1:frameNumber ), ntohl(param->ts), samples, debug );

	// check if Jitter Buffer > threshold, if already playing, ignore it, otherwise, start play
	// auto Insert no_transfer type before first item, if has packet lost !!
	if( cclGetJBLength() >= rtpthreshold && !cmediamanager->bMyFlag )
	{
		if( printOutStream )
			fprintf(printOutStream,"\n\n up2 threshold, start playing \n\n");

		cmediamanager->bMyFlag = TRUE;
	}

	// sam add: calc data receive rate in bytes per seconds
	cmediamanager->_IncStatBuf( buflen);
	time( &cmediamanager->m_DataTimestamp);

	return retval;
}
// static operation
int CMediaManager::RTPEventHandlerEx(int channel,rtp_hdr_t* param, const char* buffer,int buflen)
{

	int samples;
	
	// detect packet lost
	static unsigned int first_packet=0;
	static unsigned int last_packet=0;
	static unsigned int should_packet=0;
	static int total_packet=0,lost_packet=0;
	static DWORD lastts=0,curts=0;
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

		dxBufferWrite(cmediamanager->m_BufRTP2IO,(char*)buffer,buflen);
		if( dxBufferGetCurrLen(cmediamanager->m_BufRTP2IO)>rtpthreshold && !cmediamanager->bMyFlag)
		{
			cmediamanager->bMyFlag = TRUE;
		}
		// sam add: calc data receive rate in bytes per seconds
		cmediamanager->_IncStatBuf( buflen);
		time( &cmediamanager->m_DataTimestamp);
	}
#endif // end of DEVELOP_JITTER

}

int CMediaManager::Media_RTP_Stop()
{
	MyCString tmpStr;

#ifdef DEBUG_AUDIO2
	if( printOutStream ) fclose(printOutStream);	
#endif

	cclRTPClose(m_RtpChan);
	m_WaveType=-1;
	dxBufferClear(m_BufRTP2IO);
	dxBufferClear(m_BufIO2RTP);

	RTPLen = 0;
	memset((void*)RTPBuffer, 0, 4096);

	tmpStr.Format(_T("[MediaManager] Total RTP packet = %d"), TotalPacket);
	DebugMsg(tmpStr);
	TotalPacket = 0;
	m_IsPlayingRTP = FALSE;

	
#ifdef UACOM_WAVETHREAD // sjhuang

	StopWaveThread();
	waveOut.StopThread();
	waveIn.StopThread();
	

#ifdef SAVE_STREAMING
	// for detect streaming
	CIniFile m_ConfigFile(m_strAppPath+"config.ini");	
	if( atoi(m_ConfigFile.ReadString("DEBUG", "SAVE_STREAMING", "0")) )
	{
		CWave monWave = waveIn.MakeWave();
		monWave.Save("zz_pca_record.wav");
		waveIn.ClearBuffer();

		monWave = waveOut.MakeWave();
		monWave.Save("zz_pca_play.wav");
		waveOut.ClearBuffer();
	}
#endif

#else // use waveInOut

	m_WaveIOObj->stopPlaying();
	m_WaveIOObj->stopRecording();
	

	if (m_WaveIOObj->isWavInOpen)
		m_WaveIOObj->wavInClose();

	if (m_WaveIOObj->isWavOutOpen)
		m_WaveIOObj->wavOutClose();
#endif

#ifdef DEVELOP_JITTER
	cclClearJB();
#endif
	
	
	tmpStr.Format(_T("[MediaManager] Media_RTP_Stop()"));
	DebugMsg(tmpStr);


	for (int i = 0; i < 24; i++)
		blankframe[i] = -1;

	totalblank = 0;
	LastPutTimestamp = 0;
	
	return 0;
}

RCODE CMediaManager::SetMediaParameter(int mediatype)
{
	CUAProfile* pProfile = CUAProfile::GetProfile();
	MyCString tmpStr;

	CIniFile m_ConfigFile(m_strAppPath+"config.ini");	
	CString threshold,str_maxJB,str_dropJB;

	switch (mediatype) {
	case CODEC_G723:
		m_WaveType = WAV_G723;
		m_BitsPerSample = 16;
		m_CodecPacketizePeriod = 30;
		m_RTPPacketizePeriod = pProfile->m_RTPFrames * m_CodecPacketizePeriod;
		m_RTPPacketsize = pProfile->m_RTPFrames * 24; // 30ms: 24bytes

		if (m_BufRTP2IO)
			dxBufferFree(m_BufRTP2IO);
		m_BufRTP2IO = dxBufferNew(480*pProfile->m_RTPFrames); // 24 * 24 = 576

		//rtpthreshold = 384;
#ifndef DEVELOP_JITTER
		rtpthreshold = 24*4; // sjhuang, about 120 ms
#else		
		threshold = m_ConfigFile.ReadString("Threshold", "G723", "3");
		rtpthreshold=atoi(threshold);

		str_maxJB = m_ConfigFile.ReadString("MaxBuffer", "G723", "64");
		m_maxJB = atoi(str_maxJB);

		str_dropJB = m_ConfigFile.ReadString("DropBuffer", "G723", "32");
		m_dropJB = atoi(str_dropJB);
#endif
		m_cmdevent = event_g723;
		break;

	case CODEC_G729:
		m_WaveType = WAV_G729;
		m_BitsPerSample = 16;
		m_CodecPacketizePeriod = 10;
		m_RTPPacketizePeriod = pProfile->m_RTPFrames * m_CodecPacketizePeriod;
		m_RTPPacketsize = pProfile->m_RTPFrames * 10; // 10ms: 10bytes
		
		if (m_BufRTP2IO)
			dxBufferFree(m_BufRTP2IO);
		m_BufRTP2IO = dxBufferNew(240*(pProfile->m_RTPFrames+1)); // 10 * 24 = 240
		//rtpthreshold = 200;
#ifndef DEVELOP_JITTER
		rtpthreshold = 10*12; // sjhuang, about 120 ms
#else		
		threshold = m_ConfigFile.ReadString("Threshold", "G729", "8");
		rtpthreshold=atoi(threshold);

		str_maxJB = m_ConfigFile.ReadString("MaxBuffer", "G729", "128");
		m_maxJB = atoi(str_maxJB);

		str_dropJB = m_ConfigFile.ReadString("DropBuffer", "G729", "64");
		m_dropJB = atoi(str_dropJB);
#endif
		m_cmdevent = event_g729;
		break;
	
	case CODEC_GSM:
		m_WaveType = WAV_GSM;
		m_BitsPerSample = 16; // Ignored, set by WaveIO
		m_CodecPacketizePeriod = 40; // Ignored, set by WaveIO

		if (pProfile->m_RTPFrames > 4)
			pProfile->m_RTPFrames = 4;

		m_RTPPacketizePeriod = pProfile->m_RTPFrames * m_CodecPacketizePeriod;
		m_RTPPacketsize = pProfile->m_RTPFrames * 65; // 40ms: 65bytes
	
		if (m_BufRTP2IO)
			dxBufferFree(m_BufRTP2IO);
		m_BufRTP2IO = dxBufferNew(2000);
		break;

	case CODEC_iLBC:
		m_WaveType = WAV_iLBC;
		m_BitsPerSample = 16;
		m_CodecPacketizePeriod = 20;
		m_RTPPacketizePeriod = pProfile->m_RTPFrames * m_CodecPacketizePeriod;
		m_RTPPacketsize = pProfile->m_RTPFrames * 38; // 20ms: 38bytes
	
		if (m_BufRTP2IO)
			dxBufferFree(m_BufRTP2IO);
		m_BufRTP2IO = dxBufferNew(2000);
		break;

	case CODEC_PCMU:
		m_WaveType = WAV_PCMU;
		m_BitsPerSample = 16;
		//m_CodecPacketizePeriod = pProfile->m_G711PacketizePeriod;
		m_CodecPacketizePeriod = 10;
		m_RTPPacketizePeriod = pProfile->m_RTPFrames * m_CodecPacketizePeriod;
		m_RTPPacketsize = pProfile->m_RTPFrames * 80; // 10ms: 80bytes
	
		if (m_BufRTP2IO)
			dxBufferFree(m_BufRTP2IO);
		m_BufRTP2IO = dxBufferNew(1920*(pProfile->m_RTPFrames+1)); // 80 * 24 = 1920
		//rtpthreshold = 1600;
#ifndef DEVELOP_JITTER
		rtpthreshold = 80*12; // sjhuang, about 120 ms
#else	
		threshold = m_ConfigFile.ReadString("Threshold", "PCMU", "8");
		rtpthreshold=atoi(threshold);

		str_maxJB = m_ConfigFile.ReadString("MaxBuffer", "PCMU", "128");
		m_maxJB = atoi(str_maxJB);

		str_dropJB = m_ConfigFile.ReadString("DropBuffer", "PCMU", "64");
		m_dropJB = atoi(str_dropJB);
#endif
		m_cmdevent = event_g711u;

#ifdef UACOM_USE_G711
		m_G711Codec->SetULaw();
#endif
		break;
	
	case CODEC_PCMA:
		m_WaveType = WAV_PCMA;
		m_BitsPerSample = 16;
		//m_CodecPacketizePeriod = pProfile->m_G711PacketizePeriod;
		m_CodecPacketizePeriod = 10;
		m_RTPPacketizePeriod = pProfile->m_RTPFrames * m_CodecPacketizePeriod;
		m_RTPPacketsize = pProfile->m_RTPFrames * 80; // 10ms: 80bytes
		
		if (m_BufRTP2IO)
			dxBufferFree(m_BufRTP2IO);
		m_BufRTP2IO = dxBufferNew(1920*(pProfile->m_RTPFrames+1)); // 80 * 24 = 1920
		//rtpthreshold = 1600;
#ifndef DEVELOP_JITTER
		rtpthreshold = 80*12; // about 120 ms
#else	
		threshold = m_ConfigFile.ReadString("Threshold", "PCMA", "8");
		rtpthreshold=atoi(threshold);

		str_maxJB = m_ConfigFile.ReadString("MaxBuffer", "PCMA", "128");
		m_maxJB = atoi(str_maxJB);

		str_dropJB = m_ConfigFile.ReadString("DropBuffer", "PCMA", "64");
		m_dropJB = atoi(str_dropJB);
#endif
		m_cmdevent = event_g711a;

#ifdef UACOM_USE_G711
		m_G711Codec->SetALaw();
#endif
		break;
		
	default:
		// 8(BitsPerSample) * 8000(SamplesPerSec) / 1000 * 20(PacketizePeriod) / 8 = 160
		m_RTPPacketizePeriod = pProfile->m_RTPPacketizePeriod;
		m_CodecPacketizePeriod = pProfile->m_G711PacketizePeriod;
		m_RTPPacketsize = (unsigned int)floor(m_RTPPacketizePeriod / m_CodecPacketizePeriod) * (m_CodecPacketizePeriod * 8);
		m_BitsPerSample = 8;
		if (m_BufRTP2IO)
			dxBufferFree(m_BufRTP2IO);
		m_BufRTP2IO = dxBufferNew(2000);
		break;
	}
	tmpStr.Format(_T("[MediaManager] RTP packet size = %d"), m_RTPPacketsize);
	DebugMsg(tmpStr);
	return RC_OK;
}

RCODE CMediaManager::RTPOpenPort(WAV_FORMAT wavetype)
{
	CUAProfile* pProfile = CUAProfile::GetProfile();

	//int nTos = AfxGetApp()->GetProfileInt( SESSION_LOCAL, KEY_RTP_AUDIO_TOS, 0);
	int nTos = 0;
	return cclRTPOpen(m_RtpChan, pProfile->m_LocalAddr, m_RTPPacketsize, m_RTPPacketizePeriod, wavetype, PF_UNSPEC, nTos);
	//return cclRTPOpen(m_RtpChan, m_RTPPacketsize, m_RTPPacketizePeriod, wavetype, AF_INET, nTos);
}

RCODE	CMediaManager::RTPClosePort()
{
	return cclRTPClose(m_RtpChan);
}

int CMediaManager::RTPPeerConnect(MyCString ip,int port)
{
	MyCString buf;

	/* Open RTP channel */
	if (RTPOpenPort((WAV_FORMAT)m_WaveType) < 0)
		DebugMsg("[MediaManager] Open local RTP port fail!");	
	else
		DebugMsg("[MediaManager] Open local RTP port successful.");	

	buf.Format(_T("[MediaManager] Open peer RTP: %s:%d\n"),(LPCSTR)ip,port);
	DebugMsg(buf);

	/* Connect peer RTP port */
	cclRTPAddReceiver(m_RtpChan,ip,port, PF_UNSPEC);
	buf.Format(_T("[MediaManager] Local RTP port (%d)\n"),cclRTPGetPort(m_RtpChan));
	DebugMsg(buf);

	// sam add to calculate voice quality
	_ClearStatBuf();

	//cclRTPSetEventHandlerEx(RTPEventHandlerEx);

	PlayRTPWav((WAV_FORMAT)m_WaveType);
	m_IsPlayingRTP = TRUE;

	return 0;
}

unsigned int CMediaManager::RTPGetPort()
{
	return cclRTPGetPort(m_RtpChan);
}

BOOL CMediaManager::StopPlaySound()
{
	//m_WaveIOObj->stopPlaySound();
	(sndPlaySound(NULL,0))?0:-1;
	return TRUE;
}

RCODE CMediaManager::PlayRTPWav(WAV_FORMAT wavformat)
{
	MyCString tmpStr;

	CIniFile m_ConfigFile(m_strAppPath+"config.ini");	
	CString str_Debug,str_maxJB;
	int dwDebug;

	str_Debug = m_ConfigFile.ReadString("DEBUG", "DEBUG_AUDIO2", "0");
	dwDebug=atoi(str_Debug);

#ifdef DEBUG_AUDIO2	
	if( dwDebug )
		printOutStream = fopen("c:\\CMediaManager.log","w");
	else
		printOutStream=NULL;

	if(printOutStream)
		fprintf(printOutStream, "Start Call (wave_format=%d) at %d\n", wavformat,GetTickCount());
	
	DebugMsg2UI("Start Log:");

	start_play=FALSE;
#endif			


#ifdef UACOM_USE_DIRECTX
	
	DxStartRecording();
	DxStartPlaying();

#else

	int event_count = 0;
	CUAProfile* pProfile = CUAProfile::GetProfile();

#if USE_MIDDLEWARE

	t_CMB   RqstCMB,returnMsg = {0};
	
	if (wavformat != WAV_GSM) {
		while (m_cmdevent[event_count].timeStamp >= 0) {
			RqstCMB = m_cmdevent[event_count++].cmb;
			putCMB(RqstCMB);
			//fprintf(printOutStream, "API->VPM Messages:\tcmdID=%d, S/N=%d, CH=%d, Param0=%d, Param1=%d\n", RqstCMB.commandID, RqstCMB.serialNumber, RqstCMB.channelID, RqstCMB.param[0], RqstCMB.param[1]);
		}

		if ( pProfile->m_bEnableAEC ) {
			RqstCMB.channelID = 0;
			RqstCMB.serialNumber = 40;
			RqstCMB.commandID = CMD_AEC_START;
			RqstCMB.param[0] = (short)pProfile->m_TailLength;
			RqstCMB.param[1] = (short)pProfile->m_FarEndEchoSignalLag;
			putCMB(RqstCMB);
			//fprintf(printOutStream, "API->VPM Messages:\tcmdID=%d, S/N=%d, CH=%d, Param0=%d, Param1=%d\n", RqstCMB.commandID, RqstCMB.serialNumber, RqstCMB.channelID, RqstCMB.param[0], RqstCMB.param[1]);
		}

		channelControl();

		while (getCMB(&returnMsg)) {
			//fprintf(printOutStream, "VPM->API Messages:\tcmdID=%d, S/N=%d, CH=%d, Param0=%d, Param1=%d\n", returnMsg.commandID, returnMsg.serialNumber, returnMsg.channelID, returnMsg.param[0], returnMsg.param[1]);
		}

		channelControl();
	}
#endif

	bPacketReceived = FALSE;
	bRetry = FALSE;
	bMyFlag = FALSE;

#ifdef UACOM_WAVETHREAD
	
	BOOL bSave=FALSE;

#ifdef SAVE_STREAMING

	CString str_saveStream;
	int dwSaveStream;
	str_saveStream = m_ConfigFile.ReadString("DEBUG", "SAVE_STREAMING", "0");
	dwSaveStream=atoi(str_saveStream);

	if( dwSaveStream )
		bSave = TRUE;
	else
		bSave = FALSE;
#endif

	CString str_inBuffer,str_outBuffer;
	CString str_inCount,str_outCount;

	int i_inBuf,i_outBuf;
	int i_inCount,i_outCount;

	str_inBuffer = m_ConfigFile.ReadString("WaveInOutBuffer", "WaveInSingleBuffer", "2");
	str_outBuffer = m_ConfigFile.ReadString("WaveInOutBuffer", "WaveOutSingleBuffer", "4");
	str_inCount = m_ConfigFile.ReadString("WaveInOutBuffer", "WaveInSingleBufferCount", "6");
	str_outCount = m_ConfigFile.ReadString("WaveInOutBuffer", "WaveOutSingleBufferCount", "6");
	
	i_inBuf=atoi(str_inBuffer);
	i_outBuf=atoi(str_outBuffer);
	i_inCount = atoi(str_inCount);
	i_outCount = atoi(str_outCount);

	int framesize = BYTESPERSAMPLE * cmediamanager->m_CodecPacketizePeriod * 8;
	
	// Works with buffer sizes for 20ms sound of more (iPaq3850/PPC2002; seems like a timeslice of 10ms is used
	// in the sound driver)
	waveIn.Init(waveInEventCB,
               THREAD_PRIORITY_NORMAL, // THREAD_PRIORITY_TIME_CRITICAL, 
               16,//i_inCount, // wBufferCount
               640,//framesize*i_inBuf, // dwSingleBufferSize : BYTESPERSAMPLE * m_CodecPacketizePeriod * 8,
               1,   // wChannels
               8000,// dwSamplesPerSec
               16,bSave); // wBitsPerSample

	waveOut.Init(waveOutEventCB,
                THREAD_PRIORITY_NORMAL, // THREAD_PRIORITY_TIME_CRITICAL, 
                16,//i_outCount, // wBufferCount
                640,//framesize*i_outBuf/*framesize*/, // dwSingleBufferSize : BYTESPERSAMPLE * m_CodecPacketizePeriod * 8,
                1,   // wChannels
                8000,// dwSamplesPerSec
                16,bSave); // wBitsPerSample

#ifdef SAVE_STREAMING
	// for save record/play files..
	if( bSave )
	{
		waveIn.m_wave.BuildFormat(1,8000,16);
		waveOut.m_wave.BuildFormat(1,8000,16);
	}
#endif

	if( !waveIn.StartThread() ) AfxMessageBox("record error!");
	if( !waveOut.StartThread() ) AfxMessageBox("play error!");

	
	waveIn.ResetBuffer();
	waveOut.ResetBuffer();

	ResetEvent(m_hEventKill); //sjhuang
	
	DWORD dwThreadId;
	m_hDSPThread = CreateThread( NULL, 0, DSPThreadFunc, this, CREATE_SUSPENDED, &dwThreadId);
	ResumeThread( m_hDSPThread );

	//cmediamanager->bMyFlag = TRUE; // added by sjhuang

#else

	/* Open WaveIO input stream and start recording */
	if(m_WaveIOObj->wavInOpen(wavformat,m_BitsPerSample,8000,1,m_CodecPacketizePeriod)!=0){
		DebugMsg("[MediaManager] Open WaveIn Error!\n");
		return RC_ERROR;
	} else if ( m_WaveIOObj->startRecording()!=0 ){
		DebugMsg("[MediaManager] Record Error!\n");
		return RC_ERROR;
	}

	/* Open WaveIO output stream and start playing */
	if(m_WaveIOObj->wavOutOpen(wavformat,m_BitsPerSample,8000,1,m_CodecPacketizePeriod)!=0){
		DebugMsg("[MediaManager] Open WaveOut Error!\n");

		if (wavformat == WAV_GSM)
			m_WaveIOObj->startPlaying();

		return RC_ERROR;
	}
	/*
	else if ( m_WaveIOObj->startPlaying()!=0 ){
		DebugMsg("[MediaManager] Playing Error!\n");
		return RC_ERROR;
	}
	*/
	
	if (wavformat == WAV_GSM)
		m_WaveIOObj->startPlaying();

#endif //UACOM_WAVETHREAD

#endif //UACOM_USE_DIRECTX

	return RC_OK;
}

void CMediaManager::init()
{
	MyCString tmpStr;
	
	if (m_bInitialized)
		return;
	
	int ret = cclRTPStartup(2);

	if (RC_OK == ret) {
		DebugMsg("[MediaManager] RTP initialize successful.");
	} else {
		DebugMsg("[MediaManager] RTP initialize fail!");
		tmpStr.Format(_T("error code = %d"), ret);
		DebugMsg(tmpStr);
	}

	cclRTPSetEventHandlerEx(RTPEventHandlerEx);
	//cclRTPSetEventHandlerPkt(RTPEventHandlerPkt);

	m_BufRTP2IO = dxBufferNew(2000);
	//m_BufIO2RTP = dxBufferNew(4096);
	//m_HashRTP2IO = dxHashNew(10, FALSE, DX_HASH_POINTER);

	m_WaveIOObj = WavInOut::instance();
	
	/* Associate WAVE stream with RTP stream */
	m_WaveIOObj->init(Wav2RTP,RTP2Wav);

#if !USE_MIDDLEWARE	
	g729_init_encoder();
	g729_init_decoder();

	g723_initial_encoder();
	g723_initial_decoder();
	

	//Audio_EncodeInit(AUDIO_G723);
	//Audio_DecodeInit(AUDIO_G723);

	iLBC_initEncoder(&Enc_Inst, 20);
	iLBC_initDecoder(&Dec_Inst, 20, 1);
#endif

	// marked by sjhuang, 2006/11/14, when change network interface, it will cause
	// rtp packet can't receive success.
	RTPOpenPort(WAV_PCMU);

#ifndef _WIN32_WCE
	ReadTonePCMRes();
#endif

#ifdef UACOM_USE_DIRECTX
	if (FAILED(DxInOutInit( CUAControl::GetControl()->m_hWnd ))) {
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
	
	DxInOutInit( CUAControl::GetControl()->m_hWnd );

	m_bInitialized = TRUE;
}

RCODE CMediaManager::SendDTMFTone(int type)
{

	m_CriticalSection.Lock();
	m_ToneType = type;
	m_idx = 0;
	//m_DTMFEvent = type;
	//m_DTMFState = DTMF_PREPARE;
	m_CriticalSection.Unlock();
	
	return RC_OK;
}

RCODE CMediaManager::EndDTMFTone()
{
	return RC_OK;
}

#ifndef _WIN32_WCE
void CMediaManager::ReadToneFiles()
{
	CString dirbuf;
	int i,len;

	char dir[64];
	GetCurrentDirectory(64,dir);
	m_CurDirectory.Empty();
	m_CurDirectory.Format("%s",dir);
	
	const char* ToneNameA[]={"\\atone0.pcm","\\atone1.pcm","\\atone2.pcm","\\atone3.pcm",
				 "\\atone4.pcm","\\atone5.pcm","\\atone6.pcm","\\atone7.pcm",
				 "\\atone8.pcm","\\atone9.pcm","\\atoneS.pcm","\\atoneP.pcm",};

	const char* ToneNameU[]={"\\utone0.pcm","\\utone1.pcm","\\utone2.pcm","\\utone3.pcm",
				 "\\utone4.pcm","\\utone5.pcm","\\utone6.pcm","\\utone7.pcm",
				 "\\utone8.pcm","\\utone9.pcm","\\utoneS.pcm","\\utoneP.pcm",};
	
	for (i=0;i<12;i++){
		dirbuf = m_CurDirectory + "\\Sound" + ToneNameA[i];
		CFile f(dirbuf,CFile::modeRead);
		len=f.GetLength();
		(len>2048)? (len=2048):(len=len);
		f.Read(ToneA[i].buf,len);
		ToneA[i].buflen=len;
		f.Close();
	}
	for (i=0;i<12;i++){
		dirbuf = m_CurDirectory + "\\Sound" + ToneNameU[i];
		CFile f(dirbuf,CFile::modeRead);
		len=f.GetLength();
		(len>2048)? (len=2048):(len=len);
		f.Read(ToneU[i].buf,len);
		ToneU[i].buflen=len;
		f.Close();
	}

}

void CMediaManager::ReadTonePCMRes()
{
    LPSTR lpRes; 
    HGLOBAL hRes;
	HRSRC hResInfo;
	int i;
	size_t len;

	const UINT AToneRes[] = {
		IDR_ATONE0, IDR_ATONE1, IDR_ATONE2,
		IDR_ATONE3,	IDR_ATONE4,	IDR_ATONE5,
		IDR_ATONE6,	IDR_ATONE7,	IDR_ATONE8,
		IDR_ATONE9,	IDR_ATONES,	IDR_ATONEP
	};

	const UINT UToneRes[] = {
		IDR_UTONE0,	IDR_UTONE1,	IDR_UTONE2,
		IDR_UTONE3,	IDR_UTONE4,	IDR_UTONE5,
		IDR_UTONE6,	IDR_UTONE7,	IDR_UTONE8,
		IDR_UTONE9,	IDR_UTONES,	IDR_UTONEP
	};

	const UINT G729ToneRes[] = {
		IDR_G729TONE0,	IDR_G729TONE1,	IDR_G729TONE2,
		IDR_G729TONE3,	IDR_G729TONE4,	IDR_G729TONE5,
		IDR_G729TONE6,	IDR_G729TONE7,	IDR_G729TONE8,
		IDR_G729TONE9,	IDR_G729TONES,	IDR_G729TONEP
	};
 
	// add by tyhuang 2005/12/2
	const UINT G723ToneRes[] = {
		IDR_G723TONE0,	IDR_G723TONE1,	IDR_G723TONE2,
		IDR_G723TONE3,	IDR_G723TONE4,	IDR_G723TONE5,
		IDR_G723TONE6,	IDR_G723TONE7,	IDR_G723TONE8,
		IDR_G723TONE9,	IDR_G723TONES,	IDR_G723TONEP
	};
	
	for (i=0;i<12;i++) {
		// Find the PCM resource. 
		hResInfo = FindResource(AfxGetInstanceHandle(), MAKEINTRESOURCE(AToneRes[i]), "PCM"); 
		if (hResInfo == NULL) 
			return;

		len = SizeofResource(AfxGetInstanceHandle(), hResInfo);
		len = (len>2048)? 2048:len;
 
		// Load the PCM resource into Global memory. 
		hRes = LoadResource(AfxGetInstanceHandle(), hResInfo); 
		if (hRes == NULL) 
			return; 
 
		// Lock the PCM resource and read it 
		lpRes = (char*)LockResource(hRes); 
		if (lpRes != NULL) { 
			memcpy(ToneA[i].buf, lpRes, len);
			ToneA[i].buflen = len;
			UnlockResource(hRes); 
		} 
		// Free the PCM resource
		FreeResource(hRes);
	}
	for (i=0;i<12;i++) {
		// Find the PCM resource. 
		hResInfo = FindResource(AfxGetInstanceHandle(), MAKEINTRESOURCE(UToneRes[i]), "PCM"); 
		if (hResInfo == NULL) 
			return;

		len = SizeofResource(AfxGetInstanceHandle(), hResInfo);
		len = (len>2048)? 2048:len;
 
		// Load the PCM resource into Global memory. 
		hRes = LoadResource(AfxGetInstanceHandle(), hResInfo); 
		if (hRes == NULL) 
			return; 
 
		// Lock the PCM resource and read it. 
		lpRes = (char*)LockResource(hRes); 
		if (lpRes != NULL) {
			memcpy(ToneU[i].buf, lpRes, len);
			ToneU[i].buflen = len;
			UnlockResource(hRes); 
		}
		// Free the PCM resource
		FreeResource(hRes);
	}
	for (i=0;i<12;i++) {
		// Find the PCM resource. 
		hResInfo = FindResource(AfxGetInstanceHandle(), MAKEINTRESOURCE(G729ToneRes[i]), "PCM"); 
		if (hResInfo == NULL) 
			return;

		len = SizeofResource(AfxGetInstanceHandle(), hResInfo);
		len = (len>2048)? 2048:len;
 
		// Load the PCM resource into Global memory. 
		hRes = LoadResource(AfxGetInstanceHandle(), hResInfo); 
		if (hRes == NULL) 
			return; 
 
		// Lock the PCM resource and read it. 
		lpRes = (char*)LockResource(hRes); 
		if (lpRes != NULL) {
			memcpy(G729Tone[i].buf, lpRes, len);
			G729Tone[i].buflen = len;
			UnlockResource(hRes); 
		}
		// Free the PCM resource
		FreeResource(hRes);
	}
	// add by tyhuang 2005/12/2
	for (i=0;i<12;i++) {
		// Find the PCM resource. 
		hResInfo = FindResource(AfxGetInstanceHandle(), MAKEINTRESOURCE(G723ToneRes[i]), "PCM"); 
		if (hResInfo == NULL) 
			return;

		len = SizeofResource(AfxGetInstanceHandle(), hResInfo);
		len = (len>2048)? 2048:len;
 
		// Load the PCM resource into Global memory. 
		hRes = LoadResource(AfxGetInstanceHandle(), hResInfo); 
		if (hRes == NULL) 
			return; 
 
		// Lock the PCM resource and read it. 
		lpRes = (char*)LockResource(hRes); 
		if (lpRes != NULL) {
			memcpy(G723Tone[i].buf, lpRes, len);
			G723Tone[i].buflen = len-2; // have some tip... by tyHuang 2006/03/15
			UnlockResource(hRes); 
		}
		// Free the PCM resource
		FreeResource(hRes);
	}
}
#endif

// sam add: calc data receive rate in bytes per seconds

// reset statistic buffer with expected data rate
void CMediaManager::_ClearStatBuf()
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
void CMediaManager::_ShiftStatBuf()		
{
	time_t now;
	time(&now);

	int diff = now - m_StatBufTimestamp;
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
	for ( int i=3; i>=1; i--)
	{
		int n = i-diff;
		m_StatBuf[i] = (n>=0)? m_StatBuf[n] : 0;
	}
	m_StatBuf[0] = 0;
}

// log received bytes
void CMediaManager::_IncStatBuf( int n)
{
	_ShiftStatBuf();

	m_StatBuf[0] += n;
}


// % of real / expected received bytes
double CMediaManager::GetReceiveQuality()
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
		return 0.0;	// not support type (NOTE: not support GSM)
	}

	_ShiftStatBuf();

	// for time slot in [m_SilenceTimestamp, m_DataTimestamp], assume no data lose                  
	for ( int i=1; i<=3; i++)                                                                       
	{                                                                                               
		if ( ( m_SilenceTimestamp != 0 && m_StatBufTimestamp - i >= m_SilenceTimestamp) &&      
			( m_DataTimestamp == 0 || m_StatBufTimestamp - i <= m_DataTimestamp) )          
			m_StatBuf[i] = datarate;                                                        
	}                                                                                               

	double quality = ( m_StatBuf[1] + 
					   m_StatBuf[2] + 
					   m_StatBuf[3] ) / 3.0 / datarate;
	return quality;
}

void CMediaManager::PrintOutRTPStat()
{
	//cclRTPStat( m_RtpChan ); // sjhuang test 2006/03/01
}

