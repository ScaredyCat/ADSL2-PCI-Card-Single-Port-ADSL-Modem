#if !defined(AFX_PCASPLASHDLG_H__3248BA49_D340_4158_B6FD_FB79D173A5E7__INCLUDED_)
#define AFX_PCASPLASHDLG_H__3248BA49_D340_4158_B6FD_FB79D173A5E7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PCASplashDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPCASplashDlg dialog

class CPCASplashDlg : public CDialog
{
// Construction
public:
	void SetSplashText( LPCTSTR txt, LPCTSTR app = NULL);
	CPCASplashDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CPCASplashDlg)
	enum { IDD = IDD_SPLASH };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPCASplashDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPCASplashDlg)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PCASPLASHDLG_H__3248BA49_D340_4158_B6FD_FB79D173A5E7__INCLUDED_)
