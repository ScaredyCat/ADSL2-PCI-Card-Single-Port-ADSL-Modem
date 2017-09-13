#include "stdafx.h"
#include "dbaccess.h"



CDBAccess::CDBAccess()
{
	::CoInitialize(NULL);

	mConn.CreateInstance(__uuidof(Connection));
	mRs.CreateInstance(__uuidof(Recordset));
	mDBConnStatus = FALSE;
	mRsClosed=TRUE;
	m_CmdTimeout=600;
	m_ConnTimeout=60;

}
	
BOOL CDBAccess::Initialize(const CHAR *connString, CString &errStr)
{
	_bstr_t strCnn(connString);

	try
	{
		mConn->ConnectionTimeout=m_ConnTimeout;
		mConn->Open(strCnn,"","",NULL);
	}
	catch(_com_error &e)
	{
		errStr = (char*)e.Description();

		CString err;
		err = "Database Error ! \n";
		err += errStr;
		AfxMessageBox(err);
		exit(0);

		//HRESULT hr = e.Error();
		//WORD err = e.WCode();
		//CString src = (char*)e.Source();


		return FALSE;
	}

	mDBConnStatus = TRUE;
	mRsClosed=TRUE;
	return TRUE;
}

CDBAccess::~CDBAccess()
{
	if(!mRsClosed)
		mRs->Close();			
		
	if(mDBConnStatus) 
		mConn->Close();

	//::CoUninitialize();
}


BOOL CDBAccess::ExecuteSql(const CHAR *sqlString, CString &errStr)
{
	try
	{
		mConn->CommandTimeout=m_CmdTimeout;
		mConn->Execute(sqlString,NULL,adCmdText);
	}
	catch( _com_error &e )
	{
		errStr = (char*)e.Description();
		return FALSE;
	}

	return TRUE;
}

BOOL CDBAccess::OpenRecordSet(const CHAR *sqlString, CString &errStr)
{

	if(!mRsClosed)
	{
		try
		{
			CloseRecordSet();	
		}
		catch( _com_error &e )
		{
			errStr = (char*)e.Description();
			return FALSE;
		}
	}

	try
	{
		mRs->Open(sqlString,_variant_t((IDispatch *) mConn, true),adOpenStatic, adLockReadOnly, adCmdUnknown);
	}
	catch(_com_error &e)
	{
		errStr = (char*)e.Description();
		return FALSE;
	}

	mRsClosed=FALSE;

	return TRUE;
}

VOID CDBAccess::CloseRecordSet()
{
	mRs->Close();
	mRsClosed=TRUE;
}

BOOL CDBAccess::MoveNext()
{
	try {
		mRs->MoveNext();
	} 
	catch (...) {
		return FALSE;
	}
	return TRUE;
}

LONG CDBAccess::GetRecordCount()
{
	LONG rsCount;
	mRs->get_RecordCount(&rsCount);
	
	return rsCount;
}

BOOL CDBAccess::IsEOF()
{
	if(mRs->AbsolutePosition == adPosEOF)
	{
		return TRUE;
	}

	return FALSE;
}

DWORD CDBAccess::GetIntegerFromField(INT fIndex)
{

	Field   *pField;
	_variant_t   vValue;   

	VARIANT index;
	index.vt=VT_I4;
	index.lVal = fIndex;

	vValue.Clear();

	mRs->Fields->get_Item(index,&pField);
	pField->get_Value(&vValue);

	if ( vValue.vt != VT_NULL) 
		return (DWORD) vValue.lVal;
	else 
		return 0xFFFFFFFF;
}

VOID CDBAccess::GetStringFromField(INT fIndex,CHAR *outString, int bufsize)
{
	Field   *pField;
	_variant_t   vValue;   

	VARIANT index;
	index.vt=VT_I4;
	index.lVal = fIndex;

	vValue.Clear();

	mRs->Fields->get_Item(index,&pField);
	pField->get_Value(&vValue);

	if(vValue.vt == VT_BSTR)
	  WideCharToMultiByte(CP_ACP, 0,vValue.bstrVal, -1,outString,bufsize, NULL, NULL);
	else 
	  strcpy(outString,"");
}


int CDBAccess::GetFieldCount()
{
	long fdcnt;

	mRs->Fields->get_Count(&fdcnt);

	return fdcnt;
}

BOOL CDBAccess::OpenRecordSetFWDO(const CHAR *sqlString, CString &errStr)
{
	if(!mRsClosed)
	{
		try
		{
			CloseRecordSet();	
		}
		catch( _com_error &e )
		{
			errStr = (char*)e.Description();
			return FALSE;
		}
	}

	try
	{
		mRs->Open(sqlString,_variant_t((IDispatch *) mConn, true),adOpenForwardOnly, adLockReadOnly, adCmdUnknown);
	}
	catch(_com_error &e)
	{
		errStr = (char*)e.Description();
		return FALSE;
	}

	mRsClosed=FALSE;

	return TRUE;
}

void CDBAccess::SetTimeOut(int cmdtmout, int conntmout)
{
	m_CmdTimeout = cmdtmout;
	m_ConnTimeout = conntmout;
}

bool CDBAccess::GetCOleDateTimeFromField(int fIndex,COleDateTime* oleTime)
{
	//  example :
	//	COleDateTime myTime;
	//  db.GetCOleDateTimeFromField(1,&myTime);
	//  fIndex is which column you want to retrieve data, base 0


	Field   *pField;
	_variant_t   vValue;   

	VARIANT index;
	index.vt=VT_I4;
	index.lVal = fIndex;

	vValue.Clear();

	mRs->Fields->get_Item(index,&pField);
	pField->get_Value(&vValue);

	// cast vValue into COleDateTime
	COleDateTime getDateTime;
	getDateTime=(COleDateTime)(vValue);

	*oleTime=getDateTime;
	
	if (getDateTime==0)
		return false;
	else
		return true;
	
}

bool CDBAccess::GetboolFromField(int fIndex)
{

	Field   *pField;
	_variant_t   vValue;   

	VARIANT index;
	index.vt=VT_I4;
	index.lVal = fIndex;

	vValue.Clear();

	mRs->Fields->get_Item(index,&pField);
	pField->get_Value(&vValue);



	return (bool)vValue;
}

void CDBAccess::CloseConnection()
{
	mDBConnStatus=false;
	mConn->Close();
}

int CDBAccess::GetIntFromField(INT fIndex)
{
	// In MS Access, if filed style is "integer"(not "long integer"),
	// we will get wrong data after using GetIntegerFromField().
	// So, I use GetIntFromField() instead of GetIntegerFromField().


	Field   *pField;
	_variant_t   vValue;   

	VARIANT index;
	index.vt=VT_I4;
	index.lVal = fIndex;

	vValue.Clear();

	mRs->Fields->get_Item(index,&pField);
	pField->get_Value(&vValue);

	
	return (int)(long)vValue;	

}

