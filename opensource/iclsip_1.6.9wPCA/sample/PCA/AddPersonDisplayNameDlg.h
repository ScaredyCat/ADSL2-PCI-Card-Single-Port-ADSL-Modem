#if !defined(AFX_ADDPERSONDISPLAYNAMEDLG_H__FC2A0EB1_0E1C_4E34_A281_E8DB4FE5287F__INCLUDED_)
#define AFX_ADDPERSONDISPLAYNAMEDLG_H__FC2A0EB1_0E1C_4E34_A281_E8DB4FE5287F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AddPersonDisplayNameDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAddPersonDisplayNameDlg dialog

class CAddPersonDisplayNameDlg : public CDialog
{
// Construction
public:
	CAddPersonDisplayNameDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAddPersonDisplayNameDlg)
	enum { IDD = IDD_PERSON_ADD_USERNAME };
	CString	m_Tel;
	CString	m_DisplayName;
	//}}AFX_DATA
	bool isEditBoxReadable;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddPersonDisplayNameDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddPersonDisplayNameDlg)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDPERSONDISPLAYNAMEDLG_H__FC2A0EB1_0E1C_4E34_A281_E8DB4FE5287F__INCLUDED_)
