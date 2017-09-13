#if !defined(AFX_CREDENTIALEDITDLG_H__DFC31D47_5DFC_4662_940E_BAF96473ED01__INCLUDED_)
#define AFX_CREDENTIALEDITDLG_H__DFC31D47_5DFC_4662_940E_BAF96473ED01__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CredentialEditDlg.h : header file
//
#include "stdafx.h"
#include "PreferSheet.h"

/////////////////////////////////////////////////////////////////////////////
// CCredentialEditDlg dialog

class CCredentialEditDlg : public CDialog
{
// Construction
public:
	CCredentialEditDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCredentialEditDlg)
	enum { IDD = IDD_CREDENTIALEDITDLG };
	CEdit	m_RealmNameCTL;
	CString	m_RealmName;
	CString	m_UserID;
	CString	m_Passwd;
	CString	m_PasswdVarify;
	//}}AFX_DATA
	
	CCredentialPage* pCredentialPage;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCredentialEditDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCredentialEditDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CREDENTIALEDITDLG_H__DFC31D47_5DFC_4662_940E_BAF96473ED01__INCLUDED_)
