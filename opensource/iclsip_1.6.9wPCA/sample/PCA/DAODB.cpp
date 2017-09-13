// DAODB.cpp : implementation file
//

#include "stdafx.h"
#include "PCAUA.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDaoDB

IMPLEMENT_DYNAMIC(CDaoDB, CDaoRecordset)

CDaoDB::CDaoDB(CDaoDatabase* pdb)
	: CDaoRecordset(pdb)
{
	//{{AFX_FIELD_INIT(CDaoDB)
	m_sn = 0;
	m_class = 0;
	m_name = _T("");
	m_id = _T("");
	m_division = _T("");
	m_department = _T("");
	m_telephone = _T("");
	m_remark = _T("");
	m_pic = _T("");
	m_nFields = 9;
	//}}AFX_FIELD_INIT
	m_nDefaultType = dbOpenDynaset;
}


CString CDaoDB::GetDefaultDBName()
{

	// get location where AP running
	CString projectpath;
	GetModuleFileName( GetModuleHandle(NULL), projectpath.GetBuffer(1024), 1024);
	projectpath.ReleaseBuffer();

	//delete file name, keep file path
	int length_projectpath=projectpath.GetLength();
	int target_projectpath=projectpath.ReverseFind('\\');
	projectpath.Delete(target_projectpath,length_projectpath-target_projectpath);

	
	//define database location
	projectpath=projectpath+"\\PCADB.mdb";



	return _T(projectpath);
}

CString CDaoDB::GetDefaultSQL()
{
	return _T("[personal]");
}

void CDaoDB::DoFieldExchange(CDaoFieldExchange* pFX)
{
	//{{AFX_FIELD_MAP(CDaoDB)
	pFX->SetFieldType(CDaoFieldExchange::outputColumn);
	DFX_Long(pFX, _T("[sn]"), m_sn);
	DFX_Short(pFX, _T("[class]"), m_class);
	DFX_Text(pFX, _T("[name]"), m_name);
	DFX_Text(pFX, _T("[id]"), m_id);
	DFX_Text(pFX, _T("[division]"), m_division);
	DFX_Text(pFX, _T("[department]"), m_department);
	DFX_Text(pFX, _T("[telephone]"), m_telephone);
	DFX_Text(pFX, _T("[remark]"), m_remark);
	DFX_Text(pFX, _T("[pic]"), m_pic);
	//}}AFX_FIELD_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CDaoDB diagnostics

#ifdef _DEBUG
void CDaoDB::AssertValid() const
{
	CDaoRecordset::AssertValid();
}

void CDaoDB::Dump(CDumpContext& dc) const
{
	CDaoRecordset::Dump(dc);
}
#endif //_DEBUG
