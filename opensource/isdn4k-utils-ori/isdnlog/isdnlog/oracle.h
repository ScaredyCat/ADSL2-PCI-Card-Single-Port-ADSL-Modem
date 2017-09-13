/*
 * Copyright (C) 1999 Jan Bolt
 *
 * Permission to use, copy and distribute this software for
 * non-commercial purposes is hereby granted without fee,
 * provided that this copyright and permission notice appears
 * in all copies.   
 *
 * This software is provided "as-is", without ANY WARRANTY.
 *
 * oracle.h 1999/01/07 Jan Bolt
 *
 * $Log: oracle.h,v $
 * Revision 1.2  2002/03/11 16:17:10  paul
 * DM -> EUR
 *
 * Revision 1.1  1999/12/31 13:30:02  akool
 * isdnlog-4.00 (Millenium-Edition)
 *  - Oracle support added by Jan Bolt (Jan.Bolt@t-online.de)
 *
 */

#ifndef __ORACLE_H
#define __ORACLE_H

static const char oracle_h[] = "$Id: oracle.h,v 1.2 2002/03/11 16:17:10 paul Exp $";

typedef struct
{
  time_t  connect;            /* Zeitpunkt des Verbindungsaufbaues */
  char    calling[31];        /* Telefonnummer des Anrufers */
  char    called[31];         /* Telefonnummer des Gegners */
  int     duration;           /* Dauer der Verbindung in Sekunden */
  int     hduration;          /* Dauer der Verbindung in 1/100 Sekunden */
  int     aoce;               /* Anzahl Gebuehreneinheiten (AOC-D) */
  char    dialin[2];          /* "I" incoming call, "O" outgoing call */
  int     cause;              /* Status der Verbindung */
  long    ibytes;             /* uebertragene Byte incoming */
  long    obytes;             /* uebertragenen Byte outgoing */
  char    version[6];         /* Versionsnummer dieses Eintrages */
  int     si1;                /* Dienstkennung (1=Speech, 7=Data usw.) */
  int     si11;               /* analog oder digital ? */
  double  currency_factor;    /* Currency Factor (0,121) */
  char    currency[4];        /* Waehrung (in Europa "EUR") */
  double  pay;                /* Endbetrag in Landeswaehrung */
  int     provider;           /* Providercode */
  char    provider_name[31];  /* Provider */
  int     zone;               /* CityCall, RegioCall, ... */
} oracle_DbStrIn;

/* Oracle Connect-String */
#define DB_CONNECT_STRING "isdn/isdn"
/* where to log failed inserts */
#define DB_LOAD_ERROR "/var/log/isdn-fail.sql"

extern int oracle_dbOpen();
extern int oracle_dbClose();
extern int oracle_dbAdd(const oracle_DbStrIn *);
extern int oracle_dbStatus();

#endif
