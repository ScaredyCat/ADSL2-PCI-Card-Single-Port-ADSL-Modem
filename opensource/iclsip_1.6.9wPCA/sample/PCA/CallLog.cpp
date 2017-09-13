// CallLog.cpp: implementation of the CCallLog class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CallLog.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCallLog::CCallLog()
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

CCallLog::~CCallLog()
{

}

int CCallLog::getRecordCount()
{
	CDBAccess db;
	CString strerr, fd;	
	db.Initialize(initialString,strerr);

	if(!db.OpenRecordSet("SELECT * FROM CallLog ", strerr))
	{
		AfxMessageBox("Open database error");
		return -1;
	}
	else
		return db.GetRecordCount();	
	db.CloseRecordSet();
}

void CCallLog::storeAllRecord()
{
	//
	// store all data into (CArray)arrayInfo
	//




	// At first, we clean arrayInfo --- 

	//Remove all element in arrayInfo
	arrayInfo.RemoveAll();


	CDBAccess db;
	
	CString strerr, fd;	
	db.Initialize(initialString,strerr);

	// define condition of sql
	char *cs="SELECT * FROM CallLog order by sn desc";	

	db.OpenRecordSet(cs,strerr);
	if( !strerr.IsEmpty() )
		AfxMessageBox(strerr);   

	int resultCount=getRecordCount();
	if (resultCount>0)
	{
	
		InfoCallLog infoCallLog;	

		for (int i=0;i<resultCount;i++)
		{
			infoCallLog.m_sn=db.GetIntegerFromField(0);
			db.GetStringFromField(1, (LPTSTR)infoCallLog.m_Tel.GetBuffer(128),128);
			infoCallLog.m_Tel.ReleaseBuffer();
			db.GetStringFromField(2, (LPTSTR)infoCallLog.m_URI.GetBuffer(128),128);
			infoCallLog.m_URI.ReleaseBuffer();
			db.GetCOleDateTimeFromField(3,&infoCallLog.m_StartTime);
			db.GetCOleDateTimeFromField(4,&infoCallLog.m_EndTime);
			infoCallLog.m_Type=db.GetIntegerFromField(5);
			infoCallLog.m_CallResult=db.GetIntegerFromField(6);			
			infoCallLog.m_CallBack=db.GetboolFromField(7);			
			arrayInfo.Add(infoCallLog);

			if ( !db.MoveNext())
				break;	// no more records
		}
	}
    db.CloseRecordSet();
}

void CCallLog::insertItem(CString tel, CString URI, COleDateTime StartTime, COleDateTime EndTime, int type, int CallResult, BOOL CallBack)
{
	// type=1, call in ; type=2, call out


	//establish CDaoCallLogDB  instance : dbCallLog
	CDBAccess db;
	CString str;
	char trantype[10],tranCallResult[10],tranCallBack[10];
	itoa(type,trantype,10);
	itoa(CallResult,tranCallResult,10);
	itoa(CallBack,tranCallBack,10);
	
	CString strerr, fd;	
	db.Initialize(initialString,strerr);
	CString cs;
	cs=cs+"insert into CallLog (Tel,URI,StartTime,EndTime,Type,CallResult,CallBack) values ('";
	cs=cs+tel+"','";
	cs=cs+URI+"',#";
	cs=cs+StartTime.Format(_T("%m/%d/%Y %H:%M:%S"))+"#,#";
	cs=cs+EndTime.Format(_T("%m/%d/%Y %H:%M:%S"))+"#,";
	cs=cs+trantype+",";	
	cs=cs+tranCallResult+",";
	cs=cs+tranCallBack+")";
	db.ExecuteSql(cs,strerr);
	if( !strerr.IsEmpty() )
		AfxMessageBox(strerr);   
	

}

void CCallLog::getItemInfo(InfoCallLog *info, int itemNum)
{
	//============================================================
	//Get information of one record according to item sn.
	//I declare itemNum as a variable which is "sn" defined at 
	// table: CallLog
	//In this method, info is a (InfoCallLog)point extern declare.
	//We store information of this item.
	//============================================================

	//Get information of all records
	storeAllRecord();

	int size=arrayInfo.GetSize();
	int i;
	int order=0;
	for (i=0;i<size;i++)
	{
		if (arrayInfo[i].m_sn==itemNum)
			order=i;
	}

	//Store specialization
	info->m_sn=arrayInfo[order].m_sn;
	info->m_Tel=arrayInfo[order].m_Tel;
	info->m_URI=arrayInfo[order].m_URI;
	info->m_StartTime=arrayInfo[order].m_StartTime;
	info->m_EndTime=arrayInfo[order].m_EndTime;
	info->m_Type=arrayInfo[order].m_Type;
	info->m_CallResult=arrayInfo[order].m_CallResult;
	info->m_CallBack=arrayInfo[order].m_CallBack;
}

CString CCallLog::tranHumenSenseTime(COleDateTime oleDateTime)
{
	CString result;
	result=oleDateTime.Format(_T("%Y/%m/%d %I:%M:%S %p"));
	return result;
}

void CCallLog::deleteItem(COleDateTime timeDelete)
{
	CString cc;
	cc=timeDelete.Format(_T("%Y/%m/%d"));
	CDBAccess db;
	CString strerr, fd;	
	db.Initialize(initialString,strerr);



	CString cs="";
	cs=cs+"delete  FROM CallLog WHERE StartTime<#"+cc+"#";	
	db.ExecuteSql(cs,strerr);
	if( !strerr.IsEmpty() )
		AfxMessageBox(strerr);   
}

void CCallLog::pourRecord(CSortListCtrl *plistctrl)
{
	// In Dialog, we have a CSortListCtrl for call log.
	// According to this class method, we will pour all
	// records into this list.
	
	int recordCount=getRecordCount();
	char charSn[10];
	CString strTel;
	CString strURI;
	CString strStartTime;
	CString strEndTime;
	//char charType[10];



	//Add new elements into arrayInfo
	storeAllRecord();

	plistctrl->SetRedraw(FALSE); 

	plistctrl->DeleteAllItems();
	//MessageBox(NULL,"Had delete","Clean Box",MB_OK);
	if (recordCount>0)
	{
		
		for (int i=0;i<recordCount;i++)
		{
			itoa(arrayInfo[i].m_sn,charSn,10);
			strTel=arrayInfo[i].m_Tel;
			strURI=arrayInfo[i].m_URI;
			strStartTime=tranHumenSenseTime(arrayInfo[i].m_StartTime);
			strEndTime=tranHumenSenseTime(arrayInfo[i].m_EndTime);
			//itoa(arrayInfo[i].m_Type,charType,10);
			if (arrayInfo[i].m_Type==1)
			{
				if (arrayInfo[i].m_CallResult==CALLLOG_CALL_RESULT_NORMAL)
					(void)plistctrl->AddItem(0, _T(strTel),_T(strStartTime), _T(strEndTime),_T(strURI) );
				if (arrayInfo[i].m_CallResult==CALLLOG_CALL_RESULT_MISS)
					(void)plistctrl->AddItem(2, _T(strTel),_T(strStartTime), _T(strEndTime),_T(strURI) );
				if (arrayInfo[i].m_CallResult==CALLLOG_CALL_RESULT_REJECT)
					(void)plistctrl->AddItem(3, _T(strTel),_T(strStartTime), _T(strEndTime),_T(strURI) );

			}
			else
			{
				if (arrayInfo[i].m_CallResult==CALLLOG_CALL_RESULT_NORMAL)
					(void)plistctrl->AddItem(1, _T(strTel),_T(strStartTime), _T(strEndTime),_T(strURI) );
				if (arrayInfo[i].m_CallResult==CALLLOG_CALL_RESULT_FAIL)
					(void)plistctrl->AddItem(4, _T(strTel),_T(strStartTime), _T(strEndTime),_T(strURI) );
			}
			plistctrl->SetItemData(i,arrayInfo[i].m_sn);
		}
	}
	plistctrl->SetRedraw(TRUE); 
	plistctrl->Invalidate();
	plistctrl->UpdateWindow();


}

void CCallLog::deleteAllItem()
{
	CDBAccess db;
	CString strerr, fd,cs;	
	db.Initialize(initialString,strerr);
	cs=cs+"delete  FROM CallLog ";	
	db.ExecuteSql(cs,strerr);
	if( !strerr.IsEmpty() )
		AfxMessageBox(strerr);   
}

void CCallLog::deleteItem(int sn)
{
	CDBAccess db;
	CString strerr, fd;	
	db.Initialize(initialString,strerr);
	CString cs;
	cs.Format("delete FROM CallLog WHERE sn=%d",sn);
	db.ExecuteSql(cs,strerr);
	if( !strerr.IsEmpty() )
		AfxMessageBox(strerr);   
}
