///////////////////////////////////////////////////////////////
//
// Dosya Adý: BitmapProgress.h
// Yazan    : Cüneyt ELÝBOL
// Açýklama : Resimden Sayý göstergesi
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

#if !defined(AFX_BITMAPPROGRESS_H__EABB62C0_3AF3_11D4_8168_A2B249DE1E73__INCLUDED_)
#define AFX_BITMAPPROGRESS_H__EABB62C0_3AF3_11D4_8168_A2B249DE1E73__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// BitmapProgress.h : header file
//
#include "BitmapBtn.h"

/////////////////////////////////////////////////////////////////////////////
// CBitmapProgress window

class CBitmapProgress : public CBitmapBtn
{
// Construction
public:
	BOOL m_Horizantal;
	CBitmapProgress();
	void SetBitmap(CBitmap& mZero, CBitmap& mFull);
// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBitmapProgress)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CBitmapProgress();
	UINT GetPos() { return m_Pos; }
	void SetPos(UINT m_newPos);
	// Generated message map functions
protected:
	BOOL m_MouseOnProgress;
	//{{AFX_MSG(CBitmapProgress)
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
private:
	UINT m_Pos;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BITMAPPROGRESS_H__EABB62C0_3AF3_11D4_8168_A2B249DE1E73__INCLUDED_)
