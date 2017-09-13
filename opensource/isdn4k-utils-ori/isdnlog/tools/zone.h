/* $Id: zone.h,v 1.3 1999/11/07 13:29:29 akool Exp $
 *
 * Verzonungsberechnung
 *
 * Copyright 1999 by Leo Tötsch <lt@toetsch.at>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Log: zone.h,v $
 * Revision 1.3  1999/11/07 13:29:29  akool
 * isdnlog-3.64
 *  - new "Sonderrufnummern" handling
 *
 * Revision 1.2  1999/06/26 12:26:35  akool
 * isdnlog Version 3.37
 *   fixed some warnings
 *
 * Revision 1.1  1999/06/09 19:59:23  akool
 * isdnlog Version 3.31
 *  - Release 0.91 of zone-Database (aka "Verzonungstabelle")
 *  - "rate-de.dat" V:1.02-Germany [09-Jun-1999 21:45:26]
 *
 *
 */

#ifndef _ZONE_H_
#define _ZONE_H_

int  initZone (int provider, char *path, char **msg);
void exitZone (int provider);
int  getZone  (int provider, char *from, char *to);

#endif
