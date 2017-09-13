///////////////////////////////////////////////////////////////
//
// Dosya Adý: BitmapSlider.h
// Yazan    : Cüneyt ELÝBOL
// Açýklama : Resimden Trackbar oluþturma
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

#if !defined(AFX_BITMAPSLIDER_H__5B9A5F8F_2016_11D4_8166_C1E4D6E4D62B__INCLUDED_)
#define AFX_BITMAPSLIDER_H__5B9A5F8F_2016_11D4_8166_C1E4D6E4D62B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// BitmapSlider.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CBitmapSlider window

class CBitmapSlider : public CSliderCtrl
{
protected:
	CBitmap m_Back, m_ThumbNormal, m_ThumbDown;
// Construction
public:
	CBitmapSlider();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBitmapSlider)
	//}}AFX_VIRTUAL

// Implementation
public:
	void SetBitmap(CBitmap& mBack, CBitmap& mTNormal, CBitmap& mTDown);
	virtual ~CBitmapSlider();

	// Generated message map functions
protected:
	void getThumbRect(CRect& r);
	BOOL m_MouseOnThumb;
	//{{AFX_MSG(CBitmapSlider)
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnCaptureChanged(CWnd *pWnd);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BITMAPSLIDER_H__5B9A5F8F_2016_11D4_8166_C1E4D6E4D62B__INCLUDED_)
