// DlgBrowser.cpp : implementation file
//

#include "stdafx.h"
#include "pcaua.h"
#include "DlgBrowser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgBrowser dialog

CDlgBrowser::CDlgBrowser()
{
	m_bFirstShow = true;
	m_pBrowser = NULL;

	//{{AFX_DATA_INIT(CDlgBrowser)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

void CDlgBrowser::ShowLink( CString strTitle, CString strLink, CRect position)
{
	m_strTitle = strTitle;
	m_strLink = strLink;
	m_position = position;

	if ( !m_spBrowserControl)
		return;

	if ( m_bFirstShow)
	{
		m_bFirstShow = false;
		SetWindowPos( NULL, m_position.left, m_position.top, m_position.right, m_position.bottom, SWP_NOZORDER | SWP_SHOWWINDOW );
		SetWindowText( m_strTitle);
		m_spBrowserControl->Navigate( CComBSTR(m_strLink), NULL, NULL, NULL, NULL);
	}
	else
	{
		SetWindowPos( NULL, 0,0,0,0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW );
	}
}


void CDlgBrowser::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgBrowser)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgBrowser, CDialog)
	//{{AFX_MSG_MAP(CDlgBrowser)
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgBrowser message handlers

void CDlgBrowser::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	
	if ( m_pBrowser)
	{
		RECT r;
		GetClientRect( &r);
		m_pBrowser->SetWindowPos( NULL, r.left, r.top, r.right, r.bottom, SWP_NOZORDER);	
	}
}

BOOL CDlgBrowser::OnInitDialog() 
{
	CDialog::OnInitDialog();

	SetWindowPos( NULL, m_position.left, m_position.top, m_position.right, m_position.bottom, SWP_NOZORDER | SWP_SHOWWINDOW );
	
	m_pBrowser = GetDlgItem(IDC_EXPLORER);
	RECT r;
	GetClientRect( &r);
	m_pBrowser->SetWindowPos( NULL, r.left, r.top, r.right, r.bottom, SWP_NOZORDER);		

	CComPtr<IUnknown> spUnknown;
	spUnknown = m_pBrowser->GetControlUnknown();
	HRESULT hr = spUnknown->QueryInterface( &m_spBrowserControl);
	if ( FAILED(hr))
		return FALSE;

	if ( !m_strLink.IsEmpty())
		hr = m_spBrowserControl->Navigate( CComBSTR(m_strLink), NULL, NULL, NULL, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgBrowser::OnDestroy() 
{
	CDialog::OnDestroy();
	
	m_pBrowser = NULL;
}

void CDlgBrowser::OnClose() 
{
	ShowWindow( SW_HIDE);
	//CDialog::OnClose();
}
