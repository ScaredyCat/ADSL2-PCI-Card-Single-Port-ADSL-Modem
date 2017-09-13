/* $Id: isdn_dwabclib.h,v 1.1 1999/11/07 22:04:05 detabc Exp $
 *
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
 * $Log: isdn_dwabclib.h,v $
 * Revision 1.1  1999/11/07 22:04:05  detabc
 * add dwabc-udpinfo-utilitys in isdnctrl
 *
 */

#ifndef ISDN_DWABCLIB_H
#define ISDN_DWABCLIB_H 1
extern int isdn_udp_isisdn(char *dest, char **errm);
extern int isdn_udp_online(char *dest, char **errm);
extern int isdn_udp_dial(char *dest, char **errm);
extern int isdn_udp_hangup(char *dest, char **errm);
extern int isdn_udp_clear_ggau(char *dest, char **errm);
#endif
