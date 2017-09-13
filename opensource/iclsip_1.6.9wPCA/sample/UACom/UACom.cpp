// UACom.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f UAComps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "UACom.h"

#include "UACom_i.c"
#include "UAControl.h"


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_UAControl, CUAControl)
END_OBJECT_MAP()

class CUAComApp : public CWinApp
{
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUAComApp)
	public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CUAComApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CUAComApp, CWinApp)
	//{{AFX_MSG_MAP(CUAComApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CUAComApp theApp;


// sam: memory leak debugging
#ifdef _DEBUG

#include <crtdbg.h>
int _mem_hook( int allocType, void* userData, size_t size, int blockType, 
			   long requestNumber, const unsigned char* filename, int lineNumber)
{
	if ( size == 24)
		return TRUE;
	return TRUE;
}

#endif


BOOL CUAComApp::InitInstance()
{
	// sam: memory leak debugging
#ifdef _DEBUG
	//_CrtSetAllocHook( _mem_hook);
#endif

    _Module.Init(ObjectMap, m_hInstance, &LIBID_UACOMLib);

    SetRegistryKey( _T("CCL, ITRI") );

    return CWinApp::InitInstance();
}

int CUAComApp::ExitInstance()
{
    _Module.Term();
    return CWinApp::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    return (AfxDllCanUnloadNow()==S_OK && _Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer(TRUE);
}
