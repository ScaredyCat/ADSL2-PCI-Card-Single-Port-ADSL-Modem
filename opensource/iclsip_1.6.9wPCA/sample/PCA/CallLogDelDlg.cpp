// CallLogDelDlg.cpp : implementation file
//

#include "stdafx.h"
#include "pcaua.h"
#include "CallLogDelDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCallLogDelDlg dialog


CCallLogDelDlg::CCallLogDelDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCallLogDelDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCallLogDelDlg)
	m_RADIO_CallLogDel = -1;
	m_DATETIMEPICKER_CallLogDel = COleDateTime::GetCurrentTime();
	//}}AFX_DATA_INIT
	m_RADIO_CallLogDel=0;
}


void CCallLogDelDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCallLogDelDlg)
	DDX_Control(pDX, IDC_COMBO_CallLogDel, m_COMBO_CallLogDel);
	DDX_Radio(pDX, IDC_RADIO1, m_RADIO_CallLogDel);
	DDX_DateTimeCtrl(pDX, IDC_DATETIMEPICKER_CallLogDel, m_DATETIMEPICKER_CallLogDel);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCallLogDelDlg, CDialog)
	//{{AFX_MSG_MAP(CCallLogDelDlg)
	ON_BN_CLICKED(IDC_RADIO1, OnRadio1)
	ON_BN_CLICKED(IDC_RADIO2, OnRadio2)
	ON_BN_CLICKED(IDC_RADIO3, OnRadio3)
	ON_BN_CLICKED(IDC_RADIO4, OnRadio4)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCallLogDelDlg message handlers

BOOL CCallLogDelDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	m_COMBO_CallLogDel.SetCurSel(0);
	
	*pintSelect=0;	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CCallLogDelDlg::OnRadio1() 
{
	// TODO: Add your control notification handler code here
	CWnd*      pType = (CWnd*)GetDlgItem(IDC_COMBO_CallLogDel);
	pType->EnableWindow(TRUE);
	pType=(CWnd*)GetDlgItem(IDC_DATETIMEPICKER_CallLogDel);
	pType->EnableWindow(FALSE);
	*pintSelect=0;	
}

void CCallLogDelDlg::OnRadio2() 
{
	// TODO: Add your control notification handler code here
	CWnd*      pType = (CWnd*)GetDlgItem(IDC_COMBO_CallLogDel);
	pType->EnableWindow(FALSE);
	pType=(CWnd*)GetDlgItem(IDC_DATETIMEPICKER_CallLogDel);
	pType->EnableWindow(TRUE);
	*pintSelect=1;	
}

void CCallLogDelDlg::OnOK() 
{
	// TODO: Add extra validation here
	UpdateData(TRUE);

	if (*pintSelect==0)
	{
		int intComboItem;
		COleDateTime timeNow;
		timeNow=COleDateTime::GetCurrentTime();
		CString str = timeNow.Format(_T("%A, %B %d, %Y"));
		
	
		
		intComboItem=m_COMBO_CallLogDel.GetCurSel();
		switch (intComboItem)
		{
		case 0: timeNow-=10;
			break;
		case 1: timeNow-=20;
			break;
		case 2: timeNow-=30;
			break;
		case 3: timeNow-=60;
			break;
		case 4: timeNow-=90;
			break;
		default: timeNow-=3650;
			break;
		}

	    *pdateTime=timeNow;
	}
	else	
		*pdateTime=m_DATETIMEPICKER_CallLogDel;
	
	
		
	CDialog::OnOK();
}

void CCallLogDelDlg::OnRadio3() 
{
	// TODO: Add your control notification handler code here
	CWnd*      pType = (CWnd*)GetDlgItem(IDC_COMBO_CallLogDel);
	pType->EnableWindow(FALSE);
	pType=(CWnd*)GetDlgItem(IDC_DATETIMEPICKER_CallLogDel);
	pType->EnableWindow(FALSE);
	*pintSelect=2;		
}

void CCallLogDelDlg::OnRadio4() 
{
	// TODO: Add your control notification handler code here
	CWnd*      pType = (CWnd*)GetDlgItem(IDC_COMBO_CallLogDel);
	pType->EnableWindow(FALSE);
	pType=(CWnd*)GetDlgItem(IDC_DATETIMEPICKER_CallLogDel);
	pType->EnableWindow(FALSE);
	*pintSelect=3;		
	
}
