// CPanelDlg.h : header file
//

#if !defined(AFX_PANELDLG_H__A39C8482_4C80_46BB_A3E6_E38B8F0DD259__INCLUDED_)
#define AFX_PANELDLG_H__A39C8482_4C80_46BB_A3E6_E38B8F0DD259__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef SORTLISTCTRL_H
	#include "SortListCtrl.h"
#endif	// SORTLISTCTRL_H
#include "Picture.h"
#include "dbaccess.h"
#include "CallLogDlg.h"
#include "personal.h"

#include "xmlrpc.h"

typedef struct i_InfoDB
{
	int m_sn;
	int m_class;
	CString m_name;
	CString m_id;
	CString m_division;
	CString m_department;
	CString m_telephone;
	CString m_remark;
	CString m_pic;
	//add by alan 
	CString m_group;
}InfoDB;

#define PAGEID_ADDRESSBOOK	1
#define PAGEID_CALLLOG		2
#define PAGEID_VoiceMail	3

#define defPersonalSn			0
#define defPersonalClass		1

//add by alan
#ifdef _XCAPSERVER
#define defPersonalName			0
#define defPersonalGroup			1
#define defPersonalPresence		2
#define defPersonalCallStatus	3
#define defPersonalTelephone	4
#define defPersonalCompany		5
#define defPersonalRemark		6
#define defPersonalPic			7
#else
#define defPersonalName			0
#define defPersonalPresence		1
#define defPersonalCallStatus	2
#define defPersonalTelephone	3
#define defPersonalCompany		4
#define defPersonalRemark		5
#define defPersonalPic			6
#endif

#define Presence_Offline		0
#define Presence_Online			1
#define Presence_Unknown		2

//add by alan
void refreshPBook(CSortListCtrl	*m_ListPersonal);
/////////////////////////////////////////////////////////////////////////////
// CPanelDlg dialog

class CPanelDlg : public CSkinDialog
{
// Construction
public:
	xmlrpc_env m_env;
	int  OnAuthenticate();
	void OnBtnVoiceMailSetting();
	void StateItri();
	CString ImFileFormat(CString sSource);
	void RefreshPersonalList();
	void AddressImport();
	void AddressExport();
	void AddMsgItem(int index, LPCSTR sCaller, LPCSTR sDate, LPCSTR sTime, BOOL bStatus, LPCSTR sURL);
	void OnCancel();
	void OnOK();
	int m_curPage;
	BOOL CheckXMLRPCError();
	xmlrpc_env m_xmlrpcEnv;
	xmlrpc_value* m_xmlrpcResult;
	CString GetConnectionPort();
	CString GetConnectionURL();
	void SetConnectionPort(CString port);
	void SetConnectionURL(CString strURL);
	void ShowVoiceMail();
	CString GetAccountPasswd();
	void SetAccountPasswd(CString strPasswd);
	CString GetAccountId();
	void SetAccountId(CString strId);
	void SetInterval(int interval);
	int  GetInterval();
	static CString tranStr(CString str);
	void UpdateCallLog();
	void SetPage( int idPage );
	void ShowPhoneBook();
	void ShowCallLog();
	CWnd* m_pUIDlg;
	CString projectpath;
	
	//modify by alan
//	void AddDB(InfoDB* info);
	bool AddDB(InfoDB* info);
	
	CString m_ADDPERSONAL_PIC;
	CString m_ADDPERSONAL_REMARK;
	CString m_ADDPERSONAL_TELPHONE;
	CString m_ADDPERSONAL_COMPANY;
	CString m_ADDPERSONAL_NAME;
	int m_LVpersonal_class;
	int m_LVpersonal_sn;
	void ModifyDB(InfoDB* info);
//	void Init_LS_item(LS_item * lpLS_item,bool allow_subitems);
//	void InitLVITEM(int nItem,int nSubItem,LVITEM * pItem);
	CString m_LVpersonal_Remark;
	CString m_LVpersonal_Telephone;
	CString m_LVpersonal_Company;
	CString m_LVpersonal_Name;
	CString m_LVpersonal_PIC;
	CImageList m_SmallImg;
	CImageList m_PresenceImg;
	CImageList m_SmallImgVoiceMail;
	CPicture m_Picture;
	void StateSearch();
	void StatePersonal();
	void StateInitial();
	CPanelDlg(CWnd* pParent = NULL);	// standard constructor

	void DragWindow(int x, int y);

public:
	void SubscribePhoneBook();
	void unSubscribePhoneBook();
	void UpdateBuddyPresence(CString strNumber, int nPresence, CString strDes, CString strCallStatus);
	
	// added by shen, 20041225
	// modified by molisado, 20050406
	int GetBuddyPresence(CString strNumber);


// Dialog Data
	//{{AFX_DATA(CPanelDlg)
	enum { IDD = IDD_RIGHT_PANEL };
	CSortListCtrl	m_ListSearch;
	CSortListCtrl	m_ListPersonal;
	CSortListCtrl	m_ListItri;
	CSortListCtrl	m_ListCallLog;
	CSortListCtrl	m_ListVoiceMail;
	CString m_ITRI_EmployId;
	CString m_ITRI_DepartmentId;
	CString m_ITRI_CName;
	CString m_ITRI_Email;
	CString m_ITRI_Ext;
	CString	m_strITRILab;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPanelDlg)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPanelDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnBtnpersonal();
	afx_msg void OnBtnItri();
	afx_msg void OnBtnsearch();
	afx_msg void OnDblclkListitir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkListpersonal(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBtnCallLogProfile();
	afx_msg void OnBtnCallLogDetail();
	afx_msg void OnBtnVoiceMailReceive();
	afx_msg void OnBtnVoiceMailPlay();
	afx_msg void OnBtnVoiceMailDelete();
	afx_msg void OnBtnpersonalAdd();
	afx_msg void OnBtnpersonalDel();
	afx_msg void OnBtndoSearch();
	afx_msg void OnBtnVoiceMail();
	afx_msg void OnRadioa();
	afx_msg void OnRadiob();
	afx_msg void OnDblclkListsearch(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMoving(UINT fwSide, LPRECT pRect);
	afx_msg void OnBTNCallLog();
	afx_msg void OnBTNPhone();
	afx_msg void OnDblclkLISTCallLog(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRclickListCallLog(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRclickListPersonal(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnCallLogDial();
	afx_msg void OnCallLogPro();
	afx_msg void OnCallLogErase();
	afx_msg void OnAddressBkAdd();
	afx_msg void OnAddressBkDelete();
	afx_msg void OnAddressBkEdit();
	afx_msg void OnAddressBkDial();
	afx_msg void OnAddressBkIM();
	afx_msg void OnAddressBkIMTo();
	afx_msg void OnBTNPERSONALEdit();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnBtnItriSearch();
	afx_msg void OnMessageTo();
	afx_msg void OnBtnItriClear();
	afx_msg void OnSetPresenceStatusOffline();
	afx_msg void OnSetPresenceStatusOnline();
	afx_msg void OnNewGroup();
	afx_msg void OnAddressBkBlock();
	afx_msg void OnAddressBkUnBlock();
	afx_msg void OnDeleteGroup();
	afx_msg void OnEditGroup();
	afx_msg void OnAddressBkOfflineMsg();
	afx_msg void OnAddressBkGroupMsg();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void _OnBtnpersonalAdd(LPCTSTR szDefaultTelNum);

	void SqlRecordToArray(CString connection,CString operation,CArray<InfoPersonal,InfoPersonal> *cArray);
	int iVoiceMailError;
	CString connectionPort;
	CString connectionURL;
	CString accountPasswd;
	CString accountId;
	void ButtonPressed(CString m_ButtonName);
	void InitDlgControlPos();
	BOOL m_bItriSearch;

};

extern CPanelDlg* g_pdlgPCAExtend;

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PANELDLG_H__A39C8482_4C80_46BB_A3E6_E38B8F0DD259__INCLUDED_)
