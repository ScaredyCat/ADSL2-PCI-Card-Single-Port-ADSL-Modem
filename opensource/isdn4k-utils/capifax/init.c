/* $Id: init.c,v 1.3 1999/09/10 17:20:34 calle Exp $
 *
 * CAPI registration/deregistration.
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
 * $Log: init.c,v $
 * Revision 1.3  1999/09/10 17:20:34  calle
 * Last changes for proposed standards (CAPI 2.0):
 * - AK1-148 "Linux Extention"
 * - AK1-155 "Support of 64-bit Applications"
 *
 * Revision 1.2  1998/10/23 12:50:57  fritz
 * Added RCS keywords and GPL notice.
 *
 */
#include <stdio.h>
#include <malloc.h>
#include <sys/time.h>
#include <linux/capi.h>
#include <capi20.h>

#include "init.h"
#include "contr.h"
#include "capi.h"

/*
 * defines needed by InitISDN
 */
unsigned Appl_Id = 0;

#define MaxNumBChan        2    /* max. number of B-channels */
#define MaxNumB3DataBlocks 7    /* max. number of unconfirmed B3-datablocks */
			        /* 7 is the maximal number supported by CAPI */
#define MaxB3DataBlockSize 2048 /* max. B3-Datablocksize */
				/* 2048 is the maximum supported by CAPI */

static CAPI_MESSAGE CAPI_BUFFER = NULL;

/*
 * RegisterCAPI: Check for CAPI, allocate memory for CAPI-buffer and
 * register application. This function has to be called before using any
 * other CAPI functions.
 */
unsigned RegisterCAPI (void) {
	CAPI_REGISTER_ERROR ErrorCode;
	int numController;

	if (CAPI20_ISINSTALLED() != CapiNoError) {
		fprintf(stderr, "No CAPI support on this system.\n");
		return 0;
	}
	if (!(numController = GetNumController())) {
		fprintf(stderr, "RegisterCAPI: No ISDN-controller installed\n");
		return 0;
	}
	ErrorCode = CAPI20_REGISTER(MaxNumBChan, MaxNumB3DataBlocks,
				  MaxB3DataBlockSize, &Appl_Id);
	if (ErrorCode != CapiNoError) {
		fprintf(stderr, "RegisterCAPI: error: %04x\n", ErrorCode);
		return 0;
	}
	return numController;
}

/*
 * ReleaseCAPI: deregister application
 */
void ReleaseCAPI (void) {
	MESSAGE_EXCHANGE_ERROR ErrorCode;

	ErrorCode = CAPI20_RELEASE(Appl_Id);
	if (ErrorCode != 0)
		fprintf(stderr, "ReleaseCAPI: error: 0x%04X\n", ErrorCode);
	if (CAPI_BUFFER) {
		free(CAPI_BUFFER);
		CAPI_BUFFER = NULL;
	}
}
