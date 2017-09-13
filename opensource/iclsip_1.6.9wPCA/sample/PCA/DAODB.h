#if !defined(AFX_DAODB_H__9FCE4DE5_2A74_4C8A_A64E_E68D516BE171__INCLUDED_)
#define AFX_DAODB_H__9FCE4DE5_2A74_4C8A_A64E_E68D516BE171__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//#include <afxdao.h>
// DAODB.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDaoDB DAO recordset

class CDaoDB : public CDaoRecordset
{
public:
	CDaoDB(CDaoDatabase* pDatabase = NULL);
	DECLARE_DYNAMIC(CDaoDB)

// Field/Param Data
	//{{AFX_FIELD(CDaoDB, CDaoRecordset)
	long	m_sn;
	short	m_class;
	CString	m_name;
	CString	m_id;
	CString	m_division;
	CString	m_department;
	CString	m_telephone;
	CString	m_remark;
	CString	m_pic;
	//}}AFX_FIELD

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDaoDB)
	public:
	virtual CString GetDefaultDBName();		// Default database name
	virtual CString GetDefaultSQL();		// Default SQL for Recordset
	virtual void DoFieldExchange(CDaoFieldExchange* pFX);  // RFX support
	//}}AFX_VIRTUAL

// Implementation
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DAODB_H__9FCE4DE5_2A74_4C8A_A64E_E68D516BE171__INCLUDED_)
