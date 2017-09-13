/* $Id: data.c,v 1.3 2001/03/01 14:59:11 paul Exp $
 *
 * Functions for dealing user data.
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
 * $Log: data.c,v $
 * Revision 1.3  2001/03/01 14:59:11  paul
 * Various patches to fix errors when using the newest glibc,
 * replaced use of insecure tempnam() function
 * and to remove warnings etc.
 *
 * Revision 1.2  1998/10/23 12:50:52  fritz
 * Added RCS keywords and GPL notice.
 *
 */
#include <assert.h>
#include <sys/time.h>
#include <linux/capi.h>
#include <capi20.h>

#include "id.h"
#include "init.h"
#include "data.h"

MainDataAvailable_t MainDataAvailable_p = 0L;
MainDataConf_t MainDataConf_p = 0L;

/*
 * SendData: Sends one block with data over the specified channel
 */
unsigned SendData(ConnectionID Connection, void *Data, unsigned short DataLength, unsigned short DataHandle) {
	_cmsg   CMSG;

	assert (Connection != INVALID_CONNECTION_ID);
	assert (GetState(Connection) == Connected);

	DATA_B3_REQ_HEADER(&CMSG, Appl_Id, 0, GetConnectionNCCI(Connection));
	DATA_B3_REQ_DATA(&CMSG) = Data;
	DATA_B3_REQ_DATALENGTH(&CMSG) = DataLength;
	DATA_B3_REQ_DATAHANDLE(&CMSG) = DataHandle;
	return CAPI_PUT_CMSG(&CMSG);
}

/*
 * DataConf: signals the successful sending of a datablock
 * This function is called after receiving a DATA_B3_CONFirmation. CAPI signals
 * that the datablock identified by DataHandle has been sent and the memory
 * area may be freed. The DataHandle is the same as specified in SendBlock.
 * This function is implemented in the main program.
 */
void DataConf(ConnectionID Connection, unsigned short DataHandle, unsigned short Info) {
	if (MainDataConf_p)
		MainDataConf_p(Connection, DataHandle, Info);
}

/*
 * DataAvailable: signals received data blocks
 * This function is called after a DATA_B3_INDication is received. The flag
 * DiscardData tells CAPI to free the memora area directly after the return
 * of this function when set to TRUE (1) which is the preset. When the flag
 * is set to FALSE (0) the data area MUST be freed later with ReleaseData.
 * The datahandle identifies the memory area. When reaching 7 unconfirmed
 * blocks, no more incoming data will be signaled until freeing at least
 * one block.
 * This function is implemented in the main program.
 */
void DataAvailable(ConnectionID Connection, void *Data, unsigned short DataLength,
		   unsigned short DataHandle, int *DiscardData) {
	if (MainDataAvailable_p)
		MainDataAvailable_p(Connection, Data, DataLength, DataHandle,
				    DiscardData);
	else
		*DiscardData = TRUE;
}

/*
 * ReleaseData: allows CAPI to reuse the memory area of the specified block.
 * CAPI allows max. 7 unconfirmed Blocks. If the maximum of 7 is reached,
 * no more DATA_B3_INDications will come up.
 */
unsigned ReleaseData(ConnectionID Connection, unsigned short DataHandle) {
	_cmsg   CMSG;

	assert (Connection != INVALID_CONNECTION_ID);
	assert (GetState(Connection) == Connected);
	return DATA_B3_RESP(&CMSG, Appl_Id, 0, GetConnectionNCCI(Connection), DataHandle);
}
