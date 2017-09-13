// IPPrefDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PCAUA.h"
#include "IPPrefDlg.h"
#include "UACDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const char** CIPPrefDlg::getAllMyAddr()
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

/////////////////////////////////////////////////////////////////////////////
// CPrefDlg dialog


CIPPrefDlg::CIPPrefDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CIPPrefDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CIPPrefDlg)
	//}}AFX_DATA_INIT
}


void CIPPrefDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPrefDlg)

	DDX_Control(pDX, IDC_LOCALIPLIST, m_LocalIPList);


	//}}AFX_DATA_MAP

}


BEGIN_MESSAGE_MAP(CIPPrefDlg, CDialog)
	//{{AFX_MSG_MAP(CPrefDlg)

	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPrefDlg message handlers

BOOL CIPPrefDlg::OnInitDialog() 
{
	CWnd* pHandle;

	CDialog::OnInitDialog();
	
	CWinApp* pApp = AfxGetApp();

	UpdateData(FALSE);
	
	InitLocalIPList();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CIPPrefDlg::OnOK()
{
	UpdateData();
	
	m_LocalIPList.GetWindowText(m_strLocalIP);

	CDialog::OnOK();
}

void CIPPrefDlg::OnCancel()
{
	UpdateData();
	
	CDialog::OnCancel();
}

void CIPPrefDlg::InitLocalIPList()
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

BOOL CIPPrefDlg::PreTranslateMessage(MSG* pMsg) 
{
	//TRACE0("CPrefDlg::PreTranslateMessage\n");

	if (WM_KEYFIRST <= pMsg->message && pMsg->message <= WM_KEYLAST)
	{
		// Ctrl-Alt-P?
		if( GetKeyState(VK_CONTROL)<0 && GetKeyState(VK_MENU)<0 && pMsg->wParam==80 )
		{
			TRACE0("CTRL-ALT pressed, enter safe mode!!!\n");
			
			EndDialog(IDCANCEL);
		}
	}
	
	return CDialog::PreTranslateMessage(pMsg);
}

