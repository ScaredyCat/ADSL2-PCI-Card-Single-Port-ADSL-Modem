/* 
 * Copyright (C) 2002-2003 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* $Id: UABuddy.h,v 1.3 2005/02/03 11:52:11 ljchuang Exp $ */

#ifndef _UABUDDY
#define _UABUDDY

#ifdef _simple

#include <cpim/CPIM.h>
#include <uacore/ua_core.h>
#include <simpleapi/simple_api.h>

typedef struct _Buddy* Buddy;

//add by alan for call extend status
typedef enum {
	CALL_EXT_NULL,
	CALL_EXT_IDLE,
	CALL_EXT_BUSY
}UABuddyCallExtStatus;

Buddy BuddyNew(const char* presentity);
RCODE BuddyFree(Buddy _this);
const char* GetBuddyPresentity(Buddy _this);
RCODE SetBuddyPresentity(Buddy _this, const char* presentity);
CPIMpres GetBuddyPresInfo(Buddy _this);
RCODE SetBuddyPresInfo(Buddy _this, CPIMpres presinfo);
short  GetBuddyBasic(Buddy _this);
UaSub GetBuddyINSub(Buddy _this);
RCODE SetBuddyINSub(Buddy _this, UaSub sub);
UaSub GetBuddyOUTSub(Buddy _this);
RCODE SetBuddyOUTSub(Buddy _this, UaSub sub);
UASubState GetBuddyINState(Buddy _this);
RCODE SetBuddyINState(Buddy _this, UASubState state);
UASubState GetBuddyOUTState(Buddy _this);
RCODE SetBuddyOUTState(Buddy _this, UASubState state);
Buddy BuddyDup(Buddy _this);
BOOL GetBuddyAuth(Buddy _this);
RCODE SetBuddyAuth(Buddy _this, BOOL bAuth);
BOOL GetBuddyBlock(Buddy _this);
RCODE SetBuddyBlock(Buddy _this, BOOL bBlock);

//add by alan
short GetBuddyCallBasic(Buddy _this);
short GetBuddyIMPSBasic(Buddy _this);
UABuddyCallExtStatus GetBuddyCallExt(Buddy _this);
#endif

#endif //_UABUDDY