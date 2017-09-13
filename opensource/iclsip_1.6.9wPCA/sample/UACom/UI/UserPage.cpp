// UserPage.cpp : implementation file
//

#include "UserPage.h"
#include "PreferSheet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUserPage property page

IMPLEMENT_DYNCREATE(CUserPage, CPropertyPage)

CUserPage::CUserPage() : CPropertyPage(CUserPage::IDD)
{
	//{{AFX_DATA_INIT(CUserPage)
	//m_LocalPort = 0;
	m_LocalPort = _T("");
	m_Username = _T("");
	m_DisplayName = _T("");
	m_ContactAddr = _T("");
	m_PublicAddr = _T("");
	m_bCustomize = FALSE;
	m_STUNserver = _T("");
	m_bUseSTUN = FALSE;
	m_LocalAddr1 = _T("");
	//}}AFX_DATA_INIT
}

CUserPage::~CUserPage()
{
}

void CUserPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CUserPage)
	DDX_Control(pDX, IDC_STUNSERVER, m_STUNserverCTL);
	DDX_Control(pDX, IDC_IPADDRLIST, m_IPAddrListCTL);
	DDX_Control(pDX, IDC_USERNAME, m_UsernameCTL);
	DDX_Control(pDX, IDC_DISPLAYNAME, m_DisplayNameCTL);
	DDX_Control(pDX, IDC_PUBLICADDRESS, m_PublicAddrCTL);
	DDX_Control(pDX, IDC_CONTACTADDRESS, m_ContactAddrCTL);
	DDX_Control(pDX, IDC_LOCALADDR, m_LocalAddrCTL);
	DDX_Control(pDX, IDC_USEUDP, m_UseUDPCTL);
	DDX_Control(pDX, IDC_USETCP, m_UseTCPCTL);
	DDX_Control(pDX, IDC_LOCALPORT, m_LocalPortCTL);
	DDX_Text(pDX, IDC_LOCALPORT, m_LocalPort);
	DDX_Text(pDX, IDC_USERNAME, m_Username);
	DDX_Text(pDX, IDC_DISPLAYNAME, m_DisplayName);
	DDX_Text(pDX, IDC_CONTACTADDRESS, m_ContactAddr);
	DDX_Text(pDX, IDC_PUBLICADDRESS, m_PublicAddr);
	DDX_Check(pDX, IDC_CUSTOM, m_bCustomize);
	DDX_Text(pDX, IDC_STUNSERVER, m_STUNserver);
	DDX_Check(pDX, IDC_USESTUN, m_bUseSTUN);
	DDX_Text(pDX, IDC_LOCALADDR, m_LocalAddr1);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CUserPage, CPropertyPage)
	//{{AFX_MSG_MAP(CUserPage)
	ON_BN_CLICKED(IDC_USEUDP, OnUseUDP)
	ON_BN_CLICKED(IDC_USETCP, OnUseTCP)
	ON_NOTIFY(IPN_FIELDCHANGED, IDC_LOCALADDR, OnFieldchangedLocalAddr)
	ON_EN_CHANGE(IDC_LOCALPORT, OnChangeLocalPort)
	ON_BN_CLICKED(IDC_CUSTOM, OnCustom)
	ON_EN_CHANGE(IDC_USERNAME, OnChangeUsername)
	ON_EN_CHANGE(IDC_DISPLAYNAME, OnChangeDisplayname)
	ON_EN_CHANGE(IDC_PUBLICADDRESS, OnChangePublicaddress)
	ON_EN_CHANGE(IDC_CONTACTADDRESS, OnChangeContactaddress)
	ON_CBN_SELCHANGE(IDC_IPADDRLIST, OnSelChangeIPAddrList)
	ON_BN_CLICKED(IDC_USESTUN, OnUseSTUN)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUserPage message handlers

BOOL CUserPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	CUAProfile* pProfile = CUAProfile::GetProfile();
	
	m_DisplayName = pProfile->m_DisplayName;
	m_Username = pProfile->m_Username;

	//m_PublicAddr = pProfile->m_PublicAddr;
	//m_ContactAddr = pProfile->m_ContactAddr;
	//m_LocalPort = pProfile->m_LocalPort;

	m_LocalPort.Format("%d",pProfile->m_LocalPort);
	m_bCustomize = pProfile->m_bUseStaticIP;
	m_STUNserver = pProfile->m_STUNserver;
	m_bUseSTUN = pProfile->m_bUseSTUN;

	const char** szIPList = getAllMyAddr();
	for (int i=0; i< 16; i++) {
		if (strlen(szIPList[i]) > 0)
			m_IPAddrListCTL.AddString(szIPList[i]);
	}
	
	int nIndex = m_IPAddrListCTL.FindString(-1, pProfile->m_LocalAddr);
	if (nIndex >= 0)
		m_IPAddrListCTL.SetCurSel(nIndex);
	else
		m_IPAddrListCTL.SetCurSel(0);
	
	if (m_bCustomize) {
		m_LocalAddr1 = pProfile->m_LocalAddr;
		m_LocalAddr = m_LocalAddr1;
	} else {
		m_IPAddrListCTL.GetLBText(0, m_LocalAddr); 
		m_LocalAddrCTL.EnableWindow(FALSE);
	}

	if (m_bUseSTUN)
		m_STUNserverCTL.EnableWindow(TRUE);
	else
		m_STUNserverCTL.EnableWindow(FALSE);

	UpdateData(FALSE);
	
	// m_LocalAddrCTL.SetAddress(AddrCStrToDW(m_LocalAddr));
	
	if ( pProfile->m_bTransport ) {
		m_UseTCPCTL.SetCheck(1);
	} else {
		m_UseUDPCTL.SetCheck(1);
	}

	UpdateContactAddr();
	UpdatePublicAddr();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CUserPage::UpdateLocalAddr()
{
	if (m_bCustomize) {
		UpdateData(TRUE);
		m_LocalAddr = m_LocalAddr1;
	} else {
		int nIndex = m_IPAddrListCTL.GetCurSel();
		m_IPAddrListCTL.GetLBText(nIndex, m_LocalAddr);
	}
}

void CUserPage::UpdateContactAddr()
{
	UpdateLocalAddr();

	if (m_UseTCPCTL.GetCheck() == 1)
		m_ContactAddr.Format("sip:%s@%s:%d; transport=tcp",m_Username,m_LocalAddr,atoi(m_LocalPort));
	else
		m_ContactAddr.Format("sip:%s@%s:%d",m_Username,m_LocalAddr,atoi(m_LocalPort));

	DWORD dwSel = m_ContactAddrCTL.GetSel();
	m_ContactAddrCTL.SetSel(HIWORD(dwSel), -1);
	m_ContactAddrCTL.ReplaceSel(m_ContactAddr);
	m_ContactAddrCTL.SetSel(0,1);

	UpdateData(FALSE);
}

void CUserPage::UpdatePublicAddr()
{
	CUAProfile* pProfile = CUAProfile::GetProfile();
	
	UpdateLocalAddr();

	if (m_bCustomize) {
		UpdateData(TRUE);
		m_LocalAddr = m_LocalAddr1;
	} else {
		int nIndex = m_IPAddrListCTL.GetCurSel();
		m_IPAddrListCTL.GetLBText(nIndex, m_LocalAddr);
	}

	if (pProfile->m_bUseRegistrar) {
		m_PublicAddr.Format("sip:%s@%s:",m_Username,pProfile->m_szRegistrarAddr);
		CString tmpStr;
		tmpStr.Format("%d", pProfile->m_usRegistrarPort);
		m_PublicAddr += tmpStr;
	} else {
		m_PublicAddr.Format("sip:%s@%s:%d",m_Username,m_LocalAddr,atoi(m_LocalPort));
	}

	DWORD dwSel = m_PublicAddrCTL.GetSel();
	m_PublicAddrCTL.SetSel(HIWORD(dwSel), -1);
	m_PublicAddrCTL.ReplaceSel(m_PublicAddr);

	UpdateData(FALSE);
}

void CUserPage::OnCustom() 
{
	UpdateData(TRUE);
	if (m_bCustomize) {
		m_LocalAddrCTL.EnableWindow(TRUE);
	} else {
		m_LocalAddrCTL.EnableWindow(FALSE);
	}
	UpdateContactAddr();
	UpdatePublicAddr();
}

void CUserPage::OnUseUDP() 
{
	UpdateData(TRUE);
	UpdateContactAddr();
	SetModified(TRUE);
}

void CUserPage::OnUseTCP() 
{
	UpdateData(TRUE);
	UpdateContactAddr();
	SetModified(TRUE);
}

void CUserPage::OnFieldchangedLocalAddr(NMHDR* pNMHDR, LRESULT* pResult) 
{
	UpdateLocalAddr();
	UpdateContactAddr();
	UpdatePublicAddr();
	SetModified(TRUE);
	*pResult = 0;
}

void CUserPage::OnChangeLocalPort() 
{
	UpdateData(TRUE);
	UpdateContactAddr();
	UpdatePublicAddr();
	SetModified(TRUE);
}

void CUserPage::OnChangeUsername() 
{
	UpdateData(TRUE);
	UpdatePublicAddr();
	UpdateContactAddr();
	SetModified(TRUE);
}

void CUserPage::OnChangeDisplayname() 
{
	SetModified(TRUE);
}

void CUserPage::OnChangePublicaddress() 
{
	SetModified(TRUE);;
}

void CUserPage::OnChangeContactaddress() 
{
	SetModified(TRUE);
}

BOOL CUserPage::OnApply() 
{
	CUAProfile* pProfile = CUAProfile::GetProfile();

	UpdateData(TRUE);
	
	// User Info
	pProfile->m_DisplayName = m_DisplayName;
	pProfile->m_Username = m_Username;
	//pProfile->m_ContactAddr = m_ContactAddr;
	//pProfile->m_PublicAddr = m_PublicAddr;
		
	// Local Settings
	pProfile->m_STUNserver = m_STUNserver;
	pProfile->m_bUseSTUN = m_bUseSTUN;
	pProfile->m_LocalPort = atoi(m_LocalPort);
	if( m_bCustomize )
		pProfile->m_LocalAddr = m_LocalAddr1;
	else
		pProfile->m_LocalAddr = m_LocalAddr;
	pProfile->m_bTransport = (m_UseTCPCTL.GetCheck() == 1) ? 1:0;
	pProfile->m_bUseStaticIP = m_bCustomize;
	pProfile->Write();

	return CPropertyPage::OnApply();
}

void CUserPage::OnOK() 
{
	CUAProfile* pProfile = CUAProfile::GetProfile();

	UpdateData(TRUE);
	
	// User Info
	pProfile->m_DisplayName = m_DisplayName;
	pProfile->m_Username = m_Username;
	//pProfile->m_ContactAddr = m_ContactAddr;
	//pProfile->m_PublicAddr = m_PublicAddr;
		
	// Local Settings
	pProfile->m_STUNserver = m_STUNserver;
	pProfile->m_bUseSTUN = m_bUseSTUN;
	pProfile->m_LocalPort = atoi(m_LocalPort);
	if( m_bCustomize )
		pProfile->m_LocalAddr = m_LocalAddr1;
	else
		pProfile->m_LocalAddr = m_LocalAddr;
	pProfile->m_bTransport = (m_UseTCPCTL.GetCheck() == 1) ? 1:0;
	pProfile->m_bUseStaticIP = m_bCustomize;

	CPropertyPage::OnOK();
}

BOOL CUserPage::OnSetActive() 
{
	if ( ((CPreferSheet*)GetParent())->m_ServerPage.m_bDirty ) {
		UpdatePublicAddr();
		((CPreferSheet*)GetParent())->m_ServerPage.m_bDirty = FALSE;
		SetModified(FALSE);
	}
	return CPropertyPage::OnSetActive();
}

void CUserPage::OnSelChangeIPAddrList() 
{
	UpdateLocalAddr();
	UpdateContactAddr();
	UpdatePublicAddr();
	SetModified(TRUE);
}

void CUserPage::OnUseSTUN() 
{
	UpdateData(TRUE);
	if (m_bUseSTUN) {
		m_STUNserverCTL.EnableWindow(TRUE);
	} else {
		m_STUNserverCTL.EnableWindow(FALSE);
	}
}
