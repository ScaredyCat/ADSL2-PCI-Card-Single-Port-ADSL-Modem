/*
 * $Id: capiconn.c,v 1.7 2002/05/03 11:57:49 calle Exp $
 *
 * Copyright 2000 Carsten Paeth (calle@calle.in-berlin.de)
 * Copyright 2000 AVM GmbH Berlin (info@avm.de)
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 * $Log: capiconn.c,v $
 * Revision 1.7  2002/05/03 11:57:49  calle
 * Bugfix of Bugfix.
 *
 * Revision 1.6  2002/05/03 11:55:05  calle
 * Bugfix: some PBX send INFO_IND even when callednumber was complete.
 *
 * Revision 1.5  2001/01/30 17:21:46  calle
 * - bugfix and extention in handle_charge_info
 *
 * Revision 1.4  2001/01/25 14:45:41  calle
 * - listen always (for info messages)
 * - show versions on startup
 * - wait for capifs if needed
 *
 * Revision 1.3  2000/10/25 10:01:47  calle
 * (c) in all files
 *
 * Revision 1.2  2000/10/20 17:16:27  calle
 * phone numbers in connection info where wrong on incoming calls.
 *
 * Revision 1.1  2000/05/18 14:58:35  calle
 * Plugin for pppd to support PPP over CAPI2.0.
 *
 */

#include <stdlib.h>
#include <string.h>
#include "capiconn.h"

static char *revision = "$Revision: 1.7 $";

/* xxxxxxxxxxxxxxxxxx */
static _cmsg cmdcmsg;
static _cmsg cmsg;

/* -------- defines -------------------------------------------------- */

#ifndef CAPI_MAXDATAWINDOW
#define	CAPI_MAXDATAWINDOW	8
#endif

/* -------- type definitions ----------------------------------------- */

struct capiconn_context {
	struct capiconn_context *next;

	unsigned appid;
	struct capiconn_callbacks *cb;

	int ncontr;
	struct capi_contr *contr_list;

        /* statistic */
	unsigned long nrecvctlpkt;
	unsigned long nrecvdatapkt;
	unsigned long nsentctlpkt;
	unsigned long nsentdatapkt;
};

struct capi_contr {

	struct capi_contr *next;
	struct capiconn_context *ctx;

	unsigned contrnr;
	struct capi_contrinfo cinfo;
	unsigned ddilen;

	/*
	 * LISTEN state
	 */
	int state;
	_cdword infomask;
	_cdword cipmask;
	_cdword cipmask2;

	/*
	 * ID of capi message sent
	 */
	_cword msgid;

	/*
	 * B-Channels
	 */
	int nbchan;
	struct capi_connection {
		struct capi_connection *next;
		struct capi_contr *contr;
		struct capiconn_context *ctx;

		struct capi_conninfo conninfo;

		unsigned incoming:1,
			 disconnecting:1,
			 localdisconnect:1,
	                 callednumbercomplete:1;

		_cword disconnectreason;
		_cword disconnectreason_b3;

		_cdword plci;
		_cdword ncci;	/* ncci for CONNECT_ACTIVE_IND */
		_cword msgid;	/* to identfy CONNECT_CONF */
		int state;
		struct capi_ncci {
			struct capi_connection *plcip;
			struct capiconn_context *ctx;
			_cdword ncci;
			_cword msgid;	/* to identfy CONNECT_B3_CONF */
			int state;
			int oldstate;
			/* */
			_cword datahandle;
			struct ncci_datahandle_queue {
			    struct ncci_datahandle_queue *next;
			    _cword                         datahandle;
			    unsigned char                *data;
			} *ackqueue;
			int ackqueuelen;
		} *nccip;
	} *connections;
};

typedef struct capi_ncci capi_ncci;
typedef struct capi_contr capi_contr;
typedef struct ncci_datahandle_queue ncci_datahandle_queue;

/* -------- data definitions ----------------------------------------- */

capiconn_context *context_list = 0;

/* -------- version -------------------------------------------------- */

char *capiconn_version(void)
{
	static char retbuf[256];
	char *p;

	if ((p = strchr(revision, ':'))) {
		strncpy(retbuf, p + 1, sizeof(retbuf));
		p = strchr(retbuf, '$');
		*p = 0;
	}
	return retbuf;
}

/* -------- context handling ----------------------------------------- */

capiconn_context *
capiconn_getcontext(unsigned appid, capiconn_callbacks *cb)
{
	capiconn_context *ctx;

	if ((ctx = ((*cb->malloc)(sizeof(capiconn_context)))) == 0)
		return 0;
	memset(ctx, 0, sizeof(capiconn_context));

	ctx->appid = appid;
	ctx->cb = cb;
	ctx->next = context_list;
	context_list = ctx;
	return ctx;
};

static void free_all_cards(capiconn_context *ctx)
{
}

int
capiconn_freecontext(capiconn_context *ctx)
{
	capiconn_context **pp;
	for (pp = &context_list; *pp; pp = &(*pp)->next) {
		if (*pp == ctx) {
			*pp = (*pp)->next;
			free_all_cards(ctx);
			(*ctx->cb->free)(ctx);
			return 0;
		}
	}
	return -1;
}

static inline capiconn_context *find_context(unsigned appid)
{
	capiconn_context *p;
	for (p = context_list; p; p = p->next)
		if (p->appid == appid)
			return p;
	return 0;
}

int
capiconn_addcontr(capiconn_context *ctx, unsigned contr, capi_contrinfo *cinfo)
{
	capiconn_callbacks *cb = ctx->cb;
	capi_contr *card;

	if (!(card = (capi_contr *) (*cb->malloc)(sizeof(capi_contr))))
		return CAPICONN_NO_MEMORY;
	memset(card, 0, sizeof(capi_contr));
	card->contrnr = contr;
	card->cinfo = *cinfo;
	card->ctx = ctx;
	if (card->cinfo.ddi)
		card->ddilen = strlen(card->cinfo.ddi);
	card->next = ctx->contr_list;
	ctx->contr_list = card;
	ctx->ncontr++;
	return CAPICONN_OK;
}

/* ------------------------------------------------------------------- */

static capi_contr *findcontrbynumber(capiconn_context *ctx, unsigned contr)
{
	capi_contr *p;

	for (p = ctx->contr_list; p; p = p->next)
		if (p->contrnr == contr)
			break;
	return p;
}

/* -------- plci management ------------------------------------------ */

static capi_connection *new_plci(capi_contr * card, int incoming)
{
	capiconn_context *ctx = card->ctx;
	capiconn_callbacks *cb = ctx->cb;
	capi_connection *plcip;

	plcip = (capi_connection *) (*cb->malloc)(sizeof(capi_connection));

	if (plcip == 0)
		return 0;

	memset(plcip, 0, sizeof(capi_connection));
	plcip->contr = card;
	plcip->ctx = ctx;
	plcip->incoming = incoming;
	plcip->state = ST_PLCI_NONE;
	plcip->plci = 0;
	plcip->msgid = 0;
	plcip->next = card->connections;
	card->connections = plcip;

	return plcip;
}

static capi_connection *find_plci_by_plci(capi_contr * card, _cdword plci)
{
	capi_connection *p;
	for (p = card->connections; p; p = p->next)
		if (p->plci == plci)
			return p;
	return 0;
}

static capi_connection *find_plci_by_msgid(capi_contr * card, _cword msgid)
{
	capi_connection *p;
	for (p = card->connections; p; p = p->next)
		if (p->msgid == msgid)
			return p;
	return 0;
}

static capi_connection *find_plci_by_ncci(capi_contr * card, _cdword ncci)
{
	capi_connection *p;
	for (p = card->connections; p; p = p->next)
		if (p->plci == (ncci & 0xffff))
			return p;
	return 0;
}

static void free_plci(capi_contr * card, capi_connection * plcip)
{
	capiconn_context *ctx = card->ctx;
	capiconn_callbacks *cb = ctx->cb;
	capi_connection **pp;

	for (pp = &card->connections; *pp; pp = &(*pp)->next) {
		if (*pp == plcip) {
			*pp = (*pp)->next;
			(*cb->free)(plcip);
			return;
		}
	}
	(*cb->errmsg)("free_plci %p (0x%x) not found, Huh?",
				plcip, plcip->plci);
}

/* -------- ncci management ------------------------------------------ */

static inline capi_ncci *new_ncci(capi_contr * card,
				     capi_connection * plcip,
				     _cdword ncci)
{
	capiconn_context *ctx = card->ctx;
	capiconn_callbacks *cb = ctx->cb;
	capi_ncci *nccip;

	nccip = (capi_ncci *) (*cb->malloc)(sizeof(capi_ncci));

	if (nccip == 0)
		return 0;

	memset(nccip, 0, sizeof(capi_ncci));
	nccip->ctx = ctx;
	nccip->ncci = ncci;
	nccip->state = ST_NCCI_NONE;
	nccip->plcip = plcip;
	nccip->datahandle = 0;
	plcip->nccip = nccip;
	plcip->ncci = ncci;

	return nccip;
}

static inline capi_ncci *find_ncci(capi_contr * card, _cdword ncci)
{
	capi_connection *plcip;

	if ((plcip = find_plci_by_ncci(card, ncci)) == 0)
		return 0;

	return plcip->nccip;
}

static inline capi_ncci *find_ncci_by_msgid(capi_contr * card,
					       _cdword ncci, _cword msgid)
{
	capi_connection *plcip;

	if ((plcip = find_plci_by_ncci(card, ncci)) == 0)
		return 0;

	return plcip->nccip;
}

static void free_ncci(capi_contr * card, capi_ncci *nccip)
{
	capiconn_callbacks *cb = card->ctx->cb;

	nccip->plcip->nccip = 0;
	(*cb->free)(nccip);
}

/* ------------------------------------------------------------------- */

static void clr_conninfo1(capiconn_context *ctx, capi_conninfo *p)
{
	capiconn_callbacks *cb = ctx->cb;

	if (p->callednumber) {
		(*cb->free)(p->callednumber);
		p->callednumber = 0;
	}
	if (p->callingnumber) {
		(*cb->free)(p->callingnumber);
		p->callingnumber = 0;
	}
}

static void clr_conninfo2(capiconn_context *ctx, capi_conninfo *p)
{
	capiconn_callbacks *cb = ctx->cb;

	if (p->b1config) {
		(*cb->free)(p->b1config);
		p->b1config = 0;
	}
	if (p->b2config) {
		(*cb->free)(p->b2config);
		p->b2config = 0;
	}
	if (p->b3config) {
		(*cb->free)(p->b3config);
		p->b3config = 0;
	}
	if (p->bchaninfo) {
		(*cb->free)(p->bchaninfo);
		p->bchaninfo = 0;
	}
	if (p->ncpi) {
		(*cb->free)(p->ncpi);
		p->ncpi = 0;
	}
}

static void clr_conninfo(capiconn_context *ctx, capi_conninfo *p)
{
	clr_conninfo1(ctx, p);
	clr_conninfo2(ctx, p);
}

static int set_conninfo1a(capiconn_context *ctx,
			capi_conninfo *p,
			_cword cipvalue,
			char *callednumber,
			char *callingnumber)
{
	capiconn_callbacks *cb = ctx->cb;
	_cbyte len;

	p->cipvalue = cipvalue;
	if ((p->callednumber = (*cb->malloc)(128)) == 0)
		goto fail;
	if (callednumber) {
		len = (_cbyte)strlen(callednumber);
		if (callednumber[0] & 0x80) {
			memcpy(p->callednumber+1, callednumber, len);
			p->callednumber[0] = len;
	                p->callednumber[len+1] = 0;
		} else {
			memcpy(p->callednumber+2, callednumber, len);
			p->callednumber[0] = len+1;
			p->callednumber[1] = 0x81;
	                p->callednumber[len+2] = 0;
		}
	} else {
			p->callednumber[0] = 0;
	}
	if ((p->callingnumber = (*cb->malloc)(128)) == 0)
		goto fail;
	if (callingnumber) {
		len = (_cbyte)strlen(callingnumber);
		memcpy(p->callingnumber+3, callingnumber, len);
		p->callingnumber[0] = len+2;
		p->callingnumber[1] = 0x00;
	        p->callingnumber[2] = 0x80;
		p->callingnumber[len+3] = 0;
	} else {
		p->callingnumber[0] = 0;
	}
	return 0;
fail:
	clr_conninfo1(ctx, p);
	return -1;
}

static int set_conninfo1b(capiconn_context *ctx,
			capi_conninfo *p,
			_cword cipvalue,
			_cstruct callednumber,
			_cstruct callingnumber)
{
	capiconn_callbacks *cb = ctx->cb;
	_cbyte len;

	p->cipvalue = cipvalue;

	if ((p->callednumber = (*cb->malloc)(128)) == 0)
		goto fail;
	len = callednumber[0];
	memcpy(p->callednumber, callednumber, len+1);
	p->callednumber[len+1] = 0;

	if ((p->callingnumber = (*cb->malloc)(128)) == 0)
		goto fail;
	len = callingnumber[0];
	memcpy(p->callingnumber, callingnumber, len+1);
	p->callingnumber[len+1] = 0;
	return 0;
fail:
	clr_conninfo1(ctx, p);
	return -1;
}

static void extend_callednumber(capiconn_context *ctx, capi_conninfo *p,
				char *number, _cbyte len)
{
	capiconn_callbacks *cb = ctx->cb;
	_cbyte *curnumber = p->callednumber+2;
	_cbyte clen = p->callednumber[0]-2;

	(*cb->debugmsg)("extend number %*.*s (len=%d)",
			(int)len, (int)len, number, len);

	if (len >= clen && memcmp(curnumber, number, clen) == 0) {
		memcpy(p->callednumber + 2, number, len);
		p->callednumber[0] = 2 + len;
	} else {
		memcpy(p->callednumber + p->callednumber[0], number, len);
		p->callednumber[0] += len;
	}
	p->callednumber[p->callednumber[0]+1] = 0;
	(*cb->debugmsg)("capiconn: extended to %s", p->callednumber+2);
}

static int set_conninfo2(capiconn_context *ctx,
			 capi_conninfo *p,
			 _cword b1proto, _cword b2proto, _cword b3proto,
			_cstruct b1config, _cstruct b2config, _cstruct b3config,
			_cstruct bchaninfo, _cstruct ncpi)
{
	capiconn_callbacks *cb = ctx->cb;

	p->b1proto = b1proto;
	p->b2proto = b2proto;
	p->b3proto = b3proto;
	if (b1config) {
		if ((p->b1config = (*cb->malloc)(b1config[0]+1)) == 0)
			goto fail;
		memcpy(p->b1config, b1config, b1config[0]+1);
	}
	if (b2config) {
		if ((p->b2config = (*cb->malloc)(b2config[0]+1)) == 0)
			goto fail;
		memcpy(p->b2config, b2config, b2config[0]+1);
	}
	if (b3config) {
		if ((p->b3config = (*cb->malloc)(b3config[0]+1)) == 0)
			goto fail;
		memcpy(p->b3config, b3config, b3config[0]+1);
	}
	if (bchaninfo) {
		if ((p->bchaninfo = (*cb->malloc)(bchaninfo[0]+1)) == 0)
			goto fail;
		memcpy(p->bchaninfo, bchaninfo, bchaninfo[0]+1);
	}
	if (ncpi) {
		if ((p->ncpi = (*cb->malloc)(ncpi[0]+1)) == 0)
			goto fail;
		memcpy(p->ncpi, ncpi, ncpi[0]+1);
	}
        return 0;
fail:
	clr_conninfo2(ctx, p);
	return -1;
}

capi_conninfo *capiconn_getinfo(capi_connection *p)
{
	p->conninfo.appid = p->ctx->appid;
	p->conninfo.plci = p->plci;
	p->conninfo.plci_state = p->state;
	p->conninfo.ncci = p->ncci;
	p->conninfo.ncci_state = p->nccip ? p->nccip->state : ST_NCCI_NONE;
	p->conninfo.isincoming = p->incoming ? 1 : 0;
	p->conninfo.disconnect_was_local = p->localdisconnect ? 1 : 0;
	p->conninfo.disconnectreason = p->disconnectreason;
	p->conninfo.disconnectreason_b3 = p->disconnectreason_b3;
	return &p->conninfo;
}

/* ------------------------------------------------------------------- */

static int capi_add_ack(capi_ncci *nccip,
			_cword datahandle,
			unsigned char *data)
{
	capiconn_context *ctx = nccip->ctx;
	capiconn_callbacks *cb = ctx->cb;
	ncci_datahandle_queue *n, **pp;

	if (nccip->ackqueuelen >= CAPI_MAXDATAWINDOW)
		return 0;
	n = (ncci_datahandle_queue *)
		(*cb->malloc)(sizeof(ncci_datahandle_queue));
	if (!n) {
	   (cb->errmsg)("capiconn: cb->malloc ncci_datahandle failed");
	   return -1;
	}
	n->next = 0;
	n->datahandle = datahandle;
	n->data = data;
	for (pp = &nccip->ackqueue; *pp; pp = &(*pp)->next) ;
	*pp = n;
	nccip->ackqueuelen++;
	return 0;
}

static unsigned char *capi_del_ack(capi_ncci *nccip, _cword datahandle)
{
	capiconn_context *ctx = nccip->ctx;
	capiconn_callbacks *cb = ctx->cb;
	ncci_datahandle_queue **pp, *p;
	unsigned char *data;

	for (pp = &nccip->ackqueue; *pp; pp = &(*pp)->next) {
 		if ((*pp)->datahandle == datahandle) {
			p = *pp;
			data = p->data;
			*pp = (*pp)->next;
		        (*cb->free)(p);
			nccip->ackqueuelen--;
			return data;
		}
	}
	return 0;
}

/* -------- convert and send capi message ---------------------------- */

static void send_message(capi_contr * card, _cmsg * cmsg)
{
	capiconn_context *ctx = card->ctx;
	capiconn_callbacks *cb = ctx->cb;

	capi_cmsg2message(cmsg, cmsg->buf);
	(*cb->capi_put_message) (ctx->appid, cmsg->buf);
	ctx->nsentctlpkt++;
}

/* -------- state machine -------------------------------------------- */

struct listenstatechange {
	int actstate;
	int nextstate;
	int event;
};

static struct listenstatechange listentable[] =
{
  {ST_LISTEN_NONE, ST_LISTEN_WAIT_CONF, EV_LISTEN_REQ},
  {ST_LISTEN_ACTIVE, ST_LISTEN_ACTIVE_WAIT_CONF, EV_LISTEN_REQ},
  {ST_LISTEN_WAIT_CONF, ST_LISTEN_NONE, EV_LISTEN_CONF_ERROR},
  {ST_LISTEN_ACTIVE_WAIT_CONF, ST_LISTEN_ACTIVE, EV_LISTEN_CONF_ERROR},
  {ST_LISTEN_WAIT_CONF, ST_LISTEN_NONE, EV_LISTEN_CONF_EMPTY},
  {ST_LISTEN_ACTIVE_WAIT_CONF, ST_LISTEN_NONE, EV_LISTEN_CONF_EMPTY},
  {ST_LISTEN_WAIT_CONF, ST_LISTEN_ACTIVE, EV_LISTEN_CONF_OK},
  {ST_LISTEN_ACTIVE_WAIT_CONF, ST_LISTEN_ACTIVE, EV_LISTEN_CONF_OK},
  { 0, 0, 0 },
};

static void listen_change_state(capi_contr * card, int event)
{
	capiconn_context *ctx = card->ctx;
	capiconn_callbacks *cb = ctx->cb;
	struct listenstatechange *p = listentable;

	while (p->event) {
		if (card->state == p->actstate && p->event == event) {
			(*cb->debugmsg)("controller %d: listen_change_state %d -> %d",
			       card->contrnr, card->state, p->nextstate);
			card->state = p->nextstate;
			return;
		}
		p++;
	}
	(*cb->errmsg)("controller %d: listen_change_state state=%d event=%d ????",
	       card->contrnr, card->state, event);
}

/* ------------------------------------------------------------------ */

static void p0(capi_contr * card, capi_connection * plcip)
{
	capiconn_context *ctx = card->ctx;
	capiconn_callbacks *cb = ctx->cb;
	if (*cb->disconnected)
	   (*cb->disconnected)(plcip,
			       plcip->localdisconnect,
			       plcip->disconnectreason,
			       plcip->disconnectreason_b3);
	clr_conninfo(ctx, &plcip->conninfo);
	free_plci(card, plcip);
}

/* ------------------------------------------------------------------ */

struct plcistatechange {
	int actstate;
	int nextstate;
	int event;
	void (*changefunc) (capi_contr * card, capi_connection * plci);
};

static struct plcistatechange plcitable[] =
{
  /* P-0 */
  {ST_PLCI_NONE, ST_PLCI_OUTGOING, EV_PLCI_CONNECT_REQ, 0},
  {ST_PLCI_NONE, ST_PLCI_ALLOCATED, EV_PLCI_FACILITY_IND_UP, 0},
  {ST_PLCI_NONE, ST_PLCI_INCOMING, EV_PLCI_CONNECT_IND, 0},
  {ST_PLCI_NONE, ST_PLCI_RESUMEING, EV_PLCI_RESUME_REQ, 0},
  /* P-0.1 */
  {ST_PLCI_OUTGOING, ST_PLCI_NONE, EV_PLCI_CONNECT_CONF_ERROR, p0},
  {ST_PLCI_OUTGOING, ST_PLCI_ALLOCATED, EV_PLCI_CONNECT_CONF_OK, 0},
  /* P-1 */
  {ST_PLCI_ALLOCATED, ST_PLCI_ACTIVE, EV_PLCI_CONNECT_ACTIVE_IND, 0},
  {ST_PLCI_ALLOCATED, ST_PLCI_DISCONNECTING, EV_PLCI_DISCONNECT_REQ, 0},
  {ST_PLCI_ALLOCATED, ST_PLCI_DISCONNECTING, EV_PLCI_FACILITY_IND_DOWN, 0},
  {ST_PLCI_ALLOCATED, ST_PLCI_DISCONNECTED, EV_PLCI_DISCONNECT_IND, 0},
  /* P-ACT */
  {ST_PLCI_ACTIVE, ST_PLCI_DISCONNECTING, EV_PLCI_DISCONNECT_REQ, 0},
  {ST_PLCI_ACTIVE, ST_PLCI_DISCONNECTING, EV_PLCI_FACILITY_IND_DOWN, 0},
  {ST_PLCI_ACTIVE, ST_PLCI_DISCONNECTED, EV_PLCI_DISCONNECT_IND, 0},
  {ST_PLCI_ACTIVE, ST_PLCI_HELD, EV_PLCI_HOLD_IND, 0},
  {ST_PLCI_ACTIVE, ST_PLCI_DISCONNECTING, EV_PLCI_SUSPEND_IND, 0},
  /* P-2 */
  {ST_PLCI_INCOMING, ST_PLCI_DISCONNECTING, EV_PLCI_CONNECT_REJECT, 0},
  {ST_PLCI_INCOMING, ST_PLCI_FACILITY_IND, EV_PLCI_FACILITY_IND_UP, 0},
  {ST_PLCI_INCOMING, ST_PLCI_ACCEPTING, EV_PLCI_CONNECT_RESP, 0},
  {ST_PLCI_INCOMING, ST_PLCI_DISCONNECTING, EV_PLCI_DISCONNECT_REQ, 0},
  {ST_PLCI_INCOMING, ST_PLCI_DISCONNECTING, EV_PLCI_FACILITY_IND_DOWN, 0},
  {ST_PLCI_INCOMING, ST_PLCI_DISCONNECTED, EV_PLCI_DISCONNECT_IND, 0},
  {ST_PLCI_INCOMING, ST_PLCI_DISCONNECTING, EV_PLCI_CD_IND, 0},
  /* P-3 */
  {ST_PLCI_FACILITY_IND, ST_PLCI_DISCONNECTING, EV_PLCI_CONNECT_REJECT, 0},
  {ST_PLCI_FACILITY_IND, ST_PLCI_ACCEPTING, EV_PLCI_CONNECT_ACTIVE_IND, 0},
  {ST_PLCI_FACILITY_IND, ST_PLCI_DISCONNECTING, EV_PLCI_DISCONNECT_REQ, 0},
  {ST_PLCI_FACILITY_IND, ST_PLCI_DISCONNECTING, EV_PLCI_FACILITY_IND_DOWN, 0},
  {ST_PLCI_FACILITY_IND, ST_PLCI_DISCONNECTED, EV_PLCI_DISCONNECT_IND, 0},
  /* P-4 */
  {ST_PLCI_ACCEPTING, ST_PLCI_ACTIVE, EV_PLCI_CONNECT_ACTIVE_IND, 0},
  {ST_PLCI_ACCEPTING, ST_PLCI_DISCONNECTING, EV_PLCI_DISCONNECT_REQ, 0},
  {ST_PLCI_ACCEPTING, ST_PLCI_DISCONNECTING, EV_PLCI_FACILITY_IND_DOWN, 0},
  {ST_PLCI_ACCEPTING, ST_PLCI_DISCONNECTED, EV_PLCI_DISCONNECT_IND, 0},
  /* P-5 */
  {ST_PLCI_DISCONNECTING, ST_PLCI_DISCONNECTED, EV_PLCI_DISCONNECT_IND, 0},
  /* P-6 */
  {ST_PLCI_DISCONNECTED, ST_PLCI_NONE, EV_PLCI_DISCONNECT_RESP, p0},
  /* P-0.Res */
  {ST_PLCI_RESUMEING, ST_PLCI_NONE, EV_PLCI_RESUME_CONF_ERROR, p0},
  {ST_PLCI_RESUMEING, ST_PLCI_RESUME, EV_PLCI_RESUME_CONF_OK, 0},
  /* P-RES */
  {ST_PLCI_RESUME, ST_PLCI_ACTIVE, EV_PLCI_RESUME_IND, 0},
  /* P-HELD */
  {ST_PLCI_HELD, ST_PLCI_ACTIVE, EV_PLCI_RETRIEVE_IND, 0},
  { 0, 0, 0, 0 },
};

static void plci_change_state(capi_contr * card, capi_connection * plci, int event)
{
	capiconn_callbacks *cb = card->ctx->cb;
	struct plcistatechange *p = plcitable;
	while (p->event) {
		if (plci->state == p->actstate && p->event == event) {
			(cb->debugmsg)("plci_change_state:0x%x %d -> %d event=%d",
				  plci->plci, plci->state, p->nextstate, event);
			plci->state = p->nextstate;
			if (p->changefunc)
				p->changefunc(card, plci);
			return;
		}
		p++;
	}
	(*cb->errmsg)("plci_change_state:0x%x state=%d event=%d ????",
	       card->contrnr, plci->plci, plci->state, event);
}

/* ------------------------------------------------------------------ */


static void n0(capi_contr * card, capi_ncci * ncci)
{
	capiconn_context *ctx = card->ctx;

	capi_fill_DISCONNECT_REQ(&cmsg,
				 ctx->appid,
				 card->msgid++,
				 ncci->plcip->plci,
				 0,	/* BChannelinformation */
				 0,	/* Keypadfacility */
				 0,	/* Useruserdata */   /* $$$$ */
				 0	/* Facilitydataarray */
	);
	send_message(card, &cmsg);
	plci_change_state(card, ncci->plcip, EV_PLCI_DISCONNECT_REQ);
	free_ncci(card, ncci);
}

/* ------------------------------------------------------------------ */

struct nccistatechange {
	int actstate;
	int nextstate;
	int event;
	void (*changefunc) (capi_contr * card, capi_ncci * ncci);
};

static struct nccistatechange nccitable[] =
{
  /* N-0 */
  {ST_NCCI_NONE, ST_NCCI_OUTGOING, EV_NCCI_CONNECT_B3_REQ, 0},
  {ST_NCCI_NONE, ST_NCCI_INCOMING, EV_NCCI_CONNECT_B3_IND, 0},
  /* N-0.1 */
  {ST_NCCI_OUTGOING, ST_NCCI_ALLOCATED, EV_NCCI_CONNECT_B3_CONF_OK, 0},
  {ST_NCCI_OUTGOING, ST_NCCI_NONE, EV_NCCI_CONNECT_B3_CONF_ERROR, n0},
  /* N-1 */
  {ST_NCCI_INCOMING, ST_NCCI_DISCONNECTING, EV_NCCI_CONNECT_B3_REJECT, 0},
  {ST_NCCI_INCOMING, ST_NCCI_ALLOCATED, EV_NCCI_CONNECT_B3_RESP, 0},
  {ST_NCCI_INCOMING, ST_NCCI_DISCONNECTED, EV_NCCI_DISCONNECT_B3_IND, 0},
  {ST_NCCI_INCOMING, ST_NCCI_DISCONNECTING, EV_NCCI_DISCONNECT_B3_REQ, 0},
  /* N-2 */
  {ST_NCCI_ALLOCATED, ST_NCCI_ACTIVE, EV_NCCI_CONNECT_B3_ACTIVE_IND, 0},
  {ST_NCCI_ALLOCATED, ST_NCCI_DISCONNECTED, EV_NCCI_DISCONNECT_B3_IND, 0},
  {ST_NCCI_ALLOCATED, ST_NCCI_DISCONNECTING, EV_NCCI_DISCONNECT_B3_REQ, 0},
  /* N-ACT */
  {ST_NCCI_ACTIVE, ST_NCCI_ACTIVE, EV_NCCI_RESET_B3_IND, 0},
  {ST_NCCI_ACTIVE, ST_NCCI_RESETING, EV_NCCI_RESET_B3_REQ, 0},
  {ST_NCCI_ACTIVE, ST_NCCI_DISCONNECTED, EV_NCCI_DISCONNECT_B3_IND, 0},
  {ST_NCCI_ACTIVE, ST_NCCI_DISCONNECTING, EV_NCCI_DISCONNECT_B3_REQ, 0},
  /* N-3 */
  {ST_NCCI_RESETING, ST_NCCI_ACTIVE, EV_NCCI_RESET_B3_IND, 0},
  {ST_NCCI_RESETING, ST_NCCI_DISCONNECTED, EV_NCCI_DISCONNECT_B3_IND, 0},
  {ST_NCCI_RESETING, ST_NCCI_DISCONNECTING, EV_NCCI_DISCONNECT_B3_REQ, 0},
  /* N-4 */
  {ST_NCCI_DISCONNECTING, ST_NCCI_DISCONNECTED, EV_NCCI_DISCONNECT_B3_IND, 0},
  {ST_NCCI_DISCONNECTING, ST_NCCI_PREVIOUS, EV_NCCI_DISCONNECT_B3_CONF_ERROR,0},
  /* N-5 */
  {ST_NCCI_DISCONNECTED, ST_NCCI_NONE, EV_NCCI_DISCONNECT_B3_RESP, n0},
  { 0, 0, 0, 0 },
};

static void ncci_change_state(capi_contr * card, capi_ncci * ncci, int event)
{
	capiconn_callbacks *cb = card->ctx->cb;
	struct nccistatechange *p = nccitable;
	while (p->event) {
		if (ncci->state == p->actstate && p->event == event) {
			(*cb->debugmsg)("ncci_change_state:0x%x %d -> %d event=%d",
				  ncci->ncci, ncci->state, p->nextstate, event);
			if (p->nextstate == ST_NCCI_PREVIOUS) {
				ncci->state = ncci->oldstate;
				ncci->oldstate = p->actstate;
			} else {
				ncci->oldstate = p->actstate;
				ncci->state = p->nextstate;
			}
			if (p->changefunc)
				p->changefunc(card, ncci);
			return;
		}
		p++;
	}
	(*cb->errmsg)("ncci_change_state:0x%x state=%d event=%d ????",
	       ncci->ncci, ncci->state, event);
}

/* ------------------------------------------------------------------- */

static void handle_controller(capiconn_context *ctx, _cmsg * cmsg)
{
	capi_contr *card = findcontrbynumber(ctx, cmsg->adr.adrController&0x7f);
	capiconn_callbacks *cb = ctx->cb;

	if (!card) {
		(*cb->errmsg)("capiconn: %s from unknown controller 0x%x",
		       capi_cmd2str(cmsg->Command, cmsg->Subcommand),
		       cmsg->adr.adrController & 0x7f);
		return;
	}
	switch (CAPICMD(cmsg->Command, cmsg->Subcommand)) {

	case CAPI_LISTEN_CONF:	/* Controller */
		(*cb->debugmsg)("contr %d: listenconf Info=0x%04x (%s) infomask=0x%x cipmask=0x%x capimask2=0x%x",
			       card->contrnr, cmsg->Info,
			       capi_info2str(cmsg->Info),
			       card->infomask,
			       card->cipmask,
			       card->cipmask2);
		if (cmsg->Info) {
			listen_change_state(card, EV_LISTEN_CONF_ERROR);
		} else if (card->cipmask == 0) {
			listen_change_state(card, EV_LISTEN_CONF_EMPTY);
		} else {
			listen_change_state(card, EV_LISTEN_CONF_OK);
		}
		break;

	case CAPI_MANUFACTURER_IND:	/* Controller */
#if 0
		if (   cmsg->ManuID == 0x214D5641
		    && cmsg->Class == 0
		    && cmsg->Function == 1) {
		   _cbyte *data = cmsg->ManuData+3;
		   _cword len = cmsg->ManuData[0];
		   _cword layer;
		   int direction;
		   if (len == 255) {
		      len = (cmsg->ManuData[1] | (cmsg->ManuData[2] << 8));
		      data += 2;
		   }
		   len -= 2;
		   layer = ((*(data-1)) << 8) | *(data-2);
		   if (layer & 0x300)
			direction = (layer & 0x200) ? 0 : 1;
		   else direction = (layer & 0x800) ? 0 : 1;
		   if (layer & 0x0C00) {
		   	if ((layer & 0xff) == 0x80) {
		           handle_dtrace_data(card, direction, 1, data, len);
		           break;
		   	}
		   } else if ((layer & 0xff) < 0x80) {
		      handle_dtrace_data(card, direction, 0, data, len);
		      break;
		   }
	           (*cb->infomsg)("%s from controller %d layer 0x%x, ignored",
			capi_cmd2str(cmsg->Command, cmsg->Subcommand),
			cmsg->adr.adrController, layer);
                   break;
		}
#endif
		goto ignored;
	case CAPI_MANUFACTURER_CONF:	/* Controller */
		if (cmsg->ManuID == 0x214D5641) {
		   char *s = 0;
		   switch (cmsg->Class) {
		      case 0: break;
		      case 1: s = "unknown class"; break;
		      case 2: s = "unknown function"; break;
		      default: s = "unkown error"; break;
		   }
		   if (s)
	           (*cb->infomsg)("%s from controller 0x%x function %d: %s",
			capi_cmd2str(cmsg->Command, cmsg->Subcommand),
			cmsg->adr.adrController,
			cmsg->Function, s);
		   break;
		}
		goto ignored;
	case CAPI_FACILITY_IND:	/* Controller/plci/ncci */
		goto ignored;
	case CAPI_FACILITY_CONF:	/* Controller/plci/ncci */
		goto ignored;
	case CAPI_INFO_IND:	/* Controller/plci */
		goto ignored;
	case CAPI_INFO_CONF:	/* Controller/plci */
		goto ignored;

	default:
		(*cb->errmsg)("got %s from controller 0x%x ???",
		       capi_cmd2str(cmsg->Command, cmsg->Subcommand),
		       cmsg->adr.adrController);
	}
	return;

      ignored:
	(*cb->infomsg)("%s from controller 0x%x ignored",
	       capi_cmd2str(cmsg->Command, cmsg->Subcommand),
	       cmsg->adr.adrController);
}

static void check_incoming_complete(capi_connection *plcip)
{
	capi_contr *card = plcip->contr;
	capiconn_context *ctx = card->ctx;
	capiconn_callbacks *cb = ctx->cb;
	capi_contrinfo *cip = &card->cinfo;
	unsigned ddilen = plcip->contr->ddilen;

	if (ddilen) {
		unsigned len = plcip->conninfo.callednumber[0]-2;
		char *num = plcip->conninfo.callednumber+2;
		char *start;
		int ndigits;

		if ((start = strstr(num, cip->ddi)) != 0)
			len = strlen(start);
		ndigits = len - ddilen;
		if (ndigits < cip->ndigits) {
			(*cb->debugmsg)("%d digits missing (%s)",
				cip->ndigits-ndigits, num);
			return;
		}
	}

	if (plcip->callednumbercomplete)
	   return;

	plcip->callednumbercomplete = 1;

	if (*cb->incoming)
		(*cb->incoming)(plcip,
				plcip->contr->contrnr,
				plcip->conninfo.cipvalue,
				plcip->conninfo.callednumber+2,
				plcip->conninfo.callingnumber+3);

	if (plcip->state == ST_PLCI_INCOMING) {
		/* call not accepted, rejected or ignored */
		capi_fill_ALERT_REQ(&cmsg,
			    	ctx->appid,
			    	card->msgid++,
			    	plcip->plci,	/* adr */
			    	0,	/* BChannelinformation */
			    	0,	/* Keypadfacility */
			    	0,	/* Useruserdata */
			    	0	/* Facilitydataarray */
				);
		plcip->msgid = cmsg.Messagenumber;
		send_message(card, &cmsg);
	}
}

static void handle_incoming_call(capi_contr * card, _cmsg * cmsg)
{
	capiconn_context *ctx = card->ctx;
	capiconn_callbacks *cb = ctx->cb;
	capi_connection *plcip;

	if ((plcip = new_plci(card, 1)) == 0) {
		(*cb->errmsg)("incoming call on contr %d: no memory, sorry.", card->contrnr);
		goto ignore;
	}
	plcip->plci = cmsg->adr.adrPLCI;
	if (set_conninfo1b(ctx, &plcip->conninfo,
				cmsg->CIPValue,
				cmsg->CalledPartyNumber,
				cmsg->CallingPartyNumber) < 0) {
		free_plci(card, plcip);
		goto ignore;
	}
	plci_change_state(card, plcip, EV_PLCI_CONNECT_IND);

	(*cb->debugmsg)("incoming call contr=%d cip=%d %s -> %s", 
			card->contrnr,
			cmsg->CIPValue,
			plcip->conninfo.callingnumber + 3,
			plcip->conninfo.callednumber + 2);

	if (cb->incoming == 0)
		goto ignore;

	check_incoming_complete(plcip);

	return;

ignore:
	capi_fill_CONNECT_RESP(&cmdcmsg,
			       ctx->appid,
			       card->msgid++,
			       cmsg->adr.adrPLCI,
			       1,	/* ignore call */
			       0,
			       0,
			       0,
			       0,
			       0,
			       0,
			       0,	/* ConnectedNumber */
			       0,	/* ConnectedSubaddress */
			       0,	/* LLC */
			       0,	/* BChannelinformation */
			       0,	/* Keypadfacility */
			       0,	/* Useruserdata */
			       0	/* Facilitydataarray */
				);
	capi_cmsg2message(&cmdcmsg, cmdcmsg.buf);
	send_message(card, &cmdcmsg);
}

static int handle_charge_info(capi_connection *plcip, _cmsg *cmsg)
{
	capiconn_context *ctx = plcip->ctx;
	capiconn_callbacks *cb = ctx->cb;
	unsigned char *p = cmsg->InfoElement;
	unsigned long charge = 0;

	if ((cmsg->InfoNumber & 0x4000) && p[0] == 4) {
		unsigned char *p = &cmsg->InfoElement[1];
		charge |= ((unsigned)p[0]);
		charge |= ((unsigned)p[1]) << 8;
		charge |= ((unsigned)p[2]) << 16;
		charge |= ((unsigned)p[3]) << 24;
		if (cb->chargeinfo) {
	        	if (cmsg->InfoNumber & 0x1)
				(*cb->chargeinfo)(plcip, charge, 0);
			else (*cb->chargeinfo)(plcip, charge, 1);
		}
		return 1;
	} else if (cmsg->InfoNumber == 0x28) {
		if (p[0] > 10 && memcmp("*AOC2*12*", p+1, 9) == 0) {
			int i, len = p[0]-10;
			if (len > 8) len = 8;
			for (i=0; i < len; i++)
				charge = charge * 10 + (p[10+i] - '0');
			if (cb->chargeinfo)
				(*cb->chargeinfo)(plcip, charge, 0);
			return 1;
		} else if (p[0] > 7 && memcmp("FR.", p+1, 3) == 0) {
			int i, len = p[0]-3;
			for (i=0; p[3+i] != '.' && i < len; i++)
				charge = charge * 10 + (p[3+i] - '0');
			charge = charge * 10;
			if (p[3+i] == '.' && i+1 < len)
				charge += (p[3+i+1] - '0');
			if (cb->chargeinfo)
				(*cb->chargeinfo)(plcip, charge, 0);
			return 1;
		}
	} else if (cmsg->InfoNumber == 0x602) {
		if (p[0] > 1 && p[1] == 0x01) {
			int i, len = p[0]-1;
			for (i=0; i < len; i++)
				charge = charge * 10 + (p[1+i] - '0');
			if (cb->chargeinfo)
				(*cb->chargeinfo)(plcip, charge, 0);
			return 1;
		}
	}
	return 0;
}

static int handle_callednumber_info(capi_connection *plcip, _cmsg *cmsg)
{
	unsigned char *p = cmsg->InfoElement;
	if (cmsg->InfoNumber == 0x0070) {
		extend_callednumber(plcip->ctx, &plcip->conninfo, p+2, p[0]-1);
		check_incoming_complete(plcip);
		return 1;
	}
	return 0;
}

static void handle_plci(capiconn_context *ctx, _cmsg * cmsg)
{
	capi_contr *card = findcontrbynumber(ctx, cmsg->adr.adrController&0x7f);
	capiconn_callbacks *cb = ctx->cb;
	capi_connection *plcip;

	if (!card) {
		(*cb->errmsg)("capiconn: %s from unknown controller 0x%x",
		       capi_cmd2str(cmsg->Command, cmsg->Subcommand),
		       cmsg->adr.adrController & 0x7f);
		return;
	}
	switch (CAPICMD(cmsg->Command, cmsg->Subcommand)) {

	case CAPI_DISCONNECT_IND:	/* plci */
		if (cmsg->Reason) {
			(*cb->debugmsg)("%s reason 0x%x (%s) for plci 0x%x",
			   capi_cmd2str(cmsg->Command, cmsg->Subcommand),
			       cmsg->Reason, capi_info2str(cmsg->Reason), cmsg->adr.adrPLCI);
		}
		if (!(plcip = find_plci_by_plci(card, cmsg->adr.adrPLCI))) {
			capi_cmsg_answer(cmsg);
			send_message(card, cmsg);
			goto notfound;
		}
		plcip->disconnectreason = cmsg->Reason;
		plcip->disconnecting = 1;
		plci_change_state(card, plcip, EV_PLCI_DISCONNECT_IND);
		capi_cmsg_answer(cmsg);
		send_message(card, cmsg);
		plci_change_state(card, plcip, EV_PLCI_DISCONNECT_RESP);
		break;

	case CAPI_DISCONNECT_CONF:	/* plci */
		if (cmsg->Info) {
			(*cb->infomsg)("%s info 0x%x (%s) for plci 0x%x",
			   capi_cmd2str(cmsg->Command, cmsg->Subcommand),
			       cmsg->Info, capi_info2str(cmsg->Info), 
			       cmsg->adr.adrPLCI);
		}
		if (!(plcip = find_plci_by_plci(card, cmsg->adr.adrPLCI)))
			goto notfound;

		plcip->disconnecting = 1;
		break;

	case CAPI_ALERT_CONF:	/* plci */
		if (cmsg->Info) {
			(*cb->infomsg)("%s info 0x%x (%s) for plci 0x%x",
			   capi_cmd2str(cmsg->Command, cmsg->Subcommand),
			       cmsg->Info, capi_info2str(cmsg->Info), 
			       cmsg->adr.adrPLCI);
		}
		break;

	case CAPI_CONNECT_IND:	/* plci */
		handle_incoming_call(card, cmsg);
		break;

	case CAPI_CONNECT_CONF:	/* plci */
		if (cmsg->Info) {
			(*cb->infomsg)("%s info 0x%x (%s) for plci 0x%x",
			   capi_cmd2str(cmsg->Command, cmsg->Subcommand),
			       cmsg->Info, capi_info2str(cmsg->Info), 
			       cmsg->adr.adrPLCI);
		}
		if (!(plcip = find_plci_by_msgid(card, cmsg->Messagenumber)))
			goto notfound;

		plcip->plci = cmsg->adr.adrPLCI;
		if (cmsg->Info) {
			plci_change_state(card, plcip, EV_PLCI_CONNECT_CONF_ERROR);
		} else {
			plci_change_state(card, plcip, EV_PLCI_CONNECT_CONF_OK);
		}
		break;

	case CAPI_CONNECT_ACTIVE_IND:	/* plci */

		if (!(plcip = find_plci_by_plci(card, cmsg->adr.adrPLCI)))
			goto notfound;

		if (plcip->incoming) {
			capi_cmsg_answer(cmsg);
			send_message(card, cmsg);
			plci_change_state(card, plcip, EV_PLCI_CONNECT_ACTIVE_IND);
		} else {
			capi_ncci *nccip;
			capi_cmsg_answer(cmsg);
			send_message(card, cmsg);

			nccip = new_ncci(card, plcip, cmsg->adr.adrPLCI);

			if (!nccip) {
				(*cb->errmsg)("no mem for ncci on contr %d, sorry", card->contrnr);
				break;	/* $$$$ */
			}
			capi_fill_CONNECT_B3_REQ(cmsg,
						 ctx->appid,
						 card->msgid++,
						 plcip->plci,	/* adr */
						 plcip->conninfo.ncpi);
			nccip->msgid = cmsg->Messagenumber;
			send_message(card, cmsg);
			plci_change_state(card, plcip, EV_PLCI_CONNECT_ACTIVE_IND);
			ncci_change_state(card, nccip, EV_NCCI_CONNECT_B3_REQ);
		}
		break;

	case CAPI_INFO_IND:	/* Controller/plci */

		if (!(plcip = find_plci_by_plci(card, cmsg->adr.adrPLCI)))
			goto notfound;

		if (handle_charge_info(plcip, cmsg)) {
			capi_cmsg_answer(cmsg);
			send_message(card, cmsg);
		} else if (handle_callednumber_info(plcip, cmsg)) {
			capi_cmsg_answer(cmsg);
			send_message(card, cmsg);
		} else {
			capi_cmsg_answer(cmsg);
			send_message(card, cmsg);
		}
		break;

	case CAPI_CONNECT_ACTIVE_CONF:		/* plci */
		goto ignored;
	case CAPI_SELECT_B_PROTOCOL_CONF:	/* plci */
		goto ignored;
	case CAPI_FACILITY_IND:	/* Controller/plci/ncci */
		goto ignored;
	case CAPI_FACILITY_CONF:	/* Controller/plci/ncci */
		goto ignored;
	case CAPI_INFO_CONF:	/* Controller/plci */
		goto ignored;

	default:
		(*cb->errmsg)("got %s for plci 0x%x ???",
		       capi_cmd2str(cmsg->Command, cmsg->Subcommand),
		       cmsg->adr.adrPLCI);
	}
	return;

ignored:
	(*cb->infomsg)("%s for plci 0x%x ignored",
	       capi_cmd2str(cmsg->Command, cmsg->Subcommand),
	       cmsg->adr.adrPLCI);
	return;

notfound:
	(*cb->errmsg)("%s: plci 0x%x not found",
	       capi_cmd2str(cmsg->Command, cmsg->Subcommand),
	       cmsg->adr.adrPLCI);
	return;
}

static void handle_ncci(capiconn_context *ctx, _cmsg * cmsg)
{
	capi_contr *card = findcontrbynumber(ctx, cmsg->adr.adrController&0x7f);
	capiconn_callbacks *cb = ctx->cb;
	capi_connection *plcip;
	capi_ncci *nccip;
	unsigned char *data;

	if (!card) {
		(*cb->errmsg)("capidrv: %s from unknown controller 0x%x",
		       capi_cmd2str(cmsg->Command, cmsg->Subcommand),
		       cmsg->adr.adrController & 0x7f);
		return;
	}
	switch (CAPICMD(cmsg->Command, cmsg->Subcommand)) {

	case CAPI_CONNECT_B3_ACTIVE_IND:	/* ncci */
		if (!(nccip = find_ncci(card, cmsg->adr.adrNCCI)))
			goto notfound;

		capi_cmsg_answer(cmsg);
		send_message(card, cmsg);
		ncci_change_state(card, nccip, EV_NCCI_CONNECT_B3_ACTIVE_IND);


		(*cb->debugmsg)("ncci 0x%x up", nccip->ncci);

		(*cb->connected)(nccip->plcip, cmsg->NCPI);
		break;

	case CAPI_CONNECT_B3_ACTIVE_CONF:	/* ncci */
		goto ignored;

	case CAPI_CONNECT_B3_IND:	/* ncci */

		plcip = find_plci_by_ncci(card, cmsg->adr.adrNCCI);
		if (plcip) {
			nccip = new_ncci(card, plcip, cmsg->adr.adrNCCI);
			if (nccip) {
				ncci_change_state(card, nccip, EV_NCCI_CONNECT_B3_IND);
				capi_fill_CONNECT_B3_RESP(cmsg,
							  ctx->appid,
							  card->msgid++,
							  nccip->ncci,	/* adr */
							  0,	/* Reject */
							  0	/* NCPI */
				);
				send_message(card, cmsg);
				ncci_change_state(card, nccip, EV_NCCI_CONNECT_B3_RESP);
				break;
			}
			(*cb->errmsg)("capidrv-%d: no mem for ncci, sorry",							card->contrnr);
		} else {
			(*cb->errmsg)("capidrv-%d: %s: plci for ncci 0x%x not found",
			   card->contrnr,
			   capi_cmd2str(cmsg->Command, cmsg->Subcommand),
			       cmsg->adr.adrNCCI);
		}
		capi_fill_CONNECT_B3_RESP(cmsg,
					  ctx->appid,
					  card->msgid++,
					  cmsg->adr.adrNCCI,
					  2,	/* Reject */
					  plcip->conninfo.ncpi);
		send_message(card, cmsg);
		break;

	case CAPI_CONNECT_B3_CONF:	/* ncci */

		if (!(nccip = find_ncci_by_msgid(card,
						 cmsg->adr.adrNCCI,
						 cmsg->Messagenumber)))
			goto notfound;

		nccip->ncci = cmsg->adr.adrNCCI;
		nccip->plcip->ncci = cmsg->adr.adrNCCI;
		if (cmsg->Info) {
			(*cb->infomsg)("%s info 0x%x (%s) for ncci 0x%x",
			   capi_cmd2str(cmsg->Command, cmsg->Subcommand),
			       cmsg->Info, capi_info2str(cmsg->Info), 
			       cmsg->adr.adrNCCI);
		}

		if (cmsg->Info)
			ncci_change_state(card, nccip, EV_NCCI_CONNECT_B3_CONF_ERROR);
		else
			ncci_change_state(card, nccip, EV_NCCI_CONNECT_B3_CONF_OK);
		break;

	case CAPI_CONNECT_B3_T90_ACTIVE_IND:	/* ncci */
		capi_cmsg_answer(cmsg);
		send_message(card, cmsg);
		break;

	case CAPI_DATA_B3_IND:	/* ncci */
		/* handled in handle_data() */
		goto ignored;

	case CAPI_DATA_B3_CONF:	/* ncci */
		if (!(nccip = find_ncci(card, cmsg->adr.adrNCCI)))
			goto notfound;

		data = capi_del_ack(nccip, cmsg->DataHandle);
		if (data == 0)
			break;
		if (cb->datasent)
			(*cb->datasent)(nccip->plcip, data);
		break;

	case CAPI_DISCONNECT_B3_IND:	/* ncci */
		if (!(nccip = find_ncci(card, cmsg->adr.adrNCCI)))
			goto notfound;

		nccip->plcip->disconnectreason_b3 = cmsg->Reason_B3;
		nccip->plcip->disconnecting = 1;
		ncci_change_state(card, nccip, EV_NCCI_DISCONNECT_B3_IND);
		capi_cmsg_answer(cmsg);
		send_message(card, cmsg);
		ncci_change_state(card, nccip, EV_NCCI_DISCONNECT_B3_RESP);
		break;

	case CAPI_DISCONNECT_B3_CONF:	/* ncci */
		if (!(nccip = find_ncci(card, cmsg->adr.adrNCCI)))
			goto notfound;
		if (cmsg->Info) {
			(*cb->infomsg)("%s info 0x%x (%s) for ncci 0x%x",
			   capi_cmd2str(cmsg->Command, cmsg->Subcommand),
			       cmsg->Info, capi_info2str(cmsg->Info), 
			       cmsg->adr.adrNCCI);
			ncci_change_state(card, nccip, EV_NCCI_DISCONNECT_B3_CONF_ERROR);
		}
		break;

	case CAPI_RESET_B3_IND:	/* ncci */
		if (!(nccip = find_ncci(card, cmsg->adr.adrNCCI)))
			goto notfound;
		ncci_change_state(card, nccip, EV_NCCI_RESET_B3_IND);
		capi_cmsg_answer(cmsg);
		send_message(card, cmsg);
		break;

	case CAPI_RESET_B3_CONF:	/* ncci */
		goto ignored;	/* $$$$ */

	case CAPI_FACILITY_IND:	/* Controller/plci/ncci */
		goto ignored;
	case CAPI_FACILITY_CONF:	/* Controller/plci/ncci */
		goto ignored;

	default:
		(*cb->errmsg)("capidrv-%d: got %s for ncci 0x%x ???",
		       card->contrnr,
		       capi_cmd2str(cmsg->Command, cmsg->Subcommand),
		       cmsg->adr.adrNCCI);
	}
	return;
ignored:
	(*cb->infomsg)("%s for ncci 0x%x ignored",
	       capi_cmd2str(cmsg->Command, cmsg->Subcommand),
	       cmsg->adr.adrNCCI);
	return;
notfound:
	(*cb->errmsg)("%s: ncci 0x%x not found",
	       capi_cmd2str(cmsg->Command, cmsg->Subcommand),
	       cmsg->adr.adrNCCI);
}


static void handle_data(capiconn_context *ctx, _cmsg * cmsg)
{
	capi_contr *card = findcontrbynumber(ctx,cmsg->adr.adrController&0x7f);
	capiconn_callbacks *cb = ctx->cb;
	capi_ncci *nccip;
        unsigned char *data;

	if (!card) {
		(*cb->errmsg)("capiconn: %s from unknown controller 0x%x",
		       capi_cmd2str(cmsg->Command, cmsg->Subcommand),
		       cmsg->adr.adrController & 0x7f);
		return;
	}
	if (!(nccip = find_ncci(card, cmsg->adr.adrNCCI))) {
		(*cb->errmsg)("%s: ncci 0x%x not found",
		       capi_cmd2str(cmsg->Command, cmsg->Subcommand),
		       cmsg->adr.adrNCCI);
		return;
	}
	data = (unsigned char *)cmsg->Data;
	if (cb->received)
		(*cb->received)(nccip->plcip, data, cmsg->DataLength);
	capi_cmsg_answer(cmsg);
	send_message(card, cmsg);
}

static _cmsg s_cmsg;

void capiconn_inject(unsigned applid, unsigned char *msg)
{
	capiconn_context *ctx = find_context(applid);

	if (!ctx)
		return;

	capi_message2cmsg(&s_cmsg, msg);
	if (s_cmsg.Command == CAPI_DATA_B3 && s_cmsg.Subcommand == CAPI_IND) {
		handle_data(ctx, &s_cmsg);
		ctx->nrecvdatapkt++;
		return;
	} 
	if ((s_cmsg.adr.adrController & 0xffffff00) == 0)
		handle_controller(ctx, &s_cmsg);
	else if ((s_cmsg.adr.adrPLCI & 0xffff0000) == 0)
		handle_plci(ctx, &s_cmsg);
	else
		handle_ncci(ctx, &s_cmsg);
	ctx->nrecvctlpkt++;
}

/* ------------------------------------------------------------------- */

capi_connection *capiconn_connect(
	capiconn_context *ctx,
	unsigned contr,
	_cword cipvalue,
	char *callednumber,	/* remote number */
	char *callingnumber,	/* own number */
	_cword b1proto,
	_cword b2proto,
	_cword b3proto,
	_cstruct b1config,
	_cstruct b2config,
	_cstruct b3config,
	_cstruct bchaninfo,
	_cstruct ncpi)
{
        capi_contr *card = findcontrbynumber(ctx, contr);
	capiconn_callbacks *cb = ctx->cb;
	capi_connection *plcip;

	if (!card) {
		(*cb->errmsg)("controller %d not found", contr);
		return 0;
	}

	if ((plcip = new_plci(card, 0)) == 0) {
		(*cb->errmsg)("no mem for plci");
		return 0;
	}

	if (set_conninfo1a(ctx, &plcip->conninfo,
				cipvalue,
				callednumber,
				callingnumber) < 0) {
		clr_conninfo1(ctx, &plcip->conninfo);
		free_plci(card, plcip);
		(*cb->errmsg)("no mem for connection info (1a)");
		return 0;
	}

	if (set_conninfo2(ctx, &plcip->conninfo,
				b1proto, b2proto, b3proto,
				b1config, b2config, b3config,
				bchaninfo, ncpi) < 0) {
		clr_conninfo1(ctx, &plcip->conninfo);
		clr_conninfo2(ctx, &plcip->conninfo);
		free_plci(card, plcip);
		(*cb->errmsg)("no mem for connection info (2)");
		return 0;
	}

	capi_fill_CONNECT_REQ(&cmdcmsg,
			      ctx->appid,
			      card->msgid++,
			      card->contrnr,	/* adr */
			      plcip->conninfo.cipvalue,
			      plcip->conninfo.callednumber,
			      plcip->conninfo.callingnumber,
			      0,	/* CalledPartySubaddress */
			      0,	/* CallingPartySubaddress */
			      plcip->conninfo.b1proto,
			      plcip->conninfo.b2proto,
			      plcip->conninfo.b3proto,
			      plcip->conninfo.b1config,
			      plcip->conninfo.b2config,
			      plcip->conninfo.b3config,
			      0,	/* BC */
			      0,	/* LLC */
			      0,	/* HLC */
			      plcip->conninfo.bchaninfo, /* BChannelinformation */
			      0,	/* Keypadfacility */
			      0,	/* Useruserdata */
			      0		/* Facilitydataarray */
			    );

	plcip->msgid = cmdcmsg.Messagenumber;
	plci_change_state(card, plcip, EV_PLCI_CONNECT_REQ);
	send_message(card, &cmdcmsg);
	return plcip;
}

int capiconn_accept(
	capi_connection *plcip,
	_cword b1proto,
	_cword b2proto,
	_cword b3proto,
	_cstruct b1config,
	_cstruct b2config,
	_cstruct b3config,
	_cstruct ncpi)
{
        capi_contr *card = plcip->contr;
	capiconn_context *ctx = card->ctx;
	capiconn_callbacks *cb = ctx->cb;

	if (plcip->state != ST_PLCI_INCOMING)
		return CAPICONN_WRONG_STATE;

	if (set_conninfo2(ctx, &plcip->conninfo,
				b1proto, b2proto, b3proto,
				b1config, b2config, b3config,
				0, ncpi) < 0) {
		clr_conninfo2(ctx, &plcip->conninfo);
		(*cb->errmsg)("no mem for connection info (2)");
		return CAPICONN_NO_MEMORY;
	}

	(*cb->debugmsg)("accept plci 0x%04x %d,%d,%d",
			plcip->plci,
			(int)plcip->conninfo.b1proto,
			(int)plcip->conninfo.b2proto,
			(int)plcip->conninfo.b3proto);

	capi_fill_CONNECT_RESP(&cmdcmsg,
			       ctx->appid,
			       card->msgid++,
			       plcip->plci,	/* adr */
			       0,	/* Reject */
			       plcip->conninfo.b1proto,
			       plcip->conninfo.b2proto,
			       plcip->conninfo.b3proto,
			       plcip->conninfo.b1config,
			       plcip->conninfo.b2config,
			       plcip->conninfo.b3config,
			       0,	/* ConnectedNumber */
			       0,	/* ConnectedSubaddress */
			       0,	/* LLC */
			       0,	/* BChannelinformation */
			       0,	/* Keypadfacility */
			       0,	/* Useruserdata */
			       0	/* Facilitydataarray */
				);
	capi_cmsg2message(&cmdcmsg, cmdcmsg.buf);
	plci_change_state(card, plcip, EV_PLCI_CONNECT_RESP);
	send_message(card, &cmdcmsg);
	return CAPICONN_OK;
}

int capiconn_ignore(capi_connection *plcip)
{
        capi_contr *card = plcip->contr;
	capiconn_context *ctx = card->ctx;

	if (plcip->state != ST_PLCI_INCOMING)
		return CAPICONN_WRONG_STATE;

	capi_fill_CONNECT_RESP(&cmdcmsg,
			       ctx->appid,
			       card->msgid++,
			       plcip->plci,	/* adr */
			       1,	/* ignore call */
			       0,
			       0,
			       0,
			       0,
			       0,
			       0,
			       0,	/* ConnectedNumber */
			       0,	/* ConnectedSubaddress */
			       0,	/* LLC */
			       0,	/* BChannelinformation */
			       0,	/* Keypadfacility */
			       0,	/* Useruserdata */
			       0	/* Facilitydataarray */
				);
	capi_cmsg2message(&cmdcmsg, cmdcmsg.buf);
	plci_change_state(card, plcip, EV_PLCI_CONNECT_RESP);
	send_message(card, &cmdcmsg);
	return CAPICONN_OK;
}

int capiconn_reject(capi_connection *plcip)
{
        capi_contr *card = plcip->contr;
	capiconn_context *ctx = card->ctx;

	if (plcip->state != ST_PLCI_INCOMING)
		return CAPICONN_WRONG_STATE;

	capi_fill_CONNECT_RESP(&cmdcmsg,
			       ctx->appid,
			       card->msgid++,
			       plcip->plci,	/* adr */
			       2,	/* Reject, normal call clearing */
			       0,
			       0,
			       0,
			       0,
			       0,
			       0,
			       0,	/* ConnectedNumber */
			       0,	/* ConnectedSubaddress */
			       0,	/* LLC */
			       0,	/* BChannelinformation */
			       0,	/* Keypadfacility */
			       0,	/* Useruserdata */
			       0	/* Facilitydataarray */
				);
	capi_cmsg2message(&cmdcmsg, cmdcmsg.buf);
	plci_change_state(card, plcip, EV_PLCI_CONNECT_RESP);
	send_message(card, &cmdcmsg);
	return CAPICONN_OK;
}

int capiconn_disconnect(capi_connection *plcip, _cstruct ncpi)
{
        capi_contr *card = plcip->contr;
	capiconn_context *ctx = card->ctx;

	if (plcip->disconnecting)
		return CAPICONN_ALREADY_DISCONNECTING;

	if (plcip->nccip) {
		plcip->disconnecting = 1;
		plcip->localdisconnect = 1;
		capi_fill_DISCONNECT_B3_REQ(&cmdcmsg,
					    ctx->appid,
					    card->msgid++,
					    plcip->ncci,
					    ncpi);
		ncci_change_state(card, plcip->nccip, EV_NCCI_DISCONNECT_B3_REQ);
		send_message(card, &cmdcmsg);
		return CAPICONN_OK;
	} 
	if (plcip->state == ST_PLCI_INCOMING) {
		plcip->disconnecting = 1;
		plcip->localdisconnect = 1;
		return capiconn_reject(plcip);
	}
	if (plcip->plci) {
		plcip->disconnecting = 1;
		plcip->localdisconnect = 1;
		capi_fill_DISCONNECT_REQ(&cmdcmsg,
					 ctx->appid,
					 card->msgid++,
					 plcip->plci,
					 0,	/* BChannelinformation */
					 0,	/* Keypadfacility */
					 0,	/* Useruserdata */
					 0	/* Facilitydataarray */
					);
		plci_change_state(card, plcip, EV_PLCI_DISCONNECT_REQ);
		send_message(card, &cmdcmsg);
		return CAPICONN_OK;
	}
	return CAPICONN_WRONG_STATE;
}

static _cmsg sendcmsg;

int capiconn_send(capi_connection *plcip,
		  unsigned char *data,
		  unsigned len)
{
	capi_contr *card = plcip->contr;
	capiconn_context *ctx = card->ctx;
	capiconn_callbacks *cb = ctx->cb;

	capi_ncci *nccip;
	_cword datahandle;

	nccip = plcip->nccip;
	if (!nccip || nccip->state != ST_NCCI_ACTIVE)
		return CAPICONN_WRONG_STATE;

	datahandle = nccip->datahandle;
	capi_fill_DATA_B3_REQ(&sendcmsg, ctx->appid, card->msgid++,
			      nccip->ncci,	/* adr */
			      data,		/* Data */
			      len,		/* DataLength */
			      datahandle,	/* DataHandle */
			      0	/* Flags */
	    		);

	if (capi_add_ack(nccip, datahandle, data) < 0)
	   return CAPICONN_NOT_SENT;

	capi_cmsg2message(&sendcmsg, sendcmsg.buf);
	(*cb->capi_put_message) (ctx->appid, sendcmsg.buf);
	nccip->datahandle++;
	ctx->nsentdatapkt++;
	return CAPICONN_OK;
}


/* -------- listen handling ------------------------------------------ */

static void send_listen(capi_contr *card)
{
	capiconn_context *ctx = card->ctx;

	card->infomask = 0;
	card->infomask |= (1<<2); /* Display */
	card->infomask |= (1<<6); /* Charge Info */
	if (card->ddilen) card->infomask |= (1<<7); /* Called Party Number */
	card->infomask |= (1<<8); /* Channel Info */

	capi_fill_LISTEN_REQ(&cmdcmsg, ctx->appid,
			     card->msgid++,
			     card->contrnr,
			     card->infomask,
			     card->cipmask,
			     card->cipmask2,
			     0, 0);
	send_message(card, &cmdcmsg);
	listen_change_state(card, EV_LISTEN_REQ);
}

int
capiconn_listen(capiconn_context *ctx,
		unsigned contr, unsigned cipmask, unsigned cipmask2)
{
	capi_contr *card = findcontrbynumber(ctx, contr & 0x7f);

	if (card == 0)
		return CAPICONN_NO_CONTROLLER;

	card->cipmask = cipmask;	/* 0x1FFF03FF */
	card->cipmask2 = cipmask2;	/* 0 */

	send_listen(card);
	return CAPICONN_OK;
}

int
capiconn_listenstate(capiconn_context *ctx, unsigned contr)
{
	capi_contr *card = findcontrbynumber(ctx, contr & 0x7f);

	if (card == 0)
		return CAPICONN_NO_CONTROLLER;
	if (card->state != ST_LISTEN_NONE && card->state != ST_LISTEN_ACTIVE)
		return CAPICONN_WRONG_STATE;
	return CAPICONN_OK;
}
