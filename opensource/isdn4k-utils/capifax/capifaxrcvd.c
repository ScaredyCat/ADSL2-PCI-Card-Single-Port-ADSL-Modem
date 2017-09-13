/* $Id: capifaxrcvd.c,v 1.2 1998/10/23 12:50:48 fritz Exp $
 *
 * A FAX receive daemon for CAPI.
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
 * $Log: capifaxrcvd.c,v $
 * Revision 1.2  1998/10/23 12:50:48  fritz
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
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
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

static  char *CalledPartyNumber = NULL;
static  char *RcvDir = NULL;
static  char *notifyCmd = NULL;
static  char RcvName[1024];

static	ConnectionID	Slot;

#define B1PROTOCOL 4
#define B2PROTOCOL 4
#define B3PROTOCOL 4

#define QueueSize  8

typedef struct __DataElement {
	char DATA[SendBlockSize];
	unsigned short DATA_LENGTH;
	unsigned SENT;
} _DataElement;

typedef struct __DataQueue {
	_DataElement Element[QueueSize];
	unsigned Head;
	unsigned Tail;
	unsigned Fill;
} _DataQueue;

_DataQueue Queue;

static unsigned FileReceive = FALSE; /* signals if transfer is in progress */
static FILE *f;
static int reason;
static int reason_b3;
static int rres;
static int rpages;
static char rid[50];

/*
 * MainDataAvailable: signals received data blocks
 * This function is called after a DATA_B3_INDication is received. The flag
 * DiscardData tells CAPI to free the memora area directly after the return
 * of this function when set to TRUE (1) which is the preset. When the flag
 * is set to FALSE (0) the data area MUST be freed later with ReleaseData.
 * The datahandle identifies the memory area. When reaching 7 unconfirmed
 * blocks, no more incoming data will be signaled until freeing at least
 * one block.
 */
static void MainDataAvailable(ConnectionID Connection, void *Data, unsigned short DataLength,
			      unsigned short DataHandle, int *DiscardData) {
	assert (Connection != INVALID_CONNECTION_ID);
	if ((FileReceive) && (f != NULL))
		fwrite(Data, 1, DataLength, f);
	*DiscardData = TRUE;
}

/*
 * MainStateChange: signals a state change on both B-channels (connected,
 * disconnected). Whenever a channel changes his state this function is called
 */
static void MainStateChange(ConnectionID Connection, ConnectionState State) {
	faxNCPI_t *faxNCPI;

	assert (Connection != INVALID_CONNECTION_ID);
	if (State == Disconnected) {
		unsigned short r3 = GetB3Reason(Connection);
		unsigned short r = GetReason(Connection);

		Slot = INVALID_CONNECTION_ID;
		reason = r;
		reason_b3 = r3;
#if 0
		printf("Disconnected.\n");
		printf("  Reason            : %04x %s\n", r, Decode_Info(r));
		printf("  Reason-B3         : %04x %s\n", r3, Decode_Info(r3));
#endif
		if ((faxNCPI = GetFaxNCPI(Connection))) {
			strcpy(rid, faxNCPI->id);
			rres = faxNCPI->resolution;
			rpages = faxNCPI->pages;

#if 0
			printf("  Remote Station ID : %s\n", faxNCPI->id);
			printf("  Transfer-Rate     : %d bps\n", faxNCPI->rate);
			printf("  Resolution        : %s\n", faxNCPI->resolution ? "high" : "low");
			printf("  Number of Pages   : %d\n", faxNCPI->pages);
#endif
		}
	}
}

/*
 * MainIncomingCall: signals an incoming call
 * This function will be executed if a CONNECT_INDication appears to
 * inform the user.
 */
static void MainIncomingCall(ConnectionID Connection, char *CallingPartyNumber) {
	B3_PROTO_FAXG3  B3conf;

	assert (Connection != INVALID_CONNECTION_ID);
	syslog(LOG_INFO, "Incoming Call from %s\n", CallingPartyNumber);
	SetupB3Config(&B3conf, FAX_SFF_FORMAT);
	if (CalledPartyNumber && strlen(CalledPartyNumber)) {
		syslog(LOG_INFO, "Called #: %s\n", GetCalledPartyNumber(Connection));
		if (strcmp(GetCalledPartyNumber(Connection), CalledPartyNumber)) {
			AnswerCall(Connection, IGNORE, 4, 4, 4, (_cstruct)&B3conf);
			syslog(LOG_INFO, "Call from %s ignored\n", CallingPartyNumber);
			return;
		}
	}
	if (Slot == INVALID_CONNECTION_ID) {
		Slot = Connection;
		sprintf(RcvName, "rc-%s-%08lx", CallingPartyNumber, time(NULL));
		f = fopen(RcvName, "wb");
		if (f != NULL) {
			FileReceive = TRUE;
			syslog(LOG_INFO, "Call from %s accepted\n", CallingPartyNumber);
			AnswerCall(Connection, ACCEPT, 4, 4, 4, (_cstruct)&B3conf);
			chmod(RcvName, 0600);
			return;
		}
	}
	AnswerCall(Connection, REJECT, 4, 4, 4, (_cstruct)&B3conf);
	syslog(LOG_INFO, "Call from %s rejected\n", CallingPartyNumber);
}

void InitQueue(void) {
	unsigned x;

	for (x=0; x<QueueSize; x++)
		Queue.Element[x].SENT  = FALSE;
	Queue.Head = 0;
	Queue.Tail = 0;
	Queue.Fill = 0;
}

unsigned ReceiveFax() {
	int gotdata;
	int ret = 0;

	reason = reason_b3 = rpages = rres = 0;
	rid[0] = '\0';
	syslog(LOG_INFO, "Start listening\n");
	InitQueue();
	Listen(0x1FFF03FF);
	while (Slot == INVALID_CONNECTION_ID)
		Handle_CAPI_Msg();
	while (Slot != INVALID_CONNECTION_ID)
		Handle_CAPI_Msg();
	FileReceive = FALSE;
	gotdata = (ftell(f) != 0L);
	fclose(f);
	switch (reason) {
		case 0:
		case 0x3490: /* Normal call clearing */
		case 0x349f: /* Normal, unspecified */
			ret = reason_b3;
			break;
		default:
			ret = reason;
	}
	if (gotdata && (!ret) && rpages) {
		/* Rename fax to reflect resolution, pages and remote-id */
		int i;
		int l;
		char newname[1024];

		sprintf(newname, "f%c%03d%08lx", (rres)?'f':'n', rpages,
			time(NULL));
		l = strlen(newname);
		for (i=0; rid[i]; i++) {
			switch(rid[i]) {
				case ' ':
					newname[l++] = '_';
					break;
				case '/':
				case '\\':
				case '&':
				case ';':
				case '(':
				case ')':
				case '>':
				case '<':
				case '|':
				case '?':
				case '*':
				case '\'':
				case '"':
				case '`': 
             				if (newname[l-1] != '-' )
						newname[l++] = '-';
					break;
				default:
					newname[l++] = rid[i];
			}
		}
		newname[l] = '\0';
		rename(RcvName, newname);
		chmod(newname, 0640);
		syslog(LOG_INFO, "Received a FAX\n");
		if (notifyCmd) {
			sprintf(RcvName, "%s %s", notifyCmd, newname);
			system(RcvName);
		}
	} else
		remove(RcvName);
	return ret;
}

void usage(void) {
	fprintf(stderr, "usage: capifaxrcvd [-i stationID] [-h header] [-l listenNumber] [-n notifyCmd] rcvDirectory\n");
	exit(1);
}

int main(int argc, char **argv) {
	int numController;
	int BChannels, Contr;
	int c;

	while ((c = getopt(argc, argv, "l:i:h:n:")) != EOF) {
		switch (c) {
			case 'l':
				CalledPartyNumber = strdup(optarg);
				break;
			case 'i':
				stationID = strdup(optarg);
				break;
			case 'h':
				headLine = strdup(optarg);
				break;
			case 'n':
				notifyCmd = strdup(optarg);
				break;
			case '?':
				usage();
		}
	}
	if (argc < optind + 1)
		usage();
	RcvDir = argv[optind];
	if (chdir(RcvDir) != 0) {
		perror(RcvDir);
		return -1;
	}
	Slot = INVALID_CONNECTION_ID;
	MainStateChange_p = MainStateChange;
	MainDataAvailable_p = MainDataAvailable;
	MainIncomingCall_p = MainIncomingCall;
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
	switch (fork()) {
		case 0:
			openlog("capifaxrcvd", LOG_PID, LOG_DAEMON);
			close(0);
			close(1);
			close(2);
			while (1)
				ReceiveFax();
			if ((Slot != INVALID_CONNECTION_ID) &&
	    			(GetState(Slot) != Disconnected) &&
	    			(GetState(Slot) != D_DisconnectPending))
				Disconnect(Slot);
			break;
		case -1:
			perror("fork");
			exit(errno);
			break;
	}
	return 0;
}
