/* $Id: init.h,v 1.3 1999/09/10 17:20:34 calle Exp $
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
 * $Log: init.h,v $
 * Revision 1.3  1999/09/10 17:20:34  calle
 * Last changes for proposed standards (CAPI 2.0):
 * - AK1-148 "Linux Extention"
 * - AK1-155 "Support of 64-bit Applications"
 *
 * Revision 1.2  1998/10/23 12:50:58  fritz
 * Added RCS keywords and GPL notice.
 *
 */
#ifndef _init_h_
#define _init_h_

extern unsigned Appl_Id;

/*
 * RegisterCAPI: Check for CAPI, allocate memory for CAPI-buffer and
 * register application. This function has to be called before using any
 * other CAPI functions.
 */
unsigned RegisterCAPI (void);

/*
 * ReleaseCAPI: deregister application
 */
void ReleaseCAPI (void);

#endif	/* _init_h_ */
