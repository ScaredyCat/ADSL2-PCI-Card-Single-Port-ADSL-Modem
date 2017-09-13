/* $Id: id.c,v 1.3 2001/03/01 14:59:11 paul Exp $
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
 * $Log: id.c,v $
 * Revision 1.3  2001/03/01 14:59:11  paul
 * Various patches to fix errors when using the newest glibc,
 * replaced use of insecure tempnam() function
 * and to remove warnings etc.
 *
 * Revision 1.2  1998/10/23 12:50:55  fritz
 * Added RCS keywords and GPL notice.
 *
 */
#include <assert.h>
#include <malloc.h>
#include <string.h>

#include "id.h"

/*
 * type definitions
 */
const char *ConnectionStateString[7] = {
	"Disconnected",
	"D_ConnectPending",
	"D_Connected",
	"B_ConnectPending",
	"Connected",
	"B_DisconnectPending",
	"D_DisconnectPending"
};

typedef struct _ConnectionDesc {
	unsigned InUse;
	long PLCI;
	long NCCI;
	long Controller;
	ConnectionState State;
	int Initiator;
	_cword Reason;
	_cword B3Reason;
	faxNCPI_t *faxNCPI;                      /* CAPI struct */
	unsigned char *CalledPartyNumberStruct;  /* CAPI struct */
	unsigned char *CallingPartyNumberStruct; /* CAPI struct */
} ConnectionDesc;

static ConnectionDesc C[maxConnections] = {{0}};
static unsigned char *EmptyStruct = (unsigned char *)"\0";

/*
 * InitConnectionIDHandling: Initialisation of internal structures. Must be
 * executed before using the functions in this module.
 */
void InitConnectionIDHandling (void) {
	unsigned Con;

	/* init local data */
	for (Con=0; Con<maxConnections; Con++)
		C[Con].InUse = 0;
}

/*
 * AllocConnection: allocates one connection structure. Returns
 * INVALID_CONNECTION_ID if none free. All members are resetted.
 */
ConnectionID AllocConnection (void) {
	unsigned Con;

	for (Con=0; Con<maxConnections; Con++) {
		if (!C[Con].InUse) {
			/* found free entry */
			C[Con].InUse = 1;
			C[Con].PLCI = INVAL_PLCI;
			C[Con].NCCI = INVAL_NCCI;
			C[Con].Controller = INVAL_CONTROLLER;
			C[Con].State = Disconnected;
			C[Con].Initiator = 0;
			C[Con].CalledPartyNumberStruct = EmptyStruct;
			C[Con].CallingPartyNumberStruct = EmptyStruct;
			C[Con].faxNCPI = NULL;
			C[Con].Reason = 0;
			C[Con].B3Reason = 0;
			return Con;
		}
	}
	return INVALID_CONNECTION_ID;
}

/*
 * FreeNumberStruct: Frees the memory area if the specified pointer is not
 * a pointer to EmptyStruct.
 */
static void FreeNumberStruct (unsigned char **pStruct) {

	assert (*pStruct != NULL);
	if (*pStruct != EmptyStruct) {
		free (*pStruct);
		*pStruct = EmptyStruct;
	}
}

/*
 * FreeConnection: frees one connection, the ConnectionID is no longer valid
 */
void FreeConnection (ConnectionID Con) {

	assert (Con != INVALID_CONNECTION_ID);
	assert (C[Con].InUse);
	C[Con].InUse = 0;
	FreeNumberStruct (&C[Con].CalledPartyNumberStruct);
	FreeNumberStruct (&C[Con].CallingPartyNumberStruct);
	if (C[Con].faxNCPI)
		free(C[Con].faxNCPI);
}

/*
 * SetConnectionPLCI: sets the PLCI (and controller) for one connection
 */
void SetConnectionPLCI (ConnectionID  Con, long PLCI) {

	assert (Con != INVALID_CONNECTION_ID);
	assert (C[Con].InUse);
	C[Con].PLCI = PLCI;
	C[Con].Controller = (long)(PLCI & 0x000000FF);
}

/*
 * GetConnectionPLCI: returns the PLCI for one connection
 */
long GetConnectionPLCI (ConnectionID Con) {

	assert (Con != INVALID_CONNECTION_ID);
	assert (C[Con].InUse);
	return C[Con].PLCI;
}

/*
 * GetConnectionByPLCI: Searches the connection that uses the PLCI, if none
 * found, returns INVALID_CONNECTION_ID.
 */
ConnectionID GetConnectionByPLCI (long PLCI) {
	unsigned Con;

	assert (PLCI != INVAL_PLCI);
	for (Con=0; Con<maxConnections; Con++)
		if (C[Con].InUse && C[Con].PLCI == PLCI)
			return Con;
	return INVALID_CONNECTION_ID;
}

/*
 * SetConnectionNCCI: Sets the NCCI for one connection.
 */
void SetConnectionNCCI (ConnectionID Con, long NCCI) {

	assert (Con != INVALID_CONNECTION_ID);
	assert (C[Con].InUse);
	C[Con].NCCI = NCCI;
}

/*
 * GetConnectionNCCI: Returns the NCCI for one connection.
 */
long GetConnectionNCCI (ConnectionID Con) {

	assert (Con != INVALID_CONNECTION_ID);
	assert (C[Con].InUse);
	return C[Con].NCCI;
}

/*
 * GetConnectionByNCCI: Searches the connection that uses the NCCI, if none
 * found, returns INVALID_CONNECTION_ID.
 */
ConnectionID GetConnectionByNCCI (long NCCI) {
	unsigned Con;

	assert (NCCI != INVAL_NCCI);
	for (Con=0; Con<maxConnections; Con++)
		if (C[Con].InUse && C[Con].NCCI == NCCI)
			return Con;
	return INVALID_CONNECTION_ID;
}

/*
 * GetController: Returns the controller that is used by the specified
 * connection.
 */
long GetController (ConnectionID Con) {

	assert (Con != INVALID_CONNECTION_ID);
	assert (C[Con].InUse);
	return C[Con].Controller;
}

/*
 * GetNumberOfConnections: Returns the number of currently active connections
 * on the specified controller.
 */
unsigned GetNumberOfConnections (long Controller) {
	ConnectionID Con;
	unsigned numConnections = 0;

	assert (Controller != INVAL_CONTROLLER);
	for (Con=0; Con<maxConnections; Con++)
		if (C[Con].InUse && (C[Con].Controller == Controller))
			numConnections++;
	return numConnections;
}

/*
 * GetState: Returns the state of the specified connection.
 */
ConnectionState GetState (ConnectionID Con) {
	if (Con == INVALID_CONNECTION_ID)
		return (ConnectionState)INVAL_STATE;
	assert (C[Con].InUse);
	return C[Con].State;
}

/*
 * SetState: Sets the state of the specified connection.
 */
void SetState (ConnectionID Con, ConnectionState State) {
	assert (Con != INVALID_CONNECTION_ID);
	assert (C[Con].InUse);
	/* internal connection state */
	C[Con].State = State;
}

/*
 * SetConnectionInitiator: Changes the Initiator flag to the specified value
 */
void SetConnectionInitiator (ConnectionID Con, int Initiator) {
	assert (Con != INVALID_CONNECTION_ID);
	assert (C[Con].InUse);
	C[Con].Initiator = Initiator;
}

/*
 * GetConnectionInitiator: Returns the Initiator flag for the specified
 * connection
 */
int GetConnectionInitiator (ConnectionID Con) {
	assert (Con != INVALID_CONNECTION_ID);
	assert (C[Con].InUse);
	return C[Con].Initiator;
}

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
void SetCalledPartyNumber (ConnectionID Con, char *CalledNumber) {
	assert (Con != INVALID_CONNECTION_ID);
	assert (C[Con].InUse);
	FreeNumberStruct (&C[Con].CalledPartyNumberStruct);
	if (CalledNumber) {
		int len = strlen(CalledNumber);
		/* \xLen\x80  STRING  '\0' */
		C[Con].CalledPartyNumberStruct = (unsigned char *)malloc(len + 2 + 1);
		assert (C[Con].CalledPartyNumberStruct != NULL);
		C[Con].CalledPartyNumberStruct[0] = (unsigned char)(len + 1);
		C[Con].CalledPartyNumberStruct[1] = 0x80;
		strcpy ((char *)&(C[Con].CalledPartyNumberStruct[2]), CalledNumber);
	}
}

/*
 * SetCalledPartyNumberStruct: Sets the CalledPartyNumber belonging to the
 * specified connection. CalledStruct has to be a valid CAPI struct.
 */
void SetCalledPartyNumberStruct (ConnectionID Con, unsigned char *CalledStruct) {
	assert (Con != INVALID_CONNECTION_ID);
	assert (C[Con].InUse);
	FreeNumberStruct (&C[Con].CalledPartyNumberStruct);
	if (CalledStruct) {
		/* two more for the length byte and '\0' */
		C[Con].CalledPartyNumberStruct = (unsigned char *)malloc((size_t)(CalledStruct[0] + 2));
		assert(C[Con].CalledPartyNumberStruct != NULL);
		memcpy(C[Con].CalledPartyNumberStruct, CalledStruct, (size_t)(CalledStruct[0] + 1));
		C[Con].CalledPartyNumberStruct[CalledStruct[0] + 1] = '\0';
	}
}

/*
 * GetCalledPartyNumber: Returns the CalledPartyNumber belonging to the
 * specified connection as a zero terminated string.
 */
char *GetCalledPartyNumber (ConnectionID Con) {
	if (C[Con].CalledPartyNumberStruct[0] > 1)
		return (char *)(&(C[Con].CalledPartyNumberStruct[2]));
	else
		return (char *)EmptyStruct;
}

/*
 * GetCalledPartyNumberStruct: Returns the CalledPartyNumber belonging to the
 * specified connection as a CAPI struct.
 */
unsigned char *GetCalledPartyNumberStruct (ConnectionID Con) {
	return C[Con].CalledPartyNumberStruct;
}

/*
 * SetCallingPartyNumber: Sets the CallingPartyNumber belonging to the
 * specified connection. CalledPartyNumber has to be a zero terminated string
 * containing only ASCII numbers.
 * CallingPartyNumber contains always the number of the party that originated
 * the call, if it is not needed you dont need to set it.
 */
void SetCallingPartyNumber (ConnectionID Con, char *CallingNumber) {
	assert (Con != INVALID_CONNECTION_ID);
	assert (C[Con].InUse);
	FreeNumberStruct (&C[Con].CallingPartyNumberStruct);
	if (CallingNumber && (*CallingNumber != 0)) {
		int len = strlen(CallingNumber);
		/* \xLen\x00\x80  STRING  '\0' */
		C[Con].CallingPartyNumberStruct = (unsigned char *)malloc(len + 3 + 1);
		assert (C[Con].CallingPartyNumberStruct != NULL);
		C[Con].CallingPartyNumberStruct[0] = (unsigned char)(len + 2);
		C[Con].CallingPartyNumberStruct[1] = 0x00;
		C[Con].CallingPartyNumberStruct[2] = 0x80;
		strcpy ((char *)&(C[Con].CallingPartyNumberStruct[3]), CallingNumber);
	}
}

/*
 * SetCallingPartyNumberStruct: Sets the CallingPartyNumber belonging to the
 * specified connection. CallingStruct has to be a valid CAPI struct.
 */
void SetCallingPartyNumberStruct (ConnectionID Con, unsigned char *CallingStruct) {
	assert (Con != INVALID_CONNECTION_ID);
	assert (C[Con].InUse);
	FreeNumberStruct (&C[Con].CallingPartyNumberStruct);
	if (CallingStruct) {
		/* two more for the length byte and '\0' */
		C[Con].CallingPartyNumberStruct = (unsigned char *)malloc((size_t)(CallingStruct[0] + 2));
		assert(C[Con].CallingPartyNumberStruct != NULL);
		memcpy(C[Con].CallingPartyNumberStruct, CallingStruct, (size_t)(CallingStruct[0] + 1));
		C[Con].CallingPartyNumberStruct[CallingStruct[0] + 1] = '\0';
	}
}

/*
 * GetCallingPartyNumber: Returns the CallingPartyNumber belonging to the
 * specified connection as a zero terminated string.
 */
char *GetCallingPartyNumber (ConnectionID Con) {
	if (C[Con].CallingPartyNumberStruct[0] > 2)
		return (char *)&(C[Con].CallingPartyNumberStruct[3]);
	else
		return (char *)EmptyStruct;
}

/*
 * GetCallingPartyNumberStruct: Returns the CallingPartyNumber belonging to the
 * specified connection as a CAPI struct.
 */
unsigned char *GetCallingPartyNumberStruct (ConnectionID Con) {
    return C[Con].CallingPartyNumberStruct;
}

void SetB3Reason(ConnectionID Con, _cword r) {
	assert (Con != INVALID_CONNECTION_ID);
	assert (C[Con].InUse);
	C[Con].B3Reason = r;
}

void SetReason(ConnectionID Con, _cword r) {
	assert (Con != INVALID_CONNECTION_ID);
	assert (C[Con].InUse);
	C[Con].Reason = r;
}

unsigned short GetB3Reason(ConnectionID Con) {
	assert (Con != INVALID_CONNECTION_ID);
	return C[Con].B3Reason;
}

unsigned short GetReason(ConnectionID Con) {
	assert (Con != INVALID_CONNECTION_ID);
	return C[Con].Reason;
}

/*
 * SetFaxNCPI: Sets the faxNCPI (StationId, Speed, etc.) belonging to the
 * specified connection. faxNCPI has to be a valid CAPI struct.
 */
void SetFaxNCPI (ConnectionID Con, faxNCPI_t *faxNCPI) {
	assert (Con != INVALID_CONNECTION_ID);
	assert (C[Con].InUse);
	if (faxNCPI) {
		C[Con].faxNCPI = (faxNCPI_t *)malloc(sizeof(faxNCPI_t));
		assert(C[Con].faxNCPI != NULL);
		memset(C[Con].faxNCPI, 0, sizeof(faxNCPI_t));
		memcpy(C[Con].faxNCPI, faxNCPI, (size_t)(faxNCPI->length + 1));
		C[Con].faxNCPI->id[faxNCPI->idlen + 1] = '\0';
	}
}

faxNCPI_t *GetFaxNCPI(ConnectionID Con) {
	return C[Con].faxNCPI;
}
