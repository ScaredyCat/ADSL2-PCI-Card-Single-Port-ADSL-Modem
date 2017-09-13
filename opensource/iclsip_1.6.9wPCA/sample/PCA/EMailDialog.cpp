// EMailDialog.cpp : implementation file
//

#include "stdafx.h"
#include "pcaua.h"
#include "EMailDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEMailDialog dialog


CEMailDialog::CEMailDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CEMailDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEMailDialog)
	m_emailAddr = _T("");
	m_newPassword = _T("");
	m_notifyType = -1;
	m_interval = _T("1");
	//}}AFX_DATA_INIT
}


void CEMailDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEMailDialog)
	DDX_Text(pDX, IDC_EMAIL_EDIT, m_emailAddr);
	DDX_Text(pDX, IDC_PASSWORD_EDIT, m_newPassword);
	DDX_Radio(pDX, IDC_STOP_NOTIFY_RADIO, m_notifyType);
	DDX_Text(pDX, IDC_INTERVAL_EDIT, m_interval);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEMailDialog, CDialog)
	//{{AFX_MSG_MAP(CEMailDialog)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEMailDialog message handlers
