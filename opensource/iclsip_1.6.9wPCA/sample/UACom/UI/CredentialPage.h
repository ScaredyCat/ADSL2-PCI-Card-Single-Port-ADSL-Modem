#if !defined(AFX_CREDENTIALPAGE_H__3C14C88E_8A8B_4264_9230_787C798F6CA2__INCLUDED_)
#define AFX_CREDENTIALPAGE_H__3C14C88E_8A8B_4264_9230_787C798F6CA2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CredentialPage.h : header file
//
#include "stdafx.h"
#include "UA\UAProfile.h"

/////////////////////////////////////////////////////////////////////////////
// CCredentialPage dialog

class CCredentialPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CCredentialPage)

// Construction
public:
	CCredentialPage();
	~CCredentialPage();
	
	void UpdateItems();
	CString strRealmName;
	CString strUserID;
	CString strPasswd;

// Dialog Data
	//{{AFX_DATA(CCredentialPage)
	enum { IDD = IDD_PROPPAGE_CREDENTIAL };
	CListCtrl	m_CredentialListCTL;
	//}}AFX_DATA
	CUAProfile* pProfile;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CCredentialPage)
	public:
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CCredentialPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnClickCredentiallist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKeydownCredentiallist(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CREDENTIALPAGE_H__3C14C88E_8A8B_4264_9230_787C798F6CA2__INCLUDED_)
