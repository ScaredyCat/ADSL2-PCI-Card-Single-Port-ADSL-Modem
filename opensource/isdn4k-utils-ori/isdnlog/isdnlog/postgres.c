/* $Id: postgres.c,v 1.2 2000/01/23 22:31:13 akool Exp $
 *
 * Interface for Postgres95-Database for isdn4linux. (db-module)
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
 * $Log: postgres.c,v $
 * Revision 1.2  2000/01/23 22:31:13  akool
 * isdnlog-4.04
 *  - Support for Luxemburg added:
 *   - isdnlog/country-de.dat ... no +352 1 luxemburg city
 *   - isdnlog/rate-lu.dat ... initial LU version NEW
 *   - isdnlog/holiday-lu.dat ... NEW - FIXME
 *   - isdnlog/.Config.in  ... LU support
 *   - isdnlog/configure.in ... LU support
 *   - isdnlog/samples/isdn.conf.lu ... LU support NEW
 *
 *  - German zone-table enhanced
 *   - isdnlog/tools/zone/de/01033/mk ...fixed, with verify now
 *   - isdnlog/tools/zone/redzone ... fixed
 *   - isdnlog/tools/zone/de/01033/mzoneall ... fixed, faster
 *   - isdnlog/tools/zone/mkzonedb.c .... data Version 1.21
 *
 *  - Patch from Philipp Matthias Hahn <pmhahn@titan.lahn.de>
 *   - PostgreSQL SEGV solved
 *
 *  - Patch from Armin Schindler <mac@melware.de>
 *   - Eicon-Driver Support for isdnlog
 *
 * Revision 1.1  1997/03/16 20:58:45  luethje
 * Added the source code isdnlog. isdnlog is not working yet.
 * A workaround for that problem:
 * copy lib/policy.h into the root directory of isdn4k-utils.
 *
 * Revision 1.2  1996/12/05 10:20:04  admin
 * first version auf Postgres95-Interface seems to be ok
 *
 * Revision 1.1  1996/12/04 10:03:24  admin
 * Initial revision
 *
 *
 */


#ifdef POSTGRES
#define _POSTGRES_C_

#include "postgres.h"

void _PQfinish(void)
{
  if ( db_Conn )
    PQfinish( db_Conn);
  db_Conn = NULL;
}


int dbOpen(void)
{

  /* get strings from macros */

  db_Host = NULL;
  if ( DB_HOST )
    {
      db_Host = malloc( strlen( DB_HOST));
      strcpy( db_Host, DB_HOST);
    }

  db_Name = NULL;
  if ( DB_NAME )
    {
      db_Name = malloc( strlen( DB_NAME));
      strcpy( db_Name, DB_NAME);
    }

  db_Table = NULL;
  if ( DB_TABLE )
    {
      db_Table = malloc( strlen( DB_TABLE));
      strcpy( db_Table, DB_TABLE);
    }

  db_Port = NULL;
  if ( DB_PORT )
    {
      db_Port = malloc( strlen( DB_PORT));
      strcpy( db_Port, DB_PORT);
    }

  db_Options = NULL;
  if ( DB_OPTIONS )
    {
      db_Options = malloc( strlen( DB_OPTIONS));
      strcpy( db_Options, DB_OPTIONS);
    }

  db_Tty = NULL;
  if ( DB_TTY )
    {
      db_Tty = malloc( strlen( DB_TTY));
      strcpy( db_Tty, DB_TTY);
    }


  /* make a connection to the database */

  db_Conn = PQsetdb( db_Host, db_Port, db_Options, db_Tty, db_Name);

  /* check to see that the backend connection was successfully made */
  if ( PQstatus( db_Conn) == CONNECTION_BAD)
    {
    syslog( LOG_ERR, "%s", "Connection to ISDN-database failed.");
    syslog( LOG_ERR, "%s", PQerrorMessage( db_Conn));
    _PQfinish();
    return( -1);
    }

  return( 0);
}

int dbClose(void)
{
  _PQfinish();

  if ( db_Host)
    free( db_Host);

  if ( db_Name)
    free( db_Name);

  if ( db_Table)
    free( db_Table);

  if ( db_Port)
    free( db_Port);

  if ( db_Options)
    free( db_Options);

  if ( db_Tty)
    free( db_Tty);

  return( 0);
}

int dbAdd( DbStrIn *in)
{
  PGresult    *res;
  char         out_txt[400];
  struct tm   *tm;

  assert( (int)in);

  if ( dbStatus() )   /* returns -1 when not open */
    if ( dbOpen() )   /* returns -1 when error appears */
      return( -1);

  res = PQexec( db_Conn, "BEGIN");

  if ( PQresultStatus( res) != PGRES_COMMAND_OK)
    {
      syslog( LOG_ERR, "%s", "Connection to ISDN-database failed.");
      syslog( LOG_ERR, "%s", PQerrorMessage( db_Conn));
      _PQfinish();
      return( -1);
    }

  /* should PQclear PGresult whenever it is no longer needed to avoid
     memory leaks */
  PQclear( res);

  tm = localtime( (__const time_t *) &(in->connect));

  sprintf( out_txt
, "INSERT INTO %s "
           "( sdate, stime, calling, called, charge, dir, "
           "  in_bytes, out_bytes, msec, sec, status, service, "
          "  source, vrsion, factor, currency) values "
           "( \'%02d.%02d.%d\', \'%02d:%02d:%02d\', "
           "\'%s\', \'%s\',  \'%d\', \'%c\', "
           "\'%ld\', \'%ld\', \'%d\', \'%d\', \'%d\', \'%d\', "
          "\'%d\', \'%d\', \'%5.2f\', \'%s\' );",
          db_Table,
           tm->tm_mday, tm->tm_mon+1, tm->tm_year+1900,
           tm->tm_hour, tm->tm_min, tm->tm_sec,
           in->calling, in->called,
           in->aoce, in->dialin,
           in->ibytes, in->obytes,
           in->hduration, in->duration,
           in->cause, in->si1, in->si11,
          in->version, in->currency_factor,
          in->currency);

  /* fetch instances from the pg_database, the system catalog of databases*/
  res = PQexec( db_Conn, out_txt);

  if ( PQresultStatus( res) != PGRES_COMMAND_OK)
    {
      syslog( LOG_ERR, "%s", "Connection to ISDN-database failed.");
      syslog( LOG_ERR, "%s", PQerrorMessage( db_Conn));
      _PQfinish();
      return( -1);
    }

  PQclear( res);


  /* end the transaction */
  res = PQexec( db_Conn, "END");

  if ( PQresultStatus( res) != PGRES_COMMAND_OK)
    {
      syslog( LOG_ERR, "%s", "Connection to ISDN-database failed.");
      syslog( LOG_ERR, "%s", PQerrorMessage( db_Conn));
      _PQfinish();
      return( -1);
    }

  PQclear(res);

  return( 0);
}

int dbStatus( void)
{
  if ( PQstatus( db_Conn) == CONNECTION_BAD)
    return( -1);
  else
    return( 0);
}
#endif
