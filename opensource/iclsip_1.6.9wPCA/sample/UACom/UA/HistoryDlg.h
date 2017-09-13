
#if !defined(AFX_HISTORYDLG_H__75B0722D_DB58_45BC_83FB_2E56F8C8BDC8__INCLUDED_)
#define AFX_HISTORYDLG_H__75B0722D_DB58_45BC_83FB_2E56F8C8BDC8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "HistoryEdit.h"


/////////////////////////////////////////////////////////////////////////////
// CHistoryDlg dialog

class CHistoryDlg : public CDialog
{
// Construction
public:
	void AppendString(CString str);

	CHistoryDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CHistoryDlg)
	enum { IDD = IDD_HISTORY_DLG };
	CHistoryEdit	m_Log;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHistoryDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	
	HICON m_hIcon;
	
	// Generated message map functions
	//{{AFX_MSG(CHistoryDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg void OnClearLog();
	afx_msg void OnOK();
	afx_msg void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // !defined(AFX_HISTORYDLG_H__75B0722D_DB58_45BC_83FB_2E56F8C8BDC8__INCLUDED_)

/*
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HISTORYDLG_H__75B0722D_DB58_45BC_83FB_2E56F8C8BDC8__INCLUDED_)
*/