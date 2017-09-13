#include "DirectShowCapture.h"

/*#define DS_DEFAULT_CAM_WIDTH	320
#define DS_DEFAULT_CAM_HEIGHT	240*/

HANDLE hCapSemaphore;
HDC hdcCapDisplay;
int nCaptureWidth, nCaptureHeight;
int nDisplayWidth, nDisplayHeight;
BITMAPINFO bmiCapDisplay;

// -----------------------------------------------------------------------------
TDSCapture:: TDSCapture(HDC hdc, int capture_width, int capture_height, \
						int display_width, int display_height)
{	// Construct
	CoInitialize(NULL);	 // Initialize COM
	hCapSemaphore = CreateSemaphore(NULL, 0, 255, "capSemaphore");  // Create Semaphore
	flagRenderCamera = false;
	// Win32 API display initialization
	ZeroMemory(&bmiCapDisplay.bmiHeader,sizeof(bmiCapDisplay.bmiHeader));
	bmiCapDisplay.bmiHeader.biSize=sizeof(bmiCapDisplay.bmiHeader);
	bmiCapDisplay.bmiHeader.biWidth = 320;	// would be changed after capturing.
	bmiCapDisplay.bmiHeader.biHeight = 240;
	bmiCapDisplay.bmiHeader.biPlanes=1;
	bmiCapDisplay.bmiHeader.biBitCount=24;
	bmiCapDisplay.bmiHeader.biCompression=BI_RGB;
	hdcCapDisplay = hdc;
	nDisplayWidth = display_width;
	nDisplayHeight = display_height;
	nCaptureWidth = capture_width;
	nCaptureHeight = capture_height;
}
// -----------------------------------------------------------------------------
TDSCapture:: ~TDSCapture()
{	// De-Construct
	
	if( hCapSemaphore ) // added by sjhuang
		CloseHandle(hCapSemaphore);		// Release Semaphore
	Release();		  // Release other allocated DS variables
	CoUninitialize();	// Uninitialize COM
	
	
}
// -----------------------------------------------------------------------------
void TDSCapture:: Release()
{	// Release allocated DS variables
	if (flagRenderCamera)
	{	
		pGraph->Release();      pGraph = NULL;
		pBuild->Release();      pBuild = NULL;
		pSG->Release();         pSG = NULL;
		pSG_Filter->Release();  pSG_Filter = NULL;
		pNull->Release();       pNull = NULL;
		pControl->Release();    pControl = NULL;
		pCap->Release();        pCap = NULL;
		flagRenderCamera = false;
	}
}

// =============================================================================
// MEDIA CONTROL FUNCTIONS
// =============================================================================
bool TDSCapture:: Run()
{	if (pControl)
	{	if (IsRunning() || SUCCEEDED(pControl->Run()))
			return true;
	}
	return false;
}

// -----------------------------------------------------------------------------
bool TDSCapture:: Stop()
{	if (pControl)
	{	if (IsStopped() || SUCCEEDED(pControl->Stop()))
			return true;
	}
	return false;
}

// -----------------------------------------------------------------------------
bool TDSCapture:: Pause()
{	if (pControl)
	{	if (IsPaused() || SUCCEEDED(pControl->Pause()))
			return true;
	}
	return false;
}

// -----------------------------------------------------------------------------
bool TDSCapture:: IsRunning()
{	OAFilterState state = State_Stopped;
	if (pControl && SUCCEEDED(pControl->GetState(10, &state)))
		return state == State_Running;
	return false;
}

// -----------------------------------------------------------------------------
bool TDSCapture:: IsStopped()
{	OAFilterState state = State_Stopped;
	if (pControl && SUCCEEDED(pControl->GetState(10, &state)))
		return state == State_Stopped;
	return false;
}

// -----------------------------------------------------------------------------
bool TDSCapture:: IsPaused()
{	OAFilterState state = State_Stopped;
	if (pControl && SUCCEEDED(pControl->GetState(10, &state)))
		return state == State_Paused;
	return false;
}

// =============================================================================
// MY FUNCTIONS
// =============================================================================
BYTE* TDSCapture:: GetImageBuffer()
{	return g_StillCapCB.bufImage;
}
int TDSCapture:: GetWidth()
{	return nWidth;
}
int TDSCapture:: GetHeight()
{	return nHeight;
}

// =============================================================================
// SampleGrabberCallback FUNCTIONS
// =============================================================================
STDMETHODIMP SampleGrabberCallback:: QueryInterface(REFIID riid, void **ppvObject)
{	if (NULL == ppvObject)
		return E_POINTER;
	if (riid == IID_IUnknown)
	{	*ppvObject = static_cast<IUnknown*>(this);
		return S_OK;
	}
	if (riid == IID_ISampleGrabberCB)
	{	*ppvObject = static_cast<ISampleGrabberCB*>(this);
		return S_OK;
	}
	return E_NOTIMPL;
}

// -----------------------------------------------------------------------------
STDMETHODIMP SampleGrabberCallback:: SampleCB(double Time, IMediaSample *pSample)
{	return E_NOTIMPL;
}

// -----------------------------------------------------------------------------
STDMETHODIMP SampleGrabberCallback:: BufferCB(double Time, BYTE *pBuffer, long BufLen)
{	bufImage = pBuffer;
	// Display

	SetStretchBltMode(hdcCapDisplay,STRETCH_HALFTONE); // sjhuang, 2006/03/06

	StretchDIBits(hdcCapDisplay, 0, 0, nDisplayWidth, nDisplayHeight, 0, 0,\
			bmiCapDisplay.bmiHeader.biWidth, bmiCapDisplay.bmiHeader.biHeight, \
			pBuffer, &bmiCapDisplay, DIB_RGB_COLORS, SRCCOPY);

	// Realse semaphore
	ReleaseSemaphore(hCapSemaphore, 1, NULL);
	return S_OK;
}

// =============================================================================
// COMMON PRIVATE FUNCTIONS
// =============================================================================
HRESULT TDSCapture:: InitCaptureGraphBuilder(IGraphBuilder **ppGraph,
											 ICaptureGraphBuilder2 **ppBuild)
{   // CaptureGraphBuilder initialization
	if (!ppGraph || !ppBuild)
		return E_POINTER;
	
	IGraphBuilder *pGraph = NULL;
	ICaptureGraphBuilder2 *pBuild = NULL;

	// Step1: Create a 'Capture Graph Builder'.
	if ( FAILED( CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER,
								  IID_ICaptureGraphBuilder2, (void**)&pBuild) ))
		return E_FAIL;

	// Step2: Create a 'Filter Graph Manager' if succeeded.
	if ( FAILED( CoCreateInstance(CLSID_FilterGraph, 0, CLSCTX_INPROC_SERVER,
								  IID_IGraphBuilder, (void**)&pGraph) ))
	{	pBuild->Release();
		return E_FAIL;
	}

	// Step3: Set the manager targer of 'pBuilder' to 'Filter Graph Manager'.
	pBuild->SetFiltergraph(pGraph);
	*ppBuild = pBuild;
	*ppGraph = pGraph;
	return S_OK;
}

// -----------------------------------------------------------------------------
HRESULT TDSCapture:: FindPin(IBaseFilter* baseFilter, PIN_DIRECTION direction,
							 int pinNumber, IPin** destPin)
{   CComPtr<IEnumPins> enumPins;
	*destPin = NULL;
	if ( SUCCEEDED(baseFilter->EnumPins(&enumPins)))
	{   ULONG numFound;
		IPin* tmpPin;
		while ( SUCCEEDED(enumPins->Next(1, &tmpPin, &numFound)))
		{   PIN_DIRECTION pinDirection;
			tmpPin->QueryDirection(&pinDirection);
			if (pinDirection == direction)
			{   if (pinNumber == 0)
				{   *destPin = tmpPin;  // Return the pin's interface
					break;
				}
				pinNumber--;
			}
			tmpPin->Release();
		}
		return S_OK;
	}
	else
		return E_FAIL;
}

// -----------------------------------------------------------------------------
HRESULT TDSCapture:: ConnectPins(IBaseFilter* opFilter, unsigned int opNum,
								 IBaseFilter* ipFilter, unsigned int ipNum)
{   CComPtr<IPin> inputPin;
	CComPtr<IPin> outputPin;
	if (!opFilter || !ipFilter)
		return E_FAIL;
	FindPin(opFilter, PINDIR_OUTPUT, opNum, &outputPin);
	FindPin(ipFilter, PINDIR_INPUT, ipNum, &inputPin);

	

	if (inputPin && outputPin)
		return SUCCEEDED(pGraph->Connect(outputPin, inputPin));
	else
		return E_FAIL;
}

// -----------------------------------------------------------------------------
void TDSCapture:: DeleteMediaType(AM_MEDIA_TYPE *mt)
{	// Release AM_MEDIA_TYPE
	if (mt->cbFormat != 0)
	{	CoTaskMemFree((PVOID)mt->pbFormat);
		mt->cbFormat = 0;
		mt->pbFormat = NULL;
	}
	if (mt->pUnk != NULL)
	{	// Unnecessary because pUnk should not be used, but safest.
		mt->pUnk->Release();
		mt->pUnk = NULL;
	}
}

// =============================================================================
// CAMERA FUNCTIONS
// =============================================================================
bool TDSCapture:: RenderCamera(int idCamera=0)
{
	Release();

	// Step1: Initialize Graph Manager & Build help object.
	if ( FAILED( InitCaptureGraphBuilder(&pGraph, &pBuild) ))
		return false;

	// Step2: Get the wanna capture device and add it into filter graph.
	if ( FAILED( EnumerateVideoInputDevice(&pCap, idCamera) ))
		return false;
	pGraph->AddFilter(pCap, L"Capture Filter");

	// Step3: Create a 'Sample Grabber Filter' in filter graph to capture frames.
	if ( FAILED( CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER,
								  IID_IBaseFilter, (void**)&pSG_Filter) ))
		return false;
	pGraph->AddFilter(pSG_Filter, L"SampleGrab");

	// Step4: Create a 'Null Render Filter' to lead out video streams.
	if ( FAILED( CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER,
								  IID_IBaseFilter, (void**)&pNull) ))
		return false;
	pGraph->AddFilter(pNull, L"NullRender");

	// Step5: Set 'Sample Grabber' to save the sample buffer
	//		Use BufferCB method(1), [0]SampleCB callback method; [1]BufferCB method.
	pSG_Filter->QueryInterface(IID_ISampleGrabber, (void**)&pSG);
	pSG->SetOneShot(FALSE);	   // [Attrib] stop graph when receive a sample?
	pSG->SetBufferSamples(TRUE);  // [Attrib] put sample into internal buffer?
	pSG->SetCallback(&g_StillCapCB, 1);

	// Step6: Use 'Preview Pin' to output data since current camera has no
	//		'Still Capture' (PIN_CATEGORY_STILL) function.
	hr = pBuild->RenderStream(
				&PIN_CATEGORY_PREVIEW,	// Connect this pin ...
				&MEDIATYPE_Video,		// with this media type ...
				pCap,					// on this filter ...
				pSG_Filter,				// to the Sample Grabber ...
				pNull);					// ... and finally to the Null Render.
   	if( FAILED(hr) )
	{	
		//MessageBox(NULL, "RenderStream Error", "RenderStream Error", MB_OK);
		MessageBox(NULL, "Detect Camera Error, please try again...", "Detect Camera Error, please try again...", MB_OK);
		return false;
	}

	// Step7: Initialize camera format with specified width & height.
	//InitializeCameraFormat(DS_DEFAULT_CAM_WIDTH, DS_DEFAULT_CAM_HEIGHT);
	InitializeCameraFormat(nCaptureWidth, nCaptureHeight);
	
	// Step8: Create media control and run it.
	hr = pGraph->QueryInterface(IID_IMediaControl, (void **)&pControl);
	flagRenderCamera = true;
	Run();

	return true;
}

// -----------------------------------------------------------------------------
HRESULT TDSCapture:: EnumerateVideoInputDevice(IBaseFilter **pCap, int idDevice=0)
{	// Enumerate video capture device(s) and return the pointer of the first one device.
	ICreateDevEnum *pDevEnum = NULL;
	IEnumMoniker *pEnum = NULL;
	IMoniker *pMoniker = NULL;
	IPropertyBag *pPropBag;
	VARIANT var;
	VariantInit(&var);	

	// Step1: Create 'System Device Enumerator' to get the set of H/W Monikers.
	if ( FAILED( CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
							IID_ICreateDevEnum, reinterpret_cast<void**>(&pDevEnum)) ))
		return E_FAIL;

	// Step2: Create video capture Enumerator Class.
	if ( FAILED( pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0)) ||
         !pEnum )
	{   pDevEnum->Release();
		return E_FAIL;
	}

	// Step3: Get H/W moniker object and info using the Next method of Enumerator.
	//		Noted that the last device will be used if the wanna index is infeasible.
	int ctr = 0;
	while (pEnum->Next(1, &pMoniker, NULL) == S_OK)
	{	if ( FAILED( pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)(&pPropBag)) ))
		{	pMoniker->Release();
			continue;		// Ignore this device and try next one.
		} 
		// Step4: Read the name of the H/W.
		if ( SUCCEEDED( pPropBag->Read(L"FriendlyName", &var, 0) ))
		{	// Transfer strings from WideChar to CHAR (for UI purpose)
			char name[999];
			WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, name, MAX_PATH, NULL, NULL);
			VariantClear(&var);
			pMoniker->BindToObject (0, 0, IID_IBaseFilter, (void**)pCap); 
		}
		pPropBag->Release();
		pMoniker->Release();
		
		if (ctr++ == idDevice)
			break;		// Use the wanna device.
	}
	pEnum->Release();
	pDevEnum->Release();
	return S_OK;
}

// =============================================================================
// CAMERA CONTROL FUNCTIONS
// =============================================================================
bool TDSCapture:: SetVideoFormat()
{  // Set video format with dialogs.
	CAUUID cauuid;
	ISpecifyPropertyPages *pSpec;
	IAMStreamConfig *pSC;
	bool flagRunning = false;

	if (!flagRenderCamera)
		return false;
	if ( IsRunning() )
	{	Stop();
		flagRunning = true;
	}

	// Step1: Get the Media Stream config interface.
	if ( FAILED( pBuild->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video,
									   pCap, IID_IAMStreamConfig, (void **)&pSC) ))
	{	if (flagRunning)	// Reset status
			Run();
		return false;
	}

	// Step2: Set video format by property page.
	if ( FAILED( pSC->QueryInterface(IID_ISpecifyPropertyPages,	(void **)&pSpec) ))
	{	if (flagRunning)	// Reset status
			Run();
		pSC->Release();
		return false;
	}
	if ( FAILED( pSpec->GetPages(&cauuid) ))
	{   if (flagRunning)	// Reset status
			Run();
		pSpec->Release();
		pSC->Release();
		return false;
	}
	OleCreatePropertyFrame(NULL, 30, 30, NULL, 1, (IUnknown **)&pSC,
						   cauuid.cElems, (GUID *)cauuid.pElems, 0, 0, NULL);
	CoTaskMemFree(cauuid.pElems);	// Release the memory

	// Step3: Get video information (width/height).
	AM_MEDIA_TYPE *pmt;
	hr = pSC->GetFormat(&pmt);
	VIDEOINFOHEADER *vih = reinterpret_cast<VIDEOINFOHEADER*>(pmt->pbFormat);
	nWidth = vih->bmiHeader.biWidth;
	nHeight = vih->bmiHeader.biHeight;
	DeleteMediaType(pmt);
	// ----------------------------------------
	bmiCapDisplay.bmiHeader.biWidth = nWidth;
	bmiCapDisplay.bmiHeader.biHeight = nHeight;

	pSpec->Release();
	pSC->Release();
	Run();
	return true;
}

// -----------------------------------------------------------------------------
bool TDSCapture:: SetVideoSource()
{   // Set video source with dialogs.
	CAUUID cauuid;
	ISpecifyPropertyPages *pSpec;
	bool flagRunning = false;

	if (!flagRenderCamera)
		return false;
	if ( IsRunning() )
	{	Stop();
		flagRunning = true;
	}
	// Set video source by property page
	if ( FAILED( pCap->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pSpec) ))
	{	if (flagRunning)	// Reset status
			Run();
		return false;
	}
		
	if ( FAILED( pSpec->GetPages(&cauuid) ))
	if ( FAILED( pCap->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pSpec) ))
	{	if (flagRunning)	// Reset status
			Run();
		pSpec->Release();
		return false;
	}
	OleCreatePropertyFrame(NULL, 30, 30, NULL, 1, (IUnknown **)&pCap,
						   cauuid.cElems, (GUID *)cauuid.pElems, 0, 0, NULL);
	CoTaskMemFree(cauuid.pElems);	// Release the memory
	pSpec->Release();
	Run();
	return true;
}

// -----------------------------------------------------------------------------
bool TDSCapture:: InitializeCameraFormat(int width, int height)
{	// Set video format without dialogs.
	// This function would be changed in the future according to requirements.
	IAMStreamConfig *pSC;

	// Step1: Get the Media Stream config interface.
	if ( FAILED( pBuild->FindInterface(&PIN_CATEGORY_CAPTURE, \
				 &MEDIATYPE_Video, pCap, IID_IAMStreamConfig, (void **)&pSC) ))
		return false;

	// Step2: Get video information.
	AM_MEDIA_TYPE *pmt;
	hr = pSC->GetFormat(&pmt);

	// Step3: Set wanna video format
	VIDEOINFOHEADER *vih = reinterpret_cast<VIDEOINFOHEADER*>(pmt->pbFormat);
	bool flagSuccess = true;
    int orgWidth = vih->bmiHeader.biWidth;
    int orgHeight = vih->bmiHeader.biHeight;
	vih->bmiHeader.biWidth = width;
	vih->bmiHeader.biHeight = height;
	vih->bmiHeader.biCompression = BI_RGB;
	vih->bmiHeader.biPlanes = 1;
	vih->bmiHeader.biBitCount = 24;
	if ( FAILED( pSC->SetFormat(pmt) ))
	{	bmiCapDisplay.bmiHeader.biWidth = nWidth = orgWidth;
		bmiCapDisplay.bmiHeader.biHeight = nHeight = orgHeight;
		flagSuccess = false;
	}
    else
    {   bmiCapDisplay.bmiHeader.biWidth = nWidth = width;
		bmiCapDisplay.bmiHeader.biHeight = nHeight = height;
    }
	DeleteMediaType(pmt);
	pSC->Release();
	return flagSuccess;	
}

///////////////////////////////////////////////////////////////////////////////
// added by sjhuang  2006/03/03
//
int TDSCapture:: EnumDevices(char *buf)
{	// Enumerate video capture device(s) and return the pointer of the first one device.
	ICreateDevEnum *pDevEnum = NULL;
	IEnumMoniker *pEnum = NULL;
	IMoniker *pMoniker = NULL;
	IPropertyBag *pPropBag;
	VARIANT var;
	VariantInit(&var);	

	// Step1: Create 'System Device Enumerator' to get the set of H/W Monikers.
	if ( FAILED( CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
							IID_ICreateDevEnum, reinterpret_cast<void**>(&pDevEnum)) ))
		return -1;

	// Step2: Create video capture Enumerator Class.
	if ( FAILED( pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0)) ||
         !pEnum )
	{   pDevEnum->Release();
		return -1;
	}

	// Step3: Get H/W moniker object and info using the Next method of Enumerator.
	//		Noted that the last device will be used if the wanna index is infeasible.
	int ctr = 0;
	while (pEnum->Next(1, &pMoniker, NULL) == S_OK)
	{	if ( FAILED( pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)(&pPropBag)) ))
		{	pMoniker->Release();
			continue;		// Ignore this device and try next one.
		} 
		// Step4: Read the name of the H/W.
		if ( SUCCEEDED( pPropBag->Read(L"FriendlyName", &var, 0) ))
		{	// Transfer strings from WideChar to CHAR (for UI purpose)
			char name[999];
			WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, name, MAX_PATH, NULL, NULL);
			VariantClear(&var);
			//strcpy(buf,name);
//			pMoniker->BindToObject (0, 0, IID_IBaseFilter, (void**)pCap); 
		}
		pPropBag->Release();
		pMoniker->Release();
		
//		if (ctr++ == idDevice)
//			break;		// Use the wanna device.
	}
	pEnum->Release();
	pDevEnum->Release();
	return 1;
}