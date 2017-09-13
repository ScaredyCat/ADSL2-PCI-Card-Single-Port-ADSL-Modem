/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * dx_buf.c
 *
 * $Id: dx_buf.c,v 1.12 2004/09/23 04:41:27 tyhuang Exp $
 */

#include <memory.h>
#include <stdlib.h>
#include "dx_buf.h"
#include <low/cx_mutex.h>
#include <common/cm_trace.h>

struct dxBufferObj {
	char*		pData_;
	int		maxLen_;
	int		currLen_;
	char*		start_;
	char*		end_;

	CxMutex		cs_;
	TraceFlag	traceFlag_;
};

void dxBufferInit(void) {}

DxBuffer dxBufferNew(int total_len)
{
	DxBuffer _this = (DxBuffer)malloc(sizeof(struct dxBufferObj));
	if( !_this )
		return NULL;

	_this->pData_ = (char*)malloc((total_len+1)*sizeof(char));
	if( !_this->pData_ ) {
		free( _this );
		return NULL;	
	}
	_this->currLen_ = 0;
	_this->end_ = _this->pData_;
	_this->start_ = _this->pData_;
	_this->maxLen_ = total_len;

	_this->cs_ = cxMutexNew(CX_MUTEX_INTERTHREAD,0); 
	if(!_this->cs_) {
		free(_this->pData_);
		free(_this);
		return NULL;
	}

	return _this;
}

void dxBufferFree(DxBuffer _this)
{
	if(_this) {
		CxMutex cstemp = _this->cs_;
		_this->cs_ = NULL;
		cxMutexFree(cstemp);
		free(_this->pData_);
		free(_this);
	}
}

DxBuffer dxBufferDup(DxBuffer _this)
{
	DxBuffer buffer;

	if(!_this)
		return NULL;

	if( cxMutexLock(_this->cs_)!=0 )
		return NULL;

	buffer = dxBufferNew(_this->maxLen_);
	buffer->currLen_ = _this->currLen_;
	buffer->start_ = buffer->pData_ + ( _this->start_-_this->pData_ );
	buffer->end_ = buffer->pData_ + ( _this->end_-_this->pData_ );
	memcpy(buffer->pData_,_this->pData_,_this->maxLen_);

	if( cxMutexUnlock(_this->cs_)!=0 )
		return NULL;

	return buffer;
}

void dxBufferClear(DxBuffer _this)
{
	if(!_this)
		return;

	if( cxMutexLock(_this->cs_)!=0 )
		return;

	_this->end_ = _this->pData_;
	_this->start_ = _this->pData_;
	_this->currLen_ = 0;

	if( cxMutexUnlock(_this->cs_)!=0 )
		return;
}

int dxBufferRead(DxBuffer _this, char* buff, int len)
{
	int xlen = 0;

	if( !_this || !buff || len<0 ) return -1;
	if( _this->currLen_==0 ) return 0;
	if( cxMutexLock(_this->cs_)!=0 )
		return -1;

	if(_this->end_>=_this->start_ && _this->currLen_!=_this->maxLen_) {
		if(len>=_this->currLen_) {
			memcpy(buff,_this->start_,_this->currLen_);
			xlen = _this->currLen_;

			_this->start_ = _this->pData_;
			_this->end_ = _this->pData_;
			_this->currLen_ = 0;
		}
		else {
			memcpy(buff,_this->start_,len);
			xlen = len;

			_this->start_ += len;
			_this->currLen_ -= len;
		}
	}
	else {
		/*__________________________________________
		// ********<-end       start->*******
		//   elen			slen
		//__________________________________________*/
		int slen = (_this->pData_ + _this->maxLen_) - _this->start_;
		int elen = _this->end_ - _this->pData_;

		if(len>=_this->currLen_) {
			memcpy(buff,_this->start_,slen);
			memcpy(buff+slen,_this->pData_,elen);
			xlen = _this->currLen_;

			_this->start_ = _this->pData_;
			_this->end_ = _this->pData_;
			_this->currLen_ = 0;
		}
		else {
			if(len<slen) {
				memcpy(buff,_this->start_,len);
				xlen = len;

				_this->start_ += len;
				_this->currLen_ -= len;
			}
			else {
				memcpy(buff,_this->start_,slen);
				memcpy(buff+slen,_this->pData_,len-slen);
				xlen = len;

				_this->start_ = ((_this->start_+len)-_this->pData_)%_this->maxLen_+_this->pData_;
				_this->currLen_ -= len;
			}
		}
	}
	if( cxMutexUnlock(_this->cs_)!=0 )
		return -1;

	return xlen;
}

int dxBufferWrite(DxBuffer _this, char* buff, int len)
{
	int xlen = 0;

	if( !_this || !buff || (_this->maxLen_-_this->currLen_)==0 || len<0 ) return -1;
	if( len>(_this->maxLen_-_this->currLen_) ) 
		return -1;
	if( cxMutexLock(_this->cs_)!=0 )
		return -1;


	if(_this->start_>=_this->end_ && _this->currLen_!=0) {
		if(len==(_this->maxLen_-_this->currLen_)) {
			memcpy(_this->end_,buff,(_this->maxLen_-_this->currLen_));
			xlen = (_this->maxLen_-_this->currLen_);

			_this->end_ += xlen;
			_this->currLen_ += xlen;
		}
		else {
			memcpy(_this->end_,buff,len);
			xlen = len;

			_this->end_ += len;
			_this->currLen_ += len;
		}
	}
	else {
		/*__________________________________________
		// ++++++++start->********<-end+++++++++++++
		//   slen			 elen
		//__________________________________________*/
		int slen = _this->start_ - _this->pData_;
		int elen = (_this->pData_ + _this->maxLen_) - _this->end_;

		if(len==(_this->maxLen_-_this->currLen_)) {
			memcpy(_this->end_,buff,elen);
			memcpy(_this->pData_,buff+elen,slen);
			xlen = (_this->maxLen_-_this->currLen_);

			_this->end_ = _this->start_;
			_this->currLen_ = _this->maxLen_;
		}
		else {
			if(len<=elen) {
				memcpy(_this->end_,buff,len);
				xlen = len;

				_this->end_ += len;
				_this->currLen_ += len;
			}
			else {
				memcpy(_this->end_,buff,elen);
				memcpy(_this->pData_,buff+elen,(len-elen));
				xlen = len;

				_this->end_ = _this->pData_ + (len-elen);
				_this->currLen_ += len;
			}
		}
	}
	if( cxMutexUnlock(_this->cs_)!=0 )
		return -1;

	return xlen;
}

int		dxBufferGetCurrLen(DxBuffer _this)
{
	return (_this)?_this->currLen_:0;
}

int		dxBufferGetMaxLen(DxBuffer _this)
{
	return (_this)?_this->maxLen_:0;
}

/*========================================================================
// set this DxBuffer's trace flag ON or OFF*/
void dxBufferSetTraceFlag(DxBuffer _this, TraceFlag traceflag)
{
	if ( _this==NULL ) 
		return;
	_this->traceFlag_ = traceflag;
}


