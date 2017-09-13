//----------------------------------------------------------------------------
// File: CaptureSound.cpp
//
// Desc: The CaptureSound sample shows how to use DirectSoundCapture to capture 
//       sound into a wave file 
//
// Copyright (c) 1999-2001 Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------
#define STRICT

#include "StdAfx.h"
#include <basetsd.h>
#include <commdlg.h>
#include <mmreg.h>
#include <dxerr9.h>
#include <dsound.h>
#include "resource.h"
#include "MediaManager.h"
#include "DxInOut.h"


//-----------------------------------------------------------------------------
// Function-prototypes
//-----------------------------------------------------------------------------
HRESULT InitDirectSound( HWND hDlg );
HRESULT FreeDirectSound();

INT_PTR CALLBACK DSoundEnumCallback( GUID* pGUID, LPSTR strDesc, LPSTR strDrvName,
                                     VOID* pContext );
VOID    GetWaveFormatFromIndex( INT nIndex, WAVEFORMATEX* pwfx );
HRESULT CreateCaptureBuffer( WAVEFORMATEX* pwfxInput );
HRESULT CreateOutputBuffer( WAVEFORMATEX* pwfxInput );
HRESULT RecordCapturedData();
HRESULT FillPlayData();
HRESULT	SetAEC();


//-----------------------------------------------------------------------------
// Defines, constants, and global variables
//-----------------------------------------------------------------------------
#define NUM_REC_NOTIFICATIONS  16
#define NUM_PLAY_NOTIFICATIONS  16

//#define MAX(a,b)        ( (a) > (b) ? (a) : (b) )

#define SAFE_DELETE(p)  { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }


LPDIRECTSOUNDFULLDUPLEX		g_pDSFullDuplex	= NULL;
//LPDIRECTSOUND			g_pDSOutput	= NULL;
LPDIRECTSOUNDBUFFER8		g_pDSBOutput	= NULL;
//LPDIRECTSOUNDCAPTURE		g_pDSCapture	= NULL;
LPDIRECTSOUNDCAPTUREBUFFER8	g_pDSBCapture	= NULL;
LPDIRECTSOUNDNOTIFY        g_pDSNotify          = NULL;
LPDIRECTSOUNDNOTIFY        g_pDSNotify2         = NULL;
HINSTANCE                  g_hInst              = NULL;
GUID                       g_guidCaptureDevice  = GUID_NULL;
BOOL                       g_bRecording;
WAVEFORMATEX               g_wfxInput;
DSBPOSITIONNOTIFY          g_aPosNotify[ NUM_REC_NOTIFICATIONS + 1 ]; 
DSBPOSITIONNOTIFY          g_aPosNotify2[ NUM_PLAY_NOTIFICATIONS + 1 ];
HANDLE                     g_hNotificationEvent;
HANDLE                     g_hNotificationEvent2;
BOOL                       g_abInputFormatSupported[20];
DWORD                      g_dwCaptureBufferSize;
DWORD			   g_dwOutputBufferSize;
DWORD                      g_dwNextCaptureOffset;
DWORD			   g_dwNextOutputOffset;
DWORD                      g_dwNotifySize;
DWORD			   g_dwNotifySize2;

unsigned long threadRecordId;
unsigned long threadPlayId;
HANDLE hthreadRecord;
HANDLE hthreadPlay;

// Set aside static storage space for 20 audio drivers
GUID  AudioDriverGUIDs[20];
DWORD dwAudioDriverIndex = 0;


static DWORD WINAPI DxThreadRecording(void* data)
{
    HRESULT     hr;
    BOOL	bDone;
    DWORD	dwResult;

    bDone = FALSE;

    while (!bDone) {

	    dwResult = MsgWaitForMultipleObjects( 1, &g_hNotificationEvent, 
                                              FALSE, INFINITE, QS_ALLEVENTS );
	    switch( dwResult )
	    {
		    case WAIT_OBJECT_0 + 0:
			// g_hNotificationEvents[0] is signaled

			// This means that DirectSound just finished playing 
			// a piece of the buffer, so we need to fill the circular 
			// buffer with new sound from the wav file

			if( FAILED( hr = RecordCapturedData() ) )
			{
			    DXTRACE_ERR_MSGBOX( TEXT("RecordCapturedData"), hr );
			    AfxMessageBox( _T("Error handling DirectSound notifications. ") );
			    bDone = TRUE;
			}
			break;
	    }
    }

    return 0;
}

static DWORD WINAPI DxThreadPlaying(void* data)
{
    HRESULT     hr;
    BOOL	bDone;
    DWORD	dwResult;

    bDone = FALSE;

    while (!bDone) {

	    dwResult = MsgWaitForMultipleObjects( 1, &g_hNotificationEvent2, 
                                              FALSE, INFINITE, QS_ALLEVENTS );
	    switch( dwResult )
	    {
		    case WAIT_OBJECT_0 + 0:
			// g_hNotificationEvents[0] is signaled

			// This means that DirectSound just finished playing 
			// a piece of the buffer, so we need to fill the circular 
			// buffer with new sound from the wav file

			if( FAILED( hr = FillPlayData() ) )
			{
			    DXTRACE_ERR_MSGBOX( TEXT("FillPlayData"), hr );
			    AfxMessageBox( _T("Error handling DirectSound notifications. ") );
			    bDone = TRUE;
			}
			break;
	    }
    }

    return 0;
}

int DxStartRecording()
{
	HRESULT hr;
	int nerror = 0;

	hthreadRecord = (HANDLE)CreateThread(NULL, 0, DxThreadRecording, NULL, CREATE_SUSPENDED, &threadRecordId);
	//SetThreadPriority(hthreadRecord, THREAD_PRIORITY_ABOVE_NORMAL);
		
	if(hthreadRecord == INVALID_HANDLE_VALUE) 
		nerror = -1;
	else
		ResumeThread(hthreadRecord);

        if( FAILED( hr = g_pDSBCapture->Start( DSCBSTART_LOOPING ) ) )
            return DXTRACE_ERR_MSGBOX( TEXT("Start"), hr );

	return nerror;
}

int DxStartPlaying()
{
	HRESULT hr;
	int nerror = 0;

	hthreadPlay = (HANDLE)CreateThread(NULL, 0, DxThreadPlaying, NULL, CREATE_SUSPENDED, &threadPlayId);
	//SetThreadPriority(hthreadRecord, THREAD_PRIORITY_ABOVE_NORMAL);
		
	if(hthreadPlay == INVALID_HANDLE_VALUE) 
		nerror = -1;
	else
		ResumeThread(hthreadPlay);

        if( FAILED( hr = g_pDSBOutput->Play( 0, 0, DSBPLAY_LOOPING) ) )
            return DXTRACE_ERR_MSGBOX( TEXT("Start"), hr );

	return nerror;
}

HRESULT DxInOutInit( HWND hDlg )
{
    HRESULT hr;

    g_hNotificationEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    g_hNotificationEvent2 = CreateEvent( NULL, FALSE, FALSE, NULL );

    // Init DirectSound
    if( FAILED( hr = InitDirectSound( hDlg ) ) ) {
	// DXTRACE_ERR_MSGBOX( TEXT("InitDirectSound"), hr );
        // AfxMessageBox( _T("Error initializing DirectSound") );
        // PostQuitMessage( 0 );
		return hr;
    }

    return SetAEC();

}


HRESULT SetAEC()
{
    HRESULT hr;
    DSCFXAec		       DSCAec;
    LPDIRECTSOUNDCAPTUREFXAEC8 pDSCaptureFXAEC;
	
    if( FAILED( hr= g_pDSBCapture->GetObjectInPath( GUID_DSCFX_CLASS_AEC,
							0,
							IID_IDirectSoundCaptureFXAec8,
							(void**) &pDSCaptureFXAEC ) ) )
	return DXTRACE_ERR_MSGBOX( TEXT("GetObjectInPath"), hr );

    ZeroMemory( &DSCAec, sizeof(DSCFXAec) );
 
    DSCAec.fEnable = TRUE;
    DSCAec.fNoiseFill = TRUE;
    DSCAec.dwMode = DSCFX_AEC_MODE_FULL_DUPLEX;

    if( FAILED( hr= pDSCaptureFXAEC->SetAllParameters( &DSCAec) ) )
	return DXTRACE_ERR_MSGBOX( TEXT("SetAllParameters"), hr );

    return S_OK;
}



//-----------------------------------------------------------------------------
// Name: InitDirectSound()
// Desc: Initilizes DirectSound
//-----------------------------------------------------------------------------
HRESULT InitDirectSound( HWND hDlg )
{
    HRESULT hr;
    DSBUFFERDESC dsbd;
    DSCBUFFERDESC dscbd;
    int i;
    
    ZeroMemory( &g_aPosNotify, sizeof(DSBPOSITIONNOTIFY) * 
                               (NUM_REC_NOTIFICATIONS + 1) );

    ZeroMemory( &g_aPosNotify2, sizeof(DSBPOSITIONNOTIFY) * 
                               (NUM_PLAY_NOTIFICATIONS + 1) );

    // Initialize COM
    if( FAILED( hr = CoInitialize(NULL) ) )
        return DXTRACE_ERR_MSGBOX( TEXT("CoInitialize"), hr );

    ZeroMemory( &g_wfxInput, sizeof(g_wfxInput));
    g_wfxInput.wFormatTag = WAVE_FORMAT_PCM;
   
    /* 16 = 8000 Hz, 8bit, Mono 
       17 = 8000 Hz, 16bit, Mono */
    GetWaveFormatFromIndex( 17, &g_wfxInput );

    SAFE_RELEASE( g_pDSNotify );
    SAFE_RELEASE( g_pDSNotify2 );
    SAFE_RELEASE( g_pDSBOutput );
    SAFE_RELEASE( g_pDSBCapture );

    // Set the notification size
    g_dwNotifySize2 = g_wfxInput.nAvgBytesPerSec / 50;
    g_dwNotifySize2 -= g_dwNotifySize2 % g_wfxInput.nBlockAlign;   

    // Set the buffer sizes 
    g_dwOutputBufferSize = g_dwNotifySize2 * NUM_PLAY_NOTIFICATIONS;

    // Create the DirectSound buffer 
    ZeroMemory( &dsbd, sizeof(DSBUFFERDESC) );
    dsbd.dwSize          = sizeof(DSBUFFERDESC);
    dsbd.dwFlags         = DSBCAPS_GLOBALFOCUS | 
                           DSBCAPS_CTRLPOSITIONNOTIFY | 
                           DSBCAPS_GETCURRENTPOSITION2;
    dsbd.dwBufferBytes   = g_dwOutputBufferSize;
    // dsbd.guid3DAlgorithm = GUID_NULL;
    dsbd.lpwfxFormat     = &g_wfxInput;


    // Set the notification size
    g_dwNotifySize = g_wfxInput.nAvgBytesPerSec / 50;
    g_dwNotifySize -= g_dwNotifySize % g_wfxInput.nBlockAlign;   

    // Set the buffer sizes 
    g_dwCaptureBufferSize = g_dwNotifySize * NUM_REC_NOTIFICATIONS;

    DSCEFFECTDESC effectDesc[2];
    ZeroMemory( &effectDesc[0], sizeof( DSCEFFECTDESC ) );
    effectDesc[0].dwSize = sizeof( DSCEFFECTDESC );
    effectDesc[0].dwFlags = DSCFX_LOCSOFTWARE;
    effectDesc[0].guidDSCFXClass = GUID_DSCFX_CLASS_AEC;
    effectDesc[0].guidDSCFXInstance = GUID_DSCFX_MS_AEC;
    effectDesc[0].dwReserved1 = 0;
    effectDesc[0].dwReserved2 = 0;

    ZeroMemory( &effectDesc[1], sizeof( DSCEFFECTDESC ) );
    effectDesc[1].dwSize = sizeof( DSCEFFECTDESC );
    effectDesc[1].dwFlags = DSCFX_LOCSOFTWARE;
    effectDesc[1].guidDSCFXClass = GUID_DSCFX_CLASS_NS;
    effectDesc[1].guidDSCFXInstance = GUID_DSCFX_MS_NS;
    effectDesc[1].dwReserved1 = 0;
    effectDesc[1].dwReserved2 = 0;

    // Create the capture buffer
    ZeroMemory( &dscbd, sizeof(DSCBUFFERDESC) );
    dscbd.dwSize        = sizeof(DSCBUFFERDESC);
    dscbd.dwFlags	= DSCBCAPS_CTRLFX;
    dscbd.dwBufferBytes = g_dwCaptureBufferSize;
    dscbd.lpwfxFormat   = &g_wfxInput; // Set the format during creatation
    dscbd.dwFXCount = 2;
    dscbd.lpDSCFXDesc = &effectDesc[0];

    if( FAILED( hr= DirectSoundFullDuplexCreate8( NULL, NULL, &dscbd, &dsbd, 
						hDlg, DSSCL_PRIORITY, //DSSCL_NORMAL,
						&g_pDSFullDuplex,
						&g_pDSBCapture,
						&g_pDSBOutput, 
						NULL) ) ) {
	return hr;
	//return DXTRACE_ERR_MSGBOX( TEXT("DirectSoundFullDuplexCreate8"), hr );	
    }
	
    
    g_dwNextCaptureOffset = 0;

    // Create a notification event, for when the sound stops playing
    if( FAILED( hr = g_pDSBCapture->QueryInterface( IID_IDirectSoundNotify8, 
                                                    (VOID**)&g_pDSNotify ) ) )
        return DXTRACE_ERR_MSGBOX( TEXT("QueryInterface"), hr );

    // Setup the notification positions
    for( i = 0; i < NUM_REC_NOTIFICATIONS; i++ )
    {
        g_aPosNotify[i].dwOffset = (g_dwNotifySize * i) + g_dwNotifySize - 1;
        g_aPosNotify[i].hEventNotify = g_hNotificationEvent;             
    }
    
    // Tell DirectSound when to notify us. the notification will come in the from 
    // of signaled events that are handled in WinMain()
    if( FAILED( hr = g_pDSNotify->SetNotificationPositions( NUM_REC_NOTIFICATIONS, 
                                                            g_aPosNotify ) ) )
        return DXTRACE_ERR_MSGBOX( TEXT("SetNotificationPositions"), hr );

    g_dwNextOutputOffset = 0;

    // Create a notification event, for when the sound stops playing
    if( FAILED( hr = g_pDSBOutput->QueryInterface( IID_IDirectSoundNotify8, 
                                                    (VOID**)&g_pDSNotify2 ) ) )
        return DXTRACE_ERR_MSGBOX( TEXT("QueryInterface"), hr );

    // Setup the notification positions
    for( i = 0; i < NUM_PLAY_NOTIFICATIONS; i++ )
    {
        g_aPosNotify2[i].dwOffset = (g_dwNotifySize * i) + g_dwNotifySize - 1;
        g_aPosNotify2[i].hEventNotify = g_hNotificationEvent2;             
    }
    
    // Tell DirectSound when to notify us. the notification will come in the from 
    // of signaled events that are handled in WinMain()
    if( FAILED( hr = g_pDSNotify2->SetNotificationPositions( NUM_PLAY_NOTIFICATIONS, 
                                                            g_aPosNotify2 ) ) )
        return DXTRACE_ERR_MSGBOX( TEXT("SetNotificationPositions"), hr );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: FreeDirectSound()
// Desc: Releases DirectSound 
//-----------------------------------------------------------------------------
HRESULT FreeDirectSound()
{
    // Release DirectSound interfaces
    SAFE_RELEASE( g_pDSNotify );
    SAFE_RELEASE( g_pDSNotify2 );
    SAFE_RELEASE( g_pDSBCapture );
    SAFE_RELEASE( g_pDSBOutput );

    // Release COM
    CoUninitialize();

    return S_OK;
}


//-----------------------------------------------------------------------------
// Name: GetWaveFormatFromIndex()
// Desc: Returns 20 different wave formats based on nIndex
//-----------------------------------------------------------------------------
VOID GetWaveFormatFromIndex( INT nIndex, WAVEFORMATEX* pwfx )
{
    INT iSampleRate = nIndex / 4;
    INT iType = nIndex % 4;

    switch( iSampleRate )
    {
        case 0: pwfx->nSamplesPerSec = 48000; break;
        case 1: pwfx->nSamplesPerSec = 44100; break;
        case 2: pwfx->nSamplesPerSec = 22050; break;
        case 3: pwfx->nSamplesPerSec = 11025; break;
        case 4: pwfx->nSamplesPerSec =  8000; break;
    }

    switch( iType )
    {
        case 0: pwfx->wBitsPerSample =  8; pwfx->nChannels = 1; break;
        case 1: pwfx->wBitsPerSample = 16; pwfx->nChannels = 1; break;
        case 2: pwfx->wBitsPerSample =  8; pwfx->nChannels = 2; break;
        case 3: pwfx->wBitsPerSample = 16; pwfx->nChannels = 2; break;
    }

    pwfx->nBlockAlign = pwfx->nChannels * ( pwfx->wBitsPerSample / 8 );
    pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
}


//-----------------------------------------------------------------------------
// Name: DSoundEnumCallback()
// Desc: Enumeration callback called by DirectSoundEnumerate
//-----------------------------------------------------------------------------
INT_PTR CALLBACK DSoundEnumCallback( GUID* pGUID, LPSTR strDesc, LPSTR strDrvName,
                                  VOID* pContext )
{
    GUID* pTemp  = NULL;

    if( pGUID )
    {
        if( dwAudioDriverIndex >= 20 )
            return TRUE;

        pTemp = &AudioDriverGUIDs[dwAudioDriverIndex++];
        memcpy( pTemp, pGUID, sizeof(GUID) );
    }

    /*
    HWND hSoundDeviceCombo = (HWND)pContext;

    // Add the string to the combo box
    SendMessage( hSoundDeviceCombo, CB_ADDSTRING, 
                 0, (LPARAM) (LPCTSTR) strDesc );

    // Get the index of the string in the combo box
    INT nIndex = (INT)SendMessage( hSoundDeviceCombo, CB_FINDSTRING, 
                                   0, (LPARAM) (LPCTSTR) strDesc );

    // Set the item data to a pointer to the static guid stored in AudioDriverGUIDs
    SendMessage( hSoundDeviceCombo, CB_SETITEMDATA, 
                 nIndex, (LPARAM) pTemp );
    */

    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: FillPlayData()
// Desc: Copies data from the input buffer to the play buffer 
//-----------------------------------------------------------------------------
HRESULT FillPlayData() 
{
    HRESULT hr;
    VOID* pDSOutputLockedBuffer     = NULL;
    DWORD dwDSOutputLockedBufferSize;

    // Make sure buffers were not lost, if the were we need 
    // to start the capture again
    /*
    DWORD dwStatus;

    if( FAILED( hr = g_pDSBOutput->GetStatus( &dwStatus ) ) )
        return DXTRACE_ERR_MSGBOX( TEXT("GetStatus"), hr );

    if( dwStatus & DSBSTATUS_BUFFERLOST )
    {
        if( FAILED( hr = StartBuffers() ) )
            return DXTRACE_ERR_MSGBOX( TEXT("StartBuffers"), hr );

        return S_OK;
    }
    */

    // Lock the output buffer down
    if( FAILED( hr = g_pDSBOutput->Lock( g_dwNextOutputOffset, g_dwNotifySize2, 
                                         &pDSOutputLockedBuffer, 
                                         &dwDSOutputLockedBufferSize, 
                                         NULL, NULL, 0L ) ) )
        return DXTRACE_ERR_MSGBOX( TEXT("Lock"), hr );

    CMediaManager::RTP2Wav( (char*)pDSOutputLockedBuffer, dwDSOutputLockedBufferSize );

    // Unlock the play buffer
    g_pDSBOutput->Unlock( pDSOutputLockedBuffer, dwDSOutputLockedBufferSize, 
                          NULL, 0 );

    // Move the playback offset along
    g_dwNextOutputOffset += dwDSOutputLockedBufferSize; 
    g_dwNextOutputOffset %= g_dwOutputBufferSize; // Circular buffer

    return S_OK;
}


//-----------------------------------------------------------------------------
// Name: RecordCapturedData()
// Desc: Copies data from the capture buffer to the output buffer 
//-----------------------------------------------------------------------------
HRESULT RecordCapturedData() 
{
    HRESULT hr;
    VOID*   pbCaptureData    = NULL;
    DWORD   dwCaptureLength;
    VOID*   pbCaptureData2   = NULL;
    DWORD   dwCaptureLength2;
    DWORD   dwReadPos;
    DWORD   dwCapturePos;
    LONG lLockSize;

    if( NULL == g_pDSBCapture )
        return S_FALSE;

    /*if( NULL == g_pWaveFile )
        return S_FALSE;
     */

    if( FAILED( hr = g_pDSBCapture->GetCurrentPosition( &dwCapturePos, &dwReadPos ) ) )
        return DXTRACE_ERR_MSGBOX( TEXT("GetCurrentPosition"), hr );

    lLockSize = dwReadPos - g_dwNextCaptureOffset;
    if( lLockSize < 0 )
        lLockSize += g_dwCaptureBufferSize;

    // Block align lock size so that we are always write on a boundary
    lLockSize -= (lLockSize % g_dwNotifySize);

    if( lLockSize == 0 )
        return S_FALSE;

    lLockSize = 320;

    // Lock the capture buffer down
    if( FAILED( hr = g_pDSBCapture->Lock( g_dwNextCaptureOffset, lLockSize, 
                                          &pbCaptureData, &dwCaptureLength, 
                                          &pbCaptureData2, &dwCaptureLength2, 0L ) ) )
        return DXTRACE_ERR_MSGBOX( TEXT("Lock"), hr );

    CMediaManager::Wav2RTP( (char*)pbCaptureData, dwCaptureLength );
    	
    // Move the capture offset along
    g_dwNextCaptureOffset += dwCaptureLength; 
    g_dwNextCaptureOffset %= g_dwCaptureBufferSize; // Circular buffer

    if( pbCaptureData2 != NULL )
    {
		CMediaManager::Wav2RTP( (char*)pbCaptureData2, dwCaptureLength2 );

        // Move the capture offset along
        g_dwNextCaptureOffset += dwCaptureLength2; 
        g_dwNextCaptureOffset %= g_dwCaptureBufferSize; // Circular buffer
    }

    // Unlock the capture buffer
    g_pDSBCapture->Unlock( pbCaptureData,  dwCaptureLength, 
                           pbCaptureData2, dwCaptureLength2 );


    return S_OK;
}





