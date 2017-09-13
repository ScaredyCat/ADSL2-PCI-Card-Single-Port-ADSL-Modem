// ConfirmDeleteDlg.cpp : implementation file
//

#include "ConfirmDeleteDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConfirmDeleteDlg dialog


CConfirmDeleteDlg::CConfirmDeleteDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CConfirmDeleteDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CConfirmDeleteDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CConfirmDeleteDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConfirmDeleteDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConfirmDeleteDlg, CDialog)
	//{{AFX_MSG_MAP(CConfirmDeleteDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfirmDeleteDlg message handlers
