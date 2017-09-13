// PreferSheet.cpp : implementation file
//

#include "PreferSheet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPreferSheet

IMPLEMENT_DYNAMIC(CPreferSheet, CTreePropSheet)

CPreferSheet::CPreferSheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
	AddPage(&m_UserPage);
	AddPage(&m_ServerPage);
	AddPage(&m_CredentialPage);
//#ifdef _simple
//	AddPage(&m_SimplePage);
//#endif
	AddPage(&m_AudioPage);
#ifdef _FOR_VIDEO
	AddPage(&m_VideoPage);
#endif
	AddPage(&m_RTPPage);
	AddPage(&m_SIPExtPage);
}

CPreferSheet::CPreferSheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
	AddPage(&m_UserPage);
	AddPage(&m_ServerPage);
	AddPage(&m_CredentialPage);
//#ifdef _simple
//	AddPage(&m_SimplePage);
//#endif
	AddPage(&m_AudioPage);
#ifdef _FOR_VIDEO
	AddPage(&m_VideoPage);
#endif
	AddPage(&m_RTPPage);
	AddPage(&m_SIPExtPage);
}

CPreferSheet::~CPreferSheet()
{
}


BEGIN_MESSAGE_MAP(CPreferSheet, CPropertySheet)
	//{{AFX_MSG_MAP(CPreferSheet)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPreferSheet message handlers
