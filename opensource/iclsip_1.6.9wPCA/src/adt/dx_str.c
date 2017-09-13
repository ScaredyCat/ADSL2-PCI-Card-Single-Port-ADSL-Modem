/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* Copyright Telcordia Technologies 1999 */
/* 
 * dx_str.c
 *
 * $Id: dx_str.c,v 1.10 2003/07/09 08:30:53 sjtsai Exp $
 */

#include <stdlib.h>
#include <string.h>
#include "dx_str.h"

struct dxStrObj {
	char*	text_;
	long	len_, alloced_;
};

void dxStrInit(void) {} /* just for DLL forcelink */

CCLAPI
DxStr dxStrNew() 
{
	DxStr _this;
	_this = malloc(sizeof *_this);
	if( !_this )
		return NULL;
	
	_this->len_ = 0;
	_this->alloced_ = MINALLOC;
	_this->text_ = malloc(MINALLOC);
	if( !_this->text_ ) {
		free( _this );
		return NULL;
	}
	dxStrClr(_this);
	
	return _this;
}

CCLAPI
int dxStrLen(DxStr _this)
{
	return (_this)?_this->len_:-1;
}

CCLAPI
RCODE dxStrClr(DxStr _this)
{
	if(_this == NULL) return RC_ERROR;

	_this->len_ = 0;
	_this->text_[0] = '\0';
	return RC_OK;
}

CCLAPI
RCODE dxStrFree(DxStr _this)
{
	if(_this == NULL) return RC_ERROR;
	
	if( _this->text_ )
		free(_this->text_);
	if( _this )
		free(_this);

	return RC_OK;
}

CCLAPI
RCODE dxStrCatN(DxStr _this, const char* s, int n)
{
	long newLen;
	char *from, *to;

	if ( _this==NULL || s==NULL ) return RC_ERROR;
	
	newLen = _this->len_+n;
	if( newLen >= _this->alloced_ ) {
		char*  tmp = NULL;
		long   org_alloced = _this->alloced_;

		while( newLen >= _this->alloced_ ) {
			_this->alloced_ += _this->alloced_;
		}
		
		tmp = realloc(_this->text_, _this->alloced_);
		if( !tmp ) {
			/* if error, the original memory space is untouched */
			_this->alloced_ = org_alloced;
			return RC_ERROR;
		}
		_this->text_ = tmp;
	}
	
	/* strncpy((_this->text_) + (_this->len_), s, n); */
	/* need to determine real length copied and append '\0' 
	* n is number of characters left to copy.  if non-zero at end,
	* length was less than expected.
	*/
	from = (char*)s;
	to = (_this->text_) + (_this->len_);
	while( *from!='\0'  && n>0 ) {
		*to = *from; to++; from++;
		n--;
	}
	*to = '\0';
	_this->len_ = newLen-n;
	return RC_OK;
}

CCLAPI
RCODE dxStrCat(DxStr _this, const char* s)
{
	long newLen;

	if ( _this==NULL || s==NULL ) return RC_ERROR;;
	
	newLen = _this->len_+strlen(s);
	if( newLen >= _this->alloced_ ) {
		char*  tmp = NULL;
		long   org_alloced = _this->alloced_;
		
		while( newLen >= _this->alloced_ ) {
			_this->alloced_ += _this->alloced_;
		}
		
		tmp = realloc(_this->text_, _this->alloced_);
		if( !tmp ) {
			/* if error, the original memory space is untouched */
			_this->alloced_ = org_alloced;
			return RC_ERROR;
		}
		_this->text_ = tmp;
	}
	
	strcpy((_this->text_) + (_this->len_), s);
	_this->len_ = newLen;
	return RC_OK;
}

CCLAPI
char* dxStrAsCStr(DxStr _this)
{
	return (_this)?_this->text_:NULL;
}
