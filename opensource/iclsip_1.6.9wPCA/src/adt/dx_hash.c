/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* Copyright Telcordia Technologies 1999 */
/*
 * dx_hash.c
 *
 * $Id: dx_hash.c,v 1.22 2006/05/26 06:30:01 tyhuang Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "dx_hash.h"
#include <low/cx_mutex.h>
#include <common/cm_utl.h>
#include <common/cm_trace.h>

/*
typedef struct entryObj		*Entry;
*/
struct entryObj {
	char* key_;
	char* val_;
	Entry next_;
};

struct dxHashObj {
	Entry*		table_;
	int		CELLS_, elements_;
	int		keysUpper_, strings_;
	Entry		iterNext_;	      /* next entry in current bucket */
	int		bucketNext_;	      /* next bucket to search */

	TraceFlag	traceFlag_;
	CxMutex         cs_;
};

/* create new hashtable
 * initSize = number of entries in table_ (may grow)
 * keysUpper_: bool should keys be mapped to upper case
 * strings_: DxHashType should val be copied (cstrings and ints)
 *           by dxHashAdd
 * ps. keys are always strings_ and always copied
 */ 
DxHash dxHashNew(int _initSize, int keysUpper, DxHashType strings)
{
	/*int i;*/
	DxHash _this;
	
	/*_this = malloc(sizeof *_this);*/
	_this = calloc(1,sizeof *_this);
	if( !_this )
		return NULL;
		
	_this->cs_ = cxMutexNew(CX_MUTEX_INTERTHREAD,0);
	if( !_this->cs_ ) {
		free(_this);
		return NULL;	
	}
	
	_this->CELLS_ = _initSize;
	_this->table_ = calloc(_initSize, sizeof(void*));
	if( !_this->table_ ) {
		cxMutexFree(_this->cs_);
		free(_this);
		return NULL;	
	}
		
	_this->elements_ = 0;
	_this->keysUpper_ = keysUpper;
	_this->strings_ = strings;
	
	/*
	for( i=0; i<_this->CELLS_; i++ ){
		_this->table_[i] = NULL;
	}
	*/

	_this->iterNext_ = NULL;
	_this->bucketNext_ = 0;
	
	return _this;
}

/* destroy hashtable, call freeVal on each val */
void dxHashFree(DxHash _this, void (*freeVal)(void*))
{
	int i;
	if( _this==NULL ) return;
	
	for( i=0; i<_this->CELLS_; i++ ){
		Entry p;
	
		for(p=_this->table_[i]; p != NULL ; ) {
			Entry t;
	    
			free(p->key_);
			if( freeVal!=NULL ) (*freeVal)(p->val_);
			t=p;
			p=t->next_;
			free(t);
		}
	}

	cxMutexFree(_this->cs_);
	free(_this->table_);
	free(_this);
}


unsigned int hashCode(DxHash _this,const char* str)
{
	unsigned int i = 0;
	while( *str != 0 ) {
		i <<= 1;
		i += (_this->keysUpper_ ? toupper(*str) : *str);
		str++;
	}
	return i % _this->CELLS_;
}

Entry lookUp(DxHash _this,const char* str, Entry bucket, Entry* prev)
{
	*prev = NULL;
	while( bucket != NULL ) {
		if( (_this->keysUpper_
		     ? strICmp(bucket->key_, str)
		     : strcmp(bucket->key_, str)
		    )== 0 ) {
			return bucket;
		}
		*prev  = bucket;
		bucket = bucket->next_;
	}
	return NULL;
}

/* change string in place to upper case */
static void upcaseKey(char* s)
{
	while( *s ) {
		*s = toupper(*s);
		s++;
	}
}

void startkeys(DxHash _this)
{
	if( !_this )
		return;
	
	_this->iterNext_ = NULL;
	_this->bucketNext_ = 0;
}

char* nextkeys(DxHash _this)
{
	if( !_this )
		return NULL;

	while( _this->iterNext_ == NULL && _this->bucketNext_ < _this->CELLS_ ) {
		_this->iterNext_ = _this->table_[_this->bucketNext_];
		_this->bucketNext_ ++;
	}
	
	if( _this->iterNext_ != NULL ) {
		Entry e = _this->iterNext_;
		_this->iterNext_ = e->next_;
		
		return e->key_;
	}
	
	return NULL;
}

/* Duplicate the hash */
DxHash  dxHashDup(DxHash _this, void*(*elementDupCB)(void*))
{
	DxHash dup = NULL;
	char*  nextkey = NULL;
	Entry  e, prev;
	
	if( !_this )
		return NULL;
	
	if( cxMutexLock(_this->cs_)!=RC_OK )
		return NULL;
		
	dup = dxHashNew( _this->CELLS_, _this->keysUpper_, _this->strings_);

	/* start keys */
	startkeys( _this );

	while( (nextkey = nextkeys( _this )) ) {
		void* value = NULL;
		void* valuedup = NULL;
		int h;

		/* dxHashItem */
		h = hashCode(_this, nextkey);
		e = lookUp(_this, nextkey, _this->table_[h], &prev);
		if( e == NULL ) {
			dxHashFree( dup, NULL );
			cxMutexUnlock(_this->cs_);
			return NULL;
		}
		
		value = e->val_;

		if( _this->strings_==DX_HASH_POINTER ) {
			if( elementDupCB )
				valuedup = elementDupCB( value );
			else 
				valuedup = value;
		}		
		else if( _this->strings_==DX_HASH_CSTRING ) {
			/*valuedup = value;*/
			valuedup = strDup(value);
		}
		else if( _this->strings_==DX_HASH_INT ) {
			valuedup = value;
		}
		else {
			dxHashFree( dup, NULL );
			cxMutexUnlock(_this->cs_);
			return NULL;
		}	
		dxHashAdd( dup, nextkey, valuedup);
	}
	
	cxMutexUnlock(_this->cs_);
	
	return dup;
}

/* add pair key/val to hash, return old value */ 
void* dxHashAdd(DxHash _this,const char* key, void* val)
{
	int h;
	Entry prev, e =NULL;
	void* oldVal;
	
	if( !_this )
		return NULL; 
	
	if( cxMutexLock(_this->cs_)!=RC_OK )
		return NULL;
	
	h = hashCode(_this, key);
	e = lookUp(_this, key, _this->table_[h], &prev);
	
	if( e != NULL ) {
		oldVal = e->val_;
	} else {
		oldVal = NULL;
	
		/* create new entry */
		/*e = malloc( sizeof *e );*/
		e=calloc(1,sizeof *e);
		if( !e ) {
			TCRPrint(1, "dxHashAdd(): out of memory\n");
			goto ERR_RETURN;
		}
		e->key_ = (char *) strDup(key);
		if( !e->key_ ) {
			TCRPrint(1, "dxHashAdd(): out of memory\n");
			free( e );
			goto ERR_RETURN;
		}

		_this->elements_++;
		if( _this->keysUpper_ ) upcaseKey(e->key_);
	
		/* link into bucket */
		e->next_ = _this->table_[h];
		_this->table_[h] = e;
	}
	
	if( val == NULL ) {
		e->val_ = NULL;
	} else {
		switch( _this->strings_ ) {
		case DX_HASH_POINTER:
			e->val_ = val; 
			break;
		case DX_HASH_CSTRING:
			e->val_ = (char *) strDup(val);
			if ( !e->val_ ) {
				TCRPrint(1, "dxHashAdd(): out of memory\n");
				goto ERR_RETURN;
			}	
			break;
		case DX_HASH_INT:
			e->val_ = malloc(sizeof(int));
			if ( !e->val_ ) {
				TCRPrint(1, "dxHashAdd(): out of memory\n");
				goto ERR_RETURN;
			}
			*(int*)(e->val_) = *(int*) val;
			break;
		default:
			TCRPrint(1, "dxHashAdd():Bad value to dxHashAdd\n");
		}
	}
	
	cxMutexUnlock(_this->cs_);
	/*TCRPrint(TRACE_LEVEL_API, "dxHashAdd():number of element:%d\n",_this->elements_);*/
	
	return oldVal;

ERR_RETURN:
	cxMutexUnlock(_this->cs_);
	return NULL;	
}

/* add pair key/val to hash, return old value */ 
RCODE dxHashAddEx(DxHash _this,const char* key, void* val)
{
	int h;
	Entry e =NULL;
	
	if( !_this )
		return RC_ERROR; 
	
	if( cxMutexLock(_this->cs_)!=RC_OK )
		return RC_ERROR;
	
	h = hashCode(_this, key);
	
	/* create new entry */
	e = calloc(1,sizeof *e );
	if( !e ) {
		TCRPrint(1, "dxHashAdd(): out of memory\n");
		goto ERR_RETURN;
	}
	e->key_ = (char *) strDup(key);
	if( !e->key_ ) {
		TCRPrint(1, "dxHashAdd(): out of memory\n");
		free( e );
		goto ERR_RETURN;
	}

	_this->elements_++;
	if( _this->keysUpper_ ) upcaseKey(e->key_);

	/* link into bucket */
	if (_this->table_[h]) 
		e->next_=_this->table_[h];
	_this->table_[h] = e;
	
	if( val == NULL ) {
		e->val_ = NULL;
	} else {
		switch( _this->strings_ ) {
		case DX_HASH_POINTER:
			e->val_ = val; 
			break;
		case DX_HASH_CSTRING:
			e->val_ = (char *) strDup(val);
			if ( !e->val_ ) {
				TCRPrint(1, "dxHashAdd(): out of memory\n");
				goto ERR_RETURN;
			}	
			break;
		case DX_HASH_INT:
			e->val_ = malloc(sizeof(int));
			if ( !e->val_ ) {
				TCRPrint(1, "dxHashAdd(): out of memory\n");
				goto ERR_RETURN;
			}
			*(int*)(e->val_) = *(int*) val;
			break;
		default:
			TCRPrint(1, "dxHashAdd():Bad value to dxHashAdd\n");
		}
	}
	
	cxMutexUnlock(_this->cs_);
	
	return RC_OK;

ERR_RETURN:
	cxMutexUnlock(_this->cs_);
	return RC_ERROR;	
}
	
/* return val for key_ in hash.  NULL if not found. */
void* dxHashItem(DxHash _this,const char* key)
{
	int h;
	Entry prev,e;
	
	if( !_this )
		return NULL; 	
	
	if( cxMutexLock(_this->cs_)!=RC_OK )
		return NULL;
	
	h = hashCode(_this, key);
	e = lookUp(_this, key, _this->table_[h], &prev);
	if( e == NULL ) {
		cxMutexUnlock(_this->cs_);
		return NULL;
	}
	
	cxMutexUnlock(_this->cs_);
	
	return e->val_;
}

/* del key from hash, return old value */
void* dxHashDel(DxHash _this,const char* key)
{
	int h;
	Entry prev;
	Entry e;
	void* oldVal;
	
	if( !_this )
		return NULL; 	
		
	if( cxMutexLock(_this->cs_)!=RC_OK )
		return NULL;
	
	h = hashCode(_this, key);
	e = lookUp(_this, key, _this->table_[h], &prev);
	if( e == NULL ) {
		cxMutexUnlock(_this->cs_);
		return NULL;
	}
	
	_this->elements_--;
	oldVal = e->val_;
	
	if( prev==NULL ) 
		_this->table_[h] = e->next_;
	else             
		prev->next_ = e->next_;
	
	if( _this->iterNext_ && (_this->iterNext_ == e) )
		_this->iterNext_ = e->next_;
	
	free(e->key_);
	free(e);
	
	cxMutexUnlock(_this->cs_);
	return oldVal;
}

/* del key from hash, return RCODE*/
RCODE dxHashDelEx(DxHash _this,const char* key,void* val)
{
	int h;
	Entry prev;
	Entry e;
	
	if( !_this || ! key)
		return RC_ERROR; 	
		
	if( cxMutexLock(_this->cs_)!=RC_OK )
		return RC_ERROR;
	
	h = hashCode(_this, key);
	e=_this->table_[h];
	prev=NULL;
	while (e) {
		if (val==e->val_) break;
		prev=e;
		e=e->next_;
	}
	if( e == NULL ) {
		cxMutexUnlock(_this->cs_);
		return RC_OK;
	}
	
	_this->elements_--;
	
	if( prev==NULL ) 
		_this->table_[h] = e->next_;
	else             
		prev->next_ = e->next_;
	
	if( _this->iterNext_ && (_this->iterNext_ == e) )
		_this->iterNext_ = e->next_;
	
	free(e->key_);
	free(e);
	
	cxMutexUnlock(_this->cs_);
	
	return RC_OK;
}


/* iterator over keys in hash.  NextKeys returns NULL when no keys left.
 * Note that you cannot nest iterations over the same hashTable 
 */
void StartKeys(DxHash _this)
{
	if( !_this )
		return;
	if( cxMutexLock(_this->cs_)!=RC_OK )
		return;
	
	_this->iterNext_ = NULL;
	_this->bucketNext_ = 0;
	
	if( cxMutexUnlock(_this->cs_)!=RC_OK )
		return;
}

char* NextKeys(DxHash _this)
{
	if( !_this )
		return NULL;
	if( cxMutexLock(_this->cs_)!=RC_OK )
		return NULL;

	while( _this->iterNext_ == NULL && _this->bucketNext_ < _this->CELLS_ ) {
		_this->iterNext_ = _this->table_[_this->bucketNext_];
		_this->bucketNext_ ++;
	}
	
	if( _this->iterNext_ != NULL ) {
		Entry e = _this->iterNext_;
		_this->iterNext_ = e->next_;
		
		if( cxMutexUnlock(_this->cs_)!=RC_OK )
			return NULL;
		return e->key_;
	}
	
	cxMutexUnlock(_this->cs_);
	return NULL;
}

int dxHashSize(DxHash _this)
{
	return (_this)?_this->elements_:0;
}

/*========================================================================
// set this DxHash's trace flag ON or OFF*/
void dxHashSetTraceFlag(DxHash _this, TraceFlag traceflag)
{
	if ( _this==NULL ) 
		return;
	_this->traceFlag_ = traceflag;
}


Entry	dxHashItemLst(DxHash _this,const char* key)
{
	int h;
	Entry e;
	
	if( !_this )
		return NULL; 	
	
	if( cxMutexLock(_this->cs_)!=RC_OK )
		return NULL;
	
	h = hashCode(_this, key);
	e =  _this->table_[h];
	
	cxMutexUnlock(_this->cs_);
	
	return e;	
}

char*	dxEntryGetKey(Entry _this)
{
	if (!_this) return NULL;

	return _this->key_;
}

void*	dxEntryGetValue(Entry _this)
{
	if (!_this) return NULL;

	return _this->val_;
}

Entry	dxEntryGetNext(Entry _this)
{
	if (!_this) return NULL;

	return _this->next_;
}
