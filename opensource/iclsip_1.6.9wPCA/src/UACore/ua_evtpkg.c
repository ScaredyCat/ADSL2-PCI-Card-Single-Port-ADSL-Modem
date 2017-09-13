/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * ua_evtpkg.c
 *
 * $Id: ua_evtpkg.c,v 1.4 2004/10/28 02:08:33 tyhuang Exp $
 */
#include "ua_evtpkg.h"

/** 
 *  \brief A C-structure for a Event Package
 *
 */
struct evtpkgObj{
	char*	EvtName; /* name of event¡§presence¡¨*/
	DxLst   classLst; /* list of class */
	int	classCount;/*count of class */
	SubEvtCB	subCB;/* add by ljchuang.2003.06.13 */
};

/** create a event package object
 *  \param eventname specify the event package
 *  \retval UaEvtPkg of specific event or NULL
 */
UaEvtpkg uaEvtpkgNew(IN const char* eventname)
{
	UaEvtpkg _this=NULL;

	if(eventname != NULL){
		_this=(UaEvtpkg)calloc(1,sizeof(struct evtpkgObj));
		_this->EvtName=strDup(eventname);
		_this->classCount=0;
		_this->classLst=dxLstNew(DX_LST_POINTER);
	}
	_this->subCB = NULL;
	return _this;
}

/** Free a UaClass 
 *  \param _this the object we want to free
 */
void	uaEvtpkgFree(IN UaEvtpkg _this)
{
	if(_this != NULL){
		/* free event-name*/
		if(_this->EvtName != NULL){
			free(_this->EvtName);
			_this->EvtName=NULL;
		}
		/*free class list */
		dxLstFree(_this->classLst,(void (*)(void *))uaClassFree);
		_this->classCount=0;
		free(_this);
		_this=NULL;
	}
}

/** get event name 
 *  \param _this the event package 
 *  \retval the name of event package or NULL
 */
char*	uaEvtpkgGetName(IN UaEvtpkg _this)
{
	char *evtname=NULL;
	if(_this != NULL)
		if(_this->EvtName != NULL)
			evtname=_this->EvtName;

	return evtname;
}

/** Add a class to list 
 *  \param _this the event package 
 *  \param _addclass will be added
 *	\retval RCODE :RC_OK if success,or RC_ERROR if error occurs
 */
RCODE uaEvtpkgAddClass(IN UaEvtpkg _this, IN UaClass _addclass)
{
	int rcode=RC_ERROR;
	
	/* check input value */
	if(_this==NULL){
		UaCoreERR("[uaEvtpkgAddClass] event package is NULL!\n");
		return rcode;
	}
	if(_addclass==NULL){
		UaCoreERR("[uaEvtpkgAddClass] input uaclass is NULL!\n");
		return rcode;
	}
	rcode=dxLstPutTail(_this->classLst,_addclass);
	if(rcode == RC_OK)
		_this->classCount++;

	return rcode;
}

/** Delete a class from list 
 *  \param _this the event package 
 *  \param _classname the name of class will be deleted 
 *	\retval RCODE :RC_OK if success,or RC_ERROR if error occurs
 */
RCODE uaEvtpkgDelClass(IN UaEvtpkg _this, IN const char* _classname)
{
	int rcode=RC_ERROR,num,pos;
	UaClass tmpuaclass;
	
	if((_this != NULL)&&(_classname != NULL)){
		num=dxLstGetSize(_this->classLst);
		for(pos=0;pos<num;pos++){
			tmpuaclass=dxLstPeek(_this->classLst,pos);
			if(strcmp(uaClassGetName(tmpuaclass),_classname)==0){
				tmpuaclass=dxLstGetAt(_this->classLst,pos);
				/* uaClassFree(tmpuaclass); marked by ljchuang */
				rcode=RC_OK;
				break;
			}
		}/*end of for loop */
	}
	return rcode;
}

/** get a class with specified name from list 
 *  \param _this the event package 
 *  \param _classname the name of class will be gotten 
 *	\retval UaClass object or NULL
 */
UaClass uaEvtpkgGetClass(IN UaEvtpkg _this, IN const char* _classname)
{
	UaClass rclass=NULL;
	int num,pos;

	if((_this != NULL)&&(_classname != NULL)){
		num=dxLstGetSize(_this->classLst);
		for(pos=0;pos<num;pos++){
			rclass=dxLstPeek(_this->classLst,pos);
			if(strcmp(uaClassGetName(rclass),_classname)==0)
				break;
		}/* end of for loop */
	}
	
	return rclass;
}

/** get a class with specified name from list 
 *  \param _this the event package 
 *  \param _position the position of class will be gotten 
 *	\retval UaClass object or NULL
 */
UaClass uaEvtpkgGetClassByPos(IN UaEvtpkg _this , IN int _position)
{
	UaClass rclass=NULL;

	if(_this==NULL)
		return rclass;
	if(_position<0)
		return rclass;

	rclass=dxLstPeek(_this->classLst,_position);
	return rclass;

}

/** get number of class 
 *  \param _this the event package 
 *	\retval integer of number of class
 */
int	uaEvtpkgGetNumOfClass(IN UaEvtpkg _this)
{
	int nofclass=0;

	if(_this != NULL)
		nofclass = _this->classCount;
	return nofclass;
}

/** Get the subscription state callback */
CCLAPI SubEvtCB uaEvtpkgGetSubCB(IN UaEvtpkg _this)
{
	if (!_this)
		return NULL;

	return _this->subCB;
}

/** Set the subscription state callback */
CCLAPI RCODE uaEvtpkgSetSubCB(IN UaEvtpkg _this, IN SubEvtCB subCB)
{
	if (!_this)
		return RC_ERROR;

	_this->subCB = subCB;
	return RC_OK;
}

/** add a new event package
 *	\param _this the UaMgr
 *	\param _event the event package 
 */
CCLAPI RCODE uaMgrInsertEvtpkg(IN UaMgr _this, IN UaEvtpkg _event)
{

	int rcode=RC_ERROR;
	
	/* check input value */
	if(_this == NULL){
		UaCoreERR("[uaMgrInsertEvtpkg] manager is NULL!\n");
		return rcode;
	}
	if(_event == NULL){
		UaCoreERR("[uaMgrInsertEvtpkg] event package is NULL!\n");
		return rcode;
	}
	/* add to list tail */
	rcode=dxLstPutTail(uaMgrGetEventLst(_this),_event);

	return rcode;
}

/** remote a event package 
 *	\param _this the UaMgr
 *	\param _event the name of event package 
 */
CCLAPI RCODE uaMgrRemoveEvtpkg(IN UaMgr _this, IN const char* _event)
{
	UaEvtpkg tmpevent=NULL;
	int rcode=RC_ERROR,pos,num;
	

	/* check input value */
	if(_this == NULL){
		UaCoreERR("[uaMgrRemoveEvtpkg] manager is NULL!\n");
		return rcode;
	}
	if(_event == NULL){
		UaCoreERR("[uaMgrRemoveEvtpkg] event package is NULL!\n");
		return rcode;
	}
	num=dxLstGetSize(uaMgrGetEventLst(_this));
	for(pos=0;pos<num;pos++){
		tmpevent=dxLstPeek(uaMgrGetEventLst(_this),pos);
		if(strICmp(uaEvtpkgGetName(tmpevent),_event)==0){
			tmpevent=dxLstGetAt(uaMgrGetEventLst(_this),pos);
			/* uaEvtpkgFree(tmpevent); marked by ljchuang */
			rcode=RC_OK;
			break;
		}
	}
	return rcode;
}

/** get a event package 
 *	\param _this the UaMgr
 *	\param _event the name of event package
 */
CCLAPI UaEvtpkg uaMgrGetEvtpkg(IN UaMgr _this, IN const char* _event)
{
	UaEvtpkg tmpevent=NULL;
	int pos,num;
	

	/* check input value */
	if(_this == NULL){
		UaCoreERR("[uaMgrGetEvtpkg] manager is NULL!\n");
		return tmpevent;
	}
	if(_event == NULL){
		UaCoreERR("[uaMgrGetEvtpkg] eventname is NULL!\n");
		return tmpevent;
	}
	num=dxLstGetSize(uaMgrGetEventLst(_this));
	for(pos=0;pos<num;pos++){
		tmpevent=dxLstPeek(uaMgrGetEventLst(_this),pos);
		if(strICmp(uaEvtpkgGetName(tmpevent),_event)==0)
			return tmpevent;
	}
	return NULL;
}


