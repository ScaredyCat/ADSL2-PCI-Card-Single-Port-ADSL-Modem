// VFWCamera.h: interface for the VFWCamera class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_VFWCAMERA_H__D9F38A5F_09E7_40E2_AC0F_0180DC6DC99A__INCLUDED_)
#define AFX_VFWCAMERA_H__D9F38A5F_09E7_40E2_AC0F_0180DC6DC99A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include <vfw.h>
			
typedef void (*vfw_cb)(unsigned char *,long);
								      
class VFWCamera  
{
public:
	BOOL GetCamNameByIndex(int idx,char *name,char *ver);// added by sjhuang 2006/03/08
	int SetCameraVision(int param);
	int SetandDisplayVideoFormat(int format);
	int SetFramesPerSec(int framespersec);
	int DlgVideoCompression();
	int DlgVideoSource();
	int DlgVideoDisplay();
	int DlgVideoFormat();
	int DisplayCamera(int UseCameraIndex);
	int GetCamAmount();
	int getCameraIndex();
	int GetFramesPerSec();
	void SetDisplayPos(int Xpos,int Ypos);
	void SetVFW_CB_data( vfw_cb );
	VFWCamera(HWND hWndC);
	virtual ~VFWCamera();

	void OnCapformat();
	CString m_VideoName; // sjhuang 2006/03/08
private:
	static LRESULT CALLBACK VideoPreviewCallback(HWND hwnd, LPVIDEOHDR lpVHdr);
	static LRESULT CALLBACK VideoStreamCallback(HWND hwnd, LPVIDEOHDR lpVHdr);
	static LRESULT CALLBACK AudioStreamCallback(HWND hwnd, LPVIDEOHDR lpVHdr);
	int ShowCameraVideo(unsigned char *buff);
	int flagCamera;
	BITMAPINFO bmpin;
	BITMAPINFO * lpbi;
	CAPTUREPARMS CaptureParms;
	CAPSTATUS CapStatus;
	CAPDRIVERCAPS CapDrvCaps;
	int CameraIndex;		//Use the default Camera Number
	
	void FreeAllocateRC();
	
protected:
	HWND hWnd;
	HWND hWndC;
	int framerate;
	int displayXpos;
	int displayYpos;
	int displayWidth;
	int displayHeight;
	int buffWidth;
	int buffHeight;
	int setDlgVideoFormat;
	int setDlgVideoSource;
	int setDlgVideoDisplay;
	int setDlgVideoCompression;
	int camera_vision; //1:On 0:Off
	
};

#endif // !defined(AFX_VFWCAMERA_H__D9F38A5F_09E7_40E2_AC0F_0180DC6DC99A__INCLUDED_)
