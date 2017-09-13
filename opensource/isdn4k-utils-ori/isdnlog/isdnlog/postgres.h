/* $Id: postgres.h,v 1.3 2002/03/11 16:17:10 paul Exp $
 *
 * Interface for Postgres95-Database for isdn4linux.
 *
 * by Markus Leist (markus@hal.dirnet.com)
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
 * $Log: postgres.h,v $
 * Revision 1.3  2002/03/11 16:17:10  paul
 * DM -> EUR
 *
 * Revision 1.2  1997/03/31 20:50:56  akool
 * fixed the postgres95 part of isdnlog
 *
 * Revision 1.1  1997/03/16 20:58:46  luethje
 * Added the source code isdnlog. isdnlog is not working yet.
 * A workaround for that problem:
 * copy lib/policy.h into the root directory of isdn4k-utils.
 *
 * Revision 1.2  1996/12/05 10:18:17  admin
 * first version auf Postgres95-Interface seems to be ok
 *
 * Revision 1.1  1996/12/04 09:57:44  admin
 * Initial revision
 *
 *
 */

/****************************************************************************/

#ifndef _POSTGRES_H_
#define _POSTGRES_H_

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <limits.h>
#include <signal.h>
#include <math.h>
#include <syslog.h>
#include <assert.h>
#include <libpq-fe.h>           /* functions for postgres95 */


/* if DB_*-Macro=NULL, Postgres will use following environment variables:
   PGHOST, PGPORT, PGOPTIONS, PGTTY, PGDATABASE */
#define DB_HOST       NULL    	          /* host name of backend-server */
#define DB_PORT       NULL                /* port of backend-server */
#define DB_OPTIONS    NULL                /* special options to start up backend-server */
#define DB_TTY        NULL                /* debugging tty for backend-server */
#define DB_NAME      "isdn"               /* name of database */
#define DB_TABLE     "isdn2"              /* name of table in database */

#define NUMSIZE      20


struct _DbStrIn
{
  time_t  connect;            /* Zeitpunkt des Verbindungsaufbaues */
  char    calling[NUMSIZE];   /* Telefonnummer des Anrufers */
  char    called[NUMSIZE];    /* Telefonnummer des Gegners */
  int     duration;           /* Dauer der Verbindung in Sekunden */
  int     hduration;          /* Dauer der Verbindung in 1/100 Sekunden */
  int     aoce;               /* Anzahl zu zahlender Gebuehreneinheiten (AOC-D) */
  int     dialin;             /* "I" fuer incoming call, "O" fuer outgoing call */
  int     cause;              /* Kam eine Verbindung nicht zustande ist hier der Grund */
  long    ibytes;             /* Summe der uebertragenen Byte _von_ draussen (incoming) */
  long    obytes;             /* Summe der uebertragenen Byte _nach_ draussen (outgoing) */
  int     version;            /* Versionsnummer (LOG_VERSION) dieses Eintrages */
  int     si1;                /* Dienstkennung fuer diese Verbindung (1=Speech, 7=Data usw.) */
  int     si11;               /* Bei Dienstkennung 1=Speech -> analog oder digital ? */
  double  currency_factor;    /* Der Currency Factor fuer diese Verbinung (hier z.Zt. 0,12) */
  char    currency[32];       /* (16) Die Waehrung fuer diese Verbindung (in Europa "EUR") */
  double  pay;		      /* Der Endbetrag i.d. jeweiligen Landeswaehrung fuer diese Verbindung */
};

typedef struct _DbStrIn DbStrIn;


/****************************************************************************/

#ifdef _POSTGRES_C_
#define _EXTERN
#else
#define _EXTERN extern
#endif
_EXTERN int  dbOpen( void);
_EXTERN int  dbClose( void);
_EXTERN int  dbAdd( DbStrIn *in);
_EXTERN int  dbStatus( void);
#undef _EXTERN

#ifdef _POSTGRES_C_
#define _EXTERN static
#else
#define _EXTERN extern
#endif
_EXTERN char    *db_Host, *db_Port, *db_Options, *db_Tty;
_EXTERN char    *db_Name, *db_Table;
_EXTERN PGconn  *db_Conn;
#undef _EXTERN


#endif /* _POSTGRES_H_ */
