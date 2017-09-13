#if !defined(AFX_DLGJOINMESSAGE_H__47132479_D23B_442A_8DC4_A16CBC3A3E0E__INCLUDED_)
#define AFX_DLGJOINMESSAGE_H__47132479_D23B_442A_8DC4_A16CBC3A3E0E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DlgJoinMessage.h : header file
//

#include "SortListCtrl.h"

/////////////////////////////////////////////////////////////////////////////
// CDlgJoinMessage dialog

class CDlgJoinMessage : public CDialog
{
// Construction
public:
	CDlgJoinMessage(CWnd* pParent = NULL);   // standard constructor

	CArray<CString,CString&> m_vecTarget;

// Dialog Data
	//{{AFX_DATA(CDlgJoinMessage)
	enum { IDD = IDD_JOIN_MESSAGE };
	CSortListCtrl	m_AddrBook;
	CString	m_strURI;
	//}}AFX_DATA

	CImageList m_PresenceImg;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgJoinMessage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgJoinMessage)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGJOINMESSAGE_H__47132479_D23B_442A_8DC4_A16CBC3A3E0E__INCLUDED_)
