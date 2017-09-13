// Pack.h: interface for the CPack class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PACK_H__0A5BB711_F10E_45E1_B18F_8217E72BADD4__INCLUDED_)
#define AFX_PACK_H__0A5BB711_F10E_45E1_B18F_8217E72BADD4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <afxtempl.h>
#include "personal.h"
class CPack  
{
private:

	CString mDestFile;
	CString mTitle;
public:
	int CSVImport(CString sSource);
	CString TranHalfComma(CString sData);
	CString TranFullComma(CString sData);
	int CSVExport(CString sDest);
	CArray<InfoPersonal,InfoPersonal> CSVArray;
	CString UnpackPath;
	CString SourceFilePath;
	bool CleanTemp();
	CString projectpath;
	CArray<CString,CString>ArrayFileList;  // record output filename
	bool CheckRemainLength(CFile *cfile);
	void AddPack(CString SourthPath);
	int  UnPack(CString SourceFilePath);
	void SetTitle(CString title);
	
	void CreatePack(CString SourthPath,CString DestPath);
	CPack();
	virtual ~CPack();

};

#endif // !defined(AFX_PACK_H__0A5BB711_F10E_45E1_B18F_8217E72BADD4__INCLUDED_)
