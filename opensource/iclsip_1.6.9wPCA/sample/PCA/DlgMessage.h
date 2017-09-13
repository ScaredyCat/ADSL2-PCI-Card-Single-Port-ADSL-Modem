#if !defined(AFX_DLGMESSAGE_H__170D26C3_5147_48E5_9B19_79A0DD15EF83__INCLUDED_)
#define AFX_DLGMESSAGE_H__170D26C3_5147_48E5_9B19_79A0DD15EF83__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DlgMessage.h : header file
//

#include <vector>	// use STL's vector
#include <list>		// use STL's list

#import <msxml.dll> named_guids
//using namespace MSXML;

// added by molisado, 20050309
#define DEFAULT_ACTIVE_INTERVAL 120000
#define DEFAULT_IDLE_INTERVAL 15000
#define ID_TIMER_IDLE 2515		// sender entity idle timer
#define ID_TIMER_ACTIVE 2510	// sender entity active refresh timer
#define ID_TIMER_REFRESH 1015	// receiver entity active refresh timer

// added by molisado, 20050406
#define OneToOne_MESSAGE 1
#define CONFERENCE_MESSAGE 2
#define isComposing_COMMAND 3


/////////////////////////////////////////////////////////////////////////////
// CDlgMessage dialog

class CDlgMessage : public CDialog
{
// Construction
public:
	void SetOffline(bool bOffline);
	bool IsPartyOf(CString strName);
	bool m_bOffline;
	CDlgMessage( CWnd* pParentWnd, LPCSTR szURI, LPCSTR szDisplayName); 
	~CDlgMessage();

	bool AddParty( LPCSTR szURI, LPCSTR szDisplayName);
	bool RemoveParty( LPCSTR szURI);

	// modified by molisado, 20050406
	void SendInstMsg( LPCSTR szMessage, int option);
	void ReceiveInstMsg( LPCSTR szRemoteURI, LPCSTR szMessage);

	bool CreateIMConference(CArray<CString,CString&>* vecTarget, bool bNew=false);

	static CString GetNameFromURI(CString szURI);

// Dialog Data
	//{{AFX_DATA(CDlgMessage)
	enum { IDD = IDD_MESSAGE };
	CButton	m_Send;
	CRichEditCtrl	m_Message;
	CRichEditCtrl	m_Dialog;
	CStatic	m_Label;
	CStatic	m_staticHint;
	//}}AFX_DATA
	CMenu m_Menu;

	static CDlgMessage* GetDlgByMessage( LPCSTR szRemoteURI, LPCSTR szMessage, LPCSTR szDisplayName = NULL);
	// added by shen, 20041225
	static int GetCount();
	static void OfflineFromAll(bool bClose=false);
	static void UpdatePartyFromAll(LPCSTR szRemoteURI, bool bOnline);
	void KickParty(CString szRemoteURI);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgMessage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	virtual void OnCancel();
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	class CPartyInfo
	{
	public:
		// modified by molisado, 20050309
		CPartyInfo( LPCSTR szURI, LPCSTR szName )
					: m_strName(szName), 
					  m_strURI(szURI), 
					  m_strRemoteState("idle"), 
					  m_strRemoteContentType("text"), 
					  m_RemoteRefresh(DEFAULT_ACTIVE_INTERVAL)
		{
			memset( &m_cfText, 0, sizeof(m_cfText));
		}
		CString m_strName;
		CString m_strURI;
		CHARFORMAT m_cfText;

		// added by molisado, 20050309
		CString m_strRemoteState;
		CString m_strRemoteContentType;
		int m_RemoteRefresh;
	};
	
	std::vector<CPartyInfo> m_vecPartyInfo;

	CString _PartyInfo2Text( bool bShowURI);
	void _UpdatePartyInfo();
	void _UpdateHint(LPCSTR szHint, bool bOffline=false);
	CPartyInfo* _GetPartyInfo( LPCSTR szRemoteURI);

	void _GetWindowPos( CWnd& wnd, CRect& rt);
	void _SetWindowPos( CWnd& wnd, CRect& rt);

	void _DisplayMessage( LPCSTR szRemoteURI, LPCSTR szMessage, bool bCommand=false);
	
	CHARFORMAT m_cfNames;		// name label char format
	CHARFORMAT m_cfLocalText;	// local text char format
	CHARFORMAT m_cfDefaultText;	// default text char format

	void _XML2Font( MSXML::IXMLDOMNodePtr spFont, CHARFORMAT& cf);
	CString _Font2XML( CHARFORMAT& cf);

	CString m_strLocalName;		// my display text
	CString m_strLocalURI;		// my contact URI
	CString m_strConferenceID;	// current conference id if exist

	// added by molisado, 20050309
	// isComposing-related data members
	CString m_strLocalState;		// my status
	CString m_strLocalContentType;	// my message contenttype
	CTime m_RemoteLastActive;	// the up-to-date remote last active of all party

	static CString _RemoveURIParam( LPCSTR szURI);

// class memebers
protected:
	CString _GetActiveParty(int &nNumOfActiveParty);
	void _CheckRemainder();
	void _LeaveConference();
	static void _RegisterDlg( CDlgMessage* dlg);
	static void _UnregisterDlg( CDlgMessage* dlg);

	static std::list<CDlgMessage*> m_lstMessageDlg;

	// Generated message map functions
	//{{AFX_MSG(CDlgMessage)
	afx_msg void OnSend();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnInitDialog();
	afx_msg void OnSetfont();
	afx_msg void OnJoinConference();
	afx_msg void OnUpdateMessage();
	afx_msg void OnClose();
	afx_msg void OnUpdateJoinConference(CCmdUI* pCmdUI);
	afx_msg void OnChangeMessage();
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	bool m_bNewConf;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGMESSAGE_H__170D26C3_5147_48E5_9B19_79A0DD15EF83__INCLUDED_)
