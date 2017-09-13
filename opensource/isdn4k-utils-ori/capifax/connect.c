/* $Id: connect.c,v 1.2 1998/10/23 12:50:49 fritz Exp $
 *
 * Functions concerning activation and deactivation of connections
 * This stuff is based heavily on AVM's CAPI-adk for linux.
 *
 * This program is free software; you can redistribute it and/or modify          * it under the terms of the GNU General Public License as published by          * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *                                                                               * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Log: connect.c,v $
 * Revision 1.2  1998/10/23 12:50:49  fritz
 * Added RCS keywords and GPL notice.
 *
 */
#include <stdio.h>
#include <malloc.h>
#include <assert.h>
#include <sys/time.h>
#include <linux/capi.h>
#include <capi20.h>

#include "contr.h"
#include "init.h"
#include "connect.h"

/* Function Pointers for Callback-Functions */
MainStateChange_t  MainStateChange_p = 0L;
MainIncomingCall_t MainIncomingCall_p = 0L;

/*
 * Listen: send a LISTEN_REQ
 * parameters: CIPmask (which services shall be accepted) (see CAPI 2.0 spec.)
 * Listen will be sent to the number of controllers specified in InitISDN.
 * Listen with CIPmask = 0 results in getting no incoming calls signaled
 * by CAPI.
 */
unsigned Listen(unsigned long CIPmask) {
	MESSAGE_EXCHANGE_ERROR error;
	_cmsg CMSG;
	unsigned Controller;
	unsigned numController;

	/* send listen to all controllers */
	numController = GetNumController();
	for (Controller=1; Controller<=numController; Controller++) {
		LISTEN_REQ_HEADER(&CMSG, Appl_Id, 0, Controller);
		LISTEN_REQ_CIPMASK(&CMSG) = CIPmask;
		if ((error = CAPI_PUT_CMSG(&CMSG)) != 0)
			return error;
	}
	return 0;
}

/*
 * all capi functions can also be called with all possible parameters specified
 * direct in the function, e.g.
 *
 * error = CONNECT_REQ(&CMSG, Appl_Id, 0,
 *		Controller, CIPValue,
 *		CalledPartyNumber, CallingPartyNumber,
 *		CalledPartySubaddress, CallingPartySubaddress,
 *		B1Protokoll, B2Protokoll, B3Protokoll,
 *		B1Configuration, B2Configuration, B3Configuration,
 *		BC, LLC, HLC,
 *		NULL, NULL, NULL, NULL);
 */

/*
 * Connect: try's to connect to 'CalledPartyNumber'
 * the return value of CAPI_PUT_CMSG is the same as CAPI_PUT_MESSAGE
 * (defined in CAPI 2.0 spec. error class 0x11xx )
 * If CallingPartyNumber is not needed, set to NULL.
 * CallingPartyNumber & CalledPartyNumber have to be zero terminated strings.
 * For datatransmission set the protocols to zero, B3Configuration to NULL
 */
unsigned Connect(ConnectionID *Connection, char *CalledPartyNumber, char *CallingPartyNumber,
		 unsigned long Service, unsigned short B1Protocol, unsigned short B2Protocol,
		 unsigned short B3Protocol, unsigned char *B3Configuration) {

	_cmsg CMSG;
	MESSAGE_EXCHANGE_ERROR error;
	long Controller;
	unsigned short CIPValue;

	assert (*Connection == INVALID_CONNECTION_ID);

	Controller = GetFreeController();
	if (Controller == INVAL_CONTROLLER) {
		/* if no available controller use the first one for correct
		 * error signaling
		 */
		Controller = 1;
	}
	*Connection = AllocConnection();
	if (*Connection == INVALID_CONNECTION_ID) {
		fprintf(stderr, "Connect: No available connection identifiers.\n");
		/* no OS resources */
		return 0x1108;
	}
	/* use ConnectionIdentifier as Messagenumber */
	CONNECT_REQ_HEADER(&CMSG, Appl_Id, (unsigned short)*Connection, Controller);
	/* build up service mask */
	CIPValue = 0;
	if (Service != 0) {
		do {
			if (Service & 1)
				break;
			Service >>= 1;
			CIPValue++;
		} while (CIPValue < 31);
	}
	CONNECT_REQ_CIPVALUE(&CMSG) = CIPValue;
	SetCalledPartyNumber (*Connection, CalledPartyNumber);
	SetCallingPartyNumber (*Connection, CallingPartyNumber);
	CONNECT_REQ_CALLEDPARTYNUMBER(&CMSG)  = GetCalledPartyNumberStruct(*Connection);
	CONNECT_REQ_CALLINGPARTYNUMBER(&CMSG) = GetCallingPartyNumberStruct(*Connection);
	CONNECT_REQ_B1PROTOCOL(&CMSG) = B1Protocol;
	CONNECT_REQ_B2PROTOCOL(&CMSG) = B2Protocol;
	CONNECT_REQ_B3PROTOCOL(&CMSG) = B3Protocol;
	CONNECT_REQ_B3CONFIGURATION(&CMSG) = B3Configuration;
	error = CAPI_PUT_CMSG(&CMSG);
	if (error != 0) {
		FreeConnection (*Connection);
		*Connection = INVALID_CONNECTION_ID;
		fprintf(stderr, "Connect: error in CAPI_PUT_MESSAGE\n");
	}
	return error;
}

/*
 * Disconnect: disconnects one channel
 * The ConnectionID must be valid
 */
unsigned Disconnect(ConnectionID Connection) {
	_cmsg CMSG;
	MESSAGE_EXCHANGE_ERROR error;

	assert (Connection != INVALID_CONNECTION_ID);

	switch (GetState(Connection)) {
		case Connected:
		case B_ConnectPending:
			SetConnectionInitiator (Connection, TRUE);
			DISCONNECT_B3_REQ_HEADER(&CMSG, Appl_Id, 0, GetConnectionNCCI (Connection));
			error = CAPI_PUT_CMSG(&CMSG);
			break;
		case D_Connected:
		case D_ConnectPending:
		case B_DisconnectPending:
			SetConnectionInitiator (Connection, TRUE);
			DISCONNECT_REQ_HEADER(&CMSG, Appl_Id, 0, GetConnectionPLCI (Connection));
			error = CAPI_PUT_CMSG(&CMSG);
			break;
		default:
			error = 0;
			break;
	}
	return error;
}

/*
 * IncomingCall: signals an incoming call
 * This function will be executed if a CONNECT_INDication appears to
 * inform the user. This function is implemented in the main program
 */
void IncomingCall(ConnectionID Connection, char *CallingPartyNumber) {
	if (MainIncomingCall_p)
		MainIncomingCall_p(Connection, CallingPartyNumber);
}

/*
 * AnswerCall: answers incoming call with the specified reject-value
 *	       (some reject-values are defined in the req.h file)
 *	       (for more see CAPI 2.0 spec.)
 */
unsigned AnswerCall(ConnectionID Connection, RejectValue Reject, unsigned short B1Protocol,
		    unsigned short B2Protocol, unsigned short B3Protocol, unsigned char *B3Configuration) {

	_cmsg CMSG;

	assert (Connection != INVALID_CONNECTION_ID);

	CONNECT_RESP_HEADER(&CMSG, Appl_Id, 0, GetConnectionPLCI (Connection));
	CONNECT_RESP_REJECT(&CMSG) = (unsigned short)Reject;
	CONNECT_REQ_B1PROTOCOL(&CMSG) = B1Protocol;
	CONNECT_REQ_B2PROTOCOL(&CMSG) = B2Protocol;
	CONNECT_REQ_B3PROTOCOL(&CMSG) = B3Protocol;
	CONNECT_REQ_B3CONFIGURATION(&CMSG) = B3Configuration;
	return CAPI_PUT_CMSG(&CMSG);
}

/*
 * StateChange: signals a state change on both B-channels (connected, disconnected)
 * Whenever a channel changes his state this function is called
 * This function is implemented in the main program
 */
void StateChange(ConnectionID Connection, ConnectionState State) {
	if (MainStateChange_p)
		MainStateChange_p(Connection, State);
}
