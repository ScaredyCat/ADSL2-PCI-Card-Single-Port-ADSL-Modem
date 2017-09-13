// UAControl.cpp : Implementation of CUAControl

#include "stdafx.h"
#include "UAControl.h"

#ifdef _simple
#include <simpleapi/simple_dlg.h>
#endif

#include "UI\PreferSheet.h"
#include "UA\CallManager.h"
#include "UA\MediaManager.h"

#ifdef _simple
#include "SIMPLE\PresManager.h"
#endif

#ifdef _FOR_VIDEO
#include "VIDEO\VideoManager.h"
#endif

#ifdef UACOM_USE_IPMONITOR

#include <Iphlpapi.h>

void FindCurrentIP()
{
	PMIB_IPADDRTABLE pIPAddrTable = NULL;
	ULONG ulSizeBuf = 0;
	DWORD ret;

	ret = GetIpAddrTable( pIPAddrTable, &ulSizeBuf, TRUE);
	
	if (ret != ERROR_INSUFFICIENT_BUFFER)
	{
		/*************************************************************************/
		/* Unexpected return code from GetIfTable.  Bail.                        */
		/*************************************************************************/
		printf("Call to GetIfTable gave unexpected return code.\n");
		return;
	}

	pIPAddrTable = (PMIB_IPADDRTABLE) malloc( ulSizeBuf );

	ret = GetIpAddrTable( pIPAddrTable, &ulSizeBuf, TRUE);

	if (ret != NO_ERROR)
	{
		/*************************************************************************/
		/* Call failed. Bail.                                                    */
		/*************************************************************************/
		printf("Call to GetIfTable failed.\n");
		return;
	}

	CString tmpStr;

	/*
	tmpStr.Format("There is %d IP in the system\n", pIPAddrTable->dwNumEntries);
	AfxMessageBox(tmpStr);
	*/

	IN_ADDR tmpAddr;
	DWORD bestIf;

	ret = GetBestInterface( inet_addr("140.96.102.254"), &bestIf );

	if (ret != NO_ERROR)
	{
		/*************************************************************************/
		/* Call failed. Bail.                                                    */
		/*************************************************************************/
		printf("Call to GetBestInterface failed.\n");
		return;
	}

	tmpAddr.s_addr = pIPAddrTable->table[pIPAddrTable->dwNumEntries-1].dwAddr;
	
	tmpStr.Format("IP = %s\n", inet_ntoa(tmpAddr) );
	AfxMessageBox(tmpStr);

	if ( CUAControl::GetControl() )
	{
		CUAControl::GetControl()->Restart();
		CUAControl::GetControl()->UnRegister();
		Sleep(4000);
		CUAControl::GetControl()->Register();
		Sleep(4000);
		CUAControl::GetControl()->Redial();
	}
}


DWORD WINAPI MyThreadFunc(LPVOID p)
{
	DWORD ret;
	while (1)
	{
		ret = NotifyAddrChange(NULL, NULL);

		if (ret != NO_ERROR)
			return 0;

		FindCurrentIP();
	}

	return 0;
}

#endif

/////////////////////////////////////////////////////////////////////////////
// CUAControl

CUAControl* CUAControl::s_pUAControl = NULL;

CUAControl::CUAControl()
{
	m_bWindowOnly = TRUE;
	m_pProfile = new CUAProfile;
	m_pMediaManager = new CMediaManager;
	m_pCallManager = new CCallManager;

#ifdef _simple
	m_pPresManager = new CPresManager;
#endif

#ifdef _FOR_VIDEO
	m_pVideoManager = new CVideoManager;
#endif
	m_bInitialized = FALSE;
	m_bShowLocalVideo = FALSE;
	m_bIPMonitor = FALSE;
	s_pUAControl = this;

	InitDebug(NULL);

	
}

CUAControl::~CUAControl()
{
	if (this == s_pUAControl)
		s_pUAControl = NULL;

	if (m_pProfile && m_pProfile->m_bInitialized) {
		m_pProfile->Write();
		delete m_pProfile;
		m_pProfile = NULL;
	}
#ifdef _simple
	if (m_pPresManager) {
		delete m_pPresManager;
		m_pPresManager = NULL;
	}
#endif
	if (m_pCallManager && m_pCallManager->IsInitialized() ) {
		m_pCallManager->stop();
		delete m_pCallManager;
		m_pCallManager = NULL;
	}
	if (m_pMediaManager && m_pMediaManager->m_bInitialized) {
		delete m_pMediaManager;
		m_pMediaManager = NULL;
	}
#ifdef _FOR_VIDEO
	if (m_pVideoManager != NULL) {
		delete m_pVideoManager;
		m_pVideoManager = NULL;
	}
#endif
	ExitDebug();
}

STDMETHODIMP CUAControl::Initialize(BSTR realm, BSTR userid, BSTR password)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	USES_CONVERSION;

	WSADATA wsadata;

	/* Should not initialize again */
	if (m_bInitialized)
		return S_OK;

	/* Get the local IP address */
	
	WSAStartup(0x0101, &wsadata);
	m_LocalIPAddr = getMyAddr();
	WSACleanup();
	//m_LocalIPAddr = "[fe80::204:76ff:fed9:89b4%4]";
	//m_LocalIPAddr = "[fe80::209:6bff:fee0:804b%4]";

	if (userid)
		m_userid = OLE2CA(userid);

	if (password)
		m_password = OLE2CA(password);

	if (realm)
		m_realm = OLE2CA(realm);

	/* Initialize the UAProfile */
	m_pProfile->init(m_LocalIPAddr, m_userid, m_realm, m_password);
	
	/* Initialize the MediaManager */
	m_pMediaManager->init();

	/* Initialize the CallManager */
	m_pCallManager->init();

	/* Initialize the PresManager */
#ifdef _simple
	m_pPresManager->init();
#endif

#ifdef _UACOM_USE_IPMONITOR
	if ( m_bIPMonitor == FALSE )
	{
		HANDLE hThread;
		DWORD tid;
		if ( ( hThread = CreateThread( NULL, 0, MyThreadFunc, NULL, 0, &tid ) ) < 0)
			return S_FALSE;

		CloseHandle( hThread );
		m_bIPMonitor = TRUE;
	}
#endif
	
	m_bInitialized = TRUE;

	
	
	return S_OK;
}


// sam added: HRESULT always use negative value for error code
HRESULT _RCODE2HR( int rcode)
{
	if ( rcode > 0)
		return (HRESULT)(-rcode);
	else
		return (HRESULT)rcode;
}

/* Make call */
STDMETHODIMP CUAControl::MakeCall(BSTR dialURL, int* retval)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	USES_CONVERSION;

	if (!m_bInitialized)
		return S_OK;

	m_pCallManager->AddCommand(UICMD_DIAL, -1, -1, OLE2CA(dialURL), NULL);

	m_LastDialURL = OLE2CA(dialURL);

	return S_OK;
}

/* Hold call */
STDMETHODIMP CUAControl::HoldCall()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	return _RCODE2HR( m_pCallManager->Hold());
}

/* UnHold call */
STDMETHODIMP CUAControl::UnHoldCall(int dlgHandle)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	return _RCODE2HR( m_pCallManager->UnHold(dlgHandle) );
}

/* Cancel call */
STDMETHODIMP CUAControl::CancelCall()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	m_pCallManager->AddCommand(UICMD_CANCEL, -1, -1, NULL, NULL);
	return S_OK;
}

/* Drop call */
STDMETHODIMP CUAControl::DropCall(int dlgHandle)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	m_pCallManager->AddCommand(UICMD_DROP, dlgHandle, -1, NULL, NULL);

	return S_OK;
}

/* Answer call */
STDMETHODIMP CUAControl::AnswerCall(int dlgHandle)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	return _RCODE2HR( m_pCallManager->AcceptCall(dlgHandle));
}

/* Reject call */
STDMETHODIMP CUAControl::RejectCall(int dlgHandle)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	return _RCODE2HR( m_pCallManager->RejectCall(dlgHandle));
}

/* UnAttended Transfer dialog to xferURL */
STDMETHODIMP CUAControl::UnAttendXfer(int dlgHandle, BSTR xferURL)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	USES_CONVERSION;

	if (!m_bInitialized)
		return S_OK;

	return _RCODE2HR( m_pCallManager->UnAttendTransfer(dlgHandle, OLE2CA(xferURL)) );
}

/* Attended Transfer dialog1 to dialog2 */
STDMETHODIMP CUAControl::AttendXfer(int dlgHandle1, int dlgHandle2)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	return _RCODE2HR( m_pCallManager->AttendTransfer( dlgHandle1, dlgHandle2) );
}

/* Show preferences */
STDMETHODIMP CUAControl::ShowPreferences( BOOL* pChanged )
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	CPreferSheet PS( _T("UA Preferences") );
	if ( PS.DoModal() == IDOK)
	{
		// write the user profile
		m_pProfile->Write();
		m_pProfile->init(m_LocalIPAddr, m_userid, m_realm, m_password);

		#ifdef _simple
		m_pPresManager->stop();
		#endif

		// re-initialize CallManager
		m_pCallManager->stop();
		m_pCallManager->init();

		// re-initialize PresManager
		#ifdef _simple
		m_pPresManager->init();
		#endif

		*pChanged = TRUE;
		
	} else {

		*pChanged = FALSE;
		
	}

	return S_OK;
}

void CUAControl::Restart()
{
	WSADATA wsadata;

	/* Get the local IP address */
	WSAStartup(0x0101, &wsadata);
	m_LocalIPAddr = getMyAddr();
	WSACleanup();

	// write the user profile
	m_pProfile->Write();
	m_pProfile->init(m_LocalIPAddr, m_userid, m_realm, m_password);

	#ifdef _simple
	m_pPresManager->stop();
	#endif

	// stop current media
	m_pMediaManager->Media_RTP_Stop();
	m_pMediaManager->StopPlaySound();

	#ifdef _FOR_VIDEO
	m_pVideoManager->RTPDisconnect();
	if ( CUAControl::GetControl() ) {
		m_pVideoManager->StopRemoteVideo();
	}
	#endif

	// re-initialize CallManager
	m_pCallManager->stop();
	m_pCallManager->init();

	// re-initialize PresManager
	#ifdef _simple
	m_pPresManager->init();
	#endif
}

void CUAControl::Redial()
{
	m_pCallManager->AddCommand(UICMD_DIAL, -1, -1, m_LastDialURL, NULL);
}

/* Test function */
STDMETHODIMP CUAControl::Test(BSTR tmpStr)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	USES_CONVERSION;
	
	if (!m_bInitialized)
		return S_OK;

	//m_pMediaManager->PrintOutRTPStat();
	CString tmp = OLE2CA(tmpStr);

	if( tmp=="write" )
		m_pProfile->Write();
	else
		m_pProfile->init(m_LocalIPAddr, m_userid, m_realm, m_password);

	return S_OK;
}


STDMETHODIMP CUAControl::GetRemoteParty(int dlgHandle, BSTR *retval)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	*retval = CComBSTR( m_pCallManager->GetRemoteParty( dlgHandle ) );

	return S_OK;
}

STDMETHODIMP CUAControl::StartLocalVideo()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	POINT p;
	p.x = 2;
	p.y = 0;

#ifdef _FOR_VIDEO
	m_pVideoManager->StartLocalVideo( m_hWnd, p);
#endif
	m_bShowLocalVideo = TRUE;

	return S_OK;
}

STDMETHODIMP CUAControl::StopLocalVideo()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

#ifdef _FOR_VIDEO
	m_pVideoManager->StopLocalVideo();
#endif
	m_bShowLocalVideo = FALSE;

	return S_OK;
}

STDMETHODIMP CUAControl::Register()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	m_pCallManager->AddCommand(UICMD_REG, -1, -1, NULL, NULL);
	return S_OK;
}

STDMETHODIMP CUAControl::UnRegister()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	//return _RCODE2HR( m_pCallManager->UnRegister() );
	m_pCallManager->AddCommand(UICMD_UNREG, -1, -1, NULL, NULL);
	return S_OK;
}

STDMETHODIMP CUAControl::RegQuery()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	m_pCallManager->Query();

	return S_OK;
}

STDMETHODIMP CUAControl::SwitchLocalVideo()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	if (m_bShowLocalVideo) 
		StopLocalVideo();
	else
		StartLocalVideo();

	return S_OK;
}

STDMETHODIMP CUAControl::Stop()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	m_pCallManager->stop();
	m_bInitialized = FALSE;
	return S_OK;
}

STDMETHODIMP CUAControl::get_LocalIP(BSTR *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	*pVal = CComBSTR( m_pProfile->m_LocalAddr );

	return S_OK;
}

STDMETHODIMP CUAControl::put_LocalIP(BSTR newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized && !newVal)
		return S_OK;

	USES_CONVERSION;

	m_pProfile->m_bUseStaticIP = TRUE;
	m_pProfile->m_LocalAddr = OLE2CA( newVal );
	m_pProfile->Write();

	return S_OK;
}

STDMETHODIMP CUAControl::SendDTMF0()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	m_pMediaManager->SendDTMFTone(0);

	return S_OK;
}

STDMETHODIMP CUAControl::SendDTMF1()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	m_pMediaManager->SendDTMFTone(1);

	return S_OK;
}

STDMETHODIMP CUAControl::SendDTMF2()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	m_pMediaManager->SendDTMFTone(2);

	return S_OK;
}

STDMETHODIMP CUAControl::SendDTMF3()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	m_pMediaManager->SendDTMFTone(3);

	return S_OK;
}

STDMETHODIMP CUAControl::SendDTMF4()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	m_pMediaManager->SendDTMFTone(4);

	return S_OK;
}

STDMETHODIMP CUAControl::SendDTMF5()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	m_pMediaManager->SendDTMFTone(5);

	return S_OK;
}

STDMETHODIMP CUAControl::SendDTMF6()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	m_pMediaManager->SendDTMFTone(6);

	return S_OK;
}

STDMETHODIMP CUAControl::SendDTMF7()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	m_pMediaManager->SendDTMFTone(7);

	return S_OK;
}

STDMETHODIMP CUAControl::SendDTMF8()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	m_pMediaManager->SendDTMFTone(8);

	return S_OK;
}

STDMETHODIMP CUAControl::SendDTMF9()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	m_pMediaManager->SendDTMFTone(9);

	return S_OK;
}

STDMETHODIMP CUAControl::SendDTMFS()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	m_pMediaManager->SendDTMFTone(10);

	return S_OK;
}

STDMETHODIMP CUAControl::SendDTMFP()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	m_pMediaManager->SendDTMFTone(11);

	return S_OK;
}

STDMETHODIMP CUAControl::get_bSingleCall(BOOL *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	*pVal = m_pCallManager->m_bSingleCall;

	return S_OK;
}

STDMETHODIMP CUAControl::put_bSingleCall(BOOL newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	m_pCallManager->m_bSingleCall = newVal;

	return S_OK;
}

STDMETHODIMP CUAControl::Reset()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	m_pCallManager->stop();
	m_pCallManager->init();

	return S_OK;
}

STDMETHODIMP CUAControl::GetAudioType(int *retval)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	*retval = m_pMediaManager->GetMediaType();	

	return S_OK;
}

STDMETHODIMP CUAControl::GetVideoType(int *retval)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

#ifdef _FOR_VIDEO
	*retval = m_pVideoManager->GetMediaType();
#endif

	return S_OK;
}

STDMETHODIMP CUAControl::GetVideoSize(int *retval)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

#ifdef _FOR_VIDEO
	*retval = m_pVideoManager->GetSizeFormat();
#endif

	return S_OK;
}

// sam add
STDMETHODIMP CUAControl::GetRemotePartyDisplayName(int dlgHandle, /*[out,retval]*/ BSTR* retval)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	*retval = CComBSTR( m_pCallManager->GetRemotePartyDisplayName( dlgHandle ) );

	return S_OK;
}

STDMETHODIMP CUAControl::GetMediaReceiveQuality( /*[out,retval]*/ int* retval)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	double quality = m_pMediaManager->GetReceiveQuality();
	*retval = (int)((quality * 100) + 0.5);		// convert to 100%

	return S_OK;
}

STDMETHODIMP CUAControl::StartRemoteVideo(int x, int y, int scale)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
	
	POINT p;
	p.x = x;
	p.y = y;

#ifdef _FOR_VIDEO
	m_pVideoManager->StartRemoteVideo( m_hWnd, p, scale);
#endif
	return S_OK;
}

STDMETHODIMP CUAControl::StopRemoteVideo()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

#ifdef _FOR_VIDEO
	m_pVideoManager->StopRemoteVideo();
#endif
	return S_OK;
}

STDMETHODIMP CUAControl::SendText(BSTR dialURL, BSTR tmpstr)
{
	UaContent content = NULL;
	
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	USES_CONVERSION;

	if (!m_bInitialized)
		return S_OK;

	if (!dialURL || !tmpstr)
		return RC_OK;
		
	content = uaContentNew("text/plain",strlen(OLE2CA(tmpstr))+1, (void*)OLE2CA(tmpstr) );
	m_pCallManager->AddCommand(UICMD_MESSAGE, -1, -1, OLE2CA(dialURL), content);
	
	uaContentFree(content);
	return S_OK;
}

STDMETHODIMP CUAControl::AddBuddy(BSTR buddyURI)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	USES_CONVERSION;
	
	if (!m_bInitialized)
		return S_OK;

	if (!buddyURI)
		return RC_OK;

	#ifdef _simple
	m_pPresManager->AddBuddy( OLE2CA(buddyURI) );
	#endif

	return S_OK;
}

STDMETHODIMP CUAControl::DelBuddy(BSTR buddyURI)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	USES_CONVERSION;

	if (!m_bInitialized)
		return S_OK;

	if (!buddyURI)
		return RC_OK;

#ifdef _simple
	m_pPresManager->DelBuddy( OLE2CA(buddyURI) );
#endif

	return S_OK;
}

STDMETHODIMP CUAControl::BlockBuddy(BSTR buddyURI)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	USES_CONVERSION;

	if (!m_bInitialized)
		return S_OK;

	if (!buddyURI)
		return RC_OK;

	#ifdef _simple
	m_pPresManager->BlockBuddy( OLE2CA(buddyURI) );
	#endif

	return S_OK;
}

STDMETHODIMP CUAControl::UnBlockBuddy(BSTR buddyURI)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	USES_CONVERSION;

	if (!m_bInitialized)
		return S_OK;

	if (!buddyURI)
		return RC_OK;

	#ifdef _simple
	m_pPresManager->UnblockBuddy( OLE2CA(buddyURI) );
	#endif

	return S_OK;
}

STDMETHODIMP CUAControl::GetBuddyCount(int *retval)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	USES_CONVERSION;

	if (!m_bInitialized)
		return S_OK;

	#ifdef _simple
	*retval = m_pPresManager->GetBuddyCount();
	#endif

	return S_OK;
}

STDMETHODIMP CUAControl::GetBuddyURI(int number, BSTR *retval)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	#ifdef _simple
 	*retval = CComBSTR( m_pPresManager->GetBuddyURI(number) );
	#endif

  	return S_OK;
}

STDMETHODIMP CUAControl::UpdateBasicStatus(BOOL bOpen)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	#ifdef _simple
	m_pPresManager->ChangeBasicState(bOpen);
	#endif

	return S_OK;
}

STDMETHODIMP CUAControl::GetBuddyINSubState(BSTR buddyURI, int *SubState)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	USES_CONVERSION;

	#ifdef _simple
	*SubState = m_pPresManager->GetBuddyINSubStateFromURI( OLE2CA(buddyURI) );
	#endif

	return S_OK;
}

STDMETHODIMP CUAControl::GetBuddyOUTSubState(BSTR buddyURI, int *SubState)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	USES_CONVERSION;

	#ifdef _simple
	*SubState = m_pPresManager->GetBuddyOUTSubStateFromURI( OLE2CA(buddyURI) );
	#endif

	return S_OK;
}

STDMETHODIMP CUAControl::GetBuddyBasicStatus(BSTR buddyURI, int *BasicStatus)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	USES_CONVERSION;

	#ifdef _simple
	*BasicStatus = m_pPresManager->GetBuddyBasicStatus( OLE2CA(buddyURI) );
	#endif

	return S_OK;
}

STDMETHODIMP CUAControl::SubscribeBuddy(BSTR buddyURI)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	USES_CONVERSION;

	#ifdef _simple
	CUAProfile* pProfile = CUAProfile::GetProfile();
	m_pPresManager->SubscribeBuddy( OLE2CA(buddyURI),FALSE,"presence",pProfile->m_ulExpireTime);
	//m_pPresManager->SubscribeBuddy( OLE2CA(buddyURI) );
	#endif

	return S_OK;
}

STDMETHODIMP CUAControl::UnsubscribeBuddy(BSTR buddyURI)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

		if (!m_bInitialized)
		return S_OK;

	USES_CONVERSION;

	#ifdef _simple
	m_pPresManager->UnsubscribeBuddy(OLE2CA(buddyURI), "presence");
	//m_pPresManager->UnsubscribeBuddy( OLE2CA(buddyURI) );
	#endif

	return S_OK;
}

STDMETHODIMP CUAControl::StartLocalVideoEx(int nhwnd, int x, int y, int scale)
{
	HWND hwnd = (HWND)nhwnd;

	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	POINT p;
	p.x = x;
	p.y = y;

	if ( hwnd == NULL)	// default value
		hwnd = m_hWnd;

#ifdef _FOR_VIDEO
	m_pVideoManager->StartLocalVideo( hwnd, p);
#endif
	m_bShowLocalVideo = TRUE;

	return S_OK;
}

STDMETHODIMP CUAControl::StartRemoteVideoEx(int nhwnd, int x, int y, int scale)
{
	HWND hwnd = (HWND)nhwnd;

	AFX_MANAGE_STATE(AfxGetStaticModuleState())
	
	POINT p;
	p.x = x;
	p.y = y;

	if ( hwnd == NULL)	// default value
		hwnd = m_hWnd;

#ifdef _FOR_VIDEO
	m_pVideoManager->StartRemoteVideo( hwnd, p, scale);
#endif
	return S_OK;
}

STDMETHODIMP CUAControl::IsBuddyBlock(BSTR buddyURI, BOOL *retval)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	USES_CONVERSION;
	
	if (!m_bInitialized)
		return S_OK;

	if (!buddyURI)
		return RC_OK;

	#ifdef _simple
	*retval = m_pPresManager->IsBuddyBlock( OLE2CA(buddyURI) );
	#endif
	
	return S_OK;
}

STDMETHODIMP CUAControl::IsBuddyAuthorized(BSTR buddyURI, BOOL *retval)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	USES_CONVERSION;
	
	if (!m_bInitialized)
		return S_OK;

	if (!buddyURI)
		return RC_OK;

	#ifdef _simple
	*retval = m_pPresManager->IsBuddyAuthorized( OLE2CA(buddyURI) );
	#endif
	
	return S_OK;
}

STDMETHODIMP CUAControl::AuthorizeBuddy(BSTR buddyURI)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	USES_CONVERSION;

	if (!m_bInitialized)
		return S_OK;

	if (!buddyURI)
		return RC_OK;

	#ifdef _simple
	m_pPresManager->AuthorizeBuddy( OLE2CA(buddyURI) );
	#endif

	return S_OK;
}

STDMETHODIMP CUAControl::UnauthorizeBuddy(BSTR buddyURI)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	USES_CONVERSION;

	if (!m_bInitialized)
		return S_OK;

	if (!buddyURI)
		return RC_OK;

	#ifdef _simple
	m_pPresManager->UnauthorizeBuddy( OLE2CA(buddyURI) );
	#endif

	return S_OK;
}

STDMETHODIMP CUAControl::SendInfo(BSTR bstrURL, BSTR bstrContentType, BSTR bstrBody)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	USES_CONVERSION;

	if (!m_bInitialized)
		return S_OK;

	return _RCODE2HR( m_pCallManager->SendInfo( OLE2CA(bstrURL), OLE2CA(bstrContentType), OLE2CA(bstrBody)) );
}

STDMETHODIMP CUAControl::SubscribeWinfo(BSTR buddyURI)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	USES_CONVERSION;


	// TODO: Add your implementation code here

	CUAProfile* pProfile = CUAProfile::GetProfile();

	m_pPresManager->SubscribeBuddy( OLE2CA(buddyURI),FALSE,"presence.winfo",pProfile->m_ulExpireTime);	

	return S_OK;
}

STDMETHODIMP CUAControl::SubscribePres(BSTR buddyURI)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	USES_CONVERSION;

	CUAProfile* pProfile = CUAProfile::GetProfile();
	// TODO: Add your implementation code here
	m_pPresManager->SubscribeBuddy( OLE2CA(buddyURI),FALSE,"presence",pProfile->m_ulExpireTime);	
	return S_OK;
}

STDMETHODIMP CUAControl::SubscribeResLst(BSTR ResourceURI)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	USES_CONVERSION;


	// TODO: Add your implementation code here
	CUAProfile* pProfile = CUAProfile::GetProfile();
	m_pPresManager->SubscribeBuddy( OLE2CA(ResourceURI),TRUE,"presence",pProfile->m_ulExpireTime);	

	return S_OK;
}

STDMETHODIMP CUAControl::UnsubscribeWinfo(BSTR buddyURI)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
	if (!m_bInitialized)
		return S_OK;

	USES_CONVERSION;


	// TODO: Add your implementation code here
	m_pPresManager->UnsubscribeBuddy(OLE2CA(buddyURI), "presence.winfo");

	return S_OK;
}

STDMETHODIMP CUAControl::UnsubscribePres(BSTR buddyURI)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
	if (!m_bInitialized)
		return S_OK;

	USES_CONVERSION;

	// TODO: Add your implementation code here
	m_pPresManager->UnsubscribeBuddy(OLE2CA(buddyURI), "presence");
	
	return S_OK;
}

STDMETHODIMP CUAControl::UnsubscribeResLst(BSTR ResourceURI)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	USES_CONVERSION;

	// TODO: Add your implementation code here
	m_pPresManager->UnsubscribeBuddy(OLE2CA(ResourceURI), "presence");
	return S_OK;
}

STDMETHODIMP CUAControl::PublishStatus(BSTR szURI, BOOL bStatus)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if (!m_bInitialized)
		return S_OK;

	USES_CONVERSION;
	// TODO: Add your implementation code here

	CUAProfile* pProfile = CUAProfile::GetProfile();

	m_pPresManager->PublishBuddy(OLE2CA(szURI), "presence", bStatus, 0.0,pProfile->m_ulExpireTime);

	return S_OK;
}
