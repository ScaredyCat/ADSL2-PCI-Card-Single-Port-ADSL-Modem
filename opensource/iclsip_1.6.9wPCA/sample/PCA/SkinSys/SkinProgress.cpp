///////////////////////////////////////////////////////////////
//
// Dosya Adý: SkinProgress
// Yazan    : Cüneyt ELÝBOL
// Açýklama : SkinSys için yüzde Göstergesi
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

#include "SkinProgress.h"
#include "SkinDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSkinProgress

CSkinProgress::CSkinProgress()
{
}

CSkinProgress::~CSkinProgress()
{
}


BEGIN_MESSAGE_MAP(CSkinProgress, CBitmapProgress)
	//{{AFX_MSG_MAP(CSkinProgress)
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSkinProgress message handlers

void CSkinProgress::CopyFrom(CRect r, CBitmap &m_N, CBitmap &m_Dw)
{
	CDC* dc = GetDC();
	
	CopyBitmap(dc, m_Normal, m_N, r);
	CopyBitmap(dc, m_Down, m_Dw, r);
	ModifyStyle(WS_BORDER | WS_TABSTOP , 0);
	ReleaseDC(dc);
}

void CSkinProgress::CopyFrom2(CRect r, CBitmap &m_N, CBitmap &m_Dw)
{
	CDC* dc = GetDC();
	
	CopyBitmap(dc, m_Normal2, m_N, r);
	CopyBitmap(dc, m_Down2, m_Dw, r);
	ModifyStyle(WS_BORDER | WS_TABSTOP , 0);
	ReleaseDC(dc);
}

void CSkinProgress::OnLButtonUp(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	CBitmapProgress::OnLButtonUp(nFlags, point);
	Invalidate();
	CRect r;
	GetClientRect(&r);
	if(r.PtInRect(point))
	{
		CSkinDialog* m_Dlg = (CSkinDialog*) GetParent();
		m_Dlg->ChangeProgress(m_ID);
	}
}

void CSkinProgress::OnMouseMove(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	CBitmapProgress::OnMouseMove(nFlags, point);
	CSkinDialog* m_Dlg = (CSkinDialog*) GetParent();
	m_Dlg->MouseMoved(m_IDName, point.x, point.y);
}
