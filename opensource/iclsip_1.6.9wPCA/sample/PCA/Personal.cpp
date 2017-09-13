// Personal.cpp: implementation of the CPersonal class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Personal.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPersonal::CPersonal()
{
	//get AP location, default in .\debug\xx.exe
	//so, we have to process it after getting location of this AP
	GetModuleFileName( GetModuleHandle(NULL), projectpath.GetBuffer(1024), 1024);
	projectpath.ReleaseBuffer();

	//process AP location
	int length_projectpath=projectpath.GetLength();
	int target_projectpath=projectpath.ReverseFind('\\');
	projectpath.Delete(target_projectpath,length_projectpath-target_projectpath);

	initialString="Provider=Microsoft.Jet.OLEDB.4.0;Data Source="+projectpath+"\\PCAdb.mdb";
}

CPersonal::~CPersonal()
{

}

void CPersonal::ModifyDB(InfoPersonal *info)
{


	char txt[20];
	itoa(info->m_sn,txt,10);
	
	CString condi="sn=";
	condi+=txt;
	CDBAccess db;
	CString strerr, fd;	
	db.Initialize(initialString,strerr);


	CString cs="";
	cs=cs+"Update personal set name='"+tranStr(info->m_name)+"',";	
	cs=cs+"id='"+tranStr(info->m_id)+"',";
	cs=cs+"division='"+tranStr(info->m_division)+"',";
	cs=cs+"department='"+tranStr(info->m_department)+"',";
	cs=cs+"telephone='"+tranStr(info->m_telephone)+"',";
	cs=cs+"remark='"+tranStr(info->m_remark)+"',";
	cs=cs+"pic='"+tranStr(info->m_pic)+"' where "+condi;
	db.ExecuteSql(cs,strerr);
	if( !strerr.IsEmpty() )
		AfxMessageBox(strerr);




}

CString CPersonal::tranStr(CString str)
{
	str.Replace("'","''");
	return str;
}

int CPersonal::AddDB(InfoPersonal *info)
{
	char strclass[20];
	
	itoa(info->m_class,strclass,10);
	CString str;
	str="insert into personal (class,name,id,division,department,telephone,remark,pic) values (";
	str=str+ strclass+ ",";
	str=str+"'"+tranStr(info->m_name)+"',";
	str=str+"'"+tranStr(info->m_id)+"',";
	str=str+"'"+tranStr(info->m_division)+"',";
	str=str+"'"+tranStr(info->m_department)+"',";
	str=str+"'"+tranStr(info->m_telephone)+"',";
	str=str+"'"+tranStr(info->m_remark)+"',";
	str=str+"'"+tranStr(info->m_pic)+"')";


	CDBAccess db;
	CString strInitial,errStr;
	db.Initialize(initialString,errStr);	
	db.ExecuteSql(str,errStr);
	if( !errStr.IsEmpty() )
		AfxMessageBox(errStr);   
	db.CloseConnection();

	//return sn
	CDBAccess dbSn;
	int sn;
	dbSn.Initialize(initialString,errStr);
	dbSn.OpenRecordSet("select * from personal",errStr);
	if( !errStr.IsEmpty() )
		AfxMessageBox(errStr);   

	while(!dbSn.IsEOF())
	{
		sn=dbSn.GetIntegerFromField(0);
		dbSn.MoveNext();
	}
	return sn;
}
