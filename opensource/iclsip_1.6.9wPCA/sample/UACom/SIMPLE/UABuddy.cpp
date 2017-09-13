/* 
 * Copyright (C) 2002-2003 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* $Id: UABuddy.cpp,v 1.3 2005/02/03 11:52:11 ljchuang Exp $ */

#include "UABuddy.h"
#include <adt/dx_lst.h>

#ifdef _simple

struct _Buddy {
	char* presentity;
	UaSub insub;
	UaSub outsub;
	CPIMpres presinfo;
	UASubState instate;
	UASubState outstate;
	BOOL bAuth;
	BOOL bBlock;
};

Buddy BuddyNew(const char* presentity)
{
	Buddy _ret;

	if (!presentity)
		return NULL;

	_ret = (Buddy)malloc(sizeof(struct _Buddy));
	_ret->presentity = strDup(presentity);
	_ret->insub = NULL;
	_ret->outsub = NULL;
	_ret->presinfo = NULL;
	_ret->instate = UASUB_NULL;
	_ret->outstate = UASUB_NULL;
	_ret->bAuth = FALSE;
	_ret->bBlock = FALSE;
	return _ret;
}

RCODE BuddyFree(Buddy _this)
{
	UaDlg tmpDlg = NULL;
	if (!_this)
		return RC_ERROR;

	if (_this->presentity)
	{
		free (_this->presentity);
		_this->presentity = NULL;
	}

	if (_this->insub)
	{
		tmpDlg = uaSubGetDlg(_this->insub);
		if (tmpDlg)
		{
			uaDlgRelease(tmpDlg);
			tmpDlg = NULL;
		}
		uaSubFree(_this->insub);
		_this->insub = NULL;
	}

	if (_this->outsub)
	{
		tmpDlg = uaSubGetDlg(_this->outsub);
		if (tmpDlg)
		{
			uaDlgRelease(tmpDlg);
			tmpDlg = NULL;
		}
		uaSubFree(_this->outsub);
		_this->outsub = NULL;
	}

	if (_this->presinfo)
	{
		CPIMpresFree(_this->presinfo);
		_this->presinfo = NULL;
	}
	free (_this);
	_this = NULL;
	
	return RC_OK;
}

BOOL GetBuddyAuth(Buddy _this)
{
	if (!_this)
		return FALSE;
	return _this->bAuth;
}

RCODE SetBuddyAuth(Buddy _this, BOOL bAuth)
{
	if (!_this)
		return RC_ERROR;

	_this->bAuth = bAuth;
	return RC_OK;
}

BOOL GetBuddyBlock(Buddy _this)
{
	if (!_this)
		return FALSE;
	return _this->bBlock;
}

RCODE SetBuddyBlock(Buddy _this, BOOL bBlock)
{
	if (!_this)
		return RC_ERROR;

	_this->bBlock = bBlock;
	return RC_OK;
}

const char* GetBuddyPresentity(Buddy _this)
{
	if (!_this)
		return NULL;

	return _this->presentity;
}

RCODE SetBuddyPresentity(Buddy _this, const char* presentity)
{
	if (!_this || !presentity)
		return RC_ERROR;

	if (_this->presentity)
		free(_this->presentity);

	_this->presentity = strDup(presentity);
	return RC_OK;
}

CPIMpres GetBuddyPresInfo(Buddy _this)
{
	if (!_this)
		return NULL;

	return _this->presinfo;
}

RCODE SetBuddyPresInfo(Buddy _this, CPIMpres presinfo)
{
	if (!_this || !presinfo)
		return RC_ERROR;

	if (_this->presinfo)
		CPIMpresFree(_this->presinfo);

	_this->presinfo = CPIMpresDup(presinfo);
	return RC_OK;
}

//add by alan, used to get call status
short GetBuddyCallBasic(Buddy _this)
{
	short basic = -1;
	CPIMtuple _tuple = NULL;
	CPIMstatus _status = NULL;

	if (!_this)
		return -1;

	if (!_this->presinfo)
		return -1;

	DxLst tmpLst = GetPRESTupleLst(_this->presinfo);

	if (!tmpLst)
		return 0;

	for (int i=0; i < dxLstGetSize(tmpLst); i++)
	{
		_tuple = (CPIMtuple)dxLstPeek(tmpLst, i);
		if (_tuple)
		{
			CPIMcontact tmpContact = GetTupleContact(_tuple);
			char *tmpURI= tmpContact->URI;
			char *split_pos = strstr(tmpURI,":");
			char tmp_prefix[8];
			
			if(split_pos)
			{
				strncpy(tmp_prefix,tmpURI,3);
				tmp_prefix[3]='\0';
				if(_stricmp(tmp_prefix,"sip"))//if not for call status tuple....skip
					continue;
			}
			_status = GetTupleStatus(_tuple);
			if (_status)
			{
				basic = GetStatusBasic(_status);
				if (basic != -1)
					break;
			}
		}
	}
	return basic;

}

//add by alan, used to get Call ext Status 
UABuddyCallExtStatus GetBuddyCallExt(Buddy _this)
{
	UABuddyCallExtStatus retval=CALL_EXT_NULL;


	short basic = -1;
	CPIMtuple _tuple = NULL;
	CPIMstatus _status = NULL;

	if (!_this)
		return retval;

	if (!_this->presinfo)
		return retval;

	DxLst tmpLst = GetPRESTupleLst(_this->presinfo);

	if (!tmpLst)
		return retval;

	for (int i=0; i < dxLstGetSize(tmpLst); i++)
	{
		_tuple = (CPIMtuple)dxLstPeek(tmpLst, i);
		if (_tuple)
		{
			CPIMcontact tmpContact = GetTupleContact(_tuple);
			char *tmpURI= tmpContact->URI;
			char *split_pos = strstr(tmpURI,":");
			char tmp_prefix[8];
			
			if(split_pos)
			{
				strncpy(tmp_prefix,tmpURI,3);
				tmp_prefix[3]='\0';
				if(_stricmp(tmp_prefix,"sip"))//if not for call status tuple....skip
					continue;
			}
			_status = GetTupleStatus(_tuple);
			if (_status)
			{
				DxLst tmpLst1;
				CPIMelement _element;
				tmpLst1 = GetStatusExtLst(_status);
				if (tmpLst1) {
					for (int j = 0; j < dxLstGetSize(tmpLst1); j++) {
						_element = (CPIMelement)dxLstPeek(tmpLst1, j);
						if(!stricmp(_element->tag,"call"))
						{
							if(!stricmp(_element->val,"busy"))	
								retval = CALL_EXT_BUSY;
							if(!stricmp(_element->val,"null"))	
								retval = CALL_EXT_NULL;
							if(!stricmp(_element->val,"idle"))	
								retval = CALL_EXT_IDLE;
						}
						
					}
				}
			}
		}
	}
	return retval;
}
//add by alan, used to get IMPS presence
short GetBuddyIMPSBasic(Buddy _this)
{
	short basic = -1;
	CPIMtuple _tuple = NULL;
	CPIMstatus _status = NULL;

	if (!_this)
		return -1;

	if (!_this->presinfo)
		return -1;

	DxLst tmpLst = GetPRESTupleLst(_this->presinfo);

	if (!tmpLst)
		return 0;

	for (int i=0; i < dxLstGetSize(tmpLst); i++)
	{
		_tuple = (CPIMtuple)dxLstPeek(tmpLst, i);
		if (_tuple)
		{
			CPIMcontact tmpContact = GetTupleContact(_tuple);
			char *tmpURI= tmpContact->URI;
			char *split_pos = strstr(tmpURI,":");
			char tmp_prefix[8];
			
			if(split_pos)
			{
				strncpy(tmp_prefix,tmpURI,2);
				tmp_prefix[2]='\0';
				if(_stricmp(tmp_prefix,"im"))//if not for IMPS tuple....skip
					continue;
			}
			
			_status = GetTupleStatus(_tuple);
			if (_status)
			{
				basic = GetStatusBasic(_status);
				if (basic != -1)
					break;
			}
		}
	}
	return basic;

}


short  GetBuddyBasic(Buddy _this)
{
	short basic = -1;
	CPIMtuple _tuple = NULL;
	CPIMstatus _status = NULL;

	if (!_this)
		return -1;

	if (!_this->presinfo)
		return -1;

	DxLst tmpLst = GetPRESTupleLst(_this->presinfo);

	if (!tmpLst)
		return 0;

	for (int i=0; i < dxLstGetSize(tmpLst); i++)
	{
		_tuple = (CPIMtuple)dxLstPeek(tmpLst, i);
		if (_tuple)
		{
			_status = GetTupleStatus(_tuple);
			if (_status)
			{
				basic = GetStatusBasic(_status);
				if (basic != -1)
					break;
			}
		}
	}
	return basic;
}

UaSub GetBuddyINSub(Buddy _this)
{
	if (!_this)
		return NULL;

	return _this->insub;
}

RCODE SetBuddyINSub(Buddy _this, UaSub sub)
{
	if (!_this)
		return RC_ERROR;

	if (_this->insub)
		uaSubFree(_this->insub);

	_this->insub = uaSubDup(sub);
	return RC_OK;
}

UaSub GetBuddyOUTSub(Buddy _this)
{
	if (!_this)
		return NULL;

	return _this->outsub;
}

RCODE SetBuddyOUTSub(Buddy _this, UaSub sub)
{
	if (!_this)
		return RC_ERROR;

	if (_this->outsub)
		uaSubFree(_this->outsub);

	_this->outsub = uaSubDup(sub);
	return RC_OK;
}

UASubState GetBuddyINState(Buddy _this)
{
	if (!_this)
		return UASUB_NULL;

	if (!_this->insub)
		return UASUB_NULL;

	return uaSubGetSubstate(_this->insub);
}

RCODE SetBuddyINState(Buddy _this, UASubState state)
{
	if (!_this)
		return RC_ERROR;

	if (!_this->insub)
		return RC_ERROR;

	uaSubSetSubstate(_this->insub, state, NULL);
	return RC_OK;
}

UASubState GetBuddyOUTState(Buddy _this)
{
	if (!_this)
		return UASUB_NULL;

	if (!_this->outsub)
		return UASUB_NULL;

	return uaSubGetSubstate(_this->outsub);
}

RCODE SetBuddyOUTState(Buddy _this, UASubState state)
{
	if (!_this)
		return RC_ERROR;

	if (!_this->outsub)
		return RC_ERROR;

	uaSubSetSubstate(_this->outsub, state, NULL);
	return RC_OK;
}

Buddy BuddyDup(Buddy _this)
{
	Buddy _ret;
	
	if (!_this)
		return NULL;

	_ret = BuddyNew(_this->presentity);

	_ret->instate = _this->instate;
	_ret->outstate = _this->outstate;

	if (_this->insub)
		SetBuddyINSub(_ret, _this->insub);

	if (_this->outsub)
		SetBuddyOUTSub(_ret, _this->outsub);
	
	if (_this->presinfo)
		SetBuddyPresInfo(_ret, _this->presinfo);

	return _ret;
}

#endif
