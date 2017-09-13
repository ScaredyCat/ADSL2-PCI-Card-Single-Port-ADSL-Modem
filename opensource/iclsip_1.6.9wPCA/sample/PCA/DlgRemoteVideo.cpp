// DlgRemoteVideo.cpp : implementation file
//

#include "stdafx.h"
#include "pcaua.h"
#include "DlgRemoteVideo.h"
#include "PCAUADlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgRemoteVideo dialog

CDlgRemoteVideo* g_dlgRemoteVideo = NULL;


CDlgRemoteVideo::CDlgRemoteVideo(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgRemoteVideo::IDD, pParent)
{
	g_dlgRemoteVideo = this;

	m_bDragging=FALSE;
	m_bSnap = FALSE;

	//{{AFX_DATA_INIT(CDlgRemoteVideo)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDlgRemoteVideo::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgRemoteVideo)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgRemoteVideo, CDialog)
	//{{AFX_MSG_MAP(CDlgRemoteVideo)
	ON_WM_ERASEBKGND()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	ON_COMMAND(ID_ZOOM1, OnZoom1)
	ON_COMMAND(ID_ZOOM2, OnZoom2)
	ON_COMMAND(ID_ZOOM3, OnZoom3)
	ON_COMMAND(ID_ZOOM4, OnZoom4)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgRemoteVideo message handlers

BOOL CDlgRemoteVideo::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	if ( !(HBITMAP)m_bmpTitleBar)
		m_bmpTitleBar.LoadBitmap( IDB_VIDEO_TITLEBAR2 );
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CDlgRemoteVideo::OnEraseBkgnd(CDC* pDC) 
{
	RECT rtClient;
	GetClientRect( &rtClient);
	HBRUSH hWhite = CreateSolidBrush( RGB(255,255,255));
	FillRect( pDC->GetSafeHdc(), &rtClient, hWhite);
	DeleteObject( hWhite);

	DrawBitmap(pDC, (HBITMAP)m_bmpTitleBar, CRect(0,0,710,21), FALSE);	
	
	//return CDialog::OnEraseBkgnd(pDC);
	return TRUE;
}

void CDlgRemoteVideo::SnapMove(int x, int y)
{
	if( m_bSnap )
		SetWindowPos( NULL, x - m_ptSnap.x, y - m_ptSnap.y, 0, 0, SWP_NOZORDER|SWP_NOSIZE );
}

void CDlgRemoteVideo::OnMouseMove(UINT nFlags, CPoint point) 
{
	if( m_bDragging)
	{
		CRect r, rcUI;
		int x,y;
		static snapRange = 16;
		RECT waRc;	// rect of work area (not includes tray bar) 
		
		GetWindowRect( &r);
		SystemParametersInfo( SPI_GETWORKAREA, NULL, &waRc, NULL );

		x = r.left-(m_ptDrag.x-point.x);
		y = r.top-(m_ptDrag.y-point.y);
		
		g_pMainDlg->GetWindowRect( &rcUI );
		
		if( abs((x+r.Width()) - rcUI.left) < snapRange )
		{
			x = rcUI.left-r.Width();
			m_bSnap=TRUE;
		}
		//else if( abs( x - rcUI.right) < snapRange )
		//{
		//	x = rcUI.right;
		//	m_bSnap=TRUE;
		//}
		else
			m_bSnap=FALSE;

		if( m_bSnap )
		{
			m_ptSnap.x=rcUI.left-x;
			m_ptSnap.y=rcUI.top-y;
		}



		// snap to screen border
		/*
		if( x - waRc.left < snapRange )
			x = waRc.left;
		else if( waRc.right - (x+r.Width()) < snapRange )
			x = waRc.right - r.Width();

		if( y - waRc.top < snapRange ) 
			y = waRc.top;
		else if ( waRc.bottom - (y+r.Height()) < snapRange )
			y = waRc.bottom - r.Height();
		*/

		
		SetWindowPos(NULL,x, y, 0,0,SWP_NOSIZE|SWP_NOZORDER );
	}	
	CDialog::OnMouseMove(nFlags, point);
}

void CDlgRemoteVideo::OnLButtonDown(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	m_ptDrag = point;
	m_bDragging = TRUE;
	SetCapture();
	::SetCursor( ::LoadCursor( NULL, IDC_SIZEALL ));
	
	CDialog::OnLButtonDown(nFlags, point);
}

void CDlgRemoteVideo::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if( m_bDragging )
	{
		ReleaseCapture();
		m_bDragging=FALSE;
	}
	CDialog::OnLButtonUp(nFlags, point);
}

void CDlgRemoteVideo::OnRButtonUp(UINT nFlags, CPoint point) 
{
	// pop up video menu
	CMenu popMenu;
	popMenu.LoadMenu(IDR_MAINFRAME);

	CPoint posMouse;
	GetCursorPos(&posMouse);

	popMenu.GetSubMenu(5)->TrackPopupMenu(0, posMouse.x, posMouse.y, this);

	CDialog::OnRButtonUp(nFlags, point);
}

void CDlgRemoteVideo::OnZoom1() 
{
	g_pdlgUAControl->AdjustVideoSize( -1, 1);
}

void CDlgRemoteVideo::OnZoom2() 
{
	g_pdlgUAControl->AdjustVideoSize( -1, 2);
}

void CDlgRemoteVideo::OnZoom3() 
{
	g_pdlgUAControl->AdjustVideoSize( -1, 3);
}

void CDlgRemoteVideo::OnZoom4() 
{
	g_pdlgUAControl->AdjustVideoSize( -1, 4);
}

void CDlgRemoteVideo::AdjustVideoSize( int nWidth, int nHeight)
{
	// resize remote video window by right-up corner if snapped to main window
	nWidth += 6;		// border
	nHeight += 27;		// border

	if ( IsSnapped())
	{
		CRect r;
		GetWindowRect(&r);
		r.left = r.left + r.Width() - nWidth;
		SetWindowPos( NULL, r.left, r.top, nWidth, nHeight, SWP_NOZORDER);
		OnMouseMove( r.left, r.top);
	}
	else
	{
		SetWindowPos( NULL, 0, 0, nWidth, nHeight, SWP_NOMOVE|SWP_NOZORDER);
	}
}
