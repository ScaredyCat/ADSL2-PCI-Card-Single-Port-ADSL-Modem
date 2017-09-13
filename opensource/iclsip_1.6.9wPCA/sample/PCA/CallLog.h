// CallLog.h: interface for the CCallLog class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CALLLOG_H__84AF12BC_0E9F_4CA5_814B_E37C2513E5EA__INCLUDED_)
#define AFX_CALLLOG_H__84AF12BC_0E9F_4CA5_814B_E37C2513E5EA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "dbaccess.h"
#include "SortListCtrl.h"

#define CALLLOG_CALL_RESULT_NORMAL	0
#define CALLLOG_CALL_RESULT_MISS	1
#define CALLLOG_CALL_RESULT_FAIL	2
#define CALLLOG_CALL_RESULT_REJECT	3


typedef struct i_InfoCallLog
{
	int m_sn;	
	CString m_Tel;
	CString m_URI;
	COleDateTime m_StartTime;
	COleDateTime m_EndTime;
	int m_Type;	
	int m_CallResult;
	bool m_CallBack;
}InfoCallLog;
class CCallLog  
{
public:
	void deleteItem(int sn);
	void deleteAllItem();
	void pourRecord(CSortListCtrl *plistctrl);
	void deleteItem(COleDateTime timeDelete);
	CString tranHumenSenseTime(COleDateTime oleDateTime);
	void getItemInfo(InfoCallLog *info, int itemNum);
	void insertItem(CString tel, CString URI, COleDateTime StartTime, COleDateTime EndTime, int type, int CallResult, BOOL CallBack);
	void storeAllRecord();
	
	int getRecordCount();
	CCallLog();
	CArray<InfoCallLog,InfoCallLog>arrayInfo;
	virtual ~CCallLog();

private:
	CString projectpath;
	CString initialString;
};

#endif // !defined(AFX_CALLLOG_H__84AF12BC_0E9F_4CA5_814B_E37C2513E5EA__INCLUDED_)
