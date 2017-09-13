// HelpDlg.cpp : implementation file
//

#include "stdafx.h"
#include "pcaua.h"
#include "HelpDlg.h"

#include "PCAUADlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHelpDlg dialog


CHelpDlg::CHelpDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CHelpDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CHelpDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CHelpDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CHelpDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CHelpDlg, CDialog)
	//{{AFX_MSG_MAP(CHelpDlg)
	ON_BN_CLICKED(IDC_BTN_EMAIL, OnBtnEmail)
	ON_BN_CLICKED(IDC_BTN_WEB, OnBtnWeb)
	ON_BN_CLICKED(IDC_BTN_CALL, OnBtnCall)
	ON_BN_CLICKED(IDC_BTN_USERGUIDE, OnBtnUserguide)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHelpDlg message handlers

void CHelpDlg::OnBtnEmail() 
{
	ShellExecute(NULL, "open", "mailto:" + g_pMainDlg->m_strHelpEmailAddress, NULL, NULL, SW_SHOWNORMAL);
	EndDialog(IDOK);
}

void CHelpDlg::OnBtnWeb() 
{
	ShellExecute(NULL, "open", g_pMainDlg->m_strHelpWebSupportLink, NULL, NULL, SW_SHOWNORMAL);
	EndDialog(IDOK);
	
}

void CHelpDlg::OnBtnCall() 
{
	m_helpHotLine = g_pMainDlg->m_strHelpHotlineNumber;

	EndDialog(IDOK);
}

void CHelpDlg::OnBtnUserguide() 
{
	if ( g_pMainDlg->m_strHelpUserGuildLink.Left(7) == "http://")
	{
		ShellExecute(NULL, "open", g_pMainDlg->m_strHelpUserGuildLink, NULL, NULL, SW_SHOWNORMAL);
		EndDialog(IDOK);
	}
	else
	{
		CString helpfile = theApp.m_strAppPath + g_pMainDlg->m_strHelpUserGuildLink;
		ShellExecute( NULL, "open", helpfile, NULL, NULL, SW_SHOWNORMAL);
		EndDialog(IDOK);
	}
}

BOOL CHelpDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	SetWindowText( g_pMainDlg->m_strHelpTitle);
	GetDlgItem(IDC_BTN_USERGUIDE)->EnableWindow( !g_pMainDlg->m_strHelpUserGuildLink.IsEmpty());
	GetDlgItem(IDC_BTN_EMAIL)->EnableWindow( !g_pMainDlg->m_strHelpEmailAddress.IsEmpty());
	GetDlgItem(IDC_BTN_WEB)->EnableWindow( !g_pMainDlg->m_strHelpWebSupportLink.IsEmpty());
	GetDlgItem(IDC_BTN_CALL)->EnableWindow( !g_pMainDlg->m_strHelpHotlineNumber.IsEmpty());
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
