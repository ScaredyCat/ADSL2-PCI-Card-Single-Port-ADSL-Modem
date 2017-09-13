// DlgInfoTest.cpp : implementation file
//

#include "stdafx.h"
#include "pcaua.h"
#include "DlgInfoTest.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgInfoTest dialog


CDlgInfoTest::CDlgInfoTest(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgInfoTest::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgInfoTest)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDlgInfoTest::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgInfoTest)
	DDX_Text(pDX, IDC_TARGET_URI, m_strTargetURI);
	DDX_Text(pDX, IDC_INFO_BODY, m_strInfoBody);
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgInfoTest, CDialog)
	//{{AFX_MSG_MAP(CDlgInfoTest)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgInfoTest message handlers
