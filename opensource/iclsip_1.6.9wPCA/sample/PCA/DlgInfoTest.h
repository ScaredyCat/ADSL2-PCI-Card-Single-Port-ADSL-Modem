#if !defined(AFX_DLGINFOTEST_H__7E637E2B_D3E1_49AE_9F3E_B55936EEDDCF__INCLUDED_)
#define AFX_DLGINFOTEST_H__7E637E2B_D3E1_49AE_9F3E_B55936EEDDCF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DlgInfoTest.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgInfoTest dialog

class CDlgInfoTest : public CDialog
{
// Construction
public:
	CDlgInfoTest(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgInfoTest)
	enum { IDD = IDD_INFOTEST };
	CString	m_strTargetURI;
	CString	m_strInfoBody;
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgInfoTest)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgInfoTest)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGINFOTEST_H__7E637E2B_D3E1_49AE_9F3E_B55936EEDDCF__INCLUDED_)
