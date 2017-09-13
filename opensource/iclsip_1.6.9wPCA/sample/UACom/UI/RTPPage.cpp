// RTPPage.cpp : implementation file
//
#include "RTPPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRTPPage property page

IMPLEMENT_DYNCREATE(CRTPPage, CPropertyPage)

CRTPPage::CRTPPage() : CPropertyPage(CRTPPage::IDD)
{
	//{{AFX_DATA_INIT(CRTPPage)
	m_bOnlyIframe = FALSE;
	m_bUseQuant = FALSE;
	m_QuantValue = 0;
	m_strVideoKeyInterval = _T("5");
	m_strAudioPacketPeriod = _T("20");
	m_strVideoEncBitrate = _T("200");
	m_strVideoEncFramerate = _T("15");
	//}}AFX_DATA_INIT

}

CRTPPage::~CRTPPage()
{
}

void CRTPPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRTPPage)
	DDX_Control(pDX, IDC_RTPFRAMES, m_RTPFramesCTL);
	DDX_Control(pDX, IDC_QUANTVALUE, m_QuantValueCTL);
	DDX_Text(pDX, IDC_RTP_PACKET_PERIOD, m_strAudioPacketPeriod);
	DDX_Text(pDX, IDC_VIDEO_ENC_BITRATE, m_strVideoEncBitrate);
	DDX_Text(pDX, IDC_VIDEO_ENC_FRAMERATE, m_strVideoEncFramerate);
	DDX_Check(pDX, IDC_ONLYENCODEIFRAME, m_bOnlyIframe);
	DDX_Check(pDX, IDC_USEQUANT, m_bUseQuant);
	DDX_Text(pDX, IDC_QUANTVALUE, m_QuantValue);
	DDV_MinMaxUInt(pDX, m_QuantValue, 1, 20);
	DDX_Text(pDX, IDC_VIDEO_KEYINTERVAL, m_strVideoKeyInterval);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRTPPage, CPropertyPage)
	//{{AFX_MSG_MAP(CRTPPage)
	ON_BN_CLICKED(IDC_USEQUANT, OnUseQuant)
	ON_BN_CLICKED(IDC_ONLYENCODEIFRAME, OnOnlyencodeIframe)
	ON_CBN_SELCHANGE(IDC_RTPFRAMES, OnSelchangeRtpframes)
	ON_EN_CHANGE(IDC_VIDEO_ENC_BITRATE, OnChangeVideoEncBitrate)
	ON_EN_CHANGE(IDC_VIDEO_ENC_FRAMERATE, OnChangeVideoEncFramerate)
	ON_EN_CHANGE(IDC_VIDEO_KEYINTERVAL, OnChangeVideoKeyinterval)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRTPPage message handlers

BOOL CRTPPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	CUAProfile* pProfile = CUAProfile::GetProfile();

	m_strAudioPacketPeriod.Format("%d", pProfile->m_RTPPacketizePeriod);
	m_strVideoEncBitrate.Format("%d", pProfile->m_VideoBitrate);
	m_strVideoEncFramerate.Format("%d", pProfile->m_VideoFramerate);
	m_strVideoKeyInterval.Format("%d", pProfile->m_VideoKeyInterval);

	m_bUseQuant = pProfile->m_bUseQuant;
	m_QuantValue = pProfile->m_QuantValue;
	m_bOnlyIframe = pProfile->m_bOnlyIframe;
	m_RTPFrames = pProfile->m_RTPFrames;

	m_RTPFramesCTL.SetCurSel(m_RTPFrames - 1);

	UpdateData(false);

	OnUseQuant();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CRTPPage::OnReleasedCaptureRTPPeriodSlider(NMHDR* pNMHDR, LRESULT* pResult) 
{
	SetModified(TRUE);	
	*pResult = 0;
}

BOOL CRTPPage::OnApply() 
{
	UpdateData();

	CUAProfile* pProfile = CUAProfile::GetProfile();
	pProfile->m_RTPPacketizePeriod = atoi( m_strAudioPacketPeriod );
	pProfile->m_VideoBitrate = atoi( m_strVideoEncBitrate );
	pProfile->m_VideoFramerate = atoi( m_strVideoEncFramerate );
	pProfile->m_VideoKeyInterval = atoi( m_strVideoKeyInterval );
	pProfile->m_bUseQuant = m_bUseQuant;
	pProfile->m_QuantValue = m_QuantValue;
	pProfile->m_bOnlyIframe = m_bOnlyIframe;
	pProfile->m_RTPFrames = m_RTPFrames;
	
	return CPropertyPage::OnApply();
}

void CRTPPage::OnOK() 
{
	UpdateData();

	CUAProfile* pProfile = CUAProfile::GetProfile();
	pProfile->m_RTPPacketizePeriod = atoi( m_strAudioPacketPeriod );
	pProfile->m_VideoBitrate = atoi( m_strVideoEncBitrate );
	pProfile->m_VideoFramerate = atoi( m_strVideoEncFramerate );
	pProfile->m_VideoKeyInterval = atoi( m_strVideoKeyInterval );
	pProfile->m_bUseQuant = m_bUseQuant;
	pProfile->m_QuantValue = m_QuantValue;
	pProfile->m_bOnlyIframe = m_bOnlyIframe;
	pProfile->m_RTPFrames = m_RTPFrames;
	
	CPropertyPage::OnOK();
}

void CRTPPage::OnUseQuant() 
{
	UpdateData(TRUE);

	if (m_bUseQuant)
		m_QuantValueCTL.EnableWindow(TRUE);
	else
		m_QuantValueCTL.EnableWindow(FALSE);
	
	SetModified(TRUE);
}

void CRTPPage::OnOnlyencodeIframe() 
{
	SetModified(TRUE);
}

void CRTPPage::OnSelchangeRtpframes() 
{
	int index = m_RTPFramesCTL.GetCurSel();
	m_RTPFrames = index+1;
	SetModified(TRUE);
}

void CRTPPage::OnChangeVideoEncBitrate() 
{
	SetModified(TRUE);
}

void CRTPPage::OnChangeVideoEncFramerate() 
{
	SetModified(TRUE);
}

void CRTPPage::OnChangeVideoKeyinterval() 
{
	SetModified(TRUE);
}
