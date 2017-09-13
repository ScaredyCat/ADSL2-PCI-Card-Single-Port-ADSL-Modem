///////////////////////////////////////////////////////////////
//
// Dosya Ad? AnimatedLabel.cpp
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

#include "AnimatedLabel.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAnimatedLabel

CAnimatedLabel::CAnimatedLabel()
{
	m_MaxText = m_Strpos = 0;
	LOGFONT m_lf;
	::GetObject((HFONT)GetStockObject(DEFAULT_GUI_FONT), sizeof(m_lf), &m_lf);
	m_Font.CreateFontIndirect(&m_lf);
	m_Timered = FALSE;
	m_Label = "";

	m_bUseMode2 = false;
	m_bHideData = FALSE;
}

CAnimatedLabel::~CAnimatedLabel()
{
	m_Font.DeleteObject();
	m_Back.DeleteObject();

	if ( (HBITMAP)m_Back2)
		m_Back2.DeleteObject();
}


BEGIN_MESSAGE_MAP(CAnimatedLabel, CBitmapBtn)
	//{{AFX_MSG_MAP(CAnimatedLabel)
	ON_WM_TIMER()
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAnimatedLabel message handlers

void CAnimatedLabel::SetText(CString mNewText)
{
	SetWindowText(mNewText);
	m_Label = mNewText;
	//Invalidate();
	UpdateTimer();
}

void CAnimatedLabel::SetFont(CFont& mNewFont)
{
	LOGFONT lf;
	mNewFont.GetLogFont(&lf);
	m_Font.DeleteObject();
	m_Font.CreateFontIndirect(&lf);	
	UpdateTimer();
}

void CAnimatedLabel::UpdateTimer()
{
	//LOGFONT m_lf;
	//m_Font.GetLogFont(&m_lf);

	if(m_Timered)
		KillTimer(1);

	CString mText = m_Label;

	CRect r;
	GetClientRect(&r);
	CSize m_Size;
	CDC* mDC = GetDC();
	GetTextExtentPoint32(mDC->m_hDC, "W", 1, &m_Size);
	ReleaseDC(mDC);// added by jason, 

	m_MaxText = (r.Width() * 2) / m_Size.cx;
	if(m_MaxText < mText.GetLength())
	{
		SetTimer(1, 250, NULL);
		m_Timered = TRUE;
	}
	else m_Timered = FALSE;
	m_Strpos = 0;
	Invalidate();

}

void CAnimatedLabel::OnTimer(UINT nIDEvent) 
{
	// TODO: Add your message handler code here and/or call default
	m_Strpos++;
	CString mText = m_Label;
	if(m_Strpos > mText.GetLength())
		m_Strpos = 0;
	Invalidate();
}

void CAnimatedLabel::OnPaint() 
{
	CPaintDC dc(this); 
	CDC memdc;
	memdc.CreateCompatibleDC(&dc);
	memdc.SelectObject( GetBackBmp() );

	CRect r;
	GetClientRect(&r);
	dc.BitBlt(0, 0, r.Width(), r.Height(), &memdc, 0, 0, SRCCOPY);
	memdc.DeleteDC();

	CString m_DisplayText;
	
	if( !m_bHideData )
		m_DisplayText = m_Label;
	else
	{
		
		for(int i=0;i<m_Label.GetLength();i++)
			m_DisplayText += "*";
			
	}
	m_DisplayText.Delete(0, m_Strpos);

	int m_ScrOpt = DT_VCENTER;
	if(m_MaxText < m_Label.GetLength())
	{
		while(m_DisplayText.GetLength() < m_MaxText)
			m_DisplayText += " ** " + m_Label;

	}
	else
		m_ScrOpt = DT_VCENTER | m_textAlign;

	dc.SetBkMode(TRANSPARENT);
	dc.SelectObject((HFONT) m_Font);
	dc.SetTextColor(m_TextColor);
	dc.DrawText(m_DisplayText, m_DisplayText.GetLength(), r, m_ScrOpt);
	// Do not call CBitmapBtn::OnPaint() for painting messages
}

void CAnimatedLabel::SetBitmap(CBitmap &mBack)
{
	m_Back.DeleteObject();
	m_Back.Attach(mBack);
}

/*
 *  Sometimes, must hide data, by sjhuang 2006/06/29
 */
void CAnimatedLabel::SetHideData()
{
	m_bHideData = TRUE;
}

void CAnimatedLabel::SetShowData()
{
	m_bHideData = FALSE;
}
