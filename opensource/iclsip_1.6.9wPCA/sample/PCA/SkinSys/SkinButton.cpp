///////////////////////////////////////////////////////////////
//
// Dosya Ad? SkinButton
// Yazan    : Cüneyt ELÝBOL
// Açýklama : SkinSys için Button
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

#include "SkinButton.h"
#include "SkinDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSkinButton

BEGIN_MESSAGE_MAP(CSkinButton, CBitmapBtn)
	//{{AFX_MSG_MAP(CSkinButton)
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSkinButton message handlers

void CSkinButton::CopyFrom(CRect r, CBitmap& m_N, CBitmap& m_O, CBitmap& m_Dw, CBitmap& m_Ds)
{
	//CBitmap m_Normal, m_Over, m_Down, m_Disabled;
	//void CopyBitmap(CDC* dc, CBitmap& mRes, const CBitmap& hbmp, RECT r);
	CDC* dc = GetDC();
	
	CopyBitmap(dc, m_Normal, m_N, r);
	CopyBitmap(dc, m_Over, m_O, r);
	CopyBitmap(dc, m_Down, m_Dw, r);
	CopyBitmap(dc, m_Disabled, m_Ds, r);
	ModifyStyle(WS_BORDER | WS_TABSTOP , 0);

		
	ReleaseDC(dc);
}

void CSkinButton::CopyFrom2(CRect r, CBitmap& m_N, CBitmap& m_O, CBitmap& m_Dw, CBitmap& m_Ds)
{
	CDC* dc = GetDC();
	
	CopyBitmap(dc, m_Normal2, m_N, r);
	CopyBitmap(dc, m_Over2, m_O, r);
	CopyBitmap(dc, m_Down2, m_Dw, r);
	CopyBitmap(dc, m_Disabled2, m_Ds, r);
	ModifyStyle(WS_BORDER | WS_TABSTOP , 0);
	ReleaseDC(dc);
}

void CSkinButton::OnLButtonUp(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	CRect r;
	GetClientRect(&r);
	if(r.PtInRect(point))
	{
		m_MouseOnButton = FALSE;
		Invalidate();
		CBitmapBtn::OnLButtonUp(nFlags, point);

		CSkinDialog* m_Dlg = (CSkinDialog*) GetParent();
		m_Dlg->PressButton(m_ID);
	}
	else
		CBitmapBtn::OnLButtonUp(nFlags, point);
}

void CSkinButton::OnMouseMove(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	CSkinDialog* m_Dlg = (CSkinDialog*) GetParent();
	m_Dlg->MouseMoved(m_IDName, point.x, point.y);
	//TRACE("btn mouse moved %s\n", m_IDName);
	CBitmapBtn::OnMouseMove(nFlags, point);
}

