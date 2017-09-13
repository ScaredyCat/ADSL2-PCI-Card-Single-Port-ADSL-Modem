// PCAUARes.h : main header file for the PCAUARES DLL
//

#if !defined(AFX_PCAUARES_H__EC90D06B_3A2F_4E0B_86CA_565E36DAC606__INCLUDED_)
#define AFX_PCAUARES_H__EC90D06B_3A2F_4E0B_86CA_565E36DAC606__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CPCAUAResApp
// See PCAUARes.cpp for the implementation of this class
//

class CPCAUAResApp : public CWinApp
{
public:
	CPCAUAResApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPCAUAResApp)
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CPCAUAResApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

_declspec(dllexport) void LoadPictureRes_( LPCTSTR szResName, CBitmap* pBitmap, CSize& mSize);
_declspec(dllexport) void LoadBitmap_(CBitmap* pBitmap, UINT nResID);

_declspec(dllexport) BOOL CreateImageList_(CImageList* pImageList, UINT nBitmapID, int cx, int nGrow, COLORREF crMask);
_declspec(dllexport) BOOL Create_(CDialog* pDlg, UINT nResID);
_declspec(dllexport) LPCTSTR MAKEINTRESOURCE_(UINT nIDTemplate);

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PCAUARES_H__EC90D06B_3A2F_4E0B_86CA_565E36DAC606__INCLUDED_)
