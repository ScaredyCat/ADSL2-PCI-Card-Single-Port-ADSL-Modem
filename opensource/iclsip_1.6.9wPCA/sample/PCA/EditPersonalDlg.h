#if !defined(AFX_EDITPERSONAL_H__B8D0EAC9_2CB1_4407_B93E_5EB28D89A8D6__INCLUDED_)
#define AFX_EDITPERSONAL_H__B8D0EAC9_2CB1_4407_B93E_5EB28D89A8D6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "Picture.h"
// EDITPERSONAL.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEditPersonalDlg dialog

class CEditPersonalDlg : public CDialog
{
// Construction
public:
	void showpic(CString pic);
	CString projectpath;
	CEditPersonalDlg(CWnd* pParent = NULL);   // standard constructor
	CPicture m_Picture;
	CString m_DialNum;
// Dialog Data
	//{{AFX_DATA(CEditPersonalDlg)
	enum { IDD = IDD_PERSONAL_EDIT };
	CString	m_PERSON2_COMPANY;
	CString	m_PERSON2_NAME;
	CString	m_PERSON2_TELEPHONE;
	CString	m_PERSON2_REMARK;
	CString	m_PERSON2_PIC;
	CString	m_PERSON2_GROUP;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditPersonalDlg)
	public:
	virtual int DoModal();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEditPersonalDlg)
	afx_msg void OnPerson2Openfile();
	afx_msg void OnPaint();
	virtual BOOL OnInitDialog();
	afx_msg void OnBtnDial();
	afx_msg void OnUpdate();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EDITPERSONAL_H__B8D0EAC9_2CB1_4407_B93E_5EB28D89A8D6__INCLUDED_)
