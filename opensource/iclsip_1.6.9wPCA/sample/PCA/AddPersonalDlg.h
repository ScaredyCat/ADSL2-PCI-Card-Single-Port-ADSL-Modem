#if !defined(AFX_CAddPersonalDlg_H__C88B45A5_430F_4335_879F_8255BE78E15E__INCLUDED_)
#define AFX_CAddPersonalDlg_H__C88B45A5_430F_4335_879F_8255BE78E15E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Picture.h"

// CAddPersonalDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAddPersonalDlg dialog

class CAddPersonalDlg : public CDialog
{
// Construction
public:
	CAddPersonalDlg(CWnd* pParent = NULL);   // standard constructor
	CPicture m_Picture;
// Dialog Data
	//{{AFX_DATA(CAddPersonalDlg)
	enum { IDD = IDD_PERSONAL_ADD };
	CString	m_COMPANY;
	CString	m_NAME;
	CString	m_PIC;
	CString	m_REMARK;
	CString	m_TEL;
	CString	m_GROUP;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddPersonalDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddPersonalDlg)
	afx_msg void OnPersonalAddOpenfile();
	afx_msg void OnPersonalOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CAddPersonalDlg_H__C88B45A5_430F_4335_879F_8255BE78E15E__INCLUDED_)
