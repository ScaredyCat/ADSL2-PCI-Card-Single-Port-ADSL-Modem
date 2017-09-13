#if !defined(AFX_USERPAGE_H__1B423FDD_D1D3_4B28_8E22_182D080C0EE1__INCLUDED_)
#define AFX_USERPAGE_H__1B423FDD_D1D3_4B28_8E22_182D080C0EE1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// UserPage.h : header file
//
#include "stdafx.h"
#include "UA\UAProfile.h"

/////////////////////////////////////////////////////////////////////////////
// CUserPage dialog

class CUserPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CUserPage)

// Construction
public:
	CUserPage();
	~CUserPage();

// Dialog Data
	//{{AFX_DATA(CUserPage)
	enum { IDD = IDD_PROPPAGE_USER };
	CEdit	m_STUNserverCTL;
	CComboBox	m_IPAddrListCTL;
	CEdit	m_UsernameCTL;
	CEdit	m_DisplayNameCTL;
	CEdit	m_PublicAddrCTL;
	CEdit	m_ContactAddrCTL;
	CIPAddressCtrl	m_LocalAddrCTL;
	CButton	m_UseUDPCTL;
	CButton	m_UseTCPCTL;
	CEdit	m_LocalPortCTL;
	//UINT	m_LocalPort;
	CString m_LocalPort;
	CString m_LocalAddr;
	CString	m_Username;
	CString	m_DisplayName;
	CString	m_ContactAddr;
	CString	m_PublicAddr;
	BOOL	m_bCustomize;
	CString	m_STUNserver;
	BOOL	m_bUseSTUN;
	CString	m_LocalAddr1;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CUserPage)
	public:
	virtual void OnOK();
	virtual BOOL OnApply();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CUserPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnUseUDP();
	afx_msg void OnUseTCP();
	afx_msg void OnFieldchangedLocalAddr(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnChangeLocalPort();
	afx_msg void OnCustom();
	afx_msg void OnChangeUsername();
	afx_msg void OnChangeDisplayname();
	afx_msg void OnChangePublicaddress();
	afx_msg void OnChangeContactaddress();
	afx_msg void OnSelChangeIPAddrList();
	afx_msg void OnUseSTUN();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void UpdateLocalAddr();
	void UpdateContactAddr();
	void UpdatePublicAddr();

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_USERPAGE_H__1B423FDD_D1D3_4B28_8E22_182D080C0EE1__INCLUDED_)
