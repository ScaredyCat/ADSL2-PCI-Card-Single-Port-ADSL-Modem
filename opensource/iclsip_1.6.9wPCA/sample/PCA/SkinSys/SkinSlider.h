///////////////////////////////////////////////////////////////
//
// Dosya Adý: SkinSlider
// Yazan    : Cüneyt ELÝBOL
// Açýklama : SkinSys için Trackbar
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
#if !defined(AFX_SKINSLIDER_H__ED575DA3_2279_11D4_8166_E55A610627F6__INCLUDED_)
#define AFX_SKINSLIDER_H__ED575DA3_2279_11D4_8166_E55A610627F6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <afxwin.h>
#include <afxcmn.h>

#include "BitmapSlider.h"

// SkinSlider.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSkinSlider window

class CSkinSlider : public CBitmapSlider
{
// Construction
public:
// Attributes
public:
	CString m_IDName;
	int m_ID;

	void CopyFrom(CRect r, CBitmap& m_N, LPCTSTR m_TN, LPCTSTR m_TD);
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSkinSlider)
	//}}AFX_VIRTUAL

// Implementation
public:
	// Generated message map functions
protected:
	//{{AFX_MSG(CSkinSlider)
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
private:
	BOOL m_Scrolling;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SKINSLIDER_H__ED575DA3_2279_11D4_8166_E55A610627F6__INCLUDED_)
