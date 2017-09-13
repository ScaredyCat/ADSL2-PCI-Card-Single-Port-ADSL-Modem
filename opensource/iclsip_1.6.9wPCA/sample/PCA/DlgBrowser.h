#if !defined(AFX_DLGBROWSER_H__569C9E14_2AEB_46AD_A0B7_23A02CD6D1B4__INCLUDED_)
#define AFX_DLGBROWSER_H__569C9E14_2AEB_46AD_A0B7_23A02CD6D1B4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DlgBrowser.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgBrowser dialog

class CDlgBrowser : public CDialog
{
// Construction
public:
	CDlgBrowser();
	void ShowLink( CString strTitle, CString strLink, CRect position);  

	// clear this pointer while dialog destory
	void SetHostPointer( CDlgBrowser** ppOwnerPointer);

// Dialog Data
	//{{AFX_DATA(CDlgBrowser)
	enum { IDD = IDD_BROWSER };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgBrowser)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
	CComPtr<IWebBrowser2> m_spBrowserControl;

protected:
	CWnd* m_pBrowser;
	CString m_strTitle;
	CString m_strLink;
	CRect m_position;

	bool m_bFirstShow;

	// Generated message map functions
	//{{AFX_MSG(CDlgBrowser)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnClose();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGBROWSER_H__569C9E14_2AEB_46AD_A0B7_23A02CD6D1B4__INCLUDED_)
