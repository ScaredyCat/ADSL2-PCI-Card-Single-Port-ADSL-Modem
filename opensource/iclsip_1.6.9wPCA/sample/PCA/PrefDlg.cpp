// PrefDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PCAUA.h"
#include "PrefDlg.h"
#include "UACDlg.h"
//#include <winsock2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const char** getAllMyAddr()
{
	int i;

	static char  bufs[16][32];
	static const char *pbufs[16];
	struct hostent *hp;
	char hostname[256];
	struct in_addr addr;

	for (i = 0; i < 16; i++) {
		bufs[i][0] = '\0';
		pbufs[i] = bufs[i];
	}

	if (::gethostname(hostname, sizeof(hostname)) < 0)
		return pbufs;

	if ((hp = ::gethostbyname(hostname)) == NULL)
		return pbufs;

	for (i = 0; i < 16 && hp->h_addr_list[i]; i++) {
		memcpy(&addr.s_addr, hp->h_addr_list[i], sizeof(addr.s_addr));
		_snprintf(bufs[i], 32, "%s", ::inet_ntoa(addr));
		bufs[i][31] = '\0';
	}

	return pbufs;
}

int FindInArray( LPCSTR str, CArray<CString,CString&>& vector)
{
	for ( int i=0; i<vector.GetSize(); i++)
	{
		if ( vector.GetAt(i) == str)
			return i;
	}
	return -1;
}

/////////////////////////////////////////////////////////////////////////////
// CPrefDlg dialog


CPrefDlg::CPrefDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPrefDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPrefDlg)
	m_bAutoStartup = FALSE;
	m_bAutoCheck = FALSE;
	m_DSPort = 0;
	m_UserName = _T("");
	m_bUseDeployServer = FALSE;
	m_creRealm = _T("");
	m_creAccount = _T("");
	m_crePassword = _T("");
	m_strDSIP = _T("");
	m_strVMAccount = _T("");
	m_strVMPassword = _T("");
	m_bEnableVM = FALSE;
	m_Quality = -1;
	m_bAllowCallWaiting = FALSE;
	m_bUseLMS = FALSE;
	m_LMSPort = 0;
	m_strLMSIP = _T("");
	//}}AFX_DATA_INIT

	m_bSafeMode=FALSE;
}


void CPrefDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPrefDlg)
	DDX_Control(pDX, IDC_CONFIG_MODE, m_ConfigMode);
	DDX_Control(pDX, IDC_LOCALIPLIST, m_LocalIPList);
	DDX_Check(pDX, IDC_AUTOSTART_CHK, m_bAutoStartup);
	DDX_Check(pDX, IDC_UPDATE_CHK, m_bAutoCheck);
	DDX_Text(pDX, IDC_EDIT_DSPORT, m_DSPort);
	DDX_Text(pDX, IDC_EDIT_USERNAME, m_UserName);
	DDX_Check(pDX, IDC_USEDEPLOY, m_bUseDeployServer);
	DDX_Text(pDX, IDC_CRE_REALM, m_creRealm);
	DDX_Text(pDX, IDC_CRE_ACCOUNT, m_creAccount);
	DDX_Text(pDX, IDC_CRE_PASSWORD, m_crePassword);
	DDX_Text(pDX, IDC_DSIP, m_strDSIP);
	DDX_Text(pDX, IDC_VMACCOUNT, m_strVMAccount);
	DDX_Text(pDX, IDC_VMPASSWORD, m_strVMPassword);
	DDX_Check(pDX, IDC_USEVM, m_bEnableVM);
	DDX_Radio(pDX, IDC_RAD_BETTER, m_Quality);
	DDX_Check(pDX, IDC_CALL_WAITING, m_bAllowCallWaiting);
	DDX_Check(pDX, IDC_USELMS, m_bUseLMS);
	DDX_Text(pDX, IDC_LMSPORT, m_LMSPort);
	DDX_Text(pDX, IDC_LMSIP, m_strLMSIP);
	//}}AFX_DATA_MAP

	// ddx for deploy server ip address control
	/*
	// DO NOT use ip address control in order to enter domain name

	if (pDX->m_bSaveAndValidate)
	{
		struct in_addr oINAddr;
		m_DSIPAddr.GetAddress(oINAddr.S_un.S_un_b.s_b1, 
								 oINAddr.S_un.S_un_b.s_b2,
								 oINAddr.S_un.S_un_b.s_b3, 
								 oINAddr.S_un.S_un_b.s_b4 );
		m_strDSIP = inet_ntoa(oINAddr);
	}
	else
	{
		struct in_addr oINAddr;
		oINAddr.S_un.S_addr = inet_addr(m_strDSIP);
		m_DSIPAddr.SetAddress(oINAddr.S_un.S_un_b.s_b1, 
								 oINAddr.S_un.S_un_b.s_b2,
								 oINAddr.S_un.S_un_b.s_b3, 
								 oINAddr.S_un.S_un_b.s_b4 );
	}
	*/
}


BEGIN_MESSAGE_MAP(CPrefDlg, CDialog)
	//{{AFX_MSG_MAP(CPrefDlg)
	ON_BN_CLICKED(IDC_BTN_ADVANCED, OnBtnAdvanced)
	ON_BN_CLICKED(IDC_USEDEPLOY, OnUsedeploy)
	ON_BN_CLICKED(IDC_USEVM, OnUsevm)
	ON_BN_CLICKED(IDC_USELMS, OnUselms)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPrefDlg message handlers

BOOL CPrefDlg::OnInitDialog() 
{
	CWnd* pHandle;

	CDialog::OnInitDialog();
	
	CWinApp* pApp = AfxGetApp();
	m_bAutoCheck = pApp->GetProfileInt( "", "CheckUpdates", 0 );
	m_bAutoStartup = pApp->GetProfileInt( "", "AutoStartUp", 0 );
	m_bUseDeployServer = !pApp->GetProfileInt( "", "DisableDeployServer", 0 );
	m_bUseLMS = pApp->GetProfileInt( "", "UseLMS", 0 );

	// Arlene added : if don't use deploy server, disable the settinga about deploy server
	if (m_bUseDeployServer == false)
	{
		pHandle = GetDlgItem(IDC_DSIP);
		if (pHandle)
			pHandle->EnableWindow(FALSE);
		pHandle = GetDlgItem(IDC_EDIT_DSPORT);
		if (pHandle)
			pHandle->EnableWindow(FALSE);
		pHandle = GetDlgItem(IDC_UPDATE_CHK);
		if (pHandle)
			pHandle->EnableWindow(FALSE);
		pHandle = GetDlgItem(IDC_AUTOSTART_CHK);
		if (pHandle)
			pHandle->EnableWindow(FALSE);
	}

	if (!m_bUseLMS) {
		pHandle = GetDlgItem(IDC_LMSIP);
		if (pHandle)
			pHandle->EnableWindow(FALSE);
		pHandle = GetDlgItem(IDC_LMSPORT);
		if (pHandle)
			pHandle->EnableWindow(FALSE);
	}


	m_strDSIP = pApp->GetProfileString( "", "DeployServerAddress" );
	m_DSPort = atoi((LPCTSTR)pApp->GetProfileString( "","DeployServerPort", "80" ));
	
	m_strLMSIP =  pApp->GetProfileString( "", "LMSAddress" );
	m_LMSPort = pApp->GetProfileInt( "","LMSPort", 80 );
	
	m_bAllowCallWaiting = pApp->GetProfileInt( "", "AllowCallWaiting", 0 );
	m_bEnableVM = pApp->GetProfileInt( "", "EnableVoiceMail", 0);

	// Arlene added : if don't use voice mail server, disable the settinga about voice mail server
	if (m_bEnableVM == false)
	{
		CWnd* pHandle = GetDlgItem(IDC_VMACCOUNT);
		if (pHandle)
			pHandle->EnableWindow(FALSE);
		pHandle = GetDlgItem(IDC_VMPASSWORD);
		if (pHandle)
			pHandle->EnableWindow(FALSE);
	}
	else
	{
		m_strVMAccount = pApp->GetProfileString( "", "VoiceMailAccount");
		m_strVMPassword = pApp->GetProfileString( "", "VoiceMailPassword");
	}
	m_curConfigMode = AfxGetApp()->GetProfileInt("","QuickConfigMode", 0);
	m_Quality = AfxGetApp()->GetProfileInt("","QuickConfigQuality", 0)-1;
	// parse Credential string
	int pos1,pos2;
	pos1 = m_strCredential.Find(":",0);
	if( pos1 != -1 )
		pos2 = m_strCredential.Find(":",pos1+1);

	if( pos1 == -1 || pos2 == -1 )
	{
		m_creRealm="";
		m_creAccount="";
		m_crePassword="";
	}
	else
	{
		m_creRealm = m_strCredential.Left(pos1);
		m_creAccount = m_strCredential.Mid(pos1+1, pos2-pos1-1);
		m_crePassword = m_strCredential.Right(m_strCredential.GetLength()-pos2-1);
	}

	int item,pos,id;
	CString temp;
	item = m_ConfigMode.AddString("None");
	m_ConfigMode.SetItemData(item,0);
	if( m_curConfigMode == 0 )
		m_ConfigMode.SetCurSel(item);

	for(int i=0;i<m_ConfigModes.GetSize(); i++)
	{
		temp = m_ConfigModes.GetAt(i);
		pos = temp.Find(",");
		item =  m_ConfigMode.AddString( temp.Right( temp.GetLength()-pos-1 ));
		id = atoi(temp.Left(pos));
		m_ConfigMode.SetItemData(item,id);
		if( id == m_curConfigMode )
			m_ConfigMode.SetCurSel(item);
	}


	UpdateData(FALSE);
	
	InitLocalIPList();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPrefDlg::OnOK()
{
	UpdateData();
	
	if( m_UserName.IsEmpty() )
	{
		AfxMessageBox("User Name is NULL");
		return;
	}

	m_LocalIPList.GetWindowText(m_strLocalIP);

	HKEY hKey;
	if ( RegCreateKeyEx( HKEY_CURRENT_USER, 
			"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
			0, 0, 0, KEY_ALL_ACCESS, 0, &hKey, 0) == 0)
	{
		char szFileName[1024];
		DWORD dwSize;
		
		if ( m_bAutoStartup)
		{
			GetModuleFileName( GetModuleHandle(NULL), szFileName, 1024);
			strcat( szFileName, " /autorun");
			dwSize = strlen(szFileName) + 1;
		}
		else
		{
			szFileName[0]=0;
			dwSize=0;
		}
		RegSetValueEx( hKey, "PCA UA", 0, REG_SZ, (BYTE*)szFileName, dwSize);

		RegCloseKey( hKey);
	}

	// update reg key
	CWinApp* pApp = AfxGetApp();
	pApp->WriteProfileInt( "", "CheckUpdates", m_bAutoCheck );
	pApp->WriteProfileInt( "", "AutoStartUp", m_bAutoStartup );
	pApp->WriteProfileInt( "", "DisableDeployServer", !m_bUseDeployServer );
	pApp->WriteProfileInt( "", "UseLMS", m_bUseLMS );

	CString s;
	s.Format("%d", m_DSPort);
	pApp->WriteProfileString( "", "DeployServerPort", s );
	pApp->WriteProfileString( "", "DeployServerAddress", m_strDSIP );

	pApp->WriteProfileInt( "", "LMSPort", m_LMSPort );
	pApp->WriteProfileString( "", "LMSAddress", m_strLMSIP );

	pApp->WriteProfileInt( "", "AllowCallWaiting", m_bAllowCallWaiting );

	pApp->WriteProfileInt( "", "EnableVoiceMail", m_bEnableVM);
	pApp->WriteProfileString( "", "VoiceMailAccount", m_strVMAccount);
	pApp->WriteProfileString( "", "VoiceMailPassword", m_strVMPassword);

	AfxGetApp()->WriteProfileInt("","QuickConfigMode", 
		m_ConfigMode.GetItemData( m_ConfigMode.GetCurSel()) );

	AfxGetApp()->WriteProfileInt("","QuickConfigQuality", m_Quality+1 );

	// update Credential string
	m_strCredential.Format("%s:%s:%s", m_creRealm, m_creAccount, m_crePassword );


	CDialog::OnOK();
}

void CPrefDlg::OnCancel()
{
	UpdateData();
	
	if( m_UserName.IsEmpty() )
	{
		AfxMessageBox("User Name is NULL");
		return;
	}

	CDialog::OnCancel();
}

void CPrefDlg::InitLocalIPList()
{
	int i,nSelect = -1;
	const char** szIPList = getAllMyAddr();
	for ( i=0; i< 16; i++) 
	{
		if (strlen(szIPList[i]) > 0)
		{
			m_LocalIPList.AddString(szIPList[i]);
			if ( m_strLocalIP == szIPList[i])
				nSelect = i;
			else
				nSelect = 0; // at least, has one IP
		}
	}
	m_LocalIPList.SetCurSel( nSelect);	

}

BOOL CPrefDlg::PreTranslateMessage(MSG* pMsg) 
{
	//TRACE0("CPrefDlg::PreTranslateMessage\n");

	if (WM_KEYFIRST <= pMsg->message && pMsg->message <= WM_KEYLAST)
	{
		// Ctrl-Alt-P?
		if( GetKeyState(VK_CONTROL)<0 && GetKeyState(VK_MENU)<0 && pMsg->wParam==80 )
		{
			TRACE0("CTRL-ALT pressed, enter safe mode!!!\n");
			m_bSafeMode=TRUE;
			EndDialog(IDCANCEL);
		}
	}
	
	return CDialog::PreTranslateMessage(pMsg);
}

void CPrefDlg::OnBtnAdvanced() 
{
	UpdateData();
	
	// update Credential string
	m_strCredential.Format("%s:%s:%s", m_creRealm, m_creAccount, m_crePassword );

	AfxGetApp()->WriteProfileString("","PreferredLocalIP",m_strLocalIP);
	CUACDlg::SetUAComRegValue("User_Info","Username",m_UserName);
	CUACDlg::SetUAComRegValue("User_Info","Display_Name",m_UserName);
	CUACDlg::SetUAComRegValue("Credential","Credential#0",m_strCredential);

	TRACE0("Click Advanced, enter safe mode!!!\n");
	m_bSafeMode=TRUE;
	EndDialog(IDCANCEL);
}

void CPrefDlg::OnUsedeploy() 
{
	UpdateData();
	if (m_bUseDeployServer == TRUE)
	{
		CWnd* pHandle = GetDlgItem(IDC_DSIP);
		if (pHandle)
			pHandle->EnableWindow(TRUE);
		pHandle = GetDlgItem(IDC_EDIT_DSPORT);
		if (pHandle)
			pHandle->EnableWindow(TRUE);
		pHandle = GetDlgItem(IDC_UPDATE_CHK);
		if (pHandle)
			pHandle->EnableWindow(TRUE);
		pHandle = GetDlgItem(IDC_AUTOSTART_CHK);
		if (pHandle)
			pHandle->EnableWindow(TRUE);
	}
	else
	{
		CWnd* pHandle = GetDlgItem(IDC_DSIP);
		if (pHandle)
			pHandle->EnableWindow(FALSE);
		pHandle = GetDlgItem(IDC_EDIT_DSPORT);
		if (pHandle)
			pHandle->EnableWindow(FALSE);
		pHandle = GetDlgItem(IDC_UPDATE_CHK);
		if (pHandle)
			pHandle->EnableWindow(FALSE);
		pHandle = GetDlgItem(IDC_AUTOSTART_CHK);
		if (pHandle)
			pHandle->EnableWindow(FALSE);
	}
}


void CPrefDlg::OnUsevm() 
{
	UpdateData();
	if (m_bEnableVM == TRUE)
	{
		CWnd* pHandle = GetDlgItem(IDC_VMACCOUNT);
		if (pHandle)
			pHandle->EnableWindow(TRUE);
		pHandle = GetDlgItem(IDC_VMPASSWORD);
		if (pHandle)
			pHandle->EnableWindow(TRUE);
	}	
	else
	{
		CWnd* pHandle = GetDlgItem(IDC_VMACCOUNT);
		if (pHandle)
			pHandle->EnableWindow(FALSE);
		pHandle = GetDlgItem(IDC_VMPASSWORD);
		if (pHandle)
			pHandle->EnableWindow(FALSE);
	}
}

void CPrefDlg::OnUselms() 
{
	UpdateData();
	if (m_bUseLMS == TRUE)
	{
		CWnd* pHandle = GetDlgItem(IDC_LMSIP);
		if (pHandle)
			pHandle->EnableWindow(TRUE);
		pHandle = GetDlgItem(IDC_LMSPORT);
		if (pHandle)
			pHandle->EnableWindow(TRUE);
	} else {
		CWnd* pHandle = GetDlgItem(IDC_LMSIP);
		if (pHandle)
			pHandle->EnableWindow(FALSE);
		pHandle = GetDlgItem(IDC_LMSPORT);
		if (pHandle)
			pHandle->EnableWindow(FALSE);
	}
	
}
