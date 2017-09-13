#ifndef _DB_ACCESS
#define _DB_ACCESS

#define INITGUID
#import "c:\Program Files\Common Files\System\ADO\msado15.dll" \
   no_namespace rename("EOF", "EndOfFile")
#include "icrsint.h"

class  CDBAccess
{
public:
	int GetIntFromField(INT fIndex);
	void CloseConnection();
	bool GetboolFromField(int fIndex);
	bool GetCOleDateTimeFromField(int fIndex,COleDateTime* oleTime);
	void SetTimeOut( int cmdtmout, int conntmout );
	BOOL OpenRecordSetFWDO(const CHAR *sqlString, CString &errStr);
	int GetFieldCount();

    CDBAccess();
	~CDBAccess();
	BOOL Initialize(const CHAR* connString, CString &errStr);

	//ado operation 
	BOOL   ExecuteSql(const CHAR *sqlString, CString &errStr);
	BOOL   OpenRecordSet(const CHAR *sqlString, CString &errStr);
	VOID   CloseRecordSet();
	LONG   GetRecordCount();
	BOOL   MoveNext();
	BOOL   IsEOF();
		
	VOID   GetStringFromField(INT fIndex,CHAR *outString, int bufsize);
	DWORD  GetIntegerFromField(INT fIndex);

private:
	
	BOOL mDBConnStatus;
	BOOL mRsClosed;
	int m_CmdTimeout, m_ConnTimeout;

	_ConnectionPtr  mConn;
	_RecordsetPtr   mRs;
};

#endif

