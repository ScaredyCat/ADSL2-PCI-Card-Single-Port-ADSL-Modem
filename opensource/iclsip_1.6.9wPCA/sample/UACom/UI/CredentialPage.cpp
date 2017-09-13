// CredentialPage.cpp : implementation file
//

#include "CredentialPage.h"
#include "CredentialEditDlg.h"
#include "ConfirmDeleteDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// CCredentialPage property page

IMPLEMENT_DYNCREATE(CCredentialPage, CPropertyPage)

CCredentialPage::CCredentialPage() : CPropertyPage(CCredentialPage::IDD)
{
	//{{AFX_DATA_INIT(CCredentialPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	pProfile = CUAProfile::GetProfile();
}

CCredentialPage::~CCredentialPage()
{
}

void CCredentialPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCredentialPage)
	DDX_Control(pDX, IDC_CREDENTIALLIST, m_CredentialListCTL);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCredentialPage, CPropertyPage)
	//{{AFX_MSG_MAP(CCredentialPage)
	ON_NOTIFY(NM_DBLCLK, IDC_CREDENTIALLIST, OnClickCredentiallist)
	ON_NOTIFY(LVN_KEYDOWN, IDC_CREDENTIALLIST, OnKeydownCredentiallist)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCredentialPage message handlers

BOOL CCredentialPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	m_CredentialListCTL.SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0 , 
		                LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES |LVS_EX_HEADERDRAGDROP  |LVS_EX_UNDERLINEHOT);

	m_CredentialListCTL.InsertColumn( 1, "NO.", LVCFMT_LEFT, 40, 1 );
	m_CredentialListCTL.InsertColumn( 2, "REALM", LVCFMT_LEFT, 160, 2 );
	m_CredentialListCTL.InsertColumn( 3, "ACCOUNT ID", LVCFMT_LEFT, 160, 3 );
	//m_CredentialListCTL.InsertColumn( 4, "PASSWORD", LVCFMT_LEFT, 100, 4 );
	
	UpdateItems();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CCredentialPage::UpdateItems()
{
	CString strText;
	int tmp;
	int j=0;

	/*when update items, clear all first*/
	m_CredentialListCTL.DeleteAllItems();

	// Insert items in the list view control.
	for (int i=0;i < pProfile->m_CredentialCount;i++)
	{
		strText.Format(TEXT("%d"), i+1);

		m_CredentialListCTL.InsertItem(
		LVIF_TEXT|LVIF_STATE, i, strText, 0, LVIS_SELECTED, 0, 0);

		for (; j < 10; j++) {
			strText = pProfile->m_Credential[j];
			if (!strText.IsEmpty()) {
				pProfile->m_Credential[j].Empty();
				pProfile->m_Credential[i] = strText;
				j++;
				break;
			}
		}
			
		tmp = strText.Find(":");
		m_CredentialListCTL.SetItemText(i, 1, strText.Left(tmp) );
		
		strText = strText.Right( strText.GetLength()-tmp-1);
		tmp = strText.Find(":");
		m_CredentialListCTL.SetItemText(i, 2, strText.Left(tmp) );
		
		/*
		strText = strText.Right( strText.GetLength()-tmp-1 );
		m_CredentialListCTL.SetItemText(i, 3, strText );
		 */
	}
}

void CCredentialPage::OnClickCredentiallist(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// Get the selected items in the control
	POSITION p = m_CredentialListCTL.GetFirstSelectedItemPosition();
	int nSelected =0;

	/*make sure position is selected */
	if(p!=NULL){
		nSelected=m_CredentialListCTL.GetNextSelectedItem(p);
	}else{
		/*if no select a row, means to add a new item*/
		nSelected=m_CredentialListCTL.GetItemCount();
	}

	if (nSelected == 10) {
		AfxMessageBox("Maximum credential number reached!");
		return;
	}

	CString strText = pProfile->m_Credential[nSelected];
	int tmp = strText.Find(":");
	strRealmName = strText.Left(tmp);
		
	strText = strText.Right( strText.GetLength()-tmp-1);
	tmp = strText.Find(":");
	strUserID = strText.Left(tmp);
		
	strPasswd = strText.Right( strText.GetLength()-tmp-1 );

	/*
	strRealmName = m_CredentialListCTL.GetItemText(nSelected, 1);
	strUserID = m_CredentialListCTL.GetItemText(nSelected, 2);
	strPasswd = m_CredentialListCTL.GetItemText(nSelected, 3);
	 */
	
	CCredentialEditDlg dlg;
	int nRet = dlg.DoModal();
	
	// Handle the return value from DoModal
	/* suggest to add a delete button*/
	switch ( nRet )
	{
	case -1: 
		AfxMessageBox("Dialog box could not be created!");
		break;
	
	case IDOK:
		m_CredentialListCTL.SetItemText(nSelected, 1, strRealmName );
		m_CredentialListCTL.SetItemText(nSelected, 2, strUserID );
		m_CredentialListCTL.SetItemText(nSelected, 3, strPasswd );
		pProfile->m_Credential[nSelected].Format("%s:%s:%s",
							strRealmName,
							strUserID,
							strPasswd);
		/*make sure credential count increase when add a new item*/
		if(nSelected == (pProfile->m_CredentialCount))
			pProfile->m_CredentialCount++;
		
		/*update new item into ListCtrl*/
		pProfile->Write();
		UpdateItems();
		break;

	case IDCANCEL:
		break;
	
	default:
		break;
	};

	*pResult = 0;
}

void CCredentialPage::OnKeydownCredentiallist(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_KEYDOWN* pLVKeyDow = (LV_KEYDOWN*)pNMHDR;
	
	// Get the selected items in the control
	POSITION p = m_CredentialListCTL.GetFirstSelectedItemPosition();
	int nSelected =0;

	/*make sure position is selected */
	if(p!=NULL){
		nSelected=m_CredentialListCTL.GetNextSelectedItem(p);
	}else{
		return;
	}

	if (pLVKeyDow->wVKey == VK_DELETE) {

		CConfirmDeleteDlg dlg;
		int nRet = dlg.DoModal();
	
		// Handle the return value from DoModal
		/* suggest to add a delete button*/
		switch ( nRet )
		{
		case -1: 
			AfxMessageBox("Dialog box could not be created!");
			break;
	
		case IDOK:
			pProfile->m_Credential[nSelected].Empty();
			pProfile->m_CredentialCount--;
			UpdateItems();
			break;

		case IDCANCEL:
			break;
	
		default:
			break;
		};
	}
	*pResult = 0;
}
