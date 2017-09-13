#if !defined(AFX_GROUPDELETEDLG_H__7E9CF7D3_FFF5_4925_B861_AEC1F4087503__INCLUDED_)
#define AFX_GROUPDELETEDLG_H__7E9CF7D3_FFF5_4925_B861_AEC1F4087503__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GroupDeleteDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// GroupDeleteDlg dialog

class GroupDeleteDlg : public CDialog
{
// Construction
public:
	GroupDeleteDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(GroupDeleteDlg)
	enum { IDD = IDD_GROUP_DELETE };
	CString	m_DeleteName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(GroupDeleteDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(GroupDeleteDlg)
	afx_msg void OnDelete();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GROUPDELETEDLG_H__7E9CF7D3_FFF5_4925_B861_AEC1F4087503__INCLUDED_)
