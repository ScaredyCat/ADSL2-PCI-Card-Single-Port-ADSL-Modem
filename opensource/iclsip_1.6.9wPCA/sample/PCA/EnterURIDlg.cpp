// EnterURIDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PCAUA.h"
#include "EnterURIDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEnterURIDlg dialog


CEnterURIDlg::CEnterURIDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEnterURIDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEnterURIDlg)
	m_strURI = _T("");
	//}}AFX_DATA_INIT
}


void CEnterURIDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEnterURIDlg)
	DDX_Text(pDX, IDC_EDIT1, m_strURI);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEnterURIDlg, CDialog)
	//{{AFX_MSG_MAP(CEnterURIDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEnterURIDlg message handlers

BOOL CEnterURIDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	GetDlgItem(IDC_EDIT1)->SetFocus();

	return FALSE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
