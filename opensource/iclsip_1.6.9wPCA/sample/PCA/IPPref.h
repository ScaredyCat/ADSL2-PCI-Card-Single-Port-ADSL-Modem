#if !defined(AFX_IPPREFDLG_H__CF1BD243_06D7_4843_B73B_8F4078850576__INCLUDED_)
#define AFX_IPPREFDLG_H__CF1BD243_06D7_4843_B73B_8F4078850576__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PrefDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPrefDlg dialog

class CIPPrefDlg : public CDialog
{
// Construction
public:
	CString m_strLocalIP;

	void InitLocalIPList();
	void OnOK();
	void OnCancel();
	CPrefDlg(CWnd* pParent = NULL);   // standard constructor

	
// Dialog Data
	//{{AFX_DATA(CPrefDlg)
	enum { IDD = IDD_PREFERENCE_IP };
	CComboBox	m_LocalIPList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPrefDlg)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPrefDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	HACCEL m_hAccel;

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IPPREFDLG_H__CF1BD243_06D7_4843_B73B_8F4078850576__INCLUDED_)
