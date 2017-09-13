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
#include "SkinSlider.h"
#include "SkinDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSkinSlider

BEGIN_MESSAGE_MAP(CSkinSlider, CBitmapSlider)
	//{{AFX_MSG_MAP(CSkinSlider)
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSkinSlider message handlers

void CSkinSlider::CopyFrom(CRect r, CBitmap& m_N, LPCTSTR m_TN, LPCTSTR m_TD)
{
	CDC* dc = GetDC();
	CSize mSz;
	
	CopyBitmap(dc, m_Back, m_N, r);

	LoadPictureFile((HDC) dc, m_TN, &m_ThumbNormal, mSz);
	LoadPictureFile((HDC) dc, m_TD, &m_ThumbDown, mSz);
	ModifyStyle(WS_BORDER | WS_TABSTOP, 0);
	m_Scrolling = false;
	ReleaseDC(dc);
}

void CSkinSlider::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	// TODO: Add your message handler code here and/or call default
	CBitmapSlider::OnHScroll(nSBCode, nPos, pScrollBar);
	CSkinDialog* m_Dlg = (CSkinDialog*) GetParent();
	m_Dlg->ChangeTrack(m_ID, nSBCode, nPos);
}

void CSkinSlider::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	// TODO: Add your message handler code here and/or call default
	CBitmapSlider::OnVScroll(nSBCode, nPos, pScrollBar);
	CSkinDialog* m_Dlg = (CSkinDialog*) GetParent();
	m_Dlg->ChangeTrack(m_ID, nSBCode, nPos);
}

void CSkinSlider::OnLButtonUp(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	m_Scrolling = false;
	CSkinDialog* m_Dlg = (CSkinDialog*) GetParent();
	m_Dlg->ChangeTrack(m_ID, SB_ENDSCROLL, GetPos());
	CBitmapSlider::OnLButtonUp(nFlags, point);
}

void CSkinSlider::OnMouseMove(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	CBitmapSlider::OnMouseMove(nFlags, point);
	CSkinDialog* m_Dlg = (CSkinDialog*) GetParent();
	if(m_Scrolling)
		m_Dlg->ChangeTrack(m_ID, SB_THUMBPOSITION, GetPos());
	else
		m_Dlg->MouseMoved(m_IDName, point.x, point.y);
}

void CSkinSlider::OnLButtonDown(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	CBitmapSlider::OnLButtonDown(nFlags, point);
	m_Scrolling = true;
}
