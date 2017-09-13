#if !defined(AFX_GROUPMANAGEDLG_H__117C4038_45F1_4C39_992D_6B6915EA3328__INCLUDED_)
#define AFX_GROUPMANAGEDLG_H__117C4038_45F1_4C39_992D_6B6915EA3328__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GroupManageDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CGroupManageDlg dialog

class CGroupManageDlg : public CDialog
{
// Construction
public:
	CGroupManageDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CGroupManageDlg)
//	enum { IDD = IDD_GROUP_MANAGEMENT };
	enum { IDD = IDD_GROUP_NEW };
	CString	m_newgroupname;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGroupManageDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CGroupManageDlg)
	afx_msg void OnNewGroup();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GROUPMANAGEDLG_H__117C4038_45F1_4C39_992D_6B6915EA3328__INCLUDED_)
