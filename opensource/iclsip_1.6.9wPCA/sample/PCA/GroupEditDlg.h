#if !defined(AFX_GROUPEDITDLG_H__F85E4813_7D91_4BC6_A743_A1BD17B4DA87__INCLUDED_)
#define AFX_GROUPEDITDLG_H__F85E4813_7D91_4BC6_A743_A1BD17B4DA87__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GroupEditDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// GroupEditDlg dialog

class GroupEditDlg : public CDialog
{
// Construction
public:
	GroupEditDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(GroupEditDlg)
	enum { IDD = IDD_GROUP_EDIT };
	CString	m_newGroupName;
	CString	m_selGroupName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(GroupEditDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(GroupEditDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GROUPEDITDLG_H__F85E4813_7D91_4BC6_A743_A1BD17B4DA87__INCLUDED_)
