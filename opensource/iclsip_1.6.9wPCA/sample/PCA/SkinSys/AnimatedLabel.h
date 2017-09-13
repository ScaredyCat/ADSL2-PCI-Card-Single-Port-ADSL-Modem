///////////////////////////////////////////////////////////////
//
// Dosya Ad? AnimatedLabel.h
// Yazan    : Cüneyt ELÝBOL
// Açýklama : Kayan yaz?kodu
// 
// Detayl?Bilgi için 
//       
//    www.celibol.freeservers.com  adresini ziyaret edin
//            veya
//    celibol@hotmail.com adresine mesaj atýn.
//
// Dikkat:
//    Bu program kodlarýn?kullanýrken Aciklama.txt dosyasýndaki
//  gerekleri yerine getirmeniz gerekir.
//
///////////////////////////////////////////////////////////////

#if !defined(AFX_ANIMATEDLABEL_H__5B9A5F97_2016_11D4_8166_C1E4D6E4D62B__INCLUDED_)
#define AFX_ANIMATEDLABEL_H__5B9A5F97_2016_11D4_8166_C1E4D6E4D62B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SkinLabel.h : header file
//
#include <afxwin.h>
#include <afxcmn.h>
#include "BitmapBtn.H"

/////////////////////////////////////////////////////////////////////////////
// CAnimatedLabel window

class CAnimatedLabel : public CBitmapBtn //CStatic
{
	int m_Strpos, m_MaxText;
	CString m_Label;
// Construction
public:
	CAnimatedLabel();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAnimatedLabel)
	//}}AFX_VIRTUAL

// Implementation
public:
	COLORREF m_TextColor;
	int m_textAlign;
	void SetBitmap(CBitmap& mBack);
	void SetFont(CFont& mNewFont);
	void SetText(CString mNewText);
	virtual ~CAnimatedLabel();

	bool GetMode2()				{ return m_bUseMode2; }
	void SetMode2( bool b)		{ m_bUseMode2 =b; }

	/*
	 *  Hide Data API, 2006/06/29
	 */
	BOOL m_bHideData;
	void SetHideData();
	void SetShowData();
	


	// Generated message map functions
protected:
	CBitmap m_Back;
	void UpdateTimer();
	BOOL m_Timered;
	CFont m_Font;

	bool m_bUseMode2;
	CBitmap m_Back2;

	CBitmap& GetBackBmp()		{ return m_bUseMode2? m_Back2 : m_Back; }

	//{{AFX_MSG(CAnimatedLabel)
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnPaint();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ANIMATEDLABEL_H__5B9A5F97_2016_11D4_8166_C1E4D6E4D62B__INCLUDED_)
