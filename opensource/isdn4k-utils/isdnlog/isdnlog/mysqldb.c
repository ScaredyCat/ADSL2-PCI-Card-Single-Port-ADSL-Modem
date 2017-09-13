/* $Id: mysqldb.c,v 1.2 2000/04/02 17:35:07 akool Exp $
 *
 * Interface for mySQL-Database for isdn4linux. (db-module)
 *
 * by Sascha Matzke (sascha@bespin.escape.de)
 * based on postgres.c by Markus Leist (markus@hal.dirnet.com)
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
 * $Log: mysqldb.c,v $
 * Revision 1.2  2000/04/02 17:35:07  akool
 * isdnlog-4.18
 *  - isdnlog/isdnlog/isdnlog.8.in  ... documented hup3
 *  - isdnlog/tools/dest.c ... _DEMD1 not recogniced as key
 *  - mySQL Server version 3.22.27 support
 *  - new rates
 *
 * Revision 1.1  1998/04/06 15:45:18  keil
 * Added missing files
 *
 * Revision 0.2  1998/02/25 12:10.00  matzke
 * Fixed date bug
 *
 * Revision 0.1  1998/02/25 09:45.00  matzke
 * Initial revision
 *
 */


#ifdef MYSQLDB
#define _MYSQL_C_

#include "mysqldb.h"

MYSQL mysql;

int mysql_dbOpen(void)
{

  /* get strings from macros */

  mysql_db_Host = NULL;
  if ( DB_HOST )
    {
      mysql_db_Host = malloc( strlen( DB_HOST));
      strcpy( mysql_db_Host, DB_HOST);
    }

  mysql_db_User = NULL;
  if ( DB_USER )
    {
      mysql_db_User = malloc( strlen( DB_USER));
      strcpy( mysql_db_User, DB_USER);
    }

  mysql_db_Passwd = NULL;
  if ( DB_PASSWD )
    { 
      mysql_db_Passwd = malloc( strlen( DB_PASSWD));
      strcpy( mysql_db_Passwd, DB_PASSWD);
    }

  mysql_db_Name = NULL;
  if ( DB_NAME )
    {
      mysql_db_Name = malloc( strlen( DB_NAME));
      strcpy( mysql_db_Name, DB_NAME);
    }

  mysql_db_Table = NULL;
  if ( DB_TABLE )
    {
      mysql_db_Table = malloc( strlen( DB_TABLE));
      strcpy( mysql_db_Table, DB_TABLE);
    }

  /* make a connection to the database daemon */

  if (!(mysql_connect(&mysql, mysql_db_Host, mysql_db_User, mysql_db_Passwd)))
    {
      syslog( LOG_ERR, "%s", "Connection to mySQL failed.");
      syslog( LOG_ERR, "%s", mysql_error( &mysql ));
      return( -1);
    }

  /* select database */

  if (mysql_select_db(&mysql, mysql_db_Name))
    {
      syslog( LOG_ERR, "%s", "Databaseselection failed.");
      syslog( LOG_ERR, "%s", mysql_error( &mysql ));
      return( -1);
    }

  return( 0);
}

int mysql_dbClose(void)
{
  mysql_close(&mysql);

  if ( mysql_db_Host)
    free( mysql_db_Host);

  if ( mysql_db_User)
    free( mysql_db_User);

  if ( mysql_db_Passwd)
    free( mysql_db_User);

  if ( mysql_db_Name)
    free( mysql_db_Name);

  if ( mysql_db_Table)
    free( mysql_db_Table);

  return( 0);
}

int mysql_dbAdd( mysql_DbStrIn *in)
{
  MYSQL_RES    *res;
  char         out_txt[400];
  struct tm   *tm;

  /* assert( (int)in); */

  if ( mysql_dbStatus() )   /* returns -1 when not open */
    if ( mysql_dbOpen() )   /* returns -1 when error appears */
      return( -1);

  tm = localtime( (__const time_t *) &(in->connect));

  sprintf( out_txt
, "INSERT INTO %s "
           "( sdate, stime, calling, called, charge, dir, "
           "  in_bytes, out_bytes, msec, sec, status, service, "
          "  source, vrsion, factor, currency, pay, provider) values "
           "( \'%d-%02d-%02d\', \'%02d:%02d:%02d\', "
           "\'%s\', \'%s\',  %d, \'%c\', "
           "%ld, %ld, %d, %d, %d, %d, "
          "%d, %d, %5.4f, \'%s\', %5.4f, \'%s\')",
          mysql_db_Table,
           tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
           tm->tm_hour, tm->tm_min, tm->tm_sec,
           in->calling, in->called,
           in->aoce, in->dialin,
           in->ibytes, in->obytes,
           in->hduration, in->duration,
           in->cause, in->si1, in->si11,
          in->version, in->currency_factor,
          in->currency, in->pay, in->provider);

  /* execute query */

  mysql_query(&mysql, out_txt);

  /*
    {
      syslog( LOG_ERR, "%s", mysql_error( &mysql ));
      return( -1);
    }
  */

  res = mysql_store_result(&mysql);

  /*
    {
      syslog( LOG_ERR, "%s", mysql_error( &mysql ));
      return( -1);
    }
  */
  
  mysql_free_result(res);

  return( 0);
}

int mysql_dbStatus( void)
{
  if ( &mysql != NULL)
    return( 0);
  else
    return( -1);
}
#endif

