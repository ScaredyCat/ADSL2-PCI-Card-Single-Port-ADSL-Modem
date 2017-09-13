/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * WavInOut.h
 *
 * $Id: WavInOut.h,v 1.6 2006/03/02 09:31:55 tyhuang Exp $ 
 */

#ifndef __WAVINOUT__
#define __WAVINOUT__

//#include <Mmsystem.h>

//#define WAVIN_BUFFER_BLOCKS 64
//#define WAVOUT_BUFFER_BLOCKS 64

#define WAVIN_BUFFER_BLOCKS 20
#define WAVOUT_BUFFER_BLOCKS 20

#define WAVEIO_DSPTHREAD 0
#define WAVEIO_DSPTHREAD_PLAY 0
#define INOUTBUFSIZE 480

#if WAVEIO_DSPTHREAD
	struct InOutBuffer {
	BOOL use;
	int len;
	char buf[INOUTBUFSIZE];
} ;
#endif

typedef enum {
	WAV_PCMU = 0,
	WAV_GSM = 3,
	WAV_G723 = 4,
	WAV_PCMA = 8,
	WAV_G729 = 18,
	WAV_iLBC = 97
} WAV_FORMAT;



unsigned int ret;

#if WAVEIO_DSPTHREAD
	struct InOutBuffer g_RecordBuf[WAVIN_BUFFER_BLOCKS];
	struct InOutBuffer g_PlayBuf[WAVOUT_BUFFER_BLOCKS];
	int g_RecordBufPutIdx;
	int g_RecordBufGetIdx;
	int g_PlayBufPutIdx;
	int g_PlayBufGetIdx;
	unsigned short StopDSP;//sjhuang
	HANDLE hthreadDSP; //sjhuang
	unsigned long threadDSPId;//sjhuang
#endif

//	static WavInOut *pWavInOut;

	unsigned int wavin_buflen;
	unsigned int wavout_buflen;

	unsigned int wavIn_devID;
	unsigned int wavOut_devID;

//	WAVEFORMATEX format_wavIn;
//	WAVEFORMATEX format_wavOut;

	unsigned short StopRecording;
	unsigned short StopPlaying;

//	HWAVEIN handle_wi;
//	HWAVEOUT handle_wo;

//	WAVEHDR header_wi[WAVIN_BUFFER_BLOCKS];
//	WAVEHDR header_wo[WAVOUT_BUFFER_BLOCKS];

	unsigned long threadRecordId;
	unsigned long threadPlayId;
	int hthreadRecord;
	int hthreadPlay;

	int StopPlayingEvt;

	BOOL isWavInOpen;
	BOOL isWavOutOpen;


	WavInOut();
	
	void wave_init(int (*cbBufferWrite)(char*,int), int (*cbBufferRead)(char*,int) );

	int wavInOpen(WAV_FORMAT Format, WORD BitsPerSample, DWORD SamplesPerSec, WORD Channels, UINT PacketizePeriod);
	int wavInClose();

	int wavOutOpen(WAV_FORMAT Format, WORD BitsPerSample, DWORD SamplesPerSec, WORD Channels, UINT PacketizePeriod);
	int wavOutClose();

	int startRecording();
	int startPlaying();

	int stopRecording();
	int stopPlaying();

	// playSound :
	int playSound(LPCTSTR lpszSound, UINT fuSound); // the same with system Waveform function [sndPlaySound]
	int stopPlaySound();

	//void operator delete(void* wavio);

#if WAVEIO_DSPTHREAD
	int startDSP(); // sjhuang
	int stopDSP();//sjhuang
#endif

#if WAVEIO_DSPTHREAD
	// sjhuang
	int GetDataFromRecordBuf(int *j);
	static int PutDataToRecordBuf(char* buf,int len);

	int checkPlayBuf(void);
	int PutDataToPlayBuf(char* buf,int len);
	int GetDataFromPlayBuf(char *buf, int len);
#endif

	int (*CBBufferWrite)(char* buff,int len);
	int (*CBBufferRead)(char* buff,int len);

	static void CALLBACK WavInOut::waveInProc(HWAVEIN hwi,      
						UINT uMsg,         
						DWORD dwInstance,  
						DWORD dwParam1,    
						DWORD dwParam2);

	static void CALLBACK WavInOut::waveOutProc(HWAVEIN hwi,       
						UINT uMsg,         
						DWORD dwInstance,  
						DWORD dwParam1,    
						DWORD dwParam2);

	static DWORD WINAPI _threadRecording(void* data);
	static DWORD WINAPI _threadPlaying(void* data);

#if WAVEIO_DSPTHREAD
	static DWORD WINAPI _threadDSP(void* data); // sjhuang
#endif

};


#endif /*__WAVINOUT__*/
