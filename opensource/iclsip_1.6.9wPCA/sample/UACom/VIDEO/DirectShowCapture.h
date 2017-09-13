/* ---------------------------------------------------------------------
DirectShow Video Capture Class (CAMERA VERSION WITH AUTO DISPLAY)

Author    : Kual-Zheng Lee (improved by Joe)
Created   : 2006/01/18
Copyright : N100/ICL/ITRI
Exmaple:
	TDSCapture *dsCap = new TDSCapture(HDC, hdc_width, hdc_height);
	dsCap->RenderCamera(0);		// given a camera index, 0 as default.
	while( true )
	{	WaitForSingleObject(hCapSemaphore, INFINITE);
		BYTE *ptr = dsCap->GetImageBuffer();
		...
	}
	dsCap->Stop();
	// More functions
	dsCap->GetWidth()
	dsCap->GetHeight()
	dsCap->SetVideoSource();
	dsCap->SetVideoFormat();
----------------------------------------------------------------------*/
#ifndef __DirectShowCapture_H__
#define __DirectShowCapture_H__

#include <atlbase.h>         // for ATL font translation
#include <Dshow.h>           // for DirectShow header file
#include <Qedit.h>           // for DirectShow interface definition

#pragma comment( lib, "strmiids.lib" )     // for DirectShow library

extern HANDLE hCapSemaphore;

// ---------------------------------------------------------------------
class SampleGrabberCallback : public ISampleGrabberCB
{
private:
    // Fake referance counting.
    STDMETHODIMP_(ULONG) AddRef() { return 1; }
    STDMETHODIMP_(ULONG) Release() { return 2; }
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject);
    STDMETHODIMP SampleCB(double Time, IMediaSample *pSample);
    STDMETHODIMP BufferCB(double Time, BYTE *pBuffer, long BufferLen);
public:
	BYTE *bufImage;		// Pointer to the raw image, for outside usage.
};

// ---------------------------------------------------------------------
class TDSCapture
{
public:
	// added by sjhuang 2006/03/03
	int  EnumDevices(char *buf);

private:
	

	// FUNCTIONS
	HRESULT InitCaptureGraphBuilder(IGraphBuilder **ppGraph,  // Receives the pointers.
                                    ICaptureGraphBuilder2 **ppBuild);
	HRESULT FindPin(IBaseFilter* baseFilter, PIN_DIRECTION direction,
					int pinNumber, IPin** destPin);
	HRESULT ConnectPins(IBaseFilter* opFilter, unsigned int opNum,
						IBaseFilter* ipFilter, unsigned int ipNum);
	HRESULT EnumerateVideoInputDevice(IBaseFilter **pCap, int idDevice);
	bool InitializeCameraFormat(int width, int height);
	void Release();
	void DeleteMediaType(AM_MEDIA_TYPE *mt);
    //bool IsRunning();
    bool IsStopped();
    bool IsPaused();

	// DIRECTSHOW VARIABLES
	IGraphBuilder *pGraph;			// Graph: to overall filter graph
	ICaptureGraphBuilder2 *pBuild;	// Capture Graph Builder: a helper object
	ISampleGrabber *pSG;			// Sample Grabber
	IBaseFilter *pCap;				// Capture Filter: to camera device
	IBaseFilter *pSG_Filter;
	IBaseFilter *pNull;
	IMediaControl *pControl;		// Graph Controler : display functions
	AM_MEDIA_TYPE g_StillMediaType;	// Save the media information
	SampleGrabberCallback g_StillCapCB;		// The object for capturing frames

	// OTHER VARIABLES
	HRESULT hr;
	int nWidth, nHeight;
    bool flagRenderCamera;



public:
	bool IsRunning();

    // CAMERA METHODS
	bool RenderCamera(int idCamera);
	bool SetVideoFormat();
	bool SetVideoSource();

    // COMMON METHODS
	BYTE *GetImageBuffer();
	int GetWidth();
	int GetHeight();
    bool Run();
    bool Stop();
    bool Pause();

	// CONSTRUCT & DE-CONSTRUCT
	TDSCapture(HDC hdc, int capture_width, int capture_height, \
				int display_width, int display_height );
	~TDSCapture();
};

#endif
