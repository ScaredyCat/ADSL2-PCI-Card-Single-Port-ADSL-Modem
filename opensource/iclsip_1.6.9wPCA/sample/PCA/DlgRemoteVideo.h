#if !defined(AFX_DLGREMOTEVIDEO_H__325936D1_AFD3_4F06_B775_9BA4C5F3A85B__INCLUDED_)
#define AFX_DLGREMOTEVIDEO_H__325936D1_AFD3_4F06_B775_9BA4C5F3A85B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DlgRemoteVideo.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgRemoteVideo dialog

class CDlgRemoteVideo : public CDialog
{
// Construction
public:
	CDlgRemoteVideo(CWnd* pParent = NULL);   // standard constructor

	bool IsSnapped()
	{
		return m_bSnap != FALSE;
	}

	void SnapMove( int x, int y );

	void OnMouseMove( int x, int y)
	{
		m_bDragging = true;
		m_ptDrag = CPoint(x,y);
		OnMouseMove( 0, CPoint(x,y));
		m_bDragging = false;
	}

	void AdjustVideoSize( int nWidth, int nHeight);

// Dialog Data
	//{{AFX_DATA(CDlgRemoteVideo)
	enum { IDD = IDD_REMOTE_VIDEO };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgRemoteVideo)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void OnOK() {}
	void OnCancel() {}

	CBitmap m_bmpTitleBar;

	BOOL m_bDragging;
	CPoint	m_ptDrag;

	BOOL m_bSnap;
	CPoint m_ptSnap;

	// Generated message map functions
	//{{AFX_MSG(CDlgRemoteVideo)
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnZoom1();
	afx_msg void OnZoom2();
	afx_msg void OnZoom3();
	afx_msg void OnZoom4();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


extern CDlgRemoteVideo* g_dlgRemoteVideo;

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGREMOTEVIDEO_H__325936D1_AFD3_4F06_B775_9BA4C5F3A85B__INCLUDED_)
