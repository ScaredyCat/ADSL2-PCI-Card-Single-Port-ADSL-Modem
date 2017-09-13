// CallLogDlg.cpp : implementation file
//

#include "stdafx.h"
#include "pcaua.h"
#include "CallLogDlg.h"
#include "PCAUADlg.h"
#include "AddPersonalDlg.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCallLogDlg dialog


CCallLogDlg::CCallLogDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCallLogDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCallLogDlg)
	m_CallLog_End = _T("");
	m_CallLog_Interval = _T("");
	m_CallLog_Start = _T("");
	m_CallLog_Tel = _T("");
	m_CallLog_Type = _T("");
	m_CallLog_URI = _T("");
	//}}AFX_DATA_INIT
	isRefresh=false;
}


void CCallLogDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCallLogDlg)
	DDX_Text(pDX, IDC_CallLog_End, m_CallLog_End);
	DDX_Text(pDX, IDC_CallLog_Interval, m_CallLog_Interval);
	DDX_Text(pDX, IDC_CallLog_Start, m_CallLog_Start);
	DDX_Text(pDX, IDC_CallLog_Tel, m_CallLog_Tel);
	DDX_Text(pDX, IDC_CallLog_Type, m_CallLog_Type);
	DDX_Text(pDX, IDC_CallLog_URI, m_CallLog_URI);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCallLogDlg, CDialog)
	//{{AFX_MSG_MAP(CCallLogDlg)
	ON_BN_CLICKED(IDC_BTN_Del, OnBTNDel)
	ON_BN_CLICKED(IDC_BTN_Edit, OnBTNEdit)
	ON_BN_CLICKED(IDC_BTN_DIAL, OnBtnDial)
	ON_BN_CLICKED(IDC_BTN_Add, OnBTNAdd)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCallLogDlg message handlers

void CCallLogDlg::OnBTNDel() 
{


	// TODO: Add your control notification handler code here
	CCallLogDelDlg dlgCallLogDel;

	//which way we will delete in dlgCallLogDel
	int intSelect; 
	dlgCallLogDel.pintSelect=&intSelect;


	//when ? to delete 
	COleDateTime dateTime;
	dlgCallLogDel.pdateTime=&dateTime;
	
	dlgCallLogDel.DoModal();
	
	dateTime.Format(_T("%A, %m %d, %Y"));

	CCallLog cl;
	cl.deleteItem(dateTime);
	

	isRefresh=true;
	this->PostMessage( WM_CLOSE, 0, 0 );	
}

void CCallLogDlg::OnBTNEdit() 
{

	// TODO: Add your control notification handler code here

	CQueryDB dbQuery;

	// check whether user with this tel in our database
	// if exist, store data in attributes of dbQuery
	if (dbQuery.SearchTel(m_CallLog_Tel))
	{

		//implement CEditPersonalDlg
		CEditPersonalDlg personal;

		//pass essential data into personal
		personal.m_PERSON2_NAME=dbQuery.m_name;
		personal.m_PERSON2_COMPANY=dbQuery.m_division;
		personal.m_PERSON2_TELEPHONE=dbQuery.m_telephone;
		personal.m_PERSON2_REMARK=dbQuery.m_remark;
		
		if (dbQuery.m_class==1)
		{
			personal.m_PERSON2_COMPANY="ITRI "+dbQuery.m_division;
//@Department			personal.m_PERSON2_DEPARTMENT=dbQuery.m_department;
		}
//@Department		else
//@Department			personal.m_PERSON2_DEPARTMENT="";

		// Because dbQuery.m_pic is full path,
		// we just need file name;

		CString pic;
		int nFirst,nCount;

		nFirst=dbQuery.projectpath.GetLength();
		nCount=dbQuery.m_pic.GetLength()-dbQuery.projectpath.GetLength();
		pic=dbQuery.m_pic.Mid(nFirst,nCount);
		
		personal.m_PERSON2_PIC=pic;



		
		int result;
		result=personal.DoModal();

		if (result==IDOK)
		{

			CString strTest;

			// collect information in dialog into info_personal
			InfoPersonal info_personal;

			// sn,class are static information
			// catch from dbQuery
			info_personal.m_sn=dbQuery.m_sn;
			info_personal.m_class=dbQuery.m_class;


			info_personal.m_name=personal.m_PERSON2_NAME;
			
			if (dbQuery.m_class==1)
			{
				//if class=1 ,We truncate "ITRI" 
				CString division;
				division=personal.m_PERSON2_COMPANY;
				division.Replace("ITRI","");
				division.TrimLeft();
				division.TrimRight();
			
				info_personal.m_division=division;
//@Department		info_personal.m_department=personal.m_PERSON2_DEPARTMENT;
			}
			else
			{
				info_personal.m_division=personal.m_PERSON2_COMPANY;
//@Department				info_personal.m_department="";
			}
			
			info_personal.m_telephone=personal.m_PERSON2_TELEPHONE;
			info_personal.m_remark=personal.m_PERSON2_REMARK;
			//info_personal.m_pic=personal.m_PERSON2_PIC;


			//		Copy original path into our target path
			//		Modify pic path , map into ./pic/B&...
			char c2[20];					
			itoa(dbQuery.m_sn,c2,10);
			int m_leng=personal.m_PERSON2_PIC.GetLength();
			int m_ext=personal.m_PERSON2_PIC.Find('.');
			CString tt=personal.m_PERSON2_PIC.Mid(m_ext,m_leng-m_ext);


			CPersonal cpersonal;
			CString projectpath=cpersonal.projectpath;
			

			CString newpath=projectpath+"\\pic\\B"+c2+tt;
			CString strStorePath="\\pic\\B";	
			strStorePath=strStorePath+c2+tt;
			info_personal.m_pic=strStorePath;
			//copy picture selected by users into default path ".\pic\ "
			CopyFile(personal.m_PERSON2_PIC,newpath,FALSE);


			cpersonal.ModifyDB(&info_personal);





		}

	}
	else
		MessageBox("This user is not in our database!","Not Found",MB_ICONWARNING|MB_OK);	

  
}

void CCallLogDlg::OnBtnDial() 
{
	// TODO: Add your control notification handler code here
	UpdateData();
	((CPCAUADlg*)AfxGetMainWnd())->DialFromAddrBook(m_CallLog_Tel);
	EndDialog(IDCANCEL);
}

void CCallLogDlg::OnBTNAdd() 
{
	

	//Check tel number whether it is duplicated.
	CQueryDB qb;
	int qbResult=IDYES;

	if(qb.SearchTel(m_CallLog_Tel))
	{
		qbResult=MessageBox("This telephone number has duplicated.\n Continue do it anyway? ","Data Duplication",MB_ICONQUESTION|MB_YESNO);
	}

	
	if(qbResult==IDYES)
	{
			CAddPersonalDlg dlg;
			dlg.m_TEL=m_CallLog_Tel;
			int result;
			result=dlg.DoModal();
			

			if (result==IDOK)
			{


				InfoDB db;

				db.m_class=2;
				db.m_department="";
				db.m_division=dlg.m_COMPANY;
				db.m_id="";
				db.m_name=dlg.m_NAME;
				db.m_pic=dlg.m_PIC;
				db.m_remark=dlg.m_REMARK;
				db.m_telephone=dlg.m_TEL;



				
			//	AddDB(&db);

			char strclass[20];
			
			itoa(db.m_class,strclass,10);
			CString str;
			str="insert into personal (class,name,id,division,department,telephone,remark,pic) values (";
			str=str+ strclass+ ",";
			str=str+"'"+db.m_name+"',";
			str=str+"'"+db.m_id+"',";
			str=str+"'"+db.m_division+"',";
			str=str+"'"+db.m_department+"',";
			str=str+"'"+db.m_telephone+"',";
			str=str+"'"+db.m_remark+"',";
			str=str+"'"+db.m_pic+"')";

			CString projectpath;
			//get AP location, default in .\debug\xx.exe
			//so, we have to process it after getting location of this AP
			GetModuleFileName( GetModuleHandle(NULL), projectpath.GetBuffer(1024), 1024);
			projectpath.ReleaseBuffer();

			//process AP location
			int length_projectpath=projectpath.GetLength();
			int target_projectpath=projectpath.ReverseFind('\\');
			projectpath.Delete(target_projectpath,length_projectpath-target_projectpath);

			CDBAccess dbinsert;
			CString strInitial,errStr;
			strInitial="Provider=Microsoft.Jet.OLEDB.4.0;Data Source=";
			strInitial=strInitial+projectpath+"\\PCAdb.mdb";
			dbinsert.Initialize(strInitial,errStr);	
			dbinsert.ExecuteSql(str,errStr);
			if( !errStr.IsEmpty() )
				AfxMessageBox(errStr);   
			dbinsert.CloseConnection();
				
			// Copy source pic to target dictionary : .\pic\B+...
			// B is class=2  & A is class=1
			// In Personal, I use personal.sn to be default file name
			CString strStorPath;
			char c2[20];
			int m_leng=dlg.m_PIC.GetLength();
			if (m_leng != 0) {
				int m_ext=dlg.m_PIC.Find('.');
				CString tt=dlg.m_PIC.Mid(m_ext,m_leng-m_ext);
			
				CDBAccess m_moddb;
				
				strInitial="Provider=Microsoft.Jet.OLEDB.4.0;Data Source=";
				strInitial=strInitial+projectpath+"\\PCAdb.mdb";
				m_moddb.Initialize(strInitial,errStr);
				m_moddb.OpenRecordSet("SELECT * FROM personal",errStr);
				if( !errStr.IsEmpty() )
					AfxMessageBox(errStr);   

				
			
				int intCount=m_moddb.GetRecordCount();
				if(intCount>0)
				{
					for(int i=0;i<(intCount-1);i++)
						m_moddb.MoveNext();
				}
				
				CString getName,getDivision,getTel,getRemark;
				int getSn,getClass;
				getSn=m_moddb.GetIntegerFromField(0);
				getClass=m_moddb.GetIntegerFromField(1);

				char class2[20];		
				itoa(getSn,c2,10);
				itoa(getClass,class2,10);
				
				
				m_moddb.GetStringFromField(2,(LPTSTR)getName.GetBuffer(128),128);
				m_moddb.GetStringFromField(4,(LPTSTR)getDivision.GetBuffer(128),128);
				m_moddb.GetStringFromField(6,(LPTSTR)getTel.GetBuffer(128),128);
				m_moddb.GetStringFromField(7,(LPTSTR)getRemark.GetBuffer(128),128);

				CString newpath=projectpath+"\\pic\\B"+c2+tt;
				strStorPath="\\pic\\B";
				strStorPath=strStorPath+c2+tt;
				//	CPanelDlg::m_ListPersonal.AddItem( _T(c2),_T(class2),_T(getName), _T(getDivision), _T(getTel),_T(getRemark),_T(strStorPath) );

				
				
				CopyFile(dlg.m_PIC,newpath,FALSE);	
			}


				// Modify pic location in DB


				CDBAccess dbUp;
				
				strInitial="Provider=Microsoft.Jet.OLEDB.4.0;Data Source=";
				strInitial=strInitial+projectpath+"\\PCAdb.mdb";
				dbUp.Initialize(strInitial,errStr);
				CString strUpdate;
				strUpdate="Update personal set pic='";
				strUpdate=strUpdate+strStorPath+"' where sn=";
				strUpdate=strUpdate+c2;
				dbUp.ExecuteSql(strUpdate,errStr);
				if( !errStr.IsEmpty() )
					AfxMessageBox(errStr);   

			}		
		}
}
