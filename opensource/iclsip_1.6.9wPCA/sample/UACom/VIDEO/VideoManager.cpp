// VideoManager.cpp: implementation of the CVideoManager class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _FOR_VIDEO

#include "stdafx.h"
#include "VideoManager.h"
//#include <winsock2.h>
#include "cclRtpVideo.h"
#include "..\UA\UAProfile.h"
#include "..\UAControl.h"
#include "utility.h"
#include "VideoBuff.h"

#include "tmn.h"

#ifdef STATISTICS
#include <winbase.h>
#include <stdio.h>
#endif

//extern "C" int Video_CallBack(int channel, const char* buff, int len);
extern CVideoBuff RingBuffer;

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

CVideoManager* CVideoManager::s_pVideoManager = NULL;
char CVideoManager::RTPBuffer[65535];
//extern HANDLE	g_hVideoDecSemaphore_2631[MAX_TMC_TERMINAL_COUNT]; //add by Alan 941012
extern char H263DecSemaphore_lpName[4/*MAX_TMC_TERMINAL_COUNT*/][80];//add by Alan 941012
static LARGE_INTEGER g_videoCounterFreq; //add by Alan 941012
static LONGLONG  g_videoCountsPerMinSec; //add by Alan 941012
static LONGLONG  g_videoEncInterval;


///////////////////////////////////////////add by Alan 950214
#ifdef DXSHOW
	int cap_width, cap_height;
	TDSCapture* m_dxCap;
#endif
////////////////////////////////////////////////////////////

void VFW_CB_data(unsigned char *InData,long InLeng);


//ping pong buffer ++
struct _VFW_CAPTURE
{
	unsigned char *Camera_Data[2];
	int iLines[2],iPels[2];
	//int State[2]; //0:empty  1:full  2:used
	bool bWritableD[2];
	bool bReadableD[2];
	int color_space[2]; //add by Alan 941209
} stVFWData;
//ping pong buffer --

// some global variables for video codec
int initial_run = 0;
int enc_busy =0;
int dec_busy =0;
int *gm_VideoType;
int encframenumber = 0;	//hola 0825 for I frame for format change//
extern int firstp0;

CODEC_DATA pCodec_enc;
CODEC_DATA pCodec_dec;
IMAGE_DATA pImageData_enc;
IMAGE_DATA pImageData_dec;

/////////////////////////////////////////add by Alan 941209
// colorspaces
#define XVID_CSP_RGB24 	0
#define XVID_CSP_YV12	1
#define XVID_CSP_YUY2	2
#define XVID_CSP_UYVY	3
#define XVID_CSP_I420	4
#define XVID_CSP_RGB555	10
#define XVID_CSP_RGB565	11
#define XVID_CSP_USER	12
#define XVID_CSP_YVYU	1002
#define XVID_CSP_RGB32 	1000
#define XVID_CSP_NULL 	9999
///////////////////////////////////////////////////////////

#define THREAD_STOP_TIMEOUT	1000

#define TEST_DEBUG 0
extern FILE *ptr_test;
extern FILE *ptr_video;
extern FILE *ptr_video_des;
extern FILE *ptr_video_first;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
 
CVideoManager::CVideoManager()
{

#if TEST_DEBUG
	ptr = fopen("c:\\zz_video_debug.txt","w");
#endif

	m_RtpChan = 1;
	cclRTPVideoStartup(2);
	s_pVideoManager = this;
#ifdef DXSHOW
	m_dxCap = NULL;
#else
	m_vfwCam = NULL;
#endif
	m_bStartRemoteVideo = FALSE;
	m_SizeFormat = 2; //sam default : QCIF
	m_VideoType = VIDEO_NULL;
	gm_VideoType = &m_VideoType;	//add for H263/MPEG chanage
	m_LastLocalSizeFormat = -1;
	m_bIsRTPConnected = false;

	iVFWFrameRate = 15;//default VFW frame rate is 15;
    iEncFrameRate = 15;//defualt Enc Frame rate is 15;

	QueryPerformanceFrequency(&g_videoCounterFreq); //add by Alan 941012
	g_videoCountsPerMinSec = g_videoCounterFreq.QuadPart/1000; //add by Alan 941012

	//ping pong buffer ++
	stVFWData.Camera_Data[0]= (unsigned char *)calloc( 352*288*3, sizeof(unsigned char) );
	stVFWData.Camera_Data[1]= (unsigned char *)calloc( 352*288*3, sizeof(unsigned char) );
	stVFWData.iLines[0] = 0;	stVFWData.iLines[1] = 0;
	stVFWData.iPels[0]  = 0;	stVFWData.iPels[1]  = 0;
	//stVFWData.State[0] = 0;	//empty
	//stVFWData.State[1] = 0; //empty
	stVFWData.bWritableD[0]=true;	stVFWData.bWritableD[1]=true;
    stVFWData.bReadableD[0]=false;	stVFWData.bReadableD[1]=false;
	//ping pong buffer --
	
	m_hStopEncoder = CreateEvent( NULL, TRUE, FALSE, NULL);
	m_hStopDecoder = CreateEvent( NULL, TRUE, FALSE, NULL);
	m_hEncoder = NULL;
	m_hDecoder = NULL;

	OnVideoInit();

	// default display size, always QCIF size, sjhuang 2006/03/02
	displayWidth  	= 176;
	displayHeight 	= 144;

#if TEST_DEBUG
	if( ptr )
	{
		fprintf(ptr,"CVideoManager::CVideoManager() \n");
	}
#endif

}

CVideoManager::~CVideoManager()
{
#if TEST_DEBUG
	if( ptr )
	{
		fclose(ptr);
	}
#endif

	// stop encode/decode thread if exist
	StopRemoteVideo();
	CloseHandle( m_hStopEncoder);
	CloseHandle( m_hStopDecoder);

	// free buffer
	if ( !pImageData_dec.pY)
	{	
		free(pImageData_dec.pY);
		pImageData_dec.pY = NULL;	
	}
	if ( !pCodec_dec.packetBuff[0])
	{	
		free(pCodec_dec.packetBuff[0]);
		pCodec_dec.packetBuff[0] = NULL;	
	}	

	//ping pong buffer ++
	free(stVFWData.Camera_Data[0]);
	free(stVFWData.Camera_Data[1]);
	//ping pong buffer --

	cclRTPVideoClose(m_RtpChan);
	cclRTPVideoCleanup();

#ifdef DXSHOW
	if ( m_dxCap)
		delete m_dxCap;
	//m_dxCap->Stop();
#else
	if ( m_vfwCam)
		delete m_vfwCam;
#endif

	OnCodecDeinit();
	OnVideoDeinit();

}

RCODE CVideoManager::SetMediaParameter(int mediatype, unsigned int sizeformat)
{
	m_SizeFormat = sizeformat;
	CString s;
	s.Format( "[VideoManger] Set video size = %d", m_SizeFormat);
	DebugMsg(s);

	switch (mediatype) {
	case CODEC_H263:
		m_VideoType = VIDEO_H263;
		DebugMsg("[VideoManger] Set video codec = H.263");
		break;
	case CODEC_MPEG4:
		m_VideoType = VIDEO_MPEG4;
		DebugMsg("[VideoManger] Set video codec = MPEG4");
		break;
	default:
		return RC_ERROR;
	}

#if TEST_DEBUG
	if( ptr )
	{
		fprintf(ptr,"CVideoManager::SetMediaParameter->typ:%d format:%ud \n",mediatype,sizeformat);
	}
#endif

	return RC_OK;
}

int CVideoManager::RTPPeerConnect(CString ip,int port)
{
	cclRTPVideoAddReceiver(m_RtpChan,(LPSTR)(LPCSTR)ip,port, PF_UNSPEC);
	cclRTPVideoSetEventHandler( Video_CallBack );

	m_bIsRTPConnected = true;
	time( &m_RTPConnectedTime);

	return 0;
}

int CVideoManager::RTPDisconnect()
{
	if (m_bIsRTPConnected) {
		m_bIsRTPConnected = false;
		cclRTPVideoClose(m_RtpChan);
		m_VideoType =VIDEO_NULL;
	}
	return 0;
}



int CVideoManager::StartRemoteVideo(HWND hWnd, POINT p, unsigned short scale)
{
	CUAProfile *pProfile = CUAProfile::GetProfile();
	if( !pProfile->m_bUseVideo ) return 0;

	rtp_pkt_param_t	param; // sjhuang integral, 2006/02/24

	if ( !m_bIsRTPConnected)
		return 0;	// not allow start encoder/decoder if stream is ready

	// force reinit local video if local video size setting is different
	if ( m_LastLocalSizeFormat != m_SizeFormat)
	{
		_StartLocalVideo(); 
	}

	if ( m_bStartRemoteVideo)
	{
		RemoteVideoInit(hWnd);
		SetVideoPosition(0, p.x, p.y);
		ImageScalePercent( scale);

		return 0;	// not allow reentry
	}

	// NOTE: because new and old video codec use different coordination system
	//       we convert it first for new video codec to maintain compatibility with
	//       old client
	/*
	POINT p2;
	p2.x = p.x;
	p2.y = (m_SizeFormat==2) ? p.y+35-133-12 : p.y+35-276-12;
	SetVideoPosition(0, p2.x, p2.y);
	*/
	// NOTE: use new coorination system since v1.0.2.2
	RemoteVideoInit(hWnd);
	SetVideoPosition(0, p.x, p.y);
	ImageScalePercent( scale);

	m_bStartRemoteVideo = TRUE;

	// clear stop signal
	ResetEvent( m_hStopEncoder);
	ResetEvent( m_hStopDecoder);

	// some global parameters for video codec...
	initial_run = 0;
	firstp0 = 0;//hola 0826
	//OnVideoInit();
	OnCodecInit();
	initial_run = 1;
	encframenumber = 0; //hola 0825

	//ping pong buffer ++
	//stVFWData.State[0]=0;
	//stVFWData.State[1]=0;
	stVFWData.bWritableD[0]=true;	stVFWData.bWritableD[1]=true;
	stVFWData.bReadableD[0]=false;	stVFWData.bReadableD[1]=false;
	//ping pong buffer --

  // sjhuang integral 2006/02/24
	param.m =1;
	param.pt = 0;
	param.seq = 0;
	param.ts = 6000;
	param.csrc_list = NULL;

	cclRTPVideoWriteEx(1,&param, "\0", 1);
	cclRTPVideoWriteEx(1,&param, "\0", 1);
	cclRTPVideoWriteEx(1,&param, "\0", 1);

	// create encoder / decoder thread
	DWORD dwEncoderThreadId;
	DWORD dwDecoderThreadId;
	m_hEncoder = CreateThread( NULL, 0, EncodeFrame, this, 0, &dwEncoderThreadId);
	m_hDecoder = CreateThread( NULL, 0, DecodeFrame, this, 0, &dwDecoderThreadId);//hola 1013

	return 0;
}

int CVideoManager::StopRemoteVideo()
{
	if (!m_bStartRemoteVideo)
		return 1;

	// set stop signal
	SetEvent( m_hStopEncoder);
	SetEvent( m_hStopDecoder);

	// wait threads stop
	if ( WaitForSingleObject( m_hEncoder, THREAD_STOP_TIMEOUT) != 0)
	{
		// timeouted, kill it!
		TerminateThread( m_hEncoder, -1);
	}
	if ( WaitForSingleObject( m_hDecoder, THREAD_STOP_TIMEOUT) != 0)
	{
		// timeouted, kill it!
		TerminateThread( m_hDecoder, -1);
	}

#if TEST_DEBUG
	if( ptr )
	{
				fprintf(ptr," over, file close \n\n");
				fclose(ptr);
	}

	if( ptr_test )
	{
		fprintf(ptr_test," over, file close \n\n");
		fclose(ptr_test);
	}

#endif

	if( ptr_video )
	{
		//fprintf(ptr_video," over, file close \n\n");
		fclose(ptr_video);
		ptr_video = NULL;
	}

	if( ptr_video_des )
	{
		fprintf(ptr_video_des," over, file close \n\n");
		fclose(ptr_video_des);
		ptr_video_des = NULL;
	}

	if( ptr_video_first )
	{
		fclose(ptr_video_first);
		ptr_video_first = NULL;
	}


	// release resource
	CloseHandle( m_hEncoder);
	CloseHandle( m_hDecoder);
	m_hEncoder = NULL;
	m_hDecoder = NULL;

	// uninit video codec
	initial_run = 0;
	OnCodecDeinit();
	//OnVideoDeinit();
	
	RemoteVideoDeInit();

	m_bStartRemoteVideo = FALSE;

	return 0;
}

RCODE CVideoManager::RTPOpenPort()
{
	CUAProfile* pProfile = CUAProfile::GetProfile();

	//int nTos = AfxGetApp()->GetProfileInt( SESSION_LOCAL, KEY_RTP_VIDEO_TOS, 0);
	int nTos = 0;
	return cclRTPVideoOpen(m_RtpChan, pProfile->m_LocalAddr, 0,30,34,PF_UNSPEC,nTos);
}

RCODE	CVideoManager::RTPClosePort()
{
	return cclRTPVideoClose(m_RtpChan);
}

unsigned int CVideoManager::RTPGetPort()
{
	return cclRTPVideoGetPort(m_RtpChan);
}


int CVideoManager::StartLocalVideo(HWND hWnd, POINT p)
{
	m_LocalPoint.x = p.x;
	m_LocalPoint.y = p.y;
	m_hWnd = hWnd;

	return _StartLocalVideo();
}

int CVideoManager::_StartLocalVideo(int chooseCam)
{
	CUAProfile *pProfile = CUAProfile::GetProfile();
	if( !pProfile->m_bUseVideo ) return 0;

	// prevent reinit if local video setting is the same
	if ( m_LastLocalSizeFormat == m_SizeFormat &&
		 m_LastLocalPoint.x == m_LocalPoint.x && 
		 m_LastLocalPoint.y == m_LocalPoint.y && !chooseCam )
		return 0;

#ifdef DXSHOW
	
	if(m_dxCap) {
		if( m_dxCap->EnumDevices(NULL)>0 ) // check if has web-cam!!, may be have bug~
		{
			delete m_dxCap;
			m_dxCap = NULL; // sjhuang add
		}
	}

	if(m_SizeFormat==3){ //CIF
		cap_width = 352;
		cap_height = 288;
	}
	else{ //QCIF
		cap_width = 176;
		cap_height = 144;
	}


	m_dxCap = new TDSCapture(GetWindowDC(m_hWnd), cap_width, cap_height, displayWidth,displayHeight) ; 
		
	if( m_dxCap ) 
	{
		//char tmp[255];
		if( m_dxCap->EnumDevices(NULL)>0 )
		{
			m_dxCap->RenderCamera(0);
		}
	}

	
#else
	if ( !m_vfwCam)
	{
		m_vfwCam = new VFWCamera(m_hWnd);
	}

	// select web cam
	m_vfwCam->m_VideoName = pProfile->m_VideoName;
	
	
	//StopLocalVideo();
	m_vfwCam->SetFramesPerSec(0);

	if(m_vfwCam){
		m_vfwCam->SetFramesPerSec(iVFWFrameRate);
		m_vfwCam->SetDisplayPos( m_LocalPoint.x, m_LocalPoint.y);

		if(chooseCam)
			m_vfwCam->DlgVideoSource(); // to choose the same driver's camaras...
		
		if( m_vfwCam->SetandDisplayVideoFormat(m_SizeFormat)==2 )
		{
			//MessageBox(NULL,"")
		}
		
		m_vfwCam->SetVFW_CB_data( VFW_CB_data );
	}

	// after choose web cam, close it
	if(chooseCam)
	{
		if(m_vfwCam)
		{
			// stop preview
			m_vfwCam->SetFramesPerSec(0);
		
			delete m_vfwCam;
			m_vfwCam = NULL;
		}
		return 0;
	}
#endif

	// save last init parameters
	m_LastLocalSizeFormat = m_SizeFormat; //add by Alan 950216
	m_LastLocalPoint = m_LocalPoint;

	return 0;
}

int CVideoManager::StopLocalVideo()
{
#ifndef DXSHOW
	if(m_vfwCam)
	{
		// stop preview
		m_vfwCam->SetFramesPerSec(0);
		
		delete m_vfwCam;
		m_vfwCam = NULL;
	}
#else
	if(m_dxCap) {
		if( m_dxCap->EnumDevices(NULL)>0 ) // check if has web-cam!!
		{
			delete m_dxCap;
			m_dxCap = NULL; // sjhuang add
		}
	}
	
#endif

	m_LastLocalSizeFormat = -1;
	return 0;
}


void CVideoManager::OnCodecInit(void)
{
	H263_ENCODE_PARAM enc_param;
	H263_DECODE_PARAM dec_param;
	
	//int iBitRate=0; //del by Alan 940727
	int fix_Q = 6;//hola 1013

	CUAProfile* pProfile = CUAProfile::GetProfile();

	
	//enc_param.iVideoFormat = m_SizeFormat;
	enc_param.iVideoFormat = pProfile->m_VideoSizeFormat;
	enc_param.bH323PAYLOAD = true;
	enc_param.iTxSize	   = 50;
	enc_param.iBitRate = pProfile->m_VideoBitrate * 1000;
	enc_param.iFrameRate = pProfile->m_VideoFramerate;
	enc_param.iKeyInterval = pProfile->m_VideoKeyInterval; //在這加入你的程式碼來決定每隔幾秒送一張I frame; //add by Alan 941013
	//enc_param.iKeyInterval =30;
	//pProfile->m_bUseQuant = 28;
	if (pProfile->m_bUseQuant)
		enc_param.iQuantizer = pProfile->m_QuantValue;
	else
		enc_param.iQuantizer = 0;

 	Video_EncodeInit( m_VideoType, &enc_param);


	
	
	//Used for setting Bit Rate
	//iBitRate = pProfile->m_VideoBitrate * 1000;	// Kb --> b //del by Alan 940726

	////////////////////////////////////add by Alan 940725
	/*if(m_VideoType==VIDEO_MPEG4)
		Video_SetEncodeParam( m_VideoType, VIDPARAM_MP4_BITRATE, &iBitRate);
	else
		Video_SetEncodeParam( m_VideoType, VIDPARAM_H263_BITRATE, &iBitRate);
	//////////////////////////////////////////////////////
	//Video_SetEncodeParam( VIDEO_MPEG4, VIDPARAM_MP4_BITRATE, &iBitRate); //del by Alan 940725
	
	//Used for setting frame rate
	double fEncFrameRate = pProfile->m_VideoFramerate;
	////////////////////////////////////add by Alan 940725
	if(m_VideoType==VIDEO_MPEG4)
		Video_SetEncodeParam( m_VideoType, VIDPARAM_MP4_FRAMERATE, &fEncFrameRate);
	else
		Video_SetEncodeParam( m_VideoType, VIDPARAM_H263_FRAMERATE, &fEncFrameRate);*/
	//////////////////////////////////////////////////////
	//Video_SetEncodeParam( VIDEO_MPEG4, VIDPARAM_MP4_FRAMERATE, &fEncFrameRate); //del by Alan 940725
	

	// use fix q will disable bitrate framerate
	//Used for setting fixed Q  //hola 1013
	if (pProfile->m_bUseQuant){
		////////////////////////////////////add by Alan 940725
		if(m_VideoType==VIDEO_MPEG4)
			Video_SetEncodeParam( m_VideoType, VIDPARAM_MP4_QUANT, &pProfile->m_QuantValue);
		else
			Video_SetEncodeParam( m_VideoType, VIDPARAM_H263_QUANT, &pProfile->m_QuantValue);
		//////////////////////////////////////////////////////
		//Video_SetEncodeParam( VIDEO_MPEG4, VIDPARAM_MP4_QUANT, &pProfile->m_QuantValue);//hola 1013 //del by Alan 940725
	}


	m_bOnlyIframe = pProfile->m_bOnlyIframe;

	dec_param.iVideoFormat = pProfile->m_VideoSizeFormat;
	dec_param.iFrameRate = 30;
	dec_param.bH323PAYLOAD = true;
	Video_DecodeInit(m_VideoType, &dec_param);
}
  
void CVideoManager::OnCodecDeinit(void)
{
	Video_EncodeUnInit();
	Video_DecodeUnInit();
}

void CVideoManager::OnVideoInit() 
{
	Video_Init();
}

void CVideoManager::OnVideoDeinit() 
{
	Video_UnInit();
}


////////////////////////////////////////////////////////////////////

u_short seqnum = 1; //add by Alan for k 940314
u_long timestamp = 0; //add by Alan for k 940314


#ifdef STATISTICS
FILE *send_file;
int total_sent_bits;
LONGLONG g_preSentCounter;
char send_out_str[3000];
extern char *send_out_ptr; 

LARGE_INTEGER g_videoCounterFreq;
#endif
// the video encoder thread function
DWORD CVideoManager::EncodeFrame(void* pThdParam)
{
	CVideoManager* pMgr = (CVideoManager*)pThdParam;
	
	int iIndexE =0;
	int i=0;
	static rtp_pkt_param_t	param; //del by Alan 940816
	time_t lastIFrameTime = 0;
	encframenumber = 0;//hola 0825 
///////////For Green Screen ++//////////////////////
	int iId= 0;					//Just for one decoder
	int iEncMode = Enc_I_FRAME;

	int iFrameCount     = 1; 
	int iKeyIntervalMin = 1; //Every Five frame have one INTER Frame 
	int iKeyIntervalMax = 10; //Every Five frame have one INTER Frame 
	int iKeyIntervalADD	= 1;
	int iKeyInterval	= iKeyIntervalMin;
///////////For Green Screen --//////////////////////


	////////////new
	LONGLONG prevCounter;
	LONGLONG frameNum = 0;
	LARGE_INTEGER currCounter;
	LONGLONG temp;
	LONGLONG  nextEncCounter = 0;
	LONGLONG startEncTime;
	LONGLONG currEncInterval;
	////////////////

	int status;

	//g_videoCountsPerMinSec = g_videoCounterFreq.QuadPart/1000;
	
#ifdef STATISTICS
	LARGE_INTEGER currCounter;
	int statistics_bitrate;
#endif

	pMgr->iEncFrameRate = CUAProfile::GetProfile()->m_VideoFramerate;
	g_videoEncInterval = (LONGLONG)((g_videoCounterFreq.QuadPart)/pMgr->iEncFrameRate);
	currEncInterval = g_videoEncInterval;

	while (true)
	{
		if ( WaitForSingleObject( pMgr->m_hStopEncoder, 0) == WAIT_OBJECT_0)
			break; // thread stop condition

		//QueryPerformanceCounter( &lpPerformanceCountCurr );
		QueryPerformanceCounter( &currCounter);
		//llElapsedtime = lpPerformanceCountCurr.QuadPart - lpPerformanceCountPrev.QuadPart; //del by Alan 940816
		//fElapsedtime  = (float) (llElapsedtime) / (float)(lpFrequency.QuadPart); //del by Alan 940816

		if(currCounter.QuadPart<prevCounter){ 
			frameNum = 0;
			nextEncCounter = 0;
		}

		if(currCounter.QuadPart > nextEncCounter){
			if(frameNum==0)
				startEncTime = currCounter.QuadPart;

			prevCounter = currCounter.QuadPart;
		}
		else{
			temp = (nextEncCounter-currCounter.QuadPart)/g_videoCountsPerMinSec;
			WaitForSingleObject(pMgr->m_hStopEncoder,temp);
			continue;
		}


		if ( initial_run == 0)	
		{
			WaitForSingleObject(pMgr->m_hStopEncoder, 20); //add by Alan for high bitrate 940809
			//Sleep(30); //del by Alan for high bitrate 940809
			continue;
		}
////////////////////add by Alan 950214
#ifndef DXSHOW
	if(stVFWData.bReadableD[iIndexE]){
#endif
//////////////////////////////////////
	//if(stVFWData.bReadableD[iIndexE]){ //del by Alan 950214
		enc_busy = 1;
			
#ifdef DXSHOW
		pImageData_enc.pY		= m_dxCap->GetImageBuffer();
		pImageData_enc.iLines = 144;	
		pImageData_enc.iPels = 176;	
		pImageData_enc.color_space=XVID_CSP_RGB24; //add by Alan 941209
#else

		pImageData_enc.pY		=stVFWData.Camera_Data[iIndexE];
		pImageData_enc.color_space	=stVFWData.color_space[iIndexE]; //add by Alan 941209
		pImageData_enc.iLines	=stVFWData.iLines[iIndexE];
		pImageData_enc.iPels	=stVFWData.iPels[iIndexE];
#endif



		// NOTE: force every once second send a I-Frame for first 5 seconds of connection
		//For Green Screen ++//////////////////////
		if(iKeyInterval < iKeyIntervalMax){
			if(iFrameCount >= iKeyInterval)
			{
				iEncMode=Enc_I_FRAME; //add by Alan 041007
				iFrameCount   = 1; 
				iKeyInterval += iKeyIntervalADD;
			}
			else
			{
				iEncMode=Enc_P_FRAME;
				iFrameCount++;
			}
		}
		else
			iEncMode=Enc_FRAME_AUTO;
		//For Green Screen --//////////////////////
		__try
		{
			if (pMgr->m_bOnlyIframe)
				status = Video_EncodeFrame( Enc_I_FRAME, 0, &pImageData_enc, &pCodec_enc);
			else
				status = Video_EncodeFrame( iEncMode, 0, &pImageData_enc, &pCodec_enc);
		}
		__except(1)
		{
			status = -10;
		}

		if ( status < 0)
		{	
			////////////////////////
			stVFWData.bReadableD[iIndexE] = false;		
			stVFWData.bWritableD[iIndexE] = true;
			iIndexE = (iIndexE+1)%2; 
			///////////////////////

			enc_busy = 0;
			continue;
		}

		encframenumber++;//hola 0825

		for( i=0; i<pCodec_enc.iValidPacketCount; i++)	//hola 0731
		{
			//param.ver = RTP_VERSION;				//P				//X				//CC
			if (i == pCodec_enc.iValidPacketCount-1 )	
				param.m=1;	//M
			else	
				param.m=0;
			
			if(*gm_VideoType == VIDEO_H263)			//hola 0820
				param.pt=34;						//PT
			if(*gm_VideoType == VIDEO_MPEG4)
				param.pt=96;						//PT

			param.seq = seqnum++;//htons		//Seq

			param.ts = timestamp ;//htonl //add by Alan for k 940315

			if (param.m==1)	
				timestamp +=6000;

			//if((rand()%5)!=0) //for test
			cclRTPVideoWriteEx(1,&param, (const char *)pCodec_enc.packetBuff[i], (int) pCodec_enc.iEncPacketBuffLen[i]);

			WaitForSingleObject(pMgr->m_hStopEncoder, 10); //add by Alan for high bitrate 940809
		}

#ifdef STATISTICS
		for( i=0; i<pCodec_enc.iValidPacketCount; i++)
			total_sent_bits = total_sent_bits + 8*(int) pCodec_enc.iEncPacketBuffLen[i];

		QueryPerformanceCounter(&currCounter);

		if(currCounter.QuadPart -  g_preSentCounter > STATISTICS_PERIOD*g_videoCounterFreq.QuadPart){ //Time preiod has exceeded
			statistics_bitrate = (int)((float)total_sent_bits/((float)(currCounter.QuadPart - g_preSentCounter)/(float)g_videoCounterFreq.QuadPart));
			sprintf(send_out_ptr, "bitrate = %d bps\n", statistics_bitrate);
			send_out_ptr = send_out_str + strlen(send_out_str);
			g_preSentCounter = currCounter.QuadPart;
			total_sent_bits = 0;
		}
#endif	
		enc_busy =0;

		stVFWData.bReadableD[iIndexE] = false;		
		stVFWData.bWritableD[iIndexE] = true;
		iIndexE = (iIndexE+1)%2;
#ifndef DXSHOW
	}
#endif
	frameNum = (currCounter.QuadPart - startEncTime)/currEncInterval+1;
	nextEncCounter = startEncTime+currEncInterval*frameNum;
	if(nextEncCounter<0) //nextCounter has reached the max value
	nextEncCounter = 0x7FFFFFFFFFFFFFFF;

  }

	return 0;
}


// the video decoder thread function
DWORD CVideoManager::DecodeFrame( void* pThdParam) 
{
	CVideoManager* pMgr = (CVideoManager*)pThdParam;

	int dwId=0;
	char *pTail;
	int leng;
	int iId = 0; //add by Alan 941012
///////////For Green Screen ++//////////////////////
	int iDecStatus=0;
	int iFirstI = 1;
///////////For Green Screen --//////////////////////
	if ( !pImageData_dec.pY)	
		pImageData_dec.pY			= (unsigned char *) malloc(352*288*3);	
	if ( !pCodec_dec.packetBuff[0])	
		pCodec_dec.packetBuff[0]	= (unsigned char *) malloc(352*288*3);	

	//g_hVideoDecSemaphore_2631[iId]=CreateSemaphore(NULL,0,RING_BUF_SIZE,H263DecSemaphore_lpName[iId]); //add by Alan 941012

#if TEST_DEBUG
	if( pMgr->ptr )
	{
		fprintf(pMgr->ptr,"CVideoManager::DecodeFrame -> start decode \n");
	}
#endif

	while (true)
	{

		if ( WaitForSingleObject( pMgr->m_hStopDecoder, 0) == WAIT_OBJECT_0)
			break; // thread stop condition;

		if ( initial_run == 0)
		{	
			WaitForSingleObject(pMgr->m_hStopDecoder, 20); //add by Alan for high bitrate 940809
			//Sleep(30); //del by Alan for high bitrate 940809
			continue;
		}
	
		/*if ( dec_busy){
			WaitForSingleObject(pMgr->m_hStopDecoder, 20); //add by Alan for high bitrate 940809
			continue;
		}*/ //del by Alan 940816

		/*if(WaitForSingleObject(g_hVideoDecSemaphore_2631[iId],15)==WAIT_TIMEOUT){
			continue;
		}*/

		if ( RingBuffer.GetFromRingBuff(0, &pTail,&leng) < 0)
		{
			WaitForSingleObject(pMgr->m_hStopDecoder, 10); //add by Alan for high bitrate 940809
			//Sleep(30);

			

			continue;//hola 0731
		}
		else
		{
#if TEST_DEBUG
			if( pMgr->ptr )
			{
				fprintf(pMgr->ptr,"has data \n");
			}
#endif
			RingBuffer.GetFromRingBuff(0, &pTail,&leng);
			RingBuffer.PushRingBuffTail(0); //add by Alan 941013
			memcpy(pCodec_dec.packetBuff[0],pTail,leng);						
			pCodec_dec.iEncPacketBuffLen[0]=leng;
		}

		/*if (RingBuffer->PushRingBuffTail(0) < 0)	
			continue; */ //del by Alan 941013
		//Sleep(0);

		//dec_busy =1;
		//int status=0;
		//dec_busy =0;
///////////For Green Screen ++//////////////////////	
		__try 
		{
		//status = Video_DecodeFrame(0, &pCodec_dec, &pImageData_dec);
#if TEST_DEBUG
			if( pMgr->ptr )
			{
				fprintf(pMgr->ptr,"before Video_DecodeFrame \n");
			}
#endif
			iDecStatus = Video_DecodeFrame(0, &pCodec_dec, &pImageData_dec);  //iDecStatus == 1:Iframe 2:PFrame

#if TEST_DEBUG
			if( pMgr->ptr )
			{
				fprintf(pMgr->ptr,"after Video_DecodeFrame, status:%d \n",iDecStatus);
			}
#endif

		}
		__except(1)
		{
			//status = -10;
			iDecStatus = -10;
#if TEST_DEBUG
			if( pMgr->ptr )
			{
				fprintf(pMgr->ptr," ************ Video_DecodeFrame exception\n");
			}
#endif

		}
		
		//if ( status >= 0)		//if -2 is the format not the same (no decode)
		//	RemoteVideoShow( 0, pImageData_dec.iPels, pImageData_dec.iLines, pImageData_dec.pY );

		if (iFirstI==0 && iDecStatus>=0 ) //return value of P frame is 2, I frame is 1
		{
			RemoteVideoShow(0,pImageData_dec.iPels,pImageData_dec.iLines,pImageData_dec.pY);
#if TEST_DEBUG
			if( pMgr->ptr )
			{
				fprintf(pMgr->ptr," first remote show %d %d \n",pImageData_dec.iPels,pImageData_dec.iLines);
			}
#endif
		}
		else if(iFirstI==1 && iDecStatus==1)	//if -2 is the format not the same (no decode)
		{
#if TEST_DEBUG
			if( pMgr->ptr )
			{
				fprintf(pMgr->ptr," second remote show %d %d \n",pImageData_dec.iPels,pImageData_dec.iLines);
			}
#endif
			
			RemoteVideoShow(0,pImageData_dec.iPels,pImageData_dec.iLines,pImageData_dec.pY);
			iFirstI = 0;
		}
#if TEST_DEBUG
			if( pMgr->ptr )
			{
				fprintf(pMgr->ptr," over \n\n");
			}
#endif

///////////For Green Screen --//////////////////////

	}
	
	return 0;
}


void VFW_CB_data(unsigned char *InData,long InLeng)
{
	static int iIndexWrite =0;
	
	if(initial_run ==0){
		iIndexWrite = 0;
		return;
	}
	//ping pong buffer ++
	/*if     (stVFWData.State[0]==0 && stVFWData.State[1]==0){iIndexWrite=0;}
	else if(stVFWData.State[0]==0 && stVFWData.State[1]==1){iIndexWrite=0;}
	else if(stVFWData.State[0]==0 && stVFWData.State[1]==2){iIndexWrite=0;}
	else if(stVFWData.State[0]==1 && stVFWData.State[1]==0){iIndexWrite=1;}
	else if(stVFWData.State[0]==1 && stVFWData.State[1]==1){return;}//no do 
	else if(stVFWData.State[0]==1 && stVFWData.State[1]==2){return;}//no do 
	else if(stVFWData.State[0]==2 && stVFWData.State[1]==0){iIndexWrite=1;}
	else if(stVFWData.State[0]==2 && stVFWData.State[1]==1){return;}//no do 
	else if(stVFWData.State[0]==2 && stVFWData.State[1]==2){return;}//no do 
	stVFWData.State[iIndexWrite]=2; 
	*/
	//ping pong buffer --
if(stVFWData.bWritableD[iIndexWrite]){

	memcpy(stVFWData.Camera_Data[iIndexWrite],InData,InLeng);

	if ( enc_busy == 0 && initial_run ==1)
	{
		switch ((int)InLeng) 
		{
		case 176*144*3:	
			stVFWData.iLines[iIndexWrite] = 144;	
			stVFWData.iPels[iIndexWrite] = 176;	
			stVFWData.color_space[iIndexWrite] = XVID_CSP_RGB24; //add by Alan 941209
			break;

		case 352*288*3:	
			stVFWData.iLines[iIndexWrite] = 288;	
			stVFWData.iPels[iIndexWrite] = 352;	
			stVFWData.color_space[iIndexWrite] = XVID_CSP_RGB24; //add by Alan 941209
			break;

		///////////////////////add by Alan 941209
		case 176*144*3/2:	
			stVFWData.iLines[iIndexWrite] = 176;	
			stVFWData.iPels[iIndexWrite] = 144;	
			stVFWData.color_space[iIndexWrite] = XVID_CSP_YV12;
			break;

		case 352*288*3/2:	
			stVFWData.iLines[iIndexWrite] = 288;	
			stVFWData.iPels[iIndexWrite] = 352;	
			stVFWData.color_space[iIndexWrite] = XVID_CSP_YV12;
			break;
		/////////////////////////////////////////

		default:	
			return;
		}
	}
	//ping pong buffer ++
	//stVFWData.State[iIndexWrite]=1;
	//ping pong buffer --
	stVFWData.bWritableD[iIndexWrite] = false;
	stVFWData.bReadableD[iIndexWrite] = true;
	iIndexWrite = (iIndexWrite+1)%2;
}

}


#endif // _FOR_VIDEO
