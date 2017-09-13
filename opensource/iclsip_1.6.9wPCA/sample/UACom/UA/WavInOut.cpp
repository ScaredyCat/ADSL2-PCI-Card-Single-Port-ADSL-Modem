/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * WavInOut.cpp
 *
 * $Id: WavInOut.cpp,v 1.11 2006/03/02 09:31:55 tyhuang Exp $
 */

#include <stdafx.h>
#ifndef _WIN32_WCE
#include <process.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <mmreg.h>
#include "WavInOut.h"
#include "UACommon.h"



WavInOut *WavInOut::pWavInOut = 0;
CRITICAL_SECTION wavio_cs;

WavInOut::WavInOut()
{
	InitializeCriticalSection(&wavio_cs);
	isWavInOpen = FALSE;
	isWavOutOpen = FALSE;

	StopPlayingEvt = CreateEvent( NULL, TRUE, FALSE, NULL);
};


WavInOut::~WavInOut()
{
	stopPlaySound();

#ifdef _TEST
	StopRecording = 1;
		
	if( WaitForSingleObject(hthreadRecord,100)==WAIT_TIMEOUT ) {
		TerminateThread(hthreadRecord,0);
	}
	CloseHandle(hthreadRecord);
	hthreadRecord = 0;

	StopPlaying = 1;

	if( WaitForSingleObject(hthreadPlay,100)==WAIT_TIMEOUT ) {
		TerminateThread(hthreadPlay,0);
	}
	CloseHandle(hthreadPlay);
	hthreadPlay = 0;
#endif

	CloseHandle(StopPlayingEvt);

#if WAVEIO_DSPTHREAD
	stopDSP(); // sjhuang
#endif
	
	stopRecording();
	stopPlaying();
	wavOutClose();
	wavInClose();

	DeleteCriticalSection(&wavio_cs);

	WavInOut::pWavInOut = 0;
};

void WavInOut::init(int (*cbBufferWrite)(char*,int), 
		    int (*cbBufferRead)(char*,int) )
{
	__try {
		EnterCriticalSection(&wavio_cs);

		CBBufferWrite = cbBufferWrite;
		CBBufferRead = cbBufferRead;

		pWavInOut->wavin_buflen = 0;
		pWavInOut->wavout_buflen = 0;

		wavIn_devID = WAVE_MAPPER;
		wavOut_devID = WAVE_MAPPER;

		StopRecording = 0;
		StopPlaying = 0;

		hthreadRecord = 0;
		hthreadPlay = 0;

		handle_wi = 0;
		handle_wo = 0;

#if WAVEIO_DSPTHREAD
		StopDSP = 0; //sjhuang
		hthreadDSP = 0;
#endif
		
	}
	__finally {
		LeaveCriticalSection(&wavio_cs);
	}
};


void CALLBACK WavInOut::waveInProc(
  HWAVEIN hwi,       
  UINT uMsg,         
  DWORD dwInstance,  
  DWORD dwParam1,    
  DWORD dwParam2     
)
{
	LPWAVEHDR hdr;
//	int len;

	switch(uMsg) {
	case WIM_DATA:
		//hdr = (LPWAVEHDR)dwParam1;
		//len = pWavInOut->CBBufferWrite(hdr->lpData,hdr->dwBytesRecorded);
#if !WAVEIO_DSPTHREAD
		hdr = (LPWAVEHDR)dwParam1;
		/*len = */pWavInOut->CBBufferWrite(hdr->lpData,hdr->dwBytesRecorded);
#else
		hdr = (LPWAVEHDR)dwParam1;
		/*len = */pWavInOut->PutDataToRecordBuf(hdr->lpData,hdr->dwBytesRecorded);	
#endif
		break;
	case WIM_CLOSE:
		pWavInOut->isWavInOpen = FALSE;
		break;
	case WIM_OPEN:
		pWavInOut->isWavInOpen = TRUE;
		break;
	default:
		break;
	}
}

int WavInOut::wavInOpen(WAV_FORMAT Format, 
			WORD BitsPerSample, 
			DWORD SamplesPerSec, 
			WORD Channels, 
			UINT PacketizePeriod)
{
	GSM610WAVEFORMAT gsm;
	int nerror = 0;

	if(handle_wi!=0)
		return -1;

	__try {
		EnterCriticalSection(&wavio_cs);

		switch (Format) {
		case WAV_GSM:
			gsm.wSamplesPerBlock = 320; // 8000 / (1000ms / 40ms) = 250
			gsm.wfx.cbSize = 2;
			gsm.wfx.nAvgBytesPerSec = 1625; // 13kbps / 8 = 1625 Bps
			gsm.wfx.nBlockAlign = 65; // 40ms: 260bits * 2 = 65bytes
			gsm.wfx.nChannels = 1;
			gsm.wfx.nSamplesPerSec = 8000;
			gsm.wfx.wBitsPerSample = 0;
			gsm.wfx.wFormatTag = WAVE_FORMAT_GSM610;
			break;

/*
		case WAV_PCMU:
		case WAV_PCMA:
			format_wavIn.cbSize = 0;
			format_wavIn.nAvgBytesPerSec = (BitsPerSample/8)*SamplesPerSec;
			format_wavIn.nBlockAlign = Channels*BitsPerSample/8;
			format_wavIn.nChannels = Channels;
			format_wavIn.nSamplesPerSec = SamplesPerSec;
			format_wavIn.wBitsPerSample = BitsPerSample;
#ifdef _WIN32_WCE
			format_wavIn.wFormatTag = WAVE_FORMAT_PCM;
#elif defined(UACOM_USE_G711)
			format_wavIn.wFormatTag = WAVE_FORMAT_PCM;
#else
			format_wavIn.wFormatTag = ((Format==WAV_PCMU)?WAVE_FORMAT_MULAW:WAVE_FORMAT_ALAW);
#endif
			break;
*/

		case WAV_PCMU:
		case WAV_PCMA:
		case WAV_G723:
		case WAV_G729:
		case WAV_iLBC:
			format_wavIn.cbSize = 0;
			format_wavIn.nChannels = Channels;
			format_wavIn.nSamplesPerSec = SamplesPerSec;
			format_wavIn.wBitsPerSample = BitsPerSample;
			format_wavIn.nBlockAlign = Channels * BitsPerSample/8;
			format_wavIn.nAvgBytesPerSec = format_wavIn.nBlockAlign * SamplesPerSec;
			format_wavIn.wFormatTag = WAVE_FORMAT_PCM;
			break;

		default:
			nerror = -1;
		}

		if (nerror != -1) {
			this->wavin_buflen = (Format!=WAV_GSM)?
				PacketizePeriod*format_wavIn.nAvgBytesPerSec/1000:
				320; // (8000Hz Mono 16bit: 20ms = 320bytes)
			
			if( Format!=WAV_GSM ) 
				ret = waveInOpen(&handle_wi,wavIn_devID,&format_wavIn,(unsigned int)WavInOut::waveInProc,0L,CALLBACK_FUNCTION );
			else
				ret = waveInOpen(&handle_wi,(UINT)WAVE_MAPPER,(WAVEFORMATEX*)&(gsm.wfx),(unsigned int)WavInOut::waveInProc,0L,CALLBACK_FUNCTION );

			if( ret != MMSYSERR_NOERROR ) {
				nerror = ret;
			}
		}

		if (nerror == 0) {
			int waitCounter = 0;
			while(!isWavInOpen) {
				if(waitCounter>100)
					nerror = -1;
				Sleep(10);
				waitCounter++;
			}
		}
	}
	__finally {
		LeaveCriticalSection(&wavio_cs);
	}

	return nerror;
};


int WavInOut::wavInClose()
{
	int nerror = 0;
	if(handle_wi==0)
		return -1;

	__try {
		EnterCriticalSection(&wavio_cs);
		ret = waveInClose(handle_wi);
		if( ret != MMSYSERR_NOERROR ) 
			nerror = ret;
		else
			handle_wi = 0;
	}
	__finally {
		LeaveCriticalSection(&wavio_cs);
	}

	return nerror;
};

void CALLBACK WavInOut::waveOutProc(
  HWAVEIN hwi,       
  UINT uMsg,         
  DWORD dwInstance,  
  DWORD dwParam1,    
  DWORD dwParam2     
)
{
	switch(uMsg) {
	case WOM_DONE:	
		break;
	case WOM_CLOSE:
		pWavInOut->isWavOutOpen = FALSE;
		break;
	case WOM_OPEN:
		pWavInOut->isWavOutOpen = TRUE;
		break;
	default:
		break;
	}
}

int WavInOut::wavOutOpen(WAV_FORMAT Format, 
			 WORD BitsPerSample, 
			 DWORD SamplesPerSec, 
			 WORD Channels, 
			 UINT PacketizePeriod)
{
	GSM610WAVEFORMAT gsm;
	int nerror = 0;

	if(handle_wo!=0)
		return -1;

	__try {
		EnterCriticalSection(&wavio_cs);
		
		switch (Format) {
		case WAV_GSM:
			gsm.wSamplesPerBlock = 320; // 8000 / (1000 / 40) = 250
			gsm.wfx.cbSize = 2;
			gsm.wfx.nAvgBytesPerSec = 1625; // 13kbps / 8 = 1625 Bps
			gsm.wfx.nBlockAlign = 65; // 40ms: 260bits * 2 = 65bytes
			gsm.wfx.nChannels = 1;
			gsm.wfx.nSamplesPerSec = 8000;
			gsm.wfx.wBitsPerSample = 0;
			gsm.wfx.wFormatTag = WAVE_FORMAT_GSM610;
			break;

/*
		case WAV_PCMU:
		case WAV_PCMA:
			format_wavOut.cbSize = 0;
			format_wavOut.nAvgBytesPerSec = (BitsPerSample/8)*SamplesPerSec;
			format_wavOut.nBlockAlign = Channels*BitsPerSample/8;
			format_wavOut.nChannels = Channels;
			format_wavOut.nSamplesPerSec = SamplesPerSec;
			format_wavOut.wBitsPerSample = BitsPerSample;
#ifdef _WIN32_WCE
			format_wavOut.wFormatTag = WAVE_FORMAT_PCM;
#elif defined(UACOM_USE_G711)
			format_wavOut.wFormatTag = WAVE_FORMAT_PCM;
#else
			format_wavOut.wFormatTag = ((Format==WAV_PCMU)?WAVE_FORMAT_MULAW:WAVE_FORMAT_ALAW);
#endif
			break;
*/

		case WAV_PCMU:
		case WAV_PCMA:
		case WAV_G723:
		case WAV_G729:
		case WAV_iLBC:
			format_wavOut.cbSize = 0;
			format_wavOut.nChannels = Channels;
			format_wavOut.nSamplesPerSec = SamplesPerSec;
			format_wavOut.wBitsPerSample = BitsPerSample;
			format_wavOut.nBlockAlign = Channels*BitsPerSample/8;
			format_wavOut.nAvgBytesPerSec = format_wavOut.nBlockAlign * SamplesPerSec;
			format_wavOut.wFormatTag = WAVE_FORMAT_PCM;
			break;

		default:
			nerror = -1;
		}
	
		if (nerror != -1) {

			this->wavout_buflen = (Format!=WAV_GSM)?
				PacketizePeriod*format_wavOut.nAvgBytesPerSec/1000:
				320; // (8000Hz Mono 16bit: 20ms = 320bytes)

			if( Format!=WAV_GSM ) 
				ret = waveOutOpen(&handle_wo,wavOut_devID,&format_wavOut,(unsigned int)WavInOut::waveOutProc,0L,CALLBACK_FUNCTION  );
			else
				ret = waveOutOpen(&handle_wo,(UINT)WAVE_MAPPER,&(gsm.wfx),(unsigned int)WavInOut::waveOutProc,0L,CALLBACK_FUNCTION );
		
			if( ret != MMSYSERR_NOERROR ) {
				nerror = ret;
			}
		}

		if (nerror == 0) {
			int waitCounter = 0;
			while(!isWavOutOpen) {
				if(waitCounter>100)
					nerror = -1;
				Sleep(10);
				waitCounter++;
			}
		}
	}
	__finally {
		LeaveCriticalSection(&wavio_cs);
	}

	return nerror;
};


int WavInOut::wavOutClose()
{
	int nerror = 0;
	if(handle_wo==0)
		return -1;

	__try {
		EnterCriticalSection(&wavio_cs);
		ret = waveOutClose(handle_wo);
		if( ret != MMSYSERR_NOERROR ) 
			nerror = ret;
		else
			handle_wo = 0;
	}
	__finally {
		LeaveCriticalSection(&wavio_cs);
	}

	return nerror;
};

int WavInOut::startRecording()
{
	int nerror = 0;
	if(handle_wi==0||hthreadRecord!=0)
		return -1;

#if WAVEIO_DSPTHREAD
	startDSP();
#endif

	__try {
		EnterCriticalSection(&wavio_cs);
		StopRecording = 0;

		for(int i=0; i<WAVIN_BUFFER_BLOCKS; i++)
			header_wi[i].lpData = (char*)malloc(wavin_buflen);

		hthreadRecord = (HANDLE)CreateThread(NULL, 0, _threadRecording, NULL, CREATE_SUSPENDED, &threadRecordId);
	
		SetThreadPriority(hthreadRecord, THREAD_PRIORITY_ABOVE_NORMAL);
		
		if(hthreadRecord == INVALID_HANDLE_VALUE) 
			nerror = -1;
		else
			ResumeThread(hthreadRecord);

	}
	__finally {
		LeaveCriticalSection(&wavio_cs);
	}

	return nerror;
};


int WavInOut::startPlaying()
{
	int nerror = 0;
	if(handle_wo==0||hthreadPlay!=0)
		return -1;

	__try {
		EnterCriticalSection(&wavio_cs);
		
		StopPlaying = 0;
		ResetEvent(StopPlayingEvt);

		for(int i=0; i<WAVOUT_BUFFER_BLOCKS; i++)
			header_wo[i].lpData = (char*)malloc(wavout_buflen);

		hthreadPlay = (HANDLE)CreateThread(NULL, 0, _threadPlaying, NULL, CREATE_SUSPENDED, &threadPlayId);
		
		if(hthreadPlay == INVALID_HANDLE_VALUE) 
			nerror = -1;
		else
			ResumeThread(hthreadPlay);

	}
	__finally {
		LeaveCriticalSection(&wavio_cs);
	}

	return nerror;
};



int WavInOut::stopRecording()
{
	if(hthreadRecord==0)
		return -1;

#if WAVEIO_DSPTHREAD
	stopDSP();
#endif

	__try {
		EnterCriticalSection(&wavio_cs);
		
		StopRecording = 1;

		if( WaitForSingleObject(hthreadRecord,INFINITE)==WAIT_TIMEOUT ) {
			TerminateThread(hthreadRecord,0);
		}
		CloseHandle(hthreadRecord);
		hthreadRecord = 0;

		waveInReset(pWavInOut->handle_wi);

		for(int i=0; i<WAVIN_BUFFER_BLOCKS; i++)
			free(header_wi[i].lpData);	
	}
	__finally {
		LeaveCriticalSection(&wavio_cs);
	}

	return 0;
};


int WavInOut::stopPlaying()
{
	if(hthreadPlay==0)
		return -1;

	__try {
		EnterCriticalSection(&wavio_cs);

		SetEvent(StopPlayingEvt);

		StopPlaying = 1;

		if( WaitForSingleObject(hthreadPlay,INFINITE)==WAIT_TIMEOUT ) {
			TerminateThread(hthreadPlay,0);
		}

		CloseHandle(hthreadPlay);
		hthreadPlay = 0;

		waveOutReset(pWavInOut->handle_wo);

		for(int i=0; i<WAVOUT_BUFFER_BLOCKS; i++)
			free(header_wo[i].lpData);

	}
	__finally {
		LeaveCriticalSection(&wavio_cs);
	}

	return 0;
};

int WavInOut::playSound(LPCTSTR lpszSound, UINT fuSound)
{
	return (sndPlaySound( lpszSound ,fuSound))?0:-1;
};

int WavInOut::stopPlaySound()
{
	return (sndPlaySound(NULL,0))?0:-1;
};

DWORD WINAPI WavInOut::_threadRecording(void* data)
{
	int i,j;
	MMRESULT err;

	::CoInitialize(NULL);
	
	for(i=0;i<WAVIN_BUFFER_BLOCKS;i++) {
		pWavInOut->header_wi[i].dwBufferLength = pWavInOut->wavin_buflen;
		pWavInOut->header_wi[i].dwFlags = 0;
		pWavInOut->header_wi[i].dwBytesRecorded = 0;
	}

	err = waveInStart(pWavInOut->handle_wi);
	if( err != MMSYSERR_NOERROR ) {
		printf("waveInStart fail!!\n");
		return -1;
	}

	i = 0; j = 0;
	while(pWavInOut->StopRecording==0) {

		err = waveInPrepareHeader(pWavInOut->handle_wi,&pWavInOut->header_wi[i],sizeof(WAVEHDR));
		if( err != MMSYSERR_NOERROR ) {
			printf("waveInPrepareHeader fail!!\n");
			return -1;
		}
		err = waveInAddBuffer(pWavInOut->handle_wi,&pWavInOut->header_wi[i],sizeof(WAVEHDR));
		if( err != MMSYSERR_NOERROR ) {
			printf("waveInAddBuffer fail!!\n");
			return -1;
		}

		//i = (++i)%WAVIN_BUFFER_BLOCKS; j++;
		if( ++i==WAVOUT_BUFFER_BLOCKS ) i=0;

		if(++j < WAVIN_BUFFER_BLOCKS) continue;
		else // sjhuang, no more wait, otherwise may be small bugs!
			j = WAVOUT_BUFFER_BLOCKS;

		while((pWavInOut->header_wi[i].dwFlags&WHDR_DONE)!=WHDR_DONE) {
			if(pWavInOut->StopRecording==0)
				Sleep(5);
			else
				goto RECORD_STOP;
		}
		err = waveInUnprepareHeader(pWavInOut->handle_wi,&pWavInOut->header_wi[i],sizeof(WAVEHDR));
		if( err != MMSYSERR_NOERROR ) {
			printf("waveInUnprepareHeader fail!!\n");
		}
		pWavInOut->header_wi[i].dwFlags = 0;
		pWavInOut->header_wi[i].dwBytesRecorded = 0;

	}

RECORD_STOP:
	if( waveInReset(pWavInOut->handle_wi)!=MMSYSERR_NOERROR )
		printf("waveInReset fail!!\n");
	for(i=0;i<WAVIN_BUFFER_BLOCKS;i++) {
		//if((pWavInOut->header_wi[i].dwFlags&WHDR_DONE)==WHDR_DONE) {
		err = waveInUnprepareHeader(pWavInOut->handle_wi,&pWavInOut->header_wi[i],sizeof(WAVEHDR));
		if( err != MMSYSERR_NOERROR ) {
			printf("waveInUnprepareHeader2 fail!!\n");
		}
		//}
	}
	::CoUninitialize();

	// test if close success
	//FILE *ptr;
	//ptr = fopen("C:\\zz_Close_threadRecording.txt","w");
	//fclose(ptr);

	return 0;
};


DWORD WINAPI WavInOut::_threadPlaying(void* data)
{
	int i,j,len,reverse = 0;
	MMRESULT err;

	::CoInitialize(NULL);

	for(i=0;i<WAVOUT_BUFFER_BLOCKS;i++) {
		pWavInOut->header_wo[i].dwFlags = 0;
		pWavInOut->header_wo[i].dwBufferLength = pWavInOut->wavout_buflen;
	}

	i = 0; j = 0;
	//while(pWavInOut->StopPlaying==0) {
	while(WaitForSingleObject( pWavInOut->StopPlayingEvt, 0) != WAIT_OBJECT_0) {

#if !WAVEIO_DSPTHREAD || !WAVEIO_DSPTHREAD_PLAY
		len = pWavInOut->CBBufferRead(pWavInOut->header_wo[i].lpData,pWavInOut->wavout_buflen);
#else
		len = pWavInOut->GetDataFromPlayBuf(pWavInOut->header_wo[i].lpData, pWavInOut->wavout_buflen);
#endif

		if(len<=0) {
			if(pWavInOut->StopPlaying==0) {
				Sleep(5);	continue; 
			}
			else
				goto PLAY_STOP;
		}
		pWavInOut->header_wo[i].dwFlags = 0;

		err = waveOutPrepareHeader(pWavInOut->handle_wo,&pWavInOut->header_wo[i],sizeof(WAVEHDR));
		if( err != MMSYSERR_NOERROR ) {
			printf("waveOutPrepareHeader fail!!\n");
		}

		err = waveOutWrite(pWavInOut->handle_wo,&pWavInOut->header_wo[i],sizeof(WAVEHDR));
		if( err != MMSYSERR_NOERROR ) {
			printf("waveOutWrite fail!! i=%d\n",i);
		}

		//i = (++i)%WAVOUT_BUFFER_BLOCKS; j++;
		if( ++i==WAVOUT_BUFFER_BLOCKS ) i=0;
		
		if( ++j < WAVOUT_BUFFER_BLOCKS )	continue;
		else // sjhuang, no more wait, otherwise may be small bugs!
			j = WAVOUT_BUFFER_BLOCKS;

		// wait wile the iTH header_wo is played
		while( (pWavInOut->header_wo[i].dwFlags&WHDR_DONE)!=WHDR_DONE && pWavInOut->StopPlaying==0 )
			Sleep(2);

		err = waveOutUnprepareHeader(pWavInOut->handle_wo,&pWavInOut->header_wo[i],sizeof(WAVEHDR));
		if( err != MMSYSERR_NOERROR ) {
			printf("waveOutUnprepareHeader fail i=%d!!\n",i);
		}
	}

PLAY_STOP:
	if( waveOutReset(pWavInOut->handle_wo)!=MMSYSERR_NOERROR )
		printf("waveOutReset error!!\n");
	for(i=0;i<WAVOUT_BUFFER_BLOCKS;i++) {
		//if((pWavInOut->header_wo[i].dwFlags&WHDR_DONE)==WHDR_DONE) {
		err = waveOutUnprepareHeader(pWavInOut->handle_wo,&pWavInOut->header_wo[i],sizeof(WAVEHDR));
		if( err != MMSYSERR_NOERROR ) {
			printf("waveInUnprepareHeader2 fail!!\n");
		}
		//}
	}

	MyCString tmpStr;
	tmpStr.Format(_T("[WavInOut] PLAY_STOP"));
	DebugMsg(tmpStr);

	::CoUninitialize();

	// test if close success
	//FILE *ptr;
	//ptr = fopen("C:\\zz_Close_threadPlaying.txt","w");
	//fclose(ptr);

	return 0;
};

#if WAVEIO_DSPTHREAD
///////////////////////////////////////////////////////////////////////////////
// sjhuang 2006/02/15
//

// Interface
int WavInOut::startDSP()
{
	int nerror = 0;
	if(hthreadDSP!=0)
		return -1;


		StopDSP = 0;

		hthreadDSP = (HANDLE)CreateThread(NULL, 0, _threadDSP, NULL, CREATE_SUSPENDED, &threadDSPId);
	
		SetThreadPriority(hthreadDSP, THREAD_PRIORITY_ABOVE_NORMAL);
		
		if(hthreadDSP == INVALID_HANDLE_VALUE) 
			nerror = -1;
		else
			ResumeThread(hthreadDSP);



	return nerror;
};

int WavInOut::stopDSP()
{

	if(hthreadDSP==0)
		return -1;

	__try {
		EnterCriticalSection(&wavio_cs);
		
		StopDSP = 1;

		if( WaitForSingleObject(hthreadDSP,INFINITE)==WAIT_TIMEOUT ) {
			TerminateThread(hthreadDSP,0);
		}
		CloseHandle(hthreadDSP);
		hthreadDSP = 0;
	}
	__finally {
		LeaveCriticalSection(&wavio_cs);
	}

	return 0;
};

// DSP thread
DWORD WINAPI WavInOut::_threadDSP(void* data)
{
	int i,j;
	//MMRESULT err;
	::CoInitialize(NULL);

	// sjhuang
	for( i=0;i<WAVIN_BUFFER_BLOCKS;i++) {
		pWavInOut->g_RecordBuf[i].len = 0;
		pWavInOut->g_RecordBuf[i].use = FALSE;
	}
	for(i=0;i<WAVOUT_BUFFER_BLOCKS;i++) {
		pWavInOut->g_PlayBuf[i].len = 0;
		pWavInOut->g_PlayBuf[i].use = FALSE;
	}
	pWavInOut->g_RecordBufPutIdx = 0;
	pWavInOut->g_RecordBufGetIdx = 0;
	pWavInOut->g_PlayBufPutIdx = 0;
	pWavInOut->g_PlayBufGetIdx = 0;


	
	
	j = 0;
	while(pWavInOut->StopDSP==0) {

		if( !pWavInOut->GetDataFromRecordBuf(&j) ) {
			
			
			Sleep(2);
		}
		

#if WAVEIO_DSPTHREAD_PLAY
		pWavInOut->checkPlayBuf();
#endif		
		
	}
	::CoUninitialize();

	// test if close success
	//FILE *ptr;
	//ptr = fopen("C:\\zz_CloseDSPThread.txt","w");
	//fclose(ptr);
	
	return 0;
}

// DSP function
int WavInOut::PutDataToRecordBuf(char* buf,int len)
{
	if( pWavInOut->g_RecordBuf[pWavInOut->g_RecordBufPutIdx].use ) {
		//AfxMessageBox("Record Buffer Full!");
		return -1;
	}
	
	memcpy(&(pWavInOut->g_RecordBuf[pWavInOut->g_RecordBufPutIdx].buf[0]),buf,len);
	pWavInOut->g_RecordBuf[pWavInOut->g_RecordBufPutIdx].len = len;
	pWavInOut->g_RecordBuf[pWavInOut->g_RecordBufPutIdx].use = TRUE;

	

	//pWavInOut->g_RecordBufPutIdx = ++pWavInOut->g_RecordBufPutIdx%WAVIN_BUFFER_BLOCKS;
	if( ++pWavInOut->g_RecordBufPutIdx==WAVIN_BUFFER_BLOCKS )
		pWavInOut->g_RecordBufPutIdx = 0;

	return len;
}

int WavInOut::GetDataFromRecordBuf(int *j)
{
	//if( g_RecordBufPutIdx < 20 && !(*j) ) {
	//	return 0;
	//} else (*j)=1;

	
	if( g_RecordBuf[g_RecordBufGetIdx].use ) {

	
			
		

			
			pWavInOut->CBBufferWrite(	g_RecordBuf[g_RecordBufGetIdx].buf,
										g_RecordBuf[g_RecordBufGetIdx].len);


			
			g_RecordBuf[g_RecordBufGetIdx].len = 0;
			g_RecordBuf[g_RecordBufGetIdx].use = FALSE;
			
			//g_RecordBufGetIdx = ++g_RecordBufGetIdx%WAVIN_BUFFER_BLOCKS;
			if( ++g_RecordBufGetIdx == WAVIN_BUFFER_BLOCKS )
				g_RecordBufGetIdx = 0;

			return 1;
		}
	return 0;
}

int WavInOut::checkPlayBuf(void)
{
	char buf[480];
	int len=0;
	len = pWavInOut->CBBufferRead(	&buf[0],
									0);
	
	if( len > 0 ) {
		
		PutDataToPlayBuf(buf,len);
	}

	return 0;
}

int WavInOut::PutDataToPlayBuf(char* buf,int len)
{
	memcpy(&(g_PlayBuf[g_PlayBufPutIdx].buf[0]),buf,len);
	g_PlayBuf[g_PlayBufPutIdx].len = len;
	g_PlayBuf[g_PlayBufPutIdx].use = TRUE;

	g_PlayBufPutIdx = ++g_PlayBufPutIdx%WAVOUT_BUFFER_BLOCKS;

	return len;
}

int WavInOut::GetDataFromPlayBuf(char *buf, int len)
{
	if( g_PlayBuf[g_PlayBufGetIdx].use ) {
			
		memcpy(buf,&(g_PlayBuf[g_PlayBufGetIdx].buf[0]),g_PlayBuf[g_PlayBufGetIdx].len);
		len = g_PlayBuf[g_PlayBufGetIdx].len;
		g_PlayBuf[g_PlayBufGetIdx].len = 0;
		g_PlayBuf[g_PlayBufGetIdx].use = FALSE;
		g_PlayBufGetIdx = ++g_PlayBufGetIdx%WAVOUT_BUFFER_BLOCKS;
		return len;
	}
	return 0;
}
#endif
