#if !defined(AFX_REPEATOPTDLG_H__F78D8C34_EEA7_4B9C_A033_946311ECD15F__INCLUDED_)
#define AFX_REPEATOPTDLG_H__F78D8C34_EEA7_4B9C_A033_946311ECD15F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RepeatOptDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRepeatOptDlg dialog

class CRepeatOptDlg : public CDialog
{
// Construction
public:
	CRepeatOptDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CRepeatOptDlg)
	enum { IDD = IDD_RepeatOptDlg };
	int		m_opt;
	CString	m_msg;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRepeatOptDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CRepeatOptDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REPEATOPTDLG_H__F78D8C34_EEA7_4B9C_A033_946311ECD15F__INCLUDED_)
