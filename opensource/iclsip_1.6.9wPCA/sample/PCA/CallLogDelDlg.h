#if !defined(AFX_CALLLOGDELDLG_H__D8EFEFAB_3550_4FE8_AFEA_F18E5B5CF46E__INCLUDED_)
#define AFX_CALLLOGDELDLG_H__D8EFEFAB_3550_4FE8_AFEA_F18E5B5CF46E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CallLogDelDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCallLogDelDlg dialog

class CCallLogDelDlg : public CDialog
{
// Construction
public:
	//int SelectResult;
	CCallLogDelDlg(CWnd* pParent = NULL);   // standard constructor
	int* pintSelect; //0:COMBO_CallLogDel 1:DATETIMEPICKER_CallLogDel 2:Delete
	COleDateTime* pdateTime;
// Dialog Data
	//{{AFX_DATA(CCallLogDelDlg)
	enum { IDD = IDD_CallLogDel };
	CComboBox	m_COMBO_CallLogDel;
	int		m_RADIO_CallLogDel;
	COleDateTime	m_DATETIMEPICKER_CallLogDel;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCallLogDelDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCallLogDelDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnRadio1();
	afx_msg void OnRadio2();
	virtual void OnOK();
	afx_msg void OnRadio3();
	afx_msg void OnRadio4();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CALLLOGDELDLG_H__D8EFEFAB_3550_4FE8_AFEA_F18E5B5CF46E__INCLUDED_)
