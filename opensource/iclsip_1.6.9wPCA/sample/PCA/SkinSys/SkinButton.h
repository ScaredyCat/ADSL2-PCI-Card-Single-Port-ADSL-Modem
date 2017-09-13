///////////////////////////////////////////////////////////////
//
// Dosya Adý: SkinButton
// Yazan    : Cüneyt ELÝBOL
// Açýklama : SkinSys için Button
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


#if !defined(AFX_SKINBUTTON_H__ED575DA0_2279_11D4_8166_E55A610627F6__INCLUDED_)
#define AFX_SKINBUTTON_H__ED575DA0_2279_11D4_8166_E55A610627F6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <afxwin.h>
#include <afxcmn.h>

#include "BitmapBtn.H"

// SkinButton.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSkinButton window

class CSkinButton : public CBitmapBtn
{
// Construction
public:
// Attributes
	CString m_IDName;
	int m_ID;
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSkinButton)
	//}}AFX_VIRTUAL

// Implementation
public:
	void CopyFrom(CRect r, CBitmap& m_N, CBitmap& m_O, CBitmap& m_Dw, CBitmap& m_Ds);
	void CopyFrom2(CRect r, CBitmap& m_N, CBitmap& m_O, CBitmap& m_Dw, CBitmap& m_Ds);

	// Generated message map functions
protected:
	//{{AFX_MSG(CSkinButton)
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SKINBUTTON_H__ED575DA0_2279_11D4_8166_E55A610627F6__INCLUDED_)
