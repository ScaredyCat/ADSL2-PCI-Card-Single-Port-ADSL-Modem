// CallConnectionInfo.h: interface for the CCallConnectionInfo class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CALLCONNECTIONINFO_H__C66D9A3F_3215_4DC0_902E_F416B56FE41C__INCLUDED_)
#define AFX_CALLCONNECTIONINFO_H__C66D9A3F_3215_4DC0_902E_F416B56FE41C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CCallConnectionInfo  
{
	int DlgHandle;
	int status;
	CString telNo;
	CString name;
	CString company;
	CString photoPath;

public:
	CCallConnectionInfo();
	virtual ~CCallConnectionInfo();

};

#endif // !defined(AFX_CALLCONNECTIONINFO_H__C66D9A3F_3215_4DC0_902E_F416B56FE41C__INCLUDED_)
