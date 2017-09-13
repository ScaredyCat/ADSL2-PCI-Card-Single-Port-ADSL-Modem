// GroupEditDlg.cpp : implementation file
//

#include "stdafx.h"
#include "pcaua.h"
#include "GroupEditDlg.h"
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
// GroupEditDlg dialog


GroupEditDlg::GroupEditDlg(CWnd* pParent /*=NULL*/)
	: CDialog(GroupEditDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(GroupEditDlg)
	m_newGroupName = _T("");
	m_selGroupName = _T("");
	//}}AFX_DATA_INIT
}


void GroupEditDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(GroupEditDlg)
	DDX_Text(pDX, IDC_EDIT1, m_newGroupName);
	DDX_CBString(pDX, IDC_EDITCOMBO, m_selGroupName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(GroupEditDlg, CDialog)
	//{{AFX_MSG_MAP(GroupEditDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// GroupEditDlg message handlers

BOOL GroupEditDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here


#ifdef _XCAPSERVER

	CString UserName = CUACDlg::GetUAComRegString("User_Info","Username");
	CComboBox*      edit_group = (CComboBox *)GetDlgItem(IDC_EDITCOMBO);
	GroupList *myGroupList = GetAllGroup(),*tmpGList;
	int gCount=0;
	if(myGroupList!=NULL)
	{
		tmpGList = myGroupList;
		while(tmpGList!=NULL)
		{ 

			if(_stricmp(tmpGList->groupname,DEFAULT_GROUPNAME))
			{
				edit_group->AddString(tmpGList->groupname);
				edit_group->SetWindowText(tmpGList->groupname);
			}
			tmpGList = tmpGList->next;
			gCount++;
		}
		if(gCount<=1)//only Default Group...
		{
			AfxMessageBox("No Group exist!!Create Group First.");
			FreeGroupList(myGroupList);
			myGroupList = NULL;
			CDialog::OnCancel();
		}
//		edit_group->SetWindowText(myGroupList->groupname);
		FreeGroupList(myGroupList);
	}
	else
	{
		AfxMessageBox("No Group exist!!Create Group First.");
		CDialog::OnCancel();
		
//		del_group->AddString(DEFAULT_GROUPNAME);
//		del_group->SetWindowText(DEFAULT_GROUPNAME);		
	}
#endif
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void GroupEditDlg::OnOK() 
{
	// TODO: Add extra validation here
	UpdateData(TRUE);

#ifdef _XCAPSERVER
	GroupList *myGroupList = GetAllGroup();
	if(myGroupList!=NULL)
	{
		if(!CheckExistence(myGroupList,(LPSTR)(LPCTSTR)m_selGroupName))
		{
			AfxMessageBox("Selected Group Name not exist!!");
			FreeGroupList(myGroupList);
			return;
		}
		FreeGroupList(myGroupList);
	}
	

	
#endif
	
	// Add by cmchen : avoid chinese words
	if (isChinese(m_newGroupName)) {
		AfxMessageBox("抱歉!Group Name欄位目前不支援中文.");
		return;
	}

	if(m_newGroupName.GetLength()==0 ||m_selGroupName.GetLength()==0)
		AfxMessageBox("The Group Name should not be Empty!!");
	else
		CDialog::OnOK();
}
