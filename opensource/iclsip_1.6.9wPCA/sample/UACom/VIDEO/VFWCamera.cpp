// VFWCamera.cpp: implementation of the VFWCamera class.
//
//////////////////////////////////////////////////////////////////////
//#include "vfw.h"
#include "stdafx.h"
#include "VFWCamera.h"

void (*VFW_DATA)(unsigned char *videoframe,long BytesUsed);
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

VFWCamera::VFWCamera(HWND hwnd)
{
	hWnd		= hwnd;
	hWndC		= NULL;

	CameraIndex = NULL;		//default Index of Camera
	lpbi		= NULL;

	//protected:
	framerate	= 30;			//default framerate is 30
	displayXpos	= 0;
	displayYpos	= 0;
	displayWidth  	= 176;
	displayHeight 	= 144;
	buffWidth	= 352;//352;//176//hola 0407
	buffHeight	= 288;//288;//144
	//private
	flagCamera	= 0; //first use the camera parameter
	setDlgVideoFormat=0;
	setDlgVideoSource=0;
	setDlgVideoDisplay=0;
	setDlgVideoCompression=0;
	camera_vision = 1;
}

VFWCamera::~VFWCamera()
{
	FreeAllocateRC();
}


//return
//	 0: input HWND error
//	 1: capCreateWindows error
//	 2: can't not get Camera devide 
//   3: cap.... error
int VFWCamera::DisplayCamera(int UseCameraIndex)
{
	int wIndex=0;
	DWORD dwSize;
	char szDeviceName[80];
	char szDeviceVersion[80];

	if(hWnd==NULL) return 0; 

	FreeAllocateRC();

	if(camera_vision ==1 )
		
	hWndC = capCreateCaptureWindow (
    (LPSTR) "My Capture Window", 
#if 0	//buffer size
	WS_CHILD | WS_VISIBLE /*|WS_EX_APPWINDOW*/,      // window name if pop-up  // window style 
    displayXpos,displayYpos,buffWidth,buffHeight, (HWND) hWnd,1 );  //child ID
#else	//QCIF size
	WS_CHILD | WS_VISIBLE,      // window name if pop-up  // window style 
    displayXpos,displayYpos,displayWidth,displayHeight, (HWND) hWnd,1 );  //child ID
	//displayXpos,displayYpos,buffWidth,buffHeight, (HWND) hWnd,1 );  //child ID
#endif 

	else
		hWndC = capCreateCaptureWindow (
	    (LPSTR) "My Capture Window", 
		WS_CHILD /*| WS_VISIBLE |WS_BORDER*/,      // window name if pop-up  // window style 
	    displayXpos,displayYpos,displayWidth,displayHeight, (HWND) hWnd,1 );  //child ID

	if(hWndC==NULL)		return 1 ;


	int ret=0;
	while(1)
	{
		if ( TRUE != capGetDriverDescription ( wIndex++, szDeviceName,sizeof (szDeviceName), szDeviceVersion,sizeof (szDeviceVersion)) )
		{
			//MessageBox(NULL,"偵測不到影像裝置，請設定影像裝置","Video",MB_OK);
		} 
		else ret=1;

		if( wIndex>9 ) return 2;
		if( m_VideoName.IsEmpty() && ret ) break;
		if( strcmp(szDeviceName,m_VideoName)==0 && ret) break;
	}


	CameraIndex = --wIndex;

	
	if( capDriverConnect(hWndC, CameraIndex) != TRUE ) return 3;
	if( capDriverGetCaps(hWndC, &CapDrvCaps, sizeof (CAPDRIVERCAPS)) != TRUE )  return 31;
	

	if(setDlgVideoFormat==1){	
		setDlgVideoFormat=0; // alter by sjhuang 2006/03/03
		if (CapDrvCaps.fHasDlgVideoFormat /*fHasDlgVideoSource*/)	capDlgVideoFormat(hWndC);//capDlgVideoSource(hWndC);
	}
	
	if(setDlgVideoSource==1){	
		setDlgVideoSource=0;
		if (CapDrvCaps.fHasDlgVideoSource)	capDlgVideoSource(hWndC);
	}
	if(setDlgVideoDisplay==1){	
		setDlgVideoDisplay=0;
		if (CapDrvCaps.fHasDlgVideoDisplay)	capDlgVideoDisplay(hWndC);
	}
	if(	setDlgVideoCompression==1){
		capDlgVideoCompression(hWndC);								 
	}
    
	if( (dwSize = capGetVideoFormatSize(hWndC)) ==0) return 32;
	else 
		lpbi =(BITMAPINFO *)( new BYTE[dwSize]);
	
	capGetVideoFormat(hWndC, lpbi, dwSize);
	lpbi->bmiHeader.biSize			=dwSize;
	lpbi->bmiHeader.biWidth			=buffWidth;		
	lpbi->bmiHeader.biHeight		=buffHeight;
	lpbi->bmiHeader.biPlanes		=1;		
	//lpbi->bmiHeader.biBitCount		=24; //del by Alan 941209
	//lpbi->bmiHeader.biCompression	=BI_RGB;//0	 //del by Alan 941209
	lpbi->bmiHeader.biSizeImage		=buffWidth*buffHeight*3,
	lpbi->bmiHeader.biXPelsPerMeter	=0;	lpbi->bmiHeader.biYPelsPerMeter	=0;
	lpbi->bmiHeader.biClrUsed		=0;	lpbi->bmiHeader.biClrImportant	=0;
	if( capSetVideoFormat(hWndC, lpbi, dwSize) != TRUE ) return 33;
	
	// For Audio /////////////////////////////////////////////////////////////////
/*	WAVEFORMATEX waveformat; 
	capGetAudioFormat(hWndC,&waveformat,capGetAudioFormatSize(hWndC));
	waveformat.nSamplesPerSec	= 8000;//11025
	waveformat.nAvgBytesPerSec	= 16000;//11025
	waveformat.wBitsPerSample	= 16;	//8
	capSetAudioFormat(hWndC,&waveformat,capGetAudioFormatSize(hWndC));
	capSetCallbackOnWaveStream(hWndC,AudioStreamCallback);
*/	//End Audio/////////////////////////////////////////////////////////////////
	
	if( capCaptureGetSetup(hWndC, &CaptureParms, sizeof(CAPTUREPARMS)) != TRUE ) return 3;
	CaptureParms.AVStreamMaster = AVSTREAMMASTER_NONE;
	CaptureParms.fAbortLeftMouse  = false;
	CaptureParms.fAbortRightMouse = false;
	CaptureParms.fCaptureAudio=false; 
	CaptureParms.fYield=true;
	CaptureParms.vKeyAbort = 0; //To abort escape exit function //add by Alan 941112
	CaptureParms.dwRequestMicroSecPerFrame = (DWORD) (1.0e6 / framerate);
	if( capCaptureSetSetup(hWndC, &CaptureParms, sizeof (CAPTUREPARMS)) != TRUE ) return 34;	//frame rate 30

#if 1	//Stream
	//capSetCallbackOnFrame(hWndC,VideoPreviewCallback);
	if( capPreviewRate(hWndC,(1.0e3 / framerate) ) != TRUE ) return 351;
	if( capPreviewScale(hWndC,TRUE) != TRUE ) return 352;
	if( capPreview(hWndC,TRUE) != TRUE ) return 353;
	if( capGetStatus(hWndC, &CapStatus, sizeof (CAPSTATUS)) != TRUE ) return 354;
	if( capSetCallbackOnVideoStream(hWndC,VideoStreamCallback) != TRUE ) return 355;    
    if( capCaptureSequenceNoFile(hWndC) != TRUE ) return 356;
	if( capGrabFrameNoStop(hWndC) != TRUE ) return 357;		
	
#else	//Preview
	if( capSetCallbackOnFrame(hWndC,VideoPreviewCallback) != TRUE ) return 3;    
	if( capGrabFrameNoStop(hWndC) != TRUE ) return 3;
	if( capPreviewRate(hWndC,(1.0e3 / framerate) ) != TRUE ) return 3;
	if( capPreviewScale(hWndC,TRUE) != TRUE ) return 3;
	if( capPreview(hWndC,TRUE) != TRUE ) return 3;
#endif	

	
	return 1;
}

// added by sjhuang 2006/03/08
BOOL VFWCamera::GetCamNameByIndex(int idx,char *name,char *ver)
{
	//int wIndex = 0;
	//int amount = 0;
	char szDeviceName[80];
	char szDeviceVersion[80];
	//for(wIndex = 0; wIndex <10; wIndex ++)
		if(	TRUE == capGetDriverDescription (idx, szDeviceName,sizeof (szDeviceName), szDeviceVersion,sizeof (szDeviceVersion)) ){
			//amount ++;
			strncpy(name,szDeviceName,sizeof(szDeviceName));
			strncpy(ver,szDeviceVersion,sizeof(szDeviceVersion));
			return TRUE;
		}
	return FALSE;//amount;
}

int VFWCamera::GetCamAmount()
{
	int wIndex = 0;
	int amount = 0;
	char szDeviceName[80];
	char szDeviceVersion[80];
	for(wIndex = 0; wIndex <10; wIndex ++)
		if(	TRUE == capGetDriverDescription (wIndex, szDeviceName,sizeof (szDeviceName), szDeviceVersion,sizeof (szDeviceVersion)) ){
			amount ++;	
		}
	return amount;
}

int VFWCamera::GetFramesPerSec()
{
	return framerate; 
}

void VFWCamera::SetDisplayPos(int Xpos, int Ypos)
{
	displayXpos =  Xpos;
	displayYpos =  Ypos;
}

int VFWCamera::SetFramesPerSec(int framespersec)
{
	framerate = framespersec;
	//DisplayCamera(CameraIndex);
	//CaptureParms.dwRequestMicroSecPerFrame = (DWORD) (1.0e6 / framerate);
	//capCaptureSetSetup(hWndC, &CaptureParms, sizeof (CAPTUREPARMS));	//frame rate 30
	return framerate;
}

void VFWCamera::FreeAllocateRC()
{
	if(hWndC){  
		capPreview(hWndC,FALSE);
		capCaptureStop(hWndC);
		capDriverDisconnect (hWndC);
		DestroyWindow(hWndC);
 		hWndC=NULL;	
	}
	if(lpbi){	delete[] lpbi;
		lpbi=NULL;
	}
}

void VFWCamera::SetVFW_CB_data( vfw_cb inF )
{
	VFW_DATA = inF; 
}

LRESULT CALLBACK VFWCamera::VideoPreviewCallback(HWND hwnd, LPVIDEOHDR lpVHdr)
{
	if (!hwnd) 
        return FALSE; 
	//Setframedata(lpVHdr->lpData,lpVHdr->dwBytesUsed);
	return (LRESULT) TRUE ; 
}


LRESULT CALLBACK VFWCamera::VideoStreamCallback(HWND hwnd, LPVIDEOHDR lpVHdr)
{
	if (!hwnd) 
    return FALSE; 
	//Setframedata(lpVHdr->lpData,lpVHdr->dwBytesUsed);
	if(VFW_DATA!=NULL)
		VFW_DATA(lpVHdr->lpData,lpVHdr->dwBytesUsed);
	
	return (LRESULT) TRUE ;
}

LRESULT CALLBACK VFWCamera::AudioStreamCallback(HWND hwnd, LPVIDEOHDR lpVHdr)
{
	if (!hwnd) 
        return FALSE; 
	return (LRESULT) TRUE ; 
}

int VFWCamera::DlgVideoFormat()
{
	setDlgVideoFormat=1;
	//DisplayCamera(0);
	return 1;
	/*if (CapDrvCaps.fHasDlgVideoSource)		//In Win 2000 stream must stop
		{capDlgVideoSource(hWndC);	return 1;}
	else 							return 0;
	*/
}

int VFWCamera::DlgVideoSource()
{
	setDlgVideoSource=1;
	//DisplayCamera(0);
	return 1;
	/*if (CapDrvCaps.fHasDlgVideoSource)		
		{capDlgVideoSource(hWndC);	return 1;}
	else 							return 0;
	*/
}

int VFWCamera::DlgVideoDisplay()
{
	setDlgVideoDisplay=1;
	DisplayCamera(0);
	return 1;
	/*if (CapDrvCaps.fHasDlgVideoDisplay)		
		{capDlgVideoDisplay(hWndC);	return 1;}
	else 							return 0;
	*/
}

int VFWCamera::DlgVideoCompression()
{
	setDlgVideoCompression=1;
	DisplayCamera(0);
	return 1;
	/*capDlgVideoCompression(hWndC);
	return 1;
	*/
}

int VFWCamera::SetandDisplayVideoFormat(int format)
{	//format 2:QCIF 3:CIF
	capCaptureStop(hWndC);
	switch (format)//format 2:QCIF 3:CIF
	{
		case 2:	
			buffWidth = 176;
			buffHeight= 144;
			break;
		case 3:	
			buffWidth = 352;
			buffHeight= 288;
			break;
		default:
			return 0;
	}
	return DisplayCamera(CameraIndex);
	//return 1;
}

int VFWCamera::getCameraIndex()
{
	return CameraIndex;
}

int VFWCamera::ShowCameraVideo(unsigned char *buff)
{
	//BITMAPINFOHEADER bmih;
	//bmih.biSize		= sizeof(BITMAPINFOHEADER);
	//bmih.biWidth	= displayWidth;
	//bmih.biHeight	= displayHeight;
	//bmih.biPlanes	= 1;	
	//bmih.biBitCount = 24;
	//bmih.biCompression	= BI_RGB;
	//bmih.biSizeImage	= displayWidth*displayHeight*3;
	//bmih.biXPelsPerMeter= 0;
	//bmih.biYPelsPerMeter= 0;
	//bmih.biClrUsed = 0;
	//bmih.biClrImportant = 0;
	//if(hWnd_out)
	//	StretchDIBits( ::GetWindowDC(hWnd_out) ,0,400,176,144,0,0,176,144,lpVHdr->lpData,(BITMAPINFO *)&bmih,DIB_RGB_COLORS,SRCCOPY);

	//PAINTSTRUCT ps;
	//HDC chdc;
	//chdc = CreateCompatibleDC(::GetWindowDC(hWnd_out));
	//BeginPaint(hWnd_out,&ps);
	//SetStretchBltMode(::GetWindowDC(hWnd_out), COLORONCOLOR);
	//CreateCompatibleBitmap(::GetWindowDC(hWnd_out),352,288);
	//SelectObject(chdc,CreateCompatibleBitmap(::GetWindowDC(hWnd_out),352,288));
	//StretchDIBits(chdc,0,0,352,288,0,0,352,288,lpVHdr->lpData,(BITMAPINFO *)&bmih,DIB_RGB_COLORS,SRCCOPY);
	//StretchBlt(::GetWindowDC(hWnd_out),0,0,176,144,chdc,0,0,352,288,COLORONCOLOR|SRCCOPY); 
	//BitBlt(::GetWindowDC(hWnd_out),0,0,176,144,chdc,0,0,SRCCOPY);
	//EndPaint(hWnd_out,&ps);
 	//DeleteDC(chdc);
	
	/*unsigned char buff[176*144*3];
	for(int j=0;j<288;j+=2)
		for(int i=0;i<352;i+=2){
			buff[(j>>1)*176+(i>>1)] = (unsigned char)lpVHdr->lpData[j*352+i] ;
			buff[176*144+(j>>1)*176+(i>>1)] = (unsigned char)lpVHdr->lpData[352*288+j*352+i] ;
			buff[176*144*2+(j>>1)*176+(i>>1)] = (unsigned char)lpVHdr->lpData[352*288*2+j*352+i] ;
		}
	*/
 return 0;
}


int VFWCamera::SetCameraVision(int param)
{
	camera_vision = param;
	DisplayCamera(0);
	return 1;
}

