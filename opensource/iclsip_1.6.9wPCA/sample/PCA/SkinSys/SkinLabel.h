///////////////////////////////////////////////////////////////
//
// Dosya Adý: SkinLabel
// Yazan    : Cüneyt ELÝBOL
// Açýklama : SkinSys için Ekran Yazýlarý
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

#if !defined(AFX_SKINLABEL_H__45D9BC80_22A6_11D4_8166_F8D7D7B25646__INCLUDED_)
#define AFX_SKINLABEL_H__45D9BC80_22A6_11D4_8166_F8D7D7B25646__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SkinLabel.h : header file
//
#include "AnimatedLabel.H"

/////////////////////////////////////////////////////////////////////////////
// CSkinLabel window

class CSkinLabel : public CAnimatedLabel
{
// Construction
public:
	CString m_IDName;
	int m_ID;

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSkinLabel)
	//}}AFX_VIRTUAL

// Implementation
public:
	void CopyFrom(CRect r, CBitmap& m_B);
	void CopyFrom2(CRect r, CBitmap& m_B);
	// Generated message map functions
protected:
	//{{AFX_MSG(CSkinLabel)
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SKINLABEL_H__45D9BC80_22A6_11D4_8166_F8D7D7B25646__INCLUDED_)
