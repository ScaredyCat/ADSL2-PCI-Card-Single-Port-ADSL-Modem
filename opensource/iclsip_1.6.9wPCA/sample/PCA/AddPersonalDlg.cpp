// CAddPersonalDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PCAUA.h"
#include "AddPersonalDlg.h"

#ifdef _XCAPSERVER
#include "xcap_datatype.h"
#include "xcapapi.h"
#include "UACDlg.h"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddPersonalDlg dialog


CAddPersonalDlg::CAddPersonalDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAddPersonalDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAddPersonalDlg)
	m_COMPANY = _T("");
	m_NAME = _T("");
	m_PIC = _T("");
	m_REMARK = _T("");
	m_TEL = _T("");
	m_GROUP = _T("");
	//}}AFX_DATA_INIT
}


void CAddPersonalDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddPersonalDlg)
	DDX_Text(pDX, IDC_PERSONAL_ADD_COMPANY, m_COMPANY);
	DDX_Text(pDX, IDC_PERSONAL_ADD_NAME, m_NAME);
	DDX_Text(pDX, IDC_PERSONAL_ADD_PIC, m_PIC);
	DDX_Text(pDX, IDC_PERSONAL_ADD_REMARK, m_REMARK);
	DDX_Text(pDX, IDC_PERSONAL_ADD_TEL, m_TEL);
	DDX_CBString(pDX, IDC_PERSONAL_ADD_GROUP, m_GROUP);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddPersonalDlg, CDialog)
	//{{AFX_MSG_MAP(CAddPersonalDlg)
	ON_BN_CLICKED(IDC_PERSONAL_ADD_OPENFILE, OnPersonalAddOpenfile)
	ON_BN_CLICKED(IDOK,OnPersonalOK)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddPersonalDlg message handlers

void CAddPersonalDlg::OnPersonalAddOpenfile() 
{
	// TODO: Add your control notification handler code here
	UpdateData(true);

	CString strFilter = "Picture Files(.bmp;.dib;.emf;.gif;.ico;.jpg;.wmf)|*.bmp;*.dib;*.emf;*.gif;*.ico;*.jpg;*.wmf|BMP (*.bmp)|*.bmp|DIB (*.dib)|*.dib|EMF (*.emf)|*.emf|GIF (*.gif)|*.gif|ICO (*.ico)|*.ico|JPG (*.jpg)|*.jpg|WMF (*.wmf)|*.wmf|All Files (*.*)|*.*||";
	CFileDialog OpenFileDialog(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, strFilter);

	OpenFileDialog.m_ofn.lpstrTitle = "Open a picture file";
	
	if(OpenFileDialog.DoModal() == IDOK)
		{
		CString m_FilePathName;
		m_FilePathName.Format("%s", OpenFileDialog.m_ofn.lpstrFile);
		m_PIC=m_FilePathName;
		CWnd*      pSEARCH = (CWnd*)GetDlgItem(IDC_PERSONAL_ADD_PIC);
		

		CRect rct;
		GetDlgItem(IDC_PERSONAL_ADD_PIC)->GetWindowRect(&rct);
		ScreenToClient(&rct);
		CDC *dc=this->GetDC();
		m_Picture.Load(m_FilePathName);
		m_Picture.UpdateSizeOnDC(dc);
	
		m_Picture.Show(dc,CRect(rct.left,rct.top,rct.left+80,rct.top+80));
		//int m_leng=m_FilePathName.GetLength();
		//int m_ext=m_FilePathName.Find('.');
		//CString tt=m_FilePathName.Mid(m_ext,m_leng-m_ext);
	
		//CopyFile(m_FILEINFO,"C:\\xyz1"+tt,FALSE);
		 ReleaseDC(dc); 
	
		
		m_PIC=m_FilePathName;
		UpdateData(false);
		}

	
}

void CAddPersonalDlg::OnPersonalOK() 
{
	UpdateData(true);

	if (m_NAME.GetLength()==0||m_TEL.GetLength()==0)
	{
		AfxMessageBox("Name or TEL field can not be empty!");
		return;
	}

	// Add by cmchen : avoid chinese words
	if (isChinese(m_TEL)) {
		AfxMessageBox("抱歉!Telephone欄位不能含有中文.");
		return;
	}

	BuddyList *tmp_list = SearchBuddyByTel(((LPSTR)(LPCTSTR)m_TEL));
	if(tmp_list!=NULL)
	{
		AfxMessageBox("Telephone number already exist!!");	
		FreeBuddyList(tmp_list);
		return;
	}
	tmp_list = SearchBuddyByName(((LPSTR)(LPCTSTR)m_NAME));
	if(tmp_list!=NULL)
	{
		AfxMessageBox("Display Name already exist!!");	
		FreeBuddyList(tmp_list);
		return;
	}
	
	CDialog::OnOK();
}

BOOL CAddPersonalDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here

#ifdef _XCAPSERVER

	CComboBox*      add_group = (CComboBox *)GetDlgItem(IDC_PERSONAL_ADD_GROUP);
	GroupList *myGroupList = GetAllGroup(),*tmpGList;
	if(myGroupList!=NULL)
	{
		tmpGList = myGroupList;
		while(tmpGList!=NULL)
		{ 
			
			add_group->AddString(tmpGList->groupname);
			tmpGList = tmpGList->next;
		}
		add_group->SetWindowText(myGroupList->groupname);
		FreeGroupList(myGroupList);
	}
	else
	{
		add_group->AddString(DEFAULT_GROUPNAME);
		add_group->SetWindowText(DEFAULT_GROUPNAME);		
	}

#endif

	GetDlgItem(IDC_PERSONAL_ADD_NAME)->SetFocus();
	return FALSE;
//	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
