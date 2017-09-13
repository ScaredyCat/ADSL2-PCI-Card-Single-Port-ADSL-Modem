// PCAUA.h : main header file for the PCAUA application
//

#if !defined(AFX_PCAUA_H__F9FF125D_4248_4BC2_B8CB_B6B501275BBE__INCLUDED_)
#define AFX_PCAUA_H__F9FF125D_4248_4BC2_B8CB_B6B501275BBE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols
#include "PCAUA_i.h"

/////////////////////////////////////////////////////////////////////////////
// CPCAUAApp:
// See PCAUA.cpp for the implementation of this class
//


class CPCAUAApp : public CWinApp
{
public:
	CPCAUAApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPCAUAApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
	HMODULE m_hSkinResource;
	CString m_strAppPath;

	//{{AFX_MSG(CPCAUAApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	BOOL m_bATLInited;
	BOOL InitATL();
	BOOL IsAlreadyRunning();

#ifdef	_PCAUA_Res_
	HINSTANCE m_hInstRes;
#endif
};


extern CPCAUAApp theApp;


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PCAUA_H__F9FF125D_4248_4BC2_B8CB_B6B501275BBE__INCLUDED_)
