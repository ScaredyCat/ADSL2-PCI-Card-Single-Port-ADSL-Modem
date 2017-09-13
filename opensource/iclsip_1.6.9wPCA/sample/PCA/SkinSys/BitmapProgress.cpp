///////////////////////////////////////////////////////////////
//
// Dosya Adý: BitmapProgress.cpp
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


#include "BitmapProgress.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBitmapProgress

CBitmapProgress::CBitmapProgress()
{
	m_MouseOnProgress = m_MouseOnButton = false;
	m_Pos             = 0;
	m_Horizantal      = FALSE;
}

CBitmapProgress::~CBitmapProgress()
{
}


BEGIN_MESSAGE_MAP(CBitmapProgress, CBitmapBtn)
	//{{AFX_MSG_MAP(CBitmapProgress)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBitmapProgress message handlers

void CBitmapProgress::SetBitmap(CBitmap& mZero, CBitmap& mFull)
{
	m_Normal.DeleteObject();
	m_Down.DeleteObject();

	m_Normal.Attach(mZero);
	m_Down.Attach(mFull);
}

BOOL CBitmapProgress::OnEraseBkgnd(CDC* pDC) 
{
	// TODO: Add your message handler code here and/or call default
	return FALSE; 
}

void CBitmapProgress::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	// TODO: Add your message handler code here
	CDC memdc;
	memdc.CreateCompatibleDC(NULL);
	memdc.SelectObject( GetNormalBmp() );

	CRect r;
	GetClientRect(&r);

	dc.BitBlt(0, 0, r.Width(), r.Height(), &memdc, 0, 0, SRCCOPY);
	memdc.SelectObject( GetDownBmp() );
	if(m_Horizantal)
	{
		int h = (r.Height() * m_Pos) / 100;
		dc.BitBlt(0, 0, r.Width(), h, &memdc, 0, 0, SRCCOPY);
	}
	else
	{
		int w = (r.Width() * m_Pos) / 100;
		dc.BitBlt(0, 0, w, r.Height(), &memdc, 0, 0, SRCCOPY);
	}
	
	
	// Do not call CBitmapBtn::OnPaint() for painting messages
}

void CBitmapProgress::OnLButtonDown(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	CRect r;
	GetClientRect(&r);

	if(m_Horizantal)
		m_Pos = (point.y * 100) / r.Height();
	else
		m_Pos = (point.x * 100) / r.Width();
	m_MouseOnProgress = true;
	Invalidate();
}

void CBitmapProgress::OnLButtonUp(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	m_MouseOnProgress = false;
}

void CBitmapProgress::OnMouseMove(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	CBitmapBtn::OnMouseMove(nFlags, point);
	if(m_MouseOnButton && m_MouseOnProgress)
	{
		CRect r;
		GetClientRect(&r);

		m_Pos = (point.x * 100) / r.Width();
		Invalidate();
	}
}

void CBitmapProgress::SetPos(UINT m_newPos)
{
	if(!m_MouseOnProgress && m_newPos != m_Pos)
	{
		m_Pos = m_newPos;
		if(m_Pos < 0) m_Pos = 0;
		if(m_Pos > 100) m_Pos = 100;
		Invalidate();
	}
}
