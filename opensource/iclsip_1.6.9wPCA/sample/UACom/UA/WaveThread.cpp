// CWaveInThread.cpp : implementation file
//


#include "stdafx.h"
#include "WaveThread.h"

#ifdef DEBUG_DEBUG
#define DEBUG_WAVEIO 1
#else
#define DEBUG_WAVEIO 0
#endif

/////////////////////////////////////////////////////////////////////////////
// CWaveInThread thread

CWaveInThread::CWaveInThread()
{
   m_pcbWaveIn                = NULL;
   m_bInitialized             = FALSE;
   m_threadId                 = 0;
   m_thread                   = NULL;
   m_hWaveIn                  = NULL;
   m_pWaveInBuffer            = NULL;
   m_pWaveCopyBuffer          = NULL;
   m_pWaveHeaderArray         = NULL;
   m_dwActiveThreadPriority   = THREAD_PRIORITY_NORMAL;
   m_wBufferCount             = 0;
   m_dwSingleBufferSize       = 0;
   m_dwBufferSize             = 0;
   m_dwCopyBufferBytes        = 0;
   m_pCopyBufferPos           = NULL;
   m_bPause                   = FALSE;

   ZeroMemory(&m_wfx, sizeof(m_wfx));

   InitializeCriticalSection(&m_critSecRtp);

   m_hEventKill = CreateEvent(NULL, FALSE, FALSE, NULL);

   
}


CWaveInThread::~CWaveInThread()
{
#if DEBUG_WAVEIO
	if( ptr )
		fclose(ptr);
#endif
	
   // Stop the thread processing
   StopThread();

   // Delete the critical section
	DeleteCriticalSection(&m_critSecRtp);

   // Destroy events
   CloseHandle(m_hEventKill);
}


// Capture initialization
BOOL CWaveInThread::Init(cbWaveIn pcbWaveIn,             // Callback function for event notifications, NULL = No callback!
                         DWORD dwActiveThreadPriority,   // Thread Priority for this thread, if capturing is active
                         WORD wBufferCount,              // Count of buffers to use
                         DWORD dwSingleBufferSize,       // Size of a single buffer in bytes (eg. 8000Hz Mono 16bit: 10ms = 160bytes)
                         WORD wChannels,                 // Mono = 1; Stereo = 2
                         DWORD dwSamplesPerSec,          // 8000Hz/11025Hz/22050Hz/44100Hz
                         WORD wBitsPerSample,
						 BOOL isSave)            // 8bit/16bit
{
   if (
       ( m_bInitialized
       ) ||
       ( dwActiveThreadPriority > THREAD_PRIORITY_IDLE
       ) ||
       ( wBufferCount < INTERNAL_WAVEOUT_BUFFER_COUNT
       ) ||
       ( dwSingleBufferSize < 80
       ) ||
       ( wChannels < 1
       ) ||
       ( wChannels > 2
       ) ||
       ( 
        ( wBitsPerSample != 8
        ) && 
        ( wBitsPerSample != 16
        )
       ) ||
       ( 
        ( dwSamplesPerSec != 8000
        ) && 
        ( dwSamplesPerSec != 11025
        ) && 
        ( dwSamplesPerSec != 22050  
        ) && 
        ( dwSamplesPerSec != 44100
        )
       )
      )
   {
      return FALSE;
   }

   m_pcbWaveIn                = pcbWaveIn;
   m_dwActiveThreadPriority   = dwActiveThreadPriority;
   m_wBufferCount             = wBufferCount;
   m_dwSingleBufferSize       = dwSingleBufferSize;

   m_wfx.cbSize               = 0;
   m_wfx.wFormatTag           = WAVE_FORMAT_PCM;
   m_wfx.nChannels            = wChannels;
   m_wfx.nSamplesPerSec       = dwSamplesPerSec;
   m_wfx.nBlockAlign          = (wBitsPerSample * wChannels) / 8;
   m_wfx.nAvgBytesPerSec      = dwSamplesPerSec * m_wfx.nBlockAlign;
   m_wfx.wBitsPerSample       = wBitsPerSample;

   m_dwBufferSize = wBufferCount * dwSingleBufferSize;

   m_bInitialized = TRUE;
	m_bSave = isSave;
	
#if DEBUG_WAVEIO
	// for test
	count=0;
	if( m_bSave )
		ptr=fopen("C:\\zz_waveIn.txt","w");
	else
		ptr=NULL;
#endif

   return TRUE;
}


// Start capturing
BOOL CWaveInThread::StartThread()
{
   UINT           id;
   WORD           i;

   if ((m_thread) || (!m_bInitialized))
   {
      return FALSE;
   }

   ResetEvent(m_hEventKill); //sjhuang 2006/02/17

   // Start the thread
   m_thread = CreateThread(NULL, 
                           0, 
                           ThreadProc, 
                           (void*)this,
                           0, 
                           &m_threadId);
#if DEBUG_WAVEIO
   count = 0;
#endif
   
   if (m_thread)
   {
      DWORD dwExitCode;
      
      if (GetExitCodeThread(m_thread, &dwExitCode))
      {
         if (dwExitCode != STILL_ACTIVE)
         {
            return FALSE;
         }
      }

      if (m_hWaveIn == NULL)
      {
         m_pWaveInBuffer = new unsigned char[m_dwBufferSize];

         if (m_pWaveInBuffer)
         {
            ZeroMemory(m_pWaveInBuffer, m_dwBufferSize);

            m_pWaveCopyBuffer = new unsigned char[m_dwBufferSize];

            if (m_pWaveCopyBuffer)
            {
               ZeroMemory(m_pWaveCopyBuffer, m_dwBufferSize);

               m_pWaveHeaderArray = new WAVEHDR[m_wBufferCount];

               if (m_pWaveHeaderArray)
               {
                  ZeroMemory(m_pWaveHeaderArray, m_wBufferCount * sizeof(WAVEHDR));

                  for (id = 0; id < waveInGetNumDevs(); id++) 
                  {
	                  if (waveInOpen(&m_hWaveIn, id, &m_wfx, m_threadId, (DWORD)this, CALLBACK_THREAD) == MMSYSERR_NOERROR) 
                     {
                        for (i=0 ; i<m_wBufferCount ; i++)
                        {
                           m_pWaveHeaderArray[i].dwBufferLength = m_dwSingleBufferSize;
                           m_pWaveHeaderArray[i].lpData = (char*)m_pWaveInBuffer + i * m_dwSingleBufferSize;
                           m_pWaveHeaderArray[i].dwUser = i;

                           waveInPrepareHeader(m_hWaveIn, &m_pWaveHeaderArray[i], sizeof(WAVEHDR));

                           waveInAddBuffer(m_hWaveIn, &m_pWaveHeaderArray[i], sizeof(WAVEHDR));

#if DEBUG_WAVEIO
						   if( ptr )
						   fprintf(ptr,"start read data[waveInPrepareHeader]:%d\n",++count);
#endif

                        }

                        ResetBuffer();

                        SetThreadPriority(m_thread, m_dwActiveThreadPriority);

                        m_bPause = FALSE;

                        waveInStart(m_hWaveIn);

                        return TRUE;
	                  }
                  }
               }

               delete m_pWaveCopyBuffer;
               m_pWaveCopyBuffer = NULL;
            }

            delete m_pWaveInBuffer;
            m_pWaveInBuffer = NULL;
         }
      }
   }

   StopThread();

   return FALSE;
}


// Stop capturing
BOOL CWaveInThread::StopThread()
{
   WORD i;

   if (m_thread == NULL)
   {
      return FALSE;
   }

   m_bInitialized = FALSE; //sjhuang 2006/02/17

   SetThreadPriority(m_thread, THREAD_PRIORITY_NORMAL);

   SetEvent(m_hEventKill);

   if (m_hWaveIn)
   {
      waveInStop(m_hWaveIn);
   }

   // This is ugly, wait for 10 seconds, then kill the thread
   DWORD res = WaitForSingleObject(m_thread, 500);

   if (res == WAIT_TIMEOUT)
   {
      if (m_thread)
      {
         // OK, do it the hard way
         TerminateThread(m_thread, 0);

         CloseHandle(m_thread);

         m_thread = NULL;
      }
   }

   if (m_hWaveIn)
   {
      waveInReset(m_hWaveIn);

      if (m_pWaveHeaderArray)
      {
         for (i=0 ; i<m_wBufferCount ; i++)
         {
            waveInUnprepareHeader(m_hWaveIn, &m_pWaveHeaderArray[i], sizeof(WAVEHDR));
         }
      }

      waveInClose(m_hWaveIn);
      m_hWaveIn = NULL;
   }

   if (m_pWaveInBuffer)
   {
      delete m_pWaveInBuffer;
      m_pWaveInBuffer = NULL;
   }

   if (m_pWaveCopyBuffer)
   {
      delete m_pWaveCopyBuffer;
      m_pWaveCopyBuffer = NULL;
   }

   if (m_pWaveHeaderArray)
   {
      delete m_pWaveHeaderArray;
      m_pWaveHeaderArray = NULL;
   }

   return TRUE;
}


// Clear the buffer
void CWaveInThread::ResetBuffer()
{
   EnterCriticalSection(&m_critSecRtp);

   m_pCopyBufferPos = NULL;

   if (m_bPause)
   {
      SetThreadPriority(m_thread, m_dwActiveThreadPriority);

      waveInStart(m_hWaveIn);

      m_bPause = FALSE;
   }

   LeaveCriticalSection(&m_critSecRtp);
}


// Get the size of the needed buffer for one second of data
DWORD CWaveInThread::GetBytesPerSecond()
{
   if (m_bInitialized)
   {
      return m_wfx.nAvgBytesPerSec;
   }

   return 0;
}


// Get the current length of data in the buffer
DWORD CWaveInThread::GetDataLen()
{
   DWORD dwBytes;
   
   EnterCriticalSection(&m_critSecRtp);

   dwBytes = m_dwCopyBufferBytes;
   
   LeaveCriticalSection(&m_critSecRtp);

   return dwBytes;
}


// Get captured data
BOOL CWaveInThread::ReadData(unsigned char* pBuffer, // Data will be copied into the buffer
                             DWORD dwCount)          // Count of bytes to copy
{
   EnterCriticalSection(&m_critSecRtp);

   if (
       ( pBuffer == NULL
       ) ||
       ( m_pCopyBufferPos == NULL
       ) || 
       ( m_dwCopyBufferBytes < dwCount
       )
      )
   {
      LeaveCriticalSection(&m_critSecRtp);

      return FALSE;
   }

   if (m_pCopyBufferPos + dwCount > m_pWaveCopyBuffer + m_dwBufferSize)
   {
      memcpy(pBuffer, 
             m_pCopyBufferPos, 
             m_pWaveCopyBuffer + m_dwBufferSize - m_pCopyBufferPos);
      pBuffer += m_pWaveCopyBuffer + m_dwBufferSize - m_pCopyBufferPos;
      memcpy(pBuffer, 
             m_pWaveCopyBuffer, 
             m_pCopyBufferPos + dwCount - (m_pWaveCopyBuffer + m_dwBufferSize));
      m_pCopyBufferPos = m_pWaveCopyBuffer + ((m_pCopyBufferPos + dwCount) - (m_pWaveCopyBuffer + m_dwBufferSize));
   }
   else
   {
      memcpy(pBuffer, m_pCopyBufferPos, dwCount);
      m_pCopyBufferPos += dwCount;
   }

   m_dwCopyBufferBytes -= dwCount;

   if ((m_bPause) && (m_dwCopyBufferBytes + (m_dwBufferSize / m_wBufferCount) <= m_dwBufferSize))
   {
      SetThreadPriority(m_thread, m_dwActiveThreadPriority);

      waveInStart(m_hWaveIn);

      m_bPause = FALSE;
   }

   LeaveCriticalSection(&m_critSecRtp);

   return TRUE;
}


// Handles MM-API for capturing
DWORD WINAPI CWaveInThread::ThreadProc(LPVOID lpParameter)
{
   CWaveInThread* pWaveIn     = (CWaveInThread*)lpParameter;
   MSG            msg;
   DWORD          dwWait      = 0;
   DWORD          dwUser      = 0;
   WAVEHDR        *pWaveHdr   = NULL;
   WAVE_IN_EVENT  event;

   if (pWaveIn == NULL)
   {
      return 0;
   }

   while ((dwWait = WaitForSingleObject(pWaveIn->m_hEventKill, 5)) != 0)
   {
	   //if( WaitMessage() )
      // Handle MMAPI wave capturing thread messages
      while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
      {
         switch (msg.message)
         {
            case MM_WIM_DATA:
         	   EnterCriticalSection(&pWaveIn->m_critSecRtp);

               event = WAVE_IN_EVENT_NONE;

               pWaveHdr = (WAVEHDR*)msg.lParam;

               // Unprepare header, initialize with the next buffer position, prepare the header 
               //  again and add it to the wave capture device.
               if (pWaveHdr)
               {
				   if( pWaveIn->m_bSave )
					pWaveIn->AddNewBuffer(pWaveHdr->lpData,pWaveHdr->dwBufferLength);

#if DEBUG_WAVEIO
				   if( pWaveIn->ptr )
				fprintf(pWaveIn->ptr,"read data ok, buffer[%d]: count:%d \n",pWaveHdr->dwUser,--pWaveIn->count);
#endif				
                  waveInUnprepareHeader(pWaveIn->m_hWaveIn, pWaveHdr, sizeof(WAVEHDR));

                  dwUser = pWaveHdr->dwUser;

                  if (pWaveIn->m_pCopyBufferPos == 0)
                  {
                     pWaveIn->m_pCopyBufferPos = pWaveIn->m_pWaveCopyBuffer + dwUser * pWaveHdr->dwBufferLength;
                     pWaveIn->m_dwCopyBufferBytes = 0;
                  }

                  if (pWaveIn->m_dwCopyBufferBytes + pWaveHdr->dwBufferLength <= pWaveIn->m_dwBufferSize)
                  {
                     memcpy((char*)pWaveIn->m_pWaveCopyBuffer + dwUser * pWaveHdr->dwBufferLength, pWaveHdr->lpData, pWaveHdr->dwBytesRecorded);

                     if (pWaveHdr->dwBytesRecorded != pWaveHdr->dwBufferLength)
                     {
                        ZeroMemory(pWaveHdr->lpData + pWaveHdr->dwBytesRecorded, pWaveHdr->dwBufferLength - pWaveHdr->dwBytesRecorded);
                     }

                     pWaveIn->m_dwCopyBufferBytes += pWaveHdr->dwBufferLength;

                     event = WAVE_IN_EVENT_NEW_BUFFER;
#if DEBUG_WAVEIO
					 if( pWaveIn->ptr )
					 fprintf(pWaveIn->ptr,"WAVE_IN_EVENT_NEW_BUFFER , count:%d\n",pWaveIn->count);
#endif 
                  }
                  else
                  {
                     SetThreadPriority(pWaveIn->m_thread, THREAD_PRIORITY_NORMAL);

                     waveInStop(pWaveIn->m_hWaveIn);

                     pWaveIn->m_bPause = TRUE;

                     event = WAVE_IN_EVENT_BUFFER_FULL;
#if DEBUG_WAVEIO
					 if( pWaveIn->ptr )
					 fprintf(pWaveIn->ptr,"\n\n==> WAVE_IN_EVENT_BUFFER_FULL , count:%d <==\n\n",pWaveIn->count);
#endif
                  }

                  ZeroMemory(pWaveHdr, sizeof(WAVEHDR));

                  pWaveHdr->dwBufferLength = pWaveIn->m_dwSingleBufferSize;
                  pWaveHdr->lpData = (char*)pWaveIn->m_pWaveInBuffer + dwUser * pWaveIn->m_dwSingleBufferSize;
                  pWaveHdr->dwUser = dwUser;

#if DEBUG_WAVEIO
				  if( pWaveIn->ptr )
					 fprintf(pWaveIn->ptr,"prepare next data read[%d] , count:%d\n",dwUser,++(pWaveIn->count));
#endif

                  waveInPrepareHeader(pWaveIn->m_hWaveIn, pWaveHdr, sizeof(WAVEHDR));

                  waveInAddBuffer(pWaveIn->m_hWaveIn, pWaveHdr, sizeof(WAVEHDR));
               }

         	   LeaveCriticalSection(&pWaveIn->m_critSecRtp);

               if ((pWaveIn->m_pcbWaveIn != NULL) && (event != WAVE_IN_EVENT_NONE))
               {
                  (pWaveIn->m_pcbWaveIn)(event);
               }
               break;
            default:
               break;
         }
      }
   }

   CloseHandle(pWaveIn->m_thread);
   pWaveIn->m_thread = NULL;

   return 0;
}


/////////////////////////////////////////////////////////////////////////////
// CWaveOutThread thread

CWaveOutThread::CWaveOutThread()
{
   m_pcbWaveOut               = NULL;
   m_bInitialized             = FALSE;
   m_threadId                 = 0;
   m_thread                   = NULL;
   m_hWaveOut                 = NULL;
   m_pWaveOutBuffer           = NULL;
   m_pWaveCopyBuffer          = NULL;
   m_pWaveHeaderArray         = NULL;
   m_dwActiveThreadPriority   = THREAD_PRIORITY_NORMAL;
   m_wBufferCount             = 0;
   m_dwSingleBufferSize       = 0;
   m_dwBufferSize             = 0;
   m_dwCopyBufferBytes        = 0;
   m_pCopyBufferPos           = NULL;
   m_bPause                   = FALSE;
   m_dwUsedWaveOutBuffers     = 0;
   m_wNextCopyBuffer          = 0;

   ZeroMemory(&m_wfx, sizeof(m_wfx));

   InitializeCriticalSection(&m_critSecRtp);

   m_hEventKill = CreateEvent(NULL, FALSE, FALSE, NULL);
   
}


CWaveOutThread::~CWaveOutThread()
{
#if DEBUG_WAVEIO
	if( ptr )
		fclose(ptr);
#endif

	//fclose(ptr);
   // Stop the thread processing
   StopThread();

   // Delete the critical section
	DeleteCriticalSection(&m_critSecRtp);

   // Destroy events
   CloseHandle(m_hEventKill);
}


// Output initialization
BOOL CWaveOutThread::Init(cbWaveOut pcbWaveOut,          // Callback function for event notifications, NULL = No callback!
                          DWORD dwActiveThreadPriority,  // Thread Priority for this thread, if OUTPUT is active
                          WORD wBufferCount,             // Count of buffers to use
                          DWORD dwSingleBufferSize,      // Size of a single buffer in bytes (eg. 44100Hz Mono 16bit: 10ms = 882bytes)
                          WORD wChannels,                // Mono = 1; Stereo = 2
                          DWORD dwSamplesPerSec,         // 8000Hz/11025Hz/22050Hz/44100Hz
                          WORD wBitsPerSample,
						  BOOL isSave)           // 8bit/16bit
{
   if (
       ( m_bInitialized
       ) ||
       ( dwActiveThreadPriority > THREAD_PRIORITY_IDLE
       ) ||
       ( wBufferCount < INTERNAL_WAVEOUT_BUFFER_COUNT
       ) ||
       ( dwSingleBufferSize < 80
       ) ||
       ( wChannels < 1
       ) ||
       ( wChannels > 2
       ) ||
       ( 
        ( wBitsPerSample != 8
        ) && 
        ( wBitsPerSample != 16
        )
       ) ||
       ( 
        ( dwSamplesPerSec != 8000
        ) && 
        ( dwSamplesPerSec != 11025
        ) && 
        ( dwSamplesPerSec != 22050  
        ) && 
        ( dwSamplesPerSec != 44100
        )
       )
      )
   {
      return FALSE;
   }

   m_pcbWaveOut               = pcbWaveOut;
   m_dwActiveThreadPriority   = dwActiveThreadPriority;
   m_wBufferCount             = wBufferCount;
   m_dwSingleBufferSize       = dwSingleBufferSize;

   m_wfx.cbSize               = 0;
   m_wfx.wFormatTag           = WAVE_FORMAT_PCM;
   m_wfx.nChannels            = wChannels;
   m_wfx.nSamplesPerSec       = dwSamplesPerSec;
   m_wfx.nBlockAlign          = (wBitsPerSample * wChannels) / 8;
   m_wfx.nAvgBytesPerSec      = dwSamplesPerSec * m_wfx.nBlockAlign;
   m_wfx.wBitsPerSample       = wBitsPerSample;

   m_dwBufferSize = wBufferCount * dwSingleBufferSize;

   m_bInitialized = TRUE;

   m_bSave = isSave;

#if DEBUG_WAVEIO
	// for test
	count=0;
	if( m_bSave )
		ptr=fopen("C:\\zz_waveOut.txt","w");
	else
		ptr=NULL;
#endif
   return TRUE;
}


// Start sound output
BOOL CWaveOutThread::StartThread()
{
   UINT           id;
   WORD           i;

   if ((m_thread) || (!m_bInitialized))
   {
      return FALSE;
   }

   ResetEvent(m_hEventKill); //sjhuang 2006/02/17

   m_dwUsedWaveOutBuffers  = 0;
   m_wNextCopyBuffer       = 0;

   // Start the thread
   m_thread = CreateThread(NULL, 
                           0, 
                           ThreadProc, 
                           (void*)this,
                           0, 
                           &m_threadId);

#if DEBUG_WAVEIO
	// for test
	count=0;
#endif

   if (m_thread)
   {
      DWORD dwExitCode;
      
      if (GetExitCodeThread(m_thread, &dwExitCode))
      {
         if (dwExitCode != STILL_ACTIVE)
         {
            return FALSE;
         }
      }

      if (m_hWaveOut == NULL)
      {
         m_pWaveOutBuffer = new unsigned char[m_dwSingleBufferSize * INTERNAL_WAVEOUT_BUFFER_COUNT];

         if (m_pWaveOutBuffer)
         {
            ZeroMemory(m_pWaveOutBuffer, m_dwSingleBufferSize * INTERNAL_WAVEOUT_BUFFER_COUNT);

            m_pWaveCopyBuffer = new unsigned char[m_dwBufferSize];

            if (m_pWaveCopyBuffer)
            {
               ZeroMemory(m_pWaveCopyBuffer, m_dwBufferSize);

               m_pWaveHeaderArray = new WAVEHDR[INTERNAL_WAVEOUT_BUFFER_COUNT];

               if (m_pWaveHeaderArray)
               {
                  ZeroMemory(m_pWaveHeaderArray, INTERNAL_WAVEOUT_BUFFER_COUNT * sizeof(WAVEHDR));

                  for (id = 0; id < waveOutGetNumDevs(); id++) 
                  {
	                  if (waveOutOpen(&m_hWaveOut, id, &m_wfx, m_threadId, (DWORD)this, CALLBACK_THREAD) == MMSYSERR_NOERROR) 
                     {
                        waveOutPause(m_hWaveOut);

                        for (i=0 ; i<INTERNAL_WAVEOUT_BUFFER_COUNT ; i++)
                        {
                           m_dwUsedWaveOutBuffers |= (1 << i);
                        }

                        ResetBuffer();

                        m_bPause = TRUE;

                        return TRUE;
	                  }
                  }
               }

               delete m_pWaveCopyBuffer;
               m_pWaveCopyBuffer = NULL;
            }

            delete m_pWaveOutBuffer;
            m_pWaveOutBuffer = NULL;
         }
      }
   }

   StopThread();

   return FALSE;
}


// Stop sound output
BOOL CWaveOutThread::StopThread()
{
   WORD i;

   if (m_thread == NULL)
   {
      return FALSE;
   }

   m_bInitialized = FALSE; //sjhuang 2006/02/17

   SetThreadPriority(m_thread, THREAD_PRIORITY_NORMAL);

   SetEvent(m_hEventKill);

   if (m_hWaveOut)
   {
      waveOutPause(m_hWaveOut);
   }

   // This is ugly, wait for 10 seconds, then kill the thread
   DWORD res = WaitForSingleObject(m_thread, 500);

   if (res == WAIT_TIMEOUT)
   {
      if (m_thread)
      {
         // OK, do it the hard way
         TerminateThread(m_thread, 0);

         CloseHandle(m_thread);

         m_thread = NULL;
      }
   }

   if (m_hWaveOut)
   {
      waveOutReset(m_hWaveOut);

      if (m_pWaveHeaderArray)
      {
         for (i=0 ; i<m_wBufferCount ; i++)
         {
            waveOutUnprepareHeader(m_hWaveOut, &m_pWaveHeaderArray[i], sizeof(WAVEHDR));
         }
      }

      waveOutClose(m_hWaveOut);
      m_hWaveOut = NULL;
   }

   if (m_pWaveOutBuffer)
   {
      delete m_pWaveOutBuffer;
      m_pWaveOutBuffer = NULL;
   }

   if (m_pWaveCopyBuffer)
   {
      delete m_pWaveCopyBuffer;
      m_pWaveCopyBuffer = NULL;
   }

   if (m_pWaveHeaderArray)
   {
      delete m_pWaveHeaderArray;
      m_pWaveHeaderArray = NULL;
   }

   return TRUE;
}


// Clear the buffer
void CWaveOutThread::ResetBuffer()
{
   EnterCriticalSection(&m_critSecRtp);

   m_pCopyBufferPos     = m_pWaveCopyBuffer;
   m_dwCopyBufferBytes  = 0;
   m_wNextCopyBuffer    = 0;

   LeaveCriticalSection(&m_critSecRtp);
}


// Get the size of the needed buffer for one second of data
DWORD CWaveOutThread::GetBytesPerSecond()
{
   if (m_bInitialized)
   {
      return m_wfx.nAvgBytesPerSec;
   }

   return 0;
}


// Get the current length of data in the buffer
DWORD CWaveOutThread::GetDataLen()
{
   DWORD dwBytes;
   
   EnterCriticalSection(&m_critSecRtp);

   dwBytes = m_dwCopyBufferBytes;
   
   LeaveCriticalSection(&m_critSecRtp);

   return dwBytes;
}


// Play sound data
BOOL CWaveOutThread::WriteData(unsigned char* pBuffer, // Data will be copied from this buffer
                               DWORD dwCount)          // Count of bytes to copy
{
   int i, iLastIndex;

   EnterCriticalSection(&m_critSecRtp);
	
   if( pBuffer==NULL ) 
   {
	   LeaveCriticalSection(&m_critSecRtp);
	   return FALSE;
   }

   if( ( m_dwCopyBufferBytes + dwCount > m_dwBufferSize ) ) 
   {
		LeaveCriticalSection(&m_critSecRtp);
#if DEBUG_WAVEIO
		if( ptr )
	  fprintf(ptr,"\n\n==> WriteData error --> m_dwCopyBufferBytes + dwCount > m_dwBufferSize <== \n\n");
#endif
		return FALSE;
   }
/*
   if (
       ( pBuffer == NULL
       ) || 
       ( m_dwCopyBufferBytes + dwCount > m_dwBufferSize
       )
      )
   {


      LeaveCriticalSection(&m_critSecRtp);
#if DEBUG_WAVEIO
	  fprintf(ptr,"WriteData error --> m_dwCopyBufferBytes + dwCount > m_dwBufferSize \n");
#endif
      return FALSE;
   }
*/
  

   if (m_pCopyBufferPos + dwCount > m_pWaveCopyBuffer + m_dwBufferSize)
   {
      memcpy(m_pCopyBufferPos,
             pBuffer, 
             m_pWaveCopyBuffer + m_dwBufferSize - m_pCopyBufferPos);
      pBuffer += m_pWaveCopyBuffer + m_dwBufferSize - m_pCopyBufferPos;
      memcpy(m_pWaveCopyBuffer, 
             pBuffer, 
             m_pCopyBufferPos + dwCount - (m_pWaveCopyBuffer + m_dwBufferSize));
      m_pCopyBufferPos = m_pWaveCopyBuffer + ((m_pCopyBufferPos + dwCount) - (m_pWaveCopyBuffer + m_dwBufferSize));
   }
   else
   {
      memcpy(m_pCopyBufferPos, pBuffer, dwCount);
      m_pCopyBufferPos += dwCount;
   }

   m_dwCopyBufferBytes += dwCount;

   while (m_dwCopyBufferBytes >= m_dwSingleBufferSize)
   {
      iLastIndex = -1;

      for (i=INTERNAL_WAVEOUT_BUFFER_COUNT-1 ; i>=0; i--)
      {
         if (m_dwUsedWaveOutBuffers & (1 << i))
         {
            iLastIndex = i;
         }
         else
         {
            if (iLastIndex != -1)
            {
               break;
            }
         }
      }

      if (iLastIndex != -1)
      {
         memcpy(m_pWaveOutBuffer + iLastIndex * m_dwSingleBufferSize,
                m_pWaveCopyBuffer + m_wNextCopyBuffer * m_dwSingleBufferSize,
                m_dwSingleBufferSize);

         m_pWaveHeaderArray[iLastIndex].dwBufferLength = m_dwSingleBufferSize;
         m_pWaveHeaderArray[iLastIndex].lpData = (char*)m_pWaveOutBuffer + iLastIndex * m_dwSingleBufferSize;
         m_pWaveHeaderArray[iLastIndex].dwUser = iLastIndex;

#if DEBUG_WAVEIO
		 if( ptr )
		 fprintf(ptr,"write data[%d], count:%d thread:%d tick:%d\n",iLastIndex,++count,
																	GetThreadPriority(m_thread),GetTickCount());
#endif
		 // sjhuang
		if( m_bSave )
		AddNewBuffer(m_pWaveHeaderArray[iLastIndex].lpData,m_dwSingleBufferSize);

         waveOutPrepareHeader(m_hWaveOut, &m_pWaveHeaderArray[iLastIndex], sizeof(WAVEHDR));

         waveOutWrite(m_hWaveOut, &m_pWaveHeaderArray[iLastIndex], sizeof(WAVEHDR));

         m_dwCopyBufferBytes     -= m_dwSingleBufferSize;
         m_wNextCopyBuffer++;
         m_wNextCopyBuffer       %= m_wBufferCount;
         m_dwUsedWaveOutBuffers  ^= (1 << iLastIndex);

         if ((m_bPause) && (m_dwUsedWaveOutBuffers == 0))
         {
            SetThreadPriority(m_thread, m_dwActiveThreadPriority);

            waveOutRestart(m_hWaveOut);

            m_bPause = FALSE;
         }
      }
      else
      {
         break;
      }
   }

   LeaveCriticalSection(&m_critSecRtp);

   return TRUE;
}


// Handles MM-API for sound output
DWORD WINAPI CWaveOutThread::ThreadProc(LPVOID lpParameter)
{
   CWaveOutThread* pWaveOut   = (CWaveOutThread*)lpParameter;
   MSG            msg;
   DWORD          dwWait      = 0;
   DWORD          dwUser      = 0;
   WAVEHDR        *pWaveHdr   = NULL;
   WAVE_OUT_EVENT event;

   if (pWaveOut == NULL)
   {
      return 0;
   }


   //{	  
//	  char tmp[255];
	//  sprintf(tmp,"start out thread \n");
//	  fwrite(tmp,strlen(tmp),1,pWaveOut->ptr);
//	  }

   int count=0;
	while ((dwWait = WaitForSingleObject(pWaveOut->m_hEventKill, 5)) != 0)
   {
		//if( WaitMessage() )
      // Handle MMAPI wave capturing thread messages
      while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
      {
         switch (msg.message)
         {
            case MM_WOM_DONE:

         	   EnterCriticalSection(&pWaveOut->m_critSecRtp);

               event = WAVE_OUT_EVENT_BUFFER_PLAYED;

               pWaveHdr = (WAVEHDR*)msg.lParam;

               // Unprepare header, initialize with the next buffer position, prepare the header 
               //  again and add it to the wave capture device.
               if (pWaveHdr)
               {
				   
				   // sjhuang
				   //if( pWaveOut->m_bSave )
				  //pWaveOut->AddNewBuffer(pWaveHdr->lpData,pWaveHdr->dwBufferLength);

#if DEBUG_WAVEIO
				   if( pWaveOut->ptr )
				fprintf(pWaveOut->ptr,"write data done[%d], count:%d thread:%d tick:%d\n",pWaveHdr->dwUser,--pWaveOut->count,
																		GetThreadPriority(pWaveOut->m_thread),GetTickCount());
#endif

                  waveOutUnprepareHeader(pWaveOut->m_hWaveOut, pWaveHdr, sizeof(WAVEHDR));

                  dwUser = pWaveHdr->dwUser;

                  ZeroMemory(pWaveHdr, sizeof(WAVEHDR));

                  if (pWaveOut->m_dwCopyBufferBytes >= pWaveOut->m_dwBufferSize)
                  {
					  

                     memcpy(pWaveOut->m_pWaveOutBuffer + dwUser * pWaveOut->m_dwSingleBufferSize,
                            pWaveOut->m_pWaveCopyBuffer + pWaveOut->m_wNextCopyBuffer * pWaveOut->m_dwSingleBufferSize,
                            pWaveOut->m_dwSingleBufferSize);
         
                     pWaveOut->m_pWaveHeaderArray[dwUser].dwBufferLength = pWaveOut->m_dwSingleBufferSize;
                     pWaveOut->m_pWaveHeaderArray[dwUser].lpData = (char*)pWaveOut->m_pWaveOutBuffer + dwUser * pWaveOut->m_dwSingleBufferSize;
                     pWaveOut->m_pWaveHeaderArray[dwUser].dwUser = dwUser;

#if DEBUG_WAVEIO
					 if( pWaveOut->ptr )
				fprintf(pWaveOut->ptr,"\n\n==> CB prepare data [%d], count:%d <==\n\n",dwUser,++pWaveOut->count);
#endif

                     waveOutPrepareHeader(pWaveOut->m_hWaveOut, &pWaveOut->m_pWaveHeaderArray[dwUser], sizeof(WAVEHDR));

                     waveOutWrite(pWaveOut->m_hWaveOut, &pWaveOut->m_pWaveHeaderArray[dwUser], sizeof(WAVEHDR));

                     pWaveOut->m_dwCopyBufferBytes    -= pWaveOut->m_dwSingleBufferSize;
                     pWaveOut->m_wNextCopyBuffer++;
                     pWaveOut->m_wNextCopyBuffer      %= pWaveOut->m_wBufferCount;
                  }
                  else
                  {
					  	

                     pWaveOut->m_dwUsedWaveOutBuffers |= (1 << dwUser);

                     if (0xffffffff >> (32 - INTERNAL_WAVEOUT_BUFFER_COUNT) == pWaveOut->m_dwUsedWaveOutBuffers)
                     {
                        SetThreadPriority(pWaveOut->m_thread, THREAD_PRIORITY_NORMAL);

                        waveOutPause(pWaveOut->m_hWaveOut);

                        pWaveOut->m_bPause = TRUE;

                        event = WAVE_OUT_EVENT_BUFFER_EMPTY;
#if DEBUG_WAVEIO
						if( pWaveOut->ptr )
							fprintf(pWaveOut->ptr,"\n\n ==> WAVE_OUT_EVENT_BUFFER_EMPTY[%d], count:%d <==\n\n",dwUser,pWaveOut->count);
#endif

                     }
                  }
               }

         	   LeaveCriticalSection(&pWaveOut->m_critSecRtp);

               if ((pWaveOut->m_pcbWaveOut != NULL) && (event != WAVE_OUT_EVENT_NONE))
               {
				   
                  (pWaveOut->m_pcbWaveOut)(event);
               }
               break;
            default:
               break;
         }
      }
   }

   CloseHandle(pWaveOut->m_thread);
   pWaveOut->m_thread = NULL;

   return 0;
}

// sjhuang 2006/02/22, for detect record / play stream
void CWaveInThread::ClearBuffer()
{
	//ASSERT(pWaveHdr);
	m_listOfBuffer.RemoveAll();
	
}

void CWaveInThread::AddNewBuffer(char *buf,int size)
{
	//ASSERT(pWaveHdr);

	m_listOfBuffer.AddTail(new CWaveBuffer);
	( (CWaveBuffer*)m_listOfBuffer.GetTail() )->CopyBuffer( buf, \
		size / 2, \
		2 );
	
}

CWave CWaveInThread::MakeWave()
{
	void* pBuffer = new char[GetNumSamples() * 2];
	DWORD dwPosInBuffer = 0L;
	POSITION pos = m_listOfBuffer.GetHeadPosition();
	while (pos) {
		CWaveBuffer* p_waveBuffer = (CWaveBuffer*) m_listOfBuffer.GetNext(pos);
		CopyMemory( (char*)pBuffer + dwPosInBuffer, p_waveBuffer->GetBuffer(), p_waveBuffer->GetNumSamples() * p_waveBuffer->GetSampleSize() );
		dwPosInBuffer += p_waveBuffer->GetNumSamples() * p_waveBuffer->GetSampleSize();
	}
	if( GetNumSamples()>0 )
		m_wave.SetBuffer( pBuffer, GetNumSamples() );
	
	return m_wave;
}

DWORD CWaveInThread::GetNumSamples()
{
	DWORD dwTotal = 0L;
	POSITION pos = m_listOfBuffer.GetHeadPosition();
	while (pos) {
		CWaveBuffer* p_waveBuffer = (CWaveBuffer*) m_listOfBuffer.GetNext(pos);
		dwTotal += p_waveBuffer->GetNumSamples();
	}
	return dwTotal;
}

// sjhuang 2006/02/22, for detect record / play stream
void CWaveOutThread::AddNewBuffer(char *buf,int size)
{
	//ASSERT(pWaveHdr);
	m_listOfBuffer.AddTail(new CWaveBuffer);
	( (CWaveBuffer*)m_listOfBuffer.GetTail() )->CopyBuffer( buf, \
		size / 2, \
		2 );
	
}

CWave CWaveOutThread::MakeWave()
{
	void* pBuffer = new char[GetNumSamples() * 2];
	DWORD dwPosInBuffer = 0L;
	POSITION pos = m_listOfBuffer.GetHeadPosition();
	while (pos) {
		CWaveBuffer* p_waveBuffer = (CWaveBuffer*) m_listOfBuffer.GetNext(pos);
		CopyMemory( (char*)pBuffer + dwPosInBuffer, p_waveBuffer->GetBuffer(), p_waveBuffer->GetNumSamples() * p_waveBuffer->GetSampleSize() );
		dwPosInBuffer += p_waveBuffer->GetNumSamples() * p_waveBuffer->GetSampleSize();
	}
	if( GetNumSamples()>0 )
		m_wave.SetBuffer( pBuffer, GetNumSamples() );
		
	return m_wave;
	
}

DWORD CWaveOutThread::GetNumSamples()
{
	DWORD dwTotal = 0L;
	POSITION pos = m_listOfBuffer.GetHeadPosition();
	while (pos) {
		CWaveBuffer* p_waveBuffer = (CWaveBuffer*) m_listOfBuffer.GetNext(pos);
		dwTotal += p_waveBuffer->GetNumSamples();
	}
	return dwTotal;
}
void CWaveOutThread::ClearBuffer()
{
	//ASSERT(pWaveHdr);
	m_listOfBuffer.RemoveAll();
	
}
