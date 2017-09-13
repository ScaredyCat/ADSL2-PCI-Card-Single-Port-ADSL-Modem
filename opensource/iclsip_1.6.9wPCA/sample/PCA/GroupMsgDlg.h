#if !defined(AFX_GROUPMSGDLG_H__96386442_E1CC_4146_B9DB_F2FFBC4DBB3D__INCLUDED_)
#define AFX_GROUPMSGDLG_H__96386442_E1CC_4146_B9DB_F2FFBC4DBB3D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GroupMsgDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CGroupMsgDlg dialog

class CGroupMsgDlg : public CDialog
{
// Construction
public:
	CGroupMsgDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CGroupMsgDlg)
	enum { IDD = IDD_GROUP_MESSAGE };
	CString	m_SelectedGroupName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGroupMsgDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CGroupMsgDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GROUPMSGDLG_H__96386442_E1CC_4146_B9DB_F2FFBC4DBB3D__INCLUDED_)
