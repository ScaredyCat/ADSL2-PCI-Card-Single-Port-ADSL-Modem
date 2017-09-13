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

#include <afxwin.h>
#include <afxcmn.h>
#include "BitmapSlider.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBitmapSlider

CBitmapSlider::CBitmapSlider()
{
	m_MouseOnThumb = FALSE;
}

CBitmapSlider::~CBitmapSlider()
{
	m_Back.DeleteObject();
	m_ThumbNormal.DeleteObject();
	m_ThumbDown.DeleteObject();
}


BEGIN_MESSAGE_MAP(CBitmapSlider, CSliderCtrl)
	//{{AFX_MSG_MAP(CBitmapSlider)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_CAPTURECHANGED()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBitmapSlider message handlers

void CBitmapSlider::SetBitmap(CBitmap& mBack, CBitmap& mTNormal, CBitmap& mTDown)
{
	m_Back.DeleteObject();
	m_ThumbNormal.DeleteObject();
	m_ThumbDown.DeleteObject();
	
	m_Back.Attach(mBack);
	m_ThumbNormal.Attach(mTNormal);
	m_ThumbDown.Attach(mTDown);
	ModifyStyle(WS_TABSTOP, 0);
}

void CBitmapSlider::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	
	// TODO: Add your message handler code here
	CDC memdc;
	memdc.CreateCompatibleDC(NULL);
	memdc.SelectObject(m_Back);

	CRect r;
	GetClientRect(&r);
	dc.BitBlt(0, 0, r.Width(), r.Height(), &memdc, 0, 0, SRCCOPY);
	CRect Tr;
	getThumbRect(Tr);
	if (m_MouseOnThumb)
		memdc.SelectObject(m_ThumbDown);
	else
		memdc.SelectObject(m_ThumbNormal);
	dc.BitBlt(Tr.left, Tr.top, Tr.Width(), Tr.Height(), &memdc, 0, 0, SRCCOPY);
}

void CBitmapSlider::OnLButtonDown(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	CSliderCtrl::OnLButtonDown(nFlags, point);
	m_MouseOnThumb = FALSE;
	CRect Tr;
	getThumbRect(Tr);
	if (Tr.PtInRect(point))
		m_MouseOnThumb = TRUE;
	Invalidate();
}

void CBitmapSlider::OnLButtonUp(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	CSliderCtrl::OnLButtonUp(nFlags, point);
	m_MouseOnThumb = FALSE;	
	Invalidate();
}

void CBitmapSlider::OnMouseMove(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	CSliderCtrl::OnMouseMove(nFlags, point);
	if(m_MouseOnThumb) Invalidate();
}

void CBitmapSlider::OnCaptureChanged(CWnd *pWnd) 
{
	// TODO: Add your message handler code here
	if(m_MouseOnThumb)
	{
		ReleaseCapture();
		Invalidate();
	}
	CSliderCtrl::OnCaptureChanged(pWnd);
}

BOOL CBitmapSlider::OnEraseBkgnd(CDC* pDC) 
{
	// TODO: Add your message handler code here and/or call default
	
	return FALSE; //CSliderCtrl::OnEraseBkgnd(pDC);
}

void CBitmapSlider::getThumbRect(CRect& r)
{
	BITMAP bm;

	GetObject(m_ThumbNormal, sizeof(bm), &bm);
	
	GetThumbRect(&r);

	if ((GetStyle() & TBS_VERT) == TBS_VERT) 
	{
		CRect mR;
		GetClientRect(&mR);
		int o    = /*mR.Width() - */(r.Height() / 2);

		r.left   = 0;
		r.top    = (r.top + o) - (bm.bmHeight / 2);
		r.right  = r.left + bm.bmWidth;
		if(r.top < 0) r.top = 0;
		r.bottom = r.top + bm.bmHeight;
	}
	else
	{
		int o    = r.Width() / 2;
		r.top = 0;
		r.bottom = r.top + bm.bmHeight;
		r.left = (r.left + o) - (bm.bmWidth / 2);
		r.right = r.left + bm.bmWidth;
	}
}
