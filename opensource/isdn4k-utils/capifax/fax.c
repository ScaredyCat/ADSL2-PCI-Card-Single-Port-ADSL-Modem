/* $Id: fax.c,v 1.2 1998/10/23 12:50:54 fritz Exp $
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
 * $Log: fax.c,v $
 * Revision 1.2  1998/10/23 12:50:54  fritz
 * Added RCS keywords and GPL notice.
 *
 */
#include <string.h>

#include "fax.h"

char *stationID = "00000000";
char *headLine  = "Unconfigured Linux FAXServer";

void SetupB3Config(B3_PROTO_FAXG3  *B3conf, int FAX_Format) {
    int len1;
    int len2;

    B3conf->resolution = 0;
    B3conf->format = (unsigned short)FAX_Format;
    len1 = strlen(stationID);
    B3conf->Infos[0] = (unsigned char)len1;
    strcpy((char *)&B3conf->Infos[1], stationID);
    len2 = strlen(headLine);
    B3conf->Infos[len1 + 1] = (unsigned char)len2;
    strcpy((char *)&B3conf->Infos[len1 + 2], headLine);
    B3conf->len = (unsigned char)(2 * sizeof(unsigned short) + len1 + len2 + 2);
}
