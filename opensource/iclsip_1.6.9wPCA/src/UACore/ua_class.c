/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * ua_class.c
 *
 * $Id: ua_class.c,v 1.3 2004/08/02 07:25:17 tyhuang Exp $
 */
#include "ua_class.h"

/** 
 *  \brief A C-structure for a Class ,used by Event Package
 *
 */

struct uaclassObj{
	char* classname;
	DxLst  inLst;
	DxLst  outLst;
	int inCount;
	int outCount;
};

/**
 * create a class object
 * \param cname the event name of this package
 * \retval UaClass Object
 */
UaClass uaClassNew(IN const char* cname)
{
	UaClass _this=NULL; 

	if(cname != NULL)
		_this=(UaClass)calloc(1,sizeof(struct uaclassObj));
	if(_this!=NULL){
		_this->classname=strDup(cname);
		_this->inCount=0;
		_this->outCount=0;
		_this->inLst=dxLstNew(DX_LST_POINTER);
		_this->outLst=dxLstNew(DX_LST_POINTER);
	}

	return _this;
}

/** Free a UaClass 
 *  \param _this which is to be freed
 */
void	uaClassFree(IN UaClass _this)
{
	if(_this != NULL){
		if(_this->classname != NULL){
			free(_this->classname);
			_this->classname=NULL;
		}
		if (_this->inLst) {
			dxLstFree(_this->inLst, (void (*)(void *))uaSubFree);
			_this->inLst = NULL;
		}
		if (_this->outLst) {
			dxLstFree(_this->outLst,(void (*)(void *))uaSubFree);
			_this->outLst = NULL;;
		}
		_this->inCount=0;
		_this->outCount=0;
		free(_this);
		_this=NULL;
	}
}

/** get class name of a UaClass 
 *  \param _this which is to get name from
 *  \retval a string of classname
 */
char* uaClassGetName(IN UaClass _this)
{
	char *rname=NULL;
	
	if(_this != NULL){
		rname=_this->classname;
	}
	return rname;
}

/** Add a UaSub to in/out list ;true for inLst and otherwise
 * \param _this input UaClass
 * \param _sub  target UaSub to be added
 * \param _inLst True for putting UaSub in in-list otherwise
 * \retval RCODE :RC_OK if success,or RC_ERROR if error occurs
 */
RCODE uaClassAddSub(IN UaClass _this, IN UaSub _sub,IN BOOL _inLst)
{
	int rcode=RC_ERROR;

	/* check input values */
	if((_this != NULL)&&(_sub != NULL)){
		if(_inLst==TRUE){
			rcode=dxLstPutTail(_this->inLst,_sub);
			if(rcode==RC_OK)
				_this->inCount++;
		}else{
			rcode=dxLstPutTail(_this->outLst,_sub);
			if(rcode==RC_OK)
				_this->outCount++;
		}
	}
	return rcode;
}

/** Delete a UaSub from in/out list 
 * \param _this input UaClass
 * \param _sub  target UaSub to be deleted
 * \param _inLst True for deleting UaSub from in-list otherwise
 * \retval RCODE :RC_OK if success,or RC_ERROR if error occurs
 */
RCODE uaClassDelSub(IN UaClass _this, IN UaSub _sub, IN BOOL _inLst)
{
	int rcode=RC_ERROR,pos,num=0;
	UaSub	tmpsub=NULL;
	
	if((_this != NULL) && (_sub!= NULL )){
		if((_inLst==TRUE)&&(_this->inCount >0 )){
			num=dxLstGetSize(_this->inLst);
			/*search a exist subscription */
			for(pos=0;pos<num;pos++){
				tmpsub=dxLstPeek(_this->inLst,pos);
				if(tmpsub == _sub){
					/* compare successful and remove this UaSub */
					tmpsub=dxLstGetAt(_this->inLst,pos);
					/*uaSubFree(tmpsub);*/
					_this->inCount--;
					rcode=RC_OK;
				}
			}
		}
		else if((_inLst==FALSE)&&(_this->outCount >0 )){
			num=dxLstGetSize(_this->outLst);
			/*search a exist subscription */
			for(pos=0;pos<num;pos++){
				tmpsub=dxLstPeek(_this->outLst,pos);
				if(tmpsub == _sub){
					/* compare successful and remove this UaSub */
					tmpsub=dxLstGetAt(_this->outLst,pos);
					/*uaSubFree(tmpsub);*/
					_this->outCount--;
					rcode=RC_OK;
				}
			}

		}
	}
	
	return rcode;
}

/** get a UaSub from in/out list 
 * \param _this input UaClass
 * \param _sub  target UaSub to be gotten
 * \param _inLst True for getting UaSub from in-list otherwise
 * \retval UaSub object
 */
UaSub uaClassGetSub(IN UaClass _this, IN char* _url, IN BOOL _inLst)
{
	UaSub rSub=NULL,tmpsub=NULL;
	int pos,num;
	char *posx=NULL,buf[128]={'\0'};
	
	/*check class & url */
	if(_this == NULL){
		UaCoreERR("[uaClassGetSub] class is NULL!\n");
		return rSub;
	}
	if(_url == NULL){
		UaCoreERR("[uaClassGetSub] url is NULL!\n");
		return rSub;
	}
	posx=strchr(_url,'@');
	if(posx){
		posx++;
		posx=strchr(posx,':');
		if(!posx){
			sprintf(buf,"%s:5060",_url);
			posx=buf;
		}else
			posx=_url;
	}else
		return NULL;
	/*match url for subscriptions in list */
	if(_inLst == TRUE){
		num=dxLstGetSize(_this->inLst);
		for(pos=0;pos<num;pos++){
			tmpsub=dxLstPeek(_this->inLst,pos);
			if(strICmp(uaSubGetURI(tmpsub),posx)==0){
				rSub=tmpsub;
				break;
			}
		}/* end of for loop */
	}else{
		num=dxLstGetSize(_this->outLst);
		for(pos=0;pos<num;pos++){
			tmpsub=dxLstPeek(_this->outLst,pos);
			if(strICmp(uaSubGetURI(tmpsub),posx)==0){
				rSub=tmpsub;
				break;
			}
		}/* end of for loop */
	}
	return rSub;
}

/* get a UaSub from in/out list 
 * \param _this input UaClass
 * \param _position  position for UaSub object
 * \param _inLst True for getting UaSub from in-list otherwise
 * \retval UaSub object 
 */
UaSub uaClassGetSubAt(IN UaClass _this, IN int _position, IN BOOL _inLst)
{
	UaSub rSub=NULL;
	int num;
	
	/*check class & url */
	if(_this == NULL){
		UaCoreERR("[uaClassGetSubAt] class is NULL!\n");
		return rSub;
	}
	if(_position < 0){
		UaCoreERR("[uaClassGetSubAt] postition is illeage!\n");
		return rSub;
	}
	/*match url for subscriptions in list */
	if(_inLst == TRUE){
		num=dxLstGetSize(_this->inLst);
		if(_position<num)  
			rSub=dxLstPeek(_this->inLst,_position);
			
	}else{
		num=dxLstGetSize(_this->inLst);
		if(_position<num)  
			rSub=dxLstPeek(_this->outLst,_position);
	}
	return rSub;
}
/** get number of sub in in/out list
 * \param _this input UaClass
 * \param _inLst specify in in-list or out-list
 * \retval integer of number of UaSub objects in this class
 */
int	uaClassGetNumOfSub(IN UaClass _this, IN BOOL _inLst)
{
	int count=0;

	if(_this != NULL){
		if(_inLst==TRUE)
			count=_this->inCount;
		else
			count=_this->outCount;
	}
	return count;
}
