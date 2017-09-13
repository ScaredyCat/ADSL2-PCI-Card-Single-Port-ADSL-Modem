// VoiceMailLogin.cpp : implementation file
//

#include "stdafx.h"
#include "pcaua.h"
#include "VoiceMailLogin.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVoiceMailLogin dialog


CVoiceMailLogin::CVoiceMailLogin(CWnd* pParent /*=NULL*/)
	: CDialog(CVoiceMailLogin::IDD, pParent)
{
	//{{AFX_DATA_INIT(CVoiceMailLogin)
	m_VoiceMailAccount = _T("");
	m_VoiceMailIP = _T("");
	m_VoiceMailPort = _T("");
	m_VoiceMailPWD = _T("");
	//}}AFX_DATA_INIT
}


void CVoiceMailLogin::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CVoiceMailLogin)
	DDX_Text(pDX, IDC_VOICEMAILACCOUNT, m_VoiceMailAccount);
	DDX_Text(pDX, IDC_VOICEMAILIP, m_VoiceMailIP);
	DDX_Text(pDX, IDC_VOICEMAILPORT, m_VoiceMailPort);
	DDX_Text(pDX, IDC_VOICEMAILPWD, m_VoiceMailPWD);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CVoiceMailLogin, CDialog)
	//{{AFX_MSG_MAP(CVoiceMailLogin)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVoiceMailLogin message handlers
