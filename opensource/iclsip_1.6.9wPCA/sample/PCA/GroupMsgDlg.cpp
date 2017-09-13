// GroupMsgDlg.cpp : implementation file
//

#include "stdafx.h"
#include "pcaua.h"
#include "GroupMsgDlg.h"

#ifdef _XCAPSERVER
#include "xcap_datatype.h"
#include "xcapapi.h"
#include "UACDlg.h"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGroupMsgDlg dialog


CGroupMsgDlg::CGroupMsgDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGroupMsgDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGroupMsgDlg)
	m_SelectedGroupName = _T("");
	//}}AFX_DATA_INIT
}


void CGroupMsgDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGroupMsgDlg)
	DDX_CBString(pDX, IDC_GroupCombo, m_SelectedGroupName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGroupMsgDlg, CDialog)
	//{{AFX_MSG_MAP(CGroupMsgDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGroupMsgDlg message handlers

BOOL CGroupMsgDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	

#ifdef _XCAPSERVER

	CString UserName = CUACDlg::GetUAComRegString("User_Info","Username");
	CComboBox*      group_select = (CComboBox *)GetDlgItem(IDC_GroupCombo);
	GroupList *myGroupList = GetAllGroup(),*tmpGList;
	int gCount=0;
	if(myGroupList!=NULL)
	{
		tmpGList = myGroupList;
		while(tmpGList!=NULL)
		{ 

//			if(_stricmp(tmpGList->groupname,DEFAULT_GROUPNAME))
//			{
				group_select->SetWindowText(tmpGList->groupname);
				group_select->AddString(tmpGList->groupname);
//			}
			tmpGList = tmpGList->next;
			gCount++;
		}
		if(gCount<1)//only Group...
		{
			AfxMessageBox("No Group exist!!Create Group first.");
			FreeGroupList(myGroupList);
			myGroupList = NULL;
			CDialog::OnCancel();
		}
		FreeGroupList(myGroupList);
	}
	else
	{
		AfxMessageBox("No Group exist!!Create Group first.");
		CDialog::OnCancel();
		
	}
#endif
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CGroupMsgDlg::OnOK() 
{
	// TODO: Add extra validation here
	UpdateData(true);
	if(m_SelectedGroupName.GetLength()==0)
		AfxMessageBox("Deleted Group Name shoud not be empty!!");
	else
		CDialog::OnOK();
	CDialog::OnOK();
}
