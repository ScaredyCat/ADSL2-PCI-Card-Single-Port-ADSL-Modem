// VideoManager.h: interface for the CVideoManager class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _FOR_VIDEO

#if !defined(AFX_VIDEOMANAGER_H__55310199_D15A_4833_83F8_E87ACDFEA004__INCLUDED_)
#define AFX_VIDEOMANAGER_H__55310199_D15A_4833_83F8_E87ACDFEA004__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "..\UA\UACommon.h"
#include "VFWCamera.h"
//#include "DirectShowCapture.h" //add by Alan 950215
//#include "videoapi.h" //del by Alan 941011
#include "vidapi.h" //add by Alan 9 41011
#include <time.h>



class CVideoManager  
{
public:

	// for test
	FILE *ptr;

	CVideoManager();
	virtual ~CVideoManager();

	static CVideoManager* GetVideoMgr()    
	{
		return s_pVideoManager;
	}

	int RTPPeerConnect(CString ip,int port);
	int RTPDisconnect();
	RCODE SetMediaParameter(int mediatype, unsigned int sizeformat);
	int StartRemoteVideo(HWND hWnd, POINT p, unsigned short scale);
	int StopRemoteVideo();
	int StartLocalVideo(HWND hWnd, POINT p);
	int _StartLocalVideo(int chooseCam=0);
	int StopLocalVideo();
	int	GetMediaType() { return m_VideoType; }
	int GetSizeFormat() { return m_SizeFormat; }

	RCODE RTPOpenPort();
	RCODE	RTPClosePort();
	unsigned int RTPGetPort();

	void OnCodecInit();
	void OnCodecDeinit();
	void OnVideoInit(); 
	void OnVideoDeinit();

#ifndef DXSHOW
	VFWCamera*	m_vfwCam; //del by Alan 950214, for Video Page to access, by sjhuang 2006/03/08
#endif

	CString m_VideoName;
private:
	static CVideoManager* s_pVideoManager;

	static DWORD WINAPI EncodeFrame( void* param);
	static DWORD WINAPI DecodeFrame( void* param);
	static char RTPBuffer[65535];
	static unsigned int RTPLen;

	HANDLE m_hEncoder;
	HANDLE m_hDecoder;
	HANDLE m_hStopEncoder;
	HANDLE m_hStopDecoder;


	int			m_RtpChan;
	BOOL		m_bStartRemoteVideo;




 
	int displayWidth ; // sjhuang 2003/03/02, for DXSHOW control display size
	int displayHeight ;

	int			iVFWFrameRate;	
	int			iEncFrameRate;

	VIDEO_TYPE	m_VideoType;
	unsigned short m_SizeFormat; // 2: QCIF, 3: CIF
	POINT		m_LocalPoint;
	HWND		m_hWnd;

	bool		m_bIsRTPConnected;
	time_t		m_RTPConnectedTime;
	BOOL m_bOnlyIframe;

	// used to reduce restart local video times
	unsigned short m_LastLocalSizeFormat;
	POINT		m_LastLocalPoint;

};

#endif // !defined(AFX_VIDEOMANAGER_H__55310199_D15A_4833_83F8_E87ACDFEA004__INCLUDED_)

#endif // _FOR_VIDEO
