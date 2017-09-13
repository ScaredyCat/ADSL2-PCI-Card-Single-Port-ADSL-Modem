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
 * ora_load.c 1999/01/07 Jan Bolt
 *
 * $Log: ora_load.c,v $
 * Revision 1.2  2002/03/11 16:17:10  paul
 * DM -> EUR
 *
 * Revision 1.1  1999/12/31 13:30:02  akool
 * isdnlog-4.00 (Millenium-Edition)
 *  - Oracle support added by Jan Bolt (Jan.Bolt@t-online.de)
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include "policy.h"
#include "oracle.h"

static const char rcsid[] = "$Id: ora_load.c,v 1.2 2002/03/11 16:17:10 paul Exp $";

extern const char *db_load_error;
static const char *logfile;
static int debug, rowsInserted, insErrors;

/* overwrite syslog for console output */
void syslog(int pri, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
}

static
char *trim(char *s)
{
	char *p;

	while (*s == ' ') s++;
	if (!*s) return s;
	p = s;
	while (*p) p++;
	p--;
	while (*p == ' ') *p-- = '\0';

	return s;
}

static
int isdngeb(FILE *fp)
{
	oracle_DbStrIn call;
	int row = 0, i;
	char buf[256];

	rowsInserted = 0;
	for ( ; fgets(buf, sizeof(buf), fp); )
	{
		char *p;
		row++;
		/*printf("%s", buf);*/
		memset(&call, 0, sizeof(call));
		for (i = 0, p = buf; (p = strtok(p, "|")); p = 0)
		{
			i++;
			/*printf("%d \"%s\"\n", p);*/
			switch (i)
			{
				case 1:	/* Datum (ctime(&t)+4, Nov  1 10:25:21 1998) */
					/*struct tm tm;*/
					/*strptime(p, "%b %d %H:%M:%S %Y", &tm);*/
					/*call.connect = mktime(&tm);*/
					break;
				case 2: /* Anrufer */
					strncpy(call.calling, trim(p), sizeof(call.calling));
					call.calling[sizeof(call.calling)-1] = '\0';
					break;
				case 3: /* Angerufener */
					strncpy(call.called, trim(p), sizeof(call.called));
					call.called[sizeof(call.called)-1] = '\0';
					break;
				case 4: /* Dauer der Verbindung in Sekunden */
					call.duration = atol(p);
					break;
				case 5: /* Dauer der Verbindung in 1/100 Sekunden */
					call.hduration = atol(p);
					break;
				case 6: /* Zeitpunkt des Verbindungsaufbaues in UTC */
					call.connect = atol(p);
					break;
				case 7: /* Gebuehreneinheiten (-1 == keine Meldung) */
					call.aoce = atoi(p);
					break;
				case 8: /* "I" fuer incoming call, "O" fuer outgoing call */
					call.dialin[0] = *trim(p);
					call.dialin[1] = '\0';
					break;
				case 9: /* Status Verbindungsende */
					call.cause = atoi(p);
					break;
				case 10: /* Summe der uebertragenen Byte (incoming) */
					call.ibytes = atol(p);
					break;
				case 11: /* Summe der uebertragenen Byte (outgoing) */
					call.obytes = atol(p);
					break;
				case 12: /* Versionsnummer der "isdn.log" Eintragung */
					strncpy(call.version, trim(p), sizeof(call.version));
					call.version[sizeof(call.version)-1] = '\0';
					break;
				case 13: /* Dienstkennung (1=Speech, 7=Data usw.) */
					call.si1 = atoi(p);
					break;
				case 14: /* 0 = ISDN, 1 = analog */
					call.si11 = atoi(p);
					break;
				case 15: /* Currency Factor (0.121) */
					call.currency_factor = atof(p);
					break;
				case 16: /* Waehrung (EUR) */
					strncpy(call.currency, trim(p), sizeof(call.currency));
					call.currency[sizeof(call.currency)-1] = '\0';
					break;
				case 17: /* Gebuehrensumme (xx.xx) */
					call.pay = atof(p);
					break;
				case 18: /* Providercode (ab 3.1) */
					call.provider = atoi(p);
					break;
				case 19: /* Zone (ab 3.2) */
					call.zone = atoi(p);
					break;
			}
		}
		if (i >= 17)
		{
			if (!oracle_dbAdd(&call))
				rowsInserted++;
			else
				insErrors++;
		}
		else
		{
			fprintf(stderr, "%s: #%d: corrupted record.\n", logfile, row);
			insErrors++;
		}
	}
	
	return row;
}

/*********************************************************************/

int main(int argc, const char *argv[])
{
	int i, rows;
	FILE *fp;

	for (i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "--help"))
		{
			fprintf(stderr, "Usage: %s [ -debug ] [ logfile ]\n",
				basename(argv[0]));
			return 0;
		}
		else
		if (!strcmp(argv[i], "-debug"))
			debug = 1;
		else
		if (!logfile)
			logfile = argv[i];
	}

	db_load_error = "ora_load.sql";
	unlink(db_load_error);
	if (!logfile) logfile = LOGFILE; /*"/var/log/isdn.log"*/
	if (!(fp = fopen(logfile, "r")))
	{
		perror(logfile);
		return 1;
	}

	if (oracle_dbOpen()) return 1;
	rows = isdngeb(fp);
	fclose(fp);
	oracle_dbClose();
	fprintf(stderr, "%s: %d rows, %d inserted, %d errors\n",
		logfile, rows, rowsInserted, insErrors);
	if (insErrors)
		fprintf(stderr, "look at %s for loader errors\n", db_load_error);

	return insErrors;
}
