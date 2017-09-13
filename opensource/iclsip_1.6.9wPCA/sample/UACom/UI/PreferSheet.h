#if !defined(AFX_PREFERSHEET_H__593D095F_9B96_48AA_A7A8_9D90D7651571__INCLUDED_)
#define AFX_PREFERSHEET_H__593D095F_9B96_48AA_A7A8_9D90D7651571__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PreferSheet.h : header file
//
#include "stdafx.h"
#include "UserPage.h"
#include "ServerPage.h"
//#ifdef _simple
//#include "SimplePage.h"
//#endif
#include "AudioPage.h"
#ifdef _FOR_VIDEO
#include "VideoPage.h"
#endif
#include "CredentialPage.h"
#include "RTPPage.h"
#include "SIPExtPage.h"

#include "TreePropSheet.h"

/////////////////////////////////////////////////////////////////////////////
// CPreferSheet

using namespace TreePropSheet;

class CPreferSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CPreferSheet)

// Construction
public:
	CPreferSheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CPreferSheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:
	CUserPage		m_UserPage;
	CServerPage		m_ServerPage;
//#ifdef _simple
//	CSimplePage		m_SimplePage;
//#endif
	CAudioPage		m_AudioPage;
#ifdef _FOR_VIDEO
	CVideoPage		m_VideoPage;
#endif
	CCredentialPage m_CredentialPage;
	CRTPPage		m_RTPPage;
	SIPExtPage		m_SIPExtPage;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPreferSheet)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CPreferSheet();

	// Generated message map functions
protected:
	//{{AFX_MSG(CPreferSheet)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PREFERSHEET_H__593D095F_9B96_48AA_A7A8_9D90D7651571__INCLUDED_)
