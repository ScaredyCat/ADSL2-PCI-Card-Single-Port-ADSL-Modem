#if !defined(AFX_VIDEOPAGE_H__B40AA6F6_BEC6_437A_AA33_27C19B0EE5E8__INCLUDED_)
#define AFX_VIDEOPAGE_H__B40AA6F6_BEC6_437A_AA33_27C19B0EE5E8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// VideoPage.h : header file
//
#include "stdafx.h"
#include "UA\UACommon.h"
#include "UA\CallManager.h"
#include "UA\UAProfile.h"

/////////////////////////////////////////////////////////////////////////////
// CVideoPage dialog

class CVideoPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CVideoPage)

// Construction
public:
	CVideoPage();
	~CVideoPage();
	void UpdateItems();

// Dialog Data
	//{{AFX_DATA(CVideoPage)
	enum { IDD = IDD_PROPPAGE_VIDEO };
	CComboBox	m_SizeSelCTL;
	CComboBox	m_G711PacketizePeriodCTL;
	CComboBox	m_VideoSelCTL;
	CListCtrl	m_AvailableCodecListCTL;
	CListCtrl	m_ActiveCodecListCTL;
	BOOL	m_bUseVideo;
	UINT	m_G711PacketizePeriod;
	//}}AFX_DATA
	CUAProfile* pProfile;
	CODEC_NUMBER m_VideoCodec[10];
	unsigned short m_VideoCodecCount;
	unsigned short m_VideoSizeFormat;
	CString			m_VideoName;
	char			m_tmpName[10][80];

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CVideoPage)
	public:
	virtual void OnOK();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CVideoPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnMoveup();
	afx_msg void OnMovedown();
	afx_msg void OnUseVideo();
	afx_msg void OnAdd();
	afx_msg void OnRemove();
	afx_msg void OnSelchangeSizeselcombo();
	afx_msg void OnSelchangeVideoselcombo();
	afx_msg void OnDetectVideo();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VIDEOPAGE_H__B40AA6F6_BEC6_437A_AA33_27C19B0EE5E8__INCLUDED_)
