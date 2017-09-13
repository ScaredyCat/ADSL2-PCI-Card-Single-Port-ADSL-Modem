///////////////////////////////////////////////////////////////
//
// Dosya Ad? BitmapBtn.cpp
// Yazan    : Cüneyt ELÝBOL
// Açýklama : Resim butonlar?için kod
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

#include <afxwin.h>
#include <afxcmn.h>
#include "BitmapBtn.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBitmapBtn

CBitmapBtn::CBitmapBtn()
{
	m_CheckedButton = m_Check = m_MouseOnButton = FALSE;
	m_ToolTip.m_hWnd = NULL;

	m_bUseMode2 = false;
}

CBitmapBtn::~CBitmapBtn()
{
	m_Normal.DeleteObject();
	m_Over.DeleteObject();
	m_Down.DeleteObject();
	m_Disabled.DeleteObject();

	if ( (HBITMAP)m_Normal2)
	{
		m_Normal2.DeleteObject();
		m_Over2.DeleteObject();
		m_Down2.DeleteObject();
		m_Disabled2.DeleteObject();
	}

	m_MouseOnButton = FALSE;
}


BEGIN_MESSAGE_MAP(CBitmapBtn, CButton)
	//{{AFX_MSG_MAP(CBitmapBtn)
	ON_WM_KILLFOCUS()
	ON_WM_MOUSEMOVE()
	ON_WM_CAPTURECHANGED()
	ON_WM_ERASEBKGND()
	ON_WM_TIMER()

	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBitmapBtn message handlers

void CBitmapBtn::SetBitmap(CBitmap& mNrml, CBitmap& mOvr, CBitmap& mDwn, CBitmap& mDsbl)
{
	m_Normal.DeleteObject();
	m_Over.DeleteObject();
	m_Down.DeleteObject();
	m_Disabled.DeleteObject();

	m_Normal.Attach(mNrml);
	m_Over.Attach(mOvr);
	m_Down.Attach(mDwn);
	m_Disabled.Attach(mDsbl);
}

void CBitmapBtn::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{	
	// TODO: Add your message handler code here and/or call default

	CDC *pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	
	BOOL bIsPressed  = (lpDrawItemStruct->itemState & ODS_SELECTED);
	BOOL bIsFocused  = (lpDrawItemStruct->itemState & ODS_FOCUS);
	BOOL bIsDisabled = (lpDrawItemStruct->itemState & ODS_DISABLED);
	
	CDC memdc;
	memdc.CreateCompatibleDC(pDC);
	if(!m_CheckedButton)
	{
		if (m_Check ) //&& !m_MouseOnButton)
			memdc.SelectObject( GetDownBmp() );
		else if (bIsDisabled)				// order adjust by jason
			memdc.SelectObject( GetDisabledBmp() );
		else if (bIsPressed)
			memdc.SelectObject( GetDownBmp() );
		else if (m_MouseOnButton)
			memdc.SelectObject( GetOverBmp());
		else
			memdc.SelectObject( GetNormalBmp() );
	}
	else
	{
		if (m_Check)
			memdc.SelectObject( GetDownBmp() );
		else
			memdc.SelectObject( GetNormalBmp() );
	}

	CRect r;
	GetClientRect(&r);
	
	pDC->BitBlt(0, 0, r.Width(), r.Height(), &memdc, 0, 0, SRCCOPY);
	memdc.DeleteDC();
}

void CBitmapBtn::PreSubclassWindow() 
{
	// TODO: Add your specialized code here and/or call the base class
	UINT nBS;

	nBS = GetButtonStyle();
	if (nBS & BS_DEFPUSHBUTTON) m_MouseOnButton = TRUE;
	SetButtonStyle(nBS | BS_OWNERDRAW);
	CButton::PreSubclassWindow();
	ModifyStyle(WS_TABSTOP, 0);
}

void CBitmapBtn::OnKillFocus(CWnd* pNewWnd) 
{
	CButton::OnKillFocus(pNewWnd);
	m_MouseOnButton = FALSE;
	Invalidate();
	// TODO: Add your message handler code here
	
}

void CBitmapBtn::OnMouseMove(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	CWnd* pWnd;  // Finestra attiva
	CWnd* pParent; // Finestra che contiene il bottone

	CButton::OnMouseMove(nFlags, point);

	if (nFlags & MK_LBUTTON && m_MouseOnButton == FALSE) return;

	pWnd = GetActiveWindow();
	pParent = GetOwner();

	if ((GetCapture() != this) && (pWnd != NULL && pParent != NULL)) 
	{
		m_MouseOnButton = TRUE;
		SetCapture();
		Invalidate();
	}
	else
	{
		POINT p2 = point;
		ClientToScreen(&p2);
		CWnd* wndUnderMouse = WindowFromPoint(p2);
		if (wndUnderMouse && wndUnderMouse->m_hWnd != this->m_hWnd)
		{
			if (m_MouseOnButton == TRUE)
			{
				m_MouseOnButton = FALSE;
				Invalidate();
			}
			if (!(nFlags & MK_LBUTTON)) ReleaseCapture();
		}
	}
}

void CBitmapBtn::OnCaptureChanged(CWnd *pWnd) 
{
	// TODO: Add your message handler code here
	if (m_MouseOnButton == TRUE)
	{
		ReleaseCapture();
		Invalidate();
	}
	CButton::OnCaptureChanged(pWnd);
}

void CBitmapBtn::InitToolTip()
{
	if (m_ToolTip.m_hWnd == NULL)
	{
		m_ToolTip.Create(this);
		m_ToolTip.Activate(FALSE);
	}
}

BOOL CBitmapBtn::PreTranslateMessage(MSG* pMsg) 
{
	// TODO: Add your specialized code here and/or call the base class
	InitToolTip();
	m_ToolTip.RelayEvent(pMsg);
	
	return CButton::PreTranslateMessage(pMsg);
}

void CBitmapBtn::SetTooltipText(CString* spText, BOOL bActivate)
{
	if (spText == NULL) return;
	InitToolTip();

	if (m_ToolTip.GetToolCount() == 0)
	{
		CRect rectBtn; 
		GetClientRect(rectBtn);
		m_ToolTip.AddTool(this, (LPCTSTR) *spText, rectBtn, 1);
	}
	m_ToolTip.UpdateTipText((LPCTSTR)*spText, this, 1);
	m_ToolTip.Activate(bActivate);
}

void CBitmapBtn::ActivateTooltip(BOOL bActivate)
{
	if (m_ToolTip.GetToolCount() == 0) return;
	m_ToolTip.Activate(bActivate);
}

BOOL CBitmapBtn::OnEraseBkgnd(CDC* pDC) 
{
	// TODO: Add your message handler code here and/or call default
	
	return FALSE; //CButton::OnEraseBkgnd(pDC);
}

void CBitmapBtn::SetCheck(BOOL m_NewValue)
{
	if(m_NewValue != m_Check)
	{
		m_Check = m_NewValue;
		Invalidate();
	}
}

void CBitmapBtn::SetFlash(BOOL bEnable)
{

}
