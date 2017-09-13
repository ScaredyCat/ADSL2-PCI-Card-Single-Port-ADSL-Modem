/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * cx_event.c
 *
 * $Id: cx_event.c,v 1.13 2004/09/23 04:41:40 tyhuang Exp $
 */

#include <string.h>
#include <stdlib.h>
#ifdef UNIX
#include <sys/time.h>
#include <sys/select.h>
#endif
#include "cx_event.h"
#include <common/cm_trace.h>

typedef struct {
	int		valid; /* 0=invalid; 1=valid */
	int		event;
	CxEventCB	cb;
	CxSock		sock;
	void*		context;
} EventNode;

struct cxEventObj
{
	EventNode	nodes[FD_SETSIZE];
	int		maxi;
	SOCKET		maxfd;
	fd_set		selectRd;
	fd_set		selectWr;
	fd_set		selectEx;
};

CxEvent cxEventNew(void)
{
	CxEvent _this;
	int i;

	_this = (CxEvent)malloc(sizeof *_this);
	if (_this == NULL) return NULL;

	for (i=0; i<FD_SETSIZE; i++) {
		_this->nodes[i].valid = 0;
		_this->nodes[i].event = CX_EVENT_UNKNOWN;
		_this->nodes[i].cb = NULL;
		_this->nodes[i].sock = NULL;
		_this->nodes[i].context = NULL;
	}

	_this->maxi = -1;
	_this->maxfd = INVALID_SOCKET;

	FD_ZERO(&_this->selectRd);
	FD_ZERO(&_this->selectWr);
	FD_ZERO(&_this->selectEx);

	return _this;
}

RCODE cxEventFree(CxEvent _this)
{
	if (_this!=NULL) free(_this);

	return RC_OK;
}

RCODE cxEventRegister(CxEvent _this, CxSock sock, CxEventType event, CxEventCB cb, void* context)
{
	SOCKET fd;
	int i, j;

	if (_this == NULL || sock == NULL || !event || !cb)
		return RC_ERROR;

	fd = cxSockGetSock(sock);
	j = _this->maxi + 1;

	for (i=0; i<=_this->maxi; i++)
		if (_this->nodes[i].valid)
			if (cxSockGetSock(_this->nodes[i].sock) == fd) {
				/* re-register */
				j = i;
				break;
			} else
				continue;
		else
			if (i < j) j = i;

	if (j >= FD_SETSIZE) 
		return RC_ERROR;
	
	/* registration */
	if (event & CX_EVENT_RD) {
		FD_SET(fd, &_this->selectRd);
		if ((_this->maxfd == INVALID_SOCKET) || (fd > _this->maxfd)) _this->maxfd = fd;
	} else
		FD_CLR(fd, &_this->selectRd);

	if (event & CX_EVENT_WR) {
		FD_SET(fd, &_this->selectWr);
 		if ((_this->maxfd == INVALID_SOCKET) || (fd > _this->maxfd)) _this->maxfd = fd;
	} else
		FD_CLR(fd, &_this->selectWr);

	if (event & CX_EVENT_EX) {
		FD_SET(fd, &_this->selectEx);
		if ((_this->maxfd == INVALID_SOCKET) || (fd > _this->maxfd)) _this->maxfd = fd;
	} else
		FD_CLR(fd, &_this->selectEx);

	if (j > _this->maxi) _this->maxi = j;
	_this->nodes[j].valid=1;
	_this->nodes[j].event = event;
	_this->nodes[j].cb = cb;
	_this->nodes[j].sock = sock;
	_this->nodes[j].context = context;
    
	return RC_OK;
}

RCODE cxEventUnregister(CxEvent _this, CxSock sock, void** context)
{
	SOCKET fd, sockfd, maxfd;
	int i, index, maxi;
	int found=0;

	if (_this==NULL)
		return RC_ERROR;

	fd = cxSockGetSock(sock);
	maxi = -1;
	maxfd = INVALID_SOCKET;

	for (i=0; i<=_this->maxi; i++)
		if (_this->nodes[i].valid) {
			sockfd = cxSockGetSock(_this->nodes[i].sock);
			index = i;
			if (sockfd == fd) {
				_this->nodes[i].valid = 0;
				_this->nodes[i].event = CX_EVENT_UNKNOWN;
				_this->nodes[i].cb = NULL;
				_this->nodes[i].sock = NULL;
				/* free here ? */
				/*if (_this->nodes[i].context)
					free(_this->nodes[i].context);*/
				if( context )
					*context = _this->nodes[i].context;
				_this->nodes[i].context = NULL;

				FD_CLR(fd, &_this->selectRd);
				FD_CLR(fd, &_this->selectWr);
				FD_CLR(fd, &_this->selectEx);

				sockfd = INVALID_SOCKET;
				index = -1;
				found = 1;
			}
			if (index > maxi) 
				maxi = index;
			if ((maxfd == INVALID_SOCKET) || 
				((sockfd != INVALID_SOCKET) && (sockfd > maxfd)))
				maxfd = sockfd;
		}

	if (!found) 
		return RC_ERROR;

	_this->maxi = maxi;
	_this->maxfd = maxfd;

	return RC_OK;
}

int cxEventDispatch(CxEvent _this, int timeout)
{
	struct timeval tv;
	int numEvents=0;
	SOCKET fd;
	int event;
	int i;
	fd_set rset, wset, xset;
	int retval;

	if (!_this)
		return -1;

	rset = _this->selectRd;
	wset = _this->selectWr;
	xset = _this->selectEx;

	if ( timeout == -1)
		numEvents = select(_this->maxfd+1, &rset, &wset, &xset, NULL);
	else {
		tv.tv_sec = timeout/1000;
		tv.tv_usec = (timeout%1000)*1000;    
		numEvents = select(_this->maxfd+1, &rset, &wset, &xset, &tv);
	}
	if (numEvents == SOCKET_ERROR)
		retval  = -1;
	else
		retval = numEvents;

	/* invoke callbacks if event occurs*/
	event = CX_EVENT_UNKNOWN;
	if ( numEvents > 0 ) {
		for (i=0; i<=_this->maxi; i++) {
			if (_this->nodes[i].valid) {
				fd = cxSockGetSock(_this->nodes[i].sock);
				if (FD_ISSET(fd, &rset) || FD_ISSET(fd, &wset) || FD_ISSET(fd, &xset)) {
					if ((_this->nodes[i].event & CX_EVENT_RD) && FD_ISSET(fd, &rset) )
						event |= CX_EVENT_RD;
 
					if ( (_this->nodes[i].event & CX_EVENT_WR) && FD_ISSET(fd, &wset) )
						event |= CX_EVENT_WR;

					if ((_this->nodes[i].event & CX_EVENT_EX) && FD_ISSET(fd, &xset) )
						event |= CX_EVENT_EX;
					/* type conversion from int to CxEventType ? */
					_this->nodes[i].cb(_this->nodes[i].sock, (CxEventType)event, 0, _this->nodes[i].context);

					numEvents--;
					if (numEvents == 0) break;
				}
			}
		}
	}

	return retval;
}



