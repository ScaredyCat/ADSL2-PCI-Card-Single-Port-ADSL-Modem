#if !defined(AFX_UACDLG_H__9B79EB06_A8C7_43B2_8E40_C448E3E8A565__INCLUDED_)
#define AFX_UACDLG_H__9B79EB06_A8C7_43B2_8E40_C448E3E8A565__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


// UACDlg.h : header file
//

#include "uacom.h"
#include "DlgRemoteVideo.h"
//#include "PCAUADlg.h"

/////////////////////////////////////////////////////////////////////////////
// CUACDlg dialog
#define REG_RESULT_INPROGRESS	0
#define REG_RESULT_DONE			1
#define REG_RESULT_FAIL			2
#define REG_RESULT_TIMEOUT		3
#define REG_RESULT_NONEED		4

class CUACDlg : public CDialog
{
// Construction
public:
	CUACDlg(CWnd* pParent = NULL);   // standard constructor
	CString		m_strRegistarAddress;
	CString		m_strRegistarName;
	CString		m_strCallServerAddr;
	DWORD		m_dCallServerPort;

	IUAControl	UAControlDriver;
	CDialog*	m_pUIDlg;
	CDialog*	m_pSplashDlg;
	BOOL		m_uacInitOk;
	BOOL		InitUAComObj();

	void AdjustVideoSize( int type = -1, int scale = -1);
	void SnapMove( int x, int y );
	void RestartUACOM();
	static void SetUAComRegValue( LPCTSTR subkey, LPCTSTR name, DWORD dwValue );
	static void SetUAComRegValue( LPCTSTR subkey, LPCTSTR name, LPCTSTR pstrValue );
	static DWORD GetUAComRegDW( LPCTSTR subkey, LPCTSTR name, DWORD dwDefault );
	static CString GetUAComRegString( LPCTSTR subkey, LPCTSTR name);
	int m_regResult;


	BOOL m_bDragging;
	CPoint	m_ptDrag;

	BOOL m_bSnap;
	CPoint m_ptSnap;

	CBitmap m_bmpTitleBar;

	int m_nVideoType;		// 2 for QCIF, 3 for CIF
	int m_nVideoScale;		// 1 for 100%, 2 for 200%

	void UAComRegister();

	void OnMouseMove( int x, int y)
	{
		m_bDragging = true;
		m_ptDrag = CPoint(x,y);
		OnMouseMove( 0, CPoint(x,y));
		m_bDragging = false;
	}

	/*
	 * Debug Information
	 */
	CString m_strUseDebugMsg;
	BOOL m_bUseDebugMsg;

// Dialog Data
	//{{AFX_DATA(CUACDlg)
	enum { IDD = IDD_UACOM };
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUACDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void OnCancel();
	void OnOK();

	// Generated message map functions
	//{{AFX_MSG(CUACDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnShowTextUacontrol1(LPCTSTR strText);
	afx_msg void OnRingbackUacontrol1(long dlgHandle);
	afx_msg void OnAlertingUacontrol1(long dlgHandle);
	afx_msg void OnProceedingUacontrol1(long dlgHandle);
	afx_msg void OnConnectedUacontrol1(long dlgHandle);
	afx_msg void OnTimeOutUacontrol1(long dlgHandle);
	afx_msg void OnDialingUacontrol1(long dlgHandle);
	afx_msg void OnBusyUacontrol1(long dlgHandle);
	afx_msg void OnRejectUacontrol1(long dlgHandle);
	afx_msg void OnTransferredUacontrol1(long dlgHandle, LPCTSTR xferURL);
	afx_msg void OnHeldUacontrol1(long dlgHandle);
	afx_msg void OnRegistrationDoneUacontrol1();
	afx_msg void OnRegistrationFailUacontrol1();
	afx_msg void OnNeedAuthenticationUacontrol1(LPCTSTR strRealmName);
	afx_msg void OnWaitingUacontrol1(long dlgHandle);
	afx_msg void OnRegistrationTimeoutUacontrol1();
	afx_msg void OnMoving(UINT fwSide, LPRECT pRect);
	afx_msg void OnDestroy();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnDisconnectedUacontrol1(long dlgHandle, long StatusCode);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnReceivedTextUacontrol1(LPCTSTR remoteURI, LPCTSTR text);
	afx_msg void OnInfoResponseUacontrol1(LPCTSTR remoteURI, long StatusCode, LPCTSTR bstrContentType, LPCTSTR body);
	afx_msg void OnInfoRequestUacontrol1(LPCTSTR remoteURI, LPCTSTR bstrContentType, LPCTSTR body, long* pRspStatusCode, BSTR* pRspContentType, BSTR* pRspBody);
	afx_msg void OnBuddyUpdateStatusUaControl(LPCTSTR buddyURI, BOOL Open, LPCTSTR strMsgState, LPCTSTR strCallState);
	afx_msg void OnRemoveVideoSizeChangedUaControl(int nWidth, int nHeight);
	afx_msg void OnBuddyUpdateBasicStatusUacontrol1(LPCTSTR buddyURI, long bOpen, LPCTSTR pStatus, LPCTSTR cStatus);
	afx_msg void OnNeedAuthorizeUacontrol1(LPCTSTR strURI);
	afx_msg void OnReceivedOfflineMsgUacontrol1(LPCTSTR rcv_msg, LPCTSTR rcv_date, LPCTSTR rcv_from);
	afx_msg void OnRegisterExpiredUacontrol1();
	DECLARE_EVENTSINK_MAP()
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	BOOL CheckSockPort( LPCTSTR ipAddr);
	CWnd* m_pwndUACtrl;
	BOOL m_bIPPortCheckOK;

	CDlgRemoteVideo m_dlgRemoteVideo;
};

extern CUACDlg* g_pdlgUAControl;


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_UACDLG_H__9B79EB06_A8C7_43B2_8E40_C448E3E8A565__INCLUDED_)
