/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * dx_vec.c
 *
 * $Id: dx_vec.c,v 1.26 2004/09/23 04:41:27 tyhuang Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <common/cm_def.h>
#include <common/cm_utl.h>
#include <low/cx_mutex.h>
#include "dx_vec.h"
#include <common/cm_trace.h>

struct dxVectorObj {
	void**          elmtbl_;
	int		dxVectorSize_;
	int             capacityNow_;
	int             capacityIncrement_;
	DxVectorType	type_;

	CxMutex		cs_;
	TraceFlag	traceFlag_;
};

/*=================================================================
// DxVector methods
*/
DxVector dxVectorNew(int initialCapacity, int capacityIncrement, DxVectorType type)
{
	DxVector _this = (DxVector)malloc(sizeof(struct dxVectorObj));

	/* memory leak*/
	if ( _this==NULL )
		return NULL;

	_this->dxVectorSize_ = 0;
	_this->capacityNow_ = (initialCapacity>0)?initialCapacity:10;
	_this->capacityIncrement_ = (capacityIncrement>0)?capacityIncrement:0;
	_this->type_ = type;
	_this->elmtbl_ = (void**)malloc(sizeof(void*)*_this->capacityNow_);
	if( !_this->elmtbl_ ) {
		free( _this );
		return NULL;
	}

	memset(_this->elmtbl_,0,sizeof(void*)*_this->capacityNow_);
	_this->traceFlag_ = TRACE_OFF;

	_this->cs_ = cxMutexNew(CX_MUTEX_INTERTHREAD,0); 
	if(!_this->cs_) {
		free(_this->elmtbl_);
		free(_this);
		return NULL;
	}

	return _this;
}

void dxVectorFree(DxVector _this,void(*elementFreeCB)(void*))
{
	int    i = 0;
	void*  iter;

	if (!_this)
		return;
	
	cxMutexLock(_this->cs_);
	for( i=0; i<_this->dxVectorSize_; i++) {
		iter = _this->elmtbl_[i];

		if( _this->type_==DX_VECTOR_POINTER ) {
			if( elementFreeCB ) 
				elementFreeCB(iter);
		}
		else
			free(iter); /* string or integer */
	}
	cxMutexUnlock(_this->cs_);

	cxMutexFree(_this->cs_);

	free(_this->elmtbl_);
	free(_this);
}

DxVector  dxVectorDup(DxVector _this,void*(*elementDupCB)(void*))
{
	DxVector dup = NULL;
	int      i;

	if( !_this )
		return NULL;
	if( _this->type_!=DX_VECTOR_POINTER &&
	    _this->type_!=DX_VECTOR_INTEGER &&
	    _this->type_!=DX_VECTOR_STRING )
	    return NULL;
	
	if( cxMutexLock(_this->cs_)!=0 )
		return NULL;

	dup = (DxVector)malloc(sizeof(struct dxVectorObj));
	if( !dup ) {
		goto ERR_RETURN;
	}
		
	dup->capacityIncrement_ = _this->capacityIncrement_;
	dup->capacityNow_  = _this->capacityNow_;
	dup->dxVectorSize_ = _this->dxVectorSize_;
	dup->traceFlag_    = _this->traceFlag_;
	dup->type_         = _this->type_;

	dup->cs_ = cxMutexNew(CX_MUTEX_INTERTHREAD,0); 
	if(!dup->cs_) {
		free(dup);
		goto ERR_RETURN;
	}

	dup->elmtbl_ = (void**)malloc(sizeof(void*)*_this->capacityNow_);
	if( !dup->elmtbl_ ) {
		cxMutexFree( dup->cs_ );
		free(dup);
		goto ERR_RETURN;
	}
	memset(dup->elmtbl_,0,sizeof(void*)*_this->capacityNow_);	
	for( i=0; i<_this->dxVectorSize_; i++) {
		/* !!! NOTICE !!!
		   it is hard to manage resource when out of memory to duplicate
		   elements. It's better clean all allocated resources and return
		   NULL, but we can't do it because we don't know how to clean 
		   resource if APP uses a $elementDupCB to duplicate elements. */
		/* Currently, we leave the element pointer( $dup->elmtbl_[i] )
		   NULL, if this case happends. */
		if( dup->type_==DX_VECTOR_POINTER ) {
			dup->elmtbl_[i] = (elementDupCB)?
			                  elementDupCB(_this->elmtbl_[i]):
			                  _this->elmtbl_[i];
		}
		else if( dup->type_==DX_VECTOR_INTEGER ) {
			dup->elmtbl_[i] = (void*)malloc(sizeof(int));
			if( dup->elmtbl_[i] )
				*((int*)dup->elmtbl_[i]) = *((int*)_this->elmtbl_[i]);
		}
		else if( dup->type_==DX_VECTOR_STRING ) 
			dup->elmtbl_[i] = strDup(_this->elmtbl_[i]);
	}
	
	if( cxMutexUnlock(_this->cs_)!=0 )
		return NULL;

	return dup;

ERR_RETURN:
	if( cxMutexUnlock(_this->cs_)!=0 )
		return NULL;

	return NULL;
}

int dxVectorGetSize(DxVector _this)
{
	return ((_this)?_this->dxVectorSize_:0);
}

DxVectorType dxVectorGetType(DxVector _this)
{
	return ((_this)?_this->type_:0);
}

int dxVectorGetCapacity(DxVector _this)
{
	return ((_this)?_this->capacityNow_:0);
}

void dxVectorTrimToSize(DxVector _this)
{
	void** tmp;
	int    i;

	if( !_this )
		return;
	if( cxMutexLock(_this->cs_)!=0 )
		return;

	tmp = (void**)malloc(sizeof(void*)*_this->dxVectorSize_);
	if( !tmp ) {
		if( cxMutexUnlock(_this->cs_)!=0 )
			return;	
		return;
	}
	memset(tmp, 0, sizeof(void*)*_this->dxVectorSize_);
	for( i=0; i<_this->dxVectorSize_; i++)
		tmp[i] = _this->elmtbl_[i];

	free(_this->elmtbl_);
	_this->elmtbl_ = tmp;
	_this->capacityNow_ = _this->dxVectorSize_;

	if( cxMutexUnlock(_this->cs_)!=0 )
		return;
}

RCODE dxVectorEnsureCapacity(DxVector _this, int minCapacity)
{
	void** tmp;
	int    newCapacityNow,i;

	if( !_this )
		return RC_ERROR;

	if( _this->capacityNow_ > minCapacity )
		return RC_ERROR;

	if( cxMutexLock(_this->cs_)!=0 )
		return RC_ERROR;

	if( _this->capacityIncrement_+_this->capacityNow_ > minCapacity ) 
		newCapacityNow = _this->capacityIncrement_+_this->capacityNow_;
	else if( _this->capacityNow_*2 > minCapacity )
		newCapacityNow = _this->capacityNow_ * 2;
	else 
		newCapacityNow = minCapacity;

	tmp = (void**)malloc(sizeof(void*)*newCapacityNow);
	if( !tmp ) {
		if( cxMutexUnlock(_this->cs_)!=0 )
			return RC_ERROR;
		return RC_ERROR;
	}	
	memset(tmp, 0, sizeof(void*)*newCapacityNow);
	for( i=0; i<_this->dxVectorSize_; i++)
		tmp[i] = _this->elmtbl_[i];

	free(_this->elmtbl_);
	_this->elmtbl_ = tmp;	
	_this->capacityNow_ = newCapacityNow;
	
	if( cxMutexUnlock(_this->cs_)!=0 )
		return RC_ERROR;

	return RC_OK;
}

RCODE	dxVectorAddElement(DxVector _this,void* elemt)
{
	int newSize;

	if( !_this )
		return RC_E_PARAM;
	if( _this->type_!=DX_VECTOR_POINTER &&
	    _this->type_!=DX_VECTOR_INTEGER &&
	    _this->type_!=DX_VECTOR_STRING )
	    return RC_ERROR;

        if( cxMutexLock(_this->cs_)!=0 )
                return RC_ERROR;

	/* check capacity */
	newSize = _this->dxVectorSize_ + 1;
	if( newSize > _this->capacityNow_ && _this->capacityIncrement_<=0 ) {
			goto ERR_RETURN;
	}
	else if ( newSize > _this->capacityNow_ ) {
		if( dxVectorEnsureCapacity( _this, newSize)!=0 ) 
			goto ERR_RETURN;
	}

	/* add element*/
	if( _this->type_==DX_VECTOR_POINTER )
		_this->elmtbl_[_this->dxVectorSize_] = elemt;
	else if( _this->type_==DX_VECTOR_INTEGER ) {
		_this->elmtbl_[_this->dxVectorSize_] = malloc(sizeof(int));
		if( !_this->elmtbl_[_this->dxVectorSize_] )
			goto ERR_RETURN;
		*(int*)_this->elmtbl_[_this->dxVectorSize_] = (elemt)?*(int*)elemt:0;
	}
	else { /* DX_VECTOR_STRING */
		_this->elmtbl_[_this->dxVectorSize_] = strDup(elemt);
		if( !_this->elmtbl_[_this->dxVectorSize_] )
			goto ERR_RETURN;
	}
	
	_this->dxVectorSize_++;

	if( cxMutexUnlock(_this->cs_)!=0 )
		return RC_ERROR;

	return RC_OK;

ERR_RETURN:
	if( cxMutexUnlock(_this->cs_)!=0 )
		return RC_ERROR;
	return RC_ERROR;
}

RCODE	dxVectorAddElementAt(DxVector _this,void* elemt,int position)
{
	int   i;
	int   newSize;
	void* tmp = NULL;

	if( !_this )
		return RC_E_PARAM;
	if( _this->type_!=DX_VECTOR_POINTER &&
	    _this->type_!=DX_VECTOR_INTEGER &&
	    _this->type_!=DX_VECTOR_STRING )
	    return RC_ERROR;
	if( position<0 || position>_this->dxVectorSize_ )
		return RC_E_PARAM;

	if( cxMutexLock(_this->cs_)!=0 )
		return RC_ERROR;

	/* check capacity */
	newSize = _this->dxVectorSize_ + 1;
	if( newSize > _this->capacityNow_ && _this->capacityIncrement_<=0 ) {
			goto ERR_RETURN;
	}
	else if ( newSize > _this->capacityNow_ ) {
		if( dxVectorEnsureCapacity( _this, newSize)!=0 ) 
			goto ERR_RETURN;
	}

	if( _this->type_==DX_VECTOR_POINTER )
		tmp = elemt;
	else if( _this->type_==DX_VECTOR_INTEGER ) {
		tmp = malloc(sizeof(int));
		if( !tmp )
			goto ERR_RETURN;
		*(int*)tmp = (elemt)?*(int*)elemt:0;
	}
	else { /* DX_VECTOR_STRING */
		tmp = strDup(elemt);
		if( !tmp )
			goto ERR_RETURN;
	}

	/* add element*/
	for( i=_this->dxVectorSize_-1; i>=position; i--)
		_this->elmtbl_[i+1] = _this->elmtbl_[i];

	_this->elmtbl_[position] = tmp;
	_this->dxVectorSize_++;

	if( cxMutexUnlock(_this->cs_)!=0 )
		return RC_ERROR;

	return RC_OK;

ERR_RETURN:
	if( cxMutexUnlock(_this->cs_)!=0 )
		return RC_ERROR;
	return RC_ERROR;
}

void* dxVectorGetLastElement(DxVector _this)
{
	return (_this)?_this->elmtbl_[_this->dxVectorSize_-1]:NULL; 
}

void* dxVectorGetElementAt(DxVector _this,int position)
{
	if ( _this==NULL || position>=_this->dxVectorSize_ || position<0 )
		return NULL;

	return _this->elmtbl_[position];	
}

/*========================================================================
// get index of Element,search from given index
// retuen < 0 when error
*/
int dxVectorGetIndexOf(DxVector _this,void* elemt,int idx_from)
{
	int i = -1;

	if( _this==NULL || elemt==NULL || 
	    _this->dxVectorSize_==0 || idx_from>=_this->dxVectorSize_ ||
	    idx_from<0 )
		return -1;
	if( _this->type_!=DX_VECTOR_POINTER &&
	    _this->type_!=DX_VECTOR_INTEGER &&
	    _this->type_!=DX_VECTOR_STRING )
		return -1;
	
	if( cxMutexLock(_this->cs_)!=0 )
		return -1;

	for(i=idx_from;i<_this->dxVectorSize_;i++) {
		switch( _this->type_) {
		case DX_VECTOR_POINTER:
			if( _this->elmtbl_[i]==elemt )
				goto ELM_FOUND;
			break;
		case DX_VECTOR_INTEGER:
			if( *(int*)_this->elmtbl_[i]==*(int*)elemt )
				goto ELM_FOUND;
			break;
		case DX_VECTOR_STRING:
			if( strcmp(_this->elmtbl_[i],elemt)==0 )
				goto ELM_FOUND;
			break;	
		}
	}
	if( cxMutexUnlock(_this->cs_)!=0 )
		return -1;

	/* not found */
	return -1;

ELM_FOUND:
	if( cxMutexUnlock(_this->cs_)!=0 )
		return -1;

	return i;
}

/*========================================================================
// get index of Element,search backward from given index
// retuen < 0 when error
*/
int dxVectorGetLastIndexOf(DxVector _this,void* elemt,int idx_from)
{
	int i;

	if( _this==NULL || elemt==NULL || 
	    _this->dxVectorSize_==0 || idx_from>=_this->dxVectorSize_||
	    idx_from<0 )
		return -1;
	if( _this->type_!=DX_VECTOR_POINTER &&
	    _this->type_!=DX_VECTOR_INTEGER &&
	    _this->type_!=DX_VECTOR_STRING )
	    return -1;
	
	if( cxMutexLock(_this->cs_)!=0 )
		return -1;

	for(i=idx_from;i>=0;i--) {
		switch( _this->type_) {
		case DX_VECTOR_POINTER:
			if( _this->elmtbl_[i]==elemt )
				goto ELM_FOUND;
			break;
		case DX_VECTOR_INTEGER:
			if( *(int*)_this->elmtbl_[i]==*(int*)elemt )
				goto ELM_FOUND;
			break;
		case DX_VECTOR_STRING:
			if( strcmp(_this->elmtbl_[i],elemt)==0 )
				goto ELM_FOUND;
			break;
		}
	}
	if( cxMutexUnlock(_this->cs_)!=0 )
		return -1;

	/* not found */
	return -1;

ELM_FOUND:
	if( cxMutexUnlock(_this->cs_)!=0 )
		return -1;

	return i;
}

/*  Fun: dxVectorRemoveElement
 *
 *  Desc: Removes the first occurrence of $elemt from this vector.
 *        If vector type is DX_VECTOR_POINTER, user is responsible
 *        to free the memory pointed by $elemt.
 *        On the case of DX_VECTOR_INTEGER and DX_VECTOR_STRING,
 *        this function call free the memory directly. 
 *
 *  Ret: RC_OK, if success
 *       RC_ERROR, if error or not found
 */
RCODE	dxVectorRemoveElement(DxVector _this,void* elemt)
{
	int   i;
	void* tmp;

	if( _this==NULL || elemt==NULL || 
	    _this->dxVectorSize_==0 )
		return RC_ERROR;
	if( _this->type_!=DX_VECTOR_POINTER &&
	    _this->type_!=DX_VECTOR_INTEGER &&
	    _this->type_!=DX_VECTOR_STRING )
	    return RC_ERROR;
	
	if( cxMutexLock(_this->cs_)!=0 )
		return RC_ERROR;

	for(i=0;i<_this->dxVectorSize_;i++) {
		switch( _this->type_) {
		case DX_VECTOR_POINTER:
			if( _this->elmtbl_[i]==elemt )
				goto ELM_FOUND;
			break;
		case DX_VECTOR_INTEGER:
			if( *(int*)_this->elmtbl_[i]==*(int*)elemt )
				goto ELM_FOUND;
			break;
		case DX_VECTOR_STRING:
			if( strcmp(_this->elmtbl_[i],elemt)==0 )
				goto ELM_FOUND;
			break;
		}
	}
	if( cxMutexUnlock(_this->cs_)!=0 )
		return RC_ERROR;

	return RC_ERROR;

ELM_FOUND:
	if( cxMutexUnlock(_this->cs_)!=0 )
		return RC_ERROR;

	tmp = dxVectorRemoveElementAt(_this,i);
	if( !tmp )
		return RC_ERROR;
	if( _this->type_!=DX_VECTOR_POINTER )
		free(tmp);

	return RC_OK;
}

/*  Fun: dxVectorRemoveElementAt
 *
 *  Desc: Remove element at a specific position of vecter. 
 *        User should free the memory space returned. ( call free() function ).
 *
 *  Ret: Return element value.
 */
void* dxVectorRemoveElementAt(DxVector _this,int position)
{
	int   i;
	void* tmp;

	if( _this==NULL || _this->dxVectorSize_==0 || 
	    position>=_this->dxVectorSize_ || position<0 )
		return NULL;
	if( _this->type_!=DX_VECTOR_POINTER &&
	    _this->type_!=DX_VECTOR_INTEGER &&
	    _this->type_!=DX_VECTOR_STRING )
	    return NULL;
	
	if( cxMutexLock(_this->cs_)!=0 )
		return NULL;
	
	tmp = _this->elmtbl_[position];

	for( i=position+1; i<_this->dxVectorSize_; i++)
		_this->elmtbl_[i-1] = _this->elmtbl_[i];

	_this->dxVectorSize_--;

	if( cxMutexUnlock(_this->cs_)!=0 )
		return NULL;

	return tmp;
}

/*========================================================================
// set this vector's trace flag ON or OFF*/
void dxVectorSetTraceFlag(DxVector _this, TraceFlag traceflag)
{
	if ( _this==NULL ) 
		return;
	_this->traceFlag_ = traceflag;
}
