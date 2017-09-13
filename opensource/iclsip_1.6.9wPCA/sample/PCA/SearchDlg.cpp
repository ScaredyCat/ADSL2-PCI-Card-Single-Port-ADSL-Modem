// SearchDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PCAUA.h"
#include "SearchDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSearchDlg dialog


CSearchDlg::CSearchDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSearchDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSearchDlg)
	m_SearchDepart = _T("");
	m_SearchDivision = _T("");
	m_SearchID = _T("");
	m_SearchName = _T("");
	m_SearchTel = _T("");
	//}}AFX_DATA_INIT
}


void CSearchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSearchDlg)
	DDX_Text(pDX, IDC_EDIT_SearchDepart, m_SearchDepart);
	DDX_Text(pDX, IDC_EDIT_SearchDivision, m_SearchDivision);
	DDX_Text(pDX, IDC_EDIT_SearchID, m_SearchID);
	DDX_Text(pDX, IDC_EDIT_SearchName, m_SearchName);
	DDX_Text(pDX, IDC_EDIT_SearchTel, m_SearchTel);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSearchDlg, CDialog)
	//{{AFX_MSG_MAP(CSearchDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSearchDlg message handlers
