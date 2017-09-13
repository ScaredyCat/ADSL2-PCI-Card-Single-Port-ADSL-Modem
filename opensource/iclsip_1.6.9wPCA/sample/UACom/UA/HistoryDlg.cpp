
/*
#include "stdafx.h"
#include "PCAUA.h"
#include "LoginDlg.h"
#include "PCAUADlg.h"
*/

#include "stdafx.h"
//#include "..\PCAUA.h"
#include "UAProfile.h"
#include "HistoryDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CListCtrlDemoDlg dialog

CHistoryDlg::CHistoryDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CHistoryDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CHistoryDlg)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	//m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

}

void CHistoryDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CListCtrlDemoDlg)
	DDX_Control(pDX, IDC_LOG, m_Log);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CHistoryDlg, CDialog)
	//{{AFX_MSG_MAP(CHistoryDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_CLEAR, OnClearLog)
	//}}AFX_MSG_MAP

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHistoryDlg message handlers

BOOL CHistoryDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
/*		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
*/
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	


	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CHistoryDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	/*
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		//CAboutDlg dlgAbout;
		//dlgAbout.DoModal();
	}
	else
	{
		
	}
	*/
	CDialog::OnSysCommand(nID, lParam);
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CHistoryDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

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

/////////////////////////////////////////////////////////////////////////////
/*

#include <time.h>
#include <stdio.h>

void main( void )
{
   struct tm when;
   time_t now, result;
   int    days;

   time( &now );
   when = *localtime( &now );
   printf( "Current time is %s\n", asctime( &when ) );
   printf( "How many days to look ahead: " );
   scanf( "%d", &days );

   when.tm_mday = when.tm_mday + days;
   if( (result = mktime( &when )) != (time_t)-1 )
      printf( "In %d days the time will be %s\n", 
              days, asctime( &when ) );
   else
      perror( "mktime failed" );
}
Output

Current time is Tue May 03 12:45:47 1994

How many days to look ahead: 29
In 29 days the time will be Wed Jun 01 12:45:47 1994

*/
void CHistoryDlg::OnClearLog() 
{
	// TODO: Add your control notification handler code here
/*	static char buf1[128]={'\0'};
	FILE *ptr;
	CString txt;
	
	struct tm when;
	time_t now, result;
	
	time( &now );
	when = *localtime( &now );
	sprintf(buf1,"%s", asctime( &when ) );

	m_Log.GetWindowText(txt);
	ptr = fopen("C:\\PCAUA_Log.txt","a+");
	fwrite(buf1,sizeof(char)*strlen(buf1),1,ptr);
	fwrite(txt,txt.GetLength(),1,ptr);
	fclose(ptr);
*/
	m_Log.SetWindowText(_T(""));
}

void CHistoryDlg::AppendString(CString str)
{
	m_Log.AppendString(str);
}

void CHistoryDlg::OnOK()
{
	
}

void CHistoryDlg::OnCancel()
{
	//EndDialog(IDOK);
}