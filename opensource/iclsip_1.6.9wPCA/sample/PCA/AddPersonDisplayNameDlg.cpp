// AddPersonDisplayNameDlg.cpp : implementation file
//

#include "stdafx.h"
#include "pcaua.h"
#include "AddPersonDisplayNameDlg.h"

#ifdef _XCAPSERVER
#include "xcap_datatype.h"
#include "xcapapi.h"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddPersonDisplayNameDlg dialog


CAddPersonDisplayNameDlg::CAddPersonDisplayNameDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAddPersonDisplayNameDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAddPersonDisplayNameDlg)
	m_Tel = _T("");
	m_DisplayName = _T("");
	//}}AFX_DATA_INIT
}


void CAddPersonDisplayNameDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddPersonDisplayNameDlg)
	DDX_Text(pDX, IDC_STATIC1, m_Tel);
	DDX_Text(pDX, IDC_ADD_USER_NAME, m_DisplayName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddPersonDisplayNameDlg, CDialog)
	//{{AFX_MSG_MAP(CAddPersonDisplayNameDlg)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddPersonDisplayNameDlg message handlers

void CAddPersonDisplayNameDlg::OnOK() 
{
	// TODO: Add extra validation here
	UpdateData(true);
	CEdit *name_edit = (CEdit *)GetDlgItem(IDC_ADD_USER_NAME);
	if(!isEditBoxReadable && name_edit!=NULL)
	{	
		CDialog::OnOK();
		return;
	}


	if(m_DisplayName.GetLength()==0)
	{
		AfxMessageBox("Name field can not be empty!");
		return;
	}
	// Add by cmchen : avoid chinese words
/*	if (isChinese(m_DisplayName)) {
		AfxMessageBox("抱歉!Display Name欄位目前不支援中文.");
		return;
	}
*/
#ifdef _XCAPSERVER
	BuddyList *retList = SearchBuddyByName((LPSTR)(LPCTSTR)m_DisplayName);
	if(retList!=NULL)
	{
		AfxMessageBox("DisplayName already exist!");
		FreeBuddyList(retList);
		return;
	}
#endif
		CDialog::OnOK();
}

BOOL CAddPersonDisplayNameDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	//UpdateData(true);
	isEditBoxReadable = true;
	// TODO: Add extra initialization here
	if(m_DisplayName.GetLength()!=0)
	{
		CEdit *name_edit = (CEdit *)GetDlgItem(IDC_ADD_USER_NAME);
		if(name_edit!=NULL)
			name_edit->SetReadOnly();
		isEditBoxReadable = false;
		
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

int CAddPersonDisplayNameDlg::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
		return -1;
/*	
	UpdateData(true);
	isEditBoxReadable = true;
	// TODO: Add extra initialization here
	if(m_DisplayName.GetLength()!=0)
	{
		CEdit *name_edit = (CEdit *)GetDlgItem(IDC_PERSONAL_ADD_NAME);
		if(name_edit!=NULL)
			name_edit->SetReadOnly();
		isEditBoxReadable = false;
		
	}
	// TODO: Add your specialized creation code here
*/	
	return 0;
}
