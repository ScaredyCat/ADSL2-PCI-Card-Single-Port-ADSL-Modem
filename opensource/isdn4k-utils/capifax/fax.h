/* $Id: fax.h,v 1.2 1998/10/23 12:50:54 fritz Exp $
 *
 * Setup B3 for FAX.
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
 * $Log: fax.h,v $
 * Revision 1.2  1998/10/23 12:50:54  fritz
 * Added RCS keywords and GPL notice.
 *
 */
#ifndef _fax_h_
#define _fax_h_

/* FAX Resolutions */
#define FAX_STANDARD_RESOLUTION 0
#define FAX_HIGH_RESOLUTION     1

/* FAX Formats */
#define FAX_SFF_FORMAT                  0
#define FAX_PLAIN_FORMAT                1
#define FAX_PCX_FORMAT                  2
#define FAX_DCX_FORMAT                  3
#define FAX_TIFF_FORMAT                 4
#define FAX_ASCII_FORMAT                5
#define FAX_EXTENDED_ASCII_FORMAT       6
#define FAX_BINARY_FILE_TRANSFER_FORMAT 7

typedef struct fax3proto3 {
	unsigned char len;
	unsigned short resolution __attribute__ ((packed));
	unsigned short format __attribute__ ((packed));
	unsigned char Infos[100] __attribute__ ((packed));
} B3_PROTO_FAXG3;

void SetupB3Config(B3_PROTO_FAXG3 *B3conf, int FAX_Format);

#endif	/* _fax_h_ */
