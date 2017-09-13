// DlgJoinMessage.cpp : implementation file
//

#include "stdafx.h"
#include "pcaua.h"
#include "DlgJoinMessage.h"

#include "PCAUADlg.h"
#include "PanelDlg.h"

#ifdef	_PCAUA_Res_
#include "PCAUARes.h"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgJoinMessage dialog


CDlgJoinMessage::CDlgJoinMessage(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgJoinMessage::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgJoinMessage)
	m_strURI = _T("");
	//}}AFX_DATA_INIT
}


void CDlgJoinMessage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgJoinMessage)
	DDX_Control(pDX, IDC_ADDRBOOK, m_AddrBook);
	DDX_Text(pDX, IDC_URI, m_strURI);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgJoinMessage, CDialog)
	//{{AFX_MSG_MAP(CDlgJoinMessage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgJoinMessage message handlers

BOOL CDlgJoinMessage::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// init list view control
	m_AddrBook.SetExtendedStyle( LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES );
#ifndef _XCAPSERVER
	m_AddrBook.SetHeadings( "Name,70;Presence,55;Call State,60;TEL,120;Company,70;Remark,70");
#else
	m_AddrBook.SetHeadings( "Name,70;Group,60; Presence,60;Call State,60;TEL,100;Company,60");
#endif
	m_AddrBook.LoadColumnInfo();

	//load image list for PhoneBook
	CBitmap bmOnline, bmUnknown;
#ifdef	_PCAUA_Res_
	LoadBitmap_(&bmOnline, IDB_PRE_ONLINE);
	LoadBitmap_(&bmUnknown, IDB_PRE_UNKNOWN);
	CreateImageList_(&m_PresenceImg, IDB_PRE_OFFLINE, 16, 1, RGB(255, 255, 255) );
#else
	bmOnline.LoadBitmap(IDB_PRE_ONLINE);
	bmUnknown.LoadBitmap(IDB_PRE_UNKNOWN);
	m_PresenceImg.Create(IDB_PRE_OFFLINE, 16, 1, RGB(255, 255, 255) );
#endif
	
	m_PresenceImg.Add(&bmOnline, RGB(255, 255, 255));
	m_PresenceImg.Add(&bmUnknown, RGB(255, 255, 255));
	m_AddrBook.SetImageList(&m_PresenceImg, LVSIL_SMALL);

	// load personal address book items
	CSortListCtrl& list = g_pdlgPCAExtend->m_ListPersonal;
	int nCount = list.GetItemCount();
	for ( int i=0; i<nCount; i++)
	{
		LVITEM item;
		item.iItem = i;
		item.iSubItem = 0;
		item.mask = LVIF_IMAGE;
		list.GetItem( &item);
		m_AddrBook.AddItem( item.iImage, list.GetItemText(i,0), list.GetItemText(i,1), list.GetItemText(i,2), list.GetItemText(i,3), list.GetItemText(i,4), list.GetItemText(i,5)  );
	}

	// check the 'select from address book' button
	((CButton*)GetDlgItem(IDC_FROM_ADDRBOOK))->SetCheck(1);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgJoinMessage::OnOK() 
{
	UpdateData();

	if ( ((CButton*)GetDlgItem(IDC_FROM_ADDRBOOK))->GetCheck())
	{
		// select from address book
		POSITION pos = m_AddrBook.GetFirstSelectedItemPosition();
		while ( pos)
		{
			int nItem = m_AddrBook.GetNextSelectedItem(pos);
			
#ifndef _XCAPSERVER
			if (g_pMainDlg->GetBuddyPresence(m_AddrBook.GetItemText(nItem,3)))	// need to be on-line
				m_vecTarget.Add( m_AddrBook.GetItemText(nItem,3) );
#else
			if (g_pMainDlg->GetBuddyPresence(m_AddrBook.GetItemText(nItem,4)))	// need to be on-line
				m_vecTarget.Add( m_AddrBook.GetItemText(nItem,4) );
#endif
		}
	}
	else if (!m_strURI.IsEmpty())	// check if uri is empty, modified by shen
	{
		// use input
			m_vecTarget.Add( m_strURI);
	}
	
	if (m_vecTarget.GetSize() <= 0)
	{
		MessageBox( " There is no item properly selected.\n Please choice (more than) one online. ", "Message", MB_OK);
		return;
	}
	CDialog::OnOK();
}
