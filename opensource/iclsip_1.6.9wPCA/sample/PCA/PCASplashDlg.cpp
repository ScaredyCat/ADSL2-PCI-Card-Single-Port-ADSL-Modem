// PCASplashDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PCAUA.h"
#include "PCASplashDlg.h"

#include "PCAUADlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPCASplashDlg dialog


CPCASplashDlg::CPCASplashDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPCASplashDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPCASplashDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CPCASplashDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPCASplashDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPCASplashDlg, CDialog)
	//{{AFX_MSG_MAP(CPCASplashDlg)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPCASplashDlg message handlers

int CPCASplashDlg::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	CenterWindow();
	ShowWindow(SW_SHOWNORMAL);
	SetWindowPos(&wndTop,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
	
	return 0;
}

void CPCASplashDlg::SetSplashText(LPCTSTR txt, LPCTSTR app)
{
	SetDlgItemText( IDC_SPLASH_MSG, txt);
	if ( app)
		SetDlgItemText( IDC_APPNAME, app);
	else
		SetDlgItemText( IDC_APPNAME, g_pMainDlg->m_strAboutProductName);

	Invalidate();
	UpdateWindow();
}

BOOL CPCASplashDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	SetDlgItemText( IDC_APPNAME, g_pMainDlg->m_strAboutProductName);
		
	// set for top most and put the dialog at center of v/h of desktop!!
	
	SetWindowPos(&this->wndTopMost , 0, 0,0,0,SWP_NOSIZE | SWP_NOMOVE);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
