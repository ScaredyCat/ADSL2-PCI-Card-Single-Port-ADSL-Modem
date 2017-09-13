/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * dx_vec.c
 *
 * $Id: dx_lst.c,v 1.16 2006/04/17 03:45:48 tyhuang Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <common/cm_def.h>
#include <common/cm_utl.h>
#include <low/cx_mutex.h>
#include "dx_lst.h"
#include <common/cm_trace.h>

typedef struct lstElmObj*  LstElm;

struct lstElmObj {
	void*           value_;
	LstElm          next_;
	LstElm          pre_;
};

struct dxLstObj {
	LstElm          head_;
	int             size_;
	int             maxSize_;
	LstElm          tail_;
	DxLstType       type_;

	LstElm			Iter_; /* for store Iterator*/
	CxMutex         cs_;
	TraceFlag       traceFlag_;
};


/*
 * LstElm methods
 */
LstElm	lstElmNew(DxLstType type)
{
	LstElm e = malloc(sizeof(struct lstElmObj));
	if( !e )
		return NULL;

	switch(type)
	{
	case DX_LST_INTEGER:
		e->value_ = malloc(sizeof(int));
		if( !e->value_ ) {
			free(e);
			return NULL;
		}
		break;
	case DX_LST_STRING:
	case DX_LST_POINTER:
	default:
		e->value_ = NULL;
	}
	e->next_ = NULL;
	e->pre_ = NULL;

	return e;
}

void lstElmSetValue(LstElm e, DxLstType type, void* value)
{	
	switch(type)
	{
	case DX_LST_INTEGER:
		*(int*)e->value_ = *(int*)value;
		break;
	case DX_LST_STRING:
		e->value_ = strDup(value);
		break;
	case DX_LST_POINTER:
	default:
		e->value_ = value;
	}
}

int lstElmValueCmp(void* value1, void* value2, DxLstType type)
{	
	switch(type)
	{
	case DX_LST_INTEGER:
		return ( *(int*)value1 == *(int*)value2 )?1:0;
		break;
	case DX_LST_STRING:
		return ( strcmp(value1,value2)==0 )?1:0;
		break;
	case DX_LST_POINTER:
	default:
		return ( value1 == value2 )?1:0;
	}
}

void lstElmFree(LstElm e, DxLstType type, void(*lstElmFreeCB)(void*))
{
	switch(type)
	{
	case DX_LST_INTEGER:
	case DX_LST_STRING:
		free(e->value_);
		break;
	case DX_LST_POINTER:
	default:
		if(lstElmFreeCB)
			lstElmFreeCB(e->value_);
	}

	free(e);
}

DxLst   dxLstNew(DxLstType t)
{
	DxLst _this = (DxLst)calloc(1,sizeof(struct dxLstObj));

	if(!_this)
		return NULL;
	_this->head_ = NULL;
	_this->tail_ = NULL;
	_this->type_ = t;
	_this->size_ = 0;
	_this->traceFlag_ = 0;

	_this->Iter_ = NULL;
	_this->cs_ = cxMutexNew(CX_MUTEX_INTERTHREAD,0);
	if(!_this->cs_) {
		free(_this);
		return NULL;
	}

	return _this;
}

void    dxLstFree(DxLst _this, void(*elementFreeCB)(void*))
{
	LstElm Iterator = ((_this)?_this->head_:NULL);
	LstElm temp = NULL;
	
	if( !_this )
		return;
	
	cxMutexLock(_this->cs_);
	while ( Iterator ) {
		temp = Iterator;
		Iterator = Iterator->next_;

		lstElmFree(temp,_this->type_,elementFreeCB);
	}
	cxMutexUnlock(_this->cs_);

	cxMutexFree(_this->cs_);

	if (_this)
		free(_this);
}

DxLst   dxLstDup(DxLst _this, void*(*elementDupCB)(void*))
{
	DxLst dup = NULL;
	LstElm Iterator;
	if( !_this )
		return NULL;
	
	if( cxMutexLock(_this->cs_)!=0 )
		return NULL;

	dup = dxLstNew(_this->type_);

	Iterator = ((_this)?_this->head_:NULL);
	while (Iterator) {
		dxLstPutTail(dup,(elementDupCB)?
		                 elementDupCB(Iterator->value_):
		                 Iterator->value_);
		Iterator = Iterator->next_;
	}

	if( cxMutexUnlock(_this->cs_)!=0 )
		return NULL;

	return dup;
}

int     dxLstGetSize(DxLst _this)
{
	return (_this)?_this->size_:-1;
}

DxLstType   dxLstGetType(DxLst _this)
{
	return (_this)?_this->type_:0;
}

RCODE   dxLstPutHead(DxLst _this, void* elm)
{
	LstElm e = NULL;
	if( !_this )
		return RC_E_PARAM;

	if( cxMutexLock(_this->cs_)!=0 )
		return RC_ERROR;
	if ( (e=lstElmNew(_this->type_)) == NULL ) {
		if( _this->traceFlag_ )
			TCRPrint(1,"<dxLstPutHead>: lstElmNew error.\n");
		cxMutexUnlock(_this->cs_);
		return RC_ERROR;
	}
	lstElmSetValue(e,_this->type_,elm);

	if ( _this->size_==0 ) {
		_this->head_ = e;
		_this->tail_ = e;
		_this->size_++;
	}
	else {
		e->next_ = _this->head_;
		_this->head_->pre_ = e;
		_this->head_ = e;
		_this->size_++;
	}

	if( cxMutexUnlock(_this->cs_)!=0 )
		return RC_ERROR;

	return RC_OK;
}

RCODE   dxLstPutTail(DxLst _this,void* elm)
{
	LstElm e = NULL;
	if( !_this )
		return RC_E_PARAM;

	if( cxMutexLock(_this->cs_)!=0 )
		return RC_ERROR;
	if ( (e=lstElmNew(_this->type_)) == NULL ) {
		if( _this->traceFlag_ )
			TCRPrint(1,"<dxLstPutHead>: lstElmNew error.\n");
		cxMutexUnlock(_this->cs_);
		return RC_ERROR;
	}
	lstElmSetValue(e,_this->type_,elm);

	if ( _this->size_==0 ) {
		_this->head_ = e;
		_this->tail_ = e;
		_this->size_++;
	}
	else {
		e->pre_ = _this->tail_;
		_this->tail_->next_ = e;
		_this->tail_ = e;
		_this->size_++;
	}

	if( cxMutexUnlock(_this->cs_)!=0 )
		return RC_ERROR;

	return RC_OK;
}

void*   dxLstGetHead(DxLst _this)
{
	LstElm tmp = NULL;
	void*   val;

	if( _this==NULL || _this->size_==0 )
		return NULL;

	if( cxMutexLock(_this->cs_)!=0 )
		return NULL;

	val = _this->head_->value_;
	tmp = _this->head_;

	_this->head_ = _this->head_->next_;
	if( (--_this->size_)==0 ) 
		_this->tail_ = NULL;
	else
		_this->head_->pre_ = NULL;
	free(tmp);

	if( cxMutexUnlock(_this->cs_)!=0 )
		return NULL;

	return val;
}

void*   dxLstGetTail(DxLst _this)
{
	LstElm tmp = NULL;
	void*   val;

	if( _this==NULL || _this->size_==0 )
		return NULL;

	if( cxMutexLock(_this->cs_)!=0 )
		return NULL;

	val = _this->tail_->value_;
	tmp = _this->tail_;

	_this->tail_ = _this->tail_->pre_;
	if( (--_this->size_)==0 )
		_this->head_ = NULL;
	else
		_this->tail_->next_ = NULL;
	free(tmp);

	if( cxMutexUnlock(_this->cs_)!=0 )
		return NULL;

	return val;
}


/*  Fun: dxLstInsert
 *
 *  Desc: Insert element into list at position $pos. 
 *
 *  Ret: >=0    imply the real position inserted.
 *       -1     if error.
 */
int   dxLstInsert(DxLst _this, void* elm, int pos)
{
	LstElm e = NULL;
	LstElm iter = NULL;
	int    r_pos;

	if( !_this || pos<0 )
		return -1;

	if( cxMutexLock(_this->cs_)!=0 )
		return -1;
	if ( (e=lstElmNew(_this->type_)) == NULL ) {
		if( _this->traceFlag_ )
			TCRPrint(1,"<dxLstPutHead>: lstElmNew error.\n");
		cxMutexUnlock(_this->cs_);
		return -1;
	}
	lstElmSetValue(e,_this->type_,elm);

	if ( _this->size_==0 ) {
		_this->head_ = e;
		_this->tail_ = e;
		_this->size_++;
		r_pos = 0;
	}
	else if( pos>=_this->size_ ) { /* put element at tail of list*/
		e->pre_ = _this->tail_;
		_this->tail_->next_ = e;
		_this->tail_ = e;
		_this->size_++;

		r_pos = _this->size_-1;
	}
	else if( pos<=0 ) { /* put element at head of list*/
		e->next_ = _this->head_;
		_this->head_->pre_ = e;
		_this->head_ = e;
		_this->size_++;

		r_pos = 0;
	}
	else { /* insert element at $pos of list*/
		for( r_pos=0,iter=_this->head_; 
		     (r_pos<pos)&&(iter!=NULL); 
		     r_pos++,iter=iter->next_ 
		   );
		
		iter->pre_->next_ = e;
		e->pre_ = iter->pre_;
		e->next_ = iter;
		iter->pre_ = e;

		_this->size_++;
	}

	if( cxMutexUnlock(_this->cs_)!=0 )
		return -1;

	return r_pos;
}

void*   dxLstGetAt(DxLst _this,int pos) 
{
	void*  value = NULL;
	LstElm Iter = ((_this)?_this->head_:NULL);
	LstElm e = NULL;

	if ( !_this || !Iter )
		return NULL;
	if( cxMutexLock(_this->cs_)!=0 )
		return NULL;

	if ( pos>=_this->size_ || pos<0 ) {
		if( _this->traceFlag_ )
			TCRPrint(2,"%s,dxLstGetAt() %d:"
				   "position>=size_ or position<0.\n"
				  ,__FILE__,__LINE__);
		if( cxMutexUnlock(_this->cs_)!=0 )
			return NULL;
		return NULL;
	}
	if ( pos==0 ) { /* position equal to 0*/
		e = _this->head_;
		_this->head_ = _this->head_->next_;	
		_this->size_--;
	}
	else {	/* position>0 */
		int i = 0;

		while( i!=pos-1 ) {
			Iter = Iter->next_;
			i++;
		}
		e = Iter->next_;
		Iter->next_ = e->next_;
		if( pos==_this->size_-1 )
			_this->tail_ = Iter;
		else
			e->next_->pre_ = Iter;
		_this->size_--;
	}

	if (e) {
		value = e->value_;
		free(e);
	}
	if( cxMutexUnlock(_this->cs_)!=0 )
		return NULL;

	return value;
}

void*   dxLstPeek(DxLst _this,int pos)
{
	LstElm Iter = ((_this)?_this->head_:NULL);
	int i = 0;

	if ( _this==NULL || pos>=_this->size_ || pos<0 )
		return NULL;

	if( cxMutexLock(_this->cs_)!=0 )
		return NULL;

	while (i<pos)
	{
		Iter = Iter->next_;
		i++;
	}
	if( cxMutexUnlock(_this->cs_)!=0 )
		return NULL;

	return Iter->value_;
}

void*     dxLstPeekTail(DxLst _this)
{
	LstElm Iter = ((_this)?_this->tail_:NULL);
	
	if(Iter)
		return Iter->value_;
	else
		return NULL;
}

void*   dxLstPeekByIter(DxLst _this)
{
	LstElm Iter;
	int i = 0;

	if ( (_this==NULL) || (_this->Iter_==NULL))
		return NULL;

	if( cxMutexLock(_this->cs_)!=0 )
		return NULL;

	/* set return value */
	Iter=_this->Iter_;
	/* update */
	_this->Iter_=Iter->next_;
	
	if( cxMutexUnlock(_this->cs_)!=0 )
		return NULL;

	return Iter->value_;
}

RCODE	  dxLstResetIter(DxLst _this)
{
	if ( _this==NULL)
		return RC_ERROR;

	if( cxMutexLock(_this->cs_)!=0 )
		return RC_ERROR;
	
	_this->Iter_ = _this->head_;

	if( cxMutexUnlock(_this->cs_)!=0 )
		return RC_ERROR;

	return RC_OK;
}

