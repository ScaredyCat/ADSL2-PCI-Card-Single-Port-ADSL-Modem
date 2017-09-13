#if !defined(AFX_SIPEXTPAGE_H__3BEDC1C7_EDB1_42D7_8974_37CABBBB27D3__INCLUDED_)
#define AFX_SIPEXTPAGE_H__3BEDC1C7_EDB1_42D7_8974_37CABBBB27D3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SIPExtPage.h : header file
//
#include "stdafx.h"
#include "UA\UAProfile.h"

/////////////////////////////////////////////////////////////////////////////
// SIPExtPage dialog

class SIPExtPage : public CPropertyPage
{
	DECLARE_DYNCREATE(SIPExtPage)

// Construction
public:
	SIPExtPage();
	~SIPExtPage();

// Dialog Data
	//{{AFX_DATA(SIPExtPage)
	enum { IDD = IDD_PROPPAGE_EXT };
	CEdit	m_SessionTimerCTL;
	CEdit	m_PreferredIDCtl;
	BOOL	m_bAnonFrom;
	CString	m_strPreferredID;
	BOOL	m_bUsePreferredID;
	BOOL	m_bUseSessionTimer;
	UINT	m_uSessionTimer;
	CString	m_strVersion;
	CString	m_strDate;

	CButton m_UseHttpTunnelCTL;
	CEdit	m_HttpTunnelPortCTL;
	CEdit	m_HttpTunnelAddrCTL;
	CString m_szHttpTunnelAddr;
	UINT	m_uiHttpTunnelPort;
	BOOL	m_bUseHttpTunnel;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(SIPExtPage)
	public:
	virtual void OnOK();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(SIPExtPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelAnonFrom();
	afx_msg void OnChangePreferredIdentity();
	afx_msg void OnUsePreferredID();
	afx_msg void OnUseSessionTimer();

	afx_msg void OnUseHttpTunnel();
	afx_msg void OnChangeHttpTunneladdr();
	afx_msg void OnChangeHttpTunnelport();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SIPEXTPAGE_H__3BEDC1C7_EDB1_42D7_8974_37CABBBB27D3__INCLUDED_)
