// PCAUA.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "PCAUA.h"
#include "PCAUADlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPCAUAApp

BEGIN_MESSAGE_MAP(CPCAUAApp, CWinApp)
	//{{AFX_MSG_MAP(CPCAUAApp)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPCAUAApp construction

CPCAUAApp::CPCAUAApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CPCAUAApp object

CPCAUAApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CPCAUAApp initialization

// sam: memory leak debugging
#ifdef _DEBUG

#include <crtdbg.h>
int _mem_hook( int allocType, void* userData, size_t size, int blockType, 
			   long requestNumber, const unsigned char* filename, int lineNumber)
{
	if ( size == 264)
		return TRUE;
	return TRUE;
}

#endif
#include <initguid.h>
#include "PCAUA_i.c"

#ifdef	_PCAUA_Res_

#ifdef _DEBUG
#define	PCAUA_RES_DLL	"PCAUAResd.dll"
#else
#define	PCAUA_RES_DLL	"PCAUARes.dll"
#endif
#endif


BOOL CPCAUAApp::InitInstance()
{
	// sam: memory leak debugging
#ifdef _DEBUG
	//_CrtSetAllocHook( _mem_hook);
#endif

	// use RichEdit in this app
	AfxInitRichEdit();

	// force use english resource
	::SetThreadLocale(MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT),SORT_DEFAULT));
	//::SetThreadLocale(MAKELCID(MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_TRADITIONAL),SORT_DEFAULT));

	// get the running module path
	GetModuleFileName( GetModuleHandle(NULL), m_strAppPath.GetBuffer(1024), 1024);
	m_strAppPath.ReleaseBuffer();
	int pos = m_strAppPath.ReverseFind('\\') + 1;
	m_strAppPath.Delete( pos, m_strAppPath.GetLength() - pos);

//t	m_hSkinResource = GetModuleHandle(NULL);
	/*
	// get the external skin resource DLL 
	CString strSkinResDLL = m_strAppPath + "skin.dll";
	m_hSkinResource = LoadLibrary( strSkinResDLL);
	if ( !m_hSkinResource)
	{
		AfxMessageBox( "Fatal error. Skin resource file 'skin.dll' not found.");
		return FALSE;
	}
	*/

	if (!AfxSocketInit())
	{
		AfxMessageBox( "Fatal error. Failed to Initialize Sockets.");
		return FALSE;
	}

	if( !IsAlreadyRunning())
	{
	if (!InitATL())
		return FALSE;

		AfxEnableControlContainer();

	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	if (cmdInfo.m_bRunEmbedded || cmdInfo.m_bRunAutomated)
	{
		return TRUE;
	}



		// Standard initialization
		// If you are not using these features and wish to reduce the size
		//  of your final executable, you should remove from the following
		//  the specific initialization routines you do not need.

	#ifdef _AFXDLL
		Enable3dControls();			// Call this when using MFC in a shared DLL
	#else
		Enable3dControlsStatic();	// Call this when linking to MFC statically
	#endif
		
		// set registry key in HKCU\Software\CCL, ITRI\PCAUA
		SetRegistryKey(_T("CCL, ITRI"));
		free( (void*)m_pszProfileName);
		m_pszProfileName = _strdup( "PCAUA");

#ifdef	_PCAUA_Res_
		m_hInstRes = LoadLibrary(PCAUA_RES_DLL);
		if ( !m_hInstRes)
		{
			m_hInstRes = LoadLibrary( m_strAppPath + PCAUA_RES_DLL);
			if ( !m_hInstRes)
			{
				MessageBox( NULL, 
							"PCAUARes.dll not found.\nPCA SoftPhone Stops.", 
							"PCA SoftPhone Fatal Error", 
							MB_ICONSTOP|MB_OK);
				return FALSE;
			}
		}

		ASSERT( m_hInstRes );
		AfxSetResourceHandle( m_hInstRes );
#endif
		// create main dialog
		CPCAUADlg dlg;
		m_pMainWnd = &dlg;

		// minimized on start if specified /autorun argument
		CString cmd = m_lpCmdLine;
		if( cmd.Find("/autorun") != -1 )
			dlg.m_bMinimizeOnStart=true;

		// show main dialog
		int nResponse = dlg.DoModal();
		if (nResponse == IDOK)
		{
			// TODO: Place code here to handle when the dialog is
			//  dismissed with OK
		}
		else if (nResponse == IDCANCEL)
		{
			// TODO: Place code here to handle when the dialog is
			//  dismissed with Cancel
		}
		else if( nResponse == IDABORT || nResponse == -1 )
		{
			MessageBox( HWND_TOP, "Unable to create SoftPhone dialog.", "Fatal Error", MB_OK | MB_ICONERROR);
		}
	}
	else
	{
		AfxMessageBox( "SoftPhone is already running.");
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

BOOL CPCAUAApp::IsAlreadyRunning()
{
	// check single instance
	CreateMutex(NULL,TRUE,_T("__PCAUA__")); // mutex will be automatically deleted when process ends. 
	
	return (GetLastError() == ERROR_ALREADY_EXISTS); 
}

int CPCAUAApp::ExitInstance() 
{
	// TODO: Add your specialized code here and/or call the base class
#ifdef	_PCAUA_Res_
	FreeLibrary( m_hInstRes );
#endif
	
	if (m_bATLInited)
	{
		_Module.RevokeClassObjects();
		_Module.Term();
	
		CoUninitialize();
	}


	return CWinApp::ExitInstance();
}
	
CPCAUAModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()

LONG CPCAUAModule::Unlock()
{
	AfxOleUnlockApp();
	return 0;
}

LONG CPCAUAModule::Lock()
{
	AfxOleLockApp();
	return 1;
}
LPCTSTR CPCAUAModule::FindOneOf(LPCTSTR p1, LPCTSTR p2)
{
	while (*p1 != NULL)
	{
		LPCTSTR p = p2;
		while (*p != NULL)
		{
			if (*p1 == *p)
				return CharNext(p1);
			p = CharNext(p);
		}
		p1++;
	}
	return NULL;
}



BOOL CPCAUAApp::InitATL()
{
	m_bATLInited = TRUE;

#if _WIN32_WINNT >= 0x0400
	HRESULT hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);
#else
	HRESULT hRes = CoInitialize(NULL);
#endif

	if (FAILED(hRes))
	{
		m_bATLInited = FALSE;
		return FALSE;
	}

	_Module.Init(ObjectMap, AfxGetInstanceHandle());
	_Module.dwThreadID = GetCurrentThreadId();

	LPTSTR lpCmdLine = GetCommandLine(); //this line necessary for _ATL_MIN_CRT
	TCHAR szTokens[] = _T("-/");

	BOOL bRun = TRUE;
	LPCTSTR lpszToken = _Module.FindOneOf(lpCmdLine, szTokens);
	while (lpszToken != NULL)
	{
		if (lstrcmpi(lpszToken, _T("UnregServer"))==0)
		{
			_Module.UpdateRegistryFromResource(IDR_PCAUA, FALSE);
			_Module.UnregisterServer(TRUE); //TRUE means typelib is unreg'd
			bRun = FALSE;
			break;
		}
		if (lstrcmpi(lpszToken, _T("RegServer"))==0)
		{
			_Module.UpdateRegistryFromResource(IDR_PCAUA, TRUE);
			_Module.RegisterServer(TRUE);
			bRun = FALSE;
			break;
		}
		lpszToken = _Module.FindOneOf(lpszToken, szTokens);
	}

	if (!bRun)
	{
		m_bATLInited = FALSE;
		_Module.Term();
		CoUninitialize();
		return FALSE;
	}

	hRes = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER, 
		REGCLS_MULTIPLEUSE);
	if (FAILED(hRes))
	{
		m_bATLInited = FALSE;
		CoUninitialize();
		return FALSE;
	}	

	return TRUE;

}
