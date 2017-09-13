#if !defined(AFX_PREFDLG_H__CF1BD243_06D7_4843_B73B_8F4078850576__INCLUDED_)
#define AFX_PREFDLG_H__CF1BD243_06D7_4843_B73B_8F4078850576__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PrefDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPrefDlg dialog

class CPrefDlg : public CDialog
{
// Construction
public:
	CString m_strLocalIP;
	BOOL m_bSafeMode;
	CString m_strCredential;
	int m_curConfigMode;
	CStringArray m_ConfigModes;

	void InitLocalIPList();
	void OnOK();
	void OnCancel();
	CPrefDlg(CWnd* pParent = NULL);   // standard constructor


// Dialog Data
	//{{AFX_DATA(CPrefDlg)
	enum { IDD = IDD_PREFERENCE };
	CComboBox	m_ConfigMode;
	CIPAddressCtrl	m_DSIPAddr;
	CComboBox	m_LocalIPList;
	BOOL	m_bAutoStartup;
	BOOL	m_bAutoCheck;
	int		m_DSPort;
	CString	m_UserName;
	BOOL	m_bUseDeployServer;
	CString	m_creRealm;
	CString	m_creAccount;
	CString	m_crePassword;
	CString	m_strDSIP;
	CString	m_strVMAccount;
	CString	m_strVMPassword;
	BOOL	m_bEnableVM;
	int	m_Quality;
	BOOL	m_bAllowCallWaiting;
	BOOL	m_bUseLMS;
	int	m_LMSPort;
	CString	m_strLMSIP;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPrefDlg)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPrefDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnBtnAdvanced();
	afx_msg void OnUsedeploy();
	afx_msg void OnUsevm();
	afx_msg void OnUselms();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	HACCEL m_hAccel;

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PREFDLG_H__CF1BD243_06D7_4843_B73B_8F4078850576__INCLUDED_)
