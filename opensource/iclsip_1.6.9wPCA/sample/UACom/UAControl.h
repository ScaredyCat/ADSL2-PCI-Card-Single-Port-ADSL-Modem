// UAControl.h : Declaration of the CUAControl

#ifndef __UACONTROL_H_
#define __UACONTROL_H_

#include "resource.h"       // main symbols
#include <atlctl.h>

#ifdef _FOR_VIDEO
#include "video\VFWCamera.h"
#endif

class CUAProfile;
class CMediaManager;
class CCallManager;


#ifdef _simple
class CPresManager;
#endif

#ifdef _FOR_VIDEO
class CVideoManager;
#endif

#include "UACom.h"
#include "UAComCP.h"

/////////////////////////////////////////////////////////////////////////////
// CUAControl
class ATL_NO_VTABLE CUAControl : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<IUAControl, &IID_IUAControl, &LIBID_UACOMLib>,
	public CComControl<CUAControl>,
	public IPersistStreamInitImpl<CUAControl>,
	public IOleControlImpl<CUAControl>,
	public IOleObjectImpl<CUAControl>,
	public IOleInPlaceActiveObjectImpl<CUAControl>,
	public IViewObjectExImpl<CUAControl>,
	public IOleInPlaceObjectWindowlessImpl<CUAControl>,
	public IConnectionPointContainerImpl<CUAControl>,
	public IPersistStorageImpl<CUAControl>,
	public ISpecifyPropertyPagesImpl<CUAControl>,
	public IQuickActivateImpl<CUAControl>,
	public IDataObjectImpl<CUAControl>,
	public IProvideClassInfo2Impl<&CLSID_UAControl, &DIID__IUAControlEvents, &LIBID_UACOMLib>,
	public IPropertyNotifySinkCP<CUAControl>,
	public CComCoClass<CUAControl, &CLSID_UAControl>,
	public CProxy_IUAControlEvents< CUAControl >,
	public IObjectWithSiteImpl<CUAControl>,
	public IObjectSafetyImpl<CUAControl, INTERFACESAFE_FOR_UNTRUSTED_CALLER>
{
	friend CCallManager;

#ifdef _FOR_VIDEO
	friend CVideoManager;
#endif

public:
	CUAControl();
	~CUAControl();

DECLARE_REGISTRY_RESOURCEID(IDR_UACONTROL)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CUAControl)
	COM_INTERFACE_ENTRY(IUAControl)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IViewObjectEx)
	COM_INTERFACE_ENTRY(IViewObject2)
	COM_INTERFACE_ENTRY(IViewObject)
	COM_INTERFACE_ENTRY(IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY(IOleInPlaceObject)
	COM_INTERFACE_ENTRY2(IOleWindow, IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY(IOleInPlaceActiveObject)
	COM_INTERFACE_ENTRY(IOleControl)
	COM_INTERFACE_ENTRY(IOleObject)
	COM_INTERFACE_ENTRY(IPersistStreamInit)
	COM_INTERFACE_ENTRY2(IPersist, IPersistStreamInit)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(ISpecifyPropertyPages)
	COM_INTERFACE_ENTRY(IQuickActivate)
	COM_INTERFACE_ENTRY(IPersistStorage)
	COM_INTERFACE_ENTRY(IDataObject)
	COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(IObjectSafety)
END_COM_MAP()

BEGIN_PROP_MAP(CUAControl)
	PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
	PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()

BEGIN_CONNECTION_POINT_MAP(CUAControl)
	CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
	CONNECTION_POINT_ENTRY(DIID__IUAControlEvents)
END_CONNECTION_POINT_MAP()

BEGIN_MSG_MAP(CUAControl)
	CHAIN_MSG_MAP(CComControl<CUAControl>)
	DEFAULT_REFLECTION_HANDLER()
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);



// IViewObjectEx
	DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

// IUAControl
public:
	STDMETHOD(PublishStatus)(/*[in]*/ BSTR szURI, /*[in]*/ BOOL bStatus);
	STDMETHOD(UnsubscribeResLst)(/*[in]*/ BSTR ResourceURI);
	STDMETHOD(UnsubscribePres)(/*[in]*/ BSTR buddyURI);
	STDMETHOD(UnsubscribeWinfo)(/*[in]*/ BSTR buddyURI);
	STDMETHOD(SubscribeResLst)(/*[in]*/ BSTR ResourceURI);
	STDMETHOD(SubscribePres)(/*[in]*/ BSTR buddyURI);
	STDMETHOD(SubscribeWinfo)(/*[in]*/ BSTR buddyURI);
	STDMETHOD(SendInfo)(/*[in]*/ BSTR bstrURL, /*[in]*/ BSTR bstrContentType, /*[in]*/ BSTR bstrBody);
	STDMETHOD(UnauthorizeBuddy)(/*[in]*/ BSTR buddyURI);
	STDMETHOD(AuthorizeBuddy)(/*[in]*/ BSTR buddyURI);
	STDMETHOD(IsBuddyAuthorized)(/*[in]*/ BSTR buddyURI, /*[out, retval]*/ BOOL* retval);
	STDMETHOD(IsBuddyBlock)(/*[in]*/ BSTR buddyURI, /*[out, retval]*/ BOOL* retval);
	STDMETHOD(StartRemoteVideoEx)(/*[in]*/ int nhwnd, /*[in]*/ int x, /*[in]*/ int y, /*[in]*/ int scale);
	STDMETHOD(StartLocalVideoEx)(/*[in]*/ int nhwnd, /*[in]*/ int x, /*[in]*/ int y, /*[in]*/ int scale);
	STDMETHOD(UnsubscribeBuddy)(/*[in]*/ BSTR buddyURI);
	STDMETHOD(SubscribeBuddy)(/*[in]*/ BSTR buddyURI);
	STDMETHOD(GetBuddyBasicStatus)(/*[in]*/ BSTR buddyURI, /*[out, retval]*/ int* BasicStatus);
	STDMETHOD(GetBuddyOUTSubState)(/*[IN]*/ BSTR buddyURI, /*[out, retval]*/ int* SubState);
	STDMETHOD(GetBuddyINSubState)(/*[IN]*/ BSTR buddyURI, /*[out,retval]*/ int* SubState);
	STDMETHOD(UpdateBasicStatus)(/*[IN]*/ BOOL bOpen);
	STDMETHOD(GetBuddyCount)(/*[out, retval]*/ int* retval);
	STDMETHOD(GetBuddyURI)(/*[IN]*/ int number, /*[out, retval]*/ BSTR *retval);
	STDMETHOD(UnBlockBuddy)(/*[IN]*/ BSTR buddyURI);
	STDMETHOD(BlockBuddy)(/*[IN]*/ BSTR buddyURI);
	STDMETHOD(DelBuddy)(/*[IN]*/ BSTR buddyURI);
	STDMETHOD(AddBuddy)(/*[in]*/ BSTR buddyURI);
	STDMETHOD(SendText)(BSTR dialURL, BSTR tmpstr);
	STDMETHOD(StopRemoteVideo)();
	STDMETHOD(StartRemoteVideo)(int x, int y, int scale);
	// sam add
	STDMETHOD(GetRemotePartyDisplayName)(int dlgHandle, /*[out,retval]*/ BSTR* retval);
	STDMETHOD(GetMediaReceiveQuality)( /*[out,retval]*/ int* retval);

	STDMETHOD(GetVideoSize)(int *retval);
	STDMETHOD(GetVideoType)(int *retval);
	STDMETHOD(GetAudioType)(int *retval);
	STDMETHOD(Reset)();
	STDMETHOD(get_bSingleCall)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_bSingleCall)(/*[in]*/ BOOL newVal);
	STDMETHOD(SendDTMFP)();
	STDMETHOD(SendDTMFS)();
	STDMETHOD(SendDTMF9)();
	STDMETHOD(SendDTMF8)();
	STDMETHOD(SendDTMF7)();
	STDMETHOD(SendDTMF6)();
	STDMETHOD(SendDTMF5)();
	STDMETHOD(SendDTMF4)();
	STDMETHOD(SendDTMF3)();
	STDMETHOD(SendDTMF2)();
	STDMETHOD(SendDTMF1)();
	STDMETHOD(SendDTMF0)();
	STDMETHOD(get_LocalIP)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_LocalIP)(/*[in]*/ BSTR newVal);
	STDMETHOD(Stop)();
	STDMETHOD(SwitchLocalVideo)();
	STDMETHOD(RegQuery)();
	STDMETHOD(UnRegister)();
	STDMETHOD(Register)();
	STDMETHOD(StopLocalVideo)();
	STDMETHOD(StartLocalVideo)();
	STDMETHOD(GetRemoteParty)(int dlgHandle, /*[out,retval]*/ BSTR* retval);
	STDMETHOD(AttendXfer)(int dlgHandle1, int dlgHandle2);
	STDMETHOD(UnAttendXfer)(int dlgHandle, BSTR xferURL);
	STDMETHOD(UnHoldCall)(int dlgHandle);
	STDMETHOD(HoldCall)();
	static CUAControl* GetControl()
	{
		return s_pUAControl;
	}
	STDMETHOD(Test)(/*[in]*/ BSTR tmpStr);
	STDMETHOD(ShowPreferences)(/*[out,retval]*/ BOOL* pChanged);
	STDMETHOD(MakeCall)(BSTR dialURL, /*[out,retval]*/ int* retval);
	STDMETHOD(RejectCall)(int dlgHandle);
	STDMETHOD(AnswerCall)(int dlgHandle);
	STDMETHOD(DropCall)(int dlgHandle);
	STDMETHOD(CancelCall)();
	STDMETHOD(Initialize)(BSTR realm, BSTR userid, BSTR password);

	HRESULT OnDraw(ATL_DRAWINFO& di)
	{
		/*
		RECT& rc = *(RECT*)di.prcBounds;
		Rectangle(di.hdcDraw, rc.left, rc.top, rc.right, rc.bottom);

		SetTextAlign(di.hdcDraw, TA_CENTER|TA_BASELINE);
		LPCTSTR pszText = _T("ATL 3.0 : UAControl");
		TextOut(di.hdcDraw, 
			(rc.left + rc.right) / 2,  
			(rc.top + rc.bottom) / 2, 
			pszText, 
			lstrlen(pszText));
		*/
		return S_OK;
	}

	void Restart();
	void Redial();

	static CUAControl* s_pUAControl;
	CUAProfile*		m_pProfile;
	CMediaManager*	m_pMediaManager;
	CCallManager*	m_pCallManager;
	
#ifdef _simple
	CPresManager*	m_pPresManager;
#endif

#ifdef _FOR_VIDEO
	CVideoManager*	m_pVideoManager;
	VFWCamera		*vfwCam1;
#endif

	BOOL			m_bInitialized;
	BOOL			m_bShowLocalVideo;
	BOOL			m_bIPMonitor;
	CString			m_LocalIPAddr;
	CString			m_userid;
	CString			m_password;
	CString			m_realm;
	CString			m_LastDialURL;
};

#endif //__UACONTROL_H_
