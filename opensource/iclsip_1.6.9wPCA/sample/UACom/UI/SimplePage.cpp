// CSimplePage.cpp : implementation file
//

#include "stdafx.h"
#include "ServerPage.h"
#include "PreferSheet.h"
#include "SimplePage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSimplePage dialog

IMPLEMENT_DYNCREATE(CSimplePage, CPropertyPage)

CSimplePage::CSimplePage() : CPropertyPage(CSimplePage::IDD)
{
	//{{AFX_DATA_INIT(CSimplePage)
	m_strSimpleHost = _T("");
	m_nSimplePort = 0;
	m_bUseSimple = FALSE;
	//}}AFX_DATA_INIT
}


void CSimplePage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSimplePage)
	DDX_Text(pDX, IDC_SIMPLEHOST, m_strSimpleHost);
	DDX_Text(pDX, IDC_SIMPLEPORT, m_nSimplePort);
	DDX_Check(pDX, IDC_USESIMPLE, m_bUseSimple);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSimplePage, CDialog)
	//{{AFX_MSG_MAP(CSimplePage)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSimplePage message handlers

BOOL CSimplePage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	CUAProfile* pProfile = CUAProfile::GetProfile();

	m_strSimpleHost = pProfile->m_SimpleIP;
	m_nSimplePort = pProfile->m_SimplePort;
	m_bUseSimple = pProfile->m_bUseExtSimple;

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CSimplePage::OnOK() 
{
	CUAProfile* pProfile = CUAProfile::GetProfile();

	UpdateData(TRUE);
	
	//SIMPLE 
	pProfile->m_bUseExtSimple = m_bUseSimple;
	pProfile->m_SimpleIP = m_strSimpleHost;
	pProfile->m_SimplePort = m_nSimplePort;

	CPropertyPage::OnOK();
}

BOOL CSimplePage::OnApply() 
{
	CUAProfile* pProfile = CUAProfile::GetProfile();

	UpdateData(TRUE);
	
	//SIMPLE 
	pProfile->m_bUseExtSimple = m_bUseSimple;
	pProfile->m_SimpleIP = m_strSimpleHost;
	pProfile->m_SimplePort = m_nSimplePort;

	pProfile->Write();

	return TRUE;
}
