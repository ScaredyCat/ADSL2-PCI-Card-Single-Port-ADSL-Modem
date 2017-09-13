// CallManager.cpp: implementation of the CCallManager class.
//
//////////////////////////////////////////////////////////////////////

#include <afxinet.h>
#include "CallManager.h"
#include "UAControl.h"


#ifdef _simple
#include <simpleapi/simple_api.h>
#endif

#ifdef _FOR_VIDEO
#include "..\VIDEO\VideoManager.h"
CVideoManager *pVideoManager = NULL;
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define MAX_FORWARD 10
#define EVT_DISPATCH_WAIT_TIME 100
#define BUFLEN	256

CCallManager *pCallManager = NULL;
CMediaManager *pMediaManager = NULL;
CSDPManager *pSDPManager = NULL;
CPresManager *pPresManager = NULL;
CUAProfile *pProfile = NULL;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCallManager* CCallManager::s_pCallManager = NULL;

CCallManager::CCallManager()
{
	m_hCMThread = NULL;
	m_hUAThread = NULL;
	m_UaUser = NULL;
	m_UaCfg = NULL;
	m_UaMgr = NULL;
	m_UaMgr2 = NULL;
	pSDPManager = NULL;

	m_EventMsgLst = NULL;
	m_UICmdMsgLst = NULL;

	m_ActiveDlg = NULL;
	m_bDoing = FALSE;
	m_bInitialized = FALSE;
	m_bRTPConnected = FALSE;
	m_bClient = FALSE;
	m_DlgCount = 0;
	m_bEarlyMedia = FALSE;

	s_pCallManager = this;

	InitializeCriticalSection(&m_CallMgrCS);
}

CCallManager::~CCallManager()
{
	if (this == s_pCallManager)
		s_pCallManager = NULL;

	DeleteCriticalSection(&m_CallMgrCS);
}

DWORD CCallManager::UAThreadFunc(LPVOID pParam)
{
	while (1) {
		uaEvtDispatch(EVT_DISPATCH_WAIT_TIME);
		Sleep(10);
	}
	return 0;
}

DWORD CCallManager::CallManagerThreadFunc(LPVOID pParam)
{
	CCallManager* pCM = (CCallManager*)pParam;

	::CoInitialize(NULL);
	
	while (1) {
		EnterCriticalSection(&(pCM->m_CallMgrCS));
		if (!pCM->m_bDoing) {
			LeaveCriticalSection(&(pCM->m_CallMgrCS));
			break;
		}
		//uaEvtDispatch(EVT_DISPATCH_WAIT_TIME);

		if (dxLstGetSize(pCallManager->m_EventMsgLst) > 0)
			pCallManager->ProcessEvt((UaMsg)dxLstGetHead(pCallManager->m_EventMsgLst));

		if (dxLstGetSize(pCallManager->m_UICmdMsgLst) > 0)
			pCallManager->ProcessCmd((UICmdMsg)dxLstGetHead(pCallManager->m_UICmdMsgLst));

		Sleep(20);

		LeaveCriticalSection(&(pCM->m_CallMgrCS));
	}

	CloseHandle( pCallManager->m_hCMThread); // added by sjhuang 2006/03/01
	pCallManager->m_hCMThread = NULL;

	::CoUninitialize();

	return 0;
}

/* Initialize the CallManager */
void CCallManager::init()
{
	int rCode, cseq, tmp;
	MyCString tmpStr;
	char hostname[128];
	WSADATA wsadata;
	MyCString strText, strRealm, strAccountID, strPasswd;

	EnterCriticalSection(&m_CallMgrCS);
		
	/* set pointer to other component */
	pCallManager = this;
	
	pMediaManager = CMediaManager::GetMediaMgr();
	ASSERT( pMediaManager );

	pProfile = CUAProfile::GetProfile();
	ASSERT( pProfile );

	pSDPManager = new CSDPManager(50);
	ASSERT( pSDPManager );

#ifdef _simple
	pPresManager = CPresManager::GetPresMgr();
	ASSERT( pPresManager );
#endif

#ifdef _FOR_VIDEO
	pVideoManager = CVideoManager::GetVideoMgr();
	ASSERT( pVideoManager );
#endif

	m_EventMsgLst = dxLstNew(DX_LST_POINTER);
	m_UICmdMsgLst = dxLstNew(DX_LST_POINTER);

	/* get local host name */
	WSAStartup(0x0101, &wsadata);
	gethostname(hostname, sizeof(hostname));
	WSACleanup();

	/* Config data for uaLibInit */
	// sam modified, read registry to determine debug flag and level
	// HKCU\Software\CCL, ITRI\UACom\Debug\LogFlag = LogFlag (0,1:consle,2:file,4:socket) bitwise
	// HKCU\Software\CCL, ITRI\UACom\Debug\TraceLevel = TraceLevel (0~100)
	// HKCU\Software\CCL, ITRI\UACom\Debug\LogFile = file name (default "UACom.log")
	int nLogFlag = AfxGetApp()->GetProfileInt( "Debug", "LogFlag", LOG_FLAG_FILE);
	int nTraceLevel = AfxGetApp()->GetProfileInt( "Debug", "TraceLevel", TRACE_LEVEL_API);
	CString strFileName = AfxGetApp()->GetProfileString( "Debug", "LogFile", "UACom.log");

	CTime nowtime = CTime::GetCurrentTime();
	//CString strFileName = "UACom_Log_" + nowtime.Format("%m%d_%H%M%S")+ ".txt";

	m_ConfigStr.log_flag = static_cast<LogFlag>(nLogFlag);
	m_ConfigStr.trace_level = static_cast<TraceLevel>(nTraceLevel);
	strncpy(m_ConfigStr.fname, strFileName, sizeof(m_ConfigStr.fname));
	
	while ( !IsPortAvailable(pProfile->m_LocalPort) ) {
		pProfile->m_LocalPort += 2;
		tmpStr.Format( _T("Port occupied, change to %d"), pProfile->m_LocalPort );
	}

	if (!tmpStr.IsEmpty())
		AfxMessageBox(tmpStr);

	m_ConfigStr.tcp_port=pProfile->m_LocalPort;
	strcpy(m_ConfigStr.laddr,pProfile->m_LocalAddr);
		
	// specify ToS bit for all SIP message
	
	// marked for IPv6
	/*
	int nSIPTos = AfxGetApp()->GetProfileInt( SESSION_LOCAL, KEY_SIP_TOS, 0);
	cxSockSetDefaultToS( nSIPTos);
	*/

#ifdef USESTUN
	if ( pProfile->m_bUseSTUN && !pProfile->m_STUNserver.IsEmpty() )
		rCode=uaLibInitWithStun(UAEvtCB,m_ConfigStr,pProfile->m_STUNserver);
	else
		rCode=uaLibInitWithStun(UAEvtCB,m_ConfigStr,NULL);
#else
	rCode=uaLibInit(UAEvtCB,m_ConfigStr);
#endif

	if( rCode==RC_ERROR )
	{
		LeaveCriticalSection(&m_CallMgrCS);
		stop();
		return;
	}


	/* Setting UaUser */
	m_UaUser=uaUserNew();
	rCode=uaUserSetName(m_UaUser,pProfile->m_Username);
	
	//rCode=uaUserSetLocalAddr(m_UaUser,pProfile->m_LocalAddr);
	rCode=uaUserSetLocalAddr(m_UaUser,pProfile->m_ExtAddr);
	rCode=uaUserSetLocalPort(m_UaUser,pProfile->m_ExtPort);

	rCode=uaUserSetLocalHost(m_UaUser,hostname);

	/*
	if (pProfile->m_Realm.GetLength() > 0) {
		uaUserSetRealm(m_UaUser, pProfile->m_Realm);
		uaUserSetPasswd(m_UaUser, pProfile->m_Password);
	}
	*/
	
	/*
	MyCString strContactAddr, strLocalPort;

	if ( pProfile->m_bTransport ) {
		strContactAddr.Format(_T("sip:%s@"),pProfile->m_Username);
		strContactAddr += pProfile->m_LocalAddr;
		if (pProfile->m_Qvalue != 0)
			strLocalPort.Format(_T(":%d; transport=tcp; q=%d"), pProfile->m_LocalPort, pProfile->m_Qvalue);
		else
			strLocalPort.Format(_T(":%d; transport=tcp"), pProfile->m_LocalPort);
		
		strContactAddr += strLocalPort;
	} else {
		strContactAddr.Format(_T("sip:%s@"),pProfile->m_Username);
		strContactAddr += pProfile->m_LocalAddr;

		if (pProfile->m_Qvalue != 0)
			strLocalPort.Format(_T(":%d; q=%d"), pProfile->m_LocalPort, pProfile->m_Qvalue);
		else
			strLocalPort.Format(_T(":%d"), pProfile->m_LocalPort);

		strContactAddr += strLocalPort;
	}

	rCode=uaUserSetContactAddr(m_UaUser,strContactAddr);
	*/
	rCode=uaUserSetContactAddr(m_UaUser,pProfile->m_ContactAddr);
	rCode=uaUserSetContactAddr(m_UaUser,pProfile->m_TLSContactAddr);
	rCode=uaUserSetMaxForwards(m_UaUser,MAX_FORWARD);
	rCode=uaUserSetExpires(m_UaUser,pProfile->m_ulExpireTime);

	/* Setting Credentials */
	UaAuthz	tmpUaAuthz = NULL;
	for (int i=0;i < pProfile->m_CredentialCount;i++)
	{
		strText = pProfile->m_Credential[i];
				
		tmp = strText.Find( _T(":") );
		strRealm = strText.Left(tmp);
		
		strText = strText.Right( strText.GetLength()-tmp-1);
		tmp = strText.Find( _T(":") );
		strAccountID = strText.Left(tmp);				
		
		strPasswd = strText.Right( strText.GetLength()-tmp-1 );
	
		tmpUaAuthz = uaAuthzNew( strRealm, strAccountID, strPasswd );
		uaUserAddAuthz(m_UaUser, tmpUaAuthz);
	}

	/* Setting UaCfg */
	m_UaCfg=uaCfgNew();
	
	/* Added by sjhuang 20060120 for Http Tunnel Server */
	if( pProfile->m_bUseHttpTunnel ) {
		//CString tmp;
		//tmp.Format(":%d",pProfile->m_usHttpTunnelPort+1);
		//tmp = pProfile->m_szHttpTunnelAddr + tmp;
		//AfxMessageBox(tmp);
		
		rCode=uaCfgSetProxy(m_UaCfg,pProfile->m_bUseHttpTunnel);
		rCode=uaCfgSetProxyAddr(m_UaCfg,pProfile->m_szHttpTunnelAddr);
		rCode=uaCfgSetProxyPort(m_UaCfg,pProfile->m_usHttpTunnelPort+1);
	} else {
		rCode=uaCfgSetProxy(m_UaCfg,pProfile->m_bUseOutbound);
		rCode=uaCfgSetProxyAddr(m_UaCfg,pProfile->m_szOutboundAddr);
		rCode=uaCfgSetProxyPort(m_UaCfg,pProfile->m_usOutboundPort);
	}

	if (pProfile->m_bTransport)
		rCode=uaCfgSetProxyTXP(m_UaCfg,TCP);
	else
		rCode=uaCfgSetProxyTXP(m_UaCfg,UDP);
	
	rCode=uaCfgSetRegistrar(m_UaCfg,pProfile->m_bUseRegistrar);
	rCode=uaCfgSetRegistrarAddr(m_UaCfg,pProfile->m_szRegistrarAddr);
	rCode=uaCfgSetRegistrarPort(m_UaCfg,pProfile->m_usRegistrarPort);

	if ( pProfile->m_bTransport )
		rCode=uaCfgSetRegistrarTXP(m_UaCfg,TCP);
	else
		rCode=uaCfgSetRegistrarTXP(m_UaCfg,UDP);

#ifdef _simple
	rCode=uaCfgSetSIMPLEProxy(m_UaCfg,pProfile->m_bUseSIMPLEServer);
	rCode=uaCfgSetSIMPLEProxyAddr(m_UaCfg,pProfile->m_szSIMPLEServerAddr);
	rCode=uaCfgSetSIMPLEProxyPort(m_UaCfg,pProfile->m_usSIMPLEServerPort);
	
	if (pProfile->m_bTransport)
		rCode=uaCfgSetSIMPLEProxyTXP(m_UaCfg,TCP);
	else
		rCode=uaCfgSetSIMPLEProxyTXP(m_UaCfg,UDP);
#endif

	rCode=uaCfgSetListenPort(m_UaCfg,pProfile->m_LocalPort);

	/* Setting UaMgr */
	m_UaMgr=uaMgrNew();
	uaMgrSetUser(m_UaMgr,m_UaUser);
	uaMgrSetCfg(m_UaMgr,m_UaCfg);
	
	/* Setting UaMgr2 */
	uaCfgSetRegistrar(m_UaCfg,pProfile->m_bUseRegistrar2);
	uaCfgSetRegistrarAddr(m_UaCfg,pProfile->m_szRegistrar2Addr);
	uaCfgSetRegistrarPort(m_UaCfg,pProfile->m_usRegistrar2Port);
	m_UaMgr2=uaMgrNew();
	uaMgrSetUser(m_UaMgr2,m_UaUser);
	uaMgrSetCfg(m_UaMgr2,m_UaCfg);
	
	
	cseq=uaMgrGetCmdSeq(m_UaMgr);
	
	
	
	cseq=uaMgrGetCmdSeq(m_UaMgr);

	for (i = 0; i < MAX_DLG_COUNT; i++)
		dlgMAP[i] = NULL;

	m_DlgCount = 0; 
	
	/* Start Call Manager thread */
	m_bDoing = TRUE;

	DWORD dwThreadId;
	m_hCMThread = CreateThread( NULL, 0, CallManagerThreadFunc, this, 0, &dwThreadId);

	m_hUAThread = CreateThread( NULL, 0, UAThreadFunc, this, 0, &dwThreadId);

	m_bSingleCall = FALSE;
	m_bInitialized = TRUE;

	DebugMsg("\r\n****** Call Manager reset ******\r\n");
	
	tmpStr.Format("[CallManager] Public URI: %s", pProfile->m_PublicAddr);
	DebugMsg(tmpStr);
	
	tmpStr.Format("[CallManager] Contact URI: %s", pProfile->m_ContactAddr);
	DebugMsg(tmpStr);

	LeaveCriticalSection(&m_CallMgrCS);
}

/* Stop the CallManager */
void CCallManager::stop()
{
	EnterCriticalSection(&m_CallMgrCS);
	m_bDoing = false;	// flag to stop event process thread
    LeaveCriticalSection(&m_CallMgrCS);

	m_bInitialized = FALSE;

	if ( pSDPManager ) {
		delete pSDPManager;
		pSDPManager = NULL;
	}

	WaitForSingleObject( m_hCMThread, 50);
	if ( m_hCMThread ) {
		// wait thread terminated, max. 5 seconds
		// WaitForSingleObject( m_hCMThread, 5000);
		TerminateThread( m_hCMThread, 0);
		CloseHandle( m_hCMThread);
		m_hCMThread = NULL;
	}

	if ( m_hUAThread ) {
		TerminateThread( m_hUAThread, 0);
		CloseHandle( m_hUAThread);
		m_hUAThread = NULL;
	}
	
	if (m_EventMsgLst) {
		dxLstFree(m_EventMsgLst, (void (*)(void *))uaMsgFree);
		m_EventMsgLst = NULL;
	}
	
	if (m_UICmdMsgLst) {
		dxLstFree(m_UICmdMsgLst, (void (*)(void *))free);
		m_UICmdMsgLst = NULL;
	}
	
	if ( m_UaMgr ) {
		DisconnectAll();
		//UnRegister();
		Sleep(100);
		uaMgrFree(m_UaMgr);
			
		if (m_UaCfg)
		{
			uaCfgFree(m_UaCfg);
			m_UaCfg = NULL;
		}

		if (m_UaUser)
		{
			uaUserFree(m_UaUser);
			m_UaUser = NULL;
		}
		m_UaMgr = NULL;
		m_ActiveDlg = NULL;
	}

	if ( m_UaMgr2 ) {
		uaMgrFree(m_UaMgr2);
		m_UaMgr2 = NULL;
	}

	/* clear all falg, by sjhuang 2006/03/30 */
	m_ActiveDlg = NULL;
	m_bDoing = FALSE;
	m_bInitialized = FALSE;
	m_bRTPConnected = FALSE;
	m_bClient = FALSE;
	m_DlgCount = 0;
	m_bEarlyMedia = FALSE;

	uaLibClean();
	TCRSetMsgCB(NULL);
}

/* Event callback function for UACore */
void CCallManager::UAEvtCB(UAEvtType event, void* msg)
{
	if (!msg)
		return;
	
	dxLstPutTail( pCallManager->m_EventMsgLst, uaMsgDup((UaMsg)msg) );
}

// sam: for debug
UaDlg lastConnectedDlg = NULL;


/* Event process function */
void CCallManager::ProcessEvt(UaMsg uamsg)
{
	char* buf = NULL;
	SdpSess peersdp;
	SdpSess LocalSDP;
	int buflength;
	MyCString tmpStr;

	if (!uamsg)
		return;

	UACallStateType dlgState = uaMsgGetCallState(uamsg);
	SdpSess sdp = uaMsgContainSDP(uamsg)? uaMsgGetSdp(uamsg):NULL;
	UaContent content = uaMsgGetContent(uamsg);
	UAEvtType event = uaMsgGetEvtType(uamsg);
	UaDlg dlg = uaMsgGetDlg(uamsg);
	UaSub tmpSub = NULL;
	UaEvtpkg tmpEvtpkg = NULL;
	int dlgHandle = pCallManager->GetHandleFromDlg(dlg);

	const char *strURI;
	char* buddyURI=NULL;
	Buddy tmpBuddy; 

	if ( dlgHandle == -1 && IsDlgReleased(dlg) )
	{
		DebugMsg( _T("[UAEvtCB] Dialog already terminated") );
	}

	switch (event) {
	case UAMSG_TX_TIMEOUT:
		switch (dlgState) {
		case UACSTATE_REGISTERTIMEOUT:
			tmpStr.Format("[Dialog #%d] Register transaction timeout", dlgHandle);
			DebugMsg(tmpStr);
			/* notification */
			if ( CUAControl::GetControl() )
				CUAControl::GetControl()->Fire_RegistrationTimeout();
			break;

		case UACSTATE_TIMEOUT:
		default:
			tmpStr.Format("[Dialog #%d] Transaction timeout", dlgHandle);
			DebugMsg(tmpStr);
			/* notification */
			if ( CUAControl::GetControl() )
				CUAControl::GetControl()->Fire_TimeOut( dlgHandle );
			if (dlg)
				pCallManager->CallDisconnected(dlg, UACSTATE_TIMEOUT);
		}
		
		break;

	case UAMSG_TX_TRANSPORTERR:
		switch (dlgState) {
		case UACSTATE_REGISTERERR:
			tmpStr.Format("[Dialog #%d] Register transport error", dlgHandle);
			DebugMsg(tmpStr);
			/* notification */
			if ( CUAControl::GetControl() )
				CUAControl::GetControl()->Fire_RegistrationTxpErr();
			break;

		case UACSTATE_TRANSPORTERR:
		default:
			tmpStr.Format("[Dialog #%d] Transport error", dlgHandle);
			DebugMsg(tmpStr);
			/* notification */
			if ( CUAControl::GetControl() )
				CUAControl::GetControl()->Fire_TransportErr( dlgHandle );
			if (dlg)
				pCallManager->CallDisconnected(dlg, UACSTATE_TRANSPORTERR);
		}
		break;

	case UAMSG_TX_TERMINATED:
		tmpStr.Format("[Dialog #%d] Transaction terminated", dlgHandle);
		DebugMsg(tmpStr);
		/*
		switch (dlgState) {
		case UACSTATE_CANCEL:
			if (dlg)
				pCallManager->CallDisconnected(dlg, UACSTATE_DISCONNECT);
		}
		*/
		break;

	case UAMSG_REFER:
		if ( !pCallManager->m_ActiveDlg || (pCallManager->m_ActiveDlg == dlg) ) {
			tmpStr.Format("[Dialog #%d] has been transferred", dlgHandle);
			DebugMsg(tmpStr);
			pCallManager->MakeCallbyRefer(dlg);
		} else {
			tmpStr.Format("Error: Dialog #%d can't be transferred! There is still an active call in progress", dlgHandle);
			DebugMsg(tmpStr);
		}
		break;

	case UAMSG_INFO:
		DebugMsg("[UAEvtCB] UAMSG_INFO");
		break;

	case UAMSG_SUBSCRIBE:
		DebugMsg("[UAEvtCB] SUBSCRIBE state");
	
#ifdef _simple
		tmpSub = uaMsgGetSub(uamsg);
		if (tmpSub)
			tmpEvtpkg = uaSubGetEvtpkg(tmpSub);

		if (tmpEvtpkg) {
			tmpStr.Format("[UAEvtCB] event = %s", uaEvtpkgGetName(tmpEvtpkg));
			DebugMsg(tmpStr);
		
			if (stricmp(uaEvtpkgGetName(tmpEvtpkg), "presence") == 0)
					pPresManager->ProcessSub(tmpSub);
		
		}
#endif
		break;

	case UAMSG_NOTIFY:
		DebugMsg("[UAEvtCB] NOTIFY state");

#ifdef _simple
		tmpSub = uaMsgGetSub(uamsg);
		if (tmpSub)
			tmpEvtpkg = uaSubGetEvtpkg(tmpSub);

		if (tmpEvtpkg) {
			tmpStr.Format("[UAEvtCB] event = %s", uaEvtpkgGetName(tmpEvtpkg));
			DebugMsg(tmpStr);
//modified by alan,20050216		
			if ((stricmp(uaEvtpkgGetName(tmpEvtpkg), "presence") == 0)||(stricmp(uaEvtpkgGetName(tmpEvtpkg), "presence.winfo") == 0))
					pPresManager->ProcessNotify(tmpSub, content);
		
		}
#endif
		break;

	case UAMSG_MESSAGE:
		DebugMsg("[UAEvtCB] MESSAGE state");
//modified by alan,20050216
		strURI = uaDlgGetRemoteParty(dlg);

		if (!strURI)
			return ;

		buddyURI = pPresManager->URIfromPREStoSIP(strURI);
		tmpBuddy = pPresManager->FindMatchBuddy(buddyURI);

		if (tmpBuddy)
			if (GetBuddyBlock(tmpBuddy))//if blocked....ignore it
				break;
		
		if (content)
		{
			
			if (stricmp(uaContentGetType(content), "text/plain") == 0)
			{
				/* notification */
				if ( CUAControl::GetControl() )
				CUAControl::GetControl()->Fire_ReceivedText( CComBSTR(uaDlgGetRemoteParty(dlg)), 
															 CComBSTR((const char*)uaContentGetBody(content)) );
			}
			//receive offline message
			else if(stricmp(uaContentGetType(content),"Message/CPIM")==0)
			{
				//get date time and message from body
				buf = (char*)uaContentGetBody(content);
				if(buf!=NULL)
				{
					char *subStr,*splitStr,dateStr[32],msgStr[256];
					int dateStrLen,msgStrLen;
					//get date
					subStr = strstr(buf,"DateTime");
					if(subStr!=NULL)
					{
						splitStr = strstr(subStr,"\r\n");
						dateStrLen = strlen(subStr)-strlen(splitStr)-strlen("DateTime:");
						strncpy(dateStr,subStr+strlen("DateTime:"),dateStrLen);
						dateStr[dateStrLen]='\0';
					}
					//get message
					subStr = strstr(buf,"<body>");
					if(subStr!=NULL)
					{
						splitStr = strstr(subStr,"</body>");
						msgStrLen = strlen(subStr)-strlen(splitStr)-strlen("<body>\r\n");
						strncpy(msgStr,subStr+strlen("<body>\r\n"),msgStrLen);
						msgStr[msgStrLen]='\0';
					}
					if ( CUAControl::GetControl() )
						CUAControl::GetControl()->Fire_ReceivedOfflineMsg(CComBSTR(msgStr), CComBSTR(dateStr),CComBSTR(uaDlgGetRemoteParty(dlg)));
				}
				

			}
		}
		break;

	case UAMSG_SIP_INFO_RSP:
		DebugMsg("[UAEvtCB] Receive 200 OK INFO response");

		/* notification */
		if ( CUAControl::GetControl() )
		{
			CString strType;
			CString strBody;
			if (content)
			{
				strType = uaContentGetType(content);
				strBody = CString( (const char*)uaContentGetBody(content), uaContentGetLength(content));
			}

			CUAControl::GetControl()->Fire_InfoResponse( 
				CComBSTR( uaDlgGetRemoteParty(dlg)), 
				uaDlgGetLastestStatusCode( dlg),
				CComBSTR( strType),
				CComBSTR( strBody) );
		}
		break;

	case UAMSG_SIP_INFO_FAIL:
		DebugMsg("[UAEvtCB] Receive Error INFO response");

		/* notification */
		if ( CUAControl::GetControl() )
		{
			CString strType;
			CString strBody;
			if (content)
			{
				strType = uaContentGetType(content);
				strBody = CString( (const char*)uaContentGetBody(content), uaContentGetLength(content));
			}

			CUAControl::GetControl()->Fire_InfoResponse( 
				CComBSTR( uaDlgGetRemoteParty(dlg)), 
				uaDlgGetLastestStatusCode( dlg),
				CComBSTR( strType),
				CComBSTR( strBody) );
		}
		break;

	case UAMSG_SIP_INFO:
		DebugMsg("[UAEvtCB] Receive INFO request");

		/* notification */
		if ( CUAControl::GetControl() )
		{
			CString strType;
			CString strBody;
			if (content)
			{
				strType = uaContentGetType(content);
				strBody = CString( (const char*)uaContentGetBody(content), uaContentGetLength(content));
			}

			long nRspStatusCode = 200;
			CComBSTR bstrRspType;
			CComBSTR bstrRspBody;

			CUAControl::GetControl()->Fire_InfoRequest( 
				CComBSTR( uaDlgGetRemoteParty(dlg)), 
				CComBSTR( strType),
				CComBSTR( strBody),
				&nRspStatusCode,
				&bstrRspType,
				&bstrRspBody );

			// send SIP INFO response 
			CString strRspType = bstrRspType;
			CString strRspBody = bstrRspBody;
#ifdef _notautosend 
			UaContent rspContent = uaContentNew( 
				(char*)(LPCTSTR)strRspType, strRspBody.GetLength(), (void*)(LPCTSTR)strRspBody);
			uaMsgSendResponse( uamsg, (UAStatusCode)nRspStatusCode, rspContent);
			uaContentFree( rspContent);
#endif
		}
#ifdef _notautosend 
		else
		{
			// send 200 OK response for INFO
			UaContent rspContent = uaContentNew( "", 0, NULL);
			uaMsgSendResponse( uamsg, sipOK, rspContent);
			uaContentFree( rspContent);
		}
#endif
		break;

	case UAMSG_CALLSTATE:
		switch (dlgState) {
		case 	UACSTATE_IDLE:
			DebugMsg("[UAEvtCB] idle state");
			break;
		
		case	UACSTATE_DIALING:
			tmpStr.Format("[Dialog #%d] dialing...%s", dlgHandle, pCallManager->m_DialURL);
			DebugMsg(tmpStr);
			break;
		
		case	UACSTATE_PROCEEDING:
			tmpStr.Format("[Dialog #%d] proceeding state", dlgHandle);
			DebugMsg(tmpStr);

			/* notification */
			if ( CUAControl::GetControl() )
				CUAControl::GetControl()->Fire_Proceeding( dlgHandle );

			break;
		
		case	UACSTATE_OFFERING:
			DebugMsg("[UAEvtCB] New incoming call");
			dlgHandle = pCallManager->InsertDlgMAP(dlg);

			//uaDlgAnswer(dlg, UA_RINGING, Ringing, NULL);
			uaDlgAnswer(dlg, UA_100rel, Ringing, NULL);

			if ( !pCallManager->m_ActiveDlg ) {
				pCallManager->m_ActiveDlg = dlg;

				tmpStr.Format("[Dialog #%d] Alerting...", dlgHandle);
				DebugMsg(tmpStr);

				/* notification */
				if ( CUAControl::GetControl() )
					CUAControl::GetControl()->Fire_Alerting( dlgHandle );

			} else {
				if ( pCallManager->m_bSingleCall ) {
					pCallManager->DisconnectCall( dlgHandle );
					break;
				}
				tmpStr.Format("[Dialog #%d] Waiting...", dlgHandle);
				DebugMsg(tmpStr);
				
				/* notification */
				if ( CUAControl::GetControl() )
					CUAControl::GetControl()->Fire_Waiting( dlgHandle );

			}
			if (sdp) {
				pSDPManager->SetSDPbyDlg(dlg, sdpSessDup(sdp));
			}
			break;

		case	UACSTATE_OFFERING_REPLACE:
			DebugMsg("[UAEvtCB] New incoming call");
			dlgHandle = pCallManager->InsertDlgMAP(dlg);

			uaDlgAnswer(dlg, UA_RINGING, Ringing, NULL);
						
			if ( !pCallManager->m_ActiveDlg || (pCallManager->m_ActiveDlg == dlg) ) {
				tmpStr.Format("[Dialog #%d] Replace existing dialog", dlgHandle);
				DebugMsg(tmpStr);
				pCallManager->AcceptCall( dlgHandle );

			} else {
				if ( pCallManager->m_bSingleCall ) {
					pCallManager->DisconnectCall( dlgHandle );
					break;
				}
				tmpStr.Format("[Dialog #%d] Waiting...", dlgHandle);
				DebugMsg(tmpStr);

				/* notification */
				if ( CUAControl::GetControl() )
					CUAControl::GetControl()->Fire_Waiting( dlgHandle );

			}
			if (sdp) {
				pSDPManager->SetSDPbyDlg(dlg, sdpSessDup(sdp));
			}
			break;
		
		case	UACSTATE_RINGBACK:
			tmpStr.Format("[Dialog #%d] peer is ringing", dlgHandle);
			DebugMsg(tmpStr);
			
			if (sdp) {
				pSDPManager->SetSDPbyDlg(dlg, sdpSessDup(sdp));
			}
			peersdp = pSDPManager->GetSDPbyDlg(dlg);
			
			if( peersdp )
			{
				tmpStr.Format("[Dialog #%d] 183 RingBack", dlgHandle);
				DebugMsg(tmpStr);

				m_bClient = FALSE;
				//pCallManager->CallEstablished(dlg, peersdp);
				pCallManager->CallEstablished(dlg, peersdp, 183);
			}
			else
			{
				/* notification */
				if ( CUAControl::GetControl() )
					CUAControl::GetControl()->Fire_Ringback( dlgHandle );
			}

			break;
		
		case	UACSTATE_BUSY:
			tmpStr.Format("[Dialog #%d] peer is busy", dlgHandle);
			DebugMsg(tmpStr);

			/* notification */
			if ( CUAControl::GetControl() )
				CUAControl::GetControl()->Fire_Busy( dlgHandle );

			if (dlg)
				pCallManager->CallDisconnected(dlg, UACSTATE_BUSY);
			break;

		case	UACSTATE_REJECTED:
			tmpStr.Format("[Dialog #%d] call rejected", dlgHandle);
			DebugMsg(tmpStr);
			
			/* notification */
			if ( CUAControl::GetControl() )
				CUAControl::GetControl()->Fire_Reject( dlgHandle );
			
			if (dlg)
				pCallManager->CallDisconnected(dlg, UACSTATE_REJECTED);
			break;
				
		case	UACSTATE_CANCEL:
			tmpStr.Format("[Dialog #%d] cancel state", dlgHandle);
			DebugMsg(tmpStr);
			break;

		case	UACSTATE_REJECT:
			tmpStr.Format("[Dialog #%d] reject state", dlgHandle);
			DebugMsg(tmpStr);
			break;
		
		case	UACSTATE_ACCEPT:
			tmpStr.Format("[Dialog #%d] accept state", dlgHandle);
			DebugMsg(tmpStr);
			break;

		case  UACSTATE_SDPCHANGING:
				if (m_bClient)
					break;

				pMediaManager->Media_RTP_Stop();

				#ifdef _FOR_VIDEO
					pVideoManager->RTPDisconnect();
					pVideoManager->StopRemoteVideo();
				#endif

				m_bRTPConnected = FALSE;

				if (sdp) {
					pSDPManager->SetSDPbyDlg(dlg, sdpSessDup(sdp));
				}

				LocalSDP = pSDPManager->GetLocalSDPbyDlg(dlg);
				uaDlgAnswer(dlg, UA_ANSWER_CALL, sipOK, LocalSDP);
				uaSDPFree(LocalSDP);
				break;

		case	UACSTATE_CONNECTED:

			/* Act as UAS, occurs when received ACK
			   Act as UAC, occurs when received 200 OK */
			   
			// sam: prevent a bug from UACore, it will fire duplicate CONNECTED event
			//      if UAC receive 200 OK after receive REFER
			
			/*
			if ( lastConnectedDlg == dlg)
				break;
			else
				lastConnectedDlg = dlg;
			 */


			tmpStr.Format("[Dialog #%d] call connected", dlgHandle);
			DebugMsg(tmpStr);

			m_bClient = FALSE;
			
			if (sdp) {
				pSDPManager->SetSDPbyDlg(dlg, sdpSessDup(sdp));
			}
			peersdp = pSDPManager->GetSDPbyDlg(dlg);
			pCallManager->CallEstablished(dlg, peersdp, 200);
			break;
		
		case	UACSTATE_DISCONNECT:
			tmpStr.Format("[Dialog #%d] disconnect state", dlgHandle);
			DebugMsg(tmpStr);

			if (dlg)
				pCallManager->CallDisconnected(dlg, UACSTATE_DISCONNECT);
			break;

		case	UACSTATE_REGISTER:
			tmpStr.Format("[UAEvtCB] Registering...", dlgHandle);
			DebugMsg(tmpStr);
			break;

		case	UACSTATE_REGISTER_AUTHN:
			DebugMsg("[UAEvtCB] Need authentication");

			/* notification */
			if ( CUAControl::GetControl() )
				CUAControl::GetControl()->Fire_NeedAuthentication( CComBSTR("RELAM NAME") );

			break;

		case	UACSTATE_REGISTERED:
			DebugMsg("[UAEvtCB] Registration done");

			/* notification */
			if ( CUAControl::GetControl() )
				CUAControl::GetControl()->Fire_RegistrationDone();

			buflength = BUFLEN;
			buf = (char*)malloc(BUFLEN);
			
			if ( buf ) {
				if ( RC_OK == uaMsgGetRspContactAddr( uamsg,buf,&buflength ) )
					DebugMsg((const char*)buf);
				else 
					DebugMsg("[UAEvtCB] No Contact URLs");
				free(buf);
				buf = NULL;
			}
			break;

		case	UACSTATE_REGISTERFAIL:
			DebugMsg("[UAEvtCB] Registration fail");
			
			/* notification */
			if ( CUAControl::GetControl() )
				CUAControl::GetControl()->Fire_RegistrationFail( uaDlgGetLastestStatusCode(dlg) );
			
			break;

		case	UACSTATE_ONHOLDING:
			tmpStr.Format("[Dialog #%d] holding call", dlgHandle);
			DebugMsg(tmpStr);
			break;
		
		case	UACSTATE_ONHELD:
			tmpStr.Format("[Dialog #%d] call held", dlgHandle );
			DebugMsg(tmpStr);
			
			// sam: prevent a bug from UACore, it will fire duplicate CONNECTED event
			//      if UAC receive 200 OK after receive REFFER
			lastConnectedDlg = NULL;

			/* notification */
			if ( CUAControl::GetControl() )
				CUAControl::GetControl()->Fire_Held( dlgHandle );

			if ( pCallManager->m_ActiveDlg == dlg) {
				pCallManager->m_ActiveDlg = NULL;
				pMediaManager->Media_RTP_Stop();

				#ifdef _FOR_VIDEO
					pVideoManager->RTPDisconnect();
					pVideoManager->StopRemoteVideo();
				#endif

				m_bRTPConnected = FALSE;
			}
			break;

		case	UACSTATE_UNHOLD:
			if ( !pCallManager->m_ActiveDlg || (pCallManager->m_ActiveDlg == dlg) ) {
				tmpStr.Format("[Dialog #%d] call unhold", dlgHandle);
				DebugMsg(tmpStr);
				pCallManager->AcceptCall( dlgHandle );
			} else {
				tmpStr.Format("[Dialog #%d] unhold request wait", dlgHandle);
				DebugMsg(tmpStr);
				/* notification */
				if ( CUAControl::GetControl() )
					CUAControl::GetControl()->Fire_Waiting( dlgHandle );

			}
			if (sdp) {
				pSDPManager->SetSDPbyDlg(dlg, sdpSessDup(sdp));
			}
			break;

		case	UACSTATE_ONHOLDPENDCONF:
			break;

		case	UACSTATE_ONHOLDPENDTRANSFER:
			break;

		case	UACSTATE_CONFERENCE:
			break;

		case	UACSTATE_SPECIALINFO:
			break;
			
		case	UACSTATE_RETRIEVING:
			break;

		default:
			tmpStr.Format("[Dialog #%d] WARNING! Unknown State", dlgHandle);
			DebugMsg(tmpStr);
			break;
		}
		break;
	case UAMSG_REPLY:
		break;
	//add by alan,20050223
	case UAMSG_REGISTER_EXPIRED:
		if ( CUAControl::GetControl() )
				CUAControl::GetControl()->Fire_RegisterExpired();
		break;
	default:
		break;
	}

	if (uamsg) {
		uaMsgFree(uamsg);
		uamsg = NULL;
	}
	if (sdp) {
		uaSDPFree(sdp);
		sdp = NULL;
	}
}

/* Make a new call to DialURL - revised - */
RCODE CCallManager::MakeCall(MyCString DialURL, int& dlgHandle)
{
	MyCString tmpStr;

	if (m_DlgCount == MAX_DLG_COUNT) {
		dlgHandle = -1;
		return RC_ERROR;
	}

	if (m_bSingleCall && (m_DlgCount == 1) ) {
		dlgHandle = -1;
		return RC_ERROR;
	}
		
	if (RC_ERROR == ValidateURL(DialURL)) {
		dlgHandle = -1;
		return RC_ERROR;
	}

	if (m_ActiveDlg) {
		DebugMsg( _T("Error: Can't make another call! Please hold active call first")) ;
		dlgHandle = -1;
		return RC_ERROR;
	}

	UaDlg dlg = uaDlgNew(m_UaMgr);
	dlgHandle = InsertDlgMAP(dlg);
	m_ActiveDlg = dlg;

	DebugMsg(DialURL);

	m_DialURL = DialURL;

	/* notification */
	if ( CUAControl::GetControl() )
		CUAControl::GetControl()->Fire_Dialing( dlgHandle );

	SdpSess LocalSDP = pSDPManager->GetLocalSDPbyDlg(dlg);
	
	uaDlgSetUserPrivacy(dlg,pProfile->m_bAnonFrom);

	if (pProfile->m_bUsePreferredID)
		uaDlgSetPPreferredID(dlg, pProfile->m_strPreferredID);

	if (pProfile->m_bUseSessionTimer)
		uaDlgSetSETimer(dlg, pProfile->m_uSessionTimer);
	else
		uaDlgSetSETimer(dlg, 0);

	RCODE retcode = uaDlgDial(dlg,DialURL,LocalSDP);
	uaSDPFree(LocalSDP);

	if (retcode != RC_OK) {
		tmpStr.Format("Error: %s", uaCMRet2Phase((UARetCode)retcode) );
		DebugMsg(tmpStr);
	}
	return retcode;
}

/* Make call by Refer - revised - */
RCODE CCallManager::MakeCallbyRefer(UaDlg ReferDlg)
{
	char dest[128] = {'\0'};
	int destlen = 128;
	MyCString tmpStr;
	
	int	dlgHandle = GetHandleFromDlg(ReferDlg);

	if ( dlgHandle == -1 ) {
		DebugMsg( _T("Error: Can't make call by refer! Dialog handle doesn't exist") );
		return RC_ERROR;
	}

	uaDlgGetReferTo(ReferDlg,dest,&destlen);
	tmpStr.Format(_T("[Dialog #%d] transferred to %s"), dlgHandle, dest);;
	DebugMsg(tmpStr);

	/* notification */
	if ( CUAControl::GetControl() )
			CUAControl::GetControl()->Fire_Transferred( dlgHandle, CComBSTR(dest) );

	UaDlg dlg = uaDlgNew(m_UaMgr);
	InsertDlgMAP(dlg);

	/* If is Referdlg is Active dialog, deactive it and stop media */
	if (m_ActiveDlg == ReferDlg) {
		tmpStr.Format("[Dialog #%d] stop media by refer", dlgHandle );
		DebugMsg(tmpStr);
		
		pMediaManager->Media_RTP_Stop();
		pMediaManager->StopPlaySound();

		#ifdef _FOR_VIDEO
			pVideoManager->RTPDisconnect();
			pVideoManager->StopRemoteVideo();
		#endif

		m_bRTPConnected = FALSE;
	}

	SdpSess LocalSDP = pSDPManager->GetLocalSDPbyDlg(dlg);
	RCODE retcode = uaDlgDialRefer(dlg, dest, ReferDlg, LocalSDP);
	uaSDPFree(LocalSDP);

	m_ActiveDlg = dlg;

	/* modified by ljchuang 08/13/2003 */
	/* CallDisconnected(ReferDlg, uaDlgGetState(ReferDlg) ); */
	/* DisconnectCall(dlgHandle); */


	if (retcode == RC_OK) {
		m_ActiveDlg = dlg;
	} else {
		tmpStr.Format("Error: %s", uaCMRet2Phase((UARetCode)retcode) );
		DebugMsg(tmpStr);
	}

	return retcode;
}

/* Cancel active dialing dialog - revised - */
RCODE CCallManager::CancelCall()
{
	MyCString tmpStr;

	int dlgHandle = GetHandleFromDlg(m_ActiveDlg);

	if ( dlgHandle == -1 ) {
		DebugMsg("Error: Can't cancel call! Dialog handle doesn't exist");
		return RC_ERROR;
	}

	return DisconnectCall( dlgHandle );

}

/* Disconnect dialog of dlgHandle - revised - */
RCODE CCallManager::DisconnectCall(int dlgHandle)
{
	MyCString tmpStr;
	RCODE retcode;
	UaDlg dlg = GetDlgFromHandle(dlgHandle);

	//if( pMediaManager->m_WaveType == WAV_G723 )
	//{
	//	AfxMessageBox("disconnectCall");
	//}

	if (dlg) {
		UACallStateType dlgState = uaDlgGetState(dlg);
		switch (dlgState) {
		case UACSTATE_DIALING:
			retcode = CallDisconnected(dlg, UACSTATE_DIALING);
			break;

		case UACSTATE_PROCEEDING:			
		case UACSTATE_RINGBACK:
			tmpStr.Format("[Dialog #%d] user cancel call", dlgHandle );
			DebugMsg(tmpStr);
			retcode = uaDlgCANCEL(dlg);
			break;

		case UACSTATE_OFFERING:
		case UACSTATE_OFFERING_REPLACE:
			tmpStr.Format("[Dialog #%d] user reject call", dlgHandle );
			DebugMsg(tmpStr);
			retcode =  uaDlgAnswer(dlg, UA_REJECT_CALL, Busy_Here, NULL);
			break;

		default:
			tmpStr.Format("[Dialog #%d] user disconnect call", dlgHandle );
			DebugMsg(tmpStr);

			//if( pMediaManager->m_WaveType == WAV_G723 )
			//{
			//	AfxMessageBox("uaDlgDisconnect");
			//}
			retcode = uaDlgDisconnect(dlg);

			tmpStr.Format("[Dialog #%d] after user disconnect call ", dlgHandle );
			DebugMsg(tmpStr);

		}
		if (retcode != RC_OK) {
			tmpStr.Format("Error: %s", uaCMRet2Phase((UARetCode)retcode) );
			DebugMsg(tmpStr);
		}
	} else {
		DebugMsg("Error: Can't drop call! Dialog not found");
		retcode = RC_ERROR;
	}

	tmpStr.Format("[Dialog #%d] leave disconnect call", dlgHandle );
	DebugMsg(tmpStr);

	return retcode;
}

/* Accept dialog of dlgHandle - revised - */
RCODE CCallManager::AcceptCall(int dlgHandle)
{
	MyCString tmpStr;
	RCODE retcode;
	UaDlg dlg = GetDlgFromHandle(dlgHandle);
	UACallStateType dlgState = uaDlgGetState(dlg);

	if ( m_ActiveDlg && ( m_ActiveDlg != dlg ) ) {
		DebugMsg( _T("Error: can't answer call! Please hold active call first")) ;
		return RC_ERROR;
	}
	if (dlg) {
		tmpStr.Format("[Dialog #%d] user answer call", dlgHandle );
		DebugMsg(tmpStr);

		SdpSess LocalSDP = pSDPManager->GetLocalSDPbyDlg(dlg);
		retcode = uaDlgAnswer(dlg, UA_ANSWER_CALL, sipOK, LocalSDP);
		uaSDPFree(LocalSDP);

		if (retcode == RC_OK) {
			m_ActiveDlg = dlg;
		} else {
			tmpStr.Format("Error: %s", uaCMRet2Phase((UARetCode)retcode) );
			DebugMsg(tmpStr);
		}
	} else {
		DebugMsg("Error: Can't answer call! Dialog not found");
		retcode = RC_ERROR;
	}
	return retcode;
}

/* Reject dialog of dlgHandle - revised - */
RCODE CCallManager::RejectCall(int dlgHandle)
{
	MyCString tmpStr;
	RCODE retcode;
	UaDlg dlg = GetDlgFromHandle(dlgHandle);

	if (dlg) {
		tmpStr.Format("[Dialog #%d] user reject call", dlgHandle );
		DebugMsg(tmpStr);

		retcode =  uaDlgAnswer(dlg, UA_REJECT_CALL, Busy_Here, NULL);
		if (retcode != RC_OK) {
			tmpStr.Format("Error: %s", uaCMRet2Phase((UARetCode)retcode) );
			DebugMsg(tmpStr);
		}
	} else {
		DebugMsg("Error: Can't reject call! Dialog not found");
		retcode = RC_ERROR;
	}
	return retcode;
}

/* Post call established procedure - revised - */
RCODE CCallManager::CallEstablished(UaDlg dlg, SdpSess sdp, short status)
{
	int AudioPort, VideoPort = 0;
	CODEC_NUMBER AudioCodecChoice;
	char tmpBuf[128];
	RCODE retcode;
	MyCString tmpStr;

	if( m_bEarlyMedia )
	{
		m_bEarlyMedia = FALSE;
		m_bRTPConnected = FALSE;
	}
	
	if (m_bRTPConnected)
		return RC_ERROR;
	
	memset(tmpBuf, 0, 128);

	pMediaManager->StopPlaySound();
	pMediaManager->Media_RTP_Stop();

	int dlgHandle = GetHandleFromDlg(dlg);

	if ( dlgHandle == -1 ) {
		DebugMsg( _T("Fatal Error: Can't establish media! Dialog handle doesn't exist") );
		return RC_ERROR;
	}

	if ( sdp == NULL ) {
		tmpStr.Format("[Dialog #%d] Error: No remote SDP, fail to start media", dlgHandle );
		DebugMsg(tmpStr);
		DisconnectCall( dlgHandle );
		return RC_ERROR;
	}

	/* Get remote audio channel parameter */
	if ((retcode = uaSDPGetMediaAddr(sdp, UA_MEDIA_AUDIO, tmpBuf, 128)) != RC_OK) {
		/* Get remote SDP IP address */
		if ((retcode = uaSDPGetDestAddress(sdp, tmpBuf, 128)) != RC_OK) {
			tmpStr.Format("[Dialog #%d] Error: Can't get remote SDP address, fail to start media", dlgHandle );
			DebugMsg(tmpStr);
			DisconnectCall( dlgHandle );
			return retcode;
		}
	}
	AudioPort = uaSDPGetMediaPort(sdp, UA_MEDIA_AUDIO);
	AudioCodecChoice = pSDPManager->GetAudioCodecChoiceNum(sdp);

	if ( AudioCodecChoice == CODEC_NONE ) {
		tmpStr.Format("[Dialog #%d] Error: No match media, fail to start media", dlgHandle );
		DebugMsg(tmpStr);
		DisconnectCall( dlgHandle );
		return RC_ERROR;
	}

	tmpStr.Format("[Dialog #%d] start media", dlgHandle );
	DebugMsg(tmpStr);
	
	/* Http Tunnel Server channel, by sjhuang 20060120 */
	// ToDo:
	if( pProfile->m_bUseHttpTunnel ) {
		SdpSess LocalSDP = pSDPManager->GetLocalSDPbyDlg(dlg);
		int LocalAudioPort = uaSDPGetMediaPort(LocalSDP, UA_MEDIA_AUDIO);
		CString strLocalPort,strRemotePort;
		strLocalPort.Format("%d",LocalAudioPort);
		strRemotePort.Format("%d",AudioPort);
		reqHTTPTunnel(TRUE,strLocalPort,CString(tmpBuf),strRemotePort);
	}

	/* Connect remote audio channel */
	pMediaManager->SetMediaParameter(AudioCodecChoice);

	DebugMsg("1\n");
	if( pProfile->m_bUseHttpTunnel ) { /* Added by sjhuang 20060120 */
		CString strTunnelIP;
		int iTunnelPort;
		strTunnelIP = AfxGetApp()->GetProfileString( "Http_Tunnel", "HttpTunnel_Addr");//GetUAComRegString( "Http_Tunnel","HttpTunnel_Addr");
		iTunnelPort = AfxGetApp()->GetProfileInt("Http_Tunnel", "HttpTunnel_Port",8080);;//GetUAComRegDW( "Http_Tunnel","HttpTunnel_Port",8080);
		pMediaManager->RTPPeerConnect(strTunnelIP,iTunnelPort+1);
	} else {
		pMediaManager->RTPPeerConnect(CString(tmpBuf),AudioPort);
	}

	DebugMsg("2\n");
#ifdef _FOR_VIDEO

	// added by sjhuang 2006/03/03, prevent first connect to non-Video, causing remote size having bug...
	CUAProfile* pProfile = CUAProfile::GetProfile();
	pVideoManager->SetMediaParameter(-1, pProfile->m_VideoSizeFormat);

	CODEC_NUMBER VideoCodecChoice = pSDPManager->GetVideoCodecChoiceNum(sdp);
	if ( (VideoCodecChoice != CODEC_NONE) && pProfile->m_bUseVideo ) {
		/* Get remote video channel parameter */
		if ((retcode = uaSDPGetMediaAddr(sdp, UA_MEDIA_VIDEO, tmpBuf, 128)) != RC_OK) {
			/* Get remote SDP IP address */
			if ((retcode = uaSDPGetDestAddress(sdp, tmpBuf, 128)) != RC_OK) {
				tmpStr.Format("[Dialog #%d] Error: Can't get remote SDP address, fail to start media", dlgHandle );
				DebugMsg(tmpStr);
				DisconnectCall( dlgHandle );
				return retcode;
			}
		}
		VideoPort = uaSDPGetMediaPort(sdp, UA_MEDIA_VIDEO);

		unsigned int VideoSize = pSDPManager->GetVideoCodecChoiceSizeFormat(sdp);

		if (VideoPort != 0) {
		
			DebugMsg("2.6\n");
			/* Http Tunnel Server channel, added by sjhuang 20060120 */
			// ToDo:
			if( pProfile->m_bUseHttpTunnel ) {
				DebugMsg("2.7\n");
				SdpSess LocalSDP = pSDPManager->GetLocalSDPbyDlg(dlg);
				int LocalVideoPort = uaSDPGetMediaPort(LocalSDP, UA_MEDIA_VIDEO);
				CString strLocalPort,strRemotePort;
				strLocalPort.Format("%d",LocalVideoPort);
				strRemotePort.Format("%d",VideoPort);
				reqHTTPTunnel(TRUE,strLocalPort,CString(tmpBuf),strRemotePort);
				//CString tmp;
				//tmp.Format("local video port %d",LocalVideoPort);
				//AfxMessageBox(tmp);
				DebugMsg("2.8\n");
			}

			DebugMsg("2.9\n");

			/* Connect remote video channel */
			if (VideoSize!=2 || VideoSize!=3) /* 2:CIF , 3:QCIF */
				VideoSize = pProfile->m_VideoSizeFormat;
			
			pVideoManager->SetMediaParameter(VideoCodecChoice, VideoSize);
		
			DebugMsg("2.11\n");
			if( pProfile->m_bUseHttpTunnel ) { /* Added by sjhuang 20060120 */
				DebugMsg("2.12\n");
				CString strTunnelIP;
				int iTunnelPort;
				strTunnelIP = AfxGetApp()->GetProfileString( "Http_Tunnel", "HttpTunnel_Addr");//GetUAComRegString( "Http_Tunnel","HttpTunnel_Addr");
				iTunnelPort = AfxGetApp()->GetProfileInt("Http_Tunnel", "HttpTunnel_Port",8080);;//GetUAComRegDW( "Http_Tunnel","HttpTunnel_Port",8080);
				pVideoManager->RTPPeerConnect(strTunnelIP,iTunnelPort+1);
				DebugMsg("2.13\n");
			} else {
				char temp[255];
				sprintf(temp,"2.14:%s %d\n",tmpBuf,VideoPort);
				DebugMsg(temp);
				
				pVideoManager->RTPPeerConnect(CString(tmpBuf),VideoPort);
			}
			DebugMsg("2.15\n");

		}
        // sam: do not auto start remote video now
		/*
		// Start remote video display

		POINT p;
		p.x = 0;
		p.y = 460;

		pVideoManager->StartRemoteVideo( CUAControl::GetControl()->m_hWnd, p, 1);
		*/
	}
#endif
	
	/* Used to calculate call elapsed time */
	m_ElapsedHours = 0;
	m_ElapsedMins = 0;
	m_ElapsedSecs = 0;


	if( status==200 )
	{
		/* notification */
		if ( CUAControl::GetControl() ) 
			CUAControl::GetControl()->Fire_Connected( dlgHandle );
	}
	else if( status==183 )
	{
		m_bEarlyMedia = TRUE;
		if ( CUAControl::GetControl() ) 
			CUAControl::GetControl()->Fire_RingBack183( dlgHandle );
	}

	m_bRTPConnected = TRUE;


	return RC_OK;
}

/* Post call disconnected procedure - revised - */
RCODE CCallManager::CallDisconnected(UaDlg dlg, UACallStateType dlgState)
{
	RCODE retcode;
	char tmpBuf[128];
	MyCString tmpStr; 


	int dlgHandle = GetHandleFromDlg(dlg);

	if (!dlg || (dlgHandle == -1))
		return RC_ERROR;

	if ( lastConnectedDlg == dlg )
		lastConnectedDlg = NULL;

	tmpStr.Format("[Dialog #%d] dialog state: %s", dlgHandle, UACallStateToCStr(dlgState) );
	DebugMsg(tmpStr);

	/* If is Active dialog, deactive it and stop media */
	if (m_ActiveDlg == dlg) {
		tmpStr.Format("[Dialog #%d] stop media", dlgHandle );
		DebugMsg(tmpStr);
		
		/* Deactive dialog */
		m_ActiveDlg = NULL;

		pMediaManager->Media_RTP_Stop();
		pMediaManager->StopPlaySound();

		#ifdef _FOR_VIDEO
			pVideoManager->RTPDisconnect();
			pVideoManager->StopRemoteVideo();
		#endif

		m_bRTPConnected = FALSE;

		/* Http Tunnel Server channel, by sjhuang 20060120 */
		// ToDo:
		/* Get remote audio channel parameter */
		SdpSess peersdp = pSDPManager->GetSDPbyDlg(dlg);// LocalSDPbyDlg(dlg);
		if ((retcode = uaSDPGetMediaAddr(peersdp, UA_MEDIA_AUDIO, tmpBuf, 128)) != RC_OK) {
			/* Get remote SDP IP address */
			if ((retcode = uaSDPGetDestAddress(peersdp, tmpBuf, 128)) != RC_OK) {
				tmpStr.Format("[Dialog #%d] Error: Can't get remote SDP address, fail to start media", dlgHandle );
				DebugMsg(tmpStr);
				//DisconnectCall( dlgHandle );
				//return retcode;
			}
		}
		CString strLocalPort,strRemotePort;
		int AudioPort = uaSDPGetMediaPort(peersdp, UA_MEDIA_AUDIO);
		int VideoPort = uaSDPGetMediaPort(peersdp, UA_MEDIA_VIDEO);
		
		if( pProfile->m_bUseHttpTunnel ) {
	
			SdpSess LocalSDP = pSDPManager->GetLocalSDPbyDlg(dlg);
			if( AudioPort!=-1 ) {
			
				int LocalAudioPort = uaSDPGetMediaPort(LocalSDP, UA_MEDIA_AUDIO);
				strLocalPort.Format("%d",LocalAudioPort);
				strRemotePort.Format("%d",AudioPort);
				reqHTTPTunnel(FALSE,strLocalPort,CString(tmpBuf),strRemotePort);
			}

			if ( pProfile->m_bUseVideo ) { // close video port
				if( VideoPort!=-1 ) {
			
					int LocalVideoPort = uaSDPGetMediaPort(LocalSDP, UA_MEDIA_VIDEO);
					strLocalPort.Format("%d",LocalVideoPort);
					strRemotePort.Format("%d",VideoPort);
					reqHTTPTunnel(FALSE,strLocalPort,CString(tmpBuf),strRemotePort);
				}
			}

			//CString tmp;
			//tmp.Format("close tunnel");
			//AfxMessageBox(tmp);
		}

	}


	tmpStr.Format("[Dialog #%d] call disconnected: %d", dlgHandle, uaDlgGetLastestStatusCode(dlg) );
	DebugMsg(tmpStr);

	/* notification */
	if ( CUAControl::GetControl() ) {
		CUAControl::GetControl()->Fire_Disconnected( dlgHandle, uaDlgGetLastestStatusCode(dlg) );
	}

	/* Delete SDP associated with the dlg */
	pSDPManager->DelSDPbyDlg(dlg);

	/* Delete Dialog handle binding */
	DeleteDlgMAP(dlg);
	
	/* Release Dialog object */
	if (dlg != NULL) {
		uaDlgRelease(dlg);
		dlg = NULL;
	}

	// clear early media flag
	m_bEarlyMedia = FALSE;

	return RC_OK;
}

RCODE CCallManager::Register()
{
	MyCString tmpStr;
	RCODE retcode;

	DebugMsg("[CallManager] Performing registration");

	if (pProfile->m_bUseRegistrar) {
		if ( (retcode = uaMgrRegister(m_UaMgr, UA_REGISTER, pProfile->m_ContactAddr)) != RC_OK )
			goto EXIT_FUNC;
	}
	if (pProfile->m_bUseRegistrar2) {
		if ( (retcode = uaMgrRegister(m_UaMgr2, UA_REGISTER, pProfile->m_ContactAddr)) != RC_OK )
			goto EXIT_FUNC;
	}

EXIT_FUNC:

	if (retcode != RC_OK) {
		tmpStr.Format("Error: %s", uaCMRet2Phase((UARetCode)retcode) );
		DebugMsg(tmpStr);
	}
	return retcode;
}

RCODE CCallManager::Query()
{
	MyCString tmpStr;
	RCODE retcode;
	
	DebugMsg("[CallManager] Performing registration query");

	if (pProfile->m_bUseRegistrar) {
		if ( (retcode = uaMgrRegister(m_UaMgr, UA_REGISTER_QUERY, pProfile->m_ContactAddr)) != RC_OK )
			goto EXIT_FUNC;
	}

	if (pProfile->m_bUseRegistrar2) {
		if ( (retcode = uaMgrRegister(m_UaMgr2, UA_REGISTER_QUERY, pProfile->m_ContactAddr)) != RC_OK )
			goto EXIT_FUNC;
	}

EXIT_FUNC:

	if (retcode != RC_OK) {
		tmpStr.Format("Error: %s", uaCMRet2Phase((UARetCode)retcode) );
		DebugMsg(tmpStr);
	}
	return retcode;
}

RCODE CCallManager::UnRegister()
{
	MyCString tmpStr;
	RCODE retcode;
	
	DebugMsg("[CallManager] Performing un-registration");

	if (pProfile->m_bUseRegistrar) {
		if ( (retcode = uaMgrRegister(m_UaMgr, UA_UNREGISTERALL, NULL)) != RC_OK )
			goto EXIT_FUNC;
	}
	if (pProfile->m_bUseRegistrar2) {
		if ( (retcode = uaMgrRegister(m_UaMgr2, UA_UNREGISTERALL, NULL)) != RC_OK )
			goto EXIT_FUNC;
	}

EXIT_FUNC:

	if (retcode != RC_OK) {
		tmpStr.Format("Error: %s", uaCMRet2Phase((UARetCode)retcode) );
		DebugMsg(tmpStr);
	}
	return retcode;
}

/* Hold active dialog - revised - */
RCODE CCallManager::Hold()
{
	RCODE retcode;
	MyCString tmpStr;

	int dlgHandle = GetHandleFromDlg(m_ActiveDlg);

	if ( dlgHandle == -1 ) {
		DebugMsg("Error: Can't hold call! Dialog handle doesn't exist");
		return RC_ERROR;
	}

	tmpStr.Format("[Dialog #%d] hold call", dlgHandle );
	DebugMsg(tmpStr);
	
	retcode = uaDlgHold(m_ActiveDlg);
	if (retcode != RC_OK) {
		tmpStr.Format("Error: %s", uaCMRet2Phase((UARetCode)retcode) );
		DebugMsg(tmpStr);
	}
	return retcode;
}

/* UnHold dialog of dlgHandle - revised - */
RCODE CCallManager::UnHold(int dlgHandle)
{
	MyCString tmpStr;
	RCODE retcode;
	UaDlg dlg = GetDlgFromHandle(dlgHandle);

	if (m_ActiveDlg) {
		DebugMsg("Error: Can't unhold call! Please hold active call first");
		return RC_ERROR;
	}
	if (dlg) {
		tmpStr.Format("[Dialog #%d] unhold call", dlgHandle );
		DebugMsg(tmpStr);
		
		SdpSess LocalSDP = pSDPManager->GetLocalSDPbyDlg(dlg);
		retcode = uaDlgUnHold(dlg, LocalSDP);
		uaSDPFree(LocalSDP);
	
		if (retcode == RC_OK) {
			m_ActiveDlg = dlg;
		} else {
			tmpStr.Format("Error: %s", uaCMRet2Phase((UARetCode)retcode) );
			DebugMsg(tmpStr);
		}
	} else {
		DebugMsg("Error: Can't unhold call! Dialog not found");
		retcode = RC_ERROR;
	}
	return retcode;
}

/* Attended transfer dialog1 to dialog2 - revised - */
RCODE CCallManager::AttendTransfer(int dlgHandle1, int dlgHandle2)
{
	MyCString tmpStr;
	RCODE retcode;
	UaDlg dlg1 = GetDlgFromHandle(dlgHandle1);
	UaDlg dlg2 = GetDlgFromHandle(dlgHandle2);

	if (dlg1 && dlg2) {
		tmpStr.Format("[Dialog #%d] Attended transfer to dialog #%d", dlgHandle1, dlgHandle2 );
		DebugMsg(tmpStr);
	
		UaDlg xferDlg = uaDlgNew(m_UaMgr);
		int	dlgHandle = InsertDlgMAP(xferDlg);

		retcode = uaDlgAttendTransfer(dlg1, dlg2, xferDlg);
		if (retcode != RC_OK) {
			tmpStr.Format("Error: %s", uaCMRet2Phase((UARetCode)retcode) );
			DebugMsg(tmpStr);
		}
	} else {
		DebugMsg("Error: Can't transfer call! Dialog not found");
		retcode = RC_ERROR;
	}
	return retcode;
}

/* UnAttended transfer dialog of dlgHandle - revised - */
RCODE CCallManager::UnAttendTransfer(int dlgHandle, MyCString XferURL)
{
	MyCString tmpStr;
	RCODE retcode;
	UaDlg dlg = GetDlgFromHandle(dlgHandle);

	if (dlg) {
		tmpStr.Format("[Dialog #%d] UnAttended transfer to %s", dlgHandle, XferURL );
		DebugMsg(tmpStr);
		
		retcode = uaDlgUnAttendTransfer(dlg, XferURL);
		if (retcode != RC_OK) {
			tmpStr.Format("Error: %s", uaCMRet2Phase((UARetCode)retcode) );
			DebugMsg(tmpStr);
		}
	} else {
		DebugMsg("Error: Can't transfer call! Dialog not found");
		retcode = RC_ERROR;
	}
	return retcode;
}

RCODE CCallManager::DisconnectAll()
{
	DxLst dlgLst = uaMgrGetDlgLst(m_UaMgr);
	UaDlg dlg;


	if (dlgLst != NULL) {
		int dlgnum = dxLstGetSize(dlgLst);
		for (int dlgpos = 0; dlgpos < dlgnum; dlgpos++) {
			dlg = (UaDlg)dxLstPeek(dlgLst,dlgpos);
			if (dlg && !IsDlgReleased(dlg))
			{
				/* notification */
				if ( CUAControl::GetControl() ) {
					CUAControl::GetControl()->Fire_Disconnected( GetHandleFromDlg(dlg), uaDlgGetLastestStatusCode(dlg) );
				}
				
				DisconnectCall( GetHandleFromDlg(dlg) );
			}
		}
	}
	return RC_OK;
}

RCODE CCallManager::ValidateURL(MyCString strURL)
{
	SipURL url;
	char temp[80];

	if(strURL.IsEmpty()){
		AfxMessageBox( _T("Request URI Empty!!") );
		return RC_ERROR;
	}
	strcpy(temp, strURL);

	url =  sipURLNewFromTxt( temp );

	if ( ( url == NULL ) && ( strURL.Find( _T("tel") ) == -1) ) {
		AfxMessageBox( _T("Please input Correct Request URI!" ));
		return RC_ERROR;
	}
	sipURLFree(url);
	return RC_OK;
}

MyCString CCallManager::CountElapsedTime()
{
	if ( ++m_ElapsedSecs == 60 ) {
		m_ElapsedSecs = 0;
		if ( ++m_ElapsedMins == 60 ) {
			m_ElapsedMins = 0;
			m_ElapsedHours++;
		}
	}
	MyCString tmpStr;
	tmpStr.Format( _T("%02d:%02d:%02d"), 
			m_ElapsedHours,
			m_ElapsedMins,
			m_ElapsedSecs );

	return tmpStr;
}

int CCallManager::InsertDlgMAP(UaDlg dlg)
{
	MyCString tmpStr;

	if (!dlg)
		return -1;

	for (int i = 0; i < MAX_DLG_COUNT; i++) {
		if (dlgMAP[i] == NULL) {
			dlgMAP[i] = dlg;
			tmpStr.Format("[DlgMAP] Insert dialog handle #%d", i);
			DebugMsg(tmpStr);

			tmpStr.Format("[DlgMAP] Dialog count = %d", ++m_DlgCount);
			DebugMsg(tmpStr);
			return i;
		}
	}
	DebugMsg("[DlgMAP] Can't insert dialog! Dialog CAP reached!");

	return -1;
}

BOOL CCallManager::DeleteDlgMAP(UaDlg dlg)
{
	MyCString tmpStr;

	if (!dlg)
		return FALSE;

	for (int i = 0; i < MAX_DLG_COUNT; i++) {
		if (dlgMAP[i] == dlg) {
			dlgMAP[i] = NULL;
			tmpStr.Format("[DlgMAP] delete dialog handle #%d", i);
			DebugMsg(tmpStr);

			tmpStr.Format("[DlgMAP] Dialog count = %d", --m_DlgCount);
			DebugMsg(tmpStr);
			return TRUE;
		}
	}
	DebugMsg("[DlgMAP] dialog handle not found!");
	return FALSE;
}

UaDlg CCallManager::GetDlgFromHandle(int dlgHandle)
{
	if ( (dlgHandle >= 0) && (dlgHandle < MAX_DLG_COUNT) ) {
		return dlgMAP[dlgHandle];
	} else {
		return NULL;
	}
}

int CCallManager::GetHandleFromDlg(UaDlg dlg)
{
	int i;
	if (!dlg)
		return -1;

	for (i = 0; i < MAX_DLG_COUNT; i++) {
		if (dlgMAP[i] == dlg) {
			return i;
		}
	}
	return -1;
}

const char* CCallManager::GetRemoteParty(int dlgHandle)
{
	UaDlg dlg = GetDlgFromHandle( dlgHandle );
	if ( uaDlgGetRemoteParty(dlg) )
		return uaDlgGetRemoteParty(dlg);
	else
		return m_DialURL;
}

void CCallManager::AddCommand(int CommandID, 
							  int dlgHandle1, 
							  int dlgHandle2, 
							  const char* dialURL,
							  UaContent content)
{
	if ( ( dialURL != NULL ) && ( strlen(dialURL) > 255 ) )
	{
		DebugMsg("Error: dialURL buffer overrun!");
		return;
	}

	UICmdMsg msg = (UICmdMsg)malloc(sizeof(struct UICmdMsgObj));;
	
	msg->CmdID = CommandID;
	msg->dlgHandle1 = dlgHandle1;
	msg->dlgHandle2 = dlgHandle2;

	if (content)
		msg->content = uaContentDup(content);
	else
		msg->content = NULL;

	if (dialURL)
		strcpy(msg->dialURL, dialURL);
	dxLstPutTail( pCallManager->m_UICmdMsgLst, msg );

	switch (msg->CmdID) {
	case UICMD_DROP:
		if ( CUAControl::GetControl() )
			CUAControl::GetControl()->Fire_Cancelling( dlgHandle1 );
	}
}

void CCallManager::ProcessCmd(UICmdMsg uimsg)
{
	int retval;

	switch (uimsg->CmdID) {
	case UICMD_DIAL:
		pCallManager->MakeCall( uimsg->dialURL, retval );
		break;

	case UICMD_CANCEL:
		pCallManager->CancelCall();
		break;

	case UICMD_DROP:
		pCallManager->DisconnectCall( uimsg->dlgHandle1 );
		break;

	case UICMD_ANSWER:
	case UICMD_REJECT:
	case UICMD_HOLD:
	case UICMD_UNHOLD:
	case UICMD_UXFER:
	case UICMD_AXFER:
		break;

	case UICMD_REG:
		pCallManager->Register();
		break;

	case UICMD_UNREG:
		pCallManager->UnRegister();
		break;

	case UICMD_REGQUERY:
		pCallManager->Query();
		break;

	case UICMD_MESSAGE:
		pCallManager->SendMessage( uimsg->dialURL, uimsg->content );
		break;

	default:
		break;
	}

	if (uimsg) {
		if (uimsg->content)
		{
			uaContentFree(uimsg->content);
			uimsg->content = NULL;
		}
		free(uimsg);
		uimsg = NULL;
	}
}

// sam add
const char* CCallManager::GetRemotePartyDisplayName(int dlgHandle)
{
	UaDlg dlg = GetDlgFromHandle( dlgHandle );
	return uaDlgGetRemotePartyDisplayname(dlg);
}

RCODE CCallManager::SendMessage(MyCString dialURL, UaContent content)
{
#ifdef _simple
	UaDlg dlg = uaDlgNew(m_UaMgr);

	char* tempURI = pPresManager->URIfromPREStoSIP(dialURL);

	RCODE retcode = uaDlgMessage(dlg, tempURI, content);
	uaDlgRelease(dlg);

	if (tempURI)
	{
		free(tempURI);
		tempURI = NULL;
	}

	return retcode;
#else
	return RC_OK;
#endif
}


// sam added for support SIP INFO 
RCODE CCallManager::SendInfo(MyCString strURI, MyCString strBodyType, MyCString strBody)
{
	UaDlg dlg = uaDlgNew(m_UaMgr);
	UaContent content = uaContentNew( (char*)(LPCSTR)strBodyType, strBody.GetLength(), (void*)(LPCSTR)strBody );
	RCODE rc = uaDlgInfo( dlg, strURI, content);
	uaDlgRelease(dlg);

	return rc;
}

RCODE CCallManager::ModifySession()
{
	RCODE retcode;
	m_bClient = TRUE;

	pMediaManager->Media_RTP_Stop();

	#ifdef _FOR_VIDEO
		pVideoManager->RTPDisconnect();
		pVideoManager->StopRemoteVideo();
	#endif

	m_bRTPConnected = FALSE;

	pSDPManager->DelSDPbyDlg(m_ActiveDlg);

	SdpSess LocalSDP = pSDPManager->GetLocalSDPbyDlg(m_ActiveDlg);
	retcode = uaDlgChangeSDP(m_ActiveDlg,LocalSDP);
	uaSDPFree(LocalSDP);

	return retcode;
}

// Added by sj 20060117
void CCallManager::reqHTTPTunnel(BOOL isOpen,CString localPort, CString remoteIP,CString remotePort)
{
	CString m_op;
	CString info;
	
	if( isOpen )
		m_op = "open";
	else
		m_op = "close";
	
	if( getHTTPTunnel("/tunnel.html",m_op,localPort,remoteIP,remotePort)=="Error" )
		AfxMessageBox("Request to HTTP Tunnel Server Error");
	else
		;//AfxMessageBox("request to tunnel server ok");
}

CString CCallManager::getHTTPTunnel(CString page,CString strOP,CString localPort, CString remoteIP,CString remotePort)
{
	CString tmpPage;
	CString strRet;
	
	strRet="Error";
	
	
	CString strTunnelIP, strTunnelPort ;
	int iTunnelPort;
	strTunnelIP = AfxGetApp()->GetProfileString( "Http_Tunnel", "HttpTunnel_Addr");//GetUAComRegString( "Http_Tunnel","HttpTunnel_Addr");
	iTunnelPort = AfxGetApp()->GetProfileInt("Http_Tunnel", "HttpTunnel_Port",5060);;//GetUAComRegDW( "Http_Tunnel","HttpTunnel_Port",8080);
	strTunnelPort.Format("%d",iTunnelPort);
	
	if ( (!remoteIP.IsEmpty() && !remotePort.IsEmpty()) &&
		 (!strTunnelIP.IsEmpty() && !strTunnelPort.IsEmpty()) )
	{

		CInternetSession oInet("PCAUA");
		CHttpConnection* pHttp = NULL;

		CString strLocalIP;
		strLocalIP = AfxGetApp()->GetProfileString( "Local_Settings", "Local_IP");
		
		tmpPage = page + "?op=" + strOP + "&src=" + 
					strLocalIP + ":" +
					localPort + "&dst=" +
					remoteIP + ":" +
					remotePort + "\r\n\r\n" ;

		//AfxMessageBox(tmpPage+strTunnelIP+strTunnelPort);
		
		try {
			pHttp = oInet.GetHttpConnection( strTunnelIP, (INTERNET_PORT)atoi(strTunnelPort) );
		} catch (...) {
			return strRet;
		}

		CHttpFile* pPage = pHttp->OpenRequest( "GET", tmpPage, NULL, 1, NULL, NULL, 0 );
		try {
			pPage->SendRequest();
		} catch (...) {
			delete pPage;
			delete pHttp;
			return strRet;
		}

		int nLen = pPage->Read( strRet.GetBuffer(40960), 40959);
		strRet.ReleaseBuffer();
		strRet.SetAt( nLen, 0);

		pPage->Close();
		pHttp->Close();
		delete pPage;
		delete pHttp;
	}

	strRet = "Success";
	return strRet;
}
