// RepeatOptDlg.cpp : implementation file
//

#include "stdafx.h"
#include "pcaua.h"
#include "RepeatOptDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRepeatOptDlg dialog


CRepeatOptDlg::CRepeatOptDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRepeatOptDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRepeatOptDlg)
	m_opt = -1;
	m_msg = _T("");
	//}}AFX_DATA_INIT
}


void CRepeatOptDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRepeatOptDlg)
	DDX_Radio(pDX, IDC_RADIO1, m_opt);
	DDX_Text(pDX, IDC_STATIC_M, m_msg);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRepeatOptDlg, CDialog)
	//{{AFX_MSG_MAP(CRepeatOptDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRepeatOptDlg message handlers
