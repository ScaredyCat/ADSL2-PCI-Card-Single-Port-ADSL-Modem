// VideoPage.cpp : implementation file
//

#include "VideoPage.h"

// added by sjhuang 2006/03/08, for detect multiple webcam
#include "UAControl.h"
#include "..\Video\VideoManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const char* VideoCodecName[] =
{
	_T("ITU-T H.263"),
	_T("MPEG4")
};

const CODEC_NUMBER AvailableVideoCodec[] =
{
	CODEC_H263,
	CODEC_MPEG4
};

#define AvailableVideoCodecCount 2

/////////////////////////////////////////////////////////////////////////////
// CVideoPage property page

IMPLEMENT_DYNCREATE(CVideoPage, CPropertyPage)

CVideoPage::CVideoPage() : CPropertyPage(CVideoPage::IDD)
{
	//{{AFX_DATA_INIT(CVideoPage)
	m_bUseVideo = FALSE;
	//}}AFX_DATA_INIT
	pProfile = CUAProfile::GetProfile();
}

CVideoPage::~CVideoPage()
{
}

void CVideoPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CVideoPage)
	DDX_Control(pDX, IDC_SIZESELCOMBO, m_SizeSelCTL);
	DDX_Control(pDX, IDC_VIDEOSELCOMBO, m_VideoSelCTL);
	DDX_Control(pDX, IDC_AVAILABLECODECLISTCTL, m_AvailableCodecListCTL);
	DDX_Control(pDX, IDC_ACTIVECODECLISTCTL, m_ActiveCodecListCTL);
	DDX_Check(pDX, IDC_USEVIDEO, m_bUseVideo);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CVideoPage, CPropertyPage)
	//{{AFX_MSG_MAP(CVideoPage)
	ON_BN_CLICKED(IDC_MOVEUP, OnMoveup)
	ON_BN_CLICKED(IDC_MOVEDOWN, OnMovedown)
	ON_BN_CLICKED(IDC_USEVIDEO, OnUseVideo)
	ON_BN_CLICKED(IDC_ADD, OnAdd)
	ON_BN_CLICKED(IDC_REMOVE, OnRemove)
	ON_BN_CLICKED(IDC_BTN_DETECT_VIDEO , OnDetectVideo)
	ON_CBN_SELCHANGE(IDC_SIZESELCOMBO, OnSelchangeSizeselcombo)
	ON_CBN_SELCHANGE(IDC_VIDEOSELCOMBO, OnSelchangeVideoselcombo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVideoPage message handlers

BOOL CVideoPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	m_VideoCodecCount = pProfile->m_VideoCodecCount;
	m_VideoSizeFormat = pProfile->m_VideoSizeFormat;

	m_SizeSelCTL.AddString("CIF");
	m_SizeSelCTL.AddString("QCIF");

	switch (m_VideoSizeFormat) {
	case 2: //QCIF
		m_SizeSelCTL.SetCurSel(1);
		break;
	case 3: //CIF
		m_SizeSelCTL.SetCurSel(0);
		break;
	}

	for (int i=0;i < 10;i++)
		m_VideoCodec[i] = pProfile->m_VideoCodec[i];

	UpdateItems();
	
	m_bUseVideo = pProfile->m_bUseVideo;
	UpdateData(FALSE);

	/* for detect multiple webcam , sjhuang 2006/03/08*/
	CString tmp;
	char name[80];
	char ver[80];
	int	 sel=-1;
	CVideoManager *g_pVideo = CVideoManager::GetVideoMgr();
	m_VideoName = pProfile->m_VideoName;

#ifndef DXSHOW
	for( i=0;i<g_pVideo->m_vfwCam->GetCamAmount();i++)
	{
		memset(name,0x00,80);
		memset(m_tmpName[i],0x00,80);
		if( g_pVideo->m_vfwCam->GetCamNameByIndex(i,name,ver) )
		{
			
			m_VideoSelCTL.AddString(name);		
			strcpy(m_tmpName[i],name);
			
			tmp.Format("%s",name);
			if( m_VideoName==tmp )
			{
				m_VideoSelCTL.SetCurSel(i);
			}
		}
	}
#endif
	
	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CVideoPage::UpdateItems()
{
	CString tmpStr;
	/*when update items, clear all first*/
	m_ActiveCodecListCTL.DeleteAllItems();

	// Insert items in the list view control.
	for (int i=0;i < 10;i++)
	{
		switch (m_VideoCodec[i]) {
		case CODEC_H263:
			tmpStr = VideoCodecName[0];
			break;
		case CODEC_MPEG4:
			tmpStr = VideoCodecName[1];
			break;
		case CODEC_NONE:
			continue;
		}
		m_ActiveCodecListCTL.InsertItem(
		LVIF_TEXT|LVIF_STATE, i, tmpStr, 0, LVIS_SELECTED, 0, 0);
	}

	m_AvailableCodecListCTL.DeleteAllItems();

	for (i=0;i < AvailableVideoCodecCount;i++) {
		m_AvailableCodecListCTL.InsertItem(
		LVIF_TEXT|LVIF_STATE, i, VideoCodecName[i], 0, LVIS_SELECTED, 0, 0);
	}
}

void CVideoPage::OnMoveup() 
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

	CODEC_NUMBER tmp = m_VideoCodec[nSelected-1];
	m_VideoCodec[nSelected-1] = m_VideoCodec[nSelected];
	m_VideoCodec[nSelected] = tmp;

	UpdateItems();
	SetModified(TRUE);
}

void CVideoPage::OnMovedown() 
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

	CODEC_NUMBER tmp = m_VideoCodec[nSelected+1];
	m_VideoCodec[nSelected+1] = m_VideoCodec[nSelected];
	m_VideoCodec[nSelected] = tmp;

	UpdateItems();
	SetModified(TRUE);
}

void CVideoPage::OnOK() 
{
	for (int i=0;i < 10;i++)
		pProfile->m_VideoCodec[i] = m_VideoCodec[i];

	pProfile->m_VideoCodecCount = m_VideoCodecCount;
	pProfile->m_bUseVideo = m_bUseVideo;
	pProfile->m_VideoSizeFormat = m_VideoSizeFormat;
	pProfile->m_VideoName = m_VideoName;
	
	CPropertyPage::OnOK();
}

BOOL CVideoPage::OnApply() 
{
	for (int i=0;i < 10;i++)
		pProfile->m_VideoCodec[i] = m_VideoCodec[i];
	
	pProfile->m_VideoCodecCount = m_VideoCodecCount;
	pProfile->m_bUseVideo = m_bUseVideo;
	pProfile->m_VideoSizeFormat = m_VideoSizeFormat;
	pProfile->m_VideoName = m_VideoName;


	return CPropertyPage::OnApply();
}

void CVideoPage::OnUseVideo() 
{
	UpdateData(TRUE);
	SetModified(TRUE);
}

void CVideoPage::OnAdd() 
{
	// Get the selected items in the control
	POSITION p = m_AvailableCodecListCTL.GetFirstSelectedItemPosition();
	
	if (p == NULL)
		return;

	/*make sure position is selected */
	int nSelected=m_AvailableCodecListCTL.GetNextSelectedItem(p);

	for (int i=0;i < m_VideoCodecCount;i++) {
		if (AvailableVideoCodec[nSelected] == m_VideoCodec[i])
			return;
	}
	m_VideoCodec[m_VideoCodecCount++] = AvailableVideoCodec[nSelected];
	
	UpdateItems();
	SetModified(TRUE);
}

void CVideoPage::OnRemove() 
{
	// Get the selected items in the control
	POSITION p = m_ActiveCodecListCTL.GetFirstSelectedItemPosition();
	
	if (p == NULL)
		return;
	
	/*make sure position is selected */
	int nSelected=m_ActiveCodecListCTL.GetNextSelectedItem(p);

	for (int i=nSelected; i < m_VideoCodecCount; i++)
		m_VideoCodec[i] = m_VideoCodec[i+1];
	m_VideoCodecCount--;
	
	UpdateItems();
	SetModified(TRUE);
}

void CVideoPage::OnSelchangeSizeselcombo() 
{
	switch ( m_SizeSelCTL.GetCurSel() ) {
	case 0:
		m_VideoSizeFormat = 3;
		break;
	case 1:
		m_VideoSizeFormat = 2;
		break;
	}
	SetModified(TRUE);
}

void CVideoPage::OnSelchangeVideoselcombo() 
{
	int ret = m_VideoSelCTL.GetCurSel();
	m_VideoName.Format("%s",m_tmpName[ret]);
	
	SetModified(TRUE);
	
}

void CVideoPage::OnDetectVideo() 
{
	CVideoManager *g_pVideo = CVideoManager::GetVideoMgr();
	g_pVideo->_StartLocalVideo(1);

}