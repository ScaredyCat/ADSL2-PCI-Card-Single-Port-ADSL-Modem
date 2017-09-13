// QueryDB.h: interface for the CQueryDB class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_QUERYDB_H__3B8DD963_DB60_4BE4_824C_00F96CD1DE67__INCLUDED_)
#define AFX_QUERYDB_H__3B8DD963_DB60_4BE4_824C_00F96CD1DE67__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "PanelDlg.h"

class CQueryDB  
{
public:
	CString m_pic_short;
	CString projectpath;
	bool SearchTel(CString strTel);
	//InfoDB infoDB;
	bool SearchSn(CString strSn);
	bool SearchSn(int intSn);
	bool SearchId(CString strID);
	int m_sn;
	int m_class;
	CString m_name;
	CString m_id;
	CString m_division;
	CString m_department;
	CString m_telephone;
	CString m_remark;
	CString m_pic;
	CQueryDB();
	virtual ~CQueryDB();


};

#endif // !defined(AFX_QUERYDB_H__3B8DD963_DB60_4BE4_824C_00F96CD1DE67__INCLUDED_)
