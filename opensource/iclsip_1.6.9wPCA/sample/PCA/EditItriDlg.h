#if !defined(AFX_EDITITIR_H__4E28792A_53A1_4A43_893D_0F416A315C93__INCLUDED_)
#define AFX_EDITITIR_H__4E28792A_53A1_4A43_893D_0F416A315C93__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EditItriDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEditItriDlg dialog

class CEditItriDlg : public CDialog
{
// Construction
public:
	CEditItriDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CEditItriDlg)
	enum { IDD = IDD_ITIR_EDIT };
	CString	m_ITIR2_DEPID;
	CString	m_ITIR2_EXT;
	CString	m_ITIR2_ID;
	CString	m_ITIR2_NAME;
	CString	m_ITIR2_REMARK;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditItriDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEditItriDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EDITITIR_H__4E28792A_53A1_4A43_893D_0F416A315C93__INCLUDED_)
