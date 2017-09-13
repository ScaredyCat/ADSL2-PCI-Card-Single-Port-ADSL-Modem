/* $Id: contr.h,v 1.2 1998/10/23 12:50:52 fritz Exp $
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
 * $Log: contr.h,v $
 * Revision 1.2  1998/10/23 12:50:52  fritz
 * Added RCS keywords and GPL notice.
 *
 */
#ifndef _contr_h_
#define _contr_h_

/*
 * GetNumController: Returns the number of controllers detected by CAPI
 */
unsigned GetNumController (void);

/*
 * GetNumOfSupportedBChannels: Returns the number of supported B-channels
 * for the specified controller
 */
unsigned GetNumOfSupportedBChannels (long Controller);

/*
 * GetFreeController: Returns the number of the first controller that has
 * one free B-channel, or INVAL_CONTROLLER if none found.
 */
long GetFreeController (void);

#endif	/* _contr_h_ */
