#if !defined(AFX_CONFIRMDELETEDLG_H__411C1C4F_D553_4E64_9A5F_A613BBD0B2A3__INCLUDED_)
#define AFX_CONFIRMDELETEDLG_H__411C1C4F_D553_4E64_9A5F_A613BBD0B2A3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ConfirmDeleteDlg.h : header file
//
#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////
// CConfirmDeleteDlg dialog

class CConfirmDeleteDlg : public CDialog
{
// Construction
public:
	CConfirmDeleteDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CConfirmDeleteDlg)
	enum { IDD = IDD_CONFIRMDELETEDLG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfirmDeleteDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CConfirmDeleteDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONFIRMDELETEDLG_H__411C1C4F_D553_4E64_9A5F_A613BBD0B2A3__INCLUDED_)
