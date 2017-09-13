// Personal.h: interface for the CPersonal class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PERSONAL_H__D22D2F04_0BCE_4638_869A_FA8B30079079__INCLUDED_)
#define AFX_PERSONAL_H__D22D2F04_0BCE_4638_869A_FA8B30079079__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "dbaccess.h"

typedef struct i_InfoPersonal
{
	int m_sn;
	int m_class;
	CString m_name;
	CString m_id;
	CString m_division;
	CString m_department;
	CString m_telephone;
	CString m_remark;
	CString m_pic;
}InfoPersonal;

class CPersonal  
{
public:
	int AddDB(InfoPersonal *info);
	void ModifyDB(InfoPersonal* info);
	CString projectpath;
	CPersonal();
	virtual ~CPersonal();

private:
	CString tranStr(CString str);
	CString initialString;
};

#endif // !defined(AFX_PERSONAL_H__D22D2F04_0BCE_4638_869A_FA8B30079079__INCLUDED_)
