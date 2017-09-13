#if !defined(AFX_SERVERPAGE_H__628BE118_33DF_4921_97C8_F84B4D720D9F__INCLUDED_)
#define AFX_SERVERPAGE_H__628BE118_33DF_4921_97C8_F84B4D720D9F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ServerPage.h : header file
//
#include "stdafx.h"
#include "UA\UAProfile.h"

/////////////////////////////////////////////////////////////////////////////
// CServerPage dialog

class CServerPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CServerPage)

// Construction
public:
	CServerPage();
	~CServerPage();

	BOOL m_bDirty;

// Dialog Data
	//{{AFX_DATA(CServerPage)
	enum { IDD = IDD_PROPPAGE_SERVER };
	CButton	m_SameBTN2;
	CButton	m_ExplicitPortCTL2;
	CButton	m_UseRegistrar2CTL;
	CEdit	m_Registrar2PortCTL;
	CEdit	m_Registrar2AddrCTL;
	CButton	m_SameBTN;
	CButton	m_UseSIMPLEServerCTL;
	CButton	m_UseRegistrarCTL;
	CButton	m_UseOutboundCTL;
	//CButton m_UseHttpTunnelCTL;
	CButton	m_UseCallServerCTL;
	CEdit	m_SIMPLEServerPortCTL;
	CEdit	m_SIMPLEServerAddrCTL;
	CEdit	m_RegistrarPortCTL;
	CEdit	m_RegistrarAddrCTL;
	CEdit	m_OutboundPortCTL;
	CEdit	m_OutboundAddrCTL;
	//CEdit	m_HttpTunnelPortCTL;
	//CEdit	m_HttpTunnelAddrCTL;
	CButton	m_ExplicitPortCTL;
	CEdit	m_CallServerPortCTL;
	CEdit	m_CallServerAddrCTL;
	BOOL	m_bExplicitPort;
	CString	m_szCallServerAddr;
	UINT	m_uiCallServerPort;
	CString	m_szOutboundAddr;
	UINT	m_uiOutboundPort;
	//CString m_szHttpTunnelAddr;
	//UINT	m_uiHttpTunnelPort;
	CString	m_szRegistrarAddr;
	UINT	m_uiRegistrarPort;
	CString	m_szSIMPLEServerAddr;
	UINT	m_uiSIMPLEServerPort;
	BOOL	m_bUseCallServer;
	BOOL	m_bUseOutbound;
	//BOOL	m_bUseHttpTunnel;
	BOOL	m_bUseRegistrar;
	BOOL	m_bUseSIMPLEServer;
	long	m_lExpireTime;
	CString	m_szRegistrar2Addr;
	UINT	m_uiRegistrar2Port;
	BOOL	m_bUseRegistrar2;
	long	m_lExpireTime2;
	BOOL	m_bExplicitPort2;
	//}}AFX_DATA
	
// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CServerPage)
	public:
	virtual void OnOK();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CServerPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnExplicitPort();
	afx_msg void OnUsecallserver();
	afx_msg void OnUseregistrar();
	afx_msg void OnUsesimpleserver();
	afx_msg void OnUseoutbound();
	//afx_msg void OnUseHttpTunnel();
	afx_msg void OnChangeCallserveraddr();
	afx_msg void OnChangeCallserverport();
	afx_msg void OnChangeRegistraraddr();
	afx_msg void OnChangeExpiretime();
	afx_msg void OnChangeSimpleserveraddr();
	afx_msg void OnChangeSimpleserverport();
	afx_msg void OnChangeOutboundaddr();
	afx_msg void OnChangeOutboundport();
	//afx_msg void OnChangeHttpTunneladdr();
	//afx_msg void OnChangeHttpTunnelport();
	afx_msg void OnButton1();
	afx_msg void OnChangeRegistrarport();
	afx_msg void OnButton2();
	afx_msg void OnChangeRegistraraddr2();
	afx_msg void OnChangeRegistrarport2();
	afx_msg void OnChangeExpiretime2();
	afx_msg void OnExplicitport2();
	afx_msg void OnUseregistrar2();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SERVERPAGE_H__628BE118_33DF_4921_97C8_F84B4D720D9F__INCLUDED_)
