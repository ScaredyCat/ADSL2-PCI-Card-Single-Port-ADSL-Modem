#if !defined(AFX_ENTERURIDLG_H__BC7215DE_BC6E_4A3E_BC2A_C5D3BB61302A__INCLUDED_)
#define AFX_ENTERURIDLG_H__BC7215DE_BC6E_4A3E_BC2A_C5D3BB61302A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EnterURIDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEnterURIDlg dialog

class CEnterURIDlg : public CDialog
{
// Construction
public:
	CEnterURIDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CEnterURIDlg)
	enum { IDD = IDD_ENTERURI };
	CString	m_strURI;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEnterURIDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEnterURIDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ENTERURIDLG_H__BC7215DE_BC6E_4A3E_BC2A_C5D3BB61302A__INCLUDED_)
