/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * ua_content.c
 *
 * $Id: ua_content.c,v 1.11 2004/09/24 05:45:32 tyhuang Exp $
 */
#include "ua_content.h"
#include <common/cm_utl.h>

struct uaContentObj{
	char* ContentType;
	int ContentLength;
	void* ContentBody;
};

/* create a UaContent object */
UaContent uaContentNew(IN char* _type, IN int _length, IN void* _body)
{
	UaContent _this=NULL;

	/* check input value */
	if(_type == NULL){
		UaCoreERR("[uaContentNew] content-type is NULL!!!\n");
		return NULL;
	}
	if((_length < 0)&&(_length > 10000)){
		UaCoreERR("[uaContentNew] content-length is too large or too small!!!\n");
		return NULL;
	}
	if(_body == NULL){
		UaCoreERR("[uaContentNew] content-body is NULL!!!\n");
		return NULL;
	}
	/* create a new objec */
	_this=(UaContent)calloc(1,sizeof(struct uaContentObj));
	if(_this != NULL){
		_this->ContentLength=_length;
		_this->ContentType=strDup(_type);
		/* check memory alloc */
		if(_this->ContentType == NULL){
			free(_this);
			UaCoreERR("[uaContentNew] To create content-type is failed!!!\n");
			return NULL;
		}
		_this->ContentBody=(void *)calloc(_length+1,1);
		if(_this->ContentBody!=NULL)
			memcpy(_this->ContentBody,_body,_length);
		else{
			_this->ContentLength=0;
			UaCoreERR("[uaContentNew] To create content-body is failed!!!\n");
			free(_this->ContentType);
			free(_this);
		}
	}
	return _this;
}

/* free a UaContent object */
void uaContentFree(IN UaContent _this)
{
	if(_this != NULL){
		_this->ContentLength=0;
		if(_this->ContentType != NULL){
			free(_this->ContentType);
			_this->ContentType=NULL;
		}
		if(_this->ContentBody != NULL){
			free(_this->ContentBody);
			_this->ContentBody = NULL;
		}
		free(_this);
		_this=NULL;
	}
}

/* duplicate a object */
UaContent uaContentDup(IN UaContent _source)
{
	UaContent _this=NULL;
	
	if(_source == NULL){
		UaCoreERR("[uaContentDup] content is NULL!\n");
		return NULL;
	}
	_this=(UaContent)calloc(1,sizeof(struct uaContentObj));
	if(_this == NULL){
		UaCoreERR("[uaContentDup] To new content is failed!!!\n");
		return NULL;
	}
	if(_source->ContentLength > 0)
		_this->ContentLength=_source->ContentLength;
	else
		_this->ContentLength=0;

	if(_source->ContentType != NULL){
		_this->ContentType=strDup(_source->ContentType);
		/* check memory alloc */
		if(_this->ContentType == NULL){
			free(_this);
			UaCoreERR("[uaContentDup] To create content-type is failed!!!\n");
			return NULL;
		}
	}else
		_this->ContentType=NULL;
	if(_source->ContentBody != NULL){
		_this->ContentBody=calloc(_source->ContentLength+1,1);
		memcpy(_this->ContentBody,_source->ContentBody,_source->ContentLength);
		/* check memory alloc */
		if(_this->ContentBody == NULL){
			free(_this->ContentType);
			free(_this);
			UaCoreERR("[uaContentDup] To create content-type is failed!!!\n");
			_this=NULL;
		}
	}else
		_this->ContentBody=NULL;

	return _this;
}

/*get content type*/
char* uaContentGetType(IN UaContent _this)
{
	char *rtype=NULL;

	if(_this != NULL)
		if(_this->ContentType != NULL)
			rtype=_this->ContentType;
	return rtype;
}

/*get content length */
int uaContentGetLength(IN UaContent _this)
{
	int len=0;
	
	if(_this!=NULL)
		len=_this->ContentLength;
	return len;
}

/*get content */
void* uaContentGetBody(IN UaContent _this)
{
	void *body=NULL;
	
	if(_this != NULL)
		if(_this->ContentBody != NULL)
			body=_this->ContentBody;
	return body;
}
