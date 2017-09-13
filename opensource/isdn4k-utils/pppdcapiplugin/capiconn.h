/*
 * $Id: capiconn.h,v 1.3 2001/01/25 14:45:41 calle Exp $
 *
 * Copyright 2000 Carsten Paeth (calle@calle.in-berlin.de)
 * Copyright 2000 AVM GmbH Berlin (info@avm.de)
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 * $Log: capiconn.h,v $
 * Revision 1.3  2001/01/25 14:45:41  calle
 * - listen always (for info messages)
 * - show versions on startup
 * - wait for capifs if needed
 *
 * Revision 1.2  2000/10/25 10:01:47  calle
 * (c) in all files
 *
 * Revision 1.1  2000/05/18 14:58:35  calle
 * Plugin for pppd to support PPP over CAPI2.0.
 *
 */

#ifndef __CAPICONN_H__
#define __CAPICONN_H__

#include <stdarg.h>
#include <capi20.h>

/*
 * CAPI_MESSAGES:
 *	capiconn_inject():
 *		inject capi message into state machine
 *
 * capiconn_context:
 *	capiconn_getcontext():
 *		get a context and supply callback functions
 * 	capiconn_freecontext():
 *		free a context allocated with "capiconn_getcontext"
 * 	capiconn_addcontr():
 *		add a controller to the context	
 * capi_connection:
 * 	capiconn_connect():
 *		connection setup, return a capi_connection.
 * 	capiconn_accept():
 *		accept an incoming connection
 * 	capiconn_ignore():
 *		ignore an incoming connection
 * 	capiconn_reject():
 *		reject an incoming connection
 *	capiconn_send():
 *		send data to the connection
 *	capiconn_disconnect():
 *		disconnect a connection
 *	capiconn_getinfo()
 *		get infos about the connection
 *	capiconn_listen():
 *		setup listen
 */

/* -------- returncodes -------------------------------------------------- */

#define CAPICONN_OK			0
#define CAPICONN_NO_CONTROLLER		-1
#define	CAPICONN_NO_MEMORY		-2
#define CAPICONN_WRONG_STATE		1
#define CAPICONN_NOT_SENT		2
#define CAPICONN_ALREADY_DISCONNECTING	3

/* -------- states for CAPI2.0 machine ----------------------------------- */

/*
 * LISTEN state machine
 */
#define ST_LISTEN_NONE			0	/* L-0 */
#define ST_LISTEN_WAIT_CONF		1	/* L-0.1 */
#define ST_LISTEN_ACTIVE		2	/* L-1 */
#define ST_LISTEN_ACTIVE_WAIT_CONF	3	/* L-1.1 */


#define EV_LISTEN_REQ			1	/* L-0 -> L-0.1
						   L-1 -> L-1.1 */
#define EV_LISTEN_CONF_ERROR		2	/* L-0.1 -> L-0
						   L-1.1 -> L-1 */
#define EV_LISTEN_CONF_EMPTY		3	/* L-0.1 -> L-0
						   L-1.1 -> L-0 */
#define EV_LISTEN_CONF_OK		4	/* L-0.1 -> L-1
						   L-1.1 -> L.1 */

/*
 * per plci state machine
 */
#define ST_PLCI_NONE			0	/* P-0 */
#define ST_PLCI_OUTGOING 		1	/* P-0.1 */
#define ST_PLCI_ALLOCATED		2	/* P-1 */
#define ST_PLCI_ACTIVE			3	/* P-ACT */
#define ST_PLCI_INCOMING		4	/* P-2 */
#define ST_PLCI_FACILITY_IND		5	/* P-3 */
#define ST_PLCI_ACCEPTING		6	/* P-4 */
#define ST_PLCI_DISCONNECTING		7	/* P-5 */
#define ST_PLCI_DISCONNECTED		8	/* P-6 */
#define ST_PLCI_RESUMEING		9	/* P-0.Res */
#define ST_PLCI_RESUME			10	/* P-Res */
#define ST_PLCI_HELD			11	/* P-HELD */

#define EV_PLCI_CONNECT_REQ		1	/* P-0 -> P-0.1
                                                 */
#define EV_PLCI_CONNECT_CONF_ERROR	2	/* P-0.1 -> P-0
                                                 */
#define EV_PLCI_CONNECT_CONF_OK		3	/* P-0.1 -> P-1
                                                 */
#define EV_PLCI_FACILITY_IND_UP		4	/* P-0 -> P-1
                                                 */
#define EV_PLCI_CONNECT_IND		5	/* P-0 -> P-2
                                                 */
#define EV_PLCI_CONNECT_ACTIVE_IND	6	/* P-1 -> P-ACT
                                                 */
#define EV_PLCI_CONNECT_REJECT		7	/* P-2 -> P-5
						   P-3 -> P-5
						 */
#define EV_PLCI_DISCONNECT_REQ		8	/* P-1 -> P-5
						   P-2 -> P-5
						   P-3 -> P-5
						   P-4 -> P-5
						   P-ACT -> P-5
						   P-Res -> P-5 (*)
						   P-HELD -> P-5 (*)
						   */
#define EV_PLCI_DISCONNECT_IND		9	/* P-1 -> P-6
						   P-2 -> P-6
						   P-3 -> P-6
						   P-4 -> P-6
						   P-5 -> P-6
						   P-ACT -> P-6
						   P-Res -> P-6 (*)
						   P-HELD -> P-6 (*)
						   */
#define EV_PLCI_FACILITY_IND_DOWN	10	/* P-0.1 -> P-5
						   P-1 -> P-5
						   P-ACT -> P-5
						   P-2 -> P-5
						   P-3 -> P-5
						   P-4 -> P-5
						   */
#define EV_PLCI_DISCONNECT_RESP		11	/* P-6 -> P-0
                                                   */
#define EV_PLCI_CONNECT_RESP		12	/* P-6 -> P-0
                                                   */

#define EV_PLCI_RESUME_REQ		13	/* P-0 -> P-0.Res
                                                 */
#define EV_PLCI_RESUME_CONF_OK		14	/* P-0.Res -> P-Res
                                                 */
#define EV_PLCI_RESUME_CONF_ERROR	15	/* P-0.Res -> P-0
                                                 */
#define EV_PLCI_RESUME_IND		16	/* P-Res -> P-ACT
                                                 */
#define EV_PLCI_HOLD_IND		17	/* P-ACT -> P-HELD
                                                 */
#define EV_PLCI_RETRIEVE_IND		18	/* P-HELD -> P-ACT
                                                 */
#define EV_PLCI_SUSPEND_IND		19	/* P-ACT -> P-5
                                                 */
#define EV_PLCI_CD_IND			20	/* P-2 -> P-5
                                                 */

/*
 * per ncci state machine
 */
#define ST_NCCI_PREVIOUS			-1
#define ST_NCCI_NONE				0	/* N-0 */
#define ST_NCCI_OUTGOING			1	/* N-0.1 */
#define ST_NCCI_INCOMING			2	/* N-1 */
#define ST_NCCI_ALLOCATED			3	/* N-2 */
#define ST_NCCI_ACTIVE				4	/* N-ACT */
#define ST_NCCI_RESETING			5	/* N-3 */
#define ST_NCCI_DISCONNECTING			6	/* N-4 */
#define ST_NCCI_DISCONNECTED			7	/* N-5 */

#define EV_NCCI_CONNECT_B3_REQ			1	/* N-0 -> N-0.1 */
#define EV_NCCI_CONNECT_B3_IND			2	/* N-0 -> N.1 */
#define EV_NCCI_CONNECT_B3_CONF_OK		3	/* N-0.1 -> N.2 */
#define EV_NCCI_CONNECT_B3_CONF_ERROR		4	/* N-0.1 -> N.0 */
#define EV_NCCI_CONNECT_B3_REJECT		5	/* N-1 -> N-4 */
#define EV_NCCI_CONNECT_B3_RESP			6	/* N-1 -> N-2 */
#define EV_NCCI_CONNECT_B3_ACTIVE_IND		7	/* N-2 -> N-ACT */
#define EV_NCCI_RESET_B3_REQ			8	/* N-ACT -> N-3 */
#define EV_NCCI_RESET_B3_IND			9	/* N-3 -> N-ACT */
#define EV_NCCI_DISCONNECT_B3_IND		10	/* N-4 -> N.5 */
#define EV_NCCI_DISCONNECT_B3_CONF_ERROR	11	/* N-4 -> previous */
#define EV_NCCI_DISCONNECT_B3_REQ		12	/* N-1 -> N-4
							   N-2 -> N-4
							   N-3 -> N-4
							   N-ACT -> N-4 */
#define EV_NCCI_DISCONNECT_B3_RESP		13	/* N-5 -> N-0 */

/* ----------------------------------------------------------------------- */

char *capiconn_version(void);

/* -------- context ------------------------------------------------------ */

struct capi_connection; typedef struct capi_connection capi_connection;
struct capiconn_context; typedef struct capiconn_context capiconn_context;

struct capiconn_callbacks
{
	/* ---------- memory functions ----------------- */

	void *(*malloc)(size_t size);
	void (*free)(void *buf);

	/* ---------- connection callbacks ------------- */

	/*
	 * the capi_connection will be destoried after
	 * calling this function
	 */
	void (*disconnected)(capi_connection *,
				int localdisconnect,
				unsigned reason,
				unsigned reason_b3);
	/*
	 * The application should call capiconn_accept()
	 * capiconn_ignore() or capiconn_reject() inside
	 * of the function. If not called an ALERT_REQ will
	 * be sent, to let the application time to decide ...
	 */
	void (*incoming)(capi_connection *,
			  unsigned contr,
			  unsigned chipvalue,
			  char *callednumber,
			  char *callingnumber);

	/*
	 * Channel is ready to send and receive data
	 */
	void (*connected)(capi_connection *, _cstruct);

	/*
	 * Data received on channel, the data pointer
	 * can not be used, after this function is called.
	 */
	void (*received)(capi_connection *,
			    unsigned char *data,
			    unsigned datalen);

	/*
	 * called for every call to capiconn_send().
	 */
	void (*datasent)(capi_connection *, unsigned char *);

	/*
	 * charge info received
	 */
	void (*chargeinfo)(capi_connection *,
			   unsigned long charge,
			   int inunits);
	/*
	 * capi functions
	 */
	void (*capi_put_message) (unsigned appid, unsigned char *msg);

	/*
	 * message functions
	 */
	void (*debugmsg)(const char *fmt, ...);
	void (*infomsg)(const char *fmt, ...);
	void (*errmsg)(const char *fmt, ...);
};
typedef struct capiconn_callbacks capiconn_callbacks;

/* -------- context functions -------------------------------------------- */

capiconn_context *capiconn_getcontext(unsigned appid, capiconn_callbacks *env);

int capiconn_freecontext(capiconn_context *ctx);

/* -------- inject capi message into state machine ----------------------- */

void capiconn_inject(unsigned appid, unsigned char *msg);

/* -------- add controller to context ------------------------------------ */

struct capi_contrinfo {
	int bchannels;
	char *ddi;
	int ndigits;	/* Durchwahllaenge */
};
typedef struct capi_contrinfo capi_contrinfo;

/*
 * returncodes:
 *	CAPICONN_OK		- controller added to the context
 * 	CAPICONN_NO_MEMORY	- callback "malloc" returns no memory.
 */
int capiconn_addcontr(capiconn_context *ctx,
			unsigned contr, capi_contrinfo *cinfo);

/* -------- initiate a connection & disconnect a connection -------------- */

/*
 * returncodes:
 *	a capi connection or 0 if
 * 	- controller not found
 * 	- memory problems
 */

capi_connection *
capiconn_connect(
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
	_cstruct ncpi);

/*
 * returncodes:
 *	CAPICONN_OK			- disconnect initiated
 * 	CAPICONN_ALREADY_DISCONNECTING	- disconnect already initiated
 * 	CAPICONN_WRONG_STATE		- should not happen
 */
int capiconn_disconnect(capi_connection *connection, _cstruct ncpi);

/* -------- reaction on incoming calls ----------------------------------- */

/*
 * returncodes:
 *	CAPICONN_OK		- Accept initiated
 *	CAPICONN_WRONG_STATE	- "conn" ist not in state "incoming"
 * 	CAPICONN_NO_MEMORY	- callback "malloc" returns no memory.
 */
int capiconn_accept(
	capi_connection *conn,
	_cword b1proto,
	_cword b2proto,
	_cword b3proto,
	_cstruct b1config,
	_cstruct b2config,
	_cstruct b3config,
	_cstruct ncpi);

/*
 * returncodes:
 *	CAPICONN_OK		- call will be ignored
 *	CAPICONN_WRONG_STATE	- "conn" ist not in state "incoming"
 */
int capiconn_ignore(capi_connection *conn);

/*
 * returncodes:
 *	CAPICONN_OK		- call will be rejected
 *	CAPICONN_WRONG_STATE	- "conn" ist not in state "incoming"
 */
int capiconn_reject(capi_connection *conn);

/*
 * returncode:
 *	CAPICONN_OK		- Data sent to CAPI
 *	CAPICONN_NOT_SENT	- Data not sent (8 messages not acked)
 *	CAPICONN_WRONG_STATE	- Connection is not connected
 */
int capiconn_send(capi_connection *plcip,
		  unsigned char *data,
		  unsigned len);

/*
 * get info about connection
 */

struct capi_conninfo {
	unsigned	appid;

	unsigned	plci;
	int		plci_state;
	unsigned	ncci;
	int		ncci_state;
	unsigned	isincoming:1,
			disconnect_was_local;
	unsigned	disconnectreason;
	unsigned	disconnectreason_b3;

	/* user supplied */
	_cword		cipvalue;
	_cstruct	callednumber;
	_cstruct	callingnumber;
	_cword		b1proto;
	_cword		b2proto;
	_cword		b3proto;
	_cstruct	b1config;
	_cstruct	b2config;
	_cstruct	b3config;
	_cstruct	bchaninfo;
	_cstruct	ncpi;
};
typedef struct capi_conninfo capi_conninfo;

/*
 * returncode:
 *     always a pointer to the conninfo.
 * the conninfo will not be update automaticly,
 * you always have to call this function to get
 * the actual informations.
 */
capi_conninfo *capiconn_getinfo(capi_connection *p);

/*
 * returncodes:
 *	CAPICONN_OK		- Listen request sent
 * 	CAPICONN_NO_CONTROLLER	- Controller "contr" not added with
 *				  capiconn_addcontr()
 */
int capiconn_listen(capiconn_context *ctx,
		unsigned contr, unsigned cipmask, unsigned cipmask2);

/*
 * returncodes:
 *	CAPICONN_OK		- got ack for listen request 
 * 	CAPICONN_NO_CONTROLLER	- Controller "contr" not added with
 *				  capiconn_addcontr()
 * 	CAPICONN_WRONG_STATE	- got no ack for listen request
 */
int capiconn_listenstate(capiconn_context *ctx, unsigned contr);

#endif /* __CAPICONN_H__ */
