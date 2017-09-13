///////////////////////////////////////////////////////////////
//
// Dosya Adý: Inifile.h
// Yazan    : Cüneyt ELÝBOL
// Açýklama : *.Ini Dosyalarý için Arabirim
// 
// Detaylý Bilgi için 
//       
//    www.celibol.freeservers.com  adresini ziyaret edin
//            veya
//    celibol@hotmail.com adresine mesaj atýn.
//
// Dikkat:
//    Bu program kodlarýný kullanýrken Aciklama.txt dosyasýndaki
//  gerekleri yerine getirmeniz gerekir.
//
///////////////////////////////////////////////////////////////


#ifndef __CINIFILE_H__
#define __CINIFILE_H__

class CIniFile
{
	CString m_Name;
public:
	CIniFile(CString m_FName);

	CString ReadString(CString m_Sec, CString m_Ident, CString m_Def);
	BOOL WriteString(CString m_Sec, CString m_Ident, CString m_Val);
	BOOL ReadSections(CStringArray& m_Secs);
	BOOL ReadSection(CString m_Sec, CStringArray& m_Secs);
};

#endif