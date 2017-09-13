// ProgressListDlg.h : header file
//

#pragma once
#include "afxcmn.h"
#include "SignalListCtrlEx.h"

class CNetworkAdapter;

// CProgressListDlg dialog
class CProgressListDlg : public CDialog
{
// Construction
public:
	CProgressListDlg(CWnd* pParent = NULL);	// standard constructor
	void InsertBS(CString mac,CString ssid,int ssi,int idx);
	void UpdateBS(CString mac,CString ssid,int ssi,int idx);
	void ClearRefresh();
	void CheckRefresh();
	void CheckOK();
	BOOL GetCheckOK() { return m_bCheck; };
	
	//! network interface 
	int	m_nCount;
	CNetworkAdapter*	m_pAdapters;
	CComboBox	m_AdapterSelCTL;
	CString sType,
			sIP,
			sDesc,
			sSub,
			sGate,
			sDns1,
			sDns2;
	CString m_strDefaultNIC;

	BOOL AdapterInit();
	void AdapterClean();
	void SetDefaultNIC(CString nic);
	CString GetDefaultNIC();

	//! End of network interface

	CString m_SSID;
	BOOL m_bCheck;
	BOOL m_bAssociateOK;
	CString	m_Hint,m_HintAssociate;
	CString m_SelectItem;
// Dialog Data
	enum { IDD = IDD_SIGNALLIST_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnClickSignallist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKeydownSignallist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchangeAdapterselcombo();
	afx_msg void OnBtnSetDefault();
	afx_msg void OnBtnChangeDefault();
	afx_msg void OnShowWindow(BOOL bShow,UINT nStatus);
	DECLARE_MESSAGE_MAP()
public:
	
	//CListCtrl m_ProgressList;
	CListCtrlEx m_ProgressList;
	afx_msg void OnTimer(UINT nIDEvent);
};
