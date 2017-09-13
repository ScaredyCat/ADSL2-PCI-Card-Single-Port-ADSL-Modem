// PresManager.cpp: implementation of the CPresManager class.
//
//////////////////////////////////////////////////////////////////////
 

#include "PresManager.h"
#include "UAControl.h"
#include <uacore/Ua_cm.h>
#include <uacore/Ua_evtpkg.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#ifdef _simple

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPresManager* CPresManager::s_pPresManager = NULL;


CPresManager::CPresManager()
{
	s_pPresManager = this;
	m_BuddyLst = dxLstNew(DX_LST_POINTER);
	m_uaEvtpkg = NULL;
	m_MyPres = NULL;
	m_Presentity = NULL;
	m_CurrentState = PRES_OFFLINE;
}

CPresManager::~CPresManager()
{
	if (m_BuddyLst)
	{
		dxLstFree(m_BuddyLst, (void (*)(void *))BuddyFree);
		m_BuddyLst = NULL;
	}
	if (m_MyPres)
	{
		CPIMpresFree(m_MyPres);
		m_MyPres = NULL;
	}
	if (m_Presentity)
	{
		free(m_Presentity);
		m_Presentity = NULL;
	}
}

char* trimAQUOT(char*s)
{
	char* pre;

	if ( !s ) 
		return NULL;

	pre = s;

	if (pre[strlen(pre)-1] == '>')
		pre[strlen(pre)-1] = '\0';

	if (pre[0] == '<')
		pre++;

	if ( pre > s)
		strcpy(s, pre);

	return s;
}

void CPresManager::init()
{
	pCallManager = CCallManager::GetCallMgr();

	CUAProfile* pProfile = CUAProfile::GetProfile();

	MyCString tmpStr;

	tmpStr.Format(_T("sip:%s@"),pProfile->m_Username);
	tmpStr += pProfile->m_LocalAddr;

	MyCString portStr;
	portStr.Format(_T(":%d"), pProfile->m_LocalPort);
	tmpStr += portStr;
	
	if (m_Presentity)
		free(m_Presentity);

	m_Presentity = URIfromSIPtoPRES(tmpStr);
	trimAQUOT(m_Presentity);

	m_uaEvtpkg = uaEvtpkgNew("presence");
	m_uaClass = uaClassNew(m_Presentity);
	uaEvtpkgAddClass(m_uaEvtpkg, m_uaClass);
	uaEvtpkgSetSubCB(m_uaEvtpkg, SubStateCB);
	uaMgrInsertEvtpkg(pCallManager->m_UaMgr, m_uaEvtpkg);

//add evtpkg = presence.winfo, by alan

	m_uaEvtpkg = uaEvtpkgNew("presence.winfo");
	m_uaClass = uaClassNew(m_Presentity);
	uaEvtpkgAddClass(m_uaEvtpkg, m_uaClass);
	uaEvtpkgSetSubCB(m_uaEvtpkg, SubStateCB);
	uaMgrInsertEvtpkg(pCallManager->m_UaMgr, m_uaEvtpkg);

	ChangeBasicState(FALSE);
	
}

void CPresManager::stop()
{
	Buddy tmpBuddy = NULL;

	for (int i=0; i < dxLstGetSize(m_BuddyLst); i++)
	{
		tmpBuddy = (Buddy)dxLstPeek(m_BuddyLst, i);
		if (tmpBuddy)
		{
			//if the presentity is the user ....send unsubscribe with event= presence.winfo
			char tmpName[8];
			const char *presURI = GetBuddyPresentity(tmpBuddy);
			char *split_pos= strstr(presURI,":");
			CUAProfile* pProfile = CUAProfile::GetProfile();
			if(split_pos!=NULL)
			{
				strncpy(tmpName,presURI,split_pos-presURI-1);
				DebugMsg(tmpName);
				DebugMsg(pProfile->m_Username);
				if(!stricmp(tmpName,pProfile->m_Username))
				{
					UnsubscribeBuddy( GetBuddyPresentity(tmpBuddy),"presence.winfo" );
					continue;
				}
			}
			
				
			UnsubscribeBuddy( GetBuddyPresentity(tmpBuddy),"presence" );
			UnauthorizeBuddy( GetBuddyPresentity(tmpBuddy) );
		}
	}
	Sleep(100);
 

	pCallManager = CCallManager::GetCallMgr();
	/*uaMgrRemoveEvtpkg(pCallManager->m_UaMgr, "presence");

	if (m_uaClass) {
		uaEvtpkgDelClass(m_uaEvtpkg, m_Presentity);
		uaClassFree(m_uaClass);
		m_uaClass = NULL;
	}
	
	if (m_uaEvtpkg) {
		uaEvtpkgFree(m_uaEvtpkg);
		m_uaEvtpkg = NULL;
	}*/
	DelWinfoList();
}
 
UASubState CPresManager::SubStateCB(UaSub sub)
{
	CPresManager* pPresManager = CPresManager::GetPresMgr();
	Buddy tmpBuddy;

	if (!sub)
		return UASUB_NULL;

	UaDlg tmpDlg = uaSubGetDlg(sub);

	tmpBuddy = pPresManager->FindMatchBuddy( uaDlgGetRemoteParty(tmpDlg) );

	if (!tmpBuddy)
		return UASUB_PENDING;

	if ( GetBuddyBlock(tmpBuddy) )
		return UASUB_TERMINATED;

	if ( GetBuddyAuth(tmpBuddy) )
		return UASUB_ACTIVE;

	return UASUB_PENDING;
}

Buddy CPresManager::FindMatchBuddy(const char* strURI)
{
	Buddy tmpBuddy;

	if (!strURI)
		return NULL;
	
	for (int i=0; i < dxLstGetSize(m_BuddyLst); i++)
	{
		tmpBuddy = (Buddy)dxLstPeek(m_BuddyLst, i);
		if (_stricmp(GetBuddyPresentity(tmpBuddy), strURI) == 0)
			return tmpBuddy;
	}
	return NULL;
}

RCODE CPresManager::AuthorizeBuddy(const char* strURI)
{
	MyCString tmpStr;
	Buddy tmpBuddy = FindMatchBuddy(strURI);

	if (!tmpBuddy)
		return RC_ERROR;

	if (GetBuddyBlock(tmpBuddy))
	{
		tmpStr.Format(_T("[PresManager] Buddy %s alreay blocked, please unblock first"), strURI);
		DebugMsg(tmpStr);
		return RC_OK;
	}
	else
	{
		tmpStr.Format(_T("[PresManager] Authorize buddy %s"), strURI);
		DebugMsg(tmpStr);
	}

	SetBuddyAuth(tmpBuddy, TRUE);

	if (GetBuddyINState(tmpBuddy) == UASUB_PENDING)
		SetBuddyINState(tmpBuddy, UASUB_ACTIVE);

	NotifyBuddy(tmpBuddy);
	return RC_OK;
}

RCODE CPresManager::UnauthorizeBuddy(const char *strURI)
{
	MyCString tmpStr;
	Buddy tmpBuddy = FindMatchBuddy(strURI);

	if (!tmpBuddy)
		return RC_ERROR;

	if (GetBuddyBlock(tmpBuddy))
	{
		tmpStr.Format(_T("[PresManager] Buddy %s alreay blocked, please unblock first"), strURI);
		DebugMsg(tmpStr);
		return RC_OK;
	}
	else
	{
		tmpStr.Format(_T("[PresManager] UnAuthorize buddy %s"), strURI);
		DebugMsg(tmpStr);
	}
	SetBuddyAuth(tmpBuddy, FALSE);

	SetBuddyINState(tmpBuddy, UASUB_TERMINATED);
	NotifyBuddy(tmpBuddy);
	return RC_OK;
}
 
RCODE CPresManager::BlockBuddy(const char* strURI)
{
	Buddy tmpBuddy = FindMatchBuddy(strURI);
 
	if (!tmpBuddy)
		return RC_ERROR;

	char buf[128];
	char *_first_split,*_second_split;
	int tmp_len;
	//strip the port number
	_first_split = strstr(strURI,":");
	if(_first_split!=NULL)
	{
		_second_split = strstr(_first_split+1,":");
		if(_second_split!=NULL)
		{
			tmp_len = strlen(_second_split);
			strncpy(buf,strURI,strlen(strURI)-tmp_len);
			buf[strlen(strURI)-tmp_len]='\0';
			if(GetWatcherStatusFromWinfoLstByURI(buf)==WATCHER_NULL)
				return RC_ERROR;
		}
		else
			return RC_ERROR;
		
	}
	else
		return RC_ERROR;
	
	MyCString tmpStr;
	tmpStr.Format(_T("[PresManager] Block buddy %s"), strURI);
	DebugMsg(tmpStr);

	SetBuddyINState(tmpBuddy, UASUB_TERMINATED);
	SetBuddyOUTState(tmpBuddy, UASUB_TERMINATED);
	SetBuddyBlock(tmpBuddy, TRUE);
//	SetBuddyAuth(tmpBuddy, FALSE);
//	UnsubscribeBuddy(strURI,"presence");
//	NotifyBuddy(tmpBuddy);
	return RC_OK;
}

RCODE CPresManager::AddBuddy(const char* strURI)
{
	Buddy tmpBuddy = FindMatchBuddy(strURI);
	
	if (tmpBuddy)
		return RC_ERROR;

	tmpBuddy = BuddyNew(strURI);
	dxLstPutTail(m_BuddyLst, tmpBuddy);
	
	MyCString tmpStr;
	tmpStr.Format(_T("[PresManager] Add buddy %s"), strURI);
	DebugMsg(tmpStr);

	return RC_OK;
}

RCODE CPresManager::DelBuddy(const char* strURI)
{
	Buddy tmpBuddy;

	if (!strURI)
		return RC_ERROR;

	for (int i=0; i < dxLstGetSize(m_BuddyLst); i++)
	{
		tmpBuddy = (Buddy)dxLstPeek(m_BuddyLst, i);
		if (_stricmp(GetBuddyPresentity(tmpBuddy), strURI) == 0)
		{
			tmpBuddy = (Buddy)dxLstGetAt(m_BuddyLst, i);
			BuddyFree(tmpBuddy);
			tmpBuddy = NULL;

			MyCString tmpStr;
			tmpStr.Format(_T("[PresManager] Delete buddy %s"), strURI);
			DebugMsg(tmpStr);

			return RC_OK;
		}
	}
	return RC_ERROR;
}

void CPresManager::ChangeBasicState(BOOL on)
{
	CPIMpres pres = CPIMpresNew(m_Presentity);
	CPIMtuple tuple = CPIMtupleNew();

	/*
	char* myhostname = strDup("LAGUNA");
	char* myipaddr = strDup("140.96.102.4");

	if (myhostname) {
		free (myhostname);
		myhostname = NULL;
	}

	if (myipaddr) {
		free (myipaddr);
		myipaddr = NULL;
	}
	*/
	SetTupleID(tuple, "CCL_SIMPLE_CLIENT");

	CPIMstatus status = CPIMstatusNew();
	SetStatusBasic(status, on);
	SetTupleStatus(tuple, status);
	CPIMstatusFree(status);

	char* contactURI = URIfromPREStoSIP(m_Presentity);

	CPIMcontact contact = CPIMcontactNew(contactURI);
	SetTupleContact(tuple, contact);
	CPIMcontactFree(contact);

	DxLst tmpLst = dxLstNew(DX_LST_POINTER);
	dxLstPutTail(tmpLst, tuple);
	SetPRESTupleLst(pres, tmpLst);
	dxLstFree(tmpLst, (void (*)(void *))CPIMtupleFree);

	if (m_MyPres)
		CPIMpresFree(m_MyPres);

	m_MyPres = pres;

	MyCString tmpStr;
	if (on)
		tmpStr.Format(_T("[PresManager] Resource: %s [on]"), m_Presentity);
	else
		tmpStr.Format(_T("[PresManager] Resource: %s [off]"), m_Presentity);
	DebugMsg(tmpStr);

	if (contactURI)
	{
		free(contactURI);
		contactURI = NULL;
	}

	NotifyAll();
}

RCODE CPresManager::NotifyBuddy(Buddy tmpBuddy)
{
	UaDlg tmpDlg = NULL;
	UaSub tmpSub = NULL;
	UaContent tmpContent = NULL;

	if (!tmpBuddy)
		return RC_ERROR;

	const char* buddyURI = GetBuddyPresentity(tmpBuddy);
	char* tmpURI = URIfromPREStoSIP(buddyURI);

	char* contentbody = FromPREStoXML(m_MyPres);
	if (contentbody)
	{
		tmpContent = uaContentNew("application/cpim-pidf+xml",
										strlen(contentbody) + 1,
										contentbody);
		free(contentbody);
	}

	tmpSub = GetBuddyINSub(tmpBuddy);
	tmpDlg = uaSubGetDlg(tmpSub);

	if ( tmpSub && tmpDlg ) {
		MyCString tmpStr;
		tmpStr.Format(_T("[PresManager] Notify buddy %s"), buddyURI);
		DebugMsg(tmpStr);

		switch ( GetBuddyINState(tmpBuddy) ) {
		case UASUB_ACTIVE:
			uaSubNotify( tmpSub, tmpURI, tmpContent );
			break;
		
		case UASUB_TERMINATED:
			uaSubNotify( tmpSub, tmpURI, NULL );
			SetBuddyINSub(tmpBuddy, NULL);
			break;

		case UASUB_PENDING:
			uaSubNotify( tmpSub, tmpURI, NULL );
			break;

		default:
			break;
		}
	}

	if (tmpContent)
	{
		uaContentFree(tmpContent);
		tmpContent = NULL;
	}
	
	if (tmpURI)
	{
		free(tmpURI);
		tmpURI = NULL;
	}

	return RC_OK;
}

RCODE CPresManager::SubscribeBuddy(const char* strURI,BOOL isEventList,const char *strEvent,int expires)
{
	UaDlg tmpDlg = NULL;
	Buddy tmpBuddy = FindMatchBuddy(strURI);
	RCODE iRet = RC_OK;
	int addflag = 0;

	if (!tmpBuddy)
	{
		AddBuddy(strURI);
		addflag =1;
		tmpBuddy = FindMatchBuddy(strURI);
//		return FALSE;
	}		
	MyCString tmpStr;
	tmpStr.Format(_T("[PresManager] Entering SubscribeBuddy"));
	DebugMsg(tmpStr);

	if(strEvent==NULL)
	{
		iRet = RC_ERROR;
		return iRet;
	}


	UaSub tmpSub = GetBuddyOUTSub(tmpBuddy);
	
	const char* buddyURI = GetBuddyPresentity(tmpBuddy);
	char* tmpURI;
	if(buddyURI!=NULL)
		tmpURI = URIfromPREStoSIP(buddyURI);
	else
		tmpURI = URIfromPREStoSIP(strURI);

	if (tmpSub)
	{
		/* reuse original subscription */
		tmpDlg = uaSubGetDlg(tmpSub);
		uaSubSetExpires(tmpSub, expires);

		
	}
	else
	{
		tmpDlg = uaDlgNew(pCallManager->m_UaMgr);
		tmpSub = uaSubNew(tmpURI, expires, UASUB_ACTIVE, tmpDlg, m_uaEvtpkg, FALSE, FALSE);
		SetBuddyOUTSub(tmpBuddy, tmpSub);
		uaSubFree(tmpSub);
		tmpSub = GetBuddyOUTSub(tmpBuddy);
	}


	if(!isEventList)
		iRet = uaDlgSubscribe(tmpDlg, tmpSub,strEvent);
	else
		iRet = uaDlgSubscribeWithEventList(tmpDlg,tmpSub,strEvent);
	
//	MyCString tmpStr;
	tmpStr.Format(_T("[PresManager] Subscribe buddy %s---ret code= %d"), strURI,iRet);
	DebugMsg(tmpStr);

//	if(addflag)
//		DelBuddy(strURI);
	if (tmpURI)
	{
		free(tmpURI);
		tmpURI = NULL;
	}


	
	return iRet;
}

//add by alan for publish message

RCODE CPresManager:: PublishBuddy(const char* strURI, const char *event,BOOL basic_status, float priority,int expire)
{
	UaContent tmpContent = NULL;

	UaDlg tmpDlg = NULL;
	Buddy tmpBuddy = FindMatchBuddy(strURI);

//marked by alan
//	if (!tmpBuddy)
//		return FALSE;
	
	
	UaSub tmpSub = GetBuddyOUTSub(tmpBuddy);
	
	const char* buddyURI = GetBuddyPresentity(tmpBuddy);
	//char* tmpURI = URIfromPREStoSIP(buddyURI);
	char* tmpURI = URIfromPREStoSIP(strURI);
	if (tmpSub)
	{
		/* reuse original subscription */
		tmpDlg = uaSubGetDlg(tmpSub);
		uaSubSetExpires(tmpSub, expire);

		
	}
	else
	{
		tmpDlg = uaDlgNew(pCallManager->m_UaMgr);
		tmpSub = uaSubNew(tmpURI, expire, UASUB_ACTIVE, tmpDlg, m_uaEvtpkg, FALSE, FALSE);
		SetBuddyOUTSub(tmpBuddy, tmpSub);
		uaSubFree(tmpSub);
		tmpSub = GetBuddyOUTSub(tmpBuddy);
	}

//add by alan
//	char *eventname = uaEvtpkgGetName(uaSubGetEvtpkg(tmpSub));
//	if(strcmp(strEvent, eventname))
//	{ 		
//		free(eventname);
//		eventname = strDup(strEvent);
//	}


//	if(!isEventList)
//		uaDlgSubscribe(tmpDlg, tmpSub);
//	else
//		uaDlgSubscribeWithEventList(tmpDlg,tmpSub);


	DxLst tmpLst = GetPRESTupleLst(m_MyPres);
	CPIMtuple _tuple;
	CPIMstatus _status;
//	short basic;
	CPIMcontact _contact;

	if (!tmpLst)
		return 0;

	for (int i=0; i < dxLstGetSize(tmpLst); i++)
	{
		_tuple = (CPIMtuple)dxLstPeek(tmpLst, i);
		if (_tuple)
		{
			_status = GetTupleStatus(_tuple);
			if (_status)
				SetStatusBasic(_status,basic_status);
			_contact = GetTupleContact(_tuple);
			if(_contact)
			{
				SetContactPriority(_contact, priority);

				//change sip: to im:
				char *split_ptr = strstr(strURI,":");
				char tmp_uri[64];
				if(split_ptr!=NULL)
				{
					sprintf(tmp_uri,"im%s",split_ptr);
					SetContactURI(_contact, (const char *)tmp_uri);
				}
				
			}
		}
	}

	char* contentbody = FromPREStoXML(m_MyPres);
	if (contentbody)
	{
		tmpContent = uaContentNew("application/pidf+xml",
										strlen(contentbody) + 1,
										contentbody);
		free(contentbody);
	}

	uaDlgPublish(tmpDlg,strURI,event,NULL,tmpContent,expire);

	MyCString tmpStr;
	tmpStr.Format(_T("[PresManager] Publish User State %s"), strURI);
	DebugMsg(tmpStr);

	if (tmpURI)
	{
		free(tmpURI);
		tmpURI = NULL;
	}

	if (tmpContent)
	{
		uaContentFree(tmpContent);
		tmpContent = NULL;
	}
	
	return RC_OK;

}

void CPresManager::NotifyAll()
{
	UaDlg tmpDlg = NULL;
	UaSub tmpSub = NULL;
	Buddy tmpBuddy = NULL;
	UaContent tmpContent = NULL;

	char* contentbody = FromPREStoXML(m_MyPres);
	if (contentbody)
	{
		tmpContent = uaContentNew("application/cpim-pidf+xml",
										strlen(contentbody) + 1,
										contentbody);
		free(contentbody);
	}

	for (int i=0; i < dxLstGetSize(m_BuddyLst); i++)
	{
		tmpBuddy = (Buddy)dxLstPeek(m_BuddyLst, i);
		tmpSub = GetBuddyINSub(tmpBuddy);
		if ( tmpSub ) {
			if ( GetBuddyINState(tmpBuddy) == UASUB_ACTIVE ) {
				tmpDlg = uaSubGetDlg(tmpSub);
				uaSubNotify( tmpSub, uaDlgGetRemoteParty(tmpDlg), tmpContent );
			}
			tmpSub = NULL;
		}

	}

	if (tmpContent)
	{
		uaContentFree(tmpContent);
		tmpContent = NULL;
	}
}

/* Process subscription */
void CPresManager::ProcessSub(UaSub sub)
{
	if (!sub)
		return;

	UaDlg dlg = uaSubGetDlg(sub);
	if (!dlg)
		return;

	const char* strURI = uaDlgGetRemoteParty(dlg);
	
	if (!strURI)
		return;
	
	char* buddyURI = URIfromSIPtoPRES(strURI);
	Buddy tmpBuddy = FindMatchBuddy(buddyURI);


	if (!tmpBuddy)
	{
		DebugMsg("[PresManager] create a new buddy");

		/* create a new buddy */
		tmpBuddy = BuddyNew(buddyURI);
		dxLstPutTail(m_BuddyLst, tmpBuddy);
	}

	if (GetBuddyBlock(tmpBuddy))
	{
		DebugMsg("[PresManager] Get a subscription from my black list, ignore it");
		return;
	}

	UaSub tmpSub = GetBuddyINSub(tmpBuddy);

	if (!tmpSub)
	{
		DebugMsg("[PresManager] new-subsciption");
	}
	else
	{
		DebugMsg("[PresManager] re-subsciption");
		UaDlg tmpDlg = uaSubGetDlg(tmpSub);
		if ( dlg != tmpDlg )
		{ /* we received another subscription from another dialog */

			/* if original dlg is created by SUBSCRIBE method, release it */
			uaDlgRelease(tmpDlg);
		}
	}

	SetBuddyINSub(tmpBuddy, sub);
	sub = GetBuddyINSub(tmpBuddy);

	/* if expire time = 0 , it's an un-subscription request */
	if (uaSubGetExpires(sub) == 0)
	{
		DebugMsg("[PresManager] un-subsciption");
		SetBuddyINState(tmpBuddy, UASUB_TERMINATED);
		NotifyBuddy(tmpBuddy);

		/* Notify UI */
		if ( CUAControl::GetControl() )
//			CUAControl::GetControl()->Fire_BuddyRemoveContact( CComBSTR( buddyURI ) );
			CUAControl::GetControl()->Fire_BuddyRemoveContact(0);
	}
	else
	{
		if ( GetBuddyAuth(tmpBuddy) )
			SetBuddyINState(tmpBuddy, UASUB_ACTIVE);
		else
			SetBuddyINState(tmpBuddy, UASUB_PENDING);

		NotifyBuddy(tmpBuddy);
		/* Notify UI */
		if ( CUAControl::GetControl() )
			CUAControl::GetControl()->Fire_BuddyRequestForContact( CComBSTR( buddyURI ) );
//			CUAControl::GetControl()->Fire_BuddyRequestForContact( 0);
	}

	if (buddyURI)
	{
		free(buddyURI);
		buddyURI = NULL;
	}
}

void CPresManager::DebugWinfo(WINFOobj _winfo)
{
	WINFOobj winfo = _winfo;
	char tmp[500];
	DxLst tmpLst,tmpLst1;
	WINFOwlist _wlist = NULL;
	WINFOwatcher _watcher=NULL;
	if(winfo!=NULL)
	{
		sprintf(tmp,"[winfotest]nsname:%s ,state=%s",GetWINFONsname(winfo),FromWINFOState2Str(GetWINFOState(winfo)));
		DebugMsg(tmp);

		tmpLst = GetWINFOwListLst(winfo);
		for(int i=0;i<dxLstGetSize(tmpLst);i++)
		{
			_wlist = (WINFOwlist)dxLstPeek(tmpLst,i);
			sprintf(tmp,"[winfotest]resource:%s, package:%s",_wlist->resource,_wlist->package);
			DebugMsg(tmp);
			tmpLst1 = GetWlistWatcherLst(_wlist);
			if(tmpLst1)
			{
				for(int j = 0;j<dxLstGetSize(tmpLst1); j++)
				{
					_watcher = (WINFOwatcher)dxLstPeek(tmpLst1,j);

					sprintf(tmp,"[winfotest]status:%s,id:%s, display name:%s, event:%s,expiration:%d, duration subscribed:%d,uri: %s",FromWatcherStatus2Str(_watcher->status),_watcher->id,_watcher->display_name,FromWatcherEvent2Str(_watcher->event),_watcher->expiration,_watcher->duration_subscribed,_watcher->uri);
					DebugMsg(tmp);        
				}
			}
		}
		
	}

}

void CPresManager::testWinfoParser()
{
	static char xml_buf[2000];

	memset(xml_buf,0,2000);
	DebugMsg("[winfotest]Before parsing...");
	strcat(xml_buf,"<?xml version=\"1.0\"?> \r\n");
   	strcat(xml_buf,"<watcherinfo xmlns=\"urn:ietf:params:xml:ns:watcherinfo\"\r\n");
       strcat(xml_buf,"version=\"0\" state=\"partial\">\r\n");
	strcat(xml_buf,"<watcher-list resource=\"sip:professor@example.net\" package=\"presence\">\r\n");
       strcat(xml_buf,"<watcher status=\"active\"\r\n");
       strcat(xml_buf,"id=\"8ajksjda7s\"\r\n");
       strcat(xml_buf,"duration-subscribed=\"555\"\r\n");
       strcat(xml_buf,"display-name=\"Mr. Tester\"\r\n");
       strcat(xml_buf,"event=\"approved\" >sip:userA@example.net</watcher>\r\n");
       strcat(xml_buf,"<watcher status=\"terminated\"\r\n");
       strcat(xml_buf,"id=\"hh8juja87s997-ass7\"\r\n");
       strcat(xml_buf,"display-name=\"Mr. Subscriber\"\r\n");
       strcat(xml_buf,"event=\"subscribe\">sip:userB@example.org</watcher>\r\n");
	strcat(xml_buf,"<watcher status=\"pending\"\r\n");
       strcat(xml_buf,"id=\"lkasdjflkjasdf\"\r\n");
       strcat(xml_buf,"display-name=\"Mr. Super\"\r\n");
       strcat(xml_buf,"event=\"subscribe\">sip:userC@example.org</watcher>\r\n");
     	strcat(xml_buf,"</watcher-list>\r\n");
   	strcat(xml_buf,"</watcherinfo>\r\n\r\n");

	WINFOobj winfo =NULL;
	DebugMsg("test");
//	DebugMsg(xml_buf);
	winfo = FromXMLtoWINFO(xml_buf, strlen(xml_buf));
	DebugMsg("[winfotest]After Parsing...");

//	char tmp[500];
//	DxLst tmpLst,tmpLst1;
	WINFOwlist _wlist = NULL;
	WINFOwatcher _watcher=NULL;
	if(winfo!=NULL)
		DebugWinfo(winfo);
/*
	if(winfo!=NULL)
		DebugMsg(FromWINFOtoXML(winfo));
	else
		DebugMsg("[winfotest]parsing..NULL");
*/

	SetWinfoList(winfo);
//	SetWINFOState(winfo, WINFO_PARTIAL);
	SetWinfoList(winfo);
	WINFOobjFree(winfo);
	
	winfo = GetWatcherLst();
	DebugWinfo(winfo);


}

void CPresManager::CheckWinfoForAddingNewUser(WINFOobj _winfo)
{
	DebugMsg("[PresManager] Check Winfo for adding new user ");

	if(_winfo==NULL)
		return;

	WINFOobj winfo = _winfo;
	DxLst tmpLst,tmpLst1;
	WINFOwlist _wlist = NULL;
	WINFOwatcher _watcher=NULL;
	Buddy tmpBuddy = NULL;
	
	if(winfo!=NULL)
	{
		tmpLst = GetWINFOwListLst(winfo);
		for(int i=0;i<dxLstGetSize(tmpLst);i++)
		{
			_wlist = (WINFOwlist)dxLstPeek(tmpLst,i);
			tmpLst1 = GetWlistWatcherLst(_wlist);
			if(tmpLst1)
			{
				for(int j = 0;j<dxLstGetSize(tmpLst1); j++)
				{
					_watcher = (WINFOwatcher)dxLstPeek(tmpLst1,j);

					if(_watcher->status ==WATCHER_PENDING)
					{
						tmpBuddy = FindMatchBuddy(_watcher->uri);
						if (tmpBuddy && GetBuddyOUTSub(tmpBuddy)) 
							// The subscription to this buddy already exists, 
							// so there is no need to prompt user again
							return;

						if ( CUAControl::GetControl() )
						{
//							CUAControl::GetControl()->Fire_NeedAuthentication( CComBSTR( _watcher->uri ) );
							CUAControl::GetControl()->Fire_NeedAuthorize( CComBSTR( _watcher->uri ) );
							DebugMsg("[PresManager] Get Pending state in Winfo...Send Add contact event to UI...");
						}
					}
					else if(_watcher->status ==WATCHER_TERMINATED)
					{
//marked by alan,20050221
//						CUAControl::GetControl()->Fire_BuddyUpdateBasicStatus( CComBSTR( _watcher->uri ) , 0,CComBSTR("Offline"),CComBSTR("Offline") );
					}
					else if(_watcher->status ==WATCHER_ACTIVE)
					{
//						SubscribeBuddy(_watcher->uri,false,"presence",3600);
					}
						
				}
			}
		}
		
	}
	

	

}

/* Process notification */
//void CPresManager::ProcessNotify(UaSub sub, UaContent content)
void  CPresManager::ProcessNotify(UaSub sub, UaContent content)
{

	DebugMsg("[PresManager] entering process notify ");
	if (!sub)
		return ;

	UaDlg dlg = uaSubGetDlg(sub);
	if (!dlg)
		return ;

	const char* strURI = uaDlgGetRemoteParty(dlg);

	if (!strURI)
		return ;

	char* buddyURI = URIfromSIPtoPRES(strURI);
	Buddy tmpBuddy = FindMatchBuddy(buddyURI);
	int tmp_count =0;
	
	char pState[16],cState[16];

	if (tmpBuddy)
	{
		if(uaSubGetReason(sub)!=NULL)
		{
			DebugMsg(uaSubGetReason(sub));
		}
		if (GetBuddyBlock(tmpBuddy))
		{
			DebugMsg("[PresManager] Get a notify from my black list, ignore it");
			if (buddyURI)
			{
				free(buddyURI);
				buddyURI = NULL;
			}
			return ;
		}

		 //update subscription state 
		switch (uaSubGetSubstate(sub)) {
			case UASUB_ACTIVE:
				if (GetBuddyOUTState(tmpBuddy) != UASUB_ACTIVE)
				{
					if ( CUAControl::GetControl() )
						CUAControl::GetControl()->Fire_BuddyAllowSubscription( CComBSTR( buddyURI ) );
				}
				SetBuddyOUTState(tmpBuddy, UASUB_ACTIVE);
				break;
			case UASUB_PENDING:
				SetBuddyOUTState(tmpBuddy, UASUB_PENDING);

				if ( CUAControl::GetControl() )
					CUAControl::GetControl()->Fire_BuddyPendingSubscription( CComBSTR( buddyURI ) );
				break;
			case UASUB_TERMINATED:  // terminate the subscription

				if(uaSubGetReason(sub)!=NULL)
				{
					DebugMsg(uaSubGetReason(sub));
//					if(_stricmp(uaSubGetReason(sub),"rejected"))
//					{
						if ( CUAControl::GetControl() )
//							CUAControl::GetControl()->Fire_BuddyRemoveSubscription( CComBSTR( buddyURI ) );
							CUAControl::GetControl()->Fire_BuddyUpdateBasicStatus( CComBSTR( buddyURI ) , 0,CComBSTR("Offline"),CComBSTR("Offline") );
//					}
				}
				SetBuddyOUTState(tmpBuddy, UASUB_TERMINATED);
				SetBuddyOUTSub(tmpBuddy, NULL);
			default:
				break;
		}

		 
		UaEvtpkg tmpEvtpkg = uaSubGetEvtpkg(sub);
		if(content && (_stricmp(uaEvtpkgGetName(tmpEvtpkg),"presence.winfo")==0))//parse winfo
		{
			DebugMsg("[PresManager] Parsing watcher info...");
			WINFOobj winfo =FromXMLtoWINFO((const char*)uaContentGetBody(content), strlen(uaContentGetBody(content)));

			if(winfo!=NULL)
			{
				SetWinfoList(winfo);
				//add for testing... by alan
//				DebugWinfo(winfo);

				//process winfo--->check pending state 
				CheckWinfoForAddingNewUser(winfo);


				
				WINFOobjFree(winfo);
				
				
			}
		}
		// update the presence state, we only process the pidf+xml format 
		if ( content && (_stricmp( uaContentGetType(content), "application/pidf+xml") == 0)&&(_stricmp(uaEvtpkgGetName(tmpEvtpkg), "presence") == 0))
		{
			DebugMsg("[PresManager] Parsing presence .....");
			CPIMpres pres = FromXMLtoPRES( (const char*)uaContentGetBody(content), strlen(uaContentGetBody(content)) );
			SetBuddyPresInfo(tmpBuddy, pres);
			CPIMpresFree(pres);
			pres = NULL;
		}

		if (content && (GetBuddyOUTState(tmpBuddy) == UASUB_ACTIVE) && (_stricmp(uaEvtpkgGetName(tmpEvtpkg), "presence") == 0))
		{
			MyCString tmpStr;
//			tmpStr.Format(_T("[PresManager] %s update status %d"), buddyURI, GetBuddyBasic(tmpBuddy));
			tmpStr.Format(_T("[PresManager] %s update IMPS status %d"), buddyURI, GetBuddyIMPSBasic(tmpBuddy));
//			tmpStr.Format(_T("[PresManager] %s update Call status %d"), buddyURI, GetBuddyCallBasic(tmpBuddy));
			DebugMsg(tmpStr);

			if ( CUAControl::GetControl() )
			{
				if(GetBuddyIMPSBasic(tmpBuddy)==1)
					strcpy(pState,"Online");
				else
					strcpy(pState,"Offline");
				
				if(GetBuddyCallBasic(tmpBuddy)==1)
				{
					strcpy(cState,"Online");

					UABuddyCallExtStatus ret = GetBuddyCallExt(tmpBuddy);
					switch(ret)
					{
						case CALL_EXT_BUSY:
							strcpy(cState,"Busy");
							break;
						case CALL_EXT_IDLE:
							strcpy(cState,"Idle");
							break;
					}
					
				}
				else
					strcpy(cState,"Offline");
					
				CUAControl::GetControl()->Fire_BuddyUpdateBasicStatus( CComBSTR( buddyURI ) , GetBuddyIMPSBasic(tmpBuddy),CComBSTR(pState),CComBSTR(cState) );
			}
//				CUAControl::GetControl()->Fire_BuddyUpdateBasicStatus( 0);
		}
	}
	else
	{
		DebugMsg("[PresManager] Get a notify from an unknonw UA, ignore it");
		if (buddyURI)
		{
			free(buddyURI);
			buddyURI = NULL;
		}
		return ;
	}
	DebugMsg("[PresManager] before free .....");

	if (buddyURI)
	{
		free(buddyURI);
		buddyURI = NULL;
	}
	DebugMsg("[PresManager] return... .....");
//	return (void *)buddyURI;//remember to free after unuseless

}

const char* CPresManager::GetBuddyURI(unsigned short number)
{
	if ( number >= dxLstGetSize(m_BuddyLst) )
		return NULL;

	Buddy tmpBuddy = (Buddy)dxLstPeek(m_BuddyLst, number);

	return GetBuddyPresentity(tmpBuddy);
}

unsigned short CPresManager::GetBuddyCount()
{
	return dxLstGetSize(m_BuddyLst);
}

UASubState CPresManager::GetBuddyINSubStateFromURI(const char* buddyURI)
{
	Buddy tmpBuddy = FindMatchBuddy(buddyURI);
	return GetBuddyINState(tmpBuddy);
}

UASubState CPresManager::GetBuddyOUTSubStateFromURI(const char *buddyURI)
{
	Buddy tmpBuddy = FindMatchBuddy(buddyURI);
	return GetBuddyOUTState(tmpBuddy);
}

short CPresManager::GetBuddyBasicStatus(const char *buddyURI)
{
	Buddy tmpBuddy = FindMatchBuddy(buddyURI);
	return GetBuddyBasic(tmpBuddy);
}

RCODE CPresManager::UnsubscribeBuddy(const char *strURI,char *_eventname)
{
	if (!strURI)
		return RC_ERROR;

	Buddy tmpBuddy = FindMatchBuddy(strURI);

	if (!tmpBuddy)
		return FALSE;

	SetBuddyOUTState(tmpBuddy, UASUB_TERMINATED);
	
	UaSub tmpSub = GetBuddyOUTSub(tmpBuddy);

	if (!tmpSub)
		return RC_ERROR;
	
	UaDlg tmpDlg = uaSubGetDlg(tmpSub);

	if (!tmpDlg)
		return RC_ERROR;

	uaSubSetExpires(tmpSub, 0);

	uaDlgSubscribe(tmpDlg, tmpSub,_eventname);
	SetBuddyOUTSub(tmpBuddy, NULL);

	MyCString tmpStr;
	tmpStr.Format(_T("[PresManager] Unsubscribe buddy %s"), strURI);
	DebugMsg(tmpStr);

	return RC_OK;
}

RCODE CPresManager::UnblockBuddy(const char *strURI)
{
	Buddy tmpBuddy = FindMatchBuddy(strURI);

	if (!tmpBuddy)
		return RC_ERROR;

	MyCString tmpStr;
	tmpStr.Format(_T("[PresManager] UnBlock buddy %s"), strURI);
	DebugMsg(tmpStr);

	SetBuddyBlock(tmpBuddy, FALSE);
	return RC_OK;
}

BOOL CPresManager::IsBuddyBlock(const char *strURI)
{
	Buddy tmpBuddy = FindMatchBuddy(strURI);

	if (!tmpBuddy)
		return FALSE;

	return GetBuddyBlock(tmpBuddy);
}

BOOL CPresManager::IsBuddyAuthorized(const char *strURI)
{
	Buddy tmpBuddy = FindMatchBuddy(strURI);

	if (!tmpBuddy)
		return FALSE;

	return GetBuddyAuth(tmpBuddy);
}

char* CPresManager::URIfromPREStoSIP(const char* strURI)
{
	return strDup(strURI);
	/*
	char* token = NULL;
	char* tmpstr = NULL;
	char* tmpURI = strDup(strURI);
	token = strtok( tmpURI, ":" );

	if (token)
	{
		if (stricmp(token, "pres") == 0)
		{
			tmpstr = (char*)malloc(strlen(strURI));
			sprintf(tmpstr,"sip:%s", strURI+5);
		}
		if (stricmp(token, "sip") == 0)
			tmpstr = strDup(strURI);
	}

	if (tmpURI)
	{
		free(tmpURI);
		tmpURI = NULL;
	}
	
	return tmpstr;
	*/
}	

char* CPresManager::URIfromSIPtoPRES(const char* strURI)
{
	return strDup(strURI);
	/*
	char* token = NULL;
	char* tmpstr = NULL;
	char* tmpURI = strDup(strURI);
	token = strtok( tmpURI, ":" );

	if (token)
	{
		if (stricmp(token, "sip") == 0)
		{
			tmpstr = (char*)malloc(strlen(strURI)+2);
			sprintf(tmpstr,"pres:%s", strURI+4);
		}
		if (stricmp(token, "pres") == 0)
			tmpstr = strDup(strURI);
	}

	if (tmpURI)
	{
		free(tmpURI);
		tmpURI = NULL;
	}
	
	return tmpstr;
	*/
}


PresStatus CPresManager::GetCurrentPresStatus()
{

	return m_CurrentState;
}
/*
WINFOStruct CPresManager::testWinfoParser(const char *xmldata,int len)
{
	WINFOStruct winfoNotify = FromXMLtoPRESForParsingWinfo(xmldata,len);

	return winfoNotify;
}
*/

#endif
