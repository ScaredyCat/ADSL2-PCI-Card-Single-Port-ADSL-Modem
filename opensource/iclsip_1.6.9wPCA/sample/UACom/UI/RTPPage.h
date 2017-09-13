#if !defined(AFX_RTPPAGE_H__6623230A_EC01_4E11_83B4_BA0D9F4F808D__INCLUDED_)
#define AFX_RTPPAGE_H__6623230A_EC01_4E11_83B4_BA0D9F4F808D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RTPPage.h : header file
//
#include "stdafx.h"
#include "UA\UAProfile.h"

/////////////////////////////////////////////////////////////////////////////
// CRTPPage dialog

class CRTPPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CRTPPage)

// Construction
public:
	CRTPPage();
	~CRTPPage();

// Dialog Data
	//{{AFX_DATA(CRTPPage)
	enum { IDD = IDD_PROPPAGE_RTP };
	CComboBox	m_RTPFramesCTL;
	CEdit	m_QuantValueCTL;
	CString m_strAudioPacketPeriod;
	CString m_strVideoEncBitrate;
	CString m_strVideoEncFramerate;
	BOOL	m_bOnlyIframe;
	BOOL	m_bUseQuant;
	UINT	m_QuantValue;
	CString	m_strVideoKeyInterval;
	//}}AFX_DATA
	CUAProfile* pProfile;
	unsigned short m_RTPFrames;


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRTPPage)
	public:
	virtual BOOL OnApply();
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CRTPPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnReleasedCaptureRTPPeriodSlider(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnUseQuant();
	afx_msg void OnOnlyencodeIframe();
	afx_msg void OnSelchangeRtpframes();
	afx_msg void OnChangeVideoEncBitrate();
	afx_msg void OnChangeVideoEncFramerate();
	afx_msg void OnChangeVideoKeyinterval();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RTPPAGE_H__6623230A_EC01_4E11_83B4_BA0D9F4F808D__INCLUDED_)
