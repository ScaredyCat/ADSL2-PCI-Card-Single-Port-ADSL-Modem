#if !defined(AFX_AUDIOPAGE_H__B40AA6F6_BEC6_437A_AA33_27C19B0EE5E8__INCLUDED_)
#define AFX_AUDIOPAGE_H__B40AA6F6_BEC6_437A_AA33_27C19B0EE5E8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AudioPage.h : header file
//
#include "stdafx.h"
#include "UA\UACommon.h"
#include "UA\CallManager.h"
#include "UA\UAProfile.h"

/////////////////////////////////////////////////////////////////////////////
// CAudioPage dialog

class CAudioPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CAudioPage)

// Construction
public:
	CAudioPage();
	~CAudioPage();
	void UpdateItems();

// Dialog Data
	//{{AFX_DATA(CAudioPage)
	enum { IDD = IDD_PROPPAGE_AUDIO };
	CComboBox	m_TailLengthCTL;
	CComboBox	m_G711PacketizePeriodCTL;
	CListCtrl	m_AvailableCodecListCTL;
	CListCtrl	m_ActiveCodecListCTL;
	BOOL	m_bEnableAEC;
	//UINT	m_FarEndEchoSignalLag;
	CString	m_FarEndEchoSignalLag;
	//}}AFX_DATA
	CUAProfile* pProfile;
	CCallManager* pCallManager;
	CODEC_NUMBER m_AudioCodec[10];
	unsigned short m_AudioCodecCount;
	unsigned short m_G711PacketizePeriod;
	unsigned short m_TailLength;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CAudioPage)
	public:
	virtual void OnOK();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CAudioPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnMoveup();
	afx_msg void OnMovedown();
	afx_msg void OnUseVideo();
	afx_msg void OnAdd();
	afx_msg void OnRemove();
	afx_msg void OnSelChangePacketizePeriod();
	afx_msg void OnChange();
	afx_msg void OnEnableAEC();
	afx_msg void OnChangeFarEndEchoSignalLag();
	afx_msg void OnSelchangeTaillength();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AUDIOPAGE_H__B40AA6F6_BEC6_437A_AA33_27C19B0EE5E8__INCLUDED_)
