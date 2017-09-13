/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * cx_event.c
 *
 * $Id: cx_NU_event.c,v 1.1 2003/02/19 14:15:29 yjliao Exp $
 */

#include <string.h>
#include <stdlib.h>
#include "cx_NU_event.h"

#define NODE_SIZE	64

typedef struct {
	int		valid; /* 0=invalid; 1=valid */
	int		event;
	CxEventCB	cb;
	CxSock		sock;
	void*		context;
} EventNode;

struct cxEventObj
{
	EventNode	nodes[NODE_SIZE];
	int		maxi;
	SOCKET_FD	maxfd;
	FD_SET		selectRd;
	FD_SET		selectWr;
	FD_SET		selectEx;
};

CxEvent cxEventNew(void)
{
	CxEvent _this;
	int i;

	_this = (CxEvent)malloc(sizeof *_this);
	if (_this == NULL) return NULL;

	for (i=0; i<NODE_SIZE; i++) {
		_this->nodes[i].valid = 0;
		_this->nodes[i].event = CX_EVENT_UNKNOWN;
		_this->nodes[i].cb = NULL;
		_this->nodes[i].sock = NULL;
		_this->nodes[i].context = NULL;
	}

	_this->maxi = -1;
	_this->maxfd = INVALID_SOCKET;

	NU_FD_Init(&_this->selectRd);
	NU_FD_Init(&_this->selectWr);
	NU_FD_Init(&_this->selectEx);

	return _this;
}

RCODE cxEventFree(CxEvent _this)
{
	if (_this!=NULL) free(_this);

	return RC_OK;
}

RCODE cxEventRegister(CxEvent _this, CxSock sock, CxEventType event, CxEventCB cb, void* context)
{
	SOCKET_FD fd;
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

	if (j >= NODE_SIZE) 
		return RC_ERROR;
	
	/* registration */
	if (event & CX_EVENT_RD) {
		NU_FD_Set(fd, &_this->selectRd);
		if ((_this->maxfd == INVALID_SOCKET) || (fd > _this->maxfd)) _this->maxfd = fd;
	} else
		NU_FD_Reset(fd, &_this->selectRd);

	if (event & CX_EVENT_WR) {
		NU_FD_Set(fd, &_this->selectWr);
 		if ((_this->maxfd == INVALID_SOCKET) || (fd > _this->maxfd)) _this->maxfd = fd;
	} else
		NU_FD_Reset(fd, &_this->selectWr);

	if (event & CX_EVENT_EX) {
		NU_FD_Set(fd, &_this->selectEx);
		if ((_this->maxfd == INVALID_SOCKET) || (fd > _this->maxfd)) _this->maxfd = fd;
	} else
		NU_FD_Reset(fd, &_this->selectEx);

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
	SOCKET_FD fd, sockfd, maxfd;
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
				if (_this->nodes[i].context)
					free(_this->nodes[i].context);
				if( context )
					*context = _this->nodes[i].context;
				_this->nodes[i].context = NULL;

				NU_FD_Reset(fd, &_this->selectRd);
				NU_FD_Reset(fd, &_this->selectWr);
				NU_FD_Reset(fd, &_this->selectEx);

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
	int numEvents=0;
	SOCKET_FD fd;
	int event;
	int i;
	FD_SET rset, wset, xset;
	int retval;

	rset = _this->selectRd;
	wset = _this->selectWr;
	xset = _this->selectEx;

	if ( timeout == -1)
		numEvents = NU_Select(_this->maxfd+1, &rset, &wset, &xset, NU_SUSPEND);
	else 	numEvents = NU_Select(_this->maxfd+1, &rset, &wset, &xset, timeout);

	if (numEvents != NU_SUCCESS)
		retval  = -1;
	else
		retval = numEvents;

	/* invoke callbacks if event occurs*/
	event = CX_EVENT_UNKNOWN;
	if ( numEvents > 0 ) {
		for (i=0; i<=_this->maxi; i++) {
			if (_this->nodes[i].valid) {
				fd = cxSockGetSock(_this->nodes[i].sock);
				if (NU_FD_Check(fd, &rset) || NU_FD_Check(fd, &wset) || NU_FD_Check(fd, &xset)) {
					if ((_this->nodes[i].event & CX_EVENT_RD) && NU_FD_Check(fd, &rset) )
						event |= CX_EVENT_RD;
 
					if ( (_this->nodes[i].event & CX_EVENT_WR) && NU_FD_Check(fd, &wset) )
						event |= CX_EVENT_WR;

					if ((_this->nodes[i].event & CX_EVENT_EX) && NU_FD_Check(fd, &xset) )
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



