// GroupDeleteDlg.cpp : implementation file
//

#include "stdafx.h"
#include "pcaua.h"
#include "GroupDeleteDlg.h"

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
// GroupDeleteDlg dialog


GroupDeleteDlg::GroupDeleteDlg(CWnd* pParent /*=NULL*/)
	: CDialog(GroupDeleteDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(GroupDeleteDlg)
	m_DeleteName = _T("");
	//}}AFX_DATA_INIT
}


void GroupDeleteDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(GroupDeleteDlg)
	DDX_CBString(pDX, IDC_DeleteCombo, m_DeleteName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(GroupDeleteDlg, CDialog)
	//{{AFX_MSG_MAP(GroupDeleteDlg)
	ON_BN_CLICKED(IDOK, OnDelete)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// GroupDeleteDlg message handlers

void GroupDeleteDlg::OnDelete() 
{
	// TODO: Add your control notification handler code here
	UpdateData(true);
	if(m_DeleteName == DEFAULT_GROUPNAME)
		AfxMessageBox("Can't delete Default Group!!");
	else if(m_DeleteName.GetLength()==0)
		AfxMessageBox("Deleted Group Name shoud not be empty!!");
	else
		CDialog::OnOK();
}

BOOL GroupDeleteDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	#ifdef _XCAPSERVER

	CString UserName = CUACDlg::GetUAComRegString("User_Info","Username");
	CComboBox*      del_group = (CComboBox *)GetDlgItem(IDC_DeleteCombo);
	GroupList *myGroupList = GetAllGroup(),*tmpGList;
	int gCount=0;
	if(myGroupList!=NULL)
	{
		tmpGList = myGroupList;
		while(tmpGList!=NULL)
		{ 

			if(_stricmp(tmpGList->groupname,DEFAULT_GROUPNAME))
			{
				del_group->SetWindowText(tmpGList->groupname);
				del_group->AddString(tmpGList->groupname);
			}
			tmpGList = tmpGList->next;
			gCount++;
		}
		if(gCount<=1)//only Default Group...
		{
			AfxMessageBox("No Group exist!!Create Group first.");
			FreeGroupList(myGroupList);
			myGroupList = NULL;
			CDialog::OnCancel();
		}
//		del_group->SetWindowText(myGroupList->groupname);
		FreeGroupList(myGroupList);
	}
	else
	{
		AfxMessageBox("No Group exist!!Create Group first.");
		CDialog::OnCancel();
		
//		del_group->AddString(DEFAULT_GROUPNAME);
//		del_group->SetWindowText(DEFAULT_GROUPNAME);		
	}
#endif
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
