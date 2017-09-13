#if !defined(AFX_SIMPLEPAGE_H__E4C27B69_AE25_4D4A_94D6_A0C71D5D3A7D__INCLUDED_)
#define AFX_SIMPLEPAGE_H__E4C27B69_AE25_4D4A_94D6_A0C71D5D3A7D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SimplePage.h : header file
//
#include "UA\UAProfile.h"

/////////////////////////////////////////////////////////////////////////////
// CSimplePage dialog

class CSimplePage : public CPropertyPage
{
	DECLARE_DYNCREATE(CSimplePage)

// Construction
public:
	CSimplePage();

// Dialog Data
	//{{AFX_DATA(CSimplePage)
	enum { IDD = IDD_PROPPAGE_SIMPLE };
	CString	m_strSimpleHost;
	int		m_nSimplePort;
	BOOL	m_bUseSimple;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSimplePage)
	public:
	virtual void OnOK();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSimplePage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SIMPLEPAGE_H__E4C27B69_AE25_4D4A_94D6_A0C71D5D3A7D__INCLUDED_)
