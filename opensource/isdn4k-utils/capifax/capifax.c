/* $Id: capifax.c,v 1.2 1998/10/23 12:50:47 fritz Exp $
 *
 * A FAX send application for CAPI.
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
 * $Log: capifax.c,v $
 * Revision 1.2  1998/10/23 12:50:47  fritz
 * Added RCS keywords and GPL notice.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <linux/capi.h>
#include <capi20.h>

#include "c20msg.h"
#include "capi.h"
#include "connect.h"
#include "contr.h"
#include "data.h"
#include "id.h"
#include "init.h"
#include "fax.h"

extern char *stationID;
extern char *headLine;

static  char *CallingPartyNumber = NULL;
static  char *CalledPartyNumber = NULL;

static	ConnectionID	Slot;

#define B1PROTOCOL	    4
#define B2PROTOCOL	    4
#define B3PROTOCOL	    4

#define QueueSize           8

typedef struct __DataElement {
	char	    DATA[SendBlockSize];
	unsigned short  DATA_LENGTH;
	unsigned	    SENT;
} _DataElement;

typedef struct	__DataQueue {
	_DataElement    Element[QueueSize];
	unsigned	    Head;
	unsigned	    Tail;
	unsigned	    Fill;
} _DataQueue;

_DataQueue	    Queue;

static unsigned FileTransfer = FALSE; /* signals if transfer is in progress */
static int verbose;
static int reason;
static int reason_b3;

/*--------------------------------------------------------------------------*\
 * MainDataConf: signals the successful sending of a datablock
 * This function is called after receiving a DATA_B3_CONFirmation. CAPI signals
 * that the datablock identified by DataHandle has been sent and the memory
 * area may be freed. The DataHandle is the same as specified in SendBlock.
\*--------------------------------------------------------------------------*/
void MainDataConf(ConnectionID	  Connection,
	unsigned short  DataHandle,
	unsigned short  Info) {

	assert (Connection != INVALID_CONNECTION_ID);
	if (Info != 0)
		return;
	if (FileTransfer) {
		assert (DataHandle == (unsigned short)Queue.Tail);
		Queue.Element[Queue.Tail].SENT = FALSE;
		if (++Queue.Tail >= QueueSize)
			Queue.Tail = 0;
		Queue.Fill--;
	}
}

/*--------------------------------------------------------------------------*\
 * MainStateChange: signals a state change on both B-channels (connected,
 * disconnected). Whenever a channel changes his state this function is called
\*--------------------------------------------------------------------------*/
void MainStateChange(ConnectionID Connection, ConnectionState State) {
	faxNCPI_t *faxNCPI;

	assert (Connection != INVALID_CONNECTION_ID);
	if (State == Disconnected) {
		unsigned short r3  = GetB3Reason(Connection);
		unsigned short r = GetReason(Connection);

		Slot = INVALID_CONNECTION_ID;
		reason_b3 = r3;
		reason = r;
		if (!verbose)
			return;
		printf("Disconnected.\n");
		printf("  Reason            : %04x %s\n", r, Decode_Info(r));
		printf("  Reason-B3         : %04x %s\n", r3, Decode_Info(r3));
		if ((faxNCPI = GetFaxNCPI(Connection))) {
			printf("  Remote Station ID : %s\n", faxNCPI->id);
			printf("  Transfer-Rate     : %d bps\n", faxNCPI->rate);
			printf("  Resolution        : %s\n", faxNCPI->resolution ? "high" : "low");
			printf("  Number of Pages   : %d\n", faxNCPI->pages);
		}
	}
}

/*--------------------------------------------------------------------------*\
 * Disconnect_h: high level Disconnect
\*--------------------------------------------------------------------------*/
unsigned Disconnect_h(ConnectionID Connection) {

	ConnectionState State;

	if (Connection == INVALID_CONNECTION_ID) {
		fprintf(stderr, "Disconnect_h: ConnectionID is invalid\n");
		return 0xFFFF;
	}
	State = GetState(Connection);
	if ((State == Disconnected) || (State == D_DisconnectPending))
		return 0xFFFF;
	return Disconnect(Connection);
}

void InitQueue(void) {
	unsigned	x;

	for (x=0; x<QueueSize; x++)
		Queue.Element[x].SENT  = FALSE;
	Queue.Head = 0;
	Queue.Tail = 0;
	Queue.Fill = 0;
}

void TransferData() {
	MESSAGE_EXCHANGE_ERROR  error;
	unsigned		    t;

	if (Queue.Fill > 0) {
		t = Queue.Tail;
		do {
			if (Queue.Element[t].SENT == FALSE) {
				error = SendData(0,
					(void *)Queue.Element[t].DATA,
					Queue.Element[t].DATA_LENGTH,
					(unsigned short)t);

				if (error != 0) {
					fprintf(stderr, "Error during transfer: 0x%04X !!!\n",error);
					break;
				}
				Queue.Element[t].SENT = TRUE;
			}
			if (++t >= QueueSize)
				t = 0;
		} while (t != Queue.Head);
	}
}

unsigned SendFax(char *name) {
	int             first;
	char            mbuf[4];
	unsigned	    count;
	B3_PROTO_FAXG3  B3conf;
	FILE	    *f;

	first = 1;
	reason = reason_b3 = 0;
	if (!strcmp(name, "-"))
		f = stdin;
	else
		f = fopen(name, "rb");
	if (!f) {
		perror(name);
		exit(errno);
	}
	fread(mbuf, 4, 1, f);

	InitQueue();
	SetupB3Config(&B3conf,
		(strncmp(mbuf, "Sfff", 4)) ? FAX_ASCII_FORMAT:FAX_SFF_FORMAT);
	if (Slot != INVALID_CONNECTION_ID) {
		fprintf(stderr, "Connection is already in use\n");
		fclose(f);
		return 0xFFFF;
	}
	Connect(&Slot, CalledPartyNumber, CallingPartyNumber, SPEECH,
		B1PROTOCOL, B2PROTOCOL, B3PROTOCOL, (unsigned char *)&B3conf);
	do {
		Handle_CAPI_Msg();
		if (Slot == INVALID_CONNECTION_ID) {
			fclose(f);
			return 2;
		}
	} while (GetState(Slot) != Connected);
	FileTransfer = TRUE;
	while (!feof(f)) {
		if (Queue.Fill < 7) {
			/* max. 7 outstanding blocks supported by CAPI */
			if (first) {
				memcpy(&(Queue.Element[Queue.Head].DATA[0]), mbuf, 4);
				count = fread(&(Queue.Element[Queue.Head].DATA[4]), 1,
					SendBlockSize - 4, f);
			} else
				count = fread(&(Queue.Element[Queue.Head].DATA[0]), 1,
					SendBlockSize, f);
			if (count > 0) {
				if (first)
					count += 4;
				Queue.Element[Queue.Head].DATA_LENGTH = (unsigned short)count;
				if (++Queue.Head >= QueueSize)
					Queue.Head = 0;
				Queue.Fill++;
			}
			first = 0;
		}
		if (GetState(Slot) != Connected)
			break;
		TransferData();
		Handle_CAPI_Msg();
	}
	Disconnect_h(Slot);
	while ((Slot != INVALID_CONNECTION_ID) &&
		(GetState(Slot) != Disconnected))
		Handle_CAPI_Msg();
	FileTransfer = FALSE;
	fclose(f);
	switch (reason) {
		case 0x3490: /* Normal call clearing */
		case 0x349f: /* Normal, unspecified */
			return reason_b3;
		default:
			return reason;
	}
	return 0;
}

void usage(void) {
	fprintf(stderr, "usage: capifax [-v] [-i stationID] [-h header] [-c callerNumber] phone file\n");
	exit(1);
}

int main(int argc, char **argv) {
	int     numController;
	int     BChannels, Contr;
	int     c;
	int     ret;

	verbose = 0;
	while ((c = getopt(argc, argv, "vc:i:h:")) != EOF) {
		switch (c) {
			case 'v':
				verbose++;
				break;
			case 'c':
				CallingPartyNumber = strdup(optarg);
				break;
			case 'i':
				stationID = strdup(optarg);
				break;
			case 'h':
				headLine = strdup(optarg);
				break;
			case '?':
				usage();
		}
	}
	if (argc < optind + 2)
		usage();
	CalledPartyNumber = argv[optind++];
	Slot = INVALID_CONNECTION_ID;
	MainDataConf_p = MainDataConf;
	MainStateChange_p = MainStateChange;
	if (!RegisterCAPI())
		return -1;
	atexit (ReleaseCAPI);
	InitConnectionIDHandling();
	if (!(numController = GetNumController())) {
		fprintf(stderr, "No CAPI controllers available\n");
		return -2;
	}
	BChannels = 0;
	for (Contr=1; Contr<=numController; Contr++)
		BChannels += GetNumOfSupportedBChannels(Contr);
	if (!BChannels) {
		fprintf(stderr, "No B-Channels available\n");
		return -3;
	}
	ret = SendFax(argv[optind]);
	if ((Slot != INVALID_CONNECTION_ID) &&
		(GetState(Slot) != Disconnected) &&
		(GetState(Slot) != D_DisconnectPending))
		Disconnect(Slot);
	return ret;
}
