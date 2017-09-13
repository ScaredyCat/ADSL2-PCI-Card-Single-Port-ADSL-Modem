// PCAUARes.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "PCAUARes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
//	Note!
//
//		If this DLL is dynamically linked against the MFC
//		DLLs, any functions exported from this DLL which
//		call into MFC must have the AFX_MANAGE_STATE macro
//		added at the very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

/////////////////////////////////////////////////////////////////////////////
// CPCAUAResApp

BEGIN_MESSAGE_MAP(CPCAUAResApp, CWinApp)
	//{{AFX_MSG_MAP(CPCAUAResApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPCAUAResApp construction

CPCAUAResApp::CPCAUAResApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CPCAUAResApp object

CPCAUAResApp theApp;

void LoadPictureRes_( LPCTSTR szResName, CBitmap* pBitmap, CSize& mSize)
{
	pBitmap->DeleteObject();
	pBitmap->LoadBitmap(szResName);

	BITMAP bm;
	GetObject(pBitmap->m_hObject, sizeof(bm), &bm);
	mSize.cx = bm.bmWidth; //nWidth;
	mSize.cy = bm.bmHeight; //nHeight;
}

void LoadBitmap_(CBitmap* pBitmap, UINT nResID)
{
	pBitmap->LoadBitmap(nResID);
}

BOOL CreateImageList_(CImageList* pImageList, UINT nBitmapID, int cx, int nGrow, COLORREF crMask)
{
	return pImageList->Create(nBitmapID, cx, nGrow, crMask);
}

BOOL Create_(CDialog* pDlg, UINT nResID)
{
	return pDlg->Create(nResID);
}

LPCTSTR MAKEINTRESOURCE_(UINT nIDTemplate)
{
	return MAKEINTRESOURCE(nIDTemplate);
}