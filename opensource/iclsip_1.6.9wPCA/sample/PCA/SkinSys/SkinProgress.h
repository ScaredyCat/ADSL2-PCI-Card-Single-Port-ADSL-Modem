///////////////////////////////////////////////////////////////
//
// Dosya Ad�: SkinProgress
// Yazan    : C�neyt EL�BOL
// A��klama : SkinSys i�in y�zde G�stergesi
// 
// Detayl� Bilgi i�in 
//       
//    www.celibol.freeservers.com  adresini ziyaret edin
//            veya
//    celibol@hotmail.com adresine mesaj at�n.
//
// Dikkat:
//    Bu program kodlar�n� kullan�rken Aciklama.txt dosyas�ndaki
//  gerekleri yerine getirmeniz gerekir.
//
///////////////////////////////////////////////////////////////
#if !defined(AFX_SKINPROGRESS_H__EABB62C1_3AF3_11D4_8168_A2B249DE1E73__INCLUDED_)
#define AFX_SKINPROGRESS_H__EABB62C1_3AF3_11D4_8168_A2B249DE1E73__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SkinProgress.h : header file
//
#include "BitmapProgress.h"

/////////////////////////////////////////////////////////////////////////////
// CSkinProgress window

class CSkinProgress : public CBitmapProgress
{
// Construction
public:
	CSkinProgress();

// Attributes
public:
	CString m_IDName;
	int m_ID;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSkinProgress)
	//}}AFX_VIRTUAL

// Implementation
public:
	void CopyFrom(CRect r, CBitmap& m_N, CBitmap& m_Dw);
	void CopyFrom2(CRect r, CBitmap& m_N, CBitmap& m_Dw);
	virtual ~CSkinProgress();

	// Generated message map functions
protected:
	//{{AFX_MSG(CSkinProgress)
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SKINPROGRESS_H__EABB62C1_3AF3_11D4_8168_A2B249DE1E73__INCLUDED_)
