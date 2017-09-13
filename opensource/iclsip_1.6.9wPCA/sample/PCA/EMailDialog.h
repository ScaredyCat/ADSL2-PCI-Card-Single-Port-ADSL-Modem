#if !defined(AFX_EMAILDIALOG_H__6A8D351B_EACD_43EF_A045_27817279A5FB__INCLUDED_)
#define AFX_EMAILDIALOG_H__6A8D351B_EACD_43EF_A045_27817279A5FB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EMailDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEMailDialog dialog

class CEMailDialog : public CDialog
{
// Construction
public:
	CEMailDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CEMailDialog)
	enum { IDD = IDD_SET_EMAIL_DIALOG };
	CString	m_emailAddr;
	CString	m_newPassword;
	int		m_notifyType;
	CString	m_interval;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEMailDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEMailDialog)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EMAILDIALOG_H__6A8D351B_EACD_43EF_A045_27817279A5FB__INCLUDED_)
