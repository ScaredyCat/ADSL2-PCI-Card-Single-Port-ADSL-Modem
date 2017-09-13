// EDITPERSONAL.cpp : implementation file
//

#include "stdafx.h"
#include "PCAUA.h"
#include "EditPersonalDlg.h"
#include "PCAUADlg.h"

#ifdef _XCAPSERVER
#include "xcap_datatype.h"
#include "xcapapi.h"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditPersonalDlg dialog


CEditPersonalDlg::CEditPersonalDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEditPersonalDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEditPersonalDlg)
	m_PERSON2_COMPANY = _T("");
	m_PERSON2_NAME = _T("");
	m_PERSON2_TELEPHONE = _T("");
	m_PERSON2_REMARK = _T("");
	m_PERSON2_PIC = _T("");
	m_PERSON2_GROUP = _T("");
	//}}AFX_DATA_INIT
}


void CEditPersonalDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditPersonalDlg)
	DDX_Text(pDX, IDC_PERSON2_COMPANY, m_PERSON2_COMPANY);
	DDX_Text(pDX, IDC_PERSON2_NAME, m_PERSON2_NAME);
	DDX_Text(pDX, IDC_PERSON2_TELEPHONE, m_PERSON2_TELEPHONE);
	DDX_Text(pDX, IDC_PERSON2_REMARK, m_PERSON2_REMARK);
	DDX_Text(pDX, IDC_PERSON2_PIC, m_PERSON2_PIC);
	DDX_CBString(pDX, IDC_PERSON2_GROUP, m_PERSON2_GROUP);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEditPersonalDlg, CDialog)
	//{{AFX_MSG_MAP(CEditPersonalDlg)
	ON_BN_CLICKED(IDC_PERSON2_OPENFILE, OnPerson2Openfile)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_BTN_DIAL, OnBtnDial)
	ON_BN_CLICKED(IDOK, OnUpdate)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditPersonalDlg message handlers

//void CEditPersonalDlg::OnPerson2Modify() 
//{
	// TODO: Add your control notification handler code here
//}

void CEditPersonalDlg::OnPerson2Openfile() 
{
	// TODO: Add your control notification handler code here
	UpdateData(true);

	CString strFilter = "Picture Files (.bmp;.dib;.emf;.gif;.ico;.jpg;.wmf)|*.bmp;*.dib;*.emf;*.gif;*.ico;*.jpg;*.wmf|BMP (*.bmp)|*.bmp|DIB (*.dib)|*.dib|EMF (*.emf)|*.emf|GIF (*.gif)|*.gif|ICO (*.ico)|*.ico|JPG (*.jpg)|*.jpg|WMF (*.wmf)|*.wmf|All Files (*.*)|*.*||";
	CFileDialog OpenFileDialog(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, strFilter);
	OpenFileDialog.m_ofn.lpstrTitle = "Open a picture file";
	
	if (OpenFileDialog.DoModal() == IDOK)
	{
		CString m_FilePathName;
		m_FilePathName.Format("%s", OpenFileDialog.m_ofn.lpstrFile);
		m_PERSON2_PIC=m_FilePathName;
		//CWnd*      pSEARCH = (CWnd*)GetDlgItem(IDC_PERSONAL_ADD_PIC);

		
		CRect rct;
		GetDlgItem(IDC_PERSON2_PIC)->GetWindowRect(&rct);
		ScreenToClient(&rct);

		CDC *dc=this->GetDC();
		m_Picture.Load(m_FilePathName);
		m_Picture.UpdateSizeOnDC(dc);
	
		m_Picture.Show(dc,CRect(rct.left,rct.top,rct.left+80,rct.top+80));

		ReleaseDC(dc); 

		UpdateData(false);
	}
	
}

void CEditPersonalDlg::showpic(CString pic)
{

}

int CEditPersonalDlg::DoModal() 
{
	// TODO: Add your specialized code here and/or call the base class
	//MessageBox(m_PERSON2_PIC);
	return CDialog::DoModal();
}

void CEditPersonalDlg::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	// TODO: Add your message handler code here
	
	CString m_FilePathName=m_PERSON2_PIC;
	CRect rct;
	GetDlgItem(IDC_PERSON2_PIC)->GetWindowRect(&rct);
	ScreenToClient(&rct);

		CDC *thisdc=this->GetDC();
	
		m_Picture.Load(projectpath+m_FilePathName);
		m_Picture.UpdateSizeOnDC(thisdc);
	
		m_Picture.Show(thisdc,CRect(rct.left,rct.top,rct.left+80,rct.top+80));

		ReleaseDC(thisdc); 



	// Do not call CDialog::OnPaint() for painting messages
}

BOOL CEditPersonalDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here


	//get AP location, default in .\debug\xx.exe
	//so, we have to process it after getting location of this AP
	GetModuleFileName( GetModuleHandle(NULL), projectpath.GetBuffer(1024), 1024);
	projectpath.ReleaseBuffer();

	//process AP location
	int length_projectpath=projectpath.GetLength();
	int target_projectpath=projectpath.ReverseFind('\\');
	projectpath.Delete(target_projectpath,length_projectpath-target_projectpath);

#ifdef _XCAPSERVER
	CString UserName = CUACDlg::GetUAComRegString("User_Info","Username");
	CComboBox*      edit_group = (CComboBox *)GetDlgItem(IDC_PERSON2_GROUP);
	GroupList *myGroupList = GetAllGroup(),*tmpGList;
	if(myGroupList!=NULL)
	{
		tmpGList = myGroupList;
		while(tmpGList!=NULL)
		{
			edit_group->AddString(tmpGList->groupname);
			tmpGList = tmpGList->next;
		}
		
		BuddyInfo *tmpbuddy = NewBuddy();
		strcpy(tmpbuddy->m_telephone,(LPSTR)(LPCTSTR)m_PERSON2_TELEPHONE);
		BuddyList *tmpBuddyList = SearchBuddy(tmpbuddy);
		if(tmpBuddyList!=NULL)
		{
			edit_group->SetWindowText(tmpBuddyList->group);
			FreeBuddyList(tmpBuddyList);
		}
		FreeBuddy(tmpbuddy); 
		FreeGroupList(myGroupList);
	}
	else
	{
		edit_group->AddString(DEFAULT_GROUPNAME);
		edit_group->SetWindowText(DEFAULT_GROUPNAME);
	}

	// marked by cmchen 2005.09.27 
	//CEdit *name_edit = (CEdit *)GetDlgItem(IDC_PERSON2_NAME);
	//name_edit->SetReadOnly();

	// added by cmchen 2005.09.27 
	CEdit *tel_edit = (CEdit *)GetDlgItem(IDC_PERSON2_TELEPHONE);
	tel_edit->SetReadOnly();
#endif
	
	//GetDlgItem(IDC_PERSON2_TELEPHONE)->SetFocus();
	GetDlgItem(IDC_PERSON2_NAME)->SetFocus();
	return FALSE;
//	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CEditPersonalDlg::OnBtnDial() 
{
	UpdateData();
	((CPCAUADlg*)AfxGetMainWnd())->DialFromAddrBook(m_PERSON2_TELEPHONE);
	EndDialog(IDCANCEL);
}

void CEditPersonalDlg::OnUpdate() 
{
	// TODO: Add your control notification handler code here
	UpdateData(true);

	if (m_PERSON2_TELEPHONE.GetLength()==0)
	{
		AfxMessageBox("TEL field can not be empty!");
		return;
	}
	// Add by cmchen : avoid chinese words
	if (isChinese(m_PERSON2_TELEPHONE)) {
		AfxMessageBox("抱歉!Telephone欄位不能含有中文.");
		return;
	}
	
	BuddyList *tmp_list = SearchBuddyByTel(((LPSTR)(LPCTSTR)m_PERSON2_TELEPHONE));
	if(tmp_list!=NULL)
	{
		if(_stricmp(tmp_list->buddy->m_telephone,(LPSTR)(LPCTSTR)m_PERSON2_TELEPHONE))
		{
			AfxMessageBox("Telephone number already exist!!");	
			FreeBuddyList(tmp_list);
			return;
		}
	}
	
	CDialog::OnOK();
}
