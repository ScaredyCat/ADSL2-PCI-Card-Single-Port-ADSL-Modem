/* $Id: capi.c,v 1.2 1998/10/23 12:50:46 fritz Exp $
 *
 * Implementation of CAPI state machine
 *
 * Based heavily on
 *  CAPI.C Version 1.1 by AVM
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
 * $Log: capi.c,v $
 * Revision 1.2  1998/10/23 12:50:46  fritz
 * Added RCS keywords and GPL notice.
 *
 */
#include <stdio.h>
#include <sys/time.h>
#include <linux/capi.h>
#include <capi20.h>

#include "connect.h"
#include "data.h"
#include "init.h"
#include "capi.h"
#include "id.h"

#define zNCPI		(_cstruct)NULL

static _cmsg		CMESSAGE;
static _cmsg      	*CMSG = &CMESSAGE; /* used in all requests and responses */

/*
 *  SetState: Set the state internal and informs the user
 */
static void ChangeState (ConnectionID Con, ConnectionState  State) {
	SetState (Con, State);
	/* signal the status change to the user */
	StateChange (Con, State);
}

/*
 * Handle_Indication: CAPI logic for all indications
 */
void Handle_Indication(void) {
	ConnectionID    Connection;

	switch (CMSG->Command) {
		case CAPI_CONNECT:
			Connection = GetConnectionByPLCI (CONNECT_IND_PLCI(CMSG));
			if (Connection == INVALID_CONNECTION_ID) {
				/* incoming call */
				Connection = AllocConnection();
				if (Connection == INVALID_CONNECTION_ID) {
					/* error no internal resources, reject call */
					CONNECT_RESP(CMSG, Appl_Id, CMSG->Messagenumber,
						     CONNECT_IND_PLCI(CMSG), REJECT,
						     0, 0, 0, NULL, NULL, NULL,
						     NULL, NULL, NULL,
						     NULL, NULL, NULL, NULL);
					return;
				}
				SetConnectionPLCI(Connection, CONNECT_IND_PLCI(CMSG));
			}
			SetCallingPartyNumberStruct (Connection, CONNECT_IND_CALLINGPARTYNUMBER(CMSG));
			SetCalledPartyNumberStruct (Connection, CONNECT_IND_CALLEDPARTYNUMBER(CMSG));
			/* The ALERT_REQuest tells the caller that someone is listening
			 * for incoming calls on the line. A new timeout of 2 minutes is set
			 * Without the ALERT_REQuest a disconnect would be sent after
			 * 4 seconds with the cause "no user responding" on the caller side
			 * (Assumed that no CONNECT_RESPonse is sent in this time)
			 * of the application
			 */
			ALERT_REQ (CMSG, Appl_Id, 0, CONNECT_IND_PLCI(CMSG),
				   NULL, NULL, NULL, NULL);
			/* inform the user application */
			SetState(Connection, D_ConnectPending);
			IncomingCall(Connection, GetCallingPartyNumber (Connection));
			ChangeState(Connection, D_ConnectPending);
			/* signal incoming call to the user */
			return;
		case CAPI_CONNECT_ACTIVE:
			Connection = GetConnectionByPLCI (CONNECT_ACTIVE_IND_PLCI(CMSG));
			CONNECT_ACTIVE_RESP(CMSG, Appl_Id, CMSG->Messagenumber, CONNECT_ACTIVE_IND_PLCI(CMSG));
			ChangeState(Connection, D_Connected);
			if (GetConnectionInitiator (Connection))
				CONNECT_B3_REQ(CMSG, Appl_Id, 0, CONNECT_ACTIVE_IND_PLCI(CMSG), zNCPI);
			return;
		case CAPI_CONNECT_B3:
			Connection = GetConnectionByPLCI(CONNECT_B3_IND_NCCI(CMSG) & 0x0000FFFF);
			SetConnectionNCCI(Connection, CONNECT_B3_IND_NCCI(CMSG));
			CONNECT_B3_RESP(CMSG, Appl_Id, CMSG->Messagenumber, CONNECT_B3_IND_NCCI(CMSG), 0, zNCPI);
			ChangeState(Connection, B_ConnectPending);
			return;
		case CAPI_CONNECT_B3_ACTIVE:
			Connection = GetConnectionByNCCI(CONNECT_B3_ACTIVE_IND_NCCI(CMSG));
			SetConnectionInitiator(Connection, FALSE);
			CONNECT_B3_ACTIVE_RESP(CMSG, Appl_Id, CMSG->Messagenumber, CONNECT_B3_ACTIVE_IND_NCCI(CMSG));
			ChangeState(Connection, Connected);
			return;
		case CAPI_DISCONNECT_B3:
			Connection = GetConnectionByNCCI(DISCONNECT_B3_IND_NCCI(CMSG));
			SetFaxNCPI(Connection, (faxNCPI_t *)DISCONNECT_B3_IND_NCPI(CMSG));
			SetB3Reason(Connection, DISCONNECT_B3_IND_REASON_B3(CMSG));
			SetConnectionNCCI (Connection, INVAL_NCCI);
			DISCONNECT_B3_RESP(CMSG, Appl_Id, CMSG->Messagenumber, DISCONNECT_B3_IND_NCCI(CMSG));
			ChangeState(Connection, D_Connected);
			if (GetConnectionInitiator(Connection))
				DISCONNECT_REQ(CMSG, Appl_Id, 0, GetConnectionPLCI(Connection), NULL, NULL, NULL, NULL);
			return;
		case CAPI_DISCONNECT:
			Connection = GetConnectionByPLCI(DISCONNECT_IND_PLCI(CMSG));
			SetReason(Connection, DISCONNECT_IND_REASON(CMSG));
			DISCONNECT_RESP(CMSG, Appl_Id, CMSG->Messagenumber, DISCONNECT_IND_PLCI(CMSG));
			if (Connection != INVALID_CONNECTION_ID) {
				ChangeState(Connection, Disconnected);
				FreeConnection(Connection);
			}
			return;
		case CAPI_DATA_B3:
			Connection = GetConnectionByNCCI(DATA_B3_IND_NCCI(CMSG));
			if (CMSG->DataLength > 0) {
				int DiscardData = TRUE;

				DataAvailable(Connection,
					      (void *)DATA_B3_IND_DATA(CMSG),
					      DATA_B3_IND_DATALENGTH(CMSG),
					      DATA_B3_IND_DATAHANDLE(CMSG),
					      &DiscardData);
				if (DiscardData)
					/* let CAPI free the data area immediately */
					DATA_B3_RESP(CMSG, Appl_Id, CMSG->Messagenumber,
						     DATA_B3_IND_NCCI(CMSG), DATA_B3_IND_DATAHANDLE(CMSG));
			}
			return;
		case CAPI_INFO:
			INFO_RESP(CMSG, Appl_Id, CMSG->Messagenumber, INFO_IND_PLCI(CMSG));
			return;
		default:
			fprintf(stderr, "Handle_Indication: Unsupported Indication 0x%02x.\n", CMSG->Command);
			return;
	}
}

/*
 * Handle_Confirmation: CAPI logic for all confirmations
 */
static void Handle_Confirmation(void) {
	ConnectionID Connection;

	if (CMSG->Info > 0x00FF) {
		/* Info's with value 0x00xx are only
		 * warnings, the corresponding requests
		 * have been processed
		 */
		fprintf(stderr, "Handle_Confirmation: Info value 0x%x indicates error.\n", CMSG->Info);
		switch (CMSG->Command) {
			case CAPI_CONNECT:
				Connection = CMSG->Messagenumber;
				ChangeState (Connection, D_ConnectPending);
				ChangeState (Connection, Disconnected);
				FreeConnection(Connection);
				break;
			case CAPI_DATA_B3:
				/* return the error value */
				Connection = GetConnectionByNCCI(DATA_B3_CONF_NCCI(CMSG));
				DataConf(Connection, DATA_B3_CONF_DATAHANDLE(CMSG),
					 DATA_B3_CONF_INFO(CMSG));
				break;
			case CAPI_CONNECT_B3:
				/* disconnect line */
				Connection = GetConnectionByPLCI(CONNECT_B3_CONF_NCCI(CMSG) & 0x0000FFFF);
				if (Connection == INVALID_CONNECTION_ID)
					fprintf(stderr, "Handle_Confirmation: invalid PLCI in CONNECT_B3_CONF.\n");
				else
					DISCONNECT_REQ(CMSG, Appl_Id, 0, GetConnectionPLCI(Connection),
						       NULL, NULL, NULL, NULL);
				break;
			case CAPI_DISCONNECT:
				Connection = GetConnectionByPLCI(DISCONNECT_CONF_PLCI(CMSG));
				if (Connection == INVALID_CONNECTION_ID)
					fprintf(stderr, "Handle_Confirmation: invalid PLCI in DISCONNECT_CONF.\n");
				break;
			case CAPI_DISCONNECT_B3:
				Connection = GetConnectionByNCCI(DISCONNECT_B3_CONF_NCCI(CMSG));
				if (Connection == INVALID_CONNECTION_ID)
					fprintf(stderr, "Handle_Confirmation: invalid NCCI in DISCONNECT_B3_CONF.\n");
				break;
			case CAPI_LISTEN:
				fprintf(stderr, "Handle_Confirmation: Info != 0 in LISTEN_CONF.\n");
				break;
			case CAPI_INFO:
				fprintf(stderr, "Handle_Confirmation: Info != 0 in INFO_CONF.\n");
				break;
			case CAPI_ALERT:
				fprintf(stderr, "Handle_Confirmation: Info != 0 in ALERT_CONF.\n");
				break;
		}
	} else {
		/* no error */
		switch (CMSG->Command) {
			case CAPI_CONNECT:
				Connection = CMSG->Messagenumber;
				SetConnectionPLCI(Connection, CONNECT_CONF_PLCI(CMSG));
				SetConnectionInitiator(Connection, TRUE);
				ChangeState(Connection, D_ConnectPending);
				return;
			case CAPI_CONNECT_B3:
				Connection = GetConnectionByPLCI(CONNECT_B3_CONF_NCCI(CMSG) & 0x0000FFFF);
				SetConnectionNCCI(Connection, CONNECT_B3_CONF_NCCI(CMSG));
				ChangeState(Connection, B_ConnectPending);
				return;
			case CAPI_DISCONNECT:
				Connection = GetConnectionByPLCI(DISCONNECT_CONF_PLCI(CMSG));
				if (Connection != INVALID_CONNECTION_ID)
					ChangeState(Connection, D_DisconnectPending);
				return;
			case CAPI_DISCONNECT_B3:
				Connection = GetConnectionByNCCI(DISCONNECT_B3_CONF_NCCI(CMSG));
				SetConnectionInitiator(Connection, TRUE);
				ChangeState(Connection, B_DisconnectPending);
				return;
			case CAPI_DATA_B3:
				Connection = GetConnectionByNCCI(DATA_B3_CONF_NCCI(CMSG));
				DataConf(Connection, DATA_B3_CONF_DATAHANDLE(CMSG),
					 DATA_B3_CONF_INFO(CMSG));
				return;
			case CAPI_LISTEN:
			case CAPI_INFO:
			case CAPI_ALERT:
				return;
			default:
				fprintf(stderr, "Handle_Confirmation: Invalid Command 0x%02x.\n", CMSG->Command);
				return;
		}
	}
}

/*
 * Handle_CAPI_Msg: the main routine, checks for messages and handles them
 */
void Handle_CAPI_Msg(void) {
	MESSAGE_EXCHANGE_ERROR Info;
	struct timeval tv;

	tv.tv_sec = 1;
	tv.tv_usec = 0;
	CAPI20_WaitforMessage(Appl_Id, &tv); /* This does a select() */
	switch (Info = CAPI_GET_CMSG(CMSG, Appl_Id)) {
		case 0x0000:
			/* a message has been read */
			switch (CMSG->Subcommand) {
				case CAPI_CONF:
					Handle_Confirmation();
					break;
				case CAPI_IND:
					Handle_Indication();
					break;
				default:
					/* neither indication nor confirmation ???? */
					fprintf(stderr, "Handle_CAPI_Msg: Unknown subcommand 0x%02x.\n", CMSG->Subcommand);
					return;
			}
			break;
		case 0x1104:
			/* messagequeue is empty */
			return;
		default:
			fprintf(stderr, "Handle_CAPI_Msg: CAPI_GET_CMSG returns Info != 0.\n");
			return;
	}
}
