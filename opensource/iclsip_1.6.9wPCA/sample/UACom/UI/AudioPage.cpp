// AudioPage.cpp : implementation file
//

#include "AudioPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const char* AudioCodecName[] =
{
	_T("ITU-T G.711 u-law"),
	_T("ITU-T G.711 a-law"),
	_T("GSM"),
	_T("ITU-T G.729A"),
	_T("ITU-T G.723.1"),
	_T("iLBC")
};

const CODEC_NUMBER AvailableAudioCodec[] =
{
	CODEC_PCMU,
	CODEC_PCMA,
	CODEC_GSM,
	CODEC_G729,
	CODEC_G723,
	CODEC_iLBC
};

#define AvailableAudioCodecCount 6

/////////////////////////////////////////////////////////////////////////////
// CAudioPage property page

IMPLEMENT_DYNCREATE(CAudioPage, CPropertyPage)

CAudioPage::CAudioPage() : CPropertyPage(CAudioPage::IDD)
{
	//{{AFX_DATA_INIT(CAudioPage)
	m_bEnableAEC = FALSE;
	//m_FarEndEchoSignalLag = 0;
	m_FarEndEchoSignalLag = _T("");
	//}}AFX_DATA_INIT
	pProfile = CUAProfile::GetProfile();
	pCallManager = CCallManager::GetCallMgr();
}

CAudioPage::~CAudioPage()
{
}

void CAudioPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAudioPage)
	DDX_Control(pDX, IDC_TAILLENGTH, m_TailLengthCTL);
	DDX_Control(pDX, IDC_G711PACKETIZEPERIOD, m_G711PacketizePeriodCTL);
	DDX_Control(pDX, IDC_AVAILABLECODECLISTCTL, m_AvailableCodecListCTL);
	DDX_Control(pDX, IDC_ACTIVECODECLISTCTL, m_ActiveCodecListCTL);
	DDX_Check(pDX, IDC_ENABLEAEC, m_bEnableAEC);
	DDX_Text(pDX, IDC_FarEndEchoSignalLag, m_FarEndEchoSignalLag);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAudioPage, CPropertyPage)
	//{{AFX_MSG_MAP(CAudioPage)
	ON_BN_CLICKED(IDC_MOVEUP, OnMoveup)
	ON_BN_CLICKED(IDC_MOVEDOWN, OnMovedown)
	ON_BN_CLICKED(IDC_USEVIDEO, OnUseVideo)
	ON_BN_CLICKED(IDC_ADD, OnAdd)
	ON_BN_CLICKED(IDC_REMOVE, OnRemove)
	ON_CBN_SELCHANGE(IDC_G711PACKETIZEPERIOD, OnSelChangePacketizePeriod)
	ON_BN_CLICKED(IDC_CHANGE, OnChange)
	ON_BN_CLICKED(IDC_ENABLEAEC, OnEnableAEC)
	ON_EN_CHANGE(IDC_FarEndEchoSignalLag, OnChangeFarEndEchoSignalLag)
	ON_CBN_SELCHANGE(IDC_TAILLENGTH, OnSelchangeTaillength)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAudioPage message handlers

BOOL CAudioPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	m_AudioCodecCount = pProfile->m_AudioCodecCount;
	m_G711PacketizePeriod = pProfile->m_G711PacketizePeriod;
	m_bEnableAEC = pProfile->m_bEnableAEC;
	//m_FarEndEchoSignalLag = pProfile->m_FarEndEchoSignalLag;
	m_FarEndEchoSignalLag.Format("%d",pProfile->m_FarEndEchoSignalLag);
	m_TailLength = pProfile->m_TailLength;

	for (int i=0;i < 10;i++)
		m_AudioCodec[i] = pProfile->m_AudioCodec[i];

	UpdateItems();
	
	UpdateData(FALSE);

	m_G711PacketizePeriodCTL.SetCurSel(m_G711PacketizePeriod/10 - 1);

	m_TailLengthCTL.SetCurSel(pProfile->m_TailLength/32 -1);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAudioPage::UpdateItems()
{
	CString tmpStr;
	/*when update items, clear all first*/
	m_ActiveCodecListCTL.DeleteAllItems();

	// Insert items in the list view control.
	for (int i=0;i < 10;i++)
	{
		switch (m_AudioCodec[i]) {
		case CODEC_PCMU:
			tmpStr = AudioCodecName[0];
			break;
		case CODEC_PCMA:
			tmpStr = AudioCodecName[1];
			break;
		case CODEC_GSM:
			tmpStr = AudioCodecName[2];
			break;
		case CODEC_G729:
			tmpStr = AudioCodecName[3];
			break;
		case CODEC_G723:
			tmpStr = AudioCodecName[4];
			break;
		case CODEC_iLBC:
			tmpStr = AudioCodecName[5];
			break;
		case CODEC_NONE:
			continue;
		}
		m_ActiveCodecListCTL.InsertItem(
		LVIF_TEXT|LVIF_STATE, i, tmpStr, 0, LVIS_SELECTED, 0, 0);
	}

	m_AvailableCodecListCTL.DeleteAllItems();

	for (i=0;i < AvailableAudioCodecCount;i++) {
		m_AvailableCodecListCTL.InsertItem(
		LVIF_TEXT|LVIF_STATE, i, AudioCodecName[i], 0, LVIS_SELECTED, 0, 0);
	}
}

void CAudioPage::OnMoveup() 
{
	// Get the selected items in the control
	POSITION p = m_ActiveCodecListCTL.GetFirstSelectedItemPosition();
	int nSelected =0;

	/*make sure position is selected */
	if(p!=NULL){
		nSelected=m_ActiveCodecListCTL.GetNextSelectedItem(p);
	}

	if (nSelected == 0)
		return;

	CODEC_NUMBER tmp = m_AudioCodec[nSelected-1];
	m_AudioCodec[nSelected-1] = m_AudioCodec[nSelected];
	m_AudioCodec[nSelected] = tmp;

	UpdateItems();
	SetModified(TRUE);
}

void CAudioPage::OnMovedown() 
{
	// Get the selected items in the control
	POSITION p = m_ActiveCodecListCTL.GetFirstSelectedItemPosition();
	int nSelected =0;

	/*make sure position is selected */
	if(p!=NULL){
		nSelected=m_ActiveCodecListCTL.GetNextSelectedItem(p);
	}

	if (nSelected == m_ActiveCodecListCTL.GetItemCount())
		return;

	CODEC_NUMBER tmp = m_AudioCodec[nSelected+1];
	m_AudioCodec[nSelected+1] = m_AudioCodec[nSelected];
	m_AudioCodec[nSelected] = tmp;

	UpdateItems();
	SetModified(TRUE);
}

void CAudioPage::OnOK() 
{
	for (int i=0;i < 10;i++)
		pProfile->m_AudioCodec[i] = m_AudioCodec[i];

	pProfile->m_AudioCodecCount = m_AudioCodecCount;
	pProfile->m_G711PacketizePeriod = m_G711PacketizePeriod;
	pProfile->m_bEnableAEC = m_bEnableAEC;
	pProfile->m_FarEndEchoSignalLag = atoi(m_FarEndEchoSignalLag);
	pProfile->m_TailLength = m_TailLength;

	CPropertyPage::OnOK();
}

BOOL CAudioPage::OnApply() 
{
	for (int i=0;i < 10;i++)
		pProfile->m_AudioCodec[i] = m_AudioCodec[i];
	
	pProfile->m_AudioCodecCount = m_AudioCodecCount;
	pProfile->m_G711PacketizePeriod = m_G711PacketizePeriod;
	pProfile->m_bEnableAEC = m_bEnableAEC;
	pProfile->m_FarEndEchoSignalLag = atoi(m_FarEndEchoSignalLag);
	pProfile->m_TailLength = m_TailLength;

	return CPropertyPage::OnApply();
}

void CAudioPage::OnUseVideo() 
{
	UpdateData(TRUE);
	SetModified(TRUE);
}

void CAudioPage::OnAdd() 
{
	// Get the selected items in the control
	POSITION p = m_AvailableCodecListCTL.GetFirstSelectedItemPosition();
	
	if (p == NULL)
		return;

	/*make sure position is selected */
	int nSelected=m_AvailableCodecListCTL.GetNextSelectedItem(p);

	for (int i=0;i < m_AudioCodecCount;i++) {
		if (AvailableAudioCodec[nSelected] == m_AudioCodec[i])
			return;
	}
	m_AudioCodec[m_AudioCodecCount++] = AvailableAudioCodec[nSelected];
	
	UpdateItems();
	SetModified(TRUE);
}

void CAudioPage::OnRemove() 
{
	// Get the selected items in the control
	POSITION p = m_ActiveCodecListCTL.GetFirstSelectedItemPosition();
	
	if (p == NULL)
		return;
	
	/*make sure position is selected */
	int nSelected=m_ActiveCodecListCTL.GetNextSelectedItem(p);

	for (int i=nSelected; i < m_AudioCodecCount; i++)
		m_AudioCodec[i] = m_AudioCodec[i+1];
	m_AudioCodecCount--;
	
	UpdateItems();
	SetModified(TRUE);
}

void CAudioPage::OnSelChangePacketizePeriod() 
{
	int index = m_G711PacketizePeriodCTL.GetCurSel();
	m_G711PacketizePeriod = (index+1) * 10;
	SetModified(TRUE);
}

void CAudioPage::OnChange() 
{
	for (int i=0;i < 10;i++)
		pProfile->m_AudioCodec[i] = m_AudioCodec[i];

	pProfile->m_AudioCodecCount = m_AudioCodecCount;
	pProfile->m_G711PacketizePeriod = m_G711PacketizePeriod;
	
	pCallManager->ModifySession();
}

void CAudioPage::OnEnableAEC() 
{
	UpdateData(TRUE);
	SetModified(TRUE);
}

void CAudioPage::OnChangeFarEndEchoSignalLag() 
{

	UpdateData(TRUE);
	SetModified(TRUE);
}

void CAudioPage::OnSelchangeTaillength() 
{
	int index = m_TailLengthCTL.GetCurSel();
	m_TailLength = (index+1) * 32;
	SetModified(TRUE);
}
