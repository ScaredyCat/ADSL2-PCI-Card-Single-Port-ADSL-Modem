#if !defined(AFX_SEARCHDLG_H__D92FA649_F027_489C_BA5D_64C95F4CD4AA__INCLUDED_)
#define AFX_SEARCHDLG_H__D92FA649_F027_489C_BA5D_64C95F4CD4AA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SearchDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSearchDlg dialog

class CSearchDlg : public CDialog
{
// Construction
public:
	CSearchDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSearchDlg)
	enum { IDD = IDD_DIALOG_Search };
	CString	m_SearchDepart;
	CString	m_SearchDivision;
	CString	m_SearchID;
	CString	m_SearchName;
	CString	m_SearchTel;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSearchDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSearchDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SEARCHDLG_H__D92FA649_F027_489C_BA5D_64C95F4CD4AA__INCLUDED_)
