// EditItri.cpp : implementation file
//

#include "stdafx.h"
#include "PCAUA.h"
#include "EditItriDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditItriDlg dialog


CEditItriDlg::CEditItriDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEditItriDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEditItriDlg)
	m_ITIR2_DEPID = _T("");
	m_ITIR2_EXT = _T("");
	m_ITIR2_ID = _T("");
	m_ITIR2_NAME = _T("");
	m_ITIR2_REMARK = _T("");
	//}}AFX_DATA_INIT
}


void CEditItriDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditItriDlg)
	DDX_Text(pDX, IDC_ITIR2_DEPID, m_ITIR2_DEPID);
	DDX_Text(pDX, IDC_ITIR2_EXT, m_ITIR2_EXT);
	DDX_Text(pDX, IDC_ITIR2_ID, m_ITIR2_ID);
	DDX_Text(pDX, IDC_ITIR2_NAME, m_ITIR2_NAME);
	DDX_Text(pDX, IDC_ITIR2_REMARK, m_ITIR2_REMARK);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEditItriDlg, CDialog)
	//{{AFX_MSG_MAP(CEditItriDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditItriDlg message handlers
