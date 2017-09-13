#if !defined(AFX_CALLLOGDLG_H__68009070_71DD_4B7B_BF83_CFCA487D52CE__INCLUDED_)
#define AFX_CALLLOGDLG_H__68009070_71DD_4B7B_BF83_CFCA487D52CE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CallLogDlg.h : header file
//
#include "CallLog.h"
#include "CallLogDelDlg.h"
#include "QueryDB.h"
#include "EditPersonalDlg.h"
#include "Personal.h"

/////////////////////////////////////////////////////////////////////////////
// CCallLogDlg dialog

class CCallLogDlg : public CDialog
{
// Construction
public:
	bool isRefresh;
	CCallLogDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCallLogDlg)
	enum { IDD = IDD_CallLog };
	CString	m_CallLog_End;
	CString	m_CallLog_Interval;
	CString	m_CallLog_Start;
	CString	m_CallLog_Tel;
	CString	m_CallLog_Type;
	CString	m_CallLog_URI;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCallLogDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCallLogDlg)
	afx_msg void OnBTNDel();
	afx_msg void OnBTNEdit();
	afx_msg void OnBtnDial();
	afx_msg void OnBTNAdd();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CALLLOGDLG_H__68009070_71DD_4B7B_BF83_CFCA487D52CE__INCLUDED_)
