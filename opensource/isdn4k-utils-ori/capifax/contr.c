/* $Id: contr.c,v 1.2 1998/10/23 12:50:51 fritz Exp $
 *
 * Functions for dealing with controllers.
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
 * $Log: contr.c,v $
 * Revision 1.2  1998/10/23 12:50:51  fritz
 * Added RCS keywords and GPL notice.
 *
 */
#include <assert.h>
#include <sys/time.h>
#include <linux/capi.h>
#include <capi20.h>

#include "contr.h"
#include "id.h"

/*
 * GetNumController: Returns the number of controllers detected by CAPI
 */
unsigned GetNumController(void) {
	unsigned short Buffer;

	/* retrieve the number of installed controllers */
	CAPI20_GET_PROFILE (0, (unsigned char *)&Buffer);
	return Buffer;
}

/*
 * GetNumOfSupportedBChannels: Returns the number of supported B-channels
 * for the specified controller
 */
unsigned GetNumOfSupportedBChannels (long Controller) {
	unsigned short Buffer[64 / sizeof (unsigned short)];

	assert (Controller != INVAL_CONTROLLER);
	/* retrieve controller specific information */
	CAPI20_GET_PROFILE((unsigned)Controller, (unsigned char *)Buffer);
	return (unsigned)Buffer[1];
}

/*
 * GetFreeController: Returns the number of the first controller that has
 * one free B-channel, or INVAL_CONTROLLER if none found.
 */
long GetFreeController(void) {
	long Controller;
	int numController;

	numController = GetNumController ();
	for (Controller = 1; Controller <= numController; Controller++) {
		if (GetNumOfSupportedBChannels (Controller) > GetNumberOfConnections (Controller)) {
			return Controller;
		}
	}
	return INVAL_CONTROLLER;
}
