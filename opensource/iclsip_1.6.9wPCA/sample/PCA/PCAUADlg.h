// PCAUADlg.h : header file
//


#if !defined(AFX_PCAUADLG_H__E0DFE2A1_80C2_45D8_B0AC_4BA15C9F3374__INCLUDED_)
#define AFX_PCAUADLG_H__E0DFE2A1_80C2_45D8_B0AC_4BA15C9F3374__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SystemTray.h"
#include "UACDlg.h"
#include "DlgBrowser.h"

// right panel window
#include "PanelDlg.h"
#include "QueryDB.h"
#include "Picture.h"
#include "CallDlgList.h"
#include "DlgBrowser.h"
#include "BSOperation.h" // wireless detect
#include "SignalListDlg.h"


#define ID_TOOLLINK_FIRST	40000
#define ID_TOOLLINK_LAST	40999

#define ID_FEATURE_FIRST	50000
#define ID_FEATURE_LAST		50999

#define WM_USER_DNDREJECT	(WM_USER+100)
#define WM_USER_RECVMSG		(WM_USER+101)
#define WM_USER_RECVNEEDAUTH (WM_USER+102)
//add by alan,20050223
#define WM_USER_REGEXPIRED (WM_USER+103)

#define WM_USER_IPCHANGED	(WM_USER+104)

#define PRE_OFFLINE			"Offline"
#define PRE_ONLINE			"Online"



/////////////////////////////////////////////////////////////////////////////
// CPCAUADlg dialog


class CPCAUADlg : public CSkinDialog
{
// Construction
public:
	// added by molisado, 20050406
	bool GetPresenceState();
	CPCAUADlg(CWnd* pParent = NULL);	// standard constructor
	~CPCAUADlg();

	BOOL m_bMinimizeOnStart;
	void UpdateSignalDisp();
	void ExitProgram();
	void FillDlgPeerInfo( int idx );
	void TextClicked(CString m_TextName);
	void UpdateCallDlgListDisp();
	CString GetPeerTelno(int dlg, int type);
	void DragWindow(int x, int y );
	BOOL InitSoftphone( CDialog* pSpDlg );
	void UpdatePeerInfoDisp( BOOL bClear=FALSE );
	void DialFromAddrBook( LPCTSTR dialNum );
	void SetRightPanelPage( int pageID);
	DWORD VersionStr2DW( LPCTSTR strVer);
	void ShowMainMenu();
	void SetDisplayDigits();
	void PressDigit( char ch );

	CString URI2PhoneNum( LPCTSTR uri );
	CString ConvertNumberToURI( CString strNumber);

	void OnUARingback(long dlgHandle);
	void OnUAAlerting(long dlgHandle);
	void OnUAProceeding(long dlgHandle);
	void OnUAConnected(long dlgHandle);
	void OnUADisconnected(long dlgHandle,long StatusCode);
	void OnUATimeOut(long dlgHandle);
	void OnUADialing(long dlgHandle);
	void OnUABusy(long dlgHandle);
	void OnUAReject(long dlgHandle);
	void OnUATransferred(long dlgHandle, LPCTSTR xferURL);
	void OnUAHeld(long dlgHandle);
	void OnUAWaiting(long dlgHandle);
	void OnUAReceivedText(LPCTSTR szRemoteURI, LPCTSTR szMessage);
	void OnUAInfoResponse(LPCTSTR remoteURI, long StatusCode, LPCTSTR contentType, LPCTSTR body);
	void OnUAInfoRequest(LPCTSTR remoteURI, LPCTSTR contentType, LPCTSTR reqBody, long& rspStatusCode, CString& rspBody);
	void OnBuddyUpdateStatus(LPCTSTR buddyURI, BOOL Open, LPCTSTR strMsgState, LPCTSTR strCallState);

	bool Subscribe(CString strNumber);
	bool UnSubscribe(CString strNumber);
	void UnSubscribeAll();
	void UpdatePresence(int nPresence, CString strDes = "ready");
//add by alan
	bool unSubscribeWinfo(CString strNumber);
	bool SubscribeWinfo(CString strNumber);
	bool BlockBuddy(CString strNumber);
	bool UnBlockBuddy(CString strNumber);
	bool unSubscribe(CString strNumber);

	// configuration for display control
	CString m_strHelpTitle;
	CString m_strHelpUserGuildLink;
	CString m_strHelpHotlineNumber;
	CString m_strHelpEmailAddress;
	CString m_strHelpWebSupportLink;
	CString m_strAboutTitle;
	CString m_strAboutProductName;
	CString m_strAboutCopyright;

	// configuration for feature enable/disable
	bool m_bEnableVideo;
	bool m_bEnableInstantMessage;
	bool m_bEnableLocalAddressBook;
	bool m_bEnablePublicAddressBook;	// for CPanelDlg
	bool m_bEnableCallLog;
	bool m_bEnableVoiceMail;
	bool m_bEnableVideoConference;		// support N100 SIP MCU

	//! Dual Net
	void UpdateBSSignal();
	BOOL m_bInitDualNet;
	DxLst BSLst;
	pBSEntity pBSInfo;
	CProgressListDlg m_SignalDlg;
	CRITICAL_SECTION m_BSCS;
	BOOL m_bDoBSOK;

	//! Register
	void UpdateRegister();
	void CheckRegisterOK();
	int m_RegCount;

	//! initial variable when every re-initial PCA
	void ReInit();

	//! chage to wireless interface
	void ChangeToNewIP();
	void SetChangeIP(CString ip) { m_wireAddr=ip; };
	CString GetChangeIP() { return m_wireAddr; } ;
	CString m_wireAddr;

	//! detect IP changed ?
	static DWORD __stdcall IPChangeThreadFunc(LPVOID p);

	void UpdateFeatureButtons();
	void UpdateStatusIndicator();

	// added by shen, 20041225
	// modified by molisado, 20050406
	int GetBuddyPresence(CString strNumber);

	HACCEL m_hAccel;

// Dialog Data
	//{{AFX_DATA(CPCAUADlg)
	enum { IDD = IDD_PCAUA_DIALOG };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPCAUADlg)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CString GetDeployServerPage(LPCTSTR page);
	void	AddCallLog(int dlgHandle, int callResult );
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CPCAUADlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnKey0();
	afx_msg void OnKey1();
	afx_msg void OnKey2();
	afx_msg void OnKey3();
	afx_msg void OnKey4();
	afx_msg void OnKey5();
	afx_msg void OnKey6();
	afx_msg void OnKey7();
	afx_msg void OnKey8();
	afx_msg void OnKey9();
	afx_msg void OnKeyEnter();
	afx_msg void OnKeyEsc();
	afx_msg void OnKeyStar();
	afx_msg void OnKeyBack();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPreference();
	afx_msg void OnExitprog();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnShowabout();
	afx_msg void OnKeyDel();
	afx_msg void OnKeySpace();
	afx_msg void OnKeyF10();
	afx_msg void OnKeyF1();
	afx_msg void OnUpdateHoldcall(CCmdUI* pCmdUI);
	afx_msg void OnHoldcall();
	afx_msg void OnTransfercall();
	afx_msg void OnUpdateTransfercall(CCmdUI* pCmdUI);
	afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
	afx_msg void OnRedial();
	afx_msg void OnDialuri();
	afx_msg void OnUpdateRedial(CCmdUI* pCmdUI);
	afx_msg void OnKeyUacomPref();
	afx_msg void OnMoving(UINT fwSide, LPRECT pRect);
	afx_msg void OnUpdatePreference(CCmdUI* pCmdUI);
	afx_msg void OnPCAHelp();
	afx_msg void OnMove(int x, int y);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnMCAnswer();
	afx_msg void OnUpdateMCAnswer(CCmdUI* pCmdUI);
	afx_msg void OnMCHold();
	afx_msg void OnUpdateMCHold(CCmdUI* pCmdUI);
	afx_msg void OnDestroy();
	afx_msg void OnMcReject();
	afx_msg void OnUpdateMcReject(CCmdUI* pCmdUI);
	afx_msg void OnMcTransfer();
	afx_msg void OnUpdateMcTransfer(CCmdUI* pCmdUI);
	afx_msg void OnMcConference();
	afx_msg void OnUpdateMcConference(CCmdUI* pCmdUI);
	afx_msg void OnPresenceOffline();
	afx_msg void OnPresenceBusy();
	afx_msg void OnPresenceEat();
	afx_msg void OnPresenceMeeting();
	afx_msg void OnPresenceOnline();
 	afx_msg void OnToolLink(UINT nID);
 	afx_msg void OnUpdateToolLink(CCmdUI* pCmdUI);
 	afx_msg void OnFeaturePanel(UINT nID);
	afx_msg void OnUpdateMessageTo(CCmdUI* pCmdUI);
	afx_msg void OnMessageTo();
	afx_msg void OnMessageUri();
	afx_msg void OnUpdateMcVideoConference(CCmdUI* pCmdUI);
	afx_msg void OnMcVideoConference();
	afx_msg void OnIPChanged(WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG

	afx_msg LRESULT OnDNDReject(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnRecvMsg(WPARAM wParam, LPARAM lParam);
#ifdef _SIMPLE
	afx_msg void OnRecvNeedAuth(WPARAM wParam, LPARAM lParam);
//add by alan,20050223
	afx_msg void OnRegExpired(WPARAM wParam, LPARAM lParam);
#endif

	DECLARE_MESSAGE_MAP()
private:
	BOOL CreateRegEntry();
	int DownloadServerConfig();
	void DoSwitchRightPanel();
	int CheckNewVersion();
	void ShowDialMenu();
	void DoSwitchUAVideo();
	void DoShowToolLinks();
	void DoShowFeaturePanel();
	void DoShowConfigLink();
	void DoShowInstantMsg();
	bool FindBuddy(CString strURI);

	CUACDlg m_uacDlg;

	BOOL m_bEnding;
	void DoUIDnD();
	void DoUITransfer();
	void DoUIHold();
	void AniRingAlert( BOOL bEnable );
	void DoUIDial();
	void DoUICancel();

	void SetDisplayStatus( CString status);
	void UpdatePhoneState();
	void ButtonPressed(CString m_ButtonName);
	CString m_strDialNum;
	CString m_strLastDialURI;

	BOOL m_fActionAfterHeld;	// this flag is used to prevent OnUAHeld calls SetDisplayStatus("Held");

	IUAControl	*pUAControlDriver;
	CSystemTray m_SysTray;
	BOOL m_bEnableDND;
	BOOL m_bAllowCallWaiting;
	BOOL m_bShowVideo;
	BOOL m_bUICancel;
	BOOL m_bUIAnswer;
	int m_MsgDelayCounter;
	CPanelDlg m_dlgRightPanel;
	BOOL m_bShowRightPanel;

	CQueryDB m_queryPCADB;

	CPicture m_PeerPhoto;

	CRect m_PeerPhotoRect;
	int m_nUnanswerCalls;

	int m_curPageId;
	CRect m_rcMoving;
	CCallDlgList m_CallDlgList;
	int m_dlgNextAct;
	int m_exitTimeout;

	//CBitmap m_bmpRingScreen;
	CBitmap m_bmpMCPanel;
	CRect m_rectMCPanel;

	int m_clickMCHandle;

	CBitmap m_bmpMCIconAlert;
	CBitmap m_bmpMCIconRingBk;
	CBitmap m_bmpMCIconHold;
	CBitmap m_bmpMCIconConn;

	long m_dlgReject;

	bool m_bCallConnected;

	CStringArray m_ConfigMode;
	int m_curConfigMode;


	time_t m_lastHoldCallTimestamp;
	void _DoHoldCall()
	{
		time( &m_lastHoldCallTimestamp);
		pUAControlDriver->HoldCall();
	}
	bool _IsLocalHoldCall()
	{
		time_t now;
		time(&now);
		if ( now - m_lastHoldCallTimestamp < 5)
			return true;
		else
			return false;
	}


private:
	// general properties for a link
	class SLinkInfo
	{
	public:
		SLinkInfo();
		~SLinkInfo();
		void Show();
		void Hide();

		CString m_strTitle;
		CString m_strLink;
		CRect m_position;
		CDlgBrowser* m_dlgBrowser;
	};
	friend class SLinkInfo;

	struct SFeatures
	{
		CString strTitle;
		CString strDialString;
	};
	
	SLinkInfo m_ConfigLink;		// the link used for configuration

	// the setting of tool links
	CArray<SLinkInfo,SLinkInfo&> m_vecToolLinks;

	// the title of feature panel
	CArray<SFeatures, SFeatures&> m_vecFeatures;

	// replace $variable$ in config/tool links
	CString _ResolveFullLink( CString strLink);

	// create tool links menu on-the-fly
	void _CreateToolLinkMenu( CMenu& menu);

	// create feature panel
	void _CreateFeaturePanelMenu( CMenu& menu);

	// the setting of numbering rule
	CString m_strNumberToURITemplate;

	CString _DisconnectReasonToText(long dlgHandle,long StatusCode);

	// conferencing dlg handles
	int m_nConfDlg1;
	int m_nConfDlg2;

	// presence status
	CString m_strPresence;

	// set to true while open preference dialog
	bool m_IsInConfiguration;
};

extern CPCAUADlg* g_pMainDlg;


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PCAUADLG_H__E0DFE2A1_80C2_45D8_B0AC_4BA15C9F3374__INCLUDED_)
