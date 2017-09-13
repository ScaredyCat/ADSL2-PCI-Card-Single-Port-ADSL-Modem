// PCAUADlg.cpp : implementation file
//

#include "stdafx.h"
#include "PCAUA.h"
#include "PCAUADlg.h"
#include "EnterURIDlg.h"
#include "VersionNo.h"
#include "PrefDlg.h"
#include "PCASplashDlg.h"
#include "QueryDB.h"
#include "HelpDlg.h"
#include "CallLog.h"
#include "DlgRemoteVideo.h"
#include "DlgMessage.h"
#include "IPPrefDlg.h"
#include <Iphlpapi.h>


#ifdef _XCAPSERVER
#include "AddPersonDisplayNameDlg.h"
#endif

#include "xcapapi.h"

#ifdef	_PCAUA_Res_
#include "PCAUARes.h"
#endif

#include <Mmsystem.h>
#include <vector>	// use STL's vector

//#import "msxml3.dll"
//using namespace MSXML2;

#import <msxml.dll> named_guids
using namespace MSXML;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const IID IID_IUAControl = {0x4A91A134,0x1B60,0x497E,{0x87,0xEE,0x23,0xC9,0x7B,0x2A,0x6D,0x84}};
#define	WM_ICON_NOTIFY		WM_USER+11116

#define ID_TIMER_BS					99
#define ID_TIMER_CLOCK				100
#define ID_TIMER_ANIRINGALERT		101
#define ID_TIMER_UPDATEPEERINFO		102
#define ID_TIMER_STOPTONE			103
#define ID_TIMER_STARTLOCALVIDEO	104
#define ID_TIMER_STOPLOCALVIDEO		105
#define ID_TIMER_MINIMIZEONSTART	106
#define ID_TIMER_STARTREMOTEVIDEO	107
#define ID_TIMER_STOPREMOTEVIDEO	108

#define ID_TIMER_UNHOLDCALL			110
#define ID_TIMER_ANSWERCALL			111
#define ID_TIMER_REJECTCALL			112
#define ID_TIMER_UPDATEINDICATOR	113

#define ID_TIMER_CHANGETOWIRELESS	114
#define ID_TIMER_EXITPROG			999

//! define for test version
//#define EXPIRE_VERSION

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPCAUADlg dialog

CPCAUADlg* g_pMainDlg = NULL;


BOOL g_bSetPreDlg = FALSE;
CString g_strUserName = "";
CString g_strCredential = "";

CPCAUADlg::CPCAUADlg(CWnd* pParent /*=NULL*/)
	: CSkinDialog(CPCAUADlg::IDD, pParent)
{
	g_pMainDlg = this;

	//{{AFX_DATA_INIT(CPCAUADlg)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	
	m_bEnableDND = FALSE;
	m_bAllowCallWaiting = FALSE;
	m_bEnding=FALSE;
	m_bShowVideo=FALSE;
	m_bShowRightPanel=FALSE;
	m_lastHoldCallTimestamp=0;

	m_PeerPhotoRect =  CRect(93,87,144,146);
	m_nUnanswerCalls = 0;
	m_curPageId = -1;
	m_MsgDelayCounter = 0;
	m_rectMCPanel = CRect(48,506,287,586);
	m_bMinimizeOnStart=FALSE;

	m_bCallConnected = false;
	m_fActionAfterHeld = false;

	// configuration for display control
	m_strHelpTitle = "PCA SoftPhone Help Desk";
	m_strAboutTitle = "About PCA SoftPhone";
	m_strAboutProductName = "PCA SoftPhone";
	m_strAboutCopyright = "Copyright (C) 2005 CCL/ITRI. All rights reserved.";

	// configuration for feature enable/disable
	m_bEnableVideo = AfxGetApp()->GetProfileInt("FeatureEnable","Video",1) != 0;
	m_bEnableInstantMessage = AfxGetApp()->GetProfileInt("FeatureEnable","InstantMessage",0) != 0;
	m_bEnableLocalAddressBook = AfxGetApp()->GetProfileInt("FeatureEnable","LocalAddressBook",1) != 0;
	m_bEnablePublicAddressBook = AfxGetApp()->GetProfileInt("FeatureEnable","PublicAddressBook",0) != 0;
	m_bEnableCallLog = AfxGetApp()->GetProfileInt("FeatureEnable","CallLog",1) != 0;
	m_bEnableVoiceMail = AfxGetApp()->GetProfileInt("FeatureEnable","VoiceMail",0) != 0;
	m_bEnableVideoConference = AfxGetApp()->GetProfileInt("FeatureEnable","VideoConference",0) != 0;

	// the setting of numbering rule
	m_strNumberToURITemplate = "sip:$dial_number$@$registrar$";
	
	m_IsInConfiguration = false;
	
	//! BS module
	m_bInitDualNet = FALSE;
	InitializeCriticalSection(&m_BSCS);
}

CPCAUADlg::~CPCAUADlg()
{
	DeleteCriticalSection(&m_BSCS);
}


void CPCAUADlg::DoDataExchange(CDataExchange* pDX)
{
	CSkinDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPCAUADlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPCAUADlg, CSkinDialog)
	//{{AFX_MSG_MAP(CPCAUADlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_COMMAND(ID_KEY_0, OnKey0)
	ON_COMMAND(ID_KEY_1, OnKey1)
	ON_COMMAND(ID_KEY_2, OnKey2)
	ON_COMMAND(ID_KEY_3, OnKey3)
	ON_COMMAND(ID_KEY_4, OnKey4)
	ON_COMMAND(ID_KEY_5, OnKey5)
	ON_COMMAND(ID_KEY_6, OnKey6)
	ON_COMMAND(ID_KEY_7, OnKey7)
	ON_COMMAND(ID_KEY_8, OnKey8)
	ON_COMMAND(ID_KEY_9, OnKey9)
	ON_COMMAND(ID_KEY_ENTER, OnKeyEnter)
	ON_COMMAND(ID_KEY_ESC, OnKeyEsc)
	ON_COMMAND(ID_KEY_STAR, OnKeyStar)
	ON_COMMAND(ID_KEY_BACK, OnKeyBack)
	ON_WM_SIZE()
	ON_COMMAND(IDM_PREFERENCE, OnPreference)
	ON_COMMAND(IDM_EXITPROG, OnExitprog)
	ON_WM_TIMER()
	ON_COMMAND(IDM_SHOWABOUT, OnShowabout)
	ON_COMMAND(ID_KEY_DEL, OnKeyDel)
	ON_COMMAND(ID_KEY_SPACE, OnKeySpace)
	ON_COMMAND(ID_KEY_F10, OnKeyF10)
	ON_COMMAND(ID_KEY_F1, OnKeyF1)
	ON_UPDATE_COMMAND_UI(IDM_HOLDCALL, OnUpdateHoldcall)
	ON_COMMAND(IDM_HOLDCALL, OnHoldcall)
	ON_COMMAND(IDM_TRANSFERCALL, OnTransfercall)
	ON_UPDATE_COMMAND_UI(IDM_TRANSFERCALL, OnUpdateTransfercall)
	ON_WM_INITMENUPOPUP()
	ON_COMMAND(IDM_REDIAL, OnRedial)
	ON_COMMAND(IDM_DIALURI, OnDialuri)
	ON_UPDATE_COMMAND_UI(IDM_REDIAL, OnUpdateRedial)
	ON_COMMAND(ID_KEY_UACOMPREF, OnKeyUacomPref)
	ON_WM_MOVING()
	ON_UPDATE_COMMAND_UI(IDM_PREFERENCE, OnUpdatePreference)
	ON_COMMAND(IDM_HELP, OnPCAHelp)
	ON_WM_MOVE()
	ON_WM_ERASEBKGND()
	ON_WM_ACTIVATE()
	ON_COMMAND(IDM_MC_ANSWER, OnMCAnswer)
	ON_UPDATE_COMMAND_UI(IDM_MC_ANSWER, OnUpdateMCAnswer)
	ON_COMMAND(IDM_MC_HOLD, OnMCHold)
	ON_UPDATE_COMMAND_UI(IDM_MC_HOLD, OnUpdateMCHold)
	ON_WM_DESTROY()
	ON_COMMAND(IDM_MC_REJECT, OnMcReject)
	ON_UPDATE_COMMAND_UI(IDM_MC_REJECT, OnUpdateMcReject)
	ON_COMMAND(IDM_MC_TRANSFER, OnMcTransfer)
	ON_UPDATE_COMMAND_UI(IDM_MC_TRANSFER, OnUpdateMcTransfer)
	ON_COMMAND(IDM_MC_CONFERENCE, OnMcConference)
	ON_UPDATE_COMMAND_UI(IDM_MC_CONFERENCE, OnUpdateMcConference)
	ON_COMMAND(IDM_PRESENCE_OFFLINE, OnPresenceOffline)
	ON_COMMAND(IDM_PRESENCE_BUSY, OnPresenceBusy)
	ON_COMMAND(IDM_PRESENCE_EAT, OnPresenceEat)
	ON_COMMAND(IDM_PRESENCE_MEETING, OnPresenceMeeting)
	ON_COMMAND(IDM_PRESENCE_ONLINE, OnPresenceOnline)
 	ON_COMMAND_RANGE(ID_TOOLLINK_FIRST,ID_TOOLLINK_LAST, OnToolLink)
 	ON_UPDATE_COMMAND_UI_RANGE(ID_TOOLLINK_FIRST,ID_TOOLLINK_LAST, OnUpdateToolLink)
 	ON_COMMAND_RANGE(ID_FEATURE_FIRST, ID_FEATURE_LAST, OnFeaturePanel)
	ON_UPDATE_COMMAND_UI(ID_MESSAGE_TO, OnUpdateMessageTo)
	ON_COMMAND(ID_MESSAGE_TO, OnMessageTo)
	ON_COMMAND(ID_MESSAGE_URI, OnMessageUri)
	ON_MESSAGE(WM_USER_DNDREJECT, OnDNDReject)
	ON_MESSAGE(WM_USER_RECVMSG, OnRecvMsg)
	ON_UPDATE_COMMAND_UI(IDM_MC_VIDEO_CONFERENCE, OnUpdateMcVideoConference)
	ON_COMMAND(IDM_MC_VIDEO_CONFERENCE, OnMcVideoConference)
	ON_MESSAGE(WM_USER_IPCHANGED, OnIPChanged)
	
#ifdef _SIMPLE
	ON_MESSAGE(WM_USER_RECVNEEDAUTH, OnRecvNeedAuth)
	//add by alan,20050223
	ON_MESSAGE(WM_USER_REGEXPIRED, OnRegExpired)
	
#endif

	//}}AFX_MSG_MAP

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPCAUADlg message handlers

BOOL CPCAUADlg::OnInitDialog()
{
	CPCASplashDlg sDlg;
	CPrefDlg dlg;
	CPrefDlg* pDlg = NULL; 
	
	::CoInitialize(NULL);

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
#ifdef EXPIRE_VERSION	
	// Test Version
	{
		// Create registery entry if not exist
		CWinApp* pApp = AfxGetApp();
		DWORD dwValue;

		// First, only check date
		dwValue = pApp->GetProfileInt("","TestVersion",-1);

		if ( dwValue == -1)
		{
		
			time_t ltime;
			struct tm *today;

			time( &ltime );
			today = localtime( &ltime );

			int start;
			start = today->tm_year*365+today->tm_yday;

			pApp->WriteProfileInt( "", "TestVersion", start );
		
			CString s;
			s.Format("This is Test Version, you have 2 weeks to use PCAUA");
			AfxMessageBox(s);
		}
		else
		{
			time_t ltime;
			struct tm *today;

			time( &ltime );
			today = localtime( &ltime );

			int now;
			now = today->tm_year*365+today->tm_yday;

			if( now-dwValue>=14 )
			{
				AfxMessageBox("Expire, please contact with ICL/ITRI");
				EndDialog(IDCANCEL);

				return TRUE;
			}
			
		}

		// Second check, total time for run PCAUA.
		dwValue = pApp->GetProfileInt("","TestVersion_total",0);
		
		if( dwValue>0 )
		{
			int interval = dwValue;
			
			int expire_second = interval;
			int expire_minute = expire_second/60;
			int expire_hour = expire_minute/60;
			int expire_day = expire_hour/24;

			//CString s;
			//s.Format("%d:%d:%d:%d",interval/(24*3600),interval/3600,(interval%3600)/60,(interval%3600)%60);
			//AfxMessageBox(s);

			if( expire_day>=14 ) // 14 days
			{
				AfxMessageBox("Expire, please contact with ICL/ITRI");
				EndDialog(IDCANCEL);

				return TRUE;
			}
		}

		
		DWORD tick = GetTickCount();
		pApp->WriteProfileInt( "", "TestVersion_start", tick );
	
	}
#endif

	if( !CreateRegEntry() )
	{
		// first time run pca, force user to set preference
		dlg.m_UserName = "K200_USER";
		int ret;
		if( (ret=dlg.DoModal())== IDOK )
		{
			AfxGetApp()->WriteProfileString("","PreferredLocalIP",dlg.m_strLocalIP);
			CUACDlg::SetUAComRegValue("User_Info","Username",dlg.m_UserName);
			CUACDlg::SetUAComRegValue("User_Info","Display_Name",dlg.m_UserName);
			CUACDlg::SetUAComRegValue("Credential","Credential#0",dlg.m_strCredential);
		}
		else if( ret==IDCANCEL )
		{
			CUACDlg::SetUAComRegValue("User_Info","Username",dlg.m_UserName);
		}

	}

	if( InitSoftphone(&sDlg) )
	{
		// init skin system
		sDlg.SetSplashText("Loading skins...");
		
		// use external resource DLL while load skin dialog
//t		AfxSetResourceHandle( theApp.m_hSkinResource );
		
		CSkinDialog::SetSkinFile( theApp.m_strAppPath + "skinPhone.ini" );
		CSkinDialog::OnInitDialog();

		// restore original resource DLL
		AfxSetResourceHandle( GetModuleHandle(NULL));

		m_hAccel = ::LoadAccelerators(AfxGetResourceHandle(),
			MAKEINTRESOURCE(IDR_ACCELERATOR1));
		if (!m_hAccel)
			MessageBox("The accelerator table was not loaded");

		// Set the icon for this dialog.  The framework does this automatically
		//  when the application's main window is not a dialog
		SetIcon(m_hIcon, TRUE);			// Set big icon
		SetIcon(m_hIcon, FALSE);		// Set small icon


		sDlg.SetSplashText("Initializing phone book and voice mail...");
		// init right panel
//		CPanelDlg* pDlg = new  CPanelDlg ;
//		pDlg->Create(IDD_RIGHT_PANEL);
		m_dlgRightPanel.Create(IDD_RIGHT_PANEL);



		SetDisplayStatus("Welcome, "+ m_uacDlg.m_strRegistarName);
		CString title, welcome;
		title = "PCA SoftPhone - " + m_uacDlg.m_strRegistarName;
		SetWindowText(title);

		// initialize phone state
		SetButtonEnable("STABTN_RING", FALSE);
		SetButtonEnable("OPRBTN_ONLINE", FALSE);
		SetButtonEnable("STATUS_FOLLOW", FALSE);
		SetButtonEnable("STATUS_CAMP", FALSE);
		SetButtonEnable("STATUS_DND", FALSE);
		SetButtonEnable("STATUS_VMAIL", FALSE);
		SetButtonEnable("STATUS_UNANSCALL", FALSE);
		SetButtonEnable("STATUS_SIGNAL_0", FALSE);
		SetButtonEnable("STATUS_SIGNAL_1", FALSE);
		SetButtonEnable("STATUS_SIGNAL_2", FALSE);
		SetButtonEnable("STATUS_SIGNAL_3", FALSE);
		SetButtonEnable("STATUS_BSSIGNAL_0", FALSE);
		SetButtonEnable("STATUS_BSSIGNAL_1", FALSE);
		SetButtonEnable("STATUS_BSSIGNAL_2", FALSE);
		SetButtonEnable("STATUS_BSSIGNAL_3", FALSE);
		SetButtonEnable("STATUS_BSSIGNAL_4", FALSE);

		SetTextShow( "TEXT_PEERLIST1", FALSE );
		SetTextShow( "TEXT_PEERLIST2", FALSE );
		SetTextShow( "TEXT_PEERLIST3", FALSE );
		SetTextShow( "TEXT_PEERLIST4", FALSE );

		// enable/disable feature buttons
		UpdateFeatureButtons();

		// update status indicator
		//marked by alan
//		UpdateStatusIndicator();

		// create system tray icon
		m_SysTray.Create(NULL,WM_ICON_NOTIFY,"", ::LoadIcon(AfxGetInstanceHandle(),
			MAKEINTRESOURCE(IDR_MAINFRAME)), IDR_MAINFRAME);
		m_SysTray.SetTooltipText(title);

		SetTimer( ID_TIMER_BS, 1000, NULL);
		SetTimer( ID_TIMER_CLOCK, 1000, NULL);
		SetTimer( ID_TIMER_UPDATEINDICATOR, 5*60*1000, NULL);

		sDlg.DestroyWindow();
		m_uacDlg.m_pSplashDlg=NULL;

		//m_bmpRingScreen.LoadBitmap(IDB_RING_SCREEN);
#ifdef	_PCAUA_Res_
		LoadBitmap_(&m_bmpMCPanel, IDB_MC_PANEL);
		LoadBitmap_(&m_bmpMCIconAlert, IDB_MCICON_ALERT);
		LoadBitmap_(&m_bmpMCIconRingBk, IDB_MCICON_RINGBK);
		LoadBitmap_(&m_bmpMCIconHold, IDB_MCICON_HOLD);
		LoadBitmap_(&m_bmpMCIconConn, IDB_MCICON_CONN);
#else
		m_bmpMCPanel.LoadBitmap(IDB_MC_PANEL);
		m_bmpMCIconAlert.LoadBitmap(IDB_MCICON_ALERT);
		m_bmpMCIconRingBk.LoadBitmap(IDB_MCICON_RINGBK);
		m_bmpMCIconHold.LoadBitmap(IDB_MCICON_HOLD);
		m_bmpMCIconConn.LoadBitmap(IDB_MCICON_CONN);
#endif

		if( m_bMinimizeOnStart )
			SetTimer(ID_TIMER_MINIMIZEONSTART,500,NULL);


		ReInit();

		


	}
	else
	{
		EndDialog(IDCANCEL);
	}

	// test PUBLISH
//	UpdatePresence( 1, "Ready");
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CPCAUADlg::ReInit()
{
	m_bUICancel = FALSE;
	m_bUIAnswer = FALSE;
	m_bDoBSOK = TRUE;		// for wireless detect thread control

	// register proxy server if need
	if ( m_uacDlg.GetUAComRegDW( "Registrar", "Use_Registrar", 0) == 0)
	{
		m_MsgDelayCounter = 0;
		m_uacDlg.m_regResult = REG_RESULT_NONEED;
	}
	else
	{
		m_RegCount = 0;
		m_uacDlg.m_regResult = REG_RESULT_INPROGRESS;
		m_uacDlg.UAComRegister();
	}
}

void CPCAUADlg::UpdateFeatureButtons()
{
	if ( GetButtonCheck("FUNBTN_INSTANTMSG"))
		SetButtonCheck("FUNBTN_INSTANTMSG", FALSE);
	if ( GetButtonCheck("FUNBTN_VIDEO"))
		SetButtonCheck("FUNBTN_VIDEO", FALSE);
	if ( GetButtonCheck("FUNBTN_CALLLOG"))
		SetButtonCheck("FUNBTN_CALLLOG", FALSE);
	if ( GetButtonCheck("FUNBTN_ADDRESSBOOK"))
		SetButtonCheck("FUNBTN_ADDRESSBOOK", FALSE);


	SetButtonEnable("FUNBTN_INSTANTMSG", m_bEnableInstantMessage );
	SetButtonEnable("FUNBTN_PHONESETTING", !m_ConfigLink.m_strLink.IsEmpty() );
	SetButtonEnable("FUNBTN_SLIDESHARE", m_vecToolLinks.GetSize() > 0 );
	SetButtonEnable("FUNBTN_VOICEMAIL", 
		m_bEnableVoiceMail && 
		AfxGetApp()->GetProfileInt( "", "EnableVoiceMail", 0) != 0 );
	SetButtonEnable("FUNBTN_VIDEO", m_bEnableVideo);
	SetButtonEnable("FUNBTN_ADDRESSBOOK", m_bEnableLocalAddressBook);
	SetButtonEnable("FUNBTN_CALLLOG", m_bEnableCallLog);

	SetButtonEnable( "OPRBTN_DIALANSWER", TRUE );
}


void CPCAUADlg::UpdateStatusIndicator()
{
	// if proxy exist, query call server for status indicator
	if ( CUACDlg::GetUAComRegDW( "Call_Server", "Use_Call_Server", 0) )
	{
		CString strCallServerURI;
		strCallServerURI.Format( "sip:%s:%d", 
			CUACDlg::GetUAComRegString("Call_Server","Call_Server_Addr"), 
			CUACDlg::GetUAComRegDW("Call_Server","Call_Server_Port",5060) );

		CString strQuerySelfInfo = 
			"<?xml version=\"1.0\" ?>\n"
			"<VONTEL-Extend-Request version=\"1.0\">\n"
			"\t<QueryDeviceSelfInfo>\n"
			"\t</QueryDeviceSelfInfo>\n"
			"</VONTEL-Extend-Request>";

		pUAControlDriver->SendInfo( strCallServerURI, "text/xml", strQuerySelfInfo);
	}
}


void CPCAUADlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CSkinDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CPCAUADlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CSkinDialog::OnPaint();

		CPaintDC dc(this);

		CDC *pDC = GetDC();


		//m_picRingScreen.Show(pDC, CRect(82,34,215,234));
		// draw peer photo
		if( m_PeerPhoto.m_Height>0 )
		{
			m_PeerPhoto.Show(pDC,m_PeerPhotoRect);
	
		}

		ReleaseDC(pDC);
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CPCAUADlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CPCAUADlg::PressDigit(char ch)
{
	m_nUnanswerCalls=0; // reset unanswered calls

	m_strDialNum += ch;
	SetDisplayDigits();

	// play the specifiy signal
	/*
	int nResID = 0;
	switch (ch)
	{
		case '0':	nResID = IDR_WAVE0;	break;
		case '1':	nResID = IDR_WAVE1;	break;
		case '2':	nResID = IDR_WAVE2;	break;
		case '3':	nResID = IDR_WAVE3;	break;
		case '4':	nResID = IDR_WAVE4;	break;
		case '5':	nResID = IDR_WAVE5;	break;
		case '6':	nResID = IDR_WAVE6;	break;
		case '7':	nResID = IDR_WAVE7;	break;
		case '8':	nResID = IDR_WAVE8;	break;
		case '9':	nResID = IDR_WAVE9;	break;
		case '*':	nResID = IDR_WAVES;	break;
		case '#':	nResID = IDR_WAVEP;	break;
		default:	nResID = 0;
	}
	if ( nResID != 0)
	{
		::PlaySound( MAKEINTRESOURCE(nResID), 
					 GetModuleHandle(NULL), 
					 SND_RESOURCE | SND_NODEFAULT | SND_ASYNC );
	}
	*/
	// NOTE: play beep tone only, prevent echo feedback to remote side
	if ( (ch >= '0' && ch <= '9') || ch == '#' || ch == '*')
	{
		::PlaySound( MAKEINTRESOURCE(IDR_WAVE_BEEPTONE), 
					 GetModuleHandle(NULL), 
					 SND_RESOURCE | SND_NODEFAULT | SND_ASYNC );
	}

	int dlgActive = m_CallDlgList.GetActiveDlgHandle();
	if ( dlgActive != -1 )
	{
		// send out DTMF tone while connected
		if( m_CallDlgList.GetState(dlgActive) == CALLDLG_STATE_CONNECTED )
		{
			switch (ch)
			{
				case '0':	pUAControlDriver->SendDTMF0();	break;
				case '1':	pUAControlDriver->SendDTMF1();	break;
				case '2':	pUAControlDriver->SendDTMF2();	break;
				case '3':	pUAControlDriver->SendDTMF3();	break;
				case '4':	pUAControlDriver->SendDTMF4();	break;
				case '5':	pUAControlDriver->SendDTMF5();	break;
				case '6':	pUAControlDriver->SendDTMF6();	break;
				case '7':	pUAControlDriver->SendDTMF7();	break;
				case '8':	pUAControlDriver->SendDTMF8();	break;
				case '9':	pUAControlDriver->SendDTMF9();	break;
				case '*':	pUAControlDriver->SendDTMFS();	break;
				case '#':	pUAControlDriver->SendDTMFP();	break;
				default:	break;
			}
		}

		// send SIP INFO notify DTMF pressed
		CString strURI = pUAControlDriver->GetRemoteParty( dlgActive);
		CString strBody;
		strBody.Format( 
			"<?xml version=\"1.0\" ?>\n"
			"<VONTEL-Extend-Request version=\"1.0\">\n"
			"<DTMFRelay>%c</DTMFRelay>\n"
			"</VONTEL-Extend-Request>", ch);

		pUAControlDriver->SendInfo( strURI, "text/xml", strBody);
	}

}

BOOL CPCAUADlg::PreTranslateMessage(MSG* pMsg) 
{
	if (WM_KEYFIRST <= pMsg->message && pMsg->message <= WM_KEYLAST)
	{
		if (m_hAccel)
			::TranslateAccelerator(m_hWnd, m_hAccel, pMsg);
		return TRUE;
	}
	return CSkinDialog::PreTranslateMessage(pMsg);
}

void CPCAUADlg::OnKey0() 
{
	PressDigit('0');	
}

void CPCAUADlg::OnKey1() 
{
	PressDigit('1');	
}

void CPCAUADlg::OnKey2() 
{
	PressDigit('2');	
}

void CPCAUADlg::OnKey3() 
{
	PressDigit('3');		
}

void CPCAUADlg::OnKey4() 
{
	PressDigit('4');	
}

void CPCAUADlg::OnKey5() 
{
	PressDigit('5');	
}

void CPCAUADlg::OnKey6() 
{
	PressDigit('6');	
}

void CPCAUADlg::OnKey7() 
{
	PressDigit('7');	
}

void CPCAUADlg::OnKey8() 
{
	PressDigit('8');	
}

void CPCAUADlg::OnKey9() 
{
	PressDigit('9');	
}

void CPCAUADlg::OnKeyEnter() 
{
	if( GetButtonEnabled("OPRBTN_DIALANSWER") == TRUE )
		DoUIDial();	
}

void CPCAUADlg::OnKeyEsc() 
{
	DoUICancel();
}

void CPCAUADlg::OnKeyStar() 
{
	PressDigit('*');		
}

void CPCAUADlg::ButtonPressed(CString m_ButtonName)
{
	m_nUnanswerCalls = 0; // reset unanswered calls

	if ( m_ButtonName.Left(6) == "NUMBTN")
		PressDigit( m_ButtonName.GetAt( m_ButtonName.GetLength()-1 ));
	else if( m_ButtonName == "OPRBTN_DIALANSWER" )
		DoUIDial();
	else if( m_ButtonName == "OPRBTN_CANCEL")
		DoUICancel();
	else if( m_ButtonName == "OPRBTN_MENU" )
		ShowMainMenu();
	else if( m_ButtonName == "OPRBTN_TRANSFER" )
		DoUITransfer();
	else if( m_ButtonName == "OPRBTN_DND" )
		DoUIDnD();
	else if( m_ButtonName == "FUNBTN_VIDEO" )
		DoSwitchUAVideo();
	else if( m_ButtonName == "FUNBTN_SLIDESHARE" )
		DoShowToolLinks();
	else if( m_ButtonName == "FUNBTN_PHONESETTING" )
		DoShowConfigLink();
	else if( m_ButtonName == "FUNBTN_INSTANTMSG" )
		DoShowInstantMsg();
	else if( m_ButtonName == "WNDBTN_HELP" )
		OnPCAHelp();
	else if( m_ButtonName == "WNDBTN_MINIMIZE")
		ShowWindow(SW_MINIMIZE );
	else if( m_ButtonName == "FUNBTN_ADDRESSBOOK")
		SetRightPanelPage(PAGEID_ADDRESSBOOK);
	else if( m_ButtonName == "FUNBTN_CALLLOG")
		SetRightPanelPage(PAGEID_CALLLOG);
	else if( m_ButtonName == "FUNBTN_VOICEMAIL")
		SetRightPanelPage(PAGEID_VoiceMail);
	else if( m_ButtonName == "OPRBTN_CLEAR")
		OnKeyBack();
	else if( m_ButtonName.Left(16)=="STATUS_BSSIGNAL_" )
	{	
		if( IsWindow(m_SignalDlg.GetSafeHwnd()) )
		{
			// adjust position of signal dialog to attach at the left side of window
			CRect rc,rc1;
			GetWindowRect(rc);
			m_SignalDlg.GetWindowRect(rc1);
			
			EnterCriticalSection(&(m_BSCS));
			m_SignalDlg.SetWindowPos(&wndTopMost,rc.left+40,rc.bottom-rc1.Height(),0,0,SWP_NOZORDER|SWP_NOSIZE);
			m_SignalDlg.ShowWindow(SW_SHOWNORMAL);
			LeaveCriticalSection(&(m_BSCS));
		}
	}
}

void CPCAUADlg::DoUIDial()
{
	CString strURI;

	// if there's incoming call, answer it
	int dlgInbound = m_CallDlgList.GetDlgHandleByState(CALLDLG_STATE_INBOUND);
	if ( dlgInbound != -1 && !m_bUIAnswer )
	{
		m_bUIAnswer = TRUE;

		// answer call
		try
		{
			pUAControlDriver->AnswerCall( dlgInbound );
		}
		catch (...)
		{
			MessageBox( "Answer Call failed.", "SIP Error", MB_ICONERROR|MB_OK);
		}

		SetDisplayStatus("Answered...");
		SetButtonEnable( "OPRBTN_DIALANSWER", FALSE );
		AniRingAlert(FALSE);

		return;
	}

	// if there's no outgoing call, no connected call, no held call, make a new call
	// and not allow make new call if already have 4 calls
	if( m_CallDlgList.GetDlgHandleByState(CALLDLG_STATE_OUTBOUND) == -1 &&
		m_CallDlgList.GetDlgHandleByState(CALLDLG_STATE_CONNECTED) == -1 &&
		m_CallDlgList.GetDlgHandleByState(CALLDLG_STATE_REMOTE_HOLD) == -1 &&
		m_CallDlgList.GetCount() < 4)
	{

		if( m_strDialNum.IsEmpty())
		{
			// redial or dial URI
			ShowDialMenu();
		}
		else
		{
			// dial extension
			strURI = ConvertNumberToURI( m_strDialNum);

			int dlgHandle;
			try 
			{
				dlgHandle = pUAControlDriver->MakeCall( strURI );
			}
			catch (...)
			{
				//MessageBox( "Make Call failed.", "SIP Error", MB_ICONERROR|MB_OK);
			}

			m_strDialNum.Empty();
			SetDisplayDigits();
			m_strLastDialURI = strURI;
		}
		return;
	}

}

void CPCAUADlg::OnKeyBack()
{
	m_strDialNum.Delete(m_strDialNum.GetLength()-1,1);
	SetDisplayDigits();
}

void CPCAUADlg::OnBuddyUpdateStatus(LPCTSTR buddyURI, BOOL Open, LPCTSTR strMsgState, LPCTSTR strCallState)
{
	CString strBuddyURI = buddyURI;
	int pos_s = strBuddyURI.Find(":",0);
	int pos_e = strBuddyURI.Find("@",0);

	CString strNumber = strBuddyURI.Mid( pos_s+1, pos_e-pos_s-1);
#ifdef _XCAPSERVER
	char buf[128];
	
	CString strURI = (LPSTR)(LPCTSTR)CUACDlg::GetUAComRegString("User_Info","Username");
	if (strURI == strNumber)
		strcpy(buf, "allow");
	else if(!GetRuleActionByURI((char *)(LPCTSTR)strNumber,buf))
	{
		MessageBox( "Update Buddy Presence Failed while getting Presence rule.", "Update Error", MB_ICONERROR|MB_OK);
		return;
	}
	if(!_stricmp(buf,"block"))//if auth-rules is block now...do not change the status
	{
		m_dlgRightPanel.UpdateBuddyPresence(strNumber, 0, "Block", PRE_OFFLINE);
		return;
	}	
#endif
	m_dlgRightPanel.UpdateBuddyPresence(strNumber, Open, strMsgState, strCallState);

	// added by shen, 20041225
	// to check all sessions of conversation
	CDlgMessage::UpdatePartyFromAll(buddyURI, strcmp(strMsgState, PRE_OFFLINE)!=0);
}	

void CPCAUADlg::OnUARingback(long dlgHandle) 
{
	TRACE0("OnUARingback()\n");

	m_MsgDelayCounter = 0;	// stop delay message show counter
	SetDisplayStatus("Ringback...");

	if( !m_CallDlgList.Find(dlgHandle) )
	{
		int idx;
		idx = m_CallDlgList.Add(
				dlgHandle,
				GetPeerTelno(dlgHandle,CALLDLG_TYPE_CALLOUT),
				CALLDLG_STATE_OUTBOUND, CALLDLG_TYPE_CALLOUT);
		if( idx != -1 )
		{
			FillDlgPeerInfo(idx);
			UpdateCallDlgListDisp();
		}
	}

	PlaySound(MAKEINTRESOURCE(IDR_WAVE_RINGBACKTONE),
			  AfxGetInstanceHandle(),
			  SND_RESOURCE | SND_ASYNC | SND_LOOP);

	UpdatePhoneState();
}

void CPCAUADlg::OnUAAlerting(long dlgHandle) 
{
	TRACE0("OnUAAlerting()\n");
	
	int nMaxCurrentCall = m_bAllowCallWaiting? 4 : 1;

	// unable to accept calls if already has 4 calls or be held by remote
	// or in preference configuration
	if( m_CallDlgList.GetCount() >= nMaxCurrentCall || 
		m_CallDlgList.GetDlgHandleByState(CALLDLG_STATE_REMOTE_HOLD) != -1  ||
		m_IsInConfiguration )
	{
		m_dlgReject = dlgHandle;
		SetTimer( ID_TIMER_REJECTCALL,10,NULL);
		return;
	}
	// user DND ?
	if ( m_bEnableDND)
	{
		SetDisplayStatus("Incoming call rejected");
		SetTimer( ID_TIMER_REJECTCALL,10,NULL);
		return;
	}

	if( !m_CallDlgList.Find(dlgHandle) )
	{
		int idx;

		idx = m_CallDlgList.Add(
			dlgHandle,			
			GetPeerTelno(dlgHandle,CALLDLG_TYPE_CALLIN),
			CALLDLG_STATE_INBOUND, CALLDLG_TYPE_CALLIN );

		if( idx != -1 )
		{
			FillDlgPeerInfo(idx);
			UpdateCallDlgListDisp();
		}
	}

	m_MsgDelayCounter = 0;	// stop delay message show counter
	SetDisplayStatus("Alerting..." );

	ShowWindow(SW_SHOWNORMAL);
	SetWindowPos(&wndTopMost,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
	SetWindowPos(&wndNoTopMost,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
	SetForegroundWindow();

	PlaySound(MAKEINTRESOURCE(IDR_WAVE_RINGTONE),
			  AfxGetInstanceHandle(),
			  SND_RESOURCE | SND_ASYNC | SND_LOOP);
	
	UpdatePhoneState();
}

void CPCAUADlg::OnUAProceeding(long dlgHandle) 
{
	TRACE0("OnUAProceeding()\n");

	if( !m_CallDlgList.Find(dlgHandle) )
	{
		int idx;
		idx = m_CallDlgList.Add(
				dlgHandle,
				GetPeerTelno(dlgHandle,CALLDLG_TYPE_CALLOUT),
				CALLDLG_STATE_OUTBOUND, CALLDLG_TYPE_CALLOUT);
		if( idx != -1 )
		{
			FillDlgPeerInfo(idx);
			UpdateCallDlgListDisp();
		}
	}

	m_MsgDelayCounter = 0;	// stop delay message show counter
	SetDisplayStatus("Proceeding...");

	UpdatePhoneState();
}

void CPCAUADlg::OnUAConnected(long dlgHandle) 
{
	AfxTrace( "OnUAConnected(dlg:%d)\n", dlgHandle);

	if( !m_CallDlgList.Find(dlgHandle) )
	{
		int idx;
		idx = m_CallDlgList.Add(
			dlgHandle,			
			GetPeerTelno(dlgHandle,CALLDLG_TYPE_CALLIN),
			CALLDLG_STATE_CONNECTED, CALLDLG_TYPE_CALLIN );
		if( idx != -1 )
			FillDlgPeerInfo(idx);
	}

	if( m_CallDlgList.GetState(dlgHandle) != CALLDLG_STATE_LOCAL_HOLD || 
		m_CallDlgList.GetState(dlgHandle) != CALLDLG_STATE_REMOTE_HOLD )
		m_CallDlgList.SetConnectTime(dlgHandle,CTime::GetCurrentTime());
	m_CallDlgList.SetState(dlgHandle, CALLDLG_STATE_CONNECTED);

	UpdateCallDlgListDisp();

	m_MsgDelayCounter = 0;	// stop delay message show counter

	if (m_CallDlgList.GetDlgHandleByState(CALLDLG_STATE_INBOUND) != -1)
		SetText( "TEXT_STATUS", "Call Waiting..." );
	else
		SetText( "TEXT_STATUS", "Connected" );

	// auto start remote video if video dialog visible
	if ( m_bShowVideo)
	{
		SetTimer(ID_TIMER_STARTLOCALVIDEO,100,NULL);
		SetTimer(ID_TIMER_STARTREMOTEVIDEO,200,NULL);
	}
	m_bCallConnected = true;

	UpdatePhoneState();
}

CString CPCAUADlg::_DisconnectReasonToText(long dlgHandle,long StatusCode) 
{
	switch ( StatusCode)
	{
	case 100:
	case 180:
	case 183:	return "Call Canceled";
	case 200:	return "Disconnected";
	case 400:	return "Invalid Argument";
	case 401:	return "Unauthornized";
	case 403:	return "Not Online";
	case 404:	return "Not Exist";
	case 480:	return "Do Not Distrub";
	case 486:	return "Busy";
	case 487:	return "Call Canceled";
	case 488:	return "Not Acceptable";
	case 450:	return "Authentication failed";
	case 451:	return "Wrong Number";
	case 452:	return "Wrong Target";
	case 453:	return "No Privilege";
	case 500:	return "Server Error";
	case 998:	return "Make Call Error";
	case 999:	
		{
			if( m_CallDlgList.GetType(dlgHandle)==CALLDLG_TYPE_CALLIN)
				return "Call Rejected";
			else
				return "No Response";
		}

	}
	CString s;
	s.Format("Disconnected(%d)", StatusCode);
	return s;
}


void CPCAUADlg::OnUADisconnected(long dlgHandle,long StatusCode) 
{
	// don't care disconnect message if not exist in call dialog list
	if ( m_CallDlgList.GetType(dlgHandle) == -1)
		return;

	// stop remote video if active dlg disconnect
	if ( m_CallDlgList.GetActiveDlgHandle() == dlgHandle)
	{
		m_bCallConnected = false;
		SetTimer(ID_TIMER_STOPREMOTEVIDEO,100,NULL);
	
		// clear remote video window
		Sleep(300);
		if ( g_dlgRemoteVideo)
			g_dlgRemoteVideo->Invalidate();
	}

	AfxTrace( "OnUADisconnected(dlg:%d,status:%d)\r\n", dlgHandle, StatusCode);

	if( m_CallDlgList.GetState(dlgHandle) == CALLDLG_STATE_OUTBOUND
		&& !m_bUICancel) // phone never connected before disconnect, play busy tone
	{
		::PlaySound( MAKEINTRESOURCE(IDR_WAVE_BUSYTONE), 
				GetModuleHandle(NULL), 
				SND_RESOURCE | SND_NODEFAULT | SND_ASYNC | SND_LOOP );
		SetTimer( ID_TIMER_STOPTONE, 4000, NULL);		
	}
	else
	{
		// stop any previous play source (ex: Hold music)
		::PlaySound( NULL, GetModuleHandle(NULL), NULL);
	}

	int callType = m_CallDlgList.GetType(dlgHandle);
	CString telno = GetPeerTelno(dlgHandle,callType);
	int callResult = CALLLOG_CALL_RESULT_NORMAL;

	switch ( m_CallDlgList.GetState(dlgHandle))
	{
	case CALLDLG_STATE_INBOUND:
		if( m_bUICancel ) 
		{
			// drop call by UI user
			callResult = CALLLOG_CALL_RESULT_REJECT;
		}
		else
		{
			// missed call
			m_nUnanswerCalls++;
			callResult = CALLLOG_CALL_RESULT_MISS;
		}
		break;

	case CALLDLG_STATE_OUTBOUND:
		// call out and never connected --> fial
		callResult = CALLLOG_CALL_RESULT_FAIL;
		break;

	case CALLDLG_STATE_CONNECTED:
	case CALLDLG_STATE_LOCAL_HOLD:
	case CALLDLG_STATE_REMOTE_HOLD:
	default:
		callResult = CALLLOG_CALL_RESULT_NORMAL;
		break;
	}

	// update call log
	AddCallLog( dlgHandle, callResult );

	// update status text
	CString strStatus = _DisconnectReasonToText(dlgHandle,StatusCode);

	// update call dialog list
	if( m_CallDlgList.Find(dlgHandle) )
	{
		m_CallDlgList.Remove(dlgHandle);
		UpdateCallDlgListDisp();
	}

	if( m_CallDlgList.GetCount() == 0 ) // all call disconnected?
	{
		SetDisplayStatus( strStatus);
	}
	else
	{
		// show there are other calls if no connected call now.
		if ( m_CallDlgList.GetActiveDlgHandle() == -1)
		{
			CString s;
			s.Format("%s, still has %d call%s", strStatus,
				m_CallDlgList.GetCount(), m_CallDlgList.GetCount()>1 ? "s" : "" );
			SetDisplayStatus( s);
		}
	}

	// delay message for 5 seconds
	m_MsgDelayCounter = 5;

	// clear UI cancel flag here
	m_bUICancel = FALSE;
	m_bUIAnswer = FALSE;

	UpdatePhoneState();
}

void CPCAUADlg::OnUATimeOut(long dlgHandle) 
{
	TRACE0("OnUATimeOut()\n");

	// Arlene added : skip it if the dlgHandle is -1.
	if (dlgHandle == -1)
		return;

	if( m_CallDlgList.Find(dlgHandle) )
	{
		m_CallDlgList.Remove(dlgHandle);
		UpdateCallDlgListDisp();
	}

	m_MsgDelayCounter = 0;	// stop delay message show counter
	SetDisplayStatus("Server timeout");
	UpdatePhoneState();
}

void CPCAUADlg::OnUADialing(long dlgHandle) 
{
	TRACE0("OnUADialing()\n");

	m_MsgDelayCounter = 0;	// stop delay message show counter
	SetDisplayStatus("Dialing");

	if( !m_CallDlgList.Find(dlgHandle) )
	{
		int idx;
		idx = m_CallDlgList.Add(
				dlgHandle,
				GetPeerTelno(dlgHandle,CALLDLG_TYPE_CALLOUT),
				CALLDLG_STATE_OUTBOUND, CALLDLG_TYPE_CALLOUT);
		if( idx != -1 )
		{
			FillDlgPeerInfo(idx);
			UpdateCallDlgListDisp();
		}
	}

	UpdatePhoneState();
}

void CPCAUADlg::OnUABusy(long dlgHandle) 
{
	TRACE0("OnUABusy()\n");

	m_MsgDelayCounter = 0;	// stop delay message show counter
	SetDisplayStatus("Busy");
	UpdatePhoneState();
}

void CPCAUADlg::OnUAReject(long dlgHandle) 
{
	TRACE0("OnUAReject()\n");

	if( m_CallDlgList.Find(dlgHandle) )
	{
		m_CallDlgList.Remove(dlgHandle);
		UpdateCallDlgListDisp();
	}

	m_MsgDelayCounter = 0;	// stop delay message show counter
	SetDisplayStatus("Reject");
	UpdatePhoneState();
}

void CPCAUADlg::OnUATransferred(long dlgHandle, LPCTSTR xferURL) 
{
	TRACE0("OnUATransferred()\n");

	if( m_CallDlgList.Find(dlgHandle) )
	{
		m_CallDlgList.Remove(dlgHandle);
		UpdateCallDlgListDisp();
	}

	m_MsgDelayCounter = 0;	// stop delay message show counter
	SetDisplayStatus("Transferred");
	UpdatePhoneState();
}

void CPCAUADlg::OnUAHeld(long dlgHandle) 
{
	TRACE0("OnUAHeld()\n");

	// don't need to stop remote video but clear remote video window
	if ( g_dlgRemoteVideo)
		g_dlgRemoteVideo->Invalidate();

	bool bLocalHold = _IsLocalHoldCall();
	if ( bLocalHold)
		m_CallDlgList.SetState(dlgHandle, CALLDLG_STATE_LOCAL_HOLD);
	else
		m_CallDlgList.SetState(dlgHandle, CALLDLG_STATE_REMOTE_HOLD);

	UpdateCallDlgListDisp();

	m_MsgDelayCounter = 0;	// stop delay message show counter

	if ( !m_fActionAfterHeld)
	{
		// show local hold and remote hold
		if ( bLocalHold)
			SetDisplayStatus("Holding");
		else
		{
			SetDisplayStatus("Held");

			// play hold music only while UA is be held by remote
			PlaySound(MAKEINTRESOURCE(IDR_WAVE_HOLDTONE),
					  AfxGetInstanceHandle(),
					  SND_RESOURCE | SND_ASYNC | SND_LOOP);
		}
	}

	m_fActionAfterHeld = false;
	UpdatePhoneState();
}

void CPCAUADlg::SetDisplayDigits()
{
	if( m_bEnding )
		return;

	if( m_strDialNum.IsEmpty() )
	{
		SetButtonEnable("OPRBTN_CLEAR", FALSE);

		// enable cancel/drop call button
		if ( m_CallDlgList.GetCount() > 0)
			SetButtonEnable( "OPRBTN_CANCEL", TRUE );
		else
			SetButtonEnable( "OPRBTN_CANCEL", FALSE );
	}
	else
	{
		SetButtonEnable("OPRBTN_CLEAR", TRUE);
		SetButtonEnable("OPRBTN_CANCEL", TRUE);
	}

	SetText( "TEXT_DIGITS",m_strDialNum);
	return;
}

void CPCAUADlg::SetDisplayStatus(CString status)
{
	if( m_bEnding )
		return;

	SetText( "TEXT_STATUS", status);

	// clear digit 
	m_strDialNum.Empty();
	SetDisplayDigits();
	
	// clear remote info
	SetText( "TEXT_REMOTE","");

	// update phone state and buttons
	UpdatePhoneState();

	return;
}

void CPCAUADlg::OnSize(UINT nType, int cx, int cy) 
{
	CSkinDialog::OnSize(nType, cx, cy);
	
	// hide taskbar icon while user minimize window
	if( nType==SIZE_MINIMIZED)
	{
		ShowWindow(SW_HIDE);
	}

}

void CPCAUADlg::UpdatePhoneState()
{
	// enable dial/answer button 
	// if there's inbound call arrived or
	if ( m_CallDlgList.GetDlgHandleByState( CALLDLG_STATE_INBOUND) != -1)
		SetButtonEnable( "OPRBTN_DIALANSWER", TRUE );

	// or if there's no outgoing call, no connected call, no held call, make a new call
	// and not allow make new call if already have 4 calls
	else if( m_CallDlgList.GetDlgHandleByState(CALLDLG_STATE_OUTBOUND) == -1 &&
		m_CallDlgList.GetDlgHandleByState(CALLDLG_STATE_CONNECTED) == -1 &&
		m_CallDlgList.GetDlgHandleByState(CALLDLG_STATE_REMOTE_HOLD) == -1 &&
		m_CallDlgList.GetCount() < 4 && !m_bUIAnswer)

		SetButtonEnable( "OPRBTN_DIALANSWER", TRUE );

	// else disable dial/answer button
	else
		SetButtonEnable( "OPRBTN_DIALANSWER", FALSE );

	// enable cancel/drop call button
	if ( m_CallDlgList.GetCount() > 0 || !m_strDialNum.IsEmpty() )
		SetButtonEnable( "OPRBTN_CANCEL", TRUE );
	else
		SetButtonEnable( "OPRBTN_CANCEL", FALSE );

	// show ringing animation if there's inbound call
	if ( m_CallDlgList.GetDlgHandleByState( CALLDLG_STATE_INBOUND) != -1 && !m_bUIAnswer)
		AniRingAlert( TRUE );
	else
		AniRingAlert( FALSE );
}

void CPCAUADlg::DoUICancel()
{
	if(	!m_strDialNum.IsEmpty() )
	{
		// clear dial digit
		m_strDialNum.Empty();
		SetDisplayDigits();
		// stop busy tone if any
		::PlaySound( NULL, GetModuleHandle(NULL), NULL);
	}
	else
	{
		m_bUICancel = TRUE;	// used in UADisconnect() event handler to mark this call is canceled by local
		m_bUIAnswer = FALSE;

		// if there's an incoming call, reject it
		int dlgInbound = m_CallDlgList.GetDlgHandleByState(CALLDLG_STATE_INBOUND);
		if ( dlgInbound != -1)
		{
			try
			{
				pUAControlDriver->RejectCall(dlgInbound);
			}
			catch (...)
			{
				// if reject cal failed, try use drop call
				try
				{
					pUAControlDriver->DropCall(dlgInbound);
				}
				catch (...)
				{
					// stop ringing and clear up call list anyway.
					OnUADisconnected( dlgInbound, 200);
				}
			}
			return;
		}
		else		
		{
			// drop all call on dlg list
			for( int i=0; i<CALLDLG_MAX_COUNT ;i++)
			{
				int dlg = m_CallDlgList.GetDlgHandleByIndex(i);
				if ( dlg != -1)
				{
					try
					{
						pUAControlDriver->DropCall(dlg);
					}
					catch (...)
					{
						//MessageBox( "Drop Call failed.", "SIP Error", MB_ICONERROR|MB_OK);
						// stop ringing and clear up call list anyway.
						OnUADisconnected( dlg, 200);
					}
				}
			}
		}
	}
}

void CPCAUADlg::ChangeToNewIP()
{
	//OnPreference();
	m_SignalDlg.ShowWindow(SW_HIDE);
	SetTimer(ID_TIMER_CHANGETOWIRELESS,50,0);
}

void CPCAUADlg::OnPreference() 
{
	CPrefDlg dlg;
	CString curLocalIP = AfxGetApp()->GetProfileString("","PreferredLocalIP");
	CString curUserName = CUACDlg::GetUAComRegString("User_Info","Username");
	CString curCredential = CUACDlg::GetUAComRegString("Credential","Credential#0");

	dlg.m_strLocalIP = curLocalIP;
	dlg.m_UserName = curUserName;
	dlg.m_strCredential = curCredential;
	dlg.m_ConfigModes.Copy(m_ConfigMode);
	dlg.m_strVMAccount = AfxGetApp()->GetProfileString("", "VoiceMailAccount");
	dlg.m_strVMPassword = AfxGetApp()->GetProfileString("", "VoiceMailPassword");

 	m_IsInConfiguration = true;
 

 	int nRet = dlg.DoModal();
 
 	m_IsInConfiguration = false;
 
 	if ( nRet == IDOK )
	{	
		// unregister UA in advance.  Added by ydlin '04/07/27
		// Solve that the contact address is too long in SIPAdapter
		try
		{
			pUAControlDriver->UnRegister();
			g_pMainDlg->UpdatePresence(0, (LPSTR)(LPCTSTR)CUACDlg::GetUAComRegString("User_Info","Username"));
			g_pMainDlg->UnSubscribeAll();
			//add by alan: unsubscribe winfo
			g_pMainDlg->unSubscribeWinfo((LPSTR)(LPCTSTR)CUACDlg::GetUAComRegString("User_Info","Username"));
			Sleep(500);
		}
		catch (...) 
		{
			MessageBox( "Unable to unregister with the old settings! Please try again later.", 
				"SIP Error", MB_ICONERROR|MB_OK);
		}

//		if( curUserName != dlg.m_UserName  || dlg.m_strLocalIP != curLocalIP ) // user name changed?
//		{
			
			AfxGetApp()->WriteProfileString("","PreferredLocalIP",dlg.m_strLocalIP);
			CUACDlg::SetUAComRegValue("User_Info","Username",dlg.m_UserName);
			CUACDlg::SetUAComRegValue("User_Info","Display_Name",dlg.m_UserName);
			CUACDlg::SetUAComRegValue("Credential","Credential#0",dlg.m_strCredential);

			g_bSetPreDlg = TRUE;
			g_strUserName = dlg.m_UserName;
			g_strCredential = dlg.m_strCredential;

			pUAControlDriver->Test("profile refresh");

#ifdef _XCAPSERVER
//marked by alan,20050216
//			SetOwnerName((LPSTR)(LPCTSTR)dlg.m_UserName);
#endif
			CPCASplashDlg sDlg;

			Invalidate();
			UpdateWindow();

			/*
			 *  Because m_uacDlg will be destroy in InitSoftphone(), by sjhuang 2006/06/20
			 */
			CString _strRegistarName = dlg.m_UserName; 

			// re-initialize softphone
			if( !InitSoftphone(&sDlg) )
				EndDialog(IDCANCEL);

			SetDisplayStatus("Welcome, "+ _strRegistarName);
			CString title, welcome;
			title = "PCA SoftPhone - " + _strRegistarName;
			SetWindowText(title);
			m_SysTray.SetTooltipText(title);

			ReInit();
//		}

		//CUACDlg::SetUAComRegValue("Credential","Credential#0",dlg.m_strCredential); // alter by sjhuang 2006/06/20
	}
	else if(  dlg.m_bSafeMode )
	{
		

			pUAControlDriver->Test("profile refresh");

		OnKeyUacomPref();
	}
}

DWORD WINAPI _SelfTerminate( void* pParam)
{
	Sleep(5000);
	ExitProcess(0);
	return 0;
}

void CPCAUADlg::OnExitprog() 
{
	m_bEnding=true;
	SetTimer(ID_TIMER_EXITPROG,500,NULL);

//	DisConnect();

	// force terminate after 5 seconds
	// because sometimes it hang in unintialize
	DWORD dwThreadId;
	CreateThread( NULL, 0, _SelfTerminate, NULL, 0, &dwThreadId);
}

void CPCAUADlg::AniRingAlert(BOOL bEnable)
{

	if( bEnable )
		SetTimer( ID_TIMER_ANIRINGALERT,200, NULL);
	else
	{
		KillTimer(ID_TIMER_ANIRINGALERT);

		SetMode2(false);
	}
}

void CPCAUADlg::OnTimer(UINT nIDEvent) 
{
	switch(nIDEvent)
	{
		case ID_TIMER_BS:
			//UpdateBSSignal();
			break;
		
		case ID_TIMER_CLOCK:
			{
				// update clock display
				CTime now = CTime::GetCurrentTime();
				SetText("TEXT_DATE", now.Format("%Y/%#m/%#d"));
				SetText("TEXT_CLOCK", now.Format("%#H:%M"));

				// update connect time display for connected call
				int dlgActive = m_CallDlgList.GetActiveDlgHandle();
				if ( dlgActive != -1)
				{
					CTimeSpan ts = now - m_CallDlgList.GetConnectTime(dlgActive);
					SetText("TEXT_REMOTE", ts.Format("%M:%S"));
				}
				else
				{
					SetText("TEXT_REMOTE", "");
				}

				// decrease message delay counter
				if( m_MsgDelayCounter >0 )
				{
					m_MsgDelayCounter--;
					if ( m_MsgDelayCounter == 0)
					{
						// update phone status after message delay 
						if ( m_CallDlgList.GetActiveDlgHandle() != -1)
						{
							if (m_CallDlgList.GetDlgHandleByState(CALLDLG_STATE_INBOUND) != -1)
								SetText( "TEXT_STATUS", "Call Waiting..." );
							else
								SetText( "TEXT_STATUS", "Connected" );
						}
						else if ( m_CallDlgList.GetDlgHandleByState(CALLDLG_STATE_INBOUND) != -1)
							SetText( "TEXT_STATUS", "Alerting..." );
						else if ( m_CallDlgList.GetDlgHandleByState(CALLDLG_STATE_OUTBOUND) != -1)
							SetText( "TEXT_STATUS", "Ringback..." );
						else if ( m_CallDlgList.GetDlgHandleByState(CALLDLG_STATE_REMOTE_HOLD) != -1)
							SetText( "TEXT_STATUS", "Held" );
						else if ( m_CallDlgList.GetDlgHandleByState(CALLDLG_STATE_LOCAL_HOLD) != -1)
							SetText( "TEXT_STATUS", "Holding" );						
					}
				}
				// show ready status
				if ( m_MsgDelayCounter == 0 && m_CallDlgList.GetCount() == 0)
				{
					// update miss call indicator
					if ( m_nUnanswerCalls > 0 )
					{
						SetButtonEnable("STATUS_UNANSCALL", TRUE);
						SetButtonCheck("STATUS_UNANSCALL", TRUE );
						CString s;
						s.Format( "%d missed call%s", m_nUnanswerCalls, (m_nUnanswerCalls>1)?"s":"");
						SetButtonToolTip("STATUS_UNANSCALL", s);
						SetText("TEXT_REMOTE", s );
					}
					else
					{
						SetButtonEnable("STATUS_UNANSCALL", FALSE);


						SetButtonToolTip("STATUS_UNANSCALL", "No missed call");
						SetButtonCheck("STATUS_UNANSCALL", FALSE );
					}

					// show presence status 
					CString s;
					if( m_uacDlg.m_regResult == REG_RESULT_NONEED )
					{
						s.Format( "%s", m_uacDlg.m_strRegistarName);
					}
					else if( m_uacDlg.m_regResult == REG_RESULT_DONE )
					{
						s.Format( "%s online", m_uacDlg.m_strRegistarName);//, m_strPresence);
					}
					
					SetText( "TEXT_STATUS", s);
					UpdatePeerInfoDisp(TRUE); // clear peer info display
				}
			}

			// update RTP flow
			UpdateSignalDisp();
			
			break;

		case ID_TIMER_ANIRINGALERT:
			//animate "ring" background
			{
				CRect rt(68,23,289,267);
				ToggleMode2(&rt);
			}
			break;

		case ID_TIMER_UPDATEPEERINFO:
			// update peer info display
			KillTimer(ID_TIMER_UPDATEPEERINFO);
			UpdatePeerInfoDisp();
			break;

		case ID_TIMER_STOPTONE:
			KillTimer(ID_TIMER_STOPTONE);
			::PlaySound( NULL, GetModuleHandle(NULL), NULL);
			TRACE0("ID_TIMER_STOPTONE\n");
			break;

		case ID_TIMER_UNHOLDCALL:
			KillTimer(ID_TIMER_UNHOLDCALL);
			if( m_CallDlgList.GetActiveDlgHandle() == -1 )
			{
				try
				{
					pUAControlDriver->UnHoldCall(m_dlgNextAct);
					SetText("TEXT_STATUS","Retriving Call...");
				}
				catch (...) 
				{
					MessageBox( "Retrieve Call failed.", "SIP Error", MB_ICONERROR|MB_OK);
				}
			}
			break;

		case ID_TIMER_ANSWERCALL:
			KillTimer(ID_TIMER_ANSWERCALL);		
			try 
			{
				pUAControlDriver->AnswerCall(m_dlgNextAct);
				SetText("TEXT_STATUS","Answering Call...");
			}
			catch (...) 
			{
				MessageBox( "Answer Call failed.", "SIP Error", MB_ICONERROR|MB_OK);
			}
			break;
	
		case ID_TIMER_STARTLOCALVIDEO:
			KillTimer(ID_TIMER_STARTLOCALVIDEO);
			TRACE0("Starting local video...\n");
			pUAControlDriver->StartLocalVideo();
			break;

		case ID_TIMER_STARTREMOTEVIDEO:
			KillTimer(ID_TIMER_STARTREMOTEVIDEO);
			TRACE0("Starting remote video...\n");
			m_uacDlg.AdjustVideoSize( pUAControlDriver->GetVideoSize());
			break;

		case ID_TIMER_STOPLOCALVIDEO:
			KillTimer(ID_TIMER_STOPLOCALVIDEO);
			pUAControlDriver->StopLocalVideo();
			break;

		case ID_TIMER_STOPREMOTEVIDEO:
			KillTimer(ID_TIMER_STOPREMOTEVIDEO);
			pUAControlDriver->StopRemoteVideo();
			break;

		case ID_TIMER_MINIMIZEONSTART:
			KillTimer(ID_TIMER_MINIMIZEONSTART);
			ShowWindow(SW_MINIMIZE);
			break;
		case ID_TIMER_EXITPROG:
			KillTimer(ID_TIMER_EXITPROG);
			ExitProgram();
			break;

		case ID_TIMER_REJECTCALL:
			KillTimer(ID_TIMER_REJECTCALL);

			try
			{
				pUAControlDriver->RejectCall( m_dlgReject );
			}
			catch (...)
			{
				MessageBox( "Reject Call failed.", "SIP Error", MB_ICONERROR|MB_OK);
			}

			break;

		case ID_TIMER_UPDATEINDICATOR:
//marked by alan
//			UpdateStatusIndicator();

			break;
		case ID_TIMER_CHANGETOWIRELESS:
			KillTimer(ID_TIMER_CHANGETOWIRELESS);
			//OnPreference();
			{
				CPrefDlg dlg;
				CString curLocalIP = GetChangeIP();//AfxGetApp()->GetProfileString("","PreferredLocalIP");
				CString curUserName = CUACDlg::GetUAComRegString("User_Info","Username");
				CString curCredential = CUACDlg::GetUAComRegString("Credential","Credential#0");

				dlg.m_strLocalIP = GetChangeIP();//curLocalIP;
				dlg.m_UserName = curUserName;
				dlg.m_strCredential = curCredential;
				dlg.m_ConfigModes.Copy(m_ConfigMode);
				dlg.m_strVMAccount = AfxGetApp()->GetProfileString("", "VoiceMailAccount");
				dlg.m_strVMPassword = AfxGetApp()->GetProfileString("", "VoiceMailPassword");

 				m_IsInConfiguration = true;
 

 				//int nRet = dlg.DoModal();
 
 				m_IsInConfiguration = false;
 
 				//if ( nRet == IDOK )
				{	
					// unregister UA in advance.  Added by ydlin '04/07/27
					// Solve that the contact address is too long in SIPAdapter
					try
					{
						pUAControlDriver->UnRegister();
						g_pMainDlg->UpdatePresence(0, (LPSTR)(LPCTSTR)CUACDlg::GetUAComRegString("User_Info","Username"));
						g_pMainDlg->UnSubscribeAll();
						//add by alan: unsubscribe winfo
						g_pMainDlg->unSubscribeWinfo((LPSTR)(LPCTSTR)CUACDlg::GetUAComRegString("User_Info","Username"));
						Sleep(500);
					}
					catch (...) 
					{
						MessageBox( "Unable to unregister with the old settings! Please try again later.", 
							"SIP Error", MB_ICONERROR|MB_OK);
					}


					AfxGetApp()->WriteProfileString("","PreferredLocalIP",dlg.m_strLocalIP);
					CUACDlg::SetUAComRegValue("User_Info","Username",dlg.m_UserName);
					CUACDlg::SetUAComRegValue("User_Info","Display_Name",dlg.m_UserName);
					CUACDlg::SetUAComRegValue("Credential","Credential#0",dlg.m_strCredential);

					g_bSetPreDlg = TRUE;
					g_strUserName = dlg.m_UserName;
					g_strCredential = dlg.m_strCredential;

					pUAControlDriver->Test("profile refresh");

					CPCASplashDlg sDlg;

					Invalidate();
					UpdateWindow();

					/*
					 *  Because m_uacDlg will be destroy in InitSoftphone(), by sjhuang 2006/06/20
					 */
					CString _strRegistarName = dlg.m_UserName; 
		
					// re-initialize softphone
					if( !InitSoftphone(&sDlg) )
						EndDialog(IDCANCEL);

					SetDisplayStatus("Welcome, "+ _strRegistarName);
					CString title, welcome;
					title = "PCA SoftPhone - " + _strRegistarName;
					SetWindowText(title);
					m_SysTray.SetTooltipText(title);

					ReInit();
				}
			}
			break;
	}

	CSkinDialog::OnTimer(nIDEvent);
}

void CPCAUADlg::DoUIHold()
{
	// hold current call if exist
	int dlgActive = m_CallDlgList.GetActiveDlgHandle();
	if( dlgActive != -1 )
	{
		// hold current active call
		try
		{
			_DoHoldCall();
		}
		catch (...)
		{
			MessageBox( "Hold Call failed.", "SIP Error", MB_ICONERROR|MB_OK);
		}
	}
	// can retrieve call only if not be held
	else if ( m_CallDlgList.GetDlgHandleByState(CALLDLG_STATE_REMOTE_HOLD) == -1)
	{
		//unhold first held call dlg found on the list
		int dlg = m_CallDlgList.GetDlgHandleByState(CALLDLG_STATE_LOCAL_HOLD);
		if( dlg != -1 )
		{
			try
			{
				pUAControlDriver->UnHoldCall(dlg);
			}
			catch (...)
			{
				MessageBox( "Retrieve Call failed.", "SIP Error", MB_ICONERROR|MB_OK);
			}
		}
	}

}

void CPCAUADlg::DoUITransfer()
{
	// try to transfer active first
	int dlg = m_CallDlgList.GetActiveDlgHandle();	
	// if no active call, try first local hold call
	if ( dlg == -1)
		dlg = m_CallDlgList.GetDlgHandleByState(CALLDLG_STATE_LOCAL_HOLD);

	if ( dlg == -1) // no call to transfer
		return;

	CString strURI;
	if( m_strDialNum.IsEmpty())
	{
		MessageBox("Please enter phone number to transfer.");
		return;
	}
	else
		 strURI = m_strDialNum;

	strURI = ConvertNumberToURI( m_strDialNum);

	try
	{
		pUAControlDriver->UnAttendXfer( dlg, strURI );
	}
	catch (...)
	{
		MessageBox( "Direct Transfer Call failed.", "SIP Error", MB_ICONERROR|MB_OK);
	}

	SetDisplayStatus( "Transfering...");
}

void CPCAUADlg::DoUIDnD()
{
	m_bEnableDND = !m_bEnableDND;
	SetButtonCheck("OPRBTN_DND",m_bEnableDND);
}

LRESULT CPCAUADlg::OnDNDReject(WPARAM wParam, LPARAM lParam)
{
	Sleep(100);

	try
	{
		pUAControlDriver->RejectCall(wParam);
	}
	catch (...)
	{
	}

	return 0;
}

void CPCAUADlg::OnShowabout()
{
	CAboutDlg dlgAbout;
	dlgAbout.DoModal();
}

void CPCAUADlg::OnUAWaiting(long dlgHandle) 
{
	TRACE0("OnUAWaiting()\n");

	int nMaxCurrentCall = m_bAllowCallWaiting? 4 : 1;

	// unable to accept calls if already has 4 calls or be held by remote
	if( m_CallDlgList.GetCount() >= nMaxCurrentCall || 
		m_CallDlgList.GetDlgHandleByState(CALLDLG_STATE_REMOTE_HOLD) != -1 )
	{
		m_dlgReject = dlgHandle;
		SetTimer( ID_TIMER_REJECTCALL,10,NULL);
		return;
	}
	// user DND ?
	if ( m_bEnableDND)
	{
		SetDisplayStatus("Incoming call rejected");
		SetTimer( ID_TIMER_REJECTCALL,10,NULL);
		return;
	}

	if( !m_CallDlgList.Find(dlgHandle) )
	{
		int idx;

		idx = m_CallDlgList.Add(
			dlgHandle,			
			GetPeerTelno(dlgHandle,CALLDLG_TYPE_CALLIN),
			CALLDLG_STATE_INBOUND, CALLDLG_TYPE_CALLIN );

		if( idx != -1 )
		{
			FillDlgPeerInfo(idx);
			UpdateCallDlgListDisp();
		}
	}


	m_MsgDelayCounter = 0;	// stop delay message show counter
	SetDisplayStatus("Call Waiting..." );

	ShowWindow(SW_SHOWNORMAL);
	SetWindowPos(&wndTopMost,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
	SetWindowPos(&wndNoTopMost,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
	SetForegroundWindow();

	PlaySound(MAKEINTRESOURCE(IDR_WAVE_WAITINGTONE),
			  AfxGetInstanceHandle(),
			  SND_RESOURCE | SND_ASYNC | SND_LOOP);

	UpdatePhoneState();
}

void CPCAUADlg::OnKeyDel() 
{
	OnKeyBack();
}

void CPCAUADlg::OnKeySpace() 
{
}

void CPCAUADlg::OnKeyF10() 
{
	DoUIDnD();
}

void CPCAUADlg::OnKeyF1() 
{
	
}


void CPCAUADlg::ShowMainMenu()
{
	CMenu popMenu;
	popMenu.LoadMenu(IDR_MAINFRAME);
	
	_CreateFeaturePanelMenu(popMenu);

	CPoint posMouse;
	GetCursorPos(&posMouse);

	popMenu.GetSubMenu(0)->TrackPopupMenu(0, posMouse.x, posMouse.y, this);
}

void CPCAUADlg::OnUpdateHoldcall(CCmdUI* pCmdUI) 
{
	// enable retrieve if there's only one local hold call
	if( m_CallDlgList.GetDlgHandleByState(CALLDLG_STATE_LOCAL_HOLD) != -1 
		&& m_CallDlgList.GetCount() == 1 )
	{
		pCmdUI->SetText("Retrive");
		pCmdUI->Enable( TRUE);
	}
	// enable hold if there's only one connected call
	else if ( m_CallDlgList.GetActiveDlgHandle() != -1 
		&& m_CallDlgList.GetCount() == 1 )
	{
		pCmdUI->SetText("Hold");
		pCmdUI->Enable( TRUE);
	}
	// disable menu item
	else
		pCmdUI->Enable( FALSE);
}

void CPCAUADlg::OnHoldcall() 
{
	DoUIHold();	
}

void CPCAUADlg::OnTransfercall() 
{
	DoUITransfer();	
}

void CPCAUADlg::OnUpdateTransfercall(CCmdUI* pCmdUI) 
{
	if ( (m_CallDlgList.GetActiveDlgHandle() != -1 ||
		  m_CallDlgList.GetDlgHandleByState(CALLDLG_STATE_LOCAL_HOLD) != -1) && 
		  !m_strDialNum.IsEmpty() )
		pCmdUI->Enable(TRUE);
	else
		pCmdUI->Enable(FALSE);

}

void CPCAUADlg::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu) 
{
	// makes ON_UPDATE_COMMAND_UI works on CDialog classderived class
	CCmdUI CmdUI;
	for(UINT Idx = 0;Idx < pPopupMenu->GetMenuItemCount();Idx++)
	{
		CmdUI.m_nID = pPopupMenu->GetMenuItemID(Idx);
		CmdUI.m_nIndex = Idx;
		CmdUI.m_nIndexMax = pPopupMenu->GetMenuItemCount();
		CmdUI.m_pMenu = pPopupMenu;
		CmdUI.DoUpdate(this,FALSE);
	}
}

static bool s_InitVideoPos = false;

void CPCAUADlg::DoSwitchUAVideo()
{
	if ( !s_InitVideoPos)
	{
		s_InitVideoPos = true;
		
		// init video window position
		CRect rtUIDlg;
		GetWindowRect( &rtUIDlg);
		int x = rtUIDlg.left;
		int y = rtUIDlg.top;

		CRect rtLocalVideoDlg;
		m_uacDlg.GetWindowRect( &rtLocalVideoDlg);

		x -= rtLocalVideoDlg.Width();
		m_uacDlg.SetWindowPos( NULL, x, y, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
		m_uacDlg.OnMouseMove( x, y);

		CRect rtRemoteVideoDlg;
		g_dlgRemoteVideo->GetWindowRect( &rtRemoteVideoDlg);

		x = rtUIDlg.left;
		x -= rtRemoteVideoDlg.Width();
		y += rtLocalVideoDlg.Height();
		g_dlgRemoteVideo->SetWindowPos( NULL, x, y, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
		g_dlgRemoteVideo->OnMouseMove( x, y);

		/* ljchuang 2005/10/4 */
		m_bShowVideo = FALSE;
		
	}

	if( m_bShowVideo )
	{
		// stopRemote must before stoplocal, sjhuang 2006/07/06, otherwise will cause crash.
		// porting from babuzu project...
		if ( m_bCallConnected)
			pUAControlDriver->StopRemoteVideo(); 

		pUAControlDriver->StopLocalVideo();
		m_uacDlg.ShowWindow(SW_HIDE);
		
	}
	else
	{
		pUAControlDriver->StartLocalVideo();

		m_uacDlg.ShowWindow(SW_SHOWNORMAL);
		if ( m_bCallConnected)
			m_uacDlg.AdjustVideoSize( pUAControlDriver->GetVideoSize());
	}

/* old, by sjhuang 2006/07/06
	if( m_bShowVideo )
	{
		m_uacDlg.ShowWindow(SW_HIDE);
		if ( m_bCallConnected)
			pUAControlDriver->StopRemoteVideo();
	}
	else
	{
		m_uacDlg.ShowWindow(SW_SHOWNORMAL);
		if ( m_bCallConnected)
			m_uacDlg.AdjustVideoSize( pUAControlDriver->GetVideoSize());
	}
*/

	m_bShowVideo = ! m_bShowVideo;
	SetButtonCheck("FUNBTN_VIDEO",m_bShowVideo);

}

// replace $variable$ in config/tool links
/*
  - supported $variable$ in url link are:
	$username$ = user name
	$remote_displayname$ = remote number/alias on the screen
*/
CString CPCAUADlg::_ResolveFullLink( CString strLink)
{
	while (true)
	{
		// find $..$ in strLink
		int start = strLink.Find('$');
		int end = strLink.Find('$',start+1);
		if ( start == -1 || end == -1)
			break;	// no more $variables$

		CString strVar = strLink.Mid( start+1, end-start-1);

		// get the value of $variable$ 
		CString strValue;
		if ( strVar == "username")
		{
			strValue = CUACDlg::GetUAComRegString("User_Info","Username");
		}
		else if ( strVar == "remote_displayname")
		{
			int dlgActive = m_CallDlgList.GetActiveDlgHandle();
			if ( dlgActive != -1)
				strValue = GetPeerTelno( dlgActive, m_CallDlgList.GetType(dlgActive) );
		}

		// replace it
		strLink = strLink.Left( start) + strValue + strLink.Mid( end+1);
	}

	return strLink;
}

void CPCAUADlg::DoShowConfigLink()
{
	m_ConfigLink.Show();
}

CPCAUADlg::SLinkInfo::SLinkInfo() : 
	m_position( 100,50,400,300),
	m_dlgBrowser( NULL)
{
}

CPCAUADlg::SLinkInfo::~SLinkInfo() 
{ 
	if ( m_dlgBrowser) 
		delete m_dlgBrowser;
}


void CPCAUADlg::SLinkInfo::Show()
{
	if ( m_strLink.IsEmpty())
		return;

	if ( !m_dlgBrowser)
	{
		m_dlgBrowser = new CDlgBrowser;
		m_dlgBrowser->Create( IDD_BROWSER, g_pMainDlg);
	}

	m_dlgBrowser->ShowLink( m_strTitle, g_pMainDlg->_ResolveFullLink( m_strLink), m_position);
}

void CPCAUADlg::SLinkInfo::Hide()
{
	if ( m_dlgBrowser)
		m_dlgBrowser->ShowWindow( SW_HIDE);
}



// create tool links menu on-the-fly
void CPCAUADlg::_CreateToolLinkMenu( CMenu& menu)
{
	menu.CreatePopupMenu();

	for ( int i=0; i<m_vecToolLinks.GetSize(); i++)
	{
		SLinkInfo& info = m_vecToolLinks[i];

		CString strMenuItem = info.m_strTitle;

		UINT nFlags;
		if ( info.m_strTitle == "-" )
			nFlags = MF_SEPARATOR;
		else if ( info.m_strLink.IsEmpty() )
			nFlags = MF_STRING | MF_DISABLED;
		else
			nFlags = MF_STRING | MF_ENABLED;

		UINT nIDMenuItem = ID_TOOLLINK_FIRST + i;
	
		menu.AppendMenu( nFlags, nIDMenuItem, strMenuItem);
	}
}

// create tool links menu on-the-fly
void CPCAUADlg::_CreateFeaturePanelMenu( CMenu& menu)
{
	CMenu* pFeatureMenu = menu.GetSubMenu(0);
	int nIndex = 3;
	for ( int i=0; i<m_vecFeatures.GetSize(); i++)
	{
		CString& strTitle = m_vecFeatures[i].strTitle;

		UINT nFlags;
		if ( strTitle == "-" )
			nFlags = MF_SEPARATOR;
		else if ( strTitle.IsEmpty() )
			nFlags = MF_STRING | MF_DISABLED;
		else
			nFlags = MF_STRING | MF_ENABLED | MF_BYPOSITION;

		UINT nIDMenuItem = ID_FEATURE_FIRST + i;
		BOOL bReturn = pFeatureMenu->InsertMenu( nIndex, nFlags, nIDMenuItem, strTitle);
		nIndex++;
	}

	if ( m_vecFeatures.GetSize() > 0)
		pFeatureMenu->InsertMenu( nIndex, MF_BYPOSITION | MF_SEPARATOR);
}

void CPCAUADlg::DoShowFeaturePanel()
{
	CMenu popMenu;
	_CreateFeaturePanelMenu( popMenu);

	CPoint posMouse;
	GetCursorPos(&posMouse);

	popMenu.TrackPopupMenu( TPM_RIGHTALIGN|TPM_LEFTBUTTON, posMouse.x, posMouse.y, this);
}

void CPCAUADlg::DoShowToolLinks()
{
	CMenu popMenu;
	_CreateToolLinkMenu( popMenu);

	CPoint posMouse;
	GetCursorPos(&posMouse);

	popMenu.TrackPopupMenu( TPM_RIGHTALIGN|TPM_LEFTBUTTON, posMouse.x, posMouse.y, this);
}

void CPCAUADlg::OnFeaturePanel( UINT nID)
{
	if ( nID < ID_FEATURE_FIRST || nID > ID_FEATURE_LAST)
		return;
	CString strDial = m_vecFeatures[ nID - ID_FEATURE_FIRST].strDialString;
	if (pUAControlDriver)
	{
		int dlgHandle;
		try 
		{
			dlgHandle = pUAControlDriver->MakeCall( strDial );
		}
		catch (...)
		{
			//MessageBox( "Make Call failed.", "SIP Error", MB_ICONERROR|MB_OK);
		}
	}
}

void CPCAUADlg::OnToolLink( UINT nID)
{
	if ( nID < ID_TOOLLINK_FIRST || nID > ID_TOOLLINK_LAST)
		return;

	SLinkInfo& link = m_vecToolLinks[ nID - ID_TOOLLINK_FIRST];
	link.Show();
}


void CPCAUADlg::OnUpdateToolLink( CCmdUI* pCmdUI)
{
	if ( pCmdUI->m_nID < ID_TOOLLINK_FIRST || pCmdUI->m_nID > ID_TOOLLINK_LAST)
		return;

	SLinkInfo& link = m_vecToolLinks[ pCmdUI->m_nID - ID_TOOLLINK_FIRST];

	if ( link.m_strLink.IsEmpty())
		pCmdUI->Enable( FALSE);
}


void CPCAUADlg::ShowDialMenu()
{
	CMenu popMenu;
	popMenu.LoadMenu(IDR_MAINFRAME);
	
	CPoint posMouse;
	GetCursorPos(&posMouse);

	popMenu.GetSubMenu(1)->TrackPopupMenu(0, posMouse.x, posMouse.y, this);

}

void CPCAUADlg::OnRedial() 
{
	m_strDialNum.Empty();
	SetDisplayDigits();

	int dlgHandle;
	try
	{
		dlgHandle = pUAControlDriver->MakeCall( m_strLastDialURI );
	}
	catch (...)
	{
		//MessageBox( "Make Call failed.", "SIP Error", MB_ICONERROR|MB_OK);
	}

}

bool CPCAUADlg::FindBuddy(CString strURI)
{
#ifdef _SIMPLE

	if ( !CUACDlg::GetUAComRegDW("SIMPLE_Server", "Use_SIMPLE_Server", 0))
		return false;

	int nCount = pUAControlDriver->GetBuddyCount();
	for (int i=0 ; i<nCount ; i++)
	{
		CString strThisURI = pUAControlDriver->GetBuddyURI(i);
		if (strThisURI == strURI)
			return true;
	}
	return false;

#else
	return false;
#endif
}

void CPCAUADlg::UnSubscribeAll()
{
#ifdef _SIMPLE

	if ( !CUACDlg::GetUAComRegDW("SIMPLE_Server", "Use_SIMPLE_Server", 0))
		return;

	// added by shen, 20041225
	CDlgMessage::OfflineFromAll();

	std::vector<CString> vecBuddyList;

	CString u_name = CUACDlg::GetUAComRegString("User_Info","Username");
	CString _uri;
	_uri.Format("sip:%s@%s:%d",
					u_name,
					CUACDlg::GetUAComRegString("SIMPLE_Server", "SIMPLE_Server_Addr"),
					CUACDlg::GetUAComRegDW("SIMPLE_Server", "SIMPLE_Server_Port",5060));
	pUAControlDriver->UnsubscribeWinfo(_uri);
	int nCount = pUAControlDriver->GetBuddyCount();
	vecBuddyList.reserve( nCount);
	for (int i=0 ; i<nCount ; i++)
		vecBuddyList.push_back( pUAControlDriver->GetBuddyURI(i));

	for (i=0; i<nCount; i++)
	{
		CString strURI = vecBuddyList[i];
		if(strURI.Find(u_name)!=-1)//found
		{
//			pUAControlDriver->UnsubscribeWinfo(strURI);
		}
		else
//			pUAControlDriver->UnsubscribeBuddy(strURI);
			pUAControlDriver->UnsubscribePres(strURI);
		pUAControlDriver->DelBuddy(strURI);
	}

#endif
}

bool CPCAUADlg::UnSubscribe(CString strNumber)
{
#ifdef _SIMPLE

	if ( !CUACDlg::GetUAComRegDW("SIMPLE_Server", "Use_SIMPLE_Server", 0))
		return false;

	CString strURI;
	strURI.Format("sip:%s@%s:%d",
					strNumber,
					CUACDlg::GetUAComRegString("SIMPLE_Server", "SIMPLE_Server_Addr"),
					CUACDlg::GetUAComRegDW("SIMPLE_Server", "SIMPLE_Server_Port",5060));
	bool bIsBuddy = FindBuddy(strURI);
	if (bIsBuddy == false)
		return false;
	if (pUAControlDriver == NULL)
		return false;

	pUAControlDriver->UnsubscribePres(strURI);
	pUAControlDriver->DelBuddy(strURI);
	return true;

#else
	return false;
#endif
}


//add by alan
bool CPCAUADlg::BlockBuddy(CString strNumber)
{
#ifdef _SIMPLE

	if ( !CUACDlg::GetUAComRegDW("SIMPLE_Server", "Use_SIMPLE_Server", 0))
		return false;

	CString strURI;
	strURI.Format("sip:%s@%s:%d", 
					strNumber,
					CUACDlg::GetUAComRegString("SIMPLE_Server","SIMPLE_Server_Addr"),
					CUACDlg::GetUAComRegDW("SIMPLE_Server","SIMPLE_Server_Port",5060));
	bool bIsBuddy = FindBuddy(strURI);
	if (pUAControlDriver == NULL)
		return false;
	if (bIsBuddy) 
		pUAControlDriver->BlockBuddy(strURI);
	return true;

#else
	return false;
#endif 


}

//add by alan
bool CPCAUADlg::UnBlockBuddy(CString strNumber)
{
#ifdef _SIMPLE

	if ( !CUACDlg::GetUAComRegDW("SIMPLE_Server", "Use_SIMPLE_Server", 0))
		return false;

	CString strURI;
	strURI.Format("sip:%s@%s:%d", 
					strNumber,
					CUACDlg::GetUAComRegString("SIMPLE_Server","SIMPLE_Server_Addr"),
					CUACDlg::GetUAComRegDW("SIMPLE_Server","SIMPLE_Server_Port",5060));
	bool bIsBuddy = FindBuddy(strURI);
	if (pUAControlDriver == NULL)
		return false;
	if (bIsBuddy) 
		pUAControlDriver->UnBlockBuddy(strURI);
	return true;

#else
	return false;
#endif 


}

//add by alan
bool CPCAUADlg::unSubscribeWinfo(CString strNumber)
{
#ifdef _SIMPLE

	if ( !CUACDlg::GetUAComRegDW("SIMPLE_Server", "Use_SIMPLE_Server", 0))
		return false;

	CString strURI;
	strURI.Format("sip:%s@%s:%d", 
					strNumber,
					CUACDlg::GetUAComRegString("SIMPLE_Server","SIMPLE_Server_Addr"),
					CUACDlg::GetUAComRegDW("SIMPLE_Server","SIMPLE_Server_Port",5060));
	bool bIsBuddy = FindBuddy(strURI);
	if (pUAControlDriver == NULL)
		return false;
	if (!bIsBuddy) 
		pUAControlDriver->AddBuddy(strURI);
	pUAControlDriver->UnsubscribeWinfo(strURI);
	return true;

#else
	return false;
#endif 

}

//add by alan
bool CPCAUADlg::SubscribeWinfo(CString strNumber)
{
#ifdef _SIMPLE

	if ( !CUACDlg::GetUAComRegDW("SIMPLE_Server", "Use_SIMPLE_Server", 0))
		return false;

	CString strURI;
	strURI.Format("sip:%s@%s:%d", 
					strNumber,
					CUACDlg::GetUAComRegString("SIMPLE_Server","SIMPLE_Server_Addr"),
					CUACDlg::GetUAComRegDW("SIMPLE_Server","SIMPLE_Server_Port",5060));
	bool bIsBuddy = FindBuddy(strURI);
	if (pUAControlDriver == NULL)
		return false;
	if (!bIsBuddy) 
		pUAControlDriver->AddBuddy(strURI);
	pUAControlDriver->SubscribeWinfo(strURI);
	return true;

#else
	return false;
#endif 

}
//add by alan for unsubscribe user
bool CPCAUADlg::unSubscribe(CString strNumber)
{
#ifdef _SIMPLE

	if ( !CUACDlg::GetUAComRegDW("SIMPLE_Server", "Use_SIMPLE_Server", 0))
		return false;

	CString strURI;
	strURI.Format("sip:%s@%s:%d", 
					strNumber,
					CUACDlg::GetUAComRegString("SIMPLE_Server","SIMPLE_Server_Addr"),
					CUACDlg::GetUAComRegDW("SIMPLE_Server","SIMPLE_Server_Port",5060));
	

	if (pUAControlDriver == NULL)
		return false;
/*
	if (bIsBuddy) // already in the Buddy list
		return false;

  
	pUAControlDriver->AddBuddy(strURI);
*/
	bool bIsBuddy = FindBuddy(strURI);
	if(!bIsBuddy)
		pUAControlDriver->AddBuddy(strURI);
	pUAControlDriver->UnsubscribePres(strURI);
	return true;

#else
	return false;
#endif 

}


bool CPCAUADlg::Subscribe(CString strNumber)
{
#ifdef _SIMPLE

	if ( !CUACDlg::GetUAComRegDW("SIMPLE_Server", "Use_SIMPLE_Server", 0))
		return false;

	CString strURI;
	strURI.Format("sip:%s@%s:%d", 
					strNumber,
					CUACDlg::GetUAComRegString("SIMPLE_Server","SIMPLE_Server_Addr"),
					CUACDlg::GetUAComRegDW("SIMPLE_Server","SIMPLE_Server_Port",5060));
	

	if (pUAControlDriver == NULL)
		return false;
/*
	if (bIsBuddy) // already in the Buddy list
		return false;

  
	pUAControlDriver->AddBuddy(strURI);
*/
	bool bIsBuddy = FindBuddy(strURI);
	if(!bIsBuddy)
		pUAControlDriver->AddBuddy(strURI);
	pUAControlDriver->SubscribePres(strURI);
	return true;

#else
	return false;
#endif 
}

//update self presence status
void CPCAUADlg::UpdatePresence(int nPresence, CString strDes)
{
#ifdef _SIMPLE

	if ( CUACDlg::GetUAComRegDW("SIMPLE_Server", "Use_SIMPLE_Server", 0))
	{

		CString strURI;
		strURI.Format("sip:%s@%s:%d", 
					strDes,
					CUACDlg::GetUAComRegString("SIMPLE_Server","SIMPLE_Server_Addr"),
					CUACDlg::GetUAComRegDW("SIMPLE_Server","SIMPLE_Server_Port",5060));
	
		CString strSimpleServer;

		strSimpleServer.Format( "sip:simple@%s:%d", 
			CUACDlg::GetUAComRegString("SIMPLE_Server","SIMPLE_Server_Addr"),
			CUACDlg::GetUAComRegDW("SIMPLE_Server","SIMPLE_Server_Port",5060) );
		if (nPresence == 1)
		{
			pUAControlDriver->PublishStatus( (LPSTR)(LPCTSTR)strURI,TRUE);
			m_strPresence = PRE_ONLINE;
		}
		else
		{
			pUAControlDriver->PublishStatus( (LPSTR)(LPCTSTR)strURI, FALSE);
			m_strPresence = PRE_OFFLINE;
		}
	}
//	m_strPresence = strDes;

#endif
}

void CPCAUADlg::OnDialuri() 
{
	CEnterURIDlg dlg;

	if( IDOK == dlg.DoModal() )
	{
		m_strDialNum.Empty();
		SetDisplayDigits();
		m_strLastDialURI = dlg.m_strURI;
		if( !dlg.m_strURI.IsEmpty() )
		{
			int dlgHandle;
			try
			{
				dlgHandle = pUAControlDriver->MakeCall( dlg.m_strURI );
			}
			catch (...)
			{
				//MessageBox( "Make Call failed.", "SIP Error", MB_ICONERROR|MB_OK);
			}
		}
	}
}

void CPCAUADlg::OnUpdateRedial(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!m_strLastDialURI.IsEmpty());

	if(!m_strLastDialURI.IsEmpty())
	{
		CString s;
		s.Format("Redial [%s]",URI2PhoneNum(m_strLastDialURI));
		pCmdUI->SetText(s);
	}
}

BOOL CAboutDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	CString ver, s;
	ver = STRPRODUCTVER;
	ver.Replace(",",".");
	s.Format( IDS_ABOUT_PCAUA, ver);
	SetDlgItemText( IDC_APPNAME_VER, s);

	SetDlgItemText( IDC_APPNAME, g_pMainDlg->m_strAboutProductName);
	SetDlgItemText( IDC_COPYRIGHT, g_pMainDlg->m_strAboutCopyright);

	SetWindowText( g_pMainDlg->m_strAboutTitle);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CPCAUADlg::OnKeyUacomPref() 
{
	// open uacontrol's preference dialog
	// only if phone is in on_hook state
	if( m_CallDlgList.GetCount() == 0 )
	{
 		m_IsInConfiguration = true;
 
		BOOL bChanged = pUAControlDriver->ShowPreferences();

 		m_IsInConfiguration = false;

		if ( bChanged)
		{
			// refresh, Modify by sjhuang for select multi-interface problem 2006/07/21
			CString localIP = CUACDlg::GetUAComRegString("Local_Settings","Local_IP");
			AfxGetApp()->WriteProfileString("","PreferredLocalIP",localIP);
			
			CPCASplashDlg sDlg;

			Invalidate();
			UpdateWindow();

			// re-initialize softphone
			if( !InitSoftphone(&sDlg) )
				EndDialog(IDCANCEL);

			SetDisplayStatus("Welcome, "+ m_uacDlg.m_strRegistarName);
			CString title, welcome;
			title = "PCA SoftPhone - " + m_uacDlg.m_strRegistarName;
			SetWindowText(title);
			m_SysTray.SetTooltipText(title);

			ReInit();
		}
	}

}

void CPCAUADlg::OnMoving(UINT fwSide, LPRECT pRect) 
{
	CSkinDialog::OnMoving(fwSide, pRect);
	
}

int CPCAUADlg::CheckNewVersion()
//Return value: 
//	0 - no new version (or not need to check)
//	1 - has new version	
// -1 - fail to get version info
{
	CWinApp* pApp = AfxGetApp();

	if( pApp->GetProfileInt("","CheckUpdates",0) == 0 )
		return 0; 

	CString strNewVer;

	CString versionPage = GetDeployServerPage("PCADeploy/CheckVersion.asp");
	if ( versionPage.IsEmpty())
		return -1;


	IXMLDOMDocumentPtr plDomDocument;
	IXMLDOMNodePtr pNode;
	variant_t vResult;


	plDomDocument.CreateInstance(MSXML::CLSID_DOMDocument);
	vResult = plDomDocument->loadXML((LPCTSTR)versionPage);

	if( (bool)vResult==false )
	{
		// load xml fail
		return -1;
	}
	else
	{
		pNode = plDomDocument->selectSingleNode("/PCADeploy/Version/PCAUA");
		if( pNode )
		{
			strNewVer = (LPCTSTR)pNode->text;
			strNewVer.Replace(".",",");
		}
		else
			return -1;
	}


	/*
	// get response (xml)
	IXMLHTTPRequestPtr pIXMLHTTPRequest = NULL;
	_bstr_t bstrUrl((LPCTSTR)strCheckVerURL);
	HRESULT hr;
	IXMLDOMDocumentPtr pXmlDom;
	IXMLDOMNodePtr pNode;
	
	try 
	{
		hr=pIXMLHTTPRequest.CreateInstance("Msxml2.XMLHTTP");
		SUCCEEDED(hr) ? 0 : throw hr;
		
		hr=pIXMLHTTPRequest->open("GET", bstrUrl, false);
		SUCCEEDED(hr) ? 0 : throw hr;
		
		hr=pIXMLHTTPRequest->send();
		SUCCEEDED(hr) ? 0 : throw hr;
		
		// get xml dom document
		pXmlDom = pIXMLHTTPRequest->responseXML;

		// extract version string
		pNode = pXmlDom->selectSingleNode("/PCADeploy/Version/PCAUA");
		if( pNode )
		{
			strNewVer = (LPCTSTR)pNode->text;
			strNewVer.Replace(".",",");
		}
		else
			return -1;
	}
	catch (...) 
	{
		return -1;
	}

	*/

	// compare version string
	if( VersionStr2DW(strNewVer) > VersionStr2DW(STRFILEVER) )
	{
		// newer version found, prompt to download new version
		if( IDYES == AfxMessageBox(IDS_NOTIFY_NEW_VERSION,	MB_ICONINFORMATION | MB_YESNO) )
		{
			CString strDownloadURL;
			// extrat download url
			pNode = plDomDocument->selectSingleNode("/PCADeploy/DownloadURL/PCAUA");
			if( pNode )
				strDownloadURL = (LPCTSTR)pNode->text;
			else
				return -1;

			ShellExecute( NULL, "open", strDownloadURL, "", "", SW_SHOW);
		}
		else
		{
			AfxMessageBox("Please upgrade to newest version later.");
		}
		return 1;

	}
	else
	{
		// no new version
		return 0;
	}
	
}

DWORD CPCAUADlg::VersionStr2DW(LPCTSTR strVer)
{
	// convert version string to dword value
	int ver[4];
	DWORD ret;
	ret = 0;
	if( sscanf(strVer, "%d,%d,%d,%d", &ver[0], &ver[1], &ver[2], &ver[3]) == 4 )
	{
		ret = ver[0];
		ret<<=8;
		ret|=ver[1];
		ret<<=8;
		ret|=ver[2];
		ret<<=8;
		ret|=ver[3];
	}
	return ret;
}

void CPCAUADlg::OnUpdatePreference(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( m_CallDlgList.GetCount() == 0 );	
}

void CPCAUADlg::DoSwitchRightPanel()
{
	// adjust position of right panel to attach at the right side of window
	CRect rc;
	GetWindowRect(rc);
	m_dlgRightPanel.SetWindowPos(NULL,rc.right, rc.top,0,0,SWP_NOZORDER|SWP_NOSIZE );

	// show/hide right panel
	m_bShowRightPanel = !m_bShowRightPanel;
	m_dlgRightPanel.ShowWindow(m_bShowRightPanel);
	SetButtonCheck("WNDBTN_WIDE",m_bShowRightPanel);

}

void CPCAUADlg::SetRightPanelPage(int pageID)
{

	if(m_bShowRightPanel && pageID == m_curPageId )
	{
		SetButtonCheck("FUNBTN_ADDRESSBOOK", FALSE);
		SetButtonCheck("FUNBTN_CALLLOG", FALSE);
		// close right panel
		DoSwitchRightPanel();
	}
	else
	{
		SetButtonCheck("FUNBTN_ADDRESSBOOK", pageID==PAGEID_ADDRESSBOOK);
		SetButtonCheck("FUNBTN_CALLLOG", pageID==PAGEID_CALLLOG);
		m_dlgRightPanel.SetPage(pageID);
		if(!m_bShowRightPanel)
		{
			// show right panel
			DoSwitchRightPanel();
		}
		m_curPageId = pageID;
	}


}

void CPCAUADlg::DialFromAddrBook(LPCTSTR dialNum)
{
	if( lstrlen(dialNum) == 0 )
	{
		AfxMessageBox("Unable to dial out because phone number is empty.");
		return;
	}

	m_strDialNum = dialNum;
	SetDisplayDigits();
	DoUIDial();
}

void CPCAUADlg::UpdatePeerInfoDisp( BOOL bClear)
{
	if( bClear )
	{
		SetText( "TEXT_PEERINFO_1", "" );
		SetText( "TEXT_PEERINFO_2", "" );
		SetText( "TEXT_PEERINFO_3", "" );
		
		m_PeerPhoto.FreePictureData();
		InvalidateRect( &m_PeerPhotoRect);
	}
	else
	{
		// show connected dialog 
		int dlg = m_CallDlgList.GetActiveDlgHandle();
		// if no connected dialog, show inbound dialog
		if ( dlg == -1)
			dlg = m_CallDlgList.GetDlgHandleByState( CALLDLG_STATE_INBOUND);
		// if no inbound dialog, show oubgoing dialog
		if ( dlg == -1)
			dlg = m_CallDlgList.GetDlgHandleByState( CALLDLG_STATE_OUTBOUND);

		if( dlg != -1 )
		{
			CALL_DLG_INFO* pDlginfo = m_CallDlgList.GetInfo(dlg);
			if( pDlginfo )
			{
				SetText( "TEXT_PEERINFO_1", pDlginfo->telno);
				SetText( "TEXT_PEERINFO_2", pDlginfo->name );
				SetText( "TEXT_PEERINFO_3", pDlginfo->company );
// modified by shen, 20050407
				m_PeerPhoto.Load(m_dlgRightPanel.projectpath+pDlginfo->photoPath);
//				m_PeerPhoto.Load(pDlginfo->photoPath);
				InvalidateRect( &m_PeerPhotoRect);
			}
		}
	}
}

CString CPCAUADlg::URI2PhoneNum(LPCTSTR uri)
{
	CString strUri = uri;
	int pos_s, pos_e;

	pos_s = strUri.Find(":",0);
	pos_e = strUri.Find("@",0);

	return strUri.Mid( pos_s+1, pos_e-pos_s-1);
}


int CPCAUADlg::DownloadServerConfig()
{
	IXMLDOMDocumentPtr plDomDocument;
	IXMLDOMNodePtr pNode, pNode2;
	variant_t vResult;

	CString queryStr = "PCADeploy/ServerConfig.php?client_addr=" + 
			AfxGetApp()->GetProfileString( "", "PreferredLocalIP") +
			"&client_num=" + 
			CUACDlg::GetUAComRegString("User_Info","Username");
			
	CString configPage = GetDeployServerPage(queryStr);

	if ( configPage.IsEmpty())
		return -1;

	// load & parse xml doc
	plDomDocument.CreateInstance(MSXML::CLSID_DOMDocument);
	vResult = plDomDocument->loadXML((LPCTSTR)configPage);

	if( (bool)vResult==false )
	{
		// load xml fail
		return -1;
	}
	else
	{
		DWORD dwValue;
		// configure proxy server
		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/CallServer/Use");
		if( pNode )
		{
			dwValue = atoi((LPCTSTR)pNode->text);
			CUACDlg::SetUAComRegValue( "Call_Server", "Use_Call_Server", dwValue );
		}

		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/CallServer/IPAddress");
		if( pNode )
			CUACDlg::SetUAComRegValue( "Call_Server", "Call_Server_Addr", (LPCTSTR)pNode->text );

		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/CallServer/Port");
		if( pNode )
		{
			dwValue = atoi((LPCTSTR)pNode->text);
			CUACDlg::SetUAComRegValue( "Call_Server", "Call_Server_Port", dwValue );
		}

		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/CallServer/Transport");
		if( pNode )
			CUACDlg::SetUAComRegValue( "Call_Server", "Call_Server_Transport", (LPCTSTR)pNode->text );

		// configure registrar server
		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/RegistrarServer/Use");
		if( pNode )
		{
			dwValue = atoi((LPCTSTR)pNode->text);
			CUACDlg::SetUAComRegValue( "Registrar", "Use_Registrar", dwValue );
		}

		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/RegistrarServer/IPAddress");
		if( pNode )
			CUACDlg::SetUAComRegValue( "Registrar", "Registrar_Addr", (LPCTSTR)pNode->text );

		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/RegistrarServer/Port");
		if( pNode )
		{
			dwValue = atoi((LPCTSTR)pNode->text);
			CUACDlg::SetUAComRegValue( "Registrar", "Registrar_Port", dwValue );
		}

		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/RegistrarServer/ExpireTime");
		if( pNode )
		{
			dwValue = atoi((LPCTSTR)pNode->text);
			CUACDlg::SetUAComRegValue( "Registrar", "Expire_Time", dwValue );
		}


		// STUN server setting.  Added by ydlin 03/10/10
		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/STUNServer/Use");
		if( pNode )
		{
			dwValue = atoi((LPCTSTR)pNode->text);
			CUACDlg::SetUAComRegValue( "User_Info", "Use_STUN", dwValue );
		}
		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/STUNServer/IPAddress");
		if( pNode )
			CUACDlg::SetUAComRegValue( "User_Info", "STUN_server", (LPCTSTR)pNode->text );
		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/STUNServer/Port");
		if( pNode )
		{
			dwValue = atoi((LPCTSTR)pNode->text);
			CUACDlg::SetUAComRegValue( "User_Info", "STUN_port", dwValue );
		}

		// LMS server setting.
		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/LMSServer/Use");
		if( pNode )
		{
			dwValue = atoi((LPCTSTR)pNode->text);
			AfxGetApp()->WriteProfileInt("", "UseLMS", dwValue );
		}
		
		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/LMSServer/IPAddress");
		if( pNode )
			AfxGetApp()->WriteProfileString("", "LMSAddress", (LPCTSTR)pNode->text );
	
		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/LMSServer/Port");
		if( pNode )
		{
			dwValue = atoi((LPCTSTR)pNode->text);
			AfxGetApp()->WriteProfileInt("", "LMSPort", dwValue );
		}

		// Backup registrar server setting.
		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/BackupRegitrarServer/Use");
		if( pNode )
		{
			dwValue = atoi((LPCTSTR)pNode->text);
			CUACDlg::SetUAComRegValue( "Registrar", "Use_Registrar2", dwValue );
		}
		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/BackupRegitrarServer/IPAddress");
		if( pNode )
			CUACDlg::SetUAComRegValue( "Registrar", "Registrar2_Addr", (LPCTSTR)pNode->text );
		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/BackupRegitrarServer/Port");
		if( pNode )
		{
			dwValue = atoi((LPCTSTR)pNode->text);
			CUACDlg::SetUAComRegValue( "Registrar", "Registrar2_Port", dwValue );
		}


		// SIMPLE server setting.  Added by shen 2003/11/11
		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/SIMPLEServer/IPAddress");
		if( pNode )
			CUACDlg::SetUAComRegValue( "SIMPLE_Server", "SIMPLE_Server_Addr", (LPCTSTR)pNode->text );

		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/SIMPLEServer/Port");
		if( pNode )
		{
			dwValue = atoi((LPCTSTR)pNode->text);
			CUACDlg::SetUAComRegValue( "SIMPLE_Server", "SIMPLE_Server_Port", dwValue );
		}

		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/SIMPLEServer/Use");
		if( pNode )
		{
			dwValue = atoi((LPCTSTR)pNode->text);
			CUACDlg::SetUAComRegValue( "SIMPLE_Server", "Use_SIMPLE_Server", dwValue);
		}

		// config deploy server for next time use
		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/NextDeployServer/IPAddress");
		if( pNode )
			AfxGetApp()->WriteProfileString("", "DeployServerAddress", (LPCTSTR)pNode->text );

		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/NextDeployServer/Port");
		if( pNode )
			AfxGetApp()->WriteProfileString("", "DeployServerPort", (LPCTSTR)pNode->text );

		// pca server setting
		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/PCAServer/IPAddress");
		if( pNode )
			AfxGetApp()->WriteProfileString( "", "PCAServerAddress",  (LPCTSTR)pNode->text );

		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/PCAServer/Port");
		if( pNode )
			AfxGetApp()->WriteProfileString("", "PCAServerPort", (LPCTSTR)pNode->text );


		// ToS setting
		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/QoS/ToS");
		if ( pNode)
		{
			IXMLDOMNodePtr pTos = pNode->selectSingleNode("SIPSignal");
			if ( pTos)
				CUACDlg::SetUAComRegValue( "Local_Settings", "SIP_ToS", atoi( pTos->text) );
			
			pTos = pNode->selectSingleNode("RTPAudio");
			if ( pTos)
				CUACDlg::SetUAComRegValue( "Local_Settings", "RTP_Audio_ToS", atoi( pTos->text) );

			pTos = pNode->selectSingleNode("RTPVideo");
			if ( pTos)
				CUACDlg::SetUAComRegValue( "Local_Settings", "RTP_Video_ToS", atoi( pTos->text) );
		}

		// voice mail server setting
		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/VoiceMailServer/IPAddress");
		if( pNode )
			AfxGetApp()->WriteProfileString( "", "VoiceMailAddress",  (LPCTSTR)pNode->text );

		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/VoiceMailServer/Port");
		if( pNode )
			AfxGetApp()->WriteProfileString("", "VoiceMailPort", (LPCTSTR)pNode->text );

		// config link setting
		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/ConfigLink/Title");
		if( pNode )
			m_ConfigLink.m_strTitle = (LPCTSTR)pNode->text;
		else
			m_ConfigLink.m_strTitle = "Configuration";

		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/ConfigLink/Link");
		if( pNode )
			m_ConfigLink.m_strLink = (LPCTSTR)pNode->text;
		else
			m_ConfigLink.m_strLink = "";

		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/ConfigLink/Position");
		if( pNode )
		{
			IXMLDOMNamedNodeMapPtr pos = pNode->attributes;
			if ( (pNode = pos->getNamedItem( "x")) != NULL)
				m_ConfigLink.m_position.left = atoi( (LPCTSTR)pNode->text);
			if ( (pNode = pos->getNamedItem( "y")) != NULL)
				m_ConfigLink.m_position.top = atoi( (LPCTSTR)pNode->text);
			if ( (pNode = pos->getNamedItem( "w")) != NULL)
				m_ConfigLink.m_position.right = atoi( (LPCTSTR)pNode->text);
			if ( (pNode = pos->getNamedItem( "h")) != NULL)
				m_ConfigLink.m_position.bottom = atoi( (LPCTSTR)pNode->text);
		}
		
		// tool link settings
		m_vecToolLinks.RemoveAll();
		IXMLDOMNodeListPtr pNodeList;
		pNodeList = plDomDocument->selectNodes("/DeployConfigSection/ToolLinks/ToolLink");
		if( pNodeList )
		{
			for ( int i=0; i<pNodeList->length; i++)
			{
				pNode = pNodeList->item[i];
				SLinkInfo toolLink;

				IXMLDOMNodePtr pChildNode;
				pChildNode = pNode->selectSingleNode("Title");
				if ( pChildNode)
					toolLink.m_strTitle = (LPCTSTR)pChildNode->text;

				pChildNode = pNode->selectSingleNode("Link");
				if ( pChildNode)
					toolLink.m_strLink = (LPCTSTR)pChildNode->text;

				pChildNode = pNode->selectSingleNode("Position");
				if( pChildNode )
				{
					IXMLDOMNamedNodeMapPtr pos = pChildNode->attributes;
					if ( (pChildNode = pos->getNamedItem( "x")) != NULL)
						toolLink.m_position.left = atoi( (LPCTSTR)pChildNode->text);
					if ( (pChildNode = pos->getNamedItem( "y")) != NULL)
						toolLink.m_position.top = atoi( (LPCTSTR)pChildNode->text);
					if ( (pChildNode = pos->getNamedItem( "w")) != NULL)
						toolLink.m_position.right = atoi( (LPCTSTR)pChildNode->text);
					if ( (pChildNode = pos->getNamedItem( "h")) != NULL)
						toolLink.m_position.bottom = atoi( (LPCTSTR)pChildNode->text);
				}

				m_vecToolLinks.Add( toolLink);
			}
		}

		// dispaly feauter panel
		m_vecFeatures.RemoveAll();
		pNodeList = plDomDocument->selectNodes("/DeployConfigSection/FeaturePanels/FeaturePanel");
		if( pNodeList )
		{
			for ( int i=0; i<pNodeList->length; i++)
			{
				pNode = pNodeList->item[i];
				SFeatures sFeature;

				IXMLDOMNodePtr pChildNode;
				pChildNode = pNode->selectSingleNode("Title");
				if ( pChildNode)
					sFeature.strTitle = (LPCTSTR)pChildNode->text;
				pChildNode = pNode->selectSingleNode("Dial");
				if ( pChildNode)
					sFeature.strDialString = (LPCSTR)pChildNode->text;
				m_vecFeatures.Add(sFeature);
			}
		}
		// display control settings for application name
		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/DisplayControl/AppName");
		if ( pNode)
		{
			free( (void*)AfxGetApp()->m_pszAppName );
			AfxGetApp()->m_pszAppName = _strdup( (LPCTSTR)pNode->text );
		}

		// display control settings for help dialog
		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/DisplayControl/HelpDialog");
		if( pNode )
		{
			IXMLDOMNodePtr pChildNode;
			if ( (pChildNode = pNode->selectSingleNode( "Title")) != NULL )	
				m_strHelpTitle = (LPCTSTR)pChildNode->text;
			if ( (pChildNode = pNode->selectSingleNode( "UserGuide/Link")) != NULL)	
				m_strHelpUserGuildLink = (LPCTSTR)pChildNode->text;
			if ( (pChildNode = pNode->selectSingleNode( "Hotline/Number")) != NULL)
				m_strHelpHotlineNumber = (LPCTSTR)pChildNode->text;
			if ( (pChildNode = pNode->selectSingleNode( "EMail")) != NULL)	
				m_strHelpEmailAddress = (LPCTSTR)pChildNode->text;
			if ( (pChildNode = pNode->selectSingleNode( "WebSupport/Link")) != NULL )	
				m_strHelpWebSupportLink = (LPCTSTR)pChildNode->text;
		}

		// display control settings for about dialog
		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/DisplayControl/AboutDialog");
		if( pNode )
		{
			IXMLDOMNodePtr pChildNode;
			if ( (pChildNode = pNode->selectSingleNode( "Title")) != NULL )	
				m_strAboutTitle = (LPCTSTR)pChildNode->text;
			if ( (pChildNode = pNode->selectSingleNode( "ProductName")) != NULL )	
				m_strAboutProductName = (LPCTSTR)pChildNode->text;
			if ( (pChildNode = pNode->selectSingleNode( "Copyright")) != NULL )	
				m_strAboutCopyright = (LPCTSTR)pChildNode->text;
		}

		// feature control settings
		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/FeatureControl");
		if( pNode )
		{
			if ( pNode->selectSingleNode( "Video/Enabled") )
				m_bEnableVideo = true;
			if ( pNode->selectSingleNode( "Video/Disabled") )
				m_bEnableVideo = false;
			if ( pNode->selectSingleNode( "InstantMessage/Enabled") )
				m_bEnableInstantMessage = true;
			if ( pNode->selectSingleNode( "InstantMessage/Disabled") )
				m_bEnableInstantMessage = false;
			if ( pNode->selectSingleNode( "LocalAddressBook/Enabled") )
				m_bEnableLocalAddressBook = true;
			if ( pNode->selectSingleNode( "LocalAddressBook/Disabled") )
				m_bEnableLocalAddressBook = false;
			if ( pNode->selectSingleNode( "PublicAddressBook/Enabled") )
				m_bEnablePublicAddressBook = true;
			if ( pNode->selectSingleNode( "PublicAddressBook/Disabled") )
				m_bEnablePublicAddressBook = false;
			if ( pNode->selectSingleNode( "CallLog/Enabled") )
				m_bEnableCallLog = true;
			if ( pNode->selectSingleNode( "CallLog/Disabled") )
				m_bEnableCallLog = false;
			if ( pNode->selectSingleNode( "VoiceMail/Enabled") )
				m_bEnableVoiceMail = true;
			if ( pNode->selectSingleNode( "VoiceMail/Disabled") )
				m_bEnableVoiceMail = false;
			if ( pNode->selectSingleNode( "VideoConference/Enabled") )
				m_bEnableVideoConference = true;
			if ( pNode->selectSingleNode( "VideoConference/Disabled") )
				m_bEnableVideoConference = false;
		}

		// number to uri template
		pNode = plDomDocument->selectSingleNode("/DeployConfigSection/NumberingPlan/NumberToURITemplate");
		if( pNode )
		{
			m_strNumberToURITemplate = (LPCTSTR)pNode->text;
		}

		// Get Config Mode
		int curConfigMode = AfxGetApp()->GetProfileInt("","QuickConfigMode", 0);
		int curConfigQuality = AfxGetApp()->GetProfileInt("","QuickConfigQuality", 1);
		m_curConfigMode = curConfigMode;
		IXMLDOMNodeListPtr nodeListPtr;
		nodeListPtr = plDomDocument->selectNodes( "/DeployConfigSection/QuickConfigMode/Mode" );
	

		TRACE("Quick Config items=%d\n",nodeListPtr->length );

		m_ConfigMode.RemoveAll();

		while( (pNode = nodeListPtr->nextNode()) != NULL )
		{
			int modeId;
			CString temp;
			pNode2 = pNode->selectSingleNode("ID");
			modeId = atoi( (LPCTSTR)pNode2->text );

			if( curConfigMode == modeId )
			{
				IXMLDOMNodeListPtr pNodeList;
				int codecOrder;
				CString tagQuality = (curConfigQuality==1)?"BetterQuality":"FasterQuality";
				CString tagTemp;

				// setup audio codec
				for(int i=0;i<9;i++) // clear settings first
				{
					temp.Format( "AudioCodec#%d", i );
					CUACDlg::SetUAComRegValue( "AudioCodec", temp, 0xffffffff );
				}

				tagTemp = tagQuality+"/AudioCodec/ID";
				pNodeList = pNode->selectNodes( (LPCTSTR)tagTemp );
				if( pNodeList->length >0 )
				{
					codecOrder=0;
					while( (pNode2=pNodeList->nextNode()) != NULL )
					{
						temp.Format( "AudioCodec#%d", codecOrder );
						CUACDlg::SetUAComRegValue( "AudioCodec", temp, (DWORD)atoi( (LPCTSTR)pNode2->text ) );
						codecOrder++;
					}
				}

				// setup video codec
				for( int j=0;j<9;j++) // clear settings first
				{
					temp.Format( "VideoCodec#%d", j );
					CUACDlg::SetUAComRegValue( "VideoCodec", temp, 0xffffffff );
				}

				tagTemp = tagQuality+"/VideoCodec/ID";
				pNodeList = pNode->selectNodes((LPCTSTR)tagTemp);
				if( pNodeList->length >0 )
				{
					codecOrder=0;
					while( (pNode2=pNodeList->nextNode()) != NULL )
					{
						temp.Format( "VideoCodec#%d", codecOrder );
						CUACDlg::SetUAComRegValue( "VideoCodec", temp, (DWORD)atoi( (LPCTSTR)pNode2->text ) );
						codecOrder++;
					}
				}				

				// set other parameter
				tagTemp = tagQuality+"/UseVideo";
				pNode2 = pNode->selectSingleNode((LPCTSTR)tagTemp);
				if( pNode2 )
					CUACDlg::SetUAComRegValue( "VideoCodec", "UseVideo", (DWORD)atoi( (LPCTSTR)pNode2->text ) );

				tagTemp = tagQuality+"/VideoBitrate";
				pNode2 = pNode->selectSingleNode((LPCTSTR)tagTemp);
				if( pNode2 )
					CUACDlg::SetUAComRegValue( "VideoCodec", "VideoBitrate", (DWORD)atoi( (LPCTSTR)pNode2->text ) );

				tagTemp = tagQuality+"/VideoFramerate";
				pNode2 = pNode->selectSingleNode((LPCTSTR)tagTemp);
				if( pNode2 )
					CUACDlg::SetUAComRegValue( "VideoCodec", "VideoFramerate", (DWORD)atoi( (LPCTSTR)pNode2->text ) );

				tagTemp = tagQuality+"/VideoSizeFormat";
				pNode2 = pNode->selectSingleNode((LPCTSTR)tagTemp);
				if( pNode2 )
					CUACDlg::SetUAComRegValue( "VideoCodec", "VideoSizeFormat", (DWORD)atoi( (LPCTSTR)pNode2->text ) );

				tagTemp = tagQuality+"/UseVideoQuant";
				pNode2 = pNode->selectSingleNode((LPCTSTR)tagTemp);
				if( pNode2 )
					CUACDlg::SetUAComRegValue( "VideoCodec", "UseQuant", (DWORD)atoi( (LPCTSTR)pNode2->text ) );

				tagTemp = tagQuality+"/VideoQuant";
				pNode2 = pNode->selectSingleNode((LPCTSTR)tagTemp);
				if( pNode2 )
					CUACDlg::SetUAComRegValue( "VideoCodec", "QuantValue", (DWORD)atoi( (LPCTSTR)pNode2->text ) );

				tagTemp = tagQuality+"/VideoEncodeIFrameOnly";
				pNode2 = pNode->selectSingleNode((LPCTSTR)tagTemp);
				if( pNode2 )
					CUACDlg::SetUAComRegValue( "VideoCodec", "Only I frame", (DWORD)atoi( (LPCTSTR)pNode2->text ) );

				tagTemp = tagQuality+"/G711PacketizePeriod";
				pNode2 = pNode->selectSingleNode((LPCTSTR)tagTemp);
				if( pNode2 )
					CUACDlg::SetUAComRegValue( "AudioCodec", "G.711 PacketizePeriod", (DWORD)atoi( (LPCTSTR)pNode2->text ) );

				tagTemp = tagQuality+"/RTPPacketizePeriod";
				pNode2 = pNode->selectSingleNode((LPCTSTR)tagTemp);
				if( pNode2 )
					CUACDlg::SetUAComRegValue( "AudioCodec", "RTP PacketizePeriod", (DWORD)atoi( (LPCTSTR)pNode2->text ) );
			}

			pNode2 = pNode->selectSingleNode("Title");
			temp.Format("%d,%s", modeId, (LPCTSTR)pNode2->text );
			m_ConfigMode.Add( temp );
		}

	}
	return 0;
}

BOOL CPCAUADlg::InitSoftphone(CDialog *pSpDlg)
{
	// hold seetings
	BOOL bSetPreDlg = FALSE;
	
	CString pre_strLocalIP;
	CString pre_UserName;
	CString pre_strCredential;

RETRY_INIT:

	// create splash windiow
	CPCASplashDlg* psDlg = (CPCASplashDlg*)pSpDlg;
	if ( !psDlg->GetSafeHwnd() ) 
		psDlg->Create(IDD_SPLASH);

	// added by sjhuang, 2006/06/20
	SetWindowPos(&psDlg->wndTopMost , 0, 0,0,0,SWP_NOSIZE | SWP_NOMOVE);


	if( AfxGetApp()->GetProfileInt("","DisableDeployServer",0) == 0 )
	{
		psDlg->SetSplashText("Downloading server configuration data...","");
		if( DownloadServerConfig() == -1)
			AfxMessageBox("Warning. Unable to download server configuration data.");

		psDlg->SetSplashText("Checking for new version...");
		switch ( CheckNewVersion())
		{
		case 1:
			// new version found
			psDlg->DestroyWindow();
			return FALSE;
		case -1:
			// check version failed
			AfxMessageBox( _T("Unable to check for new version."), MB_OK);
			break;
		}
	} else {
		// Stop using STUN iff disable deploy server ydlin 03/11/14
		CUACDlg::SetUAComRegValue( "User_Info", "Use_STUN", (ULONG)0 );
	}


	// load other client config
	m_bAllowCallWaiting = AfxGetApp()->GetProfileInt( "", "AllowCallWaiting", 0 );

RETRY_INIT2:

	if (m_bShowVideo) g_dlgRemoteVideo->ShowWindow(SW_HIDE); // sjhuang 2006/07/06, disable remote visible

	psDlg->SetSplashText("Initializing SoftPhone...");


	// Create UACOM dialog (initialize UACom object)
	if( IsWindow( m_uacDlg.GetSafeHwnd()) )
	{
		// destroy uacontrol and it's parent dialog
		m_uacDlg.DestroyWindow();
		Sleep(1000);
		m_uacDlg.UAControlDriver.ReleaseDispatch();
		Sleep(1000);

#ifdef _XCAPSERVER
//marked by alan,20050216
		//add for temporary
/*		if(strlen(GetOwnerName())!=0)
		{
			CUACDlg::SetUAComRegValue("User_Info","Username",GetOwnerName());
			CUACDlg::SetUAComRegValue("User_Info","Display_Name",GetOwnerName());
		}
		//clear old cache data
		ClearCache();
*/
#endif
	}

	/*
	 *  added by sjhuang 2006/03/23, write setting data to registry, otherwise profile will erase registry.
	 *  alter by sjhuang 2006/06/20.
	 */
	if (bSetPreDlg) {
		//CUACDlg::SetUAComRegValue("Local_Settings","Use_Static_IP",1);
		bSetPreDlg = FALSE;
		CUACDlg::SetUAComRegValue("Local_Settings","Local_IP",pre_strLocalIP);
		CUACDlg::SetUAComRegValue("User_Info","Username",pre_UserName);
		CUACDlg::SetUAComRegValue("User_Info","Display_Name",pre_UserName);
		CUACDlg::SetUAComRegValue("Credential","Credential#0",pre_strCredential);
	} 
	else if( g_bSetPreDlg )
	{
		g_bSetPreDlg = FALSE;
		if( !g_strUserName.IsEmpty() && !g_strCredential.IsEmpty() )
		{
			CUACDlg::SetUAComRegValue("User_Info","Username",g_strUserName);
			CUACDlg::SetUAComRegValue("User_Info","Display_Name",g_strUserName);
			CUACDlg::SetUAComRegValue("Credential","Credential#0",g_strCredential);
		}
	}
	/*
	else 
	{
		if (!IsExistIP( AfxGetApp()->GetProfileString("","PreferredLocalIP") )) {
			CUACDlg::SetUAComRegValue("Local_Settings","Use_Static_IP",1);
			CUACDlg::SetUAComRegValue("Local_Settings","Local_IP",getIPAddr());
			AfxGetApp()->WriteProfileString("","PreferredLocalIP",getIPAddr());
		} else {
			CUACDlg::SetUAComRegValue("Local_Settings","Use_Static_IP",1);
			CUACDlg::SetUAComRegValue("Local_Settings","Local_IP",AfxGetApp()->GetProfileString("","PreferredLocalIP"));
		}
	}
	*/
	
	m_uacDlg.m_pUIDlg = this;
	m_uacDlg.m_pSplashDlg = psDlg;
	m_uacDlg.Create( IDD_UACOM );
	if( !m_uacDlg.m_uacInitOk ) // UACom initialize fail?
	{
		psDlg->DestroyWindow();
		AfxMessageBox( "Fatal error occured while initializing SoftPhone component.\nPCA Softphone is unable to start.",MB_OK|MB_ICONSTOP);
		return FALSE;
	}
//add by alan,20050216
#ifdef _XCAPSERVER
	SetOwnerName((LPSTR)(LPCTSTR)CUACDlg::GetUAComRegString("User_Info","Username"));
	ClearCache();
#endif
	// init local video preview
	pUAControlDriver=&m_uacDlg.UAControlDriver;

	/* sjhuang 2006/07/06 */
	s_InitVideoPos = false;
	if (m_bShowVideo) {	
		DoSwitchUAVideo();
	} else {
		pUAControlDriver->StartLocalVideo();
	}
	//pUAControlDriver->StartLocalVideo();

	// waiting for registration in 10 seconds
	CString szTmp;
	psDlg->GetDlgItemText(IDC_SPLASH_MSG, szTmp);
	for ( int nTimes=0; 
		  nTimes < 10 && m_uacDlg.m_regResult == REG_RESULT_INPROGRESS; 
		  nTimes++) 
	{
		szTmp+=" .";
		psDlg->SetSplashText(szTmp);
		Sleep(1000);
	}

	if( m_uacDlg.m_regResult == REG_RESULT_DONE || 
		m_uacDlg.m_regResult == REG_RESULT_NONEED ) // uacom register successful?
	{
		// update feature buttons
		UpdateFeatureButtons();

		// update status indicator
//		UpdateStatusIndicator();
#ifdef _XCAPSERVER
		if(g_pdlgPCAExtend->m_ListPersonal.m_hWnd!=NULL)//if list already initialized
			refreshPBook(&g_pdlgPCAExtend->m_ListPersonal);

#endif

#ifdef _SIMPLE
		if(g_pdlgPCAExtend->m_ListPersonal.m_hWnd!=NULL)//if list already initialized
		{
			g_pMainDlg->UpdatePresence(1, (LPSTR)(LPCTSTR)CUACDlg::GetUAComRegString("User_Info","Username"));
			// The order in which SubscribePhoneBook and SubscribeWinfo are invoked 
			// is changed by molisado, 2006/02/14
			// When IMPS cannot retrieve the authorization rules from LMS, IMPS will send winfo NOTIFY 
			// for every watcher who is already authorized
			// In such case, PCA will check the authorization rules on LMS by itself.
			// However, if PCA cannot connect to LMS, it will check if the peer is already in the buddy list
			// stored in local memory. So when PCA is initialized, SubscribePhoneBook must be invoked first, 
			// which will add each buddy to the buddy list
			g_pdlgPCAExtend->SubscribePhoneBook();
			g_pMainDlg->SubscribeWinfo((LPSTR)(LPCTSTR)CUACDlg::GetUAComRegString("User_Info","Username"));
		}
#endif

		//! detect ip changed ?
		HANDLE hThread;
		DWORD tid;
		if ( ( hThread = CreateThread( NULL, 0, IPChangeThreadFunc, NULL, 0, &tid ) ) < 0)
			return S_FALSE;

		CloseHandle( hThread );

		return TRUE; 
	}
	else
	{
		// added by sjhuang 2006/06/20
		SetWindowPos(&psDlg->wndNoTopMost , 0, 0,0,0,SWP_NOSIZE | SWP_NOMOVE);

		// registration fail
		if( AfxMessageBox(IDS_REG_FAIL, MB_YESNO|MB_ICONSTOP)==IDYES )
		{
			// adjust preference settings
			CPrefDlg dlg;
			CString curLocalIP = AfxGetApp()->GetProfileString("","PreferredLocalIP");
			dlg.m_UserName = CUACDlg::GetUAComRegString("User_Info","Username");
			dlg.m_strCredential = CUACDlg::GetUAComRegString("Credential","Credential#0");

			dlg.m_strLocalIP = curLocalIP;
			if( IDOK == dlg.DoModal())
			{
				bSetPreDlg = TRUE;

				AfxGetApp()->WriteProfileString("","PreferredLocalIP",dlg.m_strLocalIP);
				CUACDlg::SetUAComRegValue("User_Info","Username",dlg.m_UserName);
				CUACDlg::SetUAComRegValue("User_Info","Display_Name",dlg.m_UserName);
				CUACDlg::SetUAComRegValue("Credential","Credential#0",dlg.m_strCredential);

				pre_strLocalIP = dlg.m_strLocalIP;
				pre_UserName = dlg.m_UserName;
				pre_strCredential = dlg.m_strCredential;

				pUAControlDriver->Test(""); // uacom profile refresh !

#ifdef _XCAPSERVER
				SetOwnerName((LPSTR)(LPCTSTR)dlg.m_UserName);
//add by alan,20050216
				ClearCache();
#endif
			}
			else
			{
				if(  dlg.m_bSafeMode )
				{
					BOOL bChanged = pUAControlDriver->ShowPreferences();

					// Modify by sjhuang for select multi-interface problem. 2006/07/21
					if ( bChanged)
					{
						CString localIP = CUACDlg::GetUAComRegString("Local_Settings","Local_IP");
						AfxGetApp()->WriteProfileString("","PreferredLocalIP",localIP);
					}
			
					goto RETRY_INIT2;
				}
			}
			// setting changed, retry init
			goto RETRY_INIT;
		}

		// check if allow user run PCA without server
		if ( AfxGetApp()->GetProfileInt( "", "AllowRunWithoutServer", 0) != 1)
		{
			psDlg->DestroyWindow();
			return FALSE;
		}
		else
		{
			// user don't want to change perference setting
			// confirm if they want to exit
			if( IDYES == AfxMessageBox(IDS_REG_FAIL2, MB_YESNO|MB_ICONSTOP) )
			{
				psDlg->DestroyWindow();
				return FALSE;
			}
		}

		// run without server
		return TRUE;
	}
}

BOOL CPCAUADlg::CreateRegEntry()
{
	// Create registery entry if not exist
	CWinApp* pApp = AfxGetApp();
	DWORD dwValue,autostart;
	BOOL bRet=true;

	dwValue = pApp->GetProfileInt("","CheckUpdates",-1);
	if ( dwValue == -1)
		pApp->WriteProfileInt( "", "CheckUpdates", 0 );

	dwValue = pApp->GetProfileInt("","DisableDeployServer",-1);
	if ( dwValue == -1)
		pApp->WriteProfileInt( "", "DisableDeployServer", 1 );


	dwValue = pApp->GetProfileInt("","AutoStartUp",-1);
	
	if ( dwValue == -1)
	{
		pApp->WriteProfileInt( "", "AutoStartUp", 0 );
		autostart = 1;
	}
	else
		autostart = dwValue;

	CString s;
	s = pApp->GetProfileString("", "DeployServerAddress", "NotSpecified");
	if ( s == "NotSpecified" )
	{
		pApp->WriteProfileString( "", "DeployServerAddress", "" );
		bRet = false;
	}


	// create auto startup
	HKEY hKey;
	CString regPath="Software\\Microsoft\\Windows\\CurrentVersion\\Run";

	// check auto startup is "Empty" (first time run PCA)
	char szNull[] = "";

	if( RegCreateKeyEx( HKEY_CURRENT_USER, regPath , 0,
						 szNull,
						 REG_OPTION_NON_VOLATILE,
						 KEY_READ, 
						 NULL,
						 &hKey,
						 NULL ) == ERROR_SUCCESS)
	{
		DWORD dwSize = 256;
		DWORD dwType;
		RegQueryValueEx( hKey, "PCA UA", 0, &dwType, (BYTE*)s.GetBuffer(256), &dwSize);
		s.ReleaseBuffer();
		RegCloseKey(hKey);
	}

	if(s=="Empty")
	{
		if ( RegCreateKeyEx( HKEY_CURRENT_USER, 
				regPath, 
				0, 0, 0, KEY_ALL_ACCESS, 0, &hKey, 0) == 0)
		{
			char szFileName[1024];
			
			if( autostart==1 )
				GetModuleFileName( GetModuleHandle(NULL), szFileName, 1024);
			else
				szFileName[0]=NULL;

			DWORD dwType = REG_SZ;
			DWORD dwSize = strlen(szFileName) + 1;
			RegSetValueEx( hKey, "PCA UA", 0, dwType, (BYTE*)szFileName, dwSize);

			RegCloseKey( hKey);
		}
	}


	// quick config mode & quality
	dwValue = pApp->GetProfileInt("","QuickConfigMode",-1);
	if( dwValue == -1)
		pApp->WriteProfileInt( "", "QuickConfigMode", 0 );

	dwValue = pApp->GetProfileInt("","QuickConfigQuality",-1);
	if( dwValue == -1)
		pApp->WriteProfileInt( "", "QuickConfigQuality", 1 );

	return bRet;
}

void CPCAUADlg::OnPCAHelp() 
{
	CHelpDlg dlg;

	dlg.DoModal();
	if( !dlg.m_helpHotLine.IsEmpty() )
		DialFromAddrBook(dlg.m_helpHotLine);

}

void CPCAUADlg::AddCallLog( int dlgHandle, int callResult )
{
	CCallLog callLog;

	CString peerUri;
	CString tel;

	COleDateTime callEndTime=COleDateTime::GetCurrentTime();
	
	int callType = m_CallDlgList.GetType(dlgHandle);

	if( callType != -1 )
	{
		tel = GetPeerTelno(dlgHandle, callType);

		if( tel.GetLength()>50)
			tel=tel.Left(50);
		if( peerUri.GetLength()>50)
			peerUri=peerUri.Left(50);

		callLog.insertItem( tel, peerUri, 
			m_CallDlgList.GetStartTime(dlgHandle),callEndTime,
			m_CallDlgList.GetType(dlgHandle),callResult,false);

		m_dlgRightPanel.UpdateCallLog();
	}

}

CString CPCAUADlg::GetDeployServerPage(LPCTSTR page)
{
	CString strRet;

	// Get deploy serverr address and port number
	CString strServerIP, strServerPort ;
	strServerIP = AfxGetApp()->GetProfileString( "", "DeployServerAddress");
	strServerPort = AfxGetApp()->GetProfileString( "", "DeployServerPort");

	if ( !strServerIP.IsEmpty() && !strServerPort.IsEmpty())
	{

		CInternetSession oInet("PCAUA");
		CHttpConnection* pHttp = NULL;

		try {
			pHttp = oInet.GetHttpConnection( strServerIP, (INTERNET_PORT)atoi(strServerPort) );
		} catch (...) {
			return strRet;
		}

		CHttpFile* pPage = pHttp->OpenRequest( "GET", page, NULL, 1, NULL, NULL, 0 );
		try {
			pPage->SendRequest();
		} catch (...) {
			delete pPage;
			delete pHttp;
			return strRet;
		}

		Sleep(200);

		int nLen = pPage->Read( strRet.GetBuffer(40960), 40959);
		strRet.ReleaseBuffer();

		if( nLen>0 )
			strRet.SetAt( nLen, 0);

		pPage->Close();
		pHttp->Close();
		delete pPage;
		delete pHttp;
	}
	
	return strRet;
}

void CPCAUADlg::DragWindow(int x, int y)
{

	//GetWindowRect(&m_rcMoving);
	//m_rcMoving.OffsetRect( x-m_rcMoving.left, y-m_rcMoving.top );
	//PostMessage( WM_MOVING,0,(LPARAM)(LPRECT)m_rcMoving);
}

void CPCAUADlg::OnMove(int x, int y) 
{
	CSkinDialog::OnMove(x, y);
	
	CRect r;
	GetWindowRect( &r );

	UpdateWindow();
	if(IsWindow(m_dlgRightPanel.m_hWnd) )
	{
		m_dlgRightPanel.SetWindowPos(NULL,x+r.Width(),y,0,0,SWP_NOZORDER|SWP_NOSIZE);
		m_dlgRightPanel.UpdateWindow();
	}

	if(IsWindow(m_uacDlg.m_hWnd) )
	{
		m_uacDlg.SnapMove(x,y);
		m_uacDlg.UpdateWindow();
	}

	if ( g_dlgRemoteVideo && IsWindow(g_dlgRemoteVideo->m_hWnd) )
	{
		g_dlgRemoteVideo->SnapMove(x,y);
		g_dlgRemoteVideo->UpdateWindow();
	}	
}


CString CPCAUADlg::GetPeerTelno(int dlg, int type)
{
	CString telno;

	if( type == CALLDLG_TYPE_CALLOUT )
		telno = URI2PhoneNum(pUAControlDriver->GetRemoteParty(dlg));
	else
		telno = pUAControlDriver->GetRemotePartyDisplayName(dlg);

	if( telno.IsEmpty())
		telno = pUAControlDriver->GetRemoteParty(dlg);
	
	return telno;
}

void CPCAUADlg::UpdateCallDlgListDisp()
{

	CString labelName, s;
	CALL_DLG_INFO *pdlginfo;

	BOOL bShow = (m_CallDlgList.GetCount()>0);

	SetRedraw(FALSE);
	SetTextShow( "TEXT_PEERLIST1", bShow );
	SetTextShow( "TEXT_PEERLIST2", bShow );
	SetTextShow( "TEXT_PEERLIST3", bShow );
	SetTextShow( "TEXT_PEERLIST4", bShow );

	for( int i=0; i<CALLDLG_MAX_COUNT ;i++)
	{
		labelName.Format("TEXT_PEERLIST%d", i+1 );
		pdlginfo = m_CallDlgList.GetInfoIdx(i);
		if(pdlginfo)
		{
			s.Format("[%s] %s", pdlginfo->telno, pdlginfo->name);
			SetText(labelName, s);
		}
		else
			SetText(labelName, "");
	}


	SetRedraw(TRUE);
	//Invalidate();
	InvalidateRect( m_rectMCPanel );

	// update peer info panel 
	SetTimer(ID_TIMER_UPDATEPEERINFO,0,NULL);

}


void CPCAUADlg::TextClicked(CString m_TextName)
{
	if( m_TextName.Left(13)=="TEXT_PEERLIST" )
	{
		int idx;
		idx = atoi( m_TextName.Right(1) ) - 1;
		CALL_DLG_INFO* pDlginfo;
		pDlginfo = m_CallDlgList.GetInfoIdx(idx);
		if( pDlginfo )
		{
			m_clickMCHandle = pDlginfo->dlgHandle;

			// pop up mutiple call action menu
			CMenu popMenu;
			popMenu.LoadMenu(IDR_MAINFRAME);
		
			CPoint posMouse;
			GetCursorPos(&posMouse);

			popMenu.GetSubMenu(4)->TrackPopupMenu(0, posMouse.x, posMouse.y, this);
		}
	}
}

void CPCAUADlg::FillDlgPeerInfo(int idx)
{
	if( idx != -1)
	{
		CALL_DLG_INFO* pDlginfo;
		pDlginfo = m_CallDlgList.GetInfoIdx(idx);
		if( pDlginfo )
		{
			
			// phone number in local db?
			if( m_queryPCADB.SearchTel(pDlginfo->telno) )
			{
				CString companyInfo;
				companyInfo = m_queryPCADB.m_division;
				if(!m_queryPCADB.m_department.IsEmpty())
					companyInfo.Format("%s - %s", companyInfo, m_queryPCADB.m_department );

				pDlginfo->company = companyInfo;
				pDlginfo->name = m_queryPCADB.m_name;
				pDlginfo->photoPath = m_queryPCADB.m_pic;
			}
			else
			{
				pDlginfo->company = "";
				pDlginfo->name = "Unknown";
				pDlginfo->photoPath = "";
// added by shen, 20050407
	char buf[256];
	CString strLocalName = g_pdlgUAControl->m_strRegistarName;
	::GetPrivateProfileString(strLocalName, pDlginfo->telno, "", buf, sizeof(buf) - 1, "pcaPic.ini");
	pDlginfo->photoPath = buf;
	::GetPrivateProfileString(strLocalName, pDlginfo->telno+"_name", "Unknown",
	                               buf, sizeof(buf) - 1, "pcaPic.ini");
	pDlginfo->name = buf;
			}
		}
	}

}

void CPCAUADlg::ExitProgram()
{

#ifdef EXPIRE_VERSION	
	// For Test Version
	{
		// Create registery entry if not exist
		CWinApp* pApp = AfxGetApp();
		DWORD dwValue,dwTotal;
		
		dwValue = pApp->GetProfileInt("","TestVersion_start",0);
		dwTotal = pApp->GetProfileInt("","TestVersion_total",0);

		DWORD tick = GetTickCount();
		int interval = (tick-dwValue)/1000;
			
		pApp->WriteProfileInt( "", "TestVersion_total", interval+dwTotal );
	}
#endif

	CPCASplashDlg sDlg;
	sDlg.Create(IDD_SPLASH);
	sDlg.SetSplashText( "Exit program..." );

	CALL_DLG_INFO *pdlginfo;

	m_SysTray.HideIcon();

	// unsubscribe buddy list
	try 
	{
		UnSubscribeAll();
	} catch (...) {}

	// reset presence state
	try 
	{
		UpdatePresence(0, (LPSTR)(LPCTSTR)CUACDlg::GetUAComRegString("User_Info","Username"));
	} catch (...) {}

	// drop all calls in list, if any
	for( int i=0; i<CALLDLG_MAX_COUNT ;i++)
	{
		pdlginfo = m_CallDlgList.GetInfoIdx(i);
		if(pdlginfo)
		{
			try 
			{
				pUAControlDriver->DropCall(pdlginfo->dlgHandle);
			}
			catch (...) {}
		}
	}

	// unregister UA
	try
	{
		pUAControlDriver->UnRegister();
	}
	catch (...) {}

	Sleep(2000);
	// end dialog
	EndDialog(IDOK);
}


BOOL CPCAUADlg::OnEraseBkgnd(CDC* pDC) 
{
	
	CSkinDialog::OnEraseBkgnd(pDC);

	//DrawBitmap(pDC, (HBITMAP)m_bmpRingScreen, CRect(70,28,286,266), FALSE);

	
	if( m_CallDlgList.GetCount()==0 )
	{
		// draw empty mutiple call area 
		DrawBitmap(pDC, (HBITMAP)m_bmpMCPanel, m_rectMCPanel, FALSE);
	}
	else
	{
		// update icon of mutiple call area 
		CALL_DLG_INFO *pdlginfo;
		CRect r = CRect(63,514,79,530);

		for( int i=0; i<CALLDLG_MAX_COUNT ;i++)
		{
			pdlginfo = m_CallDlgList.GetInfoIdx(i);
			if(pdlginfo)
			{
				switch ( pdlginfo->state)
				{
				case CALLDLG_STATE_INBOUND:
					DrawBitmap(pDC, (HBITMAP)m_bmpMCIconAlert, r, FALSE);
					break;
				case CALLDLG_STATE_OUTBOUND:
					DrawBitmap(pDC, (HBITMAP)m_bmpMCIconRingBk, r, FALSE);
					break;
				case CALLDLG_STATE_LOCAL_HOLD:
				case CALLDLG_STATE_REMOTE_HOLD:
					DrawBitmap(pDC, (HBITMAP)m_bmpMCIconHold, r, FALSE);
					break;
				case CALLDLG_STATE_CONNECTED:
				default:
					DrawBitmap(pDC, (HBITMAP)m_bmpMCIconConn, r, FALSE);
					break;
				}
			}
			r.top+=17;
			r.bottom+=17;
		}

	}
	return TRUE;

}

void CPCAUADlg::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized) 
{
	CSkinDialog::OnActivate(nState, pWndOther, bMinimized);
	
	// TODO: Add your message handler code here
	
}

void CPCAUADlg::UpdateRegister()
{
	if( m_uacDlg.m_regResult == REG_RESULT_NONEED )//m_RegCount==-1 )
	{
		// no deal with
	}
	else if ( m_uacDlg.m_regResult == REG_RESULT_INPROGRESS && m_uacDlg.GetUAComRegDW( "Registrar", "Use_Registrar", 0)== 1 )
	{
				
		CString tmp,hint;
		//if( m_RegCount%3==0 ) hint = ".";
		//else if( m_RegCount%3==1) hint = "..";
		//else if( m_RegCount%3==2) hint = "...";

		m_RegCount++;
		tmp.Format("Register...%d",m_RegCount);
		SetDisplayStatus(tmp);
		
	}
	else if( m_uacDlg.m_regResult == REG_RESULT_DONE ) // uacom register successful?)
	{
				
		CString tmp;
		tmp.Format("Register OK");
		SetDisplayStatus(tmp);
				
		//m_RegCount = -1;
		m_uacDlg.m_regResult = REG_RESULT_NONEED;
		m_MsgDelayCounter = 5;

		CheckRegisterOK();
	}
	else
	{
				
		CString tmp;
		tmp.Format("Register Fail");
		SetDisplayStatus(tmp);
				
		//m_RegCount = -1;
		m_uacDlg.m_regResult = REG_RESULT_NONEED;
		m_MsgDelayCounter = 5;
	}
}
void CPCAUADlg::CheckRegisterOK()
{
	
	// update feature buttons
	UpdateFeatureButtons();


#ifdef _XCAPSERVER
		if(g_pdlgPCAExtend->m_ListPersonal.m_hWnd!=NULL)//if list already initialized
			refreshPBook(&g_pdlgPCAExtend->m_ListPersonal);
#endif

#ifdef _SIMPLE
		if(g_pdlgPCAExtend->m_ListPersonal.m_hWnd!=NULL)//if list already initialized
		{
			g_pMainDlg->UpdatePresence(1, (LPSTR)(LPCTSTR)CUACDlg::GetUAComRegString("User_Info","Username"));
			// The order in which SubscribePhoneBook and SubscribeWinfo are invoked 
			// is changed by molisado, 2006/02/14
			// When IMPS cannot retrieve the authorization rules from LMS, IMPS will send winfo NOTIFY 
			// for every watcher who is already authorized
			// In such case, PCA will check the authorization rules on LMS by itself.
			// However, if PCA cannot connect to LMS, it will check if the peer is already in the buddy list
			// stored in local memory. So when PCA is initialized, SubscribePhoneBook must be invoked first, 
			// which will add each buddy to the buddy list
			g_pdlgPCAExtend->SubscribePhoneBook();
			g_pMainDlg->SubscribeWinfo((LPSTR)(LPCTSTR)CUACDlg::GetUAComRegString("User_Info","Username"));
		}
#endif		
}


void CPCAUADlg::UpdateBSSignal()
{
	if( !IsWindow(m_SignalDlg.GetSafeHwnd()) )
	{
		m_SignalDlg.Create(IDD_SIGNALLIST_DIALOG,this);
	}



}

void CPCAUADlg::UpdateSignalDisp()
{
	static int q_prev=0;

	int q = pUAControlDriver->GetMediaReceiveQuality() / 20;
	if (q>4)  q=4;

	if( q != q_prev )
	{
		CString btnName;
		TRACE( "q=%d\n", q);
		for( int i = 0; i<4 ;i++)
		{
			btnName.Format("STATUS_SIGNAL_%d", i );
			SetButtonCheck(btnName, (q>i) );
		}
		q_prev = q;
	}
}

DWORD CPCAUADlg::IPChangeThreadFunc(LPVOID p)
{
	DWORD ret;

	ret = NotifyAddrChange(NULL, NULL);

	if (ret != NO_ERROR)
		return 0;

	::SendMessage(g_pMainDlg->m_hWnd,
		    WM_USER_IPCHANGED, 
		    NULL, 
		    NULL);

	return 0;
}

const char **getAllMyAddr1()
{
 int i;

 static char  bufs[16][32];
 static const char *pbufs[16];
 struct hostent *hp;
 char hostname[256];
 struct in_addr addr;

 for (i = 0; i < 16; i++) {
  bufs[i][0] = '\0';
  pbufs[i] = bufs[i];
 }

 if (::gethostname(hostname, sizeof(hostname)) < 0)
  return pbufs;

 if ((hp = ::gethostbyname(hostname)) == NULL)
  return pbufs;

 for (i = 0; i < 16 && hp->h_addr_list[i]; i++) {
  memcpy(&addr.s_addr, hp->h_addr_list[i], sizeof(addr.s_addr));
  _snprintf(bufs[i], 32, "%s", ::inet_ntoa(addr));
  bufs[i][31] = '\0';
 }

 return pbufs;
}

void CPCAUADlg::OnIPChanged(WPARAM wParam, LPARAM lParam)
{
	// ToDo: change ip and re-connect.
	CIPPrefDlg dlg;
	CString curLocalIP = AfxGetApp()->GetProfileString("","PreferredLocalIP");

	// check original ip exit ? if exit, do nothing, else choose new ip to connect.
	const char** szIPList = getAllMyAddr1();
	for ( int i=0; i< 16; i++) 
	{
		if (strlen(szIPList[i]) > 0)
		{
			if ( curLocalIP == szIPList[i])
				return;
		}
	}

	dlg.m_strLocalIP = curLocalIP;
	
	if( dlg.DoModal()==IDOK )
	{

		AfxGetApp()->WriteProfileString("","PreferredLocalIP",dlg.m_strLocalIP);
		pUAControlDriver->Test("profile refresh");

		CPCASplashDlg sDlg;

		Invalidate();
		UpdateWindow();

		// re-initialize softphone
		if( !InitSoftphone(&sDlg) )
			EndDialog(IDCANCEL);

		ReInit();
	}
}


void CPCAUADlg::OnUpdateMCAnswer(CCmdUI *pCmdUI)
{
	switch ( m_CallDlgList.GetState(m_clickMCHandle) )
	{
	case CALLDLG_STATE_INBOUND:
		pCmdUI->SetText("Answer");
		pCmdUI->Enable(TRUE);
		break;
	case CALLDLG_STATE_OUTBOUND:
	case CALLDLG_STATE_CONNECTED:
	case CALLDLG_STATE_LOCAL_HOLD:
	case CALLDLG_STATE_REMOTE_HOLD:
		pCmdUI->SetText("Hang Up");
		pCmdUI->Enable(TRUE);
		break;
	default:
		pCmdUI->Enable(FALSE);
	}
}

void CPCAUADlg::OnUpdateMCHold(CCmdUI *pCmdUI)
{
	switch ( m_CallDlgList.GetState(m_clickMCHandle) )
	{
	case CALLDLG_STATE_CONNECTED:
		pCmdUI->SetText("Hold");
		pCmdUI->Enable( TRUE);
		break;
	case CALLDLG_STATE_LOCAL_HOLD:
		// allow retrieve if call is held by local and not be held by others
		if ( m_CallDlgList.GetDlgHandleByState(CALLDLG_STATE_REMOTE_HOLD) == -1)
		{
			pCmdUI->SetText("Retrive");
			pCmdUI->Enable( TRUE);
		}
		break;
	default:
		pCmdUI->Enable( FALSE);
	}
}

void CPCAUADlg::OnMCAnswer()
{
	if ( m_CallDlgList.GetState(m_clickMCHandle) == CALLDLG_STATE_INBOUND)
	{
		// if there's another outbound call, cancel it
		int dlgOutbound = m_CallDlgList.GetDlgHandleByState(CALLDLG_STATE_OUTBOUND);
		if ( dlgOutbound != -1)
		{
			// cancel outbound call
			try {
				pUAControlDriver->DropCall(dlgOutbound);
			}
			catch (...)
			{
				MessageBox( "Drop dialing call failed. Can not answer another call.", "SIP Error", MB_ICONERROR|MB_OK);
				return;
			}
		}
		// if there's another connected call, hold it
		int dlgActive = m_CallDlgList.GetActiveDlgHandle();
		if ( dlgActive != -1)
		{
			// hold active call
			try {
				SetDisplayStatus("Holding exist call...");
				
				_DoHoldCall();

				// wait call held in 3 seconds
				for ( int i=0; i<30 && m_CallDlgList.GetState(dlgActive) != CALLDLG_STATE_LOCAL_HOLD; i++)
					Sleep(100);
				if ( m_CallDlgList.GetState(dlgActive) != CALLDLG_STATE_LOCAL_HOLD)
					throw 0;				
			}
			catch (...)
			{
				try {
					pUAControlDriver->DropCall(dlgActive);
				}
				catch (...)
				{
					MessageBox( "Held exist call failed. Can not answer another call.", "SIP Error", MB_ICONERROR|MB_OK);
					return;
				}
			}
		}

		// answer clicked peer
		m_dlgNextAct = m_clickMCHandle;
		SetTimer(ID_TIMER_ANSWERCALL,500,NULL);
		return;
	}
	
	// drop click peer
	try {
		pUAControlDriver->DropCall(m_clickMCHandle);
	}
	catch (...)
	{
		//MessageBox( "Drop Call failed.", "SIP Error", MB_ICONERROR|MB_OK);
		// stop ringing and clear up call list anyway.
		OnUADisconnected( m_clickMCHandle, 200);
	}
}

void CPCAUADlg::OnMCHold()
{
	int dlgActive = m_CallDlgList.GetActiveDlgHandle();

	// hold active call first...
	if( dlgActive != -1)
	{
		try
		{
			_DoHoldCall();
	 		SetDisplayStatus("Holding call...");
		}
		catch (...)
		{
			MessageBox( "Hold Call failed.", "SIP Error", MB_ICONERROR|MB_OK);
		}
	}

	if( m_CallDlgList.GetState(m_clickMCHandle) == CALLDLG_STATE_LOCAL_HOLD )
	{
		// unhold clickd peer
		m_dlgNextAct = m_clickMCHandle;
		SetTimer(ID_TIMER_UNHOLDCALL,500,NULL);
	}
}

void CPCAUADlg::OnDestroy() 
{
	CSkinDialog::OnDestroy();

	// destroy uacontrol and it's parent dialog
	if( m_uacDlg.UAControlDriver)
	{
		m_uacDlg.UAControlDriver.ReleaseDispatch();
	}
}

void CPCAUADlg::OnMcReject() 
{
	int dlgActive = m_CallDlgList.GetActiveDlgHandle();

	if( m_CallDlgList.GetState(m_clickMCHandle) == CALLDLG_STATE_INBOUND )
	{
		// drop click peer
		try { 
			pUAControlDriver->RejectCall(m_clickMCHandle);
		}
		catch (...)
		{
			MessageBox( "Reject Call failed.", "SIP Error", MB_ICONERROR|MB_OK);
		}
	}

}

void CPCAUADlg::OnUpdateMcReject(CCmdUI* pCmdUI) 
{
	switch ( m_CallDlgList.GetState(m_clickMCHandle))
	{
	case CALLDLG_STATE_INBOUND:
		pCmdUI->Enable(TRUE);
		pCmdUI->SetText("Reject");
		break;
	default:
		pCmdUI->Enable(FALSE);
	}
}

void CPCAUADlg::OnMcTransfer() 
{
	// transfer active call to selected holding call
	if( m_CallDlgList.GetState(m_clickMCHandle) == CALLDLG_STATE_LOCAL_HOLD 
		&& m_CallDlgList.GetActiveDlgHandle()!=-1 )
	{	
		int nActiveDlg = m_CallDlgList.GetActiveDlgHandle();

		try 
		{
			// active call must be held first
			m_fActionAfterHeld = true;
			_DoHoldCall();
			SetDisplayStatus("Holding call...");

			// wait call held in 3 seconds
			for ( int i=0; i<30 && m_CallDlgList.GetState(nActiveDlg) != CALLDLG_STATE_LOCAL_HOLD; i++)
				Sleep(100);

			if ( m_CallDlgList.GetState(nActiveDlg) != CALLDLG_STATE_LOCAL_HOLD)
				throw 0;

			// apply attend transfer
			pUAControlDriver->AttendXfer( m_clickMCHandle, nActiveDlg );
			SetDisplayStatus("Transfering call...");
		}
		catch (...)
		{
			MessageBox( "Transfer Call failed.", "SIP Error", MB_ICONERROR|MB_OK);
		}
	}

}

void CPCAUADlg::OnUpdateMcTransfer(CCmdUI* pCmdUI) 
{
	// enable if exist connected call & call is holding by local
	if ( m_CallDlgList.GetActiveDlgHandle() != -1 &&
		 m_CallDlgList.GetState(m_clickMCHandle) == CALLDLG_STATE_LOCAL_HOLD )
	{
		pCmdUI->Enable(TRUE);
	}
	else
	{
		pCmdUI->Enable(FALSE);
	}
	
}

/*
<VONTEL-Extend-Request version="1.0">
	<ConferenceCall type="adhoc">
		<Usuage mode="AudioConferenceRoom|VideoConferenceRoom"/>
		<HeldParty type="callid"|"deviceid"> callid | deviceid </HeldCall>
		<ConnectedParty type="callid"|"deviceid"> callid | deviceid </ConnectedCall>
		[ <LocalConfRole> normal | monitor | coach | pupil </LocalConfRole> ]
	</ConferenceCall>
</VONTEL-Extend-Request>
*/

void CPCAUADlg::OnMcConference() 
{
	// conference active call to selected holding call
	if( m_CallDlgList.GetState(m_clickMCHandle) == CALLDLG_STATE_LOCAL_HOLD 
		&& m_CallDlgList.GetActiveDlgHandle()!=-1 )
	{	
		int nActiveDlg = m_CallDlgList.GetActiveDlgHandle();

		try 
		{
			// save dlg handle for later use
			m_nConfDlg1 = m_clickMCHandle;
			m_nConfDlg2 = m_CallDlgList.GetActiveDlgHandle();

//marked by alan
//			CString strHeldDevice = pUAControlDriver->GetRemoteTarget( m_clickMCHandle);
//			CString strActiveDevice = pUAControlDriver->GetRemoteTarget( m_CallDlgList.GetActiveDlgHandle() );
			CString strHeldDevice ;
			CString strActiveDevice ;

			// active call must be held first
			m_fActionAfterHeld = true;
			_DoHoldCall();
			SetDisplayStatus("Holding call...");

			// wait call held in 3 seconds
			for ( int i=0; i<30 && m_CallDlgList.GetState(nActiveDlg) != CALLDLG_STATE_LOCAL_HOLD; i++)
				Sleep(100);

			if ( m_CallDlgList.GetState(nActiveDlg) != CALLDLG_STATE_LOCAL_HOLD)
				throw 0;

			// build Conference request
			CString strRequest;
			strRequest.Format(  
				"<?xml version=\"1.0\" ?>\n"
				"<VONTEL-Extend-Request version=\"1.0\">\n"
				"<ConferenceCall type=\"adhoc\">\n"
				"\t<Usuage mode=\"AudioConferenceRoom\"/>\n"
				"\t<HeldParty type=\"deviceid\">%s</HeldParty>\n"
				"\t<ConnectedParty type=\"deviceid\">%s</ConnectedParty>\n"
				"</ConferenceCall>\n"
				"</VONTEL-Extend-Request>",
					strHeldDevice, strActiveDevice);

			// send SIP INFO request
			pUAControlDriver->SendInfo( "sip:" + m_uacDlg.m_strRegistarAddress, "text/xml", strRequest);
			SetDisplayStatus("Conferencing call...");
		}
		catch (...)
		{
			MessageBox( "Conference Call failed.", "SIP Error", MB_ICONERROR|MB_OK);
		}
	}
}

void CPCAUADlg::OnUpdateMcConference(CCmdUI* pCmdUI) 
{
	if ( !m_bEnableVideoConference)
		pCmdUI->SetText( "Conference");
	else
		pCmdUI->SetText( "Audio Conference");

	// enable if exist connected call & call is holding
	if ( m_CallDlgList.GetActiveDlgHandle() != -1 &&
		 m_CallDlgList.GetState(m_clickMCHandle) == CALLDLG_STATE_LOCAL_HOLD )
	{
		pCmdUI->Enable(TRUE);
	}
	else
	{
		pCmdUI->Enable(FALSE);
	}
}

// replace $variable$ in strNumber
/*
  - supported $variable$ in strNumber are:
	$dial_number$ = user's input digit
	$registrar$ = registrar's host name or ip
*/
CString CPCAUADlg::ConvertNumberToURI( CString strNumber)
{
	CString strResult = m_strNumberToURITemplate;

	while (true)
	{
		// find $..$ in strLink
		int start = strResult.Find('$');
		int end = strResult.Find('$',start+1);
		if ( start == -1 || end == -1)
			break;	// no more $variables$

		CString strVar = strResult.Mid( start+1, end-start-1);

		// get the value of $variable$ 
		CString strValue;
		if ( strVar == "dial_number")
		{
			strValue = strNumber;
		}
		else if ( strVar == "registrar")
		{
			strValue = m_uacDlg.m_strRegistarAddress;
			
		} 
		else if ( strVar == "callserver" )
		{
			strValue = m_uacDlg.m_strCallServerAddr;
		}
		// replace it
		strResult = strResult.Left( start) + strValue + strResult.Mid( end+1);
	}

	return strResult;
}

void CPCAUADlg::OnUAInfoResponse(LPCTSTR remoteURI, long StatusCode, LPCTSTR contentType, LPCTSTR body) 
{
/*
QueryDeviceSelfInfo response format:

<?xml version="1.0" ?> 
<VONTEL-Extend-Response version="1.0">
	<QueryDeviceSelfInfo>
		<PhysicalAddress> [physical address] </PhysicalAddress>
		<RegisterInfo> [register info] </RegisterInfo>
		<Registered> [yes|no] </Registered>
		<Number> [device number] </Number>
		<Alias> [device alias] </Alias>
		
[ begin - for station device only]

		<DoNotDistrub> [on|off] </DoNotDistrub>
		<NoServiceForward> [on|off] </NoServiceForward>
		<NoServiceForwardTarget> [uri] </NoServiceForwardTarget>
		<DirectForward> [on|off] </DirectForward>
		<DirectForwardTarget> [uri] </DirectForwardTarget>
		<BusyForward> [on|off] </BusyForward>
		<BusyForwardTarget> [uri] </BusyForwardTarget>
		<NoAnswerForward> [on|off] </NoAnswerForward>
		<NoAnswerForwardTarget> [uri] </NoAnswerForwardTarget>

[ end - for station device only]

	</QueryDeviceSelfInfo>
</VONTEL-Extend-Response>
*/
	// parse response
	IXMLDOMDocumentPtr spXML;
	if ( FAILED(spXML.CreateInstance( MSXML::CLSID_DOMDocument)) )
		return;
	if ( !spXML->loadXML( body))
		return;
	
	// check command node
	IXMLDOMNodePtr spCmdNode = spXML->selectSingleNode("/VONTEL-Extend-Response/QueryDeviceSelfInfo");
	if ( spCmdNode)
	{
		IXMLDOMNodePtr spNode = spCmdNode->selectSingleNode("DoNotDisturb");
		if ( spNode)
		{
			bool bOn = stricmp( spNode->text, "on") == 0;
			SetButtonCheck( "STATUS_DND", bOn );
			SetButtonEnable( "STATUS_DND", bOn );
		}
		spNode = spCmdNode->selectSingleNode("DirectForward");
		if ( spNode)
		{
			bool bOn = stricmp( spNode->text, "on") == 0;
			SetButtonCheck( "STATUS_FOLLOW", bOn );
			SetButtonEnable( "STATUS_FOLLOW", bOn );
		}
	}

	spCmdNode = spXML->selectSingleNode("/VONTEL-Extend-Response/ConferenceCall");
	if ( spCmdNode)
	{
		IXMLDOMNodePtr spNode = spCmdNode->selectSingleNode("Result[@Code=\"200\"]");
		if ( !(bool)spNode)
		{
			// conference failed
			SetDisplayStatus("Conference Failed");
			// delay message for 5 seconds
			m_MsgDelayCounter = 5;
		}
		else
		{
			// conference ok
			if ( spCmdNode->selectSingleNode("ConferenceRoom/Usuage[@mode=\"VideoConferenceRoom\"]") )
			{
				// a video conference room reserved
				// transfer call to conference room
				spNode = spCmdNode->selectSingleNode("ConferenceRoom/ID");
				if ( spNode)
				{
					CString strConfURI = (LPCSTR)spNode->text;

					if ( m_CallDlgList.GetFlagConference( m_nConfDlg1))
					{
						// use exist conference
						pUAControlDriver->UnHoldCall( m_nConfDlg1);
						// transfer exist parties
						pUAControlDriver->UnAttendXfer( m_nConfDlg2, strConfURI);
					}
					else
					{
						// invite to new conference
						int dlgConf = pUAControlDriver->MakeCall( strConfURI);
						// transfer exist parties
						pUAControlDriver->UnAttendXfer( m_nConfDlg1, strConfURI);
						pUAControlDriver->UnAttendXfer( m_nConfDlg2, strConfURI);

						// mark the conference room
						m_CallDlgList.SetFlagConference( dlgConf, TRUE);
					}
				}
			}
			else
			{
				// a audio conference established
				// nothing to do...
			}
		}
	}
}

void CPCAUADlg::OnUAInfoRequest(LPCTSTR remoteURI, LPCTSTR contentType, LPCTSTR reqBody, long& rspStatusCode, CString& rspBody)
{
	// parse request
	IXMLDOMDocumentPtr spRequest;
	if ( FAILED(spRequest.CreateInstance( MSXML::CLSID_DOMDocument)) )
		return;
	if ( !spRequest->loadXML( reqBody))
		return;

	// check command node
	IXMLDOMNodePtr spCmdNode = spRequest->selectSingleNode("/VONTEL-Extend-Request/SetIndicator");
	if ( spCmdNode)
	{
		/* Request Format
			<?xml version=\"1.0\" ?>
			<VONTEL-Extend-Request version=\"1.0\">
				<SetIndicator>
					<Name> [DirectForward|DoNotDisturb|VoiceMail|CampCall] </Name>
					<Value> [on|off] </Value>
				</SetIndicator>
			</VONTEL-Extend-Request>
		*/

		IXMLDOMNodePtr spName = spCmdNode->selectSingleNode("Name");
		IXMLDOMNodePtr spValue = spCmdNode->selectSingleNode("Value");
		if ( (bool)spName && (bool)spValue)
		{
			CString strName = (LPCTSTR)spName->text;
			CString strButtonName;
			if ( strName == "DoNotDisturb")
				strButtonName = "STATUS_DND";
			else if ( strName == "DirectForward")
				strButtonName = "STATUS_FOLLOW";
			else if ( strName == "VoiceMail")
				strButtonName = "STATUS_VMAIL";
			else if ( strName == "CampCall")
				strButtonName = "STATUS_CAMP";
				
			if ( !strButtonName.IsEmpty())
			{
				BOOL bOn = stricmp( spValue->text, "on") == 0;
				SetButtonCheck( strButtonName, bOn );
				SetButtonEnable( strButtonName, bOn );
			}

		}
	}
}

void CPCAUADlg::OnPresenceOffline() 
{
	UpdatePresence(0, (LPSTR)(LPCTSTR)CUACDlg::GetUAComRegString("User_Info","Username"));
}

void CPCAUADlg::OnPresenceBusy() 
{
	UpdatePresence(1, (LPSTR)(LPCTSTR)CUACDlg::GetUAComRegString("User_Info","Username"));	
}

void CPCAUADlg::OnPresenceEat() 
{
	UpdatePresence(1, (LPSTR)(LPCTSTR)CUACDlg::GetUAComRegString("User_Info","Username"));
}

void CPCAUADlg::OnPresenceMeeting() 
{
	UpdatePresence(1, (LPSTR)(LPCTSTR)CUACDlg::GetUAComRegString("User_Info","Username"));
}

void CPCAUADlg::OnPresenceOnline() 
{
	UpdatePresence(1, (LPSTR)(LPCTSTR)CUACDlg::GetUAComRegString("User_Info","Username"));	
}

void CPCAUADlg::DoShowInstantMsg()
{
	CMenu popMenu;
	popMenu.LoadMenu(IDR_MAINFRAME);

	CPoint posMouse;
	GetCursorPos(&posMouse);

	popMenu.GetSubMenu(6)->TrackPopupMenu(TPM_RIGHTALIGN|TPM_LEFTBUTTON, posMouse.x, posMouse.y, this);
}

#ifdef _SIMPLE
//add by alan,20050234
void CPCAUADlg::OnRegExpired(WPARAM wParam, LPARAM lParam)
{
	if(m_strPresence==PRE_OFFLINE)
		g_pMainDlg->UpdatePresence(0, (LPSTR)(LPCTSTR)CUACDlg::GetUAComRegString("User_Info","Username"));
	else 
		g_pMainDlg->UpdatePresence(1, (LPSTR)(LPCTSTR)CUACDlg::GetUAComRegString("User_Info","Username"));
	g_pMainDlg->SubscribeWinfo((LPSTR)(LPCTSTR)CUACDlg::GetUAComRegString("User_Info","Username"));
	g_pdlgPCAExtend->SubscribePhoneBook();	
}

void CPCAUADlg::OnRecvNeedAuth(WPARAM wParam, LPARAM lParam)
{
	// Added by molisado, 2006/02/14
	// In case IMPS cannot retrieve the authorization rule on LMS, 
	// PCA will check on its own. In case PCA cannot retrieve the authorization rule,
	// it will check if the peer is already in the buddy list stored in local memory
	// because whenever a buddy is added to the buddy list, its authorization rule must be set
	if (isRuleExist((char*)lParam))
		return;
	if (FindBuddy((LPCTSTR)lParam))
		return;

	char showMsg[100];
	sprintf(showMsg,"%s invite you adding his address into your contact list. Do you accept?",(LPCTSTR)wParam);
	int authRet = AfxMessageBox(showMsg,MB_YESNO|MB_ICONQUESTION);

#ifdef _XCAPSERVER
	if(authRet==IDYES)//if press YES.....create the pres-rule for that user through XCAP
	{

		CAddPersonDisplayNameDlg dlg;

		dlg.m_Tel= (LPCTSTR)wParam;
		int Count = m_dlgRightPanel.m_ListPersonal.GetItemCount();
		bool bFound=false;
		int i;
		dlg.m_DisplayName.Empty();
		if(Count>0)
		{
			for( i=0;i<Count;i++)
			{
//				if(m_dlgRightPanel.m_ListPersonal.GetItemText(i,defPersonalName) == (LPCTSTR)wParam)
				if(m_dlgRightPanel.m_ListPersonal.GetItemText(i,defPersonalTelephone) == dlg.m_Tel)
				{
					bFound = true;
					dlg.m_DisplayName = m_dlgRightPanel.m_ListPersonal.GetItemText(i,defPersonalName);
					break;
				}
			}
		}

		int result;
		result=dlg.DoModal();
	

		if (result==IDOK && dlg.isEditBoxReadable)
		{
			Count = m_dlgRightPanel.m_ListPersonal.GetItemCount();
			bFound=false;
			if(Count>0)
			{
				for(i=0;i<Count;i++)
				{
//					if(m_dlgRightPanel.m_ListPersonal.GetItemText(i,defPersonalName) == (LPCTSTR)wParam)
					if(m_dlgRightPanel.m_ListPersonal.GetItemText(i,defPersonalName) == dlg.m_DisplayName)
					{
						bFound = true;
						AfxMessageBox("Display name already exist!!");
						break;
					}
				}
			}

			if(!isRuleExist((char *)(LPCTSTR)wParam))//if rule not exist
			{
				if(!AddNewRule((char *)(LPCTSTR)wParam,"allow"))
				{
					AfxMessageBox("Add New Authorization Rule Error!");
					return;
				}
			}
			else
				SetRuleAction2Allow((char *)(LPCTSTR)wParam);
			
			InfoDB add_info;
			if(!bFound)
			{
				add_info.m_name = dlg.m_DisplayName;
				add_info.m_group = DEFAULT_GROUPNAME;
				add_info.m_telephone = (LPCTSTR)wParam;
				if(!m_dlgRightPanel.AddDB(&add_info))
				{
					AfxMessageBox("Add Buddy into PhoneBook Error!");
					return;
				}
				else
				{
					//m_dlgRightPanel.m_ListPersonal.SetItemData(m_dlgRightPanel.m_ListPersonal.AddItem(Presence_Unknown, _T(add_info.m_name), _T(DEFAULT_GROUPNAME),_T("Unknown"), _T("Unknown"), _T("Unknow"), _T("Unknow"), _T("Unknow")),0);
					m_dlgRightPanel.m_ListPersonal.SetItemData(m_dlgRightPanel.m_ListPersonal.AddItem(Presence_Offline, _T(add_info.m_name), _T(DEFAULT_GROUPNAME),_T(PRE_OFFLINE), _T(PRE_OFFLINE), _T(add_info.m_telephone), _T("Unknow"), _T("Unknow")),0);
					Subscribe((char *)(LPCTSTR)wParam);
				}
			}
		

		}


		else if(!dlg.isEditBoxReadable)//if only authorize the user
		{
			if(!isRuleExist((char *)(LPCTSTR)wParam))//if rule not exist
			{
				if(!AddNewRule((char *)(LPCTSTR)wParam,"allow"))
				{
					AfxMessageBox("Add New Authorization Rule Error!");
					return;
				}
			}
			Subscribe((char *)(LPCTSTR)wParam);
		}
		
	}
	else
	{
		if(!isRuleExist((char *)(LPCTSTR)wParam))//if rule not exist
		{
			if(!AddNewRule((char *)(LPCTSTR)wParam,"block"))
			{
				AfxMessageBox("Add New Authorization Rule Error!");
				return;
			}
		}
//		SetRuleAction2Block((char *)(LPCTSTR)wParam);
	}

#endif
	 
}
#endif

// message wrapper to create dialog successfully in a com event hander
LRESULT CPCAUADlg::OnRecvMsg(WPARAM wParam, LPARAM lParam)
{
	OnUAReceivedText( (LPCTSTR)wParam, (LPCTSTR)lParam);
	return 0;
}

void CPCAUADlg::OnUAReceivedText( LPCTSTR szRemoteURI, LPCTSTR szMessage)
{
	char buf[128],tmp_name[64];
	char *_split=strstr((char *)szRemoteURI,"@");
	if(_split!=NULL)
	{
	
		strncpy(tmp_name,(char *)szRemoteURI+4,strlen(szRemoteURI)-strlen(_split)-4);
		tmp_name[strlen(szRemoteURI)-strlen(_split)-4]='\0';
		if(GetRuleActionByURI(tmp_name,buf)!=false)
		{
			if(!_stricmp(buf,"block"))//if the user is blocked .....ignore it..
				return;
		}
	}
	else
		strcpy(tmp_name, szRemoteURI);

	//add by alan, get ua name only
	
	
	CDlgMessage* dlg = CDlgMessage::GetDlgByMessage( szRemoteURI, szMessage);
	if ( dlg)
		dlg->ReceiveInstMsg(tmp_name, szMessage);//szRemoteURI
}

void CPCAUADlg::OnUpdateMessageTo(CCmdUI* pCmdUI) 
{
	if ( m_strDialNum.IsEmpty())
	{
		int dlgActive = m_CallDlgList.GetActiveDlgHandle();
		if ( dlgActive == -1)
			pCmdUI->Enable(false);
		else
		{
			CString strRemote = m_CallDlgList.GetInfo(dlgActive)->telno;
			pCmdUI->Enable(true);
			pCmdUI->SetText( "Message to " + strRemote);
		}
	}
	else
	{
		pCmdUI->Enable(true);
		pCmdUI->SetText( "Message to " + m_strDialNum);
	}
}

void CPCAUADlg::OnMessageTo() 
{
	CString strName;
	CString strURI;

	if ( m_strDialNum.IsEmpty())
	{
		int dlgActive = m_CallDlgList.GetActiveDlgHandle();
		if ( dlgActive != -1)
		{
			strURI = m_CallDlgList.GetInfo(dlgActive)->telno;
			strName = m_CallDlgList.GetInfo(dlgActive)->name;
		}
	}
	else
	{
		strURI = strName = m_strDialNum;
		m_strDialNum.Empty();
		SetDisplayDigits();
	}

	if ( strURI.Left(4) != "sip:")
		strURI = ConvertNumberToURI(strURI);

	new CDlgMessage( this, strURI, strName );
}

void CPCAUADlg::OnMessageUri() 
{
	CEnterURIDlg dlg;
	if ( dlg.DoModal() != IDOK || dlg.m_strURI.IsEmpty() )
		return;

	CString uri = dlg.m_strURI;
	new CDlgMessage( this, uri, NULL);
}

void CPCAUADlg::OnUpdateMcVideoConference(CCmdUI* pCmdUI) 
{
	// enable if exist connected call & call is holding
	if ( m_CallDlgList.GetActiveDlgHandle() != -1 &&
		 m_CallDlgList.GetState(m_clickMCHandle) == CALLDLG_STATE_LOCAL_HOLD &&
		 m_bEnableVideoConference )
	{
		pCmdUI->Enable(TRUE);
	}
	else
	{
		pCmdUI->Enable(FALSE);
	}
}

void CPCAUADlg::OnMcVideoConference() 
{
	// conference active call to selected holding call
	if( m_CallDlgList.GetState(m_clickMCHandle) == CALLDLG_STATE_LOCAL_HOLD 
		&& m_CallDlgList.GetActiveDlgHandle()!=-1 )
	{	
		int nActiveDlg = m_CallDlgList.GetActiveDlgHandle();

		try 
		{
			// save dlg handle for later use
			m_nConfDlg1 = m_clickMCHandle;
			m_nConfDlg2 = m_CallDlgList.GetActiveDlgHandle();
//marked by alan
//			CString strHeldDevice = pUAControlDriver->GetRemoteTarget( m_clickMCHandle);
//			CString strActiveDevice = pUAControlDriver->GetRemoteTarget( m_CallDlgList.GetActiveDlgHandle() );
			CString strHeldDevice ;
			CString strActiveDevice ;
			// active call must be held first
			m_fActionAfterHeld = true;
			_DoHoldCall();
			SetDisplayStatus("Holding call...");

			// wait call held in 3 seconds
			for ( int i=0; i<30 && m_CallDlgList.GetState(nActiveDlg) != CALLDLG_STATE_LOCAL_HOLD; i++)
				Sleep(100);

			if ( m_CallDlgList.GetState(nActiveDlg) != CALLDLG_STATE_LOCAL_HOLD)
				throw 0;

			// build Conference request
			CString strRequest;
			strRequest.Format(  
				"<?xml version=\"1.0\" ?>\n"
				"<VONTEL-Extend-Request version=\"1.0\">\n"
				"<ConferenceCall type=\"adhoc\">\n"
				"\t<Usuage mode=\"VideoConferenceRoom\"/>\n"
				"\t<HeldParty type=\"deviceid\">%s</HeldParty>\n"
				"\t<ConnectedParty type=\"deviceid\">%s</ConnectedParty>\n"
				"</ConferenceCall>\n"
				"</VONTEL-Extend-Request>",
					strHeldDevice, strActiveDevice);

			// send SIP INFO request
			pUAControlDriver->SendInfo( "sip:" + m_uacDlg.m_strRegistarAddress, "text/xml", strRequest);
			SetDisplayStatus("Conferencing call...");
		}
		catch (...)
		{
			MessageBox( "Conference Call failed.", "SIP Error", MB_ICONERROR|MB_OK);
		}
	}
}

// added by shen, 20041225
// modified by molisado, 20050406
int CPCAUADlg::GetBuddyPresence(CString strNumber)
{
	return m_dlgRightPanel.GetBuddyPresence(strNumber);
}

// added by molisado, 20050406
bool CPCAUADlg::GetPresenceState()
{
	return ( m_strPresence == PRE_ONLINE );
}

