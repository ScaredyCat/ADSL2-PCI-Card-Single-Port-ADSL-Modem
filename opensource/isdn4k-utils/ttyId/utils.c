/* $Id: utils.c,v 1.1 2000/08/30 18:27:01 armin Exp $
 *
 * ttyId - CAPI TTY AT-command emulator
 *
 * based on the AT-command emulator of the isdn4linux
 * kernel subsystem.
 *
 * Copyright 2000 by Armin Schindler (mac@melware.de)
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
 * $Log: utils.c,v $
 * Revision 1.1  2000/08/30 18:27:01  armin
 * Okay, here is the first try for an user-land
 * ttyI daemon. Compilable but not useable.
 *
 *
 */


#include <stdarg.h>
#include "ttyId.h"

void
logit(int level, char *fmt, ...)
{
	va_list pvar;
	char buf[1024];

	if ((level == LOG_DEBUG) && (!debug))
		return;

#if defined(__STDC__)
	va_start(pvar, fmt);
#else
	char *fmt;
	va_start(pvar);
	fmt = va_arg(pvar, char *);
#endif

	vsprintf(buf, fmt, pvar);
	syslog(level, "%s", buf);
	va_end(pvar);
}

void CopyString(char *Destination, char *Source, int Len)
{
        strncpy(Destination, Source, Len);

        Destination[Len] = 0;
}

/*
 * Get integer from char-pointer, set pointer to end of number
 */
int
getnum(char **p)
{
        int v = -1;

        while (*p[0] >= '0' && *p[0] <= '9')
                v = ((v < 0) ? 0 : (v * 10)) + (int) ((*p[0]++) - '0');
        return v;
}

/*
 * Get phone-number from modem-commandbuffer
 */
void
getdial(char *p, char *q,int cnt)
{
        int first = 1;
        int limit = ISDN_MSNLEN - 1;    /* MUST match the size of interface var to avoid
                                        buffer overflow */

        while (strchr(" 0123456789,#.*WPTS-", *p) && *p && --cnt>0) {
                if ((*p >= '0' && *p <= '9') || ((*p == 'S') && first) ||
                    (*p == '*') || (*p == '#')) {
                        *q++ = *p;
                        limit--;
                }
                if(!limit)
                        break;
                p++;
                first = 0;
        }
        *q = 0;
}

/*
 * Get MSN-string from char-pointer, set pointer to end of number
 */
void
get_msnstr(char *n, char **p)
{
        int limit = ISDN_MSNLEN - 1;

        while (((*p[0] >= '0' && *p[0] <= '9') ||
               /* Why a comma ??? */
               (*p[0] == ',')) &&
                (limit--))
                *n++ = *p[0]++;
        *n = '\0';
}

