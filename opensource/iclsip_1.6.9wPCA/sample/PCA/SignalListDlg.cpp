// ProgressListDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PCAUA.h"
#include "PCAUADlg.h"
#include "SignalListDlg.h"
#include "BSOperation.h"
//#include <IPTypes.h>
//#include <IPHlpApi.h>
#include "netadapter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CProgressListDlg dialog
CProgressListDlg::CProgressListDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CProgressListDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CProgressListDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS, m_ProgressList);
	DDX_Control(pDX, IDC_ADAPTERSELCOMBO, m_AdapterSelCTL);
	
	DDX_Text( pDX, IDC_STATIC_IPADDR, sIP );
	DDX_Text( pDX, IDC_STATIC_IPTYPE, sType );
	DDX_Text( pDX, IDC_STATIC_SUBNET, sSub );
	DDX_Text( pDX, IDC_STATIC_GATEWAY, sGate );
	DDX_Text( pDX, IDC_STATIC_DNSONE, sDns1 );
	DDX_Text( pDX, IDC_STATIC_DNSTWO, sDns2 );	
}

BEGIN_MESSAGE_MAP(CProgressListDlg, CDialog)
	ON_WM_SHOWWINDOW()
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_NOTIFY(NM_CLICK, IDC_PROGRESS, OnClickSignallist)
	ON_NOTIFY(LVN_KEYDOWN, IDC_PROGRESS, OnKeydownSignallist)
	//}}AFX_MSG_MAP
	ON_WM_TIMER()
	ON_CBN_SELCHANGE(IDC_ADAPTERSELCOMBO, OnSelchangeAdapterselcombo)
	ON_BN_CLICKED(IDC_SETDEFAULT, OnBtnSetDefault)
	ON_BN_CLICKED(IDC_CHANGEDEF, OnBtnChangeDefault)
END_MESSAGE_MAP()


// CProgressListDlg message handlers

BOOL CProgressListDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	//! first param - assign col 2 for progress
	//! second param - assign progress strength according col 1
	m_ProgressList.InitProgressColumn(2,2);

	m_ProgressList.InsertColumn(0,"SSID",LVCFMT_LEFT,100);
	m_ProgressList.InsertColumn(1,"Strengh",LVCFMT_LEFT,100);
	m_ProgressList.InsertColumn(2,"Progress",LVCFMT_LEFT,100);
	m_ProgressList.InsertColumn(3,"MAC",LVCFMT_LEFT,100);
	m_ProgressList.InsertColumn(4,"Refresh",LVCFMT_LEFT,100);

	m_SelectItem = "";
	m_SSID = "";
	m_bCheck = FALSE;
	m_bAssociateOK = FALSE;
	SetTimer(100,1000,NULL);
	
	//! Network adapter
	{
		CNetworkAdapter* pAdapt = NULL;
		m_pAdapters	= NULL;
		AdapterInit();

		if( m_pAdapters ) {	
			
			for(int i=0;i<m_nCount;i++)
			{
				pAdapt = &m_pAdapters[ i ];
				CString tmp;
				tmp.Format("%s",pAdapt->GetAdapterDescription().c_str());
				m_AdapterSelCTL.AddString(tmp);
			}	
		}

		AdapterClean();

		m_AdapterSelCTL.SetCurSel( 0 );
		OnSelchangeAdapterselcombo();
	}
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CProgressListDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		//CAboutDlg dlgAbout;
		//dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CProgressListDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CProgressListDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CProgressListDlg::OnShowWindow(BOOL bShow,UINT nStatus)
{
	if( bShow )
	{
		//! Network adapter
		{
			CNetworkAdapter* pAdapt = NULL;
			m_pAdapters	= NULL;
			AdapterInit();

			if( m_pAdapters ) {	
			
				for(int i=0;i<m_nCount;i++)
				{
					pAdapt = &m_pAdapters[ i ];
					CString tmp;
					tmp.Format("%s",pAdapt->GetAdapterDescription().c_str());
					//m_AdapterSelCTL.AddString(tmp);

					if( tmp==GetDefaultNIC() )
					{
						m_AdapterSelCTL.SetCurSel( i );
						break;
					}
				}	
			}
			AdapterClean();
		}
	}
}

void CProgressListDlg::OnTimer(UINT nIDEvent)
{
	static int count=0;
	// TODO: Add your message handler code here and/or call default
	if(nIDEvent==100)
	{
		if( GetCheckOK() )
		{
			CheckOK();
		}
	}
	CDialog::OnTimer(nIDEvent);
}

void CProgressListDlg::OnClickSignallist(NMHDR* pNMHDR, LRESULT* pResult)
{
	// select from m_ProgressList
	POSITION pos = m_ProgressList.GetFirstSelectedItemPosition();
	if ( pos)
	{
		int item = m_ProgressList.GetNextSelectedItem(pos);
		m_SelectItem = m_ProgressList.GetItemText(item,0);
	}
}
void CProgressListDlg::OnKeydownSignallist(NMHDR* pNMHDR, LRESULT* pResult)
{
	// select from m_ProgressList
	POSITION pos = m_ProgressList.GetFirstSelectedItemPosition();
	if ( pos)
	{
		int item = m_ProgressList.GetNextSelectedItem(pos);
		m_SelectItem = m_ProgressList.GetItemText(item,0);
	}
}

/*
 *  Associate to BS, and if success, connect to DHCP.
 */

BOOL AssociatedToBS(void *pParam)
{
	bool bRet = false;
	DWORD Err;
	PIP_ADAPTER_INFO pAdapterInfo, pAdapt;
    DWORD AdapterInfoSize;
	CString csTmp;
	UINT nIndex = 0;
 
	CProgressListDlg *dlg = (CProgressListDlg*)pParam;

	//cmdAdapter.Clear();
    //
    // Enumerate all of the adapter specific information using the IP_ADAPTER_INFO structure.
    // Note:  IP_ADAPTER_INFO contains a linked list of adapter entries.
    //
    AdapterInfoSize = 0;
    if ((Err = GetAdaptersInfo(NULL, &AdapterInfoSize)) != 0)
    {
        if (Err != ERROR_BUFFER_OVERFLOW)
        {            
            return false;
        }
    }

    // Allocate memory from sizing information
    if ((pAdapterInfo = (PIP_ADAPTER_INFO) GlobalAlloc(GPTR, AdapterInfoSize)) == NULL)
    {        
        return false;
    }

    // Get actual adapter information
    if ((Err = GetAdaptersInfo(pAdapterInfo, &AdapterInfoSize)) != 0)
    {        
        return false;
    }

    pAdapt = pAdapterInfo;

	CString tmp;
    while (pAdapt)
    {                
	
		//	int total = dlg->m_ProgressList.GetItemCount();
		//	dlg->m_ProgressList.InsertItem(total,pAdapt->Description);
		//	dlg->m_ProgressList.SetItemText(total,1,pAdapt->IpAddressList.IpAddress.String);
		
		tmp.Format("%s",pAdapt->Description);
		if( tmp.Find("wireless") ) // i don't know if every wireless interface has this key description.
		{
			
			CString tmp1;
			tmp1.Format("Connecting to DHCP!!");
			dlg->GetDlgItem(IDC_HINT)->SetWindowText(tmp1);

			tmp.Format("%s",pAdapt->IpAddressList.IpAddress.String);
			int count=0;
			while( tmp=="0.0.0.0" && count<10 )
			{
				count++;
				Sleep(1000);
			}
			if( count>=10 ) return FALSE;

			tmp.Format("Do you want to change IP to %s ?",pAdapt->IpAddressList.IpAddress.String);
			if (MessageBox(dlg->GetSafeHwnd(), tmp,
				"PCA Confirmation",
				MB_ICONQUESTION | MB_YESNO) != IDYES)
				return FALSE; 
			else
			{
				
				// ToDo: re-initial PCA and choose wireless IP Address
				g_pMainDlg->SetChangeIP(pAdapt->IpAddressList.IpAddress.String);
				g_pMainDlg->ChangeToNewIP();
				return TRUE;
			}
		}
		

		nIndex++;
		pAdapt = pAdapt->Next;
    }

	return FALSE;
}





void CProgressListDlg::OnOK()
{
	GetWindowText(m_Hint);
	UpdateData();

	// select from m_ProgressList
	POSITION pos = m_ProgressList.GetFirstSelectedItemPosition();
	if ( pos)
	{
		int nItem = m_ProgressList.GetNextSelectedItem(pos);
		
		//associate to selected BS
		m_SSID = m_ProgressList.GetItemText(nItem,0);
		//AfxMessageBox(m_SSID);
		
		m_HintAssociate.Format("Associate %s",m_SSID);



		GetDlgItem(IDOK)->EnableWindow(FALSE);
		GetDlgItem(IDC_PROGRESS)->EnableWindow(FALSE);
		GetDlgItem(IDCANCEL)->EnableWindow(FALSE);
		
		//SetWindowText(m_HintAssociate);
	}

}

void CProgressListDlg::CheckRefresh()
{
	int total = m_ProgressList.GetItemCount();

	
	for(int i=0;i</*total*/5;i++)
	{
		CString refresh	= m_ProgressList.GetItemText(i,4);
		if( refresh=="0" )
		{
			// remove it
			m_ProgressList.DeleteItem(i);
		}
	}
}

void CProgressListDlg::ClearRefresh()
{
	int total = m_ProgressList.GetItemCount();

	for(int i=0;i</*total*/5;i++)
	{
		m_ProgressList.SetItemText(i,4,"0");
	}
}

void CProgressListDlg::UpdateBS(CString mac,CString ssid,int ssi,int idx)
{
	int total = m_ProgressList.GetItemCount();


	for(int i=0;i<total;i++)
	{
		if( mac==m_ProgressList.GetItemText(i,3) )
		{
			CString temp,temp1;
			temp.Format("%d",ssi);
				
				if( ssi<0 )
				{
					temp1.Format("%d",120+ssi);
				}
				else if( ssi>0 )
				{
					temp1.Format("%d",ssi);
				}
	
				
				m_ProgressList.SetItemText(i,1,temp);
				m_ProgressList.SetItemText(i,2,temp1);
				m_ProgressList.SetItemText(i,4,"1");
				
				return;
		}
	}

	// no exit data, create one.
	InsertBS( mac, ssid, ssi, idx);
	
}

void CProgressListDlg::InsertBS(CString mac,CString ssid,int ssi,int idx)
{
	int total = m_ProgressList.GetItemCount();
	m_ProgressList.InsertItem(total,ssid);

	CString temp,temp1;
	temp.Format("%d",ssi);
	//temp1.Empty();
	if( ssi<0 )
	{
		temp1.Format("%d",120+ssi);
	}
	else if( ssi>0 )
	{
		temp1.Format("%d",ssi);
	}

	m_ProgressList.SetItemText(total,1,temp);
	m_ProgressList.SetItemText(total,2,temp1);
	m_ProgressList.SetItemText(total,3,mac);
	//m_ProgressList.SetItemText(total,4,"0");
}

void CProgressListDlg::CheckOK()
{
	//KillTimer(100);
	//m_SelectItem = "";
	//m_SSID = "";
	m_bCheck = FALSE;
	m_bAssociateOK = FALSE;

	
	GetDlgItem(IDOK)->EnableWindow(TRUE);
	GetDlgItem(IDC_PROGRESS)->EnableWindow(TRUE);
	GetDlgItem(IDCANCEL)->EnableWindow(TRUE);
	SetWindowText(m_Hint);

	if( m_bAssociateOK )
		CDialog::OnOK();
	else
		SetWindowText(m_Hint);
}

/////////////////////////////////////////////////////////
//	Desc:
//		Used to build the list of adapters.  Must be 
//		called again after each call that modifies 
//		the adapters information.  Currently this includes
//		the renewing and releasing of adapter information.
/////////////////////////////////////////////////////////
BOOL CProgressListDlg::AdapterInit() {
	BOOL	bRet		= TRUE;
	DWORD	dwErr		= 0;
	ULONG	ulNeeded	= 0;

	dwErr = EnumNetworkAdapters( m_pAdapters, 0, &ulNeeded );
	if( dwErr == ERROR_INSUFFICIENT_BUFFER ) {		
		m_nCount	= ulNeeded / sizeof( CNetworkAdapter );
		m_pAdapters = new CNetworkAdapter[ ulNeeded / sizeof( CNetworkAdapter ) ];		
		dwErr		= EnumNetworkAdapters( m_pAdapters, ulNeeded, &ulNeeded );		
		if( ! m_pAdapters ) {
			AfxMessageBox( _T("No Network Adapters Found on System."), MB_OK | MB_ICONERROR );
			::SendMessage( m_hWnd, WM_CLOSE, 0, 0 );
		}		
	}else{
		AfxMessageBox( _T("No Network Adapters Found on System."), MB_OK | MB_ICONERROR );
		::SendMessage( m_hWnd, WM_CLOSE, 0, 0 );
		bRet = FALSE;
	}

	return bRet;
}

void CProgressListDlg::AdapterClean()
{
	delete [] m_pAdapters;
}

void CProgressListDlg::OnBtnChangeDefault()
{
	g_pMainDlg->SetChangeIP(GetDefaultNIC());
	g_pMainDlg->ChangeToNewIP();
}

void CProgressListDlg::OnBtnSetDefault()
{
	SetDefaultNIC(sIP);
}

void CProgressListDlg::OnSelchangeAdapterselcombo()
{
	int ret = m_AdapterSelCTL.GetCurSel();
	
	m_pAdapters	= NULL;

	AdapterInit();

	CNetworkAdapter *pAdapt=NULL;
	pAdapt = &m_pAdapters[ ret ];
	
	( pAdapt->IsDhcpUsed() ) ? sType = _T("Dynamic Address (DHCP)") : sType = _T("Static IP");
	sIP = pAdapt->GetIpAddr().c_str();
	sDesc = pAdapt->GetAdapterDescription().c_str();
	sSub = pAdapt->GetSubnetForIpAddr().c_str();
	sGate = pAdapt->GetGatewayAddr().c_str();
	sDns1 = pAdapt->GetDnsAddr( 0 ).c_str();
	sDns2 = pAdapt->GetDnsAddr( 1 ).c_str();

	AdapterClean();
	
	UpdateData( FALSE );

	SetDefaultNIC(sDesc);
	
}

void CProgressListDlg::SetDefaultNIC(CString nic)
{
	m_strDefaultNIC = nic;

	// ToDo: save default NIC.
	// ... depend on your design
}

CString CProgressListDlg::GetDefaultNIC()
{
	// ToDo: get default NIC.
	// ... depend on your design

	return m_strDefaultNIC;
}
