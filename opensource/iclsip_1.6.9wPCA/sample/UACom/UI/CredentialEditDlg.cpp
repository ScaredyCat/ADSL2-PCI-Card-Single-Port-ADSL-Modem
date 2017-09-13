// CredentialEditDlg.cpp : implementation file
//

#include "CredentialEditDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCredentialEditDlg dialog


CCredentialEditDlg::CCredentialEditDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCredentialEditDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCredentialEditDlg)
	m_RealmName = _T("");
	m_UserID = _T("");
	m_Passwd = _T("");
	m_PasswdVarify = _T("");
	//}}AFX_DATA_INIT
}


void CCredentialEditDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCredentialEditDlg)
	DDX_Control(pDX, IDC_REALMCTL, m_RealmNameCTL);
	DDX_Text(pDX, IDC_REALMCTL, m_RealmName);
	DDX_Text(pDX, IDC_USERIDCTL, m_UserID);
	DDX_Text(pDX, IDC_PASSWDCTL, m_Passwd);
	DDX_Text(pDX, IDC_PASSWDVARIFYCTL, m_PasswdVarify);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCredentialEditDlg, CDialog)
	//{{AFX_MSG_MAP(CCredentialEditDlg)
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCredentialEditDlg message handlers

BOOL CCredentialEditDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	pCredentialPage = &((CPreferSheet*)GetParent())->m_CredentialPage;
	m_RealmName = pCredentialPage->strRealmName;
	m_UserID = pCredentialPage->strUserID;
	m_Passwd = pCredentialPage->strPasswd;

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CCredentialEditDlg::OnOK() 
{
	UpdateData(TRUE);
	if ( !m_Passwd.Compare(m_PasswdVarify) ) {
		pCredentialPage->strRealmName = m_RealmName;
		pCredentialPage->strUserID = m_UserID;
		pCredentialPage->strPasswd = m_Passwd;	
		CDialog::OnOK();
	} else {
		AfxMessageBox( _T("Password not matched! Please re-type the password") );
	}
}

void CCredentialEditDlg::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CDialog::OnShowWindow(bShow, nStatus);
	m_RealmNameCTL.SetFocus();
}
