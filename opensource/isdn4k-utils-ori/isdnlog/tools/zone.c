/* $Id: zone.c,v 1.22 2001/10/15 11:35:46 leo Exp $
 *
 * Zonenberechnung
 *
 * Copyright 1999 by Leopold Toetsch <lt@toetsch.at>
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
 * $Log: zone.c,v $
 * Revision 1.22  2001/10/15 11:35:46  leo
 * fixed cdb zonefiles
 *
 * Revision 1.21  2001/06/12 14:24:17  paul
 * zone.c and mkzonedb.c now understand filename "-" to mean stdin.
 *
 * Revision 1.20  2001/06/12 13:54:47  paul
 * zone files are now created byte-order independent, so:
 * - creating the zone files works on sparc
 * - CDB files created on e.g. intel can be used without problem om sparc
 * Our copy of CDB has also been modified for this.
 *
 * Revision 1.19  1999/11/07 13:29:29  akool
 * isdnlog-3.64
 *  - new "Sonderrufnummern" handling
 *
 * Revision 1.18  1999/10/25 18:30:04  akool
 * isdnlog-3.57
 *   WARNING: Experimental version!
 *   	   Please use isdnlog-3.56 for production systems!
 *
 * Revision 1.17  1999/10/22 19:57:59  akool
 * isdnlog-3.56 (for Karsten)
 *
 * Revision 1.16  1999/07/31 09:25:49  akool
 * getRate() speedup
 *
 * Revision 1.15  1999/07/26 16:28:51  akool
 * getRate() speedup from Leo
 *
 * Revision 1.14  1999/07/25 15:58:13  akool
 * isdnlog-3.43
 *   added "telnum" module
 *
 * Revision 1.13  1999/07/10 21:38:54  akool
 * isdnlog-3.41
 *   rate-de.dat V:1.02-Germany [10-Jul-1999 23:32:27]
 *   country-de.dat V:1.02-Germany [10-Jul-1999 23:32:36]
 *   added all "zone-*" files in binary mode
 *
 * Revision 1.12  1999/07/07 19:44:20  akool
 * patches from Michael and Leo
 *
 * Revision 1.11  1999/07/01 20:44:07  akool
 * zone-1.12
 *
 * Revision 1.10  1999/06/29 20:11:45  akool
 * now compiles with ndbm
 * (many thanks to Nima <nima_ghasseminejad@public.uni-hamburg.de>)
 *
 * Revision 1.9  1999/06/26 12:25:54  akool
 * isdnlog Version 3.37
 *   fixed some warnings
 *
 * Revision 1.8  1999/06/26 10:12:14  akool
 * isdnlog Version 3.36
 *  - EGCS 1.1.2 bug correction from Nima <nima_ghasseminejad@public.uni-hamburg.de>
 *  - zone-1.11
 *
 * Revision 1.7  1999/06/22 19:41:28  akool
 * zone-1.1 fixes
 *
 * Revision 1.6  1999/06/22 16:31:15  akool
 * zone-1.10
 *
 * Revision 1.4  1999/06/18 12:41:57  akool
 * zone V1.0
 *
 * Revision 1.3  1999/06/15 20:05:25  akool
 * isdnlog Version 3.33
 *   - big step in using the new zone files
 *   - *This*is*not*a*production*ready*isdnlog*!!
 *   - Maybe the last release before the I4L meeting in Nuernberg
 *
 * Revision 1.2  1999/06/09 20:58:09  akool
 * CVS-Tags added
 *
 *
 * Interface:
 *
 * int initZone(int provider, char *path, char **msg)
 *	initialize returns -1 on error, 0 if ok
 *
 * void exitZone(int provider)
 *   deinitialize
 *
 * int getZone(int provider, char *from, char *to)
 *	returns zone for provider, UNKNOWN on not found, -2 on error
 *
 * Changes:
 *
 * 1.24 1999.10.06 lt removed getAreacode
 *
 * 1.23 1999.10... lt switch to getDest
 *
 * 1.22 1999.07.26 lt bug fix, getZone returned junk, when diff. providers
 *                    used the same zone file
 *
 * 1.21 1999.07.22 lt fixed bug, were T was overwritten, when an 'A'
 *                    followed versio, occured w. DTAG 		
 *
 * 1.20 1999.07.08 lt added support for NL
 *
 *      in NL areacode may be shorter than actual aeracodenumber
 *      in this case \tLEN is appended to text
 *	this version reads also datafiles V1.1
 *
 */

#define _ZONE_C_

#ifdef STANDALONE
#include <stdlib.h>
#include <stdio.h>
#ifdef __GLIBC__
# define __USE_GNU  /* for declaration of basename() */
#endif
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#if !defined(__GLIBC__) && !defined(basename)
extern const char *basename (const char *name);
#endif
#else
#include "isdnlog.h"
#include "tools.h"
#endif

#include "zone.h"
/* this config (from config.in) could go in global policy */
#include "zone/config.h"
#include "zone/common.h"
#include "zone/upack.h"
#include <dirent.h>

struct sth {
	_DB fh;
	char *path;
	int provider;
	int used;
	int real;
	int *table;
	char pack_key, pack_table;
	int table_size;
	int oz;
	int numlen;
	int cc;
} ;

#ifdef STANDALONE
#define min(a,b) (a) < (b) ? (a) : (b)
#endif

static struct sth *sthp;
static int count;
static char version[] = "1.25";

#define LINK 127
#define INFO_LEN 80
#define LENGTH 160
#define BUFSIZE 200

#ifdef STANDALONE
#define UNKNOWN -1
#endif

static void warning (char *file, char *fmt, ...)
{
  va_list ap;
  char msg[BUFSIZ];

  va_start (ap, fmt);
  vsnprintf (msg, BUFSIZ, fmt, ap);
  va_end (ap);
#ifdef STANDALONE
  fprintf(stderr, "WARNING: %s %s\n", basename(file), msg);
#else
  print_msg(PRT_NORMAL, "WARNING: %s %s\n", basename(file), msg);
#endif
}

static void _exitZone(int provider);

void exitZone(int provider) {
	int i;
	_exitZone(provider);
	for (i=0; i<count; i++)
		if(sthp[i].provider>=10000)
			_exitZone(sthp[i].provider);
}

static void _exitZone(int provider)
{
	int i;
	bool any = false;
	bool found = false;
	for (i=0; i<count; i++)
		if (sthp[i].provider == provider) {
			found = true;
			if (sthp[i].real >= 0)
				sthp[sthp[i].real].used--;
			else if (sthp[i].path && sthp[i].used == 0) {
				free(sthp[i].path);
				sthp[i].path = 0;
				free(sthp[i].table);
				sthp[i].table = 0;
				CLOSE(sthp[i].fh);
				sthp[i].fh = 0;
				if (i == count-1) /* last released ? */
					count--;
			}
			break;
	}
	if (!found) {
		warning(sthp[i].path ? sthp[i].path : sthp[sthp[i].real].path,
		 "ExitZone for unknown provider %d", provider);
	}
	for (i=0; i<count; i++)
		if (sthp[i].used || sthp[i].path) {
			any=true;
			break;
		}
	if (!any) {
		free(sthp);
		sthp = 0;
	}
}
#define ZONES 0
#define AREACODES 1
static int _initZone(int provider, char *path, char **msg, bool area_only);

int initZone(int provider, char *path, char **msg)
{
	static char message[LENGTH];
	int res;
	if (msg)
    	*(*msg=message)='\0';
	res = _initZone(provider, path, msg, ZONES);
	return res;
}

static int _initZone(int provider, char *path, char **msg, bool area_only)
{
	int i;
	struct sth *newsthp;
	bool found;
	int ocount;
	int csize=0, tsize=0, n;
	datum key, value;
	char *message=0;

	if (msg)
		message = *msg;
	if (!path || !*path) {
		if (msg)
			snprintf (message, LENGTH,
				"Zone V%s: Error: no zone database specified!", version);
			return -1;
	}
	ocount = count;
	if (sthp == 0) {
		if ((sthp = calloc(++count, sizeof(struct sth))) == 0) {
			if (msg)
				snprintf (message, LENGTH,
					"Zone V%s: Error: Out of mem 0", version);
			count--;
			return -1;
		}
	}
	else {
		/* look if we can reuse some space */
		found = false;
		for (i=0; i<ocount; i++)
			if (sthp[i].fh == 0) {
				ocount = i;
				found = true;
				break;
			}
		if (!found) {
			if ((newsthp = realloc(sthp, sizeof(struct sth) * ++count)) == 0) {
				if (msg)
					snprintf (message, LENGTH,
						"Zone V%s: Error: Out of mem 1", version);
				count--;
				return -1;
			}
			sthp = newsthp;
		}
	}
	sthp[ocount].path=0;
	sthp[ocount].table=0;
	sthp[ocount].used=0;
	sthp[ocount].fh=0;
	sthp[ocount].real = -1;
	sthp[ocount].oz=1;
	sthp[ocount].numlen=0;
	sthp[ocount].cc=0;
	/* now search for same path */
	found = false;
	for (i=0; i<count-1; i++) {
		if (sthp[i].path && strcmp(sthp[i].path, path) == 0) {
			sthp[ocount].fh = sthp[i].fh;
			sthp[ocount].oz = sthp[i].oz;
			sthp[ocount].numlen = sthp[i].numlen;
			sthp[ocount].pack_key = sthp[i].pack_key;
			sthp[ocount].pack_table = sthp[i].pack_table;
			sthp[ocount].table = sthp[i].table;
			sthp[ocount].cc = sthp[i].cc;
			sthp[i].used++;
			sthp[ocount].real = i;
			found = true;
			break;
		}
	}
	sthp[ocount].provider = provider;
	if (!found) {
		char vinfo[] = "vErSiO";
		char table[] = "_tAbLe";
		char dversion[6];
		char *p, *q;

		*dversion = '\0';
		if((sthp[ocount].path = strdup(path)) == 0) {
			if (msg)
				snprintf (message, LENGTH,
					"Zone V%s: Error: Out of mem 2", version);
			return -1;
		}
		if((sthp[ocount].fh = OPEN(path, READ)) == 0) {
			if (msg)
				snprintf (message, LENGTH,
					"Zone V%s: Error: gdbm_open '%s': '%s'",
					version, path, GET_ERR);
			count--;
			return -1;
		}
		/* read info */
		key.dptr = vinfo;
		key.dsize = 7;
		value = FETCH(sthp[ocount].fh, key);
		if (value.dptr == 0) {
			if (msg)
				snprintf (message, LENGTH,
					"Zone V%s: Error: Provider %d File '%s': no Vinfo",
					version, provider, path);
			exitZone(provider);
			return -1;
		}
		for (p=value.dptr; *p; p++) {
			switch (*p) {
			case 'V' :
				for (p++,n=0,q=dversion; n<6 && *p != ' '; n++)
					*q++ = *p++;
				*q = '\0';
				if (memcmp(dversion, version, 3) > 0) {
					if (msg)
						snprintf (message, LENGTH,
							"Zone V%s: Error: Provider %d File '%s': incompatible Dataversion %s",
							version, provider, path, dversion);
					exitZone(provider);
					return -1;
				}
				break;
			case 'K' :
				sthp[ocount].pack_key = *(++p);
				break;
			case 'C' :
				sthp[ocount].pack_table = *(++p);
				break;
			case 'N' :
				p++;
				csize = strtol(p, &p, 10);
				break;
			case 'T' :
				p++;
				tsize = strtol(p, &p, 10);
				break;
			case 'O' :
				p++;
				sthp[ocount].oz = strtol(p, &p, 10);
				break;
			case 'L' :
				p++;
				sthp[ocount].numlen = strtol(p, &p, 10);
				p--; /* get's incr after, so we miss 0x0*/
				break;
			}
		} /* for */
		if (*dbv == 'G')
			free (value.dptr);
		/* check it */
		if (strlen(dversion) == 0 ||
			sthp[ocount].pack_key == '\x0' ||
			strchr("SL", sthp[ocount].pack_key) == 0 ||
			sthp[ocount].pack_table == '\x0' ||
			strchr("CSL", sthp[ocount].pack_table) == 0 ||
			sthp[ocount].numlen == 0 ||
			csize == 0 ||
			tsize == 0) {
				if (msg)
					snprintf (message, LENGTH,
						"Zone V%s: Error: Provider %d File '%s' seems to be corrupted:\n%s",
						version, provider, path, value.dptr);
			exitZone(provider);
			return -1;
		}
		sthp[ocount].pack_table = sthp[ocount].pack_table == 'C' ? 1 :
				sthp[ocount].pack_table == 'S' ? 2 : 4;
		sthp[ocount].pack_key = sthp[ocount].pack_key == 'S' ? 2 : 4;

		if (area_only) {
			if (sthp[ocount].cc == 0)
				_exitZone(provider); /* discard this one */
			return 0;
		}
		/* alloc & read table */
		if ( (sthp[ocount].table = calloc(csize > 256 ? 256 : csize,
				sizeof(int)) ) == 0) {
			if (msg)
				snprintf (message, LENGTH,
					"Zone V%s: Error: Out of mem 3", version);
			exitZone(provider);
			return -1;
		}
		key.dptr = table;
		key.dsize = 7;
		value = FETCH(sthp[ocount].fh, key);
		if (value.dptr == 0) {
			if (msg)
				snprintf (message, LENGTH,
					"Zone V%s: Error: Provider %d File '%s': no NTable",
					version, provider, path);
			exitZone(provider);
			return -1;
		}
		for (p = value.dptr, i=0; i< (csize > 256 ? 256 : csize); i++)
/*
			sthp[ocount].table[i] =
				sthp[ocount].pack_table == 1 ? (int)(UC)*((UC*)p)++ :
				sthp[ocount].pack_table == 2 ? (int)(US)*((US*)p)++ :
				(int)(UL)*((UL*)p)++;
 */
	    		switch(sthp[ocount].pack_table) {
			 case 1: sthp[ocount].table[i] = (int)(UC) *p;
			  	p++;
			  	break;
			 case 2: sthp[ocount].table[i] = (int)tools_unpack16(p);
			  	p += 2;
			  	break;
			 case 4: sthp[ocount].table[i] = (int)tools_unpack32(p);
			  	p += 4;
			  	break;
			}
		if (*dbv == 'G')
			free(value.dptr);
		if (msg) {
			snprintf (message, LENGTH,
				"Zone V%s: Provider %d File '%s' opened fine - "
				"V%s K%d C%d N%d T%d O%d L%d",
				version, provider, path,
				dversion, sthp[ocount].pack_key, sthp[ocount].pack_table,
				csize, tsize, sthp[ocount].oz, sthp[ocount].numlen);
		}
	}
	else {
		if (msg)
#if 1
			*message = 0;
#else
			snprintf (message, LENGTH,
				"Zone V%s: Provider %d is open as '%s' for provider %d",
				version, provider, path, sthp[sthp[ocount].real].provider);
#endif
	}
	return 0;
}

static int _getZ(struct sth *sthp, char *from, char *sto) {
	_DB fh = sthp->fh;
	datum key, value;
	static char newfrom[LENGTH];
	bool found = false;
	char *temp;
	int res;

	if ((res=strcmp(from, sto)) == 0)
		return sthp->oz;
	else if (res > 0) {
		temp=from;
		from=sto;
		sto=temp;
	}
	strncpy(newfrom, from, LENGTH-1);
	while (strlen(newfrom)) {
		UL lifrom = (UL) atol(newfrom); /* keys could be long */
		US ifrom = (US) lifrom;
		if (sthp->pack_key == 2) {
			key.dptr = (char *) &ifrom;
			key.dsize = sizeof(US);
		}
		else {
			key.dptr = (char *) &lifrom;
			key.dsize = sizeof(UL);
		}
		value = FETCH(fh, key);
		if (value.dptr) {
			char *p = value.dptr;
			char to[10];
			US count;
			int ito;
			unsigned char z=0;
			if (sthp->cc) /* if areacodes */
				/* here is since 1.00 a zero-terminated strring */
				while (*p++);
			// count = *((US*)p)++;
			count = tools_unpack16(p); p+=2;
			while (count--) {
				bool ind = true;
				int len=1;
				z = *p++;
				if (z >= 128) {
					z -= 128;
					ind = false;
					len = sthp->pack_key;
				}
			  /*
				ito = len==1 ? (int)*((UC*)p)++ :
					  len==2 ? (int)*((US*)p)++ : (int)*((UL*)p)++;
			   */
			  	switch(len) {
				 case 1:
				  ito = (int)(UC) *p;
				  p++;
				  break;
				 case 2:
				  ito = (int)tools_unpack16(p);
				  p += 2;
				  break;
				 case 4:
				  ito = (int)tools_unpack32(p);
				  p += 4;
				  break;
				}
				if (ind)
					ito = sthp->table[ito];
				if (z == LINK) {
					sprintf(newfrom, "%d", ito);
					if (*dbv == 'G')
						free(value.dptr);
					return _getZ(sthp, newfrom, sto);
				}
				else {
					sprintf(to, "%d", ito);
					if (memcmp(to, sto, strlen(to))==0) {
						found = true;
						break;
					}
				}
			}
			if (*dbv == 'G')
				free(value.dptr);
			if (found)
				return z;
		} /* if dptr */
		newfrom[strlen(newfrom)-1] = '\0';
	}
	return UNKNOWN;
}

int getZone(int provider, char *from, char *to)
{
	int i;
	for (i=0; i<count; i++)
		if (sthp[i].provider == provider) {
			if (sthp[i].fh == 0)
				return UNKNOWN;
			return _getZ(&sthp[i], from, to);
		}
	return UNKNOWN;
}


#ifdef ZONETEST

static int checkZone(char *zf, char* df,int num1,int num2, bool verbose)
{
	char *msg;
	int z, ret=0;
	char from[10];
	char to[10];
	int cc;
	if (initZone(1, df, &msg)) {
		fprintf(stderr,"%s\n", msg);
		exit(1);
	}
	cc = sthp[0].cc;
	if(verbose)
		printf("%s\n", msg);
	if (num1 && num2) {
		snprintf(from, 9, "%d",num1);
		snprintf(to, 9, "%d",num2);
		ret = getZone(1, from, to);
		printf("%s %s = %d\n", from, to, ret);
	}
	else {
		FILE *fp;
		char line[40];
		char *p, *q;
		int rz, i, n;
		if (strcmp(zf, "-")) {	/* use stdin? */
		    if ((fp = fopen(zf, "r")) == 0) {
			fprintf(stderr, "Can't read %s\n", zf);
			exitZone(1);
			exit(1);
		    }
		}
		else {
		    fp = stdin;
		}
		n=0;
		while (!feof(fp)) {
			if((++n % 1000) == 0 && verbose) {
				fprintf(stderr,"%d\r",n);
				fflush(stderr);
			}
			if (!fgets(line, 39, fp))
				break;
			p=line;
			q=from;
			if (!isdigit(*p))
				continue;
			i=0;
			while (isdigit(*p) && ++i<9) {
				*q++ = *p++;
			}
			*q = '\0';
			p++;
			q=to;
			i=0;
			while (isdigit(*p) && ++i<9) {
				*q++ = *p++;
			}
			*q = '\0';
			p++;
			rz = atoi(p);
			if ((z=getZone(1, from, to)) != rz) {
				if(verbose)
					printf("Err: %s %s = %d not %d\n", from, to, rz, z);
				ret = -1;
				break;
			}
		}
		if (fp != stdin)
		    fclose(fp);
		if (verbose)
			printf("'%s' verified %s.\n", df, ret==0? "Ok": "NoNo");
	}
	exitZone(1);
	return ret;
}


int main (int argc, char *argv[])
{
	int verbose=false;
	char *df=0;
	char *zf=0;
	int c;
	int num1=0, num2=0;
	char snum1[LENGTH];
	while ( (c=getopt(argc, argv, "vVd:z:")) != EOF) {
		switch (c) {
			case 'v' : verbose = true; break;
			case 'V' : printf("%s: V%s\n", basename(argv[0]), version); exit(1);
			case 'd' : df = strdup(optarg); break;
			case 'z' : zf = strdup(optarg); break;
		}
	}
	while (optind < argc) {
		if (!num1 && isdigit(*argv[optind])) {
			num1 = atoi(argv[optind]);
			strncpy(snum1, argv[optind], LENGTH);
			optind++;
			continue;
		}
		else if (!num2 && isdigit(*argv[optind])) {
			num2 = atoi(argv[optind]);
			optind++;
			continue;
		}
		optind++;
	}
	if (df && (zf || (num1 && num2)))
		return checkZone(zf, df, num1, num2, verbose);
	fprintf(stderr, "Usage:\n%s -d DBfile -v -V { -z Zonefile | num1 num2 }\n", basename(argv[0]));
	return 0;
}
#endif
