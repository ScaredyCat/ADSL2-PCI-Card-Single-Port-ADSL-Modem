// UACDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PCAUA.h"
#include "UACDlg.h"
#include "PCAUADlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const IID IID_IUAControl = {0x4A91A134,0x1B60,0x497E,{0x87,0xEE,0x23,0xC9,0x7B,0x2A,0x6D,0x84}};
const LPCTSTR gstrUAComRegKey = "Software\\CCL, ITRI\\UACom\\";
CPCAUADlg* _pUIDlg = NULL;

extern void DrawBitmap(CDC* dc, HBITMAP hbmp, RECT r, BOOL Stretch);

/////////////////////////////////////////////////////////////////////////////
// CUACDlg dialog


CUACDlg* g_pdlgUAControl = NULL;


CUACDlg::CUACDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CUACDlg::IDD, pParent)
{
	g_pdlgUAControl = this;

	//{{AFX_DATA_INIT(CUACDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_regResult = 0;
	m_uacInitOk=FALSE;
	m_pwndUACtrl=NULL;
	m_pSplashDlg=NULL;
	m_bDragging=FALSE;
	m_bSnap = FALSE;

	m_nVideoScale = 1;

	m_bUseDebugMsg = 0;

	CIniFile m_ConfigFile(theApp.m_strAppPath+"config.ini");
	m_strUseDebugMsg = m_ConfigFile.ReadString("PCAUA", "DEBUG_MSG", "0");
	m_bUseDebugMsg = atoi(m_strUseDebugMsg);
}


void CUACDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CUACDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CUACDlg, CDialog)
	//{{AFX_MSG_MAP(CUACDlg)
	ON_WM_MOVING()
	ON_WM_DESTROY()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_ERASEBKGND()
	ON_WM_RBUTTONUP()
	ON_WM_CLOSE()
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUACDlg message handlers

BOOL CUACDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	if ( !(HBITMAP)m_bmpTitleBar)
		m_bmpTitleBar.LoadBitmap( IDB_VIDEO_TITLEBAR );
	if ( !m_dlgRemoteVideo)
		m_dlgRemoteVideo.Create( IDD_REMOTE_VIDEO);

	// TODO: Add extra initialization here
	m_uacInitOk = InitUAComObj();
	_pUIDlg=(CPCAUADlg*)m_pUIDlg;

	if (m_pwndUACtrl)
	{
		int nVideoSizeFormat = GetUAComRegDW( "VideoCodec", "VideoSizeFormat", 2);	// QCIF default
		AdjustVideoSize( nVideoSizeFormat, 1); // 100% size by default
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CUACDlg::InitUAComObj()
// initialize UACOM ActiveX Control
{
	if(m_pSplashDlg)
	{
		m_pSplashDlg->SetDlgItemText(IDC_SPLASH_MSG,"Initialize UAControl...");
		m_pSplashDlg->UpdateWindow();
	}

	m_bIPPortCheckOK = CheckSockPort(AfxGetApp()->GetProfileString( "", "PreferredLocalIP"));


	m_pwndUACtrl = GetDlgItem(IDC_UACONTROL1);
	if( m_pwndUACtrl )
	{
		HRESULT hr;
		CComPtr<IUnknown> pUACTL;
		
		try
		{
			pUACTL = m_pwndUACtrl->GetControlUnknown();

			/* Attach IDispatch interface to Dispatch driver */
			hr = pUACTL->QueryInterface( IID_IUAControl, (void**)&UAControlDriver.m_lpDispatch);
			SUCCEEDED(hr) ? 0 : throw hr;

			// NOTE: UACom often crash while uninitialize,
			// this will prevent it release
			//UAControlDriver.m_bAutoRelease = FALSE;
		}
		catch(...)
		{
			return FALSE;
		}

		RestartUACOM();
		
		m_strCallServerAddr = GetUAComRegString("CALL_Server","CALL_Server_Addr");
		m_dCallServerPort = GetUAComRegDW("CALL_Server","CALL_Server_Port", 5060);

		return TRUE;
	}
	return FALSE;

}

void CUACDlg::UAComRegister()
{
	UAControlDriver.Register();

	// load UACom's display name, registar address
	m_strRegistarName = GetUAComRegString("User_Info","Username");
	m_strRegistarAddress = GetUAComRegString("Registrar","Registrar_Addr");
	
	DWORD portNum = GetUAComRegDW("Registrar","Registrar_Port", 5060);
	if ( portNum != 5060)
	{
		CString strPort;
		strPort.Format( ":%d", portNum);
		m_strRegistarAddress += strPort;
	}

	return;
}


BEGIN_EVENTSINK_MAP(CUACDlg, CDialog)
    //{{AFX_EVENTSINK_MAP(CUACDlg)
	ON_EVENT(CUACDlg, IDC_UACONTROL1, 1 /* ShowText */, OnShowTextUacontrol1, VTS_BSTR)
	ON_EVENT(CUACDlg, IDC_UACONTROL1, 2 /* Ringback */, OnRingbackUacontrol1, VTS_I4)
	ON_EVENT(CUACDlg, IDC_UACONTROL1, 3 /* Alerting */, OnAlertingUacontrol1, VTS_I4)
	ON_EVENT(CUACDlg, IDC_UACONTROL1, 4 /* Proceeding */, OnProceedingUacontrol1, VTS_I4)
	ON_EVENT(CUACDlg, IDC_UACONTROL1, 5 /* Connected */, OnConnectedUacontrol1, VTS_I4)
	ON_EVENT(CUACDlg, IDC_UACONTROL1, 8 /* TimeOut */, OnTimeOutUacontrol1, VTS_I4)
	ON_EVENT(CUACDlg, IDC_UACONTROL1, 9 /* Dialing */, OnDialingUacontrol1, VTS_I4)
	ON_EVENT(CUACDlg, IDC_UACONTROL1, 10 /* Busy */, OnBusyUacontrol1, VTS_I4)
	ON_EVENT(CUACDlg, IDC_UACONTROL1, 11 /* Reject */, OnRejectUacontrol1, VTS_I4)
	ON_EVENT(CUACDlg, IDC_UACONTROL1, 12 /* Transferred */, OnTransferredUacontrol1, VTS_I4 VTS_BSTR)
	ON_EVENT(CUACDlg, IDC_UACONTROL1, 13 /* Held */, OnHeldUacontrol1, VTS_I4)
	ON_EVENT(CUACDlg, IDC_UACONTROL1, 14 /* RegistrationDone */, OnRegistrationDoneUacontrol1, VTS_NONE)
	ON_EVENT(CUACDlg, IDC_UACONTROL1, 15 /* RegistrationFail */, OnRegistrationFailUacontrol1, VTS_NONE)
	ON_EVENT(CUACDlg, IDC_UACONTROL1, 16 /* NeedAuthentication */, OnNeedAuthenticationUacontrol1, VTS_BSTR)
	ON_EVENT(CUACDlg, IDC_UACONTROL1, 17 /* Waiting */, OnWaitingUacontrol1, VTS_I4)
	ON_EVENT(CUACDlg, IDC_UACONTROL1, 18 /* RegistrationTimeout */, OnRegistrationTimeoutUacontrol1, VTS_NONE)
	ON_EVENT(CUACDlg, IDC_UACONTROL1, 6 /* Disconnected */, OnDisconnectedUacontrol1, VTS_I4 VTS_I4)
	ON_EVENT(CUACDlg, IDC_UACONTROL1, 30 /* ReceivedText */, OnReceivedTextUacontrol1, VTS_BSTR VTS_BSTR)
	ON_EVENT(CUACDlg, IDC_UACONTROL1, 31 /* InfoResponse */, OnInfoResponseUacontrol1, VTS_BSTR VTS_I4 VTS_BSTR VTS_BSTR)
	ON_EVENT(CUACDlg, IDC_UACONTROL1, 32 /* InfoRequest */, OnInfoRequestUacontrol1, VTS_BSTR VTS_BSTR VTS_BSTR VTS_PI4 VTS_PBSTR VTS_PBSTR)
	ON_EVENT(CUACDlg, IDC_UACONTROL1, 26 /* BuddyUpdateBasicStatus */, OnBuddyUpdateBasicStatusUacontrol1, VTS_BSTR VTS_I4 VTS_BSTR VTS_BSTR)
	ON_EVENT(CUACDlg, IDC_UACONTROL1, 33 /* NeedAuthorize */, OnNeedAuthorizeUacontrol1, VTS_BSTR)
	ON_EVENT(CUACDlg, IDC_UACONTROL1, 34 /* ReceivedOfflineMsg */, OnReceivedOfflineMsgUacontrol1, VTS_BSTR VTS_BSTR VTS_BSTR)
	ON_EVENT(CUACDlg, IDC_UACONTROL1, 35 /* RegisterExpired */, OnRegisterExpiredUacontrol1, VTS_NONE)
	//}}AFX_EVENTSINK_MAP
END_EVENTSINK_MAP()

void CUACDlg::OnShowTextUacontrol1(LPCTSTR strText) 
{
	TRACE("[ShowText]%s\r\n",	strText);

	if( m_bUseDebugMsg )
	{
		static FILE *ptr_error=NULL;
		CString tmp;
		tmp = theApp.m_strAppPath;
		tmp += "\\PCA_DebugMsg.log";

		if( ptr_error==NULL )
		{
			ptr_error = fopen(tmp,"w");
			fprintf(ptr_error,"start log \n");
			fclose(ptr_error);
		}
		else if( ptr_error )
		{
			ptr_error = fopen(tmp,"a");
			fprintf(ptr_error,"%s \n",strText);
			fclose(ptr_error);
		}
	}
}

void CUACDlg::OnBuddyUpdateStatusUaControl(LPCTSTR buddyURI, BOOL Open, LPCTSTR strMsgState, LPCTSTR strCallState)
{
	AfxTrace( "Event:: IUAControl::OnBuddyUpdateStatus(%s, msg:%d, reason:%s, call:%s)\n", buddyURI, Open, strMsgState, strCallState);
	AfxTrace( "Event:: IUAControl::ONBuddyUpdatestatus()~~\n" );
	TRACE0("Event:: IUAControl::ONBuddyUpdatestatus()~~\n");
	_pUIDlg->OnBuddyUpdateStatus(buddyURI, Open, strMsgState, strCallState);
}

void CUACDlg::OnRingbackUacontrol1(long dlgHandle) 
{
	AfxTrace( "Event:: IUAControl::OnRingback(%d)\n", dlgHandle);
	_pUIDlg->OnUARingback(dlgHandle);
}

void CUACDlg::OnAlertingUacontrol1(long dlgHandle) 
{
	AfxTrace( "Event:: IUAControl::OnAlerting(%d)\n", dlgHandle);
	_pUIDlg->OnUAAlerting(dlgHandle);	
}

void CUACDlg::OnProceedingUacontrol1(long dlgHandle) 
{
	AfxTrace( "Event:: IUAControl::OnProceeding(%d)\n", dlgHandle);
	_pUIDlg->OnUAProceeding(dlgHandle);
}

void CUACDlg::OnConnectedUacontrol1(long dlgHandle) 
{
	AfxTrace( "Event:: IUAControl::OnConnected(%d)\n", dlgHandle);
	_pUIDlg->OnUAConnected(dlgHandle);
}

//void CUACDlg::OnDisconnectedUacontrol1(long dlgHandle) 
//{
//	_pUIDlg->OnUADisconnected(dlgHandle);	
//}
void CUACDlg::OnDisconnectedUacontrol1(long dlgHandle, long StatusCode) 
{
	AfxTrace( "Event:: IUAControl::OnDisconnected(%d,%d)\n", dlgHandle,StatusCode);
	_pUIDlg->OnUADisconnected(dlgHandle,StatusCode);	
}

void CUACDlg::OnTimeOutUacontrol1(long dlgHandle) 
{
	AfxTrace( "Event:: IUAControl::OnTimeOut(%d)\n", dlgHandle);
	_pUIDlg->OnUATimeOut(dlgHandle);
}

void CUACDlg::OnDialingUacontrol1(long dlgHandle) 
{
	AfxTrace( "Event:: IUAControl::OnDialing(%d)\n", dlgHandle);
	_pUIDlg->OnUADialing(dlgHandle);
}

void CUACDlg::OnBusyUacontrol1(long dlgHandle) 
{
	AfxTrace( "Event:: IUAControl::OnBusy(%d)\n", dlgHandle);
	_pUIDlg->OnUABusy(dlgHandle);	
}

void CUACDlg::OnRejectUacontrol1(long dlgHandle) 
{
	AfxTrace( "Event:: IUAControl::OnReject(%d)\n", dlgHandle);
	_pUIDlg->OnUAReject(dlgHandle);	
}

void CUACDlg::OnTransferredUacontrol1(long dlgHandle, LPCTSTR xferURL) 
{
	AfxTrace( "Event:: IUAControl::OnTransferred(%d,%s)\n", dlgHandle, xferURL);
	_pUIDlg->OnUATransferred(dlgHandle,xferURL);	
}

void CUACDlg::OnHeldUacontrol1(long dlgHandle) 
{
	AfxTrace( "Event:: IUAControl::OnHeld(%d)\n", dlgHandle);
	_pUIDlg->OnUAHeld(dlgHandle);	
}

void CUACDlg::OnRegistrationDoneUacontrol1() 
{
	AfxTrace( "Event:: IUAControl::OnRegisterationDone()~~\n" );
	m_regResult=REG_RESULT_DONE;
}

void CUACDlg::OnRegistrationFailUacontrol1() 
{
	AfxTrace( "Event:: IUAControl::OnRegisterationFail()\n" );
	m_regResult=REG_RESULT_FAIL;

}

void CUACDlg::OnNeedAuthenticationUacontrol1(LPCTSTR strRealmName) 
{
	AfxTrace( "Event:: IUAControl::OnNeedAuthentication()\n" );

}

void CUACDlg::OnWaitingUacontrol1(long dlgHandle) 
{
	AfxTrace( "Event:: IUAControl::OnWaiting(%d)\n", dlgHandle);
	_pUIDlg->OnUAWaiting(dlgHandle);
}

void CUACDlg::OnRegistrationTimeoutUacontrol1() 
{
	AfxTrace( "Event:: IUAControl::OnRegisterationTimeout()\n");
	m_regResult=REG_RESULT_TIMEOUT;
}

void CUACDlg::OnReceivedTextUacontrol1(LPCTSTR remoteURI, LPCTSTR text) 
{
	//_pUIDlg->OnUAReceivedText( remoteURI, text);

	// message wrapper to create dialog successfully in a com event hander
	_pUIDlg->SendMessage( WM_USER_RECVMSG, (WPARAM)remoteURI, (LPARAM)text);
}

void CUACDlg::OnInfoResponseUacontrol1(LPCTSTR remoteURI, long StatusCode, LPCTSTR bstrContentType, LPCTSTR body) 
{
	_pUIDlg->OnUAInfoResponse( remoteURI, StatusCode, bstrContentType, body);
}

void CUACDlg::OnInfoRequestUacontrol1(LPCTSTR remoteURI, LPCTSTR bstrContentType, LPCTSTR body, long* pRspStatusCode, BSTR* pRspContentType, BSTR* pRspBody)
{
	*pRspStatusCode = 200;

	CString strRspBody = "<?xml version=\"1.0\"?>\n"
						 "<VONTEL-Extend-Response version=\"1.0\">\n"
						 "</VONTEL-Extend-Response>";

	_pUIDlg->OnUAInfoRequest( remoteURI, bstrContentType, body, *pRspStatusCode, strRspBody);

	if ( pRspContentType)
		SysFreeString( *pRspContentType);
	if ( pRspBody)
		SysFreeString( *pRspBody);

	CComBSTR bstrType = "text/xml";
	CComBSTR bstrBody = strRspBody;

	bstrType.CopyTo( pRspContentType);
	bstrBody.CopyTo( pRspBody);
}


void CUACDlg::OnOK()
{
	return;
}

void CUACDlg::OnCancel()
{
	// do hide window instead of close
}


void CUACDlg::OnMoving(UINT fwSide, LPRECT pRect) 
{
	//if( pRect->left<10 )
	//	SetWindowPos(NULL,0, pRect->top,0,0,SWP_NOZORDER|SWP_NOSIZE );
	CDialog::OnMoving(fwSide, pRect);
}

CString CUACDlg::GetUAComRegString(LPCTSTR subkey, LPCTSTR name)
{

	CString strRet;
	CRegKey reg;
	char szNull[] = "";
	CString strSubKey=gstrUAComRegKey;
	HKEY hKey;

	strSubKey += subkey;	
	if ( RegCreateKeyEx( HKEY_CURRENT_USER, strSubKey, 0,
						 szNull,
						 REG_OPTION_NON_VOLATILE,
						 KEY_READ, 
						 NULL,
						 &hKey,
						 NULL ) == ERROR_SUCCESS)
	{
		DWORD dwSize = 256;
		DWORD dwType;
		RegQueryValueEx( hKey, name, 0, &dwType, (BYTE*)strRet.GetBuffer(256), &dwSize);
		strRet.ReleaseBuffer();
		RegCloseKey(hKey);
	}
	return strRet;
}

DWORD CUACDlg::GetUAComRegDW(LPCTSTR subkey, LPCTSTR name, DWORD dwDefault)
{
	DWORD dwRet;
	char szNull[] = "";
	CString strSubKey=gstrUAComRegKey;
	HKEY hKey;

	strSubKey += subkey;	
	dwRet = dwDefault;
	if ( RegCreateKeyEx( HKEY_CURRENT_USER, strSubKey, 
						 0,
						 szNull,
						 REG_OPTION_NON_VOLATILE,
						 KEY_READ, 
						 NULL,
						 &hKey,
						 NULL ) == ERROR_SUCCESS)
	{
		DWORD dwSize = sizeof(DWORD);
		DWORD dwType;
		RegQueryValueEx( hKey, name, 0, &dwType, (BYTE*)&dwRet, &dwSize);
		RegCloseKey(hKey);
	}

	return dwRet;
}

void CUACDlg::SetUAComRegValue(LPCTSTR subkey, LPCTSTR name, LPCTSTR pstrValue)
{
	char szNull[] = "";
	CString strSubKey=gstrUAComRegKey;
	HKEY hKey;

	strSubKey += subkey;
	if ( RegCreateKeyEx( HKEY_CURRENT_USER, strSubKey, 
						 0,
						 szNull,
						 REG_OPTION_NON_VOLATILE,
						 KEY_ALL_ACCESS, 
						 NULL,
						 &hKey,
						 NULL ) == ERROR_SUCCESS)
	{
		int setRet;
		setRet =RegSetValueEx( hKey, name, 0, REG_SZ, (CONST BYTE*)pstrValue, lstrlen(pstrValue));
/*		
		if(!setRet)
		{
			LPVOID lpMsgBuf;
			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,GetLastError(),0,(LPTSTR) &lpMsgBuf,0,NULL);
			::MessageBox(NULL,(LPCTSTR)lpMsgBuf,NULL, MB_OK|MB_ICONINFORMATION);
			LocalFree(lpMsgBuf);
			
		
		}	
*/
		RegCloseKey(hKey);
	}

}

void CUACDlg::SetUAComRegValue(LPCTSTR subkey, LPCTSTR name, DWORD dwValue)
{
	char szNull[] = "";
	CString strSubKey=gstrUAComRegKey;
	strSubKey += subkey;
	
	HKEY hKey;
	if ( RegCreateKeyEx( HKEY_CURRENT_USER, strSubKey, 
						 0,
						 szNull,
						 REG_OPTION_NON_VOLATILE,
						 KEY_ALL_ACCESS, 
						 NULL,
						 &hKey,
						 NULL ) == ERROR_SUCCESS)
	{
		RegSetValueEx( hKey, name, 0, REG_DWORD, (CONST BYTE*)&dwValue, sizeof(DWORD));
		RegCloseKey(hKey);
	}
}

void CUACDlg::RestartUACOM()
{
	m_regResult = REG_RESULT_INPROGRESS;

	UAControlDriver.Initialize( "CCL_SIP_PROXY","ua1","a");


	// init local IP

	CString curLocalIP = UAControlDriver.GetLocalIP();
	CString preferedLocalIP = AfxGetApp()->GetProfileString( "", "PreferredLocalIP");
	
	if( !preferedLocalIP.IsEmpty() && preferedLocalIP != curLocalIP && m_bIPPortCheckOK )
	{
		// use prefered ip as local ip and re-init
		UAControlDriver.SetLocalIP(preferedLocalIP);
		UAControlDriver.Stop();
		UAControlDriver.Initialize( "CCL_SIP_PROXY","ua1","a");
	}
	else
	{
		// use UAControl default local IP as prefered ip
		AfxGetApp()->WriteProfileString("","PreferredLocalIP",curLocalIP);
	}

	// init video
	if(m_pSplashDlg)
	{
		m_pSplashDlg->SetDlgItemText(IDC_SPLASH_MSG,"Initializing video ...");
		m_pSplashDlg->UpdateWindow();
	}

	// register proxy server if need
	if ( GetUAComRegDW( "Registrar", "Use_Registrar", 0) == 0)
	{
		m_regResult = REG_RESULT_NONEED;//REG_RESULT_DONE;
		m_strRegistarName = GetUAComRegString("User_Info","Username"); // added by sjhuang
	}
	else
	{
		if(m_pSplashDlg)
		{
			m_pSplashDlg->SetDlgItemText(IDC_SPLASH_MSG,"Registering call server.");
			m_pSplashDlg->UpdateWindow();
		}
		UAComRegister();
	}
}

BOOL CUACDlg::CheckSockPort( LPCTSTR ipAddr)
{
	//CString curIP = GetUAComRegString( "Local_Settings","Local_IP");
	CString curIP = ipAddr;
	DWORD curPort = GetUAComRegDW( "Local_Settings","Local_Port", 5060);

	CAsyncSocket sk;

	// try default port 
	if( !sk.Create( curPort,SOCK_STREAM, FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE, curIP))
	{
		// unable to use default port, let window sockets select a port
		if( !sk.Create( 0,SOCK_STREAM, FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE, curIP) )
		{
			// unable to find a available port or preferred local IP not availible, return fail
			return FALSE;
		}
		else
		{
			CString strAddr;
			UINT newPort;
			sk.GetSockName( strAddr, newPort );
			sk.Close();

			SetUAComRegValue("Local_Settings","Local_Port", newPort);
			return TRUE;
		}
	}
	else
	{
		sk.Close();
		return TRUE;
	}

}

void CUACDlg::OnDestroy() 
{
	CDialog::OnDestroy();
	
	TRACE0("CUACDlg::OnDestroy()\r\n");

}

void CUACDlg::OnMouseMove(UINT nFlags, CPoint point) 
{
	if( m_bDragging)
	{
		CRect r, rcUI;
		int x,y;
		static snapRange = 16;
		RECT waRc;	// rect of work area (not includes tray bar) 
		
		GetWindowRect( &r);
		SystemParametersInfo( SPI_GETWORKAREA, NULL, &waRc, NULL );

		x = r.left-(m_ptDrag.x-point.x);
		y = r.top-(m_ptDrag.y-point.y);
		
		m_pUIDlg->GetWindowRect( &rcUI );
		
		if( abs((x+r.Width()) - rcUI.left) < snapRange )
		{
			x = rcUI.left-r.Width();
			m_bSnap=TRUE;
		}
		//else if( abs( x - rcUI.right) < snapRange )
		//{
		//	x = rcUI.right;
		//	m_bSnap=TRUE;
		//}
		else
			m_bSnap=FALSE;

		if( m_bSnap )
		{
			m_ptSnap.x=rcUI.left-x;
			m_ptSnap.y=rcUI.top-y;
		}



		// snap to screen border
		/*
		if( x - waRc.left < snapRange )
			x = waRc.left;
		else if( waRc.right - (x+r.Width()) < snapRange )
			x = waRc.right - r.Width();

		if( y - waRc.top < snapRange ) 
			y = waRc.top;
		else if ( waRc.bottom - (y+r.Height()) < snapRange )
			y = waRc.bottom - r.Height();
		*/

		
		SetWindowPos(NULL,x, y, 0,0,SWP_NOSIZE|SWP_NOZORDER );
	}	
	CDialog::OnMouseMove(nFlags, point);
}

void CUACDlg::OnLButtonDown(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	TRACE0("CUACDlg::OnLButtonDown\n");
	
	m_ptDrag = point;
	m_bDragging = TRUE;
	SetCapture();
	::SetCursor( ::LoadCursor( NULL, IDC_SIZEALL ));
	
	CDialog::OnLButtonDown(nFlags, point);
}

void CUACDlg::OnLButtonUp(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	TRACE0("CUACDlg::OnLButtonUp\n");
	if( m_bDragging )
	{
		ReleaseCapture();
		m_bDragging=FALSE;
	}
	CDialog::OnLButtonUp(nFlags, point);
}

void CUACDlg::SnapMove(int x, int y)
{
	if( m_bSnap )
		SetWindowPos( NULL, x - m_ptSnap.x, y - m_ptSnap.y, 0, 0, SWP_NOZORDER|SWP_NOSIZE );
}


BOOL CUACDlg::OnEraseBkgnd(CDC* pDC) 
{
	DrawBitmap(pDC, (HBITMAP)m_bmpTitleBar, CRect(0,0,710,21), FALSE);	
	return TRUE;
}



void CUACDlg::AdjustVideoSize(int type, int scale )
{
	if ( type != -1)		// type == -1 mean no change type
		m_nVideoType = type;
	else
		type = m_nVideoType;

	if ( scale != -1)
		m_nVideoScale = scale;	// scale == -1 means no change scale
	else
		scale = m_nVideoScale;

	// update local video window
	SetWindowPos( NULL, 0, 0, 182, 171, SWP_NOZORDER|SWP_NOMOVE);
	CRect r;
	GetClientRect( &r);
	m_pwndUACtrl->SetWindowPos( NULL, r.left-2, r.top+21, r.right+2, r.bottom+1, SWP_NOZORDER);

	// update remote video window
	int vscale;
	if ( type == 3 )// CIF
		vscale = scale * 2;
	else // if ( type == 2 ) // QCIF
		vscale = scale;

	int width = 176 * vscale + 6;
	int height = 144 * vscale + 27;

	// resize remote video window by right-up corner if snapped to main window
	if ( m_dlgRemoteVideo.IsSnapped())
	{
		m_dlgRemoteVideo.GetWindowRect(&r);
		r.left = r.left + r.Width() - width;
		m_dlgRemoteVideo.SetWindowPos( NULL, r.left, r.top, width, height, SWP_NOZORDER);
		m_dlgRemoteVideo.OnMouseMove( r.left, r.top);
	}
	else
	{
		m_dlgRemoteVideo.SetWindowPos( NULL, 0, 0, width, height, SWP_NOMOVE|SWP_NOZORDER);
	}

	// start remote video 
	UAControlDriver.StartRemoteVideoEx( (long)m_dlgRemoteVideo.GetSafeHwnd(), 3, 3+20, scale);


	/*
	CRect r;

	if( type==2) // QCIF
	{
		// resize local video window
		SetWindowPos(NULL, 0, 0, 182, 171, SWP_NOZORDER|SWP_NOMOVE);
		GetClientRect(&r);
		m_pwndUACtrl->SetWindowPos(NULL, r.left-2, r.top+21, r.right+2, r.bottom+1, SWP_NOZORDER);
		
		// resize remote video window by right-up corner
		m_dlgRemoteVideo.GetWindowRect(&r);
		r.left = r.left + r.Width() - 182;
		m_dlgRemoteVideo.SetWindowPos( NULL, r.left, r.top, 182, 171, SWP_NOZORDER);
		m_dlgRemoteVideo.OnMouseMove( r.left, r.top);

		if ( !bInitWindowOnly)
		{
			UAControlDriver.StopRemoteVideo();
			UAControlDriver.StartRemoteVideoEx( (long)m_dlgRemoteVideo.GetSafeHwnd(), 3, 133, scale);
		}
	}

	if( type==3) // CIF
	{
		// resize local video window
		SetWindowPos(NULL, 0, 0, 182, 171, SWP_NOZORDER|SWP_NOMOVE);
		GetClientRect(&r);
		m_pwndUACtrl->SetWindowPos(NULL, r.left-2, r.top+21, r.right+2, r.bottom+1, SWP_NOZORDER);

		// resize remote video window by right-up corner
		m_dlgRemoteVideo.GetWindowRect(&r);
		r.left = r.left + r.Width() - 358;
		m_dlgRemoteVideo.SetWindowPos(NULL, r.left, r.top, 358, 314, SWP_NOZORDER);
		m_dlgRemoteVideo.OnMouseMove( r.left, r.top);

		if ( !bInitWindowOnly)
		{
			UAControlDriver.StopRemoteVideo();
			UAControlDriver.StartRemoteVideoEx( (long)m_dlgRemoteVideo.GetSafeHwnd(), 3, 276, scale);
		}
	}
	*/
}

void CUACDlg::OnRButtonUp(UINT nFlags, CPoint point) 
{
	CDialog::OnRButtonUp(nFlags, point);
}

void CUACDlg::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CDialog::OnShowWindow(bShow, nStatus);
	
	if ( bShow)
		m_dlgRemoteVideo.ShowWindow(SW_SHOWNORMAL);
	else
		m_dlgRemoteVideo.ShowWindow(SW_HIDE);
}

void CUACDlg::OnRemoveVideoSizeChangedUaControl(int nWidth, int nHeight)
{
	m_dlgRemoteVideo.AdjustVideoSize( nWidth, nHeight);
}

/*
void CUACDlg::OnBuddyUpdateBasicStatusUacontrol1(LPCTSTR buddyURI, long bOpen) 
{
	// TODO: Add your control notification handler code here
		TRACE0("Event:: IUAControl::ONBuddyUpdatestatus()~~\n");
	_pUIDlg->OnBuddyUpdateStatus(buddyURI, bOpen, PRE_ONLINE, PRE_ONLINE);
	
}
*/
void CUACDlg::OnBuddyUpdateBasicStatusUacontrol1(LPCTSTR buddyURI, long bOpen, LPCTSTR pStatus, LPCTSTR cStatus) 
{
	// TODO: Add your control notification handler code here
	_pUIDlg->OnBuddyUpdateStatus(buddyURI, bOpen, pStatus, cStatus);
}

void CUACDlg::OnNeedAuthorizeUacontrol1(LPCTSTR strURI) 
{
	// TODO: Add your control notification handler code here

#ifdef _XCAPSERVER
	CString recvName= strURI;
	CString pName;

	int prefix_start = recvName.Find(":",0);
	int prefix_end = recvName.Find("@",0);

	if((prefix_start>=0)&&(prefix_end>=0))
	{
		pName = recvName.Mid(prefix_start+1,prefix_end-prefix_start-1);
		if(!pName.IsEmpty())
			_pUIDlg->SendMessage( WM_USER_RECVNEEDAUTH, (WPARAM)(LPCTSTR)pName, (LPARAM)NULL);
	}
	else
		_pUIDlg->SendMessage( WM_USER_RECVNEEDAUTH, (WPARAM)strURI, (LPARAM)NULL);
#endif
	
}

void CUACDlg::OnReceivedOfflineMsgUacontrol1(LPCTSTR rcv_msg, LPCTSTR rcv_date, LPCTSTR rcv_from) 
{
	// TODO: Add your control notification handler code here
	
	// message wrapper to create dialog successfully in a com event hander
	char text[512];
	char *tmpstr=NULL;
	
	tmpstr = strstr(rcv_msg,"<ConferenceID>");
	if(tmpstr!=NULL)
		return;
	
	sprintf(text,"<This is a offline message sent at %s>\n%s",rcv_date,rcv_msg);
	_pUIDlg->SendMessage( WM_USER_RECVMSG, (WPARAM)rcv_from, (LPARAM)text);
	
}
//add by alan, 20050223
void CUACDlg::OnRegisterExpiredUacontrol1() 
{
	// TODO: Add your control notification handler code here
	
	_pUIDlg->SendMessage( WM_USER_REGEXPIRED, NULL, NULL);

	
}
