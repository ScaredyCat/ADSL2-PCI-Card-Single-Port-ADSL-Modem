// DlgMessage.cpp : implementation file
//

#include "stdafx.h"
#include "pcaua.h"
#include "DlgMessage.h"

#include "PCAUADlg.h"
#include "UACDlg.h"
#include "DlgJoinMessage.h"

#include "EscapeCoding.h"

#ifdef _XCAPSERVER
#include "xcapapi.h"
#endif
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// static class memebers

std::list<CDlgMessage*> CDlgMessage::m_lstMessageDlg;


/////////////////////////////////////////////////////////////////////////////
// CDlgMessage dialog


CDlgMessage::CDlgMessage( CWnd* pParentWnd, LPCSTR szURI, LPCSTR szDisplayName)
{

	m_strLocalName = g_pdlgUAControl->m_strRegistarName;
	m_strLocalURI = "sip:" + m_strLocalName + "@" + g_pdlgUAControl->m_strRegistarAddress;

	if (szURI)	// allow empty, modified by shen
	{
		CString strURI = _RemoveURIParam(szURI);
		CString uaName = GetNameFromURI(strURI);
		if ( !uaName.IsEmpty())
			AddParty( uaName, szDisplayName);
//		m_bOffline = !(g_pMainDlg->GetBuddyPresence(uaName));	// check party's presence 
	}
//	else
//		m_bOffline = false;

	m_bOffline = false;
	m_bNewConf = false;

//	if ( szURI)
//		AddParty( szURI, szDisplayName);
	_RegisterDlg( this);

	Create( IDD, pParentWnd);
	CRect r;
	GetClientRect( &r);
	SetWindowPos( NULL, 0,0,333,300, SWP_NOZORDER | SWP_NOMOVE | SWP_SHOWWINDOW );//SWP_NOSIZE | 

	// init displaying font style
	memset( &m_cfNames, 0, sizeof(m_cfNames));
	m_cfNames.cbSize = sizeof(m_cfNames);
	m_cfNames.dwMask = CFM_BOLD|CFM_COLOR|CFM_FACE|CFM_ITALIC|CFM_SIZE|CFM_STRIKEOUT|CFM_UNDERLINE;
	m_cfNames.dwEffects = 0;
	m_cfNames.yHeight = 9 * 20;
	m_cfNames.crTextColor = RGB(128,128,128);
	strcpy( m_cfNames.szFaceName, "Times New Roman");

	memset( &m_cfLocalText, 0, sizeof(m_cfLocalText));
	m_cfLocalText.cbSize = sizeof(m_cfLocalText);
	m_cfLocalText.dwMask = CFM_BOLD|CFM_COLOR|CFM_FACE|CFM_ITALIC|CFM_SIZE|CFM_STRIKEOUT|CFM_UNDERLINE;
	//m_cfLocalText.dwEffects = 0;
	//m_cfLocalText.yHeight = 9 * 20;
	//m_cfLocalText.crTextColor = RGB(0,0,0);
	//strcpy( m_cfLocalText.szFaceName, "Times New Roman");
	m_cfLocalText.dwEffects = AfxGetApp()->GetProfileInt( "MessageText", "Effect", 0);
	m_cfLocalText.yHeight = AfxGetApp()->GetProfileInt( "MessageText", "Height", 9 * 20);
	m_cfLocalText.crTextColor = AfxGetApp()->GetProfileInt( "MessageText", "Color", RGB(0,0,0) );
	strcpy( m_cfLocalText.szFaceName, AfxGetApp()->GetProfileString( "MessageText", "Face", "Times New Roman") );

	memset( &m_cfDefaultText, 0, sizeof(m_cfDefaultText));
	m_cfDefaultText.cbSize = sizeof(m_cfDefaultText);
	m_cfDefaultText.dwMask = CFM_BOLD|CFM_COLOR|CFM_FACE|CFM_ITALIC|CFM_SIZE|CFM_STRIKEOUT|CFM_UNDERLINE;
	m_cfDefaultText.dwEffects = 0;
	m_cfDefaultText.yHeight = 9 * 20;
	m_cfDefaultText.crTextColor = RGB(0,0,128);
	strcpy( m_cfDefaultText.szFaceName, "Times New Roman");

	// added by molisado, 20050309
	m_strLocalState = "idle";
	m_strLocalContentType = "text";
	m_RemoteLastActive = CTime::GetCurrentTime();
}

CDlgMessage::~CDlgMessage()
{
	_UnregisterDlg( this);
}


CDlgMessage* CDlgMessage::GetDlgByMessage( LPCSTR szRemoteURI, LPCSTR szMessage, LPCSTR szDisplayName)
{
	// match by conference id in the message
//	bool bNewConf = false;
	unsigned int unFlag = 0;	// 1 for new conf, 2 for join, 3 for leave

	// match by remote URI
	CString strRemoteURI = _RemoveURIParam(szRemoteURI);

	//add by alan, modified by shen
	CString uaName = GetNameFromURI(strRemoteURI);
	
	if ( szMessage)
	{
		MSXML::IXMLDOMDocumentPtr spXMLDoc;	
		spXMLDoc.CreateInstance(MSXML::CLSID_DOMDocument);
		if ( spXMLDoc->loadXML( szMessage))
		{
			bool bLeave = false;
			CString strConfID("");
			MSXML::IXMLDOMNodePtr spCmd = spXMLDoc->selectSingleNode( "/Message/Command");
			if (spCmd)
			{
				MSXML::IXMLDOMNodePtr spCmdType = spCmd->selectSingleNode( "LeaveConference");
				if (spCmdType)	// leave conference
					unFlag = 3;
//					bLeave = true;
				else
				{
					spCmdType = spCmd->selectSingleNode( "JoinConference");
					if (spCmdType)
						unFlag = 2;
//						bNewConf = true;
					else 
					{
						spCmdType = spCmd->selectSingleNode( "NewConference");
						if (spCmdType)
							unFlag = 1;
					}
				}

				if (spCmdType)
				{
					MSXML::IXMLDOMNodePtr spConfID = spCmdType->selectSingleNode( "ConferenceID");
					if (spConfID)
					{
						strConfID = (LPCSTR)spConfID->text;
						if (strConfID.IsEmpty())
							return NULL;	// failed
					}
				}
			}
			else
			{
				MSXML::IXMLDOMNodePtr spConfID = spXMLDoc->selectSingleNode("/Message/Header/ConferenceID");
				if (spConfID)
				{
					strConfID = (LPCSTR)spConfID->text;
					if (strConfID.IsEmpty())
						return NULL;	// failed
				}
			}

			// added by molisado, 20050309
			// isComposing handling
			MSXML::IXMLDOMNodePtr spIsCom = spXMLDoc->selectSingleNode("/isComposing");
			if( spIsCom )
			{
				MSXML::IXMLDOMNodePtr spConfID = spIsCom->selectSingleNode("cclimps:ConferenceID");
				if (spConfID)
				{
					strConfID = (LPCSTR)spConfID->text;
					if (strConfID.IsEmpty())
						return NULL;	// failed
					unFlag = 5;
				}
				else
					unFlag = 4;
			}

			if (!strConfID.IsEmpty() && unFlag != 1)
			{
				// if not a new one, need to find the existed conference
				std::list<CDlgMessage*>::iterator it;
				for ( it = m_lstMessageDlg.begin(); it != m_lstMessageDlg.end(); it++)
				{
					CDlgMessage* dlg = *it;
					if ( dlg->m_strConferenceID == strConfID)
					{
						// matched
						if (dlg->IsPartyOf(uaName))
							return dlg;
						else
							return NULL;	// conf ID matched, but party not
					}
				}
				// not found, need to know if it is a command to leave conference
				// modified by molisado, 20050309
				if ( (unFlag == 3) || (unFlag == 5) ) //(bLeave)
					// it is an invalid command, failed
					return NULL;
			}
		}

		// added by molisado, 20050406
		// check if the message is an offline message of conference or isComposing
		// because offline messages have a leading element "<This is an offline message...>"
		// so offline conference or isComposing messages will not be regarded as XML document
		CString strMsg( szMessage );
		if( ( strMsg.Find( "<This is a offline message sent at" ) != -1 ) &&
			( strMsg.Find( "<?xml version=" ) != -1 ) &&
			( ( strMsg.Find( "<isComposing xmlns=" ) != -1 ) ||
			  ( strMsg.Find( "<ConferenceID>" ) != -1 ) ) )
			return NULL;
	}


	if (unFlag != 1)
	{
		std::list<CDlgMessage*>::iterator it;
		for ( it = m_lstMessageDlg.begin(); it != m_lstMessageDlg.end(); it++)
		{
			CDlgMessage* dlg = *it;

			// modified by shen
			if (dlg->m_vecPartyInfo.size() != 1)
				continue;	// need to be matched exactly - shen
				
			if (dlg->IsPartyOf(uaName))
				return dlg;
/*		std::vector<CPartyInfo>::iterator it2;
		for (it2 = dlg->m_vecPartyInfo.begin(); it2 != dlg->m_vecPartyInfo.end(); it2++)
		{
			if ( it2->m_strURI == uaName)
			{
				// matched.
				return dlg;
			}
		}*/
		}

		// added by molisado, 20050309
		if( unFlag == 4 )
			return NULL;
	}

	// create new dialog on-the-fly
	CDlgMessage* dlg = new CDlgMessage( g_pMainDlg, strRemoteURI, szDisplayName);
	dlg->m_bNewConf = (unFlag == 1 || unFlag == 2);//bNewConf;
	return dlg;
}

void CDlgMessage::_RegisterDlg( CDlgMessage* dlg)
{
	std::list<CDlgMessage*>::iterator it;
	for ( it = m_lstMessageDlg.begin(); it != m_lstMessageDlg.end(); it++)
	{
		if ( *it == dlg)
			return;	// already exist in list
	}

	m_lstMessageDlg.push_back( dlg);
}

void CDlgMessage::_UnregisterDlg( CDlgMessage* dlg)
{
	std::list<CDlgMessage*>::iterator it;
	for ( it = m_lstMessageDlg.begin(); it != m_lstMessageDlg.end(); it++)
	{
		if ( *it == dlg)
		{
			m_lstMessageDlg.erase(it);
			return;
		}
	}
}


void CDlgMessage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgMessage)
	DDX_Control(pDX, IDC_SEND, m_Send);
	DDX_Control(pDX, IDC_MESSAGE, m_Message);
	DDX_Control(pDX, IDC_DIALOG, m_Dialog);
	DDX_Control(pDX, IDC_LABEL, m_Label);
	DDX_Control(pDX, IDC_STATIC_HINT, m_staticHint);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgMessage, CDialog)
	//{{AFX_MSG_MAP(CDlgMessage)
	ON_BN_CLICKED(IDC_SEND, OnSend)
	ON_WM_SIZE()
	ON_COMMAND(ID_SETFONT, OnSetfont)
	ON_COMMAND(ID_JOIN_CONFERENCE, OnJoinConference)
	ON_EN_UPDATE(IDC_MESSAGE, OnUpdateMessage)
	ON_WM_CLOSE()
	ON_UPDATE_COMMAND_UI(ID_JOIN_CONFERENCE, OnUpdateJoinConference)
	ON_EN_CHANGE(IDC_MESSAGE, OnChangeMessage)
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgMessage message handlers

void CDlgMessage::OnSend() 
{
	// get user input
	CString strText;
	m_Message.GetWindowText( strText);

	// clear user input
	m_Message.SetWindowText( "");

	// set focuse to user input again
	m_Message.SetFocus();

	// show text 
	_DisplayMessage( NULL, strText);
	
	// added by molisado, 20050406
	// wrap message into xml format if this is a conference
	CString strMsg;
	int option;
	if ( m_strConferenceID.IsEmpty() )
	{
		strMsg = strText;
		option = OneToOne_MESSAGE;
	}
	else
	{
		std::string tmpString = strText, retString;
		retString = Escape(tmpString);
		strMsg.Format( 
				"<?xml version=\"1.0\"?>\n"
				"<Message version=\"1.0\">\n"
				"\t<Header>\n"
				"\t\t%s\n"
				"\t<ConferenceID>%s</ConferenceID>\n"
				"\t</Header>\n"
				"\t<Body>%s</Body>\n"
				"</Message>\n", 
					_Font2XML( m_cfLocalText),
					(const char*) m_strConferenceID,
					retString.c_str());
		option = CONFERENCE_MESSAGE;
	}
	
	// send text out
	// modified by molisado, 20050406
	// change strText variable to strMsg and second variable to option
	SendInstMsg( strMsg, option );

	// added by molisado, 20050309
	// isComposing handling
	m_strLocalState = "idle";

	// stop timer
	KillTimer( ID_TIMER_ACTIVE );
	KillTimer( ID_TIMER_IDLE );
}

void CDlgMessage::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	
	if ( !IsWindow(m_Dialog.m_hWnd))
		return;

	// re-layout UI
	CRect rtClient, rtDialog, rtMessage, rtLabel, rtSend, rtHint;
	GetClientRect( &rtClient);
	_GetWindowPos( m_Dialog, rtDialog);
	_GetWindowPos( m_Message, rtMessage);
	_GetWindowPos( m_Label, rtLabel);
	_GetWindowPos( m_Send, rtSend);
	_GetWindowPos( m_staticHint, rtHint);

	// offset between border and control
	int margin = rtMessage.left;

	// resize width of m_Label and m_Dialog
	int width = rtClient.right - margin*2;
	rtLabel.right = rtLabel.left + width;
	rtDialog.right = rtDialog.left + width;
	rtHint.right = rtHint.left + width;

	// resize height of m_Dialog
	int height = rtClient.bottom - margin*2 - rtMessage.Height() - rtDialog.top - rtHint.Height();
	rtDialog.bottom = rtDialog.top + height;

	// move top of m_Message
	int top = rtDialog.bottom + margin;
	rtMessage.bottom = rtMessage.Height() + top;
	rtMessage.top = top;

	// move top of m_Send
	rtSend.bottom = rtSend.Height() + top;
	rtSend.top = top;

	// move top of m_staticHint
	rtHint.bottom = rtHint.Height() + top + rtMessage.Height();
	rtHint.top = top + rtMessage.Height();

	// resize width of m_Message
	width = rtClient.right - margin*3 - rtSend.Width();
	rtMessage.right = rtMessage.left + width;

	// move left of m_Send
	int left = rtMessage.right + margin;
	rtSend.right = rtSend.Width() + left;
	rtSend.left = left;
	
	// update control position
	_SetWindowPos( m_Dialog, rtDialog);
	_SetWindowPos( m_Message, rtMessage);
	_SetWindowPos( m_Label, rtLabel);
	_SetWindowPos( m_Send, rtSend);
	_SetWindowPos( m_staticHint, rtHint);
}

void CDlgMessage::_GetWindowPos( CWnd& wnd, CRect& rt)
{
	wnd.GetWindowRect( &rt);
	ScreenToClient( &rt);
}

void CDlgMessage::_SetWindowPos( CWnd& wnd, CRect& rt)
{
	wnd.SetWindowPos( NULL, rt.left, rt.top, rt.Width(), rt.Height(), SWP_NOZORDER|SWP_SHOWWINDOW);
}

BOOL CDlgMessage::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	m_hIcon = AfxGetApp()->LoadIcon(IDI_MESSAGE);
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// load menu bar
	m_Menu.LoadMenu( IDR_MESSAGE);
	SetMenu( &m_Menu);

	// init rich edit
	PARAFORMAT pf;
	pf.cbSize = sizeof(pf);
	pf.dwMask = PFM_TABSTOPS;
	pf.cTabCount = 1;
	pf.rgxTabs[0] = 200;			// change tab stop position
	m_Dialog.SetParaFormat( pf);

	// modified by molisado, 20050406
	// to facilitate OnChangeMessage() and OnUpdateMessage()
	m_Message.SetEventMask( ENM_CHANGE|ENM_UPDATE );

	// update display
	_UpdatePartyInfo();

//	if (m_bOffline && IsWindow( m_staticHint.m_hWnd))
//		m_staticHint.SetWindowText("unknown or off-line destination.");

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgMessage::OnCancel()
{
	// close modeless dialog
	DestroyWindow();
}

void CDlgMessage::PostNcDestroy() 
{
	CDialog::PostNcDestroy();

	// free object
	delete this;
}

////////////////////////////////////////////////////////////////////////////////////

bool CDlgMessage::AddParty( LPCSTR szURI, LPCSTR szDisplayName)
{
	CString strURI = _RemoveURIParam(szURI);
	CString strLocal = GetNameFromURI(m_strLocalURI);	// added by shen

	if ( strURI == strLocal)	// modified by shen //m_strLocalURI)
		return false;

	std::vector<CPartyInfo>::iterator it;
	for ( it = m_vecPartyInfo.begin(); it != m_vecPartyInfo.end(); it++)
	{
		if ( it->m_strURI == strURI)
			return false;
	}

	CString strName = szDisplayName;
	if ( strName.IsEmpty() )
	{
		strName = GetNameFromURI(strURI);
	}

	m_vecPartyInfo.push_back( CPartyInfo(strURI,strName));
	_UpdatePartyInfo();

	return true;
}

bool CDlgMessage::RemoveParty( LPCSTR szURI)
{
	CString strURI = _RemoveURIParam(szURI);

	std::vector<CPartyInfo>::iterator it;
	for ( it = m_vecPartyInfo.begin(); it != m_vecPartyInfo.end(); it++)
	{
		if ( it->m_strURI == strURI)
		{
			m_vecPartyInfo.erase(it);
			_UpdatePartyInfo();

	// deleted by shen, 20041225
			// stop conference mode if # of remote party < 2
//			if ( !m_strConferenceID.IsEmpty() && m_vecPartyInfo.size() < 2)
//			{
//				m_strConferenceID = "";	// stop conference mode
//			}

			return true;
		}
	}

	return false;
}

CString CDlgMessage::_PartyInfo2Text( bool bShowURI)
{
	CString s;
	CString strToURI;
	std::vector<CPartyInfo>::iterator it;
	for ( it = m_vecPartyInfo.begin(); it != m_vecPartyInfo.end(); it++)
	{
		if ( !s.IsEmpty())
			s += ", ";

	//modified by alan
		if ( CUACDlg::GetUAComRegDW( "Call_Server", "Use_Call_Server", 0) )
		{
			strToURI.Format( "sip:%s@%s:%d", it->m_strURI,
				CUACDlg::GetUAComRegString("Call_Server","Call_Server_Addr"), 
				CUACDlg::GetUAComRegDW("Call_Server","Call_Server_Port",5060) );
		}
		else if(CUACDlg::GetUAComRegDW( "SIMPLE_Server", "Use_SIMPLE_Server", 0))
		{
			strToURI.Format( "sip:%s@%s:%d", it->m_strURI,
				CUACDlg::GetUAComRegString("SIMPLE_Server","SIMPLE_Server_Addr"), 
				CUACDlg::GetUAComRegDW("SIMPLE_Server","SIMPLE_Server_Port",5060) );
		}
		else
		{
			strToURI = it->m_strURI;
		}
		s+=strToURI;
			
/*		s += it->m_strName;

		if ( bShowURI)
		{
			s += "(";
			s += it->m_strURI;
			s += ")";
		}
*/
	}

	return s;
}


void CDlgMessage::_UpdatePartyInfo()
{
	if ( IsWindow( m_hWnd))
		SetWindowText( "Message - " + _PartyInfo2Text(false));

	if ( IsWindow( m_Label.m_hWnd))
		m_Label.SetWindowText( "To: " + _PartyInfo2Text(true));
}

void CDlgMessage::_UpdateHint(LPCSTR szHint, bool bOffline)
{
	if ( IsWindow( m_staticHint.m_hWnd))
		m_staticHint.SetWindowText(szHint);

	m_bOffline = bOffline;
	if (bOffline)
	{
//		m_Send.EnableWindow(false);
//		m_Message.EnableWindow(false);
	}
}

CDlgMessage::CPartyInfo* CDlgMessage::_GetPartyInfo( LPCSTR szRemoteURI)
{
	CString strRemoteURI = _RemoveURIParam(szRemoteURI);

	std::vector<CPartyInfo>::iterator it;
	for ( it = m_vecPartyInfo.begin(); it != m_vecPartyInfo.end(); it++)
	{
		if ( it->m_strURI == strRemoteURI)
			return &(*it);
	}
	return NULL;
}

void CDlgMessage::_DisplayMessage( LPCSTR szRemoteURI, LPCSTR szMessage, bool bCommand)
{
	CString strRemoteURI = _RemoveURIParam(szRemoteURI);

	CPartyInfo* pPartyInfo = NULL;
	CString strName;

	if ( !szRemoteURI)
		strName = m_strLocalName;
	else
	{
		strName = strRemoteURI;
		pPartyInfo = _GetPartyInfo( strRemoteURI);
		if ( pPartyInfo && !pPartyInfo->m_strName.IsEmpty())
			strName = pPartyInfo->m_strName;
	}

	// show name
	// modified by shen
	CString strText;
	if (bCommand)
		strText.Format( "(%s)\n", strName );
	else
		strText.Format( "%s says:\n", strName );

	m_Dialog.SetSel(-1,-1);
	m_Dialog.SetWordCharFormat(m_cfNames);
	m_Dialog.ReplaceSel(strText);
	
	// show text
	if (szMessage)
	{
		strText = szMessage;
		strText.Replace( "\n", "\n\t");
		strText = "\t" + strText + "\n";

		m_Dialog.SetSel(-1,-1);
		if ( !szRemoteURI)
			m_Dialog.SetWordCharFormat(m_cfLocalText);
		else
		{
			if ( pPartyInfo)
				m_Dialog.SetWordCharFormat( pPartyInfo->m_cfText);
			else
				m_Dialog.SetWordCharFormat(m_cfDefaultText);
		}
		m_Dialog.ReplaceSel(strText);

		// restore font style
		m_Dialog.SetSel(-1,-1);
		m_Dialog.SetWordCharFormat(m_cfNames);
	}	// show message

	// scroll edit
	m_Dialog.SendMessage( WM_VSCROLL, SB_BOTTOM);
}

/* Message format:

	<?xml version="1.0"?>
	<Message version="1.0">
		<Header>
			<Font>
				<FaceName>Times New Roman</FaceName>
				<Height>180</Height>
				<Color>0</Color>
				<Bold/>
				<Italic/>
				<Strikeout/>
				<Underline/>
			</Font>

			[ <ConferenceID> ....GUID... </ConferenceID> ] /// option....
		</Header>
		<Body>
			.....message...here....
		</Body>
	</Message>


	<?xml version="1.0"?>
	<Message version="1.0">
		<Command>
			<JoinConference>
				<ConferenceID> ....GUID.... </ConferenceID>
				<Party> ....party1's uri... </Party>
				<Party> ....party2's uri... </Party>
				<Party> ....party3's uri... </Party>
			</JoinConference>
		</Command>
	</Message>

	<?xml version="1.0"?>
	<Message version="1.0">
		<Command>
			<LeaveConference>
				<ConferenceID> ....GUID.... </ConferenceID>
				<Party> ....party's uri... </Party>
			</LeaveConference>
		</Command>
	</Message>

*/

void CDlgMessage::_XML2Font( MSXML::IXMLDOMNodePtr spFont, CHARFORMAT& cf)
{
	memcpy( &cf, &m_cfDefaultText, sizeof(cf));
	cf.cbSize = sizeof(cf);
		
	MSXML::IXMLDOMNodePtr spNode = spFont->selectSingleNode("FaceName");
	if ( spNode)
		strcpy( cf.szFaceName, spNode->text);
	if ( (spNode = spFont->selectSingleNode("Height")) )
		cf.yHeight = atoi( spNode->text);
	if ( (spNode = spFont->selectSingleNode("Color")) )
		cf.crTextColor = (DWORD)atoi( spNode->text);

	if ( spFont->selectSingleNode("Bold") )
		cf.dwEffects |= CFE_BOLD;
	else
		cf.dwEffects &= ~CFE_BOLD;
	if ( spFont->selectSingleNode("Italic") )
		cf.dwEffects |= CFE_ITALIC;
	else
		cf.dwEffects &= ~CFE_ITALIC;
	if ( spFont->selectSingleNode("Strikeout") )
		cf.dwEffects |= CFE_STRIKEOUT;
	else
		cf.dwEffects &= ~CFE_STRIKEOUT;
	if ( spFont->selectSingleNode("Underline") )
		cf.dwEffects |= CFE_UNDERLINE;
	else
		cf.dwEffects &= ~CFE_UNDERLINE;

	cf.dwMask = CFM_BOLD|CFM_COLOR|CFM_FACE|CFM_ITALIC|CFM_SIZE|CFM_STRIKEOUT|CFM_UNDERLINE;
}

CString CDlgMessage::_Font2XML( CHARFORMAT& cf)
{
	CString xml;
	xml.Format( "<Font><FaceName>%s</FaceName><Height>%d</Height><Color>%d</Color>%s%s%s%s</Font>",
		cf.szFaceName, cf.yHeight, cf.crTextColor,
		(cf.dwEffects&CFE_BOLD)?"<Bold/>":"",
		(cf.dwEffects&CFE_ITALIC)?"<Italic/>":"",
		(cf.dwEffects&CFE_STRIKEOUT)?"<Strikeout/>":"",
		(cf.dwEffects&CFE_UNDERLINE)?"<Underline/>":"" );

	return xml;
}


// modified by molisado, 20050406
void CDlgMessage::SendInstMsg( LPCSTR szMessage, int option)
{
	// modified by molisado, 20050406
	// remove the block which wraps conference message into xml
	// indent the rest code one more level (under the switch structure)
	char tmp_buf[128];
	std::vector<CPartyInfo>::iterator it;
	switch( option )
	{
	case OneToOne_MESSAGE:
		for ( it = m_vecPartyInfo.begin(); it != m_vecPartyInfo.end(); it++)
		{
			sprintf(tmp_buf,
					"sip:%s@%s:%d",
					it->m_strURI,
					CUACDlg::GetUAComRegString("SIMPLE_Server", "SIMPLE_Server_Addr"),
					CUACDlg::GetUAComRegDW("SIMPLE_Server","SIMPLE_Server_Port",5060));
			g_pdlgUAControl->UAControlDriver.SendText( tmp_buf, szMessage);
		}
		break;
	case CONFERENCE_MESSAGE:
	case isComposing_COMMAND:
		for ( it = m_vecPartyInfo.begin(); it != m_vecPartyInfo.end(); it++)
		{
			if( g_pMainDlg->GetBuddyPresence(it->m_strURI)==Presence_Offline )	// Peer must be online
				continue;
			sprintf(tmp_buf,
					"sip:%s@%s:%d",
					it->m_strURI,
					CUACDlg::GetUAComRegString("SIMPLE_Server", "SIMPLE_Server_Addr"),
					CUACDlg::GetUAComRegDW("SIMPLE_Server","SIMPLE_Server_Port",5060));
			g_pdlgUAControl->UAControlDriver.SendText( tmp_buf, szMessage);
		}
		break;
	}

/* The following are legacy code
   commented by molisado, 20050406
	CString strMsg;

	if ( bXML)
		strMsg = szMessage;
	else
	{
		std::string tmpString = szMessage,retString;
		retString = Escape(tmpString);
		if (m_strConferenceID.IsEmpty())
/*			strMsg.Format( 
				"<?xml version=\"1.0\"?>\n"
				"<Message version=\"1.0\">\n"
				"\t<Header>\n"
				"\t\t%s\n"
				"\t</Header>\n"
				"\t<Body>%s</Body>\n"
				"</Message>\n", 
					_Font2XML( m_cfLocalText),
					szMessage);
/				strMsg.Format( 
				"<?xml version=\"1.0\"?>\n"
				"<Message version=\"1.0\">\n"
				"\t<Header>\n"
				"\t\t%s\n"
				"\t</Header>\n"
				"\t<Body>%s</Body>\n"
				"</Message>\n", 
					_Font2XML( m_cfLocalText),
					retString.c_str());

		else
/*			strMsg.Format( 
				"<?xml version=\"1.0\"?>\n"
				"<Message version=\"1.0\">\n"
				"\t<Header>\n"
				"\t\t%s\n"
				"\t<ConferenceID>%s</ConferenceID>\n"
				"\t</Header>\n"
				"\t<Body>%s</Body>\n"
				"</Message>\n", 
					_Font2XML( m_cfLocalText),
					(const char*) m_strConferenceID,
					szMessage);
/			strMsg.Format( 
				"<?xml version=\"1.0\"?>\n"
				"<Message version=\"1.0\">\n"
				"\t<Header>\n"
				"\t\t%s\n"
				"\t<ConferenceID>%s</ConferenceID>\n"
				"\t</Header>\n"
				"\t<Body>%s</Body>\n"
				"</Message>\n", 
					_Font2XML( m_cfLocalText),
					(const char*) m_strConferenceID,
					retString.c_str());
	}

	std::vector<CPartyInfo>::iterator it;

	//add by alan
//	char *_pre_split_pos,*_port_split_pos;
	char tmp_buf[128];
//	int tmp_len;
	for ( it = m_vecPartyInfo.begin(); it != m_vecPartyInfo.end(); it++)
	{
		//add by alan

		sprintf(tmp_buf,"sip:%s@%s:%d",it->m_strURI,CUACDlg::GetUAComRegString("SIMPLE_Server", "SIMPLE_Server_Addr"),CUACDlg::GetUAComRegDW("SIMPLE_Server","SIMPLE_Server_Port",5060));
		g_pdlgUAControl->UAControlDriver.SendText( tmp_buf, strMsg);
/*
		_pre_split_pos=strstr(it->m_strURI,"sip");
		if(_pre_split_pos!=NULL)
		{
			_port_split_pos = strstr((LPSTR)(LPCTSTR)(it->m_strURI)+4,":");
			if(_port_split_pos==NULL)//no port assigned....assigne the SIMPLE Server port
				sprintf(tmp_buf,"%s:%d",it->m_strURI,CUACDlg::GetUAComRegDW("SIMPLE_Server","SIMPLE_Server_Port",5060));
			else
			{
				tmp_len = strlen(_port_split_pos);
				strncpy(tmp_buf1,it->m_strURI,strlen((LPSTR)(LPCTSTR)(it->m_strURI))-tmp_len);
				tmp_buf1[strlen((LPSTR)(LPCTSTR)(it->m_strURI))-tmp_len]='\0';
				sprintf(tmp_buf,"%s:%d",tmp_buf1,CUACDlg::GetUAComRegDW("SIMPLE","SIMPLE_Port",5060));
			}
			g_pdlgUAControl->UAControlDriver.SendText( tmp_buf, strMsg);
		}
		else
			return;
/		
	}
*/
}

void CDlgMessage::ReceiveInstMsg( LPCSTR szRemoteURI, LPCSTR szMessage)
{
	CString strRemoteURI = _RemoveURIParam(szRemoteURI);

	MSXML::IXMLDOMDocumentPtr spXMLDoc;	
	spXMLDoc.CreateInstance(MSXML::CLSID_DOMDocument);
/*	char buf[128],tmp_name[64];
	char *_split=strstr((char *)szRemoteURI,"@");
	if(_split!=NULL)
	{
	
		strncpy(tmp_name,(char *)szRemoteURI+4,strlen(szRemoteURI)-strlen(_split)-4);
		tmp_name[strlen(szRemoteURI)-strlen(_split)-4]='\0';
		if(GetRuleActionByURI(tmp_name,buf)!=false)
		{
			if(!_stricmp(buf,"block"))//if the user is blocked .....ignore it..
				return;
		}
	}
*/	bool bUseXML = spXMLDoc->loadXML( szMessage) != 0;


	if ( !bUseXML)
	{
		_DisplayMessage( strRemoteURI, szMessage);

		// added by molisado, 20050415
		// SetFocus when receiving content messages
		this->SetWindowPos( &wndTopMost, 0, 0, 0, 0, !SWP_NOZORDER|SWP_NOSIZE|SWP_NOMOVE );
		this->SetWindowPos( &wndNoTopMost, 0, 0, 0, 0, !SWP_NOZORDER|SWP_NOSIZE|SWP_NOMOVE );

		// added by molisado, 20050309
		// isComposing handling
		CPartyInfo* pPartyInfo = _GetPartyInfo( szRemoteURI );
		if ( pPartyInfo)
		{
			// change remote state to idle
			pPartyInfo->m_strRemoteState = "idle";

			// record the current time as the remote last active
			// no comparison is needed because it is impossible for the m_RemoteLastActive
			// to go beyond the current time
			m_RemoteLastActive = CTime::GetCurrentTime();

			// get the list of active parties to update hint
			int nCount = -1;
			CString ActivePartyList = _GetActiveParty( nCount );
			_UpdateHint( ActivePartyList );

			// shutdown timer if no active party exists
			if( nCount == 0 )
				KillTimer( ID_TIMER_REFRESH );
		}
	}
	else
	{
		// added by molisado 20050309
		// isComposing handling
		MSXML::IXMLDOMNodePtr spIsCom = spXMLDoc->selectSingleNode( "/isComposing");
		if( spIsCom )
		{
			CPartyInfo* pPartyInfo = _GetPartyInfo( szRemoteURI );
			if ( pPartyInfo)
			{
				MSXML::IXMLDOMNodePtr spState = spIsCom->selectSingleNode( "state" );
				if( spState )
				{
					CString strRemoteState = (LPCSTR)spState->text;
					if( strRemoteState == "active" )
					{
//						_ReceiveActiveStatusMsg( szRemoteURI, szMessage );
						// change remote state to active
						pPartyInfo->m_strRemoteState = "active";

						// get the list of active parties and update hint
						int nCount = -1;
						CString ActivePartyList = _GetActiveParty( nCount );
						_UpdateHint( ActivePartyList );

						// get active refresh timer from the message and set timer
						MSXML::IXMLDOMNodePtr spRefresh = spIsCom->selectSingleNode( "refresh" );
						if( spRefresh )
						{
							CString strRemoteRefresh = (LPCSTR)spRefresh->text;
							pPartyInfo->m_RemoteRefresh = atoi( strRemoteRefresh ) * 1000;
						}
						else
						{
							// the sender does not provide active refresh interval
							// set to the default value: 120 seconds
							pPartyInfo->m_RemoteRefresh = DEFAULT_ACTIVE_INTERVAL;
						}
						SetTimer( ID_TIMER_REFRESH, pPartyInfo->m_RemoteRefresh, NULL );
					}
					else	// all unknown states are treated as "idle"
					{
//						_ReceiveIdleStatusMsg( szRemoteURI, szMessage );
						if( pPartyInfo->m_strRemoteState = "active" )
						{
							// change remote state to idle
							pPartyInfo->m_strRemoteState = "idle";

/* commented by molisado, 20050309
 * since the lastactive is only updated upon receiving the content message
							// update this party's last active
							MSXML::IXMLDOMNodePtr spLastActive = spIsCom->selectSingleNode( "lastactive" );
							if( spLastActive )
							{
								CString strLastActive = (LPCSTR)spLastActive->text;
								CString temp;
								temp.Format( "%.4s", (const char *)strLastActive );
								int nRemoteYear = atoi( temp );
								temp.Format( "%.2s", ((const char *)strLastActive)+5 );
								int nRemoteMonth = atoi( temp );
								temp.Format( "%.2s", ((const char *)strLastActive)+8 );
								int nRemoteDay = atoi( temp );
								temp.Format( "%.2s", ((const char *)strLastActive)+11 );
								int nRemoteHour = atoi( temp );
								temp.Format( "%.2s", ((const char *)strLastActive)+14 );
								int nRemoteMinute = atoi( temp );
								temp.Format( "%.2s", ((const char *)strLastActive)+17 );
								int nRemoteSecond = atoi( temp );

								CTime RemoteLastActive(	nRemoteYear, 
														nRemoteMonth, 
														nRemoteDay, 
														nRemoteHour, 
														nRemoteMinute, 
														nRemoteSecond);

								// handling time zone
								_tzset();
								CTimeSpan TimeDifference( _timezone );
								RemoteLastActive -= TimeDifference;

								if( RemoteLastActive > m_RemoteLastActive )
									m_RemoteLastActive = RemoteLastActive;
							}
*/
							// get the list of active parties and update hint
							int nCount = -1;
							CString ActivePartyList = _GetActiveParty( nCount );
							_UpdateHint( ActivePartyList );

							// shutdown timer if no active party exists
							if( nCount == 0 )
								KillTimer( ID_TIMER_REFRESH );
						}
					}
				}
			}
		}
		else
		{
			// modified by molisado, 20050309
			// the following block is further indented
			// conference message handling
		
			MSXML::IXMLDOMNodePtr spCmd = spXMLDoc->selectSingleNode( "/Message/Command");
			if ( spCmd)
			{
				// it is a special command
				MSXML::IXMLDOMNodePtr spCmdType = spCmd->selectSingleNode( "JoinConference");
				if ( spCmdType == NULL)
					spCmdType = spCmd->selectSingleNode( "NewConference");
				if ( spCmdType)
				{
					// join conference command
	//				VERIFY( m_strConferenceID.IsEmpty());	// maybe have existed, modified by shen
					MSXML::IXMLDOMNodePtr spNode = spCmdType->selectSingleNode( "ConferenceID");
					if ( spNode)
						m_strConferenceID = (LPCSTR)spNode->text;
					MSXML::IXMLDOMNodeListPtr spPartyList = spCmdType->selectNodes( "Party");
					if ( spPartyList)
					{
						for ( int i=0; i<spPartyList->length; i++)
						{
							CString strURI = (LPCTSTR)spPartyList->item[i]->text;
							// added by shen
							CString strName = GetNameFromURI(strURI);
							if (AddParty(strName , NULL) && !m_bNewConf)	// strURI
								_DisplayMessage(strName+" joins", NULL, true);
						}
						m_bNewConf = false;
					}
				}
				spCmdType = spCmd->selectSingleNode( "LeaveConference");
				if ( spCmdType)
				{
					// leave conference command
	//				VERIFY( !m_strConferenceID.IsEmpty());
					if (m_strConferenceID.IsEmpty())	// modified by shen
					{
						// error...
					}
					MSXML::IXMLDOMNodePtr spNode = spCmdType->selectSingleNode( "ConferenceID");
					if ( (bool)spNode && m_strConferenceID == spNode->text)
					{
						MSXML::IXMLDOMNodeListPtr spPartyList = spCmdType->selectNodes( "Party");
						if ( spPartyList)
						{
							for ( int i=0; i<spPartyList->length; i++)
							{
								CString strURI = (LPCTSTR)spPartyList->item[i]->text;
								CString strName = GetNameFromURI(strURI);	// modified by shen
								if (RemoveParty( strName))	// strURI
								{
									// added by shen, 20041225
									_DisplayMessage(strName+" quits", NULL, true);
									_CheckRemainder();
								}
								else
								{	
									CString strLocal = GetNameFromURI(m_strLocalURI);
									if (strName == strLocal)
										_DisplayMessage(strLocal+" quits", NULL, true);
								}
							}
						}
					}
				}
			}
			else
			{
				// it is a message
				CPartyInfo* pPartyInfo = _GetPartyInfo( szRemoteURI);
				if ( pPartyInfo)
				{
					MSXML::IXMLDOMNodePtr spFont = spXMLDoc->selectSingleNode( "/Message/Header/Font");
					if ( spFont)
						_XML2Font( spFont, pPartyInfo->m_cfText);

					// modified by molisado, 20050309
					// originally outside the "if ( pPartyInfo) block"
					MSXML::IXMLDOMNodePtr spMsg = spXMLDoc->selectSingleNode( "/Message/Body");
					if ( spMsg)
					{
						//modified by alan
						
						//_DisplayMessage( szRemoteURI, spMsg->text);
						std::string tmpString = spMsg->text,retString;
						retString = Unescape(tmpString);
						_DisplayMessage( szRemoteURI, retString.c_str());

						// added by molisado, 20050418
						// SetFocus when receiving content messages
						this->SetWindowPos( &wndTopMost, 0, 0, 0, 0, !SWP_NOZORDER|SWP_NOSIZE|SWP_NOMOVE );
						this->SetWindowPos( &wndNoTopMost, 0, 0, 0, 0, !SWP_NOZORDER|SWP_NOSIZE|SWP_NOMOVE );

						// added by molisado, 20050309
						// isComposing handling
						// change remote state to idle
						pPartyInfo->m_strRemoteState = "idle";

						// record the current time as the remote last active
						// no comparison is needed because it is impossible for the m_RemoteLastActive
						// to go beyond the current time
						m_RemoteLastActive = CTime::GetCurrentTime();

						// get the list of active parties to update hint
						int nCount = -1;
						CString ActivePartyList = _GetActiveParty( nCount );
						_UpdateHint( ActivePartyList );

						// shutdown timer if no active party exists
						if( nCount == 0 )
							KillTimer( ID_TIMER_REFRESH );
					}
				}
			}
		}
	}

}


void CDlgMessage::OnSetfont() 
{
	LOGFONT lf;
	memset(&lf,0,sizeof(lf));
	strcpy( lf.lfFaceName, m_cfLocalText.szFaceName);
	lf.lfHeight = -m_cfLocalText.yHeight * 4/3 / 20 ;  // in units of a point
	lf.lfItalic = (m_cfLocalText.dwEffects & CFE_ITALIC) != 0;
	lf.lfUnderline = (m_cfLocalText.dwEffects & CFE_UNDERLINE) != 0;
	lf.lfStrikeOut  = (m_cfLocalText.dwEffects & CFE_STRIKEOUT) != 0;
	lf.lfWeight = (m_cfLocalText.dwEffects & CFE_BOLD)? FW_BOLD:FW_NORMAL;
	lf.lfCharSet = DEFAULT_CHARSET;

	CHOOSEFONT cf;
	memset(&cf,0,sizeof(cf));
	cf.lStructSize = sizeof(cf);
	cf.hwndOwner = m_hWnd;
	cf.lpLogFont = &lf;
	cf.iPointSize = m_cfLocalText.yHeight / 2;	// in units of 1/10 of a point
	cf.Flags = CF_EFFECTS|CF_INITTOLOGFONTSTRUCT|CF_BOTH;
	cf.rgbColors = m_cfLocalText.crTextColor;

	if ( !ChooseFont( &cf))
		return;

	// change font
	strcpy( m_cfLocalText.szFaceName, lf.lfFaceName);
	m_cfLocalText.yHeight = cf.iPointSize * 2;  // in units of twips ( 20 point)
	if ( lf.lfItalic)
		m_cfLocalText.dwEffects |= CFE_ITALIC;
	else
		m_cfLocalText.dwEffects &= ~CFE_ITALIC;
	if ( lf.lfUnderline)
		m_cfLocalText.dwEffects |= CFE_UNDERLINE;
	else
		m_cfLocalText.dwEffects &= ~CFE_UNDERLINE;
	if ( lf.lfStrikeOut)
		m_cfLocalText.dwEffects |= CFE_STRIKEOUT;
	else
		m_cfLocalText.dwEffects &= ~CFE_STRIKEOUT;
	if ( lf.lfWeight == FW_BOLD)
		m_cfLocalText.dwEffects |= CFE_BOLD;
	else
		m_cfLocalText.dwEffects &= ~CFE_BOLD;

	m_cfLocalText.crTextColor = cf.rgbColors;
	m_cfLocalText.dwMask = CFM_BOLD|CFM_COLOR|CFM_FACE|CFM_ITALIC|CFM_SIZE|CFM_STRIKEOUT|CFM_UNDERLINE;

	AfxGetApp()->WriteProfileInt( "MessageText", "Effect", m_cfLocalText.dwEffects );
	AfxGetApp()->WriteProfileInt( "MessageText", "Height", m_cfLocalText.yHeight );
	AfxGetApp()->WriteProfileInt( "MessageText", "Color", m_cfLocalText.crTextColor );
	AfxGetApp()->WriteProfileString( "MessageText", "Face", m_cfLocalText.szFaceName );
}

void CDlgMessage::OnJoinConference() 
{
	CDlgJoinMessage dlg;
	if ( dlg.DoModal() != IDOK)
		return;
	if ( dlg.m_vecTarget.GetSize() == 0)
		return;

	if (!CreateIMConference(&dlg.m_vecTarget))
		MessageBox( "Cannot invite this one to join conference\n"
			"who has been included in this conference session.", 
			"Message", MB_OK);

	return;

}

void CDlgMessage::OnUpdateMessage() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_UPDATE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	m_Send.EnableWindow( m_Message.GetTextLength() != 0);// && !m_bDisabled);	
}

void CDlgMessage::OnClose() 
{
	// added by molisado, 20050309
	// isComposing handling
	CString strMsg;
	CTime currentTime;
	CTimeSpan idle( 0, 0, 0, 15 );
	CString strCurrentTime;

	if ( !m_strConferenceID.IsEmpty())
	{
		if ( MessageBox( "If you leave a message conference, you will never receive messages from"
					" conferernce parties.\nAre you sure to leave conference now?", 
					"Message", MB_ICONQUESTION | MB_YESNO) != IDYES)
			return;

		// added by molisado, 20050309
		// isComposing handling
		if( m_strLocalState == "active" )
		{
			// change state to idle
			m_strLocalState = "idle";

			// stop timer
			KillTimer( ID_TIMER_ACTIVE );
			KillTimer( ID_TIMER_IDLE );

			// get current time and substract 15 seconds as last active time
			currentTime = CTime::GetCurrentTime();
			currentTime -= idle;
			strCurrentTime = currentTime.FormatGmt( "%Y-%m-%dT%H:%M:%SZ" );
			strMsg.Format(
					"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
					"<isComposing xmlns=\"urn:ietf:params:xml:ns:im-iscomposing\" "
					"xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
					"xsi:schemaLocation=\"urn:ietf:params:xml:ns:im-composing "
					"iscomposing.xsd\" "
					"xmlns:cclimps=\"http://cclweb.itri.org.tw/k200/imps/\">\n"
					"\t<state>idle</state>\n"
					"\t<lastactive>%s</lastactive>\n"
					"\t<contenttype>text/plain</contenttype>\n"
					"\t<cclimps:ConferenceID>%s</cclimps:ConferenceID>\n"
					"</isComposing>\n",
					(const char *) strCurrentTime, 
					(const char *) m_strConferenceID );	// <lastactive> is not added
			SendInstMsg( strMsg, true );	// the message content is already XML
		}

		// notify other leave conference
		_LeaveConference();
	}
	else
	{
		// added by molisado, 20050309
		if( m_strLocalState == "active" )
		{
			// change state to idle
			m_strLocalState = "idle";

			// stop timer
			KillTimer( ID_TIMER_ACTIVE );
			KillTimer( ID_TIMER_IDLE );

			// get current time and substract 15 seconds as last active time
			currentTime = CTime::GetCurrentTime();
			currentTime -= idle;
			strCurrentTime = currentTime.FormatGmt( "%Y-%m-%dT%H:%M:%SZ" );

			// send idle status message
			strMsg.Format(
				"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
				"<isComposing xmlns=\"urn:ietf:params:xml:ns:im-iscomposing\" "
				"xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
				"xsi:schemaLocation=\"urn:ietf:params:xml:ns:im-composing "
				"iscomposing.xsd\">\n"
				"\t<state>idle</state>\n"
				"\t<lastactive>%s</lastactive>\n"
				"\t<contenttype>text/plain</contenttype>\n"
				"</isComposing>\n",
				(const char *) strCurrentTime );
			SendInstMsg( strMsg, true );	// the message content is already XML
		}
	}
	
	CDialog::OnClose();
}

CString CDlgMessage::_RemoveURIParam( LPCSTR szURI)
{
	if ( !szURI)
		return CString();

	char* pSep = strchr( szURI, ';');
	if ( !pSep)
		return szURI;
	else
		return CString( szURI, pSep-szURI);
}

// added by shen
bool CDlgMessage::CreateIMConference(CArray<CString,CString&>* vecTarget, bool bNew)
{
	bool bSucceed = false;

	// prepare party list
	for ( int i=0; i<vecTarget->GetSize(); i++)
	{
		CString& strURI = (*vecTarget)[i];
		// added by shen
		CString strName;
		if ( strURI.Left(4) != "sip:")
		{
			strName = strURI;
			strURI = g_pMainDlg->ConvertNumberToURI(strURI);
		}
		else
		{
			strName = GetNameFromURI(strURI);	// added by shen
		}

//		if ( !_GetPartyInfo(strName))	//strURI
//		{
			// add the party list
		if (AddParty(strName , NULL))
		{
			bSucceed = true;
			if (!bNew)
				_DisplayMessage(strName+" joins", NULL, true);
		}
//		}
	}

	if (!bSucceed)
		return false;

	// prepare conference id
	if ( m_strConferenceID.IsEmpty())
	{
		UUID uuid;
		unsigned char* szUuid;
		UuidCreate( &uuid);
		UuidToString( &uuid, &szUuid);
		
		CString strLocal = GetNameFromURI(m_strLocalURI);
		m_strConferenceID.Format("%s-%s", (const char*) strLocal, (char*)szUuid);	// modified by shen
//		m_strConferenceID = (char*)szUuid;

		RpcStringFree( &szUuid);
	}

	CString strPartyList = "\t\t\t<Party>" + m_strLocalURI + "</Party>\n";
	for ( i=0; i<m_vecPartyInfo.size(); i++)
		strPartyList += "\t\t\t<Party>" + g_pMainDlg->ConvertNumberToURI(m_vecPartyInfo[i].m_strURI) + "</Party>\n";

	// invite other to join conference
	CString strMsg;
	strMsg.Format( 
		"<?xml version=\"1.0\"?>\n"
		"<Message version=\"1.0\">\n"
		"\t<Command>\n"
		"\t\t<%s>\n"
//		"\t\t<JoinConference>\n"
		"\t\t\t<ConferenceID>%s</ConferenceID>\n"
		"%s"
		"\t\t</%s>\n"
//		"\t\t</JoinConference>\n"
		"\t</Command>\n"
		"</Message>\n", 
		(bNew)?"NewConference":"JoinConference",
		m_strConferenceID,
		strPartyList, 
		(bNew)?"NewConference":"JoinConference");

	SendInstMsg( strMsg, true);
	return true;
}

CString CDlgMessage::GetNameFromURI(CString szURI)
{
	//add by alan
//	CString uaName;
	int p = szURI.Find(':');
	int q = szURI.Find('@',p+1);
	if (p <= 0 || q <= 0)
		return szURI;//CString("");
	return szURI.Mid( p+1, q-p-1);
//	uaName = strRemoteURI.Mid( p+1, q-p-1);
}

void CDlgMessage::_LeaveConference()
{
	// notify other leave conference
	CString strMsg;
	strMsg.Format( 
		"<?xml version=\"1.0\"?>\n"
		"<Message version=\"1.0\">\n"
		"\t<Command>\n"
		"\t\t<LeaveConference>\n"
		"\t\t\t<ConferenceID>%s</ConferenceID>\n"
		"\t\t\t<Party>%s</Party>\n"
		"\t\t</LeaveConference>\n"
		"\t</Command>\n"
		"</Message>\n", m_strConferenceID, m_strLocalURI);

	SendInstMsg( strMsg, true);
	m_strConferenceID = "";
}

// added by shen, 20041225
void CDlgMessage::UpdatePartyFromAll(LPCSTR szRemoteURI, bool bOnline)
{
	CString uaName = GetNameFromURI(szRemoteURI);
	std::list<CDlgMessage*>::iterator it;
	for ( it = m_lstMessageDlg.begin(); it != m_lstMessageDlg.end(); it++)
	{
		CDlgMessage* dlg = *it;

		std::vector<CPartyInfo>::iterator it2;
		for (it2 = dlg->m_vecPartyInfo.begin(); it2 != dlg->m_vecPartyInfo.end(); it2++)
		{
			if ( it2->m_strURI == uaName)
			{
				// matched.
				if (!bOnline)
					dlg->KickParty(uaName);
				dlg->_CheckRemainder();
				break;
			}
		}

	}
}

// added by shen, 20041225
void CDlgMessage::KickParty(CString strName)
{
	if (m_strConferenceID.IsEmpty())	// not a conference room
		return;

	if (!RemoveParty( strName))
		return;
	
	_DisplayMessage(strName+" quits", NULL, true);

	// notify others
	CString strMsg;
	strMsg.Format( 
		"<?xml version=\"1.0\"?>\n"
		"<Message version=\"1.0\">\n"
		"\t<Command>\n"
		"\t\t<LeaveConference>\n"
		"\t\t\t<ConferenceID>%s</ConferenceID>\n"
		"\t\t\t<Party>%s</Party>\n"
		"\t\t</LeaveConference>\n"
		"\t</Command>\n"
		"</Message>\n", m_strConferenceID, 
		g_pMainDlg->ConvertNumberToURI(strName));

	SendInstMsg( strMsg, true);
	
//	_CheckRemainder();
}

int CDlgMessage::GetCount()
{
	return m_lstMessageDlg.size();
}


void CDlgMessage::OfflineFromAll(bool bClose)
{
	std::list<CDlgMessage*>::iterator it;
	for ( it = m_lstMessageDlg.begin(); it != m_lstMessageDlg.end(); it++)
	{
		CDlgMessage* dlg = *it;

		if ( !dlg->m_strConferenceID.IsEmpty())
		{
			dlg->_LeaveConference();
		}
		if (bClose)
			// to close all conversation windows
			dlg->PostMessage( WM_CLOSE, 0, 0 );		
	}
}


void CDlgMessage::_CheckRemainder()
{
	// check the current status... //?
	if ( !m_strConferenceID.IsEmpty() && m_vecPartyInfo.size() < 2)
	{
		m_strConferenceID = "";	// stop conference mode
	}

	// for remainder party, check if he is my buddy
	bool bStillAlive = false;
	bool bSomeoneUnknown = false;
	std::vector<CPartyInfo>::iterator it;
	for (it = m_vecPartyInfo.begin(); it != m_vecPartyInfo.end(); it++)
	{
		if (g_pMainDlg->GetBuddyPresence(it->m_strURI))	// on-line
			// at least one online.
			bStillAlive = true;
		else
			bSomeoneUnknown = true;
	}

	if (bSomeoneUnknown)
	{
		if (m_strConferenceID.IsEmpty())
			// off-line message
			_UpdateHint("unknown or off-line destination.", true);
		else
			// give a hint
			_UpdateHint("some message will probably be missed due to unknown presence");
	}
	else
		_UpdateHint("", false);

/*	if (!bStillAlive)	
	{
		if (m_strConferenceID.IsEmpty())
		{
			// off-line message
			_UpdateHint("unknown or off-line destination.");
		}
		else
		{
			// disabled
			_UpdateHint("conference not allowed due to all parties unknown or off-line.", false);
		}
	}
	else
	{
		if (bSomeoneUnknown)
		{
			// give a hint
			_UpdateHint("some message will probably be missed due to unknown presence");
		}
//	}*/
	
}

bool CDlgMessage::IsPartyOf(CString strName)
{
	std::vector<CPartyInfo>::iterator it;
	for (it = m_vecPartyInfo.begin(); it != m_vecPartyInfo.end(); it++)
		{
			if ( it->m_strURI == strName)
			{
				// matched.
				return true;
			}
		}
	return false;
}

void CDlgMessage::OnUpdateJoinConference(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!m_bOffline);
}

void CDlgMessage::SetOffline(bool bOffline)
{
	if (bOffline)
		_UpdateHint("unknown or off-line destination.", true);
	else
		_UpdateHint("", false);
}

// added by molisado, 20050309
CString CDlgMessage::_GetActiveParty(int &nNumOfActiveParty)
{
	CString strActivePartyList = "";
	int count = 0;
	std::vector<CPartyInfo>::iterator it;
	for ( it = m_vecPartyInfo.begin(); it != m_vecPartyInfo.end(); it++)
	{
		if ( it->m_strRemoteState == "active" )
		{
			if( count > 0 )
				strActivePartyList += ", ";
			strActivePartyList += it->m_strName;
			count ++;
		}
	}
	switch( count )
	{
	case 0:
		strActivePartyList = "Last time message is typed: " +	// 上次收到訊息的時間
							 m_RemoteLastActive.Format( "%Y/%m/%d %H:%M:%S" );
		break;
	case 1:
		strActivePartyList += " is typing message...";		// 正在輸入訊息
		break;
	default:
		strActivePartyList = "Currently typing: " +	// 正在輸入訊息的使用者
							 strActivePartyList;
	}

	nNumOfActiveParty = count;
		
	return strActivePartyList;
}

// added by molisado, 20050309
void CDlgMessage::OnChangeMessage() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	// TODO: Add your control notification handler code here
	if( m_strLocalState == "idle" )
	{
		// change state to active
		m_strLocalState = "active";

		// start active refresh timer
		SetTimer( ID_TIMER_ACTIVE, DEFAULT_ACTIVE_INTERVAL, NULL );

		// send active status message
		CString strMsg;
		if( m_strConferenceID.IsEmpty() )
			strMsg.Format(
				"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
				"<isComposing xmlns=\"urn:ietf:params:xml:ns:im-iscomposing\" "
				"xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
				"xsi:schemaLocation=\"urn:ietf:params:xml:ns:im-composing "
				"iscomposing.xsd\">\n"
				"\t<state>active</state>\n"
				"\t<contenttype>text/plain</contenttype>\n"
				"\t<refresh>120</refresh>\n"
				"</isComposing>\n" );
		else
			strMsg.Format(
				"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
				"<isComposing xmlns=\"urn:ietf:params:xml:ns:im-iscomposing\" "
				"xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
				"xsi:schemaLocation=\"urn:ietf:params:xml:ns:im-composing "
				"iscomposing.xsd\" "
				"xmlns:cclimps=\"http://cclweb.itri.org.tw/k200/imps/\">\n"
				"\t<state>active</state>\n"
				"\t<contenttype>text/plain</contenttype>\n"
				"\t<refresh>120</refresh>\n"
				"\t<cclimps:ConferenceID>%s</cclimps:ConferenceID>\n"
				"</isComposing>\n",
				(const char *) m_strConferenceID );
		SendInstMsg( strMsg, true );	// the message content is already XML
	}

	// reset idle timer
	SetTimer( ID_TIMER_IDLE, DEFAULT_IDLE_INTERVAL, NULL );	
}

// added by molisado, 20050309
void CDlgMessage::OnTimer(UINT nIDEvent) 
{
	// TODO: Add your message handler code here and/or call default
	CString strMsg;
	CTime currentTime;
	CTimeSpan idle( 0, 0, 0, 15 );
	CTimeSpan activeRefresh( 0, 0, 0, 120 );
	CString strCurrentTime;
	switch( nIDEvent )
	{
	case ID_TIMER_ACTIVE: 
		// send active status message
		if( m_strConferenceID.IsEmpty() )
			strMsg.Format(
				"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
				"<isComposing xmlns=\"urn:ietf:params:xml:ns:im-iscomposing\" "
				"xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
				"xsi:schemaLocation=\"urn:ietf:params:xml:ns:im-composing "
				"iscomposing.xsd\">\n"
				"\t<state>active</state>\n"
				"\t<contenttype>text/plain</contenttype>\n"
				"\t<refresh>120</refresh>\n"
				"</isComposing>\n" );
		else
			strMsg.Format(
				"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
				"<isComposing xmlns=\"urn:ietf:params:xml:ns:im-iscomposing\" "
				"xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
				"xsi:schemaLocation=\"urn:ietf:params:xml:ns:im-composing "
				"iscomposing.xsd\" "
				"xmlns:cclimps=\"http://cclweb.itri.org.tw/k200/imps/\">\n"
				"\t<state>active</state>\n"
				"\t<contenttype>text/plain</contenttype>\n"
				"\t<refresh>120</refresh>\n"
				"\t<cclimps:ConferenceID>%s</cclimps:ConferenceID>\n"
				"</isComposing>\n",
				(const char *) m_strConferenceID );
		SendInstMsg( strMsg, true );	// the message content is already XML
		break;

	case ID_TIMER_IDLE:
		// change state to idle
		m_strLocalState = "idle";

		// stop timer
		KillTimer( ID_TIMER_ACTIVE );
		KillTimer( ID_TIMER_IDLE );

		// get current time and substract 15 seconds as last active time
		currentTime = CTime::GetCurrentTime();
		currentTime -= idle;
		strCurrentTime = currentTime.FormatGmt( "%Y-%m-%dT%H:%M:%SZ" );

		// send idle status message
		if( m_strConferenceID.IsEmpty() )
			strMsg.Format(
				"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
				"<isComposing xmlns=\"urn:ietf:params:xml:ns:im-iscomposing\" "
				"xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
				"xsi:schemaLocation=\"urn:ietf:params:xml:ns:im-composing "
				"iscomposing.xsd\">\n"
				"\t<state>idle</state>\n"
				"\t<lastactive>%s</lastactive>\n"
				"\t<contenttype>text/plain</contenttype>\n"
				"</isComposing>\n",
				(const char *) strCurrentTime );
		else
			strMsg.Format(
				"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
				"<isComposing xmlns=\"urn:ietf:params:xml:ns:im-iscomposing\" "
				"xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
				"xsi:schemaLocation=\"urn:ietf:params:xml:ns:im-composing "
				"iscomposing.xsd\" "
				"xmlns:cclimps=\"http://cclweb.itri.org.tw/k200/imps/\">\n"
				"\t<state>idle</state>\n"
				"\t<lastactive>%s</lastactive>\n"
				"\t<contenttype>text/plain</contenttype>\n"
				"\t<cclimps:ConferenceID>%s</cclimps:ConferenceID>\n"
				"</isComposing>\n",
				(const char *) strCurrentTime, 
				(const char *) m_strConferenceID );	// <lastactive> is not added
		SendInstMsg( strMsg, true );	// the message content is already XML
		break;

	case ID_TIMER_REFRESH:
		// change all party's state to idle
		std::vector<CPartyInfo>::iterator it;
		for ( it = m_vecPartyInfo.begin(); it != m_vecPartyInfo.end(); it++)
			it->m_strRemoteState = "idle";

/* commented by molisado, 20050309
 * since the lastactive is only updated upon receiving the content message
		// record the current time and substract 120 seconds
		currentTime = CTime::GetCurrentTime();
		currentTime -= activeRefresh;
		// update lastactive if necessary
		if( currentTime > m_RemoteLastActive )
			m_RemoteLastActive = currentTime;
*/
		// update hint
		int nCount = -1;
		CString ActivePartyList = _GetActiveParty( nCount );
		_UpdateHint( ActivePartyList );

		// shutdown timer if no active party exists
		KillTimer( ID_TIMER_REFRESH );
		break;
	}

	CDialog::OnTimer(nIDEvent);
}
