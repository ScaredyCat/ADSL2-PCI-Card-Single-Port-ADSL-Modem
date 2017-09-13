/* $Id: id.h,v 1.2 1998/10/23 12:50:56 fritz Exp $
 *
 * Connection-ID management.
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
 * $Log: id.h,v $
 * Revision 1.2  1998/10/23 12:50:56  fritz
 * Added RCS keywords and GPL notice.
 *
 */
#ifndef _id_h_
#define _id_h_

#include <capi20.h>

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

typedef enum _ConnectionState {
	Disconnected,
	D_ConnectPending,
	D_Connected,
	B_ConnectPending,
	Connected,
	B_DisconnectPending,
	D_DisconnectPending
} ConnectionState;

typedef struct {
	_cbyte length;
	_cword rate        __attribute__ ((packed));
	_cword resolution  __attribute__ ((packed));
	_cword format      __attribute__ ((packed));
	_cword pages       __attribute__ ((packed));
	_cbyte idlen       __attribute__ ((packed));
	_cbyte id[50]      __attribute__ ((packed));
} faxNCPI_t;

extern const char *ConnectionStateString[7];

typedef int ConnectionID;

#define maxConnections        8
#define INVALID_CONNECTION_ID (-1)
#define INVAL_STATE           (-1)
#define INVAL_NCCI            (-1)
#define INVAL_PLCI            (-1)
#define INVAL_CONTROLLER      (-1)

/*
 * InitConnectionIDHandling: Initialisation of internal structures. Must be
 * executed before using the functions in this module.
 */
void InitConnectionIDHandling(void);

/*
 * AllocConnection: allocates one connection structure. Returns
 * INVALID_CONNECTION_ID if none free. All members are resetted.
 */
ConnectionID AllocConnection(void);

/*
 * FreeConnection: frees one connection, the ConnectionID is no longer valid
 */
void FreeConnection(ConnectionID Connection);

/*
 * SetConnectionPLCI: sets the PLCI (and controller) for one connection
 */
void SetConnectionPLCI(ConnectionID Con, long PLCI);

/*
 * GetConnectionPLCI: returns the PLCI for one connection
 */
long GetConnectionPLCI(ConnectionID Con);

/*
 * GetConnectionByPLCI: Searches the connection that uses the PLCI, if none
 * found, returns INVALID_CONNECTION_ID.
 */
ConnectionID GetConnectionByPLCI(long PLCI);

/*
 * SetConnectionNCCI: Sets the NCCI for one connection.
 */
void SetConnectionNCCI(ConnectionID Con, long NCCI);

/*
 * GetConnectionNCCI: Returns the NCCI for one connection.
 */
long GetConnectionNCCI(ConnectionID Con);

/*
 * GetConnectionByNCCI: Searches the connection that uses the NCCI, if none
 * found, returns INVALID_CONNECTION_ID.
 */
ConnectionID GetConnectionByNCCI(long NCCI);

/*
 * GetController: Returns the controller that is used by the specified
 * connection.
 */
long GetController(ConnectionID Con);

/*
 * GetNumberOfConnections: Returns the number of currently active connections
 * on the specified controller.
 */
unsigned GetNumberOfConnections(long Controller);

/*
 * GetState: Returns the state of the specified connection.
 */
ConnectionState GetState(ConnectionID Connection);

/*
 * SetState: Sets the state of the specified connection.
 */
void SetState(ConnectionID Connection, ConnectionState State);

/*
 * SetConnectionInitiator: Changes the Initiator flag to the specified value
 */
void SetConnectionInitiator(ConnectionID Con, int Initiator);

/*
 * GetConnectionInitiator: Returns the Initiator flag for the specified
 * connection
 */
int GetConnectionInitiator(ConnectionID Con);

/*
 * SetCalledPartyNumber: Sets the CalledPartyNumber belonging to the specified
 * connection. CalledPartyNumber has to be a zero terminated string containing
 * only ASCII numbers.
 * Note: CalledPartyNumber has to different meanings in incoming and outgoing
 * call. For the outgoing call it contains the number of the party that will
 * be called, in incoming calls it contains the number that has been dialed
 * by the external party. On an ISDN access with multiple numbers this
 * CalledPartyNumber must be used to determine which number has been called.
 * Note, that this number may contain only the last one or two digits of
 * the called number, but this is enough to uniquely identify the called port.
 */
void SetCalledPartyNumber(ConnectionID Con, char *CalledPartyNumber);

/*
 * SetCalledPartyNumberStruct: Sets the CalledPartyNumber belonging to the
 * specified connection. CalledStruct has to be a valid CAPI struct.
 */
void SetCalledPartyNumberStruct(ConnectionID Con, unsigned char *CalledPartyNumber);

/*
 * GetCalledPartyNumber: Returns the CalledPartyNumber belonging to the
 * specified connection as a zero terminated string.
 */
char *GetCalledPartyNumber(ConnectionID Con);

/*
 * GetCalledPartyNumberStruct: Returns the CalledPartyNumber belonging to the
 * specified connection as a CAPI struct.
 */
unsigned char *GetCalledPartyNumberStruct(ConnectionID Con);

/*
 * SetCallingPartyNumber: Sets the CallingPartyNumber belonging to the
 * specified connection. CalledPartyNumber has to be a zero terminated string
 * containing only ASCII numbers.
 * CallingPartyNumber contains always the number of the party that originated
 * the call, if it is not needed you dont need to set it.
 */
void SetCallingPartyNumber(ConnectionID Con, char *CallingPartyNumber);

/*
 * SetCallingPartyNumberStruct: Sets the CallingPartyNumber belonging to the
 * specified connection. CallingStruct has to be a valid CAPI struct.
 */
void SetCallingPartyNumberStruct(ConnectionID Con, unsigned char *CallingPartyNumber);

/*
 * GetCallingPartyNumber: Returns the CallingPartyNumber belonging to the
 * specified connection as a zero terminated string.
 */
char *GetCallingPartyNumber(ConnectionID Con);

/*
 * GetCallingPartyNumberStruct: Returns the CallingPartyNumber belonging to the
 * specified connection as a CAPI struct.
 */
unsigned char *GetCallingPartyNumberStruct(ConnectionID Con);

void SetFaxNCPI (ConnectionID Con, faxNCPI_t *faxNCPI);
faxNCPI_t *GetFaxNCPI(ConnectionID Con);

void SetB3Reason(ConnectionID Con, _cword r);
void SetReason(ConnectionID Con, _cword r);

unsigned short GetB3Reason(ConnectionID);
unsigned short GetReason(ConnectionID);
#endif	/* _id_h_ */
