/* $Id: mysqldb.h,v 1.3 2002/03/11 16:17:10 paul Exp $
 *
 * Interface for mySQL-Database for isdn4linux.
 *
 * by Sascha Matzke (sascha@bespin.escape.de)
 * based on postgres.h by Markus Leist (markus@hal.dirnet.com)
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
 * $Log: mysqldb.h,v $
 * Revision 1.3  2002/03/11 16:17:10  paul
 * DM -> EUR
 *
 * Revision 1.2  2000/04/02 17:35:07  akool
 * isdnlog-4.18
 *  - isdnlog/isdnlog/isdnlog.8.in  ... documented hup3
 *  - isdnlog/tools/dest.c ... _DEMD1 not recogniced as key
 *  - mySQL Server version 3.22.27 support
 *  - new rates
 *
 * Revision 1.1  1998/04/06 15:45:19  keil
 * Added missing files
 *
 * Revision 0.1  1998/02/25 11:50:56  admin
 * Initial revision
 *
 *
 */

/****************************************************************************/

#ifndef _MYSQL_H_
#define _MYSQL_H_

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
#include <mysql/mysql.h>           /* functions for mysql */


/*   */
#define DB_HOST       "localhost"          /* host name of backend-server */
#define DB_USER       "isdn"               /* mysql_db user */
#define DB_PASSWD     "isdn"               /* mysql_db password */
#define DB_NAME       "isdn"               /* name of database */
#define DB_TABLE      "isdnlog"            /* name of table in database */

#define NUMSIZE    32


struct _DbStrIn
{
  time_t  connect;            /* Zeitpunkt des Verbindungsaufbaues */
  char    calling[NUMSIZE];   /* Telefonnummer des Anrufers */
  char    called[NUMSIZE];    /* Telefonnummer des Gegners */
  int     duration;           /* Dauer der Verbindung in Sekunden */
  int     hduration;          /* Dauer der Verbindung in 1/100 Sekunden */
  int     aoce;               /* Anzahl zu zahlender Gebuehreneinheiten (AOC-D) */
  char    dialin;             /* "I" fuer incoming call, "O" fuer outgoing call */
  int     cause;              /* Kam eine Verbindung nicht zustande ist hier der Grund */
  long    ibytes;             /* Summe der uebertragenen Byte _von_ draussen (incoming) */
  long    obytes;             /* Summe der uebertragenen Byte _nach_ draussen (outgoing) */
  int     version;            /* Versionsnummer (LOG_VERSION) dieses Eintrages */
  int     si1;                /* Dienstkennung fuer diese Verbindung (1=Speech, 7=Data usw.) */
  int     si11;               /* Bei Dienstkennung 1=Speech -> analog oder digital ? */
  double  currency_factor;    /* Der Currency Factor fuer diese Verbinung (hier z.Zt. 0,12) */
  char    currency[32];       /* (16) Die Waehrung fuer diese Verbindung (in Europa "EUR") */
  double  pay;		      /* Der Endbetrag i.d. jeweiligen Landeswaehrung fuer diese Verbindung */
  char    provider[NUMSIZE];  /* Der Provider der Verbindung */
};

typedef struct _DbStrIn mysql_DbStrIn;


/****************************************************************************/

#ifdef _MYSQL_C_
#define _EXTERN
#else
#define _EXTERN extern
#endif
_EXTERN int  mysql_dbOpen( void);
_EXTERN int  mysql_dbClose( void);
_EXTERN int  mysql_dbAdd( mysql_DbStrIn *in);
_EXTERN int  mysql_dbStatus( void);
#undef _EXTERN

#ifdef _MYSQL_C_
#define _EXTERN static
#else
#define _EXTERN extern
#endif
_EXTERN char    *mysql_db_Host, *mysql_db_User, *mysql_db_Passwd;
_EXTERN char    *mysql_db_Name, *mysql_db_Table;
/* _EXTERN MYSQL   *mysql; */
#undef _EXTERN


#endif /* _MYSQL_H_ */
