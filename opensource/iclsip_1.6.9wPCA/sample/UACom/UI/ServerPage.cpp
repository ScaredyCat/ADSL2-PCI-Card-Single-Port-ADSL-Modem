// ServerPage.cpp : implementation file
//

#include "ServerPage.h"
#include "PreferSheet.h"
//#include "dnssrv.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CServerPage property page

IMPLEMENT_DYNCREATE(CServerPage, CPropertyPage)

CServerPage::CServerPage() : CPropertyPage(CServerPage::IDD)
{
	//{{AFX_DATA_INIT(CServerPage)
	m_bExplicitPort = FALSE;
	m_szCallServerAddr = _T("");
	m_uiCallServerPort = 0;
	m_szOutboundAddr = _T("");
	m_uiOutboundPort = 0;
	//m_szHttpTunnelAddr = _T("");
	//m_uiHttpTunnelPort = 0;
	m_szRegistrarAddr = _T("");
	m_uiRegistrarPort = 0;
	m_szSIMPLEServerAddr = _T("");
	m_uiSIMPLEServerPort = 0;
	m_bUseCallServer = FALSE;
	m_bUseOutbound = FALSE;
	//m_bUseHttpTunnel = FALSE;
	m_bUseRegistrar = FALSE;
	m_bUseSIMPLEServer = FALSE;
	m_lExpireTime = 0;
	m_szRegistrar2Addr = _T("");
	m_uiRegistrar2Port = 0;
	m_bUseRegistrar2 = FALSE;
	m_lExpireTime2 = 0;
	m_bExplicitPort2 = FALSE;
	//}}AFX_DATA_INIT
	m_bDirty = FALSE;
}

CServerPage::~CServerPage()
{
}

void CServerPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServerPage)
	DDX_Control(pDX, IDC_BUTTON2, m_SameBTN2);
	DDX_Control(pDX, IDC_EXPLICITPORT2, m_ExplicitPortCTL2);
	DDX_Control(pDX, IDC_USEREGISTRAR2, m_UseRegistrar2CTL);
	DDX_Control(pDX, IDC_REGISTRARPORT2, m_Registrar2PortCTL);
	DDX_Control(pDX, IDC_REGISTRARADDR2, m_Registrar2AddrCTL);
	DDX_Control(pDX, IDC_BUTTON1, m_SameBTN);
	DDX_Control(pDX, IDC_USESIMPLESERVER, m_UseSIMPLEServerCTL);
	DDX_Control(pDX, IDC_USEREGISTRAR, m_UseRegistrarCTL);
	DDX_Control(pDX, IDC_USEOUTBOUND, m_UseOutboundCTL);
	//DDX_Control(pDX, IDC_USEHTTPTUNNEL, m_UseHttpTunnelCTL);
	DDX_Control(pDX, IDC_USECALLSERVER, m_UseCallServerCTL);
	DDX_Control(pDX, IDC_SIMPLESERVERPORT, m_SIMPLEServerPortCTL);
	DDX_Control(pDX, IDC_SIMPLESERVERADDR, m_SIMPLEServerAddrCTL);
	DDX_Control(pDX, IDC_REGISTRARPORT, m_RegistrarPortCTL);
	DDX_Control(pDX, IDC_REGISTRARADDR, m_RegistrarAddrCTL);
	DDX_Control(pDX, IDC_OUTBOUNDPORT, m_OutboundPortCTL);
	DDX_Control(pDX, IDC_OUTBOUNDADDR, m_OutboundAddrCTL);
	//DDX_Control(pDX, IDC_HTTPTUNNELPORT, m_HttpTunnelPortCTL);
	//DDX_Control(pDX, IDC_HTTPTUNNELADDR, m_HttpTunnelAddrCTL);
	DDX_Control(pDX, IDC_EXPLICITPORT, m_ExplicitPortCTL);
	DDX_Control(pDX, IDC_CALLSERVERPORT, m_CallServerPortCTL);
	DDX_Control(pDX, IDC_CALLSERVERADDR, m_CallServerAddrCTL);
	DDX_Check(pDX, IDC_EXPLICITPORT, m_bExplicitPort);
	DDX_Text(pDX, IDC_CALLSERVERADDR, m_szCallServerAddr);
	DDX_Text(pDX, IDC_CALLSERVERPORT, m_uiCallServerPort);
	DDX_Text(pDX, IDC_OUTBOUNDADDR, m_szOutboundAddr);
	DDX_Text(pDX, IDC_OUTBOUNDPORT, m_uiOutboundPort);
	//DDX_Text(pDX, IDC_HTTPTUNNELADDR, m_szHttpTunnelAddr);
	//DDX_Text(pDX, IDC_HTTPTUNNELPORT, m_uiHttpTunnelPort);
	DDX_Text(pDX, IDC_REGISTRARADDR, m_szRegistrarAddr);
	DDX_Text(pDX, IDC_REGISTRARPORT, m_uiRegistrarPort);
	DDX_Text(pDX, IDC_SIMPLESERVERADDR, m_szSIMPLEServerAddr);
	DDX_Text(pDX, IDC_SIMPLESERVERPORT, m_uiSIMPLEServerPort);
	DDX_Check(pDX, IDC_USECALLSERVER, m_bUseCallServer);
	DDX_Check(pDX, IDC_USEOUTBOUND, m_bUseOutbound);
	//DDX_Check(pDX, IDC_USEHTTPTUNNEL, m_bUseHttpTunnel);
	DDX_Check(pDX, IDC_USEREGISTRAR, m_bUseRegistrar);
	DDX_Check(pDX, IDC_USESIMPLESERVER, m_bUseSIMPLEServer);
	DDX_Text(pDX, IDC_EXPIRETIME, m_lExpireTime);
	DDV_MinMaxLong(pDX, m_lExpireTime, 0, 86400);
	DDX_Text(pDX, IDC_REGISTRARADDR2, m_szRegistrar2Addr);
	DDX_Text(pDX, IDC_REGISTRARPORT2, m_uiRegistrar2Port);
	DDX_Check(pDX, IDC_USEREGISTRAR2, m_bUseRegistrar2);
	DDX_Text(pDX, IDC_EXPIRETIME2, m_lExpireTime2);
	DDX_Check(pDX, IDC_EXPLICITPORT2, m_bExplicitPort2);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CServerPage, CPropertyPage)
	//{{AFX_MSG_MAP(CServerPage)
	ON_BN_CLICKED(IDC_EXPLICITPORT, OnExplicitPort)
	ON_BN_CLICKED(IDC_USECALLSERVER, OnUsecallserver)
	ON_BN_CLICKED(IDC_USEREGISTRAR, OnUseregistrar)
	ON_BN_CLICKED(IDC_USESIMPLESERVER, OnUsesimpleserver)
	ON_BN_CLICKED(IDC_USEOUTBOUND, OnUseoutbound)
	//ON_BN_CLICKED(IDC_USEHTTPTUNNEL, OnUseHttpTunnel)
	ON_EN_CHANGE(IDC_CALLSERVERADDR, OnChangeCallserveraddr)
	ON_EN_CHANGE(IDC_CALLSERVERPORT, OnChangeCallserverport)
	ON_EN_CHANGE(IDC_REGISTRARADDR, OnChangeRegistraraddr)
	ON_EN_CHANGE(IDC_EXPIRETIME, OnChangeExpiretime)
	ON_EN_CHANGE(IDC_SIMPLESERVERADDR, OnChangeSimpleserveraddr)
	ON_EN_CHANGE(IDC_SIMPLESERVERPORT, OnChangeSimpleserverport)
	ON_EN_CHANGE(IDC_OUTBOUNDADDR, OnChangeOutboundaddr)
	ON_EN_CHANGE(IDC_OUTBOUNDPORT, OnChangeOutboundport)
	//ON_EN_CHANGE(IDC_HTTPTUNNELADDR, OnChangeHttpTunneladdr)
	//ON_EN_CHANGE(IDC_HTTPTUNNELPORT, OnChangeHttpTunnelport)
	ON_BN_CLICKED(IDC_BUTTON1, OnButton1)
	ON_EN_CHANGE(IDC_REGISTRARPORT, OnChangeRegistrarport)
	ON_BN_CLICKED(IDC_BUTTON2, OnButton2)
	ON_EN_CHANGE(IDC_REGISTRARADDR2, OnChangeRegistraraddr2)
	ON_EN_CHANGE(IDC_REGISTRARPORT2, OnChangeRegistrarport2)
	ON_EN_CHANGE(IDC_EXPIRETIME2, OnChangeExpiretime2)
	ON_BN_CLICKED(IDC_EXPLICITPORT2, OnExplicitport2)
	ON_BN_CLICKED(IDC_USEREGISTRAR2, OnUseregistrar2)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServerPage message handlers

BOOL CServerPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	CUAProfile* pProfile = CUAProfile::GetProfile();
	
	m_bUseCallServer = pProfile->m_bUseCallServer;
	m_szCallServerAddr = pProfile->m_szCallServerAddr;
	m_uiCallServerPort = pProfile->m_usCallServerPort;

	m_bUseRegistrar = pProfile->m_bUseRegistrar;
	m_szRegistrarAddr = pProfile->m_szRegistrarAddr;
	m_uiRegistrarPort = pProfile->m_usRegistrarPort;
	m_lExpireTime = pProfile->m_ulExpireTime;
	m_bExplicitPort = pProfile->m_bExplicitPort;

	m_bUseRegistrar2 = pProfile->m_bUseRegistrar2;
	m_szRegistrar2Addr = pProfile->m_szRegistrar2Addr;
	m_uiRegistrar2Port = pProfile->m_usRegistrar2Port;
	m_lExpireTime2 = pProfile->m_ulExpireTime2;
	m_bExplicitPort2 = pProfile->m_bExplicitPort2;

	m_bUseSIMPLEServer = pProfile->m_bUseSIMPLEServer;
	m_szSIMPLEServerAddr = pProfile->m_szSIMPLEServerAddr;
	m_uiSIMPLEServerPort = pProfile->m_usSIMPLEServerPort;

	m_bUseOutbound = pProfile->m_bUseOutbound;
	m_szOutboundAddr = pProfile->m_szOutboundAddr;
	m_uiOutboundPort = pProfile->m_usOutboundPort;

/*
	// Http Tunnel
	m_bUseHttpTunnel = pProfile->m_bUseHttpTunnel;
	m_szHttpTunnelAddr = pProfile->m_szHttpTunnelAddr;
	m_uiHttpTunnelPort = pProfile->m_usHttpTunnelPort;
*/

	UpdateData(FALSE);

	if ( !m_bUseCallServer ) {
		m_CallServerAddrCTL.EnableWindow(FALSE);
		m_CallServerPortCTL.EnableWindow(FALSE);
	}
	if ( !m_bUseRegistrar ) {
		m_RegistrarAddrCTL.EnableWindow(FALSE);
		m_RegistrarPortCTL.EnableWindow(FALSE);
	}
	if ( !m_bUseRegistrar2 ) {
		m_Registrar2AddrCTL.EnableWindow(FALSE);
		m_Registrar2PortCTL.EnableWindow(FALSE);
	}
	if ( !m_bUseSIMPLEServer ) {
		m_SIMPLEServerAddrCTL.EnableWindow(FALSE);
		m_SIMPLEServerPortCTL.EnableWindow(FALSE);
	}
	if ( !m_bUseOutbound ) {
		m_OutboundAddrCTL.EnableWindow(FALSE);
		m_OutboundPortCTL.EnableWindow(FALSE);
	}
/*
	if ( !m_bUseHttpTunnel ) {
		m_HttpTunnelAddrCTL.EnableWindow(FALSE);
		m_HttpTunnelPortCTL.EnableWindow(FALSE);
	}
*/
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CServerPage::OnOK() 
{
	OnApply();
	
	CPropertyPage::OnOK();
}

BOOL CServerPage::OnApply() 
{
	CUAProfile* pProfile = CUAProfile::GetProfile();

	UpdateData(TRUE);
	
	//Call Server
	pProfile->m_bUseCallServer = m_bUseCallServer;
	pProfile->m_szCallServerAddr = m_szCallServerAddr;
	pProfile->m_usCallServerPort = m_uiCallServerPort;
	
	//SIMPLE Server
	pProfile->m_bUseSIMPLEServer	= m_bUseSIMPLEServer;
	pProfile->m_szSIMPLEServerAddr = m_szSIMPLEServerAddr;
	pProfile->m_usSIMPLEServerPort = m_uiSIMPLEServerPort;

	//Outbound
	pProfile->m_bUseOutbound   = m_bUseOutbound;
	pProfile->m_szOutboundAddr = m_szOutboundAddr;
	pProfile->m_usOutboundPort = m_uiOutboundPort;
/*
	//Http Tunnel
	pProfile->m_bUseHttpTunnel = m_bUseHttpTunnel;
	pProfile->m_szHttpTunnelAddr = m_szHttpTunnelAddr;
	pProfile->m_usHttpTunnelPort = m_uiHttpTunnelPort;
*/
	//Registrar
	pProfile->m_bUseRegistrar = m_bUseRegistrar;
	pProfile->m_szRegistrarAddr = m_szRegistrarAddr;
	pProfile->m_usRegistrarPort = m_uiRegistrarPort;
	pProfile->m_ulExpireTime = m_lExpireTime;
	pProfile->m_bExplicitPort = m_bExplicitPort;

	//Registrar2
	pProfile->m_bUseRegistrar2 = m_bUseRegistrar2;
	pProfile->m_szRegistrar2Addr = m_szRegistrar2Addr;
	pProfile->m_usRegistrar2Port = m_uiRegistrar2Port;
	pProfile->m_ulExpireTime2 = m_lExpireTime2;
	pProfile->m_bExplicitPort2 = m_bExplicitPort2;

	pProfile->Write();
	m_bDirty = TRUE;

	return 1;
}

void CServerPage::OnExplicitPort() 
{
	SetModified(TRUE);
}

void CServerPage::OnUsecallserver() 
{
	m_bUseCallServer = !m_bUseCallServer;
	
	if ( m_bUseCallServer ) {
		m_CallServerAddrCTL.EnableWindow(TRUE);
		m_CallServerPortCTL.EnableWindow(TRUE);
	} else {
		m_CallServerAddrCTL.EnableWindow(FALSE);
		m_CallServerPortCTL.EnableWindow(FALSE);
	}

	SetModified();
}

void CServerPage::OnUseregistrar() 
{
	m_bUseRegistrar = !m_bUseRegistrar;
	
	if ( m_bUseRegistrar ) {
		m_RegistrarAddrCTL.EnableWindow(TRUE);
		m_RegistrarPortCTL.EnableWindow(TRUE);
	} else {
		m_RegistrarAddrCTL.EnableWindow(FALSE);
		m_RegistrarPortCTL.EnableWindow(FALSE);
	}

	SetModified();
}

void CServerPage::OnUsesimpleserver() 
{
	m_bUseSIMPLEServer = !m_bUseSIMPLEServer;
	
	if ( m_bUseSIMPLEServer ) {
		m_SIMPLEServerAddrCTL.EnableWindow(TRUE);
		m_SIMPLEServerPortCTL.EnableWindow(TRUE);
	} else {
		m_SIMPLEServerAddrCTL.EnableWindow(FALSE);
		m_SIMPLEServerPortCTL.EnableWindow(FALSE);
	}

	SetModified();
	
}
/*
void CServerPage::OnUseHttpTunnel() 
{
	m_bUseHttpTunnel = !m_bUseHttpTunnel;
	
	if ( m_bUseHttpTunnel ) {
		m_HttpTunnelAddrCTL.EnableWindow(TRUE);
		m_HttpTunnelPortCTL.EnableWindow(TRUE);
		
		if( m_bUseOutbound ) {
		
			m_bUseOutbound=FALSE;
			m_OutboundAddrCTL.EnableWindow(FALSE);
			m_OutboundPortCTL.EnableWindow(FALSE);
			m_UseOutboundCTL.SetCheck(0);// EnableWindow(FALSE);
		}
	} else {
		m_HttpTunnelAddrCTL.EnableWindow(FALSE);
		m_HttpTunnelPortCTL.EnableWindow(FALSE);
	}

	SetModified();
	
}
*/
void CServerPage::OnUseoutbound() 
{
	m_bUseOutbound = !m_bUseOutbound;
	
	if ( m_bUseOutbound ) {
		m_OutboundAddrCTL.EnableWindow(TRUE);
		m_OutboundPortCTL.EnableWindow(TRUE);
/*
		if( m_bUseHttpTunnel ) {
		
			m_bUseHttpTunnel=FALSE;
			m_HttpTunnelAddrCTL.EnableWindow(FALSE);
			m_HttpTunnelPortCTL.EnableWindow(FALSE);
			m_UseHttpTunnelCTL.SetCheck(0);//EnableWindow(FALSE);
		}
*/
		// use OutBound, not use Tunnel
		
		CUAProfile* pProfile = CUAProfile::GetProfile();
		pProfile->m_bUseHttpTunnel = !m_bUseOutbound;
		
		
	} else {
		m_OutboundAddrCTL.EnableWindow(FALSE);
		m_OutboundPortCTL.EnableWindow(FALSE);

	}

	SetModified();
	
}

void CServerPage::OnChangeCallserveraddr() 
{
	SetModified(TRUE);
}

void CServerPage::OnChangeCallserverport() 
{
	SetModified(TRUE);
}

void CServerPage::OnChangeRegistraraddr() 
{
	SetModified(TRUE);
}

void CServerPage::OnChangeRegistrarport() 
{
	SetModified(TRUE);
}

void CServerPage::OnChangeExpiretime() 
{
	SetModified(TRUE);
}

void CServerPage::OnChangeSimpleserveraddr() 
{
	SetModified(TRUE);
}

void CServerPage::OnChangeSimpleserverport() 
{
	SetModified(TRUE);
}

void CServerPage::OnChangeOutboundaddr() 
{
	SetModified(TRUE);
}

void CServerPage::OnChangeOutboundport() 
{
	SetModified(TRUE);
}

/*void CServerPage::OnChangeHttpTunneladdr() 
{
	SetModified(TRUE);
}

void CServerPage::OnChangeHttpTunnelport() 
{
	SetModified(TRUE);
}
*/

void CServerPage::OnButton1() 
{
	UpdateData(TRUE);
	m_szRegistrarAddr = m_szCallServerAddr;
	m_uiRegistrarPort = m_uiCallServerPort;

	UpdateData(FALSE);
	SetModified(TRUE);
}


void CServerPage::OnButton2() 
{
	UpdateData(TRUE);
	m_szRegistrar2Addr = m_szCallServerAddr;
	m_uiRegistrar2Port = m_uiCallServerPort;

	UpdateData(FALSE);
	SetModified(TRUE);
	
}

void CServerPage::OnChangeRegistraraddr2() 
{
	SetModified(TRUE);
}

void CServerPage::OnChangeRegistrarport2() 
{
	SetModified(TRUE);
}

void CServerPage::OnChangeExpiretime2() 
{
	SetModified(TRUE);
}

void CServerPage::OnExplicitport2() 
{
	SetModified(TRUE);
}

void CServerPage::OnUseregistrar2() 
{
	m_bUseRegistrar2 = !m_bUseRegistrar2;
	
	if ( m_bUseRegistrar2 ) {
		m_Registrar2AddrCTL.EnableWindow(TRUE);
		m_Registrar2PortCTL.EnableWindow(TRUE);
	} else {
		m_Registrar2AddrCTL.EnableWindow(FALSE);
		m_Registrar2PortCTL.EnableWindow(FALSE);
	}

	SetModified();
}
