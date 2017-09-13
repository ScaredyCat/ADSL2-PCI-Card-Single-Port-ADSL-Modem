///////////////////////////////////////////////////////////////
//
// Dosya Ad�: SkinLabel
// Yazan    : C�neyt EL�BOL
// A��klama : SkinSys i�in Ekran Yaz�lar�
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

#include "SkinLabel.h"
#include "SkinDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSkinLabel

BEGIN_MESSAGE_MAP(CSkinLabel, CAnimatedLabel)
	//{{AFX_MSG_MAP(CSkinLabel)
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSkinLabel message handlers

void CSkinLabel::CopyFrom(CRect r, CBitmap &m_B)
{
	CDC* dc = GetDC();
	
	CopyBitmap(dc, m_Back, m_B, r);
	ReleaseDC(dc);
}

void CSkinLabel::CopyFrom2(CRect r, CBitmap &m_B)
{
	CDC* dc = GetDC();
	
	CopyBitmap(dc, m_Back2, m_B, r);
	ReleaseDC(dc);
}

void CSkinLabel::OnLButtonUp(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	CAnimatedLabel::OnLButtonUp(nFlags, point);
	CSkinDialog* m_Dlg = (CSkinDialog*) GetParent();
	m_Dlg->ClickText(m_ID);
}

void CSkinLabel::OnMouseMove(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	//CAnimatedLabel::OnMouseMove(nFlags, point);
	CSkinDialog* m_Dlg = (CSkinDialog*) GetParent();
	m_Dlg->MouseMoved(m_IDName, point.x, point.y);
}
