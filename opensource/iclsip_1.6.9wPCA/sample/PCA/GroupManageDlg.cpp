// GroupManageDlg.cpp : implementation file
//

#include "stdafx.h"
#include "pcaua.h"
#include "GroupManageDlg.h"
#include "xcapapi.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGroupManageDlg dialog


CGroupManageDlg::CGroupManageDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGroupManageDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGroupManageDlg)
	m_newgroupname = _T("");
	//}}AFX_DATA_INIT
}


void CGroupManageDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGroupManageDlg)
	DDX_Text(pDX, IDC_EDIT1, m_newgroupname);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGroupManageDlg, CDialog)
	//{{AFX_MSG_MAP(CGroupManageDlg)
	ON_BN_CLICKED(IDOK, OnNewGroup)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGroupManageDlg message handlers

void CGroupManageDlg::OnNewGroup() 
{
	// TODO: Add your control notification handler code here
	UpdateData(true);

	// Add by cmchen : avoid chinese words
	if (isChinese(m_newgroupname)) {
		AfxMessageBox("抱歉!Group Name欄位目前不支援中文.");
		return;
	}
	if (m_newgroupname.GetLength()==0)
		AfxMessageBox("Group field can not be empty!");
	else
		CDialog::OnOK();
}
