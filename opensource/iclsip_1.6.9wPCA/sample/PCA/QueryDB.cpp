// QueryDB.cpp: implementation of the CQueryDB class.
//	此類別主要作為查詢資料庫使用
//	共有兩種查詢條件,可取得該筆記錄的所有資訊
//	1.SearchId(CString strID)
//	  根據員工ID來作查詢
//	2.SearchSn(CString strSn)
//	  SearchSn(int intSn)
//	  根據資料表內的sn(serial number <<唯一>>)
//	  來作查詢
//  3.SearchTel(CString strTel)
//    根據電話(分機)來作查詢
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "PCAUA.h"
#include "QueryDB.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CQueryDB::CQueryDB()
{

	//Initial member variables
	m_sn=0;
	m_class=1;
	m_name="";
	m_id="";
	m_division="";
	m_department="";
	m_telephone="";
	m_remark="";
	m_pic="";
	m_pic_short="";

	//get AP location
	//so, we have to process it after getting location of this AP
	GetModuleFileName( GetModuleHandle(NULL), projectpath.GetBuffer(1024), 1024);
	projectpath.ReleaseBuffer();

	//process AP location
	int length_projectpath=projectpath.GetLength();
	int target_projectpath=projectpath.ReverseFind('\\');
	projectpath.Delete(target_projectpath,length_projectpath-target_projectpath);

}

CQueryDB::~CQueryDB()
{

}

bool CQueryDB::SearchId(CString strID)
{

	CDBAccess daoDB;

	// truncate empty character enclose strID
	strID.TrimLeft();
	strID.TrimRight();

	// define condition of sql
	CString cs="SELECT * FROM personal where id='"+strID+"'";
	CString strInitial,errStr;
	strInitial="Provider=Microsoft.Jet.OLEDB.4.0;Data Source=";
	strInitial=strInitial+projectpath+"\\PCAdb.mdb";
	daoDB.Initialize(strInitial,errStr);
	daoDB.OpenRecordSet(cs,errStr);
	if( !errStr.IsEmpty() )
		AfxMessageBox(errStr);   
	if (daoDB.GetRecordCount()>0)
	{

		// store query results into member variables
		int getSn,getClass;
		CString getName,getId,getDivision,getDepartment,getTel,getRemark,getPic;

		getSn=daoDB.GetIntegerFromField(0);
		getClass=daoDB.GetIntegerFromField(1);
		daoDB.GetStringFromField(2,(LPTSTR)getName.GetBuffer(128),128);
		getName.ReleaseBuffer();
		daoDB.GetStringFromField(3,(LPTSTR)getId.GetBuffer(128),128);
		getId.ReleaseBuffer();
		daoDB.GetStringFromField(4,(LPTSTR)getDivision.GetBuffer(128),128);
		getDivision.ReleaseBuffer();
		daoDB.GetStringFromField(5,(LPTSTR)getDepartment.GetBuffer(128),128);
		getDepartment.ReleaseBuffer();
		daoDB.GetStringFromField(6,(LPTSTR)getTel.GetBuffer(128),128);
		getTel.ReleaseBuffer();
		daoDB.GetStringFromField(7,(LPTSTR)getRemark.GetBuffer(128),128);
		getRemark.ReleaseBuffer();
		daoDB.GetStringFromField(8,(LPTSTR)getPic.GetBuffer(128),128);
		getPic.ReleaseBuffer();
		
		m_sn=getSn;
		m_class=getClass;
		m_name=getName;
		m_id=getId;
		m_division=getDivision;
		m_department=getDepartment;
		m_telephone=getTel;
		m_remark=getRemark;
		m_pic=projectpath+getPic;
		m_pic_short=getPic;
		daoDB.CloseRecordSet();
		return true;	
	}
	else
		return false;
}

bool CQueryDB::SearchSn(int intSn)
{

	CDBAccess daoDB;
	char tmpSn[20];
	// define condition of sql
	CString cs="SELECT * FROM personal where sn=";
	cs=cs+itoa(intSn,tmpSn,10);
	CString strInitial,errStr;
	strInitial="Provider=Microsoft.Jet.OLEDB.4.0;Data Source=";
	strInitial=strInitial+projectpath+"\\PCAdb.mdb";
	daoDB.Initialize(strInitial,errStr);
	daoDB.OpenRecordSet(cs,errStr);
	if( !errStr.IsEmpty() )
		AfxMessageBox(errStr);   

	if (daoDB.GetRecordCount()>0)
	{
		// store query results into member variables
		int getSn,getClass;
		CString getName,getId,getDivision,getDepartment,getTel,getRemark,getPic;

		getSn=daoDB.GetIntegerFromField(0);
		getClass=daoDB.GetIntegerFromField(1);
		daoDB.GetStringFromField(2,(LPTSTR)getName.GetBuffer(128),128);
		getName.ReleaseBuffer();
		daoDB.GetStringFromField(3,(LPTSTR)getId.GetBuffer(128),128);
		getId.ReleaseBuffer();
		daoDB.GetStringFromField(4,(LPTSTR)getDivision.GetBuffer(128),128);
		getDivision.ReleaseBuffer();
		daoDB.GetStringFromField(5,(LPTSTR)getDepartment.GetBuffer(128),128);
		getDepartment.ReleaseBuffer();
		daoDB.GetStringFromField(6,(LPTSTR)getTel.GetBuffer(128),128);
		getTel.ReleaseBuffer();
		daoDB.GetStringFromField(7,(LPTSTR)getRemark.GetBuffer(128),128);
		getRemark.ReleaseBuffer();
		daoDB.GetStringFromField(8,(LPTSTR)getPic.GetBuffer(128),128);
		getPic.ReleaseBuffer();
		
		m_sn=getSn;
		m_class=getClass;
		m_name=getName;
		m_id=getId;
		m_division=getDivision;
		m_department=getDepartment;
		m_telephone=getTel;
		m_remark=getRemark;
		m_pic=projectpath+getPic;
		m_pic_short=getPic;
		daoDB.CloseRecordSet();
		return true;
	}
	else
		return false;
}

bool CQueryDB::SearchSn(CString strSn)
{

	CDBAccess daoDB;
	// truncate empty character enclose strID
	strSn.TrimLeft();
	strSn.TrimRight();

	// define condition of sql
	CString cs="SELECT * FROM personal where sn=";
	cs=cs+strSn;
	CString strInitial,errStr;
	strInitial="Provider=Microsoft.Jet.OLEDB.4.0;Data Source=";
	strInitial=strInitial+projectpath+"\\PCAdb.mdb";
	daoDB.Initialize(strInitial,errStr);
	daoDB.OpenRecordSet(cs,errStr);
	if( !errStr.IsEmpty() )
		AfxMessageBox(errStr);   

	if (daoDB.GetRecordCount()>0)
	{
		// store query results into member variables
		int getSn,getClass;
		CString getName,getId,getDivision,getDepartment,getTel,getRemark,getPic;

		getSn=daoDB.GetIntegerFromField(0);
		getClass=daoDB.GetIntegerFromField(1);
		daoDB.GetStringFromField(2,(LPTSTR)getName.GetBuffer(128),128);
		getName.ReleaseBuffer();
		daoDB.GetStringFromField(3,(LPTSTR)getId.GetBuffer(128),128);
		getId.ReleaseBuffer();
		daoDB.GetStringFromField(4,(LPTSTR)getDivision.GetBuffer(128),128);
		getDivision.ReleaseBuffer();
		daoDB.GetStringFromField(5,(LPTSTR)getDepartment.GetBuffer(128),128);
		getDepartment.ReleaseBuffer();
		daoDB.GetStringFromField(6,(LPTSTR)getTel.GetBuffer(128),128);
		getTel.ReleaseBuffer();
		daoDB.GetStringFromField(7,(LPTSTR)getRemark.GetBuffer(128),128);
		getRemark.ReleaseBuffer();
		daoDB.GetStringFromField(8,(LPTSTR)getPic.GetBuffer(128),128);
		getPic.ReleaseBuffer();
		
		m_sn=getSn;
		m_class=getClass;
		m_name=getName;
		m_id=getId;
		m_division=getDivision;
		m_department=getDepartment;
		m_telephone=getTel;
		m_remark=getRemark;
		m_pic=projectpath+getPic;
		m_pic_short=getPic;
		daoDB.CloseRecordSet();
		return true;
	}
	else
		return false;
}

bool CQueryDB::SearchTel(CString strTel)
{
	CDBAccess daoDB;
	// truncate empty character enclose strID
	strTel.TrimLeft();
	strTel.TrimRight();

	// define condition of sql
	CString cs="SELECT * FROM personal where telephone='"+strTel+"'";
	
	CString strInitial,errStr;
	strInitial="Provider=Microsoft.Jet.OLEDB.4.0;Data Source=";
	strInitial=strInitial+projectpath+"\\PCAdb.mdb";
	daoDB.Initialize(strInitial,errStr);
	daoDB.OpenRecordSet(cs,errStr);
	if( !errStr.IsEmpty() )
		AfxMessageBox(errStr);   

	if (daoDB.GetRecordCount()>0)
	{
		// store query results into member variables
		int getSn,getClass;
		CString getName,getId,getDivision,getDepartment,getTel,getRemark,getPic;

		getSn=daoDB.GetIntegerFromField(0);
		getClass=daoDB.GetIntegerFromField(1);
		daoDB.GetStringFromField(2,(LPTSTR)getName.GetBuffer(128),128);
		getName.ReleaseBuffer();

		daoDB.GetStringFromField(3,(LPTSTR)getId.GetBuffer(128),128);
		getId.ReleaseBuffer();

		daoDB.GetStringFromField(4,(LPTSTR)getDivision.GetBuffer(128),128);
		getDivision.ReleaseBuffer();

		daoDB.GetStringFromField(5,(LPTSTR)getDepartment.GetBuffer(128),128);
		getDepartment.ReleaseBuffer();

		daoDB.GetStringFromField(6,(LPTSTR)getTel.GetBuffer(128),128);
		getTel.ReleaseBuffer();

		daoDB.GetStringFromField(7,(LPTSTR)getRemark.GetBuffer(128),128);
		getRemark.ReleaseBuffer();

		daoDB.GetStringFromField(8,(LPTSTR)getPic.GetBuffer(128),128);
		getPic.ReleaseBuffer();
		
		m_sn=getSn;
		m_class=getClass;
		m_name=getName;
		m_id=getId;
		m_division=getDivision;
		m_department=getDepartment;
		m_telephone=getTel;
		m_remark=getRemark;
		m_pic=projectpath+getPic;
		m_pic_short=getPic;
		daoDB.CloseRecordSet();
		return true;
	}
	else
		return false;

}
