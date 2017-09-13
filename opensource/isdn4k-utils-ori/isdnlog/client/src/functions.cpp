/* $Id: functions.cpp,v 1.2 1998/05/10 23:40:02 luethje Exp $
 *
 * kisdnog for ISDN accounting for isdn4linux. (Report-module)
 *
 * Copyright 1996, 1997 by Stefan Luethje (luethje@sl-gw.lake.de)
 *                         Claudia Weber  (weber@sl-gw.lake.de)
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
 * $Log: functions.cpp,v $
 * Revision 1.2  1998/05/10 23:40:02  luethje
 * some changes
 *
 */

#include <string.h>

#define _FUNCTIONS_CPP_ 

#include "kisdnlog.h"

/****************************************************************************/

#define MAX_STRINGS	1

/****************************************************************************/

const char *Byte2Str(double Bytes, int flag)
{
	static char string[MAX_STRINGS][20];
	static int num = 0;
	int  factor = 1;
	char prefix = ' ';


	num = (num+1)%MAX_STRINGS;

	if (Bytes >= 9999999999.0)
	{
		factor = 1073741824;
		prefix = 'G';
	}
	else
	if (Bytes >= 9999999)
	{
		factor = 1048576;
		prefix = 'M';
	}
	else
	if (Bytes >= 9999)
	{
		factor = 1024;
		prefix = 'k';
	} 

	sprintf(string[num], "%s  %0.*f %cB%s",
	        flag&NO_DIR?"":(flag&OUT_DATA?"Out":"In"),
          ((flag&GET_BPS)||prefix!=' ')?2:0, Bytes/factor, prefix, flag&GET_BPS?"/s":"");

	return string[num];
}

/****************************************************************************/

