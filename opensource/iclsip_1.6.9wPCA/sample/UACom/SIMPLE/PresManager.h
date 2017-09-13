// PresManager.h: interface for the CPresManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PRESMANAGER_H__6CFA973F_A2F4_4E81_B170_B30EED08DAF1__INCLUDED_)
#define AFX_PRESMANAGER_H__6CFA973F_A2F4_4E81_B170_B30EED08DAF1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdafx.h"
#include <uacore/ua_core.h>
#include "..\UA\UACommon.h"
#include "..\UA\UAProfile.h"
#include "UABuddy.h"
#include "..\UA\CallManager.h"

#ifdef _simple
#include <simpleapi/simple_api.h>
#endif

#include <adt/dx_lst.h>
#include <cpim/cpim.h>
#include <winfo/winfo.h>
#ifdef _simple

class CCallManager;

typedef enum{
	PRES_ONLINE,
	PRES_BUSY,
	PRES_AWAY,
	PRES_DND,
	PRES_OFFLINE

}PresStatus;

class CPresManager  
{
public:
	BOOL IsBuddyAuthorized(const char* strURI);
	BOOL IsBuddyBlock(const char* strURI);
	RCODE UnblockBuddy(const char* strURI);
	RCODE UnsubscribeBuddy(const char* strURI, char *_eventname);
	RCODE AuthorizeBuddy(const char* strURI);
	RCODE UnauthorizeBuddy(const char *strURI);
	short GetBuddyBasicStatus(const char* buddyURI);
	UASubState GetBuddyOUTSubStateFromURI(const char* buddyURI);
	UASubState GetBuddyINSubStateFromURI(const char* buddyURI);
	unsigned short GetBuddyCount();
	const char* GetBuddyURI(unsigned short number);
	RCODE SubscribeBuddy(const char* strURI,BOOL isEventList,const char *eventpkg,int expires);
//	RCODE PublishBuddy(const char* strURI, const char *event,BOOL basic_status, float priority);
	RCODE PublishBuddy(const char* strURI, const char *event,BOOL basic_status, float priority,int expire);
	
	RCODE NotifyBuddy(Buddy tmpBuddy);
	char* URIfromPREStoSIP(const char* strURI);
	char* URIfromSIPtoPRES(const char* strURI);
	void ProcessNotify(UaSub sub, UaContent content);
	void NotifyAll();
	void ChangeBasicState(BOOL on);
	RCODE DelBuddy(const char* strURI);
	RCODE AddBuddy(const char* strURI);
	RCODE BlockBuddy(const char* strURI);
	Buddy FindMatchBuddy(const char* strURI);
	void ProcessSub(UaSub sub);
	void init();
	void stop();
	CPresManager();
	virtual ~CPresManager();
	CCallManager* pCallManager;

	PresStatus GetCurrentPresStatus();

	static CPresManager* GetPresMgr()
	{
		return s_pPresManager;
	}

	static UASubState SubStateCB(UaSub sub);

	void CheckWinfoForAddingNewUser(WINFOobj _winfo);

	//add by alan for testing winfo
//	WINFOStruct testWinfoParser(const char *xmldata,int len);
	void testWinfoParser();
	void DebugWinfo(WINFOobj _winfo);
	

private:
	DxLst m_BuddyLst;
	CPIMpres m_MyPres;
	UaEvtpkg m_uaEvtpkg;
	UaClass m_uaClass;
	char* m_Presentity;
	static CPresManager *s_pPresManager;
	PresStatus m_CurrentState;
};

#endif

#endif // !defined(AFX_PRESMANAGER_H__6CFA973F_A2F4_4E81_B170_B30EED08DAF1__INCLUDED_)
