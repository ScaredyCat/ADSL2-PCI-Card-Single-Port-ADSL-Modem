// SIPExtPage.cpp : implementation file
//

#include "SIPExtPage.h"
#include "VersionNo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// SIPExtPage property page

IMPLEMENT_DYNCREATE(SIPExtPage, CPropertyPage)

SIPExtPage::SIPExtPage() : CPropertyPage(SIPExtPage::IDD)
{
	//{{AFX_DATA_INIT(SIPExtPage)
	m_bAnonFrom = FALSE;
	m_strPreferredID = _T("");
	m_bUsePreferredID = FALSE;
	m_bUseSessionTimer = FALSE;
	m_uSessionTimer = 0;
	m_strVersion = _T("");
	m_strDate = _T("");

	m_szHttpTunnelAddr = _T("");
	m_uiHttpTunnelPort = 0;
	m_bUseHttpTunnel = FALSE;
	//}}AFX_DATA_INIT
}

SIPExtPage::~SIPExtPage()
{
}

void SIPExtPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(SIPExtPage)
	DDX_Control(pDX, IDC_SESSIONTIMER, m_SessionTimerCTL);
	DDX_Control(pDX, IDC_PreferredIdentity, m_PreferredIDCtl);
	DDX_Check(pDX, IDC_ANONFROM, m_bAnonFrom);
	DDX_Text(pDX, IDC_PreferredIdentity, m_strPreferredID);
	DDX_Check(pDX, IDC_USEPREFERREDID, m_bUsePreferredID);
	DDX_Check(pDX, IDC_USESESSIONTIMER, m_bUseSessionTimer);
	DDX_Text(pDX, IDC_SESSIONTIMER, m_uSessionTimer);
	DDX_Text(pDX, IDC_VERSION, m_strVersion);
	DDX_Text(pDX, IDC_DATE, m_strDate);

	DDX_Control(pDX, IDC_USEHTTPTUNNEL, m_UseHttpTunnelCTL);
	DDX_Control(pDX, IDC_HTTPTUNNELPORT, m_HttpTunnelPortCTL);
	DDX_Control(pDX, IDC_HTTPTUNNELADDR, m_HttpTunnelAddrCTL);
	DDX_Text(pDX, IDC_HTTPTUNNELADDR, m_szHttpTunnelAddr);
	DDX_Text(pDX, IDC_HTTPTUNNELPORT, m_uiHttpTunnelPort);
	DDX_Check(pDX, IDC_USEHTTPTUNNEL, m_bUseHttpTunnel);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(SIPExtPage, CPropertyPage)
	//{{AFX_MSG_MAP(SIPExtPage)
	ON_BN_CLICKED(IDC_ANONFROM, OnSelAnonFrom)
	ON_EN_CHANGE(IDC_PreferredIdentity, OnChangePreferredIdentity)
	ON_BN_CLICKED(IDC_USEPREFERREDID, OnUsePreferredID)
	ON_BN_CLICKED(IDC_USESESSIONTIMER, OnUseSessionTimer)
	
	ON_BN_CLICKED(IDC_USEHTTPTUNNEL, OnUseHttpTunnel)
	ON_EN_CHANGE(IDC_HTTPTUNNELADDR, OnChangeHttpTunneladdr)
	ON_EN_CHANGE(IDC_HTTPTUNNELPORT, OnChangeHttpTunnelport)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// SIPExtPage message handlers

BOOL SIPExtPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	CUAProfile* pProfile = CUAProfile::GetProfile();

	m_bAnonFrom = pProfile->m_bAnonFrom;
	m_strPreferredID = pProfile->m_strPreferredID;
	m_bUsePreferredID = pProfile->m_bUsePreferredID;
	m_uSessionTimer = pProfile->m_uSessionTimer;
	m_bUseSessionTimer = pProfile->m_bUseSessionTimer;

	m_strVersion = STRFILEVERSION;
	m_strDate = __DATE__;

	
	// Http Tunnel
	m_bUseHttpTunnel = pProfile->m_bUseHttpTunnel;
	m_szHttpTunnelAddr = pProfile->m_szHttpTunnelAddr;
	m_uiHttpTunnelPort = pProfile->m_usHttpTunnelPort;


	UpdateData(false);

	OnUsePreferredID();
	OnUseSessionTimer();

#ifdef UACOM_USE_DIRECTX
	SetDlgItemText( IDC_SHOWDX, "DX");
#endif

	
	if ( !m_bUseHttpTunnel ) {
		m_HttpTunnelAddrCTL.EnableWindow(FALSE);
		m_HttpTunnelPortCTL.EnableWindow(FALSE);
	}


	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void SIPExtPage::OnSelAnonFrom() 
{
	SetModified(TRUE);
}

void SIPExtPage::OnOK() 
{
	UpdateData();

	CUAProfile* pProfile = CUAProfile::GetProfile();
	pProfile->m_bAnonFrom = m_bAnonFrom;
	pProfile->m_strPreferredID = m_strPreferredID;
	pProfile->m_bUsePreferredID = m_bUsePreferredID;
	pProfile->m_bUseSessionTimer = m_bUseSessionTimer;
	pProfile->m_uSessionTimer = m_uSessionTimer;
	
	CPropertyPage::OnOK();
}

BOOL SIPExtPage::OnApply() 
{
	UpdateData();

	CUAProfile* pProfile = CUAProfile::GetProfile();
	pProfile->m_bAnonFrom = m_bAnonFrom;
	pProfile->m_strPreferredID = m_strPreferredID;
	pProfile->m_bUsePreferredID = m_bUsePreferredID;
	pProfile->m_bUseSessionTimer = m_bUseSessionTimer;
	pProfile->m_uSessionTimer = m_uSessionTimer;

	
	//Http Tunnel
	pProfile->m_bUseHttpTunnel = m_bUseHttpTunnel;
	pProfile->m_szHttpTunnelAddr = m_szHttpTunnelAddr;
	pProfile->m_usHttpTunnelPort = m_uiHttpTunnelPort;


	return CPropertyPage::OnApply();
}

void SIPExtPage::OnChangePreferredIdentity() 
{
	SetModified(TRUE);
}

void SIPExtPage::OnUsePreferredID() 
{
	UpdateData(TRUE);
	if (m_bUsePreferredID) {
		m_PreferredIDCtl.EnableWindow(TRUE);
	} else {
		m_PreferredIDCtl.EnableWindow(FALSE);
	}
	SetModified(TRUE);
}

void SIPExtPage::OnUseSessionTimer() 
{
 	UpdateData(TRUE);
	if (m_bUseSessionTimer) {
		m_SessionTimerCTL.EnableWindow(TRUE);
	} else {
		m_SessionTimerCTL.EnableWindow(FALSE);
	}
	SetModified(TRUE);
}


void SIPExtPage::OnUseHttpTunnel() 
{
	m_bUseHttpTunnel = !m_bUseHttpTunnel;
	
	if ( m_bUseHttpTunnel ) {
		m_HttpTunnelAddrCTL.EnableWindow(TRUE);
		m_HttpTunnelPortCTL.EnableWindow(TRUE);

		// use Tunnel , not use OutBound
		CUAProfile* pProfile = CUAProfile::GetProfile();
		pProfile->m_bUseOutbound = !m_bUseHttpTunnel;	
		
		/*
		if( m_bUseOutbound ) {
		
			m_bUseOutbound=FALSE;
			m_OutboundAddrCTL.EnableWindow(FALSE);
			m_OutboundPortCTL.EnableWindow(FALSE);
			m_UseOutboundCTL.SetCheck(0);// EnableWindow(FALSE);
		}*/
	} else {
		m_HttpTunnelAddrCTL.EnableWindow(FALSE);
		m_HttpTunnelPortCTL.EnableWindow(FALSE);
	}

	

	SetModified();
	
}

void SIPExtPage::OnChangeHttpTunneladdr() 
{
	SetModified(TRUE);
}

void SIPExtPage::OnChangeHttpTunnelport() 
{
	SetModified(TRUE);
}
